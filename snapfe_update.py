#!/usr/bin/env python3
"""Snap FE updates from GitHub Releases -- no server of our own.

    snapfe_update.py check    --current 1.3.2 --status-dir /tmp/snapfe-update
    snapfe_update.py install  --current 1.3.2 --status-dir /tmp/snapfe-update
    snapfe_update.py rollback --status-dir /tmp/snapfe-update

Every release carries a small latest.json beside its ZIP, and GitHub serves the
newest one at a fixed address. "check" compares it with the running version.
"install" downloads the ZIP, verifies its SHA-256, unpacks the files that
changed into a staging folder, moves each file it replaces into a backup, and
puts the new one in place. Only files the release ZIP carries under
system/snapos/ and roms/ports/ are ever written -- never ROMs, saves, settings,
favorites, scraped art, Wi-Fi or anything else of the player's. "rollback"
moves the backup back.

After an install Snap FE restarts. The new version confirms it started (it
removes update/pending); custom.sh restores the backup by itself if it never
does. Progress goes to <status-dir>/status as key=value lines for the Settings
screen, and the release notes to <status-dir>/notes.txt.
"""
import argparse
import hashlib
import json
import os
import re
import shutil
import socket
import time
import urllib.error
import urllib.request
import zipfile
import zlib

LATEST_URL = ('https://github.com/differentlightproductions-cyber/snap-fe/'
              'releases/latest/download/latest.json')
ROOT = os.environ.get('SNAPFE_UPDATE_ROOT', '/userdata')
URL = os.environ.get('SNAPFE_UPDATE_URL', LATEST_URL)
ALLOWED = ('system/snapos/', 'roms/ports/')
# Release ZIPs never carry these; an update never writes them either way.
PERSONAL_DIRS = ('system/snapos/config/', 'system/snapos/update/', 'system/snapos/fastlaunch/')
PERSONAL_EXT = ('.cfg', '.dat', '.key', '.srm', '.sav', '.state', '.log', '.lpl')
HOOK_MARKERS = ('Snap FE frontend hook', 'Snap OS frontend hook')
MB = 1024 * 1024
CHUNK = 256 * 1024


class UpdateError(Exception):
    """A problem to show the player, worded for them."""


class Status:
    def __init__(self, directory):
        self.dir = directory
        os.makedirs(directory, exist_ok=True)
        self.values = {}
        self.last = 0.0

    def _write(self, name, text):
        tmp = os.path.join(self.dir, name + '.tmp')
        with open(tmp, 'w', encoding='utf-8') as f:
            f.write(text)
        os.replace(tmp, os.path.join(self.dir, name))

    def set(self, **values):
        self.values.update({k: str(v).replace('\n', ' ') for k, v in values.items()})
        self._write('status', ''.join(f'{k}={v}\n' for k, v in self.values.items()))
        self.last = time.monotonic()

    def progress(self, state, done, total, message):
        if self.values.get('state') != state or time.monotonic() - self.last >= 0.25 or done >= total:
            self.set(state=state, done=done, total=total, message=message)

    def notes(self, text):
        self._write('notes.txt', text or '')


def version_tuple(text):
    """1.3.2 < 1.3.2.1 (a revision, "Rev 1") < 1.3.3."""
    return tuple(int(n) for n in re.findall(r'\d+', text or '')[:4])


def app_dir():
    return os.path.join(ROOT, 'system', 'snapos')


def update_dir():
    return os.path.join(app_dir(), 'update')


def remove(path):
    try:
        os.remove(path)
    except OSError:
        pass


def open_url(url, timeout):
    request = urllib.request.Request(url, headers={'User-Agent': 'SnapFE-Updater',
                                                   'Cache-Control': 'no-cache'})
    return urllib.request.urlopen(request, timeout=timeout)


