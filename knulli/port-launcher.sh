#!/bin/bash
# Launch Snap FE from EmulationStation's Ports menu without changing startup.
# Any loader/runtime error is kept in a support log and shown on the device.

SNAPDIR=/userdata/system/snapos
SNAPOS=$SNAPDIR/snapos_ui
LOADER=/lib/ld-linux-aarch64.so.1
LOG=$SNAPDIR/port-launch.log

mkdir -p "$SNAPDIR"
exec >>"$LOG" 2>&1
printf '\n[%s] Snap FE Ports launch\n' "$(date -Iseconds 2>/dev/null || date)"

show_status() {
  printf '%s\n' "$1"
  printf '%s\n' "$1" > "$SNAPDIR/LAST-PORT-STATUS.txt"
  if [ -c /dev/tty1 ] && command -v dialog >/dev/null 2>&1; then
    TERM=linux dialog --clear --title "Snap FE" --infobox "$1" 10 70 \
      </dev/tty1 >/dev/tty1 2>&1 || true
  fi
}

fail() {
  show_status "$1"
  sleep 8
  exit 1
}

[ -s "$SNAPOS" ] || fail "Snap FE is not installed. Extract the release ZIP directly onto the SHARE drive so SHARE/system/snapos/snapos_ui exists."
[ -x "$LOADER" ] || fail "This firmware has no ARM64 loader at $LOADER. Snap FE requires an H700/aarch64 Knulli image."

# --list resolves the executable and all shared libraries without starting the
# UI. It catches incompatible glibc and missing SDL libraries before the screen
# is handed over.
if ! "$LOADER" --list "$SNAPOS" >"$SNAPDIR/loader-check.log" 2>&1; then
  cat "$SNAPDIR/loader-check.log"
  fail "Snap FE cannot start on this Knulli build. See system/snapos/loader-check.log on SHARE for the exact missing library or glibc version."
fi

cd "$SNAPDIR" || fail "Could not open $SNAPDIR. Check the SHARE filesystem for errors."
export XDG_RUNTIME_DIR=/var/run
pidof wireplumber >/dev/null 2>&1 || ( wireplumber >/var/log/wireplumber.log 2>&1 & )

"$LOADER" "$SNAPOS"
rc=$?
if [ "$rc" -ne 0 ]; then
  fail "Snap FE exited during startup (code $rc). Send system/snapos/port-launch.log and loader-check.log with your Knulli version."
fi
exit 0
