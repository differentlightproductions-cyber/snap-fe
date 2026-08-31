# Snap FE

A frontend (menu / game launcher) for **Knulli** CFW on Anbernic handhelds,
developed on the RG SP (Allwinner H700). It runs *instead of* the
EmulationStation menu — Knulli still does everything under it (kernel, drivers,
controllers, audio, RetroArch + cores, BIOS, `emulatorlauncher`). This is just the behavior for the Alpha. At full launch, we plan to turn this into an actual fork of Knulli that has nothing to do with Emulation Station. 

**Current release: Alpha 1.2.0.** It launches RetroArch / libretro games right
now — not every emulator or system Knulli supports. Expect alpha rough edges.

### 1.2.0 stability and performance release

- Game handoff now releases SNAP's Mali renderer before Knulli configgen starts,
  removes leaked joystick references, and avoids redundant governor/config
  writes. On the RG34XX-SP test device, SNAP's measured press-to-launcher handoff
  is 69 ms; the remaining startup work belongs to Knulli and RetroArch.
- Knulli's configgen Python modules and RetroArch executable are warmed at low
  priority behind the branded boot intro, reducing the cold black-screen gap
  without bypassing Knulli's controller, core, save, shader, or hook setup.
- In-game brightness remains independent from volume, moves in exact 5% steps,
  and reaches the real panel's 1% floor. Readiness-based reassertion now works on
  the H700's Linux 4.9 kernel, and a bounded boot guard defeats late brightness
  resets without permanent polling.
- Devices affected by the prior RetroArch bridge's 800+ generated overrides are
  repaired automatically with an exact backup. User-owned portable RetroArch
  preferences and hotkeys remain intact; generated controller/path state does not
  leak back into global configuration.
- Post-game video recovery retries the complete window/renderer transaction and
  invalidates stale art caches, preventing corrupt carousel/bookshelf/library art.
  Wi-Fi status checks no longer fork shell pipelines, and duplicate Bluetooth
  agents are suppressed.
- The release package is flat for direct SHARE extraction, includes the complete
  bundled console art and helpers, rejects incompatible glibc builds, and excludes
  ROMs, saves, settings, accounts, and API keys.

### 1.1.9 Hotfix 2

- Restores Knulli's `udev` controller map for both games and the Home RetroArch
  app, eliminating the false “RG34XX-SP Controller not configured” message.
- Menu is again the RetroArch hotkey starter: **Menu + Select** opens the menu,
  **Menu + Start** exits, and **Menu + R2** remains fast-forward by default.
- Function + Volume brightness is now handled by Knulli's system key service,
  not RetroArch. It stays available in games, moves in exact 5% steps down to
  a true 1% floor, and cannot be broken by a RetroArch hotkey edit.
- Only portable RetroArch menu and notification preferences are shared between
  the Home app and games; controller, driver, path, and protected hotkey values
  are deliberately kept out of that bridge.

### 1.1.9 highlights

- Brightness and volume now stay under Snap FE control while a RetroArch game
  is running. Brightness moves in exact 5% steps down to a true 1% night floor,
  never wraps back to maximum, and both controls show native RetroArch notices.
- RetroArch's in-game menu defaults to **Menu + Select** and **Menu + Start**
  still exits to Snap FE. If a user changes the menu hotkey in RetroArch, Snap
  FE records and preserves that choice instead of overwriting it next launch.
- Syncthing controls live under System settings, and the Accounts area now has
  cleaner Scraping, RetroAchievements, and WallHaven groups. Successful
  RetroAchievements login reports `ON + Linked` and Achievements Book is an app.
- Surprise Me is centered and pre-cached for faster browsing. App Focused Home
  keeps its 4x2 page/row position across boots, moves apps between pages, and
  uses simpler monochrome placeholder icons.
- New RGSPink and Yellow/Gold themes, improved charging feedback, faster first
  pass through Carousel, correct Bookshelf game counts, expanded Breakout, and
  the new Connect 4 mini game are included.
- Boot now preloads libraries and artwork behind the themed particle intro;
  shutdown has a matching short animation. WallHaven provides optional SFW
  wallpaper search and downloads with an API key stored only on the device.

### 1.1.8 highlights

- App Focused Home is now a strict, paged 4x2 layout. Hold X to rearrange apps
  with the familiar jiggle mode, including movement between pages.
- The Game settings tab now contains Games Folder, Display Art, and Show
  Descriptions. Display Art refreshes the library only after Save and only when
  the saved art type actually changed.
- Battery and Night controls are separate collapsible groups, and all remaining
  test-build PC-key prompts have been replaced with handheld ABXY/D-pad labels.
- CPU Performance Mode has a visible `PERF` status badge, Top Bar underlines
  adapt to the selected backdrop color, and a Dark Purple theme is included.
- Link Play protects its controls footer and shows the seven or eight games that
  fit the current font size while keeping the full list scrollable.
- Block Roll now contains ten solvable stages. The Mini Games browser shows five
  legible games at a time and scrolls to the rest.
- The 1.1.7 in-game lid sleep/wake and post-game renderer-cache hotfixes remain
  included in this release.

### 1.1.7 hotfix

- Closing the RG34XX-SP lid during a RetroArch game now pauses the emulator,
  enters Snap FE deep rest, and resumes the same session when the lid opens.
- The lid watcher now follows the real AXP hall-sensor input device rather than
  the game-controller event node, so it remains active while Snap FE gives the
  display to RetroArch.
