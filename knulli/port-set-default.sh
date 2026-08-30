#!/bin/bash
# Validate Snap FE, install its boot hook atomically, then reboot. This is also
# the supported upgrade path from 1.1.5 and earlier.

SNAPDIR=/userdata/system/snapos
SRC=$SNAPDIR/snapos-custom.sh
BIN=$SNAPDIR/snapos_ui
HOOK=/userdata/system/custom.sh
LOADER=/lib/ld-linux-aarch64.so.1
LOG=$SNAPDIR/install.log

mkdir -p "$SNAPDIR"
exec >>"$LOG" 2>&1
printf '\n[%s] Set Snap FE as default\n' "$(date -Iseconds 2>/dev/null || date)"

show_status() {
  printf '%s\n' "$1"
  printf '%s\n' "$1" > "$SNAPDIR/LAST-INSTALL-STATUS.txt"
  if [ -c /dev/tty1 ] && command -v dialog >/dev/null 2>&1; then
    TERM=linux dialog --clear --title "Snap FE Installer" --infobox "$1" 10 70 \
      </dev/tty1 >/dev/tty1 2>&1 || true
  fi
}

fail() {
  show_status "$1"
  sleep 10
  exit 1
}

[ -s "$BIN" ] || fail "Install failed: snapos_ui is missing. Extract the release ZIP directly onto SHARE, not into a SnapFE-Alpha folder."
[ -s "$SRC" ] || fail "Install failed: snapos-custom.sh is missing. Re-extract the complete release ZIP directly onto SHARE."
[ -x "$LOADER" ] || fail "Install failed: this is not a supported H700/aarch64 Knulli image."

if ! "$LOADER" --list "$BIN" >"$SNAPDIR/loader-check.log" 2>&1; then
  cat "$SNAPDIR/loader-check.log"
  fail "Install stopped safely: this Snap FE binary is incompatible with the installed Knulli libraries. See system/snapos/loader-check.log."
fi

if [ -f "$HOOK" ] && [ ! -f "$HOOK.pre-snapos" ] \
   && ! grep -q "Snap FE frontend hook\|Snap OS frontend hook" "$HOOK"; then
  cp -p "$HOOK" "$HOOK.pre-snapos" || fail "Could not back up the existing system/custom.sh."
fi

cp -p "$SRC" "$HOOK.snapfe-new" || fail "Could not stage the Snap FE boot hook. Check free space on SHARE."
chmod 0755 "$HOOK.snapfe-new" 2>/dev/null || true
mv -f "$HOOK.snapfe-new" "$HOOK" || fail "Could not activate the Snap FE boot hook."
grep -q "Snap FE frontend hook\|Snap OS frontend hook" "$HOOK" || fail "Boot-hook verification failed; the previous frontend was left unchanged."

sync
show_status "Snap FE installed successfully. The device will reboot automatically in 3 seconds."
sleep 3
if reboot; then
  # Knulli schedules the reboot and returns immediately; exit before the final
  # status can be replaced while shutdown is in progress.
  exit 0
fi

# Reaching here means reboot was refused; keep the hook installed and explain.
show_status "Snap FE is installed. Automatic reboot was unavailable; restart the device manually."
sleep 8
exit 0
