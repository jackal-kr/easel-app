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
| 0014 | free-form-selection | Free-Form Select, Shift smear and stamp, stretching a selection by its handles | Split from Selection (2026-10-20) |
| 0015 | clipboard | Cut, copy, paste, oversize paste, paste from / copy to file | |
| 0016 | text | Text box, fonts, styles, per-selection fonts | |
| 0017 | image-transforms | Flip, rotate, stretch, skew, invert, clear, black and white | |
| 0018 | preferences | Appearance, new-picture and drawing defaults, palette persistence, undo budget | Dialog edits the same configuration file the app already reads. Two smoothing defaults (Smooth text; Smooth shapes and brushes) arrive with Phase 2. Open question from 0005: ask before Save overwrites an opened file (today silent, as in XP)? Candidates: a setting, once per opened file, or only for lossy formats; also `[save] jpeg-quality` |
| 0019 | window-extras | Shortcuts window, thumbnail, frame time, print, context menus, About with acknowledgements (XP Paint, JS Paint) | Adds the View menu's Show Thumbnail item (not in 0004); Frame Time has no View item, only a Preferences setting (Preferences) |
| 0020 | accessibility | Names/roles, keyboard-only drawing and palette | |
| 0021 | i18n | UI languages (pl, uk, ru, de, cs, it, es, pt + English) | Mark the 0001 strings (title, quit and save prompts, errors) translatable; deferred from the 0001 check |
| 0022 | packaging-release | Flatpak, deb, AppImage, metadata | |

### Phase 2: Modern extras (optional, starts only after the core XP scope is delivered)
| Feature | One line |
|---|---|
| Crop to selection | Crops the picture to the current selection in one command |
| Zoom slider and wheel zoom in every tool | A zoom slider in the status bar, and Ctrl+wheel zooms with any tool, not only in Pan mode |
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
| M2 Shapes and selection | 0010-0017 | XP feature parity for editing |
| M3 Polish | 0018-0021 | Preferences, accessibility, languages |
| M4 Release | 0022 | Flatpak, deb, AppImage |
