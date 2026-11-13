# 0043 Extra shapes

Status: implemented
Reviewed: spec-reviewer, 2026-11-11 (27 findings: 24 merged as routine; three asked: a click makes nothing, the outline inside the box, Space moves a waiting shape)
Depends on: 0006, 0010, 0011, 0013, 0015, 0018, 0022, 0029, 0039

## Why
XP Paint draws four kinds of shape; people making diagrams, labels and simple pictures also want triangles, arrows, stars,
speech bubbles and a heart without building them corner by corner with Polygon. One Shapes tool holds them all, Rectangle,
Rounded Rectangle and Ellipse with them, so the toolbox stays short (JOD, 2026-11-11). A shape that waits with handles after it is
drawn can be put right (moved, resized, recolored) before it becomes pixels, which saves an Undo and a redraw (JOD, 2026-11-11).

## Requirements
Pointer notes to add at acceptance:
- Rectangle, Rounded Rectangle and Ellipse become cells of Shapes, and the order: REQ-0006-01 (authoritative for the order;
  REQ-0010-01, REQ-0011-01, REQ-0012-01, REQ-0013-01 and REQ-0014-01 repeat it), REQ-0010-03, REQ-0010-04, REQ-0011-02,
  REQ-0029-05 (their icons), REQ-0030-05, REQ-0039-01 and 0022's AS-6.
- Shapes' options, cursor, hint and Shift: REQ-0010-02, REQ-0010-14, REQ-0010-15, REQ-0010-16.
- The shape waits: REQ-0010-05, -06, -08, -09, -18, -19; REQ-0002-06 and -07 (Redo while it waits); REQ-0004-16, REQ-0015-01,
  REQ-0015-02 (its handles); REQ-0013-06 (the size field); REQ-0018-11 (mirroring); REQ-0016-05, REQ-0026-12 (Paste and Print
  put it down); REQ-0022-02 and -03 (Enter puts it down), REQ-0022-15 (announcements); REQ-0039-02, -05, -08, -09.
- `docs/product.md` (Scope: Shapes holds rectangle, rounded rectangle and ellipse; a named exception to P1, JOD, 2026-11-11),
  `docs/reference/new-behaviors.md` (the merge, waiting with handles), `docs/design.md` (16 tools; the waiting shape in Pending
  state; Esc drops a waiting shape).

### The tool and its options
- REQ-0043-01 The toolbox shall show Shapes in the place of Rectangle, Rounded Rectangle and Ellipse, which leave the
  toolbox and become shapes of Shapes (JOD, 2026-11-11), with the tooltip "Shapes". The order of REQ-0006-01 becomes Pan,
  Magnifier, Select, Free-Form Select, Eraser, Pick Color, Fill, Gradient, Pencil, Brush, Airbrush, Text, Line, Curve, Shapes,
  Polygon: 16 tools in two columns, eight rows, the last Shapes / Polygon. Its icon is the outline of the chosen shape (REQ-0043-16),
  so the button shows which shape a drag will draw. Clicking it makes it the active tool (REQ-0001-09).
- REQ-0043-02 While Shapes is active, the options panel shall show first a well holding one button the well's width, showing
  the chosen shape's outline (REQ-0043-16) and a downward arrow, tinted as a chosen cell is (REQ-0030-17); its tooltip is
  "Shape" and its accessible name "Shape:" followed by the chosen shape's name (JOD, 2026-11-11: layout B of
  `docs/reference/proto-0043/panel-mockup.png`). A click on it, or Space with the focus on it (or Enter when no shape waits,
  REQ-0022-02), opens a popover beside it, toward larger x, holding 19 cells five to a row: Rectangle, Rounded Rectangle,
  Ellipse, then the 16 of `docs/reference/proto-0043/corners.md` in its order (Triangle, Right Triangle, Diamond, Pentagon,
  Hexagon, Right Arrow, Left Arrow, Up Arrow, Down Arrow, 4-Point Star, 5-Point Star, 6-Point Star, Rounded Rectangle Callout,
  Oval Callout, Heart, Lightning Bolt; JOD, 2026-11-11: list A of `docs/reference/proto-0043/lists-sheet.png`), each its
  outline (REQ-0043-16), its tooltip and accessible name the shape's name, the chosen one shown as REQ-0030-17 shows a chosen
  cell. A click on a cell, or Space or Enter on it (the arrow keys move the focus between the cells; the popover holds the
  keys as a menu does, so Enter there chooses the cell), chooses that shape and closes the popover, the focus back on the
  button; choosing the shape already chosen closes it and changes nothing. Esc, a click outside it or the window losing
  focus closes it with no change, and the press that closes it does nothing else (it starts no shape and puts nothing down).
  While it is open the keys of REQ-0021-01 do nothing, as with a menu open. At each start of the app the chosen shape is
  Rectangle; it is kept for the session when the tool changes, as the width is (REQ-0010-03), and is not a setting. Below
  that well come the fill styles, the widths and the Sharp | Smooth well, as for Rectangle before (REQ-0010-02,
  REQ-0039-01): the same shared choices (REQ-0010-03, REQ-0039-01), Numpad + and − stepping the width (REQ-0010-04). A
  shape is chosen only so: no shortcut. With the window at its least size in the long test language the toolbox and the
  panel keep REQ-0023-11.