- Returning from a game now rebuilds cover-art, shadow, Home-widget, and app-grid
  GPU caches instead of occasionally showing corrupted or overlapping artwork.

### 1.1.6 highlights

- Favorites is now its own Home button and has an independent game-layout
  setting; Surprise Me navigation, rapid-input handling, and art layout were
  repaired.
- App Focused Home is a compact phone-style grid with bundled placeholder art
  and hold-X reordering.
- Link Play is organized by console and supports GB/GBC plus supported GBA
  cable/Wireless Adapter modes using packaged Gambatte and gpSP cores.
- Brightness persists into RetroArch games, scraping can be stopped, free
  system backgrounds can be browsed over Wi-Fi, and RetroAchievements has an
  achievements-book view.
- Night mode, power-save theme restoration, low-battery colors/icon, art
  shadows, Favorites card clipping, Radio layout, and build labeling were
  corrected.
- The release ZIP now extracts directly to the SHARE root, validates runtime
  compatibility before taking over, records useful install logs, and uses a
  glibc 2.34-compatible ARM64 binary for pinned Knulli releases.

## What it is / what it isn't

- **A standalone program** (`snapos_ui`), written in C with SDL2. It is **not a
  fork** of EmulationStation or Knulli and does not patch, recompile, or modify
  either. It calls Knulli's own `emulatorlauncher` to start games — the same
  path ES uses.
- **Its persistent files are confined to these locations** on the device:
  - `/userdata/system/snapos/` — the binary, bundled assets, its `settings.cfg`
    and its art/scrape cache.
  - `/userdata/system/custom.sh` — the boot hook that starts it. If you already
    had a `custom.sh`, the installer copies it to `custom.sh.pre-snapos` first.
  - `/userdata/roms/ports/Snap FE*.sh` — try/install/restore launchers shown by
    EmulationStation.
  - Link Play's packaged cores are kept under `system/snapos/cores/`; the boot
    hook backs up Knulli's stock gpSP/Gambatte cores and refreshes their runtime
    copies under `/usr/lib/libretro/`.
- **It never** repartitions, formats, runs `mkfs`/`dd`, or writes to a block
  device. It does not modify your ROMs, saves, save states, or BIOS.
- **Removing it** is deleting `/userdata/system/custom.sh` (or running the
  "Restore EmulationStation" port). Nothing else needs to be undone.
- **Network access** only happens when you use the feature that needs it, and
  only to these hosts:
  | Feature | Host |
  |---|---|
  | Weather | `wttr.in`, `ip-api.com` (approx location) |
  | Internet radio | `*.api.radio-browser.info` |
  | RetroAchievements | `retroachievements.org` |
| Box-art scraping (`scrape_boxart.py`) | `api.screenscraper.fr`, `api.thegamesdb.net` |
| Free background browser | `wallhaven.cc` (SFW-only search; optional personal API key) |
| Link Play | Other Snap FE devices on the same local network only |

## Install or update

Download the release asset named `SnapFE-Alpha-1.2.0.zip`—not GitHub's
automatically generated Source Code ZIP—and extract it **directly onto the
SHARE drive**. `SHARE/system/snapos/snapos_ui` and
`SHARE/roms/ports/Snap FE (Set As Default).sh` should then exist. Boot Knulli,
open Ports, and run **Snap FE (Set As Default)**. It validates the executable
and libraries, installs the hook atomically, and reboots automatically.

If a launcher returns to EmulationStation, send the files named
`loader-check.log`, `port-launch.log`, and `install.log` from
`SHARE/system/snapos/`; current packages no longer fail silently.

## Development

Snap FE is written and maintained by **NICK.OFFICIAL**, in collaboration with
AI-assisted coding tools. NICK.OFFICIAL directs the project and understands a
great deal of its code, while using AI to help research, implement, and review
areas that are still being learned. The source is published so anyone can read,
build, and check what it does. Issues and PRs welcome.

It's a single C translation unit — [`main.c`](main.c) (~14k lines) — plus:

| Path | What |
|---|---|
| [`main.c`](main.c) | The whole frontend. `-DSNAPOS_TARGET_KNULLI` retargets paths/launch for the device; without it, it builds for desktop Linux dev. |
| [`build-knulli.sh`](build-knulli.sh) | Cross-compile for aarch64 / Knulli. |
| [`knulli/`](knulli/) | Install + boot-hook + packaging scripts. [`knulli/custom.sh`](knulli/custom.sh) is the boot hook; [`knulli/README.md`](knulli/README.md) explains the takeover. |
| [`scrape_boxart.py`](scrape_boxart.py) | Optional box-art / metadata scraper (ScreenScraper / TheGamesDB). Needs your own API key/account. |
| [`background_browser.py`](background_browser.py) | Downloads optional system backgrounds selected in the on-device browser. |
| [`ra_achievements.py`](ra_achievements.py) | Fetches the signed-in user's unlocked RetroAchievements for the achievements book. |
| [`brightness-hotkey.sh`](brightness-hotkey.sh) | Gives Function + Volume a reliable 5% panel-brightness step in every app and game without modifying RetroArch bindings. |
| [`assets/`](assets/) | Open-licensed fonts, sounds, and Snap FE's generated Home placeholder icons. Other wallpaper/console-art slots are documented by the included README files. |

See **[BUILD.md](BUILD.md)** for exact build + install steps.

## License

Code: MIT (see [LICENSE](LICENSE)). Bundled fonts keep their own licenses
(OFL 1.1 / Ubuntu Font License / DejaVu) — [CREDITS.md](CREDITS.md).
