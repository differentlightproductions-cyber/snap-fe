#!/usr/bin/env bash
# Cross-compile Snap FE for Knulli on Allwinner H700 / aarch64.
#
# For public release builds prefer Ubuntu's aarch64-linux-gnu-gcc. Its glibc
# 2.35 baseline runs on pinned and current Knulli releases. A compiler from a
# current Knulli Buildroot may import GLIBC_2.38 ISO C23 symbols and black-screen
# immediately on older firmware.

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
  if command -v aarch64-linux-gnu-gcc >/dev/null 2>&1; then
    CC="$(command -v aarch64-linux-gnu-gcc)"
  elif [[ -x "$HOME/buildroot/output/host/bin/aarch64-buildroot-linux-gnu-gcc" ]]; then
    CC="$HOME/buildroot/output/host/bin/aarch64-buildroot-linux-gnu-gcc"
  fi
fi
[[ -n "$CC" ]] || { echo "no aarch64 gcc -- pass --cc" >&2; exit 1; }
[[ -n "$SYSROOT" && -d "$SYSROOT/usr/include/SDL2" ]] || {
  echo "pass --sysroot (run ./knulli/setup-sysroot.sh first)" >&2; exit 1;
}

echo ">> CC      = $CC"
echo ">> SYSROOT = $SYSROOT"

"$CC" -DSNAPOS_TARGET_KNULLI -O2 -mcpu=cortex-a53 -pipe -ffp-contract=fast \
  -Wall -Wno-unused-parameter -s \
  -I"$SYSROOT/usr/include" -I"$SYSROOT/usr/include/SDL2" \
  main.c -o "$OUT" \
  -L"$SYSROOT/usr/lib" -Wl,-rpath-link,"$SYSROOT/usr/lib" \
  -Wl,--unresolved-symbols=ignore-in-shared-libs \
  -lSDL2 -lSDL2_ttf -lSDL2_image -lm

echo
file "$OUT"
if strings "$OUT" | grep -E 'GLIBC_2\.(38|39|4[0-9])' >/dev/null; then
  echo "WARNING: $OUT needs glibc 2.38+ and is not suitable for pinned Knulli releases." >&2
else
  echo "Compatibility check: no glibc 2.38+ imports."
fi
echo
echo "OK. Install: cd knulli && ./install.sh"
