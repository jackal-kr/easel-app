# 0014 Free-Form Selection

Status: accepted
Depends on: 0001, 0002, 0004, 0005, 0006, 0010, 0013

## Why
XP Paint's Free-Form Select marks any shape drawn with the pointer, not only a rectangle, so that a part of the picture with an
irregular edge can be moved, copied or cleared. XP also lets a selection leave a trail while it is dragged (Shift, "smear") and
lets it be stamped where it is (Shift-click). This feature brings the Free-Form Select tool, which moves, copies, clears and
puts down like the rectangle of 0013, and the smear and the stamp for both selection tools.
Decisions by JOD on 2026-10-20: stretching a selection by its eight handles is not here but the feature selection-handles after
this one (JOD's mock-up draws the handles round the bounding rectangle of the shape; here a free-form selection has none); the
smear leaves a copy at every pixel step, with no gaps; a Shift-click stamps only on the selection; a free-form selection is
shown by an outline of its shape and no bounding rectangle. The mock-up of 2026-10-20 (the Free-Form Select button in the toolbox
after Select, the same two mode buttons below the toolbox, the shape outlined by dashes) is described in REQ-0014-01 and -05.

## Requirements
Throughout, **the shape** of a free-form selection is its set of picture pixels, and its **bounding rectangle** is the smallest
rectangle that holds them. Everything REQ-0013 says of a selection holds for a free-form one (marked and floating, the lift, the
hole, the stamps and the put-down as one undo step, Esc, Enter, Undo, the arrow keys, Clear Selection, Select All, the context
menu, drags that do not finish, memory, blocked input) except where this spec says otherwise; where REQ-0013 says "its
rectangle" for the pixels, a free-form selection means its shape: only the pixels of the shape are lifted, leave a hole, are
moved, copied and put down, and a press is on the selection only on a pixel of the shape.

### The tool
- REQ-0014-01 The toolbox shall show Free-Form Select, with the tooltip "Free-Form Select", right after Select in the final order of REQ-0006-01, so that the toolbox is, in two columns, row by row: Pan, Magnifier / Select, Free-Form Select / Eraser, Pick Color / Fill, Gradient / Pencil, Brush / Airbrush, Line / Curve, Rectangle / Rounded Rectangle, Ellipse / Polygon. Its icon is a dashed lasso-like outline as in JOD's mock-up. Clicking it makes it the active tool (REQ-0001-09); clicking it while it is active keeps the selection as it is. Numpad + and Numpad − do nothing with it, and Ctrl+Numpad + and − keep zooming.
- REQ-0014-02 While Free-Form Select is active, the options panel shall show the two mode buttons of Select, Opaque (chosen at each start) and Transparent, drawn and worded as REQ-0013-02 says (as the Eraser's, not the mock-up's dots: JOD's choice at M1 of 0013). The mode is one choice shared with Select: choosing it with either tool is the mode of both, and it is not a setting.
- REQ-0014-03 The status bar hint (REQ-0004-09) shall be "Selects a free-form part of the picture to move, copy, or edit." While Free-Form Select is active the cursor over the canvas area shall be the crosshair of REQ-0010-15, and over the selection (a pixel of its shape) the move cursor; the resize cursor wins over a canvas handle (REQ-0004-17), also where the handle's hit area covers a pixel of the selection, and a press there is a resize, not a press on the selection. While a path is traced the cursor stays the crosshair, and during a move, smear or stamp press the move cursor.

### Marking a free-form selection
- REQ-0014-04 When the user presses the primary button with Free-Form Select in the canvas area, not on the selection and not on a canvas resize handle, the app shall first put down a floating selection and remove any selection (REQ-0013-11), then trace a path while the button is held: the path is the pixels of the 8-connected straight lines that the Pencil draws between successive pointer positions (REQ-0001-20), starting at the pixel under the press, without changing the picture. A press may start outside the picture but inside the canvas area (not in a handle's hit area, which lies partly inside the picture), and while the button is held the pointer may go outside the picture, the canvas area or the window (REQ-0010-21); the path is traced there too. As with the Pencil, the path depends on the pointer positions reported, the pixel under the pointer at each. If putting a floating selection down by this press fails (REQ-0013-19), the dialog shows and no path starts. The release closes the path with the straight line, drawn the same way, from the pixel under the release to the pixel under the press. The shape is then the pixels of the closed path and every pixel that it encloses, limited to the picture, and the selection is marked. A pixel is enclosed when no walk to the left, right, larger or smaller y, over pixels that are not on the path, reaches outside the path's own extent; so two path pixels that touch only at a corner close the path there. A press and release with no movement to another pixel (a click) makes no selection, and a path whose shape has no pixel inside the picture makes none. Shift and Ctrl change nothing of the tracing.
- REQ-0014-05 The selection shall be outlined, marked or floating, by a line 1 logical pixel wide along the whole edge of its shape, just outside its pixels, in dashes of 4 logical pixels alternating black and white, not animated, over the picture and the canvas area and not part of the picture, as REQ-0013-05 says for a rectangle (and as JOD's mock-up of 2026-10-20 shows); no bounding rectangle and no handle is drawn. Every edge between a pixel of the shape and a pixel that is not, is drawn on the side of the pixel that is not, the dashes counted along the whole closed outline: each closed loop of it from its top-left corner (the left end of its topmost edge), along that edge first. A moved selection keeps the same dashes, moved with it; a zoom change counts them again at the new size. While the path is traced, the outline of the pixels of the path so far is shown with the same line and dashes of 4 logical pixels, counted along each row and each column from the picture's origin; the dashes of the whole closed outline, counted along it, appear at the release. It scrolls with the picture and keeps its width of 1 logical pixel and its dashes of 4 at every zoom. The closing line from the pointer back to the press is not shown while tracing: it, and the whole shape, appear at the release.
- REQ-0014-06 The shape of a path that crosses itself or runs back over itself shall be every pixel enclosed by any part of the path, the union of its loops (a bow tie is one shape of both triangles; JOD, 2026-10-20). A path that encloses nothing (a straight drag, or out and back along the same line) shall make a selection of just the pixels of the path, as every path pixel belongs to the shape (JOD, 2026-10-20); only a path of one pixel, a click, makes none.
- REQ-0014-07 The status bar shall show the selection's size as "W × H" (REQ-0013-06) for the bounding rectangle of its shape: while the path is traced, that of the pixels of the path so far inside the picture (nothing while it has none); while a selection exists, its whole size (also when it floats partly outside the picture); nothing otherwise. The shape's pixels inside its bounding rectangle but outside the shape are not part of the selection: they are not lifted, not part of the hole, not written at the put-down, and a press on them is a press outside the selection.

### Moving, clearing and the modes with a shape
- REQ-0014-08 Moving a free-form selection (REQ-0013-07 to REQ-0013-10, with Ctrl as REQ-0013-09) shall lift only the pixels of the shape. The hole it leaves (REQ-0013-08) is the shape's place: those pixels become fully transparent (#00000000) in Transparent mode and the background color current at the lift in Opaque mode; the rest of the bounding rectangle is not touched. The floating pixels keep their colors in both modes (REQ-0013-13), and the put-down writes only them, each replacing the picture's pixel in all four channels (REQ-0013-11).
- REQ-0014-09 Clear Selection (REQ-0013-14) shall act on the shape: on a marked free-form selection its pixels become fully transparent in Transparent mode and the background color in Opaque mode, as one undo step (none if no pixel changes); on a floating one its pixels are discarded and the hole and the copies stay. Select All (REQ-0013-15) shall mark the whole picture as a rectangle selection with Select, whatever the selection was.
- REQ-0014-10 Choosing Select while a free-form selection exists, or Free-Form Select while a rectangle exists, shall put the selection down or remove it (REQ-0013-11, REQ-0013-12), as any other tool change does: a selection belongs to no tool but is not kept across tools.

### Smear and stamp (Select and Free-Form Select)
- REQ-0014-11 While Shift is held at the press of a move drag on the selection (without Ctrl; Ctrl+Shift acts as Ctrl), the app shall smear: the selection moves as REQ-0013-07 says, no hole is left (as with Ctrl, REQ-0013-09), and a copy is left at every place the selection passes, so that a drag leaves a continuous trail: between each two successive pointer positions (picture pixels), a copy stays at each whole-pixel place on the 8-connected line (REQ-0001-20) between the selection's two places, the place it came from included and the place it moves to excluded. A copy is the selection's pixels as they would be put down there (REQ-0013-11), laid in order, the later one over the earlier, the floating selection over all. A marked selection is lifted at the first movement to another pixel, as REQ-0013-07 says. Shift pressed or released after the press changes nothing; the arrow keys do not smear. The copies belong to the one undo step of REQ-0013-11.
- REQ-0014-12 When the user presses the primary button with Shift on a floating selection (on a pixel of it) and releases it without moving to another pixel, the app shall stamp: a copy of the selection's pixels is left where it is, as they would be put down there, and the selection keeps floating where it is. The stamp is made at the release; if the release never comes, the selection ends as last shown (REQ-0014-14) and no stamp is made. With Ctrl and Shift the click is Ctrl's (REQ-0013-09: no stamp). On a marked selection a Shift-click changes nothing, and so does a Shift press with a movement that never reaches another pixel; a movement to another pixel lifts it and smears. A Shift-click that is not on the selection is a press of the tool (REQ-0014-04 or REQ-0013-04, which Shift changes only as REQ-0013-04 says). The stamp belongs to the one undo step of REQ-0013-11; if no pixel differs at the end, no step is added.
- REQ-0014-13 If the memory for a copy of a smear or a stamp cannot be had, then the app shall show the dialog of REQ-0013-19 ("Could Not Move the Selection", "There is not enough memory to move this selection."), end the drag there, and put the selection back where and as it was before this drag (marked, if this drag lifted it, with the lift undone as REQ-0013-18 says; floating, if it floated), with this drag's copies gone; the release then does nothing. The memory to lift the shape (its pixels and its mask) is that of REQ-0013-19, with the same dialog.

### Drags that do not finish, memory, blocked input
- REQ-0014-14 During the tracing of a path, the app shall cancel it when the other button is pressed, Esc is pressed or Undo is chosen (after Undo, REQ-0002-09 applies until the release): no selection is made, a put-down made by the press stays, and any release after that does nothing. Enter, Delete, Select All, Redo and the arrow keys do nothing during it. If the release never arrives (REQ-0007-18) or the window loses focus (REQ-0010-19), the path is closed and made as last shown, as if released there; when a press shows that the release was lost, that press then acts as a press of its own. Save or Quit requested while the button is held makes the selection as last shown first (marked, so Save keeps it), then goes on. If the release never arrives and no pointer event follows, the path is the pixels traced so far, one pixel if the pointer never moved, which makes no selection. "As last shown" means as a release at the last pointer position would make it. A smear or a stamp is cancelled, ended as last shown and put down by exactly the events of REQ-0013-18: the other button, Esc or Undo put the selection back where it was before this drag, this drag's copies gone, and it stays selected; a lost release or focus ends the drag as last shown; Save, Save As, New, Open and Quit put it down first.
- REQ-0014-15 If the memory for the path or the shape cannot be had, then at the release the app shall show an error dialog with the heading "Could Not Make the Selection" and the body "There is not enough memory to make this selection.", make no selection and change nothing beyond the put-down made by the press. The memory may run out while the path is traced: the trace goes on as far as it can be kept, the path's last shown form stays, and the dialog shows at the release. A press with Free-Form Select shall be ignored when a press of Select is (REQ-0013-20), and with a dialog, a prompt or the color chooser open the keys act on that.
- REQ-0014-16 Tracing a long path, and smearing a selection along a long drag, shall keep the pointer's response within a frame (16 ms median, as the pointer-to-pixels budget of `docs/engineering.md` on a 1920 × 1080 view) up to a path of 100000 pointer positions on an 8000 × 8000 picture and to a smear of 2000 copies of a 400 × 300 selection, and the redraw of the outline of a long path shall not grow beyond that frame. `perf-0014` measures them and reports the numbers, which are recorded in the Plan; a case over budget goes to JOD before any fix, as in 0013, where the fit zoom of an 8000 × 8000 picture was accepted as over budget.

## Scenarios
The picture of the scenarios, unless said otherwise, is that of 0013: 21 × 11 pixels of #FFFFFFFF (white) with a red block
#FF0000FF at the columns 2 to 4 of the rows 2 and 3; the foreground #000000FF, the background #FFFF00FF (yellow); 100 %.
"The triangle" is the path traced by a drag that moves the pointer from (1, 1) to (7, 1) to (1, 7) and releases at (1, 7); it
closes along the column 1, and its shape is the 28 pixels (1, 1) to (7, 1), (1, 2) to (6, 2), (1, 3) to (5, 3), (1, 4) to (4, 4),
(1, 5) to (3, 5), (1, 6) to (2, 6) and (1, 7), with the bounding rectangle (1, 1) to (7, 7), and holding all 6 red pixels. "Marked"
is the triangle after the release; a move "from (2, 2)" presses on its pixel (2, 2).

- AS-1 (REQ-0014-01, REQ-0014-02, REQ-0014-03) Given the app just started, Then the toolbox shows the rows of REQ-0014-01 with the tooltip "Free-Form Select" after Select; When the user clicks Free-Form Select, Then it is the only selected tool, the options panel shows Opaque (chosen) above Transparent as with Select, the status bar shows the hint, the cursor over the canvas area is the crosshair, over a canvas resize handle the resize cursor; When the user chooses Transparent, then Select, Then Transparent is still chosen; When the user presses Numpad + or −, Then nothing changes | Verified by: automated (ui) + manual M1 (the icon and the options against the mock-up; the eight handles that the mock-up draws round the bounding rectangle are not there and that is expected until the feature selection-handles)
- AS-2 (REQ-0014-04, REQ-0014-05, REQ-0014-07) When the user drags the triangle, Then while dragging the outline of the path so far is shown, and after the release the dashed outline follows the edge of the 28 pixels (stepped along the diagonal), no rectangle and no handle is drawn, the status bar shows "7 × 7", the picture is unchanged and no step is added; the outline is 1 logical pixel just outside the same pixels at 400 %, at the fit zoom and in a scrolled view; a click, a press and release on the same pixel, leaves no selection; a drag from (-2, -2) to (3, -2) to (3, 3) selects the 10 pixels of its shape inside the picture, (0, 0) to (3, 3), "4 × 4"; a drag wholly outside the picture, from (-5, -5) to (-1, -1) to (-5, -1), shows no size and leaves no selection | Verified by: automated (core, the shape) + automated (ui) + manual M2 (JOD traces with a real mouse: a leaf shape at normal speed, a fast stroke, a path that ends near its start, and a bow tie, and checks the outline and the shape against what was drawn)
- AS-3 (REQ-0014-04, REQ-0014-06) Given the bow tie (1, 1) to (5, 5) to (5, 1) to (1, 5), Then its shape is the 17 pixels of both triangles, the bounding rectangle (1, 1) to (5, 5), "5 × 5"; Given the straight drag from (1, 1) to (5, 1), Then the 5 pixels of the line are selected, "5 × 1", and can be moved; Given a drag out and back, from (1, 1) to (5, 3) and back to (1, 1), Then its 7 path pixels are the shape | Verified by: automated (core, the shape)
- AS-4 (REQ-0014-07, REQ-0014-08, REQ-0013-11) Given marked, Then the cursor over (2, 2) is the move cursor and over (7, 7), inside the bounding rectangle but outside the shape, the crosshair; When the user presses at (2, 2) and releases without moving, Then nothing changes; When the user drags from (2, 2) to (12, 5) and releases, Then the 28 pixels of the shape show yellow (the hole) and the selection floats at the triangle shifted by (10, 3), with the red block at the columns 12 to 14 of the rows 5 and 6 and the rest of its bounding rectangle showing the picture; When the user then presses Esc, Then it is put down: 34 pixels changed (the 28 of the hole, the 6 red ones), one undo step, the title marked; one Undo restores every pixel; Given Transparent mode, the same drag leaves a fully transparent hole (#00000000) with the same 34 pixels changed | Verified by: automated (core, the lift and the put-down by shape) + automated (ui)
- AS-5 (REQ-0014-07) Given marked, When the user presses at (7, 7) (outside the shape, inside its bounding rectangle), Then it is not a press on the selection: the selection is removed, a new trace starts, and a release without movement leaves no selection; the hole of AS-4 is exactly the 28 pixels, never the 49 of the rectangle | Verified by: automated (ui)
- AS-6 (REQ-0014-09, REQ-0013-14) Given marked, When the user presses Delete, Then those 28 pixels become yellow, one step, no selection remains; in Transparent mode they become #00000000; Given it floating after the drag of AS-4, When Delete is pressed, Then the floating pixels are gone, the hole stays, one step; Given the rectangle (1, 1) to (5, 4) marked with Select, When the user chooses Free-Form Select, Then it is removed and the picture unchanged; When the user chooses Select All with the triangle marked, Then the whole picture is marked as a rectangle, "21 × 11" | Verified by: automated (ui)
- AS-7 (REQ-0014-10, REQ-0013-11, REQ-0013-12) Given the triangle floating after the drag of AS-4, When the user chooses Select, or Pencil, or Save, Then it is put down first (34 pixels, one step) and the action goes on; Given it marked, Then the same removes it, except Save, after which it stays marked; Given marked and moved from (2, 2) to (-2, 4), so floating at the triangle shifted by (-4, 2), Then the pixels with a negative x are drawn left of the picture, the status bar shows "7 × 7", a press at the pixel of the shape at (-2, 4) grabs it, and Esc writes only the pixels inside the picture (26 pixels changed) | Verified by: automated (ui)
- AS-8 (REQ-0014-11) Given marked, When the user presses Shift at (2, 2), drags to (5, 2), releases and presses Esc, Then no hole was left and the selection passed the shifts (0, 0), (1, 0) and (2, 0) of the triangle and stopped at the shift (3, 0): the copies and the selection are laid in order, so the later over the earlier and the white pixels of a later copy cover the red of an earlier one, and the picture shows the red block at the columns 5 to 7 of the rows 2 and 3 only: 12 pixels changed (the 6 old red ones back to white, the 6 new), one step; a fast horizontal drag from (2, 2) to (12, 2) in one pointer event leaves the same trail as ten drags of one pixel (ten copies, no gap), and a diagonal one leaves a copy at each pixel of the 8-connected line between the two positions; with Ctrl and Shift held at the press it moves a copy as Ctrl does (REQ-0013-09), no trail; with Shift pressed after the press nothing changes; the same Shift drag with Select on a rectangle leaves a trail of rectangles | Verified by: automated (core, the trail) + automated (ui)
- AS-9 (REQ-0014-12) Given the triangle floating after the drag of AS-4, When the user Shift-clicks at (13, 5) (a pixel of the shape: its pixel (3, 2)), Then a copy stays where it is and it keeps floating there; When the user then drags it from (13, 5) to (13, 9) releases and presses Esc, Then the picture shows the hole (28 yellow pixels), the stamp at the triangle shifted by (10, 3) (its 6 red pixels at the columns 12 to 14 of the rows 5 and 6), and the selection at the shift (10, 7), with its red block at the rows 9 and 10, over the stamp's lower rows: 40 pixels changed, one step; Given marked, When the user Shift-clicks on it, Then nothing changes and no step is added, and with Ctrl and Shift held the click is Ctrl's and stamps nothing; the selection of AS-9 sits at the rows 8 to 10 and a press on its pixel at the columns 11 to 14 of the rows 9 and 10 is a press on the bottom handle's hit area, a resize, with the resize cursor, not a press on the selection (REQ-0014-03); When the user Shift-clicks outside the selection, Then a new trace starts (a click: no selection) | Verified by: automated (ui)
- AS-10 (REQ-0014-13) Given memory made too small for the copies, When the user smears or Shift-clicks, Then the dialog "Could Not Move the Selection" appears, the selection stays floating where it was before the drag and this drag's copies are gone, and the release does nothing | Verified by: automated (core, refusal) + automated (ui, dialog)
- AS-11 (REQ-0014-14) Given a path being traced, When the secondary button is pressed, or Esc, or Undo, Then no selection is made and the release does nothing, and Enter, Delete, Select All, Redo and the arrow keys do nothing; When the release never arrives and the next press comes with no button down, or the window loses focus, Then the path is closed and the selection made as last shown; When Save is requested while the button is held, Then the selection is made, kept marked and the picture saved; Given a smear held, When the secondary button, Esc or Undo comes, Then the selection is back where it was, the copies of this drag are gone, and it stays selected; When the release of a smear or a stamp never arrives and the next press comes with no button down, or the window loses focus, Then it ends as last shown (a stamp: none made); When Save is requested while a smear is held, Then the selection is put down with its copies and saved; a path whose release never arrives and no pointer event follows (the pointer never moved) makes no selection | Verified by: automated (ui)
- AS-12 (REQ-0014-15) Given memory made too small for the shape, When the user releases a traced path, Then the dialog "Could Not Make the Selection" appears and no selection is made; Given a dialog, the quit prompt or the color chooser open, When the user presses with Free-Form Select, Then nothing starts | Verified by: automated (core, refusal) + automated (ui)
- AS-13 (REQ-0014-08, REQ-0013-13) Given a free-form selection floating over a partly transparent picture, in Opaque and in Transparent mode, at 100 %, at a scrolled 400 % and at the fit zoom, Then the canvas area rendered while it floats equals the one rendered after it is put down, except the outline; the floating pixels keep their colors and the pixels of the bounding rectangle outside the shape show the picture | Verified by: automated (ui, rendering)
- AS-14 (REQ-0014-16) Given a path of 100000 pointer positions on an 8000 × 8000 picture at the fit zoom, and a smear of 2000 copies of a 400 × 300 selection at 1920 × 1080, Then the median time of a pointer event to the painted frame, the time of the release and the peak memory are measured and recorded in the Plan, the outline of the long path costing no more than the frame; a case over the 16 ms goes to JOD | Verified by: measurement (`perf-0014`, in the real session; it reports numbers and fails no test) + automated (ui, that the trace stays usable)
- AS-15 (REQ-0014-08, REQ-0013-09) Ctrl copy: Given marked, When the user drags it with Ctrl from (2, 2) to (12, 5), releases and presses Esc, Then the original stays, no hole was left, and a copy of the shape is at the shift (10, 3): 6 pixels changed (the red ones), one step | Verified by: automated (ui)
- AS-16 (REQ-0014-08, REQ-0013-10) Arrow keys: Given marked, When the user presses Right, then Shift+Down, Then it floats shifted by (1, 0), then by (1, 10), the uncovered part of the hole showing yellow, and Enter puts it down as one step with the pixels inside the picture only | Verified by: automated (ui)
- AS-17 (REQ-0014-07, REQ-0013-17) Context menu: When the user presses the secondary button on the pixel (2, 2) of the marked or floating triangle, Then the context menu of REQ-0013-17 opens with Clear Selection and Select All; on (7, 7), inside the bounding rectangle and outside the shape, nothing opens and no selection changes | Verified by: automated (ui)
- AS-18 (REQ-0013-11, REQ-0013-12) Handle and Attributes: Given the triangle marked, When the user presses the primary button on a resize handle, or chooses Attributes, Then it is removed (no change to the picture) and the action goes on | Verified by: automated (ui)
- AS-19 (REQ-0013-12) Save: Given the triangle marked, When the user saves, Then it stays marked and the picture is saved | Verified by: automated (ui)
- AS-20 (REQ-0014-04, REQ-0014-06) Given the diamond path (1, 3) to (3, 1) to (5, 3) to (3, 5), closed back to (1, 3), Then its diagonal sides close it at the corners where two path pixels touch only diagonally, and its shape is the 13 pixels with |x − 3| + |y − 3| at most 2, "5 × 5"; Given a path that surrounds the whole picture (every pixel of the picture enclosed, started inside the canvas area away from any handle), Then the shape is the whole picture and "21 × 11" | Verified by: automated (core, the shape) + automated (ui)
- AS-21 (REQ-0014-05) Given Free-Form Select at 100 %, When the user presses at (1, 1) and moves to (7, 1) and (1, 7) with the button held, Then the outline of the path's pixels so far shows dashes of 4 logical pixels counted along each row and each column from the picture's origin, and View actions such as Zoom In do nothing while the button is held (REQ-0004-03); When the user releases, Then the outline of the 28 pixels shows dashes counted along the closed outline from its top-left corner; Given the same trace at 400 % (zoomed before the press), Then the same holds in logical pixels at 400 %; Given the triangle marked, When the user zooms in, Then the dashes are counted again along the outline at the new size; When the user moves it by (10, 3), Then the outline is the same, moved by (10, 3), with the same dashes | Verified by: automated (core, the outline's dashes) + automated (ui, rendering)

## Edge cases
- A path that starts and ends on the same pixel with movement between (a loop): the closing line is one pixel; the shape is the loop's pixels and what it encloses.
- A path outside the picture that encloses a part of it: the shape is the enclosed pixels inside the picture only; one that surrounds the whole picture makes the whole picture the shape, as a rectangle would.
- A one-pixel shape (a path of one pixel plus a tiny loop, or clipping): it is marked, moved and cleared as any other.
- A selection floating wholly outside the picture and put down: nothing is written, the hole remains (one step; no step after a Ctrl lift).
- The shape moved back exactly onto its hole: no pixel differs, no step is added, in both modes.
- Shift pressed during tracing, a move or the arrow keys: nothing (REQ-0014-04, REQ-0014-11); Ctrl+Shift at the press of a move acts as Ctrl.
- A Shift press on a selection then a movement of less than a pixel (zoomed in): a click, a stamp; the first movement to another pixel makes it a smear that began with the copy at the place of the press, so a stamp then a smear leave the same first copy.
- A very long smear (a 10000 × 10000 selection dragged across the picture, or copies wholly outside the picture): each copy costs its place only (8 bytes) while it floats; the put-down step costs what REQ-0002-16 allows for the pixels it changes, and a step or a copy that cannot be had shows the dialog of REQ-0014-13.
- The same copy stamped twice at one place: the same pixels; no extra step cost beyond the pixels that differ.
- The same action again while its dialog is open: as 0011.
- For a state that waits: the mouse alone finishes a floating selection by a press outside it or another tool; color changes leave it as it is (REQ-0013-13); view changes keep it; tool changes put it down (REQ-0013-11, REQ-0014-10).

## Out of scope
- The eight handles of a selection and stretching it by them (the feature selection-handles), including the handles JOD's mock-up draws round the bounding rectangle of a free-form selection.
- Cut, Copy, Paste, Copy To…, Paste From…, the canvas menu outside a selection (Clipboard); Flip, Rotate, Stretch, Skew, Invert Colors on a selection (Image Transforms).
- A free-form selection that is not a single closed path: adding to or subtracting from a selection (Phase 2 at most).
- Smoothing the edge of the shape (P4: hard-edged pixels).

## Open questions
None.

## Plan
### Approach
A selection gets an optional **mask**: one byte per pixel of its bounding rectangle, 1 for a pixel of the shape (no mask = the
whole rectangle, as in 0013). Everything 0013 built takes the mask: the lift copies the mask into the float with the pixels,
the lay and the put-down skip pixels outside it, the hole is only the mask's pixels, a stamp is the masked float at a place,
Clear fills the masked pixels, and the hit test reads it. So the free-form selection reuses the one lay function, the one
put-down (`finish_selection`), the one cancel of a drag (`cancel_selection_drag`) and the one cancel of a float
(`cancel_float`) of 0013 unchanged in structure.
The shape is made at the release by a pure core function from the pointer positions: the closed path is rasterized with
the Pencil's 8-connected line, then the pixels not on the path are cut, row by row, into intervals; intervals of adjacent rows
that share a column are joined (union-find), and those that touch the path's extent border are outside. What is left, with
the path pixels, is the shape, limited to the picture. Memory and time grow with the path's length, not with its extent (a
path dragged far outside the picture at 10 % costs no more than its pixels).
The smear is a core function: from the float's place to a new one it stamps every place on the 8-connected line, the start
included and the end excluded, then moves the float. The stamp of a Shift-click is the existing `easel_selection_stamp`.

### Files
- `src/core/selection.{c,h}`: `EaselFloat` gains `uint8_t *mask` (NULL: the whole rectangle); `easel_selection_lift` and
  `easel_selection_clear` take a `const uint8_t *mask`; new `easel_selection_smear`. `tests/core/test-selection.c`.
- `src/core/lasso.{c,h}` (new): the path (pointer positions) and the shape; `tests/core/test-lasso.c` (new).
- `src/core/outline.{c,h}` (new): the edges of a mask (the unit edges between a pixel of the shape and one that is not),
  as horizontal and vertical runs, for the outline and its tests; `tests/core/test-outline.c` (new).
- `src/ui/tool.{c,h}`: `EASEL_TOOL_FREE_SELECT` ("free-select") and `easel_tool_selects (tool)` (Select or Free-Form Select).
- `src/ui/canvas.{c,h}`: the trace (`lassoing`, the path), the mask of the selection, the smear and the stamp in the move, the
  shape's outline, the masked outside part, the hit test; `selection-failed` gains kind 2 (the shape).
- `src/ui/window.{c,ui}`: the toolbox button after Select, the hint, Esc for both tools, the dialog of REQ-0014-15;
  `src/ui/tool-options.c` (the selection page for both tools); the icon `easel-tool-free-select-symbolic.svg` and
  `easel.gresource.xml`.
- Tests: `tests/ui/test-window.c` (a /0014/ block; the toolbox tables of 0006, 0007, 0010, 0011, 0012 and 0013 and the 0008
  width list), `tests/perf/perf-0014.c`, `tests/meson.build`, `tests/mutants.txt`.

### Key interfaces
- `EaselSelectionResult easel_selection_lift (EaselFloat *f, const EaselImage *img, int x, int y, int w, int h,
  const uint8_t *mask, bool copy, uint32_t hole_color, size_t max_bytes)` (the mask is copied; its w × h bytes count);
  `EaselSelectionResult easel_selection_clear (EaselImage *img, EaselHistory *h, int x, int y, int w, int h,
  const uint8_t *mask, uint32_t color, size_t max_bytes)`;
  `bool easel_selection_smear (EaselFloat *f, int x, int y, size_t max_bytes)` (FALSE with the stamps of this call taken back
  and the float where it was).
- `EaselPath` (`int *xy; size_t n, cap`), `bool easel_path_add (EaselPath *p, int x, int y, size_t max_bytes)`,
  `EaselSelectionResult easel_lasso_shape (const EaselPath *p, int img_w, int img_h, int *x, int *y, int *w, int *h,
  uint8_t **mask, size_t max_bytes)` (DONE, NOTHING for a click or a shape wholly outside, NO_MEMORY), `easel_path_free`.
- `size_t easel_outline_edges (const uint8_t *mask, int w, int h, EaselEdge *out, size_t max)` with
  `EaselEdge { int x, y, len; bool vertical; }` (runs in pixel-corner coordinates relative to the rectangle), for the trace;
  `EaselOutline` (the closed loops, each a list of corners from its top-left corner) made by
  `EaselSelectionResult easel_outline_loops (const uint8_t *mask, int w, int h, EaselOutline *o, size_t max_bytes)` and
  `easel_outline_dashes (const EaselOutline *o, double scale, ...)` (the black dash rectangles in logical pixels for a
  pixel size of scale), for a marked or floating selection.
- Canvas: `easel_canvas_get_selection_mask (self, &w, &h)` (test hook, NULL for a rectangle); the existing hooks for the
  lift and step limits also bound the path and the shape.

### Where it is put down, removed or cancelled
The same functions as 0013: `finish_selection` puts down (now writes only masked pixels, through the lay), `cancel_float`
forgets a float, `cancel_selection_drag` cancels a drag: for a trace it drops the path; for a move, smear or stamp press it
truncates the float's stamps to the count at the press (replacing the one-stamp `sel_stamped_here`, so a Ctrl copy and a
smear's many copies go back the same way) and puts the float back. The trace ends in one function, `end_lasso`, from the
release, a lost release, a lost focus, Save and Quit.

### Every reader of what changes
- `self->tool == EASEL_TOOL_SELECT` becomes `easel_tool_selects (self->tool)` at: `on_selection` (canvas), the cursor choice
  (canvas, crosshair and move), the press (canvas, with the trace for Free-Form Select instead of the rectangle), Esc during a
  drag (`window.c`, `on_key_pressed`), the options page (`tool-options.c`). The hint is per tool (`window.c`).
- The new flag `lassoing` (not `selecting`, which means a rectangle being dragged out) is added beside `selecting` in every
  reader: `easel_canvas_is_stroking`, the release-lost check, the focus loss, the size in the status bar, the outline, the
  cancel, the motion, the release, the end of a drag on Save and Quit.
- The mask is read by: the hit test, the lift, Clear, the outline, `append_float_outside` (pixels outside the mask are not drawn),
  `easel_canvas_get_selection_size` (unchanged: the bounding rectangle). Select All, a new rectangle and every removal free it.
- Shift at a move's press: read once at the press into `sel_smear` (Ctrl wins: `sel_copy`); no 0013 test asserts that Shift does
  nothing on a move (checked: the Shift cases of 0013's tests are the square of AS-2 and the arrows of AS-5).
- The toolbox gains a button after Select: the tables of 0006 `as1_toolbox_and_options`, 0007 `as1_new_tools_toolbox_hints`,
  0010 `as1_shape_tools_toolbox_hints`, 0011 `as1_curve_polygon_toolbox_hints`, 0012 `as1_gradient_toolbox_hints`, 0013
  `as1_select_toolbox_options`; the 0008 width list. The toolbox goes from 16 to 17 buttons, from eight rows to nine
  (Polygon alone in the last): its height grows by one button row (no test reads the height; 0008 reads widths).

### What grows
- The path: 8 bytes per pointer position (doubling, freed at the end of the trace; never handed to the history, so no shrink).
- The shape: the mask, 1 byte per pixel of the bounding rectangle (at most the picture: 100 MB for 10000 × 10000), held while
  the selection exists; the intervals while it is made: about 16 bytes per path pixel and row, freed at once.
- The float: its pixels (4 bytes) and its mask copy (1 byte) per pixel of the bounding rectangle; a stamp is 8 bytes (its place)
  for a Ctrl copy, a Shift-click and each copy of a smear (2000 copies: 16 kB).
- The put-down step: as 0013 (the log shrunk to its length, held-versus-counted tested for the masked put-down and the masked clear).

### Constitution check
Layering holds (the path, the shape, the edges, the masked lay and the smear in the libc core); P4 holds (hard-edged masks, no
blending); P5 holds (plain lay and per-frame outline first, `perf-0014` before anything faster); one undo step per put-down.

### Risks
- **The outline's dashes (REQ-0014-05, decided by JOD on 2026-10-20).** A marked or floating selection's dashes are counted
  along each closed loop from its top-left corner: the outline module walks the loops of the edge set once when the shape is
  made, and the canvas keeps the result as a cached path of dash rectangles in logical pixels, translated when the selection
  moves and made again on a zoom change (the scale changes the dash count per edge). While tracing, the dashes are counted
  along rows and columns from the picture's origin, so a new path pixel changes only its own edges. Core tests: the loops of
  a shape with a hole and a bow tie, the phase from the top-left corner, and a moved shape's outline equal to the original
  translated, with the same phase (`as21_moved_outline_same_phase`).
- **The cost of the outline per frame** grows with the shape's edge length (each unit edge is up to two rectangles); a jagged
  shape over 8000 × 8000 may hold hundreds of thousands of edges. Measured in T-perf with the long path of AS-14; the fallback,
  only after the measurement, is to draw the edges once into a texture per zoom.
- **The trace's outline while tracing** must not cost the whole path per event: the edges of the path pixels are kept and
  only the new pixels' edges change (a pixel set in a hash of 64 × 64 tiles); checked in the trace task by a core test of the
  incremental edges against the edges of the whole set, and measured in T-perf.
- **The Pencil's line**: the path must be the Pencil's 8-connected line (REQ-0001-20); the lasso test compares its path with
  the pixels the Pencil stroke draws for the same positions (core).
- **A float's outside part** used the float's pixels directly; with a mask it must leave the outside-the-mask pixels
  transparent, checked by AS-13's rendering test in the task of the masked move.

### Scenario-to-test map
| AS | Test |
|---|---|
| 1 | ui `as1_free_select_toolbox_options` + manual M1 |
| 2 | core `as2_lasso_shape`, core `outline_edges`, ui `as2_trace_selection` + manual M2 |
| 3 | core `as3_crossing_and_thin_paths` |
| 4 | core `as4_masked_move_put_down`, ui `as4_move_free_selection` |
| 5 | ui `as5_press_outside_shape` |
| 6 | core `as6_masked_clear`, ui `as6_clear_free_selection` |
| 7 | ui `as7_free_put_down_sites` |
| 8 | core `as8_smear_trail`, ui `as8_smear` |
| 9 | ui `as9_stamp` |
| 10 | core `as10_smear_memory`, ui `as10_smear_dialog` |
| 11 | ui `as11_traces_that_do_not_finish` |
| 12 | core `as12_shape_memory`, ui `as12_shape_dialog_and_blocked` |
| 13 | ui `as13_free_float_equals_put_down` |
| 14 | `perf-0014` (real session) + ui `as14_long_trace` |
| 15 | ui `as15_free_ctrl_copy` |
| 16 | ui `as16_free_arrow_keys` |
| 17 | ui `as17_free_context_menu` |
| 18 | ui `as18_free_handle_attributes` |
| 19 | ui `as19_free_save_keeps_marked` |
| 20 | core `as20_diamond_and_whole_picture`, ui `as20_surround_picture` |
| 21 | core `as21_outline_dash_phase`, core `as21_moved_outline_same_phase`, ui `as21_trace_and_release_dashes` |
Core also: `lasso_matches_pencil` (the path equals the Pencil's pixels), `incremental_edges` (the trace's edges equal those of
the whole set), `as13_masked_step_holds_what_it_counts` (held against counted for the masked put-down and clear).

## Tasks
Group A: the core (no ui change beyond passing no mask)
- [ ] T001 The masked selection: lift, lay, put-down and clear with a mask (REQ-0014-07, REQ-0014-08, REQ-0014-09; AS-4, AS-6) - done when core `as4_masked_move_put_down` (34 pixels, the hole only the 28), `as6_masked_clear` and `as13_masked_step_holds_what_it_counts` pass after a red run, the 0013 core tests pass with no mask, and the canvas passes no mask (ui-0013 green)
- [ ] T002 The path and the shape (REQ-0014-04, REQ-0014-06, REQ-0014-15; AS-2, AS-3, AS-12, AS-20) - done when core `as2_lasso_shape` (the triangle's 28, the clipped 10, wholly outside none, a click none), `as3_crossing_and_thin_paths` (bow tie 17, line 5, out and back 7), `as20_diamond_and_whole_picture` (13, the whole picture), `lasso_matches_pencil` and `as12_shape_memory` pass after a red run
- [ ] T003 The outline: edges for the trace, loops and dashes for a selection (REQ-0014-05; AS-2, AS-21) - done when core `outline_edges`, `incremental_edges`, `as21_outline_dash_phase` (a hole, a bow tie, the top-left start) and `as21_moved_outline_same_phase` pass after a red run
- [ ] T004 The smear (REQ-0014-11, REQ-0014-13; AS-8, AS-10) - done when core `as8_smear_trail` (12 pixels changed, red at the columns 5 to 7; a jump of 10 equals ten steps; a diagonal jump stamps its 8-connected line) and `as10_smear_memory` (the stamps of the call taken back) pass after a red run; group A gate: all core tests, `mutate.sh` for the new core mutants, `--lint`

Group B: the tool and the trace
- [ ] T005 Free-Form Select in the toolbox, its options, hint and cursors (REQ-0014-01, REQ-0014-02, REQ-0014-03; AS-1) - done when ui `as1_free_select_toolbox_options` passes after a red run, the toolbox tables of 0006, 0007, 0010, 0011, 0012, 0013 and the 0008 width list pass with the new button, and manual M1: JOD looks at the toolbox (nine rows, the icon) and the options panel against the mock-up, the missing handles expected
- [ ] T006 Tracing and marking, with the outline while tracing and at the release, and the size (REQ-0014-04, REQ-0014-05, REQ-0014-06, REQ-0014-07; AS-2, AS-21) - done when ui `as2_trace_selection` and `as21_trace_and_release_dashes` (rendered, at 100 % and 400 %, after a zoom, after a move) pass after a red run, and manual M2: JOD traces a leaf at normal speed, a fast stroke, a path ending near its start and a bow tie with a real mouse
- [ ] T007 A press outside the shape, the hit test and the context menu (REQ-0014-03, REQ-0014-07; AS-5, AS-17) - done when ui `as5_press_outside_shape` and `as17_free_context_menu` pass after a red run; group B gate: full `meson test` isolated, mutants of REQ-0014-01 to -07, `--lint`

Group C: moving a shape
- [ ] T008 The masked move: the hole, the float, its outside part, the put-down (REQ-0014-08; AS-4, AS-13) - done when ui `as4_move_free_selection` and `as13_free_float_equals_put_down` (rendered at 100 %, a scrolled 400 % and the fit zoom, both modes) pass after a red run
- [ ] T009 Ctrl copy, arrows, Clear, Select All, tool changes and the put-down sites (REQ-0014-08, REQ-0014-09, REQ-0014-10, REQ-0013-11, REQ-0013-12; AS-6, AS-7, AS-15, AS-16, AS-18, AS-19) - done when ui `as6_clear_free_selection`, `as7_free_put_down_sites`, `as15_free_ctrl_copy`, `as16_free_arrow_keys`, `as18_free_handle_attributes` and `as19_free_save_keeps_marked` pass (each after a red run, or as a characterization proven by a mutant); group C gate as group B, and manual M3: JOD moves, Ctrl-copies, arrows, clears and puts down a free-form selection in both modes

Group D: smear and stamp
- [ ] T010 The smear with both selection tools (REQ-0014-11; AS-8) - done when ui `as8_smear` passes after a red run
- [ ] T011 The stamp (REQ-0014-12; AS-9) - done when ui `as9_stamp` (40 pixels; marked: nothing; Ctrl+Shift: no stamp; the handle's hit area wins) passes after a red run
- [ ] T012 The smear's and stamp's memory dialog (REQ-0014-13; AS-10) - done when ui `as10_smear_dialog` (from marked and from floating) passes after a red run; group D gate as group B, and manual M4: JOD smears and stamps with Select and Free-Form Select, fast and slow, in both modes

Group E: drags that do not finish, memory, blocked input
- [ ] T013 Traces, smears and stamps that do not finish (REQ-0014-14; AS-11) - done when ui `as11_traces_that_do_not_finish` (cancel by the other button, Esc, Undo; lost release and focus; Save during a trace and a smear; a release that never comes with no motion) passes after a red run
- [ ] T014 The shape's memory dialog and blocked presses; the whole-picture path (REQ-0014-15; AS-12, AS-20) - done when ui `as12_shape_dialog_and_blocked` and `as20_surround_picture` pass after a red run

Group F: measure and close
- [ ] T015 Measure the long trace and the long smear (REQ-0014-16; AS-14) - done when `perf-0014` has run three times in the real session with no Easel window open, its numbers are in the Plan's Measurements, a case over budget is reported to JOD before any fix (fallback for the outline: drawn once into a texture per zoom), and ui `as14_long_trace` passes
- [ ] T016 Group gate and manual M5 (all) - done when the full `meson test` (isolated) passes, `mutate.sh` for every REQ-0014 id and `--lint` are clean, the Plan's interfaces and map match the names as built, and JOD has tried the whole feature by hand
(filled in when the task is ticked, a line under it: `  Result: ...`. A code task: the test that failed first and its message,
or `characterization: <why there was no red run>`. A manual step: one line of what JOD saw.)

## Check
(written by /sdd-check as its last step, about ten lines)
