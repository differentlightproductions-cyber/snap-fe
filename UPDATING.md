# Updating Snap FE Without Losing User Data

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

For extra safety, copy `SHARE/system/snapos/settings.cfg` somewhere on your PC
before updating. If an update is interrupted, extract the same release ZIP
again and rerun **Snap FE (Set As Default)**.
