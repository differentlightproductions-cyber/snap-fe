# Snap FE

A frontend (menu / game launcher) for **Knulli** CFW on Anbernic handhelds,
developed on the RG SP (Allwinner H700). It runs *instead of* the
EmulationStation menu — Knulli still does everything under it (kernel, drivers,
controllers, audio, RetroArch + cores, BIOS, `emulatorlauncher`).

**Status: alpha.** It launches RetroArch / libretro games right now — not every
emulator or system Knulli supports. Artwork and themes are still being filled
in. Expect rough edges.

## What it is / what it isn't

- **A standalone program** (`snapos_ui`), written in C with SDL2. It is **not a
  fork** of EmulationStation or Knulli and does not patch, recompile, or modify
  either. It calls Knulli's own `emulatorlauncher` to start games — the same
  path ES uses.
- **It writes to exactly two places** on the device:
  - `/userdata/system/snapos/` — the binary, bundled assets, its `settings.cfg`
    and its art/scrape cache.
  - `/userdata/system/custom.sh` — the boot hook that starts it. If you already
    had a `custom.sh`, the installer copies it to `custom.sh.pre-snapos` first.
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

## Development

Snap FE is written and maintained by one person (**NICK.OFFICIAL**), with
AI-assisted tooling — the same way a lot of software is written now. Every line
is reviewed and understood by a human, and the point of publishing the source
is that you don't have to take that on faith: read it, build it, check what it
does. Issues and PRs welcome.

It's a single C translation unit — [`main.c`](main.c) (~14k lines) — plus:

| Path | What |
|---|---|
| [`main.c`](main.c) | The whole frontend. `-DSNAPOS_TARGET_KNULLI` retargets paths/launch for the device; without it, it builds for desktop Linux dev. |
| [`build-knulli.sh`](build-knulli.sh) | Cross-compile for aarch64 / Knulli. |
| [`knulli/`](knulli/) | Install + boot-hook + packaging scripts. [`knulli/custom.sh`](knulli/custom.sh) is the boot hook; [`knulli/README.md`](knulli/README.md) explains the takeover. |
| [`scrape_boxart.py`](scrape_boxart.py) | Optional box-art / metadata scraper (ScreenScraper / TheGamesDB). Needs your own API key/account. |
| [`assets/`](assets/) | Fonts (open-licensed — see [CREDITS.md](CREDITS.md)) and the built-in sounds. Wallpapers and console icons are **not** in this repo; `assets/*/README.txt` documents where they go and the app runs fine without them. |

See **[BUILD.md](BUILD.md)** for exact build + install steps.

## License

Code: MIT (see [LICENSE](LICENSE)). Bundled fonts keep their own licenses
(OFL 1.1 / Ubuntu Font License / DejaVu) — [CREDITS.md](CREDITS.md).
