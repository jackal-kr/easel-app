# Easel

A GNOME-native clone of Windows XP Paint, built spec-first: specifications are the source of truth and code is derived from them.

Status: in development. Each feature's status is in its spec (`docs/specs/`, or run `tools/spec.sh status`).

## Scope
All the tools and menus of Windows XP Paint, plus Pan and Gradient (`docs/product.md`). XP-faithful by default; a later optional
phase adds a few selected modern extras, and smoothing (antialiasing) is an option, off by default.

## Non-goals# Easel

Draw and touch up pictures. A simple, native GNOME paint program in the style of Paint in Windows XP.

![Easel](data/screenshots/main.png)

Easel has the classic tools and workflow, built with GTK 4 and libadwaita, so it looks and behaves like a GNOME app: header bar, dark style, native dialogs. Pictures stay exact: hard edges and no surprise smoothing.

**Status:** early alpha (0.1.0-alpha.1). It is usable, but expect rough edges and keep copies of pictures you care about.

## Try it
Download the Flatpak or the deb from the [latest release](https://github.com/jackal-kr/easel-app/releases/latest).

    flatpak install --user easel-0.1.0-alpha.1.flatpak      # any distribution
    sudo apt install ./easel_0.1.0-alpha.1_amd64.deb         # Ubuntu 26.10, x86-64

The Flatpak needs Flathub for the GNOME runtime. The [release notes](docs/release-notes-0.1.0-alpha.1.md) have the full steps, how to remove it, and the checksums.

## What it does
- **Tools:** Pencil, Brush, Airbrush, Eraser, Fill, Pick Color, Magnifier, Text, Line, Curve, Rectangle, Rounded Rectangle, Ellipse, Polygon, Select and Free-Form Select, plus Pan and Gradient.
- **Selections:** move, copy, smear, transparent mode, clipboard, flip, rotate, stretch and skew.
- **View:** zoom from 10 % to 1600 %, a pixel grid and a thumbnail.
- **History:** undo and redo within a memory budget you can change.
- **Files:** open and save PNG, JPEG, BMP, TIFF and WebP; open GIF; print.
- **Palette:** 28 colors, edited in place.
- **Everyone:** draw with the keyboard, screen-reader labels, a shortcuts window and a help page in 12 languages (cs, de, en, es, fr, hu, it, pl, pt, ro, ru, uk). Most translations are drafts; corrections are welcome.

## Build from source
Needs GTK 4.16 or newer, libadwaita 1.8 or newer, Meson and a C compiler.

    meson setup build
    meson compile -C build
    ./build/src/easel
    meson test -C build --print-errorlogs

## How it is made
Easel is built spec first: every feature has a specification in `docs/specs/` and tests that check it, and the code follows from them. See [how we work](docs/how-we-work.md), the [product](docs/product.md) and [constitution](docs/constitution.md) documents, and the [feature list](docs/specs/INDEX.md).

## Credits and license
Inspired by Paint in Windows XP. How its classic tools behave was also studied in [JS Paint](https://github.com/1j01/jspaint) by Isaiah Odhner (MIT); no code is copied.

Licensed under the GPL-3.0-or-later. See [LICENSE](LICENSE).

XP visual styling; the third (Ctrl) color slot; layers; non-destructive editing; plugins; cloud features; Windows/macOS ports;
ribbon interface, editable shapes after commit, brush gallery and rulers (not planned for Phase 2).

## How we work
Work happens in VS Code with the Claude Code extension. One spec file per feature in `docs/specs/`; three steps:
`/sdd-spec NNNN` (write and settle the spec), `/sdd-build NNNN` (plan, tasks, implement test-first), `/sdd-check NNNN` (review).
Rules: `CLAUDE.md` and `docs/constitution.md`. Feature order: `docs/specs/INDEX.md`. Note surprises in `docs/learning-log.md`.

Learning path: read the constitution and change what you disagree with; run the pilot (0001) through all three steps slowly;
note what felt heavy or useful; trim the process accordingly; then continue through the INDEX.

## Inspired by
- **Microsoft Paint in Windows XP**: the original whose functionality and workflow this app follows (XP version specifically, not later Paint releases).
- **[JS Paint](https://github.com/1j01/jspaint) by Isaiah Odhner**: studied for how the XP behavior works. MIT licensed; no code is copied.
