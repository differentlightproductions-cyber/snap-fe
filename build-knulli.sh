#!/usr/bin/env bash
# Cross-compile Snap OS for Knulli CFW (Batocera 42, Allwinner H700 / aarch64).
#
#   1) ./knulli/setup-sysroot.sh          # pulls SDL .so + headers -> ~/knulli-sysroot
#   2) ./build-knulli.sh --sysroot ~/knulli-sysroot
#      -> snapos_ui.aarch64
#
# Options:
#   --sysroot DIR   sysroot with usr/lib/libSDL2*.so + usr/include/SDL2/*.h  (required)
#   --cc GCC        aarch64 compiler (default: buildroot's, then aarch64-linux-gnu-gcc)
#   --out FILE      output name (default: snapos_ui.aarch64)

set -euo pipefail
cd "$(dirname "$0")"

CC=""
SYSROOT="${SNAPOS_SYSROOT:-}"
OUT="snapos_ui.aarch64"
while [[ $# -gt 0 ]]; do
  case "$1" in
    --cc)      CC="$2"; shift 2 ;;
    --sysroot) SYSROOT="$2"; shift 2 ;;
    --out)     OUT="$2"; shift 2 ;;
    *) echo "unknown arg: $1" >&2; exit 1 ;;
  esac
done

if [[ -z "$CC" ]]; then
  if [[ -x "$HOME/buildroot/output/host/bin/aarch64-buildroot-linux-gnu-gcc" ]]; then
    CC="$HOME/buildroot/output/host/bin/aarch64-buildroot-linux-gnu-gcc"
  elif command -v aarch64-linux-gnu-gcc >/dev/null; then
    CC="aarch64-linux-gnu-gcc"
  fi
fi
[[ -n "$CC" ]] || { echo "no aarch64 gcc -- pass --cc" >&2; exit 1; }
[[ -n "$SYSROOT" && -d "$SYSROOT/usr/include/SDL2" ]] || {
  echo "pass --sysroot (run ./knulli/setup-sysroot.sh first)" >&2; exit 1; }

echo ">> CC      = $CC"
echo ">> SYSROOT = $SYSROOT"

# --unresolved-symbols=ignore-in-shared-libs: we only ship the 3 SDL .so's in the
# sysroot; their own deps (freetype, png, z, ...) all exist on the device, so
# leave them for the runtime loader instead of demanding them at link time.
# -mcpu=cortex-a53: schedule for the H700's cores. -O2 + -ffp-contract=fast is
# plenty; the app isn't FP-heavy. -s strips (smaller, faster to load off SD).
"$CC" -DSNAPOS_TARGET_KNULLI -O2 -mcpu=cortex-a53 -pipe -ffp-contract=fast \
  -Wall -Wno-unused-parameter -s \
  -I"$SYSROOT/usr/include" -I"$SYSROOT/usr/include/SDL2" \
  main.c -o "$OUT" \
  -L"$SYSROOT/usr/lib" -Wl,-rpath-link,"$SYSROOT/usr/lib" \
  -Wl,--unresolved-symbols=ignore-in-shared-libs \
  -lSDL2 -lSDL2_ttf -lSDL2_image -lm

echo; file "$OUT"
echo; echo "OK. Install:  cd knulli && ./install.sh          (over SSH to the device)"
echo "         or:  cd knulli && ./install.sh --root /mnt/<userdata-partition>"
