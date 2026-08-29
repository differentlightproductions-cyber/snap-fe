#!/usr/bin/env bash
# Assemble a minimal aarch64 sysroot to cross-compile Snap FE for Knulli:
#   - SDL2 / SDL2_ttf / SDL2_image .so files  <- copied from the device over SSH
#   - matching headers                        <- from this machine
#
#   ./setup-sysroot.sh [user@ip]        (required, e.g. root@192.168.1.42)
#
# You'll be asked for the device password once (Batocera default: "linux").
set -euo pipefail
cd "$(dirname "$0")"

DEV="${1:?usage: $0 user@device-ip   (e.g. root@192.168.1.42 -- password: linux)}"
SR="${SNAPOS_SYSROOT:-$HOME/knulli-sysroot}"
mkdir -p "$SR/usr/lib" "$SR/usr/include"

echo ">> pulling SDL libs from $DEV  (password once)"
# one scp call = one password prompt
scp -O "$DEV":'/usr/lib/libSDL2-2.0.so.0*' \
       "$DEV":'/usr/lib/libSDL2_ttf-2.0.so.0*' \
       "$DEV":'/usr/lib/libSDL2_image-2.0.so.0*' \
       "$SR/usr/lib/"

# dev symlinks the linker looks for (-lSDL2 -> libSDL2.so). Point each plain
# name at the fully-versioned file, not the glob (ln with >1 source needs a dir).
( cd "$SR/usr/lib"
  for base in libSDL2-2.0 libSDL2_ttf-2.0 libSDL2_image-2.0; do
    real=$( { ls -1 "$base".so.0.* 2>/dev/null || true; } | head -1 )
    [ -n "$real" ] && ln -sf "$real" "${base%%-*}.so" || true
  done )

echo ">> copying headers from this machine"
mkdir -p "$SR/usr/include/SDL2"
BR_INC="$HOME/buildroot/output/staging/usr/include/SDL2"
if [ -d "$BR_INC" ]; then
  cp -f "$BR_INC"/*.h "$SR/usr/include/SDL2/"
elif [ -d /usr/include/SDL2 ]; then
  cp -f /usr/include/SDL2/*.h "$SR/usr/include/SDL2/"
fi
# SDL_ttf.h isn't in buildroot staging -- take it from the host dev package
[ -f /usr/include/SDL2/SDL_ttf.h ]   && cp -f /usr/include/SDL2/SDL_ttf.h   "$SR/usr/include/SDL2/" || true
[ -f /usr/include/SDL2/SDL_image.h ] && cp -f /usr/include/SDL2/SDL_image.h "$SR/usr/include/SDL2/" || true

echo
echo ">> sysroot ready:  $SR"
ls -la "$SR/usr/lib"
[ -f "$SR/usr/include/SDL2/SDL.h" ]     && echo "   SDL.h      OK" || echo "   SDL.h      MISSING"
[ -f "$SR/usr/include/SDL2/SDL_ttf.h" ] && echo "   SDL_ttf.h  OK" || echo "   SDL_ttf.h  MISSING"
echo
echo "Now:  ../build-knulli.sh --sysroot $SR"
