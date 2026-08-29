# Snap FE for Knulli (Anbernic RG34XX-SP / Allwinner H700)

Snap FE is a **frontend replacement** for EmulationStation. Knulli keeps doing
the hard parts — kernel, Mali GPU, controller, audio, battery, RetroArch + its
cores, BIOS — Snap FE is only the menu/launcher on top. No custom kernel, no
reflashing.

## What it is / what it isn't

- **It is a separate, standalone program** (`snapos_ui`) — an alternative menu.
  It is **not a fork** of EmulationStation or Knulli, and it does not patch,
  recompile or modify either. "Fork" would mean continuing someone's source;
  this is its own program that runs *instead of* the ES menu.
- **Knulli does all the real work.** Snap FE shells out to Knulli's own
  `emulatorlauncher` / configgen to start games — same path ES uses.
- **It only writes to its own two locations:** `/userdata/system/snapos/`
  (the binary, assets, its config + cache) and `/userdata/system/custom.sh`
  (the boot hook). It never repartitions, formats, or writes to your ROMs,
  saves, states or BIOS.
- **EmulationStation is left fully intact.** Restoring it is deleting one file
  (`/userdata/system/custom.sh`) or running the "Restore EmulationStation"
  port. Your old `custom.sh`, if you had one, is saved next to it as
  `custom.sh.pre-snapos`.
- **Alpha.** Right now it launches RetroArch / libretro games only — not every
  emulator/system Knulli supports. System artwork and themes are still being
  filled in.

## For users — install the alpha

Download `dist/SnapFE-Alpha-<version>.zip`, then:

1. Power off, put the SD card in your computer, open the **SHARE** partition
   (that's `/userdata` on the device).
2. Copy the `system/` and `roms/` folders from the zip into the root of SHARE
   (merge — it only adds files).
3. Eject, reboot. Under **Ports** in EmulationStation you now have:
   * **Snap FE** — launch to try it; quit to return to ES (nothing else changes)
   * **Snap FE (Set As Default)** — make it the boot frontend, then reboot
   * **Snap FE (Restore EmulationStation)** — undo that

Full steps + uninstall are in `INSTALL.txt` inside the zip. No card removal
needed if you have SSH or the network share — see INSTALL.txt.

## For developers — build + package

```bash
# 1. one-time: pull the device's SDL .so + headers into a sysroot
./knulli/setup-sysroot.sh                       # device password once ("linux")

# 2. cross-compile the aarch64 binary
./build-knulli.sh --sysroot ~/knulli-sysroot    # -> snapos_ui.aarch64

# 3a. iterate on a device over SSH
./knulli/deploy.sh root@<device-ip>             # push binary + assets + custom.sh, then reboot

# 3b. cut a release zip
./knulli/package.sh                             # -> dist/SnapFE-Alpha-<version>.zip
```

Toolchain: `build-knulli.sh` auto-uses
`~/buildroot/output/host/bin/aarch64-buildroot-linux-gnu-gcc`, falling back to
`aarch64-linux-gnu-gcc`.

### What `-DSNAPOS_TARGET_KNULLI` changes

Every path in `main.c` is routed through one helper; the flag flips them:

| | Desktop | Knulli |
|---|---|---|
| settings / boxart / config | `~/snapos-ui/` | `/userdata/system/snapos/` |
| ROMs | `~/snapos-ui/roms/<sys>/` | `/userdata/roms/<sys>/` |
| game launch | bundled mGBA / RetroArch | `/usr/bin/emulatorlauncher` (configgen) |

It also sets `reduce_motion` on by default and renders at the panel's real
resolution.

## How the frontend takeover works

`custom.sh` (installed as `/userdata/system/custom.sh`, or bundled as
`snapos-custom.sh` for the "Set As Default" port) is run near the end of boot.
It stops EmulationStation before it draws, wipes the framebuffer, brings up
the audio session (pipewire + **wireplumber** — Knulli's `S06audio` skips the
latter), sets the lid to Snap-FE control, then runs Snap FE in a relaunch loop.
ES is never started again until you restore it. It touches nothing outside
`/userdata/system/` — no partitions, no ROMs, no saves.

## Known-good target

Knulli (Batocera 42 base), `retroarch 1.22.2`, `libSDL2 2.32.8` /
`_ttf 2.24.0` / `_image 2.8.5`, aarch64. All shipped by Knulli — nothing is
static-linked.

## Not done yet

- **GBA Link Play** — needs a standalone VBA-M-SDL build (no libretro core has
  GBA link cable over network). GB/GBC link works now via gambatte.
- Full custom `.img` ("Snap FE Edition" of Knulli) — a real image means a
  Buildroot package + OS build.
- Parse `es_systems.cfg` for every console instead of the 6-system table.
