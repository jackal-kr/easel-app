# Feature map

Order = dependency order. One file per feature: `docs/specs/NNNN-slug.md`. Create with `tools/spec.sh new NNNN slug`, list status with `tools/spec.sh status`.
Vertical slices first (a thin end-to-end feature), breadth later.

| No | Slug | One line | Notes |
|---|---|---|---|
| 0001 | walking-skeleton | Start in Pan mode on a blank canvas (size and background from the configuration file), pan and zoom it, draw with the pencil, save as PNG, quit with an unsaved-changes prompt | **Pilot.** Includes Pan and the settings file. Write together, slowly |
| 0002 | undo-redo | Undo/redo with memory budget and "saved" state | Deferred idea: keep old undo steps on disk instead of dropping them; revisit only if 512 MB proves too small |
| 0003 | colors-palette | Palette, fore/background colors, Edit Colors | |
| 0004 | canvas-view | Zoom range and commands, Fit to Window, View and Image menus with panel toggles, status bar, pixel grid and Grid dialog, resize handles, Attributes dialog | Builds on Pan from 0001. History steps store pixel indexes; any change of picture size must clear the history or record the size. 8000x8000 now draws, undoes and redoes within one refresh: the picture is shown as tiles (T001, T002, measured in the 0004 Plan) |
| 0005 | file-io | New, Open and Save As, open/save formats (png, jpeg, bmp, tiff, webp; gif open-only), modified tracking for Open and New, a file on the command line, errors | gif open-only; the saved state itself is 0002. Open and New clear the undo history; sizes may differ (a resize inside one picture is a history step, 0004; a different picture is not) |
| 0006 | freehand-tools | Brush shapes/sizes, airbrush, eraser | |
| 0007 | fill-pick-magnify | Fill, pick color, magnifier | Magnifier levels 1/10x, 1/4x, 1/2x, 1x, 2x (default), 4x, 6x, 8x, chosen then applied on a click; Pick Color returns to the previous tool |
| 0008 | zoom-presets | One list of zoom presets for the Magnifier's options and a new View > Zoom submenu | Accepted 2026-10-18, from JOD's screencast; the lens list (no 300 %) is the preset list, Zoom In and Out keep their own steps with 300 % |
| 0009 | view-polish | A 12 px margin and a soft shadow around the picture, a workspace color setting | Implemented 2026-10-18, from JOD's reference measurements; touches every rule that maps the pointer to a picture pixel After it, the picture-color key `background` was renamed `canvas-color` (still read as `background`; a new settings file writes the new name; a valid `canvas-color` wins): REQ-0001-03, done 2026-10-18 |
| 0010 | lines-and-shapes | Line, rectangle, rounded rectangle, ellipse, the line width and the fill style | Implemented 2026-10-19; split from "lines, curves and shapes" by JOD; widths 1-5 shared, a square pen. After it, a small change to the freehand tools: the stroke history's log is shrunk to its length before it is pushed, as the shapes' log now is, and a test checks the memory held against the memory counted for a long stroke (REQ-0002-16) |
| 0011 | curve-and-polygon | Curve and polygon: the multi-click tools, with their pending state | Implemented 2026-10-19; split from "lines, curves and shapes"; uses the line width of lines and shapes; the first pending shape (put down by what changes the picture or its file, dropped by Esc and Undo). After it, a small change, done 2026-10-19: `tools/isolated.sh` runs the ui tests, the mutation tool and `/sdd-check` in a private session (a headless GNOME Shell with its own display; the spike found no `Xvfb` or Weston here, and `GDK_BACKEND=x11` would test another backend), so a real pointer and JOD's own work cannot affect them; the benchmarks stay in the real session |
| 0012 | gradient | The Gradient tool (`docs/reference/new-behaviors.md`) | Implemented 2026-10-20; split from "lines, curves and shapes"; a pending tool by the rules of curve-and-polygon; a click, a new drag or Enter applies it, its colors follow the palette, its ends have markers joined by a guide line |
| 0013 | selection | Select (rectangle), the floating selection, move, Ctrl copy-move, arrow keys, put down, Clear Selection, Select All, Opaque / Transparent, the selection menu, the Edit submenu | Implemented 2026-10-20; split from "rect and free-form select, ..." (JOD): free-form, smear, stamp and stretching by handles are Free-form Selection |
| 0014 | free-form-selection | Free-Form Select, Shift smear and Shift-click stamp (for the rectangle too) | Implemented 2026-10-22; split from Selection (2026-10-20); stretching by handles is its own feature after it (JOD, 2026-10-20). Accepted slow cases: a 100000-position trace on 8000 x 8000 at the fit zoom, about 24 ms a frame and about 60 ms its release (the 0014 Plan) |
| 0015 | selection-handles | The eight handles of a selection and stretching it by them (rectangle and free-form) | Implemented 2026-10-22; split from Free-form Selection (JOD, 2026-10-20). Deferred idea: a display-resolution preview of a huge stretch at the fit zoom, only if the 450 ms an event of 8000 x 8000 becomes a complaint |
| 0016 | clipboard | Cut, copy, paste, oversize paste, paste from / copy to file | Implemented 2026-10-26; adds Cut, Copy, Paste and Copy To… to the selection menu (REQ-0013-17) and the canvas menu outside a selection; pointer notes in 0004, 0005, 0011, 0012, 0013, 0014, 0015. Deferred idea: keep the copy after quit (run without a window) when no clipboard manager keeps it; not planned |
| 0017 | text | Text box, fonts, styles, per-selection fonts, rich text on copy and paste | Implemented 2026-10-27; formatting can differ within one box (font, size, color, bold, italic, underline, strikethrough); copying or cutting text from a box puts rich text, with its formatting, on the clipboard besides plain text; pasting rich text into a box keeps its formatting, and plain text takes the box's current style; takes over "text in the clipboard" from the Out of scope of 0016 |
| 0018 | image-transforms | Flip, rotate, invert colors, clear image, flipping a selection by dragging a handle past the opposite edge | Implemented 2026-10-28. Split (JOD, 2026-10-27): Stretch and Skew is its own feature after it, Black and white a later feature. Flip › and Rotate › submenus with Invert Colors (Ctrl+I) join the selection menu (REQ-0013-17); while a text box is open Ctrl+I is Italic (Text, REQ-0017-12); Clear Image (Shift+Ctrl+N) also goes in the third section of the canvas menu (0016, REQ-0016-14). Accepted memory case (JOD, 2026-10-27, perf-0018): a command on an 8000 × 8000 picture made right after an Undo peaks at 845 MB resident (three buffers alive for about 190 ms) against 602 MB for the first command in the window; a shortage is REQ-0018-10's dialog with nothing changed. A second memory case (JOD, 2026-10-27, perf-0018): a flip or a turn of an 8000 × 8000 floating selection made after earlier commands peaks at 1101 MB resident (the lifted image, the old shown image and the new shown one alive together for about a quarter of a second), 858 MB for the first command; a shortage is REQ-0018-10's dialog with nothing changed. Accepted slow cases (perf-0018 `handle-flip`, 3 runs, load 0.3-0.4, 2026-10-27; accepted by JOD, 2026-10-27): a stretch by a handle of an 8000 × 8000 selection of the whole picture across the opposite corner and back. At 100 %: the lift 72, 79 and 289 ms, the events after it a frame (medians 15.9-16.8 ms, worst 17.6-19.1 ms), the crossing event 8-14 ms, peak 623, 623 and 859 MB resident (358 before). At the fit zoom (10 %), where the whole image is made at each event: the lift 589-632 ms, the crossing event 177-182 ms, the other events a median of 16.4-16.6 ms with the worst of the ten at 37.5-48.9 ms, release 17 ms, peak 1570-1571 MB resident (360 before); 0015's own stretch at that size and zoom is the comparison |
| 0019 | stretch-skew | Stretch and Skew of the picture or a selection, in percent or pixels, with Keep aspect ratio | Implemented 2026-11-03; split from Image Transforms (JOD, 2026-10-27); Ctrl+T, since Ctrl+W closes the window; Black and white comes after it (a later feature) |
| 0020 | preferences | Appearance, new-picture and drawing defaults, palette persistence, undo budget | Dialog edits the same configuration file the app already reads. The strength of the selection's tint (Select, Free-Form Select and the text box; default 12 %, REQ-0014-17 b) is an Appearance setting (JOD, 2026-10-28). Two smoothing defaults (Smooth text; Smooth shapes and brushes) arrive with Phase 2. Open question from 0005: ask before Save overwrites an opened file (today silent, as in XP)? Candidates: a setting, once per opened file, or only for lossy formats; also `[save] jpeg-quality` |
| 0021 | window-extras | Shortcuts window, thumbnail, frame time, print, context menus, About with acknowledgements (XP Paint, JS Paint) | Adds the View menu's Show Thumbnail item (not in 0004); Frame Time has no View item, only a Preferences setting (Preferences) |
| 0022 | accessibility | Names/roles, keyboard-only drawing and palette | stretching a selection from the keyboard (0015 has the mouse only) |
| 0023 | i18n | UI languages: cs, de, en, es, fr, hu, it, pl, pt (pt_BR), ro, ru, uk (uk_UA); English the default and fallback | Mark the 0001 strings (title, quit and save prompts, errors) translatable; deferred from the 0001 check. The language picker goes in Preferences; no right-to-left language |
| 0024 | packaging-release | Flatpak, deb, AppImage, metadata | Choose the minimum GTK, then re-decide the accepted mutant of REQ-0017-14 (JOD, 2026-11-02: GTK 4.24 cannot kill it) |
| 0025 | pixel-contract-tests | The six points of P7 (ADR-0003) as requirements, one test per gap, and fixes for what the tests find wrong (fractional-zoom edges, the grid and the pointer on them): the picture as squares, the same drag at two zooms, overlays leaving the picture unchanged, shown equals put-down | Accepted 2026-10-28; runs in this order (JOD): /sdd-check 0018, the 0017 text and tint task, then this feature (its text-box cases fail until the 0017 fix lands); the number moves when it is placed in the order |

