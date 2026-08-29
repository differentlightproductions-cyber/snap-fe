List view art (Settings > Display > Console View > List)

Filename-agnostic: drop in any .png / .jpg / .jpeg, name it anything.
The first image found in each folder is used.

  background/            the one fixed List-view backdrop. Not user-changeable
                        in Settings -- this file is the only way to set it.
                        720 x 480 or larger at 3:2; COVER-fit (centre-cropped).

  gb/  gbc/  gba/        the per-system badge shown beside each row in the list.
  nes/ snes/ genesis/    Small; a square-ish logo works best. No file -> the
                        row just shows the system name.

The right-hand panel now shows built-in system info (name, maker, year,
game count, description) -- no scraping needed, nothing to place here.
