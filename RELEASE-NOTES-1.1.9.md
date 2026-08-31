# Snap FE Alpha 1.1.9

This release focuses on making Snap FE feel native both in the frontend and
inside RetroArch, while preserving each user's own settings.

## In-game controls

- Brightness now remains synchronized between Snap FE, Knulli, and RetroArch.
- Menu + Volume Up/Down changes brightness in exact 5% steps, including a true
  1% night setting, with no wrap back to maximum.
- Volume Up/Down keeps 5% steps and both controls display native RetroArch
  percentage notifications during games and in the RetroArch app.
- Knulli's duplicate media-key listener is safely suspended only while Snap FE
  is active, eliminating skipped levels and double adjustments.
- Menu + Select opens the RetroArch Quick Menu by default. Menu + Start still
  closes the game and returns to Snap FE.
- Custom RetroArch menu hotkeys are detected, saved, and reused on later game
  launches instead of being reset to Snap FE's default.

## Frontend and settings

- Added Syncthing controls under System settings.
- Reworked Accounts into Scraping, RetroAchievements, and WallHaven groups;
  RetroAchievements now reports successful linking clearly and Achievements
  Book is available as its own app.
- Added SFW WallHaven background search/downloads. API keys are optional,
  supplied by each user, stored only on the device, and never bundled.
- Fixed Surprise Me alignment, rapid navigation, and slow repeated selection by
  caching the game pool.
- Corrected App Focused Home's 4x2 page transitions and boot position while
  preserving hold-X reordering across pages; placeholder icons are simpler.
- Added RGSPink and Yellow/Gold themes.
- Improved charging detection and direction/status feedback.
- Fixed Bookshelf system counts and warmed Carousel artwork for a smoother
  first pass.

## Polish and mini games

- Boot animation now preloads the game/system libraries and artwork so quotes
  remain visible longer while useful work happens in the background.
- Added a matching, short themed shutdown animation.
- Expanded Breakout with additional stages/effects and replaced Tic Tac Toe
  with Connect 4.

## Updating

Extract `SnapFE-Alpha-1.1.9.zip` directly onto the Knulli SHARE partition and
choose Merge/Replace. Existing ROMs, saves, scraped artwork, favorites,
settings, and account credentials are not included or removed.
