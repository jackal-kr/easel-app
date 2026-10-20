# 0010 Lines and Shapes

Status: implemented
Depends on: 0001, 0002, 0003, 0004, 0006, 0007, 0008, 0009

## Why
After the freehand tools, XP Paint's shape tools: a straight line and the three closed shapes, each drawn by one drag,
with a choice of line width and, for the shapes, of outline and fill. They complete the toolbox's second half as far as
it needs no pending state (Curve and Polygon are their own feature, "curve and polygon", and the Gradient is "gradient").
Decisions by JOD on 2026-10-19: the split in three; XP's five widths, shared by every tool that draws a line; a square
pen; XP's fill-style colors; Shift takes the smaller side; a corner radius of 8, less for small boxes; the shapes' options
show the fill styles and the widths.

## Requirements
### The tools and their options
- REQ-0010-01 The toolbox shall show Line, Rectangle, Rounded Rectangle and Ellipse, with the tooltips "Line", "Rectangle", "Rounded Rectangle" and "Ellipse", in their places of the final order of REQ-0006-01, so that in this feature the toolbox is, in two columns, row by row: Pan, Magnifier / Eraser, Pick Color / Fill, Pencil / Brush, Airbrush / Line, Rectangle / Rounded Rectangle, Ellipse. Clicking one makes it the active tool, as for the other tools (REQ-0001-09). (0011 adds Curve and Polygon, which moves Rectangle, Rounded Rectangle and Ellipse down: REQ-0011-01.) (0012 adds Gradient after Fill, which moves the tools after it one place on: REQ-0012-01.)
- REQ-0010-02 While Line is active, the options panel (REQ-0006-02) shall show five toggle buttons for the line widths 1, 2, 3, 4 and 5 pixels, one under the other (this amends the two columns of REQ-0006-02 for these tools), each drawn as a horizontal line w logical pixels thick, with the tooltips "1 pixel", "2 pixels", "3 pixels", "4 pixels" and "5 pixels", one selected at a time. While Rectangle, Rounded Rectangle or Ellipse is active, the panel shall show three toggle buttons for the fill styles, one under the other, "Outline", "Outline and fill" and "Fill" (each drawn as a small rectangle in that style, the tooltips as the names), then a line, then the same five widths. The panel keeps the width of the toolbox (REQ-0008-07).
- REQ-0010-03 The selected width shall be one choice shared by Line and the three shapes (and by the curve and polygon tools of a later feature): choosing a width with one of them is the width of all. The fill style shall be one choice shared by the three shapes. At start-up the width is 1 pixel and the fill style "Outline", each later time the app starts too, and the choices are kept for the rest of the session when the user switches tools (as REQ-0006-03: not settings in this feature). Choosing a width or a fill style changes no pixel.
- REQ-0010-04 Numpad + and Numpad − (REQ-0006-14) shall select the next larger or smaller width while one of the four tools is active, stopping at 5 and at 1; they do not change the fill style. During a drag, before the picture exists and while a dialog, the quit prompt or the color chooser is open, they do nothing (as REQ-0006-14). Ctrl+Numpad + and − keep zooming.

### Drawing
- REQ-0010-05 When the user presses the primary or the secondary button in the canvas area with one of the four tools and drags, the app shall show the shape from the pixel under the press (the start) to the pixel under the pointer (the end) as it would be drawn, following the pointer, without changing the picture; when the user releases the button, the app shall draw that shape into the picture. A press may start outside the picture, and only the pixels of the shape inside the picture are drawn. A press and release without a move draw the shape whose start and end are the same pixel. The middle button does nothing. A primary press on a resize handle starts a resize, and a secondary press there does nothing (REQ-0004-16).
- REQ-0010-06 The drawing button's color shall be the foreground color for the primary button and the background color for the secondary one, and the other color is the other one. The colors are those current at the press, until the release, even if they are changed during the drag (as REQ-0003-08). Drawn pixels get exactly the color, with no blending, also when it has transparency (REQ-0003-08), and no pixel is antialiased (P4).
- REQ-0010-07 A shape shall be one undo step (REQ-0002-01) and mark the picture modified; a shape that changes no pixel adds no step and no mark. Its undo step costs at most what REQ-0002-16 allows for the pixels it changed.
- REQ-0010-08 If, while the button is held, the user presses the other mouse button, presses Esc, or chooses Undo, the app shall cancel the shape: nothing is drawn and no step is added; the release afterwards does nothing more (as REQ-0001-23 for strokes), and after an Undo further Undo and Redo are ignored until the button is released (REQ-0002-09). While the button is held, choosing another tool, a width or a fill style is ignored (REQ-0006-03), and the View and Image actions are ignored (REQ-0004-03).
- REQ-0010-18 If Save or Quit is requested while the button is held, the app shall first draw the shape as last shown, then proceed (as REQ-0001-33).
- REQ-0010-09 If the release never reaches the window (REQ-0007-18: the next pointer move or press with no button down), the app shall draw the shape as it was last shown, as if the button had been released there.

- REQ-0010-19 If the window loses focus while the button is held, the app shall draw the shape as last shown, as a stroke is kept (REQ-0001-25) and as a lost release does (REQ-0010-09).
- REQ-0010-20 If the memory for the shape or its undo step cannot be had, then at the release the app shall show an error dialog with the heading "Could Not Draw the Shape" and the body "There is not enough memory to draw this shape.", and change nothing: neither the picture nor the history (as REQ-0007-07 for Fill); an undo step larger than the whole budget is kept as the only step (REQ-0002-17).
- REQ-0010-21 While the button is held, the end shall follow the pointer also when it leaves the canvas area or the window, and the view does not scroll by itself. During a drag Ctrl+wheel and the pinch do not zoom, and the view scrolls as usual (REQ-0001-15, REQ-0001-17, REQ-0004-03); after a scroll in the middle of a drag, the shape's end moves at the next pointer motion, to the pixel under the pointer then.

