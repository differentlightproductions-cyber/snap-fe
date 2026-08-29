Snap FE artwork -- where each image goes and what size it should be
=================================================================

Every folder below has one sub-folder per system, using these slugs:

  gba  gbc  gb  nes  snes  genesis            (art already included)
  n64  psx  mastersystem  gamegear            (NEED ART)
  pcengine  neogeo  atari2600  fbneo          (NEED ART)

Drop a .png / .jpg / .jpeg in the matching folder. For everything except
backgrounds/, the FIRST image found in the folder is used -- name it anything.


1. assets/backgrounds/<system>/
   Full-screen backdrop on the console screen (Single Card / Carousel / Grid
   when Background = Image). Every image here becomes a pickable option in
   Settings > Display > Background; the filename (minus extension) is its label.
   Size:  1280 x 854  (3:2), or at least 960 x 640. JPG or PNG. Cover-fit
          (centre-cropped), so keep the subject roughly centred.

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


WHAT'S MISSING RIGHT NOW
-----------------------
The 8 newly added systems have NO art in any of the folders above:

  n64  psx  mastersystem  gamegear  pcengine  neogeo  atari2600  fbneo

  -> per system that's: 1 background, 1 carousel card, 1 list badge,
     1 bookshelf spine  = 4 images each, 32 total (grid icons optional).

Grid icons: none exist for ANY system (all 14) -- the grid view falls back to
a themed box + short name, so these are lowest priority.

Until art is added, each of those systems shows a clean themed placeholder
with its name, so nothing is broken -- it just isn't illustrated.
