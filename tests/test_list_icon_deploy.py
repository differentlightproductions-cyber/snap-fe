"""Test the icon installer only against isolated local directories; no SSH."""
from pathlib import Path
import hashlib
import importlib.util
import json
import shutil
import subprocess
import sys
import tempfile

repo = Path(__file__).resolve().parent.parent
spec = importlib.util.spec_from_file_location('icon_deploy', repo/'tools/deploy_list_icon_delta.py')
deploy = importlib.util.module_from_spec(spec)
spec.loader.exec_module(deploy)
source = repo/'assets/icons/list/n64/n64 icon.png'
assert hashlib.sha256(source.read_bytes()).hexdigest() == deploy.EXPECTED_SHA256


def fixture(base):
    root = base/'snapos'
    (root/'assets').mkdir(parents=True)
    (root/'assets/keep.png').write_bytes(b'keep-other-artwork')
    for name in ('brightness-hotkey.sh', 'volume-gate.sh', 'snapos_ui', 'VERSION'):
        (root/name).write_text('preserve '+name)
    code = deploy.REMOTE_SCRIPT.replace("ROOT = Path('/userdata/system/snapos')", f'ROOT = Path({str(root)!r})', 1)

    def run(mode, stage=None, fail=False):
        command = [sys.executable, '-c', code, mode, deploy.EXPECTED_SHA256]
        if stage: command.append(str(stage))
        result = subprocess.run(command, text=True, capture_output=True, timeout=10)
        if fail:
            assert result.returncode != 0, 'Unsafe install unexpectedly succeeded'
            return result.stderr
        assert result.returncode == 0, result.stderr
        return json.loads(result.stdout)

    def stage():
        result = run('prepare')
        directory = Path(result['stage'])
        shutil.copy2(source, directory/'icon.png')
        return directory

    return root, root/'assets/icons/list/n64/n64 icon.png', run, stage


with tempfile.TemporaryDirectory(prefix='snapfe-icon-deploy-tests-') as temporary:
    base = Path(temporary)
    root, target, run, stage = fixture(base/'normal')
    result = run('install', stage())
    assert result['count_delta'] == 1 and result['previous_sha256'] is None and result['backup'] is None
    assert result['verified_sha256'] == deploy.EXPECTED_SHA256 and not result['frontend_restarted']
    prior_mtime = target.stat().st_mtime_ns
    result = run('install', stage())
    assert result['count_delta'] == 0 and result['changed_files'] == 0 and target.stat().st_mtime_ns == prior_mtime
    target.write_bytes(b'old-user-icon')
    result = run('install', stage())
    assert result['count_delta'] == 0 and result['changed_files'] == 1
    assert Path(result['backup']).read_bytes() == b'old-user-icon'
    assert result['previous_sha256'] == hashlib.sha256(b'old-user-icon').hexdigest()

    for case in ('other-art', 'input-helper', 'target', 'bad-upload'):
        root, target, run, stage = fixture(base/case)
        directory = stage()
        if case == 'other-art': (root/'assets/keep.png').write_bytes(b'concurrent-change')
        elif case == 'input-helper': (root/'volume-gate.sh').write_text('changed-input')
        elif case == 'target':
            target.parent.mkdir(parents=True); target.write_bytes(b'newer-user-art')
        else: (directory/'icon.png').write_bytes(b'wrong-image')
        run('install', directory, fail=True)
        if case == 'target': assert target.read_bytes() == b'newer-user-art'
        else: assert not target.exists()

    root, target, run, stage = fixture(base/'redirect')
    target.parent.parent.mkdir(parents=True)
    outside = base/'outside'; outside.mkdir()
    target.parent.symlink_to(outside, target_is_directory=True)
    run('prepare', fail=True)
    assert not list(outside.iterdir())

print('PASS: icon-only atomic install, existing-icon rollback, idempotent hash, unrelated-art/input preservation, concurrent-change/hash/symlink rejection; no SSH used')
