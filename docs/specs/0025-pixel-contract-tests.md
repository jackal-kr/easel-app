# 0025 Pixel contract tests

Status: implemented
Reviewed: spec-reviewer, 2026-10-28 (changes after the review shown to JOD old/new, not reviewed again)
Depends on: 0001, 0004, 0006, 0007, 0009, 0010, 0012, 0013, 0014, 0015, 0016, 0017, 0018 (the features whose pixels the contract covers)

## Why
Constitution P7 (ADR-0003) says what a pixel is in Easel: whole, exact, magnified by the view, the same wherever and however it is
made, and never touched by an overlay. Two bugs that broke it (the same letter drawn differently in one line, the grid hidden by the text
box) passed every test, because no test stated the rule across features. This feature gives each point of P7 a requirement and a
test that can fail, where today there is none. It adds tests and fixes the places they find wrong: at a fractional zoom (129 %, say) the edges of the squares move with the
scroll, the grid's lines can fall inside a square, and a pointer can map to a pixel other than the one drawn under it. Beyond those
fixes it states what the app already promises and makes it checkable. Words: a logical pixel is the unit the window is laid out in (a screen of scale 2 has 2 × 2 device pixels in one); a
device pixel is one dot of the screen. P7.2 is stated from 100 % up, where a picture pixel is at least one device pixel; below
100 % the view shrinks the picture and several picture pixels share a device pixel.

## Requirements
- REQ-0025-01 (P7.1) Every tool shall write whole picture pixels with exact colors: a pixel written by a tool is one color in all four
  channels, never a blend of two positions, at any zoom. The Airbrush is the exception: the choice of the pixels it writes is
  random (the colors it writes are exact). (Smoothing, when a later feature adds it, is opt-in and still writes whole pixels.) The drawing
  features' own requirements already say this tool by tool; this feature adds no new rule for them, only the statement and its check.
