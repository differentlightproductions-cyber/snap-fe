"""Verify a Snap FE release archive against a chosen build."""
from pathlib import Path
import hashlib
import json
import re
import sys
import zipfile

repo = Path(__file__).resolve().parent.parent
if len(sys.argv) > 1:
    version = sys.argv[1]
else:
    m = re.search(r'#define SNAPFE_VERSION "Alpha Build ([0-9.]+)"',
                  (repo / 'main.c').read_text(errors='replace'))
    version = m.group(1) if m else ''
if not re.fullmatch(r'\d+\.\d+\.\d+', version):
    raise SystemExit('usage: check_release_package.py <version> [binary]')

binary = Path(sys.argv[2]).resolve() if len(sys.argv) > 2 else repo / 'snapos_ui.aarch64'
archive = repo / f'dist/SnapFE-Alpha-{version}.zip'
checksum = repo / f'dist/SHA256SUMS-{version}.txt'
background_exts = {'.png', '.jpg', '.jpeg'}
with zipfile.ZipFile(archive) as package:
    assert package.testzip() is None
    names = package.namelist()
    assert len(names) == len(set(names))
    assert package.read('system/snapos/snapos_ui') == binary.read_bytes()
    assert package.read('system/snapos/VERSION').decode().startswith(f'Snap FE Alpha Build {version}\n')
    forbidden = {'.cfg', '.dat', '.key', '.srm', '.sav', '.gba', '.gb', '.gbc',
                 '.nes', '.sfc', '.smc', '.iso', '.chd', '.bios'}
    for name in names:
        path = Path(name)
        assert not path.is_absolute() and '..' not in path.parts
        assert ':Zone.Identifier' not in name, name
        assert path.suffix.lower() not in forbidden, name
        assert not (str(path.parent) == 'system/snapos' and
                    path.name.lower().startswith(('settings.', 'battery-prompt.'))), name
        assert not name.startswith('system/snapos/config/'), name
        assert not name.startswith('roms/') or name.startswith('roms/ports/'), name
    list_icons = [name for name in names if name.startswith('system/snapos/assets/icons/list/')
                  and Path(name).suffix.lower() in {'.png', '.jpg', '.jpeg', '.svg'}]
package_backgrounds = [
    name for name in names
    if name.startswith('system/snapos/assets/backgrounds/')
    and Path(name).suffix.lower() in background_exts
]
package_background_jpgs = [
    name for name in package_backgrounds
    if Path(name).suffix.lower() in {'.jpg', '.jpeg'}
]
local_backgrounds = [
    path for path in (repo / 'assets/backgrounds').rglob('*')
    if path.is_file() and path.suffix.lower() in background_exts
]
assert len(package_backgrounds) == len(local_backgrounds)
digest = hashlib.sha256(archive.read_bytes()).hexdigest()
assert checksum.read_text().split()[0] == digest
print(json.dumps({'zip': str(archive), 'sha256': digest, 'bytes': archive.stat().st_size,
                  'entries': len(names), 'list_icons': len(list_icons),
                  'backgrounds': len(package_backgrounds),
                  'background_jpgs': len(package_background_jpgs),
                  'exact_device_binary': True, 'personal_files_excluded': True}, indent=2))
