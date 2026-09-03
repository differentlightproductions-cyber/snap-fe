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

# The panel's PWM range is 0-255 (lcd_pwm_max_limit in the device tree on both
# the RG34XX-SP and the RG35XX-SP), not 0-200. Doubling the percentage capped
# the backlight at ~78% of what the panel can actually do.
if [ "$pct" -le 1 ]; then absolute=1; else absolute=$((pct * 255 / 100)); fi
[ "$absolute" -lt 1 ] && absolute=1
printf '%s\n' "$pct" > "$desired"
LCD_BRIGHTNESS_MINIMUM=1 /usr/bin/brightness set "$absolute" >/dev/null 2>&1

# Keep SNAP's next boot and post-game state in agreement with the panel. Do not
# run knulli-settings-set for every key repeat: that rewrites knulli.conf while
# configgen is launching and was both a latency source and a late brightness
# writer. SNAP persists the final value once when the emulator exits; a crash is
# still recovered from this settings file on the next frontend start.
if [ -f "$settings" ]; then
  if grep -q '^brightness_pct=' "$settings" 2>/dev/null; then
    sed -i "s/^brightness_pct=.*/brightness_pct=$pct/" "$settings"
  else
    printf 'brightness_pct=%s\n' "$pct" >> "$settings"
  fi
fi
