# Building Snap FE

Snap FE is one C file (`main.c`) linked against SDL2, SDL2_ttf and SDL2_image.
No build system — just a compiler. Two targets:

- **desktop** — for development on a Linux PC. Paths live under `~/snapos-ui/`.
- **Knulli** — the aarch64 build that runs on the handheld. Add
  `-DSNAPOS_TARGET_KNULLI`; paths become `/userdata/...` and game launch goes
  through Knulli's `emulatorlauncher`.

---

## Desktop build (development)

Dependencies (Debian/Ubuntu names):

```bash
sudo apt install build-essential libsdl2-dev libsdl2-ttf-dev libsdl2-image-dev
```

Build:

```bash
cc -O2 -Wall -Wno-unused-parameter \
   main.c -o snapos_ui \
   $(pkg-config --cflags --libs sdl2 SDL2_ttf SDL2_image) -lm
```

Run it from the repo directory:

```bash
./snapos_ui
```

It creates its config/cache under `~/snapos-ui/`. Put test ROMs in
`~/snapos-ui/roms/<system>/` (e.g. `~/snapos-ui/roms/gba/`). Launching a game
on desktop shells out to a system `retroarch` / `mgba` if you have one; the
menu itself doesn't need them.

---

## Knulli build (for the device)

Cross-compiles to `snapos_ui.aarch64`. You need an aarch64 GCC and the
device's own SDL shared objects + headers in a sysroot.

**1. aarch64 toolchain** — use Ubuntu's cross compiler for public releases. Its
glibc 2.35 baseline keeps the executable compatible with pinned Knulli builds:

```bash
sudo apt install gcc-aarch64-linux-gnu
```

**2. Sysroot** — pull the three SDL `.so`s off a running device over SSH and
grab matching headers (SSH must be enabled on the handheld; default password
`linux`):

```bash
./knulli/setup-sysroot.sh root@<device-ip>      # -> ~/knulli-sysroot
```

**3. Compile:**

```bash
./build-knulli.sh --sysroot ~/knulli-sysroot \
  --cc /usr/bin/aarch64-linux-gnu-gcc           # -> snapos_ui.aarch64
```

`build-knulli.sh` flags: `--cc <gcc>`, `--sysroot <dir>`, `--out <file>`.

**4. Fetch the official Link Play cores before packaging or SSH deployment:**

```bash
./knulli/setup-link-cores.sh
```

The script downloads the ARM64 gpSP and Gambatte builds from Libretro's
official buildbot and verifies both architecture and required link options.

The build script warns if a different toolchain introduces glibc 2.38+ symbols,
and the release packager refuses such a binary. Nothing is static-linked — the binary loads the device's own
`libSDL2 / _ttf / _image` (and their deps: freetype, png, z, …) at runtime.

---

## Install on the device

Any one of:

- **SD card:** run `./knulli/package.sh 1.1.8` to build
  `dist/SnapFE-Alpha-1.1.8.zip`, then extract the ZIP directly onto the card's
  SHARE partition. The archive root is already `system/` + `roms/`. Details + revert steps in
  `knulli/INSTALL.txt`.
- **Over SSH:** `./knulli/deploy.sh root@<device-ip>` — pushes the binary +
  assets + `custom.sh` and reboots.
- **On the device:** copy the tree to `/userdata/system/snapos/` and run
  `knulli/install.sh` there.

## Revert

Delete `/userdata/system/custom.sh` (your previous one, if any, is at
`custom.sh.pre-snapos`), or run the **"Snap FE (Restore EmulationStation)"**
port, or `knulli/uninstall.sh`. EmulationStation runs again on the next boot.

## What the boot hook does

`knulli/custom.sh` → `/userdata/system/custom.sh`, run near the end of boot:
stops the EmulationStation service before it draws, blanks the framebuffer,
brings up the audio session (pipewire + wireplumber), hands lid control to
Snap FE, then runs `snapos_ui` in a relaunch loop. It touches nothing outside
`/userdata/system/`. Full walkthrough in `knulli/README.md`.
