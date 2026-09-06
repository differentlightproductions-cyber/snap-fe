# Computer systems: controls and BIOS

SNAP uses Knulli's emulator launcher and controller generator, the same route
used by EmulationStation. Keep the firmware's controller profile; replacing it
with one universal computer-game map can break consoles and hotkeys.

On a system, press **Select**, then open **Controls & BIOS**. This is read-only
help, not an automatic remap. Your saved RetroArch core/game remaps take priority.
The per-system core selector now explicitly passes a chosen core to Knulli;
Default continues to use the firmware's choice.

## The easier diagnostic route

Run `tools/audit_system_controls.py` with Python on the device. It reports the
firmware's resolved emulator/core, built-in controller profiles, relevant input
overrides and BIOS file presence for installed computer systems. It does not
change settings, launch games, download firmware or print account credentials.
Presence is not a checksum validation; optional BIOS is not automatically a
launch requirement. This avoids having to describe every failing game first.

If controls fail, check these in order:

1. Required BIOS and the machine model expected by the game.
2. The game's startup menu: trainers and disk loaders often need keyboard keys.
3. Joystick versus mouse mode and joystick port 1 versus port 2.
4. The core's controller device type, then any saved game/core remaps.

Open RetroArch with your configured menu shortcut (SNAP default: Menu + Select).
Under Quick Menu, save button changes with Controls > Manage Remap Files >
Save Game Remap File. Save machine/joystick options with Core Options > Manage
Core Options > Save Game Options File. Exact labels vary with core versions.

## Common cases

- **Amiga / PUAE:** Select opens the virtual keyboard; its J/M switch enables
  joystick/mouse control. Knulli maps L2/R2 to mouse clicks. AROS fallback can
  start some games without Kickstart, but it does not provide full compatibility.
  Put legally obtained Kickstart files in `SHARE/bios/amiga/`.
- **Atari 800:** Y opens its keyboard, A fires, B is Return. Use the ATARI
  Joystick device. Atari OS/BASIC files and the selected machine model matter;
  a fallback boot is not proof that the correct BIOS is installed.
- **C64 / VICE:** Select opens its keyboard. JOYP switches the joystick port;
  many apparent controller failures are the wrong port. Standard firmware is
  included with VICE; JiffyDOS is optional.
- **Spectrum / Fuse:** Select opens its keyboard. Choose a matching joystick
  type in the game menu and the core (for example, Kempston).
- **Amstrad CPC:** disk games may stop at a BASIC prompt or keyboard menu.
  Knulli's cap32 mapping uses the Amstrad joystick device.
- **Atari ST / Hatari:** needs a compatible `SHARE/bios/tos.img`. Games may
  expect mouse, joystick or keyboard input; one mapping cannot replace all three.
- **MSX / blueMSX:** preserve the `Machines` and `Databases` directory structure
  within `SHARE/bios/`. Keyboard startup commands and joystick port matter.

Do not download BIOS from an untrusted collection or rename arbitrary firmware
to satisfy a missing-file warning. This release does not distribute BIOS files.

## References

- [PUAE](https://docs.libretro.com/library/puae/)
- [Atari800](https://docs.libretro.com/library/atari800/)
- [VICE](https://docs.libretro.com/library/vice/)
- [Fuse](https://docs.libretro.com/library/fuse/)
- [Caprice32](https://docs.libretro.com/library/caprice32/)
- [Hatari](https://docs.libretro.com/library/hatari/)
- [blueMSX](https://docs.libretro.com/library/bluemsx/)

Device checks in this batch confirm the firmware generator and installed core
configuration, not playability of every title. Missing BIOS must still be
provided by its owner; game-specific keyboard/mouse requirements remain.
