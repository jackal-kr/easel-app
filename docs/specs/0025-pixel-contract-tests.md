# 0025 Pixel contract tests

Status: accepted
Depends on: 0001, 0004, 0006, 0007, 0009, 0010, 0012, 0013, 0014, 0015, 0016, 0017, 0018 (the features whose pixels the contract covers)

## Why
Constitution P7 (ADR-0003) says what a pixel is in Easel: whole, exact, magnified by the view, the same wherever and however it is
made, and never touched by an overlay. Two bugs that broke it (the same letter drawn differently in one line, the grid hidden by the text
box) passed every test, because no test stated the rule across features. This feature gives each point of P7 a requirement and a
test that can fail, where today there is none. It adds no behavior for the user: it states what the app already promises and makes it
checkable. Words: a logical pixel is the unit the window is laid out in (a screen of scale 2 has 2 × 2 device pixels in one); a
device pixel is one dot of the screen. P7.2 is stated from 100 % up, where a picture pixel is at least one device pixel; below
100 % the view shrinks the picture and several picture pixels share a device pixel.

## Requirements
- REQ-0025-01 (P7.1) Every tool shall write whole picture pixels with exact colors: a pixel written by a tool is one color in all four
  channels, never a blend of two positions, at any zoom. The Airbrush is the exception: the choice of the pixels it writes is
  random (the colors it writes are exact). (Smoothing, when a later feature adds it, is opt-in and still writes whole pixels.) The drawing
  features' own requirements already say this tool by tool; this feature adds no new rule for them, only the statement and its check.
- REQ-0025-02 (P7.2) While the zoom is 100 % or more, the view shall draw one picture pixel as one square of zoom/100 × the screen scale
  device pixels on a side, at every scroll position and window size, with no gap and no overlap between neighbouring squares. Where
  that is not a whole number of device pixels (a fractional zoom, or a fractional screen scale), the edges of the square of picture
  pixel k shall be at round(k × zoom/100 × scale) device pixels from the picture's origin, halves rounded up, so that neighbouring
  squares share an edge and a square's size depends only on its place in the picture, not on the scroll position. (A fractional screen
  scale is not tested yet: it waits for a real screen, see Out of scope.)
- REQ-0025-03 (P7.3) A preview, a floating selection and a text box shall show exactly the pixels the put-down writes, made by the same
  rules, in Opaque and Transparent mode, at any zoom and scroll position (the overlays of REQ-0025-05 excepted).
- REQ-0025-04 (P7.4) The same input shall give the same pixels: a figure's pixels depend only on its integer coordinates, tool, width,
  fill style and colors, not on the zoom, the scroll position, the window size or the screen scale; and a letter shall have the same
  pixels wherever it falls in a line.
- REQ-0025-05 (P7.5) An overlay (the grid, the tint, the outline, the handles, the caret, the Gradient's guide line, the Magnifier's
  box) shall not change a picture pixel, the list of undo steps, the modified mark or the file saved; the tint and the grid let the
  pixels show through, and the outline, the handles and the caret cover pixels only with thin lines and small marks (the outline at
  most 2 logical pixels thick, REQ-0014-17; a handle 5 × 5, REQ-0015-01; the caret at most 2 wide, REQ-0017-05).
- REQ-0025-06 (P7.6) A pointer position shall map to an integer picture pixel, so that a drag from the picture pixel A to the picture
  pixel B makes the same figure at any zoom, wherever inside A and B the pointer is, and however many pointer events the drag has. A
  pointer exactly on the border of two pixels maps to the pixel to its right or below; a position left of or above the first pixel
  (-0.5, say) maps to the pixel -1, not 0, and the pixels written are clipped to the picture (REQ-0010-21).

