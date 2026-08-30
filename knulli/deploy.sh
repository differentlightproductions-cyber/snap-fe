#!/usr/bin/env bash
# Deploy Snap FE to a Knulli handheld over SSH, in one connection.
# Run from anywhere; it cd's to the snapos-ui/ dir itself.
#
#   ./knulli/deploy.sh [user@ip]          (required, e.g. root@192.168.1.42)
#
# TIP: `ssh-keygen -t ed25519 -N "" -f ~/.ssh/id_ed25519 && ssh-copy-id root@<ip>`
# once, and you'll never be asked for the password again.

set -euo pipefail
cd "$(dirname "$0")/.."

DEV="${1:?usage: $0 user@device-ip   (e.g. root@192.168.1.42 -- password: linux)}"
BIN="snapos_ui.aarch64"
[[ -f "$BIN" ]] || { echo "build first:  ./build-knulli.sh --sysroot ~/knulli-sysroot" >&2; exit 1; }
[[ -d assets ]] || { echo "no assets/ dir here?" >&2; exit 1; }

# strip Windows NTFS metadata streams if any crept back in
find assets -name '*:Zone.Identifier' -delete 2>/dev/null || true

echo ">> deploying to $DEV  (one password prompt)"
EXTRA=""
[ -f scrape_boxart.py ] && EXTRA="$EXTRA scrape_boxart.py"
[ -f background_browser.py ] && EXTRA="$EXTRA background_browser.py"
[ -f ra_achievements.py ] && EXTRA="$EXTRA ra_achievements.py"
for core in gpsp_libretro.so gambatte_libretro.so; do
  [ -s "vendor/link-cores/$core" ] || {
    echo "missing vendor/link-cores/$core -- run ./knulli/setup-link-cores.sh" >&2
    exit 1
  }
done
# Personal dev convenience: if knulli/thegamesdb.key exists locally, ship it so
# you don't have to type the key on the device. Not part of a clean checkout.
KEYSEED=""
if [ -f knulli/thegamesdb.key ]; then
  cp -f knulli/thegamesdb.key ./_thegamesdb.key
  KEYSEED="_thegamesdb.key"
fi
tar czf - --exclude='*:Zone.Identifier' "$BIN" $EXTRA $KEYSEED assets \
  vendor/link-cores/gpsp_libretro.so vendor/link-cores/gambatte_libretro.so \
  -C knulli custom.sh \
| ssh "$DEV" '
  set -e
  # stop the frontend so nothing holds the old files open. Do NOT pkill on
  # "custom.sh" -- this very script mentions it and would kill our own shell.
  killall -9 snapos_ui retroarch mgba 2>/dev/null || true
  mkdir -p /userdata/system/snapos/config /userdata/roms
  # configgen needs a controller map at this path; ES normally writes it on
  # first boot, but we never let ES run. Seed it from the built-in template
  # (has the Anbernic RG34XX-SP block). Dont clobber a user-tuned one.
  mkdir -p /userdata/system/configs/emulationstation
  if [ ! -f /userdata/system/configs/emulationstation/es_input.cfg ] && \
     [ -f /usr/share/emulationstation/es_input.cfg ]; then
    cp -f /usr/share/emulationstation/es_input.cfg /userdata/system/configs/emulationstation/es_input.cfg
    echo "   seeded es_input.cfg"
  fi
  cd /userdata/system/snapos
  # Clear the old asset FILES first (keep the dirs -- fuse-exfat throws a bogus
  # "Directory not empty" on rm -rf). Otherwise a renamed/reformatted asset
  # (e.g. foo.jpg -> foo.png) leaves BOTH on the device, and the "first image in
  # the folder wins" loaders (bookshelf / list / carousel / home icons) may keep
  # showing the stale one. The tar below repopulates everything immediately.
  [ -d assets ] && find assets -type f -delete 2>/dev/null || true
  # extract straight over the top (tar replaces each file).
  # --no-same-owner/-perms because /userdata is FAT-ish.
  tar xzf - --no-same-owner --no-same-permissions
  mkdir -p cores core-backup
  cp -f vendor/link-cores/gpsp_libretro.so cores/gpsp_libretro.so
  cp -f vendor/link-cores/gambatte_libretro.so cores/gambatte_libretro.so
  [ -e core-backup/gpsp_libretro.so ] || cp -p /usr/lib/libretro/gpsp_libretro.so core-backup/gpsp_libretro.so
  [ -e core-backup/gambatte_libretro.so ] || cp -p /usr/lib/libretro/gambatte_libretro.so core-backup/gambatte_libretro.so
  cp -f cores/gpsp_libretro.so /usr/lib/libretro/gpsp_libretro.so
  cp -f cores/gambatte_libretro.so /usr/lib/libretro/gambatte_libretro.so
  sync
  mv -f snapos_ui.aarch64 snapos_ui
  if [ -f _thegamesdb.key ]; then
    if [ -s config/thegamesdb.key ]; then
      rm -f _thegamesdb.key
      echo "   kept the TheGamesDB key already on the device (set via Settings)"
    else
      mv -f _thegamesdb.key config/thegamesdb.key
      echo "   seeded TheGamesDB key"
    fi
  fi
  if [ -f /userdata/system/custom.sh ] && [ ! -f /userdata/system/custom.sh.pre-snapos ] \
     && ! grep -q "Snap FE frontend hook\|Snap OS frontend hook" /userdata/system/custom.sh; then
    cp -f /userdata/system/custom.sh /userdata/system/custom.sh.pre-snapos
    echo "   saved old custom.sh -> custom.sh.pre-snapos"
  fi
  mv -f custom.sh /userdata/system/custom.sh
  chmod 0755 snapos_ui *.py cores/*.so /usr/lib/libretro/gpsp_libretro.so \
    /usr/lib/libretro/gambatte_libretro.so /userdata/system/custom.sh 2>/dev/null || true
  echo "   installed to /userdata/system/snapos/"
  if [ -x snapos_ui ]; then echo "   exec bit: OK"; else
    echo "   NOTE: no exec bit (FAT /userdata). custom.sh runs it as: sh -c ./snapos_ui"
  fi
'
echo
echo ">> done. Reboot:  ssh $DEV reboot"
echo "   revert:        ssh $DEV \"mv -f /userdata/system/custom.sh.pre-snapos /userdata/system/custom.sh 2>/dev/null || rm -f /userdata/system/custom.sh\""
