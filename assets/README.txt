Snap FE artwork -- where each image goes and what size it should be
=================================================================

Use a sub-folder with the system's Knulli short name, for example gba, gbc,
gb, nes, snes, genesis, psx or amiga500. Keep existing folders and filenames
when replacing bundled artwork. PNG/JPG/JPEG files are supported; transparent
PNG is useful for icons. See USER-FOLDERS.md in the release for the full guide.


1. assets/backgrounds/<system>/
   Full-screen backdrop on the console screen (Single Card / Carousel / Grid
   when Background = Image). Every image here becomes a pickable option in
   Settings > Display > Backgrounds. Downloaded images can have descriptive labels.
   Size:  1440 x 960 (3:2) is the standard master size. JPG or PNG.
          1280 x 960 is suitable for a dedicated 4:3 version. Existing
          1280 x 854 or 960 x 640 images still work. Cover-fit (centre-cropped),
          so keep important art/text away from the outer edges.

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