def fetch_latest():
    try:
        with open_url(URL, 20) as reply:
            raw = reply.read(65537)
    except urllib.error.HTTPError as error:
        if error.code == 404:
            raise UpdateError('No update information was found on GitHub yet.')
        raise UpdateError(f'GitHub answered with an error ({error.code}). Try again later.')
    except (urllib.error.URLError, socket.timeout, OSError):
        raise UpdateError('No internet connection. Connect to Wi-Fi and try again.')
    # Only GitHub's HTTPS addresses in real use; a file:// release is for tests.
    schemes = ('https://', 'file://') if URL.startswith('file://') else ('https://',)
    try:
        info = json.loads(raw.decode('utf-8')) if len(raw) <= 65536 else None
        valid = (isinstance(info, dict) and isinstance(info.get('version'), str)
                 and version_tuple(info['version'])
                 and isinstance(info.get('url'), str) and info['url'].startswith(schemes)
                 and re.fullmatch(r'[0-9a-fA-F]{64}', str(info.get('sha256', '')))
                 and isinstance(info.get('size'), int) and info['size'] > 0)
    except ValueError:
        valid = False
    if not valid:
        raise UpdateError('The update information from GitHub is not valid.')
    return info


def check(current, status):
    status.set(state='checking', message='Checking for updates...')
    info = fetch_latest()
    if version_tuple(info['version']) <= version_tuple(current):
        status.set(state='current', latest=info['version'], message='Snap FE is up to date.')
    else:
        status.notes(info.get('notes', ''))
        status.set(state='available', latest=info['version'], size=info['size'],
                   message=f"Snap FE {info['version']} is available (you have {current}).")
    return info


def download(info, folder, status):
    part = os.path.join(folder, 'download.part')
    final = os.path.join(folder, 'download.zip')
    total, done, digest = info['size'], 0, hashlib.sha256()
    label = f"Downloading Snap FE {info['version']}..."
    status.progress('downloading', 0, total, label)
    try:
        with open_url(info['url'], 30) as reply, open(part, 'wb') as out:
            while True:
                chunk = reply.read(CHUNK)
                if not chunk:
                    break
                done += len(chunk)
                if done > total:
                    raise UpdateError('The download was larger than expected, so it was not installed. '
                                      'Nothing was changed.')
                out.write(chunk)
                digest.update(chunk)
                status.progress('downloading', done, total, label)
    except UpdateError:
        remove(part)
        raise
    except urllib.error.HTTPError as error:
        remove(part)
        raise UpdateError(f'The download failed (GitHub error {error.code}). Nothing was changed.')
    except (urllib.error.URLError, OSError) as error:
        remove(part)
        reason = getattr(error, 'reason', None) or getattr(error, 'strerror', None) or error
        raise UpdateError(f'The download stopped ({reason}). Check Wi-Fi and free space, then try again. '
                          'Nothing was changed.')
    if done != total:
        remove(part)
        raise UpdateError('The download was incomplete. Check Wi-Fi and try again. Nothing was changed.')
    status.set(state='verifying', message='Checking the download...')
    if digest.hexdigest() != info['sha256'].lower():
        remove(part)
        raise UpdateError("The download didn't pass its checksum check, so it was not installed. "
                          'Try again. Nothing was changed.')
    os.replace(part, final)
    return final


def member_ok(name):
    if name.startswith('/') or '\\' in name or not name.startswith(ALLOWED):
        return False
    if any(part in ('', '.', '..') for part in name.split('/')):
        return False
    lower = name.lower()
    return not (lower.startswith(PERSONAL_DIRS) or lower.endswith(PERSONAL_EXT))


def crc_of(path):
    crc = 0
    with open(path, 'rb') as f:
        for block in iter(lambda: f.read(CHUNK), b''):
            crc = zlib.crc32(block, crc)
    return crc & 0xffffffff


def unchanged(dest, entry):
    try:
        return (os.path.isfile(dest) and not os.path.islink(dest)
                and os.path.getsize(dest) == entry.file_size and crc_of(dest) == entry.CRC)
    except OSError:
        return False


