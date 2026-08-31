#!/bin/bash
# Restore EmulationStation as the boot frontend.
HOOK=/userdata/system/custom.sh
BAK="$HOOK.pre-snapos"

# Return Knulli's stock media-key handler before removing the Snap frontend.
umount /usr/bin/volume-button >/dev/null 2>&1 || true
TRIGGER_CFG=/etc/triggerhappy/triggers.d/multimedia_keys.conf
TRIGGER_BACKUP=/userdata/system/snapos/triggerhappy-multimedia_keys.stock
if [ -s "$TRIGGER_BACKUP" ] && grep -q '^# SNAP_FE_VOLUME_OWNER$' "$TRIGGER_CFG" 2>/dev/null; then
  cp -f "$TRIGGER_BACKUP" "$TRIGGER_CFG"
  for service in /etc/init.d/S*triggerhappy; do
    [ -x "$service" ] && { "$service" restart >/dev/null 2>&1 || true; break; }
  done
fi

is_snap() { grep -q "Snap FE frontend hook" "$1" 2>/dev/null || grep -q "Snap OS frontend hook" "$1" 2>/dev/null; }

if [ -f "$BAK" ] && ! is_snap "$BAK"; then
  # A genuine pre-Snap custom.sh -- put it back.
  mv -f "$BAK" "$HOOK"
  echo "restored your original custom.sh"
elif is_snap "$HOOK" || [ -f "$HOOK" ]; then
  rm -f "$HOOK"
  # An older install may have saved the Snap hook itself as the "backup" -- clear it.
  if [ -f "$BAK" ] && is_snap "$BAK"; then rm -f "$BAK"; fi
  echo "removed the Snap FE boot hook -- EmulationStation will run on next boot"
else
  echo "Snap FE was not set as the default frontend -- nothing to do"
fi
sync
echo
echo "  Reboot to boot back into EmulationStation."
echo "  Snap FE files are left in /userdata/system/snapos/ (delete by hand to fully remove)."
echo
sleep 5
