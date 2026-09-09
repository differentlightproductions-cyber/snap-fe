# Snap FE Alpha 1.3.0

This release focuses on the Systems experience, background management, library
smoothness, and cleanup from the 1.2.9 bug-fix pass.

For **Knulli on Allwinner H700 handhelds**, tested on RG34XX-SP and RG35XX-SP.
This is a frontend add-on; it does not replace or reflash Knulli.

## Systems And Backgrounds

- Single Card now shows clear community system names instead of maker-only
  labels. Common short names such as PS1, GBA, SNES, N64, PSP, and C64 are used
  where they make sense, and full names are used elsewhere without bracketed
  folder names.
- Single Card title placement can be switched between left and right focus from
  Display settings or the quick settings overlay. Right-side mode keeps the
  text aligned and readable against the card.
- Per-system settings now include a Background row, so each system's wallpaper
  can be changed from the same place as its aspect, rotation, and core options.
- The Background picker now previews before saving: press **A** once to preview
  the selected image, then press **A** again on that same image to commit it.
- Bundled system backgrounds now use the 3:2 and 4:3 folder layout documented
  in `USER-FOLDERS.md`, with the refreshed PNG artwork set included in the
  package: 275 backgrounds for 3:2 screens and 272 backgrounds for 4:3 screens.

## Library, Scraping, And Performance

- Deleting a game no longer forces a full visible library reload. The removed
  game is dropped from the active list immediately, keeping the screen feeling
  responsive.
- Carousel rendering does less unnecessary off-screen work, which improves
  scrolling smoothness when all systems and many backgrounds are enabled.
- The art scraper now knows the full SNAP FE system list, including Atari 7800
  and other systems that were previously skipped by the older limited mapping.
- Scraping falls back more gracefully for systems without a ScreenScraper ID
  instead of stopping the scan.
- Nearby Pong and other mini-games send small network updates redundantly and
  apply correction faster, reducing the joiner's visible stutter on link play.

## Cleanup

- Removed stale background duplication from the shipped asset layout and kept
  Windows `Zone.Identifier` sidecars out of the package.
- The release checker and GitHub helper now accept the target version instead of
  being fixed to the previous 1.2.9 release.

## Download And Update

Download **`SnapFE-Alpha-1.3.0.zip`** from this release's Assets. The matching
**`SHA256SUMS-1.3.0.txt`** is provided to verify the download. GitHub's automatic
Source Code archives are for development and are not the install package.

1. Back up your SNAP settings and any artwork you customized in the bundled
   assets folders. Power the handheld off and connect its Knulli card to a PC.
2. Extract the ZIP directly onto the card's **SHARE** partition, merging folders
   and replacing matching files. Keep the existing `system/snapos` directory.
3. Safely eject the card, boot Knulli, open **Ports**, and run
   **Snap FE (Set As Default)** once to refresh the startup hook.

For a new Knulli installation, boot it once first so it creates SHARE. The
package contains no ROMs, BIOS files, personal settings, account keys,
favorites, saves, or scraped game artwork. Matching bundled artwork files are
replaced, so restore any customized copies afterward. The included `UPDATING.md`
and `USER-FOLDERS.md` provide the full instructions.

## Verification

The ARM64 build passes Knulli library compatibility checks with no glibc 2.38+
imports. The release package checker confirms the archive layout, executable,
checksum, bundled background count, and personal-file exclusions. Both test
handhelds were updated with the same executable and refreshed background set for
device testing before packaging.
