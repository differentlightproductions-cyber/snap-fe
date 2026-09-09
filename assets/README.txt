Snap FE artwork -- where each image goes and what size it should be
=================================================================

Use a sub-folder with the system's Knulli short name, for example gba, gbc,
gb, nes, snes, genesis, psx or amiga500. Keep existing folders and filenames
when replacing bundled artwork. PNG/JPG/JPEG files are supported; transparent
PNG is useful for icons. See USER-FOLDERS.md in the release for the full guide.


1. assets/backgrounds/4x3/<system>/
   assets/backgrounds/3x2/<system>/
   Full-screen backdrop on the console screen (Single Card / Carousel / Grid
   when Background = Image). Every image here becomes a pickable option in
   Settings > Display > Backgrounds. Downloaded images can have descriptive labels.
   Size:  1280 x 960 in 4x3, or 1440 x 960 in 3x2. JPG or PNG.
          Each screen lists only its matching ratio's folder. Use the same
          filename in both ratio folders for two versions of the same design.
          The folder controls visibility, even while images are unfinished.
          Cover-fit (centre-cropped), so keep important art/text away from edges.
   Once either ratio contains a system folder, the matching ratio folder is
   authoritative. An empty/missing matching folder shows no custom backgrounds;
   it never borrows the other ratio's files or old duplicate images. Keep the
   README.txt files in empty system folders so this structure survives copying.
   Legacy assets/backgrounds/<system>/ remains supported only when neither
   ratio has a folder for that system. You can add more images at any time.

2. assets/icons/carousel/<system>/    (one image)
   The system "card" in Carousel view. Card is 190 x 280 portrait
   (220 x 220 if you set card shape = Square).
   Size:  ~420 x 600 portrait, transparent PNG. Fit inside, never stretched.

3. assets/icons/grid/<system>/         (one image, OPTIONAL)
   The system box in Grid view. Cell size varies with the grid dimensions;
   figure ~150-220 px square. No file -> a themed box with the short name.
   Size:  ~320 x 320 square, transparent PNG.

4. assets/icons/list/<system>/         (one image)
   Small badge beside each row in List view. Drawn ~40 x 40 (smaller when many
   systems are shown). A simple logo mark reads best.
   Size:  128 x 128 square, transparent PNG.

5. assets/icons/bookshelf/<system>/    (one image)
   The book spine in Bookshelf view. Spine face is 76 x 260 portrait; the
   selected book grows ~8%.
   Size:  152 x 520 portrait (2x), transparent PNG. Keep art/text centred with
          a ~10 px safe margin and a little headroom at the top.
   (assets/icons/bookshelf/background/ is the shared shelf photo -- already set.)


6. assets/icons/home/simple/ and assets/icons/home/pixel-art/
   Separate App Focused home icon packs, selectable in settings. Preserve
   the pack's app filenames. SVG and transparent PNG are supported.

The release includes the bundled art in these folders. Systems without custom
art fall back to labeled placeholders. Personal ROMs, BIOS, account files and
scraped game art are not distributed. Back up artwork you customize before
updating: an update replaces bundled files with matching filenames.
