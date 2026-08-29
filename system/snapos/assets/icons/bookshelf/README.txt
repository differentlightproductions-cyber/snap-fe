Bookshelf view art (Settings > Display > Console View > Bookshelf)

Filename-agnostic: drop in any .png / .jpg / .jpeg, name it whatever you
like. The first image found in each folder is used.

  background/            one backdrop for the whole shelf + open-book view.
                         Panel is 720 x 480; supply that or larger at 3:2
                         (e.g. 1440x960). COVER-fit: centred and cropped to
                         fill, never stretched.

  gb/  gbc/  gba/        the book spine for that system, one folder each.
  nes/ snes/ genesis/    Displayed spine face: 76 x 260 px (portrait) -- author
                         at 2x (152 x 520), transparent PNG. COVER-fit into the
                         spine, so keep artwork/text centred with a ~10px safe
                         margin. The selected book scales up ~8% and lifts, so
                         leave a little headroom at the top.
                         No file -> a coloured spine with the short name on it.

Bookshelf never uses the per-system assets/backgrounds/<system>/ images.
