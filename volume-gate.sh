#!/bin/sh
# Preserve Knulli's stock Volume behavior unless SNAP is currently holding the
# in-game Menu/Fn brightness modifier. The native frontend owns that chord.
state=/tmp/snapfe-brightness-modifier
[ "$(sed -n '1p' "$state" 2>/dev/null)" = "1" ] && exit 0
exec /usr/bin/volume-button "$@"
