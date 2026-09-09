# Where your files go

`SHARE` is the Knulli user-data partition (`/userdata` over SSH). Do not copy the
SNAP package to the small boot partition or reformat the card for an update.

| Files | Location on SHARE |
| --- | --- |
| ROMs | `roms/<system>/` (or up to three folders selected in SNAP's Games settings) |
| BIOS owned by you | `bios/`, with the folders required by each emulator |
| Knulli saves | `saves/` (custom emulator/save paths can differ) |
| SNAP program | `system/snapos/snapos_ui` |
| SNAP settings/history/favorites | `system/snapos/`; preserve existing data files |
| Account keys, custom theme colors and local caches | `system/snapos/config/`; keep private |
| Scraped game art | `system/snapos/boxart/<system>/<art-type>/` |
| Console wallpapers for 4:3 screens | `system/snapos/assets/backgrounds/4x3/<system>/` |
| Console wallpapers for 3:2 screens | `system/snapos/assets/backgrounds/3x2/<system>/` |
| Console icons | `system/snapos/assets/icons/<view>/<system>/` |
| App icon packs | `system/snapos/assets/icons/home/simple/` and `pixel-art/` |

System folder names must match Knulli's short names, such as `gb`, `gba`, `snes`,
`psx`, `amiga500` and `atari800`. ROMs do not belong in the program's assets tree.

For game art, keep the game's existing filename base and scraper-created art
type folder. Existing EmulationStation artwork is also supported through its
gamelist/media references; you do not need to move it into SNAP's folders.

The console icon view folders include `carousel`, `grid`, `list` and `bookshelf`.
The shared bookshelf background is under `icons/bookshelf/background/`.
Transparent PNG is suitable for personal icons; existing SVG app icons are
supported. Keep the app filenames from the selected pack when replacing them.

Use Display > Backgrounds to select/download wallpapers. In the system background
selector, X renames and Y deletes with confirmation. Retain hidden metadata:
it identifies downloaded originals even after renaming. Windows
`:Zone.Identifier` sidecars are not artwork and are excluded from packaging.

For Systems backgrounds, put **1280 x 960 (4:3)** PNG/JPG images in
`assets/backgrounds/4x3/<system>/` and **1440 x 960 (3:2)** images in
`assets/backgrounds/3x2/<system>/`. For example, the two versions of a Game Boy
wallpaper can both be named `GB Red.png`, each inside its own ratio's `gb` folder.
Each device lists only the backgrounds in the folder for its screen shape; the
other ratio's version never appears beside it. The folder controls visibility,
so you can replace unfinished images or add more later without changing code.
SNAP scales and crops images from the center to fill the screen; keep important
text/art away from the edges.

Once either ratio has a folder for a system, that system uses only the current
screen's ratio folder. An empty or missing matching folder shows no custom
wallpapers for that system. Old `assets/backgrounds/<system>/` folders remain
supported only for systems with neither ratio folder. This prevents old copies
from appearing alongside your new versions. Leave the small README files in
empty system folders so their structure is retained when copying the artwork.

## Updating safely

Follow [UPDATING.md](UPDATING.md). Merge the release contents into SHARE without
deleting the existing folder. Bundled files with the same filename are replaced;
back up your customized bundled icons/backgrounds before updating. New filenames
and personal ROMs, BIOS, saves, scraped art and account settings are not bundled
and must not be removed. Release packages include the bundled theme artwork,
both home icon packs and fonts, but never the developer's personal game library.
