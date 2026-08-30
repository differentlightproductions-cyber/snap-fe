#!/usr/bin/env bash
# Build the Snap FE alpha add-on package for Knulli (Allwinner H700 / RG34XX-SP).
#
#   ./knulli/package.sh
#     -> dist/SnapFE-Alpha-<date>.zip
#
# The zip root mirrors the SD card's SHARE (/userdata) partition, so a user can
# extract it directly onto SHARE. Snap FE rides on top of Knulli -- it
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
DEST="$STAGE/system/snapos"
PORTS="$STAGE/roms/ports"
mkdir -p "$DEST/config" "$PORTS"

echo ">> staging $VER"

# Builds made with the newer Knulli/Buildroot compiler import ISO C23 glibc
# symbols (GLIBC_2.38). Pinned H700 releases can be older and reject such a
# binary before main() runs, producing a one-second black screen from Ports.
# Release binaries are built with Ubuntu's ARM64 cross compiler (glibc 2.35)
# and must remain free of those imports.
if strings "$BIN" | grep -E 'GLIBC_2\.(38|39|4[0-9])' >/dev/null; then
  echo "REFUSING TO PACKAGE: $BIN requires glibc 2.38+; rebuild with:" >&2
  echo "  ./build-knulli.sh --sysroot ~/knulli-sysroot --cc /usr/bin/aarch64-linux-gnu-gcc" >&2
  rm -rf "$STAGE"; exit 1
fi

# --- the app ---------------------------------------------------------------
cp "$BIN" "$DEST/snapos_ui"
cp scrape_boxart.py "$DEST/"
cp background_browser.py "$DEST/"
cp ra_achievements.py "$DEST/"
cp knulli/custom.sh "$DEST/snapos-custom.sh"          # staged; 'Set As Default' installs it
cp knulli/port-restore-es.sh "$DEST/snapos-uninstall.sh"
printf 'Snap FE Alpha Build %s\nbuilt %s\n' "$RELEASE" "$(date -u +%FT%TZ)" > "$DEST/VERSION"

# Link Play cores. Knulli's stock gpSP/Gambatte builds may predate their
# serial/network transports, so package the official ARM64 Libretro builds in
# persistent storage. snapos-custom.sh backs up the stock copies and installs
# these into Knulli's runtime core directory at boot.
CORE_SRC="vendor/link-cores"
for core in gpsp_libretro.so gambatte_libretro.so; do
  [[ -s "$CORE_SRC/$core" ]] || { echo "missing Link Play core: $CORE_SRC/$core (run ./knulli/setup-link-cores.sh)" >&2; exit 1; }
  install -D -m 0755 "$CORE_SRC/$core" "$DEST/cores/$core"
done
{
  echo "Official Libretro ARM64 nightly cores used by Snap FE Link Play"
  echo "https://buildbot.libretro.com/nightly/linux/aarch64/latest/"
  sha256sum "$DEST/cores/gpsp_libretro.so" "$DEST/cores/gambatte_libretro.so"
} > "$DEST/cores/CORE-SOURCES.txt"

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
        -o -iname 'activity.dat' -o -iname 'favorites.dat' \
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
chmod 0755 "$PORTS/"*.sh "$DEST/snapos_ui" "$DEST"/*.sh "$DEST"/*.py "$DEST/cores/"*.so

# --- instructions ----------------------------------------------------------
cp knulli/INSTALL.txt "$STAGE/INSTALL.txt"

# --- zip (via python3 so we don't need the 'zip' binary) ------------------
mkdir -p dist
OUT="dist/SnapFE-$VER.zip"
rm -f "$OUT"
ABS_OUT="$(pwd)/$OUT"
python3 - "$STAGE" "$ABS_OUT" <<'PY'
import os, sys, zipfile, stat
stage, out = sys.argv[1], sys.argv[2]
root = stage
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

# A valid release must extract straight to SHARE/system and SHARE/roms. Never
# reintroduce the version wrapper that made updates appear installed while old
# Ports entries kept launching the previous files.
python3 - "$OUT" <<'PY'
import sys, zipfile
with zipfile.ZipFile(sys.argv[1]) as z:
    names = set(z.namelist())
    required = {
        'system/snapos/snapos_ui',
        'system/snapos/snapos-custom.sh',
        'roms/ports/Snap FE.sh',
        'roms/ports/Snap FE (Set As Default).sh',
        'INSTALL.txt',
    }
    missing = sorted(required - names)
    wrapped = [n for n in names if n.startswith('SnapFE-Alpha-')]
    if missing or wrapped:
        raise SystemExit(f'bad release layout: missing={missing}, wrapped={wrapped[:3]}')
PY

echo
echo ">> $OUT  ($(du -h "$OUT" | cut -f1))"
python3 -c "import zipfile,sys; [print('    ', i.filename) for i in zipfile.ZipFile(sys.argv[1]).infolist()]" "$OUT"
