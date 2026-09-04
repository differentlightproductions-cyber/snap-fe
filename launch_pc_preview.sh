#!/usr/bin/env bash
# Run the desktop build against the repo's own assets, without touching your
# real ~/snapos-ui. Creates a throwaway HOME whose snapos-ui/ is a symlink back
# to this checkout, so settings and cache land in the temp dir and the assets,
# fonts and icons come from the tree you are working in.
#
#   cc -O2 -Wall -Wno-unused-parameter main.c -o snapos_ui \
#      $(pkg-config --cflags --libs sdl2 SDL2_ttf SDL2_image) -lm
#   ./launch_pc_preview.sh
set -euo pipefail

repo_root="$(cd "$(dirname "$0")" && pwd)"
bin=""
for cand in snapos_ui snapos_ui.desktop; do
  [[ -x "$repo_root/$cand" ]] && { bin="$repo_root/$cand"; break; }
done
[[ -n "$bin" ]] || { echo "build first -- see the comment at the top of this script" >&2; exit 1; }

preview_root="$(mktemp -d /tmp/snapfe-preview.XXXXXX)"
mkdir -p "$preview_root/home"
ln -s "$repo_root" "$preview_root/home/snapos-ui"
cd "$repo_root"
exec env HOME="$preview_root/home" "$bin" "$@"
