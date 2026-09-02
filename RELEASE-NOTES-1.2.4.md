# Snap FE Alpha 1.2.4

This update introduces selectable App Focused icon packs and refines their
presentation while preserving the layout and controls from 1.2.3.

## Changes

- Added **Simple** and **Pixel Art** icon packs under separate asset folders.
- The new Simple pack is the default; the complete 1.2.3 illustrated set is
  preserved as Pixel Art.
- User-facing labels now identify the illustrated choices as **Pixel Art (AI
  Art)** and **Bookshelf (AI Art)**.
- Added **Settings > Display > Home > App Icon Pack**. The selected pack is
  saved and can be changed with A or left/right.
- Simple icons automatically follow the active theme's text color for readable
  contrast across light and dark themes.
- Simple icons now ship as real SVG paths and are rasterized at their requested
  runtime size by the SVG renderer already included in Knulli's SDL_image 2.8.
  PNG copies remain as safe fallback assets.
- Rebuilt the Simple set from a consistent Lucide line-icon foundation instead
  of auto-tracing the PNG edges; all symbols now share clean rounded geometry.
- Balanced individual icon sizes inside the existing rounded tiles without
  changing the 4x2 grid, labels, selection outline, or app movement.
- Missing artwork in a selected pack falls back to Simple and then the legacy
  asset location instead of leaving a blank tile.
- Replaced contextless boot quotes with a short, auditable selection of
  attributed moments from landmark GDC and E3 presentations.
- Windows `Zone.Identifier` metadata is removed during packaging and background
  scans, and deleting a wallpaper also removes its identifier sidecars.

## Updating

Download `SnapFE-Alpha-1.2.4.zip`, extract it directly onto the Knulli `SHARE`
partition with Merge/Replace enabled, then run **Snap FE (Set As Default)** from
Ports once. Do not delete the existing `system/snapos` folder first.

The package does not contain or remove user settings, favorites, ROMs, saves,
save states, BIOS files, accounts, API keys, or scraped game artwork.
