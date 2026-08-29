#!/bin/bash
# Make Snap FE the boot frontend (replaces EmulationStation until you undo it).
# Your existing custom.sh, if any, is saved to custom.sh.pre-snapos.
set -e
SRC=/userdata/system/snapos/snapos-custom.sh
HOOK=/userdata/system/custom.sh

[ -f "$SRC" ] || { echo "missing $SRC -- reinstall the Snap FE package"; sleep 3; exit 1; }

if [ -f "$HOOK" ] && [ ! -f "$HOOK.pre-snapos" ] \
   && ! grep -q "Snap FE frontend hook\|Snap OS frontend hook" "$HOOK"; then
  cp "$HOOK" "$HOOK.pre-snapos"
  echo "saved your existing custom.sh -> custom.sh.pre-snapos"
fi
cp "$SRC" "$HOOK"
chmod 0755 "$HOOK" 2>/dev/null || true
sync
echo
echo "  Snap FE is now the default frontend."
echo "  Reboot the device to boot into it."
echo "  (Undo any time with the 'Restore EmulationStation' port.)"
echo
sleep 5
