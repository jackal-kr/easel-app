# 0006 Freehand tools

Status: accepted
Depends on: 0001, 0002, 0003, 0004

## Why
Pencil draws one pixel wide. Quick sketches and touch-ups need XP Paint's other freehand tools: a brush with a few shapes
and sizes, an airbrush that sprays, and an eraser, each with its options below the toolbox. This feature also adds the
options panel that later tools will use.

## Requirements

### Toolbox and options panel
- REQ-0006-01 The toolbox shall show Brush, Airbrush and Eraser next to Pan and Pencil, in a grid of two columns, with the tooltips "Brush", "Airbrush" and "Eraser". Only the tools that work are shown, packed in the order of the final layout (Pan, Magnifier, Select, Free-Form Select, Eraser, Pick Color, Fill, Gradient, Pencil, Brush, Airbrush, Text, Line, Curve, Rectangle, Rounded Rectangle, Ellipse, Polygon), so in this feature: Pan, Eraser / Pencil, Brush / Airbrush; each later feature adds its tools in their place in that order.
- REQ-0006-02 Below the toolbox, separated from it by a line, the app shall show an options panel for the active tool, always visible while the toolbox is (Tool Box of REQ-0004-27 hides and shows both): the presets of REQ-0006-04, REQ-0006-07 and REQ-0006-10 as toggle buttons in two columns, filled row by row in the order those requirements give (so the Brush's first row is circle 7, circle 4, its second circle 1, square 8), with one selected at a time per group. A size preset shows the tool's footprint drawn at one logical pixel per picture pixel, and has a tooltip naming it ("Circle, 7 pixels", "Square, 8 pixels", "Slash, 8 pixels", "Backslash, 8 pixels", "Spray, 16 pixels", "Eraser, 8 pixels"), which is also its accessible name. The Eraser's two mode presets show an opaque and a transparent square. Pan and Pencil have no options: the panel and its line are hidden while they are active. Pencil stays one pixel wide, with no options (REQ-0001-20, as XP).
- REQ-0006-03 Each tool shall keep its own selected preset for the rest of the session: switching to another tool and back shows the same preset. At start-up the presets are the ones REQ-0006-04, REQ-0006-07 and REQ-0006-10 name as the default; they are not settings in this feature: Preferences (0013) will make them settings in the configuration file, as principle P6 asks, by the owner's decision of 2026-10-16. Choosing a preset or mode during a stroke shall be ignored (as REQ-0001-09 does for tools).

### Brush
- REQ-0006-04 The Brush shall have twelve presets, in this order: circles of 7, 4 and 1 pixels; squares of 8, 5 and 2 pixels; lines from bottom-left to top-right ("/") of 8, 5 and 2 pixels; lines from top-left to bottom-right ("\") of 8, 5 and 2 pixels. The default is the 4-pixel circle.
- REQ-0006-05 The footprint of each preset, in picture pixels, shall be: a circle of diameter d is the pixels of a d × d box whose centers lie within d / 2 of the box's center (so the 7-pixel circle is the mask of AS-3 and the 4-pixel circle is `.##.` `####` `####` `.##.`); a square of d is the whole d × d box; "/" of d is the box's diagonal from bottom-left to top-right, d pixels; "\" is the other diagonal. The box is placed with the pixel under the pointer at column and row ⌊(d − 1) / 2⌋ of it.
- REQ-0006-06 While the Brush is active, pressing the primary button shall paint one footprint at the pointer's pixel, and dragging shall paint the footprint at every pixel of the 8-connected line between each pair of consecutive pointer positions (REQ-0001-20), in the foreground color; with the secondary button, in the background color (REQ-0001-21). Every stamp is placed, so no pointer position is skipped, but a slanted footprint moved across its own direction leaves the space between its stamps as the 8-connected line does. The stripes stay, as XP shows them: a "/" brush dragged along "\\" leaves an empty diagonal between each two stamps. Nothing is antialiased, only the parts inside the picture are painted (REQ-0001-22), and a stroke uses the colors that are current when it starts, to its end (REQ-0003-08).

### Airbrush
- REQ-0006-07 The Airbrush shall have three presets: discs of 9, 16 and 24 pixels in diameter (the disc of REQ-0006-05's circle rule). The default is 16.
- REQ-0006-08 While the Airbrush is active and the primary or secondary button is held (the middle button does nothing, REQ-0001-24), the app shall spray: one spray at the press, and one more for every full 5 ms the button has been held since (xp-behaviors.md; a late spray is caught up, so the count follows the time held, not the timer). A spray paints a fixed number of pixels (below), each chosen at random, independently and with equal chance, from the disc around the pointer's position at that moment (so a pixel can be chosen twice), in the foreground color for the primary button and the background color for the secondary one, with the colors current at the press (REQ-0003-08); only pixels inside the picture are painted. It sprays also while the pointer rests, and while it is held outside the canvas area (where nothing is painted); it stops at the release. A fast move leaves gaps, since a spray lands only where the pointer is at that moment. A spray paints 1 pixel for the 9- and 16-pixel discs and 3 for the 24-pixel one, so that held still each disc is two thirds full or more after one second.
- REQ-0006-09 The random choice of REQ-0006-08 shall start differently each time, so two sprays at the same point give different pixels (with near certainty, not by rule), and every sprayed pixel shall be inside the disc at the pointer's position at the moment it was sprayed.

### Eraser
- REQ-0006-10 The Eraser shall have four size presets, squares of 4, 6, 8 and 10 pixels (footprint as a square of REQ-0006-05), default 8, and below them, after a line, two mode presets: Opaque (default) and Transparent, with the tooltips "Opaque: erase to the background color" and "Transparent: erase to transparency". The mode stays when the size changes and the other way round.
- REQ-0006-11 While the Eraser is active, pressing and dragging with the primary button shall paint its footprint as the Brush does (REQ-0006-06): with the background color in Opaque mode, and with fully transparent pixels (#00000000) in Transparent mode.
- REQ-0006-12 While the Eraser is active, pressing and dragging with the secondary button (XP's color eraser) shall change, within the footprint along the stroke, only the pixels equal to the foreground color in all four channels (a fully transparent pixel matches only a fully transparent foreground of the same value), to the background color; other pixels stay. If the foreground and background colors are equal, nothing changes and no step is added. In Transparent mode those pixels become fully transparent (#00000000) instead, so the mode applies to both buttons.

### Shared rules
- REQ-0006-13 A stroke of any of the three tools, from press to release, shall be one undo step (REQ-0002-01), and a stroke that changes no pixel adds none (REQ-0002-02). The other button during a stroke cancels it (REQ-0001-23), losing the focus or releasing outside the window ends it and keeps it (REQ-0001-25), and Undo during it cancels it (REQ-0002-09); a cancelled spray stops spraying.
- REQ-0006-14 When the user presses Numpad + or Numpad − (the keypad keys; the main keyboard's + and − do nothing here), wherever keyboard focus is in the window, the app shall select the next larger or next smaller preset of the active tool, one step per key press or key repeat: for the Brush within the same shape (7, 4, 1 for circles; 8, 5, 2 for the others), for the Airbrush among its discs, for the Eraser among its sizes (the mode stays). At the largest or smallest preset nothing changes. With Pan or Pencil active, during a stroke, before the picture exists, and while a dialog, the quit prompt or the color chooser is open (as REQ-0004-03), the keys do nothing. Ctrl+Numpad + and − keep zooming (REQ-0004-04).
- REQ-0006-15 Choosing a preset, and Numpad + or −, shall change only the tool's preset: no undo step and no modified mark (REQ-0002-02).
- REQ-0006-16 While the pointer is over the canvas area, the cursor shall show the active tool: for the Brush and the Eraser the outline around the footprint's pixels (a staircase for a diagonal), at the current zoom in device pixels, placed as REQ-0006-05 places the footprint on the pixel under the pointer, drawn as a black line with a white line beside it so that it shows on any color; for the Airbrush a spray-can cursor. Over a resize handle the resize cursor wins (REQ-0004-17). The outline is never smaller than 5 × 5 device pixels: when the footprint at the current zoom is smaller, a 5 × 5 square is shown around the pointer's pixel instead.
- REQ-0006-17 The status bar hint (REQ-0004-09) shall be, for the Brush: "Paint with the foreground color. The secondary button uses the background color."; for the Airbrush: "Spray with the foreground color. The secondary button uses the background color."; for the Eraser: "Erase to the background color. The secondary button changes only the foreground color." In Transparent mode the Eraser's hint is "Erase to transparency. The secondary button erases only the foreground color." A change of mode changes the hint at once.

## Scenarios
- AS-1 (REQ-0006-01, REQ-0006-02, REQ-0006-04, REQ-0006-07, REQ-0006-10) Given the app just started (Pan active), Then the toolbox shows Pan, Eraser / Pencil, Brush / Airbrush in two columns, with their tooltips, and no options panel; When the user clicks Brush, Then the panel shows twelve presets in two columns, the 4-pixel circle selected; Airbrush shows three, 16 selected; Eraser shows four sizes (8 selected), a line, and Opaque (selected) and Transparent; When Pencil is chosen, Then the panel and its line are gone; Given the Brush active, When Tool Box is hidden, Then the options panel is hidden too, and shown again with it, with the Brush presets; the preset tooltips are "Circle, 4 pixels" and so on | Verified by: automated (ui) + manual M1
- AS-2 (REQ-0006-03) Given the Brush with the "/" 5-pixel preset and the Eraser with 4 and Transparent, When the user switches to Pencil and back to each, Then the same presets are selected; after a restart the defaults are back | Verified by: automated (ui)
- AS-3 (REQ-0006-05) The footprints, as masks with the pointer pixel at the marked position: circle 7 is `..###..` `.#####.` `#######` `#######` `#######` `.#####.` `..###..` with the pointer at column 3, row 3; circle 4 has the pointer at column 1, row 1; circle 1 is one pixel; square 8 has the pointer at 3, 3; "/" 5 is the pixels (0,4), (1,3), (2,2), (3,1), (4,0) of a 5 × 5 box with the pointer at (2,2) | Verified by: automated (core)
- AS-4 (REQ-0006-06) Given a white picture and the 7-pixel circle, When the user clicks at (20, 20), Then exactly the mask of AS-3 around it is the foreground color; When the user drags from (10, 10) to (40, 10) with the primary button, Then every pixel within the circle around each point of that line is the foreground color and nothing else changed; given the pointer positions (10, 50) and (90, 80) with nothing between, Then a stamp is at every pixel of the line between them; with the secondary button the background color is used; a stroke starting outside the picture paints only inside it | Verified by: automated (core) + automated (ui)
- AS-5 (REQ-0006-06) Given the "/" 8-pixel preset, When the user drags from (10, 20) to (30, 20), Then the painted pixels are exactly those of the stamps at (10..30, 20), a band 8 pixels tall; When the user drags from (20, 40) to (30, 30) along the "/" direction, Then the painted pixels form one 8-connected diagonal from (17, 44) to (34, 27), one pixel wide (every stamp lies on the line x + y = 61); When the user drags from (10, 60) to (20, 70) along "\", Then the painted pixels lie only on the diagonals x + y = 71, 73, …, 91, and the diagonals between them stay white (XP's stripes) | Verified by: automated (core) + manual M2
- AS-6 (REQ-0006-08, REQ-0006-09) Given the Airbrush (16), a white picture, a fixed random sequence and a simulated clock, When the user presses at (50, 50) and holds for 1000 ms without moving, Then exactly 201 sprays of 1 pixel have been made (the press and one per 5 ms), every painted pixel is in the disc, and the painted pixels are exactly the ones the sequence chose; a timer that fires late still gives 201 sprays by 1000 ms; releasing stops the spray; with the middle button nothing happens; held over the toolbox, it sprays nothing; Given two real one-second sprays at the same point, Then they paint different pixels (a test that relies on chance, with odds of failing below one in a million) | Verified by: automated (core, simulated clock and sequence) + automated (ui) + manual M2
- AS-14 (REQ-0006-08) Given the Airbrush (9) and a simulated clock, When the pointer moves 100 pixels to the right in 10 ms, Then there are sprays only around the three positions it had at 0, 5 and 10 ms, and gaps between them | Verified by: automated (core)
- AS-7 (REQ-0006-10, REQ-0006-11) Given a yellow background color and the Eraser (8, Opaque), When the user drags over black pixels, Then they become yellow in an 8-pixel-wide path, and a click changes one 8 × 8 square placed as REQ-0006-05; Given Transparent mode, Then they become #00000000 and show the checkerboard | Verified by: automated (core) + automated (ui)
- AS-8 (REQ-0006-12) Given a picture with red and blue pixels, foreground red and background white, When the user drags the Eraser with the secondary button over both, Then the red pixels under the footprint become white and the blue ones stay; in Transparent mode they become #00000000; a click does the same for one footprint; a pixel #00FF0000 is not changed by a foreground #00000000; Given foreground and background both white, Then nothing changes and no undo step is added | Verified by: automated (core) + automated (ui)
- AS-9 (REQ-0006-13) Given any of the three tools, When the user draws one stroke, Then Undo removes all of it in one step and the title's modified mark follows the history; When the other button is pressed during the stroke, Then the stroke is undone and no step is added; a spray held still for one second is one step | Verified by: automated (ui)
- AS-10 (REQ-0006-14) Given the Brush with the 4-pixel circle, When the user presses Numpad +, Then the 7-pixel circle is selected; pressed again, nothing changes; Numpad − twice gives the 1-pixel circle, and pressed once more, nothing changes; Given the Eraser at 8 Transparent, Numpad + gives 10 Transparent; Given Pencil, Numpad + does nothing; the main keyboard's + does nothing; Ctrl+Numpad + still zooms in; during a stroke and while the color chooser is open the keys do nothing | Verified by: automated (ui)
- AS-11 (REQ-0006-15) Given an unmodified picture, When the user clicks presets and presses Numpad + and −, Then Undo stays insensitive and the title has no modified mark; a preset chosen during a stroke (by keyboard) is ignored | Verified by: automated (ui)
- AS-12 (REQ-0006-16) Given the Eraser (10) at 400 %, Then the cursor over the canvas is a 40 × 40 device-pixel square outline, placed with the pointer's pixel at column 4, row 4 of it, black and white so that it shows on black and on white; Given the Brush "\" 8 at 100 %, Then the outline is the staircase around the 8-pixel diagonal; over a resize handle, the resize cursor shows; Given the Eraser (8) at 10 %, Then the outline is 5 × 5 device pixels | Verified by: manual M3
- AS-13 (REQ-0006-17) Given each tool, Then the status bar shows its hint; Given the Eraser in Transparent mode, Then the hint is the Transparent one; When the mode is changed, Then the hint changes at once | Verified by: automated (ui)

## Edge cases
- A footprint partly outside the picture paints only the part inside; a stroke entirely outside changes nothing and adds no step.
- A spray whose disc reaches past the picture's edge sprays only the pixels inside, at the same density per picture pixel.
- Painting a color over the same color changes no pixel; a stroke made only of such pixels adds no undo step.
- A transparent foreground or background color paints exactly that value (REQ-0003-08: it replaces, it does not blend).
- Numpad + and − with Num Lock off still change the preset (they are the keypad's plus and minus keys).
- Very fast drags and very long strokes (thousands of pointer positions): no gaps, one undo step.
- The window loses focus during a spray: the spray stops and the stroke is kept (REQ-0001-25).
- At high zoom a footprint covers many device pixels: the cursor outline follows the pixel grid, not the pointer's exact position.
- Key repeat on Numpad + steps one preset per repeat, until the largest.
- A 1 × 1 picture with the 24-pixel spray: only its one pixel can be painted.
- A window too short for the toolbox and the options panel: the column of both scrolls; the window's minimum height does not grow.
- With the pointer over a resize handle, a press starts a resize, not a stroke (REQ-0004-16).

## Out of scope
Line, Curve, shapes and their widths (0008); selection tools and their Opaque / Transparent option (0009); Text (0011);
Gradient (new-behaviors.md, its own feature); Magnifier, Fill, Pick Color (0007); brush sizes beyond the presets; keyboard-only
drawing (0015); setting the start-up presets in Preferences (0013); pressure-sensitive tablets.

## Open questions
(none)

## Plan

### Approach
The core gets the footprints and presets (pure data and stepping rules), and the pencil stroke grows into a stroke that
stamps a footprint, replaces only one color, or sprays: all three are still "one color written, each pixel changed at most
once", so the existing change log, cancel and one-undo-step rules carry over unchanged. The spray's randomness and its
5 ms timing live in the core as a seeded generator and a "sprays due by this time" rule, so a core test can drive them
with a fixed seed and a simulated clock. The ui adds three tool buttons, an options widget below the toolbox, the canvas
input for the new tools, footprint cursors, two keypad actions and the hints. No threads, no settings (owner's decision,
REQ-0006-03).

### Files
- **New:** `src/core/footprint.{c,h}` (shapes, masks, presets, stepping), `src/ui/tool-options.{c,h}` (the options panel),
  three symbolic icons in `src/ui/icons/scalable/actions/` (`easel-tool-brush`, `-airbrush`, `-eraser`, drawn for this
  app in the style of the existing two, not copied), `tests/core/test-footprint.c`, `tests/core/test-spray.c`,
  `tests/perf/perf-0006.c`.
- **Changed:** `src/core/stroke.{c,h}` (stamp, replace, spray), `src/ui/tool.{c,h}` (three tools), `src/ui/canvas.{c,h}`
  (input, spray timer, cursors), `src/ui/window.{c,ui}` (toolbox grid, options panel, keypad actions, hints),
  `src/ui/application.c` (keypad accels), `src/ui/easel.gresource.xml`, both `meson.build` files,
  `tests/core/test-stroke.c`, `tests/ui/test-window.c`, `tests/mutants.txt`, `docs/engineering.md` (module list).

### Key interfaces
- core `footprint.h`: `typedef enum { EASEL_SHAPE_CIRCLE, EASEL_SHAPE_SQUARE, EASEL_SHAPE_SLASH, EASEL_SHAPE_BACKSLASH }
  EaselShape;` `typedef struct { EaselShape shape; int size; } EaselFootprint;`
  `bool easel_footprint_has (EaselFootprint f, int col, int row)` (the mask of REQ-0006-05);
  `int easel_footprint_anchor (int size)` (⌊(size − 1) / 2⌋); the preset tables `easel_brush_presets[12]`,
  `easel_airbrush_presets[3]`, `easel_eraser_presets[4]` with their defaults; `int easel_preset_step (const EaselFootprint
  *table, int n, int index, int direction)` (Numpad: the next larger or smaller preset of the same shape, the ends stay,
  REQ-0006-14).
- core `stroke.h`: `easel_stroke_begin` keeps the Pencil; new `EaselStroke *easel_stroke_begin_stamp (EaselImage *img,
  int x, int y, uint32_t color, EaselFootprint f, const uint32_t *only)` (`only` non-NULL: change only pixels of that
  value, the color eraser; NULL: all); `easel_stroke_to` stamps along the 8-connected line;
  `EaselStroke *easel_stroke_begin_spray (EaselImage *img, uint32_t color, int diameter, int per_spray, uint64_t seed)`,
  `void easel_stroke_spray (EaselStroke *s, int x, int y, int sprays)`;
  `int easel_spray_due (int64_t held_us)` (1 + full 5 ms periods, REQ-0006-08). `end` and `cancel` are unchanged.
- ui: `easel_canvas_set_footprint (EaselCanvas *, EaselFootprint)` and `easel_canvas_set_eraser_mode (..., gboolean
  transparent)`; `EaselToolOptions` widget with `easel_tool_options_show (self, EaselTool)` and a "preset-changed"
  signal; window actions `win.preset-larger` (KP_Add) and `win.preset-smaller` (KP_Subtract), gated like REQ-0004-03.

### How each rule is kept
- **Footprint placement:** the mask is a function of (column, row) in the size × size box, the box's top-left at the
  pointer pixel minus the anchor; masks are computed, not stored, so the 7-pixel circle of AS-3 is a test of the rule.
- **Stamping:** each pixel of the Bresenham line gets a full stamp; plotting skips a pixel that already has the color, so a
  pixel is logged once however many stamps cover it (the log stays 12 bytes per changed pixel, as 0002 says).
- **Color eraser:** a pixel is changed only if all four channels equal the foreground at the press; foreground equal to
  background changes nothing (no log entry, no step).
- **Spray:** a 64-bit xorshift generator seeded from the monotonic clock at the press (a fixed seed in tests); the disc's
  pixel offsets are listed once per press (at most 452 for the 24 disc); each pick is an index chosen with equal chance,
  repeats allowed. The canvas runs a 5 ms GLib timeout while the button is held; at each call it asks the core how many
  sprays are due for the time held (the monotonic clock) and makes the missing ones at the pointer's position, so a late
  timer catches up and never runs ahead.
- **Colors at the press, presets during a stroke:** the stroke keeps its color; the options widget and the keypad
  actions are insensitive while the canvas reports a stroke.
- **Cursor:** a texture made from the footprint mask at the current zoom (device pixels, at least 5 × 5), black outline
  with a white one outside it, hotspot at the anchor pixel's center; rebuilt when the zoom, the preset, the mode or the
  tool changes; the resize-handle cursor still wins (0004).

### What grows
Nothing new grows with use. A stroke's log grows by 12 bytes per pixel it changes, as for the Pencil; the spray adds the
list of disc offsets (8 bytes each, at most 452) for the length of one press. A long spray stops growing once the disc's
pixels have the color, since a pixel is logged once.

### Measurements (budget: engineering.md "Pointer event to pixels on screen (pencil, brush)")
A task adds `perf-0006`: a Brush motion with the 7-pixel circle and the 8 "/" preset, and an Eraser motion with the 10
square, from the event to the next painted frame at 1920 x 1080 and 8000 x 8000, and the cost of one second of spray
(24 disc) at both sizes; the numbers go here. Budget: one frame (16 ms) at 1920 x 1080. No speed work is planned before
the numbers; the tiles of 0004 already rebuild only what a stamp touches.

### Scenario-to-test map
| AS | Test |
|---|---|
| 1 | ui `as1_toolbox_and_options` (buttons, order, tooltips, presets, defaults, Tool Box hides the panel) + manual M1 |
| 2 | ui `as2_presets_kept` |
| 3 | core `as3_footprint_masks` |
| 4 | core `as4_brush_stroke`, ui `as4_brush_on_canvas` |
| 5 | core `as5_slanted_brush` (exact pixels, the stripes) + manual M2 |
| 6 | core `as6_spray_held` (fixed seed, simulated clock: exactly 201 sprays, all in the disc, catch-up), ui `as6_airbrush_sprays` (real timer: in one real second never more than 201 sprays and at least 100; two sprays differ) + manual M2 |
| 7 | core `as7_eraser`, ui `as7_eraser_on_canvas` |
| 8 | core `as8_color_eraser` (incl. transparent mode, #00FF0000, equal colors), ui `as8_color_eraser_on_canvas` |
| 9 | ui `as9_one_step_and_cancel` (each tool; a held spray) |
| 10 | core `as10_preset_steps`, ui `as10_numpad_steps` (ends, Pencil, main keyboard, Ctrl+Numpad zooms, during a stroke, chooser open) |
| 11 | ui `as11_presets_are_not_edits` |
| 12 | core `as12_cursor_geometry` (outline size and hotspot for a size, zoom and scale, the 5 × 5 minimum) + manual M3 |
| 13 | ui `as13_hints` |
| 14 | core `as14_spray_gaps` |

### Results before the stroke refactor (T001, 2026-10-16)
Six mutants of the stroke rules of 0001 and 0002 were added to `tests/mutants.txt` and run against the unchanged stroke code
with `tools/mutate.sh REQ-0002-01 REQ-0002-02 REQ-0001-23`: **6 of 6 killed, each by an assertion, in 5 s**:
no undo step for a stroke (core `stroke_ends_into_history`), a wrong old value in the step (core `as21_cancel_restores`), a
pixel with the color already changed again (core `as18_line_is_8_connected`), a cancel that restores nothing (core
`as21_cancel_restores`), the other button that does not cancel (ui `as21_pencil_on_canvas`) and a stroke not handed to the
history (ui `as6_initial_sensitivity`). "One stroke is one undo step" has no separate mutant: a stroke that made two steps
would need a second call to the history, and `stroke_ends_into_history` fails on any byte count other than one step's.
After the refactor (T003, 2026-10-16) the same six were run again, with the text of one adapted to the new line of code,
and with three new ones for the stamping and the color eraser: **9 of 9 killed, each by an assertion, in 7 s**; the Pencil
tests of 0001 and 0002 passed unchanged.

### Constitution check
Core stays libc only (footprints, stroke, spray, seeded generator); ui only turns input into core calls; no threads (P5);
the presets are constants by the owner's recorded decision (P6, REQ-0006-03); tests ship with each task.

### Risks
- **GLib timeouts at 5 ms are not exact** (timer slack, busy main loop): the catch-up rule makes the count follow the time
  held. The exact count (201 in 1000 ms) is checked only in the core with a simulated clock; the ui test with the real
  timer checks only "never more than 201 in a real second, and at least 100", since a busy machine may run late.
- **Cursor size limits** (some Wayland compositors cap cursor images, often at 64 or 256 pixels): the 10 eraser at 800 %
  is an 80 × 80 cursor. Probe before the cursor task relies on it: make 64, 80 and 160 pixel cursors on this session and look (M3);
  fallback: cap the outline at the largest size that works and say so in the spec.
- **Keypad accelerators with Num Lock off:** GTK may report KP_Add / KP_Subtract either way; `as10_numpad_steps` sends
  both, and M1 checks a real keyboard with Num Lock on and off.
- **Icons:** drawn for this app; M1 checks they read well in light and dark themes.

## Tasks
Groups, each ending with a look-check by the owner:
A. Core, nothing on screen (T001 to T004): check A is the mutation results before and after the stroke refactor.
B. Toolbox and options panel (T005, T006): M1.
C. Drawing on the canvas (T007, T008): M2.
D. Keys, cursors, hints and the benchmark (T009 to T012): M3 and the numbers in the Plan.

- [x] T001 Mutants for the stroke rules of 0001 and 0002, before the refactor (REQ-0006-13, REQ-0001-23, REQ-0002-01, REQ-0002-02) - done when `tests/mutants.txt` has mutants for "one stroke is one undo step", "the other button cancels the stroke", "the log keeps only pixels the stroke changed" and "a pixel that already has the color is not changed or logged", `tools/mutate.sh` for those requirements kills each of them by an assertion (not a crash) against the unchanged stroke code, and the results are written in the Plan
- [x] T002 Footprints, presets and their steps in the core (REQ-0006-04, REQ-0006-05, REQ-0006-07, REQ-0006-10, REQ-0006-14; AS-3, AS-10) - done when core `as3_footprint_masks` (the masks of AS-3, the anchor, circle 1 is one pixel) and `as10_preset_steps` (same shape for the Brush, the ends stay) pass
- [x] T003 Stamping and color-erasing strokes in the core (REQ-0006-06, REQ-0006-11, REQ-0006-12, REQ-0006-13; AS-4, AS-5, AS-7, AS-8) - done when core `as4_brush_stroke` (a click, a drag, two far positions with nothing between), `as5_slanted_brush` (the exact pixels and the stripes), `as7_eraser` and `as8_color_eraser` (Transparent mode, #00FF0000, equal colors) pass, the Pencil tests of 0001 and 0002 pass unchanged, and the T001 mutants are killed again by an assertion after the refactor
- [ ] T004 The spray in the core (REQ-0006-08, REQ-0006-09; AS-6, AS-14) - done when core `as6_spray_held` (fixed seed, simulated clock: exactly 201 sprays of 1 pixel by 1000 ms, all in the disc, a late tick caught up, 3 pixels per spray for the 24 disc) and `as14_spray_gaps` pass
- [ ] T005 The toolbox with five tools and their icons (REQ-0006-01; AS-1) - done when the toolbox is a two-column grid Pan, Eraser / Pencil, Brush / Airbrush with tooltips and icons, the toolbox part of ui `as1_toolbox_and_options` passes, and the 0001 tool tests still pass
- [ ] T006 The options panel and the presets of each tool (REQ-0006-02, REQ-0006-03, REQ-0006-10; AS-1, AS-2) - done when ui `as1_toolbox_and_options` (presets, order, defaults, tooltips, the line, hidden for Pan and Pencil, Tool Box hides it) and `as2_presets_kept` pass, and manual M1 (the look in light and dark themes, the icons, the footprints in the buttons) passes
- [ ] T007 Brush and Eraser on the canvas (REQ-0006-06, REQ-0006-11, REQ-0006-12, REQ-0006-13, REQ-0006-15; AS-4, AS-7, AS-8, AS-9, AS-11) - done when ui `as4_brush_on_canvas`, `as7_eraser_on_canvas`, `as8_color_eraser_on_canvas`, the Brush and Eraser parts of `as9_one_step_and_cancel` and `as11_presets_are_not_edits` (incl. a preset chosen during a stroke is ignored) pass
- [ ] T008 The Airbrush on the canvas with its timer (REQ-0006-08, REQ-0006-09, REQ-0006-13; AS-6, AS-9) - done when ui `as6_airbrush_sprays` (a real second: never more than 201 sprays and at least 100; two sprays differ; the middle button does nothing; released, it stops) and the spray part of `as9_one_step_and_cancel` pass, and manual M2 (Brush shapes and stripes, Eraser and color eraser, the airbrush's feel) passes
- [ ] T009 Numpad + and − (REQ-0006-14, REQ-0006-15; AS-10, AS-11) - done when ui `as10_numpad_steps` (the ends, the Eraser keeps its mode, Pencil and Pan do nothing, the main keyboard's + does nothing, Ctrl+Numpad zooms, nothing during a stroke or with the color chooser open, both keypad keysyms) and the keypad part of `as11_presets_are_not_edits` pass
- [ ] T010 Footprint cursors, after a probe of the cursor size limit (REQ-0006-16; AS-12) - done when the probe (64, 80 and 160 pixel cursors on this session) is recorded in the Plan's risks, core `as12_cursor_geometry` (size, hotspot, the 5 x 5 minimum, scale 1 and 2) passes, and manual M3 (eraser at 400 % and 10 %, slanted brush, over black and white, resize handles win, Numpad + and − with Num Lock on and off) passes
- [ ] T011 The status bar hints (REQ-0006-17; AS-13) - done when ui `as13_hints` (each tool, Transparent mode, a mode change updates the hint at once) passes
- [ ] T012 Measure the strokes against the budget (budget of engineering.md "Pointer event to pixels on screen (pencil, brush)"; REQ-0006-06, REQ-0006-08) - done when `perf-0006` records Brush (circle 7, "/" 8) and Eraser (10) motions to the next painted frame and one second of spray (24) at 1920 x 1080 and 8000 x 8000, and the numbers are in the Plan
