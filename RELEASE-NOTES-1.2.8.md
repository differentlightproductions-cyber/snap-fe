# Snap FE Alpha 1.2.8

Snap FE now draws on top of a running game, every system has carousel art, and
several settings rows that looked wrong or behaved badly are fixed.

## The overlay survives the game

The performance overlay used to vanish the moment a game started. Everything
Snap FE draws goes through SDL, and SDL's video device belongs to the emulator
for as long as one is running — so there was nothing left to draw with.

The panel is not a single image, though. The display engine on these handhelds
composites six layers, and the framebuffer that Snap FE, RetroArch and mGBA all
end up drawing into is only one of them. Snap FE now takes layers of its own
above it and scans out its own picture there. Nothing is asked of the emulator,
and none of this goes through RetroArch's notification bar.

- **Performance Overlay** stays up for the whole session: CPU, temperature, RAM,
  battery and charge. There is deliberately no FPS line — the frame rate on
  screen belongs to the emulator, and it reports that to nothing but its own
  on-screen notifications.
- **Volume and brightness** get a bar along the bottom when you change them
  in-game. Snap FE still does not *drive* either one during play — Knulli's own
  volume helper and the Fn+Volume brightness helper do, exactly as before — it
  reads the level each of them settles on and shows it.
- If Snap FE ever dies with an overlay up, the next launch clears it.

## Carousel art for every system

The Carousel had art for fourteen systems and a name card for the other
thirty-nine. All 53 now have a drawn card — Atari 2600 through 7800, the 800 and
ST, Amiga, Amstrad, C64, MSX, ZX Spectrum, Master System, SG-1000, Mega CD,
32X, Saturn, Dreamcast, PlayStation, PSP, Neo Geo CD, PC Engine CD, SuperGrafx,
Neo Geo Pocket and Color, WonderSwan and Color, Virtual Boy, Pokémon Mini,
Supervision, Vectrex, Intellivision, ColecoVision, Lynx, Famicom Disk System,
Game & Watch, MAME, ScummVM, DOS, Cave Story, Doom, Quake and Quake II.

Grid, List and Bookshelf still fall back to name cards for the systems they have
no art for; only the Carousel is complete.

## Settings

- **A on a value row dropped you out of Settings.** Every Display row that is
  only a Left/Right wheel fell past the handler chain into the catch-all that
  leaves the page, so pressing A on Theme, Brightness or Font Size threw you out
  of the screen. A now nudges the value forward, the same as Right.
- **"Library View: Follow Systems View"** read as a second copy of the row above
  it, and opening it showed an empty dropdown unless the layout was Grid. It
  names its own layout now, tagged `(Auto)` while it is following, and only
  offers to open when there is something underneath it.
- **Display Art** moves from Game to Display, under Library View. It decides how
  the library looks, so it belongs beside the layout that arranges it. Its
  "Favorites only" note no longer runs off the edge of the row.
- **Performance Overlay** moves from Display to **Device**, next to CPU
  Performance Mode. It reports on the handheld, not on how the UI looks. Its
  opacity and text colour follow it, and Device's Restore to Default owns them.
- The build tag in the corner no longer draws over the last row of a full list.

## Backgrounds

**Search Backgrounds Online** is a row on the Backgrounds page. The Wallhaven
browser was already there, but the only way in was to open one system's picker
first; now you can search from the page itself and choose which system gets the
result afterwards.

## Grids page sideways

In a grid, Left and Right at the edge of a row dropped you onto the row below
instead of moving on. They now go to the same row of the next page, which is
what the page indicator has been promising all along. Fixed in both the systems
grid and the game library.

## Known limits

- **Bookshelf** as a *library* layout is still not implemented; it falls back to
  the grid. Bookshelf as a *Systems* view is unaffected.
- While a Link Play partner is inside a game their pairing socket is down, so
  they show as being in a game but cannot be joined until they exit.
