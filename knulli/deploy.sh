#!/usr/bin/env bash
# Deploy Snap FE to one or more Knulli handhelds over SSH.
# Run from anywhere; it cd's to the snapos-ui/ dir itself.
#
#   ./knulli/deploy.sh root@192.168.1.42                 one device
#   ./knulli/deploy.sh root@192.168.1.42 root@...43      several, in order
#   ./knulli/deploy.sh                                   every device listed in
#                                                        knulli/devices.local
#
# devices.local is one "user@host" per line (# comments allowed). It is
# gitignored: keep your own handhelds there rather than in this script, so the
# published copy carries nobody's addresses.
#
# TIP: `ssh-keygen -t ed25519 -N "" -f ~/.ssh/id_ed25519 && ssh-copy-id root@<ip>`
# once, and you'll never be asked for the password again.

set -euo pipefail
cd "$(dirname "$0")/.."

TARGETS=("$@")
if [[ ${#TARGETS[@]} -eq 0 && -f knulli/devices.local ]]; then
  while IFS= read -r line; do
    line="${line%%#*}"; line="${line// /}"
    [[ -n "$line" ]] && TARGETS+=("$line")
  done < knulli/devices.local
fi
[[ ${#TARGETS[@]} -gt 0 ]] || {
  echo "usage: $0 user@device-ip [user@device-ip ...]" >&2
  echo "   or: list them in knulli/devices.local and run with no arguments" >&2
  echo "   (e.g. root@192.168.1.42 -- password: linux)" >&2
  exit 1
}
BIN="snapos_ui.aarch64"
[[ -f "$BIN" ]] || { echo "build first:  ./build-knulli.sh --sysroot ~/knulli-sysroot" >&2; exit 1; }
[[ -d assets ]] || { echo "no assets/ dir here?" >&2; exit 1; }

# strip Windows NTFS metadata streams if any crept back in
find assets -name '*:Zone.Identifier' -delete 2>/dev/null || true

echo ">> deploying to ${#TARGETS[@]} device(s): ${TARGETS[*]}"
EXTRA=""
[ -f scrape_boxart.py ] && EXTRA="$EXTRA scrape_boxart.py"
[ -f background_browser.py ] && EXTRA="$EXTRA background_browser.py"
[ -f ra_achievements.py ] && EXTRA="$EXTRA ra_achievements.py"
[ -f brightness-hotkey.sh ] || { echo "missing brightness-hotkey.sh" >&2; exit 1; }
EXTRA="$EXTRA brightness-hotkey.sh"
[ -f volume-gate.sh ] || { echo "missing volume-gate.sh" >&2; exit 1; }
EXTRA="$EXTRA volume-gate.sh"
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
# Stamp VERSION from the source constant. package.sh writes this for real
# releases; deploy.sh used to leave it alone, so a dev deploy left whatever
# version the last *packaged* install wrote and the device then reported a
# release number it was not running.
SNAPVER="$(sed -n 's/.*#define[[:space:]]\+SNAPFE_VERSION[[:space:]]\+"\(.*\)".*/\1/p' main.c | head -1)"
[ -n "$SNAPVER" ] || SNAPVER="unknown (SNAPFE_VERSION not found in main.c)"
printf 'Snap FE %s\nbuilt %s\nsource: knulli/deploy.sh (dev deploy, not a packaged release)\n' \
  "$SNAPVER" "$(date -u +%FT%TZ)" > ./_VERSION
echo "   version: $SNAPVER"

# Pack once. Re-running tar per device would re-read the whole asset tree for
# each handheld and, worse, let two devices receive different bytes if anything
# changed on disk mid-deploy.
PAYLOAD="$(mktemp -t snapfe-deploy.XXXXXX.tgz)"
trap 'rm -f "$PAYLOAD" ./_VERSION' EXIT
tar czf "$PAYLOAD" --exclude='*:Zone.Identifier' "$BIN" $EXTRA $KEYSEED _VERSION assets \
  vendor/link-cores/gpsp_libretro.so vendor/link-cores/gambatte_libretro.so \
  -C knulli custom.sh

FAILED=()
for DEV in "${TARGETS[@]}"; do
echo
echo ">> $DEV"
ssh "$DEV" '
  set -e
  # stop the frontend so nothing holds the old files open. Do NOT pkill on
  # "custom.sh" -- this very script mentions it and would kill our own shell.
  killall -9 snapos_ui retroarch mgba 2>/dev/null || true
  mkdir -p /userdata/system/snapos/config /userdata/roms
  # Configgen always loads both the built-in controller database and this user
  # file. Older SNAP deploys copied the complete built-in database here, making
  # configgen parse every controller twice on every game launch. Migrate only a
  # byte-identical stock copy out of the active path; preserve any genuinely
  # user-edited controller map untouched and keep the migrated copy recoverable.
  mkdir -p /userdata/system/configs/emulationstation
  USER_INPUT=/userdata/system/configs/emulationstation/es_input.cfg
  STOCK_INPUT=/usr/share/emulationstation/es_input.cfg
  if [ -f "$USER_INPUT" ] && [ -f "$STOCK_INPUT" ] && cmp -s "$USER_INPUT" "$STOCK_INPUT"; then
    mv -f "$USER_INPUT" "$USER_INPUT.snap-stock-duplicate"
    echo "   removed duplicate stock controller database from launch path"
  fi
  cd /userdata/system/snapos
  # Extract over the top (tar replaces each bundled file). Never clear the
  # whole assets tree: bookshelf spines, list icons and backgrounds are user-
  # extensible, and an update must not erase artwork placed there by the owner.
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
  mv -f _VERSION VERSION
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
  chmod 0755 snapos_ui *.py *.sh cores/*.so /usr/lib/libretro/gpsp_libretro.so \
    /usr/lib/libretro/gambatte_libretro.so /userdata/system/custom.sh 2>/dev/null || true
  # A large asset transfer can outlast the one-second custom.sh restart delay,
  # allowing the old binary to relaunch while files are still arriving. Stop
  # that stale process once more now that snapos_ui has been atomically replaced;
  # the persistent custom.sh loop will immediately start the new build.
  killall -9 snapos_ui 2>/dev/null || true
  # A manually stopped or crashed supervisor cannot relaunch the newly installed
  # frontend. Start exactly one copy when it is absent; leave a healthy existing
  # loop alone so repeated deployments never stack supervisors.
  if ! pgrep -f "[/]userdata/system/custom.sh start" >/dev/null 2>&1; then
    nohup /userdata/system/custom.sh start >/tmp/snapfe-deploy-restart.log 2>&1 </dev/null &
  fi
  echo "   installed to /userdata/system/snapos/"
  if [ -x snapos_ui ]; then echo "   exec bit: OK"; else
    echo "   NOTE: no exec bit (FAT /userdata). custom.sh runs it as: sh -c ./snapos_ui"
  fi
' < "$PAYLOAD" || { echo "   !! $DEV FAILED"; FAILED+=("$DEV"); }
done

echo
if [ ${#FAILED[@]} -gt 0 ]; then
  echo ">> finished with failures: ${FAILED[*]}"
else
  echo ">> done on all ${#TARGETS[@]} device(s)."
fi
for DEV in "${TARGETS[@]}"; do
  echo "   reboot $DEV:  ssh $DEV reboot"
done
echo "   revert:  ssh <dev> \"mv -f /userdata/system/custom.sh.pre-snapos /userdata/system/custom.sh 2>/dev/null || rm -f /userdata/system/custom.sh\""
[ ${#FAILED[@]} -eq 0 ]
