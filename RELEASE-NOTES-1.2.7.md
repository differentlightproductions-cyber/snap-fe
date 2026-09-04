# Snap FE Alpha 1.2.7

A settings reorganisation, a separate Library View, four new themes and four
new fonts — plus fixes to Wi-Fi, Link Play and the status bar.

## Library View is its own setting

- The game library no longer just inherits whatever the Systems View is. It
  defaults to **Follow Systems View**, so nothing changes until you pick a
  layout — and once you do, changing the Systems View no longer silently
  reverts it.
- Its layouts have their own names, because they are not the same things:
  index 1 is **Cartridges**, drawn here rather than scraped, so it loses the
  "(AI Art)" tag. The Systems View keeps Carousel.
- **Single Card** as a library layout was falling through to the ordinary grid.
  It now renders as the 1×1 grid it describes.

## Settings

- **Systems View** and **Library View** move to Settings > Display, which had
  room; the Game tab was overcrowded. **Theme & Text** moves to second, under
  Home.
- Each group names its layout once, on its own header — `Systems View:
  Carousel`, `Library View: Follow Systems View`. Left/Right on the header
  changes it, A opens the group.
- **Backgrounds is now a page of its own**, opened from Display. Scrolling
  every system inside an already-long tab was unworkable. It lists only the
  systems the Systems view is showing, so "Show Systems Without Games"
  governs it too.
- **App Icon Pack** only appears when App Focused is the active home view — it
  does nothing for the others.
- Dropdown groups now close when you leave Settings **or change tab**. Some
  never closed at all.

## Themes and fonts

Four themes, each different in kind from what was there:

| | |
|---|---|
| **Rainbow Road** | deep space under a full-spectrum wash — the only theme that paints a gradient rather than a flat field. Its accents shimmer, and the wordmark carries a rainbow across its glyphs. |
| **Sunset Drive** | dusk plum with coral and amber |
| **Moss** | forest green with cream type; the only natural dark theme |
| **Sepia** | parchment and brown ink; the only light theme with no blue |

Four fonts, none with a relative in the existing set: **Signage** (Bungee),
**Techno** (Audiowide), **Slab** (Zilla Slab) and **Marker** (Permanent
Marker). Licences ship in `assets/fonts/licenses/`.

## Home and status bar

- The **Radio** card was claiming L1/L2 for tuning, which silently disabled
  widget cycling for the whole Informational home whenever Radio was in either
  slot. Cycling always owns those keys now; Radio tuning moved onto the same
  Right-to-focus mechanism Calendar and Recently Played already use, where
  Up/Down tune and A plays or stops. The L1/L2 hints no longer disappear.
- The **clock widget** cut its date off in 12-hour mode. It now steps down
  through shorter date formats until one fits.
- Wi-Fi drew its own pill next to the battery's, so **Pill and Rectangle**
  status styles showed two separate pills. All three items are one group now.
- **SNAP FE** was missing from the top-left in Settings.
- Backing out of the **Achievements book** always went to Settings, even when
  opened from the Home app. It returns where it came from.
- The Achievements list was drawn over its own "<user> — N earned" subtitle at
  larger font sizes.

## Wi-Fi

- Snap FE reconnected to the saved network every 45 seconds without asking, so
  joining a different network — a phone hotspot, say — was quietly undone a
  minute later. Rejoining the network **you** chose still happens silently;
  being moved to one you did not pick now asks first, and declining is
  remembered until the handheld is fully powered off.
- Connecting gave no feedback: a wrong password, an out-of-range network and a
  successful join all looked identical. The result is now specific — wrong
  password, out of range, no response — and failures show in red.
- **Forget a network** with X twice on its row. Networks show "saved" only
  when credentials have actually worked, so it never offers to forget
  something it does not have.

## Link Play

- Peers appeared before anyone was hosting, and joining one could only fail.
  Only sessions that are actually hosting are listed now.
- **Host Name** is editable from the lobby.
- A session outlives the game it launched: quitting returns you to the lobby
  with it intact, and your partner sees you as being in a game rather than
  losing you entirely.
- Link Play scanned only the first ROM folder, so games on a second card were
  invisible there while the library listed them fine.

## Two SD cards

Carried over from 1.2.6 and refined: the folder-name resolution, the List
view's preview art and a game's manual all only ever looked at the first card.
All three now search every ROM root.

## Known limits

- **Bookshelf** as a *library* layout is not implemented yet; it falls back to
  the grid. Bookshelf as a *Systems* view is unaffected.
- While a Link Play partner is inside a game their pairing socket is down, so
  they show as being in a game but cannot be joined until they exit.