### Phase 2: Modern extras (optional, starts only after the core XP scope is delivered)
| Feature | One line |
|---|---|
| Crop to selection | Crops the picture to the current selection in one command |
| Zoom slider and wheel zoom in every tool | A zoom slider in the status bar (its range 10 to 1600 %), and Ctrl+wheel zooms with any tool, not only in Pan mode |
| Invert selection | Selects everything outside the current selection |
| Smooth text option | An opt-in switch that draws text with antialiasing; off by default |
| Smooth shapes and brushes option | One opt-in switch that draws lines, curves, shapes and brush strokes with antialiasing; off by default |
| Extra shapes | Triangles, diamond, pentagon, hexagon, arrows, stars, callouts, heart, lightning bolt |

## Roadmap notes
- Gradient modes (a follow-up to Gradient, no number yet): more than the one fade of Gradient, in a later spec of its own.
- The Phase 2 block is a plan, not a commitment of numbers: a row gets its number when its spec is written.
- After Gradient: a later change draws the picture and the gradient preview through one path below 100 %, so that the preview
  is exact and its memory bounded (today a gradient dragged at the fit zoom of an 8000 x 8000 picture takes about 390 ms a frame
  and holds the picture's size in memory; the same for a selection dragged at that zoom, about 165 to 173 ms a move and 470 MB, and for the put-down of the whole 8000 x 8000 picture, about 380 ms and 1.8 GB, accepted with 0013). It needs its own spec and Plan.

## Last clean full mutation run
2026-11-02, the commit of 0025's T017: 1140 mutants, 1 accepted, 1 long-only killed by hand. GTK 4.24.1, Pango 1.58.0, Cairo 1.18.4, GNOME Shell 51.0.

## Known flakes
- `/0006/as6_airbrush_sprays` and `/0006/as6_airbrush_follows_the_pointer` (found in 0006, failed again in 0007 and 0008). Cause
  established in the 0011 check (2026-10-19): a real pointer motion over the test window with no button down reads as a lost
  release (REQ-0007-18) and ends the test's held (synthetic) stroke, so the airbrush stops spraying (`made >= 100`: 4, 3, 3, 3,
  1, 1). With a virtual mouse sweeping about 100 pixels over the windows `ui-0006` failed 6 of 6 at `as6_airbrush_sprays`; with
  the pointer parked there it passed 5 of 5 (`ui-0010`: 5 of 5 both ways). The earlier reading (its result depends on the wall
  clock and a real GLib timer) is true but is not the cause. Avoided, not fixed in the test: the ui tests and the mutation runs
  go through `tools/isolated.sh` (a headless GNOME Shell with its own display, 2026-10-19). With the same virtual mouse wiggling
  over the real screen, `ui-0006` passed 5 of 5 in the isolated display and, as the control, failed in the real session (`made
  >= 100`: 3); the whole ui suite passed 11 of 11 there.

