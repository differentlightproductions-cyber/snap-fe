#!/usr/bin/env bash
# Revert install.sh -- restore EmulationStation as the frontend.
#   ./uninstall.sh                       (on device)
#   ./uninstall.sh --root /mnt/knulli    (SD mounted on a PC)
set -euo pipefail
ROOT="/userdata"
[[ "${1:-}" == "--root" ]] && ROOT="$2"

HOOK="$ROOT/system/custom.sh"
if [[ "$ROOT" == "/userdata" ]]; then
  umount /usr/bin/volume-button >/dev/null 2>&1 || true
  TRIGGER_CFG=/etc/triggerhappy/triggers.d/multimedia_keys.conf
  TRIGGER_BACKUP="$ROOT/system/snapos/triggerhappy-multimedia_keys.stock"
  if [[ -s "$TRIGGER_BACKUP" ]] && grep -q '^# SNAP_FE_VOLUME_OWNER$' "$TRIGGER_CFG" 2>/dev/null; then
    cp -f "$TRIGGER_BACKUP" "$TRIGGER_CFG"
    for service in /etc/init.d/S*triggerhappy; do
      [[ -x "$service" ]] && { "$service" restart >/dev/null 2>&1 || true; break; }
    done
  fi
fi
if [[ -f "$HOOK.pre-snapos" ]]; then
  mv "$HOOK.pre-snapos" "$HOOK"
  echo ">> restored your original custom.sh"
else
  rm -f "$HOOK"
  echo ">> removed Snap FE custom.sh"
fi

echo ">> Snap FE files left in $ROOT/system/snapos/ (delete by hand if you want)."
echo ">> Reboot. EmulationStation is the frontend again."
