# 0018 Image Transforms

Status: implemented
Depends on: 0002 (undo), 0004 (Image menu, resize, view after a resize), 0013 (selection, its menu), 0014 (free-form selection),
0015 (selection handles), 0016 (canvas menu), 0017 (text box: Ctrl+I, put-down)

## Why
XP Paint flips, rotates, inverts and clears a picture or a selection in one command; people turn a sideways photo, mirror a
sketch or reset a canvas this way. Easel offers these as direct Image menu commands (no dialog), on the selection when there is
one, and adds XP's flip by dragging a selection handle past the opposite edge. Stretch and Skew, and Black and white, are later
features (JOD, 2026-10-27).

## Requirements
### Menus and keys
- REQ-0018-01 (Pointer note, Stretch and Skew, 2026-11-02: its item follows Rotate ›, REQ-0019-01. Pointer note, Black and white, 2026-11-09: Black and White… follows Invert Colors, REQ-0034-01. Pointer note, Remove background, 2026-11-09: Remove Background follows it, REQ-0035-01.) The Image menu (REQ-0004-01) shall hold, in three sections (JOD, 2026-10-27): Flip › (Flip Horizontal, Flip
  Vertical) and Rotate › (Rotate 90°, Ctrl+R; Rotate 180°; Rotate 270°, Ctrl+L: XP's three angles, each clockwise, JOD,
  2026-10-27; Ctrl+L because Shift+Ctrl+R is the desktop's, JOD, 2026-10-27); Invert Colors (Ctrl+I); Attributes… (Ctrl+E) and Clear Image (Shift+Ctrl+N). Each item shows its key as the menu writes
  it where it has one; the flips and Rotate 180° have none. The items and keys follow REQ-0004-03: they act wherever the focus is in
  the window, and do nothing before the picture exists, while a dialog, the quit prompt or the color chooser is open, or while any
  mouse button is held in the canvas area (a stroke, a drag of a shape, a selection or a text box, a stretch, a resize drag, a pan:
  REQ-0010-08, REQ-0013-18, REQ-0015-08, REQ-0017-11). They also do nothing while the focus is in a text field of the font card (the
  size, the family's search): the key is the field's (JOD, 2026-10-27). With Ctrl held, a key of a layout that is not Latin acts as the key
  at the same place that writes a Latin letter or symbol, so on a Ukrainian or Russian layout the key that reports "ш" is Ctrl+I; this holds
  for the text box's Ctrl+B, Ctrl+I, Ctrl+U, Ctrl+[, Ctrl+] and Ctrl+Shift+F too, but not with Alt (AltGr). While a text box is open Ctrl+I is Italic (REQ-0017-12) and the menu's Invert
  Colors still inverts.
- REQ-0018-02 (Pointer note, Black and white, 2026-11-09: the selection menu holds Black and White… after Invert Colors, REQ-0034-01, and Remove Background after it, REQ-0035-01.) The selection menu (REQ-0013-17, with Select or Free-Form Select) shall hold in its third section Flip ›, Rotate › and
  Invert Colors (Ctrl+I), the same submenus and items as the Image menu (JOD, 2026-10-27, in place of the mock-up's direct Flip items; pointer note, Stretch and Skew, 2026-11-02: both menus also hold its item, REQ-0019-01);
  the canvas menu (REQ-0016-14) shall hold in its third section Clear Image (Shift+Ctrl+N). Each acts as its Image menu item does.

### What is changed
- REQ-0018-03 Flip, Rotate and Invert Colors shall act on the selection while one exists (marked or floating, with Select or
  Free-Form Select), lifting a marked one (REQ-0018-07), and on the whole picture otherwise; they neither put a selection down nor
  remove it. Clear Image always acts on the whole picture: a floating selection is put down first and a marked one removed
  (REQ-0013-11, REQ-0013-12); if that put-down cannot get its memory, REQ-0013-19 holds (the selection's pixels go back, the clear
  goes on, then that dialog shows). Before any of these, as for any Image action: a pending curve, polygon or gradient is put down
  as shown (REQ-0011-12, REQ-0012-07), and an open text box is put down (REQ-0017-17), then there is no selection and the whole
  picture is changed; a text box whose put-down cannot get its memory stays open and the command is not done (REQ-0017-18).
- REQ-0018-04 On the whole picture: Flip Horizontal mirrors it across the vertical middle (the pixel at x goes to width − 1 − x);
  Flip Vertical mirrors it across the horizontal middle (y goes to height − 1 − y); Rotate 90° turns it clockwise, a W × H picture
  into an H × W one whose pixel (H − 1 − y, x) is the old (x, y); Rotate 270° turns it three quarters clockwise (one quarter
  counter-clockwise), the pixel (y, W − 1 − x) taking the old (x, y); Rotate 180° keeps the size and gives (W − 1 − x, H − 1 − y) the
  old (x, y). Every pixel is moved exactly, in all four channels. A quarter turn changes the picture's size as a resize does: the
  zoom stays, the scroll position is kept as far as the new size allows (REQ-0004-25), and the status bar shows the new size
  (REQ-0004-09).
- REQ-0018-05 Invert Colors shall make each pixel's red, green and blue 255 minus their value and keep its alpha (JOD, 2026-10-27):
  #12345678 becomes #EDCBA978, a fully transparent pixel stays as it is (2026-11-10, P7.7, ADR-0008; before: #00000000 became
  #FFFFFF00, still fully transparent); inverting twice gives the pixels back exactly. Since no channel value equals 255 minus itself,
  it changes every pixel that is not fully transparent.
- REQ-0018-06 Clear Image shall give every pixel of the picture the background color current at that moment, exactly in all four
  channels (a transparent background gives #00000000), in the Opaque and the Transparent mode alike (JOD, 2026-10-27: the
  Transparent mode's "clearing" is Clear Selection's), as REQ-0004-20 fills a grown picture.

### On a selection
- REQ-0018-07 A marked selection shall first be lifted as a move lifts it (REQ-0013-07, REQ-0013-08: the hole in the mode's color,
  only the shape's pixels for a free-form one; the hole stays as the lift made it). A transform of a selection then shows exactly
  the transform of what is shown (JOD, 2026-10-27): the selection keeps its pixels as lifted (REQ-0015-05; for a pasted selection the
  pixels as pasted, after Fit the fitted ones, REQ-0016-06) and the sum of the flips and turns made to it since; its shown pixels
  are the lifted ones scaled (REQ-0015-05) to its size with the turns taken back, then turned and flipped. So a flip of a selection
  shown as b d shows d b; a later stretch still starts from the pixels as lifted, and a selection stretched back to the lifted size
  shows the lifted pixels flipped and turned, exactly. A free-form selection's shape (REQ-0015-06) is made the same way, and its line
  (REQ-0014-05) is the traced path turned and flipped within its box on the grid lines: for a clockwise quarter turn of the box from
  grid line L to R across and T to B down, the corner (x, y) goes to (L + (B − y), T + (x − L)); a flip mirrors the corners across the
  box's middle; this also holds for the parts of the path outside the picture. Invert Colors inverts the lifted pixels (only the
  shape's for a free-form one; a pasted selection's fully transparent pixels stay not put down, REQ-0016-06). A flip and Rotate 180°
  keep the selection's place and size; a quarter turn keeps its smallest x and its smallest y (JOD, 2026-10-27: its top-left corner)
  and swaps its width and height, the status bar showing the new "W × H" (REQ-0013-06), and it may then reach outside the picture as
  a moved selection may (REQ-0013-07). The selection floats afterwards and its handles (REQ-0015-01) follow its new size. Copies left
  before the transform (by Ctrl, a smear or a stamp) keep their pixels.
- REQ-0018-08 A transform of a selection shall join the one undo step of its lift, moves, stretches and put-down (REQ-0013-11,
  REQ-0015-08): Undo while it floats forgets it all; nothing is added to the history until it is put down.

### Undo and memory
- REQ-0018-09 A transform of the whole picture, and Clear Image, shall be one undo step (REQ-0002-01) that gives the picture's
  size and every pixel back exactly on Undo and the same result on Redo. A flip, Rotate 180°, Invert Colors and Clear Image cost what
  REQ-0002-16 allows for the pixels they change; a quarter turn costs 4 bytes for every pixel of the picture plus 64 bytes, square or
  not (JOD, 2026-10-27, as a resize counts what it removes, REQ-0004-23). A step larger than the whole budget is kept as the only
  step (REQ-0002-17). One that changes no pixel and no size (a flip of a mirror-symmetric picture, Rotate 180° of a picture that is
  the same turned, Clear Image of a picture already all in the background color) adds no step and no modified mark (REQ-0010-07).
- REQ-0018-10 If an allocation a transform of the whole picture needs (its new pixels or its undo step; the history's budget itself
  never refuses a step, REQ-0002-17) cannot be had, the app shall show an error dialog with the heading of its action, "Could Not
  Flip the Picture", "Could Not Rotate the Picture", "Could Not Invert the Colors" or "Could Not Clear the Picture", and the body
  "There is not enough memory for this change." (JOD, 2026-10-27), and change nothing, neither the picture nor the history. Undo and
  Redo of a whole-picture transform swap the kept buffer and allocate nothing: they cannot fail (the check's F1, 2026-10-28, replacing a
  dialog for an Undo or Redo of a quarter turn that no input can reach). On a selection: "Could Not Flip the Selection", "Could Not Rotate the
  Selection" or "Could Not Invert the Selection" with the same body, the selection as before the command (marked again if the command
  lifted it); if the lift itself cannot get its memory, REQ-0013-19's dialog shows and the selection stays marked.

### Flipping by a handle
- REQ-0018-11 When a selection handle (REQ-0015-02; not a text box's, REQ-0017-05) is dragged past the opposite edge
  (REQ-0015-03), the selection shall be mirrored on that axis (JOD, 2026-10-27; replacing REQ-0015-03's "no mirroring"): its extent on
  that axis runs from the opposite edge, which stays where it is, to the dragged edge, at least 1 pixel; with the dragged edge exactly
  on the opposite edge the extent is 1 pixel and not mirrored. A corner handle mirrors on each axis it passes. While the button is
  held, dragging back across the opposite edge undoes that drag's mirroring. At the release a mirror counts as a Flip of
  REQ-0018-07 on that axis: what is shown is the selection as it was at the press, mirrored, at the new size; a later drag mirrors
  relative to the selection as it is at that press. With Shift a corner handle keeps the ratio of REQ-0015-07 on the sizes: the side
  that does not lead runs from its opposite edge toward the pointer's side of that edge, mirrored when the pointer is past it.
  Everything else of REQ-0015-03 to REQ-0015-10 holds (the 10000-pixel limit, the lift, Ctrl's copy, the one undo step, the drags
  that do not finish, a frame's response).

### Speed
- REQ-0018-12 A flip, turn, invert or clear of an 8000 × 8000 picture, and a flip, turn or invert of an 8000 × 8000 floating
  selection, shall take less than 500 ms in the core (JOD, 2026-10-27, as Fill's whole-picture budget), the result painted at the
  next refresh after it; flipping by a handle keeps REQ-0015-10's response within a frame.

## Scenarios
- AS-1 (REQ-0018-01, REQ-0018-02) Given a fresh start, When the Image menu opens, Then it shows Flip ›, Rotate ›, then Invert Colors Ctrl+I, then Attributes… Ctrl+E and Clear Image Shift+Ctrl+N; Flip › holds Flip Horizontal and Flip Vertical, Rotate › holds Rotate 90° Ctrl+R, Rotate 180° and Rotate 270° Ctrl+L; Given a selection made with Select, and one made with Free-Form Select, When the secondary button is pressed on it, Then the third section holds Flip ›, Rotate › and Invert Colors Ctrl+I; Given no selection, When pressed elsewhere with Select, Then the canvas menu's third section holds Clear Image Shift+Ctrl+N | Verified by: automated (ui) + manual M1 (the look)
- AS-2 (REQ-0018-04, REQ-0018-09) Given a 3 × 2 picture, white but red #FF0000FF at (0, 0) and blue #0000FFFF at (2, 1), When Ctrl+R, Then it is 2 × 3 with red at (1, 0) and blue at (0, 2), the status bar shows 2 × 3, and Undo gives the 3 × 2 picture back exactly; When Ctrl+L (Rotate 270°) instead, Then 2 × 3 with red at (0, 2) and blue at (1, 0); When Rotate 180°, Then 3 × 2 with red at (2, 1) and blue at (0, 0); When Flip Horizontal, Then red at (2, 0) and blue at (0, 1); When Flip Vertical, Then red at (0, 1) and blue at (2, 0); When Ctrl+R four times, Then the picture is as it was, four undo steps | Verified by: automated (core: the pixels; ui: the commands, the size and the step)
- AS-3 (REQ-0018-04) Given a 300 × 200 picture at 200 % scrolled to the middle, When Ctrl+R, Then it is 200 × 300, the zoom is still 200 %, and the scroll position is kept as far as the new size allows | Verified by: automated (ui)
- AS-4 (REQ-0018-05, REQ-0018-09) (2026-11-10, P7.7, ADR-0008: a fully transparent pixel stays as it is; before: #00000000 became #FFFFFF00, and an all transparent picture took one step and the modified mark.) Given pixels #12345678, #00000000 and #FFFFFFFF, When Invert Colors, Then they are #EDCBA978, #00000000 and #000000FF; When Invert Colors again, Then they are as they were; Given a picture all #00000000, When Invert Colors, Then nothing changes: no step, no modified mark; Given one opaque pixel in it, When Invert Colors, Then that pixel alone changes, one step | Verified by: automated (core, ui)
- AS-5 (REQ-0018-06, REQ-0018-03) Given a 20 × 20 white picture with a 4 × 4 floating selection of red #FF0000FF at (2, 2) in Transparent mode, and background #00FF00FF, When Shift+Ctrl+N, Then the selection is put down (its step: red at (2, 2)), then every pixel is #00FF00FF as one more step; Undo once gives the white picture with the red square at (2, 2); Given background transparent, Then every pixel is #00000000; Given a picture all #00FF00FF and background #00FF00FF, When Clear Image, Then no step and no modified mark | Verified by: automated (ui)
- AS-6 (REQ-0018-03, REQ-0018-07, REQ-0018-08) Given a 4 × 2 marked selection at (10, 10) in Opaque mode with background white, When Rotate 90°, Then the hole (10, 10)–(13, 11) is white, the selection floats 2 × 4 at (10, 10) with its pixels turned and the status bar shows 2 × 4, and no step is added; When Esc, Then it is put down as one step; Undo gives the picture as before the lift | Verified by: automated (ui)
- AS-7 (REQ-0018-07) Given a floating selection lifted 4 × 1 as a b c d and stretched to 2 × 1 (shown b d), When Flip Horizontal, Then it shows d b; When stretched back to 4 × 1, Then it shows d c b a, exactly; Given one lifted 2 × 2 and stretched to 4 × 4, When Flip Horizontal, Then its pixels are the shown ones mirrored | Verified by: automated (core: the pixels; ui)
- AS-8 (REQ-0018-07) Given a free-form selection shaped like an L, When Rotate 90°, Then its shape, pixels and line are turned, its smallest x and smallest y stay, and only the old shape's pixels were the hole | Verified by: automated (ui)
- AS-9 (REQ-0018-07) Given a 300 × 10 marked selection at (0, 190) in a 400 × 200 picture, When Ctrl+R, Then it floats 10 × 300 at (0, 190), its rows from y 200 to 489 outside the picture, the status bar showing 10 × 300; When Esc, Then only its 10 × 10 pixels inside, (0, 190) to (9, 199), are put down | Verified by: automated (ui)
- AS-10 (REQ-0018-01, REQ-0018-03) Given a text box open with "ab" selected, When Ctrl+I, Then "ab" turns italic and the picture is not inverted; When Invert Colors is chosen from the Image menu, Then the box is put down and then the whole picture inverted, two steps; Given a pending polygon, When Ctrl+R, Then the polygon is put down closed as shown, then the picture turned, two steps | Verified by: automated (ui)
- AS-11 (REQ-0018-01) Given a pencil stroke held, a selection being stretched, or a pan drag held, When Ctrl+R, Ctrl+I or Shift+Ctrl+N, Then nothing happens and the stroke, stretch or pan goes on; Given a text box open and the focus in the size field or the family's search, When Ctrl+I, Ctrl+R, Ctrl+L or Shift+Ctrl+N is pressed, Then the picture and the box are unchanged; Given a Ukrainian or Russian layout, When the key at Ctrl+I's place is pressed with no text box open, Then the picture is inverted; with a box open and "ab" selected, Then it is italic; the keys at B's, U's, [ 's and ]'s places are Bold, Underline and the size steps; the same keys with Alt change nothing | Verified by: automated (ui; the key handler given the Cyrillic keyval and the Latin key's code) + manual M1 a (the real layouts)
- AS-12 (REQ-0018-10) Given the memory for a quarter turn made unavailable, When Ctrl+R, Then "Could Not Rotate the Picture" with "There is not enough memory for this change." shows once and the picture and history are unchanged; the same for Flip, Invert Colors and Clear Image with their headings; Given a marked selection and the memory for its turn made unavailable, Then "Could Not Rotate the Selection" and the selection is marked as before; Given a quarter turn, When Undo and then Redo are chosen, Then each swaps the kept buffer, allocating nothing, so neither can fail and no dialog shows | Verified by: automated (ui, core)
- AS-13 (REQ-0018-11) Given a 4 × 2 marked selection at (10, 10) whose rows read a b c d left to right, When its right handle is dragged by (−6, 0), Then while held it is 2 × 2 at (8, 10) showing d b (b d mirrored); When dragged back to (0, 0) without releasing, Then it is 4 × 2 at (10, 10) showing a b c d; When released by (−4, 0), exactly on the left edge, Then it is 1 × 2 at (10, 10), not mirrored; Given it released by (−6, 0) (2 × 2 at (8, 10) showing d b), When its right handle, at x 10, is then dragged by (2, 0), Then it is 4 × 2 at (8, 10) showing d c b a: still mirrored, the lifted pixels scaled to 4 wide and flipped | Verified by: automated (core: the pixels; ui)
- AS-14 (REQ-0018-11) Given the marked 5 × 4 selection at (1, 1) of 0015's AS-4, When its bottom-right handle is dragged by (−15, −6) with Shift, Then it is 10 × 8 at (−9, −7), mirrored on both axes, the width leading (twice its size at the press); When the right handle alone is dragged by (−10, 0), Then it is 5 × 4 at (−4, 1), mirrored across | Verified by: automated (ui) + manual M1 (the real mouse)
- AS-15 (REQ-0018-12) Given an 8000 × 8000 picture, When it is flipped, turned, inverted or cleared, Then each takes less than 500 ms in the core; Given an 8000 × 8000 floating selection, When it is flipped, turned or inverted, Then the same; When a handle of a 2000 × 2000 selection is dragged past the opposite edge, Then each motion is answered within a frame (16 ms median) | Verified by: benchmark (perf-0018, the real session)

## Edge cases
- A 1 × 1 picture or selection: every transform shows it as it was (no step for the picture; the selection still floats, REQ-0018-07).
- A transform repeated: four quarter turns, two flips on one axis, two inverts give the original pixels exactly (AS-2, AS-4).
- A transform during a drag, a stroke, a stretch or a pan is ignored (AS-11); with a menu or dialog open it is not reachable
  (REQ-0004-03).
- Ctrl+R, Ctrl+L and Shift+Ctrl+N with a text box open and the focus in the box put the box down and then act on the whole picture
  (REQ-0017-17).
- A selection turned by a quarter turn cannot pass 10000 on a side: its sides are swapped, each within 10000.
- The view after a quarter turn of the whole picture: as after a resize (AS-3).
- 0015's AS-4 (a handle dragged past the opposite edge leaves 1 pixel, no mirroring) is replaced by AS-14's second clause.

## Out of scope
- Stretch and Skew (the feature Stretch and Skew); Black and white (a later feature); rotation by other angles; Crop to selection
  and Invert selection (Phase 2).

## Open questions
Open items start a line with `- [NEEDS CLARIFICATION` and a question. Resolve all before `accepted`.

## Plan
### Approach
- **One core module, `transform`,** for the eight flips and turns of a square (the identity, the two flips, the three turns and the
  two diagonal mirrors a flip and a turn make; one small number, `EaselOrient`, with a compose rule) and Invert, on a buffer of
  pixels and on a mask. The whole picture and a selection use the same functions, so AS-2's pixel rules are written once.
- **The whole picture:** the window's Image actions call `easel_image_transform (img, history, op, background, max_bytes)` in the
  core, which counts first the pixels the command changes (a pass with no allocation), then makes one undo step of the cheaper kind
  (REQ-0018-09; REQ-0002-16's 16 bytes per changed pixel always hold): a change log (12 bytes per changed pixel plus 64, as today) when
  12 n ≤ 4 N, done in place; otherwise a new kind of step, the **buffer step**, which keeps the picture's whole other buffer and its
  size (4 N + 64 bytes): the command writes a new buffer, and the old one moves into the step (no copy). A quarter turn is always a
  buffer step (REQ-0018-09: 4 bytes a pixel plus 64). Undo and Redo of a buffer step swap the picture's buffer and size with the
  step's: no allocation, so they cannot fail for memory (REQ-0018-10's Undo/Redo dialog is kept as a guard no input reaches; the
  Check notes say so). A command that changes nothing adds no step.
- **A selection:** the float gains an orientation (`EaselOrient`, identity at the lift). A flip or turn composes it and remakes the
  shown image as REQ-0018-07 says: the pixels as lifted scaled (REQ-0015-05) to the size with the turns taken back, then oriented;
  the shape the same way; the line (`sel_path0`, the traced path) scaled to that box, then oriented within the box. Invert inverts
  the pixels as lifted, the shown image and nothing else (copies and kept images keep theirs). A stretch (0015) keeps the
  orientation and remakes the image the same way; the lazily made image (`easel_selection_stretch_lazy`, 256 x 256 blocks) maps each
  shown pixel back through the orientation to the scaled pixel. A quarter turn keeps x and y and swaps w and h (REQ-0018-07).
- **Flipping by a handle:** `easel_stretch_rect` gains the mirror of REQ-0018-11 (the extent from the opposite edge to the dragged
  edge, at least 1, and a flag per axis); the canvas's stretch shows the selection at the press composed with the drag's mirrors, and
  at the release that becomes its orientation.
- **The one function every path goes through:** whole picture, `image_action (window, op)` in the window: it puts down what is
  pending (a text box, a curve, polygon or gradient, through the existing `put_down_for_action`), takes the selection branch when one
  exists, else calls the core, then rebuilds the view (`easel_canvas_picture_changed`, the resize path for a quarter turn). Every menu
  item, key and context-menu entry calls it. Selection: `easel_canvas_transform_selection (canvas, op)`, which lifts a marked one
  first (the lift path of a move, `lift_selection`).

### Memory peak of each command (the measure is T001's)
An 8000 × 8000 picture is N = 64 000 000 pixels, B = 256 MB a buffer.
| Command | Held at the peak | Peak above the picture |
|---|---|---|
| Quarter turn of the picture | old buffer (it becomes the step) + new buffer | B (512 MB with the picture) |
| Flip, Rotate 180°, Invert, Clear of the picture, buffer step | old (to the step) + new | B |
| The same, change-log step (12 n ≤ 4 N) | picture + log of 12 n bytes, done in place | at most B (12 n ≤ 4 N) |
| Undo or Redo of a buffer step | the two buffers already held: a swap | 0 |
| Flip or turn of a floating 8000 × 8000 selection, first time, not stretched | the lifted pixels (they become `orig_px`) + the new shown image | B |
| The same, stretched or already oriented | `orig_px` + old shown image + new shown image (the old one freed only once the new one exists, so a failure changes nothing); the lifted image is not copied | B |
| Invert of a selection, no copy left by a Ctrl move, a smear or a stamp | in place on the lifted pixels and the shown image: nothing allocated | 0 |
| Invert of a selection with such copies | new shown and lifted images, the old ones kept for the copies | up to 2 B |
Measured in T001 (`probe-transform-memory`, one command a process, core only, `-O2`, 8000 × 8000 with every pixel different, load average
about 2.5 to 3, three runs each; the process's peak resident set against the picture's 246 MiB, B = 244 MiB): every command peaked at
490 MiB, 244 MiB (one B) above the picture, as the table says; Undo and Redo of the step added nothing (0 ms, the peak unchanged). Core
times of the command (REQ-0018-12 asks < 500 ms): Clear 55 to 57 ms, Invert 71 to 77 ms, Flip Vertical 160 to 165 ms, Rotate 180° 176 to
180 ms, Flip Horizontal 180 to 249 ms, Rotate 90° and 270° 100 to 105 ms (the first build, which counted every changed pixel before a
quarter turn and copied by calling the source map per pixel, took 860 ms; it counts now only as far as the answer needs and copies
by a direct index). The rows for a floating selection are T004's; the peak with the window open, and the whole path, are perf-0018's
(T003): the probe has no window, so "with a window" is measured there, a deviation from T001's text.
Measured in T003 (`perf-0018`, the real session, pointer parked, no other Easel window, load average 0.2 to 0.6, three runs each; a
window open on the picture at 100 %; the picture shown is 357 MB resident at 8000 × 8000 and 122 to 130 MB at 1920 × 1080; "core" is
`easel_image_transform` on a scratch picture, "whole path" the window's action to the next painted frame, the tiles' upload included):
| Command | 1920 × 1080 core / whole path | 8000 × 8000 core / whole path | Undo, Redo (8000 × 8000) |
|---|---|---|---|
| Flip Horizontal | 8 ms / 11 to 20 ms | 180 to 218 ms / 183 to 196 ms | 4 to 5 ms, 22 to 26 ms |
| Rotate 90° | 2 to 4 ms / 13 to 18 ms | 97 to 105 ms / 101 to 111 ms | 8 to 15 ms, 17 ms |
| Invert Colors | 1 to 3 ms / 12 to 18 ms | 67 to 75 ms / 68 to 78 ms | 19 ms, 17 to 18 ms |
| Clear Image | 1 to 3 ms / 12 to 17 ms | 57 to 63 ms / 57 to 66 ms | 13 to 15 ms, 16 to 17 ms |
Every core time and every whole path is inside REQ-0018-12's 500 ms (the whole path costs almost nothing beyond the core: only the
visible tiles are uploaded at 100 %). At 1920 × 1080 the whole path is about one frame (11 to 20 ms). Memory at 8000 × 8000: the first
command in a window peaked at 602 MB, 244 MB (one buffer) above the 358 MB shown, as the table says; a later command, made while the
previous one's step waits to be redone (Undo, then another command), peaked at 845 MB, two buffers above: the new pixels are made
before the history drops the redoable step, so the picture, the new buffer and the old redo step are alive together. This peak is above the table and was accepted by JOD (2026-10-27): it is transient (the whole path is about 190 ms), only for pictures of
the largest sizes, and a shortage is REQ-0018-10's dialog with nothing changed; the Plan's table row "Flip or turn ... of the picture" reads
one buffer above the picture for the first command and two for a command made while a redoable step waits. It is recorded in the INDEX's
row for 0018 with the other accepted cases.
Measured in T004 (`probe-transform-memory sel-*`, core only, an 8000 × 8000 picture and a float of all of it, 491 MiB resident, the peak
counter reset before each command): a flip or a turn of the float peaked one image above (244 MiB: 735 MiB), the second command the same
(979 MiB); stretched to one pixel narrower than it was lifted (735 MiB resident) the same, 244 MiB above; Invert Colors 0 above, with no
copy (9 to 17 ms). Core times of the command on the float: unstretched flip 50 to 60 ms, turn 60 to 102 ms (the tiled copy, when the size
before the orientation is the lifted size); stretched flip 229 to 431 ms, turn 415 to 457 ms (the general scaling, per pixel; inside 500
ms, the slowest case of the feature); Invert 9 to 17 ms. My first build copied the images for Invert (244 MiB above, 488 stretched) and
scaled per pixel even at the lifted size (turn 454 ms, flip 427 ms): both changed after the probe.
Measured in T004 in the real session (`perf-0018 8000 sel-*`, a window, the pointer parked, load average 0.4 to 1.8, a selection of all of an
8000 × 8000 picture, the whole path to the next painted frame, 358 MB resident with the picture shown): Flip Horizontal 149 ms with the lift,
then 75 and 81 ms; Rotate 90° 165 ms with the lift, then 70 and 115 ms; Invert Colors 68 ms with the lift, then 20 and 17 ms. The peak of
the first command was 858 MB (the lift and the new shown image: two images above the picture shown, as the table says), after three
1101 MB for a flip or a turn (the lifted, the old shown and the new shown image alive together: one image above the float's two) and
620 MB for Invert (the lift only; it is done in place).
The texture of the view is extra: after a whole-picture command the canvas uploads the picture again as its tiles (0004); T001's
probe reads the process's peak (`getrusage` max RSS) around each command, with and without the window, and the Plan's table gets
the measured numbers. If a measured peak is above this table, the task stops and JOD decides.

### Files
- New, core: `src/core/transform.{c,h}` (EaselOrient, its compose and inverse, the pixel map of a buffer, of a mask, Invert;
  `easel_image_transform`); tests `tests/core/test-transform.c`.
- Changed, core: `src/core/history.{c,h}` (the buffer step: push, undo and redo by swap, its bytes); `src/core/selection.{c,h}`
  (the float's orientation, `easel_selection_transform`, the stretch and lazy fill through the orientation, `easel_stretch_rect`'s
  mirror); `src/core/lasso.{c,h}` (a path oriented within its box).
- Changed, ui: `src/ui/window.{c,ui}` (the Image menu's three sections and submenus, the selection menu's third section, the canvas
  menu's third section, the actions and `image_action`, the error dialogs); `src/ui/application.c` (Ctrl+R, Ctrl+L, Ctrl+I,
  Shift+Ctrl+N); `src/ui/canvas.{c,h}` (`easel_canvas_transform_selection`, the handle mirror in the stretch drag, the float's
  orientation through the lay and the outline).
- Tests: `tests/core/test-transform.c`, cases in `test-history.c` and `test-selection.c`; ui `/0018/` in `tests/ui/test-window.c`
  and `ui-0018` in `tests/meson.build`; 0015's `as4` changed for the mirror (its pointer note); `tests/perf/perf-0018.c`; a probe
  `tests/probe/probe-transform-memory.c` for the peaks.

### Key interfaces (as built, at the gate, 2026-10-28)
- `typedef uint8_t EaselOrient;` with `EASEL_ORIENT_IDENTITY`, `_TRANSPOSE`, `_FLIP_X`, `_FLIP_Y` (a transpose, then a mirror of x, then of
  y); `EaselOp` (`EASEL_OP_FLIP_H`, `_FLIP_V`, `_ROTATE_90`, `_ROTATE_180`, `_ROTATE_270`, `_INVERT`, `_CLEAR`); `easel_op_orient (op)`,
  `easel_orient_compose (first, then)`, `easel_orient_inverse (o)`, `easel_orient_swaps (o)`, `easel_orient_size`, `easel_orient_source`,
  `easel_orient_target`.
- `void easel_orient_pixels (const uint32_t *src, int w, int h, EaselOrient o, uint32_t *dst)` (the caller allocates dst; the mask of a
  float is oriented by `scale_oriented`).
- `EaselTransformResult easel_image_transform (EaselImage *img, EaselHistory *h, EaselOp op, uint32_t background, size_t max_bytes)`:
  `EASEL_TRANSFORM_DONE`, `_NOTHING` (no pixel and no size changed, no step), `_NO_MEMORY` (nothing changed).
- `bool easel_history_push_buffer (EaselHistory *h, int w, int hgt, uint32_t *px)` (takes px; counts 4 w hgt + 64).
- `EaselSelectionResult easel_selection_transform (EaselFloat *f, EaselOp op, size_t max_bytes)`; the float's `orient` field;
  `easel_selection_stretch_oriented` and `easel_selection_stretch_lazy_oriented (f, x, y, W, H, orient, max_bytes)` (the plain
  `easel_selection_stretch` and `_lazy` keep the float's own orientation).
- `void easel_stretch_rect_mirror (handle, x, y, w, h, dx, dy, keep_ratio, &nx, &ny, &nw, &nh, &mirror_x, &mirror_y)`; the old
  `easel_stretch_rect` stays, unchanged, for the text box's resize (REQ-0017-10: no mirroring); the shared Shift ratio is `ratio_sizes`.
- `easel_lasso_orient_path (src, x, y, w, h, orient, out, max)` (a free-form line oriented within its box).
- Window: `image_action (self, op)`; canvas: `gboolean easel_canvas_transform_selection (EaselCanvas *self, EaselOp op)`;
  `easel_canvas_is_panning`; the test switches `easel_window_set_transform_limit` and the selection lift limit.

### Every reader of what changes
- The picture's size after a quarter turn: the view and scroll (REQ-0004-25), the tiles and texture, the resize handles, the grid,
  the status bar's size, the window's title mark, the history's later steps (their indexes are of the size at their time, as for a
  resize); all go through the existing resize path (`easel_window_resize_picture`'s view part), which the quarter turn calls.
- The float's w and h and image: `easel_selection_lay`, `_lay_range`, `_put_down`, `_copy_image`, `_bounds`, `_stamp`, `_smear`,
  the handles' places (REQ-0015-01), the status bar's "W × H", the hit test of a press on the selection, the outline, the copy of
  0016; each reads `px`, `mask`, `w`, `h`, which keep their meaning (the shown image), so only the makers change (stretch, lazy fill,
  transform).
- The stretch drag: its caller `easel_stretch_rect_mirror` (the canvas's `stretch_apply`, with the orientation at the press, `stretch_orient0`,
  kept for the cancel and the free-form line) and 0015's test `as4_stretch_limits_and_left`, whose past-the-edge cases changed (0015's
  pointer note); the text box's resize still calls `easel_stretch_rect`.

### What grows
The history's buffer step: one picture buffer (4 bytes a pixel) and its size, no copy (the old buffer moves into it). Nothing else
grows with use; the float's orientation is one byte.

### Constitution check
P4 (exact pixels: every map is a permutation, no resampling beyond 0015's nearest pixel), P5 (no threads; a whole pass over the
picture), layering (the maps in `src/core`, libc only), S3 (every REQ has its AS). No ADR.

### Risks (each probed or tested before the code relies on it)
- R1 Orientation then nearest scaling: REQ-0018-07 is "scale to the pre-turn size, then orient", never the other order (the
  reviewer: 672 of 3136 sizes differ). T004's core test runs every size up to 8 × 8 against the rule written out naively.
- R2 The lazy stretch through an orientation: a block of the shown image maps to a block of the scaled image turned; T004 tests that
  a lazily made oriented image equals the whole one, pixel for pixel, at 8000 × 4000.
- R3 GTK's menu label for Shift+Ctrl+N (the spec writes it as the menu shows it; Rotate 270° is Ctrl+L since JOD's M1 a, the desktop
  using Shift+Ctrl+R): T002 reads the label GTK makes and
  the spec's AS-1 text is brought to it if it differs (a wording fix shown to JOD).
- R4 The texture upload of a whole 8000 × 8000 picture after a command, and the memory peaks: T001's probe and perf-0018 (T003).
- R5 The buffer step's swap must keep the saved state and the modified mark right (REQ-0002-12..14): T001's history cases.
- R6 Ctrl+I is Italic only while a text box is open (REQ-0017-12): the window's key path already routes it; T002 checks both ways.

### perf-0018 (T003, right after the first task that produces the cost)
The whole path from the menu item's activation to the next painted frame (the core command, the step, the tiles' upload of the
8000 × 8000 picture, the frame), in the real session with the pointer parked and no other Easel window, 3 runs each with the load
average noted: Flip Horizontal, Rotate 90°, Invert Colors and Clear Image of an 8000 × 8000 and a 1920 × 1080 picture; the same
three transforms of an 8000 × 8000 floating selection (added to perf-0018 in T004); a handle flip's motion on a 2000 × 2000 selection
(added in T006). REQ-0018-12
is the core part (< 500 ms); the whole path is recorded beside it, and any case over 500 ms on the whole path goes into the INDEX's
accepted slow cases with its numbers for JOD.

### Scenario-to-test map
| AS | Test |
|---|---|
| 1 | ui `as1_image_menus`, `as1_selection_menu`, `as1_keys_any_layout` + manual M1 |
| 2 | core `as2_orient_pixels`, ui `as2_picture_transforms` |
| 3 | ui `as3_turn_keeps_view` |
| 4 | core `as4_invert`, ui `as4_invert_transparent_step` |
| 5 | ui `as5_clear_image` |
| 6 | ui `as6_turn_marked_selection`, `as8_turn_undo_redo` (REQ-0018-08); core `line_oriented` |
| 7 | core `as7_transform_stretched_float`, `stretch_with_orientation`, ui `as7_flip_stretched` |
| 8 | core `line_oriented`, ui `as8_turn_free_form`, `handles_follow_turn` |
| 9 | ui `as9_turn_past_picture` |
| 10 | ui `as10_text_box_and_pending`, `as11_keys_in_an_entry` |
| 11 | ui `as11_ignored_while_held` |
| 12 | core `as12_transform_no_memory`, ui `as12_transform_dialogs`, `as12_selection_dialogs` |
| 13 | core `as13_stretch_rect_mirror`, `stretch_with_orientation`, ui `as13_handle_flip` (also the Undo, Redo and cancel cases) |
| 14 | ui `as14_handle_flip_shift` + manual M1 |
| 15 | perf-0018 (the real session) |

## Tasks
Every task: test first and a red run (or a characterization with its named mutant), `ninja -C build` warning-free, the tests of
0018 and the core green through `tools/isolated.sh`, the task's mutants against the short suite, `mutate.sh --lint`.

A. The whole picture
- [x] T001 Core: the eight orientations, Invert, Clear, `easel_image_transform` and the buffer step, with the memory peaks
  measured (REQ-0018-04, REQ-0018-05, REQ-0018-06, REQ-0018-09, REQ-0018-10; AS-2, AS-4, AS-12 core parts; R4, R5) - done when core
  `as2_orient_pixels` (AS-2's 3 × 2 picture, each command, four turns back), `as4_invert`, `buffer_step_swap` (Undo and Redo swap,
  the saved state and the modified mark, the step's held bytes against the bytes it counts), `cheaper_step_chosen` (a flip changing
  few pixels makes a change log, one changing most a buffer step, each within 16 bytes per changed pixel) and
  `as12_transform_no_memory` (nothing changed when the new buffer or the step cannot be had) pass after red runs, and
  `probe-transform-memory` has measured the peak of each command of the Plan's table at 8000 × 8000 (max RSS before and after, with
  and without a window), its numbers written into the table; a peak above the table stops the task for JOD
  Result: `test-transform` written first, red by build (`fatal error: transform.h: No such file or directory`), then green with
  `src/core/transform.{c,h}` and the buffer step in `history.c` (`STEP_BUFFER`, `easel_history_push_buffer`, undo and redo swap the
  picture's buffer and size with the step's, nothing allocated, the whole picture dirty). My test mistake: the first as4_invert wrote
  the pixels as #RRGGBBAA where they are 0xAARRGGBB (`img->px[0] == 0xEDCBA978u` failed). 11 core cases: `as2_orient_pixels` (every size
  to 5 × 4 and four across the 64-pixel blocks), `orient_composition`, `as2_picture_commands`, `as4_invert`, `as5_clear`,
  `nothing_changed_no_step`, `buffer_step_swap`, `cheaper_step_chosen`, `log_at_the_limit`, `as12_transform_no_memory`,
  `as12_log_above_limit`. The cheaper step: a log (12 bytes a changed pixel plus 64) while it changes at most a third of the
  pixels, else a buffer step (4 bytes a pixel plus 64), a quarter turn always the latter. Mutants: 26 written; 3 survived first, each a
  test gap (a clear's color with an even low bit, the dirty area right after the command, the log's limit), each closed; 4 older
  were made stale by the faster count and rewritten; one (a log that fails to push stays applied) was left unwritten: only a failed
  allocation of the step array reaches it, which no input causes (the revert is a guard, as the Undo/Redo memory bell of 0017);
  REQ-0018-04, -05, -06, -09, -10: 26 of 26 killed (29 s). The counting's early exit changes only the time, so it has no mutant
  (perf-0018 and the probe's times guard it). Probe numbers in the Plan. Full isolated suite 50 of 50, warning-free; `--lint` 986,
  0 stale; `spec.sh lint` ok.
- [x] T002 The Image menu, the keys and the commands on the whole picture (REQ-0018-01, REQ-0018-02 for the canvas menu,
  REQ-0018-03, REQ-0018-04 to REQ-0018-06, REQ-0018-09, REQ-0018-10; AS-1 Image menu and canvas menu, AS-2 ui, AS-3, AS-4 ui, AS-5,
  AS-10, AS-11, AS-12 ui; R3, R6) - done when ui `as1_image_menus` (without the selection menu), `as2_picture_transforms`,
  `as3_turn_keeps_view` (rendered at the scrolled position), `as4_invert_transparent_step`, `as5_clear_image`,
  `as10_text_box_and_pending` and `as11_ignored_while_held` and `as12_transform_dialogs` (picture part) pass after red runs; AS-10's
  pending polygon and AS-11 likely pass through shared code (`put_down_for_action`, REQ-0004-03's guard): characterizations, proven by
  the mutants "an Image action leaves a pending polygon pending" and "Ctrl+R acts during a held stroke"; the menu label GTK writes for
  Shift+Ctrl+N is read and shown to JOD if it differs from the spec's; then the manual look (M1 a): the Image menu and its submenus,
  the canvas menu's third section, and where the keys go after each menu closes (Pan, then Select active); step 8: with the Text tool
  and a box open, Ctrl+I, Ctrl+R, Ctrl+L and Shift+Ctrl+N with the focus in the size field and in the family popover's search field:
  the picture does not change and the key does nothing more than an entry does with it; step 9: with a Ukrainian or Russian layout
  active, the four keys work as on the US layout (Ctrl+I, Ctrl+R, Ctrl+L, Shift+Ctrl+N, with and without a box open, and the box's
  Ctrl+B and Ctrl+U); the look of the menu's "Ctrl+I" (an `accel` attribute) in the light and the dark theme is what JOD judges in step 1
  Result (the automated part; M1 a to come): ui `/0018/` written first, red by build (`implicit declaration of function
  'easel_window_set_transform_limit'`), then green with the window's `image_action` (the one function every Image command goes
  through: `view_actions_allowed`, then `put_down_for_action`, then `easel_image_transform`, then the view and `sync_history_ui`), seven
  actions (`flip-horizontal`, `flip-vertical`, `rotate-90`, `-180`, `-270`, `invert`, `clear-image`), the Image menu in three sections
  with Flip › and Rotate › submenus, the canvas menu's third section, Ctrl+R, Ctrl+L and Shift+Ctrl+N as accelerators. Ctrl+I is
  not an accelerator (a deviation from the Plan's wording, R6): the Invert Colors item shows it through its menu `accel` attribute and
  the window's key handler runs the command when no text box took the key for Italic, so the box's Italic cannot lose to an action
  whatever order GTK runs the two in (`as10` checks both). R3: GTK writes "Shift+Ctrl+N" and "Ctrl+L" as the spec does. Two older tests brought to the new menus (0004's `as1_menus` and 0016's `as13_canvas_menu`, by their pointer notes). My test
  mistakes: `as3` expected a vertical scroll of 100 that the 800-pixel picture had clamped to 48 before the turn (it now compares
  with what was held), and its first drawn-pixel check added a margin the canvas origin already includes. Found by `as11`, from the
  spec's list of held drags: a Pan drag was not among the states `view_actions_allowed` knows, so the commands acted during it
  (`easel_canvas_is_panning`, which `image_action` now also asks). AS-10's polygon and its failed text-box put-down, and AS-11's
  stroke, passed through shared code at their first run after the code (`put_down_for_action`, `view_actions_allowed`):
  characterizations, proven by the mutants "an Image command leaves a pending polygon pending", "an Image command goes on when the
  text box could not be put down" and "an Image command acts during a held stroke". Not reachable by an input: the Undo/Redo of a
  buffer step failing for memory (it swaps buffers). Mutants: 31 written, REQ-0018-01 to -10 and the older REQ-0004-22 (re-anchored
  on its own lines): 57 of 57 killed with T001's (200 s), the 13 ui kills under 5 s again alone 13 of 13. Full isolated suite 51 of 51,
  warning-free; `--lint` 1013, 0 stale; `spec.sh lint` ok.
  After JOD's M1 a steps 1 to 7 (ok) and his two requests (2026-10-27): (a) Rotate 270° moved from Shift+Ctrl+R, which his desktop uses
  to start a terminal, to Ctrl+L (his choice; `as1_image_menus` red at `has_accel (win.rotate-270, Ctrl+L)`, then green). (b) Steps 8
  and 9, tests first: `as11_keys_in_an_entry` red at `box_text_is (win, "ab")` (with the focus in the size field the accelerators'
  actions put the box down and turned the picture; the key handler already let Ctrl+I through to the field), green with the focus
  check at the start of `image_action` (`card_field_focus`, as Undo goes to a focused field); `as1_keys_any_layout` red at the first
  Ctrl+"ш" (the handler compared the symbol with `GDK_KEY_i`; GTK matches accelerators across layouts, so Ctrl+R, Ctrl+L and
  Shift+Ctrl+N were not the problem), green with `shortcut_keyval` (Ctrl held, no Alt: a non-ASCII keyval is read as the Latin keyval
  of the same key and level), which also makes the box's Ctrl+B, Ctrl+U, Ctrl+[, Ctrl+] and Ctrl+Shift+F follow the key's place on
  such a layout (0017's REQ-0017-12 and -25 compared the symbol before). Both new cases need the real layouts for their last word:
  step 9. Mutants: 5 more (the field check, the Latin place, Alt not mapped, Ctrl+L; one older re-anchored): REQ-0018-01 to -10: 56 of
  56 killed (154 s), the 14 ui kills under 5 s again alone 14 of 14. Full isolated suite 51 of 51, warning-free; `--lint` 1016, 0 stale.
  Wording of REQ-0018-01, AS-1, AS-2, AS-11 and the edge case, and a pointer note on REQ-0017-12, applied with JOD's yes.
  M1 a (JOD, 2026-10-27): steps 1 to 7 ok (the menus' look in both themes, every command from the menu, the keys, Ctrl+I in a text box as
  Italic and from the menu as Invert, the canvas menu's Clear Image, the keys after each menu closes); steps 8 to 10 ok (Ctrl+I, Ctrl+R,
  Ctrl+L and Shift+Ctrl+N with the focus in the size field and in the family's search do nothing to the picture; the four keys on a
  Ukrainian or Russian layout; the look of the menu's "Ctrl+I", an `accel` attribute, in the light and the dark theme).
- [x] T003 Measure: perf-0018, the whole path on the whole picture (REQ-0018-12; AS-15) - done when perf-0018 has run in the real
  session (pointer parked, no other Easel window, load noted, 3 runs) for Flip Horizontal, Rotate 90°, Invert Colors and Clear Image
  of an 8000 × 8000 and a 1920 × 1080 picture, the core part and the whole path (menu activation to the next painted frame, the tiles'
  upload included) recorded in the Plan, and a whole-path case over 500 ms written into the INDEX's accepted slow cases for JOD
  Result: `tests/perf/perf-0018.c` (a benchmark; `perf-0018 [SIZE [COMMAND]]`), run in the real session on 2026-10-27 after JOD's "ready"
  (his M1 window closed first: the benchmark refused to start with it open), load average 0.2 to 0.6, pointer parked: all four commands at
  1920 × 1080 and 8000 × 8000, three runs each, in the Plan: every core time inside 500 ms (57 to 218 ms at 8000 × 8000), the whole path
  within a few ms of the core (about one frame at 1920 × 1080), Undo and Redo 4 to 26 ms. The peak memory read first as 845 MB in every case
  because the core phase of the same process was in it (reset with `clear_refs`), then as 602 MB for the first command and 845 MB for one
  made while a redoable step waits; JOD accepted that (option 1) and it is in the INDEX. No case is slow.

B. A selection
- [x] T004 Flip, turn and invert of a selection: the pixels and the shape (REQ-0018-02 selection menu, REQ-0018-03, REQ-0018-07 pixels,
  hole, place and size, REQ-0018-08, REQ-0018-10 selection part; AS-1 selection menu, AS-6, AS-7, AS-12 selection part; R1, R2) - done
  when core `as7_transform_stretched_float` (every size up to 8 × 8 and every orientation against the rule written out naively, R1),
  `oriented_lazy_equals_whole` (8000 × 4000, R2), `selection_invert` (the lifted pixels only; copies and kept images untouched) and
  `shape_oriented_with_pixels` (a free-form shape's mask oriented like its pixels, never empty) pass after red runs, ui
  `as1_image_menus` gains the selection menu (Select and Free-Form Select), ui `as6_turn_marked_selection`, `as7_flip_stretched` and
  `as12_transform_dialogs` (selection part) pass after red runs, the memory peaks of the selection rows (the Plan's table) measured by
  the probe through a command, an Undo and a command, perf-0018 gains the three transforms of an 8000 × 8000 floating selection (the
  real session), each rule has its mutant against the short suite, and JOD has tried it (M1 b: a marked and a floating selection with
  Select and with Free-Form Select, each command from the Image menu, the selection menu and the keys; the outline of a free-form
  selection after a turn is T005's)
  Result: the four core cases of `test-selection` written first, red
  by build (`implicit declaration of function 'easel_selection_transform'`), then green: the float gains an orientation (`orient`), its
  shown image is the lifted one scaled to the size before the orientation and then oriented (`scale_region` and `keep_one_pixel` take
  the orientation; a command is `stretch_to` with the composed orientation at the same place, a quarter turn swapping the size), which
  makes a command exactly the command of what was shown (R1: `as7_transform_stretched_float` and `shape_oriented_with_pixels`, every
  lifted size to 4 × 4, every stretch to 6 × 6, a second command on top and a stretch back to the lifted size) and a lazily made image
  equal to the whole one (R2: `oriented_lazy_equals_whole`, random and block-edge sizes, `oriented_lazy_shape_small`, every small size
  where the nearest pixel loses a shape). Invert Colors (`selection_invert`): in place when no copy uses the images, else new images with
  the old ones kept for the copies (the stretch's own `kept` list). Ui: `as1_selection_menu`, `as6_turn_marked_selection`,
  `as7_flip_stretched` and `as12_selection_dialogs` written first, red (`2 == 3` sections; `selection_is (10, 10, 2, 4)`;
  `selection_is (10, 5, 2, 1)`; no dialog), then green with `easel_canvas_transform_selection` (lifts a marked selection, remakes the
  float, keeps the place), the window's selection branch in `image_action`, the selection menu's third section and three dialog kinds.
  Not reachable by an input, so untested: the marked-again path when a lift succeeds and the new image then fails (both use the same
  limit); the Undo/Redo of a floating selection is 0013's. Design changes after the memory probe (above): the unscaled copy and the
  in-place Invert. Mutants: 40 written, all killed with the older ones re-anchored (REQ-0013-08, REQ-0014-08, REQ-0015-05, -06, -09, whose
  lines moved or repeated); five survivors found and closed on the way, each a test gap (the lazy completion's one pixel under an
  orientation, each of Invert's two limits, and, after the in-place change, the Ctrl-copy case of the dialog). The slowest case of the
  feature is a stretched 8000 × 8000 float turned (443 ms of 500). Full isolated suite 51 of 51, warning-free; `--lint` 1046, 0 stale.
  M1 b (JOD, 2026-10-27): all seven steps ok (a marked rectangle flipped, the selection menu's third section, Rotate 90° with the handles
  and the status bar following and the selection reaching past the edge, Ctrl+R, Ctrl+L and Ctrl+I on the selection, one Undo for the whole,
  a stretched selection flipped and stretched back, Invert with a Ctrl copy left, Free-Form Select's pixels). The perf-0018 rows of the
  selection ran in the real session after JOD's "ready" (his window closed first): inside 500 ms everywhere, in the Plan.
- [x] T005 After a turn: the free-form line, the status bar and the handles (REQ-0018-07 line, new size, outside the picture; AS-8, AS-9) -
  done when core `line_oriented` (the corner formula of REQ-0018-07 for a quarter turn and a flip, also for the parts of the path outside
  the box) and ui `as8_turn_free_form` (shape, pixels, line, the hole), `as9_turn_past_picture` (a 300 × 10 selection turned to 10 × 300
  reaching outside the picture, the put-down only inside) and `handles_follow_turn` (the status bar's "W × H" swapped, the eight handles at
  the new size, the hit area and the outline) pass after red runs, each rule has its mutant against the short suite, and JOD has tried
  it (M1 b2: a free-form selection's outline and handles after a turn, a flip and a stretch)
  Result: core `line_oriented` written first (an L in the 4 × 3 box at (3, 2), three corners outside
  it, one of them negative; the five commands' corners listed by hand in the test and checked again by a separate script; four turns,
  two flips on one axis, a turn and its opposite, and the memory limit), red by build (`implicit declaration of function
  'easel_lasso_orient_path'`), then green with `easel_lasso_orient_path` (the corner, relative to the box, exchanged for a quarter turn,
  mirrored in the box after the exchange, and put back at the box's left and top). Its 12 mutants (a sign, a swap, the box before or after
  the turn, the left, the top, added back or not, the limit) all killed on the first run, so the corners are asymmetric enough. Ui
  `as8_turn_free_form` red at `xy[i] == want[i]: (3 == 6)` (the line was not turned), then green with `oriented_line` in the canvas (the
  path as traced scaled to the size before the orientation, then oriented; used by the stretch, by its cancel and by the command, which
  computes it before it changes anything) and the command's commit of the line (path0 kept, the line's anchor moved to the float);
  `as8` also moves the selection first (the offset is gone after the turn) and cancels a stretch of a turned selection (the line is
  the turned one again). `as9_turn_past_picture` and `handles_follow_turn` passed at their first run after the code of T004 (the
  rectangle, the status bar and the handles already followed the new size): characterizations, proven by the mutants "a turned selection
  is limited to the picture" and "the status bar does not follow a command on the selection", and, for the handles, by T004's "a turned
  selection keeps its old size in the rectangle". My test mistake: the handles of a 2 × 4 selection are not 2 pixels apart (their squares
  lie outside the rectangle), so `handles_follow_turn` compares them with the handles of a 2 × 4 selection marked at the same place, and
  hides them to check the outline, as 0013's test does. Not testable by an input: the line's copy failing with the float's passing (the
  line, 48 bytes, is smaller than the float's). Mutants: 12 core and 11 canvas written, REQ-0018-07 and the re-anchored REQ-0015-06 (the
  line scaled from a box one narrower, now in `oriented_line`): 51 of 51 killed (226 s), none under 5 s. Full isolated suite 51 of 51,
  warning-free; `--lint` 1069, 0 stale. M1 b2 (JOD, 2026-10-27): a free-form selection's outline turned, flipped, stretched after a turn,
  cancelled and moved then turned again: ok.

C. Flipping by a handle
- [x] T006 A handle dragged past the opposite edge mirrors (REQ-0018-11; AS-13, AS-14; 0015's AS-4 by its pointer note) - done
  when core `as13_stretch_rect_mirror` (the extents, the 1-pixel edge, both axes, Shift's ratio on the sizes) and ui
  `as13_handle_flip` and `as14_handle_flip_shift` pass after red runs, 0015's `as4` brought to its pointer note (red first: it
  expects 1 × 4), perf-0018 gains a handle flip's motion on a 2000 × 2000 selection (within a frame, REQ-0015-10), and JOD has tried
  it with the real mouse (M1 c: each handle past its opposite edge, back again before the release, Shift on a corner, Ctrl's copy, Esc
  during a mirrored drag, Undo before and after the put-down and Redo)
  Result (code and tests; the perf run and M1 c were done after this text, see the last lines of this Result): core `as13_stretch_rect_mirror` and
  `stretch_with_orientation` red by build (`implicit declaration of function 'easel_stretch_rect_mirror'`), then green. The ui
  tests `as13_handle_flip` (7 runs: held past the edge and back, exactly on the edge, released past it, widened again, a mirrored float
  and an unmirrored one with a cancelled drag, a cancelled drag across to the same size) and `as14_handle_flip_shift` were written after
  the canvas code (pulled forward by mistake); I made them red by disabling the mirror in the canvas (as13: `0xff00bb00 == 0xffdddd00`
  at the first pixel of d; as14 first passed because 0015's picture is symmetric, then failed with `0xff0000ff == 0xffffff00` once a blue
  corner pixel was added), and put the code back (diff against the saved copy: same). 0015's `as4` now expects 5 x 4 at (-4, 1) and
  (6, 1); it was not run red first. The Shift ratio block shared by `easel_stretch_rect` and the new function is `ratio_sizes`, so the
  0015 mutants have one copy to anchor to; 14 older mutants re-anchored. Mutants: 23 written for REQ-0018-11; the first run of 124
  (REQ-0018-11 and the REQ-0015/0018-07 ones of the stretch) left 2: "a change of side alone is no movement" is equivalent (a mirrored
  rectangle is never the rectangle of the press, so the clause was dead and is removed with its mutant) and "a cancelled drag does not
  restore the orientation" was a gap (a drag to the same size on the other side keeps w and h), closed by run 6 of `as13`. Then 28 of 28
  killed (163 s). Full isolated suite 51 of 51 before the last two edits; `--lint` 1091, 0 stale; warning-free.
  Runs 7 and 8 of `as13` (Undo of a released mirrored drag forgets it all as 0015's AS-2 says, no Redo; put down, Undo and Redo give the
  picture as before and the mirrored result again) passed at their first run: characterizations of existing behavior.
  perf-0018 `handle-flip` (2000 x 2000 selection of the whole picture, bottom-right handle across the top-left corner and back; real
  session, load 0.26, no other Easel window; 3 runs): the lift 25-30 ms at 100 %, 43-59 ms at fit; the event across the opposite corner
  22-26 ms at 100 %, 28-30 ms at fit; the events on one side after it median 16.4-17.5 ms (one frame at 60 Hz), worst 17.9-19.6 ms; the
  event back 14-21 ms; release to the painted frame 8-12 ms. The crossing events take a frame and a little more (the whole image is made
  again with its orientation), not more than two.
  The same at 8000 x 8000 (3 runs each, load 0.34-0.39, pointer parked; the drag is 1 pixel in for the lift, then across the opposite
  corner, ten events on that side, back to 3/4 of the size, release): at 100 % the lift 72, 79 and 289 ms, the crossing event 8, 12 and
  14 ms, the ten events after it median 15.9-16.8 ms and worst 17.6-19.1 ms, the event back 15-16 ms, release 16-17 ms, peak resident
  623, 623 and 859 MB (358 before); at the fit zoom (10 %), where the whole image is made at each event, the lift 589-632 ms, the
  crossing event 177-182 ms, the events after it median 16.4-16.6 ms with the worst of ten at 37.5-48.9 ms, the event back 17 ms,
  release 17 ms, peak resident 1570-1571 MB (360 before). Over a frame: the lift at both zooms, the crossing event at the fit zoom and the
  worst of ten at the fit zoom; they are in the INDEX's accepted slow cases (accepted by JOD, 2026-10-27).
  M1 c (JOD, 2026-10-28): all twelve steps ok (each handle past its opposite edge, back before the release, exactly on the edge, Shift on a
  corner, Ctrl's copy, Esc during a mirrored drag, Undo before and after the put-down and Redo, Transparent mode, a free-form selection, a
  Text box's handle). T006 ticked.

D. Manual and gate
- [x] T007 Manual M1 and M2 with the real mouse and keys (AS-1, AS-14, and the manual parts of the others) - done when JOD has tried
  M1 (the menus' look in the light and dark theme, every command on the whole picture and on a selection, Ctrl+I in a text box as
  Italic and from the menu as Invert, a pending polygon then Ctrl+R, the view after a quarter turn at 200 %) and M2 (an 8000 × 8000
  picture: each command's feel and the memory shown by the system monitor), and each step has its Result
  Result so far: M2 ok (JOD, 2026-10-28: an 8000 × 8000 picture, each command's feel and the memory). M1 a, b and b2 are in the Results of T002,
  T004 and T005 (ok). The last two M1 steps (JOD, 2026-10-28): a pending polygon then Ctrl+R (put down closed, then the picture turned, two
  steps) and the view after a quarter turn at 200 % (AS-3): both checked, ok. T007 ticked.
- [x] T008 Gate (all) - done when the full isolated `meson test` passes, `mutate.sh` for every REQ-0018 id and `--lint`,
  `spec.sh lint --names 0018`, and the Plan's interfaces and map are brought to the names as built
  Gate so far (2026-10-28): mutants for every REQ-0018 id that has some (01-07, 09, 10, 11): 131 of 131 killed (559 s), 0 crashed, 0
  inconclusive, on the code BEFORE the `scale_region` fix below (the run on the changed code is the next paragraph); `spec.sh lint` and `lint --names 0018` ok. REQ-0018-08 and -12 have no mutants (-12 is the time budget, measured by
  perf-0018). perf-0015 rerun (real session, load 0.9-1.8, three runs, A/B against a build of HEAD, interleaved) found a regression
  that the tests could not see: `scale_region` of T004 computed the source row for every pixel, so the 8000 x 8000 release at 100 % took
  222-229 ms (HEAD 137-140), the events at the fit zoom a median 514-545 ms (HEAD 428-455), the first event there 591-605 ms (HEAD 507-519),
  the 2000 x 2000 fit events 31-32 ms (HEAD 26-28). Fix: the row is computed once per row when there is no orientation, as before; after it
  the release is 134-147 ms, the fit events 455-457 ms, the first event 506-516 ms, the 2000 x 2000 fit events 26-28 ms, the peak memory
  unchanged (970-985 MB at 100 %, 2063-2065 MB at fit): the numbers of 0015's Plan and its accepted slow cases hold again.
  Mutants again on the changed `scale_region` (2026-10-28, every REQ id with a mutant in `src/core/selection.c` plus all REQ-0018 ids and
  REQ-0015-03 to -09): 317 mutants, 316 killed, 0 killed by crash, 0 inconclusive, 0 survived, 1 error in 2130 s; the error was a stale
  find ("the shown image is not oriented", REQ-0018-07: the lines it anchored on moved with the fix), re-anchored and run again: 40 of 40
  killed for REQ-0018-07 (176 s). So 317 of 317 killed on the changed code; `--lint` 1091, 0 stale.
  REQ-0018-08 (a transform joins the one step of its lift and put-down), proved by breaking it (2026-10-28; `canvas.c` copied aside, put
  back, diff identical): the code adds no history call for a transform, so the proof of the absence is the break. Line inserted in
  `easel_canvas_transform_selection`, after `self->sel_epoch++;` and before `g_signal_emit (self, signals[SIGNAL_SELECTION_CHANGED], 0);`:
  `{ uint32_t *copy = g_memdup2 (self->img->px, w * h * 4); if (copy != NULL && !easel_history_push_buffer (self->history, w, h, copy)) g_free (copy); }`
  (a step of its own). Every /0018/ ui case run alone with it: two failed, `as6_turn_marked_selection` (at
  `easel_window_get_modified (win) should be FALSE`: the step marked the picture modified while the selection floats) and the new
  `as8_turn_undo_redo` (the same line); the others passed, `as13_handle_flip` among them. My earlier note that runs 7 and 8 of `as13` check
  REQ-0018-08 was wrong: they test handle drags, which are not a transform command; the missing test was `as8_turn_undo_redo` (a marked
  4 x 2 selection turned and flipped: no modified mark while it floats; Undo before the put-down forgets it all, nothing to Undo or Redo;
  put down, Undo gives the picture before the lift, exactly one step, Redo the turned result), written after the break showed that only
  as6 observed it; it is red with the break and green without. The break is now a mutant ("a transform of a selection adds a history step
  of its own", killed in 9 s by ui-0018).
  The dropped-signal mutant is equivalent. Old text of `easel_canvas_transform_selection` after the lift:
  `lifted_here = TRUE;` then `g_signal_emit (self, signals[SIGNAL_PENDING_CHANGED], 0);`; new text: `lifted_here = TRUE;` alone. It survived
  all of ui-0018 and the reason is in the code: the `selection-changed` signal that the same function emits at its end makes the window
  run `on_selection_changed`, which calls `sync_history_ui` (Undo, Redo, the title), the same function `pending-changed` calls; the
  pending signal there only repeats it, so no input can tell them apart. It is not kept in `tests/mutants.txt`.
  Full isolated suite on the final code before this test: 51 of 51 (2026-10-28), warning-free.
  Final gate run (2026-10-28, after T006 and T007 were ticked and with the new `as8_turn_undo_redo`; nothing else running): the full isolated
  `meson test`: 51 of 51, 0 failed; `ninja` warning-free; `mutate.sh --lint` 1091, 0 stale; `spec.sh lint --names 0018` ok. T008 ticked.


- [x] T009 The check's findings F1 to F4 (REQ-0018-03, REQ-0018-10, REQ-0018-01; AS-5, AS-12, AS-13, AS-11) - done when ui
  `as5_clear_image_marked` (Clear Image with a marked selection removes it) and `as5_clear_image_put_down_refused` (Clear Image when the
  float's put-down cannot get its memory: its pixels go back, the clear goes on, then REQ-0013-19's dialog) are written first and shown
  failing (each by its mutant if it passes at once), with a mutant each; runs 7 and 8 of `as13_handle_flip` have a named mutant or a manual
  break shown failing; REQ-0018-10 reworded (F1, old and new shown to JOD) with no unreachable dialog path left in the code; the key mapping
  of REQ-0018-01 limited to keys of non-Latin scripts (F4: Czech ú gives no shortcut, Cyrillic ш gives Ctrl+I), test first, with a mutant;
  the group gate (full `meson test`, `mutate.sh --lint`) and `mutate.sh` for the changed REQ-0018 ids
  Result (2026-10-28): F4: ui `as1_keys_latin_accents_not_mapped` written first, red at `run_style_at (win, 0).size_pt == 12` (11: Ctrl+ú
  acted as Ctrl+[), green with `keyval_of_non_latin_script` in `window.c` (a key is mapped only when its letter's Unicode script is not
  Latin, Common, Inherited or unknown); Cyrillic ш at I's place still Italic (и is at B's place: Bold); mutant "a Latin letter with a
  diacritic is mapped to the key at its place" killed. F2: ui `as5_clear_image_marked` and `as5_clear_image_put_down_refused` passed at
  their first run (characterizations: the behavior was there, through `put_down_for_action`), proven by the mutants "Clear Image keeps a
  marked selection" and "Clear Image stops when the floating selection could not be put down", both killed (14 and 16 s). F3: runs 7 and 8
  of `as13_handle_flip` proven by applying 0013's mutants by hand with only those runs in the loop (the file put back, diff identical): run 7
  fails under "Undo puts a floating selection down instead of cancelling the lift" (`assert_same_picture`), run 8 under "Esc and Enter
  cancel a floating selection instead of putting it down" (Redo not there) and under "the float is put down at the place of its lift" (the
  pixel after Redo). F1: REQ-0018-10 and AS-12 reworded (Undo and Redo of a whole-picture transform swap the kept buffer and allocate
  nothing); no dialog path of the code was transform-only (the refused-Undo dialog is the resize's, REQ-0004-26), so none was removed;
  0002 says nothing that contradicts it. Gate: full isolated `meson test` 51 of 51; `mutate.sh REQ-0018-01 REQ-0018-03`: 17 mutants, 17
  killed (111 s; the one under 5 s, "Clear Image is on Ctrl+N", run again alone: killed by `as1_image_menus`); `--lint` 1095, 0 stale;
  warning-free. Learning-log lines for F2 and F1 added.

## Retro (proposed changes, 0018)
- sdd-build (step 3, Implement) and `docs/engineering.md` (Testing): a change to a shared hot loop reruns the benchmark of every feature
  that uses it, compared with a build of the last commit (a git worktree), the runs interleaved so that both see the same load, not with
  numbers written in a spec weeks before. (Found at T008: `scale_region` made 0015's 8000 x 8000 release 63 % slower with every test green.)


## Check
- Date and commit: 2026-10-28, on `850fbbf` with the 0018 work uncommitted in the tree (nothing of 0018 is committed yet).
- Tests: the full isolated `meson test` 51 of 51 (the T008 run, 2026-10-28; ui-0018 holds the /0018/ cases, among them
  `as8_turn_undo_redo` added at T008); core `test-transform`, `test-selection`, `test-lasso` and `test-history` green.
- Mutants: the whole list, `tools/isolated.sh tools/mutate.sh`: 1092 mutants: 1091 killed, 0 killed by crash (test gap), 0 inconclusive,
  0 survived, 0 errors, 1 long-only (not run), in 9436 s.
- Warnings: 0 (`ninja -C build`). Lints: `spec.sh lint`, `lint --names 0018`, `mutate.sh --lint` ok.
- Constitution: the core stays libc-only (`transform.c`, `lasso.c`, `selection.c`, `history.c`); tests shipped with each task; no AI
  trailers in the commits so far.
- Scenarios: AS-1 `as1_image_menus`, `as1_selection_menu`, `as1_keys_any_layout` + M1 a; AS-2 `as2_orient_pixels`, `as2_picture_transforms`;
  AS-3 `as3_turn_keeps_view` + M1; AS-4 `as4_invert`, `as4_invert_transparent_step`; AS-5 `as5_clear_image`; AS-6
  `as6_turn_marked_selection`, `as8_turn_undo_redo`, `line_oriented`; AS-7 `as7_transform_stretched_float`, `stretch_with_orientation`,
  `as7_flip_stretched`; AS-8 `line_oriented`, `as8_turn_free_form`, `handles_follow_turn` (C: proven by the mutant "a turned selection keeps
  its old size in the rectangle"); AS-9 `as9_turn_past_picture` (C: proven by "a turned selection is limited to the picture" and "the status
  bar does not follow a command on the selection"); AS-10 `as10_text_box_and_pending` (C: proven by "an Image command leaves a pending
  polygon pending" and "an Image command goes on when the text box could not be put down"), `as11_keys_in_an_entry`; AS-11
  `as11_ignored_while_held` (C: proven by "Ctrl+R acts during a held stroke"); AS-12 `as12_transform_no_memory`, `as12_transform_dialogs`,
  `as12_selection_dialogs`; AS-13 `as13_stretch_rect_mirror`, `stretch_with_orientation`, `as13_handle_flip` (runs 7 and 8: C, see F3);
  AS-14 `as14_handle_flip_shift` + M1 c; AS-15 perf-0018 (the Plan's measurements). REQ-0018-08: `as6_turn_marked_selection` and
  `as8_turn_undo_redo`, proven by the break written under T008 and its mutant.
- Results of the tasks: T001 red by build (`transform.h`), measured peaks in the Plan; T002 red by build, M1 a ok, characterizations of
  AS-10 and AS-11 proven by mutants; T003 perf-0018 in the real session, accepted memory case in the INDEX; T004 red runs, M1 b ok; T005
  `line_oriented` and `as8` red, `as9` and `handles_follow_turn` characterizations proven by mutants, M1 b2 ok; T006 core red by build, ui
  tests made red by taking the mirror out, M1 c ok (twelve steps), perf at 2000 and 8000 (accepted slow cases); T007 M1 and M2 ok; T008 the
  gate, including the perf-0015 regression of `scale_region` found and fixed, and REQ-0018-08's proof by a break.
- Findings (ranked):
  - F2 (medium, coverage, a failure path): two clauses of REQ-0018-03 have no test and no mutant: Clear Image with a marked selection
    (the selection is removed: only a floating one is tested, in `as5_clear_image`), and Clear Image when the floating selection's
    put-down cannot get its memory (REQ-0013-19: its pixels go back, the clear goes on, then the dialog; `image_action` goes on because
    `put_down_for_action` returns TRUE for a selection). Open: proposed, two ui tests written first and a mutant each.
  - F3 (low, proof): runs 7 and 8 of `as13_handle_flip` (Undo before and after the put-down of a mirrored drag) passed at their first run
    and name no mutant or break. Open: proposed, a break (the stretch drag's lift not undone by Undo) shown failing, or the 0015 Undo
    mutants named that kill it.
  - F1 (low, spec drift): REQ-0018-10 promises a dialog when Undo or Redo of a quarter turn cannot get the memory for the size it restores;
    the buffer step swaps the buffers and allocates nothing (`history.c`), so no input reaches it (said in `as12_transform_dialogs`' comment,
    not in the spec). Open: proposed wording "Undo and Redo of a whole-picture transform swap the kept buffer and allocate nothing: they
    cannot fail."
  - F4 (low, drift): `shortcut_keyval` (`window.c`) maps every key whose keyval is not ASCII to the Latin key at the same place, also Latin
    letters with diacritics (a Czech ú would act as Ctrl+[ in a text box); REQ-0018-01 says "a layout that is not Latin". Open: the spec
    states the broader rule, or the code is limited to non-Latin scripts; JOD decides.
- Settled in T009 (2026-10-28, JOD's decisions): F2 fixed (two tests and two mutants); F3 proven (0013's mutants fail runs 7 and 8); F1
  the spec reworded (REQ-0018-10, AS-12); F4 the code changed, not the spec (test first, a mutant). After T009: full isolated `meson test`
  51 of 51, `mutate.sh REQ-0018-01 REQ-0018-03` 17 of 17 killed, `--lint` 1095, 0 stale, 0 warnings. The full mutation run above was on the
  code before T009; the changed code (`image_action`'s callers and `shortcut_keyval`) is covered by the REQ-0018-01 and -03 run.
- Verdict: clean; `Status: implemented` proposed to JOD.
