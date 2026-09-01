# Snap FE Alpha 1.2.2

This update expands Home customization, improves first-run guidance, and
includes the latest H700 game-launch latency repair while retaining SNAP's
working in-game brightness controls.

## Changes

- The Radio Tuner Home widget now uses L1 for the previous station, L2 for the
  next station, and Select for play/stop, with a shorter on-screen hint.
- App Focused Home now fills the handheld screen with a true 4x2 app grid,
  supports L1/R1 page changes, and draws supplied transparent app icons without
  an extra tile border. The selected app now gets a tight three-pixel outline,
  and app titles follow the user's selected font family and size.
- A separate two-cell App Focused widget can be added or changed with X and
  moved between app slots with Y. Weather, Clock, Battery Health, and an
  interactive Calendar are available; apps automatically flow around it.
- Pressing A on the App Focused Weather widget refreshes it immediately. Y is
  reserved consistently for moving the selected app or widget.
- App movement now uses a predictable two-stage jiggle mode: Y enters edit
  mode, Y picks up the highlighted item, A confirms its placement while
  remaining in edit mode, and B cancels a pickup or exits editing.
- Calendar is now a useful current-week widget instead of a cramped month.
  Pressing A opens a full Calendar app, the D-pad selects days, L1/R1 changes
  months, A adds or edits a persistent reminder, and X removes one after
  confirmation. Each reminder can be all-day or use a specific time adjustable
  in five-minute and one-hour steps. It appears on Home when due if a Calendar
  widget is active. Leaving Calendar resets browsing to today.
- Calendar is also available in either Informational Home widget slot. Focus it
  with Right and press A to open the full calendar.
- A new Calculator app is available under Apps in both Home layouts, with
  D-pad navigation, clear, delete, decimal and standard arithmetic controls.
- App Focused widget choices are independent from Informational Home widgets.
  Battery Health now reports the H700's real health state, cell voltage and
  temperature instead of estimated information the hardware does not expose.
- L1/R1 cycle the top Informational widget backward/forward, while L2/R2 do the
  same for the bottom widget. Their hints align with the actual top of each
  card, including bottom-anchored widgets.
- Auto-Sleep now enters SNAP's real low-power rest path instead of only drawing
  a black frame, then restores the display, radios and chosen brightness on wake.
- Game aspect-ratio and rotation choices now pass into Knulli configgen, so
  explicit 4:3, 3:2, 16:9 and pixel-perfect selections reach RetroArch games.
- First-time setup now includes a dedicated physical-button and built-in-hotkey
  guide covering SNAP navigation, app pages/widgets, RetroArch access, game
  exit, fast-forward, brightness, and volume.
- New user-supplied Link Play, Music, Achievements and Calculator App Focused
  icons are bundled with the complete icon set.

## Game launch performance

- Knulli config generation no longer sends unused debug/stdout traffic into the
  frontend launch path.
- The CPU receives a temporary performance boost only while configgen prepares
  the game, then returns to the user's selected power mode as soon as RetroArch
  is running.
- A byte-identical duplicate of Knulli's stock controller database is migrated
  out of the user override path, avoiding a second full controller-map parse on
  every launch while preserving genuinely edited controller files.
- On the RG SP test device, the same launch diagnostic improved from about
  9.262 seconds to 2.834 seconds. ROM/core startup time can still vary by game,
  storage card, and Knulli build.

The in-game Menu + Volume brightness path and its real 1% panel floor are not
replaced or reset by these launch changes.

## Screenshots

### App Focused Home and system views

![App Focused Home with Weather widget](https://github.com/differentlightproductions-cyber/snap-fe/releases/download/V1.2.2/SnapFE-1.2.2-App-Focused-Weather.png)

![Console Carousel](https://github.com/differentlightproductions-cyber/snap-fe/releases/download/V1.2.2/SnapFE-1.2.2-Console-Carousel.png)

![Bookshelf console view](https://github.com/differentlightproductions-cyber/snap-fe/releases/download/V1.2.2/SnapFE-1.2.2-Bookshelf.png)

![Achievements Book](https://github.com/differentlightproductions-cyber/snap-fe/releases/download/V1.2.2/SnapFE-1.2.2-Achievements.png)

### New Calendar and Calculator

![Full Calendar app](https://github.com/differentlightproductions-cyber/snap-fe/releases/download/V1.2.2/SnapFE-1.2.2-Calendar.png)

![Calendar reminder alert](https://github.com/differentlightproductions-cyber/snap-fe/releases/download/V1.2.2/SnapFE-1.2.2-Reminder.png)

![Optional reminder time picker](https://github.com/differentlightproductions-cyber/snap-fe/releases/download/V1.2.2/SnapFE-1.2.2-Reminder-Time.png)

![Calculator app](https://github.com/differentlightproductions-cyber/snap-fe/releases/download/V1.2.2/SnapFE-1.2.2-Calculator.png)

## Updating

Download `SnapFE-Alpha-1.2.2.zip`, extract it directly onto the Knulli `SHARE`
partition with Merge/Replace enabled, then run **Snap FE (Set As Default)** from
Ports once. Do not delete the existing `system/snapos` folder first.

The package does not contain or remove user settings, favorites, ROMs, saves,
save states, BIOS files, accounts, API keys, or scraped game artwork.
