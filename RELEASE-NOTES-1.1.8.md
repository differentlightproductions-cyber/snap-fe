# Snap FE — Alpha Build 1.1.8

This release polishes navigation, settings, themes, Link Play, and Mini Games,
while retaining the in-game lid sleep/wake and renderer-cache fixes from 1.1.7.

## Highlights

- App Focused Home now uses a fixed 4x2 layout with multiple pages. Hold X to
  rearrange apps, including moving them between pages.
- All remaining test-build PC keyboard prompts now show the matching handheld
  ABXY, D-pad, shoulder, Menu, or Select control.
- Games Folder moved to Settings > Game. Display Art and Show Descriptions are
  together there, and Display Art reloads only after Save when its value changed.
- Night Mode and its schedule/brightness/hotkey controls are condensed into a
  dedicated collapsible group, separate from Battery settings.
- CPU Performance Mode now displays a compact `PERF` HUD badge while active.
- Top Bar underlines derive a brighter, contrasting edge from the selected
  status backdrop color.
- New Dark Purple theme: dark plum background, lavender accents, and white text.
- Link Play keeps its A/B footer clear and shows the seven or eight game rows
  that fit the current font, with the rest available by scrolling.
- Block Roll now has ten verified-solvable levels and persistent level progress.
- The Mini Games browser shows five legible entries at a time and scrolls.

## Included hotfixes

- Closing the RG34XX-SP lid during a RetroArch game pauses the emulator and
  enters deep rest; opening it wakes the device and resumes the same session.
- Returning from a game invalidates renderer-owned art and shadow caches,
  preventing corrupted or overlapping artwork.

## Install or update

Download `SnapFE-Alpha-1.1.8.zip` from this release—not GitHub's automatic
source archive—and extract it directly onto the Knulli SHARE partition. Then
open Ports and run `Snap FE (Set As Default)`.

Existing settings, favorites, ROMs, saves, BIOS files, scraped artwork, and
account data are preserved.
