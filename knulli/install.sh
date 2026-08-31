#!/usr/bin/env bash
# Install Snap FE as the frontend on a Knulli CFW handheld.
#
# Run it ONE of two ways:
#   A) On the device itself over SSH:         ./install.sh
#   B) Against the SD card mounted on a PC:   ./install.sh --root /mnt/knulli-userdata
#      (--root must point at what becomes /userdata on the device)
#
# What it does:
#   - copies the aarch64 binary + assets to  <userdata>/system/snapos/
#   - installs  <userdata>/system/custom.sh  so Snap FE runs instead of
#     EmulationStation at boot (your previous custom.sh is backed up)
#   - leaves EmulationStation fully intact -- ./uninstall.sh restores it

set -euo pipefail
cd "$(dirname "$0")"

ROOT="/userdata"
[[ "${1:-}" == "--root" ]] && ROOT="$2"

BIN_SRC="../snapos_ui.aarch64"
[[ -f "$BIN_SRC" ]] || { echo "build first: ../build-knulli.sh --sysroot ..." >&2; exit 1; }

DEST="$ROOT/system/snapos"
mkdir -p "$DEST"
cp "$BIN_SRC" "$DEST/snapos_ui"
chmod +x "$DEST/snapos_ui"

for helper in scrape_boxart.py background_browser.py ra_achievements.py brightness-hotkey.sh volume-gate.sh; do
  [[ -f "../$helper" ]] && cp "../$helper" "$DEST/$helper"
done

for core in gpsp_libretro.so gambatte_libretro.so; do
  [[ -s "../vendor/link-cores/$core" ]] || {
    echo "missing ../vendor/link-cores/$core -- run ./knulli/setup-link-cores.sh" >&2
    exit 1
  }
  install -D -m 0755 "../vendor/link-cores/$core" "$DEST/cores/$core"
done

# Ship the assets Snap FE needs without deleting user-added bookshelf spines,
# list icons or backgrounds during an upgrade.
rsync -a ../assets/ "$DEST/assets/" 2>/dev/null || cp -r ../assets "$DEST/"
mkdir -p "$DEST/config" "$ROOT/roms"

# --- frontend hook -------------------------------------------------------------
HOOK="$ROOT/system/custom.sh"
if [[ -f "$HOOK" && ! -f "$HOOK.pre-snapos" ]] && ! grep -q "Snap FE frontend hook\|Snap OS frontend hook" "$HOOK"; then
  cp "$HOOK" "$HOOK.pre-snapos"
  echo ">> backed up existing custom.sh -> custom.sh.pre-snapos"
fi
cp ./custom.sh "$HOOK"
chmod +x "$HOOK" "$DEST/brightness-hotkey.sh" "$DEST/volume-gate.sh" 2>/dev/null || true

echo
echo "Installed to $DEST"
echo "Reboot the device. Snap FE launches instead of EmulationStation."
echo "To go back:  ./uninstall.sh${1:+ --root $ROOT}"
