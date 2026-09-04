#!/bin/sh
# Preserve Knulli's stock Volume behavior unless SNAP is currently holding the
# in-game Menu/Fn brightness modifier. The native frontend owns that chord.
state=/tmp/snapfe-brightness-modifier
[ "$(sed -n '1p' "$state" 2>/dev/null)" = "1" ] && exit 0

/usr/bin/volume-button "$@"
rc=$?

# Leave the resulting level where SNAP can read it cheaply. While a game owns
# the display SNAP draws its own volume bar on a spare display-engine layer, and
# polling it needs a value that costs a file read rather than a process spawn.
level=$(XDG_RUNTIME_DIR=/var/run /usr/bin/knulli-audio getSystemVolume 2>/dev/null)
case "$level" in
  ''|*[!0-9]*) ;;
  *) printf '%s\n' "$level" > /userdata/system/snapos/.volume-current 2>/dev/null ;;
esac
exit $rc
