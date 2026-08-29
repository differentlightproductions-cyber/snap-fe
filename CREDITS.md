# Credits & third-party licenses

## Runtime libraries (not bundled — loaded from the OS)

- **SDL2**, **SDL2_ttf**, **SDL2_image** — zlib license. On the device these
  ship with Knulli; on desktop you install them yourself.

## Bundled fonts — `assets/fonts/`

| File | Family | License |
|---|---|---|
| `DejaVuSans.ttf`, `DejaVuSerif.ttf` | DejaVu | DejaVu Fonts License (Bitstream Vera derivative, permissive) |
| `Ubuntu-R.ttf`, `Ubuntu-B.ttf`, `Ubuntu-C.ttf`, `UbuntuMono-R.ttf` | Ubuntu / Ubuntu Mono | Ubuntu Font Licence 1.0 |
| `Poppins-Bold.ttf` | Poppins | SIL Open Font License 1.1 |
| `PressStart2P.ttf` | Press Start 2P | SIL Open Font License 1.1 |
| `Silkscreen-Regular.ttf` | Silkscreen | SIL Open Font License 1.1 |
| `VT323-Regular.ttf` | VT323 | SIL Open Font License 1.1 |

All are redistributable under the terms above. Full license texts ship with
each font upstream (Google Fonts / the DejaVu and Ubuntu projects).

## Bundled sounds — `assets/sounds/`

`boot_chime.wav`, `boot_chime_2.wav`, `ui_click.wav`, `theme_music_1.wav` —
made for Snap FE, covered by this repo's MIT license.

## Not in this repo

Console wallpapers and system icons are **not** included — their provenance
varies and much of it is third-party. `assets/backgrounds/` and
`assets/icons/<view>/` carry `README.txt` files describing the expected layout;
the frontend falls back to solid colours / text when art is missing. Add your
own, or drop art into `system/snapos/assets/...` on the device.

## Platform

Knulli CFW (Batocera 42 base) — its own project and licenses. Snap FE is a
separate program that runs on top of it and is not affiliated with the Knulli
or EmulationStation projects.
