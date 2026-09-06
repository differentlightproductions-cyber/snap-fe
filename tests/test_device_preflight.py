"""Pure fixture checks: no SSH, user files, or hardware changes."""
import ast
from pathlib import Path
import tempfile
from types import SimpleNamespace

source=Path(__file__).resolve().parents[1]/'tools/device_candidate.py'
tree=ast.parse(source.read_text())
names={'is_emulator_process','running','display_awake_evidence','require_awake_idle'}
nodes=[node for node in tree.body if isinstance(node,(ast.Import,ast.ImportFrom)) or
       isinstance(node,ast.FunctionDef) and node.name in names or
       isinstance(node,ast.Assign) and any(isinstance(t,ast.Name) and t.id=='EMULATOR_NAMES' for t in node.targets)]
scope={};exec(compile(ast.Module(body=nodes,type_ignores=[]),str(source),'exec'),scope)
is_game=scope['is_emulator_process'];awake=scope['display_awake_evidence']
assert is_game('python3',[b'python3',b'/usr/bin/emulatorlauncher',b'-rom',b'/roms/test.gba'])
assert is_game('PPSSPPSDL',[b'/usr/bin/PPSSPPSDL'])
assert is_game('retroarch',[b'/usr/bin/retroarch'])
assert not is_game('mpv',[b'mpv',b'/music/a song.mp3'])
assert not is_game('snapos_ui',[b'/userdata/system/snapos/snapos_ui'])

def rejected(fn):
    try:fn()
    except AssertionError:return
    raise AssertionError('Unsafe state was accepted')

with tempfile.TemporaryDirectory() as d:
    root=Path(d);(root/'var/run').mkdir(parents=True)
    fallback=lambda *a,**k:SimpleNamespace(returncode=0,stdout='89\n')
    assert awake(root,fallback)=={'brightness_get':89} # real H700 fallback
    marker=root/'var/run/knulli-brightness';marker.write_text('35\n')
    rejected(lambda:awake(root,fallback));marker.unlink()
    rejected(lambda:awake(root,lambda *a,**k:SimpleNamespace(returncode=0,stdout='0\n')))
    rejected(lambda:awake(root,lambda *a,**k:SimpleNamespace(returncode=1,stdout='')))
    blank=root/'sys/class/graphics/fb0/blank';blank.parent.mkdir(parents=True);blank.write_text('')
    assert awake(root,fallback)['brightness_get']==89 # write-only blank node
    blank.write_text('4\n');rejected(lambda:awake(root,fallback));blank.write_text('0\n')
    backlight=root/'sys/class/backlight/lcd';backlight.mkdir(parents=True)
    (backlight/'brightness').write_text('90\n');(backlight/'bl_power').write_text('0\n')
    assert awake(root)['sys/class/backlight/lcd/brightness']==90
    (backlight/'bl_power').write_text('4\n');rejected(lambda:awake(root))
print('PASS: awake/idle preflight detects Python emulator launchers and standalone games; permits audio; rejects sleep marker, blanked/off display, zero/unknown brightness; accepts verified H700 fallback')
