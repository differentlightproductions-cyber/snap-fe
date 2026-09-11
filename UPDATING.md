# Updating Snap FE Without Losing User Data

## Update from inside Snap FE (1.3.2 and later)

Connect to Wi-Fi and open **Settings > Device > Check for Updates**. Snap FE
reads the newest release from GitHub, shows its notes, and on **A** downloads
it, checks its SHA-256, and installs only the files the release carries. Your
settings, ROMs, saves, favorites, scraped art, themes and Wi-Fi are never
touched. The files it replaces are kept, and Snap FE restarts into the new
version. If that version does not start properly, the previous one is put back
automatically; **Settings > Device > System > Roll Back Previous Update** does
the same by hand.

The steps below still work for every version, and are the way to install
Snap FE for the first time.

OTA-Hub's public example targets ESP32/PlatformIO firmware, not the ARM64 Linux
environment used by Knulli. Until a SNAP-native updater has been fully tested,
use this safe in-place update method.

## Update from an earlier Snap FE build

1. Download the newest release asset named `SnapFE-Alpha-<version>.zip`.
   Do not use GitHub's automatic Source Code archives.
2. Power the handheld completely off and insert its Knulli card into your PC.
3. Open the card's `SHARE` partition.
4. Extract the contents of the release ZIP directly onto `SHARE`. Choose
   **Merge** or **Replace** when asked. Do not delete the existing
   `SHARE/system/snapos` folder first.
5. Safely eject the card, reinstall it, and boot the handheld.
6. Open Ports and run **Snap FE (Set As Default)** once. This refreshes the boot
   hook and restarts into the updated frontend.

The release archive intentionally contains no `settings.cfg`, account keys,
favorites, activity history, ROMs, saves, save states, BIOS files, or scraped
game artwork. Extracting it over an existing installation replaces program and
bundled asset files while leaving those personal files in place.

The same ZIP installs or updates Wildkins (`SHARE/roms/ports/Wildkins.sh` and
`SHARE/roms/ports/wildkins/`). Its saves in `SHARE/saves/wildkins` are not in
the package and are kept. `WHATS-NEW.txt` lists what changed in this release.

Before updating, back up `SHARE/system/snapos/settings.cfg`, its `config/`
folder and any icons/backgrounds you replaced in the bundled `assets/` tree.
Merge means keeping the folder and replacing matching files, not deleting or
replacing the whole directory. Bundled artwork with the same filename will be
overwritten; restore your customized copies afterward. See `USER-FOLDERS.md`
for the folder guide. If an update is interrupted, extract the same release ZIP
again and rerun **Snap FE (Set As Default)**.
