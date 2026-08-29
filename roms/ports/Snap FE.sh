#!/bin/bash
# Snap FE -- launch from EmulationStation's Ports menu to try it.
# EmulationStation stays paused underneath; quit Snap FE to return to it.
SNAPOS=/userdata/system/snapos/snapos_ui
LOADER=/lib/ld-linux-aarch64.so.1
cd /userdata/system/snapos || exit 1
export XDG_RUNTIME_DIR=/var/run
# make sure the audio session manager is up (ES doesn't guarantee it)
pidof wireplumber >/dev/null 2>&1 || ( wireplumber >/var/log/wireplumber.log 2>&1 & )
if [ -x "$SNAPOS" ]; then exec "$SNAPOS"; else exec "$LOADER" "$SNAPOS"; fi
