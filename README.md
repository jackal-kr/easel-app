# Easel

Draw and touch up pictures. A simple, native GNOME paint program in the style of Paint in Windows XP.

![Easel](data/screenshots/main.png)

Easel has the classic tools and workflow, built with GTK 4 and libadwaita, so it looks and behaves like a GNOME app: header bar, dark style, native dialogs. Pictures stay exact: hard edges and no surprise smoothing.

**Status:** 1.0.0, the first full release.

## Try it
Download the Flatpak or the .deb from the [Releases](https://github.com/jackal-kr/easel-app/releases) page.

    flatpak install --user easel-1.0.0.flatpak              # any distribution
    sudo apt install ./easel_1.0.0_amd64.deb                 # Ubuntu 26.10, x86-64

The Flatpak needs Flathub for the GNOME runtime. The [release notes](docs/release-notes-1.0.0.md) have the full steps, how to remove it, and the checksums.

## What it does
- **Tools:** Pencil, Brush, Airbrush, Eraser, Fill, Pick Color, Magnifier, Text, Line, Curve, Shapes (19 of them, adjustable before they are put down), Polygon, Select and Free-Form Select, plus Pan and Gradient.
- **Selections:** move, copy, crop, invert, smear, transparent mode, clipboard, flip, rotate, stretch and skew.
- **Image:** Remove Background (with Keep this and Remove this brushes), Black and White, gradients (Linear, Radial, Reflected), and optional smooth edges for lines, shapes, brushes and text.
- **View:** zoom from 10 % to 1600 %, a pixel grid and a thumbnail.
- **History:** undo and redo within a memory budget you can change.
- **Files:** open and save PNG, JPEG, BMP, TIFF and WebP; open GIF; print.
- **Palette:** 28 colors, edited in place.
- **Other:** draw with the keyboard, screen-reader labels, a shortcuts window and a help page in 12 languages (cs, de, en, es, fr, hu, it, pl, pt, ro, ru, uk). Most translations are drafts; corrections are welcome.

## Build from source
Needs GTK 4.16 or newer, libadwaita 1.8 or newer, Meson and a C compiler.

    meson setup build
    meson compile -C build
    ./build/src/easel
    meson test -C build --print-errorlogs

## How it is made
Easel is built spec first: every feature has a specification in `docs/specs/` and tests that check it, and the code follows from them. See [how we work](docs/how-we-work.md), the [product](docs/product.md) and [constitution](docs/constitution.md) documents, and the [feature list](docs/specs/INDEX.md). The code is written by an AI assistant (Claude) from those specs, under the instructions in [CLAUDE.md](CLAUDE.md); the owner checks each feature by using the app, not by reading every line.

## Credits and license
Inspired by Paint in Windows XP. How its classic tools behave was also studied in [JS Paint](https://github.com/1j01/jspaint) by Isaiah Odhner (MIT); no code is copied.

Licensed under the GPL-3.0-or-later. See [LICENSE](LICENSE).
