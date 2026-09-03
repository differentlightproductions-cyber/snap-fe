# Snap FE

A frontend (menu / game launcher) for **Knulli** CFW on Anbernic handhelds,
developed on the RG SP (Allwinner H700). It runs *instead of* the
EmulationStation menu — Knulli still does everything under it (kernel, drivers,
controllers, audio, RetroArch + cores, BIOS, `emulatorlauncher`). This is just the behavior for the Alpha. At full launch, we plan to turn this into an actual fork of Knulli that has nothing to do with Emulation Station. 

**Current release: Alpha 1.2.6.** Not every emulator or system Knulli supports
is wired up yet. Expect alpha rough edges.

### 1.2.6 highlights

- **Both SD cards are visible now.** Knulli mounts exactly one card as its data
  drive and leaves the other completely unmounted, so a second card of games
  could not be reached by any frontend. Snap FE mounts it at startup, adds its
  games to the library, and tells the two apart as TF1 and TF2 (the card
  carrying `/boot` is TF1, whichever device node it came up as).
- **Settings > Games > Storage & Games Folders** — a card list with size, mount
  point and game count, and per-card actions: use for games, check folders,
  create missing folders, browse, mount, eject. A blank card gets its per-system
  folders created by copying the names your main card already uses, so Knulli's
  `megadrive/` never ends up shadowed by an empty `genesis/`.
- The first-run wizard can set a second card up in one press.
- **Link Play saw only the first card** and so could not list games that the
  library launched fine. It now scans every ROM folder. The scraper had the same
  bug and now scrapes all of them.
- **Link Play sessions outlive the game.** Quitting no longer ends the session
  for both sides: discovery stays up through a game, your partner shows as being
  in a game, and you land back in the lobby ready to start another. Leaving Link
  Play is what ends it.
- **In-game brightness and the Menu button work on more handhelds.** Both were
  bound to button numbers measured on one specific device. The pad is now
  resolved from its evdev keycodes, which are the same across these handhelds
  even when the button *indices* are not.
- **Full panel brightness.** 100% was mapped to 200 of the panel's 0-255 PWM
  range, so about a fifth of it was unreachable. The flashlight was capped the
  same way.
- **Fix Game Titles** (Settings > Scraping, on by default) uses the name the
  scraper matched instead of the filename, in the library, favourites and Link
  Play. The name is saved during a normal scrape at no extra API cost.
- Wi-Fi shows whether you are connected in a banner under the header. The old
  fixed-height network list ran into the status line and the button hint.
- **RG35XX-SP** added to the handheld list, and that setup step is now one
  cycling row instead of a line per model.
- Turning the backlight to Off no longer survives a reboot — the handheld used
  to come back up rendering correctly behind a dark panel, with no way to see
  the menu that would turn it back on.

### 1.2.5 highlights

- **53 systems, up from 14** — handhelds, 8/16-bit consoles, CD systems, home
  computers, MAME, ScummVM, DOS and standalone game ports. Systems better served
  by a standalone emulator (PSP, Dreamcast, Saturn, MAME, ScummVM, DOS) let
  Knulli's `emulatorlauncher` pick it.
- Ten libretro core filenames never matched what Knulli ships, including the
  defaults for SNES, N64, Atari 2600, Genesis, Master System, Game Gear and PC
  Engine — which also made the per-system Core override do nothing on them.
- **Games List** and **Games Carousel**: opening a system now mirrors the
  Systems View you picked it from, the carousel showing games as cartridges.
- **Cartridge art** is scrapeable (ScreenScraper `support-2D`/`3D`). Games
  without it get one drawn — borrowing the outline of a real one where the
  system has any, and a disc for CD systems.
- Delete Game opened the search keyboard instead of confirming; X did not sort
  inside a single system's library; `gamelist.xml` names and hidden entries were
  ignored. All fixed.
- Brightness can reach **Off** again, and Display Art says when the active view
  supplies its own art.
- "Console View" is now "Systems View".

### 1.2.4 App icon packs and boot quote cleanup

- App Focused Home now ships two organized icon packs: the new **Simple** pack
  and the preserved **Pixel Art** artwork from 1.2.3.
- Switch packs under **Settings > Display > Home > App Icon Pack**. The choice
  is saved, incomplete packs fall back safely, and Simple icons adapt to the
  active theme's text color for reliable contrast.
- Simple icons use individually balanced artwork sizing without changing the
  4x2 tile layout, selection outline, labels, or app movement.
- Boot quotes are limited to self-contained, attributed moments from notable
  GDC and E3 presentations instead of contextless interview fragments.

### 1.2.3 App Focused icon refresh

- App Focused Home now draws each transparent icon on a rounded tile generated
  from the active color theme, so the same artwork fits every theme.
- The selected app uses a tight three-pixel accent outline with no extra border
  baked into the artwork.
- The complete transparent icon set is refreshed, including Favorites,
  RetroArch, Radio, Mini Games, Achievements, Calculator, Library, Link Play,
  Music, Settings, Consoles, Flashlight, and Surprise Me.

### 1.2.2 Home widgets and launch-time update

- App Focused Home now uses more of the screen, keeps a true 4x2 layout across
  pages, ships the latest transparent app icons, and supports L1/R1 page changes.
- X adds or changes a separate movable two-cell app widget, while A refreshes
  its Weather card and Y enters app movement. Weather, Clock, Battery Health,
  and a current-week Calendar are available without changing the two
  Informational Home widget selections.
- The weekly Calendar widget opens into a full Calendar app and always returns
  to the current month on the next visit. D-pad selects a day, L1/R1 changes
  months, and A adds or edits a saved reminder. Reminders can be all-day or use
  a specific five-minute time, and appear on Home when due if the Calendar
  widget is active. Calculator is a new app in both Home layouts.
- Radio Tuner uses L1/L2 for previous/next and Select for play/stop. L1/R1 and
  L2/R2 otherwise cycle the two Informational widget slots in both directions,
  with their guidance aligned to the cards. Battery Health displays real health,
  voltage and temperature data exposed by the device.
- Auto-Sleep now enters the low-power display/radio/CPU rest path, and explicit
  aspect-ratio choices now reach Knulli's generated RetroArch game configuration.
- First-time setup includes a physical-button and built-in-hotkey guide.
- The newest game-launch repair removes unused configgen output, avoids parsing
  a byte-identical duplicate controller database, and boosts only the configgen
  window. The RG SP diagnostic improved from about 9.262s to 2.834s while the
  working in-game brightness path and 1% floor remain intact.

### 1.2.1 interface and control update

- Selected games have a clearer artwork outline, and missing artwork now uses a
  clean `NO ART` placeholder instead of an empty tile.
- Home widgets can be changed live with R1/R2. Recently Played is available as
  a widget and shows games 2–4 because the newest game remains under HOME.
- Up to three ROM folders can be selected and rescanned, including during first
  setup. Per-system settings now open with Select.
- SNAP-only Menu double-tap and triple-tap actions are programmable without
  altering RetroArch hotkeys.
- Settings dropdowns collapse after leaving Settings. Unsaved Hotkey changes
  now show an explicit Save, Discard, or Cancel prompt.

See [UPDATING.md](UPDATING.md) for the safe in-place update procedure. Updating
does not erase settings, favorites, ROMs, saves, scraped artwork, or accounts.

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

**Boot Knulli on the handheld once before you start.** A freshly flashed card
has no SHARE drive — Knulli creates it on first boot, growing the partition to
fill the card and formatting it exFAT so your computer can read it. Until then
a PC sees only unreadable Linux partitions.

Then download the release asset named `SnapFE-Alpha-1.2.6.zip`—not GitHub's
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
