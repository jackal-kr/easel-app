# 0039 Smooth shapes and brushes

Status: implemented
Reviewed: spec-reviewer, 2026-11-10 (15 findings merged; two asked: the 1-pixel circle, the Brush cells)
Depends on: 0006, 0010, 0011, 0020, 0030, 0033

## Why
Every line, shape and brush stroke is drawn in whole hard-edged pixels, as XP Paint does: a diagonal or a circle shows stairs. For
drawings meant to look clean (a logo, a diagram, a sketch), JOD wants an opt-in smoothing (antialiasing) that blends the edge pixels,
off by default so the app stays XP-faithful (`docs/product.md`, `docs/reference/new-behaviors.md`, ADR-0003).

## Requirements
Pointer notes added at acceptance to REQ-0006-06 and REQ-0010-06 (smoothing when Smooth is chosen), REQ-0010-02 and REQ-0030-04 (a new well), REQ-0030-02 (the Brush cells, REQ-0039-11), REQ-0020-05 and REQ-0020-11 (the new key), and `docs/reference/new-behaviors.md` (two cells, not a switch).
- REQ-0039-01 (Pointer note, Smooth text, 2026-11-11: Text now shows a Sharp | Smooth well of its own, REQ-0040-01; this one stays the seven tools'.) (Adds to REQ-0030-04's look.) While Line, Curve, Rectangle, Rounded Rectangle, Ellipse, Polygon or Brush is active, the options panel shall show, below that tool's wells, one more well with two cells side by side, Sharp then Smooth, drawn as the Eraser's mode well is (REQ-0030-04): Sharp shows a line at 45 degrees made of whole pixels, Smooth the same line with blended edges, each in the theme's text color. Their tooltips and accessible names are "Sharp edges" and "Smooth edges". One of the two is chosen; it is one choice shared by the seven tools (choosing it with one is the choice of all, as the width is, REQ-0010-03). Pencil, Airbrush, Eraser, Fill, Gradient, Text and the selection tools show no such well and draw as before (`docs/reference/new-behaviors.md`).
- REQ-0039-02 The choice shall be changed by a click on a cell, or from the keyboard as every cell of the panel is (the focus moved to it and Space or Enter), and by nothing else: no shortcut (JOD, 2026-11-10). While a button is held for a shape, a stage of a curve or polygon, or a stroke, choosing it is ignored, as choosing a width is (REQ-0010-08, REQ-0011-13); while a curve or polygon waits, choosing Sharp or Smooth redraws it at once in the new choice, and it is put down that way, as a width chosen then does (REQ-0011-12).
- REQ-0039-03 (Adds to REQ-0020-05 and REQ-0020-11.) At start-up the choice shall be Smooth when the settings file's `[drawing]` key `smooth` is `true`, in any case as the file's other values (REQ-0020-11: `style=DARK` is dark), and Sharp otherwise (missing, `false` or any other value, silently, REQ-0020-11). Preferences shall show a switch "Smooth shapes and brushes" in its Tools group, after the line width and the fill style (a switch where the group's other rows are choices), written as `smooth=true` or `smooth=false` at OK or Apply (REQ-0027-07); like the other rows of that group it sets the start-up choice and leaves the current session's choice as it is. A settings file the app creates holds `smooth=false`. A choice made in the options panel lasts for the session and is not written to the file.
- REQ-0039-04 While Smooth is chosen, each of the seven tools shall draw its ideal shape (REQ-0039-05) by coverage: a pixel's coverage by a part of the figure is how many of the 256 points at (x + (i + 1/2) / 16, y + (j + 1/2) / 16), i and j from 0 to 15, of pixel (x, y) lie inside that part or on its edge (a point on the edge between the outline and the inside counts for the outline only, so the counts never add up to more than 256), picture coordinates having pixel (x, y) span x to x + 1 and y to y + 1. A pixel no part covers is not changed; a pixel one part covers with all 256 points takes that part's color exactly; any other covered pixel takes the mix of REQ-0039-06.
- REQ-0039-05 The ideal shapes shall be, with w the width (REQ-0010-02), positions at pixel centers (pixel (x, y) at x + 1/2, y + 1/2), and the parts and colors of REQ-0010-13 (the outline in the drawing button's color, the inside in the other): (a) Rectangle, Rounded Rectangle and Ellipse: the shapes of REQ-0010-11 and REQ-0010-12, read for points instead of pixel centers (the outline: the points of the shape not in the shape of the smaller box; the inside: the rest). Their box's sides lie on pixel edges, so a Rectangle is the same with Sharp and Smooth. (b) Line: the rectangle of width w along the segment between the centers of the start and end pixels, reaching w / 2 beyond each of them, both moved by 1/2 to larger x and larger y when w is even (where REQ-0010-10's square puts the line's pixel); a point alone (start = end) is the w x w square there. So a level or upright Line is the same with Sharp and Smooth. (c) Curve: the 1-pixel path's points of REQ-0011-05 and REQ-0033-05 (its N + 1 points of the curve, before rounding to pixels, each taken to the nearest 1/32 of a pixel, a half rounded up toward larger x or y (JOD, 2026-11-11: exact arithmetic)), joined by straight pieces, each drawn as a Line is and the joins rounded (the points within w / 2 of the path), with the Line's square ends at the start and the end; whenever REQ-0011-05 makes the curve its Line (no bend, or both control points on the segment), it is the Line of (b). (d) Polygon: the outline is the union of its sides, each the rectangle of a Line without the parts beyond its ends, and of the round joins at its corners (the points within w / 2 of a corner), the closing side included (JOD, 2026-11-11: no square tips at corners); the region is the polygon by the alternate rule of REQ-0011-08, read for points; the inside is the region less the outline. A polygon not yet closed shows its sides so, joins rounded, with the Line's square ends at its first and last corner only, no closing side and no fill (REQ-0011-09). (e) Brush: the footprint's shape (a circle of diameter d centered in its d x d box, but the circle of 1 is its 1 x 1 square, so a click paints one full pixel (JOD, 2026-11-10: a true circle of 1 covers 208 of 256 points and never paints a full pixel); a square, the box; "/" and "\", the parallelogram of the points of the box whose distance along x from its diagonal (from the bottom-left corner to the top-right one for "/", from the top-left to the bottom-right for "\") is at most 1/2), placed at each pointer pixel as REQ-0006-05 places the footprint, and swept along the straight segment between each two consecutive pointer pixels of REQ-0006-06, so that the stroke has no gaps and no stripes (JOD, 2026-11-10: all four shapes).
- REQ-0039-06 A partly covered pixel shall take the mix of its old color and the colors that cover it, each weighted by its count of points (the old color by 256 less their sum), in premultiplied alpha: for old (r, g, b, a) and parts k with color (r_k, g_k, b_k, a_k) and count n_k, with M = a (256 - Σ n_k) + Σ a_k n_k, the alpha is round (M / 256) and the red, when M is not 0, round ((r a (256 - Σ n_k) + Σ r_k a_k n_k) / M), at most 255, green and blue alike (divided by M, not by the rounded alpha, so that a color drawn over a fully transparent pixel keeps its red, green and blue exactly); an alpha of 0 gives 0x00000000 (P7.7). Halves round up.
- REQ-0039-07 A figure or a stroke shall change each pixel once, by its whole shape: where its own pieces overlap (a stroke crossing itself, two sides of a polygon at a corner, the stamps of a stroke) the coverage is that of their union, so a crossing is no darker. A Brush stroke shows, while it is dragged, the coverage of all its path so far, each pixel mixed from its value before the stroke.
- REQ-0039-08 With Smooth chosen, everything else shall be as with Sharp: the buttons and colors (REQ-0010-06 with blending only by REQ-0039-06), one undo step and the modified mark, cancel, a lost release, Shift, Save or Quit during a drag, pending curves and polygons, the pixels inside the picture only, nothing drawn outside a marked selection's shape where the tool is limited to it as with Sharp, the undo budget (REQ-0002-16), and a press on a waiting curve (REQ-0033-01: the squares of the pixels the curve covers at all). If the memory for a figure, its coverage or its undo step cannot be had, the app shall show "Could Not Draw the Shape" and change nothing, as REQ-0010-20 and REQ-0011-15 say, and a Brush stroke as a stroke does with Sharp; a Brush stroke whose release never arrives (REQ-0007-18) ends with the coverage last shown. The pointer over the canvas is as with Sharp (the Brush's footprint outline of REQ-0006-16 included). With Sharp chosen, every tool draws exactly as before this feature.
- REQ-0039-09 The pixel contract (P7): (1) every pixel is whole, its color the part's exact color, the old color, or the mix of REQ-0039-06: smoothing, opt-in, as P7.1 allows; (2) the view only magnifies; nothing changes in it; (3) a preview, a pending curve or polygon and a Brush stroke being dragged show exactly the pixels put down, made by the same code; (4) the coverage depends only on the integer positions and the width, in exact arithmetic, never on the zoom, the scroll or the screen scale.
- REQ-0039-11 (Changes REQ-0030-02 while Smooth is chosen.) The Brush's preset cells shall show each footprint as a click with Smooth paints it, its partly covered pixels mixed into the cell's background by REQ-0039-06, and the sharp footprint again when Sharp is chosen, so a cell always shows what a click paints (JOD, 2026-11-10).
- REQ-0039-10 With Smooth, a Brush stroke shall keep the pointer-to-screen budget of `docs/engineering.md` (within one frame, 16 ms, on a 1920 x 1080 picture) with the largest footprint, and a shape's or a curve's preview shall show within one frame (16 ms) at 1920 x 1080 and within 100 ms at 8000 x 8000 for a figure as large as the picture.

## Scenarios
- AS-1 (REQ-0039-01, REQ-0039-02) Given Line active, Then a well Sharp | Smooth shows below the widths, Sharp chosen, with its tooltips; When Smooth, Then Rectangle, Ellipse, Curve, Polygon, Rounded Rectangle and Brush show Smooth chosen; Pencil, Airbrush, Eraser show no such well; Tab reaches the cells and Space chooses; during a drag a click on Sharp changes nothing | Verified by: automated (ui) + manual M1 (the look, JOD)
- AS-2 (REQ-0039-03) Given `smooth=true` in `[drawing]`, When the app starts, Then Smooth is chosen; with `smooth=yes` or no key, Sharp; When Preferences' switch is turned on and Apply is pressed, Then the file holds `smooth=true` and the panel's choice is unchanged; turned on and Cancel, the file is unchanged; a created file holds `smooth=false`; choosing Smooth in the panel writes nothing | Verified by: automated (ui)
- AS-3 (REQ-0039-04, REQ-0039-05, REQ-0039-06) Given a white 20 x 20 picture and black, Smooth, width 1, When a Line from (2, 2) to (12, 7), Then pixel (2, 2) has 229 points and is #1B1B1BFF, (3, 2) 144 and #707070FF, (4, 3) 238 and #121212FF, (13, 7) 10 and #F5F5F5FF; When a Line from (2, 15) to (17, 15), Then the pixels equal Sharp's (row 15 from x 2 to 17 black, rows 14 and 16 white) | Verified by: automated (core)
- AS-4 (REQ-0039-05) Given Ellipse, Outline and fill, width 2, black on white, Smooth, When an ellipse in the box (2, 2) to (17, 11), Then pixel (3, 4) has 211 outline points and is #2D2D2DFF, (4, 6) 28 outline and 228 inside points and is #E30000FF, (9, 6) is inside only, #FF0000FF; a Rectangle in any box equals Sharp's | Verified by: automated (core)
- AS-5 (REQ-0039-05, REQ-0039-07) Given the Brush circle of 7, Smooth, When a drag (5, 5) to (25, 5) to (5, 6), Then each pixel is as by the union of the sweep, the overlap no darker than the one-pass edge; When "/" of 8 dragged across its own direction, Then no empty diagonal between stamps | Verified by: automated (core)
- AS-6 (REQ-0039-06) Given a Line from (2, 2) to (12, 7), width 3, in #FF000080 with Smooth, over a fully transparent picture, Then pixel (5, 2) (176 points) is #FF000058, (4, 5) (49 points) #FF000019, (2, 2) (all 256) #FF000080, (3, 5) (none) #00000000; over white, (5, 2) is #FF7979A8, (4, 5) #FFE4E4E7, (2, 2) #FF000080, (3, 5) #FFFFFFFF | Verified by: automated (core)
- AS-7 (REQ-0039-08, REQ-0039-09) Given Smooth and an ellipse dragged, Then the preview at 100 % and at 800 % shows the pixels put down; a second run gives the same pixels; a scroll and a zoom during a Brush drag leave the pixels as an undisturbed drag's; Esc during the drag draws nothing; Undo restores the picture; Given a curve waiting with Sharp, When Smooth is chosen, Then it shows smoothed at once and Enter puts it down so; a lost release ends a Brush stroke with the coverage last shown | Verified by: automated (ui, rendering the window)
- AS-8 (REQ-0039-10) Given 1920 x 1080, Then a Brush motion with the circle of 7 shows within 16 ms and an ellipse's preview frame too; Given 8000 x 8000, Then an ellipse as large as the picture shows within 100 ms | Verified by: automated (perf, in the real session)

- AS-9 (REQ-0039-05, REQ-0039-07) Given a closed Polygon whose sides cross (a bow tie), width 1, Fill style Outline and fill, Smooth, Then its region follows the alternate rule (the crossing's two triangles filled), each corner pixel changed once, no darker than a side's edge; a Rounded Rectangle and a Curve with one bend have pixels equal to the prototype's numbers recorded in the Plan's test | Verified by: automated (core)
- AS-11 (REQ-0039-05, REQ-0039-11) Given Brush, Smooth, black on white, When a click with the circle of 1, Then one pixel is #000000FF and no other changes; with the circle of 4 at (5, 5), Then its middle 2 x 2 pixels are #000000FF, its edge pixels have 235 points (#151515FF) and its corners 81; Then the circle of 4's cell shows those pixels; When Sharp, Then the cell shows `.##.` `####` `####` `.##.` again | Verified by: automated (core, ui)
- AS-10 (REQ-0039-08) Given memory made too small, When an ellipse is drawn with Smooth, Then "Could Not Draw the Shape" shows and nothing changes | Verified by: automated (ui)

## Edge cases
- Width 1 at 45 degrees: the line is about 1.4 pixels across in coverage; its pixels are partly covered on both sides.
- A shape partly outside the picture: only the pixels inside change (REQ-0039-08); coverage is counted as if the picture went on.
- A color with transparency: a fully covered pixel takes it exactly (as Sharp does); a partly covered one by REQ-0039-06.
- Outline style: the inside's pixels keep their color; an outline pixel half over the inside is mixed with the old color there.
- A one-pixel box or a press and release without a move: the shape of start = end, by the same rules.
- The options panel of Rectangle, Rounded Rectangle and Ellipse gains a third well: it must fit the toolbox at the smallest window (REQ-0008-07), checked in the first ui task.
- A Brush stroke over pixels changed earlier in the same stroke: mixed from the value before the stroke, once (REQ-0039-07).

## Out of scope
- Smooth text (a later feature); smoothing for Pencil, Airbrush, Eraser, Fill, Gradient, selections, stretch, skew and rotation.
- Other ends or joins (round ends, mitred corners), sub-pixel positions, gamma-correct blending.
- A shortcut for the choice (JOD, 2026-11-10).

## Open questions
- None. JOD looked at the prototype (a smooth line, ellipse, curve, polygon and brush strokes at 100 %, 600 % and 800 %) and accepted it with the spec, 2026-11-10.

## Plan
(Opus, 2026-11-11.) The prototype of the spec was lost in a reboot; a new one (`docs/reference/proto-0039/`, exact fractions,
kept in the repo this time) gives every number of AS-3, AS-4, AS-6 and AS-11 again, so the reading below is the spec's.

Approach. One unit, 1/32 pixel: a sample point of pixel (x, y) is at (32x + 2i + 1, 32y + 2j + 1), a pixel center at 32x + 16, all
integers. A **sample row** is one of the 16 rows of points of a picture row. Every shape is described, for each sample row, by its
sample columns (column c is the point 16x + i), as intervals; a pixel's count is the number of its 256 points in them. All arithmetic
is integer (`__int128` where products can exceed 63 bits, as `figure.c` and `remove-bg.c` already do), so REQ-0039-09 (4) holds.
- Rectangle, Rounded Rectangle and Ellipse: the Sharp shape on a grid 16 times finer: `extent` and `member` of `figure.c` called with
  the box, the corner radius and the inset w all times 16, one sample row being one row of that grid (the new prototype checks this
  against the spec's points). So Rectangle with Smooth equals Sharp by construction.
- Line: the rectangle of REQ-0039-05 (b) as four half-planes; with d the segment and v a point from the start's center, |v x d| <= K
  and -K <= v . d <= |d|^2 + K, where K = floor (sqrt ((16w)^2 |d|^2)) (v x d and v . d are integers, so the test is exact); each bound
  gives a column range by exact floor and ceiling division. Start = end: the w x w square.
- Curve and Polygon outline: pieces, each a band (a segment's rectangle without its ends) or a disk of radius w/2 at a join, plus
  the square ends where the spec has them; a piece's sample-row interval is exact (bands as above, disks by an integer square root);
  the row's intervals are sorted and merged, so overlaps count once (REQ-0039-07). Pieces are bucketed by picture row at prepare, as
  the Sharp curve's `PathRows` are, so a row costs its own pieces only.
- Polygon region: the alternate rule per sample row: the crossings of the sides with the sample row (a sample row's y is odd, a
  corner's even, so it never passes through a corner), the columns between the 1st and 2nd, 3rd and 4th, ... crossing; the inside is
  the region less the outline, interval by interval.
- Brush: a stamp is the footprint's shape (a disk, a square, a parallelogram; the circle of 1 is its square); a segment between two
  pointer pixels sweeps it: the shape at both ends and, for each edge of a polygon shape (or the band of a disk), the edge swept, all
  convex, so the sweep's sample-row interval is the least and greatest column of its pieces. The stroke keeps, for every pixel it has
  touched, its 256 points as a 32-byte mask in tiles of 16 x 16 pixels made when first touched; a new segment ORs its intervals into
  the masks and each changed pixel is written as `mix (value before the stroke, color, popcount)`, the value before being the log's
  `old` (each pixel logged once, its `new` updated in place through the tile's slot). Union by construction (REQ-0039-07).
- Mix (REQ-0039-06): one function, with M and the rounding of the spec in 64-bit integers.
- One description serves preview and put-down (P7.3): the canvas draws, for a smooth figure, the full runs as today and each partly
  covered pixel as its mixed color over the picture's pixel (`mix (easel_image_get (...), ...)`), the same call the apply makes.

Files and interfaces (brought to the names as built, 2026-11-11).
- New `src/core/cover.{c,h}`: `easel_cover_mix (old, c_out, n_out, c_in, n_in)` (REQ-0039-06, 64-bit integers);
  `EaselCoverRun { int x0, x1; uint16_t out, in; }`; `EaselCoverSpan { int64_t a, b; int part; }` and `easel_cover_count (spans, n,
  scratch, runs, max)` (a row's spans of its 16 sample rows to runs, by a sorted sweep); the pieces, exact in 128 bits:
  `easel_cover_band` (a segment's band, square ends optional), `easel_cover_disk`, `easel_cover_sweep` (a footprint swept from one
  pointer pixel to the next: a capsule for a circle, else the polygon at both ends and each edge swept) and `easel_cover_footprint`
  (a click's points per pixel of the box, for the cells).
- `src/core/figure.{c,h}`: `EaselFigure.smooth`; `easel_figure_cover_max_runs`, `easel_figure_cover_row`; `easel_figure_cover_color`
  (the style's parts mixed over an old color, shared by the put-down and the preview). Inside: `box_spans` (Rectangle, Rounded
  Rectangle, Ellipse: Sharp's `extent` with the box, radius and inset times 16), `line_spans`, and for Curve and Polygon `Smooth`
  (pieces listed per picture row by `smooth_prepare`; `smooth_spans` merges a sample row's pieces and cuts the region by the alternate
  rule), `curve_coord32`, `curve_steps` (factored out of `curve_path`). The put-down: `easel_figure_apply_capped` calls `sharp_apply`
  or `cover_apply`.
- `src/core/stroke.{c,h}`: `easel_stroke_begin_smooth`, the test hook `easel_stroke_limit_tiles`; tiles of points
  (`struct SmoothTile`: 256 x 16 bits and a log slot per pixel), `smooth_segment`, `smooth_set`.
- `src/core/settings.{c,h}`: `EaselSettings.smooth`, `[drawing] smooth` (true in any case; else false), `smooth=false` created.
- `src/ui/canvas.{c,h}`: `easel_canvas_set_smooth`; `current_figure` and `pending_figure` carry it; `append_cover_preview`, the smooth
  branch of `compose_pixels`, `curve_hit_row`; the Brush's press calls `easel_stroke_begin_smooth` with Smooth.
- `src/ui/window.c`: `win.smooth` (boolean, `smooth_changed`: ignored while stroking; sets the canvas and the options panel), start
  state from the settings; the options' scrolled holder now scrolls vertically (JOD, 2026-11-11, T008).
- `src/ui/tool-options.{c,h}`: `new_smooth_well` and `draw_edges` on the Line, shape and Brush pages; `easel_tool_options_set_smooth`
  (the Brush cells drawn by `easel_cover_footprint` at alpha points / 256, `redraw_areas`).
- `src/ui/preferences-dialog.c`: the switch after Fill style; `add_switch` returns its row (for the group's subtitle).
- Strings: "Sharp edges", "Smooth edges", "Smooth shapes and brushes" in 11 languages; one Help sentence in 12 pages.
- What grows: as planned; a Brush stroke holds 9 KB per touched 16 x 16 tile (`struct SmoothTile`), freed at its end.

| Scenario | Test |
|---|---|
| AS-1 | ui `/0039/as1_well_shared`, `/0039/as1_keyboard`, `/0039/as1_ignored_while_held`, `/0039-a11y/as1_names` + manual M1a |
| AS-2 | core `settings_smooth` + ui `/0039/as2_preferences` |
| AS-3 | core `as3_line_points`, `as3_level_line_is_sharp` |
| AS-4 | core `as4_ellipse_points`, `as4_rectangle_is_sharp` |
| AS-5 | core `as5_brush_union`, `as5_slash_no_gaps` (`test-stroke-smooth`) |
| AS-6 | core `as6_mix_transparent`, `as6_mix_white`, `as6_line_counts` |
| AS-7 | ui `/0039/as7_preview_is_put_down`, `/0039/as7_esc_and_undo`, `/0039/as7_pending_switch`, `/0039/as7_curve_hit`, `/0039/as7_brush_scroll_zoom`, `/0039/as7_lost_release` |
| AS-8 | `perf-0039` (real session) |
| AS-9 | core `as9_bow_tie`, `as9_rounded_and_curve` |
| AS-10 | ui `/0039/as10_no_memory` + core `smooth_apply_no_memory` |
| AS-11 | core `as11_brush_click` + ui `/0039/as11_cells` |

Measured (T007, T011; `perf-0039` in the real session, three runs, load 1.7 to 1.9, 2026-11-11), each move to the next painted
frame: the smooth Ellipse's preview at 1920 x 1080 median 16.8 to 16.9 ms at 100 % (worst 22.5) and 16.5 to 16.9 ms fitted (worst
49.1, one move, under three medians); at 8000 x 8000 fitted median 17.2 to 17.3 ms, the picture-sized moves 16.1 to 16.5 ms (budget
100 ms); the smooth Brush, circle of 7, at 1920 x 1080 median 16.6 to 16.7 ms (worst 20.2): each within a frame. The release of a
picture-sized smooth Ellipse: 14 ms at 1920 x 1080, 348 to 359 ms at 8000 x 8000 (no budget). The core alone: the 8000 x 8000
ellipse's cover rows of a fitted frame 8.7 to 9.5 ms, a Brush move 0.07 ms at most.

Oracle (engineering.md): core `cover_oracle`: 2000 random lines, ellipses, rounded rectangles, polygons, curves and brush strokes,
widths 1 to 5, each pixel's counts against a brute force written again in the test (every one of the 256 points tested on its own,
in `__int128`), and `mix_oracle`: the mix against the formula in doubles over a sweep of colors and counts.

Constitution: P7 as REQ-0039-09 (smoothing opt-in, P7.1); layering: all coverage in core, the canvas only reads runs; no threads,
no caches; Sharp untouched (every existing figure and stroke test must pass unchanged).

Risks:
- Settled with JOD, 2026-11-11 (spec changed): curve points to the nearest 1/32 pixel, since the exact ones (denominator N^3)
  overflow 128 bits on long curves; a polygon's sides without square tips at corners, round joins there, square ends only at an open
  polygon's two free ends.
- The ×16 reading of `member` and `extent`: `extent` estimates with doubles and then corrects by exact `member` calls; at ×16 the
  estimate's error stays under one sample, checked by `cover_oracle` in T002.
- The preview of a large smooth ellipse at 8000 x 8000: 16 sample rows per picture row in view; measured in the task right after
  the shapes' preview (T007) before anything relies on it.
- The cells' mix by cairo against REQ-0039-06: within 1 per channel; probed in T010's test.
- The third well on the shapes' page must fit the toolbox at the smallest window (REQ-0008-07): T008 renders it.

## Tasks
(Opus, 2026-11-11; JOD's go to the Plan, 2026-11-11.)

Group A, core coverage
- [x] T001 The mix (REQ-0039-06; AS-6) - done when core `mix_oracle` (a sweep of old colors, part colors and counts against the
  formula in doubles, P7.7's zero) and `as6_mix_transparent`, `as6_mix_white` (given the spec's counts 176, 49, 256, 0) fail first
  and pass, with mutants (divide by the rounded alpha; halves down; no 0x00000000)
  Result: new `test-cover` (`as6_mix_transparent`, `as6_mix_white` with AS-4's two-part pixel, `mix_oracle`: 4000 color triples
  with alphas 0, 1, 128 and 255 forced often, counts on a grid, against REQ-0039-06 in doubles, and the three ends) failed first on a stub
  returning the old color at `test-cover.c:77: CHECK failed: easel_cover_mix (0, 0x80FF0000, 176, 0, 0) == 0x58FF0000`; `easel_cover_mix`
  in 64-bit integers passes; 3 mutants (divided by the rounded alpha, halves down, alpha 0 keeping the color) killed by `core-cover`
- [x] T002 Rectangle, Rounded Rectangle and Ellipse by coverage (REQ-0039-04, REQ-0039-05 (a); AS-4, AS-9 rounded part) - done when
  core `as4_ellipse_points`, `as4_rectangle_is_sharp` and `cover_oracle` (these kinds, random boxes and widths) fail first and pass,
  with mutants (the box not times 16; the inner box not inset by 16w; an edge point not counted)
  Result: `as4_ellipse_points` (211/0, 28/228, 0/256 and a pixel outside), `as4_rectangle_is_sharp` (300 random boxes, every pixel
  whole in Sharp's part and no other) and `cover_oracle_boxes` (600 random Rectangles, Rounded Rectangles and Ellipses, widths 1 to 5,
  every pixel of a 44 x 44 area against a brute force of the 256 points written again in the test) failed first on stubs at
  `test-cover.c:181: CHECK failed: o == 211 && i == 0`; with `easel_cover_count` (a row's spans to runs by a sorted sweep) and
  `box_spans` (the Sharp `extent` with the box, radius and inset times 16) they pass. Decided (routine): a point on the inside's
  edge counts for the inside, as (a) says ("the inside: the rest") and Sharp does; REQ-0039-04's "outline only" is met in that no
  point counts twice. 4 mutants killed (radius from the fine box, inset in fine pixels, a span's last column, a partial count).
- [x] T003 Line by coverage (REQ-0039-05 (b); AS-3, AS-6) - done when core `as3_line_points`, `as3_level_line_is_sharp`, AS-6's
  counts through the line, and `cover_oracle` for lines (even and odd widths, start = end) fail first and pass, with mutants (no
  half shift for even w; no square ends; K rounded up)
  Result: `as3_line_points` (229, 144, 238, 10 and their colors), `as3_level_line_is_sharp` (level, upright and one-point Lines,
  widths 1 to 5, every pixel against Sharp's), `as6_line_counts` (176, 49, 256, 0) and `cover_oracle_lines` (1500 random Lines, one in
  ten a point, against the test's own 256-point test in 128 bits; 50 Lines near the coordinate limit equal to the same at the origin)
  failed first at `test-cover.c:291: CHECK failed: o == want[k][2] && i == 0`; with `easel_cover_band` and `easel_cover_disk` (exact
  column ranges by floor and ceiling division in 128 bits, the half width taken down to the whole number K) and `line_spans` they
  pass. 3 mutants killed (no 1/2 shift for an even width, no square ends, K rounded up).
- [x] T004 Curve and Polygon by coverage (REQ-0039-05 (c), (d), REQ-0039-07; AS-9) - done when core `as9_bow_tie`,
  `as9_rounded_and_curve` (the numbers from the new prototype, recorded in the test) and `cover_oracle` for curves (straight and
  bent) and polygons (open, closed, crossing) fail first and pass, with mutants (overlaps counted twice; the alternate rule as
  nonzero; points not taken to 1/32; square tips at corners)
  Result: `as9_bow_tie` (10 pixels: both side triangles 256 inside, the middle above and below empty, the crossing 256 outline,
  corners 233, (5, 5) 165/91, (8, 6) 92), `as9_rounded_and_curve` (the Rounded Rectangle (2, 2) to (21, 13), width 2, and the Curve
  start (2, 10), both bends (10, 2), end (18, 10), width 2, 10 pixels each; all numbers from the new prototype, `docs/reference/proto-0039/curve.py`
  and `rounded.py`, exact fractions) and `cover_oracle_paths` (400 random curves, one in eight straight, and 600 random polygons of
  1 to 7 corners, open and closed, a corner repeated in one in nine, widths 1 to 5, against the test's own reference: pieces, round
  joins, square ends, the alternate rule with crossings compared in 128 bits) failed first at `test-cover.c:577: CHECK failed` (pixel
  (4, 7): 0/0); with `smooth_prepare` (the pieces listed per picture row), `smooth_spans` (a sample row's pieces sorted and merged; the
  region's crossings sorted exactly, less the outline) and `curve_coord32` they pass. `curve_steps` factored out of `curve_path`
  (the Sharp curve unchanged: `core-figure-curve` passes; `--lint` 1403, 0 stale). The oracle first took 32 s; each pixel now tries only
  the pieces whose box meets it and a row is counted once (4 s for the file). 4 mutants killed (overlaps counted twice, every gap filled,
  points to 1/16, square tips at corners).
- [x] T005 The put-down (REQ-0039-04, REQ-0039-08; AS-4, AS-10 core part) - done when core `smooth_apply_styles` (Outline,
  Outline and fill, Fill; one undo step; pixels outside the picture untouched) and `smooth_apply_no_memory` (capped: nothing
  changes) fail first and pass, and every existing figure test passes unchanged
  Result: `smooth_apply_styles` (an Ellipse, a Line, a Rounded Rectangle and a closed Polygon, each partly outside a 24 x 24 picture
  of mixed opaque and transparent pixels, in the three styles: every pixel equals `easel_cover_mix` of its old color by the style's
  counts, the changed count matches, one undo step restores the picture) and `smooth_apply_no_memory` (a log capped at 100 pixels:
  NO_MEMORY, picture and history unchanged) failed first at `test-cover.c:778` (pixel (2, 3): FFBC092D, want FDB70A2D: the Sharp
  put-down); `easel_figure_apply_capped` now calls `sharp_apply` (the old loop, moved) or `cover_apply`, which mixes through the new
  `easel_figure_cover_color` (shared with the preview, T006). The whole core suite passes unchanged. Two Sharp mutants repointed
  (`--lint` 1405, 0 stale) and killed again; 2 new killed (Outline drawing the inside, the last column); one written first was
  equivalent (the old color mixed back at the inside's weight) and was replaced.

Group B, shapes in the window
- [x] T006 The choice and the shapes on the canvas (REQ-0039-02, REQ-0039-03 core part, REQ-0039-08, REQ-0039-09; AS-2 core part,
  AS-7 shapes, AS-10) - done when core `settings_smooth` (true, yes, missing; the created file) and ui `/0039/as7_preview_is_put_down`
  (rendered at 100 and 800 %, also scrolled), `/0039/as7_esc_and_undo`, `/0039/as7_pending_switch` (a waiting curve redrawn, Enter
  puts it down smooth), `/0039/as10_no_memory` and a curve hit on a partly covered pixel fail first and pass, through the action
  `win.smooth` (no well yet)
  Result: core `settings_smooth` (true and TRUE Smooth; yes, false, empty, a bad repeat, another section and none Sharp; the created
  file's `fill-style=outline\nsmooth=false\n`) failed first at `test-settings.c:395` (the field was never set); `core-settings-edit`'s
  pinned `[drawing]` text gains the line. ui `/0039/as7_preview_is_put_down` (an Ellipse, Outline and fill, width 2, Smooth, on a
  200 x 150 picture: the rendered preview equals a core put-down on a scratch picture pixel by pixel at 100 % and, after Undo and a
  zoom made before its own drag since 0036 refuses one while a button is held, at 800 % scrolled to the middle; the picture
  unchanged while held, equal after the release), `/0039/as7_esc_and_undo`, `/0039/as7_pending_switch` (a bent curve waiting with
  Sharp, Smooth chosen: the render changes, Enter puts down what was shown), `/0039/as7_curve_hit` (the 5-pixel line (10, 10) to
  (20, 20): a click at (24.5, 20.5) at 800 % keeps it waiting with Smooth, puts it down with Sharp) and `/0039/as10_no_memory` failed
  first at `smooth_state: 'v' should not be NULL` (no `win.smooth`). With the action (ignored while stroking, start state from the
  settings), `easel_canvas_set_smooth`, the figures' `smooth`, `append_cover_preview`, the smooth branch of `compose_pixels` and
  `curve_hit_row` (a buffer sized from the figure) they pass. 6 mutants killed by ui-0039 (preview drawn sharp, a partial pixel in the
  drawing color, put-down sharp, a waiting curve not following the choice, the hit sharp, a log failure ignored); `as10_no_memory`
  seen red alone by the last break made by hand. `as7_esc_and_undo`: characterization (cancel and Undo are Sharp's paths, unchanged).
  Two polygon mutants repointed (`--lint` 1411, 0 stale), killed. ui-0010, -0011, -0020, -0033 pass.
- [x] T007 Measure the shapes (REQ-0039-10; AS-8 shapes) - done when `perf-0039` prints an ellipse preview frame at 1920 x 1080 and a
  picture-sized ellipse at 8000 x 8000 (three runs each, load noted) and the numbers are in the Plan
  Result: `perf-0039`, three runs: the preview of a smooth Ellipse within a frame at 1920 x 1080 (median 16.8 to 16.9 ms at 100 %,
  16.5 to 16.9 fitted) and at 8000 x 8000 fitted (17.2 to 17.3 ms; the picture-sized moves 16.1 to 16.5 ms against 100 ms); numbers in
  the Plan.
- [x] T008 The well (REQ-0039-01, REQ-0039-02; AS-1) - done when ui `/0039/as1_well_shared`, `/0039/as1_keyboard`,
  `/0039/as1_ignored_while_held` and the toolbox's width at the smallest window (REQ-0008-07) fail first and pass; manual M1a: JOD
  looks at the Line, Ellipse and Brush panels (the cells, their tooltips, light and dark)
  Result: `/0039/as1_well_shared` (Line: Sharp and Smooth toggles side by side below the widths, in an `options-well`, Sharp chosen;
  Smooth clicked there is chosen on Rectangle, Ellipse, Curve, Polygon, Rounded Rectangle and Brush; none on Pencil, Airbrush, Eraser,
  Fill, Gradient, Text and Select; the toolbox's width and the window's minimum height the same for every tool; no step, no mark),
  `/0039/as1_keyboard` (Tab from "5 pixels" to Sharp, then Smooth; their activation, Space's path, waited for: GTK clicks a button
  after a short delay), `/0039/as1_ignored_while_held` and `/0039-a11y/as1_names` (a new suite `ui-0039-a11y` in the accessibility
  environment, as 0022's, since `GTK_A11Y=none` makes no names) failed first at `as1_well_shared: 'GTK_IS_TOGGLE_BUTTON (sharp) &&
  ...' should be TRUE`; with `new_smooth_well` (two cells bound to `win.smooth`, false and true, on the Line, shape and Brush pages)
  and `draw_edges` they pass. The minimum height: a shape tool raised the window's from 490 to 706 px before this feature (0006's
  edge case "the column scrolls" was never built) and to 752 with the well; JOD, 2026-11-11: make it scroll. The options' holder now
  scrolls vertically, the panel aligned to its top (`ui-0006`'s empty-panel check needed that). 4 mutants killed (the held choice
  taken, the cells not focusable, no well on the Brush, the holder never scrolling). ui-0006, -0007, -0008, -0013, -0017, -0023,
  -0030 (and its scale 2), -0032 pass.
  M1a (JOD, 2026-11-11): the Line, Ellipse and Brush panels, the cells and their tooltips, light and dark, the short window
  scrolling, and a smooth ellipse at 100 and 800 %: "all is ok".

Group C, the Brush
- [x] T009 The smooth stroke (REQ-0039-05 (e), REQ-0039-07; AS-5, AS-11 core) - done when core `as5_brush_union`,
  `as5_slash_no_gaps`, `as11_brush_click` and `cover_oracle` for strokes (all four shapes, random paths) fail first and pass, plus
  `smooth_stroke_cancel` (cancel restores) and a capped tile allocation (pixels left undrawn, no crash), with mutants (a pixel mixed
  from its value in the stroke, not before; the circle of 1 as a true circle; no sweep between positions)
  Result: new `test-stroke-smooth`: `as11_brush_click` (the circle of 1 paints one whole pixel; the circle of 4's counts 81, 235, 256
  and #151515 for an edge, `easel_cover_footprint` alike), `as5_brush_union` (the circle of 7 dragged (5, 5) to (25, 5) to (5, 6),
  every pixel against the reference; the turn's (26, 2) holds the union's 215 points, not more; the top row whole but for its six
  end pixels, from the prototype), `as5_slash_no_gaps` ("/" of 8 from (10, 10) to (30, 30): the reference, and (k, k), (k + 1, k)
  whole from 12 to 28), `cover_oracle_strokes` (180 strokes of 1 to 4 positions, nine presets, a position repeated in one in seven,
  over a picture of opaque and transparent colors, every pixel against the test's own sweep: a point is in it when some t in [0, 1]
  puts it in the shape moved by t v, solved in fractions; the circle as the distance to the segment), `smooth_stroke_cancel_and_undo`
  and `smooth_stroke_tiles_capped` (a test hook, `easel_stroke_limit_tiles`: the pixels of a tile that cannot be had stay) failed
  first at `test-stroke-smooth.c:180: CHECK failed: easel_cover_footprint (one, 0, 0) == 256` on stubs drawing Sharp; with
  `easel_cover_sweep` (a capsule for a circle; for a polygon the shape at both ends and each edge swept, the row's least and greatest
  crossing exact in 128 bits) and the stroke's tiles of points (`smooth_segment`, `smooth_set`: a pixel logged once with its value
  before the stroke, its new value updated) they pass. My first check of `as5_brush_union` assumed a blended top row; the prototype
  showed the circle's top lies on a pixel edge, and the check was corrected, not the code. 6 mutants killed (mixed from the value in
  the stroke, the circle of 1 round, no sweep, edges not swept, "/" as "\\", the tile limit unused). Core and io 50 of 50.
- [x] T010 The Brush in the window and its cells (REQ-0039-08, REQ-0039-11; AS-7 brush, AS-11 ui) - done when ui
  `/0039/as7_brush_scroll_zoom`, `/0039/as7_lost_release` and `/0039/as11_cells` (the circle of 4's cell against the mix, 1 per
  channel; back to sharp) fail first and pass
  Result: ui `/0039/as7_brush_scroll_zoom` (a 2000 x 1500 picture, the circle of 7 chosen by its cell, Smooth: a drag (40, 40),
  (200, 90), the view scrolled by 80 and 120, a zoom to 200 % asked for and refused while held, then (60, 160): the picture equals the
  core's stroke through the three positions; one Undo), `/0039/as7_lost_release` (the circle of 4 dragged, the release lost: the
  coverage last shown stays, one step) and `/0039/as11_cells` (the circle of 4's cell rendered: with Smooth each pixel within 1 a
  channel of the mix of the cell's background and the text color by 81, 235 and 256 points, a corner not left out; with Sharp
  `.##.` `####` `####` `.##.` again) failed first at `as7_brush_scroll_zoom: assertion failed (memcmp (...) == 0): (90 == 0)` (the
  Brush painted Sharp). With the Brush's press calling `easel_stroke_begin_smooth` when Smooth is chosen, the Brush cells' data
  pointing at the options' choice (`easel_tool_options_set_smooth`, called by `win.smooth` and at start-up) and drawn at alpha
  points / 256 they pass. `as11_cells` then failed on its own: the cells did not draw again, since GTK keeps a child's drawing until
  that child asks; `redraw_areas` asks each cell's drawing area. 3 mutants killed (the Brush Sharp with Smooth, the cells sharp, the
  cells not redrawn). ui-0006, -0020, -0030 (and its scale 2) pass.
- [x] T011 Measure the Brush (REQ-0039-10; AS-8 brush) - done when `perf-0039` prints the Brush motion with the circle of 7 at
  1920 x 1080 (three runs) and the numbers are in the Plan
  Result: `perf-0039`, three runs: a smooth Brush move with the circle of 7 at 1920 x 1080, median 16.6 to 16.7 ms, worst 20.2 ms, within
  a frame; numbers in the Plan.

Group D, settings and words
- [x] T012 Preferences (REQ-0039-03; AS-2) - done when ui `/0039/as2_preferences` (Apply writes `smooth=true` and leaves the panel's
  choice; Cancel writes nothing; the panel's choice writes nothing) fails first and passes
  Result: ui `/0039/as2_preferences` (`smooth=true` starts Smooth, `smooth=yes` and no key Sharp; the panel's choice writes nothing;
  the switch row "Smooth shapes and brushes", with the group's subtitle "Takes effect the next time Easel starts.", right after Fill
  style and before Magnifier level; on and Cancel writes nothing; on and Apply writes `smooth=true` and leaves the panel at Sharp)
  failed first at `pref_row: 'row' should not be NULL` (its start-up part already passed, from T006); `add_switch` now returns its row
  so the new one gets the group's subtitle. 1 mutant killed (another key). ui-0020, -0027 pass.
- [x] T013 Strings and Help (REQ-0039-01, REQ-0039-03) - done when the three strings are in the 11 languages, the mnemonic and
  translation tests pass, and Help's Line, Shapes and Brush pages say what Smooth does
  Result: "Sharp edges", "Smooth edges" and "Smooth shapes and brushes" in the 11 catalogs (added by hand, as the catalogs are kept:
  a full merge rewrapped every file and guessed a fuzzy "Scharf", so it was put back), the template regenerated (the three strings
  and moved line numbers); Help's first tools paragraph ends with one sentence on Sharp and Smooth and Preferences, in the 12 pages,
  with each language's tool names from its catalog; `docs/reference/translations-to-review.md` lists them for JOD. `translations`
  seen red by hand (the German "Smooth edges" taken out: `po/de.po: missing: 'Smooth edges'`), green again restored; ui-0021,
  -0023 pass. No labels, so no mnemonic to check.
- [x] T014 Gate - done when the full suite and the scale 2 run pass through `tools/isolated.sh`, `tools/mutate.sh --core` is clean or
  its survivors are in Check, and manual M1: JOD draws with Smooth (a diagonal Line, an Ellipse with fill, a Curve, a Polygon, a
  Brush stroke over itself, at 100 % and 800 %), turns the switch in Preferences and restarts
  Result: the gate's first full run 133 of 135: `ui-0025`, `ui-0025-vulkan` and `ui-0025-scale2` failed at `set_area: ... (424 ==
  366)`, T008's scrolling holder having raised every tool's least window height by its scrollbar's 58 pixels (Check F1); after the fix
  `meson test` (isolated) 135 of 135 and the scale 2 list 6 of 6, 0 warnings; `tools/mutate.sh --core` 544 mutants, 541 killed, 2
  survived (both of 0038's check, equivalent), 1 accepted (204 s). M1 (JOD, 2026-11-11): a diagonal Line, an Ellipse with fill, a Curve,
  a Polygon and a Brush stroke over itself with Smooth at 100 and 800 %, the Preferences switch and a restart: "ok".

## Check
- Date and commit: 2026-11-11, on `0512923` with the uncommitted work of 0039.
- Tests: `meson test` (isolated) 133 of 135 at the gate, 135 of 135 after the check's fixes; the scale 2 list 6 of 6; the cases of
  this feature: core `test-cover` (16 cases), `test-stroke-smooth` (6), `settings_smooth`; ui `/0039/*` (14) and `/0039-a11y/as1_names`.
- Mutants: `tools/mutate.sh --core` 544 mutants: 541 killed, 0 by crash, 0 inconclusive, 2 survived, 1 accepted (204 s); the 0039 ui
  mutants run by REQ id during the tasks, all killed. `--lint` 1425 mutants, 0 stale.
- Survivor, accepted (both of 0038's check, unchanged here): REQ-0037-02 "a fully transparent pixel comes back with a kept piece" and
  REQ-0038-03 "the seam changes a stroke's cell", equivalent. REQ-0008-07 "the Magnifier's level labels are smaller" survives the
  REQ-0008-07 runs as before 0039 (the beta gate's equivalent).
- Warnings: 0. Lint and `--names 0039` clean.
- Benchmarks: `perf-0039`, three runs (Plan): each preview and Brush move within a frame; 8000 x 8000's picture-sized ellipse in 17 ms.
- Scenarios: AS-1 `as1_well_shared`, `as1_keyboard`, `as1_ignored_while_held`, `as1_names` + M1a; AS-2 `settings_smooth` +
  `as2_preferences`; AS-3 `as3_line_points`, `as3_level_line_is_sharp`; AS-4 `as4_ellipse_points`, `as4_rectangle_is_sharp`; AS-5
  `as5_brush_union`, `as5_slash_no_gaps`; AS-6 `as6_mix_*`, `as6_line_counts`; AS-7 `as7_preview_is_put_down`, `as7_esc_and_undo` (C:
  Sharp's cancel and Undo paths), `as7_pending_switch`, `as7_curve_hit`, `as7_brush_scroll_zoom`, `as7_lost_release`; AS-8 `perf-0039`;
  AS-9 `as9_bow_tie`, `as9_rounded_and_curve`; AS-10 `as10_no_memory` (broken by hand) + `smooth_apply_no_memory`; AS-11
  `as11_brush_click` + `as11_cells`. Every task has its `Result:`; the one characterization is AS-7's Esc and Undo.

Findings (a review agent read the diff and the spec blind; each finding checked in the code before it was settled):
- F1 (fixed): T008's vertical scrolling of the options' holder gave every tool 58 more pixels of least window height (the holder's
  scrollbar), seen by `ui-0025` at the gate; the holder without a scrollbar left the options no room to scroll in. Now the toolbox's
  whole column scrolls (0006's "the column of both scrolls"); `as1_well_shared` compares every tool with Pan and checks the wheel
  scrolls the column in the shortest window; two mutants (never scrolls; the 58 pixels, by `ui-0025`).
- F2 (fixed): a smooth polygon with no corner (a pending one before its first) crashed at prepare (`n - 1` wrapped); seen as a
  segmentation fault of `smooth_polygon_no_corner` before the fix, which gives it no pieces. No mutant: one could only crash.
- F3 (fixed, not of the spec): `tools/release.sh` could replace a published release's files with a build of another commit when the
  version was not bumped; it now refuses a tag on another commit, a branch other than main and missing notes
  (`docs/release-notes-<version>.md`). The VS Code commit task now passes the typed text as environment, not as shell words.
- F4 (spec changed, JOD 2026-11-11): REQ-0039-03 said Smooth for `true`; the code takes `true` in any case, as every value of the
  file (`style=DARK`, REQ-0020-11); the requirement now says so.
- F5 (known gap): a smooth Brush stroke holds 36 bytes per touched pixel while it is drawn (planned, freed at its end); a stroke over
  all of an 8000 x 8000 picture would need about 2.3 GB, and a tile that cannot be had leaves its pixels undrawn, as Sharp does.
- F6 (known gap, manual): with "Always show scrollbars", the toolbox column's scrollbar may take width in a short window; not seen
  with GNOME's overlay scrollbars.
- Decided during the build (routine): a point on the inside's edge counts for the inside (REQ-0039-05 (a)); the cells are mixed by
  cairo, within 1 a channel of REQ-0039-06.

Known gaps: F5 and F6 (also in `docs/phase2-plan.md`).
