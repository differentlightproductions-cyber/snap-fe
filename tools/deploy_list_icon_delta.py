#!/usr/bin/env python3
"""Deploy only the verified N64 list icon, before the combined binary install.

Usage: python3 tools/deploy_list_icon_delta.py root@IP SSH_SOCKET [ICON_PATH]
No restart, settings changes, asset deletion, or unrelated artwork replacement.
"""
from pathlib import Path
import argparse
import hashlib
import ipaddress
import json
import re
import stat
import subprocess

EXPECTED_SHA256 = '86bbe59c589c4a4bfa8467ffff3c0f52a686541301b40302275da06444e106ff'
REMOTE_SCRIPT = r'''
from pathlib import Path
import hashlib,json,os,re,shutil,stat,sys,tempfile

ROOT = Path('/userdata/system/snapos')
RELATIVE = Path('icons/list/n64/n64 icon.png')
ASSETS = ROOT/'assets'
TARGET = ASSETS/RELATIVE
mode,expected=sys.argv[1:3]
assert expected=='86bbe59c589c4a4bfa8467ffff3c0f52a686541301b40302275da06444e106ff','Unexpected icon hash'
assert ROOT.is_dir() and ROOT.resolve()==ROOT and not ROOT.is_symlink(),'SNAP root missing or redirected'
assert ASSETS.is_dir() and ASSETS.resolve()==ASSETS and not ASSETS.is_symlink(),'Assets root missing or redirected'

def checked_target():
    cursor=ASSETS
    for part in RELATIVE.parts:
        cursor=cursor/part
        assert not cursor.is_symlink(),'N64 artwork path is redirected'
    assert TARGET.resolve().is_relative_to(ASSETS),'N64 target escaped assets'
    assert not TARGET.exists() or TARGET.is_file(),'N64 target is not a file'
    return TARGET

def digest(path):
    if not path.exists():return None
    h=hashlib.sha256()
    with path.open('rb') as f:
        for block in iter(lambda:f.read(1024*1024),b''):h.update(block)
    return h.hexdigest()

def metadata(path):
    st=path.lstat()
    return [st.st_size,st.st_mtime_ns,stat.S_IMODE(st.st_mode),
            os.readlink(path) if path.is_symlink() else None]

def target_info():
    checked_target()
    return {'sha256':digest(TARGET),'metadata':metadata(TARGET)} if TARGET.exists() else None

def protected():
    paths=[ROOT/'brightness-hotkey.sh',ROOT/'volume-gate.sh',
           Path('/userdata/system/custom.sh'),
           Path('/etc/triggerhappy/triggers.d/multimedia_keys.conf'),
           ROOT/'snapos_ui',ROOT/'VERSION']
    return {str(p):{'sha256':digest(p),'metadata':metadata(p) if p.exists() else None} for p in paths}

def inventory():
    other={};count=0
    for base,dirs,files in os.walk(ASSETS,followlinks=False):
        dirs.sort()
        for name in sorted(files):
            path=Path(base)/name;count+=1
            if path!=TARGET:other[str(path.relative_to(ASSETS))]=metadata(path)
        for name in dirs:
            path=Path(base)/name
            if path.is_symlink():other[str(path.relative_to(ASSETS))+'/']=metadata(path)
    return {'other':other,'count':count}

def private_stage(path):
    assert path.parent==ROOT and re.fullmatch(r'\.list-icon-stage-[A-Za-z0-9_-]+',path.name),'Invalid stage path'
    assert path.is_dir() and path.resolve()==path and not path.is_symlink(),'Stage missing or redirected'
    # Knulli SHARE can be exFAT, which exposes every directory as 0777.
    # The unique, canonical staging path and file hashes still protect this
    # non-sensitive icon transfer; do not require POSIX mode support.
    return path

checked_target()
if mode=='prepare':
    before={'target':target_info(),'protected':protected(),'assets':inventory()}
    stage=Path(tempfile.mkdtemp(prefix='.list-icon-stage-',dir=ROOT))
    private_stage(stage)
    (stage/'before.json').write_text(json.dumps(before))
    print(json.dumps({'stage':str(stage),'previous_sha256':before['target']['sha256'] if before['target'] else None,
                      'asset_files_before':before['assets']['count']}))
elif mode=='install':
    stage=private_stage(Path(sys.argv[3]));source=stage/'icon.png'
    assert source.is_file() and not source.is_symlink() and digest(source)==expected,'Staged icon checksum mismatch'
    manifest=stage/'before.json';assert manifest.is_file() and not manifest.is_symlink(),'Missing stage manifest'
    before=json.loads(manifest.read_text())
    assert target_info()==before['target'],'N64 artwork changed during staging'
    assert protected()==before['protected'],'Protected input helpers or frontend changed during staging'
    assert inventory()==before['assets'],'Other artwork changed during staging'
    previous=before['target']['sha256'] if before['target'] else None
    backup=None;temporary=None
    if previous!=expected:
        if TARGET.exists():
            backup_dir=Path(tempfile.mkdtemp(prefix='pre-list-icon-',dir=ROOT))
            assert backup_dir.parent==ROOT and backup_dir.resolve()==backup_dir
            backup=backup_dir/'assets'/RELATIVE;backup.parent.mkdir(parents=True)
            shutil.copy2(TARGET,backup)
            assert digest(backup)==previous,'Rollback copy checksum mismatch'
        (stage/'rollback.json').write_text(json.dumps({'previous_sha256':previous,'backup':str(backup) if backup else None}))
        TARGET.parent.mkdir(parents=True,exist_ok=True);checked_target()
        fd,name=tempfile.mkstemp(prefix='.n64-icon-',suffix='.tmp',dir=TARGET.parent)
        temporary=Path(name)
        try:
            with os.fdopen(fd,'wb') as f:
                f.write(source.read_bytes());f.flush();os.fsync(f.fileno())
            os.chmod(temporary,stat.S_IMODE(TARGET.stat().st_mode) if TARGET.exists() else 0o644)
            assert digest(temporary)==expected,'Temporary icon checksum mismatch'
            checked_target();assert target_info()==before['target'],'N64 artwork changed before replacement'
            os.replace(temporary,TARGET);temporary=None
            dirfd=os.open(str(TARGET.parent),os.O_RDONLY|os.O_DIRECTORY)
            try:os.fsync(dirfd)
            finally:os.close(dirfd)
        finally:
            if temporary is not None and temporary.exists():temporary.unlink()
    assert digest(TARGET)==expected,'Installed icon checksum mismatch'
    after=inventory()
    assert after['other']==before['assets']['other'],'Other artwork metadata changed'
    count_delta=after['count']-before['assets']['count']
    assert count_delta==(1 if before['target'] is None else 0),'Unexpected asset file-count change'
    assert protected()==before['protected'],'Protected input helpers or frontend changed'
    report={'target':str(TARGET),'verified_sha256':expected,'previous_sha256':previous,
            'backup':str(backup) if backup else None,'asset_files_before':before['assets']['count'],
            'asset_files_after':after['count'],'count_delta':count_delta,
            'changed_files':0 if previous==expected else 1,'other_assets_unchanged':True,
            'protected_input_helpers_unchanged':True,'frontend_restarted':False,'stage':str(stage)}
    (stage/'installed-icon.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(report))
else:raise SystemExit('Unknown mode')
'''


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('target')
    parser.add_argument('socket', type=Path)
    parser.add_argument('icon', nargs='?', type=Path,
                        default=Path(__file__).resolve().parent.parent / 'assets/icons/list/n64/n64 icon.png')
    args = parser.parse_args()
    assert args.target.startswith('root@'), 'Expected explicit root@device target'
    address = args.target[5:]
    assert str(ipaddress.IPv4Address(address)) == address, 'Expected a literal device IPv4 address'
    assert args.socket.is_absolute() and stat.S_ISSOCK(args.socket.stat().st_mode), 'Existing SSH control socket required'
    assert args.icon.is_file() and not args.icon.is_symlink(), 'Missing or redirected source icon'
    assert hashlib.sha256(args.icon.read_bytes()).hexdigest() == EXPECTED_SHA256, 'Source does not match the approved N64 icon'
    ssh = ['ssh', '-S', str(args.socket), '-o', 'BatchMode=yes', '-o', 'ConnectTimeout=6', args.target]

    def remote(mode, *extra):
        result = subprocess.run(ssh + ['python3', '-', mode, EXPECTED_SHA256, *extra],
                                input=REMOTE_SCRIPT, text=True, capture_output=True, timeout=90)
        if result.returncode:
            raise RuntimeError(f'Device icon {mode} failed: {result.stderr.strip()}')
        return json.loads(result.stdout)

    before = remote('prepare')
    stage = before['stage']
    assert re.fullmatch(r'/userdata/system/snapos/\.list-icon-stage-[A-Za-z0-9_-]+', stage), 'Unexpected remote stage path'
    subprocess.run(['scp', '-O', '-o', 'BatchMode=yes', '-o', 'ConnectTimeout=6',
                    '-o', 'ControlPath=' + str(args.socket), str(args.icon), args.target + ':' + stage + '/icon.png'],
                   check=True, timeout=45)
    result = remote('install', stage)
    result['device'] = args.target
    print(json.dumps(result, indent=2))


if __name__ == '__main__':
    main()
