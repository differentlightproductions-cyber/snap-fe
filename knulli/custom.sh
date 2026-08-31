#!/bin/bash
# Snap FE frontend hook for Knulli / Batocera-family CFW.
#
# Batocera runs /userdata/system/custom.sh near the end of boot -- by which
# point S31emulationstation has already launched EmulationStation. We kill it
# immediately (before it can draw a frame), wipe the framebuffer so no
# half-rendered ES screen is left behind, and run ONLY Snap FE as the
# frontend. If Snap FE ever exits it is relaunched; ES is never brought back.
#
# Revert with knulli/uninstall.sh (restores your previous custom.sh).

SNAPOS=/userdata/system/snapos/snapos_ui
LOADER=/lib/ld-linux-aarch64.so.1
SNAPOS_CORES=/userdata/system/snapos/cores
SYSTEM_CORES=/usr/lib/libretro
CORE_BACKUP=/userdata/system/snapos/core-backup
VOLUME_STUB=/userdata/system/snapos/volume-button-snapfe.sh
VOLUME_TARGET=/usr/bin/volume-button
TRIGGER_CFG=/etc/triggerhappy/triggers.d/multimedia_keys.conf
TRIGGER_BACKUP=/userdata/system/snapos/triggerhappy-multimedia_keys.stock

# Link Play needs newer gpSP/Gambatte serial transports than the stock cores on
# some Knulli images. Keep the shipped copies on /userdata (persistent), back
# up the original system cores once, then refresh the writable root overlay on
# every boot before Snap FE probes or launches them.
install_link_cores() {
  [ -d "$SNAPOS_CORES" ] || return 0
  mkdir -p "$CORE_BACKUP"
  for core in gpsp_libretro.so gambatte_libretro.so; do
    [ -s "$SNAPOS_CORES/$core" ] || continue
    if [ -e "$SYSTEM_CORES/$core" ] && [ ! -e "$CORE_BACKUP/$core" ]; then
      cp -p "$SYSTEM_CORES/$core" "$CORE_BACKUP/$core"
    fi
    install -m 0755 "$SNAPOS_CORES/$core" "$SYSTEM_CORES/$core"
  done
}

kill_es() {
  /etc/init.d/S31emulationstation stop >/dev/null 2>&1
  killall -q -9 emulationstation emulationstation-standalone emulationstation.sh 2>/dev/null
}

# Blank every framebuffer + console right now, so nothing ES drew (or the boot
# console text) is visible in the gap before Snap FE takes the screen.
hide_screen() {
  for fb in /dev/fb0 /dev/fb1; do [ -c "$fb" ] && dd if=/dev/zero of="$fb" bs=1M count=16 2>/dev/null; done
  for c in /dev/tty0 /dev/tty1; do [ -c "$c" ] && { printf '\033[2J\033[H\033[?25l' > "$c" 2>/dev/null; }; done
  setterm --blank force >/dev/null 2>&1 || true
}

# EmulationStation normally triggers Knulli's audio bring-up. We skip ES, so:
#   - S06audio starts pipewire (+ pipewire-pulse) but NOT wireplumber, the
#     session manager that routes audio to the sink -> total silence
#   - the H700 codec's "SPK" amp switch is off at reset
# Bring it all up ourselves, in order. Runs sequentially before Snap FE starts.
init_audio() {
  export XDG_RUNTIME_DIR=/var/run

  /etc/init.d/S06audio start >/dev/null 2>&1          # alsactl init + pipewire + pipewire-pulse

  if ! pidof wireplumber >/dev/null 2>&1; then
    ( wireplumber >/var/log/wireplumber.log 2>&1 & )
  fi
  # wait for pipewire-pulse to answer AND wireplumber to be up (max ~9s)
  i=0
  while [ "$i" -lt 30 ]; do
    if pactl info >/dev/null 2>&1 && pidof wireplumber >/dev/null 2>&1; then break; fi
    sleep 0.3; i=$((i + 1))
  done

  /etc/init.d/S27audioconfig start >/dev/null 2>&1   # knulli-audio profile/sink/volume
  knulli-audio set auto            >/dev/null 2>&1   # h700: routes to the internal codec
  knulli-audio setSystemVolume 90  >/dev/null 2>&1
  amixer -c 0 sset 'SPK' unmute on           >/dev/null 2>&1
  amixer -c 0 sset 'LINEOUT' unmute on       >/dev/null 2>&1   # 3.5mm jack path (H700 shares lineout)
  amixer -c 0 sset 'digital volume' 63       >/dev/null 2>&1
  amixer -c 0 sset 'lineout volume' 31       >/dev/null 2>&1
  # Straight stereo DAC routing. If these land off (some Knulli builds) there's
  # total silence on both speaker AND headphones, so force the correct pair on.
  amixer -c 0 sset 'OutputL Mixer DACL' on   >/dev/null 2>&1
  amixer -c 0 sset 'OutputR Mixer DACR' on   >/dev/null 2>&1
}

