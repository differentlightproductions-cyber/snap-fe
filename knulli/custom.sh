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
VOLUME_GATE=/userdata/system/snapos/volume-gate.sh
MODIFIER_STATE=/tmp/snapfe-brightness-modifier
TRIGGER_CFG=/etc/triggerhappy/triggers.d/multimedia_keys.conf
TRIGGER_BACKUP=/userdata/system/snapos/triggerhappy-multimedia_keys.stock
TRIGGER_OVERLAY=/userdata/system/snapos/triggerhappy-multimedia_keys.snapfe

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
    # Root is an overlay on these images; rewriting an identical multi-MB core
    # every boot wastes I/O and extends the black startup gap.
    if ! cmp -s "$SNAPOS_CORES/$core" "$SYSTEM_CORES/$core"; then
      install -m 0755 "$SNAPOS_CORES/$core" "$SYSTEM_CORES/$core"
    fi
  done
}

kill_es() {
  /etc/init.d/S31emulationstation stop >/dev/null 2>&1
  killall -q -9 emulationstation emulationstation-standalone emulationstation.sh 2>/dev/null
}

# Blank every framebuffer + console right now, so nothing ES drew (or the boot
# console text) is visible in the gap before Snap FE takes the screen.
hide_screen() {
  # 720x480x32bpp is ~1.4 MiB; two MiB covers the whole panel without pushing
  # 16 MiB through each framebuffer character device at every boot.
  for fb in /dev/fb0 /dev/fb1; do [ -c "$fb" ] && dd if=/dev/zero of="$fb" bs=1M count=2 2>/dev/null; done
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

# Clamshell: let Snap FE own the lid (screen off on close, on on open) instead
# of Knulli's lid-control doing a real suspend whose wake path is broken when
# ES isn't running. Set back to "suspend" if you prefer true suspend-to-RAM.
init_lid() {
  [ "$(knulli-settings-get system.lid 2>/dev/null)" = "none" ] ||
    knulli-settings-set system.lid none >/dev/null 2>&1
}

# Bluetooth: Knulli's S32bluetooth brings up the radio + bluetoothd + pairing
# agent, but only if the user has enabled it. It's normally kicked by the ES
# session; do it here. Opt-in -- BT off = no extra battery drain.
init_bluetooth() {
  [ "$(knulli-settings-get controllers.bluetooth.enabled 2>/dev/null)" = "1" ] || return 0
  # custom.sh and S32bluetooth can overlap late in boot. Give the stock service
  # a moment to win the race, start it only if it did not, then keep the oldest
  # agent if this firmware nevertheless spawned a duplicate.
  sleep 2
  if ! pgrep -f '[/]usr/bin/knulli-bluetooth-agent' >/dev/null 2>&1; then
    /etc/init.d/S29namebluetooth start >/dev/null 2>&1
    /etc/init.d/S32bluetooth start     >/dev/null 2>&1
    sleep 1
  fi
  keep=""
  for pid in $(pgrep -f '[/]usr/bin/knulli-bluetooth-agent' 2>/dev/null); do
    if [ -z "$keep" ]; then keep="$pid"; else kill "$pid" >/dev/null 2>&1 || true; fi
  done
}

# Configgen imports a sizeable Python module tree before every emulator start.
# Populate the reclaimable Linux page cache once, at low CPU/I/O priority,
# while SNAP is showing its boot quote and loading libraries/art. Nothing stays
# resident and no system file is modified; the first game simply avoids cold SD
# reads for the same modules and RetroArch executable.
prewarm_game_launcher() {
  (
    command -v ionice >/dev/null 2>&1 && IO="ionice -c 3" || IO=""
    $IO nice -n 15 /usr/bin/python -c 'import configgen.emulatorlauncher' >/dev/null 2>&1 || true
    # Configgen lazily imports generator modules only after it knows the chosen
    # system. Read those small files sequentially now instead of paying hundreds
    # of cold SD metadata reads on the black handoff screen.
    for root in /usr/lib/python*/site-packages/configgen /usr/lib/python*/configgen; do
      [ -d "$root" ] || continue
      $IO nice -n 15 find "$root" -type f -size -2M -exec cat '{}' + >/dev/null 2>&1 || true
    done
    for file in /usr/bin/retroarch \
      /usr/lib/libretro/mgba_libretro.so /usr/lib/libretro/gpsp_libretro.so \
      /usr/lib/libretro/gambatte_libretro.so /usr/lib/libretro/snes9x_libretro.so \
      /usr/lib/libretro/fceumm_libretro.so /usr/lib/libretro/genesis_plus_gx_libretro.so \
      /usr/lib/libretro/pcsx_rearmed_libretro.so /usr/lib/libretro/fbneo_libretro.so \
      /userdata/system/knulli.conf /userdata/system/configs/emulationstation/es_input.cfg; do
      [ -r "$file" ] && $IO nice -n 15 dd if="$file" of=/dev/null bs=256K 2>/dev/null || true
    done
  ) &
}

run_snapos() {
  cd /userdata/system/snapos || return 1
  if [ -x "$SNAPOS" ]; then "$SNAPOS"; else "$LOADER" "$SNAPOS"; fi
}

# Keep Knulli's normal Volume controls intact, but route their press through a
# tiny gate while SNAP owns a game. SNAP's native evdev reader sees Menu and
# Volume on their separate kernel devices, changes brightness itself, and marks
# MODIFIER_STATE so triggerhappy does not also change volume for that chord.
install_input_routing() {
  [ -r "$TRIGGER_CFG" ] || return 0
  [ -x "$VOLUME_GATE" ] || return 0
  # A bind mount from an in-place update may still be active. Drop it first,
  # then refresh the base from this firmware's real stock rules. Keeping one
  # forever across Knulli updates silently resurrected obsolete controller
  # mappings and was a source of "controller not configured" regressions.
  umount "$TRIGGER_CFG" >/dev/null 2>&1 || true
  cp -p "$TRIGGER_CFG" "$TRIGGER_BACKUP.tmp" 2>/dev/null || return 0
  mv -f "$TRIGGER_BACKUP.tmp" "$TRIGGER_BACKUP" || return 0
  printf '0\n' > "$MODIFIER_STATE"
  awk -v gate="$VOLUME_GATE" '
       $1 ~ /^KEY_VOLUME(UP|DOWN)\+(BTN_TL2|BTN_MODE|KEY_GOTO)$/ {next}
       $1 == "KEY_VOLUMEUP" && $2 == "1"   {print "KEY_VOLUMEUP 1 " gate " volup"; next}
       $1 == "KEY_VOLUMEDOWN" && $2 == "1" {print "KEY_VOLUMEDOWN 1 " gate " voldown"; next}
       {print}' "$TRIGGER_BACKUP" > "$TRIGGER_OVERLAY" || return 0
  mount --bind "$TRIGGER_OVERLAY" "$TRIGGER_CFG" >/dev/null 2>&1 || return 0
  for svc in /etc/init.d/S*triggerhappy; do
    [ -x "$svc" ] && { "$svc" restart >/dev/null 2>&1 || true; break; }
  done
}

restore_input_routing() {
  umount "$TRIGGER_CFG" >/dev/null 2>&1 || true
  rm -f "$TRIGGER_OVERLAY"
}

case "$1" in
  start|"")
    (
      # ES was just started, so stop it once before it renders. Keep a short
      # conditional guard for a late service restart, but do not call the full
      # init script 80 times: that process storm competed with SNAP's boot art
      # preload (and with a game launched immediately after boot) on the H700.
      kill_es
      hide_screen
      install_link_cores
      install_input_routing
      ( for i in $(seq 1 20); do
          if pidof emulationstation emulationstation-standalone emulationstation.sh >/dev/null 2>&1; then
            kill_es
          fi
          sleep 0.25
        done ) &
      init_lid
      init_bluetooth &      # opt-in; runs in parallel, not on the critical path
      init_audio            # sequential -- audio must be routed before Snap FE
      prewarm_game_launcher # low-priority work overlaps SNAP's branded intro
      while true; do
        kill_es
        run_snapos
        sleep 1             # Snap FE exited/crashed -> relaunch it, never ES
      done
    ) &
    ;;
  stop) restore_input_routing ;;
esac