- `/0011/as5_polygon_clicks` and a mutant run of `/0011/as2_curve_stages_and_preview` (found in 0011: once in about 17 runs after
  a rebuild, and once as a crash of the full check's mutation run, `crash-210.log`): `Gsk-FATAL-CRITICAL:
  gsk_renderer_render_texture: assertion 'GSK_IS_RENDER_NODE (root)' failed` in `render_pixels`, a window with no snapshot; not
  reproduced under CPU load, the mutant alone died by its assertion 3 of 3 times. Cause unknown. Changed in the 0011 check (F6):
  `render_pixels` waits up to 30 frames for a snapshot, and a window with none ends the run with `EASEL-INCONCLUSIVE: no
  snapshot` and exit status 75, which is no longer a crash: `tools/mutate.sh` runs that mutant once more and counts a second one
  apart (failing the run), never as killed. Hunted when it recurs as "inconclusive".

## Milestones
| Milestone | Features | Outcome |
|---|---|---|
| M0 Pilot | 0001 | Whole loop felt once; templates and skills tuned |
| M1 Core drawing | 0002-0009 | Usable paint program: draw, fill, pick, zoom, undo, open/save |
| M2 Shapes and selection | 0010-0019 | XP feature parity for editing |
| M3 Polish | 0020-0023 | Preferences, accessibility, languages |
| M4 Release | 0024 | Flatpak, deb, AppImage |
