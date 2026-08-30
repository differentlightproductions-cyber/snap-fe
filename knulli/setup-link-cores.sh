#!/usr/bin/env bash
# Download the official ARM64 Libretro cores used by Snap FE Link Play.

set -euo pipefail
cd "$(dirname "$0")/.."

BASE=https://buildbot.libretro.com/nightly/linux/aarch64/latest
DEST=vendor/link-cores
mkdir -p "$DEST"

for core in gpsp_libretro.so gambatte_libretro.so; do
  echo ">> $core"
  curl -fL "$BASE/$core.zip" -o "$DEST/$core.zip"
  unzip -jo "$DEST/$core.zip" "$core" -d "$DEST"
  chmod 0755 "$DEST/$core"
done

file "$DEST/gpsp_libretro.so" "$DEST/gambatte_libretro.so"
file "$DEST/gpsp_libretro.so" | grep -q 'ARM aarch64'
file "$DEST/gambatte_libretro.so" | grep -q 'ARM aarch64'
strings "$DEST/gpsp_libretro.so" | grep -E 'gpsp_serial' >/dev/null
strings "$DEST/gpsp_libretro.so" | grep -E 'mul_poke' >/dev/null
strings "$DEST/gambatte_libretro.so" | grep -E 'gambatte_gb_link_mode' >/dev/null

echo
echo "Link Play core verification: PASS"
sha256sum "$DEST/gpsp_libretro.so" "$DEST/gambatte_libretro.so"
