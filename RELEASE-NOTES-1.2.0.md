# Snap FE Alpha 1.2.0

This is a stability and launch-performance release for Knulli H700 handhelds,
tested on the Anbernic RG34XX-SP. It consolidates the 1.1.9 hotfix work and
repairs the regressions found during a full device/configuration audit.

## Launch and responsiveness

- SNAP now releases its Mali renderer before starting Knulli configgen. The
  measured frontend handoff on the test device is 69 ms, including 46 ms to
  release video; the remaining interval is Knulli controller/core generation
  and RetroArch initialization.
- Configgen's Python modules and the RetroArch executable are prewarmed at low
  priority during SNAP's branded intro. Knulli's normal per-game controller,
  save, shader, core, and hook generation remains enabled.
- Redundant CPU-governor writes, recurring Wi-Fi shell pipelines, leaked SDL
  joystick references, and repeated unchanged configuration writes were removed.

## Brightness, volume, and controls

- Menu + Volume changes brightness only; Volume by itself changes volume only.
- Brightness uses exact 5% transitions with a real 1% hardware floor and no
  wrap-to-maximum behavior.
- A readiness-based guard preserves the selected level through Knulli and
  RetroArch display initialization. Its process detection supports the H700's
  Linux 4.9 kernel, then stops completely after launch.
- A separate bounded startup guard prevents a late Knulli boot service from
  restoring an older brightness value. There is no permanent brightness poll.
- Knulli's udev controller mapping and user-edited RetroArch hotkeys/preferences
  are preserved, while SNAP's protected brightness chord remains system-owned.

## Configuration and recovery

- Installs polluted by the earlier settings bridge (more than 256 generated
  `global.retroarch.*` entries) are compacted automatically. The exact original
  is retained as `/userdata/system/knulli.conf.snapfe-pre-cleanup`.
- Settings and Knulli configuration updates use checked temporary files and
  same-directory replacement instead of exposing partial writes.
- Returning from a game retries the complete SDL video transaction and clears
  stale renderer-owned art references, preventing corrupted carousel,
  bookshelf, list, and library artwork.
- Duplicate Bluetooth agents are prevented, and boot-time core copies occur
  only when their contents differ.

## Install/update

Download `SnapFE-Alpha-1.2.0.zip` from this release—not GitHub's automatic
Source Code archives—and extract it directly onto the Knulli SHARE partition.
Then open Ports and run **Snap FE (Set As Default)**.

The archive contains no ROMs, saves, account credentials, settings, or API keys.
It includes the ARM64 executable, install/restore Ports, helpers, Link Play
cores, fonts, sounds, backgrounds, and bundled console-view artwork.
