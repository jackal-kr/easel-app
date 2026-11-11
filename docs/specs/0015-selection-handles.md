# 0015 Selection handles

Status: implemented
Depends on: 0002 (undo), 0004 (the picture's resize handles), 0007 (a lost release), 0013 (Select, the floating selection),
0014 (Free-Form Select, smear and stamp)

## Why
A selection can be moved, copied, smeared and stamped, but not made bigger or smaller. XP Paint draws eight handles round a
selection and stretches its pixels when one is dragged; JOD's reference (2026-10-20, the Easel he liked, at 100 %) draws them at
the corners and edge middles of the bounding rectangle of a free-form selection. Decisions by JOD on 2026-10-22 are noted where
they are used.

## Requirements

### The handles
- REQ-0015-01 (Pointer note, Extra shapes, 2026-11-11: a waiting shape shows the same eight handles, REQ-0043-06.) While a selection exists, marked or floating, with Select or Free-Form Select active, the app shall show eight handles
  round its bounding rectangle, with the look of the picture's resize handles (REQ-0004-16: a filled navy square of 5 x 5 logical
  pixels; JOD: the same look for the same meaning, not the black of the reference): a corner handle has its inner corner at the
  rectangle's corner, outside it; an edge handle touches the middle of its edge from outside, centered on it, its position rounded
  down to a whole logical pixel. The handles are drawn over the picture, the tint and the selection's line, the corner handles over
  the edge handles, and are not part of the picture; they scroll with the picture, keep their logical size at every zoom, and move
  with the selection, also while it is moved, smeared or stretched with the button held (JOD). The picture's own resize handles (REQ-0004-16) lie under the selection's tint and its floating pixels where they overlap (JOD,
  2026-10-22), and under the selection's handles. While a rectangle is dragged out or a
  path traced there is no selection yet and no handle. (Pointer notes: REQ-0013-05 and REQ-0014-05, which said no handle is drawn.) (Pointer note, Text, 2026-10-26: An open text box shows the same eight handles with Text active, REQ-0017-05.)
