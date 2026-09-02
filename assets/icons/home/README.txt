"App Focused" Home icon packs
(Settings > Display > Home > App Icon Pack)

Bundled packs:

  simple/       Lightweight line icons. SVG is loaded first and rendered at
                the requested runtime size; PNG remains a fallback. The result
                is automatically tinted for each theme.
  pixel-art/    The full-color Pixel Art set shipped with Snap FE 1.2.3.

The Simple SVG symbols are adapted from Lucide and distributed under its ISC
license. See LICENSE-lucide.txt in this folder. Their shared 24x24 geometry,
1.8px rounded stroke and transparent canvas keep the entire pack visually
consistent at every tile size.

Each pack may contain .svg / .png / .jpg / .jpeg files named exactly by slug:

  achievements, calculator, consoles, favorites, flashlight, library,
  link, minigames, music, radio, retroarch, settings, surprise

resume.png remains in this parent folder as a compatibility fallback for a
Resume tile with no scraped cover. Resume and game-favorite tiles normally use
the game's real artwork.

If an icon is absent from the selected pack, Snap FE tries the Simple pack and
then this legacy parent folder. A missing file therefore never produces an
empty tile.

Recommended: square transparent SVG, with a matching PNG fallback. Tiles render
at roughly 140 px on the 720x480 panel and are fitted without stretching.
