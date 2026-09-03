# Snap FE Alpha 1.2.6

This release is mostly one theme: several features only ever looked at the
first SD card. Fixing that took in the library, Link Play and the scraper, and
turned up a second family of bugs where behaviour was pinned to button numbers
measured on one specific handheld.

## Two SD cards

- **Both cards are visible now.** Knulli mounts exactly one data partition as
  `/userdata` and leaves the other card **completely unmounted** — `/media/<LABEL>`
  exists as an empty placeholder directory with nothing behind it. A second card
  of games was therefore not merely hard to find; no path in the filesystem
  reached it. Snap FE mounts it at startup and adds its games to the library.
- Which card loses is not fixed. On one handheld here it was the boot card's
  53 GB `SHARE` partition that was orphaned; on another it was a 58 GB card
  labelled `ROMS`. Snap FE names the slots by hardware: **the card carrying
  `/boot` is TF1**, whichever device node it happens to have come up as.
- **Settings > Games > Storage & Games Folders** is new. It lists each card with
  its size, mount point and game count, and offers per-card actions: use this
  card for games, check games folders, create missing folders, browse, mount,
  eject.
- A blank card gets its per-system folders created by **copying the names your
  main card already uses**, not Snap FE's own slugs. Knulli calls it
  `megadrive/`, and creating a canonical `genesis/` beside it would shadow the
  folder that actually holds the games.
- The first-run wizard can set a second card up in one press.
- Knulli only lays those folders down once, at first boot, for the primary card.
  A card added later was never going to be populated by anything.

## Link Play

- **Link Play scanned only the first ROM folder**, so games on a second card
  were invisible there while the main library listed and launched them fine. It
  now scans every folder. It also derived a game's system by stripping the first
  card's path, which mislabelled anything outside it.
- **A session now outlives the game it launched.** Quitting used to close every
  socket and end the session for both sides, forcing the pair to set it up
  again. Only the pairing port actually belongs to the emulator, so discovery
  now stays up: your handheld keeps announcing itself through the whole game,
  appears to your partner as being in a game, and returns to the lobby with the
  session intact. Leaving Link Play is what ends it.
- Partners survive 12 seconds of silence instead of 5, so a brief stall no
  longer drops them from the list.

## Buttons and brightness

- **In-game brightness worked on only one handheld.** The pad was matched by the
  exact string `"Anbernic RG34XX-SP Controller"`, so on any other device the
  input node was never opened, the Menu modifier was never seen, and Fn+Volume
  silently did nothing. The pad's real name now comes from the kernel.
- **The Menu button behaved like a shoulder button** on handhelds with a
  different button count. SDL numbers buttons by walking the pad's key bitmap in
  ascending order, so an index only means something on the pad it was measured
  on: one device here reports 15 buttons where another reports 17, shifting
  everything above L2 down by two. Buttons are now resolved from their evdev
  keycodes, which are identical across these handhelds.
- **Full panel brightness.** 100% was mapped to 200 of the panel's 0-255 PWM
  range, so roughly a fifth of it was unreachable. The flashlight was capped the
  same way — the one feature that most wants the whole panel.
- **Off no longer survives a reboot.** Stepping brightness down to Off persisted,
  and the handheld came back up rendering perfectly behind a dark panel, with no
  way to see the menu that would turn it back on. Off remains available while
  running; 1% and above are still restored exactly as set.

## Scraping

- **Fix Game Titles** (Settings > Scraping, on by default) shows the name the
  scraper matched instead of the filename — in the library, in favourites and in
  Link Play. The name is saved during a normal scrape at **no extra API cost**,
  because it is already in the response that fetched the artwork. A name-only
  pass lets an already-scraped library pick titles up without refetching images.
- The scraper was also passed a single ROM folder and now takes them all,
  skipping a game already covered by another card.

## Elsewhere

- **Wi-Fi** shows whether you are connected in a banner under the header, with
  the network name and address. The old status line sat one row above the button
  hint under a fixed ten-row network list, so on a full list the three ran into
  each other and connected was hard to tell from not connected.
- **RG35XX-SP** added to the handheld list. That setup step is now a single
  cycling row rather than a line per model, and its description wraps instead of
  being cut off.
- The Single Card control hint no longer clips off both edges.
- Install instructions now say to boot Knulli once before looking for the SHARE
  drive, and explain what to do when a second card is blank.

## Known limits

- While your Link Play partner is inside a game their pairing socket is down, so
  they show as being in a game but cannot be joined until they exit.
- Snap FE's own first-run wizard still runs on a fresh install; it is not skipped
  for a known device.
