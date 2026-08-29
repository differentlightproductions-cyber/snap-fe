#!/usr/bin/env bash
# Build the Snap FE alpha add-on package for Knulli (Allwinner H700 / RG34XX-SP).
#
#   ./knulli/package.sh
#     -> dist/SnapFE-Alpha-<date>.zip
#
# The zip mirrors the layout of the SD card's SHARE (/userdata) partition, so a
# user just extracts it there and reboots. Snap FE rides on top of Knulli -- it
# uses Knulli's SDL2, RetroArch, cores and drivers, so no OS reflashing.

set -euo pipefail
cd "$(dirname "$0")/.."

BIN=snapos_ui.aarch64
[[ -f "$BIN" ]] || { echo "build first:  ./build-knulli.sh --sysroot ~/knulli-sysroot" >&2; exit 1; }
[[ -d assets ]] || { echo "no assets/ here?" >&2; exit 1; }

# Version tag for the zip name + VERSION file. Override: ./knulli/package.sh 1.2.0
RELEASE="${1:-1.1.0}"
VER="Alpha-${RELEASE}"
STAGE="$(mktemp -d)"
ROOT="$STAGE/SnapFE-$VER"
DEST="$ROOT/system/snapos"
PORTS="$ROOT/roms/ports"
mkdir -p "$DEST/config" "$PORTS"

echo ">> staging $VER"

# --- the app ---------------------------------------------------------------
cp "$BIN" "$DEST/snapos_ui"
cp scrape_boxart.py "$DEST/"
cp knulli/custom.sh "$DEST/snapos-custom.sh"          # staged; 'Set As Default' installs it
cp knulli/port-restore-es.sh "$DEST/snapos-uninstall.sh"
printf 'Snap FE Alpha Build %s\nbuilt %s\n' "$RELEASE" "$(date -u +%FT%TZ)" > "$DEST/VERSION"

# assets ONLY -- fonts, background/icon art, sound effects, READMEs. Never any
# ROMs, save files, scraped box art, API keys or account configs (those all
# live at the repo root, not under assets/, and are never referenced here).
# The include-list + exclude-list below are belt-and-suspenders for that.
( cd assets && find . -type f \
    ! -name '*:Zone.Identifier' ! -name '.DS_Store' \
    ! -iname '*.key' ! -iname 'settings.cfg' ! -iname '*.cfg' ! -iname '*.dat' \
    ! -iname '*.srm' ! -iname '*.sav' ! -iname '*.state*' \
    \( -iname '*.ttf' -o -iname '*.otf' -o -iname '*.png' -o -iname '*.jpg' \
       -o -iname '*.jpeg' -o -iname '*.gif' -o -iname '*.bmp' -o -iname '*.wav' \
       -o -iname '*.ogg' -o -iname '*.mp3' -o -iname '*.flac' -o -iname '*.txt' \) \
    -exec install -D -m 0644 '{}' "$DEST/assets/{}" ';' )

# Refuse to ship if anything personal slipped into the staged tree.
if find "$DEST" -type f \( -iname '*.key' -o -iname 'settings.cfg' \
        -o -iname 'screenscraper.cfg' -o -iname 'retroachievements.cfg' \
        -o -iname 'activity.dat' -o -iname 'favorites*' \
        -o -iname '*.gba' -o -iname '*.gb' -o -iname '*.gbc' -o -iname '*.nes' \
        -o -iname '*.sfc' -o -iname '*.smc' -o -iname '*.md' -o -iname '*.bin' \
        -o -iname '*.iso' -o -iname '*.chd' -o -iname '*.srm' -o -iname '*.state*' \) \
        -print | grep -q .; then
  echo "REFUSING TO PACKAGE: personal / ROM / save file found in staged tree:" >&2
  find "$DEST" -type f \( -iname '*.key' -o -iname 'settings.cfg' -o -iname '*.gba' \
        -o -iname '*.state*' -o -iname 'activity.dat' \) -print >&2
  rm -rf "$STAGE"; exit 1
fi
[ -z "$(ls -A "$DEST/config")" ] || { echo "REFUSING: $DEST/config is not empty" >&2; rm -rf "$STAGE"; exit 1; }

# --- EmulationStation Ports entries -------------------------------------------
cp knulli/port-launcher.sh   "$PORTS/Snap FE.sh"
cp knulli/port-set-default.sh "$PORTS/Snap FE (Set As Default).sh"
cp knulli/port-restore-es.sh "$PORTS/Snap FE (Restore EmulationStation).sh"
chmod 0755 "$PORTS/"*.sh "$DEST/snapos_ui" "$DEST"/*.sh

# --- instructions ----------------------------------------------------------
cp knulli/INSTALL.txt "$ROOT/INSTALL.txt"

# --- zip (via python3 so we don't need the 'zip' binary) ------------------
mkdir -p dist
OUT="dist/SnapFE-$VER.zip"
rm -f "$OUT"
ABS_OUT="$(pwd)/$OUT"
python3 - "$STAGE" "$ABS_OUT" "SnapFE-$VER" <<'PY'
import os, sys, zipfile, stat
stage, out, top = sys.argv[1], sys.argv[2], sys.argv[3]
root = os.path.join(stage, top)
with zipfile.ZipFile(out, "w", zipfile.ZIP_DEFLATED) as z:
    for dp, _dn, fns in os.walk(root):
        for fn in sorted(fns):
            full = os.path.join(dp, fn)
            arc = os.path.relpath(full, stage)
            zi = zipfile.ZipInfo(arc)
            st = os.stat(full)
            # preserve the exec bit so .sh files run on the device
            zi.external_attr = (stat.S_IMODE(st.st_mode) & 0o777) << 16
            zi.compress_type = zipfile.ZIP_DEFLATED
            with open(full, "rb") as f:
                z.writestr(zi, f.read())
PY
rm -rf "$STAGE"

echo
echo ">> $OUT  ($(du -h "$OUT" | cut -f1))"
python3 -c "import zipfile,sys; [print('    ', i.filename) for i in zipfile.ZipFile(sys.argv[1]).infolist()]" "$OUT"
