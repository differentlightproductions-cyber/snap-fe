#!/usr/bin/env python3
"""Device-only staged install. Pipe to SSH python3; no release packaging.

Modes: preflight CHECKSUM, prepare CHECKSUM, install CHECKSUM, verify CHECKSUM.
Preserves firmware/input helpers/assets, backs up the old binary/settings on
the device, checks the binary hash, atomically swaps, restarts SNAP only.
"""
from pathlib import Path
import hashlib
import json
import os
import re
import shutil
import signal
import subprocess
import sys
import tempfile
import time

ROOT = Path('/userdata/system/snapos')
mode, expected = sys.argv[1:3]
assert re.fullmatch(r'[0-9a-f]{64}', expected), 'Invalid checksum'
assert ROOT.is_dir() and not ROOT.is_symlink(), 'SNAP directory missing or redirected'
stage = ROOT / ('.candidate-1.2.9-' + expected[:12])
assert not stage.is_symlink()


def digest(path):
    try:
        with open(path, 'rb') as f:
            h = hashlib.sha256()
            for chunk in iter(lambda: f.read(1024*1024), b''):
                h.update(chunk)
            return h.hexdigest()
    except FileNotFoundError:
        return None


EMULATOR_NAMES = {'retroarch','mgba','hatari','fs-uae','emulatorlauncher','emulatorlaunche',
                  'ppsspp','ppssppsdl','ppssppqt','duckstation','pcsx2','pcsx2-qt','flycast',
                  'mupen64plus','dolphin-emu','dolphin-emu-nogui','melonds','drastic',
                  'dosbox','dosbox-x','scummvm','yabasanshiro','yabause','mame','mame64'}


def is_emulator_process(comm, args):
    # Script launchers often have comm=python3, so inspect argv too. Match
    # executable basenames only; never export ROM paths or arbitrary arguments.
    names = {Path(arg.decode(errors='replace') if isinstance(arg,bytes) else arg).name.lower()
             for arg in args if arg}
    return comm.lower() in EMULATOR_NAMES or bool(names & EMULATOR_NAMES)


def running(proc_root=Path('/proc')):
    snaps, games = [], []
    for proc in proc_root.iterdir():
        if not proc.name.isdigit():
            continue
        try:
            args = (proc/'cmdline').read_bytes().split(b'\0')
            comm = (proc/'comm').read_text().strip()
            if is_emulator_process(comm,args):
                games.append(int(proc.name))
            if str(ROOT/'snapos_ui').encode() in args or comm == 'snapos_ui':
                snaps.append(int(proc.name))
        except (OSError, ProcessLookupError):
            pass
    return snaps, games


def display_awake_evidence(fs_root=Path('/'), run=subprocess.run):
    def rooted(path): return fs_root / path.lstrip('/')
    # This is the exact saved-brightness marker created by Knulli's dim and
    # dispoff paths and removed by undim/dispon; do not wake the device for it.
    assert not rooted('/var/run/knulli-brightness').exists(), 'Wake the handheld before updating or testing'
    readings={}
    for directory in rooted('/sys/class/backlight').glob('*'):
        for name in ('brightness','actual_brightness','bl_power'):
            path=directory/name
            if path.is_file():
                value=path.read_text().strip()
                if value: readings[str(path.relative_to(fs_root))]=int(value)
    for path,value in readings.items():
        if path.endswith('/bl_power'): assert value==0, 'Display is powered off'
    levels=[value for path,value in readings.items() if path.endswith(('/brightness','/actual_brightness'))]
    if not levels:
        # H700 devices have no /sys/class/backlight. The firmware helper's
        # get subcommand only reads display brightness and acquires no lock.
        result=run([str(rooted('/usr/bin/brightness')),'get'],capture_output=True,text=True,timeout=3)
        assert result.returncode==0 and re.fullmatch(r'\s*\d+\s*',result.stdout), 'Cannot verify display brightness'
        levels=[int(result.stdout)];readings['brightness_get']=levels[0]
    assert all(value>0 for value in levels), 'Wake the handheld; display brightness is zero'
    blank=rooted('/sys/class/graphics/fb0/blank')
    if blank.is_file():
        value=blank.read_text().strip()
        # The H700 node may have no readable value. Positive brightness plus
        # absence of the firmware sleep marker is the validated fallback.
        if value: assert int(value)==0, 'Display is blanked'
    return readings


def require_awake_idle():
    snaps,games=running()
    assert not games, 'Exit the emulator before updating or testing'
    assert len(snaps)==1, 'Expected exactly one running SNAP frontend before updating or testing'
    status=(Path('/proc')/str(snaps[0])/'status').read_text()
    state=re.search(r'^State:\s+(\S)',status,re.M)
    assert state and state[1] not in {'T','t','Z','X'}, 'Frontend is paused or exiting; wake it before updating'
    return {'running_pid':snaps[0],'awake':True,'emulator_idle':True,
            'display':display_awake_evidence()}