- REQ-0043-03 The status bar hint (REQ-0004-09) for Shapes shall be "Draws the chosen shape with the selected fill style.",
  whichever shape is chosen (it replaces REQ-0010-16's hints of the three tools). Away from a waiting shape the cursor over
  the canvas area is the crosshair of REQ-0010-15 (over a picture handle its resize cursor, REQ-0004-17); on a waiting
  shape, REQ-0043-07's cursors.
- REQ-0043-16 The outline of a shape on the toolbox button, the Shape button and a popover cell shall be drawn in a 16 x 16
  box in the weight and look of REQ-0029-05 and REQ-0029-07 (1.5-pixel lines, symbolic, in the theme's text color), in the
  proportions of `docs/reference/proto-0043/lists-sheet.png`; Rectangle, Rounded Rectangle and Ellipse keep the icons they had
  as tools. The 16 new outlines are drawn from scratch, not adapted from any icon set (JOD, 2026-11-11). The sheet of all 19 at
  real size, light and dark, is shown to JOD and approved in the first task that draws them; that sheet is a local file, not
  committed, and only the approved final icons go into the repository.

### The shapes' pixels
- REQ-0043-04 A Shapes drag shall show the chosen shape in the box of REQ-0010-11 (from the outer corner of the start pixel
  to the outer corner of the end pixel; a drag toward smaller x or y gives the same box as the other way and does not mirror),
  with Shift a square box of side the smaller of its width and height, from the start toward the end, as REQ-0010-14 says for
  Rectangle. The box is at most 10000 x 10000: the end stops there. Rectangle, Rounded Rectangle and Ellipse are drawn in the
  box by their rules (REQ-0010-11 to REQ-0010-13, REQ-0039-05 (a)); mirroring leaves them as they are. Each of the other 16 is
  the polygon whose corners are its corners of `docs/reference/proto-0043/corners.md` (normative: it changes only by a change
  of this spec), in their order, placed in the box made smaller by the width w (JOD, 2026-11-11: the outline stays inside the
  box): for a box W x H pixels wide and high whose top-left pixel is (x0, y0), the inner box starts at the pixel
  (x0 + ⌊(w − 1) / 2⌋, y0 + ⌊(w − 1) / 2⌋) and is W' = W − (w − 1) pixels wide and H' = H − (w − 1) high; where W' would be
  less than 1 it is 1 pixel at x0 + ⌊(W − 1) / 2⌋, and so for H'. A corner (u, v) is the pixel at x' + p(u, W') of the
  inner box's left x', where p(u, N) = ⌊(u (N − 1) + 500) / 1000⌋ for u ≤ 500 and (N − 1) − ⌊((1000 − u)(N − 1) + 500) / 1000⌋
  for u > 500, and likewise y' + p(v, H'), all in whole numbers, so that a mirrored shape is the pixel mirror of the unmirrored
  one (except a corner at u = 500 in an even width, which no rule can center); u is taken as 1000 − u while the shape is
  mirrored left to right, v as 1000 − v while it is mirrored top to bottom (REQ-0043-08).
- REQ-0043-18 Rectangle, Rounded Rectangle and Ellipse, drawn in a given box with a given width, fill style, colors and
  smoothing, shall give pixels identical, bit for bit, to those the Rectangle, Rounded Rectangle and Ellipse tools give today
  (REQ-0010-11 to REQ-0010-14, REQ-0039-05 (a)); the inset of REQ-0043-04 does not apply to them. Their existing core tests
  pass unchanged and are not re-goldened (JOD, 2026-11-11). If any rule of this feature would move one of their pixels, the
  build stops and JOD decides: that is a P7 decision.
- REQ-0043-05 With Sharp, the shape's pixels shall be those of the polygon through those corners as REQ-0011-08 makes them
  (the outline: the Lines of its sides with the width w, the closing side included; the region by the alternate rule), in
  the fill style's parts and colors (REQ-0010-13), so it is the very polygon the Polygon tool draws through the same corners,
  and its outline lies inside the box when W and H are at least w. With Smooth, they are its ideal shape of REQ-0039-05 (d)
  through those corners, by coverage (REQ-0039-04, REQ-0039-06, REQ-0039-07); its outline may reach 1/2 pixel beyond the box.
  Only the shape's pixels inside the picture are shown and drawn.

### Waiting with handles
- REQ-0043-06 (Pointer note, change of 2026-11-13: the waiting shape's box is also outlined by the selection's dashed line, REQ-0043-19;
  the handles are as here.) (Changes REQ-0010-05, -08, -09, -18 and -19 for Shapes; JOD, 2026-11-11.) A Shapes press that never leaves its
  pixel before its release makes nothing (JOD, 2026-11-11: no 1 x 1 shape from a click); "a move" below is the pointer
  leaving the press's pixel. When the button that drags out a shape (one that moved) is released, the app shall not draw it
  yet: the shape waits. A first drag whose release is lost (REQ-0007-18) or during which the window loses focus ends as last
  shown, and the shape then waits; a press that shows the release was lost then acts as a press of its own. Save or Quit
  during the first drag ends it as last shown, puts the shape down and proceeds. Esc, the other button or Undo during the
  first drag cancels it as REQ-0010-08 says: nothing waits. While a shape waits it is shown over the picture exactly as it
  will be put down (P7.3), the picture is unchanged and not marked modified, and eight handles stand round its box with the
  look, places and drawing order of REQ-0015-01 (the picture's own handles under the shape and its handles). The status bar's
  size field (REQ-0013-06) shows the box's "W × H" while it waits and during its drags, and is cleared when it is put down or
  dropped. It waits with no time limit and no timer; View actions, scrolling, zooming and the window losing focus keep it
  waiting and shown. Redo is not available while it waits (as REQ-0011-11). A shape made with a keyboard press
  (REQ-0022-10) waits as well. Line, Curve and Polygon are unchanged.
- REQ-0043-07 While a shape waits, a press shall go: to a handle, in its hit area of REQ-0015-02 (its 5 x 5 square, else
  9 x 9 logical pixels round its center less the box, a corner handle winning over an edge handle, a shape's handle winning
  over the picture's resize handles where their hit areas overlap and owning a picture handle's whole hit area where its
  5 x 5 square covers that handle's square); else, inside the box, to the shape (a picture handle inside the box cannot be
  pressed while the shape waits); else away. The cursor is the handle's resize cursor of REQ-0015-02 over a handle, the move
  cursor (REQ-0013-03) inside the box, and REQ-0043-03's cursor away. The movement of a drag is the pixel under the pointer
  less the pixel under the press, also after a scroll during it (as REQ-0013-07).
  - A primary press on the shape and a drag shall move its box by that movement; the drag goes on outside the canvas area
    and the window and the view does not scroll by itself (REQ-0010-21). A move keeps at least one pixel of the box inside
    the picture. Shift and Ctrl change nothing for it.
  - A primary press on a handle and a drag shall resize it (REQ-0043-08).
  - A press away, with either button, shall put the shape down (REQ-0043-10), then act as a press of the active tool with
    nothing waiting: a drag from it makes the next shape, which then waits; one that does not move makes nothing more (JOD,
    2026-11-11). Esc, the other button or Undo during that new drag cancels only the new shape (REQ-0010-08): the put-down
    stays, and that Undo does not undo it. If the put-down fails (REQ-0043-13), the dialog shows at the release and no new
    shape starts. A primary press away on a picture resize handle puts it down and starts that resize; a secondary press on a
    picture handle does nothing and keeps it waiting (REQ-0004-16).
  - A secondary or middle press on the shape or on one of its handles, and a middle press away, do nothing.
  - A keyboard press (REQ-0022-10) never takes a handle (REQ-0022-07). Inside the box it moves the shape as a primary press
    does, the arrows moving it while it is held (JOD, 2026-11-11); away it puts the shape down and starts the next, as a press
    away does. With no keyboard press held, the arrows move only the marker (REQ-0022-08).
- REQ-0043-08 A handle drag shall move the edges the handle stands for by the movement, the opposite edges staying where they
  are, as REQ-0015-03 says for a selection; each side is kept within 1 to 10000 pixels. A handle dragged past the opposite edge
  mirrors the shape on that axis (JOD, 2026-11-11), as a selection is mirrored (REQ-0018-11): the box then runs from the
  opposite edge the other way, and the corners are taken mirrored (REQ-0043-04), so a Right Arrow dragged past its left edge
  is the Left Arrow of that box; a second mirror on an axis undoes the first, and the mirror is kept when another shape is
  chosen. While Shift is held a corner handle keeps the ratio of width to height as REQ-0015-07 says; Shift changes nothing
  for an edge handle. The shape is made again from its corners at every size (never scaled from its pixels), so making it
  smaller and then as large as before gives the same pixels back.
- REQ-0043-09 While a shape waits (no button held), choosing a width, a fill style, Sharp | Smooth or another shape, Numpad +
  or −, and changing the foreground or the background color (a palette click, the swap, the color chooser's OK) shall redraw
  it at once with the new choice (JOD, 2026-11-11); choosing what is already chosen changes nothing. Its colors are those of
  REQ-0010-06 for the button that dragged it out, taken from the colors current now, not those at the press (changes
  REQ-0010-06 for a waiting shape). During its first drag, a move or a handle drag, a color change is made at once (REQ-0003)
  but the shape keeps the colors of that drag's press until the release, then takes the colors current then; widths, fill
  styles, Sharp | Smooth, Numpad + or − and shapes chosen during such a drag are ignored (REQ-0010-08).
- REQ-0043-19 (Change of 2026-11-13; JOD, 2026-11-13.) While a shape waits, the app shall outline its box with the line of a
  rectangle selection, the line a text box has too (REQ-0013-05, REQ-0014-17 (a) and (c), REQ-0017-05): white with black dashes,
  just outside the box's pixels on all four sides, one line counted clockwise from the top-left corner of the box as shown (also
  when the shape is mirrored; after a handle drag, from the new box's top-left corner), its thickness and the length of its
  dashes and gaps as REQ-0014-17 (a) gives them at the zoom; at a new zoom it takes that zoom's thickness and dash length and its
  dashes are counted again. It lies over the shape, the picture, the grid, the picture's resize handles and the canvas area (where
  the box lies outside the picture), under the shape's eight handles (REQ-0043-06, unchanged), and scrolls and moves with the box.
  Its dashes move as a selection's while no drag of the shape is held. They stand still during the shape's first drag, a move
  drag and a handle drag, by the mouse or by a keyboard press (REQ-0022-10); a press that does nothing (REQ-0043-07) changes
  nothing. When the user's settings turn animations off they never move, and the first dash starts the line. During the first
  drag, once it has left its press's pixel, the line is shown round the box, over the canvas area too, as round a selection's
  rectangle while it is dragged out (the handles come at the release, REQ-0043-06). The line goes when the shape is put down or
  dropped, or its first drag is cancelled. No tint is drawn over the box (REQ-0014-17 (b) does not apply). The line is not part
  of the picture: no pixel of it enters the picture, a put-down, the history, a saved file or a printed page.

### Putting down and dropping
- REQ-0043-10 A waiting shape shall be put down as shown, as one undo step that marks the picture modified (REQ-0010-07: no
  step and no mark if it changes no pixel, the redo steps then kept; only the pixels inside the picture), its handles removed,
  when: the user presses Enter (also with the focus on a panel cell or a swatch, REQ-0022-02 and -03); presses away from it
  (REQ-0043-07); or does any of the things that put a waiting curve or polygon down (REQ-0011-12, with its rules on cancelled
  prompts, Quit and the redo steps): another tool chosen (choosing the active tool again keeps it waiting), Save, Save As,
  New, Open, Quit, Print, Paste, Paste From, an Image action, a picture resize drag, or anything else that changes the
  picture or its file.
- REQ-0043-11 Esc and Undo while a shape waits (no button held) shall drop it: nothing is drawn, no step is added, and the
  history is as it was, its redo steps included; that Undo undoes nothing else (as REQ-0011-11). While a shape waits, Undo is
  available even with no step to undo. While a dialog, the quit prompt or the color chooser is open, Esc and Enter act on that
  and presses on the canvas are ignored (REQ-0010-17); the shape keeps waiting.
- REQ-0043-12 During a move or handle drag of a waiting shape: Esc or a press of the other button shall cancel that drag, the
  shape back as it was at its press, still waiting; Undo shall drop the shape (REQ-0043-11), the release afterwards doing
  nothing more (REQ-0002-09). Enter, Delete, Select All, Redo, the arrows, another tool, the View, Image and clipboard actions
  change nothing during it (as REQ-0015-08). A lost release (REQ-0007-18) or the window losing focus ends the drag as last
  shown, the shape still waiting; a press that shows the release was lost then acts as a press of its own. Save, Save As,
  New, Open and Quit during the drag end it as last shown, put the shape down and go on. Scrolling during it acts as during a
  shape drag, and Ctrl+wheel and the pinch do not zoom (REQ-0010-21).

### Failure, announcements and the pixel contract
- REQ-0043-13 If the memory to show, redraw or put down a shape cannot be had, then the app shall show the dialog "Could Not
  Draw the Shape" of REQ-0010-20 when no button is held (at the release if one is), drop the shape (nothing waits) and change
  nothing else: neither the picture nor the history (as REQ-0011-15). The action that asked for the put-down then goes on,
  except Print, which stops (REQ-0026-12); a step larger than the whole budget is kept as the only step (REQ-0002-17).
- REQ-0043-14 (Pointer note, change of 2026-11-13: "the handles are not part of the picture" holds for the dashed line of REQ-0043-19
  too.) The pixel contract (P7): (1) every pixel is whole and exactly the part's color, with Sharp; with Smooth the mix
  of REQ-0039-06 (smoothing, opt-in, as P7.1 allows); (2) the view only magnifies; the handles are not part of the picture;
  (3) the shown waiting shape and the shape put down are the same pixels, made the same way; (4) the same corners, box,
  mirroring, width, fill style, colors and smoothing always give the same pixels: the corners come from exact whole-number
  arithmetic (u, v ≤ 1000, W, H ≤ 10000, so each product is below 10 000 000), the box's pixels within 10000 of the picture.
- REQ-0043-15 "Shapes", "Shape", its hint, the 16 new shape names and the announcements of REQ-0043-17 shall be translated in
  every language of REQ-0023-01 (Rectangle, Rounded Rectangle and Ellipse keep their translated names, now as shapes), and
  Help (REQ-0021-03) shall describe the toolbox as it now is in every language.
- REQ-0043-17 The app shall announce (REQ-0022-15) a shape that starts waiting ("Shape W by H"), one put down ("Shape put
  down") and one dropped ("Shape removed"), as it announces a selection.

## Scenarios
Unless said otherwise: a 40 x 30 white picture (#FFFFFFFF), foreground #000000FF, background #FFFF00FF, width 1, Outline,
Sharp, 100 %, Shapes active. "Polygon P" means the picture the Polygon tool draws there through the given corners, with the
same width, fill style, colors and smoothing, closed.
- AS-1 (REQ-0043-01, REQ-0043-02, REQ-0043-03) Given the app just started, Then the toolbox is 16 tools in eight rows, Line / Curve then Shapes / Polygon, with no Rectangle, Rounded Rectangle or Ellipse button, Shapes' tooltip "Shapes" and its icon the Rectangle's outline; When the user clicks it, Then it is the only selected tool, the panel shows the Shape button with the Rectangle's outline, then the fill styles, widths and Sharp | Smooth, and the status bar shows the hint; When the user clicks the Shape button, Then the popover shows the 19 cells five to a row in the listed order with their names as tooltips, Rectangle chosen; When the user clicks Heart, Then the popover closes, Heart is chosen, and the button and the toolbox icon show the Heart's outline; When the user chooses Pencil and Shapes again, Then Heart is still chosen; When the user opens it by the keyboard (Space on the button), moves to Lightning Bolt with the arrows and presses Enter, Then Lightning Bolt is chosen and the focus is on the button; When the user opens it and clicks Lightning Bolt again, presses Esc, or clicks on the canvas, Then it closes, nothing changes and no shape is started; Given the long test language and the window at its least size, Then the toolbox and the panel keep REQ-0023-11 | Verified by: automated (ui)
- AS-2 (REQ-0043-04, REQ-0043-05, REQ-0043-06, REQ-0043-17) Given Triangle, When the user drags from (10, 10) to (20, 18) and releases, Then the picture is unchanged, the shape shows with eight handles round the 11 x 9 box, the status bar "11 × 9", "Shape 11 by 9" is announced, and the shown pixels are Polygon P through (15, 10), (20, 18), (10, 18); When Enter, Then the picture is Polygon P, one step, the title marked, the size field cleared; Given width 5, When the same drag, Then Polygon P through (15, 12), (18, 16), (12, 16), every pixel inside the box; When the user clicks at (30, 5) with nothing waiting and no move, Then nothing is made | Verified by: automated (ui)
- AS-3 (REQ-0043-04, REQ-0043-05, REQ-0043-14, REQ-0043-18) Given each of the 16 shapes of the table, in boxes 1 x 1, 2 x 2, 11 x 9 and 101 x 61, mirrored on each axis or not, Sharp and Smooth, widths 1 and 5, every fill style, Then the pixels equal Polygon P through the corners REQ-0043-04 computes, the same input twice gives the same pixels, a mirrored shape at odd sizes is the pixel mirror of the unmirrored one, and with Sharp no pixel lies outside a box of at least w x w; Rectangle, Rounded Rectangle and Ellipse, mirrored or not, equal the pixels of REQ-0010-11 to -13 and REQ-0039-05 (a) in the box, and the existing core tests of 0010 and 0039 pass with their expected pixels unchanged | Verified by: automated (core)
- AS-4 (REQ-0043-07, REQ-0043-08) Given a Right Arrow waiting in the box (10, 10) to (20, 14), at 400 %, When the user drags the shape from its pixel (12, 12) to (17, 15), Then the box is (15, 13) to (25, 17); When the user drags its right handle 15 pixels to smaller x, Then the box is (11, 13) to (14, 17) mirrored, and the shown pixels equal the Left Arrow drawn in that box unmirrored; When the user drags it back, Then the first pixels come back; Given the box (15, 13) to (25, 17), When the user drags its bottom-right handle by (11, 1) with Shift, Then the box is 22 x 10; When the user drags the shape far to larger x, Then at least one column of its box stays in the picture; Given a drag from (20, 14) to (10, 10), Then the arrow is not mirrored | Verified by: automated (ui)
- AS-5 (REQ-0043-09) Given a Rectangle waiting, When the user chooses width 3, Outline and fill, Smooth, then clicks red in the palette, Then the waiting shape is redrawn after each, its outline red; When the user chooses width 3 again, Then nothing changes and it waits; Given a Triangle waiting, When the user chooses Heart, Then the Heart waits in the same box; When Enter, Then the picture is the red Heart, one step; Given a move drag held, When the user clicks red in the palette, Then the foreground is red and the shape keeps its colors until the release, then shows red; When the user chooses width 5 during the drag, Then the width stays as it was | Verified by: automated (ui)
- AS-6 (REQ-0043-07, REQ-0043-10, REQ-0043-17) Given an Ellipse waiting in (5, 5) to (15, 12), When the user presses away at (25, 20) and drags to (30, 25), Then the ellipse is put down (one step, "Shape put down") and a second ellipse waits in (25, 20) to (30, 25); When Esc during that drag instead, Then the first stays put down and nothing waits; When the user clicks away at (5, 25) with no move, Then the waiting one is put down and nothing more waits; Given a shape waiting, When the user chooses Pencil, Save, New, an Image action, Paste, or starts a picture resize, Then it is put down first; When the user chooses the active tool again, Then it keeps waiting; Given the focus on a width cell and a shape waiting, When Enter, Then the shape is put down and the width unchanged | Verified by: automated (ui)
- AS-7 (REQ-0043-11, REQ-0043-17) Given an earlier edit and a shape waiting, When Esc, Then nothing waits, "Shape removed" is announced, the picture and history as before, Redo as before; Given the same, When Undo, Then the shape is dropped and the earlier edit is still there; When Undo again, Then the earlier edit is undone; Given an undone edit and a shape waiting, Then Redo is not available, and after Esc it is | Verified by: automated (ui)
- AS-8 (REQ-0043-12) Given a shape waiting, When a handle drag is held and the user presses Esc, or the other button, Then the shape is as before that press and still waits; When a move drag is held and the user chooses Undo, Then the shape is dropped and the release does nothing; When during a handle drag Enter, Delete, Redo or another tool is chosen, Then nothing changes; When a handle drag's release is lost (the next motion with no button), or the window loses focus during it, Then the drag ends as last shown and the shape waits; When Save is chosen during a move drag, Then the drag ends as shown, the shape is put down and the picture saved; When the view is scrolled by the wheel during a handle drag, Then at the next motion the dragged edge is at the pixel under the pointer, and Ctrl+wheel does not zoom | Verified by: automated (ui)
- AS-9 (REQ-0043-07) Given a Rectangle waiting in (20, 0) to (39, 29) (its right handle on the picture's right handle, both centered on y = 15), at 100 %, Then a press on its right handle resizes the shape, not the picture; a secondary press on the shape does nothing; a middle press away does nothing; the cursor is the resize cursor over its handles, the move cursor inside the box and the crosshair at (5, 5) | Verified by: automated (ui)
- AS-10 (REQ-0043-13) Given memory made too small for the put-down, When Enter on a waiting shape, Then "Could Not Draw the Shape" shows, nothing waits and the picture and history are unchanged; When the same by Print, Then Print does not go on; When by a press away and a drag, Then the dialog shows at the release and no new shape is started; Given memory too small to redraw, When the width is changed on a waiting shape, Then the same dialog and nothing waits | Verified by: automated (ui)
- AS-11 (REQ-0043-06, REQ-0043-14) Given a Smooth 5-Point Star waiting over a red picture, at 100 %, a scrolled 400 % and 25 %, Then the picture's pixels under it show as the put-down will make them, and after Enter the picture equals what was shown | Verified by: automated (ui)
- AS-12 (REQ-0043-15) Given each language, Then "Shapes", "Shape", its hint, the 19 names and the announcements are translated (no English left), and Help names Shapes and not the three old tools | Verified by: automated (ui)
- AS-13 (REQ-0043-06, REQ-0043-07) Given Rectangle and the keyboard marker on the canvas, When Space is held, the marker moved by 5 and 3, and Space released, Then a rectangle waits with handles; When Space is held inside its box and Right pressed twice, Then the shape moves by (2, 0); When Space is pressed and released away with no move, Then it is put down and nothing more waits; Given a shape waiting and no Space held, When Right, Then only the marker moves | Verified by: automated (ui)
- AS-14 (REQ-0043-01, REQ-0043-02, REQ-0043-06, REQ-0043-16) Manual M1: JOD approves the sheet of the 19 outlines at real size, light and dark; draws each shape, moves, resizes and mirrors it, changes its color and width while it waits, and puts it down by a click away and by Enter; the toolbox and the panel look right in light and dark style and at the window's least size | Verified by: manual M1
- AS-15 (REQ-0043-19, REQ-0043-14) Given Triangle at 400 % at display scale 1, When the user drags from (10, 10) to (20, 18) and holds the button, Then the line is shown round the 11 x 9 box and, at two times 0.3 s apart, its dashes are in the same places; When the user releases, Then a line 1 logical pixel thick lies just outside the box on each of its four sides (in logical pixels from the picture's top-left corner: the rows 39 and 76 across the columns 39 to 84, the columns 39 and 84 down the rows 39 to 76), each of its pixels white or black, with black on each side in runs of 4 logical pixels except where a dash turns a corner or meets the line's start; the box's pixels outside the triangle show the picture's white with no tint; the eight handles show over the line; When 0.3 s pass with no button held, Then the black dashes are in other places; Given a handle drag held, or animations off, Then at two times 0.3 s apart they are in the same places, with animations off the first dash at the box's top-left corner; When the zoom becomes 800 %, Then the line is 2 logical pixels thick just outside the box, its dashes and gaps 8 long; Given a shape waiting in (35, 25) to (45, 32), Then the line shows over the canvas area beyond the picture's right and bottom edges, and when the view is scrolled it keeps its place round the box; Given the first shape, When Enter, Then the picture is Polygon P through (15, 10), (20, 18), (10, 18), every picture pixel under the line's place (the columns 9 and 21, the rows 9 and 19) still white, one step; Given width 5 and a Triangle in (10, 10) to (12, 12), whose outline reaches beyond its box under the line, When Enter, Then the picture equals Polygon P there, with no pixel of the line in it; When the picture is saved as PNG and opened again, Then its pixels are those of the picture | Verified by: automated (ui, a rendered window) + manual M2 (JOD compares the line round a waiting shape with a rectangle selection's at 100 % and 800 %, light and dark)

## Edge cases
- The dashed line round a box narrower or lower than w, or round a Smooth shape, whose soft edge may reach half a pixel beyond any
  box (REQ-0043-05, REQ-0043-19): the shape's pixels under the line are hidden by it while it waits, and put down as computed,
  without the line.
- A 1 x 1 box (a drag that left its pixel and came back): every corner is the one pixel; with Sharp the w x w square of the
  drawing color at it (REQ-0011-08's Line of one point), with Smooth the disc of diameter w at its center, by coverage.
- A box narrower or lower than w: its inner box is 1 pixel at the middle, and the outline may reach beyond the box.
- A shape partly or wholly outside the picture (a first drag may start outside it): its handles may lie outside the picture
  in the canvas area; only the pixels inside are shown and put down; one wholly outside puts down no pixel, no step.
- A box larger than the picture: allowed while it waits, up to 10000 x 10000.
- Choosing Select, Free-Form Select, Pick Color, Magnifier or Pan while a shape waits is a tool change: it is put down first.
- A floating selection cannot exist while a shape waits: making a shape needs Shapes, and choosing it puts the selection down
  (REQ-0013-11).
- Shift pressed or released during the first drag changes the box at once (REQ-0010-14).

## Out of scope
- Resizing a waiting shape from the keyboard: Known gap G1.
- Rotating shapes; text inside a callout; Cloud Callout and other shapes; a custom shape.
- Waiting with handles for Line, Curve and Polygon.
- Keeping the chosen shape between runs.

## Known gaps
- G1: a waiting shape cannot be resized from the keyboard: a keyboard press never takes a handle (REQ-0022-07), and a key for
  each of eight handles, or a mode that picks one, is a design of its own, as is announcing the size as it changes. A keyboard
  user can move a waiting shape (Space and the arrows, REQ-0043-07), put it down or drop it, and draws the size wanted from
  the start. Known gap (JOD, 2026-11-11); also in `docs/phase2-plan.md`.
- G2: while a shape waits with animations on, the shape is drawn again at each step of its moving dashes (12 a second): up to about
  18 points of a core more than a selection's moving line, 32 % in all for a large Smooth shape in the isolated session (0043 Check of
  2026-11-13, F2); no pixel changes. Also in `docs/phase2-plan.md`.

## Open questions
None.

## Plan
(Opus, 2026-11-12.) Read with `docs/engineering.md`, `docs/design.md`, 0010, 0011 (the waiting curve and polygon), 0012 (a waiting gradient whose markers are dragged), 0015 and 0018 (the handles, the mirror), 0022 (the keyboard press), 0039 (Smooth) and the code they built.

Approach.
- **Core: one new module, `src/core/shapes.{c,h}`** (libc only). `EaselShapeKind` (the name `EaselShape` is the brush footprint's), the 19 shapes in the popover's order; the corner table of `corners.md` as static data (at most 96 corners); `easel_shape_box` (the start and end pixels, Shift, the 10000 limit → the box x, y, W, H, REQ-0043-04); `easel_shape_figure` (a shape, its box, its mirror flags, the width, the fill style and Smooth → the `EaselFigure` that draws it, with its corners in a buffer the caller gives). Rectangle, Rounded Rectangle and Ellipse give the figure of their old kind from (x, y) to (x + W − 1, y + H − 1), unmirrored, no inset (REQ-0043-18); the 16 give `EASEL_FIGURE_POLYGON`, closed, through the corners placed by REQ-0043-04's inner box and p(u, N), u taken as 1000 − u when mirrored left to right, v likewise. So the pixels, Sharp and Smooth, the preview and the put-down are those of `figure.c`, unchanged (P7.3, REQ-0043-05, REQ-0043-14): no new rasterizer. Its products are below 10 000 000 (u ≤ 1000, N ≤ 10000), in `int`.
- **Canvas: the waiting shape is a fourth kind of pending figure** (`pend_shape`, beside the curve, the polygon and the gradient), so every put-down and drop path that REQ-0011-12 and REQ-0011-11 already go through applies as it is: `easel_canvas_put_down` (tool change, Save, New, Open, Quit, Print, Paste, the Image actions), `easel_canvas_enter`, `easel_canvas_drop` (Esc, Undo), `easel_canvas_is_pending` (Redo off, Undo on with no step). The one function every put-down goes through is `put_down_pending`; for a shape it applies `easel_shape_figure`'s figure with the colors of now (REQ-0043-09) by `easel_figure_apply_capped`. State: the chosen shape, the box (x, y, W, H), two mirror flags, the button that dragged it out (its colors are read from the foreground and background at each draw), and, while a move or handle drag is held, the box, mirror and colors at its press (to cancel it, REQ-0043-12, and to keep its colors, REQ-0043-09). The first drag is the existing one-drag path of Line (`shaping`, `update_shape_end`, Shift by `easel_figure_constrain` as for Rectangle); its end (`end_shape`) for Shapes makes the box and sets the shape waiting instead of drawing it; a drag that never left its pixel makes nothing (REQ-0043-06). A lost release, focus loss, Save and Quit already end `shaping` as last shown; `easel_canvas_put_down` ends a held first drag first, so the shape waits and is then put down.
- **The handles**: `easel_canvas_get_selection_handle_rect` and `easel_canvas_selection_handle_at` read the box of a waiting shape while Shapes is the tool, as they read the text box with Text (REQ-0015-01, -02; its rule that a handle on a picture handle owns that handle's area is already there); `append_selection_handles` draws them. A move drag is the selection's movement rule (pixel under the pointer less pixel under the press, REQ-0013-07), clamped so that one pixel of the box stays in the picture. A handle drag uses the core's `easel_stretch_rect_mirror` (0018: the edges, 1 to 10000, the mirror, Shift's ratio of REQ-0015-07) and turns its mirror result into a flip of the flags. Cursors in `update_cursor` as for a selection. The press dispatch: on a waiting shape, a handle, then inside the box, then away (put down, then the press as a new one, REQ-0043-07); the picture-handle branch that a waiting curve uses (`pending && !pend_held && handles_shown`) stays for a press away.
- **Window**: `EaselTool` loses `RECTANGLE`, `ROUNDED_RECTANGLE` and `ELLIPSE` and gains `EASEL_TOOL_SHAPES` ("shapes"; `easel_tool_draws_shape` and `easel_tool_uses_line_width` say TRUE for it). A stateful action `win.shape` (a string: "rectangle", "rounded-rectangle", "ellipse", "triangle", ... "lightning-bolt"; "rectangle" at each start, not a setting) is the one source of the chosen shape: the popover's cells use it as their action, its change sets the canvas's shape (`easel_canvas_set_shape`) and the icons of the toolbox button and of the Shape button. The toolbox in `window.ui` is reordered to REQ-0043-01. The hint (REQ-0043-03), the tool's name in announcements. The size field and the three announcements of REQ-0043-17 from a new canvas signal `shape-changed` (kind: waiting with W and H, put down, removed), emitted where `pending-changed` is, for a shape. A failure is the existing `shape-failed` (REQ-0010-20's dialog), emitted at a release when a button is held.
- **Options panel** (`tool-options.c`): for Shapes, the Shape well first: one button the well's width (its outline icon, a down arrow, the chosen tint of REQ-0030-17), opening a `GtkPopover` at `GTK_POS_RIGHT` that holds a 5-column grid of 19 toggle buttons on `win.shape`. Its visibility is added to `menu_is_open` (REQ-0021-01's keys do nothing while it is open). Then the existing fill styles, widths and Sharp | Smooth.
- **Icons**: 16 new symbolic SVGs, `src/ui/icons/scalable/actions/easel-shape-<name>-symbolic.svg`, 16 x 16, 1.5-pixel strokes in the proportions of `lists-sheet.png`, drawn from scratch; Rectangle, Rounded Rectangle and Ellipse reuse their tool icons. A sheet of the 19 at real size, light and dark, is rendered from the app's own resources into `.scratch/` for JOD before anything else uses them.
- **Strings**: "Shapes", "Shape", the hint, the 16 names, "Shape %d by %d" (plural-free: two numbers), "Shape put down", "Shape removed", the Shape button's accessible name "Shape: %s"; eleven languages; Help's toolbox section in twelve pages.
- **Tests that name the old tools** (about 40 ui cases, `perf-0010`, `perf-0039`, `probe-screenshot`): a helper `use_shapes (win, "ellipse")` sets the tool and `win.shape`; a 0010 or 0039 case that expected the release to draw now presses Enter after it (REQ-0010-05's pointer note); the toolbox and icon cases follow the new toolbox. Their expected pixels do not change; a case whose pixels change is a P7 stop (REQ-0043-18).

What grows: nothing with use. A waiting shape holds 96 corners at most (768 bytes, fixed). The preview's runs are `easel_figure_cover_max_runs` per frame, as today.
Peak memory: the picture (8000 x 8000: 256 MB), its tiles as today, and at a put-down the step's log, 12 bytes for each changed pixel inside the picture plus 64 (a Fill-only box over a whole 8000 x 8000 picture: 768 MB, above the default 512 MB budget, kept as the only step, REQ-0002-17): the same as Rectangle today, as nothing else is alive then.

Measured (T007, `perf-0043`, three runs, 2026-11-12, the real session, no other Easel window, load 0.4 to 0.9): a waiting shape, width 5, Outline and fill, its bottom-right handle dragged in 30 steps toward a 10000 x 10000 box, each motion to the painted frame; then Enter.

| Case | Sharp: median, 90th, worst | Smooth: median, 90th, worst | Put-down, Sharp / Smooth |
|---|---|---|---|
| 1920 x 1080 at 100 %, Heart | 16.7 to 16.9, 20 to 40, 37 to 52 ms | 16.5 to 16.7, 19 to 20, 21 to 25 ms | 18 / 11 ms |
| 1920 x 1080 fitted, Heart | 16.7 to 16.9, 18 to 20, 21 to 23 ms | 16.7 to 16.8, 30 to 32, 34 to 38 ms | 20 / 26 to 34 ms |
| 8000 x 8000 fitted, Heart | 16.5 to 16.8, 18 to 19, 21 to 22 ms | 25 to 33, 37 to 43, 45 to 47 ms | 59 to 70 / 313 to 321 ms |
| 1920 x 1080 at 100 %, Oval Callout | 16.6 to 16.9, 18 to 20, 21 to 47 ms | 16.6 to 16.7, 17 to 22, 19 to 27 ms | 16 to 18 / 11 to 12 ms |
| 1920 x 1080 fitted, Oval Callout | 16.6 to 16.7, 18, 21 ms | 16.6 to 16.9, 19 to 29, 29 to 33 ms | 19 to 20 / 22 to 23 ms |
| 8000 x 8000 fitted, Oval Callout | 16.6 to 16.8, 18 to 19, 19 to 21 ms | 22 to 23, 33 to 35, 37 to 41 ms | 59 to 65 / 311 to 314 ms |

On the 1920 x 1080 picture the median is one frame in every case, as the budget asks; the slowest event of a case is its first to third motion (the press's state change: the panel greyed, the first frame of the drag), the same as a Rectangle's or Ellipse's drag of 0010 (`perf-0010` the same day: medians 16.6 to 16.8 ms, worst 39.6 ms at 1920 x 1080 and 60.7 ms at 8000 x 8000 at 100 %), so not of this feature. A Smooth shape on 8000 x 8000 takes two frames a motion (median 22 to 33 ms): the budget names 1920 x 1080 only; reported to JOD, no fix planned without his word. The put-down of a Smooth 10000 x 10000 shape over 8000 x 8000 is about 0.3 s (0010's filled Ellipse over the whole picture: 0.25 s).

Scenario map.
| AS | Test |
|---|---|
| AS-1 | ui `/0043/as1_toolbox` (the toolbox, the tooltip, the icon following `win.shape`, the hint, the crosshair), `/0043/as1_popover` (the well, the popover's 19 cells, Heart kept, the keys, Esc, the canvas press), `/0043-a11y/as1_shape_button_name`, `/0043-long/as1_least_size`; Risk 2's probe (keys) and M1 (the real mouse, T004's look) |
| AS-2 | core `shapes_triangle_corners`, ui `/0043/as2_waits_then_enter` |
| AS-3 | core `shapes_every_shape_is_polygon_p` (with Sharp nothing outside a box of at least w x w), `shapes_mirror_is_pixel_mirror`, `shapes_old_three_identical`, `shapes_table_is_corners_md`, `shapes_place`, `shapes_box`, `shapes_small_boxes`, `shapes_deterministic`, `shapes_largest_box`, `shapes_names`; the 0010 and 0039 core tests unchanged |
| AS-4 | ui `/0043/as4_move_resize_mirror` (400 %, rendered), `/0043/as4_shift_square_box` |
| AS-5 | ui `/0043/as5_options_while_waiting` |
| AS-6 | ui `/0043/as6_put_down_sites`, `/0043/as6_press_away` |
| AS-7 | ui `/0043/as7_esc_undo_redo` |
| AS-8 | ui `/0043/as8_during_a_drag` |
| AS-9 | ui `/0043/as9_handles_over_picture_handles` |
| AS-10 | ui `/0043/as10_memory` |
| AS-11 | ui `/0043/as11_shown_is_put_down` (100 %, scrolled 400 %, 25 %, by the render) |
| AS-12 | ui `/0023-lang/as12_shapes_translated` in the 24 language runs, 0023's `as9_help_page` and 0021's `as4_help_page_text` |
| AS-13 | ui `/0043/as13_keyboard` |
| AS-14 | M1 (T014), the icon sheet approved (T002), ui `/0043/as16_shape_icons_at_16` |
| AS-15 | ui `/0043/as15_line_pixels` (rendered, 400 %), `/0043/as15_line_not_in_picture` (the user's path, the saved file), M2 (T018) |
Constitution: the pixels come from `figure.c` alone, the same call for the preview and the put-down (P7.3); the new arithmetic is whole numbers in `src/core` with its own tests and mutants; the toolbox stays one list (`tool.h`), the chosen shape one action; no thread, no cache.

Risks.
1. REQ-0043-18 rests on the old kinds giving the same pixels from their box as from their drag in any direction. Checked by `shapes_old_three_identical` in T001 before anything uses `easel_shape_figure`; if one pixel differs, the work stops for JOD.
2. The popover: whether GTK 4 delivers the press that closes an autohide popover to the canvas below it (REQ-0043-02: it must start nothing), and whether the arrow keys move between the grid's cells. A probe in the task that builds the popover, in the real session (the isolated one has no real pointer), before its tests; if the press reaches the canvas, the canvas ignores a press that arrives while the popover is open or in the same event cycle that closed it.
3. Cost: a Smooth Heart (96 sides) or Oval Callout in a 10000 x 10000 box, previewed at every motion of a handle drag, against the budget "pointer event to pixels within one frame at 1920 x 1080". Measured by `perf-0043` right after the task that makes the handle drag (frames of a handle drag at 1920 x 1080 and 8000 x 8000 fit, Sharp and Smooth); a fix, if one is needed, only after that measure (P5).
4. The migration of about 40 ui cases is wide but mechanical; a case is edited only to choose Shapes and put the shape down with Enter, never in its expected pixels. The diff of those tests is reviewed for that at the end of their task.
5. The 10000 limit is new for the three old shapes (REQ-0043-04): a box above it was drawn before. No core test draws one; the change is visible only past 10000 pixels, as the spec says.

### Change of 2026-11-13 (the waiting shape's dashed line, REQ-0043-19, AS-15; implemented 2026-11-13)
(Opus, 2026-11-13.) Read with REQ-0013-05, REQ-0014-17 and REQ-0017-05 and the code that draws their line.
- **One line, one drawing code.** `append_selection_outline` already draws a rectangle's ring (the white strips and `ring_dashes`)
  for a selection being dragged out, a marked or floating one, and an open text box, choosing the box by tool. Shapes becomes a
  third branch of that choice, reading a new `shape_line_box (self, &x, &y, &w, &h)`: the waiting box (`wait_x` .. `wait_h`, as
  `easel_canvas_get_waiting_shape` gives it) while a shape waits, and during the first drag of Shapes, once `shape_moved`, the
  box `easel_shape_box` makes of the drag (as `current_figure` does); FALSE otherwise, and always FALSE for Line. The thickness
  and the dashes are `line_thickness` and `ants_phase` as for the others, so REQ-0014-17 (a) and (c) hold by construction.
- **Movement.** `ants_active` gains a Shapes branch: active while a shape waits with no held drag (`pending && pend_shape &&
  !pend_held`); not during the first drag (`shaping`). A keyboard press's move drag sets `pend_held` as the mouse's does (the same
  press path, REQ-0022-10); a press that does nothing sets none. The zoom's thickness and dash length, and the count from the
  shown box's top-left corner (mirrored or resized), come from the shared ring code at each frame. Animations off and the stepped clock are its existing tail. The timer
  starts from the frame that draws the line (`ants_phase`), and ends itself when nothing moves, as now.
- **No tint.** `append_selection_tint` is not changed: Shapes has no selection, so it draws nothing; a test asserts it.
- **Order.** The snapshot order stays: the shape's preview, the grid, the picture's handles (`append_handles`), then the line,
  then `append_selection_handles` (the shape's handles over the line, REQ-0015-01).
- **Never in the picture.** The line is a snapshot overlay only; the put-down (`put_down_pending`) and Save read the picture's
  buffer, which no overlay touches. Proved by AS-15's real-path case, and by a manual break written in the task's Result.
- **Every reader** of the selection line's gate (grep `ants_active`, `append_selection_outline`): the Select and Free-Form
  branches and the Text branch keep their conditions; the new branch is reached only with Shapes the tool. 0013's, 0014's and
  0017's line cases (`ui-0013`, `ui-0014`, `ui-0017`) run with the task.
- **Tests.** ui `/0043/as15_line_pixels` (a rendered window at 400 %, the scale asserted 1: the first drag held, the line round the
  11 x 9 box the same at two clock values 0.3 s apart; after the release the four sides at the logical rows 39 and 76 and columns
  39 and 84 from the picture's corner, every pixel white or black, black on each side, black runs of 4 away from the corners and the
  start; no tint inside the box; a handle's navy over the line; the clock 0.3 s on, the dashes moved; a handle drag held, and a
  keyboard move held, and animations off, the line the same at two clock values, with animations off black at the top-left; at
  800 % the line 2 thick, runs of 8; a shape in (35, 25) to (45, 32), the line over the canvas area right of and below the picture,
  and at the same place round the box after a scroll). ui `/0043/as15_line_not_in_picture` through the user's path: the drags by
  the canvas's gesture, Enter by the window's key controller, Save to a PNG through `win.save` with a file set, and the file
  opened again: the triangle's picture is Polygon P with the line's places (columns 9 and 21, rows 9 and 19) white; the width-5
  Triangle in (10, 10) to (12, 12) equals Polygon P; in the window's picture and in the file. The process-wide animations setting
  is set back to the test program's start value.
- **Mutants** (ui, REQ-0043-19): the Shapes branch of the line left out; the line during the first drag left out; the dashes
  moving during a held drag (`!pend_held` dropped); a tint drawn over the waiting box; the line under the shape's handles.
- **Cost** (`docs/engineering.md`, the budget "pointer event to pixels within one frame"): the dashes are not clipped to the view,
  so a 10000 x 10000 box at 100 % adds about 5000 rectangles to each frame of a handle drag. `perf-0043` is run in the task right
  after the line exists, three runs interleaved with a build of the last commit (a worktree), the numbers recorded here.
- Constitution: a ui-only change, one drawing path shared with the selection and the text box; no pixel of the picture changes
  (P7.2); tests first. Risks: the cost above (fallback, if a case leaves the budget: clip the dashes to the visible area, which
  changes no pixel shown, and measure again); none other.
- Measured (T017, 2026-11-13, the real session, no other Easel window, load 0.2 to 1.6): `perf-0043` at `b01a2e5` (a worktree
  build) and with the line, interleaved, three runs each; median, 90th and worst of the handle drag's events, in ms:

| Case | Without the line | With the line |
|---|---|---|
| 1920 x 1080 at 100 %, Heart, Sharp | 16.5-16.7, 17.7-18.5, 21-22.5 | 16.7-16.8, 18.4-21, 22.1-27.9 |
| 1920 x 1080 fitted, Heart, Sharp | 16.7, 17.5-18.2, 18.3-36.3 | 16.6-16.7, 18.2-18.6, 19-19.7 |
| 8000 x 8000 fitted, Heart, Sharp | 16.2-16.6, 17.5-18.4, 17.7-34.7 | 16.6-16.8, 17.6-18.3, 18.9-40.6 |
| 1920 x 1080 at 100 %, Heart, Smooth | 16.8-16.9, 17.8-29.3, 18.8-40.6 | 16.6-16.8, 17.5-17.6, 18.6-22.4 |
| 1920 x 1080 fitted, Heart, Smooth | 24.9-33.1, 34.9-40.8, 36.4-42.6 | 17-33.4, 34.7-35.4, 36.1-36.5 |
| 8000 x 8000 fitted, Heart, Smooth | 32-37.9, 41.1-49.6, 43.9-50.4 | 28.7-29.5, 40.5-41.8, 43.6-47.4 |
| 1920 x 1080 at 100 %, Oval Callout, Sharp | 16.7, 17.9-18.3, 18.9-41.2 | 16.7-16.8, 18-18.9, 18.4-19.8 |
| 1920 x 1080 fitted, Oval Callout, Sharp | 16.6-16.7, 17.7-18.6, 19.1-19.5 | 16.5-16.7, 17.9-19.2, 18-34 |
| 8000 x 8000 fitted, Oval Callout, Sharp | 16.5-16.7, 17.6-21.4, 18.3-62.4 | 16.5-16.8, 17.8-19, 18.6-41.9 |
| 1920 x 1080 at 100 %, Oval Callout, Smooth | 16.6-16.7, 18.2-18.5, 21.4-25.8 | 16.7-16.8, 17.7-21.6, 18.3-27.2 |
| 1920 x 1080 fitted, Oval Callout, Smooth | 16.6-16.8, 24.5-32.9, 34.4-39.8 | 16.6, 17.8-20.7, 25.7-32 |
| 8000 x 8000 fitted, Oval Callout, Smooth | 28.3-31.6, 40.3-48.3, 46.6-50.1 | 24.6-28.4, 39.2-43.4, 42.7-46.9 |

  The line costs nothing that shows: every case of the build with it lies within the other build's spread or below it. Put-downs
  as T007's. One case is over a frame in both builds: the fitted 1920 x 1080 Smooth Heart, median 24.9 to 33.1 ms without the line
  and 17 to 33.4 with it, against 16.7 to 16.8 at T007 (load 0.4 to 0.9 then); its cost is the Smooth polygon's, not the line's, so
  the Plan's fallback (clipping the dashes) would not change it. Reported to JOD; not hunted in this change.

## Tasks
Group A: the core.
- [x] T001 The shapes' figures (REQ-0043-04, -05, -14, -18; AS-3, AS-2's corners) - done when core `shapes_triangle_corners` (AS-2's corners at widths 1 and 5), `shapes_every_shape_is_polygon_p` (the 16 shapes, boxes 1 x 1, 2 x 2, 11 x 9 and 101 x 61, mirrored on each axis or not, Sharp and Smooth, widths 1 and 5, the three fill styles: the pixels equal a closed polygon figure through corners computed in the test from `corners.md`, read at run time, by REQ-0043-04's formula written again in the test), `shapes_mirror_is_pixel_mirror` (odd sizes), `shapes_inside_the_box` (Sharp, W and H at least w), `shapes_box` (drag directions, Shift toward the end, the 10000 limit, a 1 x 1 box), `shapes_old_three_identical` (Risk 1: Rectangle, Rounded Rectangle and Ellipse from their box equal the old figure from the drag in each of the four directions, sizes 1 to 20, widths 1 to 5, the three styles, Sharp and Smooth; written and run first: a pixel that differs stops the work for JOD) and `shapes_deterministic` pass, the 0010 and 0039 core tests unchanged, with mutants of the corner placement (p's rounding, the inset, the mirror, a corner of the table).
  Result: red first: `core-shapes` against a stub failed at `shapes_names` (`easel_shape_name` NULL). Module `src/core/shapes.{c,h}`: `EaselShapeKind` (the name `EaselShape` was taken by the brush footprint), `easel_shape_name`/`_from_name`, `easel_shape_corners` (the table generated from `corners.md` by a script), `easel_shape_box`, `easel_shape_place`, `easel_shape_figure`. Cases: `shapes_names`, `shapes_table_is_corners_md` (corner for corner against `corners.md` read at run time through `EASEL_SOURCE_ROOT`), `shapes_place` (p against the spec's formula written again in the test, every u at sides 1 to 10000, the mirror identity), `shapes_triangle_corners` (AS-2's corners at widths 1 and 5, and 2 and 4), `shapes_box` (the four directions, Shift toward the end, 1 x 1, the 10000 limit both ways), `shapes_old_three_identical` (Risk 1: Rectangle, Rounded Rectangle and Ellipse from their box equal the old figure from the drag in each direction, 20 x 20 sizes, widths 1 to 5, three styles, Sharp and Smooth: no pixel differs, so no stop), `shapes_every_shape_is_polygon_p` (3 072 cases: the 16 shapes, four boxes, four mirrors, Sharp and Smooth, widths 1 and 5, three styles, against Polygon P built in the test; with Sharp nothing outside the box), `shapes_mirror_is_pixel_mirror` (odd sizes, both axes: exact), `shapes_small_boxes`, `shapes_deterministic`, `shapes_largest_box`. 13 mutants (p's two roundings, the inset, the inner width, the middle pixel, each mirror, a Heart corner, the box limit, Shift, the polygon open, the old three's box and their Smooth): all killed; one survived at first (the inset `w / 2` for `(w - 1) / 2`: only even widths tell them apart) and the widths 2 and 4 were added. `tools/mutate.sh` now copies `corners.md` into its tree (it leaves out `docs/`). The 0010, 0011 and 0039 core tests unchanged and green; core and io 52 of 52; build warning-free.
Group B: the tool, its icons and its panel (each task keeps every suite green).
- [x] T002 The 16 outlines (REQ-0043-16; AS-14's sheet) - done when the 16 SVGs are drawn from scratch in the proportions of `lists-sheet.png`, a sheet of the 19 at real size, light and dark, rendered from the app's resources into `.scratch/`, and JOD has approved it (his words in the Result); 0029's icon tests list the new icons.
  Result: red first: `/0043/as16_shape_icons_at_16` failed at the one-color check (`245 <= 2`: GTK's missing-image icon) before the 16 SVGs were in the resources. The outlines are made by `tools/shape-icons.py` from `corners.md` (the line's middle in 1.75 to 14.25, the arrows 8.75 deep across their shaft; the line written as its outline, round outside each turn, mitred inside, bevelled past a mitre of 2 (the Heart's notch had a spike), a hole's tip cut where it is one pixel wide (the Right Triangle's 45-degree tips left a half-covered pixel inside, found by the test)). The test: the 16 at 16 pixels in a dark and a light text color, one color, partial transparency only at the edges, each spanning 14 pixels (the arrows 14 x 10). The sheet of the 19 at real size and 2x, light and dark (`.scratch/0043-icon-sheet.png`, not committed): approved by JOD, 2026-11-12 ("go").
- [x] T003 Shapes in the toolbox (REQ-0043-01, -03; AS-1's toolbox) - done when `EaselTool` has Shapes in place of the three, `win.shape` exists with "rectangle" at start, the toolbox is REQ-0043-01's 16 buttons in its order with the tooltip "Shapes" and its icon following `win.shape`, the hint and the crosshair are REQ-0043-03's, and a Shapes drag still draws at its release as the old tools did (the waiting comes in T005); ui `/0043/as1_toolbox` passes, the cases that named the old tools use `use_shapes` and pass unchanged in their pixels (Risk 4: their diff reviewed for that), `perf-0010`, `perf-0039` and `probe-screenshot` follow, and the strings "Shapes", the hint and the 16 names are in the eleven languages and Help's toolbox section in the twelve pages (0021's Help test).
  Result: characterization, stated: the code (the enum, the toolbox, `win.shape`, the hint) was written before `/0043/as1_toolbox`, against the rule; the test was then proved by breaks: 5 mutants (the icon not following `win.shape`, the shape changed during a drag, the old hint, Shapes always drawing a Rectangle (killed by 0010's `as9_shift_during_drag`), Shift constraining as Line's 45-degree steps (it survived every old case, so `/0043/as4_shift_square_box` was added: a Shift drag from (10, 10) to (30, 14) gives the 5 x 5 outline)), all killed; a first Shift mutant (the kind Curve) was equivalent (`easel_figure_constrain` makes every kind but Line a square) and replaced. Cases: `as1_toolbox` (16 buttons in eight rows in REQ-0043-01's order, none for the three old tools, the tooltip, the Rectangle's icon at start and `win.shape` "rectangle", the click making Shapes the only selected tool, the hint, the crosshair, the icon following Heart (`easel-shape-heart-symbolic`) and Ellipse (its tool icon), a name not in the list and a change during a drag ignored, the shape kept across Pencil), `as4_shift_square_box`. Shapes still draws at its release (the waiting is T005's). The migration (Risk 4): the test helpers `use_tool` and `set_tool` choose Shapes and `win.shape` for "rectangle", "rounded-rectangle" and "ellipse" (in place of a separate `use_shapes`), seven toolbox tables and 0022's count (16) follow the new toolbox, 0021's and 0023's Help tool lists name Shapes, `find_tool_button` skips non-string targets (it met a width cell once no Rectangle button existed); the diff of the existing cases was read: no expected pixel changed. `perf-0010`, `perf-0039` and `probe-screenshot` choose Shapes. One stale 0039 mutant (the quoted line of `current_figure` was replaced) repointed and killed. Strings: "Shapes" and the hint in the eleven languages (the two old hints dropped; the names of the 16 shapes come with the popover that uses them, T004); Help's toolbox section in the twelve pages: one Shapes entry in place of three, Polygon's "Options as Shapes". `tools/isolated.sh meson test`: 120 of 144 with the 24 language runs failing on 0023's Help list (fixed), then those 32 green; core and io green; build warning-free.
- [x] T004 The Shape well and its popover (REQ-0043-02, -16; AS-1) - done when the real-session probe of Risk 2 is reported (a press that closes the popover, the arrows between cells), then ui `/0043/as1_popover` (the well first, its tooltip and accessible name, the 19 cells five to a row in order with their names, the chosen tint, a click choosing Heart and the icons following, Heart kept across tools, Space on the button, the arrows and Enter by the real key controllers, choosing the chosen one, Esc, a press on the canvas closing it with nothing started, REQ-0021-01's keys doing nothing while open) and `/0043/as1_least_size` pass; then a manual look at the panel and the popover in light and dark, and where the focus goes after it closes, in the real session.
  Result: Risk 2's probe, in `tools/isolated.sh` with real input through Mutter's RemoteDesktop API (`.scratch/probe-0043-popover.py`, not committed): the arrows move the focus between a popover grid's cells (Left, Right, Down; the first Right after a grab was lost, a probe artifact), Enter clicks the focused cell and the popover closes with the focus back on its button; the press half is inconclusive (the virtual pointer moved but delivered no button press, a control click on the area included), so the Plan's fallback is built: the window blocks canvas presses while the popover is open and until the idle after it closes, and the real mouse is M1's. Red first: `/0043/as1_popover` failed at `shape_button` (no button with the tooltip "Shape"). Built: Shapes' page (its own, Polygon's has no Shape well) with the Shape well first (a `GtkMenuButton`, the outline and a down arrow, tinted as a chosen cell, `GTK_ARROW_RIGHT` so the popover opens toward larger x), a popover of 19 toggle cells on `win.shape` five to a row (30 x 30 each with 2-pixel gaps, after JOD found the first popover cramped: "now looks ok"), its visibility in `menu_is_open`, the accessible name "Shape: <name>" (the window's naming by tooltip skips it). Cases: `as1_popover` (the well first and as wide as its well, the position, Polygon without it, the 19 cells' order, targets, tooltips and the chosen one checked, Ctrl+I and a canvas press doing nothing while open, a click on Heart closing it with both icons following, Heart kept across tools, the button's toggle clicked opening it, `child_focus` Right and Down, the focused cell's click choosing Lightning Bolt with the focus on the button after, the chosen one again, Esc on the popover's key controller and a popdown with a press in the same cycle, each with no change and nothing started, a press acting again after the idle), `/0043-a11y/as1_shape_button_name` ("Shape: Rectangle", "Shape: 5-Point Star"; suite `ui-0043-a11y`, `GTK_A11Y=test`), `/0043-long/as1_least_size` (the long language at 1024 x 768: the panel 87 wide, labels whole, the open popover within the window; suite `ui-0043-long`). The arrows and Enter are GTK's own (`child_focus`, the cell's click), the real keys the probe's and M1's. 8 mutants (the keys while open, the canvas press while open, the same-cycle press, the cell not closing it, the popover below, the name, the outline, Polygon showing the well): 7 killed by an assertion, the name's by `assert_label`'s `g_error` (accepted, as in 0041); at first the outline survived (the test read a stored name, not the image) and the keys survived (Numpad + is an accelerator the gate never sees): both cases rewritten. Two 0011 mutants of the page choice repointed, killed. Strings: "Shape", "Shape: %s" and the 16 names in the eleven languages. `tools/isolated.sh meson test`: 145 of 146 (the long case looked the button up by its English tooltip, fixed), then the three 0043 suites green; build warning-free. Manual look (JOD, 2026-11-12, the real session: the panel and the popover in light and dark, the closing click on the canvas, Space, the arrows and Enter, Esc): "test is good".
Group C: the waiting shape.
- [x] T005 The shape waits (REQ-0043-06, -10, -11, -17; AS-2, AS-6's put-down sites, AS-7) - done when ui `/0043/as2_waits_then_enter`, `/0043/as7_esc_undo_redo` and `/0043/as6_put_down_sites` (Pencil, Save, New, an Image action, Paste, a picture resize, the active tool again keeping it, Enter with the focus on a width cell) pass: a click makes nothing, the release leaves it waiting with eight handles, the picture unchanged and not modified, the size field and "Shape W by H"; Enter puts it down as one step ("Shape put down"); Esc and Undo drop it ("Shape removed"), the history and Redo as before; Redo off while it waits; a lost release and focus loss during the first drag end it waiting; Esc, the other button and Undo during the first drag cancel it; Save and Quit during the first drag put it down; the 0010 and 0039 cases that expect a drawn release press Enter after it (pixels unchanged); the three announcements in the eleven languages.
  Result: red first: the three cases failed as Shapes still drew at the release (`as2_waits_then_enter` at the unchanged-picture check, `as7_esc_undo_redo` and `as6_put_down_sites` at "a shape waits"). Built: the waiting shape is a fourth pending kind (`pend_shape`, its box, two mirror flags) so every put-down and drop path of the curve applies; `end_shape` for Shapes makes it wait when the first drag left its pixel (`shape_moved`), else nothing; `put_down_pending` keeps the shape's own closed polygon and takes the colors of now for its button (`pending_colors`); `set_pending (FALSE)` clears the kind; the handles' box source in `easel_canvas_get_selection_handle_rect` and their drawing for Shapes; `easel_canvas_get_waiting_shape`; a signal `shape-changed` (waiting with W and H, put down, removed) that the window turns into the announcements and the size field (with the selection's). Cases: `as2_waits_then_enter` (a click makes nothing; the Triangle drag waits, picture unchanged, not modified, Redo off, eight handles, "11 × 9", "Shape 11 by 9", the window rendered inside the box equal to Polygon P from the core; Enter: Polygon P, modified, handles and size field gone, "Shape put down", one step; width 5: the inset corners, every changed pixel in the box; Line still drawn at its release), `as7_esc_undo_redo` (Esc and Undo drop it with "Shape removed", the edit kept, a second Undo undoes it; Redo off while waiting, on after Esc; Esc, the other button and Undo during the first drag cancel it; a lost release ends it waiting; a polygon after a dropped shape is a polygon), `as6_put_down_sites` (Shapes again keeps it; Pencil, Flip Horizontal, Paste, Save (saved, not modified), a picture resize drag, Enter with the focus on a width cell, New (put down before its prompt), each with the picture the put-down makes). Migration: a case that chooses an old shape by its old tool name has its shape put down by the release helpers (`shape_release`, `run_drag`, and two direct releases in 0010) with `easel_canvas_enter` (a flag set by `choose_tool`, cleared by each new window), 0010's lost-release case puts it down after (REQ-0043-06 changes REQ-0010-18); no expected pixel changed. Strings: the three announcements in the eleven languages. 9 mutants (drawing at the release, a click making a shape, no handles, no size field, an open polygon put down, the put-down said as removed, the drop not said, the kind not cleared (it survived until the polygon-after-drop case was added)), all killed; four mutants of 0011, 0015 and 0017 made stale by the edited lines repointed, the 36 of those requirements killed. `tools/isolated.sh meson test` 146 of 146; build warning-free. Help's description of the waiting comes with the handles (T006). After JOD's look (a second star drew lines: a press on a waiting shape went to the curve's stages, which read the same `pending`), the press away of REQ-0043-07 was pulled forward: a primary or secondary press puts it down and goes on as a new press, a middle one does nothing; `as6_put_down_sites` gained the case (red first at "a shape waits" after the second drag) and a mutant of it, killed; inside the box and on a handle stay T006's, the rest of the press away T009's.
- [x] T006 Move, resize and mirror (REQ-0043-07, -08; AS-4, AS-9) - done when ui `/0043/as4_move_resize_mirror` (400 %, rendered: the move, the right handle past the left edge giving the Left Arrow of that box, back again to the first pixels, Shift on a corner 22 x 10, the move clamped to a column in the picture, a reversed first drag not mirrored, the mirror kept when another shape is chosen) and `/0043/as9_handles_over_picture_handles` (the shape's right handle over the picture's, a secondary press on the shape, a middle press away, the three cursors) pass.
  Result: red first: `as4_move_resize_mirror` failed at the size field during the move ("" for "11 × 5": nothing moved) and `as9_handles_over_picture_handles` at the bottom-left cursor ("ns-resize", the picture's). Built: a press on a waiting shape goes to its handle (the selection's hit rule, `easel_canvas_selection_handle_at`, now reading a waiting shape's box with Shapes), else inside its box (`on_selection`), the primary button starting its drag and the others doing nothing, before the picture's handles; the drag keeps the box, the mirrors and the colors of its press (`shape_drag_begin`), follows by the pixel under the pointer less the pixel under the press (a move clamped to one pixel in the picture; a handle by the core's `easel_stretch_rect_mirror`, its mirror result flipping the flags, Shift its ratio), ends as last shown at its release or a lost one, and is cancelled by Esc or the other button (`easel_canvas_cancel_shape_drag`, which the window's Esc tries before it drops a pending figure); the cursors through the existing hover. A first run after the code failed: the selection's own handle block took the shape's handles (it started a selection's stretch), so it now skips Shapes; every reader of the two hit functions was then read: the others are guarded by their tools. Cases: `as4_move_resize_mirror` (400 %, the window rendered at each pixel's center inside the box: the move to (15, 13), the size field during it, the right handle 15 pixels past the left edge giving the Left Arrow of (11, 13) to (14, 17), dragged back giving the first pixels, Shift on the bottom-right corner 22 x 10, a move far right stopping at x 39, a mirrored shape kept mirrored when Right Triangle is chosen, a first drag toward smaller x and y not mirrored), `as9_handles_over_picture_handles` (the shape's right handle on the picture's: its cursor, the shape resized and the picture not, a corner's cursor, the move cursor inside, the crosshair away, a secondary press on the shape and a middle press away doing nothing). Help: the waiting shape (move, handles, the mirror, Enter, a press away, Esc) in the twelve pages. 10 mutants: 9 killed; "Esc during a drag drops the shape" survives until T009's `as8_during_a_drag` (AS-8 is its scenario). `tools/isolated.sh meson test` 146 of 146; build warning-free.
- [x] T007 Measure (Risk 3; the budget "pointer event to pixels within one frame") - done when `perf-0043` (a handle drag of a Smooth and a Sharp Heart and Oval Callout, width 5, in a box up to 10000 x 10000, event to painted frame, at 1920 x 1080 and 8000 x 8000 fit; three runs, the load noted) has its numbers in the Plan, and any case over the budget is reported to JOD before a fix is planned.
  Result: `perf-0043` (a waiting Heart and Oval Callout, width 5, Outline and fill, Sharp and Smooth, the bottom-right handle dragged in 30 steps toward 10000 x 10000, at 1920 x 1080 at 100 % and fitted and 8000 x 8000 fitted, then Enter), three runs: the numbers in the Plan. Every 1920 x 1080 case has a median of one frame; each case's slowest event is one of its first three motions (`PERF_0043_WORST`), as for 0010's drags (`perf-0010` run beside: worst 39.6 and 60.7 ms at 100 %), so it is the start of a drag, not this feature. Over a frame: Smooth on 8000 x 8000 (median 22 to 33 ms, two frames); the budget names 1920 x 1080; reported to JOD, no fix planned without his word. `perf-0010`'s release measure became the put-down by Enter (its shape now waits at the release): 245 ms for a filled Ellipse over 8000 x 8000.
- [x] T008 Choices while it waits (REQ-0043-09; AS-5) - done when ui `/0043/as5_options_while_waiting` (width, fill style, Smooth, a palette color, the swap, each redrawing at once; the same choice again changing nothing; another shape in the same box; the colors of the press kept during a move and taken at its release; a width chosen during a drag ignored) passes.
  Result: characterization: T005 and T006 already read the width, the style, Smooth and the shape live and took a waiting shape's colors from now by its button and a held drag's from its press, so `/0043/as5_options_while_waiting` passed once two mistakes of the test were put right (the colors set on the canvas only, which the window then replaced; swatch 0 for black where swatches count from 1). Cases: a Rectangle waiting redrawn for width 3, Outline and fill, Smooth and red from the palette, each against the core's pixels in the render; width 3 again changing nothing (waiting, picture and history as they were); a Triangle waiting becoming a Heart in its box, put down red by Enter in one step; during a move a palette click making the foreground red at once while the shape stays black until the release, then red; width 5 during the drag ignored. Proved by 4 mutants (the colors those of the first drag, a held drag taking the colors of now, the shape fixed at its first kind, the other color the foreground), all killed. Built: a color change redraws a waiting shape in the canvas itself (the window happened to redraw it). Two 0012 mutants of the edited line repointed, 7 of REQ-0012-04 killed. ui-0003, -0010, -0012 and -0043 green.
- [x] T009 A press away and the rules during a drag (REQ-0043-07, -12; AS-6, AS-8) - done when ui `/0043/as6_press_away` (put down then the next shape waits; Esc in that drag keeps the put-down; a click away puts down and makes nothing more; a primary press on a picture handle puts down and resizes) and `/0043/as8_during_a_drag` (Esc and the other button cancel a handle drag; Undo during a move drops it and the release does nothing; Enter, Delete, Redo, another tool change nothing; a lost release and focus loss end it waiting; Save during a move puts down and saves; a wheel scroll during a handle drag, the edge at the pointer's pixel; Ctrl+wheel does not zoom) pass.
  Result: characterization: the press away was pulled into T005 after JOD's look and the drags were built in T006, so both cases passed once one expectation of the test was put right (a scroll's edge is the press's edge moved by the pointer's pixel less the press's pixel, not the pointer's pixel: the press was on the handle, below the edge). Cases: `as6_press_away` (an Ellipse waiting, a press away dragged on: put down, "Shape put down", modified, the next one waiting with "Shape 6 by 6"; Esc in that drag keeping the put-down with nothing waiting; Undo in it cancelling only the new shape, a second Undo then undoing the put-down; a click away putting it down and nothing more), `as8_during_a_drag` (a handle drag: Esc and the other button put it back as at its press; Enter, Delete, Redo and Pencil change nothing; a lost release and a lost focus end it as last shown, still waiting, the later release doing nothing; Undo during a move drops it, the release doing nothing; Save during a move puts it down as shown, saved, not modified; on 3000 x 2000 a handle drag with Ctrl+wheel not zooming and a 100-point scroll before the next motion: the edge moved by 110 pixels). Proved by 9 mutants of REQ-0043-07 and -12 (with T006's Esc mutant, which survived until this case, and a new one: the other button not cancelling), all killed. ui-0043 green.
- [x] T010 The keyboard (REQ-0043-06, -07; AS-13) - done when ui `/0043/as13_keyboard` (Space held, the marker moved by 5 and 3, released: a rectangle waits; Space inside the box and Right twice: moved by (2, 0); Space away with no move: put down, nothing more; Right with no Space: only the marker) passes.
  Result: characterization: a keyboard press reaches the canvas as the mouse's press does (REQ-0022-10), and the handles already refuse it (`easel_canvas_selection_handle_at` returns none while `key_pressing`), so `/0043/as13_keyboard` passed at its first run. Case: with Rectangle, Space at (6, 5), Right 5, Down 3, Space: a 6 x 4 rectangle waits with eight handles, the picture unchanged; with no Space held Right moves only the marker (20 to 21) and the box stays; Space at (8, 6) inside it, Right twice, Space: moved to (8, 5); Space at (30, 20) and Space with no move: put down, the picture the core's rectangle at (8, 5), nothing waiting, one step. Proved by a break: a keyboard press inside the box not taken as a move (it then put the shape down and started a new one) failed the case at the moved box; killed, with the 7 other press mutants of REQ-0043-07.
- [x] T011 Memory (REQ-0043-13; AS-10) - done when ui `/0043/as10_memory` (the put-down refused by Enter, by Print, which stops, and by a press away and a drag, the dialog at the release and no new shape; a redraw refused at a width change; each: "Could Not Draw the Shape", nothing waits, the picture and history as before, compared with the state before) passes, its limits set between the sizes of the allocations it separates.
  Result: red first: `/0043/as10_memory` failed at "no dialog while the button is held" (a put-down refused at a press away showed its dialog at once). Built: a put-down at a press away records its button when refused (`defer_shape_failed`, `shape_failed_button`) and emits `shape-failed` at that button's release, the press starting nothing; a waiting shape's preview checks its row buffer against the preview memory hook (`preview_max_bytes`, Sharp and Smooth) and a refused or failed preparation or buffer calls `shape_preview_failed`: from an idle (never inside a frame) a waiting shape is dropped with the dialog (`drop_unshown_shape`), or, a button held, at its release, and a first drag that could not be shown leaves no shape and the dialog at its release; Print already stopped on a refused put-down (its `put_down_failed`). Cases: an earlier edit, then with the put-down's memory at 100 bytes: Enter shows "Could Not Draw the Shape", nothing waits, the picture, Undo and Redo as before; Print: the dialog and no print; a press away and a drag: no dialog while held, the dialog at the release, no new shape; the redraw: a Smooth Heart waiting at width 1 with the preview's memory between its buffer at width 1 and at width 5 (the sizes from the core, printed), width 5: the dialog, nothing waits; Sharp: a Rectangle waiting (3 runs) becoming a Heart (about 200), the same. 5 mutants (the dialog at the press, no dialog, the Smooth and the Sharp limit ignored, an unshown shape left waiting), all killed; one more (the press's button not swallowed) was equivalent (the press returns and starts nothing either way) and is not kept. Three mutants made stale by the edited lines (0011's dialog, 0043's click and press) repointed, the 16 of those requirements killed. `tools/isolated.sh meson test` 146 of 146; build warning-free.
- [x] T012 What is shown is what is put down (REQ-0043-06, -14; AS-11) - done when ui `/0043/as11_shown_is_put_down` (a Smooth 5-Point Star over a red picture at 100 %, a scrolled 400 % and 25 %, the render against the picture after Enter) passes; likely to pass at once through `figure.c`'s shared preview: the break that proves it is the preview's colors taken from the press while the put-down takes them from now.
  Result: characterization, as the task said: the preview and the put-down make the same figure by `figure.c` with the same colors (`pending_colors`), so `/0043/as11_shown_is_put_down` passed once the test compared the right thing (first the whole window, whose title, Undo button and status bar change with a put-down: only the picture's area in the view now, less the 40-pixel band the scroller shades; then a picture large enough to scroll at 400 %, 600 x 400; then 25 % reached by Zoom Out, as no preset gives it). Case: a Smooth 5-Point Star, width 3, Outline and fill, waiting over a red picture, its colors changed from the palette while it waits (blue and yellow), the handles hidden: at 100 % (212 784 rendered pixels of the picture), at 400 % scrolled to the picture's middle (886 008) and at 25 % (8 784), the render with the shape waiting equals the render after Enter, 0 pixels differing. The break the task names, the preview in other colors than the put-down (the drawing color of the press, black, shown while blue is put down), applied by hand with this case alone: 6 563 pixels differ at 100 %, failed; as a mutant it is killed first by `as5_options_while_waiting`. ui-0043 green.
- [x] T013 Every language (REQ-0043-15; AS-12) - done when ui `/0043/as12_translated` in the 24 language runs (no English left in "Shapes", "Shape", the hint, the 19 names, the announcements) and 0021's Help test (Shapes named, the three old tools not) pass.
  Result: characterization: the strings were translated as they were added (T003 to T005) and the core's `translations` check holds every message translated and reviewed in the eleven catalogs, so the new case passed in every run. Case `/0023-lang/as12_shapes_translated`, run in the 24 language runs (and the plain and only-C ones): the words Shapes shows through the real UI (the toolbox's tooltip, the hint in the status bar after its click, the Shape button's tooltip, the 19 cells' tooltips of the opened popover, and the four announcements of a shape waiting, put down, waiting, removed), each present; in Russian and Ukrainian none with an English word; in every language but English the hint, "Shape" and the announcements not the English ones (a single word may be the same, French "Triangle", German "Ellipse"). Help: 0023's `as9_help_page` and 0021's `as4_help_page_text` (their tool lists name Shapes since T003) pass in every language. Break: the Russian "Shape removed" set back to the English: `ui-0023-ru` failed ("Shape removed" has the English word Shape); restored. 25 language suites green.
- [x] T014 Manual M1 (AS-14) - done when JOD has drawn each shape, moved, resized and mirrored it, changed its color and width while it waits, put it down by a click away and by Enter, and looked at the toolbox and the panel in light and dark and at the least size, in the real session; his notes in the Result.
  Result: JOD, 2026-11-12, the real session: "t014 is ok" (each shape drawn, moved, resized and mirrored, its color and width changed while it waits, put down by a click away and by Enter, the toolbox and the panel in light and dark and at the least size).

Change of 2026-11-13 (JOD; the waiting shape's dashed line, REQ-0043-19, AS-15). Implemented 2026-11-13 (T015 to T019).
- [x] T015 The line round a waiting shape (REQ-0043-19, REQ-0043-14; AS-15 pixels) - done when ui `/0043/as15_line_pixels` is red
  first (no line round the box) and green after `shape_line_box`, the Shapes branch of `append_selection_outline` and of
  `ants_active`; the five ui mutants killed; `ui-0043`, `ui-0013`, `ui-0014`, `ui-0017` and core and io green; `--lint` 0 stale;
  warning-free
  Result: red first: `/0043/as15_line_pixels` failed at the first drag's line (`the line at (39, 39), its pixel 0: 0xffffffff, not
  0xff000000`: nothing round the box). Built: `shape_line_box` (the waiting box, or the first drag's once it has left its pixel) as the
  Shapes branch of `append_selection_outline`'s choice of box, and a Shapes branch of `ants_active` (a shape waits, no drag held, no
  first drag). Case `/0043/as15_line_pixels`, scale 1 asserted: a Triangle from (10, 10) to (20, 18) at 400 %; the first drag held, the
  whole line round the 11 x 9 box (164 pixels, each the pattern's black or white from the top-left corner) at two clock values 0.3 s
  apart; released, animations off, the same 164 pixels, a box pixel off the triangle exactly white (no tint); animations on, the top side
  changed after 0.3 s; held the same during a handle press and a keyboard press (Space) inside the box, changed during a secondary press
  on the shape; a navy handle pixel on the top side's row; at 800 % the 656 pixels of a line 2 thick with dashes of 8; a 600 x 400
  picture at 400 %, the shape in (591, 393) to (601, 400), scrolled to its end with overlay scroll bars off: the 156 pixels, 91 of them
  past the picture's edges, then after a scroll of (20, 12) the visible part in its new place, read 16 pixels inside the view's edges
  (GTK shades them where the picture goes on). Mutants, 5 of 5 killed (`tests/mutants.txt`): no line for a waiting shape, none in the
  first drag, dashes moving during a held drag, a tint over the box, the line over the handles. `ui-0013`, `ui-0014`, `ui-0017`,
  `ui-0043` green; core and io 52 of 52; `--lint` 1631, 0 stale; warning-free.
- [x] T016 Never in the picture (REQ-0043-19, REQ-0043-14; AS-15 put-down and file) - done when ui `/0043/as15_line_not_in_picture`
  (the user's path: drag, Enter, Save, the file opened again) passes; likely to pass at once, since the line is an overlay only:
  its Result names the manual break that makes it red (the line's places written into the picture at the put-down) and that run
  Result: characterization, as the task said: the line is drawn by the snapshot only, so `/0043/as15_line_not_in_picture` passed at its
  first run. The break that makes it red: in `put_down_pending`, after the shape's `easel_figure_apply_capped`, the line's left side
  (the column `wait_x - 1`, the box's rows) written black into the picture; the case alone failed at the first put-down
  (`assert_same_pixels`, -255), and passed again with the line taken out (the full `ui-0043` stops earlier on that break, at
  `as4_shift_square_box`'s count of black pixels). Case: at 400 %, the picture given a file first; a Triangle dragged from (10, 10) to
  (20, 18), the line drawn (a frame waited for), Enter by the window's keys: Polygon P, the columns 9 and 21 and the rows 9 and 19 white;
  Undo; "5 pixels" clicked; a Triangle in (10, 10) to (12, 12) (its outline asserted to reach the column 9), Enter: Polygon P; Save
  (no dialog), the file opened in a second window: the same pixels. `ui-0043` and core and io 52 of 52 green; warning-free.
- [x] T017 Measure (the budget "pointer event to pixels within one frame") - done when `perf-0043`, three runs interleaved with a
  build of the last commit, is recorded in the Plan, every 1920 x 1080 case within the budget, or the fallback of the Plan's risk
  is taken and measured again
  Result: `perf-0043` at `b01a2e5` and with the line, three runs each interleaved (`.scratch/perf-0043-ab.sh`), numbers in the Plan. No
  cost from the line; the fallback not taken. The fitted 1920 x 1080 Smooth Heart is over a frame in both builds at this run's load
  (reported to JOD).
- [x] T018 Manual M2 (AS-15) - done when JOD has drawn shapes in the real session at 100 %, 400 % and 800 %, light and dark, and
  seen the line move while a shape waits, stand still while it is dragged and with animations off, and leave nothing after Enter;
  his words in the Result
  Result: JOD, 2026-11-13, the real session: "i checked, all is good. i see the flowing boarer." (shapes drawn, the line against a
  selection's at 100 % and 800 %, light and dark, standing still during drags and with animations off, nothing left after the put-down).
- [x] T019 Gate of the change - done when one `tools/mutate.sh --core` run, `spec.sh lint` and `--names 0043`, `ui-0043` and core
  and io green, the Plan's map gains AS-15 by the names as built, and the change is marked implemented in the spec and the INDEX
  Result: `tools/mutate.sh --core`: 623 mutants, 619 killed, 0 killed by crash, 0 inconclusive, 3 survived, 0 errors, 1 accepted
  (225 s); the survivors are the three accepted before (REQ-0037-02, REQ-0038-03, REQ-0041-06). `spec.sh lint` and `--names 0043` ok;
  `ui-0043` and core and io 52 of 52 green; warning-free; the map's AS-15 row names the cases as built; the spec and the INDEX say
  implemented.

## Check
- Date and commit: 2026-11-12, on `8e08fc3` with the uncommitted work of T001 to T014 and the six fixes below (about 50 changed files; `src/core/shapes.{c,h}`, `tests/core/test-shapes.c`, `tests/perf/perf-0043.c`, `tools/shape-icons.py` and 16 icons new).
- Tests: `tools/isolated.sh meson test` 146 of 146 before the fixes and 146 of 146 after them; the scale 2 list 5 of 5. The cases of this feature: core `test-shapes` (11 cases); ui `/0043/*`: `as1_toolbox`, `as1_popover`, `as4_shift_square_box`, `as2_waits_then_enter`, `as7_esc_undo_redo`, `as6_put_down_sites`, `as4_move_resize_mirror`, `as9_handles_over_picture_handles`, `as5_options_while_waiting`, `as6_press_away`, `as8_during_a_drag`, `as13_keyboard`, `as10_memory`, `as11_shown_is_put_down`, `as16_shape_icons_at_16`, `check_findings`; `/0043-a11y/as1_shape_button_name`, `/0043-long/as1_least_size`, `/0023-lang/as12_shapes_translated` in the 24 language runs; `perf-0043`.
- Mutants: `tools/mutate.sh --core`: 622 mutants, 618 killed, 0 killed by crash, 0 inconclusive, 3 survived, 0 errors, 1 accepted (236 s). The ui mutants of REQ-0043 ran by id during the tasks, all killed; the 7 of the fixes below and the 5 they made stale (REQ-0011-12, REQ-0012-12), repointed, all killed; `--lint` 1 621, 0 stale.
- Survivors, accepted, none of this feature's: REQ-0041-06 (needs the history's `realloc` to fail), REQ-0037-02 and REQ-0038-03 (equivalent), as in 0041's and 0042's Checks.
- Warnings: 0. Lint and `--names 0043` clean; `tools/spec.sh status`: 18 requirements, 14 scenarios, 14 of 14 tasks.
- Scenarios: as the Plan's map; `check_findings` adds AS-4 (the cursor), AS-6 and AS-10 (F1 to F6). Edge cases: a box past 10000 core `shapes_box`, `shapes_largest_box`; boxes of 1 and 2 pixels `shapes_small_boxes`; a box larger than the picture `check_findings` (F1); a box partly outside the picture `as4_move_resize_mirror` (the move stopped at x 39).
- Results: every task has its `Result:`, the manual ones JOD's words (T002, T004, T014). Characterizations, each proved by the mutants or the break its Result names: T008 (`as5_options_while_waiting`), T009 (`as6_press_away`, `as8_during_a_drag`), T010 (`as13_keyboard`), T012 (`as11_shown_is_put_down`), T013 (`as12_shapes_translated`). T003 wrote code before its test (stated there; proved by 5 mutants). T007: a Smooth shape on 8000 x 8000 takes two frames a motion (the budget names 1920 x 1080); reported to JOD, no answer yet: noted, no fix.

Findings (a review agent read the diff and the spec blind; each checked in the code; F1 to F6 are seen by a user and were fixed test first: `/0043/check_findings` failed at F1's cursor (`"ew-resize" == "move"`) before the fixes, and each fix has a mutant that the case kills):
- F1 (fixed, REQ-0043-07): a picture handle inside a waiting shape's box showed its resize cursor while a press there moves the shape. The hover now leaves the picture's handle out where the shape's handle or box takes the press.
- F2 (fixed, REQ-0043-06, -13): a put-down refused (Enter, a press away) or a waiting shape dropped as unshown left its size in the size field. Both now emit the change that clears it.
- F3 (fixed, REQ-0043-13): a first drag or a move drag whose preview failed, cancelled by Esc, left its "show the dialog at the release" mark, so the next shape's release showed the dialog and dropped a shape that could be shown. The mark is cleared when a first drag or a shape's drag starts (the clears first added at the two cancels were found redundant by their surviving mutants and removed).
- F4 (fixed, REQ-0043-13): a put-down refused at a press whose release was lost (a lost focus) kept its dialog for a later release of that button. A lost release and a lost focus now show it at once.
- F5 (fixed, REQ-0043-13): a press on a picture handle that put a shape down and was refused showed the dialog at the press and then started the picture's resize. It now starts nothing and shows the dialog at the release, as a press away does.
- F6 (fixed, REQ-0043-03): after Enter or Esc ended a shape under a still pointer, the move cursor stayed until the pointer moved. Ending a waiting shape now drops its hover and shows the tool's crosshair (a picture handle under the pointer gets its own cursor at the next motion).
- F7 (checked, none): the core's arithmetic (p(u, N) below 10 000 000, the 10000 limit, the inset), the frees on each failure path, one undo step per put-down, the press order (a shape's handle, its box, a picture handle, away) and every user-visible string against the spec were read by the agent and by me; nothing more found.
- Known gaps: G1 (above). Not tested, sharing a tested path: Quit, Open, Save As and Paste From put a shape down through the same `easel_canvas_put_down` as Save and New (tested); the swap and the color chooser set the colors through the palette's path (tested); Enter on the Shape button opens the popover as GTK's own menu button does (the probe and M1).

### Check of the change of 2026-11-13 (REQ-0043-19, the dashed line)
- Date and commit: 2026-11-13, on `b01a2e5` with the change uncommitted (`src/ui/canvas.c`, `tests/ui/test-window.c`, `tests/mutants.txt`, the
  docs).
- Tests: `tools/isolated.sh meson test` 144 of 146 at first (F1), 146 of 146 after its fix; the scale 2 list 5 of 5. The cases of the change:
  `/0043/as15_line_pixels`, `/0043/as15_line_not_in_picture`, `/0043/check_line_findings` (F3).
- Mutants: `tools/mutate.sh --core` (T019): 623 mutants, 619 killed, 0 killed by crash, 0 inconclusive, 3 survived, 0 errors, 1 accepted
  (225 s); survivors the three accepted before (REQ-0037-02, REQ-0038-03, REQ-0041-06). REQ-0043-19 by id: 7 mutants, 7 killed (the five of
  T015 and the two of F3). `--lint` 1633, 0 stale.
- Warnings: 0. Lint and `--names 0043` clean; `tools/spec.sh status`: 19 requirements, 15 scenarios, 19 of 19 tasks.
- Results: T015 red first; T016 characterization, proved by the break its Result names (the line written into the picture at the
  put-down); T017 measured; T018 JOD's words; T019 the gate.

Findings (a review agent read the diff and the spec blind; each checked in the code):
- F1 (fixed, tests): two cases that compare the view during a shape's drag with the put-down, `/0010/as4_preview_equals_drawing` and
  `/0039/as7_preview_is_put_down`, failed in the full run: the line of the first drag is in the render now. The Plan listed the readers of
  the line's code, not the tests that compare renders of a shape drag, and T015 ran only 0013, 0014, 0017 and 0043. Both now hide the
  guides (the shape's pixels are their subject; AS-15 is the line's).
- F2 (accepted, known gap G2): while a shape waits with animations on, each step of the dashes (12 a second) draws the shape again; the
  preview keeps nothing between frames. Measured idle in the isolated session (cairo renderer, 3 s, a temporary probe not kept): a
  1920 x 1080 fitted Heart, width 5, Outline and fill, 32 % of a core Smooth and 19 % Sharp, a 200 x 150 one 17 %; a marked selection of the
  same size, whose dashes move since 0014, 14 % and 12 %; with animations off 0 % for all. The moving line's own cost is a selection's, as
  REQ-0014-17 (c) chose; the shape's redraw adds up to about 18 points of a core. No pixel is wrong; recorded as G2 and in
  `docs/phase2-plan.md`.
- F3 (fixed, tests): cases no test observed, the code right: no line while the first drag has not left its pixel; none for Line's drag;
  the timer starting by itself at the release and ending by itself after Enter, the line gone; the line over the picture's resize handle.
  `/0043/check_line_findings` (passed at its first run, characterization) and two mutants that it kills: the "left its pixel" check
  dropped, the picture's handles drawn over the line.
- F4 (checked, as the spec says): at the press of a move or handle drag the dashes go to their start (phase 0) and at the release back to
  the clock's phase, as a selection's do (REQ-0014-17 (c)); a shape that cannot be shown in its first drag (no memory) shows the line
  round its box until the release's dialog, as REQ-0043-19 says.
- F5 (checked, none): the states of REQ-0043-19 (first drag, waiting, move and handle drags by the mouse and the keyboard, a press that
  does nothing, mirrored, past the picture, scroll, zoom, put-down, drop, refused put-down), the tint, the drawing order, and the picture,
  the saved file and the thumbnail (`easel_canvas_compose_view` draws no line) read in the code by the agent and by me.

Verdict: the change meets REQ-0043-19 and AS-15; F1 and F3 fixed in the tests, F2 accepted as G2.

