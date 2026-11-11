# 0043 Extra shapes

Status: accepted
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
- REQ-0043-06 (Changes REQ-0010-05, -08, -09, -18 and -19 for Shapes; JOD, 2026-11-11.) A Shapes press that never leaves its
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
- REQ-0043-14 The pixel contract (P7): (1) every pixel is whole and exactly the part's color, with Sharp; with Smooth the mix
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

## Edge cases
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

## Open questions
None.

## Plan

## Tasks

## Check