## Scenarios
- AS-1 (REQ-0025-02) Given a 40 × 30 picture in which every pixel has a color different from its four neighbours, When the canvas is drawn, for every combination, at 100, 200, 300, 400, 600 and 800 % in canvas areas of 1390 × 853, 1001 × 777 and 300 × 200 logical pixels, scrolled to the start, by 1 logical pixel, by 7 logical pixels, to the middle and to the far corner (where the picture is larger than the area; else it stays at its place), Then each visible picture pixel (x, y) is drawn on every device pixel of one square of zoom/100 device pixels on a side and on no other, the squares of neighbouring pixels touch with no gap and no overlap, and moving the scroll by one logical pixel moves every square by one logical pixel and changes no square's size; Given an 8000 × 8000 picture at 800 % scrolled to its far corner, Then the pixel (7999, 7999) is drawn as one square of the same size | Verified by: automated (ui, a rendered window)
- AS-1b (REQ-0025-02) Given the same as AS-1 on a screen of scale 2, Then each picture pixel is drawn as a square of zoom/100 × 2 device pixels on a side | Verified by: automated (ui, a rendered window, on a session of scale 2)
- AS-1c (REQ-0025-02) Given a 290 × 100 picture in which every pixel has a color different from its neighbours, When Fit to Window is chosen in a canvas area 400 logical pixels wide (the zoom is 129 %, the largest whole percent at which the picture and its two 12-pixel margins fit), the area is then made 300 wide and the zoom is kept at 129 % (Fit to Window chosen in 300 would give 95 %, where REQ-0025-02 does not apply: that is why the zoom is kept) and the canvas is drawn scrolled to the start, by 1 logical pixel, by 7 logical pixels and to the far end, Then the edges of the square of each picture pixel k are at round(k × 1.29) device pixels from the picture's origin, halves rounded up, neighbouring squares share an edge with no gap and no overlap, and the size of a square does not change with the scroll position (it is 1 or 2 device pixels by its place) | Verified by: automated (ui, a rendered window)
- AS-2 (REQ-0025-04, REQ-0025-06) Given a 200 × 150 picture with the grid off, When a drag from the picture pixel (20, 30) to (150, 100) is made with the Pencil, the Brush, the Eraser, the Line (width 3), the Rectangle (width 2, filled), the Ellipse (width 4) and the Gradient, a Curve (the end points (20, 30) and (150, 100) and the control points (60, 120) and (110, 10)), a Polygon (the corners (20, 30), (150, 100), (30, 140), closed by a click on the first corner), and a Fill is clicked at (60, 60) inside a filled Rectangle drawn first from (40, 40) to (90, 90), once at 100 %, once at 400 % and once at 800 %, with the pointer at the centre of each pixel, and once at 25 % and once at 50 % with each pointer position at the left and top edge of its pixel (the pixel's coordinates × zoom/100, which maps back to the same pixel), Then the picture's pixels are identical to those of the 100 % run in each of the other runs (all 30 000 of them) | Verified by: automated (ui)
- AS-2b (REQ-0025-06) Given the drag of AS-2 with the Line, the Rectangle and the Ellipse, When the pointer is at 0.1 and then at 0.9 of a pixel from the pixel's left and top edge, at 400 % and at 800 %, and when the same drag is made in one pointer jump and in 100 small steps, Then the picture's pixels are identical to those of AS-2's 100 % run; and a drag whose pointer positions are all inside the two end pixels gives the same pixels at 400 % and at 800 %; and a pointer exactly on the border of two pixels (the left edge of pixel 21, say) is the pixel to its right, and a Line that starts at the position (-0.5, 30.5), which is the pixel (-1, 30), gives the pixels of a Line from the pixel (-1, 30) clipped to the picture, not those of a Line from the pixel (0, 30) | Verified by: automated (ui)
- AS-3 (REQ-0025-05) Given a picture with every pixel different, When the grid is on at 800 %, a rectangle is marked with Select (tint, outline, eight handles), a polygon is being drawn (its pending preview is not written to the picture), a text box is open with the caret blinking, the Magnifier's box shows, and a Gradient guide line waits, and the canvas is drawn, Then the picture's pixels are exactly what they were, the list of undo steps and the modified mark are as they were, and a file saved then has the same pixels and no grid, tint, outline, handle or caret | Verified by: automated (ui)
- AS-3b (REQ-0025-05) Given the grid on at 800 %, When the canvas is drawn with a tinted rectangle selection, a floating selection and an open text box, in Opaque and in Transparent mode, Then a device pixel on a grid line inside each shows the grid's color over the content, a device pixel between two lines shows the content (under the tint, where there is one) and not the tint alone, and the outline is at most 2 logical pixels thick and a handle 5 × 5 | Verified by: automated (ui, a rendered window)
- AS-4 (REQ-0025-03) Given a text box with black "Hi" and a second run in red italic, and a floating selection after Rotate 90°, Flip Horizontal, Invert Colors and a stretch to 2 pixels wider, each in Opaque and in Transparent mode, at 100 % and 800 %, also scrolled by 7 logical pixels, When the canvas is drawn with the grid, the tint, the outline, the handles and the caret not drawn, Then every pixel of the box or the selection is the color the put-down then writes at that pixel (compared with the picture after the put-down) and the pixels outside it are the picture's | Verified by: automated (ui, a rendered window)
- AS-4b (REQ-0025-03) Given a Line, a Rectangle (outline and filled), an Ellipse, a Curve (after its first, second and third click) and a Polygon (pending, with two sides drawn), at 100 % and at 800 %, When the canvas is drawn while the drag or the pending shape is shown, Then its preview is the same pixels as the picture after the shape is released or closed; and when the drag is cancelled with Esc, Then no pixel of the picture is changed | Verified by: automated (ui, a rendered window)
- AS-5 (REQ-0025-01) Given the Pencil, the Brush, the Eraser (with the background color #00000000, #FF000080 and an opaque one), Fill, the Line, the Rectangle, the Ellipse, the Curve, the Polygon and the Gradient, each with each of the colors #00000000, #FF000080 and #336699FF, and a paste, a text put-down and a transform, When each is used, Then every pixel it changes has exactly the color its rule gives (the given color; for the Eraser the background color; for a transform and a paste the colors the pixels already had, moved or inverted), every other pixel keeps its value, and no pixel is a blend of the given color and the one under it (the Airbrush is not in this list: REQ-0025-01) | Verified by: automated (core)
- AS-6 (REQ-0025-04) Given a text box with the same letter ("h") typed ten times and a second line "kkkkkk", in Ubuntu Sans, DejaVu Sans and DejaVu Serif (the families the test session's `fc-list` shows on 2026-10-28; the test reads which of them the session has and uses those), regular, bold and italic, at 8, 12 and 24 points, When it is put down and the picture is read, at 100 % and again after the same text is made at 800 % with the view scrolled, Then every "h" has the same pixels as every other "h" and every "k" the same as every other "k", and the pixels do not depend on the zoom or the scroll | Verified by: automated (ui)

## Edge cases
- A picture smaller than the canvas area: its squares are as in AS-1; the margin and the shadow around it (REQ-0009-01) are not picture pixels and are not counted.
- A zoom below 100 % (Fit to Window): REQ-0025-02 does not apply, REQ-0025-04 and REQ-0025-06 do (AS-2's 25 % and 50 % runs). A zoom that is not a multiple of 100 % (Fit to Window's 129 % for a 290-pixel picture in 400, say) follows REQ-0025-02's rounding rule: AS-1c.
- A very large picture (8000 × 8000) at 800 % scrolled to its far corner: in AS-1.
- A window resize or a zoom change while a preview, a float or a text box is shown: the shown pixels stay those the put-down writes; the cases are those of 0015 (zoom changes during a stretch) and 0017 (the view changing with a box open), and are not repeated here.
- Undo and Redo of a put-down give the same pixels again (a case of REQ-0025-04, checked in AS-5's transforms and in the history tests of 0002).

## Out of scope
- Smoothing (Phase 2): when it exists it gets its own scenarios against REQ-0025-01 to -06.
- Colour choice and blending rules (ADR-0002 and each feature's spec); the PNG round trip (REQ-0001-30).
- Performance (each feature's own benchmark).
- A fractional screen scale: deferred by JOD (2026-10-28) until a real screen with one is available; REQ-0025-02's rounding rule for it is then untested here, and not simulated.
- The Airbrush's pixels at a given input (random; its seeded rule is tested in its own feature).

## Open questions
None.

## Plan
(draft, to be completed by /sdd-build after the spec is accepted)
- **Order (JOD, 2026-10-28):** /sdd-check 0018, then the 0017 text and tint task, then this feature: the text-box cases of AS-3 and AS-4 fail
  until the 0017 fix (REQ-0017-28, the grid and the tint over the box) lands.
- **AS-5, the test each tool has today and where a new one is needed** (a color of any alpha, exact; read from the tests on 2026-10-28):
  | Tool | Existing test | New test needed |
  |---|---|---|
  | Pencil, Brush | core `test-stroke`: `as6_transparent_stroke_exact` (#00000000 and #FF000080) | no |
  | Eraser | core `test-stroke`: `as7_eraser` (background color or transparency) | no |
  | Fill | core `test-fill` (colors with alpha, 9 cases) | no |
  | Gradient | core `test-gradient` (colors with alpha, 23 cases) | no |
  | Line, Rectangle, Ellipse, Curve, Polygon | core `test-figure*`, `test-figure-apply` have no color with alpha; ui `as4_preview_equals_drawing` compares preview and drawing with a transparent color, not the pixel value | **yes**: a core test per shape that a color with alpha is written exactly (#00000000 and #FF000080) |
  | Paste | core `test-selection` (put-down of pixels with alpha, 34 cases) | no |
  | Text put-down | core `test-text-put` (one case with alpha below 255) | **yes**: a foreground of #00000000 and one of #FF000080, written as they are (REQ-0017-16) |
  | Transform | core `test-transform` (invert keeps alpha, 8 cases) | no |
  | PNG round trip | io `test-png`: `as24_png_round_trip`, `as27_transparent_round_trip` | no |
  | Airbrush | core `test-spray` (seeded; the ui seed is the clock, so no ui test can fix its pixels) | the exception of REQ-0025-01 |
- **One manual break per scenario** (each test is written first, then made to fail by the break, the failure shown, the break reverted;
  the failing runs are shown when the tests exist, they cannot be shown before): AS-1: round the cell origin differently (the tile origin
  in the canvas drawing); AS-1b: the same on the session of scale 2; AS-2: scale the pointer by the zoom before it is mapped to a pixel;
  AS-1c: floor instead of round for the edge of a square; AS-2b: truncate the pointer toward zero instead of flooring it; AS-3: draw the grid into the picture's pixels; AS-3b: draw the text box's
  pixels over the grid (the 0017 bug) or the tint alone over the content; AS-4: skip a step of the put-down in the float's drawing;
  AS-4b: skip a step in the preview (draw the preview one step short); AS-5: blend the given color with the pixel under it in one shape's
  write; AS-6: the old text settings (gray antialiasing, no whole-pixel positions) put back.
- **AS-1b, scale 2:** the isolated session can run a screen of scale 2: probed on 2026-10-28, a headless GNOME Shell with its virtual
  monitor set to scale 2.0 through `org.gnome.Mutter.DisplayConfig.ApplyMonitorsConfig` (method 1, one logical monitor, scale 2.0)
  reports the monitor at 960 × 600 logical, scale 2.00, and a GTK window's surface scale 2.00. A `monitors.xml` in the throwaway
  `HOME` was not honored. A fractional scale was not tried and is not simulated.

## Tasks
(added by /sdd-build after the plan is agreed: `- [ ] T001 title (REQ-..., AS-...) - done when ...`)
(filled in when the task is ticked, a line under it: `  Result: ...`. A code task: the test that failed first and its message,
or `characterization: <why there was no red run>`. A manual step: one line of what JOD saw.)

## Check
(written by /sdd-check as its last step, about ten lines)
- Date and commit: YYYY-MM-DD, `abc1234`
- Tests: `meson test` N of N pass (the cases of this feature: M)
- Mutants: K killed, C killed by crash (test gap), S survived, as `tools/mutate.sh` reports (seconds)
- Warnings: 0
- Scenarios: each AS with its test, marked C for a characterization with how it was proven (the mutant or the manual break that killed it)
- Findings, each with how it was settled (fixed in T0NN, decided by the owner, left open and why)
