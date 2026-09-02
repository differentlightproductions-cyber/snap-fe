# Snap FE Alpha 1.2.5

This update takes the system list from 14 to 53, gives the library two new
browsing layouts, and fixes a set of defects that had been quietly wrong for
several releases.

## Systems

- **53 systems, up from 14.** Virtual Boy, WonderSwan and Color, Neo Geo Pocket
  and Color, Lynx, Atari 5200/7800/800, ColecoVision, Intellivision, Vectrex,
  MSX1/2, ZX Spectrum, Amstrad CPC, Commodore 64, SG-1000, Famicom Disk System,
  Pokémon Mini, Game & Watch, Supervision, SuperGrafx, Sega CD, 32X, PC Engine
  CD, Neo Geo CD, PSP, Dreamcast, Saturn, Amiga, Atari ST, MAME, ScummVM, DOS,
  Doom, Quake, Quake II and Cave Story.
- Snap FE was never limited to RetroArch. Systems that are better served by a
  standalone emulator — PSP, Dreamcast, Saturn, MAME, ScummVM, DOS — now let
  Knulli's `emulatorlauncher` choose it.
- **Ten core filenames never matched what Knulli ships**, including the
  defaults for SNES, N64, Atari 2600, Genesis, Master System, Game Gear and PC
  Engine. Those systems still launched because `emulatorlauncher` falls back on
  an unknown core, which also meant the per-system **Core** override silently
  did nothing on them. All corrected.
- Console art is now loaded only for systems that are actually shown, instead
  of all 53 on every cold load.

## Library

- **Games List.** Opening a system while Systems View is List now lists its
  games in a matching layout: titles down the left, and a panel on the right
  with artwork, a themed rule, and the write-up. L1/R1 step the artwork, L2/R2
  scroll the text.
- **Games Carousel.** Opening a system while Systems View is Carousel fans the
  games out as cartridges.
- **Cartridge art.** A new scrapeable type, mapped to ScreenScraper's
  `support-2D` / `support-3D`. TheGamesDB has no equivalent.
- Games with no cartridge art get one drawn for them. If any game in a system
  has real cartridge art, its outline becomes that system's template; otherwise
  one of about thirty built-in shapes is used. **Disc systems get a disc**, not
  a cartridge.
- The highlighted game's screenshot is used as a backdrop, falling back to its
  title screen.
- Systems with no artwork at all now show a designed stand-in rather than a
  blank slot or a floating label.

## Fixes

- **Delete Game opened the search keyboard** instead of asking to confirm,
  anywhere except All Games view. The panel and its input handler disagreed
  about how many rows it had.
- **X did not sort inside a single system's library** — sorting had been
  limited to All Games.
- Titles and hidden entries from `gamelist.xml` are now honoured, so games read
  "Doom" and "Fix-It Felix Jr." rather than `doom1_shareware` and
  `fix_it_felix_64`, and PrBoom's engine file no longer appears as a game.
- **Brightness can reach Off again.** The floor had been raised so the panel's
  own minimum was unreachable; the ladder is 10, 5, 1, Off and back.
- Display Art now says when the active Systems View supplies its own art and
  the setting is only affecting Favorites.
- Scrape Art Types lists only what the selected source can serve.
- Aspect Ratio asks before it sticks, previewing the proportions and reverting
  after 15 seconds unless confirmed.
- "Console View" is now "Systems View".
- Doom and Commodore 64 have artwork.
- The desktop development build compiles again on SDL_image older than 2.6.

## Updating

Download `SnapFE-Alpha-1.2.5.zip`, extract it directly onto the Knulli `SHARE`
partition with Merge/Replace enabled, then run **Snap FE (Set As Default)** from
Ports once. Do not delete the existing `system/snapos` folder first.

The package does not contain or remove user settings, favorites, ROMs, saves,
save states, BIOS files, accounts, API keys, or scraped game artwork.
