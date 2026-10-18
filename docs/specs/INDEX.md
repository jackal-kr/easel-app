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
| 0008 | lines-curves-shapes | Line, curve, polygon, rectangle, rounded rectangle, ellipse, fill styles, gradient | |
| 0009 | selection | Rect and free-form select, floating selection, move, smear, stamp, transparent mode | |
| 0010 | clipboard | Cut, copy, paste, oversize paste, paste from / copy to file | |
| 0011 | text | Text box, fonts, styles, per-selection fonts | |
| 0012 | image-transforms | Flip, rotate, stretch, skew, invert, clear, black and white | |
| 0013 | preferences | Appearance, new-picture and drawing defaults, palette persistence, undo budget | Dialog edits the same configuration file the app already reads. Open question from 0005: ask before Save overwrites an opened file (today silent, as in XP)? Candidates: a setting, once per opened file, or only for lossy formats; also `[save] jpeg-quality` |
| 0014 | window-extras | Shortcuts window, thumbnail, frame time, print, context menus, About with acknowledgements (XP Paint, JS Paint) | Adds the View menu's Show Thumbnail item (not in 0004); Frame Time has no View item, only a Preferences setting (0013) |
| 0015 | accessibility | Names/roles, keyboard-only drawing and palette | |
| 0016 | i18n | UI languages (pl, uk, ru, de, cs, it, es, pt + English) | Mark the 0001 strings (title, quit and save prompts, errors) translatable; deferred from the 0001 check |
| 0017 | packaging-release | Flatpak, deb, AppImage, metadata | |

## Known flakes
- `/0006/as6_airbrush_follows_the_pointer` (found in 0006, failed again in 0007, cause not found; the log has the data). Read
  on 2026-10-18, nothing changed: yes, its result depends on the wall clock and a real GLib timer. The test holds the
  press with `run_for (300)`, which spins the main loop for 300 ms of real time (`tests/ui/test-window.c`:
  `gint64 end = g_get_monotonic_time () + ms * 1000; while (g_get_monotonic_time () < end) g_main_context_iteration (NULL, FALSE);`),
  and the canvas sprays from `g_timeout_add (5, on_spray_tick, self)`, which asks `easel_spray_due (g_get_monotonic_time () -
  self->spray_start_us)` how many sprays are due (`src/ui/canvas.c`), with the seed from `g_get_monotonic_time ()` and
  `g_get_real_time ()`. No clock is injected; the core's exact count (201 in 1000 ms) uses a simulated clock, the ui test
  does not. Hypothesis (not tested): displaced pixels mean a different number of random draws, which means a different
  spray count under load. Doubt to check first: a different count changes how many pixels land, and where they land only
  if the draws shift the sequence between the two points; the pixels seen were about 91, 32 pixels away, which a count alone
  does not explain. Not hunted.

## Milestones
| Milestone | Features | Outcome |
|---|---|---|
| M0 Pilot | 0001 | Whole loop felt once; templates and skills tuned |
| M1 Core drawing | 0002-0007 | Usable paint program: draw, fill, pick, zoom, undo, open/save |
| M2 Shapes and selection | 0008-0012 | XP feature parity for editing |
| M3 Polish | 0013-0016 | Preferences, accessibility, languages |
| M4 Release | 0017 | Flatpak, deb, AppImage |