# Black out any partial EmulationStation frame so the gap before Snap FE's own
# boot screen is just black, not a broken ES screen.
blank_fb() {
  for fb in /dev/fb0 /dev/fb1; do
    [ -c "$fb" ] && dd if=/dev/zero of="$fb" bs=1M count=16 2>/dev/null
  done
}

# Clamshell: let Snap FE own the lid (screen off on close, on on open) instead
# of Knulli's lid-control doing a real suspend whose wake path is broken when
# ES isn't running. Set back to "suspend" if you prefer true suspend-to-RAM.
init_lid() {
  knulli-settings-set system.lid none >/dev/null 2>&1
}

# Bluetooth: Knulli's S32bluetooth brings up the radio + bluetoothd + pairing
# agent, but only if the user has enabled it. It's normally kicked by the ES
# session; do it here. Opt-in -- BT off = no extra battery drain.
init_bluetooth() {
  [ "$(knulli-settings-get controllers.bluetooth.enabled 2>/dev/null)" = "1" ] || return 0
  /etc/init.d/S29namebluetooth start >/dev/null 2>&1
  /etc/init.d/S32bluetooth start     >/dev/null 2>&1
}

run_snapos() {
  cd /userdata/system/snapos || return 1
  if [ -x "$SNAPOS" ]; then "$SNAPOS"; else "$LOADER" "$SNAPOS"; fi
}

# Knulli's triggerhappy service normally runs /usr/bin/volume-button for the
# same physical keys that Snap watches. Letting both handlers act caused large
# jumps, a wrap from minimum to maximum, and mismatched values. Bind a tiny
# no-op over that one callback while Snap is active; all unrelated power/lid
# triggerhappy actions remain untouched. If file bind mounts are unavailable,
# fall back to filtering only the four volume/brightness trigger lines.
install_input_routing() {
  if [ -s "$VOLUME_STUB" ] && [ -e "$VOLUME_TARGET" ]; then
    chmod 0755 "$VOLUME_STUB" 2>/dev/null || true
    if ! grep -q " $VOLUME_TARGET " /proc/mounts 2>/dev/null; then
      mount --bind "$VOLUME_STUB" "$VOLUME_TARGET" >/dev/null 2>&1 || true
    fi
    grep -q " $VOLUME_TARGET " /proc/mounts 2>/dev/null && return 0
  fi

  [ -r "$TRIGGER_CFG" ] || return 0
  grep -q '^# SNAP_FE_VOLUME_OWNER$' "$TRIGGER_CFG" 2>/dev/null && return 0
  [ -s "$TRIGGER_BACKUP" ] || cp -p "$TRIGGER_CFG" "$TRIGGER_BACKUP" 2>/dev/null || true
  tmp=/tmp/snapfe-triggerhappy.$$
  awk 'BEGIN{print "# SNAP_FE_VOLUME_OWNER"}
       $1 ~ /^KEY_VOLUMEUP($|\+)/ || $1 ~ /^KEY_VOLUMEDOWN($|\+)/ {next}
       {print}' "$TRIGGER_CFG" > "$tmp" || { rm -f "$tmp"; return 0; }
  if cp -f "$tmp" "$TRIGGER_CFG" 2>/dev/null; then
    for svc in /etc/init.d/S*triggerhappy; do
      [ -x "$svc" ] && { "$svc" restart >/dev/null 2>&1 || true; break; }
    done
  fi
  rm -f "$tmp"
}

restore_input_routing() {
  umount "$VOLUME_TARGET" >/dev/null 2>&1 || true
  if [ -s "$TRIGGER_BACKUP" ] && grep -q '^# SNAP_FE_VOLUME_OWNER$' "$TRIGGER_CFG" 2>/dev/null; then
    cp -f "$TRIGGER_BACKUP" "$TRIGGER_CFG" 2>/dev/null || true
    for svc in /etc/init.d/S*triggerhappy; do
      [ -x "$svc" ] && { "$svc" restart >/dev/null 2>&1 || true; break; }
    done
  fi
}

case "$1" in
  start|"")
    (
      # No sleep -- ES was just started, kill it before it renders. Hammer it
      # hard for the first ~2s (tight loop), then relax to a slow guard so a
      # tester never sees a frame of EmulationStation.
      kill_es
      hide_screen
      blank_fb
      install_link_cores
      install_input_routing
      ( for i in $(seq 1 40); do kill_es; sleep 0.05; done
        for i in $(seq 1 40); do kill_es; sleep 0.25; done ) &
      init_lid
      init_bluetooth &      # opt-in; runs in parallel, not on the critical path
      init_audio            # sequential -- audio must be routed before Snap FE
      while true; do
        kill_es
        run_snapos
        sleep 1             # Snap FE exited/crashed -> relaunch it, never ES
      done
    ) &
    ;;
  stop) restore_input_routing ;;
esac