- REQ-0015-02 (Pointer note, Extra shapes, 2026-11-11: a waiting shape's handles take presses by this rule, REQ-0043-07.) A press inside a handle's own 5 x 5 square shall go to that handle; elsewhere each handle takes presses in a hit area of
  9 x 9 logical pixels round its center, less the selection's bounding rectangle, and where those hit areas overlap a corner handle
  wins over an edge handle (JOD). Inside the bounding rectangle a press is what 0013 and 0014 say (on the selection, outside its
  shape, or on a picture handle whose hit area reaches in, REQ-0014-03). A selection handle wins over the picture's resize handles
  where their hit areas overlap (JOD); where a selection handle lies on a picture handle (the selection reaches the picture's edge) it
  owns that handle's whole hit area, also the part inside the picture (JOD, 2026-10-22). While the pointer is over a handle the cursor shall be the resize cursor of its direction:
  horizontal for the left and right handles, vertical for the top and bottom ones, diagonal (top-left to bottom-right) for those two
  corners and the other diagonal for the top-right and bottom-left corners. (Pointer notes: REQ-0004-16, REQ-0013-03, REQ-0013-04,
  REQ-0013-11, REQ-0013-12, REQ-0014-03 and REQ-0014-04, where "resize handle" means the picture's.) (Pointer note, Text, 2026-10-26: A text box's handles take presses as a selection's do, but inside the box a press always goes to its text, REQ-0017-09.) (Pointer note, Tool cursors, 2026-11-06: the cursors named here are drawn by Easel in black with a white outline; which cursor shows where stays as here.)

### Stretching
- REQ-0015-03 When the user presses the primary button on a selection handle and drags, the app shall stretch the selection: the
  edges that handle stands for (one for an edge handle, two for a corner) move by the pointer's movement since the press, rounded to
  the nearest pixel boundary, and the opposite edges stay where they are. The width and the height are each limited to 1 to 10000
  pixels (JOD): a handle dragged past the opposite edge leaves that dimension at 1 pixel, beside the opposite edge, with no
  mirroring (JOD). A press and a release with no movement to another size change nothing. The drag goes on while the pointer is
  outside the canvas area or the window, and the view does not scroll (as REQ-0004-18). The status bar shows the size "W × H"
  (REQ-0013-06) while the button is held. A press on a selection handle neither puts the selection down nor removes it. The
  secondary or the middle button on a handle does nothing. (Pointer note, Image Transforms, 2026-10-27: a handle dragged past the opposite edge now mirrors the selection on that axis, replacing "with no mirroring"; AS-4's second and third clauses change with it, REQ-0018-11.)
- REQ-0015-04 A marked selection shall be lifted at the first movement to another size, as a move lifts it (REQ-0013-07,
  REQ-0013-08: the hole in the mode's color, only the shape's pixels for a free-form selection). With Ctrl held at the press the
  stretch leaves a copy as a Ctrl move does (REQ-0013-09; JOD): a marked selection is lifted with no hole, the original staying in
  the picture; a floating one leaves a copy where it was, at the size and with the pixels it had at the press. Ctrl with Shift at the
  press does both (REQ-0015-07). A stretch never smears or stamps.
- REQ-0015-05 The stretched selection's pixels shall be the pixels as they were lifted, scaled to the new size by the nearest pixel
  (hard-edged, P4): the new pixel (i, j) of a W x H selection made from a w x h one is the lifted pixel
  (floor((i + 1/2) × w / W), floor((j + 1/2) × h / H)). They are always made from the pixels as lifted, however many times the
  selection is stretched, so a selection made smaller and then as big as before has its pixels back exactly (JOD). Copies left
  before a stretch (by Ctrl, a smear or a stamp, REQ-0013-09, REQ-0014-11, REQ-0014-12) keep the size and pixels they had; copies
  left after it have the stretched size and pixels. (Pointer note, Clipboard, 2026-10-22: a pasted selection is stretched from its pixels as pasted, after Fit the fitted ones, REQ-0016-06.) (Pointer note, Image Transforms, 2026-10-27: after a flip or turn, the shown pixels are the lifted ones scaled, then flipped and turned, REQ-0018-07.)
- REQ-0015-06 A free-form selection's shape shall be stretched by the same rule: the new pixel (i, j) is in the shape when the lifted
  pixel it is made from is. If that leaves no pixel, the one new pixel nearest the center of the new box among those whose footprint
  (the lifted pixels it covers) holds a shape pixel stays in the shape, with the color of the first such shape pixel in reading order
  (JOD: never empty). The bounding rectangle, the handles and the size shown are the new box, W x H, even where the shape does not
  reach its edges. Its line (REQ-0014-05) is the traced path scaled (JOD): the box of the path's corners becomes the new box widened
  by the same number of grid lines on each side as at the lift; each traced corner keeps its relative place in that box, rounded to
  the nearest grid corner (halves toward larger x and y); between successive corners the line is the walk of REQ-0014-04. (Pointer note, Image Transforms, 2026-10-27: after a flip or turn, the shape and line are the lifted ones scaled, then flipped and turned, REQ-0018-07.)
- REQ-0015-07 While Shift is held, a corner handle shall keep the selection's ratio of width to height as it was at the press (JOD):
  after the limit of REQ-0015-03, the side whose new size divided by its size at the press differs more from 1 leads (the width on a
  tie) and follows the pointer; the other is its size at the press times the same factor, rounded to the nearest whole pixel (halves
  up) and at least 1; if that would pass 10000, the leading side is reduced so that both fit and the ratio holds. Pressing or
  releasing Shift during the drag changes it at once. Shift changes nothing for an edge handle. (Pointer note, Image Transforms, 2026-10-27: with a mirror, the ratio holds on the sizes, REQ-0018-11.)

### Ending a stretch, undo, memory and speed
- REQ-0015-08 The stretch shall end at the release, the selection floating at its new size and place. The lift, every stretch, move
  and copy, and the put-down together make the one undo step of REQ-0013-11; Undo while it floats forgets it all (REQ-0013-12). During
  the drag, as for a move (REQ-0013-18): the other button, Esc or Undo cancel this drag only, the selection back at the size, place
  and pixels it had at the press, marked again if this drag lifted it and with no copy this drag left; after Undo REQ-0002-09 applies
  until the release; the release then does nothing. A lost release (the next motion or press with no button down, REQ-0007-18) or a
  lost focus ends it as last shown, and a press that shows the lost release then acts as a press of its own. Save, Save As, New, Open
  and Quit put it down first as last shown. Enter, Delete, Select All, Redo, the arrow keys, another tool, the View and Image actions
  and Ctrl pressed after the press change nothing during it; Shift acts as REQ-0015-07 says. (JOD: a lost focus or Save keeps the
  stretch as last shown, as for a move, unlike a picture resize, which they cancel, REQ-0004-19.) (Pointer note, Clipboard, 2026-10-22: the Clipboard commands are ignored during the stretch, REQ-0016-02.)
- REQ-0015-09 If the memory for the stretched pixels (and shape), or for the copy of REQ-0015-04, cannot be had, then the app shall
  show the dialog "Could Not Stretch the Selection", "There is not enough memory to stretch this selection.", put the selection back
  at the size, place and pixels it had at the press (marked again if this drag lifted it, with no copy of this drag), and end the
  drag; the release does nothing. If the lift itself cannot get its memory, the dialog of REQ-0013-19 ("Could Not Move the
  Selection") shows and the selection stays marked. This holds whether the failure comes at the first movement or after earlier
  movements of the same drag succeeded.
- REQ-0015-10 Stretching a selection shall keep the pointer's response within a frame (16 ms median, as the pointer-to-pixels budget
  of `docs/engineering.md`) for a 2000 × 2000 and an 8000 × 8000 selection, at 100 % and at the fit zoom (JOD). `perf-0015` measures
  them and reports the numbers, recorded in the Plan; a case over budget goes to JOD before any fix.

## Scenarios
The picture of the scenarios, unless said otherwise, is that of 0013: 21 × 11 pixels of #FFFFFFFF (white) with a red block
#FF0000FF at the columns 2 to 4 of the rows 2 and 3; the background #FFFF00FF (yellow); 100 %. "Marked" is the selection (1, 1) to
(5, 4) of 0013; "the triangle" is that of 0014 (corners (1, 1), (7, 1), (1, 7), its shape the 15 pixels (i, j) of the box (1, 1)
to (5, 5) with i + j <= 4 from its top-left). A drag "by (dx, dy)" moves the pointer by dx and dy picture pixels from the press,
which is at the center of the handle's own square.

- AS-1 (REQ-0015-01, REQ-0015-02) Given marked, Then eight navy 5 x 5 handles touch the rectangle (1, 1) to (5, 4) from outside, at its corners and edge middles, the corners drawn over the edge handles; the cursor over the center of the right handle's square is the horizontal resize cursor, over the top one's the vertical, over the bottom-right corner's the diagonal and over the top-right corner's the other diagonal; over the selection's pixel (1, 1), which a corner handle's 9 x 9 hit area would reach, the move cursor; Given the triangle, Then the handles stand round (1, 1) to (5, 5); while a rectangle is dragged out or a path traced, no handle; at 400 % and at the fit zoom the handles are 5 x 5 logical pixels and scroll with the picture; during a move of the selection they follow it | Verified by: automated (ui, rendering) + manual M1
- AS-2 (REQ-0015-03, REQ-0015-04, REQ-0015-05, REQ-0015-08) Given marked, When the user drags its right handle by (5, 0) and releases, Then it floats at (1, 1), 10 × 4, the status bar "10 × 4" while dragging and after, its red pixels at the columns 3 to 8 of the rows 2 and 3; When the user presses Esc, Then it is put down: 10 pixels changed, one step; Undo restores every pixel; Given the same stretch floating, When Undo is chosen before Esc, Then every pixel is as before and no step is added | Verified by: automated (core, the scale) + automated (ui)
- AS-3 (REQ-0015-05, REQ-0015-06) Given the red pixel (2, 2) selected alone with Free-Form Select (a path round it through the corners (2, 2), (3, 2), (3, 3) and (2, 3); with Select a press and release on one pixel is a click and marks nothing, REQ-0013-04), When the user drags the bottom-right handle by (2, 2), Then the selection is 3 × 3, all red, its shape the 9 pixels; put down, 3 pixels changed (the row 4, columns 2 to 4) | Verified by: automated (ui)
- AS-4 (REQ-0015-03, REQ-0015-05) Given marked, When the user drags the right handle by (-4, 0), releases, then drags it by (4, 0) and puts it down, Then no pixel differs and no step is added; Given marked again, When the user drags the right handle by (-10, 0), Then it is 1 × 4 at (1, 1), no mirroring; Given marked again, When the user drags the left handle by (10, 0), Then it is 1 × 4 at (5, 1); Given marked again, When the user drags the left handle by (-1, 0), Then it is 6 × 4 at (0, 1), its red pixels at the columns 1 to 4; put down, 2 pixels changed (Pointer note, Image Transforms, 2026-10-27: the right handle by (-10, 0) now gives 5 × 4 at (-4, 1) and the left handle by (10, 0) 5 × 4 at (6, 1), both mirrored, REQ-0018-11, 0018's AS-14.) | Verified by: automated (ui)
- AS-5 (REQ-0015-07) Given marked (5 × 4), When the user drags the bottom-right handle by (5, 1) with Shift, Then it is 10 × 8; by (1, 5), 11 × 9; by (-3, -1), 2 × 2; by (5, -3), 10 × 8 (the width, ×2, differs more from 1 than the height, ×0.25); releasing Shift during the drag by (5, 1) gives 10 × 5 at once; Shift with the right handle by (5, 0) gives 10 × 4 | Verified by: automated (core, the rule) + automated (ui)
- AS-6 (REQ-0015-04) Given marked, When the user drags the right handle by (-2, 0) with Ctrl held at the press and presses Esc, Then it was 3 × 4 at (1, 1) and the picture is red at the columns 2 and 4 of the rows 2 and 3, white at the column 3: 2 pixels changed, one step; without Ctrl the columns 4 and 5 of the rows 1 to 4 are yellow: 10 pixels changed; Given the stretch of AS-2 floating, When the user drags its right handle by (-5, 0) with Ctrl held at the press and presses Esc, Then a copy 10 × 4 stays at (1, 1) under the selection, now 5 × 4 | Verified by: automated (ui)
- AS-7 (REQ-0015-06) Given the triangle, When the user drags its right handle by (5, 0), Then the selection is 10 × 5 and its shape 30 pixels (rows 1 to 5 holding 10, 8, 6, 4 and 2 from the left), and its line runs (1, 1) → (12, 1) → (11, 1) → (11, 2) → (9, 2) → (9, 3) → (7, 3) → (7, 4) → (5, 4) → (5, 5) → (3, 5) → (3, 6) → (1, 6) → (1, 7) → (1, 1); put down, 8 pixels changed; Given an L of Free-Form Select (the path through the corners (1, 1), (2, 1), (2, 4), (5, 4), (5, 5) and (1, 5): the column 1 of the rows 1 to 4 and the row 4 of the columns 1 to 4, all white), When the user drags its bottom-right handle by (-3, -3) to 1 × 1, Then the nearest-pixel sample, the pixel (3, 3), is not in the L, and the shape is the one pixel, white, made from the L's first pixel in reading order, (1, 1), never empty; dragged instead to 2 × 2, its shape is the 2 pixels of its lower row | Verified by: automated (core, the shape and the line) + automated (ui)
- AS-8 (REQ-0015-02) Given Select All (the whole picture selected), Then the selection's bottom-right handle lies where the picture's corner handle lies; a press there, or anywhere in the hit area of the picture's corner, right or bottom handle (inside the picture too), stretches the selection (the cursor the diagonal resize cursor at the corner) and the picture's size does not change; after Esc (no selection) the same press resizes the picture | Verified by: automated (ui)
- AS-9 (REQ-0015-03, REQ-0015-08) Given marked, When the user drags the bottom handle by (0, 20000) at 100 % (the pointer far outside the window), Then the height is 10000 and the view does not scroll; Given a stretch held, When the other button, Esc or Undo comes, Then the selection is 5 × 4 at (1, 1), marked, the picture unchanged, and the release does nothing; Enter, Delete, Select All, Redo and the arrow keys change nothing during it; When the next press comes with no button down (a lost release) or the window loses the focus, Then it ends as last shown; When Save is requested, Then it is put down as last shown and saved | Verified by: automated (ui)
- AS-10 (REQ-0015-09) Given memory made too small for the stretched pixels, When the user drags a handle of marked, Then the dialog "Could Not Stretch the Selection" appears, the selection is 5 × 4 at (1, 1), marked, and the release does nothing; Given a drag that stretched marked to 6 × 4 and then moves to a size whose memory cannot be had, Then the same dialog, and it is 5 × 4 at (1, 1), marked; Given memory too small for the lift, Then the dialog "Could Not Move the Selection" | Verified by: automated (core, the refusal) + automated (ui, the dialogs)
- AS-11 (REQ-0015-05) Given marked and smeared by (3, 0) (copies at the shifts 0 to 2, the selection at (4, 1)), When the user stretches it by its right handle by (-3, 0), Then it is 2 × 4 at (4, 1) and the copies keep their size; after Esc, red at the columns 4 to 6 of the rows 2 and 3: 8 pixels changed, one step; Given AS-2's stretch floating, When the user Shift-clicks it (a stamp), moves it down by (0, 5) and presses Esc, Then the stamp is 10 × 4 | Verified by: automated (ui)
- AS-12 (REQ-0015-10) Given a 2000 × 2000 and an 8000 × 8000 selection at 100 % and at the fit zoom, When the user drags a corner handle, Then the median time of a pointer event to the painted frame is measured and recorded in the Plan; a case over the 16 ms goes to JOD | Verified by: measurement (`perf-0015`, in the real session)

## Edge cases
- A selection 4 logical pixels or less high or wide: its handles' squares touch or overlap; a press in an edge handle's own square
  goes to it, and the corners are drawn over it (REQ-0015-02).
- A selection partly or wholly outside the picture: its handles are drawn where its rectangle is; a handle outside the canvas area
  cannot be pressed until the view shows it.
- During a stretch drag no zoom action works (REQ-0004-03); the view can still scroll by the wheel, and a window resize can change the
  fit zoom: the drag goes on by the pointer's movement in picture pixels.
- A stretch to another size and back to the size at the press, in one drag or several: the selection is lifted (as a move back to its
  place is) and adds no step if no pixel differs at the put-down (REQ-0013-11).
- A free-form shape whose stretched shape is smaller than its box (an L made 2 × 2 keeps 2 of its 4 pixels): the handles stay round
  the W x H box (REQ-0015-06).

## Out of scope
- Stretch and Skew by numbers (Image menu transforms, with Keep aspect ratio): the feature Image transforms.
- Smoothing of stretched pixels: always the nearest pixel (P4).
- Rotation by a handle.
- Flip Horizontal and Vertical of a selection, and flipping by dragging a handle past the opposite edge (REQ-0015-03 says no mirroring): the feature Image transforms.
- Stretching from the keyboard: the feature Accessibility.

## Open questions

## Plan
### Approach
The float keeps the pixels and shape as lifted (the original) beside the pixels and shape it has now, made from the original by one
pure core function (nearest pixel, REQ-0015-05, -06); a stretch replaces only the current ones. Copies keep the image they were left
with: each stamp names its image, and an image is kept while a stamp or the float uses it (REQ-0015-05). The geometry of a handle
drag (the new rectangle from the handle, the movement, the limits and Shift's ratio, REQ-0015-03, -07) is a second pure core
function, and the scaled path of a free-form selection (REQ-0015-06) a third. The canvas draws the handles after the line, hit-tests
them before the picture's handles and the selection (REQ-0015-02), and adds a stretch drag beside the move drag, with the same
lift, cancel, lost release and put-down functions. The put-down still writes the current pixels through the lay, one step.

### Files
- `src/core/selection.{c,h}`: the original and current images of `EaselFloat` (`ow`, `oh`, `orig_px`, `orig_mask`), the images of copies
  (`stamp_img`, `kept`), `easel_selection_scale`, `easel_selection_stretch`, `easel_selection_stretch_lazy`, `easel_selection_fill`,
  `easel_selection_complete`, `easel_stretch_rect`.
- `src/core/lasso.{c,h}`: `easel_lasso_scale_path`.
- `src/ui/canvas.{c,h}`: the handles (`easel_canvas_get_selection_handle_rect`, `easel_canvas_selection_handle_at`, the drawing and
  cursor code), the stretch drag (`begin_stretch`, `stretch_apply`, `restore_stretch`, `end_failed_stretch`, `stretch_restore_path`),
  `selection_mask`, the test hooks.
- `src/ui/window.c`: the dialog "Could Not Stretch the Selection" (the selection's failure, kind 3).
- `tests/core/test-selection.c`, `tests/core/test-lasso.c`, `tests/ui/test-window.c` (`/0015/` and the 0013 and 0014 cases the
  pointer notes move), `tests/perf/perf-0015.c`, `tests/meson.build` (ui-0015, perf-0015), `tests/mutants.txt`.

### Key interfaces (as built)
- `EaselSelectionResult easel_selection_scale (const uint32_t *px, const uint8_t *mask, int w, int h, int W, int H, uint32_t **out_px,
  uint8_t **out_mask, size_t max_bytes)` (the nearest-pixel rule; for a mask, the keep-one-pixel rule of REQ-0015-06; NO_MEMORY with
  nothing made).
- `EaselSelectionResult easel_selection_stretch (EaselFloat *f, int x, int y, int W, int H, size_t max_bytes)` (the float at (x, y),
  W x H, made from the original; on failure the float as it was) and `easel_selection_stretch_lazy` (the same memory had and refused,
  but only the 256 x 256 blocks asked for by `void easel_selection_fill (EaselFloat *f, int x0, int y0, int x1, int y1)` are made, the
  rest by `void easel_selection_complete (EaselFloat *f)`; a copy, a lay and a put-down complete first).
- `void easel_stretch_rect (EaselSelHandle handle, int x, int y, int w, int h, int dx, int dy, bool keep_ratio, int *nx, int *ny,
  int *nw, int *nh)` (REQ-0015-03, -07; pure, the core's own enum of the eight handles).
- `bool easel_lasso_scale_path (const EaselPath *src, int sx, int sy, int sw, int sh, int nx, int ny, int nw, int nh, EaselPath *out,
  size_t max_bytes)` (REQ-0015-06: a corner inside the shape box keeps its relative place, halves toward larger x and y; one outside
  keeps its distance from the nearest edge; out untouched on failure).
- `EaselStamp { int x, y; }` (8 bytes) with the parallel `int *stamp_img` of `EaselFloat` (NULL: all copies use the current image;
  `EASEL_IMAGE_CURRENT`, `EASEL_IMAGE_ORIGINAL`, or an index into `kept`, each kept image `{ w, h, px, mask }`); `EaselFloat` has the
  original `ow, oh, orig_px, orig_mask` (NULL until the first stretch), and the hole uses the original's size and shape.
- Canvas hooks: `easel_canvas_get_selection_handle_rect`, `easel_canvas_selection_handle_at`, `easel_canvas_set_selection_handles_shown`,
  `easel_canvas_get_stretch_made`, `easel_canvas_get_selection_copies`, `easel_canvas_get_selection_path` and the 0014 ones.

### Where it is put down, removed or cancelled
The functions of 0013 and 0014: `finish_selection` puts down (and completes a lazily stretched image first); `cancel_float` forgets;
`cancel_selection_drag` gains the stretch through `restore_stretch`: a lift this drag made is dropped (`drop_float`, marked again), else
a copy this drag left is cut (`cut_copies`) and the pixels are made afresh at the size of the press unless they are already at it,
and the line is scaled back (`stretch_restore_path`). A failed stretch ends in `end_failed_stretch`, which shows the dialog and calls
`restore_stretch`. A lost release or focus ends the stretch in `easel_canvas_end_stroke` (it completes the image), as a move.

### Every reader of what changes
- The float's size, pixels and shape (now the current image): `lay_float_at`, `easel_selection_bounds`, `easel_selection_put_down`,
  `easel_selection_stamp`, `easel_selection_smear` (core); in the canvas `append_float_outside`, `on_selection`, the trail's key and
  tiles (`trail_key_of`, `selection_preview`), `append_selection_tint`, `append_selection_outline`, the status bar's size,
  `easel_canvas_get_selection_mask`, and the canvas's own `sel_w`, `sel_h` and `sel_mask` (set from the float after a stretch).
- The hole: `in_shape` at the hole and `easel_selection_bounds` use the hole's own size and shape (the original's), no longer the
  float's.
- The stamps: `lay_float_at` for each stamp takes the stamp's image (`stamp_img`); `cut_copies` and the trail's epoch are unchanged in meaning.
- The press: `easel_canvas_press_timed` asks for a selection handle first, then the picture's handles, then the selection
  (REQ-0015-02); `update_cursor` and `easel_canvas_pointer_at` likewise; `easel_canvas_handle_at` (the picture's) stays as it is.
- Tests of 0013 and 0014 whose presses now fall in a selection handle's hit area (the pointer notes): the put-down clicks at
  (17, 2), the presses outside the selection, the outline and tint renders near the handles.

### What grows
The float holds the original image (w x h pixels and mask) and the current one (W x H); each stamp 8 bytes (its place) and, once a
stretch has made images differ, 4 bytes more in the parallel image list; each image kept for stamps its pixels and mask; a lazy image
also a flag per 256 x 256 block. Each of these buffers is checked on its own against the selection's memory limit (`sel_lift_max`) when it is
allocated, the original and the current image each; the limit is per allocation, not a total (REQ-0013-19 and REQ-0015-09 speak of
the memory of one lift, one stretch, one copy, which can be had or not). At most one image is made per stretch motion; the one it replaces is freed unless a stamp or the press of the drag
still uses it.

### Constitution check
P4: the nearest pixel only. P5: no cache beyond what REQ-0015-05 asks (the original). Core pure and tested headless; the ui calls it.

### Risks
- **Speed (REQ-0015-10):** an 8000 x 8000 stretch resamples 64 M pixels a motion; measured in T006 right after the first stretch
  (fallback, only after the measurement: resample only the part in view while the button is held and the whole at the release).
- **Memory, in numbers:** an 8000 x 8000 floating selection holds the original and the current image: 256 MB of pixels each, so
  512 MB for a rectangle (a free-form shape adds 64 MB of mask to each: 640 MB); each Ctrl copy or stamp made at that size keeps its
  image, another 256 MB (320 MB with a mask), unless it shares the image of an earlier copy at the same size. The selection's memory
  limit (`sel_lift_max`, the test hook) is unlimited in the app, so the real limit is what the system gives: when an image cannot be
  had, the stretch shows "Could Not Stretch the Selection" and the selection goes back to its state at the press (REQ-0015-09), and
  the lift's own failure shows "Could Not Move the Selection" (REQ-0013-19). The tests set the limit to reach each case.
- **Small selections:** the edge handles' own squares overlap the corners' (a 1 x 1 selection); the hit test and the draw order are
  checked at 100 % on 1 x 1 and 4 x 4 selections in T004.
- **The 0013 and 0014 tests** that click at (17, 2) and outside selections will press a selection handle once T004 adds hit areas;
  T004 moves them as the pointer notes say, so the gate of its group can pass.
- **The free-form line** after a stretch: the scaled corners and the walk between them; AS-7's line is checked in core in T009.

### Measurements
`perf-0015`, real session, no Easel window open, the machine of the earlier measurements (60 Hz display: a refresh is 16.7 ms), load
average 0.4 to 0.5, view 1353 x 775; a selection of the whole picture stretched by its bottom-right handle, one picture pixel a pointer
event, the first event (the lift and the first image) and the median of the next 15 to the painted frame, three runs (2026-10-22):

| Selection | Zoom | First event | Next events, median (worst) | Peak memory (before) |
|---|---|---|---|---|
| 2000 x 2000 | 100 % | 20 to 26 ms | 16.6 to 16.8 ms (26 to 64) | 169 to 189 MB (127) |
| 2000 x 2000 | fit (37 %) | 43 to 46 ms | 16.6 to 16.8 ms (39 to 46) | 235 MB (168 to 189) |
| **8000 x 8000** | 100 % | 191 to 238 ms | **136.1 / 139.5 / 139.4 ms** (142 to 158) | 1197 to 1212 MB (463 to 479) |
| **8000 x 8000** | fit (10 %) | 487 to 534 ms | **454.4 / 442.8 / 446.2 ms** (461 to 516) | 2061 to 2063 MB (1197 to 1212) |

The 2000 x 2000 cases are one display frame. The 8000 x 8000 cases are over it, 8 and 27 frames: reported to JOD before any fix
(REQ-0015-10). JOD's decision (2026-10-22): at 100 % and above only the part in view is made while the button is held, the whole at the
release; at the fit zoom nothing is built for now. Measured split: the core's `easel_selection_scale` alone takes 122 to 125 ms for 64 million pixels (a probe outside the
window), so at 100 % it is nearly the whole frame; at the fit zoom the other 330 ms are the 256 tiles of the picture, each laid over the
float and made a texture every frame. The memory is the original (256 MB), the current image (256 MB) and the tiles' buffers.

After the lazy stretch (T006; `perf-0015` again, three runs, 2026-10-22, load average 1.3 to 2.2 from other work, view 1353 x 775):

| Selection | Zoom | First event | Next events, median (worst) | Release: to return / to the painted frame | Peak memory (before) |
|---|---|---|---|---|---|
| 2000 x 2000 | 100 % | 24 to 34 ms | 16.4 / 17.0 / 16.7 ms (30 to 36) | 6, 0, 0 / 9, 16, 17 ms | 175 MB (127) |
| 2000 x 2000 | fit (37 %) | 41 to 43 ms | 26.3 / 16.8 / 17.1 ms (36 to 47) | 0 / 12 to 19 ms | 250 to 280 MB (174) |
| **8000 x 8000** | 100 % | 62 to 63 ms | **16.6 / 16.6 / 16.8 ms** (29, 157, 31) | **139, 0, 134 / 144, 16, 137 ms** | 982 to 984 MB (494) |
| **8000 x 8000** | fit (10 %) | 489 to 505 ms | **448.5 / 464.4 / 451.7 ms** (457 to 524) | 0 / 159 to 173 ms | 2061 to 2062 MB (981 to 982) |

At 100 % the 8000 x 8000 stretch is now one display frame an event (it was 136 to 140 ms); the release, which makes the other 90 % of the
image, takes 134 to 139 ms (the core resample alone: 122 to 125 ms), and the first event, which no longer makes the image, 62 ms (it was
191 to 238 ms); the peak memory dropped to 0.98 GB because the pages of the image that are not made are never touched. In one of the
three runs one event took 157 ms and the release then 0 ms: the whole image was made at that event (a frame laid tiles over the whole
image, probably while the view was still being laid out); not reproduced in the other two and explained below. At the fit zoom nothing
changed, as decided: about 450 ms an event, 2.06 GB peak (with the load of two runs, the 2000 x 2000 fit case once took 26 ms).

The memory columns: "(before)" and "was" are the process's peak resident memory (`VmHWM`, a high-water mark that only grows, and the
cases of one run share one process) read after the picture is open, the view painted and Select All done, just before the press: the
picture, its textures and what the earlier cases left. The number before it is the peak at the end of the stretch. For 8000 x 8000 the
stretch adds about 490 MB (982 less 494; 952 less 463), twice the 256 MB that a move of the same selection adds (the lift copies the
pixels once; a stretch also keeps the original beside the current image, REQ-0015-05, though the lazy image touches only the pages in
view while the button is held; the release makes the rest).

Accepted slow cases (JOD, 2026-10-22): 8000 x 8000 at the fit zoom (about 450 ms an event, 2.06 GB peak) and the release of an
8000 x 8000 stretch at 100 % (about 140 ms, the core resample alone being 122 to 125 ms). No display-resolution preview is built and the
cache key is not split. The one 157 ms event was looked into in five more runs before the gate (T015), below.

Five more runs before the gate (2026-10-22, T015; real session, no Easel window open, load average 0.65 to 2.3 rising with the runs, plus
one run before them), 8000 x 8000 at 100 %: next events, median 16.5 / 16.8 / 16.7 / 16.7 / 16.8 / 16.6 ms; first event 62 to 79 ms; peak
memory 952 to 982 MB (was 463 to 494 MB). The slow event appeared in all six runs (worst 150.8 to 180.1 ms), with the release at 0 ms to
return and 15 to 17 ms to the painted frame each time: those two release numbers are the early-completion artifact below, not the cost
of a release. The release that counts stays the one of the lazy-stretch table: 134 to 139 ms to return and 137 to 144 ms to the
painted frame at 8000 x 8000 and 100 % (the image is made then, in full). Its cause, found with a breakpoint on `easel_selection_complete`: the first
completion comes from `easel_canvas_buttons_released`, called by the canvas's `on_event` for a real pointer motion that reports no button
down (REQ-0007-18, a lost release). The benchmark presses and drags with calls, not events, so a real motion of the pointer over the
window, in the real session, ends the held stretch as a lost release does, which makes the whole image at that event. The product behaves as
specified (REQ-0015-08); the number is the benchmark's, not a stretch's cost, and the median is unaffected.

Checked with the pointer away from the window (JOD parked it, 2026-10-22; two runs of 8000 x 8000 at 100 %, load average 0.6): next events
median 16.5 / 16.8 ms, worst 30.2 / 30.7 ms (it was 150.8 to 180.1 ms in the six runs with the pointer over the window, and 195.3 and 186.5 ms in two
later runs made after the same start delay, when the pointer was probably not yet away), first event 61 / 62 ms, release 133 / 136 ms to return and 135 / 138 ms to the painted
frame, peak memory 968 MB (was 479 MB). With a line printed at every lost release there was none in these runs. So the slow event and the
0 ms release are the pointer's, as found; the header of `perf-0015.c` now says to keep the pointer outside the window.

### Scenario-to-test map
| AS | Test |
|---|---|
| 1 | ui `as1_handles_drawn`, `as1_handle_cursors` + manual M1 |
| 2 | core `as2_scale_pixels`, `as2_lazy_stretch_matches_full`, ui `as2_stretch_right`, `as2_stretch_part_in_view`, `as2_stretch_part_outside_picture`, `as2_zoom_changes_during_stretch` |
| 3 | ui `as3_one_pixel_to_three` |
| 4 | core `as4_scale_back_exactly`, ui `as4_stretch_limits_and_left` |
| 5 | core `as5_stretch_rect_ratio`, ui `as5_shift_keeps_ratio` |
| 6 | ui `as6_ctrl_stretch` |
| 7 | core `as7_scale_shape`, `as7_lazy_shape_keeps_one_pixel`, `as7_scaled_path_line`, `as7_scaled_path_rounding_and_margins`, ui `as7_stretch_free_form`, `as7_tint_while_lazy_stretch_held` |
| 8 | ui `as8_selection_handle_wins`, `as8_selection_handle_over_picture_handle_area`, `as8_put_down_while_stretch_held` |
| 9 | ui `as9_stretch_that_does_not_finish` |
| 10 | core `as10_scale_memory`, `as10_stretch_memory`, `as10_lazy_stretch_memory`, `as10_copies_list_counted_at_stretch`, ui `as10_stretch_dialog` |
| 11 | core `as11_copies_keep_their_image`, ui `as11_copies_and_stretch` |
| 12 | `perf-0015` (real session) |

## Tasks
Every task gets a `Result:` line when it is ticked: the test that failed first and its message, or `characterization: <why there
was no red run>` with the mutant that proves it; a manual step, one line of what JOD saw.

Group A, the core (no ui change)
- [x] T001 The scale of pixels and shape (REQ-0015-05, REQ-0015-06; AS-2, AS-4, AS-7) - done when core `as2_scale_pixels` (5 x 4 to 10 x 4: the red at the columns 2 to 7 of the box), `as4_scale_back_exactly` (5 to 1 to 5 from the original gives it back), `as7_scale_shape` (the triangle at 10 x 5: 30 pixels, rows of 10, 8, 6, 4, 2; the L at 1 x 1: one pixel, (1, 1)'s color; the L at 2 x 2: its lower row) and a held-against-counted case pass after a red run, each test and its function in two edits
  Result: red by build first (`implicit declaration of function 'easel_selection_scale'`: the tests `as2_scale_pixels`, `as4_scale_back_exactly`, `as7_scale_shape` and `as10_scale_memory` were written and built red in one edit, the function in the next), then green: core `easel_selection_scale` (the nearest pixel at the new pixel's center, the shape by the same sample, the keep-one-pixel rule with the footprint, the bytes of pixels and mask counted and overflow refused). Test change after the first run, mine: the overflow case first asked for 100000 x 100000 (40 GB, which this machine's overcommit grants); it now asks for INT_MAX x INT_MAX. Mutants of REQ-0015-05, -06 and -09: 6 of 6 killed (the sampling at the left and top edge, an empty shape, the last pixel kept, every pixel in the shape, the mask not counted); "a size whose bytes overflow is not refused" is equivalent (an int size cannot wrap 4 W H, and malloc refuses the huge request) and was removed. Full isolated suite 43 of 43, `mutate.sh --lint` 518 mutants, 0 stale.
- [x] T002 The rectangle of a handle drag (REQ-0015-03, REQ-0015-07; AS-4, AS-5, AS-9) - done when core `as5_stretch_rect_ratio` (each handle's edges; 10 x 8, 11 x 9, 2 x 2, 10 x 8 for (5, -3); the 1-pixel stop beside the opposite edge for the right and the left handle; 10000 and the ratio kept at the limit; an edge handle ignores Shift) passes after a red run
  Result: red by build first (`unknown type name 'EaselSelHandle'`, `EASEL_SEL_HANDLE_TL` undeclared: core `as5_stretch_rect_ratio` was written and built red in one edit, `easel_stretch_rect` in the next), then green: each handle moves only its own edges (the eight, marked dragged through (2, 1)); the 1-pixel stop beside the opposite edge for the right, left and top-left handles, no mirroring; 10000 for the right, bottom and left handles with the fixed edge staying; AS-5's four Shift cases (10 x 8, 11 x 9, 2 x 2, 10 x 8), an edge handle ignoring Shift, the width leading on a tie (4 x 2 by (-2, 1): 2 x 1), halves up (2 x 3 by (1, 0): 3 x 5), the top-left corner keeping the bottom-right one, the ratio kept at 10000 (1 x 10 by (3000, 0): 1000 x 10000) and the limit applied before the comparison. Nine mutants of REQ-0015-03 and -07 (mirroring, the two limits, the tie, the wrong leader, halves down, an edge handle keeping the ratio, the ratio broken at the limit, the wrong fixed corner): 9 of 9 killed (core, under a second each). Full isolated suite 43 of 43, `mutate.sh --lint` 527 mutants, 0 stale.
- [x] T003 The float stretched: the original and current images, the hole's own size, the copies' images (REQ-0015-04, REQ-0015-05, REQ-0015-09; AS-10, AS-11) - done when core `as11_copies_keep_their_image` (a stamp before a stretch keeps its size, one after has the new size), `as10_stretch_memory` (a refused stretch leaves the float as it was, and later motions of the same drag likewise) and the 0013 and 0014 core cases pass after a red run; group A gate: all core tests, `mutate.sh` for the new core mutants, `--lint`
  Result: red by build first (`implicit declaration of function 'easel_selection_stretch'`: core `as11_copies_keep_their_image` and `as10_stretch_memory` written and built red in one edit), then green: the float keeps `px`, `mask`, `w` and `h` as the current image (so the canvas and the 0013 and 0014 code read it as before, no ui change was needed) and gains `ow`, `oh`, `orig_px` and `orig_mask` (the pixels as lifted, set at the first stretch from the float's own image, no copy), `stamp_img` (the image of each copy, NULL while all use the current one, so a copy still costs 8 bytes and the 0013 and 0014 memory tests are unchanged) and `kept` (the images of copies made before a later stretch); the lay, the bounds and the hole read an `ImageView` of the image they mean; `easel_selection_stretch` makes the new image from the original with `easel_selection_scale` and has everything it needs before it changes anything. AS-11: a copy before the first stretch stays 5 x 4, one between the stretches stays 10 x 4, the float 3 x 4 is made from the original (20 reds: 6, 12 and 2), the hole stays 20 yellow pixels. AS-10: a refused stretch (exact bytes minus one, with and without a mask) leaves the pointer, size, place and copies as they were, also after an earlier stretch succeeded. Two survivors of my first mutants ("a stretch is made from the current image", "the hole takes the stretched size") showed that the red block's middle pixel and the copies hid the difference; the test `stretch_keeps_the_original_and_the_hole` (2 wide, then 5 wide gives the lifted pixels back; the hole's bounds (1, 1) to (5, 4) at 12 x 6) kills both. Seven new mutants of REQ-0015-05 and -09 killed (7 of 7 of REQ-0015-05 with the earlier ones), the five mutants of 0013 and 0014 that quoted the lay's loop rewritten for the view and killed. valgrind on `test-selection`: no error, no leak. Full isolated suite 43 of 43, `mutate.sh --lint` 534 mutants, 0 stale. Group A gate (2026-10-22): all core tests pass (`test-selection`, `test-lasso`, `test-lattice`), the full isolated suite 43 of 43, `mutate.sh` for REQ-0015-03, -05, -06, -07 and -09: 22 mutants: 22 killed, 0 killed by crash, 0 inconclusive, 0 survived, 0 errors in 24 s, `--lint` 534 mutants, 0 stale.

Group B, the handles in the window
- [x] T004 The handles drawn (REQ-0015-01; AS-1) - done when ui `as1_handles_drawn` (eight navy 5 x 5 squares at the corners and edge middles of marked and of the triangle, corners over edges on a 4 x 4 and a 1 x 1 selection, none while dragging out or tracing, the same logical size at 400 % and the fit zoom and in a scrolled view, following a move) passes after a red run; then manual M1: JOD looks at the handles at 100 %, 400 % and the fit zoom, with Select and Free-Form Select
  Result: red by build first (`implicit declaration of function 'easel_canvas_get_selection_handle_rect'`: ui `as1_handles_drawn` written and built red), then green with the canvas's `easel_canvas_get_selection_handle_rect` (a corner's inner corner at the rectangle's corner, an edge handle centered and rounded down, in logical pixels; not limited by the guides, so a press can hit them) and `append_selection_handles` (navy 5 x 5, the edge handles first, after the line, before the picture's handles, none with the guides hidden). The test renders the window and compares the navy pixels of the render with the union of the eight squares, pixel by pixel, for a marked rectangle at 100 %, the same moved to (11, 4), at 400 % at the top left and scrolled by 6, and the triangle of Free-Form Select; none while a rectangle is dragged out or a path traced, none with the guides hidden, none and no geometry without a selection. Two test changes of mine after the first runs: the zoom keeps the view's center, so at 400 % the test shows the top left first, and an expectation of the corner draw order is not testable (the handles are one color, so the order is unobservable). Group B's pointer notes already reached four 0013 and 0014 cases (`as2_mark_selection`, `as21_trace_and_release_dashes`, `as23_line_thickness_by_zoom`, `as24_tint_of_the_selection`: navy handles over the pixels they check): they call a new test hook, `easel_canvas_set_selection_handles_shown`, as they hide the guides elsewhere, since their subject is the line and the tint; the other 0013 and 0014 cases pass as they are. `ui-0015` added to `tests/meson.build`. Mutants of REQ-0015-01, 7 of 7 killed, each also run alone (applied by hand against `as1_handles_drawn`: killed): drawn with the guides hidden, an edge handle not centered, rounded up, a corner inside the rectangle, 4 pixels wide, not drawn, not following a move. Full isolated suite 44 of 44, `mutate.sh --lint` 541 mutants, 0 stale. Manual M1 (JOD, 2026-10-22): pass, at 100 %, 400 % and the fit zoom with Select and Free-Form Select.
- [x] T005 Their hit areas, cursors and priority; the 0013 and 0014 presses they now catch (REQ-0015-02; AS-1, AS-8) - done when ui `as1_handle_cursors` (each direction's cursor at the center of each handle's square; the move cursor on (1, 1); the own square first on a 4 x 4 selection) and `as8_selection_handle_wins` (Select All: the bottom-right press stretches, after Esc it resizes the picture) pass after a red run, and the 0013 and 0014 cases of the pointer notes (the clicks at (17, 2) and the presses outside a selection moved to (3, 9), the handle press of 0013 AS-7 at the picture's handle center, the outline, tint and render checks away from the handles) pass; group B gate: the full isolated suite, `mutate.sh` for REQ-0015-01, -02 and for REQ-0013 and REQ-0014, `--lint`
  Result: red by build first (`implicit declaration of function 'easel_canvas_selection_handle_at'`: ui `as1_handle_cursors` and `as8_selection_handle_wins` written and built red), then green: `easel_canvas_selection_handle_at` (a handle's own 5 x 5 square first, then the 9 x 9 areas less the selection's rectangle, a corner before an edge handle in each), the press (a press on a handle of the selection is taken before the picture's handles and before the selection, whichever button: nothing happens until T006) and the cursors (the resize cursor of the handle's direction before the picture handle's: `ew-resize`, `ns-resize`, `nwse-resize`, `nesw-resize`). Tests: each handle's cursor and hit at its square's center, on the 5 x 4 and on a 4 x 4 selection (where every edge handle's square lies in the corners' areas); the move cursor inside the rectangle where a corner's area reaches; the corner winning where areas overlap (0.5, -5.5) and (0.5, -3.5); the crosshair away; after Select All a press on the bottom-right and right handles is the selection's (no picture resize, the selection stays, the secondary button too), after Esc the same press resizes the picture; a floating selection stays floating under a press on its handle. The pointer notes' 0013 and 0014 cases, found by running every case of both suites: seven failed on a press or a cursor that now meets a handle (`as2_mark_selection` and `as2_trace_selection`: a click away before a drag whose press began on a handle of the earlier selection; `as3_move_selection`: the outline's cursor and press at 400 % on the ring between the top-left and left handles, since at 100 % the whole outline of a 5 x 4 selection lies in handle areas; the put-down clicks at (17, 2) moved to (3, 9) in `as3_move_selection`, `as4_ctrl_copies`, `as12_drags_that_do_not_finish` and `as13_selection_dialogs`, but not in `as5_arrow_keys`, whose selection sits at (2, 6); `as5_press_outside_shape`: (7, 7) lies in the bottom-right handle's square, (14, 14) does not); after them every 0013 and 0014 case passes. Mutants of REQ-0015-02: 7 of 7 killed, each in 2 to 4 s and so run again alone (applied by hand against the two tests: all killed); "only the primary button is taken by a handle" is equivalent (with Select a secondary press away from the selection's pixels does nothing, REQ-0013-17) and was removed. A mutant of REQ-0013-13 that no longer built after T003 (the lay's float argument is an image view with no hole color) was rewritten and killed. Group B gate (2026-10-22): full isolated suite 44 of 44, `mutate.sh` for REQ-0015-01 and -02 and every REQ-0013 and REQ-0014 id: 199 mutants: 197 killed, 0 killed by crash, 0 inconclusive, 0 survived, 1 error (the REQ-0013-13 mutant above, rewritten and run again: killed), 1 long-only (REQ-0014-16, not run); `--lint` 548 mutants, 0 stale; 0 warnings.

Group C, stretching
- [x] T006 Stretch a marked or floating selection in the window (REQ-0015-03, REQ-0015-04, REQ-0015-05, REQ-0015-08; AS-2, AS-4, AS-9) - done when ui `as2_stretch_right` (10 x 4, the status bar, 10 changed, one step, Undo, Undo before Esc) and `as4_stretch_limits_and_left` (back to the original adds no step; the 1 x 4 at (1, 1) and at (5, 1); 6 x 4 at (0, 1), 2 changed; the height 10000 by (0, 20000), the view not scrolled) pass after a red run
  Result: red first: ui `as2_stretch_right` (`'easel_canvas_is_stroking (canvas)' should be TRUE`: a press on a handle started nothing) and `as4_stretch_limits_and_left` (`'selection_is (win, 1, 1, 1, 4)' should be TRUE`), then green with the canvas's stretch drag: `begin_stretch` at a primary press on a handle (the rectangle and pointer at the press kept), `stretch_apply` at every motion (the pointer's movement since the press in logical pixels over the logical pixels of a picture pixel, rounded, through `easel_stretch_rect`; the first movement to another size lifts a marked selection; `easel_selection_stretch` makes the float from the original; the selection's rectangle and the status bar follow), the end at the release or at a lost release, and `selection_mask` (the float's mask while it floats, the canvas's own copy while it is marked) for the readers of the shape. AS-2: 7 x 4 at the first motion (+2) and 10 x 4 at the second (+5, measured from the press, not the last motion), "10 × 4", floating at (1, 1), the red at the columns 3 to 8 of the rows 2 and 3 in the render, the picture unchanged, Esc: 10 pixels changed, Undo restores, Undo before Esc forgets it and adds no step. AS-4 and AS-9: smaller by 4 and bigger by 4, put down: no step; the right handle by (-10, 0): 1 x 4 at (1, 1); the left by (10, 0): 1 x 4 at (5, 1); the left by (-1, 0): 6 x 4 at (0, 1), the red at the columns 1 to 4, 2 pixels changed; the bottom by 20000 rows (the pointer far outside the window): 5 x 10000, the view unmoved; the movement rounded (0.4 of a pixel none and not lifted, 0.6 one, back to 0.4 the size at the press and still floating). Not yet built, by plan: Ctrl (T008), the free-form line and path scaled (T010; until then a stretched free-form selection shows its pixels and tint right but the dashed line stays the traced path, which JOD saw in his first screenshots), Shift (T011), Esc, the other button and Undo during the drag and the failure's restore (T012, T013: a refused stretch ends the drag where it is for now). JOD's two remarks of 2026-10-22, taken at once: the picture's resize handles are drawn under the selection's tint and floating pixels where they overlap (REQ-0015-01 gained the sentence; ui `as1_picture_handles_under_selection` red first, `render_near ... tinted (YELLOW)` false at the corner handle, then green; the mutant that draws them over is killed). Mutants of REQ-0015-03, -04 and -08 (7 for the drag): 7 of 7 killed, the ones under 5 s run again alone against the two tests (all killed), and the 11 mutants of earlier features whose quoted lines the drag changed (REQ-0007-16 and -18, REQ-0011-13, REQ-0012-07, REQ-0013-09, REQ-0014-07, -08, -14 and -17 twice) rewritten and killed: 77 of 77 killed in the run of the requirements touched. Full isolated suite 44 of 44, `mutate.sh --lint` 556 mutants, 0 stale. Added at JOD's decision after T007 (2026-10-22), at 100 % and above only the part in view is made while the button is held: core `easel_selection_stretch_lazy` (the memory of the whole image had at once and refused as the full stretch's is, so REQ-0015-09 reads as written: a failure shows at the first movement), `easel_selection_fill` (the 256 x 256 blocks that meet a rectangle, made by the same `scale_region` the full scale uses) and `easel_selection_complete` (the rest, and a shape lost by the nearest pixel keeps its one pixel then); the canvas stretches lazily at zoom 100 and above, makes the part its tiles and the float's outside part read, and completes at the release, at a lost release and before any put-down. Tests: core `as2_lazy_stretch_matches_full` (40 random rounds: the pixels of a filled rectangle equal the full scale's, the other blocks are not made, the sizes 257 and 513 put a block's last column alone, the completion equals the full scale, with a shape too), `as7_lazy_shape_keeps_one_pixel`, `as10_lazy_stretch_memory`, built red (the functions were missing); ui `as2_stretch_part_in_view` (a 3000 x 2000 picture: made < half of the pixels while held, the whole at the release; the window shows while held what the put-down shows, at 100 % with a scroll while held, at 400 % scrolled, and at 100 % with the float taken left of the picture), `as2_stretch_part_outside_picture` (the float's part more than a block outside the picture) and `as8_put_down_while_stretch_held` (a pixel the view never showed is the right one after Save-like put-down while held); the first run was red by switching the lazy path off (`made * 2 < total`: 6149000 against 3074500), and the test grew after survivors: the tile edge on a block's first pixel (the selection at (243, 243)), a negative origin, a float wider than the picture by more than a block. Mutants: 12 of the lazy part killed in the tool, the 7 ui ones also run alone (all killed); six survivors during the work were gaps of mine, closed by those cases (block granularity hid ±1 and over-filling errors); one T001 mutant set rewritten for the refactor. `perf-0015` after: the Plan's Measurements. Full isolated suite 44 of 44, `mutate.sh --lint` 569 mutants, 0 stale, valgrind on `test-selection` clean.
- [x] T007 Measure the stretch (REQ-0015-10; AS-12), right after T006 - done when `perf-0015` (2000 x 2000 and 8000 x 8000 at 100 % and the fit zoom, the median of a pointer event to the painted frame, the peak memory) has run three times in the real session with no Easel window open, with the load average noted, and its numbers are in the Plan; if the 8000 x 8000 case is over one display frame, the numbers go to JOD before any fix and the tasks below wait
  Result: `perf-0015` written (the selection of the whole picture, its bottom-right handle, one pixel a pointer event, at 100 % and the fit zoom); it ran three times in the real session with no Easel window open and the load average noted (0.4 to 0.5); the numbers are in the Plan's Measurements. The 2000 x 2000 selection is one display frame at both zooms (16.6 to 16.8 ms medians). The 8000 x 8000 selection is over: 136 to 140 ms at 100 % and 443 to 454 ms at the fit zoom, with 1.2 GB and 2.06 GB of peak memory. The core resample alone is 122 to 125 ms for that size (a probe, `easel_selection_scale` 8000 x 8000 to 8002 x 8002). Reported to JOD before any fix; the tasks below wait for his decision.
- [x] T008 Ctrl at the press: no hole, a copy of a floating selection (REQ-0015-04; AS-6) - done when ui `as6_ctrl_stretch` (2 changed with Ctrl, 10 without; the floating copy 10 x 4 under the 5 x 4 selection) passes after a red run
  Result: ui `as6_ctrl_stretch` was written first and ran red at its floating part (`easel_canvas_get_selection_copies (canvas) == 1`: 0): the marked part (`characterization`: a marked selection lifted with the press's Ctrl and no hole) passed at once, because `begin_stretch` already read Ctrl into `sel_copy` for the lift (T006), proven by the mutant "Ctrl is ignored at the press of a stretch" (killed); the floating part needed the copy: a Ctrl press on a floating selection leaves one copy where it was, with the size and pixels it has, at the first movement to another size (`stretch_copied`, `easel_selection_stamp`, which re-points the copy to the image it has before the stretch replaces it). The test: marked with Ctrl by (-2, 0): 3 x 4 over the original, the red at the columns 2 and 4, no cyan, no copy, 2 changed, Undo; without Ctrl: the columns 4 and 5 of the rows 1 to 4 cyan, 10 changed; floating (AS-2's 10 x 4) with Ctrl at the press only and two motions: one copy (not one per motion), the selection 5 x 4, put down: the red at the columns 2 to 4 and 6 to 8 (the copy's under the columns the selection does not cover); without Ctrl no copy and the columns 6 to 8 stay. Mutants of REQ-0015-04: 4 new killed (Ctrl ignored, no copy of a floating selection, a copy at every motion, a copy for a marked selection too), each also run alone. Full isolated suite 44 of 44, `mutate.sh --lint` 573 mutants, 0 stale.
- [x] T009 Copies and a stretch in the window (REQ-0015-05; AS-11) - likely to pass already through T003's stamps: if so, `characterization`, proven by the mutant "a stamp is laid with the float's current image" - done when ui `as11_copies_and_stretch` (8 changed, red at the columns 4 to 6; the stamp 10 x 4) passes
  Result: characterization: ui `as11_copies_and_stretch` passed on its first run (T003's copy images and `easel_selection_stretch`'s re-pointing of the copies already do what AS-11 asks); proven by the mutant "a copy is laid with the float's current image, not the one it was made with", which the ui test kills when it is the only test run (6 s). The test: marked, smeared by (3, 0) with Shift (three copies, the selection at (4, 1)), its right handle by (-3, 0): 2 x 4 at (4, 1), still three copies, put down: the red at the columns 4 to 6 of the rows 2 and 3, 8 pixels changed, Undo restores; AS-2's stretch floating (10 x 4), Shift-clicked (a stamp), moved down by 5: the stamp is 10 x 4 at (1, 1) and the float 10 x 4 at (1, 6). `mutate.sh --lint` 574 mutants, 0 stale; 17 of 17 REQ-0015-05 mutants killed.
- [x] T010 A free-form selection stretched: its shape and its line (REQ-0015-06; AS-3, AS-7) - done when core `as7_scaled_path_line` (AS-7's line, corner by corner) and ui `as7_stretch_free_form` (10 x 5, 30 pixels, the line rendered on the grid lines, 8 changed; the L at 1 x 1) and `as3_one_pixel_to_three` (3 x 3, 3 changed) pass after a red run, the path function and its test in two edits
  Result: core `as7_scaled_path_line` and `as7_scaled_path_rounding_and_margins` written first and red by the build (`easel_lasso_scale_path` undeclared), then the function in a second edit: a corner inside the shape box keeps its relative place (halves to the larger x or y), one outside it keeps its distance from the nearest edge; AS-7's 14 runs equal `easel_lattice_runs` of the scaled corners (1, 1), (12, 1), (1, 7). Canvas: the path as traced is kept (`sel_path0` with its box) at the first stretch and the line is scaled from it at every stretch, placed at the selection; a hook `easel_canvas_get_selection_path` shows the corners and the offset. Characterization for the ui cases `as7_stretch_free_form` (10 x 5, 30 pixels in rows of 10, 8, 6, 4, 2; corners (1, 1), (12, 1), (1, 7); moved by (3, 2) the offset follows; dragged back to 5 wide the corners are scaled from the traced path: (4, 3), (10, 3), (4, 9); Undo restores) and `as3_one_pixel_to_three` (3 x 3 all red, row 4 columns 2 to 4 changed, 3 pixels): I wrote the canvas code before the tests (a slip against test-first), so both passed on their first run; proven by the mutants "the line is not scaled with the shape", "scaled from a box one narrower" and "not placed at the selection" (killed, 6 to 7 s) and three core mutants of the corner rule (killed). 10 of 10 REQ-0015-06 mutants killed; `mutate.sh --lint` 580, 0 stale. The line rendered on the grid lines is not drawn-checked beyond the hook (the draw uses the same path); the L at 1 x 1 is core `as7_scale_shape` (T001). JOD's test of 2026-10-22 (the zoom changes while a stretch is held, across 100 %): ui `as2_zoom_changes_during_stretch` (a 3000 x 2000 picture, held at the fit zoom, then at 100 % with the pointer over the pixel 60 right and 36 below the press: the selection 2660 x 1636, and the window's render while held equals the put-down's) ran red first (`selection_is (win, 243, 243, 2660, 1636)` false: the press was kept in logical pixels of the zoom at the press), then green with the press and the pointer kept in picture pixels. The zoom actions are off while a drag is held, so the test calls `easel_canvas_set_zoom` as the fit zoom's follow-up of a window resize does.
- [x] T011 Shift on a corner (REQ-0015-07; AS-5) - done when ui `as5_shift_keeps_ratio` (10 x 8, 11 x 9, 2 x 2, 10 x 8; Shift released during the drag; the right handle ignores it) passes after a red run
  Result: ui `as5_shift_keeps_ratio` ran red at the Shift release (`selection_is (win, 1, 1, 10, 5)` false: the size stayed 10 x 8 until the next motion), then green with `easel_canvas_set_shift` applying the stretch again while one is held. The rest was characterization through T002's `easel_stretch_rect` and the live `shift` the drag already passed: 10 x 8, 11 x 9, 2 x 2, 10 x 8 and the right handle's 10 x 4 passed at once, proven by the mutant "Shift is ignored by a stretch" (killed, 7 s). The new mutant "the ratio does not follow Shift" is killed (8 s); 8 of 8 REQ-0015-07 mutants killed; `mutate.sh --lint` 582, 0 stale; full isolated suite 44 of 44.
- [x] T012 Stretches that do not finish (REQ-0015-08; AS-9) - partly likely to pass already through the move's cancel and lost-release paths: what passes first is `characterization`, proven by the mutants "a cancelled stretch keeps its new size" and "a lost release drops the stretch" - done when ui `as9_stretch_that_does_not_finish` (the other button, Esc, Undo; Enter, Delete, Select All, Redo and the arrows ignored; a lost release by the next press with no button down; a lost focus; Save) passes
  Result: ui `as9_stretch_that_does_not_finish` ran red at the other button (`easel_canvas_is_stroking (canvas)` should be FALSE: the drag was not cancelled), then green with the stretch's cancel in `cancel_selection_drag`: a marked selection lifted by the drag is marked again (the float dropped), a floating one has the copies of this drag cut (`stretch_stamps0`) and its pixels made afresh at the size of the press, the rectangle and the free-form line (`stretch_restore_path`) are those of the press, the held button is waited for. The test covers the other button, Esc and Undo for a marked selection and for a floating one with a Ctrl copy (10 x 4 again, floating, no copy, 10 pixels on put-down), Enter, Delete, Select All, Redo and the arrows doing nothing, a lost release and a lost focus (7 x 4 as last shown, all of it made), and Save (put down and saved). Characterization for the lost release, the lost focus, Save and the ignored keys (T006's end paths and the stroking guards already did them), proven by the mutant "a lost release leaves the stretch unfinished" (killed) and T006's mutants of the put-down; one survivor of mine ("a cancelled stretch does not wait for the held button") closed by asserting `easel_canvas_is_waiting_for_release`. 6 new mutants killed, 16 of 16 REQ-0015-08 and REQ-0013-18 killed (one REQ-0013-18 mutant lengthened to stay unique); `mutate.sh --lint` 588, 0 stale; full isolated suite 44 of 44. A failure of the afresh stretch at the cancel shows the stretch dialog and leaves the pixels at the dragged size (memory the drag already held, so not expected; T013).
- [x] T013 The memory dialogs (REQ-0015-09; AS-10) - done when ui `as10_stretch_dialog` (the first motion and a later one refused: "Could Not Stretch the Selection", 5 x 4 at (1, 1) marked; the lift refused: "Could Not Move the Selection") passes after a red run; group C gate as group B
  Result: ui `as10_stretch_dialog` ran red first (`assert_alert`: the heading was "Could Not Move the Selection", not "Could Not Stretch the Selection": the window had no text for a refused stretch), then green with the dialog for kind 3 in `show_selection_error` and `end_failed_stretch` putting the selection back through the same `restore_stretch` that Esc, the other button and Undo use (T012 refactored into it). The test: the lift refused (limit 10): "Could Not Move", 5 x 4 marked; the lift fits and the stretched pixels do not (limit 80): "Could Not Stretch", 5 x 4 at (1, 1) marked, not pending, no step, the release does nothing; 6 x 4 fits (limit 100) and the later 10 x 4 does not: 5 x 4 marked; floating at 6 x 4, a refused movement leaves it 6 x 4 and floating; Ctrl at the press with no memory for the copy: the dialog, 6 x 4, no copy. The Ctrl phase ran red once more at its first run (no dialog: the cancel remade the pixels under the same tiny limit and a second refusal came), so `restore_stretch` makes the pixels afresh only when they are not already at the size of the press. Mutants: 3 new (the dialog text, the selection left at the size reached, the needless remake) killed; 16 of 16 REQ-0015-08 and -09 killed after the refactor (five T012 mutants rewritten); `mutate.sh --lint` 591, 0 stale; full isolated suite 44 of 44.
- [x] T014 Manual M2 with the real mouse (REQ-0015-02, REQ-0015-03, REQ-0015-04, REQ-0015-07, REQ-0015-08) - done when JOD has dragged each of the eight handles with Select and with Free-Form Select, Shift on the corners, Ctrl on a handle of a marked and of a floating selection, and Esc during a drag, at 100 % and 400 %
  Result: JOD tried it with the real mouse (2026-10-22): "all works" (the eight handles with Select and Free-Form Select, Shift on the corners, Ctrl on a marked and a floating selection, Esc during a drag, at 100 % and 400 %).
- [x] T015 Gate (all) - done when the full isolated `meson test` passes, `mutate.sh` for every REQ-0015 id and `--lint` are clean, `tools/spec.sh lint --names 0015` is clean, and the Plan's interfaces and map match the names as built; and the manual M1 of the handles' look (REQ-0015-01, REQ-0015-02; AS-1, AS-8): JOD has looked at the handles at 100 %, 400 % and the fit zoom, over the tint and the line, on a marked and on a free-form selection, and on a 1 x 1 and a 4 x 4 selection, and has answered; T015 stays open until then
  Result: full isolated suite 44 of 44, warning-free; `mutate.sh` for every REQ-0015 id: 81 of 81 killed (0 by crash, 0 inconclusive, 0 survived); the group C gate for REQ-0013, -0014, -0015: 263 of 264 killed, 1 long-only not run; `mutate.sh --lint` 593, 0 stale; `spec.sh lint --names 0015` ok; the Plan's files, interfaces, put-down functions, growth and scenario-to-test map brought to the names as built; the Measurements hold the accepted slow cases, the real release, the memory baseline and the pointer artifact (checked with the pointer parked: worst 30 ms, release 133 to 136 ms). Manual M1 (2026-10-22): JOD looked at the handles at 100 %, 400 % and the fit zoom, over the tint and the line, on a marked and a free-form selection and on 1 x 1 and 4 x 4 selections: M1 confirmed ok by JOD (2026-10-22).

- [x] T016 The copies' image list counted at a stretch (REQ-0015-09, REQ-0015-05; AS-10; Check F2) - done when a core test that compares the memory held by the copies' list (12 bytes per entry of capacity: 8 of place, 4 of image) with the memory counted, and a ui case where the first stretch after copies exist is refused for that list ("Could Not Stretch the Selection", the selection as at the press, the copies kept), ran red first; then the check in `stretch_to` before the list is allocated; mutants for it against the short suite; the full isolated suite, `mutate.sh --lint`, `spec.sh lint` and `--names 0015`; the Check section updated for what changed; and Check F4 and F5: a render test of the tint of a free-form selection while a lazy stretch is held (red first, or characterization with the mutant that proves it), and AS-7's "put down, 8 pixels changed" asserted in `as7_stretch_free_form` (red against a mutant that changes the count); the mutants of REQ-0015 and of the touched REQs against the short suite, with counts
  Result: red first, both: core `as10_copies_list_counted_at_stretch` (`CHECK failed: easel_selection_stretch (&f, 1, 1, 10, 4, 191) == EASEL_SELECTION_NO_MEMORY`: 9 copies, a list of 16 entries, 192 bytes with the places; the stretch was done) and the ui case added to `as10_stretch_dialog` (no dialog: `assert_alert`, `dialog` should not be NULL); then green with the check in `stretch_to` before the image list is allocated (`cap * (8 + 4)` against the limit, the way `easel_selection_stamp` counts the list when it grows; a refusal frees the new image and leaves the float and its copies as they were, so the dialog shows and the selection is as at the press). The list is 12 bytes per entry of capacity. Check F5: the "put down, 8 pixels changed" of AS-7 added to `as7_stretch_free_form` (characterization: it passed on its first run, the logic being T001's and T010's; proven by the mutant "a stretched free-form shape is the whole rectangle", killed, 8 s). Check F4: ui `as7_tint_while_lazy_stretch_held` (a 3000 x 2000 picture, the lower-left triangle 2600 x 1600 made 2700 wide by its right handle, held at 100 %: less than half of the image made; three pixels in the shape tinted and three outside it not, also in blocks of 256 only the tint reads, held and after the release); characterization, it passed on its first run (the tint reads the mask the tile drawing has filled), proven by three mutants, all killed (the tiles in view not made; the tint the whole box while held; no tint while held). Mutants: 9 new (3 core and 1 ui for the list, 1 for the shape, 4 for the tint and tiles), 89 of 89 REQ-0015 killed against the short suite (419 s), 42 of 42 of REQ-0015-05, -06 and -09 in the first run of the new ones; `mutate.sh --lint` 601, 0 stale; full isolated suite 44 of 44, warning-free.
## Check
- Date and commit: 2026-10-22, `8160b80` (0014's retrospective) plus the uncommitted 0015 work tree (spec, Plan, Tasks and code); updated after T016
- Tests: `meson test` 44 of 44 pass through `tools/isolated.sh`, the long suite `ui-0014-long` included (5.4 s; again 44 of 44 after
  T016); the cases of this feature: core `test-selection` 14 of 0015 (scale, stretch, lazy, rect, copies, memory), `test-lasso` 2; ui `/0015/` 20; `perf-0015`
  in the real session (T007, T006's addendum, T015: the Plan's Measurements)
- Mutants: the whole list before T016: 593 mutants: 592 killed, 0 killed by crash (test gap), 0 inconclusive (no snapshot), 0 survived, 0
  errors, 1 long-only (not run) in 5615 s, as `tools/mutate.sh` reports (isolated). The long-only mutant (REQ-0014-16, "a long trace
  keeps only its first 65536 corners") applied by hand and run against `ui-0014-long`: killed (`as14_long_trace`); the source restored.
  After T016 (nine mutants added, the list is 601): every REQ-0015 id, 89 mutants: 89 killed, 0 killed by crash, 0 inconclusive, 0
  survived, 0 errors in 419 s; `selection.c` changed only in `stretch_to`, which only REQ-0015 mutants quote
- Warnings: 0; `tools/spec.sh lint`, `lint --names 0015` and `mutate.sh --lint` (601 mutants, 0 stale) clean; no AI trailer in the
  commits
- Scenarios: AS-1 `as1_handles_drawn`, `as1_picture_handles_under_selection`, `as1_handle_cursors` + M1; AS-2 core `as2_scale_pixels`,
  `as2_lazy_stretch_matches_full`, ui `as2_stretch_right`, `as2_stretch_part_in_view`, `as2_stretch_part_outside_picture`,
  `as2_zoom_changes_during_stretch`; AS-3 `as3_one_pixel_to_three` (C); AS-4 core `as4_scale_back_exactly`, ui
  `as4_stretch_limits_and_left`; AS-5 core `as5_stretch_rect_ratio`, ui `as5_shift_keeps_ratio` (C in part); AS-6 `as6_ctrl_stretch`
  (C in part); AS-7 core `as7_scale_shape`, `as7_lazy_shape_keeps_one_pixel`, `as7_scaled_path_line`,
  `as7_scaled_path_rounding_and_margins`, ui `as7_stretch_free_form` (C); AS-8 `as8_selection_handle_wins`,
  `as8_selection_handle_over_picture_handle_area`, `as8_put_down_while_stretch_held`; AS-9 `as9_stretch_that_does_not_finish` (C in
  part); AS-10 core `as10_scale_memory`, `as10_stretch_memory`, `as10_lazy_stretch_memory`, ui `as10_stretch_dialog`; AS-11 core
  `as11_copies_keep_their_image`, ui `as11_copies_and_stretch` (C); AS-12 `perf-0015`; M2 done (T014); M1 confirmed ok by JOD (2026-10-22, T015)
- Task results: every task has a `Result:`; red first in T001 to T007, T010's core, T011 to T013 and the two additions of JOD (the zoom
  change while held, the picture handle's hit area); characterizations, each proven by the mutants named in its Result: T008's marked
  part, T009, T010's two ui cases (the canvas code was written before them, a slip against test-first), T011's ratio cases, T012's lost
  release, lost focus, Save and ignored keys
- Findings:
  - F1 (low, Plan drift, settled): "What grows" said the original and the current image both count against the selection's memory
    limit; the code checks each allocation on its own. REQ-0013-19 and REQ-0015-09 name the memory of one lift, one stretch, one copy,
    so the limit is per allocation: the Plan's sentence was corrected (JOD, 2026-10-22); no code change.
  - F2 (medium, memory, fixed in T016): the first stretch after copies exist allocated the copies' image list (12 bytes per entry of
    capacity with the places) without checking it against the limit; now `stretch_to` counts it as `easel_selection_stamp` does. Red
    first: core `as10_copies_list_counted_at_stretch` and the ui case in `as10_stretch_dialog`; four mutants killed.
  - F3 (info): the characterizations above; each has its mutant.
  - F4 (low, test gap, closed in T016): the tint of a free-form selection stretched lazily is rendered while the stretch is held:
    ui `as7_tint_while_lazy_stretch_held`, characterization, three mutants.
  - F5 (low, test gap, closed in T016): AS-7's "put down, 8 pixels changed" is asserted in `as7_stretch_free_form`; proven by the
    mutant "a stretched free-form shape is the whole rectangle".
  - F6 (info, decisions by JOD, 2026-10-22): 8000 x 8000 at the fit zoom (about 450 ms an event, 2.06 GB) and its release at 100 %
    (134 to 144 ms) are accepted slow cases; `perf-0015` must run with the pointer outside its window (its header says why).