def stage(zip_path, staging, version, status):
    """Unpack the files that differ from what is installed. Returns their paths."""
    try:
        with zipfile.ZipFile(zip_path) as archive:
            entries = [e for e in archive.infolist() if not e.is_dir()]
            names = {e.filename for e in entries}
            if 'system/snapos/snapos_ui' not in names or 'system/snapos/VERSION' not in names:
                raise UpdateError('This update package does not contain Snap FE. Nothing was changed.')
            stamp = archive.read('system/snapos/VERSION').decode('utf-8', 'replace')
            if not stamp.startswith(f'Snap FE Alpha Build {version}\n'):
                raise UpdateError('This update package is for a different version. Nothing was changed.')
            files = [e for e in entries if member_ok(e.filename)]
            total = sum(e.file_size for e in files) or 1
            done, changed = 0, []
            for entry in files:
                if not unchanged(os.path.join(ROOT, entry.filename), entry):
                    target = os.path.join(staging, entry.filename)
                    os.makedirs(os.path.dirname(target), exist_ok=True)
                    with archive.open(entry) as source, open(target, 'wb') as out:
                        shutil.copyfileobj(source, out, CHUNK)   # zipfile checks each CRC
                    if (entry.external_attr >> 16) & 0o111:
                        try:
                            os.chmod(target, 0o755)
                        except OSError:
                            pass
                    changed.append(entry.filename)
                done += entry.file_size
                status.progress('extracting', done, total, 'Unpacking the update...')
            return changed
    except UpdateError:
        raise
    except (zipfile.BadZipFile, zlib.error, EOFError, KeyError, OSError) as error:
        reason = getattr(error, 'strerror', None) or error
        raise UpdateError(f'The update could not be unpacked ({reason}). Nothing was changed.')


def is_snap_hook(path):
    try:
        with open(path, 'r', encoding='utf-8', errors='replace') as f:
            text = f.read(65536)
    except OSError:
        return False
    return any(marker in text for marker in HOOK_MARKERS)


def put(rel, source, backup_root, added, moved):
    """Move what is installed at rel into the backup, then the new file into place.
    Both are renames on the same card: instant, and exactly reversible."""
    dest = os.path.join(ROOT, rel)
    os.makedirs(os.path.dirname(dest), exist_ok=True)
    if os.path.lexists(dest):
        backup = os.path.join(backup_root, 'files', rel)
        os.makedirs(os.path.dirname(backup), exist_ok=True)
        os.replace(dest, backup)
        moved.append((dest, backup))
    else:
        added.write(rel + '\n')
        added.flush()
        moved.append((dest, None))
    os.replace(source, dest)


def install_files(changed, staging, current, version, status):
    folder = update_dir()
    fresh = os.path.join(folder, 'previous.new')
    shutil.rmtree(fresh, ignore_errors=True)
    os.makedirs(os.path.join(fresh, 'files'))
    with open(os.path.join(fresh, 'backup.json'), 'w') as f:
        json.dump({'version': current, 'to': version, 'created': int(time.time())}, f)
    added = open(os.path.join(fresh, 'added.txt'), 'w')
    moved = []
    try:
        for i, rel in enumerate(changed, 1):
            put(rel, os.path.join(staging, rel), fresh, added, moved)
            status.progress('installing', i, len(changed) + 1, 'Installing...')
        # The live boot hook follows the new staged copy, when it is Snap FE's own.
        hook = os.path.join(ROOT, 'system', 'custom.sh')
        if 'system/snapos/snapos-custom.sh' in changed and is_snap_hook(hook):
            tmp = hook + '.snapfe-new'
            shutil.copyfile(os.path.join(ROOT, 'system/snapos/snapos-custom.sh'), tmp)
            try:
                os.chmod(tmp, 0o755)
            except OSError:
                pass
            put('system/custom.sh', tmp, fresh, added, moved)
        added.close()
        previous = os.path.join(folder, 'previous')
        shutil.rmtree(previous, ignore_errors=True)
        os.replace(fresh, previous)
        status.progress('installing', 1, 1, 'Installing...')
    except Exception as error:
        added.close()
        for installed, backup in reversed(moved):
            try:
                if backup:
                    os.replace(backup, installed)
                elif os.path.lexists(installed):
                    os.remove(installed)
            except OSError:
                pass
        shutil.rmtree(fresh, ignore_errors=True)
        reason = getattr(error, 'strerror', None) or error
        raise UpdateError(f'The update could not be installed ({reason}), so the current version was kept.')


