#!/bin/sh
# System-level Function + Volume brightness for SNAP FE on H700 handhelds.
# triggerhappy invokes this while a game owns the display, so the action never
# depends on RetroArch's hotkeys or on SNAP's SDL event loop.

case "$1" in
  up)   direction=1 ;;
  down) direction=-1 ;;
  *) exit 2 ;;
esac

settings=/userdata/system/snapos/settings.cfg
desired=/userdata/system/snapos/.brightness-desired
pct=""
[ -r "$desired" ] && pct=$(sed -n '1p' "$desired" 2>/dev/null)
case "$pct" in ''|*[!0-9]*) pct=$(knulli-settings-get display.brightness 2>/dev/null) ;; esac
case "$pct" in ''|*[!0-9]*) pct=100 ;; esac
[ "$pct" -lt 1 ] && pct=1
[ "$pct" -gt 100 ] && pct=100

if [ "$direction" -gt 0 ]; then
  if [ "$pct" -le 1 ]; then pct=5; else pct=$((pct + 5)); fi
  [ "$pct" -gt 100 ] && pct=100
else
  pct=$((pct - 5))
  [ "$pct" -lt 5 ] && pct=1
fi

absolute=$((pct * 2))
[ "$absolute" -lt 3 ] && absolute=3
/usr/bin/brightness set "$absolute" >/dev/null 2>&1
knulli-settings-set display.brightness "$pct" >/dev/null 2>&1
printf '%s\n' "$pct" > "$desired"

# Keep SNAP's next boot and post-game state in agreement with the panel.
if [ -f "$settings" ]; then
  if grep -q '^brightness_pct=' "$settings" 2>/dev/null; then
    sed -i "s/^brightness_pct=.*/brightness_pct=$pct/" "$settings"
  else
    printf 'brightness_pct=%s\n' "$pct" >> "$settings"
  fi
fi