- REQ-0025-02 (Pointer note, change of 2026-11-13: a fractional screen scale is now in scope and tested, REQ-0025-07; this requirement's closing sentence on fractional scales is superseded.) (P7.2) While the zoom is 100 % or more, the view shall draw one picture pixel as one square of zoom/100
  device pixels on a side, on any screen, at every scroll position and window size, with no gap and no overlap between neighbouring squares. Where
  that is not a whole number of device pixels (a fractional zoom), each square shall be the floor or the ceiling of zoom/100 device pixels on a side, neighbouring squares shall share
  an edge with no gap and no overlap, the edge of picture pixel k shall have one position that depends only on k and the zoom (not on the screen's scale, the scroll, the window
  size or the picture's size), and it shall be at most half a device pixel from k × zoom/100. A scroll of N device pixels moves every edge by exactly N. Which way
  an edge exactly halfway between two device pixels goes is not specified. (A fractional screen
  scale is not tested yet: it waits for a real screen, see Out of scope.)
- REQ-0025-03 (P7.3) A preview, a floating selection and a text box shall show exactly the pixels the put-down writes (a pending Polygon shows the sides drawn so far; the closing side is written only when it closes, so it is not part of this comparison), made by the same
  rules, in Opaque and Transparent mode, at any zoom and scroll position (the overlays of REQ-0025-05 excepted). The one exception: while
  the view shows more than 3000 picture rows (far below 100 %), a shape's preview draws one row in every few, so that a frame costs what
  the view costs (REQ-0010-05); the put-down writes every row.
- REQ-0025-04 (P7.4) The same input shall give the same pixels: a figure's pixels depend only on its integer coordinates, tool, width,
  fill style and colors, not on the zoom, the scroll position, the window size or the screen scale; and a letter shall have the same
  pixels wherever it falls in a line.
- REQ-0025-05 (Pointer note, Accessibility, 2026-11-03: the keyboard marker is such an overlay, REQ-0022-05.) (P7.5) An overlay (the grid, the tint, the outline, the handles, the caret, the Gradient's guide line, the Magnifier's
  box) shall not change a picture pixel, the list of undo steps, the modified mark or the file saved; the tint and the grid let the
  pixels show through, and the outline, the handles and the caret cover pixels only with thin lines and small marks (the outline at
  most 2 logical pixels thick, REQ-0014-17; a handle 5 × 5, REQ-0015-01; the caret at most 2 wide, REQ-0017-05).
- REQ-0025-06 (Pointer note, One view path below 100 %, 2026-11-08: below 100 % a pointer maps to the pixel its device pixel shows, REQ-0031-03.) (P7.6) A pointer position shall map to an integer picture pixel, so that a drag from the picture pixel A to the picture
  pixel B makes the same figure at any zoom, wherever inside A and B the pointer is, and however many pointer events the drag has. The picture pixel is the
  one whose square is drawn under the pointer's device pixel, by the same rule that places the squares, so that what is pointed at and what is
  drawn cannot differ; where zoom/100 is a whole number, and below 100 % the pixel its device pixel shows (REQ-0031-03; before 2026-11-08, the position divided by the pixel's size),
  rounded down. A position exactly on the border of two squares belongs to the one starting there (to its right or below); a negative position
  (-0.5, say) maps to a negative pixel (-1, not 0), and the pixels written are clipped to the picture (REQ-0010-21).
- REQ-0025-07 (Change of 2026-11-13; JOD, 2026-11-13, from a 1920 × 1200 screen at 125 %: at 100 % rectangles' and a star's
  outlines dotted or missing and a faint line across the picture, all there at 200 %.) On a screen whose scale is not a whole number
  (1.25 and 1.5, say) the view shall meet REQ-0025-02 as on a screen of a whole scale. While the zoom is 100 % or more:
  - the picture's top-left corner lies on a whole device pixel at every scroll position, window size and picture size, so a scroll moves
    the squares by whole device pixels (one logical pixel of scroll at 1.25 moves them by 1 or 2; JOD, 2026-11-13), and REQ-0025-02's
    scroll rule holds for that whole-pixel movement;
  - the picture is drawn by nearest sampling: every picture row and column whose square lies wholly inside the visible area shows on at
    least one device row or column, and no device pixel shows a blend of a picture pixel with another picture pixel, the margin, the
    shadow or the checkerboard;
  - at one zoom, no picture pixel's square is more than one device pixel wider or higher than another's (REQ-0025-02's floor or ceiling);
    at exactly 100 % each picture pixel is exactly one device pixel (1:1);
  - the grid stays exact as in AS-1d (each line on the device column or row where square 4k starts);
  - a pointer maps to the picture pixel drawn under its device pixel, as REQ-0025-06 says.
  Below 100 % on such a screen the view's look is as before this change and not specified (not exact); REQ-0025-04 holds there, and a
  pointer maps by REQ-0031-03's formula (its device position is its logical position times the scale). The picture's pixels, the undo steps
  and a saved file never depend on the screen scale (REQ-0025-04, -05).


## Scenarios
- AS-1 (REQ-0025-02) Given a 200 × 150 picture in which every pixel has a color different from its four neighbours, When the canvas is drawn, for every combination, at 100, 200, 300, 400, 600, 800 and 1600 % in canvas areas of 1390 × 853, 1001 × 777 and 514 × 366 logical pixels (the smallest the window allows with its panels showing), scrolled to the start, by 1 logical pixel, by 7 logical pixels, to the middle and to the far corner (where the picture is larger than the area; else it stays at its place), Then each visible picture pixel (x, y) is drawn on every device pixel of one square of zoom/100 device pixels on a side and on no other, the squares of neighbouring pixels touch with no gap and no overlap, and moving the scroll by one logical pixel moves every square by one logical pixel and changes no square's size; Given an 8000 × 8000 picture at 800 % and at 1600 % scrolled to its far corner, Then the pixel (7999, 7999) is drawn as one square of the same size | Verified by: automated (ui, a rendered window)
- AS-1b (REQ-0025-02) Given the same as AS-1 on a screen of scale 2, Then each picture pixel is drawn as a square of zoom/100 device pixels on a side, as on a screen of scale 1 | Verified by: automated (ui, a rendered window, on a session of scale 2)
- AS-1e (REQ-0001-18) Given black and white columns, a checkerboard and a picture with every pixel different, on a screen of scale 2, at 10, 25, 33, 50 and 75 %, Then every device pixel is exactly one picture pixel's color | Verified by: automated (ui, a rendered window, scale 2)
- AS-1c (REQ-0025-02) Given a 757 × 100 picture and a 900 × 100 picture in which every pixel has a color different from its neighbours, When Fit to Window is chosen in a canvas area 1001 wide for the first (129 %) and 1374 wide for the second (150 %), the area is then made 514 wide with the zoom kept, and the canvas is drawn scrolled to the start, by 1 logical pixel, by 7 logical pixels and to the far end, Then, with the edges read from the drawn pixels on every row and column: each square is the floor or the ceiling of the zoom's size, neighbours share an edge, each edge is at most half a device pixel from k × zoom/100, a scroll of N device pixels moves every edge by exactly N, and the first 757 edges at 129 % are the same as those of the first 757 pixels of a 900 × 100 picture at 129 % (Fit to Window in 1185 wide) | Verified by: automated (ui, a rendered window)
- AS-1d (REQ-0025-02, REQ-0025-05) Given each picture of AS-1c at its zoom with a grid cell of 4, When the canvas is drawn with the grid off and on at each scroll position of AS-1c, Then every vertical grid line lies on the device column where the square of picture pixel 4k starts, every horizontal one on the row where the square of 4k starts, both read from the frame with the grid off, and no line falls inside a square | Verified by: automated (ui, a rendered window)
- AS-2 (REQ-0025-04, REQ-0025-06) Given a 200 × 150 picture with the grid off, When a drag from the picture pixel (20, 30) to (150, 100) is made with the Pencil, the Brush, the Eraser, the Line (width 3), the Rectangle (width 2, filled), the Ellipse (width 4) and the Gradient, a Curve (the end points (20, 30) and (150, 100) and the control points (60, 120) and (110, 10)), a Polygon (the corners (20, 30), (150, 100), (30, 140), closed by a click on the first corner), and a Fill is clicked at (60, 60) inside a filled Rectangle drawn first from (40, 40) to (90, 90), once at 100 %, once at 129 %, once at 400 % and once at 800 %, with the pointer at the centre of each pixel, and once at 25 % and once at 50 % with each pointer position at the left and top edge of its pixel (the pixel's coordinates × zoom/100, which maps back to the same pixel), Then the picture's pixels are identical to those of the 100 % run in each of the other runs (all 30 000 of them) | Verified by: automated (ui)
- AS-2b (REQ-0025-06) Given the drag of AS-2 with the Line, the Rectangle and the Ellipse, When the pointer is at 0.1 and then at 0.9 of a pixel from the pixel's left and top edge, at 400 % and at 800 %, and at the centre of each pixel at 129 %, and when the same drag is made in one pointer jump and in 100 small steps, Then the picture's pixels are identical to those of AS-2's 100 % run; and a drag whose pointer positions are all inside the two end pixels gives the same pixels at 400 % and at 800 %; and a pointer exactly on the border of two pixels (the left edge of pixel 21, say) is the pixel to its right, and a Line that starts at the position (-0.5, 30.5), which is the pixel (-1, 30), gives the pixels of a Line from the pixel (-1, 30) clipped to the picture, not those of a Line from the pixel (0, 30); and for every k up to 1000 at 129 %, a pointer position exactly on the device column where the square of k starts is the pixel k | Verified by: automated (ui)
- AS-2c (REQ-0025-06) Given a picture in which every pixel has a different color, When the canvas is drawn at 129 % and at 150 %, scrolled to the start and by 7 logical pixels, Then for every device pixel of the view over the picture, the picture pixel the pointer position there maps to is the pixel drawn on that device pixel, read from the frame | Verified by: automated (ui, a rendered window)
- AS-3 (REQ-0025-05) Given a picture with every pixel different, When the grid is on at 800 %, a rectangle is marked with Select (tint, outline, eight handles), a polygon is being drawn (its pending preview is not written to the picture), a text box is open with the caret blinking, the Magnifier's box shows, and a Gradient guide line waits, and the canvas is drawn, Then the picture's pixels are exactly what they were, the list of undo steps and the modified mark are as they were, and a file saved then has the same pixels and no grid, tint, outline, handle or caret | Verified by: automated (ui)
- AS-3b (REQ-0025-05) Given the grid on at 800 %, When the canvas is drawn with a tinted rectangle selection, a floating selection and an open text box, in Opaque and in Transparent mode, Then a device pixel on a grid line inside each shows the grid's color over the content, a device pixel between two lines shows the content (under the tint, where there is one) and not the tint alone, and the outline is at most 2 logical pixels thick and a handle 5 × 5 | Verified by: automated (ui, a rendered window)
- AS-4 (REQ-0025-03) Given a text box with black "Hi" and a second run in red italic, and a floating selection after Rotate 90°, Flip Horizontal, Invert Colors and a stretch to 2 pixels wider, each in Opaque and in Transparent mode, at 100 %, 129 % and 800 %, also scrolled by 7 logical pixels, When the canvas is drawn with the grid, the tint, the outline, the handles and the caret not drawn, Then every pixel of the box or the selection is the color the put-down then writes at that pixel (compared with the picture after the put-down) and the pixels outside it are the picture's; and Given a floating selection partly outside the picture at 129 %, Then its part over the picture is what the put-down writes, and each device pixel of its part outside the picture shows the float's pixel of the square the drawn edges put there | Verified by: automated (ui, a rendered window)
- AS-4b (REQ-0025-03) Given a Line, a Rectangle (outline and filled), an Ellipse, a Curve (after its first, second and third click) and a Polygon (pending, with two sides drawn), at 100 %, at 129 % and at 800 %, When the canvas is drawn while the drag or the pending shape is shown, Then its preview is the same pixels as the picture after the shape is released or closed; and when the drag is cancelled with Esc, Then no pixel of the picture is changed | Verified by: automated (ui, a rendered window)
- AS-5 (REQ-0025-01) Given the Pencil, the Brush, the Eraser (with the background color #00000000, #FF000080 and an opaque one), Fill, the Line, the Rectangle, the Ellipse, the Curve, the Polygon and the Gradient, each with each of the colors #00000000, #FF000080 and #336699FF, and a paste, a text put-down and a transform, When each is used, Then every pixel it changes has exactly the color its rule gives (the given color; for the Eraser the background color; for a transform and a paste the colors the pixels already had, moved or inverted), every other pixel keeps its value, and no pixel is a blend of the given color and the one under it (the Airbrush is not in this list: REQ-0025-01) | Verified by: automated (core)
- AS-6 (REQ-0025-04) Given the text of 0017's AS-28, which proves at 100 % that every "h" has the same pixels as every other "h", in the same families, styles and sizes, When it is put down at 800 % with the view scrolled, Then the picture's pixels are those of the same text put down at 100 % | Verified by: automated (ui)

- AS-7 (REQ-0025-07, REQ-0025-02) Given an 800 × 600 picture (smaller than the view) and a 3000 × 2000 picture (larger), every pixel opaque and different from its neighbours, on the 1920 × 1200 screen at scale 1.25 and at 1.5, with the grid off, no selection, no text box, nothing pending and the pointer outside the window, When the canvas is drawn at 100, 125, 129, 150 and 200 %, in the window as it opens and with a canvas area of 1001 × 777 logical pixels, at the scroll's start, scrolled by 1, 3 and 7 logical pixels, to the middle and to the far corner (where the picture is larger than the area), after Fit to Window where it gives 100 % or more, and after 200 % → 100 % → 129 % while scrolled to the middle, Then the picture's top-left corner is on a whole device pixel; every picture row and column whose square lies wholly inside the visible area shows; every device pixel over the picture is exactly one picture pixel's color; each square's side is the floor or the ceiling of zoom/100 device pixels, each edge where k × zoom/100 from the corner puts it (REQ-0025-02's rule, computed by the test); at 100 % each picture pixel is one device pixel; the view's shaded border (4 logical pixels: 5 device pixels at 1.25, 6 at 1.5) and the picture's shadow are not counted, and a square cut by the view's edge is counted for its visible part; Given a 3000 × 2000 picture of one color at each of those combinations, Then every device pixel over the picture is that color; Given scale 1 and scale 2, Then AS-1 and AS-1b to AS-1e pass unchanged | Verified by: automated (ui, isolated at scales 1.25 and 1.5, a rendered window) + automated (core)
- AS-8 (REQ-0025-07) Given a white 800 × 600 picture at scale 1.25 and 100 %, When a Rectangle from (10, 10) to (790, 590) and a 5-Point Star from (100, 100) to (700, 500) are drawn with Shapes, width 1, and put down, Then every pixel of their outlines shows in the render in its exact color, and between any two neighbouring picture rows or columns no device row or column shows a color other than theirs | Verified by: automated (ui, isolated at scale 1.25) + manual M2 (T023: JOD at desktop scale 1.25)
- AS-9 (REQ-0025-07, REQ-0025-06) Given scale 1.25 and 1.5, at 100, 125 and 129 %, at the scroll's start and scrolled by 3 logical pixels, When the pointer is at 0.1 and at 0.9 of each device pixel over the picture (its logical position the device position over the scale), Then it maps to the picture pixel drawn on that device pixel, read from the frame (at 1.25 and 100 %, from the picture's left edge, logical x 0.79, device 0.9875, is pixel 0, and logical x 0.8, device 1.0, is pixel 1) | Verified by: automated (core) + automated (ui, isolated at scale 1.25)
- AS-10 (REQ-0025-07, REQ-0025-04, REQ-0025-05) Given AS-2's drags at scale 1.25 and 1.5, at 100 %, 129 % and 50 %, Then the picture's pixels equal AS-2's run at scale 1 and 100 %; When the picture is saved as PNG at scale 1.25 and opened again, Then the file's pixels equal the picture's | Verified by: automated (ui, isolated at scales 1.25 and 1.5)
- AS-11 (REQ-0025-07, REQ-0025-05) Given AS-1d's pictures with the grid on, at scale 1.25 and 1.5, at 100, 129 and 200 %, Then each grid line lies on the device column or row where square 4k starts and none falls inside a square | Verified by: automated (ui, isolated at scales 1.25 and 1.5)

## Edge cases
- A fractional screen scale (REQ-0025-07): the tint, the outline, the text box's frame and the handles may lie up to 2 device pixels off
  the drawn cell edges, as at a fractional zoom (the limit accepted on 2026-11-02); the grid is not in this list, it stays exact (AS-11).
  The view's shaded border and the overlay scrollbars are not counted, as in AS-1 to AS-1d.
- Below 100 % on a fractional screen scale: the look as before this change, not specified (REQ-0025-07); the picture's pixels and the
  pointer's pixel follow REQ-0025-04 and REQ-0031-03.
- A picture smaller than the canvas area: its squares are as in AS-1; the margin and the shadow around it (REQ-0009-01) are not picture pixels and are not counted.
- A zoom below 100 % (Fit to Window): REQ-0025-02 does not apply, REQ-0025-04 and REQ-0025-06 do (AS-2's 25 % and 50 % runs). On a screen of scale 2, REQ-0001-18 holds as at scale 1: AS-1e. A zoom that is not a multiple of 100 % (Fit to Window's 129 % for a 757-pixel picture in 1001, say) follows REQ-0025-02's rule for a fractional zoom: AS-1c.
- A named exception: the 4 logical pixels at the view's border are the scrolled window's own shading, drawn over a scrolled view, and are not counted in AS-1 to AS-1d (found from where the pixels sat, not from GTK's code).
- The overlay scrollbars lie over the view's far sides once the picture outgrows the area; they are the window's, and what they cover is not counted in AS-1 to AS-1d.
- At a fractional zoom the tint, the outline, the box's frame and the picture's far-edge resize handles may lie up to 2 device pixels off the drawn cell edges (JOD, 2026-11-02: accepted; the test overlay_sides_at_fractional_zoom stays under /0025-open/ as a known-red record).
- If there is no memory for a view built on the drawn edges, that frame is drawn by the renderer's scaling instead: the picture and the undo steps are untouched, no dialog shows, and the next frame tries again (JOD, 2026-11-02).
- A very large picture (8000 × 8000) at 800 % and 1600 % scrolled to its far corner: in AS-1.
- A window resize or a zoom change while a preview, a float or a text box is shown: the shown pixels stay those the put-down writes; the cases are those of 0015 (zoom changes during a stretch) and 0017 (the view changing with a box open), and are not repeated here.
- Undo and Redo of a put-down give the same pixels again (a case of REQ-0025-04, checked in AS-5's transforms and in the history tests of 0002).

## Out of scope
- Smoothing (Phase 2): when it exists it gets its own scenarios against REQ-0025-01 to -06.
- Colour choice and blending rules (ADR-0002 and each feature's spec); the PNG round trip (REQ-0001-30).
- Performance (each feature's own benchmark).
- A fractional screen scale: deferred by JOD (2026-10-28) until a real screen with one is available; REQ-0025-02's rule for it is then untested here, and not simulated. (Pointer note, change of 2026-11-13: from 100 % in scope, REQ-0025-07, tested on the isolated session's 1920 × 1200 monitor at 1.25 and 1.5; below 100 % still out of scope.)
- Scales other than 1, 1.25, 1.5 and 2 (1.33, 1.67, 2.5): not tested; REQ-0025-07 covers them by its words.
- A change of the screen scale while Easel is open (the display settings, a move to another monitor): its look is not specified or tested here (2026-11-13), but it shall not change the picture, the undo steps or a saved file (REQ-0025-04, -05; JOD, 2026-11-13).
- The Airbrush's pixels at a given input (random; its seeded rule is tested in its own feature).

## Open questions
None.

## Plan
**Approach.** The tests read every edge from the drawn pixels and compare frames, so they hold under any renderer; a formula is used only for the half-pixel bound. At a whole zoom the drawing stays as it is (the probe below: clean in every renderer). At a fractional one the renderer no longer picks the edges: the canvas builds the visible part of the picture as one texture at device size, through a source index per device column and using one edge function, and appends it 1:1; it reads the picture's pixels, or, while a preview replaces them, that preview's. The grid's axis lines use the same function, and so do the text box, the floating selection and the shape previews, so that one cell has one edge wherever it is drawn. The pointer maps by that function's inverse (JOD, 2026-11-02): one function for edges, drawn and pointed. The order of features asked for on 2026-10-28 is done (0018, 0017's fix, 1600 %).

**Probe, 2026-11-02** (a 1200-wide texture with NEAREST, horizontal only, offscreen through `gsk_renderer_render_texture`; JOD's real session uses `GskVulkanRenderer` by default, and ngl there gave the same results). No renderer rounds a tie one way: at 129 % as tiles, cairo 7 up and 5 down, ngl and vulkan 9 and 3; at 150 %, 300 and 300 against 425 and 175. Every width was the floor or the ceiling, no cell was skipped, and every edge that is not a tie was at its nearest device pixel. A scroll moved tied edges: at 150 % 27 under ngl and vulkan (3 under cairo, at scroll 101); at 133 % 2 under ngl and vulkan; at 129 % 1 under cairo. Tile boundaries themselves are not the fault: 500 × zoom/100 is whole at any whole percent, so each tile starts on a whole device pixel; but the division changes ties (5 edges at 129 % between one texture and tiles). 100, 200 and 800 % were clean everywhere. **T001's probe, the same day** (`probe-edge-drawing edges`, 113 to 200 %, pictures 290 to 4000 wide, both axes, 5 scrolls, each edge read on three lines across the cell):
- The view built by the edge rule (per pixel, and from a source index per device column) holds every rule under vulkan, ngl and cairo: 0 widths, 0 past half a pixel, 0 skipped, 0 blended, 0 jagged, 0 moved by a scroll, 0 differing with the picture's size.
- The renderer's scaling, as today: under vulkan and ngl **an edge differs from row to row** (jagged: 748 readings at 129 %, 34146 at 150 %), 0 moved on the lines read, 4 differing with the size at 150 %. Under cairo 0 jagged, 26 moved at 129 %, 28 at 150 %, 254 differing with the size at 150 %. The picture's far column also read as blended in 30 readings at 113, 133 and 171 % under vulkan and cairo; it looks like the partly covered last column at a fractional far edge, not traced.
- **Why the first two probes disagreed** (27 moved against 0 at 150 %, ngl and vulkan): the height of the rectangle drawn, 8 logical pixels in the first and 60 in the second. One difference at a time: the rows of the texture (3 or 40) and the viewport's width change nothing; the height alone takes 27 to 0, and the ties up and down from 425 and 175 to 578 and 22. Both probes read one row, and the jagged edges above show that a tie's direction changes with the row. So a test that reads one row can pass by luck: `drawn_edges` reads every row and column.
- **The shifted texture fails** (1/256 device pixel, with and without a clip to whole device pixels): under vulkan and ngl the first column and the column at each tile's seam come out at alpha 254, the pixel after the seam is lost (pixel 500 at 129 %), and at 150, 171 and 200 % edges go past half a pixel or widths break (at 200 %, a whole zoom, 208 widths). It passes under cairo only. It is not an option.
- **The choice (the cheapest that passes under all three):** the view built from a source index per device column. T001's frame numbers decide whether it is accepted.


**The blend at scale 2 (compositor screenshots, the private session, 2026-11-02; its shell started with `--unsafe-mode` through `EASEL_ISOLATED_SHELL`, for this probe only, since the shell refuses a screenshot otherwise).** The real app, a 200 × 150 picture at 100 %. Scale 1, cairo and vulkan: 30000 of 30000 exact. Scale 2, before T006, cairo and vulkan: none exact, the first row `000000 000800 001800 002800 003800`, the first column `000000 000000 000001 000002`. A bare GTK program does the same: `gtk_snapshot_append_scaled_texture` with NEAREST at one device pixel per texel, and `gtk_snapshot_append_texture` is exact. So on a screen of scale 2 the view goes through the built view at whole zooms too, and the built view is appended as a plain texture. After T006, scale 1 and 2, cairo and vulkan: 30000 of 30000 exact, the first row `000000 001000 002000 003000`.

**Overlays at a fractional zoom (T009 to T012's report, 2026-11-02, report only, nothing fixed; the test is `overlay_sides_at_fractional_zoom` under `/0025-open/`, which no meson test runs).** At 129 % and 150 %, 7 rectangles each. The tint of a marked selection and of an open box is drawn at k × zoom, not on the drawn edges: its sides are at most 1 device pixel from them, and 5 to 7 of the 7 rectangles are off, the difference being a column or row tinted in part. The outline's outer side is at most 2 device pixels from the drawn edge; part of that may be its thickness, which is not told apart here. A handle's touching side, read from its rectangle and not from the frame, is on the drawn edge. `to_corner` picks the corner at `floor (x / l + 0.5)`: at most 1 device pixel from the point, and no nearer drawn edge exists at any of 3000 device positions at either zoom, so it needs no change. The box's drag rounds to whole picture pixels: the drawn left side ends at most 1 device pixel from the pointer's move, which is what a whole-pixel move must do at these zooms. No requirement asks an overlay to lie on the drawn edges (REQ-0025-05 asks that it leave the pixels alone), and JOD left it so (T015, decided: not done; the limit is in the edge cases).

**Expected to fail today.** AS-1d on every renderer (the grid floors each edge: at 129 %, k = 16 is 20.64, the line at 20, the cell at 21; from the code, not yet run). AS-1c: under cairo edges move with the scroll and differ with the picture's size; under Vulkan edges differ from row to row, which `drawn_edges` sees because it reads every row and column (this is what T005's gate turns on). AS-2c at 129 % and 150 %: the pointer maps by floor (x / l) while the squares follow the edges (k = 1 at 129 %: the device column 1 is the square of the pixel 1, and a position at its start gives floor (1 / 1.29) = 0). Measured 2026-11-02: `easel_view_to_pixel` gives k - 1 for 78 of k = 0 to 1000 at 129 % with x = k × l (first k = 27), a double's error, which the integer inverse does not have. AS-4 and AS-4b at 129 % are not probed; their tests decide.

**Interfaces.**
- `int easel_view_edge (int k, int zoom)` in `core/view.c`: `floor (k * zoom / 100.0 + 0.5)`. The tie direction is the code's choice, not the spec's.
- `int easel_view_cell_at (int d, int zoom)`, the inverse: the k with `easel_view_edge (k) <= d < easel_view_edge (k + 1)`, in 64-bit integers, `ceil ((2d + 1) × 50 / zoom) - 1` (checked by script 2026-11-02 for k from -200 to 3000 at 113, 129, 133, 150, 171, 258 and 300: no error; at 300, 400 and 1600 % the same as floor; a pixel's centre always falls in its own square).
- `bool easel_view_draws_by_edges (int zoom)`: the zoom is 100 % or more and not a multiple of 100 (below 100 % REQ-0025-02 does not apply and several picture pixels share a device pixel).
- `append_view_by_edges` in `easel_canvas_snapshot`, chosen by `view_built_by_edges` (the zoom is 100 % or more, the screen scale is whole, and a square is fractional or the screen's scale is not 1). It reads the picture's pixels, or a preview's tile where one replaces it, and keeps the last texture for a repaint with nothing changed; `grid_segment`, `append_text_box`, `append_float_outside` and `append_shape_preview` place cells by `easel_view_edge` in that case.
- `easel_view_to_pixel`: where `easel_view_draws_by_edges`, the device pixel `floor (x × scale)` through `easel_view_cell_at`, no tolerance; everywhere else today's `floor (x / l)`, unchanged. A negative position gives a negative pixel (-0.5: the device pixel -1, the pixel -1).
- **The readers of a pointer position** (read 2026-11-02): `to_pixel` goes through `easel_view_to_pixel`. `to_corner` (`floor (x / l + 0.5)`) and the text box's drag (`canvas.c:1546`) map to a corner or an offset, not a pixel, and stay unless T008 shows otherwise. The view's bounds (`canvas.c:1642`, `1793`, `2754`, `3559`) are ranges with a pixel to spare and do not change.

**The exact device scale.** The new drawing path needs a whole device scale (the canvas checks it beside `easel_view_draws_by_edges`), since it places a texture of whole device pixels in logical coordinates. At a fractional screen scale the current drawing and the current pointer mapping stay, the grid's floor included: untested, an open item until a real screen with one is available (Out of scope).

**The upload (T014).** A new view texture costs about 2.3 ms of CPU per frame, more than the build (1.4 to 1.9 ms). Frame times of the fractional-zoom cases: not reliably measured (a covered window gets no frame callbacks on Wayland; CPU figures valid; M1 by eye smooth, JOD 2026-11-02). Optimizations are Phase 2.

**Frame budget (REQ-0025-02; JOD, 2026-11-02).** Median CPU per frame at most 16.7 ms at 1920 × 1080 while scrolling and while drawing with the Pencil, at 129 % and at 150 %, in the real session; worst frames are reported; a miss in 2 of 3 runs counts as a miss. A repaint with nothing changed (a pointer move) reuses the last texture and costs no more than today. Goal: median at most 8 ms; a 4K view is a named slow case. Before the number is accepted, the loop is tried with a precomputed source index per device column and per row, and the new median reported. T001 ends with the numbers here, shown to JOD; T006 waits for his OK. A miss brings the fallback to JOD and stops the work. 
The numbers (`probe-edge-drawing frames`, 2026-11-02, real session, `GskVulkanRenderer`, JOD's "ready", load 0.48 to 0.54; one process, the ways interleaved over 3 rounds, 90 frames each; the window got 1853 × 1131, so the view was 1853 × 1080, not 1920 wide). Process CPU per frame, median of the 3 rounds / worst frame, ms:

  | Case | Today | The per-pixel loop | With the index |
  |---|---|---|---|
  | 129 %, scrolling | 1.05 to 1.10 / 1.41 | 7.56 to 7.70 / 14.37 | 4.32 to 4.72 / 6.20 |
  | 150 %, scrolling | 1.07 to 1.12 / 1.41 | 7.48 to 8.06 / 13.21 | 4.24 to 4.73 / 5.83 |
  | 129 %, Pencil | 0.91 to 1.43 / 2.60 | not run | 3.88 to 4.30 / 6.21 |
  | 150 %, Pencil | 1.13 to 1.30 / 2.54 | not run | 4.27 to 4.34 / 6.01 |
  | 129 %, nothing changed | 0.35 to 0.37 / 0.48 | not run | 0.31 to 0.32 / 0.52 |
  | 150 %, nothing changed | 0.37 to 0.38 / 0.54 | not run | 0.31 to 0.32 / 0.49 |

  Frame times of the fractional-zoom cases: not reliably measured (a covered window gets no frame callbacks on Wayland; CPU figures valid; M1 by eye smooth, JOD 2026-11-02). The slow case, a 4K view, the build alone: 3.93 / 8.80 ms at 129 %, 3.49 / 3.69 ms at 150 %, no frame timed. The budget (16.7 ms) and the goal (8 ms) are met in all 3 rounds; the index cuts the loop's median by about 40 %. The probe is not the canvas: T007 measures the real path.

**T006's gate (JOD, 2026-11-02).** The new drawing path is built only if AS-1c and AS-1d are red in the real app path under Vulkan (T005 run under Vulkan, not only cairo or a probe). T001 also tries the cheap alternative, the texture shifted by under 0.01 device pixel to break ties one way, under vulkan, ngl and cairo, against the same edge rules on both axes; the Plan chooses the cheapest option that passes under all three.

**Files.** `src/core/view.*`, `src/ui/canvas.c`, `tests/core/test-view.c`, the core tests of the shapes and of the text put-down, `tests/ui/test-window.c`, `tests/meson.build`, `tools/isolated.sh` (an option for a session of scale 2), `tests/mutants.txt`, `tests/probe/`, `tests/perf/`, `docs/learning-log.md`.

**Test runs.** `ui-0025` under cairo as usual; `ui-0025-vulkan` runs AS-1, AS-1c and AS-1d under Vulkan (AS-2c joins at T008), and under another renderer where there is none; `ui-0025-scale2` runs AS-1b on a session of scale 2, a 3840 × 2400 virtual monitor through `tools/isolated.sh --scale 2` (6.0 s to start against 4.0 s, the shell's peak 292 MB against 293 MB; it skips in a plain session, so `/sdd-check` runs it by hand) (probed 2026-10-28: a headless GNOME Shell's virtual monitor through `org.gnome.Mutter.DisplayConfig.ApplyMonitorsConfig`; a `monitors.xml` in the throwaway `HOME` was not honored). AS-1 is 7 zooms × 3 areas × 5 scrolls: if it takes over 15 s it goes under `/long/0025`, with a fast version in the short suite.

**AS-5, the test each tool has today and where a new one is needed** (a color of any alpha, exact; read from the tests on 2026-10-28):
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

### Scenario-to-test map
| AS | Test |
|---|---|
| AS-1 | `as1_squares_at_whole_zooms`, `as1_far_corner_of_8000` |
| AS-1b | `as1b_squares_at_scale_2`, `as1b_far_corner_of_8000` |
| AS-1e | `as1e_no_blend_below_100_at_scale_2`, core `built_by_edges_at_any_zoom_on_a_scale_above_1` |
| Edge case, no memory for the built view | `view_without_memory_falls_back` |
| AS-1c | `as1c_edges_at_fractional_zoom`, core `as1c_edge_rule`, `draws_by_edges_only_where_a_square_is_fractional` |
| AS-1d | `as1d_grid_on_cell_edges` |
| AS-2 | `as2_same_figure_at_any_zoom` |
| AS-2b | `as2b_pointer_inside_pixel`, core `as2b_pointer_on_border` |
| AS-2c | `as2c_pointer_hits_the_drawn_pixel`, core `as2c_cell_at_is_the_inverse` |
| AS-3 | `as3_overlays_change_nothing` |
| AS-3b | `as3b_overlays_show_through` |
| AS-4 | `as4_shown_equals_put_down`, `as4_float_partly_outside` |
| AS-4b | `as4b_preview_equals_release` |
| AS-5 | the existing tests of the table above, and core `as5_shapes_write_exact_colors`, `as5_text_writes_exact_colors` |
| AS-6 | `as6_letters_at_800_scrolled` |
| AS-7 | core and ui at 1.25 and 1.5 (T018, T019; names when built); T020's matrix |
| AS-8 | ui at 1.25 (T018, T019); M2 (T023) |
| AS-9 | core and ui at 1.25 (T018, T019) |
| AS-10 | ui at 1.25 and 1.5 (T018, T021) |
| AS-11 | ui at 1.25 and 1.5 (T018, T019) |

### The rest of the Plan

**One manual break per scenario** (the test is written first, the break makes it fail, the failure is shown, the break is reverted). AS-1 and AS-1b: round the tile origin differently. AS-1c: draw a fractional zoom through the renderer again. AS-1d: floor the grid's axis lines again. AS-2: scale the pointer by the zoom before it is mapped. AS-2b: truncate instead of floor (fails the -0.5 case). AS-2c: map by `floor (x / zoom)` (JOD). AS-3: draw the grid into the picture's pixels. AS-3b: the text box over the grid, or the tint alone over the content. AS-4: skip a step of the put-down in the float's drawing. AS-4b: draw the preview one step short. AS-5: blend the given color with the pixel under it in one shape's write. AS-6: the put-down made to depend on the zoom (the old text settings cannot fail it: they belong to 0017's AS-28, T012).

**Constitution.** P7 is what this feature tests; core stays libc only; every fix follows a red test.

**Risks.**
- CPU cost per frame at a fractional zoom, up to the view's size, and a texture upload: measured by T001 before the path is relied on (the Frame budget above), and in the real session by T007.
- **No cheap fallback:** the shifted texture fails under vulkan and ngl (above). If the indexed view misses the budget, that is brought to JOD and the work stops; today's drawing stays.
- A floating selection or a gradient preview replaces tiles with textures: reading those each frame cost about 7.4 ms median in the isolated session against 3.1 for the index, still under the goal; T006 measures it in the real path.
- The tests see `render_texture`, not the screen: M1 is JOD's look in the real session.
- The probe is horizontal only: AS-1c tests both axes.

### Change of 2026-11-13 (a fractional screen scale, REQ-0025-07, AS-7 to AS-10; accepted 2026-11-13)
(Opus, 2026-11-13.) From JOD's bug at desktop scale 1.25 and the probe of 2026-11-13 (`tools/isolated.sh --scale 1.25` and `1.5`, an
800 × 600 picture of distinct pixels at 100 %, read from the window's render): under Cairo and Vulkan about 476 000 device pixels of 480 000
were a blend of two to four picture pixels (a pixel at 1.25 read `0x001713` for `0x001014`), at scale 1 none. Two causes:
(1) `easel_view_built_by_edges` (`src/core/view.c`) gives FALSE at any fractional scale, so the picture is left to the renderer's scaling of
500 × 500 tiles, which interpolates there (the hairline is the seam at row 500); (2) the canvas lies at a fractional device position (logical
(99, 58) is device (123.75, 72.5)), so even a view built on device edges lands between device pixels: with the rule forced, 1 394 (Cairo)
and 10 776 (Vulkan) blended pixels remained.
- **Core** (`view.c`): `easel_view_built_by_edges (pct, scale)` gives TRUE from 100 % at any scale (a fractional one included); below 100 %
  it stays whole scales only (REQ-0025-07: unchanged there). `easel_view_to_pixel` then takes the edges' branch at a fractional scale too,
  so a pointer maps to the drawn cell (AS-9). New `easel_view_device_shift (double origin_device)`: the fraction to move the view by so that
  its first device pixel is a whole one (`origin - floor (origin)`, in [0, 1)); pure, tested.
- **Canvas** (`canvas.c`): the canvas's device origin in its surface, from `gtk_widget_compute_point` to the native, the native's
  `gtk_native_get_surface_transform`, times the scale; the snapshot is translated by minus its shift over the scale (at most one device pixel
  up and left), before anything is drawn, so the picture, the grid and every overlay move together. Every place that turns a pointer into a
  device position adds the same shift (`to_pixel`, `mouse_takes_marker`), so what is pointed at is what is drawn (REQ-0025-06). At a whole
  scale with a whole origin the shift is 0: nothing changes at scales 1 and 2 (the regression list proves it).
- **The scale is read, never kept** (JOD, 2026-11-13): every draw and every pointer mapping asks the surface for its scale (`easel_canvas_get_scale`) and the canvas's device origin at that moment; no value is cached, so a scale change while open (out of scope for its look) cannot leave a stale one. T019 checks by grep that no new field holds either.
- **Every reader** of `view_built_by_edges` and `easel_view_to_pixel`, which now see TRUE at 1.25 and 1.5 from 100 %:
  `easel_canvas_get_handle_rect` (below 100 % only), `append_text_box`, `append_float_outside`, `append_grid`, `append_shape_preview`,
  `canvas_snapshot_frame` (three, the tiles and the previews), `to_pixel`, `mouse_takes_marker`. Each already works in device pixels with
  `easel_canvas_get_scale` (whole scale 2 today), so a fractional scale is a value they have not seen: each gets a ui case at 1.25 in the
  regression list or the probe matrix of T020. The 45 calls of `easel_canvas_get_scale` are read for an integer cast (a `(int) scale` would
  turn 1.25 into 1) before T019 is done.
- **Tests, red first (T018):**
  - core `as7_fractional_screen_rule` in `test-view`: at scales 1.25 and 1.5 and zooms 100, 125, 150, 200 the view is built by edges; for every
    device column 0 to 2999 `easel_view_to_pixel` gives the cell `easel_view_cell_at` draws there; at 100 % cell d is pixel d; no cell is
    skipped and the widths differ by at most one; scales 1 and 2 as before; `easel_view_device_shift` on 123.75, 72.5, 15, 0.
  - ui `/0025-frac/as7_rows_and_columns` (AS-7: the 800 × 600 and 3000 × 2000 pictures of distinct pixels and the one-color 3000 × 2000,
    each edge computed as k × zoom/100 from the picture's corner; at 100, 125, 129, 150 and 200 %; the window as it opens and a 1001 × 777
    area (`set_area`); the scroll's start, 1, 3 and 7 logical pixels, the middle and the far corner; Fit to Window; 200 → 100 → 129 %
    scrolled; the shaded border, the shadow and the scrollbars left out as AS-1 does), `/0025-frac/as8_shapes_outlines` (AS-8, Shapes
    through the canvas's press path, the neighbour rule over the whole picture), `/0025-frac/as9_pointer` (AS-9, the canvas's mapping at
    0.1 and 0.9 of each device pixel, against the frame), `/0025-frac/as10_same_pixels_and_file` (AS-10: AS-2's drags at 100, 129 and 50 %
    against a scale 1 run's expected pixels, and the PNG round trip at 1.25), `/0025-frac/as11_grid` (AS-11). Each skips unless the session's
    scale is 1.25 or 1.5 (as `/0025-scale2` does at 2). New meson tests `ui-0025-scale125` and `ui-0025-scale150`, run by
    `tools/isolated.sh --scale 1.25` and `--scale 1.5`; `docs/engineering.md`'s list of runs gains them.
- **Proof that GTK leaves the pixels exact (T020, JOD's matrix):** the AS-7 reading run at zooms 100, 125, 150 and 200 % on scales 1, 1.25, 1.5
  and 2, under Cairo and Vulkan; each of the 32 combinations recorded exact or not, with its counts. If one stays blended, the work stops and
  JOD hears it before anything builds on it.
- **Regression (every task from T019):** the ui suites that compare screen captures: `ui-0010`, `ui-0013`, `ui-0014`, `ui-0017`, `ui-0025`,
  `ui-0031`, `ui-0039`, `ui-0043`; the scale 2 list; `ui-0025-scale125` and `ui-0025-scale150`; core and io.
- **Cost (T022):** the built view costs more than a scaled texture. `perf-0025`'s cases (a scroll and a drag at 100 % and 150 %) on a
  1536 × 960 view, in the real session (benchmarks never run isolated), the working tree against `797639b` (a worktree), interleaved, three
  runs each, at scale 1.25 (JOD's desktop) and, after JOD sets them in Settings › Displays, at 1 and 2; against the budget "pointer event to
  pixels within one frame".
- **Mutants** (T018 core, T019 ui): the fractional scale left out of the rule from 100 %; the rule also taken below 100 %; the shift left out of
  the snapshot; the shift left out of the pointer; the shift's sign.
- Constitution: the rule in `src/core` with its test and mutants; no picture pixel, undo step or file changes (P7.4, P7.5; AS-10 proves the
  file); tests first. Risks: (1) the renderer may still blend inside a built cell at a fractional scale (the cells are textures drawn at
  device size over 1/scale logical): T020's matrix answers it before anything else; the fallback is the cells drawn as colour rectangles of
  whole device size. (2) The surface's own transform at a fractional scale is not known to be whole device pixels: the probe in T019 prints
  it. (3) Cost, T022.

## Tasks
- [x] T001 Probe the 1:1 texture (REQ-0025-02) - done when `tests/probe` shows, at 129, 133 and 150 % under cairo, ngl and vulkan, edges that hold across scrolls and picture sizes, for the view built by the edges and for the shifted texture, the reason the first two probes disagreed is found, and the Frame budget's numbers (with the precomputed index, scrolling, drawing and a repaint with nothing changed) are in the Plan and shown to JOD; **gate: T006 waits for his OK; a miss brings the fallback and stops**
  Result: characterization: a probe, no red run; its control, the renderer's scaling, fails the rules under all three renderers. The index holds every edge rule under vulkan, ngl and cairo, the shifted texture fails under vulkan and ngl, and the budget's numbers are in the Plan: median CPU 3.88 to 4.73 ms. JOD accepted the numbers for the budget gate, 2026-11-02; T007 must confirm them in the real canvas.
- [x] T002 The `drawn_edges` helper (every row and column) and AS-1, with 1600 % and the 8000 × 8000 corner (REQ-0025-02; AS-1) - done when the test passes and fails under the tile-origin break; likely passes through shared code, proved by that mutant
  Result: characterization: passes through shared code, on the 200 × 150 picture after the area fix (frames scrolled: 40 in x, 44 in y; the earlier green run never scrolled and proved nothing about the scroll rule). Made red by the tile-origin break (`strange == 0`: 150) and by a break moving the picture 1 device pixel once scrolled, each shown and reverted.
- [x] T003 AS-1b on a session of scale 2 (REQ-0025-02; AS-1b) - done when `ui-0025-scale2` passes and fails under a break of the built view
  Result: red first, for a reason the task did not expect: a picture pixel at 100 % on scale 2 was blended with its neighbour (`000000 000800 001800 002800`), fixed in T006. Green after T006 through `tools/isolated.sh --scale 2 meson test -C build ui-0025-scale2` (the view at whole zooms goes through the built view on that screen, so the tile-origin break cannot reach it); red under a break moving the built view 1 device pixel (`strange == 0`: 150), shown and reverted. In a plain session the case skips.
- [x] T004 `easel_view_edge`, `easel_view_cell_at` and `easel_view_draws_by_edges`, core test first (REQ-0025-02; AS-1c) - done when `test-view` was red, then green
  Result: `as1c_edge_rule` failed first against stubs: `CHECK failed: easel_view_edge (0, 129, 1) == 0`; green with the functions (`core-view` 1 of 1). Not yet used by the canvas; `as2b_pointer_on_border` is core only until T008. The scale taken out (JOD, 2026-11-02, Option A): the tests first, and the build failed on the old signatures, then the functions.
- [x] T005 AS-1c and AS-1d written and run in the real app path under cairo and under Vulkan (REQ-0025-02, REQ-0025-05; AS-1c, AS-1d) - done when each run's result is in the Result line; **gate: T006 is built only if both are red under Vulkan**
  Result: both red in the real app path under Vulkan, in the real session and in isolation, with the same numbers. AS-1c, Vulkan and ngl, 757 wide at 129 %: an edge differing row to row 22 and column to column 6, moved by a scroll 2, differing with the picture's size 2; 900 wide at 150 %: row to row 1241, column to column 214, moved 7. AS-1c, cairo: 1 differing with the size at 129 %; 168 moved and 450 device pixels not following a scroll at 150 %. AS-1d, all three, 129 %: 181 of 386 vertical lines not on a square's edge and 182 edges without their line; 48 of 96 horizontal and 48. At 150 % no grid failure: a cell of 4 is 6 device pixels, whole. Widths and half-pixel 0 everywhere. The gate is open; `ui-0025` stays red until T006.
- [x] T006 Fix: the fractional drawing path and the grid on `easel_view_edge`, and `ui-0025-vulkan` (REQ-0025-02, REQ-0025-05; AS-1c, AS-1d) - done when T005's tests pass under cairo and Vulkan; starts only after JOD's OK at T001 and T005's gate
  Result: T005's red runs were the failing tests. AS-1c and AS-1d green under cairo, ngl and vulkan in isolation and in the real session, every count 0. Red again under breaks: the built view moved 1 device pixel (AS-1c, half-pixel), the grid floored again (AS-1d: 181 and 182 as in T005). `tools/isolated.sh meson test` 54 of 54. The floating selection's and the gradient's path through the built view has no test at a fractional zoom until T010.
- [x] T007 Frame cost at 129 % and 150 % against 800 %, the working tree (the new path) against `5f1d6d3` (the old path; HEAD equals it), interleaved, in the real session after JOD's "ready", in the real canvas with overlays and with a floating selection or a gradient preview showing (REQ-0025-02) - done when the numbers are in the Plan and confirm T001's
  Result: characterization: a measurement. `perf-0025`, real session, `GskVulkanRenderer`, 2026-11-02, JOD's "ready", 3 runs each of the working tree (the new path) interleaved with `5f1d6d3` (the old path; HEAD equals it), load 0.50 to 0.71, the view maximized at 1766 × 1043 (11 % fewer pixels than the budget's 1920 × 1080). Process CPU, each run's median, ms, the working tree / `5f1d6d3`: 129 % scroll 3.91 6.17 4.09 / 1.09 1.25 1.26; Pencil 3.92 6.21 4.01 / 1.01 1.31 1.47; nothing changed 0.61 1.02 0.71 / 0.59 0.99 0.57; grid and selection 9.64 6.56 6.39 / 4.82 4.84 4.88; float 4.58 4.70 4.60 / 1.34 1.36 1.30. 150 % scroll 6.16 3.59 3.88 / 1.11 1.31 1.17; Pencil 6.02 3.64 3.83 / 0.90 1.02 1.07; nothing changed 0.45 0.59 0.50 / 0.60 1.08 0.58; grid and selection 8.87 5.86 5.67 / 4.05 4.08 4.09; float 7.41 4.53 4.62 / 1.38 1.41 1.26. Worst frame's CPU 14.17 ms. The budget (16.7 ms) is met in every run; the goal (8 ms) is missed once in 3 runs by grid and selection, at 129 and at 150 %, so not in 2 of 3. Peak memory 183 MB against 164 MB: the 19 MB extra is the kept view, not a slow case (JOD; one view's texture at this size is 7.4 MB, what makes up the rest was not measured). 800 % as before. Frame times: not reliably measured (a covered window gets no frame callbacks on Wayland; CPU figures valid; M1 by eye smooth, JOD 2026-11-02).
- [x] M1 JOD looks at 129 % and 150 % with the grid on, scrolling (Pan tool active; REQ-0025-02; AS-1c, AS-1d) - done when he has seen the lines on the cell edges and no cell changing size
  Result: JOD, 2026-11-02, the real session: "all is good as for me", on `m1-pale-129.png` at 129 % with the grid on and the Pencil, and at 300 % with a marked rectangle; earlier at 443 % and 1600 %. Read from his screenshots: 129 %, 257 vertical and 144 horizontal lines, all on `easel_view_edge (4k)`, 5 and 6 device pixels apart (a few pixels, about 0.01 %, of other colors, not traced); 443 %, widths 4 and 5, four colors, no blend, the lines on the edge rule; 1600 %, 3901 squares of one color. The tint was seen at 300 %, a whole zoom, and no shot shows it at 129 %; scrolling and the aim of the Pencil rest on his word.
- [x] T008 AS-2, AS-2b and AS-2c with 129 %: the pointer maps by the inverse of the edge function (REQ-0025-04, REQ-0025-06; AS-2, AS-2b, AS-2c) - done when AS-2c was shown red, `easel_view_to_pixel` goes through `easel_view_cell_at` where the squares are drawn by edges and is unchanged elsewhere, the core test holds the border column for every k up to 1000 at 129 %, and AS-2c fails under the `floor (x / zoom)` break; the rest likely passes, proved by the Plan's breaks.
  Result: `as2c_pointer_hits_the_drawn_pixel` failed first: of 8070440 device positions the map was wrong at 2394312 and the canvas's pointer at 2059, the first at 129 %, device pixel (1, 0), which shows the pixel 1 and mapped to 0; `as2b_pointer_on_border` failed first in core (`px == k && py == k`). Green with `easel_view_to_pixel` through `easel_view_cell_at`, 0 wrong under cairo, ngl and vulkan. `as2_same_figure_at_any_zoom` and `as2b_pointer_inside_pixel`: characterization, they passed before the fix, since a pixel's centre is always in its own square; made red by the breaks: scaling the pointer by the zoom (AS-2, 290 pixels differing), truncating instead of flooring (AS-2b, the -0.5 case). The map by `floor (x / zoom)` brings AS-2c back to 2394312. Each shown and reverted. `tools/isolated.sh meson test` 54 of 54. `to_corner` and the text box's drag map to a corner or an offset, not a pixel: unchanged and not tested here at a fractional zoom.
- [x] T009 AS-3 and AS-3b (REQ-0025-05; AS-3, AS-3b) - done when they pass and fail under the Plan's breaks; likely pass through shared code
  Result: characterization: `as3_overlays_change_nothing` and `as3b_overlays_show_through` pass through shared code. Made red by the breaks, each shown and reverted: the grid written into the picture's first pixel (AS-3), the tint at full strength (AS-3b), the text box drawn over the grid, the 0017 bug (AS-3b). The test's own faults on the way: Undo is enabled while a shape waits, since it cancels it, so the steps are read once the state has ended; the Font card lies over the view, so the point outside the box is above and left of it. The saved file is read with the grid on and a rectangle marked; a polygon or a box open at a save is put down by its own rule, so it is not read there.
- [x] T010 AS-4 and AS-4b with 129 %, and any fix they call for (REQ-0025-03; AS-4, AS-4b) - done when they pass and fail under the Plan's breaks
  Result: `as4b_preview_equals_release` failed first at 129 % only: 17786 device pixels, the preview's runs blended at fractional places (a rectangle's first at (25, 38): `fff1f5f8` against `ffffffff`). `as4_shown_equals_put_down` failed first with 1266, the box at 129 % only (`ff009f09` against `ff00a009`); the float was clean. Green under cairo, ngl and vulkan after the shape runs, the box and the float's part outside the picture go on the drawn edges (`append_cells_by_edges`). Made red by the breaks, shown and reverted: the box's ink left out, the float's preview left out of the built view (5811 pixels), a run one pixel short. The test hides the Font card, whose soft shadow lies over the view. A float reaching outside the picture is T016's. The polygon's preview has no closing side, so what closing adds is left out of the comparison: REQ-0025-03 reads as if the two were the same.
- [x] T011 AS-5: new core tests for the shapes and the text put-down with colors with alpha (REQ-0025-01; AS-5) - done when they pass and fail under the blend break
  Result: characterization: the rules hold. `as5_shapes_write_exact_colors` (5 shapes, 3 colors) made red by blending #FF000080 in a figure's write; `as5_text_writes_exact_colors` by writing a #00000000 foreground as white, which `as16_put_down_ink` cannot see. Each shown and reverted.
- [x] T012 AS-6 (REQ-0025-04; AS-6) - done when it passes and fails with the old text settings back; likely passes, proved by that break
  Result: characterization: `as6_letters_at_800_scrolled`, 54 runs, passes. The Plan's break does not reach it: with the old text settings (gray antialiasing, no whole-pixel positions) it still passes, since those change a letter's pixels at every zoom alike, and that break belongs to 0017's AS-28. Made red by a break that makes the put-down depend on the zoom, shown and reverted.
- [x] T013 Mutants for REQ-0025-01 to -06, every manual break's run recorded, the full suite, the learning-log line for the rewritten rule, the renderers' mixed ties and one function for edges, drawn and pointed - done when all are green and the map carries the names as built
  Result: `tools/mutate.sh` of REQ-0025-01 to -06: 19 killed, 0 by crash, 0 survived, 401 s; the one ui mutant killed in under 5 s (a tile 1 device pixel off, 3 s) killed again alone. The manual breaks are in each task's Result; those a plain session cannot run, at scale 2 (the built view as a scaled texture; whole zooms left to the renderer on that screen), turned AS-1b red, `strange == 0`: 29700, shown and reverted. `tools/isolated.sh meson test` 54 of 54; `ui-0025-scale2` passes through `--scale 2`. `as4_shown_equals_put_down` takes 13.8 s, so it stays in the short suite. The learning-log lines are in. Not run here: the whole mutant list, which `/sdd-check` runs.
- [x] T014 The final perf run, on a quiet machine, after JOD's "ready" (REQ-0025-02) - done when, for the working tree and for the commit before it, the share of frames over 16.7 ms and over 33 ms is reported, not only medians, and the upload is timed apart from the build, which tests the guess in T007's open finding
  Result: characterization: a measurement. `perf-0025`, real session, `GskVulkanRenderer`, 2026-11-02, JOD's "ready", 180 frames a case, 3 runs each of the working tree interleaved with `5f1d6d3`, load 0.46 to 1.06, the view 1766 × 1043. CPU median at most 8.35 ms, so the budget holds; the goal (8 ms) is missed in one run of 3 (129 %, grid and selection). Frame times: not reliably measured (a covered window gets no frame callbacks on Wayland; CPU figures valid; M1 by eye smooth, JOD 2026-11-02). A "frame" there was the elapsed time from before the action to the next after-paint, which is no frame period; those figures are removed, and the shares this task asked for were not obtained. The upload, tested with `probe-edge-drawing frames` (the view's pixels made beforehand): render alone 0.8 to 0.9 ms CPU, with a new texture each frame 3.1 to 3.2 ms, so the upload costs about 2.3 ms, the build 1.4 to 1.9 ms. The higher CPU seen in some runs is in the upload: in round 3 at 129 % the three ways that upload rose together (edges 9.70, indexed 5.66, upload 5.75 ms) and the two that do not stayed (renderer 0.82, reused 0.84); what brings that state on is not known. Optimizations are Phase 2 (JOD, 2026-11-02).
- [x] T015 If JOD decides it: the tint and the outline on the drawn edges at a fractional zoom, with AS-3b's wording at 129 % (REQ-0025-05; AS-3b) - done when `overlay_sides_at_fractional_zoom` moves from `/0025-open/` to `/0025/` and passes under cairo, ngl and vulkan
  Result: decided: not done (JOD, 2026-11-02: nothing visible at 129 %). The limit is in the edge cases; `overlay_sides_at_fractional_zoom` stays under `/0025-open/` as a known-red record.
- [x] T016 AS-4: a float partly outside the picture at 129 % (REQ-0025-03; AS-4) - done when the case passes under cairo, ngl and vulkan and fails under a break
  Result: characterization: `as4_float_partly_outside` passed at its first run, since T010 had already put that part on the drawn edges (1352 device pixels outside the picture, 0 wrong, under all three). Made red by leaving that part to the renderer's scaling (77 wrong under cairo and under vulkan, the first at the picture's far edge, `ff868994` against `ff17203c`) and by moving it a pixel (1352 wrong), shown and reverted. Its mutant is in the list.
- [x] T017 The first check's findings (REQ-0001-18, REQ-0017-14, REQ-0017-16, REQ-0025-02, REQ-0025-03, REQ-0025-05, REQ-0025-06; AS-1b, AS-1e, AS-3, AS-3b) - done when each finding is settled, with its test or JOD's decision, and the tests are green under the breaks named
  Result: probe first: below 100 % on scale 2, black and white columns gave up to 263000 grey device pixels of 393750 at 75 % under cairo (all three renderers blend, none at scale 1), so REQ-0001-18 was broken there. `as1e_no_blend_below_100_at_scale_2` failed first with `wrong == 0: 43750`, core `built_by_edges_at_any_zoom_on_a_scale_above_1` with `CHECK failed: py == easel_view_cell_at (d, 75)`; green under cairo, ngl and vulkan once the view and the pointer use one rule (`easel_view_built_by_edges`). Characterizations, each passed at its first run and proven by a mutant: `as1b_far_corner_of_8000` (passed already; its cases share the code of `as1_far_corner_of_8000`), `view_without_memory_falls_back` (the four "no memory" mutants; two survived its first version, so its counts were tightened), the caret and the saved-file checks of AS-3 and the float and box of AS-3b (their mutants). Coverage values: a histogram over 5 fonts, 6 styles and 14 sizes gives no partial value on letters, and every value on a color emoji; the 102 and 154 of the survivor came from the ends of an underline under italic text, which REQ-0017-16 already names, so no defect: `as16_half_covered_underline_ends` kills both threshold mutants (64 and 192). The popover mutant of REQ-0017-14 is marked accepted (JOD, 2026-11-02; GTK 4.24 cannot kill it; re-decide at packaging-release). The long-only mutant of REQ-0014-16 was killed by hand by `as14_long_trace`, the same way twice.

Change of 2026-11-13 (JOD; a fractional screen scale, REQ-0025-07, AS-7 to AS-10; for 1.0.3).
- [ ] T018 The failing tests (REQ-0025-07, REQ-0025-06; AS-7 to AS-11) - done when core `as7_fractional_screen_rule` and the five
  `/0025-frac/` cases, with `ui-0025-scale125` and `ui-0025-scale150` and the tool's `--scale 1.25` and `1.5`, are written and seen red
  today (core at the rule; ui at 1.25 and 1.5 at the blended pixels, AS-11 at a grid line off its edge if the grid is off there; AS-10
  likely green, a characterization whose break is named in T021), and the five ui cases skip at scale 1; no product code changed
- [ ] T019 The fix (REQ-0025-07, REQ-0025-06; AS-7, AS-8, AS-9, AS-11) - done when the core rule from 100 % at any scale and `easel_view_device_shift`
  are in `view.c` with their mutants, the canvas shifts its snapshot and its pointer by it, the `easel_canvas_get_scale` calls are read for
  integer casts, T018's core test and AS-7, AS-8, AS-9 and AS-11 at 1.25 and 1.5 are green, and the regression list of the Plan is green; warning-free;
  `--lint` 0 stale
- [ ] T020 Prove exact (REQ-0025-07; AS-7) - done when the AS-7 reading has run at zooms 100, 125, 150 and 200 % on scales 1, 1.25, 1.5 and 2
  under Cairo and Vulkan and the 32 results are recorded in the Plan, each exact or with its counts; any that is not stops the work for JOD
- [ ] T021 The same pixels and the saved file (REQ-0025-07, REQ-0025-04, REQ-0025-05; AS-10) - done when `/0025-frac/as10_same_pixels_and_file`
  passes at 1.25 and 1.5 and its Result names the breaks that make it red (a drag's pixel taken from the logical position without the
  scale; the file written from the rendered view)
- [ ] T022 Measure (the budget "pointer event to pixels within one frame") - done when `perf-0025` has run in the real session at scale 1.25,
  and at 1 and 2 after JOD sets them, the working tree interleaved with `797639b`, three runs each, the numbers in the Plan
- [ ] T023 Manual M2 (AS-8) - done when JOD, at desktop scale 1.25, has drawn rectangles and a star at 100 %, 125 %, 150 % and 200 % and seen
  every outline and no line across the picture; his words in the Result
- [ ] T024 Gate of the change - done when `tools/mutate.sh --core` once, the ui mutants of REQ-0025-07 by id, the full `meson test`, the
  scale lists (2, 1.25, 1.5), `spec.sh lint` and `--names 0025`, the map brought to the names as built, `docs/engineering.md`'s runs, and
  the change marked implemented in the spec, the INDEX and `docs/phase2-plan.md` (its fractional-scale item)
- [ ] T025 Version 1.0.3 (after the gate) - done when the version is 1.0.3 in `meson.build`, `debian/changelog` (1.0.3) and the metainfo (a
  stable release entry above 1.0.2), `docs/release-notes-1.0.3.md` holds the one line "Fixed: at fractional screen scales such as 125 % and
  150 %, thin lines could look faint or missing on screen. The picture itself and saved files were not affected.", `version-sync`,
  `metainfo` and `ui-0021` pass; not pushed, no release

## Known gaps
What stays open after this spec is in `docs/phase2-plan.md`: the overlay limit at a fractional zoom (accepted), fractional desktop scales, a real screen of scale 2, the frame times of fractional zooms, two small leftovers in the code and the benchmark, and the graphics path audit.

## Check
- Date and commit: 2026-11-02, on `9c72974` with T017, the tools and the rules in the working tree. Versions: GTK 4.24.1, Pango 1.58.0, Cairo 1.18.4, GNOME Shell 51.0 (GTK, Pango and the Shell changed on 2026-11-01).
- Tests: `meson test` 54 of 54 through `tools/isolated.sh` (390 s); `--scale 2 ui-0025-scale2` 1 of 1; the cases of this feature: 14 in `/0025` and 3 in `/0025-scale2`.
- Mutants, the whole list: 1140 mutants: 1138 killed, 0 killed by crash (test gap), 0 inconclusive (no snapshot or skipped), 0 survived, 0 errors, 1 accepted, 1 long-only (not run) in 733 s. The long-only mutant of REQ-0014-16 was killed by hand by `as14_long_trace`, the same way twice.
- Warnings: 0 after a forced rebuild. `spec.sh lint`, `lint --names 0025` and `--names 0017`, `mutate.sh --lint`: clean.
- Scenarios: as in the map. C marks, each proven by the probe, the mutant or the break in its Result: T001, T002, T003 (partly), T007, T009, T011, T012, T014, T016, the AS-2 and AS-2b part of T008, and in T017 `as1b_far_corner_of_8000`, `view_without_memory_falls_back` and the new checks of AS-3 and AS-3b. T007 and T014 are measurements; T014's "done when" was not fully met (shares not obtained), as its Result says, and JOD closed it (no more perf reruns). Each task's Result is in Tasks above.
- Findings:
  - F1 Below 100 % at scale 2 the view blended, against REQ-0001-18: fixed in T017, AS-1e.
  - F2 REQ-0017-16's threshold survived (`e5bbe7e` made coverage almost all whole): the values 102 and 154 are the ends of an underline under italic text, no defect; test added in T017.
  - F3 REQ-0017-14's popover guard survives on GTK 4.24: survivor, accepted, JOD 2026-11-02, re-decide at packaging-release.
  - F4 AS-1b lacked the far corner: added in T017. F5 Aborting allocations in the view paths: soft, with a fallback, in T017. F6 Weak spots in AS-3 and AS-3b: fixed in T017.
  - F7 REQ-0025-03's polygon wording and the far-edge handles' limit: worded (JOD, 2026-11-02). The limit stays open in `docs/phase2-plan.md`.
  - F8 `tools/isolated.sh` let the session's services see the real accounts: fixed, with `tools/check-isolated-private.sh`.
  - F9 `docs/engineering.md` was stale on scale 2, Vulkan, the skip counted as OK and the suite's time: fixed.