### The shapes
- REQ-0010-10 Line shall draw the 8-connected, 1-pixel line from the start to the end that the Pencil draws between two pointer positions (REQ-0001-20); with a width w above 1, every pixel of that line is stamped with the w × w square of REQ-0006-05 (placed as REQ-0006-05 places it, the line's pixel at column and row ⌊(w − 1) / 2⌋ of the square).
- REQ-0010-11 Rectangle, Rounded Rectangle and Ellipse shall be drawn in the box that runs from the outer corner of the start pixel to the outer corner of the end pixel, so both pixels are included (the box of a start (10, 10) and an end (19, 14) is 10 × 5 pixels); the ellipse touches the box's outer edges, not the centers of its edge pixels. A pixel belongs to the shape when its center lies inside the shape or on its edge: the box itself for Rectangle; for Ellipse the ellipse that touches the four sides of the box, its center at the box's center; for Rounded Rectangle the box with each corner replaced by a quarter circle of radius r = 8, or half the box's smaller side (rounded down) when that is less than 8.
- REQ-0010-12 With a width w, the outline of a shape shall be its pixels that do not belong to the same shape drawn in the box made smaller by w on each of its four sides (for Rounded Rectangle with the radius made smaller by w, not below 0); the inside is the rest of its pixels. A box of 2w pixels or less in a direction has no inside: the whole shape is outline.
- REQ-0010-13 The fill styles shall draw (XP's rule): "Outline": the outline in the drawing button's color and nothing inside, the picture's pixels there kept; "Outline and fill": the outline in the drawing button's color and the inside in the other color; "Fill": the whole shape, outline and inside, in the drawing button's color.
- REQ-0010-14 While Shift is held, the app shall constrain the shape, and the preview follows Shift being pressed or released at once: Line goes horizontally, vertically or at 45 degrees, whichever of the eight directions is nearest by angle to the line from the start to the pointer, the end being the start moved along that direction by the pointer's distance along it (for 45 degrees, the smaller of the horizontal and vertical distances); Rectangle and Rounded Rectangle draw a square and Ellipse a circle, of side the smaller of the box's width and height, from the start toward the end.

### Feedback
- REQ-0010-15 While the pointer is over the canvas area with one of the four tools active, the cursor shall be a crosshair whose center is the pixel the shape would start from; over a resize handle the resize cursor wins (REQ-0004-17).
- REQ-0010-16 The status bar hint (REQ-0004-09) shall be, for Line: "Draws a straight line with the selected line width."; for Rectangle: "Draws a rectangle with the selected fill style."; for Rounded Rectangle: "Draws a rounded rectangle with the selected fill style."; for Ellipse: "Draws an ellipse with the selected fill style."
- REQ-0010-17 A press with one of the four tools is ignored, as a stroke is, before the picture exists and while a dialog, the quit prompt or the color chooser is open or a resize drag is in progress (REQ-0001-39, REQ-0004-03, REQ-0007-15).

## Scenarios
- AS-1 (REQ-0010-01, REQ-0010-16, REQ-0010-15) Given the app just started, Then the toolbox shows Pan, Magnifier / Eraser, Pick Color / Fill, Pencil / Brush, Airbrush / Line, Rectangle / Rounded Rectangle, Ellipse with their tooltips; When the user clicks each of the four, Then it is the active tool, the status bar shows its hint and the cursor is a crosshair | Verified by: automated (ui, toolbox and hints) + manual M1 (the cursor, the buttons' look)
- AS-2 (REQ-0010-02, REQ-0010-03) Given Line active, Then the panel shows the five widths with "1 pixel" selected and no fill styles; When the user chooses 3 pixels and then Rectangle, Then the panel shows "Outline" selected, a line, and the widths with 3 pixels selected; When the user chooses "Fill" and Ellipse, Then "Fill" is selected there too; When the user switches to Pencil and back to Line, Then 3 pixels is still selected; the toolbox keeps one width for every tool (REQ-0008-07) | Verified by: automated (ui) + manual M1 (the options' look)
- AS-3 (REQ-0010-04) Given Line with 1 pixel, When the user presses Numpad + five times, Then the width is 5; Numpad − six times gives 1; with Rectangle the same keys change the width and not the fill style; Ctrl+Numpad + zooms | Verified by: automated (ui)
- AS-4 (REQ-0010-05, REQ-0010-07, REQ-0010-10) Given a white 300 × 200 picture, Line with 1 pixel and black as the foreground, When the user drags from (10, 10) to (40, 20), Then while dragging the picture is unchanged and the line is shown; after the release the black pixels are the Pencil's line from (10, 10) to (40, 20), one Undo restores white, and the title has the modified mark; the step's cost is within REQ-0002-16 for the 31 pixels changed | Verified by: automated (core, the line and the step's cost) + automated (ui, preview and step)
- AS-5 (REQ-0010-10) Given Line with 3 pixels, When the user draws from (10, 10) to (10, 10) (a click), Then the black pixels are the 3 × 3 square from (9, 9) to (11, 11); with 4 pixels the 4 × 4 square from (9, 9) to (12, 12); a line from (10, 10) to (20, 10) with 2 pixels is the rows 10 and 11 from x 10 to 21 | Verified by: automated (core)
- AS-6 (REQ-0010-11, REQ-0010-12, REQ-0010-13) Given Rectangle with 1 pixel and "Outline", foreground black, background red, When the user drags from (10, 10) to (19, 14), Then 26 pixels are black (the border of the 10 × 5 box) and the 24 inside keep their color; with 2 pixels 44 are black and 6 keep theirs; with "Outline and fill" and 1 pixel the 26 are black and the 24 inside red; with "Fill" all 50 are black; with the secondary button the colors swap | Verified by: automated (core) + automated (ui)
- AS-7 (REQ-0010-11, REQ-0010-12) Given Ellipse with 1 pixel and "Outline and fill", When the user drags from (0, 0) to (8, 8) (a 9 × 9 box), Then 69 pixels change: the 32 of the outline (rows `..#####..` `.##...##.` `##.....##` `#.......#` three times, then the same upward) black and the 37 inside red; the box's 12 corner pixels outside the circle keep their color; a 20 × 10 box gives 160 pixels with an outline of 48 | Verified by: automated (core)
- AS-8 (REQ-0010-11, REQ-0010-12) Given Rounded Rectangle with 1 pixel and "Fill", When the user drags over a 30 × 20 box, Then 552 pixels change (the radius is 8, the four corners leave 48 pixels); "Outline" gives an outline of 88 pixels; a 10 × 4 box has the radius 2 and 36 pixels (rows `.########.` `##########` `##########` `.########.`); a 1 × 1 box is one pixel | Verified by: automated (core)
- AS-9 (REQ-0010-14) Given Line, When the user drags from (10, 10) to (40, 22) with Shift, Then the line is horizontal from (10, 10) to (40, 10); to (40, 35) with Shift, Then it goes at 45 degrees to (35, 35); to (12, 40), Then vertically to (10, 40); Given Rectangle with Shift from (10, 10) to (49, 34) (a 40 × 25 box), Then the square is 25 × 25 from (10, 10) to (34, 34); toward the top-left, from (50, 50) to (30, 20), Then the square is 21 × 21 from (30, 30) to (50, 50); When Shift is released before the button, Then the preview and the drawn shape are the 21 × 31 rectangle again | Verified by: automated (core, the constraint) + automated (ui, Shift during the drag)
- AS-10 (REQ-0010-05, REQ-0010-06) Given a 300 × 200 picture, When the user drags with Line from (−20, 50) to (50, 50), Then only the line's pixels inside the picture change; with the foreground #FF000080, Then they are exactly #FF000080; with the middle button nothing happens | Verified by: automated (core, clipping and color) + automated (ui)
- AS-11 (REQ-0010-08, REQ-0010-18) Given a Rectangle drag in progress, When the user presses the other button, Then the picture is as before and no step is added, and the release does nothing; the same with Esc and with Ctrl+Z, and a second Ctrl+Z before the release does nothing; When the user clicks the Pencil tool, a width or a fill style, or chooses Zoom In, during the drag, Then nothing changes; Given a Rectangle drag and Save requested before the release, Then the rectangle is drawn as shown and then saved | Verified by: automated (ui)
- AS-12 (REQ-0010-09) Given an Ellipse drag whose release never arrives, When the window sees the pointer move with no button down, Then the ellipse is drawn as last shown and is one undo step, and the options panel is usable again | Verified by: automated (ui)
- AS-13 (REQ-0010-07) Given a white picture and a white foreground, When the user draws a Rectangle with "Outline", Then no pixel changes, Undo stays insensitive and the title has no modified mark | Verified by: automated (ui)
- AS-14 (REQ-0010-17) Given the color chooser open, the Grid dialog open, the quit prompt open, a resize drag in progress, or a window whose picture does not exist yet, When the user presses with each of the four tools, Then nothing changes | Verified by: automated (ui)

- AS-15 (REQ-0010-19) Given a Line drag from (10, 10) shown to (60, 30), When the window loses focus, Then that line is drawn and is one undo step, and a later release does nothing more | Verified by: automated (ui)
- AS-16 (REQ-0010-20) Given a filled Ellipse over a whole picture and memory made too small for its step (as the fill test of REQ-0007-07 does), When the user releases, Then the dialog "Could Not Draw the Shape" with its body appears and the picture, the title and Undo are unchanged; Given a 16 MB undo budget and a shape whose undo step is larger than the budget (the Plan names a size that makes it so), Then it is the only step and one Undo restores the picture | Verified by: automated (core, refusal and budget) + automated (ui, dialog)
- AS-17 (REQ-0010-21) Given a Rectangle drag from (10, 10) at 100 % in a 3000 × 2000 picture, When the pointer moves 200 pixels past the canvas area's right edge, Then the preview's end is the picture pixel under the pointer there, and the view's scroll offsets are unchanged; Ctrl+wheel during the drag does not zoom and is left to the view's scrolling, and the drag goes on; after the view scrolls 100 points down, the next pointer motion at the same place in the view puts the end 100 points further down the picture | Verified by: automated (ui, through the wheel handler)
## Edge cases
- A drag that ends where it started, a box of 1 pixel in one direction (a 1 × 20 rectangle or ellipse is a line), and width 5 on a box of 3 × 3 (all outline).
- A shape partly or wholly outside the picture: only its pixels inside are drawn; a shape wholly outside changes nothing and adds no step.
- The colors swapped (Ctrl+Shift+X) during a drag: the shape keeps the colors of the press (REQ-0010-06).
- A shape at the largest picture, 10000 × 10000 (REQ-0005-17), filled (about 78 million pixels): one step, or the dialog of REQ-0010-20.
- The drawing color equal to the picture's pixels: no step (REQ-0010-07).
- A Shift that is held before the press and released during the drag, or pressed only during it.
- Zoom levels: the shape follows picture pixels at any zoom; the preview is drawn as picture pixels, hard-edged.

## Out of scope
- Curve and Polygon ("curve and polygon"), the Gradient ("gradient"), and Text.
- Antialiased shapes, other pens, dashed lines, and arrowheads.
- The shape's size in the status bar while dragging (the selection feature adds a size field).
- Hover previews of a shape before the press.
- A setting for the start-up width or fill style (Preferences).
- Keyboard-only drawing (Accessibility).

## Open questions
None. Settled by JOD on 2026-10-19: the split; widths 1 to 5 shared; a square pen; XP's fill colors; the shapes' panel with fill styles and widths; Shift takes the smaller side; a corner radius of 8, less for small boxes; a focus loss draws the shape as shown; no memory gives the "Could Not Draw the Shape" dialog and changes nothing; a drag follows the pointer outside the area without scrolling.

## Plan
### Approach
The geometry is a pure core module: a shape is a small description (kind, start, end, width, fill style), and one function
gives, for any picture row, the runs of that row the shape covers and whether each run is outline or inside. Everything
else is built on that one function, so what is previewed is what is drawn: applying a shape walks the rows of its box
(clipped to the picture) and writes the runs; the preview draws the same runs as colored rectangles in the canvas's
snapshot, only for the picture rows the view shows (at a zoom below 100 % one sampled row per device row), so a preview
frame costs the view's size, not the shape's. Rectangle, Ellipse and Rounded Rectangle give their runs from closed formulas
(REQ-0010-11, -12: the pixel centers, the box from the outer corners, the shrunken box for the inside). Line builds, once
per pointer move, the Pencil's 8-connected line (the same Bresenham as REQ-0001-20) and from it the covered x range of each
row under the square pen (REQ-0010-10). Shift's constraint is a core function too (REQ-0010-14). In the window, a shape is a
pending gesture of the canvas, as a stroke is, so the existing rules for strokes (the other button, Esc, Undo, focus loss,
lost release, Save and Quit, tool changes and View actions ignored) apply without a copy; the release applies the shape as
one history step. The width and the fill style are two stateful window actions (`win.line-width`, `win.fill-style`), the
options' buttons are bound to them (as the Magnifier's levels are), so the shared width is one state by construction.

### The undo step
(Corrected at T005: no new step kind.) The existing step of changed pixels is used: the index, the old value and the new
value of each changed pixel, 12 bytes a pixel plus 64, within REQ-0010-07 and REQ-0002-16 (16 bytes a changed pixel);
undo and redo are the history's own. A shape that changes no pixel adds no step. A
whole 10000 × 10000 filled ellipse changes up to about 78.5 million pixels, a step of about 942 MB, larger than the default
budget of 512 MB, so it is kept as the only step (REQ-0002-17); if that memory cannot be had, the shape is not drawn and
the dialog of REQ-0010-20 is shown, from an idle callback as Fill's is.

### Files
- `src/core/figure.{c,h}` (new; `shape` would clash with the footprint's `EaselShape`): the description, the row runs, the constraint, the apply.
  The undo step is the history's existing step of changed pixels (`easel_history_push`); `history.{c,h}` is unchanged. Tests
  `tests/core/test-figure.c` and `tests/core/test-figure-apply.c` (new).
- `src/ui/tool.{c,h}` (four tools), `src/ui/window.ui` (toolbox), icons `easel-tool-{line,rectangle,rounded-rectangle,ellipse}-symbolic.svg`,
  `src/ui/tool-options.{c,h}` (a widths page and a shapes page in the options stack, buttons on the two actions),
  `src/ui/canvas.{c,h}` (the shape gesture, Shift, the preview runs in the snapshot, the crosshair cursor),
  `src/ui/window.c` (the two actions, the hints, the Numpad step, the memory dialog). Tests: `tests/ui/test-window.c` (a 0010
  block), `tests/perf/perf-0010.c`, `tests/mutants.txt`.

### Key interfaces
- `EaselFigureKind` (`EASEL_FIGURE_LINE`, `_RECTANGLE`, `_ROUNDED`, `_ELLIPSE`), `EaselFigureStyle` (`EASEL_FIGURE_OUTLINE`, `_OUTLINE_AND_FILL`, `_FILL_ONLY`),
  `EaselFigure { kind, x0, y0, x1, y1, width, style }` (the constrained end already applied).
- `size_t easel_figure_row (const EaselFigure *f, int y, EaselFigureRun *runs, size_t max)`: the runs `{x0, x1, inside}` of row
  y, left to right; `void easel_figure_bounds (const EaselFigure *f, int *x, int *y, int *w, int *h)`;
  `void easel_figure_constrain (EaselFigureKind k, int x0, int y0, int *x1, int *y1)`.
  `EaselFigureResult easel_figure_apply (EaselImage *img, EaselHistory *h, const EaselFigure *f, uint32_t draw, uint32_t other, size_t *changed)`:
  DONE, NOTHING (no pixel changed: no step) or NO_MEMORY (nothing changed). The Line needs its rows once per move:
  `EaselFigure` keeps a pointer to a per-row range table, made by `easel_figure_prepare` and freed by `easel_figure_release`.
- Canvas: `easel_canvas_set_shape_options (width, style)`, `easel_canvas_set_shift (gboolean)`; the existing
  `stroke-finished` and `stroking-changed` signals; a `shape-failed` signal for the dialog.

### How each rule is kept
- **REQ-0010-05, -06, -07:** the gesture keeps the press point and the colors of the press; motion recomputes the end
  (constrained if Shift) and queues a draw; the release calls `easel_figure_apply`.
- **REQ-0010-08, -09, -18, -19:** the gesture is a "stroke" for the canvas's existing cancel and end paths.
- **REQ-0010-10 to -14:** the core functions, tested against brute force (every pixel of the box classified directly).
- **REQ-0010-15 to -17, -21:** the named "crosshair" cursor; hints; the existing input blocks; motion is taken from the
  scrolled area's controller, which keeps reporting outside it during a grab, and nothing in the gesture scrolls.

### What grows
Per motion: for Line, a range table of two ints per row of the line's box (at most 2 × 10000 ints). Per step: 12 bytes per
changed pixel plus 64 (the log is shrunk to its length before the push, so that is also what it holds). The preview stores nothing between frames.

### Measurements
`docs/engineering.md` budgets a pointer event to pixels on screen within a frame (pencil, brush). A task runs `perf-0010`:
a filled Ellipse and a 5-pixel Line being dragged, to the painted frame, at 1920 × 1080 and 8000 × 8000, at 100 % and at the
fit zoom; and the release of a whole-picture filled Ellipse at 8000 × 8000 (time and peak memory). The numbers go here.

Measured at T011 (`tests/perf/perf-0010.c`, `meson test --benchmark`; 30 pointer moves each, to the next painted frame; the
display is vsync-bound, so a frame costs about 16.6 ms and that is the floor):

| Drag | 1920 × 1080, 100 % | 1920 × 1080, fit (69 %) | 8000 × 8000, 100 % | 8000 × 8000, fit (10 %) |
|---|---|---|---|---|
| filled Ellipse, width 1 | median 16.7, worst 20.7 ms | 16.6, 19.5 | 16.6, 18.7 | 16.8, **85.0** |
| Line, width 5 | 16.6, 20.2 | 16.6, 19.4 | 16.6, 19.3 | 16.4, 19.4 |

- Every median is one frame. One worst case is over a frame: the filled Ellipse at 8000 × 8000 at the fit zoom, 85 ms once
  in 30 moves (not found which; the other 29 are within a frame). Reported to JOD, not fixed (no optimization without his
  word, rule P5).
- Release of a filled Ellipse over the whole 8000 × 8000 picture (64 million pixels, about 50 million inside): 204 ms for
  the call to return, 301 ms to the next painted frame; peak memory 952 MB (369 MB before it: the picture is 256 MB and the
  undo step about 580 MB).

### Scenario-to-test map
| AS | Test |
|---|---|
| 1 | ui `as1_shape_tools_toolbox_hints` + manual M1 (cursor, buttons) |
| 2 | ui `as2_width_and_fill_style_shared` + manual M1 (the options' look) |
| 3 | ui `as3_numpad_steps_width` |
| 4 | core `as4_line_pixels` (with the step's cost), ui `as4_line_preview_and_step` |
| 5 | core `as5_square_pen` |
| 6 | core `as6_rectangle_fill_styles`, ui `as6_rectangle_on_canvas` |
| 7 | core `as7_ellipse_pixels` |
| 8 | core `as8_rounded_rectangle_pixels` |
| 9 | core `as9_shift_constraint`, ui `as9_shift_during_drag` |
| 10 | core `as10_clipped_and_exact_color`, ui `as10_line_from_outside` |
| 11 | ui `as11_cancel_and_ignored_during_drag` |
| 12 | ui `as12_lost_release_draws_shape` |
| 13 | ui `as13_shape_changing_nothing` |
| 14 | ui `as14_shapes_ignored_when_blocked` |
| 15 | ui `as15_focus_loss_draws_shape` |
| 16 | core `as16_shape_memory_and_budget`, ui `as16_shape_memory_dialog` |
| 17 | ui `as17_drag_outside_area` |

### Constitution check
Layering holds (the geometry, the constraint and the step are libc-only core; the canvas turns input and frames into core
calls); no threads or caches (P5); the width and fill style are session choices, not settings (P6 kept for Preferences);
hard-edged pixels (P4); every REQ has a scenario and every scenario a test (S3).

### Risks
- **The preview equals the drawing:** both use the same row function; a core test compares, for many random shapes, the
  rows with a brute-force classification of every pixel of the box, and a ui test renders the window during a drag and after
  the release and compares the shape's pixels.
- **Line's per-row ranges are contiguous:** a square pen stamped along an 8-connected line covers one contiguous x range per
  row; the brute-force test above checks it for random lines and widths before the preview relies on it.
- **A transparent drawing color in the preview:** a color with alpha below 255 replaces the pixel (REQ-0010-06), so the
  preview must show the checkerboard under it, not the picture; the preview draws the checkerboard first for such runs.
  Checked by rendering, with the foreground #FF000080.
- **Shift pressed or released without a pointer move (REQ-0010-14):** the window's key controller must reach the canvas
  during a held button; a probe (the first task) checks that a Shift key event arrives during a drag before the code relies
  on it.
- **Probe T006 (2026-10-19, real keys on JOD's Wayland session, temporary code): the Shift risk holds.** With the left
  mouse button held over the canvas, the window's key controller (capture phase) received `key-pressed Shift_L` and
  `key-released Shift_L`, each with `button1 held` in its state, and the `modifiers` signal fired when Shift went down
  (shift 1, button1 1). The same events arrive with no button held. In `key-pressed` and `key-released` the state is the one
  before the change (shift 0 at the press, shift 1 at the release), so the gesture takes the new Shift state from the key controller's
  `modifiers` signal, whose state is the new one (it covers both Shift keys), and from the modifier state of every pointer
  press and motion.
  Not shown by the paste: a press of the right button, which uses the same path (the keyboard does not depend on the
  pointer's grab).
- **Memory of a big filled shape:** 12 bytes a changed pixel, held as counted (F1 of the check); the failure path is tested with an address-space limit, as
  Fill's is; peak memory measured in `perf-0010`.

## Tasks
Groups end with a full `meson test` and `tools/mutate.sh --lint`; each ticked task gets a `Result:` line under it. M1 and
M2 are manual checks (the look and the feel cannot be judged headless).

A. The tools and their options (the look first)
- [x] T001 Four tools in the toolbox, their hints and the crosshair, and the options: the widths and the fill styles on two shared actions, Numpad steps the width (REQ-0010-01, -02, -03, -04, -15, -16; AS-1, AS-2, AS-3) - done when ui `as1_shape_tools_toolbox_hints`, `as2_width_and_fill_style_shared` and `as3_numpad_steps_width` pass and fail first, the toolbox tests of 0006 to 0008 (the width for every tool included) pass, mutants (the width not shared; the fill style shown for Line; Numpad changing the fill style) are killed by assertions, and manual M1 (the four icons, the crosshair, the widths and fill-style buttons in the light and dark themes) is done before any drawing is built
  Result: each failed first, by not compiling (`EASEL_TOOL_LINE` undeclared) and then at run time: `as2_width_and_fill_style_shared` at `shown_selected: 'b' should not be NULL`, `as1_shape_tools_toolbox_hints` and `as3_numpad_steps_width` by fatal GLib messages (no tool button, no `line-width` action); with the four tools, two option pages bound to `win.line-width` and `win.fill-style`, the hints, the crosshair and the Numpad step, all pass. A press with the four tools returns early for now (it would have fallen into the Eraser branch). `as2` first clicked the toolbox's Fill tool, whose tooltip is the same as the "Fill" fill style, so it finds the style buttons by their action. `as6_toolbox_width_is_the_same_for_every_tool` (0008) now includes the four tools: 87 / 87 / 93 px at 100 / 150 / 200 % text, the same for every tool. 3 mutants killed by assertions (the width not shared; Line showing the fill styles; Numpad changing the fill style); the REQ-0006-14 keypad-guard mutant rewritten for the new code. Full `meson test` 29 of 29; `mutate.sh --lint` 172 mutants, 0 stale. Manual M1 (JOD, 2026-10-19): pass (the four icons, the crosshair, the widths and fill-style buttons, the shared width, Numpad, light and dark themes)

B. The geometry in the core
- [x] T002 Rectangle, Rounded Rectangle and Ellipse as row runs, with the outline, the inside and the fill styles (REQ-0010-11, -12, -13; AS-6, AS-7, AS-8) - done when core `as6_rectangle_fill_styles`, `as7_ellipse_pixels` and `as8_rounded_rectangle_pixels` pass and fail first, a brute-force test classifies every pixel of random boxes (1 × 1 to 40 × 40, widths 1 to 5) and finds the same runs, and mutants (the box from the pixel centers; the radius not capped; the inside shrunk by w − 1) are killed
  Result: `test-figure` failed first by not building (no `figure.h`), then with a stub that returned no pixels at `test-figure.c:69: CHECK failed: x == 10 && y == 10 && w == 10 && h == 5`; with the geometry (the ellipse's test in exact 128-bit integer arithmetic, since dx² · h² reaches 10²⁶ at the coordinate limit; rows from a formula estimate cut back to the exactly-tested members) `as6_rectangle_fill_styles`, `as7_ellipse_pixels`, `as8_rounded_rectangle_pixels` and the brute-force `as6_runs_match_the_definition` (1800 random figures of 1 to 40 pixels, widths 1 to 5, every pixel against a per-pixel classification) pass. Three expected numbers of mine were wrong and were replaced with those of an independent script before the test passed: a radius-8 corner leaves 12 pixels out, not 16 (a 40 × 16 box has 592 pixels), a radius-7 corner 10 (40 × 15 has 560). The module is `figure`, not `shape` (`EaselShape` is the footprint's enum): the Plan is corrected. 5 mutants (the box from the pixel centers; no radius cap; the inside shrunk by w − 1; the inside radius not made smaller; an ellipse estimate too narrow) killed by assertions; core, io and ui-0010 green; `mutate.sh --lint` 177 mutants, 0 stale
- [x] T003 Line under the square pen as row ranges (REQ-0010-10; AS-4 core part, AS-5) - done when core `as4_line_pixels` and `as5_square_pen` pass and fail first, the brute-force test stamps random lines and widths and finds one contiguous range per row equal to the rows given (the Plan's contiguity risk), and mutants (the pen placed at the line's pixel, not at ⌊(w − 1) / 2⌋; a 4-connected line) are killed
  Result: `as4_line_pixels`, `as5_square_pen` and the random `line_matches_the_stamps` failed first by not building (no `easel_figure_prepare`), then with a stub at `test-figure.c:343: CHECK failed: (img->px[i] == 0xFF000000u) == (set[i] == 1)` (no Line pixels); with the Line's rows (Bresenham as the Pencil's, one pass per figure into a table of the picture rows plus the pen's reach, one contiguous range per row) they pass: nine lines against the Pencil's own pixels, the 3, 4, 2 and 5-pixel pens of AS-5, and 400 random lines and widths against the stamps of every Pencil pixel, with the contiguity asserted per row (the Plan's risk: it holds). `figure.c` carries its own copy of the Bresenham loop; the test against the Pencil guards it from drifting. 3 mutants (the pen at the corner; a 4-connected line; the pen one pixel short) killed by assertions; core and io green; `mutate.sh --lint` 180 mutants, 0 stale
- [x] T004 Shift's constraint (REQ-0010-14; AS-9 core part) - done when core `as9_shift_constraint` (the three directions of AS-9, the square toward each of the four quadrants, a zero-length drag) passes and fails first, and a mutant (the larger side) is killed
  Result: characterization, by my slip: `as9_shift_constraint` and `easel_figure_constrain` were written in one step, so the test had no red run before the code. Checked afterwards by breaking the code once (the function made a no-op, a copy of `figure.c` set aside and restored): the test failed at `test-figure.c:446: CHECK failed: x == 40 && y == 10`, and passes with the function. It covers the three lines of AS-9, the eight directions, the 22.5-degree boundary on both sides (a 29-by-12 drag is horizontal, a 28-by-12 one diagonal), a drag of no movement, and squares and circles toward all four quadrants for the three shapes. 3 mutants (the larger side; the larger distance on the diagonal; a cut at 45 degrees) killed by assertions; core and io green; `mutate.sh --lint` 183 mutants, 0 stale
- [x] T005 Applying a shape and its undo step (REQ-0010-06, -07, -20 core part; AS-4 the step's cost, AS-10 core part, AS-13 core part, AS-16 core part) - done when core `as10_clipped_and_exact_color`, the step test (undo and redo restore exactly; 8 bytes a changed pixel plus 64; no step when nothing changes; the steps owned on success and kept by the caller on failure) and `as16_shape_memory_and_budget` (an address-space limit makes it refuse and change nothing; a step larger than a 16 MB budget is the only step) pass and fail first, the core tests of 0002, 0004 and 0007 pass unchanged, and mutants (undo writes the new color; the cost counted per covered pixel; no restore on failure) are killed
  Result: `as10_clipped_and_exact_color`, `as13_step_undo_redo_and_nothing` and `as16_shape_memory_and_budget` (a new `test-figure-apply`) failed first by not building (no `easel_figure_apply`), then with a stub at `test-figure-apply.c:53: CHECK failed: easel_figure_apply (...) == EASEL_FIGURE_DONE`; with `easel_figure_apply` they pass: a line from (-20, 50) clipped to its 51 pixels in row 50 in exactly #FF000080, a figure wholly outside giving NOTHING, the three fill styles on a 10 x 5 rectangle (26 outline pixels; then 24 inside in the other color; then all 50 in the drawing color), one step of 12 bytes a changed pixel plus 64 (`bytes == changed * 12 + 64`), undo and redo exact, a repeat and a white-on-white figure adding no step, no history just drawing, an address-space limit and a 1 MB cap both giving NO_MEMORY with every pixel back and no step, and a 1800 x 1800 filled rectangle (3.24 million pixels, 38.9 MB) kept as the only step against a 16 MB budget, one Undo restoring it. Plan corrected: no new history step kind is built; the existing step of changed pixels (12 bytes a pixel) is within REQ-0010-07 and REQ-0002-16, so the Plan's "shape" step of 8 bytes a pixel is dropped (a whole 10000 x 10000 ellipse is then about 942 MB, still kept as the only step or refused). 4 mutants (fill-only in the other color; no restore on failure; no clip at the left edge; a no-change figure still adding a step) killed by assertions. Group B gate: full `meson test` 31 of 31, `mutate.sh --lint` 187 mutants, 0 stale, 0 warnings

C. Drawing in the window
- [x] T006 Probe: a Shift key press and release reach the window while a mouse button is held over the canvas (Plan risk), in a real frame - done when the result is in the Plan's risks, and if they do not arrive, JOD has chosen the fallback before T009
  Result: probe, no code kept (the print statements reverted, `window.c` as before): no key-injection tool is installed here (`ydotool`, `wtype`, `xdotool` missing), so JOD ran it by hand; real Shift key presses and releases reach the window with a mouse button held, with `button1 held: 1` in their state (the paste of JOD's terminal in this session), and the modifiers signal fires on Shift going down; details and what the gesture takes from them in the Plan's risks
- [x] T007 The shape gesture: the preview and the release (REQ-0010-05, -06, -07; AS-4, AS-6, AS-10, AS-13 ui parts) - done when ui `as4_line_preview_and_step`, `as6_rectangle_on_canvas`, `as10_line_from_outside` and `as13_shape_changing_nothing` pass and fail first, a test renders the window during a drag and after the release (at 100 % and at a scrolled 400 %) and finds the same pixels, and the preview of a #FF000080 shape shows the checkerboard under it
  Result: `as4_line_preview_and_step`, `as6_rectangle_on_canvas` and `as10_line_from_outside` failed first (`as4` at the rendered preview, the other two at `count_color (...) == 26: (0 == 26)` and `== 51: (0 == 51)`: a press with the four tools did nothing yet); `as13_shape_changing_nothing` passed at once (characterization: a press did nothing, so nothing changed; it is broken by the mutant below); `as4_preview_equals_drawing` failed at its last check for the right reason (no shape drawn). With the gesture (press keeps the start, the colors and the button; motion follows the pointer; the preview draws the core's runs for the rows the view shows, the checkerboard under a transparent color; the release applies one step) all pass: the Line's preview is the Pencil's line pixel by pixel, the picture after the release renders the same pixels as the preview, the rectangle's three styles and a 2-pixel width with either button's colors (26 / 24 / 50 and 44 / 6), a line from outside the picture clipped to its 51 pixels in exactly #FF000080 (half-transparent red, `0x80FF0000`), and a filled 3-pixel Ellipse at a scrolled 400 %, opaque and half-transparent, rendered the same during and after (the canvas area less 40 pixels along the edges, where the scrolled area animates a shading). Two slips of mine fixed on the way: my first test colors `0xFF000080` were opaque navy, not the spec's `#FF000080`, so the transparent path was not tested until corrected; and the core's `paint_run` wrote the pixels directly, so the tiles did not refresh (the history and Fill write through `easel_image_set`): the apply and its restore now use it. The preview checks of the Plan's risks pass. 6 mutants (no checkerboard under the preview; no preview; the secondary button drawing in the foreground; the release not drawing; no modified mark; plus the REQ-0010-07 one of T005) killed by assertions; the REQ-0009-05 grid mutant's find text was made unique. Full `meson test` 31 of 31, `mutate.sh --lint` 192 mutants, 0 stale, 0 warnings
- [x] T008 Cancel, ignored input, a lost release, focus loss, Save and Quit during a drag (REQ-0010-08, -09, -18, -19; AS-11, AS-12, AS-15) - done when ui `as11_cancel_and_ignored_during_drag`, `as12_lost_release_draws_shape` and `as15_focus_loss_draws_shape` pass and fail first (or are recorded as characterization with a break, if the stroke paths already give them), and mutants (Esc does not cancel; a lost release cancels instead of drawing) are killed
  Result: `as11_cancel_and_ignored_during_drag`, `as12_lost_release_draws_shape` and `as15_focus_loss_draws_shape` failed first (`as11` at `easel_canvas_is_stroking (canvas) should be TRUE`: a shape drag was not a stroke yet; `as12` at the options not usable again; `as15` at `count_color (...) == 51: (0 == 51)`); the shape drag now counts as a stroke for the canvas's existing paths (`is_stroking`, `cancel_stroke` for Undo and the other button, `end_stroke` for Save, Quit, a lost focus and a lost release, `buttons_released`), and the window's Esc cancels it, and the two actions ignore a width or fill style chosen during a drag, so all pass: the other button, Esc and Undo cancel (nothing drawn, no step, the release after does nothing, a second Undo before the release is ignored and then works), a tool, a width, a fill style, Zoom In and the numpad are ignored during a drag, Save draws the rectangle as shown and saves it (51 x 31 pixels), a lost release and a focus loss draw it as one undo step. 5 mutants (Esc not cancelling; the other button not cancelling; a lost release or focus loss dropping the shape; a width chosen during a drag; a second Undo not ignored) killed by assertions; two older mutants (REQ-0007-16 and -18) rewritten for the changed lines and killed; core, io, ui-0001, -0007 and -0010 green; `mutate.sh --lint` 197 mutants, 0 stale
- [x] T009 Shift during the drag (REQ-0010-14; AS-9 ui part) - done when ui `as9_shift_during_drag` (pressed and released during the drag, with and without a move) passes and fails first
  Result: `as9_shift_during_drag` failed first at `easel_image_get (img, x, 10) == 0xFF000000u: (0xffffff00 == 0xff000000)` (Shift down did not flatten the Line). With the canvas keeping the pointer's pixel apart from the end pixel and `easel_figure_constrain` applied on every change, fed by the window's key controller `modifiers` signal (new state; it returns gboolean, a void handler crashed the first run) and by the modifier state of pointer events, it passes: Line flat at Shift down with no move, Rectangle square and back to 40 x 25 at Shift up with no move, a move under Shift to the upper left, and Shift held before the press (a 25 x 25 circle of 489 pixels). Mutant: Shift ignored in `easel_canvas_set_shift` killed by the test. Not testable here: the pointer event's own modifier state (the test cannot send events), left to manual M2. Full `meson test` 31 of 31
- [x] T010 Blocked input, a drag outside the area, and the memory dialog (REQ-0010-17, -20, -21; AS-14, AS-16 ui part, AS-17) - done when ui `as14_shapes_ignored_when_blocked`, `as16_shape_memory_dialog` and `as17_drag_outside_area` pass and fail first, and manual M2 (drawing each tool on a real picture with the mouse: widths, fill styles, Shift, the other button, Esc, a drag outside the window, the dark theme) is done
  Result: `as14_shapes_ignored_when_blocked` failed first at the Grid dialog case (`shape_tools_change_something (...)` should be FALSE: a press reaching the canvas drew; the dialog is modal for the real pointer, so the window now blocks canvas input on `notify::visible-dialog` too, also for the quit prompt); `as16_shape_memory_dialog` failed first at `dialog should not be NULL` (no hook, no signal; now `easel_canvas_set_shape_memory_limit`, the `shape-failed` signal and the window's "Could Not Draw the Shape" dialog from an idle callback); `as17_drag_outside_area` passed at once (characterization: the end already followed the pointer and Ctrl+wheel and the pinch zoom only in Pan; broken by the mutant that ends the follow at the area's edge, killed by `as4_preview_equals_drawing` and `as17`). A slip of mine on the way: the first dialog block used `dialog_is_open`, whose prompt flag is still set when the dialog closes, so 0005 `as15_actions_ignored_and_stroke_ended` caught input staying blocked; it now looks at the visible dialog, the chooser and loading only. 4 new mutants killed; full `meson test` 31 of 31, `mutate.sh --lint` 202, 0 stale. Manual M2 done by JOD with the mouse, seven steps, all as expected: Line widths 1-5; the three shape tools in all three fill styles at widths 3 and 1; Shift for Line (0/45/90 degrees) and the square and circle, pressed during the drag and held before the press; Esc, the other button and ignored option changes during a drag; a drag past the window edge and a line from outside the picture; undo and redo per shape and a no-change shape adding no step; the dark theme

D. The numbers
- [x] T011 Measure the preview and a big shape (engineering.md: a pointer event within a frame) - done when `perf-0010` has run and its numbers (a filled Ellipse and a 5-pixel Line dragged, at 1920 × 1080 and 8000 × 8000, at 100 % and at the fit zoom; the release of a whole-picture filled Ellipse at 8000 × 8000, time and peak memory) are in the Plan, and any frame over budget has been reported to JOD before a fix
  Result: `perf-0010` added (a meson benchmark) and run; the numbers and the one frame over budget (the filled Ellipse at 8000 × 8000 and fit zoom, 85 ms worst, medians all one frame) are in the Plan's Measurements, reported to JOD before any fix. The release of a whole 8000 × 8000 filled Ellipse takes 204 ms and peaks at 952 MB

E. Fixes from the check
- [x] T012 The step holds what the budget counts (F1; REQ-0010-07, REQ-0002-16) - done when core `as4_step_holds_what_it_counts` (the heap held after a figure whose log just passed a doubling, against `easel_history_bytes`) fails first, passes with the log shrunk to its length before the push, the mutant "the log is pushed without shrinking" is killed by an assertion, and the 8000 × 8000 ellipse holds what it counts
  Result: `as4_step_holds_what_it_counts` failed first at `test-figure-apply.c:213: CHECK failed: held <= counted + counted / 100 + 4096` (a filled 513 × 512 rectangle, 262,656 pixels: 6,293,024 bytes held, 3,151,936 counted, the log's capacity doubled to 524,288 entries); with a `realloc` to the log's length before `easel_history_push` it passes (3,153,440 held, 3,151,936 counted). The whole 8000 × 8000 filled ellipse, measured the same way (glibc's `mallinfo2`, a scratch program on the core library): 50,265,448 pixels, 603.2 MB held, 603.2 MB counted. Mutant "the log is pushed without shrinking" killed by that assertion. `stroke.c` keeps the same doubling log for strokes (left as is: a stroke's log is small, outside 0010)
- [x] T013 Ctrl+wheel through the wheel handler during a drag (F3; REQ-0010-21 reworded, AS-17) - done when `as17_drag_outside_area` drives the wheel handler with Ctrl held, checks no zoom and that the view is left to scroll, scrolls the view and finds the end moved at the next motion, and a mutant that zooms there is killed by an assertion
  Result: GTK 4 has no public way to make a scroll event, so the wheel's logic moved from the signal callback `on_scroll` into `easel_canvas_wheel (canvas, state, notches, x, y)`, which the callback calls with the event's modifier state and the test calls with `GDK_CONTROL_MASK`. `as17` passed with the new code at once (characterization: the behavior was there; the spec was reworded to it); proven by the mutant "Ctrl+wheel zooms during a shape drag (and still lets the view scroll)", killed at `easel_window_get_zoom (win) == 100: (400 == 100)`. The scroll itself is the scrolled area's own, which the test stands in for by moving the adjustment 100 points; the next motion at the same view point puts the end 100 points lower

## Check
- Date and commit: 2026-10-19, `5d138a5` plus the uncommitted 0010 work tree (repeated after the fixes; the first check's findings kept below)
- Tests: `meson test` 31 of 31 pass (the cases of this feature: core `test-figure` 8 and `test-figure-apply` 4, ui `/0010/` 15)
- Mutants: 204 killed, 0 killed by crash (test gap), 0 survived, as `tools/mutate.sh` reports (1130 s); at the first check 201 killed, 1 killed by crash, 0 survived (1107 s)
- Warnings: 0; `tools/spec.sh lint`, `lint --names 0010` and `mutate.sh --lint` (204, 0 stale) clean
- Scenarios: AS-1 `as1_shape_tools_toolbox_hints` + M1; AS-2 `as2_width_and_fill_style_shared` + M1; AS-3 `as3_numpad_steps_width`; AS-4 `as4_line_pixels`, `as4_step_holds_what_it_counts`, `as4_line_preview_and_step`, `as4_preview_equals_drawing`; AS-5 `as5_square_pen`; AS-6 `as6_rectangle_fill_styles`, `as6_runs_match_the_definition`, `as6_rectangle_on_canvas`; AS-7 `as7_ellipse_pixels`; AS-8 `as8_rounded_rectangle_pixels`; AS-9 `as9_shift_constraint` (C: T004, proven by making the function a no-op) and `as9_shift_during_drag` (it checks the Shift release with the 40 × 25 box, not the scenario's 21 × 31 one: the same rule); AS-10 `as10_clipped_and_exact_color`, `as10_line_from_outside`; AS-11 `as11_cancel_and_ignored_during_drag`; AS-12 `as12_lost_release_draws_shape`; AS-13 `as13_step_undo_redo_and_nothing`, `as13_shape_changing_nothing` (C: T007, killed by the no-modified-mark mutant); AS-14 `as14_shapes_ignored_when_blocked`; AS-15 `as15_focus_loss_draws_shape`; AS-16 `as16_shape_memory_and_budget`, `as16_shape_memory_dialog`; AS-17 `as17_drag_outside_area` (C: T010 and T013, killed by the stop-at-the-area mutant and by the Ctrl+wheel zoom mutant); M2 by JOD
- Findings:
  - F1 (medium, REQ-0010-07, REQ-0002-16): the change log grew by doubling and the history kept it at full capacity, up to about 24 bytes a changed pixel held while 12 were counted. Fixed in T012: the log is shrunk before the push; the 8000 × 8000 filled ellipse holds 603.2 MB and counts 603.2 MB; mutant killed.
  - F2 (low, test gap, not 0010): the REQ-0005-12 mutant crashed once (SIGABRT after `Invalid utf8 passed to gdk_surface_set_title`) and in five single runs and one full run was killed only by a title read from freed memory: its text freed `self->path` while `path` could be it. Fixed: the mutant made memory-safe (a copy assigned, the old path leaked); it then survived, so `as11_save_as_fails` now checks that Save after the failed Save As writes the old file and not the new name, which kills it. `mutate.sh` now keeps each crash's log.
  - F3 (low, REQ-0010-21, AS-17): the spec said Ctrl+wheel "does nothing" during a drag, but the view scrolls as usual, and `as17` called the zoom function. Fixed in T013: REQ-0010-21 and AS-17 reworded (no zoom, the view scrolls as usual, the end moves at the next motion); the wheel's logic is `easel_canvas_wheel`, which `as17` drives with Ctrl held; a mutant that zooms there is killed by the zoom assertion.
  - F4 (low, Plan drift): fixed: the Plan's Files, How each rule is kept, What grows and Risks now say the existing step of changed pixels, `easel_figure_apply`, 12 bytes a changed pixel held as counted, and Shift from the `modifiers` signal and the pointer events.
  - F5 (info, T011): one frame of 85.0 ms in 30 moves for the filled Ellipse at 8000 × 8000 at the fit zoom (10 %), median 16.8 ms; accepted by JOD on 2026-10-19.
  - F6 (info): characterizations T004, AS-13 ui, AS-17 (T010, T013), each proven as listed above; the pointer event's own Shift state has no automated test (no event injection), covered by M2.
