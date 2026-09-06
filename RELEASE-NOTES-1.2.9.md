# Snap FE Alpha 1.2.9

This release expands the mini-games, improves nearby play, adds saved time and
weather locations, and makes the library and settings easier to use. It also
includes complete Systems List artwork, configurable sleep screens, and battery
saver recommendations that stay out of the way during games.

For **Knulli on Allwinner H700 handhelds**, tested on RG34XX-SP and RG35XX-SP.
This is a frontend add-on; it does not replace or reflash Knulli.

## Mini-games

- **Crazy Fish** now progresses by buying three egg pieces and hatching a
  permanent companion. A hatch animation reveals each discovery, and the
  collection lists only unlocked animals. Companions help collect resources,
  slow coins, fight monsters, or protect young fish. Chapters span four levels,
  with changing tank scenery and brief introductions.
- The tank fills most of the screen, with compact status information. Food
  appears at the cursor or click, costs ten coins, and each fish waits ten
  seconds between meals. Overfeeding wastes money and leaves uneaten food.
  **A** feeds a peaceful tank and attacks during an invasion; **L1** upgrades
  the ray, **R1** buys egg pieces, and **Y** opens the collection. Purchase
  failures explain what is missing. Monsters arrive about every ninety seconds
  of active play, with their schedule continuing across levels.
- **Pulse Runner** stages are three times longer and finish at 100%. Later
  stages add gravity flips and ceiling jumps, alongside more forgiving jump
  timing. The longer courses reuse authored sections.
- **Pocket Crossing** adds twelve varied section types, including moving-log
  rivers, railways, swamps, tunnels, farms, and bridges, with difficulty growing
  over a run.
- **Art Shuffle** offers 4-, 9-, and 16-piece puzzles, remembers the chosen
  difficulty for New Art, rejects blank or placeholder artwork, and keeps the
  completed picture visible.
- Genre headings fit their boxes, game counts are clearer, and categories with
  more than four games show a small scrollbar. Per-game and per-level folders
  under `assets/minigames/` accept looping PCM `music.wav` tracks; folder
  instructions are included.

## Link Play and Friends

- **Connect 4** joins Pong and Tank Duel for nearby Wi-Fi play. Tank Duel has
  five maps and defaults to best of three for friendly matches. Leaving a
  connected match asks for confirmation.
- Pong has a clearer lobby, a round ball, confirmation from both players, and
  a three-second countdown before serving. Updated movement prediction and
  correction improve paddle response and ball motion between network updates.
- The Link/Friends game list scrolls correctly, with a clearer selected row
  and larger Mini Games/Friends controls.
- Friends have persistent IDs, saved lists, presence, explicit connection and
  friend acceptance, game invitations, and optional preset messages. Nearby
  discovery uses the local network; ordinary game invitations require a
  matching game on the receiving device.

**Update both players to 1.2.9.** Nearby mini-games use an updated protocol.
Connection quality still affects play, and handheld-to-handheld smoothness can
vary with Wi-Fi conditions.

## Home, weather, and artwork

- Interactive widgets have a more visible selection highlight and corrected
  navigation. **A** opens time or weather locations; each family saves up to
  three places. App Focused uses **L2/R2** to cycle saved places. Informational
  Home keeps focus on an unchanged widget when the other widget is cycled.
- Add Location searches available timezone and city entries, offers choices,
  and tolerates small spelling mistakes. Local includes the detected city when
  available. This searches the installed location catalog, not every address
  worldwide.
- Weather keeps each city's request and cache separate, repairs truncated saved
  city names such as Las Vegas, and preserves the last good reading when a
  request fails. Refreshes run in the background and failed requests retry.
  Day/night icons follow the selected location's timezone.
- Systems List now includes the completed system icons, including Nintendo 64.
  App Focused recent games without cover art use a theme-colored fallback icon.
- **Surprise Me** favors games with artwork: when both groups are available,
  new picks have an 80% chance of coming from games with art and 20% from games
  without it.
- Display Art changes refresh images in the background. Library view changes
  reuse the loaded game index, and sorting large libraries is faster. Explicit
  rescans still rebuild the index when needed.
- Scrape progress replaces the SNAP FE status label with the game's first word,
  completed/total count, and overall percentage, leaving room for the clock.

## Settings, sound, and power

- **Systems View** opens one dropdown, beginning with Show Systems Without
  Games. The redundant Informational Widgets group is removed from Display.
- **Default Themes (X Edit)** and **Custom Themes** are separate choices, with
  eight saved custom slots. Editing a default saves a custom copy. Custom theme
  deletion asks for confirmation and safely handles deleting the active theme.
- Sound aligns **Game Audio With Radio** with the other settings and adds a
  separate **Game Audio With Music** option for SD-card music when Play Over
  Games is enabled. Both preferences apply to emulator launches and built-in
  mini-games.
- **Screen & Power** offers DVD Bounce, Starfield, Aquarium, System Dream,
  Random, and Off sleep-screen choices. Set the animation duration to 15 or
  30 seconds, or 1, 2, 5, or 10 minutes before the display sleeps. The first
  input wakes the screen without activating the item underneath.
- The low-battery light follows the status bar's red threshold at **20%**.
  Battery saver recommendations show the actual percentage at 20% or below;
  after an answer, the next recommendation waits until 5% or below while
  Power Save remains off. Answers survive restarts and reset after recharge.
- During games, a brief passive battery banner leaves game controls alone;
  the Enable/Not now choice waits until returning to an appropriate SNAP menu.
  Charging or already-enabled Power Save suppresses recommendations.
- In-game brightness, volume, and performance overlays use improved buffering.
  Startup also includes an updated greeting, time-of-day presentation, and a
  saved quote rotation.

For your own Systems backgrounds, use **1440 x 960 PNG/JPG** as a 3:2 master,
or **1280 x 960** for a dedicated 4:3 image. Keep important content centered
because SNAP scales and crops to fill the screen. See the included folder guide
for artwork and music locations.

## Download and update

Download **`SnapFE-Alpha-1.2.9.zip`** from this release's Assets. The matching
**`SHA256SUMS-1.2.9.txt`** is provided to verify the download. GitHub's automatic
Source Code archives are for development and are not the install package.

1. Back up your SNAP settings and any artwork you customized in the bundled
   assets folders. Power the handheld off and connect its Knulli card to a PC.
2. Extract the ZIP directly onto the card's **SHARE** partition, merging folders
   and replacing matching files. Keep the existing `system/snapos` directory.
3. Safely eject the card, boot Knulli, open **Ports**, and run
   **Snap FE (Set As Default)** once to refresh the startup hook.

For a new Knulli installation, boot it once first so it creates SHARE. The
package contains no ROMs, BIOS files, personal settings, account keys, favorites,
saves, or scraped game artwork. Matching bundled artwork files are replaced,
so restore any customized copies afterward. The included `UPDATING.md` and
`USER-FOLDERS.md` provide the full instructions.

## Verification

The regression suite passes with address and undefined-behavior checks. All 55
bundled list images decode, and the final ARM64 build passes Knulli library
compatibility checks. Both test handhelds run this package's executable and
passed on-device widget, battery-reminder persistence, live weather, artwork,
and hardware-overlay checks. Earlier device network checks covered Pong, Tank,
and Connect 4.

Battery thresholds and recharge behavior were tested with isolated fixtures;
a full physical battery discharge was not performed. Automated checks do not
cover every emulator, SD card, or perceived gameplay timing. This remains an
alpha release.
