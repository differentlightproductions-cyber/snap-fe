#!/usr/bin/env bash
# Revert install.sh -- restore EmulationStation as the frontend.
#   ./uninstall.sh                       (on device)
#   ./uninstall.sh --root /mnt/knulli    (SD mounted on a PC)
set -euo pipefail
ROOT="/userdata"
[[ "${1:-}" == "--root" ]] && ROOT="$2"

HOOK="$ROOT/system/custom.sh"
if [[ -f "$HOOK.pre-snapos" ]]; then
  mv "$HOOK.pre-snapos" "$HOOK"
  echo ">> restored your original custom.sh"
else
  rm -f "$HOOK"
  echo ">> removed Snap FE custom.sh"
fi

echo ">> Snap FE files left in $ROOT/system/snapos/ (delete by hand if you want)."
echo ">> Reboot. EmulationStation is the frontend again."