def preserved():
    result = {}
    for p in [ROOT/'brightness-hotkey.sh', ROOT/'volume-gate.sh',
              Path('/userdata/system/custom.sh'),
              Path('/etc/triggerhappy/triggers.d/multimedia_keys.conf')]:
        result[str(p)] = digest(p)
    # Metadata inventory: no artwork contents or personal names are exported.
    h = hashlib.sha256(); count = 0
    for base, dirs, files in os.walk(ROOT/'assets', followlinks=False):
        dirs.sort()
        for name in sorted(files):
            p=Path(base)/name; st=p.lstat();count+=1
            h.update(str(p.relative_to(ROOT)).encode());h.update(f'{st.st_size}:{st.st_mtime_ns}'.encode())
    result['assets_inventory'] = {'count':count,'sha256':h.hexdigest()}
    return result


def brightness():
    return {p.parent.name:p.read_text().strip() for p in Path('/sys/class/backlight').glob('*/brightness')}


if mode == 'preflight':
    print(json.dumps(require_awake_idle()))
elif mode == 'prepare':
    safe=require_awake_idle();snaps=[safe['running_pid']]
    stage.mkdir(mode=0o700,exist_ok=True)
    (stage/'before.json').write_text(json.dumps(preserved()))
    print(json.dumps({'stage':str(stage),'running':snaps,'brightness':brightness(),
                      'preflight':safe,
                      'previous_version':(ROOT/'VERSION').read_text().strip()}))
elif mode == 'install':
    binary=stage/'snapos_ui';assert digest(binary)==expected, 'Candidate checksum mismatch'
    safe=require_awake_idle();snaps=[safe['running_pid']]
    before=json.loads((stage/'before.json').read_text())
    assert preserved()==before, 'Assets/input scripts changed during staging; inspect before replacing'
    backup=Path(tempfile.mkdtemp(prefix='pre-1.2.9-',dir=ROOT))
    for name in ('snapos_ui','VERSION','settings.cfg','activity.dat','favorites.dat','minigames.dat',
                 'widget-places.cfg','widget-local-city.txt','battery-prompt.cfg','friends.cfg','player-id.txt','fish-helpers.cfg'):
        if (ROOT/name).is_file():shutil.copy2(ROOT/name,backup/name)
    if (ROOT/'config').is_dir():shutil.copytree(ROOT/'config',backup/'config',symlinks=True)
    (stage/'backup-path.txt').write_text(str(backup))
    # Existing files under docs are backed up too; never touch assets or helpers.
    docs=ROOT/'docs';docs.mkdir(exist_ok=True)
    for name in ('USER-FOLDERS.md','COMPUTER-CONTROLS.md','UPDATING.md',
                 'BOOT-QUOTE-SOURCES.md','boot_quotes.h','audit_system_controls.py'):
        if (docs/name).exists():shutil.copy2(docs/name,backup/name)
        shutil.copy2(stage/name,docs/name)
    os.chmod(binary,0o755)
    (stage/'VERSION').write_text('Snap FE Alpha Build 1.2.9\nbuilt '+time.strftime('%Y-%m-%dT%H:%M:%SZ',time.gmtime())+'\ndevice-test candidate; not a packaged release\n')
    os.sync()
    # Recheck immediately before the swap, after backups and docs copying.
    final_safe=require_awake_idle()
    assert final_safe['running_pid']==snaps[0], 'Frontend restarted during staging; retry the update'
    os.replace(binary,ROOT/'snapos_ui');os.replace(stage/'VERSION',ROOT/'VERSION');os.sync()
    for pid in snaps:
        try:os.kill(pid,signal.SIGTERM)
        except ProcessLookupError:pass
    print(json.dumps({'installed_sha256':expected,'backup':str(backup),'restarting':snaps}))
elif mode == 'verify':
    assert digest(ROOT/'snapos_ui')==expected
    before=json.loads((stage/'before.json').read_text());assert preserved()==before, 'Protected files changed'
    snaps,games=running();assert len(snaps)==1, 'Expected exactly one running frontend'
    # Hash the process image, not just the newly installed path.
    assert digest(Path('/proc')/str(snaps[0])/'exe')==expected, 'Old process is still running'
    print(json.dumps({'verified_sha256':expected,'running_pid':snaps[0],
                      'assets_files_preserved':before['assets_inventory']['count'],
                      'input_helpers_unchanged':True,'brightness':brightness(),
                      'backup':(stage/'backup-path.txt').read_text()}))
else:
    raise SystemExit('Unknown mode')
