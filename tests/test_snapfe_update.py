"""Check for Updates end to end, against a release published to a local folder.

    python3 tests/test_snapfe_update.py
"""
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import unittest
import zipfile

REPO = Path(__file__).resolve().parent.parent
UPDATER = REPO / 'snapfe_update.py'
HOOK = '#!/bin/bash\n# Snap FE frontend hook for Knulli.\n'


def make_zip(path, version, files, stamp=None, stored=False):
    with zipfile.ZipFile(path, 'w', zipfile.ZIP_STORED if stored else zipfile.ZIP_DEFLATED) as z:
        z.writestr('system/snapos/VERSION', f'Snap FE Alpha Build {stamp or version}\nbuilt now\n')
        for name, data in files.items():
            info = zipfile.ZipInfo(name)
            executable = name.endswith(('.sh', '.py', 'snapos_ui'))
            info.external_attr = (0o755 if executable else 0o644) << 16
            info.compress_type = zipfile.ZIP_STORED if stored else zipfile.ZIP_DEFLATED
            z.writestr(info, data)


class UpdaterTests(unittest.TestCase):
    def setUp(self):
        self.tmp = Path(tempfile.mkdtemp(prefix='snapfe-ota-'))
        self.root = self.tmp / 'userdata'
        app = self.root / 'system/snapos'
        (app / 'assets').mkdir(parents=True)
        (app / 'config').mkdir()
        (app / 'snapos_ui').write_bytes(b'old binary')
        (app / 'VERSION').write_text('Snap FE Alpha Build 1.3.1\n')
        (app / 'settings.cfg').write_text('sys_volume_pct=40\n')
        (app / 'config' / 'favorites').write_text('mine\n')
        (app / 'assets' / 'same.png').write_bytes(b'same art')
        (app / 'snapos-custom.sh').write_text(HOOK + 'old hook\n')
        (self.root / 'system/custom.sh').write_text(HOOK + 'old hook\n')
        (self.root / 'roms/ports').mkdir(parents=True)
        (self.root / 'roms/ports/Snap FE.sh').write_text('old launcher\n')
        (self.root / 'roms/gba').mkdir(parents=True)
        (self.root / 'roms/gba/game.gba').write_bytes(b'rom')
        self.status = self.tmp / 'status'
        self.release = self.tmp / 'release'
        self.release.mkdir()

    def tearDown(self):
        shutil.rmtree(self.tmp, ignore_errors=True)

    def files_132(self):
        return {
            'system/snapos/snapos_ui': b'new binary',
            'system/snapos/assets/same.png': b'same art',
            'system/snapos/assets/new.png': b'new art',
            'system/snapos/snapos-custom.sh': HOOK + 'new hook\n',
            'roms/ports/Snap FE.sh': 'new launcher\n',
            # Never written, whatever a package says:
            'system/snapos/settings.cfg': 'sys_volume_pct=100\n',
            'system/snapos/config/favorites': 'replaced\n',
            'roms/gba/game.gba': b'not a rom of ours',
            'INSTALL.txt': 'for computers\n',
            '../evil.txt': 'escape\n',
        }

    def publish(self, version, files, sha=None, stamp=None, corrupt=False):
        archive = self.release / f'SnapFE-Alpha-{version}.zip'
        make_zip(archive, version, files, stamp=stamp, stored=corrupt)
        if corrupt:
            data = archive.read_bytes().replace(b'new binary', b'new bXnary')
            archive.write_bytes(data)
        data = archive.read_bytes()
        info = {'format': 1, 'version': version, 'zip': archive.name, 'url': archive.as_uri(),
                'sha256': sha or hashlib.sha256(data).hexdigest(), 'size': len(data),
                'notes': f'Snap FE Alpha {version}\n\nNEW\n- Things got better\n'}
        (self.release / 'latest.json').write_text(json.dumps(info))

    def run_updater(self, *args, url=None):
        env = dict(os.environ, SNAPFE_UPDATE_ROOT=str(self.root),
                   SNAPFE_UPDATE_URL=url or (self.release / 'latest.json').as_uri())
        subprocess.run([sys.executable, str(UPDATER), *args, '--status-dir', str(self.status)],
                       env=env, timeout=120, check=False)
        lines = (self.status / 'status').read_text().splitlines()
        return dict(line.split('=', 1) for line in lines)

    def app(self, name):
        return self.root / 'system/snapos' / name

    def assert_untouched(self):
        self.assertEqual(self.app('snapos_ui').read_bytes(), b'old binary')
        self.assertEqual((self.root / 'roms/ports/Snap FE.sh').read_text(), 'old launcher\n')
        self.assertFalse(self.app('assets/new.png').exists())
        self.assertFalse((self.app('update') / 'previous').exists())

    def test_up_to_date(self):
        self.publish('1.3.1', self.files_132())
        status = self.run_updater('check', '--current', '1.3.1')
        self.assertEqual(status['state'], 'current')
        self.assertEqual(status['message'], 'Snap FE is up to date.')

    def test_newer_version_is_offered_with_its_notes(self):
        self.publish('1.3.2', self.files_132())
        status = self.run_updater('check', '--current', '1.3.1')
        self.assertEqual((status['state'], status['latest']), ('available', '1.3.2'))
        self.assertIn('Things got better', (self.status / 'notes.txt').read_text())

    def test_a_revision_is_newer(self):
        self.publish('1.3.2.1', self.files_132())
        status = self.run_updater('check', '--current', '1.3.2')
        self.assertEqual((status['state'], status['latest']), ('available', '1.3.2.1'))
        self.assertEqual(self.run_updater('check', '--current', '1.3.2.1')['state'], 'current')
        self.assertEqual(self.run_updater('check', '--current', '1.3.3')['state'], 'current')

    def test_install_then_roll_back(self):
        self.publish('1.3.2', self.files_132())
        status = self.run_updater('install', '--current', '1.3.1')
        self.assertEqual(status['state'], 'installed', status.get('message'))
        # The release's own files are in place, live boot hook included...
        self.assertEqual(self.app('snapos_ui').read_bytes(), b'new binary')
        self.assertEqual(self.app('assets/new.png').read_bytes(), b'new art')
        self.assertEqual((self.root / 'roms/ports/Snap FE.sh').read_text(), 'new launcher\n')
        self.assertIn('new hook', (self.root / 'system/custom.sh').read_text())
        # ...the player's are not, nor anything outside Snap FE's own folders.
        self.assertEqual(self.app('settings.cfg').read_text(), 'sys_volume_pct=40\n')
        self.assertEqual(self.app('config/favorites').read_text(), 'mine\n')
        self.assertEqual((self.root / 'roms/gba/game.gba').read_bytes(), b'rom')
        self.assertFalse((self.root / 'INSTALL.txt').exists())
        self.assertFalse((self.root.parent / 'evil.txt').exists())
        # Only what changed was backed up; the download and staging are gone.
        previous = self.app('update/previous')
        self.assertEqual((previous / 'files/system/snapos/snapos_ui').read_bytes(), b'old binary')
        self.assertFalse((previous / 'files/system/snapos/assets/same.png').exists())
        self.assertIn('system/snapos/assets/new.png', (previous / 'added.txt').read_text())
        self.assertEqual(self.app('update/pending').read_text().strip(), '1.3.2')
        self.assertFalse(self.app('update/download.zip').exists())
        self.assertFalse(self.app('update/staging').exists())

        status = self.run_updater('rollback')
        self.assertEqual((status['state'], status['latest']), ('rolled-back', '1.3.1'))
        self.assert_untouched()
        self.assertIn('old hook', (self.root / 'system/custom.sh').read_text())
        self.assertEqual(self.app('settings.cfg').read_text(), 'sys_volume_pct=40\n')
        self.assertFalse(self.app('update/pending').exists())

    def test_checksum_mismatch_changes_nothing(self):
        self.publish('1.3.2', self.files_132(), sha='0' * 64)
        status = self.run_updater('install', '--current', '1.3.1')
        self.assertEqual(status['state'], 'error')
        self.assertIn('checksum', status['message'])
        self.assert_untouched()

    def test_corrupt_package_changes_nothing(self):
        self.publish('1.3.2', self.files_132(), corrupt=True)
        status = self.run_updater('install', '--current', '1.3.1')
        self.assertEqual(status['state'], 'error')
        self.assertIn('could not be unpacked', status['message'])
        self.assert_untouched()

    def test_package_for_another_version_is_refused(self):
        self.publish('1.3.2', self.files_132(), stamp='1.3.3')
        status = self.run_updater('install', '--current', '1.3.1')
        self.assertIn('different version', status['message'])
        self.assert_untouched()

    def test_no_internet(self):
        status = self.run_updater('check', '--current', '1.3.1', url='http://127.0.0.1:9/latest.json')
        self.assertEqual(status['state'], 'error')
        self.assertIn('No internet connection', status['message'])

    def test_nothing_to_roll_back(self):
        status = self.run_updater('rollback')
        self.assertEqual(status['state'], 'error')
        self.assertIn('no previous version', status['message'])

    def test_boot_hook_rolls_back_a_version_that_never_starts(self):
        self.publish('1.3.2', self.files_132())
        self.assertEqual(self.run_updater('install', '--current', '1.3.1')['state'], 'installed')
        env = dict(os.environ, SNAPFE_UPDATE_ROOT=str(self.root))
        guard = ['bash', '-c', '. "$0" unit-test; update_guard', str(REPO / 'knulli/custom.sh')]
        for _ in range(2):   # two starts that never confirm
            subprocess.run(guard, env=env, check=True, timeout=30)
            self.assertEqual(self.app('snapos_ui').read_bytes(), b'new binary')
        subprocess.run(guard, env=env, check=True, timeout=30)   # the third start restores
        self.assert_untouched()
        self.assertEqual(self.app('update/auto-rollback').read_text().strip(), '1.3.2')
        self.assertFalse(self.app('update/pending').exists())


if __name__ == '__main__':
    unittest.main(verbosity=2)