def install(current, status):
    info = check(current, status)
    if version_tuple(info['version']) <= version_tuple(current):
        return
    version, folder = info['version'], update_dir()
    os.makedirs(folder, exist_ok=True)
    free = shutil.disk_usage(folder).free
    need = info['size'] * 2 + 64 * MB
    if free < need:
        raise UpdateError(f'Not enough free space on the SD card: {need // MB} MB needed, '
                          f'{free // MB} MB free. Nothing was changed.')
    staging = os.path.join(folder, 'staging')
    shutil.rmtree(staging, ignore_errors=True)
    zip_path = download(info, folder, status)
    try:
        changed = stage(zip_path, staging, version, status)
        install_files(changed, staging, current, version, status)
    finally:
        remove(zip_path)
        shutil.rmtree(staging, ignore_errors=True)
    with open(os.path.join(folder, 'pending'), 'w') as f:
        f.write(version + '\n')
    remove(os.path.join(folder, 'tries'))
    status.set(state='installed', latest=version, message=f'Snap FE {version} is installed. Restarting...')


def restore_previous():
    """Put back what the last update replaced, and remove what it added."""
    previous = os.path.join(update_dir(), 'previous')
    try:
        with open(os.path.join(previous, 'backup.json')) as f:
            info = json.load(f)
    except (OSError, ValueError):
        raise UpdateError('There is no previous version to go back to.')
    added = os.path.join(previous, 'added.txt')
    if os.path.isfile(added):
        with open(added) as f:
            for rel in f.read().splitlines():
                if rel and not rel.startswith('/') and '..' not in rel.split('/'):
                    remove(os.path.join(ROOT, rel))
    files = os.path.join(previous, 'files')
    for base, _dirs, names in os.walk(files):
        for name in names:
            source = os.path.join(base, name)
            dest = os.path.join(ROOT, os.path.relpath(source, files))
            os.makedirs(os.path.dirname(dest), exist_ok=True)
            os.replace(source, dest)
    shutil.rmtree(previous, ignore_errors=True)
    for name in ('pending', 'tries'):
        remove(os.path.join(update_dir(), name))
    return info.get('version') or 'the previous version'


def rollback(status):
    status.set(state='rolling-back', message='Restoring the previous version...')
    try:
        version = restore_previous()
    except UpdateError:
        raise
    except OSError as error:
        raise UpdateError(f'The previous version could not be restored ({error.strerror or error}).')
    status.set(state='rolled-back', latest=version, message=f'Snap FE {version} is back. Restarting...')


def main(argv=None):
    parser = argparse.ArgumentParser(description='Snap FE updates from GitHub Releases.')
    parser.add_argument('action', choices=['check', 'install', 'rollback'])
    parser.add_argument('--current', default='0')
    parser.add_argument('--status-dir', default='/tmp/snapfe-update')
    args = parser.parse_args(argv)
    status = Status(args.status_dir)
    try:
        if args.action == 'check':
            check(args.current, status)
        elif args.action == 'install':
            install(args.current, status)
        else:
            rollback(status)
        return 0
    except UpdateError as error:
        status.set(state='error', message=str(error))
    except Exception as error:   # never leave the Settings screen waiting
        status.set(state='error', message=f'The updater hit an unexpected problem ({error}).')
    return 1


if __name__ == '__main__':
    raise SystemExit(main())
