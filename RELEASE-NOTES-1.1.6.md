## Snap FE Alpha 1.1.6

This release rolls up the latest interface, device, library, artwork, power-management, and Link Play work. It also fixes the silent one-second black-screen failure some users encountered when installing from 1.1.5 on pinned Knulli builds.

### Important installation fix

- The release ZIP is now flat and can be extracted directly onto the Knulli **SHARE** drive. `system/` and `roms/` land in the correct locations automatically.
- The ARM64 binary now targets a glibc 2.34 baseline instead of importing glibc 2.38 symbols, improving compatibility with pinned/older H700 Knulli releases.
- **Snap FE (Set As Default)** validates the executable, loader, SDL dependencies, and boot hook before takeover, installs atomically, and reboots automatically.
- Ports launch failures now create readable diagnostics under `SHARE/system/snapos/` instead of flashing black with no explanation.
- Updating preserves settings, favorites, ROMs, saves, BIOS, scraped art, and account configuration.

### Home, Favorites, and Surprise Me

- Favorites is a dedicated Home button rather than a Home-screen section.
- Favorites can follow the normal Game Library layout or use its own Single Card, 2×2, 3×2, or 4×3 view.
- Fixed clipped text in Favorites Single Card view.
- App Focused Home now uses a compact phone-style grid with bundled placeholder icons.
- Hold **X** on an app to enter reorder mode, with clear movement feedback.
- Surprise Me moved beside the Home title with its own styling and a larger art area.
- Fixed reversed/stuck vertical navigation and fast double-right input skipping an extra game.
- Recent-game titles remain cleanly bold while playtime metadata is visually softened.

### Display, artwork, and power

- Added the greeting/status underline to the Top Bar status-backdrop layout.
- Renamed Game Art Display to **Columns/Rows** and reorganized Console View and Display Art under Game settings.
- Added art-aware outlines/shadows so 3D box art no longer spills over neighboring games.
- Removed the redundant Bookshelf artwork page; artwork remains scrollable from More Info.
- Night Mode now asks for confirmation before a manual theme change turns it off.
- Power Save correctly restores the previously selected Midnight theme.
- Raising Auto Power Save above the current battery level now asks before activating immediately.
- Battery percentages below 30% use readable muted warning colors, with an optional filling battery icon.
- Brightness set in Snap FE persists when RetroArch games launch; Fn + Volume can adjust it in-game.

### Online features, achievements, and media

- Added an on-device browser for downloading free per-system backgrounds over Wi-Fi.
- Active artwork scraping can now be stopped from Settings.
- Added a RetroAchievements achievements-book view for unlocked achievements.
- Wi-Fi reconnect settings persist after moving away from and returning to an access point.
- Removed the redundant `VOL 100%` text from Radio and moved location information to the right.

### Link Play and games

- Link Play now starts with organized console entries and system icons.
- GB/GBC link sessions use the packaged Gambatte network-link transport.
- Supported GBA Pokémon, Advance Wars, and Wireless Adapter modes use the packaged gpSP serial/netpacket transport.
- Added lightweight mini-game content suitable for the H700, including block-rolling and light-gun-inspired play.
- Build labeling now consistently reports **Alpha Build 1.1.6**.

### Install

Download **SnapFE-Alpha-1.1.6.zip** below—not GitHub's automatically generated Source Code ZIP—then extract it directly onto the Knulli **SHARE** drive. Boot Knulli, open **Ports**, and run **Snap FE (Set As Default)**.

SHA-256 checksums are included in `SHA256SUMS-1.1.6.txt`.
