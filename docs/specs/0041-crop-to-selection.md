# 0041 Crop to selection

Status: implemented
Reviewed: spec-reviewer, 2026-11-11 (9 findings merged; none asked)
Depends on: 0002, 0004, 0013, 0014, 0016, 0021, 0023

## Why
To keep only part of a picture (a screenshot's window, a face in a photo) today takes a selection, Copy, New and Paste, or dragging the
resize handles twice and moving the picture. Crop to Selection does it in one command, as later versions of Paint do; it is a modern
extra (`docs/product.md`, `docs/reference/new-behaviors.md`).

## Requirements
- REQ-0041-01 The Image menu shall have "Cr_op to Selection" directly after "Stretch and _Skew…", and a selection's context menu (REQ-0013-17) the same item directly after its "Stretch and _Skew…"; both with the shortcut Ctrl+Shift+K (JOD, 2026-11-11: free in Easel and in the desktop; Ctrl+Shift+X, other Paints' key, is Easel's swap colors, REQ-0003-07), shown beside the item, in the Shortcuts window and in Help. The item and its key are sensitive only while a selection exists (marked or floating, Select or Free-Form Select, REQ-0013-04, REQ-0014-01), and do nothing where REQ-0021-01's keys do nothing (before the picture exists, with a dialog, the quit prompt or the color chooser open, a button held in the canvas area, the focus in a text field of the font card); the key does nothing while a menu is open, but the item acts from its own menu (JOD, 2026-11-11, as REQ-0042-01 words it). The selection's context menu groups its last items as the Image menu groups its first two sections (JOD, 2026-11-11): Flip, Rotate, Stretch and Skew… and Crop to Selection, then a separator, then Invert Colors, Black and White… and Remove Background….
- REQ-0041-02 When the user crops with a marked selection, the app shall make the picture the selection's rectangle (REQ-0013-04; for a Free-Form selection, the bounding rectangle of its shape, REQ-0014-07, every pixel in it kept, the shape's outside included: JOD, 2026-11-11), its pixels exactly as they were, the rest removed.
- REQ-0041-03 When the user crops with a floating selection (moved, pasted, stretched or flipped, REQ-0013-07), the app shall make the picture the floating selection's rectangle as it is shown (JOD, 2026-11-11): each pixel is what the picture would hold if the selection were put down there (REQ-0013-11): the selection's own pixels where it has them (in both modes, REQ-0013-13; a fully transparent one stays #00000000, P7.7), elsewhere (a Free-Form shape's outside) the old picture, and where the rectangle lies outside the old picture the background color current at the crop (as REQ-0004-20 fills a grown picture; a transparent background gives transparent pixels).
- REQ-0041-04 A crop shall be exactly one undo step (REQ-0002-01): with a floating selection, the lift, its moves and copies and the crop are that one step, so Undo returns to the picture before the lift (REQ-0013-11); Undo restores the previous size and every pixel exactly (also those removed) and no selection; Redo gives the same picture, with the background color used at the crop even if it has changed since (as REQ-0004-22). It counts for the memory budget (REQ-0002-16) as 4 bytes per pixel of the old picture not in the new one, plus 12 bytes per pixel that differs from the old picture cut to the new size (the float's pixels and its hole; REQ-0002-16 allows 16), plus 64 (JOD, 2026-11-11: was "16 bytes per pixel the put-down changed"; every change log counts 12). The title gets the modified mark, as after any edit.
- REQ-0041-05 After a crop, the selection shall be gone, the tool stays as it was, the zoom stays, the view shows the new picture's top-left corner, and the status bar shows the new size (REQ-0004-09); Undo and Redo of a crop keep the scroll position as far as the size allows (REQ-0004-25). A crop with a marked selection covering the whole picture (Select All) changes nothing and adds no step (as REQ-0004-24); one with a floating selection exactly over the whole picture puts it down as one step (REQ-0013-11) with no change in size, and a memory failure of that put-down is REQ-0013-19's, not REQ-0041-06's: its dialog "Could Not Move the Selection", the picture as before the lift and the selection removed (JOD, 2026-11-12).
- REQ-0041-06 If the memory for the new picture or the crop's undo step cannot be had, then the app shall show the dialog of REQ-0004-26 ("Could Not Resize the Picture", with the new size) and change nothing: the picture, the history and the selection (marked, or floating where it was) stay as they were.
- REQ-0041-07 The pixel contract (P7): the crop copies whole pixels exactly (1); the view only magnifies (2); a floating selection's crop gives exactly the pixels it showed over the picture (3); the same picture, selection and background color give the same result (4).

## Scenarios
- AS-1 (REQ-0041-01) Given no selection, Then Image › Crop to Selection is insensitive and Ctrl+Shift+K does nothing; Given a marked selection, a floating one and a Free-Form one, Then the item is sensitive in the Image menu and in the selection's context menu, directly after Stretch and Skew, shows Ctrl+Shift+K, and the Shortcuts window and Help list it; Given a stroke held, a dialog, the quit prompt or the color chooser open, When Ctrl+Shift+K, Then nothing happens; Given the Image menu or the selection's menu open and a selection, When its item is clicked, Then it crops (the key with a menu open: manual M1) | Verified by: automated (ui)
- AS-2 (REQ-0041-02, REQ-0041-04, REQ-0041-05, REQ-0041-07) Given a 300 x 200 picture with distinct pixels and a marked selection (40, 30) to (139, 89), When Crop to Selection, Then the picture is 100 x 60, its pixel (0, 0) is the old (40, 30) and (99, 59) the old (139, 89), every pixel equal to the old one shifted, no selection, one step, the modified mark, the view at the top-left; When Undo, Then 300 x 200 and every pixel as before; When Redo, Then the 100 x 60 picture again | Verified by: automated (ui)
- AS-3 (REQ-0041-02) Given a Free-Form selection shaped as a triangle whose bounding rectangle is (10, 10) to (59, 49), When Crop to Selection, Then the picture is 50 x 40 with every pixel of that rectangle, those outside the triangle included | Verified by: automated (ui)
- AS-4 (REQ-0041-03, REQ-0041-04, REQ-0041-07) Given a 100 x 100 white picture and a 20 x 20 floating selection of black pixels moved to (90, 90), hanging 10 pixels past the right and bottom edges, When Crop to Selection, Then the picture is 20 x 20 and all black (the selection's pixels, also where they were outside the picture); one step; When Undo, Then the 100 x 100 white picture as before the lift, and no selection | Verified by: automated (ui)
- AS-5 (REQ-0041-05) Given a marked selection of the whole picture (Select All), When Crop to Selection, Then nothing changes and no step is added; Given a floating selection exactly over the whole picture, Then it is put down as one step and the size is unchanged | Verified by: automated (ui)
- AS-6 (REQ-0041-05, REQ-0041-06) Given memory made too small, When Crop to Selection with a marked and with a floating selection, Then "Could Not Resize the Picture" shows with the new size, and the picture, the history and the selection (the floating one where it was) are as they were; Given memory made too small for the put-down of a floating selection exactly over the whole picture, When Crop to Selection, Then "Could Not Move the Selection" shows, the picture is as before the lift and the selection is removed (REQ-0041-05, REQ-0013-19) | Verified by: automated (ui)
- AS-7 (REQ-0041-01) Given each language, Then the item's mnemonic is used by no other item of the Image menu and of the selection's context menu (the mnemonic tests of 0023 and 0035) | Verified by: automated (ui)
- AS-8 (REQ-0041-03) Given a 100 x 100 green picture, red as the background color, a Free-Form triangle of blue pixels floating so that its bounding rectangle (80, 20) to (119, 59) hangs 20 pixels past the right edge, When Crop to Selection, Then the picture is 40 x 40: the triangle's pixels blue, the pixels outside the triangle and inside the old picture green, those outside both red | Verified by: automated (ui)
- AS-9 (REQ-0041-01) Given a selection, When its context menu opens, Then it has four groups: Cut, Copy and Paste; Clear Selection, Select All and Copy To…; Flip, Rotate, Stretch and Skew… and Crop to Selection; and, after a separator, Invert Colors, Black and White… and Remove Background…, the last two groups being the Image menu's first and second sections | Verified by: automated (ui)

## Edge cases
- A 1 x 1 selection: the picture becomes 1 x 1.
- A marked selection partly outside the picture cannot exist (REQ-0013-04 keeps it inside); a floating one can (REQ-0041-03).
- A floating selection moved wholly outside the picture: the result is its own pixels and the background color; no old pixel is kept.
- A floating selection with fully transparent pixels: they stay #00000000 (P7.7).
- A selection over a pending curve, polygon, gradient or text box cannot exist (choosing a selection tool puts them down, REQ-0011-12, REQ-0017-17).
- Crop while a resize handle is held: ignored (REQ-0041-01).
- Ctrl+Shift+K again right after a crop: nothing, no selection is left.
- A floating selection pasted bigger than the picture (REQ-0016-07): the crop gives the pasted picture's size.

## Out of scope
- Crop by dragging the picture's edges (the resize handles of REQ-0004-16 already do it from the right and bottom).
- Auto-crop to the content; crop to a ratio.

## Plan
(Opus, 2026-11-11.) Read with `docs/engineering.md`, `docs/design.md`, 0004's resize step, 0013's float and 0016's grown paste.

Approach.
- One core function makes the crop, marked or floating: it builds the new picture's buffer, records one undo step, then swaps the
  buffer into the picture. A floating selection does not touch the picture while it floats (`selection.h`), so the picture is still
  the one before the lift and is the crop's "old picture"; the float's pixels come in through the same lay its view and put-down use
  (P7.3).
- The new picture, W x H at (x, y) of the old: a pixel inside the old picture is the lay's (picture, hole, copies, then the float),
  one outside it is the background color, then the float's own pixel over it where the float has one (REQ-0041-03; the copies left
  outside the old picture were never in it, so they are not either). A marked crop is the same with no float: the old pixels.
- A new step kind, the crop: the old size, the place (x, y), the new size, the fill, the removed pixels (the old pixels outside the
  rectangle, 4 bytes each, row-major) and a change log in the new picture's indexes for every pixel that differs from "the old
  picture cut to the rectangle, the fill outside" (12 bytes each: the float's pixels and the hole). Undo writes the changes' old
  values and puts the old size back with the removed pixels; Redo cuts again with the step's own fill and writes the new values,
  so a later change of the background color does not matter (REQ-0041-04). It counts 4 bytes a removed pixel plus its change log, the
  log charged by the one function every change log is charged by (12 bytes a change plus the step's 64; JOD, 2026-11-11: no second copy
  of that arithmetic).
  The marked crop has no changes. The resize step of 0004 is left as it is: no reader of it changes.
- The buffers are counted before anything is made (the removed pixels and the changes in a first pass), so each is allocated once,
  at its length, and no buffer handed to the history is longer than what it counts.
- Edge cases as the spec says: a marked selection of the whole picture changes nothing (no step); a floating one exactly over the
  picture goes through the ordinary put-down (REQ-0013-11), since that is what it is. A grown paste (REQ-0016-07) floating: the
  picture is first given back as it was before the paste (the canvas's `paste_base`), so Undo returns to before the paste, size too.
- The ui: the action `win.crop-to-selection` (Ctrl+Shift+K), its two menu items after Stretch and Skew, enabled with the selection
  in `on_selection_changed` and the gates of `update_clipboard_actions` (no picture, a stroke, a resize), and refused on activation by
  the gates Stretch and Skew uses (`view_actions_allowed`, panning, the font card's field). After the crop, the canvas forgets the
  selection, the window calls `picture_size_changed`, scrolls to the top-left and syncs the history (title, Undo).

Files and interfaces.
- `src/core/history.{c,h}`: `size_t easel_history_change_log_bytes (size_t n)`, the charge of a change log of n changes with its step
  (12 n + 64), made public and used by `step_bytes` for every kind that holds a log, the crop's included; `STEP_CROP`; `bool easel_history_push_crop (EaselHistory *h, int old_w, int old_h, int x, int y, int
  new_w, int new_h, uint32_t fill, uint32_t *removed, size_t n_removed, EaselChange *changes, size_t n)`, ownership as the other
  pushes (false: no memory, the caller still owns both).
- `src/core/selection.{c,h}`: `EaselSelectionResult easel_selection_crop (const EaselFloat *f, EaselImage *img, EaselHistory *history,
  int x, int y, int w, int h, uint32_t fill, size_t max_bytes)`: f NULL for a marked crop; DONE, NOTHING (the rectangle is the whole
  picture and nothing floats), or NO_MEMORY with the picture and the history as they were (an allocation above max_bytes counts as
  none, as the put-down's).
- `src/ui/canvas.{c,h}`: `EaselSelectionResult easel_canvas_crop_to_selection (EaselCanvas *self, int *new_w, int *new_h)`: the
  selection's rectangle (the bounding rectangle of a Free-Form shape, REQ-0014-07), the grown paste given back first, the float
  freed and the selection removed on DONE; on NO_MEMORY the selection stays (the float where it was) and the new size is returned for
  the dialog.
- `src/ui/window.{c,ui}`: the action, its accelerator, the two menu items, sensitivity; `show_resize_error` for REQ-0041-06.
- `src/ui/help.c` (the Shortcuts window), `data/help/*/index.html` (twelve pages), `po/` (two messages: "Cr_op to Selection" and
  "Crop to Selection" in eleven languages, mnemonics checked by the existing tests).

What grows. Per crop: the new buffer (4 bytes a new pixel), the removed pixels (4 bytes each) and the change log (12 bytes a changed
pixel, at most the new picture's pixels). Each counts against max_bytes as one allocation, as the put-down's log does; the step keeps the
last two. Nothing grows with use.

Scenario map.
| AS | Test |
|---|---|
| AS-1 | ui `/0041/as1_item_and_key`: sensitivity with none, marked, floating, Free-Form; both menus' order; the Shortcuts window and Help; the key refused with a stroke held, a dialog, the quit prompt, the color chooser, a menu open |
| AS-2 | core `crop_marked_offset` (every pixel, undo, redo, bytes held = counted); ui `/0041/as2_marked` (size, pixels, one step, title, view at the top-left by rendering the window, Undo, Redo) |
| AS-3 | ui `/0041/as3_free_form` |
| AS-4 | core `crop_float_hanging`; ui `/0041/as4_floating` (one step back to before the lift) |
| AS-5 | ui `/0041/as5_whole` (marked: no step; floating: the put-down) |
| AS-6 | core `crop_no_memory` (picture and history as they were); ui `/0041/as6_memory` (the dialog's text, marked and floating, the float where it was) |
| AS-7 | the existing mnemonic tests of 0023 and 0035 over the Image menu and the selection menu, all languages (they fail first if a mnemonic clashes) |
| AS-8 | core `crop_float_shape_fill` (blue, green, red by place); ui `/0041/as8_free_form_float` |
| REQ-0041-04 (Redo after a background change) | core `crop_redo_keeps_fill` |
| Edge: grown paste | ui `/0041/grown_paste` (Undo returns to the size before the paste) |

Constitution check: the rule and its arithmetic are in `src/core` (libc only, headless tests); the ui only gathers the rectangle and
calls it; one undo step per crop; P7 by copying whole pixels and the lay shared with the view.

Risks.
1. The change log of a floating crop can be as large as the new picture (12 bytes a pixel): an 8000 x 8000 float hanging over the
   edge is about 770 MB, over the default 512 MB budget, so it becomes the only step (REQ-0002-17) or, above max_bytes, the memory
   dialog. Checked by core `crop_float_hanging` at a small size and by the measure task at 8000 x 8000; no other choice is open
   without changing REQ-0041-04's count.
2. Undo and Redo of a crop keep the scroll by `picture_size_changed` (REQ-0004-25), as a resize's do; "the view at the top-left after
   the crop" is new and is checked by rendering the window (AS-2), at a scrolled 400 % too.
3. The grown paste: `paste_base` must be given back before the crop and the float's place kept; a ui test of that path (the edge row).
4. Budgets touched (engineering.md): Undo / redo at the next refresh on 1920 x 1080, and 8000 x 8000 without freezing. A measure task
   after the core task: `perf-0041`, the crop, its Undo and its Redo, on 1920 x 1080 and 8000 x 8000, marked and floating.

Measured (T004, `perf-0041`, the core: `easel_selection_crop`, `easel_history_undo` and `_redo`; six runs of three, 2026-11-11, real session, no Easel
window open, one-minute load 0.7 to 1.2; the whole path, from the action to the painted frame, is measured with the action, in T006). Milliseconds, the
lowest to the highest of the runs; the peak is the process's VmHWM after the first crop, undo, redo and undo, and after three (the later crops find the
redoable step of the one before, which the first does not).

| Case | Crop | Undo | Redo | Peak, first / three | The step counts |
|---|---|---|---|---|---|
| 1920 x 1080, marked, half the sides | 2 to 4 | 1 to 3 | 0 to 1 | 24 MB | 6 MB |
| 1920 x 1080, marked, 100 x 100 | 1 to 2 | 1 | 0 | 26 MB | 8 MB |
| 1920 x 1080, floating, half the sides, hanging | 2 to 7 | 1 to 2 | 0 to 1 | 41 MB | 13 MB |
| 1920 x 1080, floating, the whole picture moved | 11 to 17 | 4 to 5 | 2 | 85 to 86 MB | 30 MB |
| 8000 x 8000, marked, half the sides | 86 to 125 | 64 to 96 | 28 to 33 | 490 / 674 MB | 183 MB |
| 8000 x 8000, marked, 100 x 100 | 82 to 99 | 58 to 62 | 9 to 11 | 491 / 735 MB | 244 MB |
| 8000 x 8000, floating, half the sides, hanging | 164 to 189 | 80 to 93 | 30 to 34 | 781 / 1193 MB | 412 MB |
| 8000 x 8000, floating, the whole picture moved | 390 to 439 | 154 to 162 | 95 to 99 | 1650 / 2566 MB | 916 MB |

Against the budgets: Undo and Redo at 1920 x 1080 take at most 5 ms in the core (a frame is 16 ms; the texture upload is T006's); the slowest 8000 x 8000
crop, 439 ms, is under the 500 ms of the other commands (REQ-0018-12). The memory is above what the Plan said: the worst case, a floating selection of the
whole 8000 x 8000 picture moved by half its sides, peaks at 1650 MB for the first crop (the picture 255 MB, the float's image 255 MB, the new picture
255 MB, the removed pixels 192 MB and the log of 64 million pixels at 12 bytes, 768 MB), not about 1.3 GB, and at 2566 MB when an earlier crop's redoable step
(916 MB) is still held while the next is made; the step, 916 MB, is over the default 512 MB budget and is kept alone (REQ-0002-17). The earlier accepted
8000 x 8000 peaks are 845 MB (0018, a command) and 1101 MB (a flip of a floating selection).
Accepted by JOD, 2026-11-11, as a cost of the same kind: the worst floating crop's 1650 MB for a first crop and 2566 MB with an earlier crop's redoable
step held, with nothing added to reduce either (no new mechanism): the later crop is made as 0018's commands are made right after an Undo, the new step
built while the old redoable one is still held and freed when the new one is committed, so three buffers are alive for a short time. A shortage is
REQ-0041-06's dialog with nothing changed, which T008 proves. The window's whole path is measured with the action in T006: `perf-0041` gains a window
mode there (JOD, 2026-11-11) and its numbers join this table.

Measured through the window (T006, `perf-0041 window`: the action Crop to Selection, Undo and Redo, each from the activation to the next painted frame, a
marked selection made with the pointer at the fit zoom; three runs of three, 2026-11-11, real session, no Easel window open; the one-minute load was 14
to 15 while the processes of my earlier mutation run finished, the CPU idle during the runs, and the three agree). The floating cases are measured with
the floating crop in the window (T007; its rows are below).

| Case (the window) | Crop | Undo | Redo | Peak, first / three |
|---|---|---|---|---|
| 1920 x 1080 at 69 %, marked 960 x 540 | 13 to 21 ms | 13 to 19 ms | 14 to 16 ms | 138 / 144 MB (129 MB shown) |
| 1920 x 1080 at 69 %, marked 101 x 101 | 14 to 18 ms | 17 to 20 ms | 14 to 17 ms | 145 MB |
| 8000 x 8000 at 10 %, marked 3991 x 3991 | 100 to 118 ms | 73 to 147 ms | 32 to 44 ms | 617 / 804 MB (374 MB shown) |
| 8000 x 8000 at 10 %, marked 91 x 91 | 91 to 102 ms | 64 to 133 ms | 11 to 15 ms | 617 / 862 MB |

Floating, T007 (`perf-0041 window`, the same three runs of three; the one-minute load was 12 to 16 from other processes of the machine, steady, the three
runs agreeing; a floating selection made by dragging a marked half, or Select All, by half the sides):

| Case (the window) | Crop | Undo | Redo | Peak, first / three |
|---|---|---|---|---|
| 1920 x 1080 at 69 %, floating 960 x 540 hanging | 14 to 16 ms | 17 to 20 ms | 14 to 17 ms | 155 / 169 MB (142 MB shown) |
| 1920 x 1080 at 69 %, floating 1920 x 1080 moved | 14 to 20 ms | 10 to 44 ms | 13 to 22 ms | 187 / 216 MB (155 MB shown) |
| 8000 x 8000 at 10 %, floating 3991 x 3991 hanging | 169 to 190 ms | 84 to 86 ms | 33 to 39 ms | 930 / 1341 MB (459 MB shown) |
| 8000 x 8000 at 10 %, floating 8000 x 8000 moved | 399 to 454 ms | 157 to 162 ms | 97 to 101 ms | 1801 / 2717 MB (643 MB shown) |

The worst floating crop through the window peaks at 1801 MB for a first crop and 2717 MB with the earlier crop's redoable step held: the core's 1650 and
2566 MB (accepted by JOD, 2026-11-11) and about 150 MB of the window's own (its tiles and the float's shown image). Reported to JOD, who accepted the
figures of the core as the cost; these are those figures and the window's overhead, with no mechanism added.

A painted frame cannot come sooner than the frame clock's next tick (16.7 ms), so 13 to 20 ms at 1920 x 1080 is one to two frames: the crop, its Undo and
its Redo are shown at the next refresh (the budget of docs/engineering.md), and at 8000 x 8000 they are 0.1 s and under.

Spec gap settled (JOD, 2026-11-11): REQ-0041-04's "16 bytes per pixel the put-down changed" became "12 bytes per pixel that differs
from the old picture cut to the new size", charged by `easel_history_change_log_bytes`, which a core test checks the charge against.

## Tasks
(Opus, 2026-11-11.) Each task: its test first and seen red, then the code; the build warning-free; the feature's ui cases and the core
and io tests green; a task that adds a rule under `src/core` adds its mutants to `tests/mutants.txt` in the same task.

- [x] T001 The crop step (REQ-0041-04; AS-2, AS-4) - done when core `history_crop_step` passes: a crop step pushed with removed pixels
  and a change log, Undo giving the old size and every pixel back (the removed and the changed), Redo giving the cut picture with the
  step's own fill even after the caller's fill changed, an asymmetric picture and an off-center rectangle hanging past two edges; the
  bytes counted (`easel_history_bytes`) equal 4 x removed + `easel_history_change_log_bytes (n)`, compared with that function and
  not with a number written in the test; the memory held (capacity of both buffers) equal to the memory counted; `step_bytes` uses
  `easel_history_change_log_bytes` for every change log (the existing history tests pass unchanged); mutants: the removed pixels'
  order, the fill on Redo, the old values on Undo, the charge.
  Result: red first: `test-history` failed to build (`implicit declaration of function easel_history_push_crop` and
  `easel_history_change_log_bytes`). Cases of `history_crop_step`: a marked crop inside (no log), hanging past the right and bottom
  edges, past the left and top, larger than the old picture all around it, wholly outside it, a fully transparent fill (#00000000, P7.7)
  and 1 x 1, each on distinct asymmetric pixels with an earlier step kept undoable through it, Undo and Redo twice, and the bytes counted
  compared with `4 x removed + easel_history_change_log_bytes (n)`; `history_crop_charge` checks the function against 64, 64 + 12 and
  64 + 12000, that an ordinary log's step and a growth's are charged by it, a crop's, and a step above the budget kept alone. The memory
  held is the two buffers the caller allocated at their length (the history allocates none; Undo and Redo allocate the new picture only);
  the builder that allocates them is T002's, with its own held-against-counted case. 10 mutants (undo's removed order, old values and
  corner; redo's fill, canonical fill, new values and corner; 8 bytes a removed pixel; 16 bytes a change; an ordinary step leaving the
  one charge function), all killed. Core and io 51/51, ui-0002 and ui-0004 green, build warning-free.
- [x] T002 The marked crop (REQ-0041-02, -05, -06, -07; AS-2, AS-3, AS-5, AS-6) - done when core `crop_marked_offset` (every pixel of a
  100 x 60 cut from a 300 x 200 picture of distinct pixels), `crop_free_form_box` (a shape's whole bounding rectangle kept),
  `crop_whole_is_nothing` (NOTHING, no step) and `crop_no_memory` (max_bytes below each allocation in turn: NO_MEMORY, the picture and
  the history byte for byte as before) pass; mutants for each.
  Result: red first: `test-selection` failed to build (`implicit declaration of function easel_selection_crop`). Cases:
  `crop_marked_offset` (100 x 60 at (40, 30) of 300 x 200 distinct pixels: every pixel the old one shifted; Undo and Redo exact; bytes counted
  = 4 x 54000 + 64; memory held by the step, measured with `mallinfo2`, within 1 % + 4096 of the counted), `crop_free_form_box` (the box's
  pixels all kept, outside any shape included), `crop_whole_is_nothing` (NOTHING, no step, 0 bytes; then a 1 x 1 piece), `crop_no_memory`
  (max_bytes one below the removed pixels with them smaller and with them larger than the new picture, so that each check stands alone;
  each at its size: DONE; a side of 0 or 10001 with and without a history; a real address-space shortage for a 13 MB picture; each time the
  picture, and the history's bytes and redoable step, as before) and `crop_hangs_fill` (six rectangles hanging past one to four edges, wholly
  outside and larger than the picture, a fully transparent fill: the inside the old pixels, the outside the fill, #00000000 when
  transparent; Undo and Redo exact). The first mutant run found two mutants killed only by a heap abort (a wrong count of removed pixels
  overran the buffer) and two survivors in the size rule (`easel_history_push_crop` refuses a bad size too, so with a history the rule was
  redundant): the removed-pixels loop is now bounds-clipped (core.md), and the size rule is tested with no history. Of the 12 mutants for
  this task 11 are killed, none by crash. Survivor, accepted: "a crop that cannot be recorded still changes the picture" (REQ-0041-06): it
  needs `easel_history_push_crop` to fail, which only the history array's `realloc` can cause (the sizes are checked before), and the
  core tests have no hook for that; the ui's memory case (T008) goes through the same return value. With T001's 10, 22 mutants of REQ-0041.
  Core and io 51/51, build warning-free.
- [x] T003 The floating crop (REQ-0041-03, -04, -07; AS-4, AS-8) - done when core `crop_float_hanging` (AS-4: 20 x 20 black over 100 x 100
  white at (90, 90): all black, Undo to before the lift), `crop_float_shape_fill` (AS-8: blue in the triangle, green outside it in the
  old picture, red outside both), `crop_float_equals_lay` (inside the old picture every pixel equals `easel_selection_lay` of the same
  float, with copies and a hole) and `crop_redo_keeps_fill` pass; the change log holds exactly the pixels that differ from the plain
  cut (counted, and compared with the charge of T001); mutants for the fill outside, the float over it, the copies outside left out.
  Result: red first: with the floating branch still a stub that refused, `crop_float_hanging` failed at its first `easel_selection_crop ... ==
  EASEL_SELECTION_DONE` check. Cases: `crop_float_hanging` (AS-4: 20 x 20 black at (90, 90) of 100 x 100: all black, Undo to the picture before
  the lift, bytes = 4 x 9900 + the charge of a log of 400, that count compared with an independent count of the pixels differing from the plain
  cut), `crop_float_shape_fill` (AS-8: blue in the triangle, green outside it inside the old picture, red outside both; bytes and Undo),
  `crop_float_equals_lay` (inside the old picture every pixel equals `easel_selection_lay` of a float with a hole and two copies, one hanging
  past the edges that the crop leaves out; outside, the float's own pixels), `crop_redo_keeps_fill`, `crop_float_step_holds_what_it_counts`
  (`mallinfo2`: held within 1 % + 4096 of counted for a 513 x 512 float over 600 x 600 stripes), `crop_float_no_memory` (the log is the third
  allocation: a limit one byte below its 4800 refuses with the picture, the history and the float as they were; at 4800 it is made),
  `crop_float_paste_skips_clear` (a pasted float's transparent pixels: the picture's own inside, the fill outside) and `crop_float_whole`
  (a float over the whole picture is a step, never NOTHING). 9 new mutants (the float outside the picture, its shape outside, a pasted float's
  transparent pixels outside, the lay inside, the float in the lay, the log's old value, the log keeping every pixel, the log's limit, NOTHING
  for a float), all killed, none by crash; with the 22 before, 31 for REQ-0041, 30 killed, the survivor accepted in T002. Core and io 51/51,
  build warning-free. Not yet done, by design: the float's lazily made image is completed by the caller (T007), as for a put-down.
- [x] T004 Measure (engineering.md: Undo / redo at the next refresh on 1920 x 1080; 8000 x 8000 without freezing) - done when
  `perf-0041` has run three times in the real session with no Easel window open: the crop, its Undo and its Redo, marked and floating,
  on 1920 x 1080 and on 8000 x 8000, the time of each and the peak resident memory, recorded in the Plan; anything over budget
  reported to JOD before T005.
  Result: `tests/perf/perf-0041.c` (a benchmark, registered, core only) run six times in the real session with no Easel window open (load 0.7 to 1.2);
  the numbers are in the Plan's table. Time is within budget everywhere (the slowest 8000 x 8000 crop 439 ms; Undo and Redo at 1920 x 1080 at most
  5 ms). The memory of the worst floating case, 1650 MB for a first crop (2566 MB with an earlier crop's redoable step held), is above the Plan's 1.3 GB
  and above the 845 and 1101 MB accepted for 0018: accepted by JOD (2026-11-11), see the Plan. Deviation: the whole path, from the window's action to the painted frame,
  cannot be measured until the action exists; it is added to T006 (perf-0041 gains a window mode there). The first run of the benchmark showed 2574 MB for the
  worst case; that was the previous run's redoable step, so the benchmark now prints the peak after the first run and after three.
- [x] T005 The item, the menus and the key (REQ-0041-01; AS-1) - done when ui `/0041/as1_item_and_key` passes by the real paths: the
  two items read from the real menu models (the Image menu and the selection's context menu), each directly after Stretch and Skew with
  its label, action and Ctrl+Shift+K, and activated as a click activates them (the item's own action and target through the window's
  action group); Ctrl+Shift+K sent through the window's application shortcut controller (the shortcut whose trigger is Ctrl+Shift+K,
  activated as GTK activates it; a probe first that this path reaches the action and that a disabled action does nothing through it);
  enabled with a marked, a floating and a Free-Form selection, disabled with none, after a crop, before the picture exists, with a
  stroke held and during a resize; with a dialog, the quit prompt, the color chooser or a menu open, and with the focus in the font
  card's field, the key does nothing; the Shortcuts window lists it. Manual check: JOD presses Ctrl+Shift+K and uses both menus in the
  real session, and says where the keyboard focus is after the crop.
  Result: manual (JOD, 2026-11-11, real session): "all works" (the key, both menus, the grey item with no selection); where the keyboard focus is after the crop was not reported and is not claimed. Automated part: Red first: `/0041/as1_item_and_key` failed at its first check
  (`has_accel ("win.crop-to-selection", GDK_KEY_k, CS)` should be TRUE). Cases (`tests/ui/test-window.c`): `as1_item_and_key`: a probe that
  the window's own shortcut controller path (`fire_shortcut`: the shortcut whose trigger is Ctrl+Shift+K, activated as the controller does)
  reaches an enabled action (Ctrl+Shift+X swaps the colors) and that a disabled one does nothing through it (Ctrl+Z with nothing to undo);
  no selection: insensitive, the key does nothing; a marked selection: sensitive; with the selection being dragged, a dialog (Stretch and
  Skew's) and the color chooser open the key does nothing; the key crops to 100 x 60, no selection, modified, the item insensitive again,
  the key again does nothing, Undo gives 300 x 200 back; the Image menu's item found in the really opened menu and activated as a click does;
  the selection's menu opened by the secondary press and its item activated; Free-Form and floating selections make it sensitive and Clear
  Selection insensitive again; before the picture exists it is insensitive and the key does nothing. `as1_key_with_the_quit_prompt`: the
  prompt over a marked selection, the key does nothing. The three menu-structure expectations of 0018 gained the item (and one index moved by
  one); `as2_every_key_listed` (0021) holds the Shortcuts window to the menu's label and key. Pulled forward from T006 and T009, because the
  gates are observable only through a crop and the suites must stay green: the marked crop in `easel_canvas_crop_to_selection` and the
  window's handler (T006 tests it deeper: the view, the status bar, Undo and Redo's scroll, the floating case is T007's), the eleven
  translations of the two messages and the twelve Help pages' row (T009 then only checks them). Mnemonics: five of the eleven first drafts
  clashed in their menus (cs, fr, it, pt, ro: found by `/0023-lang/as14_menu_mnemonics_differ`) and were chosen again from the letters the
  pages leave free. 10 mutants (the key's accelerator, the handler's gate, the sensitivity, each menu's item, the Shortcuts name and key, the
  selection left after a crop, the history's state), 8 killed by a check, 2 killed only by a `g_error` abort in 0021's existing test (the
  Shortcuts row's title and key: its way of failing, accepted), and survivor, accepted: "a crop does not update the history's state in the
  window" (equivalent: the selection's removal reaches `on_selection_changed`, which syncs it; kept explicit since the crop is a history change
  whichever signal follows). Not testable and not claimed: the key with a menu open (GTK holds the keys for the popup; M1's), and the focus in
  the font card's field with a selection (a text box and a selection cannot coexist; the action is insensitive then). Core and io 51/51, ui-0002,
  -0004, -0010, -0013 to -0016, -0018, -0019, -0021, -0022, -0023, -0035 and -0041 green, the 24 language runs of 0023 green, build warning-free.
  Afterwards (JOD, 2026-11-11, from his two screenshots, the Image menu and the selection's menu): the wording about "a menu open" was changed as 0042's is
  (the key does nothing with a menu open, the item acts from its own menu), and the selection's menu got the Image menu's grouping, a separator after Crop to
  Selection (REQ-0041-01, AS-9; pointer notes on REQ-0013-17 and REQ-0018-02). Tests: 0018's `as1_selection_menu` (four sections, the third and the fourth
  by their items, Invert Colors' key in the fourth), 0034's and 0035's expectations (the section of Invert Colors, Black and White and Remove Background is
  now the fourth); the three menu mutants were made again with their tails (the two menus are now the same around the item), and a fourth, the separator,
  is killed by 0018's test. ui-0013, -0014, -0016, -0018, -0019, -0021, -0022, -0023, -0034, -0035, -0037 to -0041 and the 24 language runs green.
- [x] T006 The marked crop in the window (REQ-0041-02, -05; AS-2, AS-3, AS-5) - done when ui `/0041/as2_marked` (size, pixels, one
  step, the title marked, no selection, the status bar's size, the view at the top-left by rendering the window, also from a scrolled
  400 %; Undo and Redo keeping the scroll as far as the size allows), `/0041/as3_free_form` and `/0041/as5_whole` (marked: no step;
  floating exactly over the picture: one put-down step, size unchanged) pass, and `perf-0041` has a window mode that times the action to the painted
  frame, its Undo and its Redo on 1920 x 1080 and 8000 x 8000 (T004's deferred half), the numbers in the Plan.
  Result: red first: `/0041/as2_marked_scrolled` failed at `gtk_adjustment_get_value (view.h) == 0` (671: the scroll was only clamped, not at the top-left); the
  other three cases passed when first run, `as2_marked` and `as3_free_form` because the marked crop and the 100 % view were already right from T005, and
  `as5_whole` ran only after its code, so mutants stand for them (below). Cases: `as2_marked` (100 x 60 at (40, 30) of 300 x 200 distinct pixels: the
  size by `assert_size` (picture, tile texture and the status bar's size), every pixel the old one shifted, no selection and the selection field empty,
  modified, the tool and the zoom as they were, `easel_canvas_top_left_in_view` (0, 0), the window rendered by `assert_shown`; Undo: 300 x 200 and every
  pixel, no step left, not modified; Redo: the piece), `as2_marked_scrolled` (600 x 400 at 400 %, scrolled to (671, 900+) in the old picture, a 500 x 300 piece:
  both scrolls 0, top-left (0, 0), the centers of four pixels by rendering the window; then Undo and Redo with the scroll as the new size allows, from the
  values the adjustments really have), `as3_free_form` (a lasso whose box of pixels is (10, 10) to (59, 49): the whole 50 x 40 box kept, outside the triangle
  included, the tool stays Free-Form Select; Undo) and `as5_whole` (Select All: nothing changes, no step, not modified, the selection stays; a flipped
  floating selection over the whole picture: put down, one step, the picture flipped and shown, size unchanged; Undo). Spec finding, not a code one: AS-3's
  "bounding rectangle (10, 10) to (59, 49)" was written without tracing it: the straight-line triangle (10, 10), (60, 10), (10, 50) gives 49 x 40 by the
  alternate rule and the app's lasso gives 50 x 39 for (10, 10), (61, 10), (10, 50); (10, 51) gives 50 x 40, found by running it; AS-3 describes the box and
  is met. Code: the crop sets both scrolls to 0 when the size changes (the zoom stays), and a floating selection exactly over the whole picture is put down by
  `finish_selection` (its own dialog if that fails). Mutants: 5 (the scroll on both axes, the vertical axis alone, the rebuild of the view, the put-down of
  a whole-picture float, the history's state) with the earlier two of REQ-0041-05; all killed but the history's state, the survivor accepted in T005 (equivalent:
  the selection's removal syncs it); a first mutant showed the `else easel_canvas_update` I had written to be redundant (the put-down updates the canvas), and
  I removed it. `perf-0041 window` added (T004's deferred half): numbers in the Plan. Core and io 51/51, ui-0041 6/6, build warning-free.
- [x] T007 The floating crop in the window (REQ-0041-03, -04; AS-4, AS-8) - done when ui `/0041/as4_floating` (one step; Undo to before
  the lift, no selection), `/0041/as8_free_form_float` and `/0041/grown_paste` (a paste larger than the picture with Resize Canvas to
  Picture, cropped: Undo returns to the size and pixels before the paste) pass.
  Result: red first: `/0041/as4_floating` failed at `assert_size`: `(img->width == w): (100 == 20)`, a floating selection not cropped yet. Cases: `as4_floating`
  (AS-4: 20 x 20 black dragged to (90, 90) of 100 x 100 white: all black, floating before, no selection after, the tool and the view at the top-left;
  Undo: the picture before the lift with the black block where it was, no step left, not modified), `as8_free_form_float` (AS-8: a lasso triangle of blue
  pixels over green, red the background, dragged to the box (80, 20): blue by the selection's own mask, green inside the old picture outside it, red outside
  both; Undo; the background changed to cyan, Redo: the same picture, red kept) and `grown_paste` (a 320 x 100 fully transparent paste with Resize Canvas
  to Picture on 300 x 200, dragged down by 50, the background set to red: the 320 x 100 picture is the old picture's pixels where it had them (not the grown
  picture's strip) and red beyond its width; Undo gives 300 x 200 as before the paste, no step left; Redo; then an ordinary move and put-down whose Undo
  does not resize). Code: `easel_canvas_crop_to_selection` takes a float's box and image; for a grown paste it swaps the picture and `paste_base` (no
  allocation) so the crop is made from the picture as it was before the paste, and swaps back if the crop fails (T008 proves that). Mutants: 5 new (the
  grown paste's swap, the float left out, the background color of the outside, the float's box, the grown paste's base kept after the crop), all killed; a
  first run left the last alive, which was user-visible (a later put-down would have been taken for a grown paste's, its Undo resizing the picture), so
  `grown_paste` gained its last step and killed it. Not mutated, equivalent: `easel_selection_complete` before the crop (the float is whole at the release of
  any stretch, and a crop is refused while a button is held). With the earlier tasks' 44 the file has 49 mutants of REQ-0041, in the core and the
  window; two survive, both accepted (the push failure of the core, and the history's state in the window). `perf-0041 window` has the four floating-and-marked cases per size; numbers in the
  Plan. Core and io 51/51, ui-0041 9/9, build warning-free.
- [x] T008 Memory (REQ-0041-06, REQ-0002-17; AS-6) - done when ui `/0041/as6_memory` passes through the memory-limit hooks: a marked and a
  floating crop refused by the allocation limit show "Could Not Resize the Picture" with the new size, and the picture, the history
  and the selection (the float where it was) are as they were; and `/0041/memory_8000`, an 8000 x 8000 floating selection hanging past
  an edge: (a) with the history budget set below the step and the allocation limit above it, the crop is made and is the only undo
  step (REQ-0002-17), Undo giving the picture back exactly; (b) with the allocation limit below the change log, the dialog, and the
  picture byte for byte unchanged, the float where it was. (a) holds about 1.7 GB at its peak (T004 measured 1650 MB: the picture, the float's image, the new buffer, the removed
  pixels and the log of about 770 MB); if that is too much for the suite's machine the case is reported to JOD, not shrunk.
  Result: characterization: `/0041/as6_memory` passed at its first run, the refusal being built in T005 to T007, so it is proved by breaks (below). Cases:
  `as6_memory` (300 x 200 of distinct pixels, the allocation limit 100 bytes: a marked 100 x 60 selection, then a floating one hanging past the edge, then a
  grown paste's float, each refused with "Could Not Resize the Picture" and "There is not enough memory for a picture of W x H pixels." with the new size
  (100 x 60, 100 x 60, 320 x 100), closed by its button; each time the picture byte for byte, the history (no step, not modified) and the selection as they were
  (the floating one still floating where it was, the grown picture still 320 x 200); with the limit back the marked-then-floating crop is made and Undone, and
  the grown paste is put down after the refusal as its one step whose Undo gives back 300 x 200 as before the paste) and `memory_8000` (8000 x 8000 of distinct
  pixels, one earlier step, the history budget 16 MB, the whole picture flipped, lifted and moved by half the sides; its crop: 192 MB of removed pixels, a new
  picture of 256 MB, a log of 768 MB; (b) with the limit at 300 MB, over the first two and under the log: the dialog for 8000 x 8000, the picture byte for byte
  as it was, the float where it was and still floating, the earlier step kept; (a) with the limit off: made, the pixels the flipped float's, its 916 MB step far
  over the 16 MB budget kept as the only step, Undo giving the picture before the lift and nothing left to undo, the earlier step gone, REQ-0002-17). The case
  ran at 5 s and holds about 2 GB at its peak, which this machine has. A first version of `memory_8000` moved the whole picture unflipped, so that the crop
  equalled the picture and the budget mutant survived; the float is flipped now and the picture is the crop's, not its own. Mutants: 4 new (no dialog, the
  wrong size in it, the grown paste not given back after a refusal, a step over the budget dropped instead of kept), all killed; the survivor of the core, push
  failure, stays accepted. Core and io 51/51, ui-0002, -0004, -0013, -0016, -0018 and -0041 green, build warning-free.
- [x] T009 Words in every language (REQ-0041-01; AS-1, AS-7) - done when "Cr_op to Selection" and "Crop to Selection" are in
  `po/easel.pot` and the eleven languages, with mnemonics the existing tests of 0023 and 0035 accept over the Image menu and the
  selection's menu (those tests run first with a clashing draft and fail), and Help lists the key in its twelve pages.
  Result: characterization: nothing new was built here; the work was done in T005 (pulled forward, so that the suites stayed green) and is checked again here. The two
  messages "Cr_op to Selection" and "Crop to Selection" are in `po/easel.pot` and in the eleven languages (`msgfmt -c` clean but for the headers the other files
  lack too), the key's row is in all twelve Help pages (Ctrl+Shift+K, the language's name for the command). The mnemonics are tested by 0023's
  `as14_menu_mnemonics_differ` over the Image menu and the selection's menu in every language (the 24 runs of `ui-0023-<language>` and `-only-c-`, green after the
  menu was split, and first red for five languages with the drafts' letters: cs, fr, it, pt, ro), the Shortcuts window and its Help rows by 0021's
  `as2_every_key_listed` and `as4_help_page_text` (green; mutants of the row's title and key killed by them, by a `g_error` abort, accepted). The translations are
  machine drafts like the others, unread (the Known limit of the release notes).
- [x] T010 Manual M1 (AS-1 to AS-8) - done when JOD has cropped a marked, a Free-Form and a floating selection (one hanging past the edge),
  undone and redone each, and tried Ctrl+Shift+K with no selection, in the real session; his notes in the Result.
  Result: manual (JOD, 2026-11-11, real session): "all works" (marked, Free-Form and floating crops, Undo and Redo, Ctrl+Shift+K with no selection, the menus with the
  separator); where the keyboard focus is after a crop, and the key with a menu open, were not reported separately and are not claimed.

## Check
- Date and commit: 2026-11-12 (the runs from 2026-11-11 23:30), on `5c29c3c` (T001 to T004) with the uncommitted work of T005 to T010, the menu change and the check's fixes.
- Tests: `meson test` (isolated) 141 of 141 before the check's fixes, and again 141 of 141 on the final tree (then `ui-0041` 14 of 14 with F2's case, and core and io 51 of 51); the scale 2 list 5 of 5; core and io 51 of 51.
  The cases of this feature: core `history_crop_step`, `history_crop_charge`; core `crop_marked_offset`, `crop_free_form_box`, `crop_whole_is_nothing`,
  `crop_no_memory`, `crop_hangs_fill`, `crop_float_hanging`, `crop_float_shape_fill`, `crop_float_equals_lay`, `crop_redo_keeps_fill`,
  `crop_float_step_holds_what_it_counts`, `crop_float_no_memory`, `crop_float_paste_skips_clear`, `crop_float_whole`, `crop_float_over_own_hole` (the check's);
  ui `/0041/*` (14): `as1_item_and_key`, `as1_key_with_the_quit_prompt`, `as2_marked`, `as2_marked_scrolled`, `as3_free_form`, `as5_whole`, `as4_floating`,
  `as4_float_same_size` (the check's), `as4_stretched` (the check's), `as8_free_form_float`, `grown_paste`, `as6_memory`, `memory_8000`, `as6_whole_float_put_down_fails` (F2's); `perf-0041` (core and window).
- Mutants: `tools/mutate.sh --core`: 585 mutants, 581 killed, 0 killed by crash, 0 inconclusive, 3 survived, 1 accepted (212 s). The 56 mutants of REQ-0041 and the 2 of
  REQ-0002-17 run by id during the tasks and the check: all killed but the accepted ones below; `--lint` 1509, 0 stale.
- Survivors, accepted: (1) REQ-0041-06 "a crop that cannot be recorded still changes the picture" (core): it needs the history's `realloc` to fail, and the core tests have
  no hook for that; the ui memory cases go through the same return value. (2) REQ-0041-04 "a crop does not update the history's state in the window": equivalent,
  the selection's removal reaches `on_selection_changed`, which syncs it. (3) REQ-0037-02 and REQ-0038-03 (the two of 0038's check), equivalent. Killed only by a
  `g_error` abort in 0021's existing test, accepted (its way of failing): the Shortcuts row's title and its key (REQ-0041-01).
- Warnings: 0. Lint and `--names 0041` clean; `tools/spec.sh status`: 7 requirements, 9 scenarios, 10 of 10 tasks.
- Scenarios: AS-1 `as1_item_and_key`, `as1_key_with_the_quit_prompt` + 0021's `as2_every_key_listed`, `as4_help_page_text` + M1; AS-2 `crop_marked_offset`,
  `as2_marked`, `as2_marked_scrolled`; AS-3 `crop_free_form_box`, `as3_free_form`; AS-4 `crop_float_hanging`, `as4_floating`, `as4_float_same_size`, `as4_stretched`;
  AS-5 `crop_whole_is_nothing`, `crop_float_whole`, `as5_whole`; AS-6 `crop_no_memory`, `crop_float_no_memory`, `as6_memory`, `memory_8000`, `as6_whole_float_put_down_fails`; AS-7 0023's
  `as14_menu_mnemonics_differ` in the 24 language runs; AS-8 `crop_float_shape_fill`, `as8_free_form_float`; AS-9 0018's `as1_selection_menu`. The edge cases: 1 x 1 core
  `crop_whole_is_nothing`; a float wholly outside the picture is not tested alone (the rectangle wholly outside is, `crop_hangs_fill`); a grown paste `grown_paste`.
- Results: every task has its `Result:` with a red run, but T008 `as6_memory` (characterization: the refusal was built in T005 to T007; proved by four mutants, and `memory_8000`
  was made red for the budget mutant after its first version was blind) and T009 (nothing built: done in T005 and re-verified); T006's `as2_marked`, `as3_free_form` and
  `as5_whole` passed at their first run (stated there, proved by mutants). M1 (JOD, T005 and T010): "all works".

Findings (a review agent read the diff and the spec blind; each finding checked in the code before it was settled):
- F1 (fixed, user-visible): a floating crop that keeps the picture's size (a Select All moved off (0, 0), then Crop to Selection) was never redrawn: the core left the dirty
  rectangle empty and the window rebuilt the tiles only when the size changed, so the window kept showing the old picture while the data, the title and Undo were right.
  `/0041/as4_float_same_size` (a flipped whole-picture float moved by (10, 10)) failed first at `assert_shown` (`0xffcccccc == 0xffcdab89`); the core now marks the new picture
  dirty and the window updates when the size is the same; two mutants kill it. It was my own: T006 had removed that `else` as redundant when a mutant of it survived.
- F2 (decided by JOD, 2026-11-12, "yes"): a floating selection exactly over the whole picture is put down (REQ-0041-05), and a memory failure there follows REQ-0013-19 (the put-down's own dialog,
  the selection removed, the picture as before the lift), not REQ-0041-06 (the resize dialog, the float where it was). REQ-0041-05 and AS-6 now say so. `/0041/as6_whole_float_put_down_fails`
  (characterization: the behaviour was there; it asserts the dialog's heading and body, one dialog only, the picture as before the lift, no selection, no step). A mutant that made the path
  answer NO_MEMORY, so that the window would show the crop's dialog besides, survived and was removed as equivalent: the put-down's dialog is up and the user sees no second one.
- F3 (accepted, info): the canvas emits the selection's signals after swapping the buffers and before the window rebuilds the tiles; their handlers read no tile
  (`on_selection_changed`, `sync_history_ui`), and the marked and the floating paths do the same.
- F4 (settled by a test already in place): the view at the top-left after a size change, `as2_marked_scrolled` renders the window from a scrolled 400 % and reads both scrolls.
- G1 (the check's, test honesty, fixed in the test): `as1_key_with_the_quit_prompt` was vacuous: closing removes the selection before the prompt shows (REQ-0013-12), so the item is
  insensitive and the handler's own gate is not what stops the key. It now asserts that and says so. The same is true of the font card's field and a picture resize drag (a press on a
  handle removes the selection): those gates of REQ-0041-01 are reached only through the item's insensitivity (Known gap).
- G2 (the check's, test gap, fixed): the clause "moved, pasted, stretched or flipped" had no ui case for a stretched float, and no test had a Free-Form float over its own hole: `as4_stretched`
  (20 x 20 made 40 x 40: the pixel (i, j) the lifted (i / 2, j / 2)) and `crop_float_over_own_hole` (the hole's color, the background or #00000000, where the box is outside the
  float's shape and inside the hole); a mutant of the lay's hole kills the second.
- Drift (the code does what the spec does not say): a float larger than 10000 in a side gives the memory dialog naming that size; the crop announces nothing of its own, the end of the
  selection is announced as "Selection put down" (a float) or "Selection removed" (REQ-0022-15); the Help page's prose on the Image menu does not name Crop to Selection (its key's row is there);
  the `#:` references of the two new messages in `po/easel.pot` are approximate.
- Memory: a crop's peak is 1650 MB (first) and 2566 MB (with an earlier crop's redoable step) in the core and 1801 and 2717 MB through the window for the worst case, an
  8000 x 8000 float moved by half its sides; accepted by JOD (2026-11-11) as 0018's, no mechanism added.

Known gaps (also in `docs/phase2-plan.md`): the key with a menu open (GTK holds the popup's keys; M1 only); the gates of REQ-0041-01 for the quit prompt, the font card's field and a picture resize
drag (insensitive, G1); ui cases for a Ctrl-copy float and an ordinary pasted float (the core has both: `crop_float_equals_lay`, `crop_float_paste_skips_clear`); a float wholly outside the picture; Help's prose; the `.pot` references; where the keyboard focus is after a crop (not reported).

Verdict: one user-visible bug found and fixed (F1), two test gaps and one vacuous test fixed, F2 decided by JOD and written into the spec; `Status: implemented`.
