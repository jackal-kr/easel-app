# 0041 Crop to selection

Status: accepted
Reviewed: spec-reviewer, 2026-11-11 (9 findings merged; none asked)
Depends on: 0002, 0004, 0013, 0014, 0016, 0021, 0023

## Why
To keep only part of a picture (a screenshot's window, a face in a photo) today takes a selection, Copy, New and Paste, or dragging the
resize handles twice and moving the picture. Crop to Selection does it in one command, as later versions of Paint do; it is a modern
extra (`docs/product.md`, `docs/reference/new-behaviors.md`).

## Requirements
- REQ-0041-01 The Image menu shall have "Cr_op to Selection" directly after "Stretch and _Skew…", and a selection's context menu (REQ-0013-17) the same item directly after its "Stretch and _Skew…"; both with the shortcut Ctrl+Shift+K (JOD, 2026-11-11: free in Easel and in the desktop; Ctrl+Shift+X, other Paints' key, is Easel's swap colors, REQ-0003-07), shown beside the item, in the Shortcuts window and in Help. The item and its key are sensitive only while a selection exists (marked or floating, Select or Free-Form Select, REQ-0013-04, REQ-0014-01), and do nothing where REQ-0021-01's keys do nothing (before the picture exists, with a dialog, the quit prompt, the color chooser or a menu open, a button held in the canvas area, the focus in a text field of the font card).
- REQ-0041-02 When the user crops with a marked selection, the app shall make the picture the selection's rectangle (REQ-0013-04; for a Free-Form selection, the bounding rectangle of its shape, REQ-0014-07, every pixel in it kept, the shape's outside included: JOD, 2026-11-11), its pixels exactly as they were, the rest removed.
- REQ-0041-03 When the user crops with a floating selection (moved, pasted, stretched or flipped, REQ-0013-07), the app shall make the picture the floating selection's rectangle as it is shown (JOD, 2026-11-11): each pixel is what the picture would hold if the selection were put down there (REQ-0013-11): the selection's own pixels where it has them (in both modes, REQ-0013-13; a fully transparent one stays #00000000, P7.7), elsewhere (a Free-Form shape's outside) the old picture, and where the rectangle lies outside the old picture the background color current at the crop (as REQ-0004-20 fills a grown picture; a transparent background gives transparent pixels).
- REQ-0041-04 A crop shall be exactly one undo step (REQ-0002-01): with a floating selection, the lift, its moves and copies and the crop are that one step, so Undo returns to the picture before the lift (REQ-0013-11); Undo restores the previous size and every pixel exactly (also those removed) and no selection; Redo gives the same picture, with the background color used at the crop even if it has changed since (as REQ-0004-22). It counts for the memory budget (REQ-0002-16) as 4 bytes per pixel of the old picture not in the new one, plus 12 bytes per pixel that differs from the old picture cut to the new size (the float's pixels and its hole; REQ-0002-16 allows 16), plus 64 (JOD, 2026-11-11: was "16 bytes per pixel the put-down changed"; every change log counts 12). The title gets the modified mark, as after any edit.
- REQ-0041-05 After a crop, the selection shall be gone, the tool stays as it was, the zoom stays, the view shows the new picture's top-left corner, and the status bar shows the new size (REQ-0004-09); Undo and Redo of a crop keep the scroll position as far as the size allows (REQ-0004-25). A crop with a marked selection covering the whole picture (Select All) changes nothing and adds no step (as REQ-0004-24); one with a floating selection exactly over the whole picture puts it down as one step (REQ-0013-11) with no change in size.
- REQ-0041-06 If the memory for the new picture or the crop's undo step cannot be had, then the app shall show the dialog of REQ-0004-26 ("Could Not Resize the Picture", with the new size) and change nothing: the picture, the history and the selection (marked, or floating where it was) stay as they were.
- REQ-0041-07 The pixel contract (P7): the crop copies whole pixels exactly (1); the view only magnifies (2); a floating selection's crop gives exactly the pixels it showed over the picture (3); the same picture, selection and background color give the same result (4).

## Scenarios
- AS-1 (REQ-0041-01) Given no selection, Then Image › Crop to Selection is insensitive and Ctrl+Shift+K does nothing; Given a marked selection, a floating one and a Free-Form one, Then the item is sensitive in the Image menu and in the selection's context menu, directly after Stretch and Skew, shows Ctrl+Shift+K, and the Shortcuts window and Help list it; Given a stroke held, a dialog, the quit prompt, the color chooser or a menu open, When Ctrl+Shift+K, Then nothing happens | Verified by: automated (ui)
- AS-2 (REQ-0041-02, REQ-0041-04, REQ-0041-05, REQ-0041-07) Given a 300 x 200 picture with distinct pixels and a marked selection (40, 30) to (139, 89), When Crop to Selection, Then the picture is 100 x 60, its pixel (0, 0) is the old (40, 30) and (99, 59) the old (139, 89), every pixel equal to the old one shifted, no selection, one step, the modified mark, the view at the top-left; When Undo, Then 300 x 200 and every pixel as before; When Redo, Then the 100 x 60 picture again | Verified by: automated (ui)
- AS-3 (REQ-0041-02) Given a Free-Form selection shaped as a triangle whose bounding rectangle is (10, 10) to (59, 49), When Crop to Selection, Then the picture is 50 x 40 with every pixel of that rectangle, those outside the triangle included | Verified by: automated (ui)
- AS-4 (REQ-0041-03, REQ-0041-04, REQ-0041-07) Given a 100 x 100 white picture and a 20 x 20 floating selection of black pixels moved to (90, 90), hanging 10 pixels past the right and bottom edges, When Crop to Selection, Then the picture is 20 x 20 and all black (the selection's pixels, also where they were outside the picture); one step; When Undo, Then the 100 x 100 white picture as before the lift, and no selection | Verified by: automated (ui)
- AS-5 (REQ-0041-05) Given a marked selection of the whole picture (Select All), When Crop to Selection, Then nothing changes and no step is added; Given a floating selection exactly over the whole picture, Then it is put down as one step and the size is unchanged | Verified by: automated (ui)
- AS-6 (REQ-0041-06) Given memory made too small, When Crop to Selection with a marked and with a floating selection, Then "Could Not Resize the Picture" shows with the new size, and the picture, the history and the selection (the floating one where it was) are as they were | Verified by: automated (ui)
- AS-7 (REQ-0041-01) Given each language, Then the item's mnemonic is used by no other item of the Image menu and of the selection's context menu (the mnemonic tests of 0023 and 0035) | Verified by: automated (ui)
- AS-8 (REQ-0041-03) Given a 100 x 100 green picture, red as the background color, a Free-Form triangle of blue pixels floating so that its bounding rectangle (80, 20) to (119, 59) hangs 20 pixels past the right edge, When Crop to Selection, Then the picture is 40 x 40: the triangle's pixels blue, the pixels outside the triangle and inside the old picture green, those outside both red | Verified by: automated (ui)

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

Spec gap settled (JOD, 2026-11-11): REQ-0041-04's "16 bytes per pixel the put-down changed" became "12 bytes per pixel that differs
from the old picture cut to the new size", charged by `easel_history_change_log_bytes`, which a core test checks the charge against.

## Tasks
(Opus, 2026-11-11.) Each task: its test first and seen red, then the code; the build warning-free; the feature's ui cases and the core
and io tests green; a task that adds a rule under `src/core` adds its mutants to `tests/mutants.txt` in the same task.

- [ ] T001 The crop step (REQ-0041-04; AS-2, AS-4) - done when core `history_crop_step` passes: a crop step pushed with removed pixels
  and a change log, Undo giving the old size and every pixel back (the removed and the changed), Redo giving the cut picture with the
  step's own fill even after the caller's fill changed, an asymmetric picture and an off-center rectangle hanging past two edges; the
  bytes counted (`easel_history_bytes`) equal 4 x removed + `easel_history_change_log_bytes (n)`, compared with that function and
  not with a number written in the test; the memory held (capacity of both buffers) equal to the memory counted; `step_bytes` uses
  `easel_history_change_log_bytes` for every change log (the existing history tests pass unchanged); mutants: the removed pixels'
  order, the fill on Redo, the old values on Undo, the charge.
  Result:
- [ ] T002 The marked crop (REQ-0041-02, -05, -06, -07; AS-2, AS-3, AS-5, AS-6) - done when core `crop_marked_offset` (every pixel of a
  100 x 60 cut from a 300 x 200 picture of distinct pixels), `crop_free_form_box` (a shape's whole bounding rectangle kept),
  `crop_whole_is_nothing` (NOTHING, no step) and `crop_no_memory` (max_bytes below each allocation in turn: NO_MEMORY, the picture and
  the history byte for byte as before) pass; mutants for each.
  Result:
- [ ] T003 The floating crop (REQ-0041-03, -04, -07; AS-4, AS-8) - done when core `crop_float_hanging` (AS-4: 20 x 20 black over 100 x 100
  white at (90, 90): all black, Undo to before the lift), `crop_float_shape_fill` (AS-8: blue in the triangle, green outside it in the
  old picture, red outside both), `crop_float_equals_lay` (inside the old picture every pixel equals `easel_selection_lay` of the same
  float, with copies and a hole) and `crop_redo_keeps_fill` pass; the change log holds exactly the pixels that differ from the plain
  cut (counted, and compared with the charge of T001); mutants for the fill outside, the float over it, the copies outside left out.
  Result:
- [ ] T004 Measure (engineering.md: Undo / redo at the next refresh on 1920 x 1080; 8000 x 8000 without freezing) - done when
  `perf-0041` has run three times in the real session with no Easel window open: the crop, its Undo and its Redo, marked and floating,
  on 1920 x 1080 and on 8000 x 8000, the time of each and the peak resident memory, recorded in the Plan; anything over budget
  reported to JOD before T005.
  Result:
- [ ] T005 The item, the menus and the key (REQ-0041-01; AS-1) - done when ui `/0041/as1_item_and_key` passes by the real paths: the
  two items read from the real menu models (the Image menu and the selection's context menu), each directly after Stretch and Skew with
  its label, action and Ctrl+Shift+K, and activated as a click activates them (the item's own action and target through the window's
  action group); Ctrl+Shift+K sent through the window's application shortcut controller (the shortcut whose trigger is Ctrl+Shift+K,
  activated as GTK activates it; a probe first that this path reaches the action and that a disabled action does nothing through it);
  enabled with a marked, a floating and a Free-Form selection, disabled with none, after a crop, before the picture exists, with a
  stroke held and during a resize; with a dialog, the quit prompt, the color chooser or a menu open, and with the focus in the font
  card's field, the key does nothing; the Shortcuts window lists it. Manual check: JOD presses Ctrl+Shift+K and uses both menus in the
  real session, and says where the keyboard focus is after the crop.
  Result:
- [ ] T006 The marked crop in the window (REQ-0041-02, -05; AS-2, AS-3, AS-5) - done when ui `/0041/as2_marked` (size, pixels, one
  step, the title marked, no selection, the status bar's size, the view at the top-left by rendering the window, also from a scrolled
  400 %; Undo and Redo keeping the scroll as far as the size allows), `/0041/as3_free_form` and `/0041/as5_whole` (marked: no step;
  floating exactly over the picture: one put-down step, size unchanged) pass.
  Result:
- [ ] T007 The floating crop in the window (REQ-0041-03, -04; AS-4, AS-8) - done when ui `/0041/as4_floating` (one step; Undo to before
  the lift, no selection), `/0041/as8_free_form_float` and `/0041/grown_paste` (a paste larger than the picture with Resize Canvas to
  Picture, cropped: Undo returns to the size and pixels before the paste) pass.
  Result:
- [ ] T008 Memory (REQ-0041-06, REQ-0002-17; AS-6) - done when ui `/0041/as6_memory` passes through the memory-limit hooks: a marked and a
  floating crop refused by the allocation limit show "Could Not Resize the Picture" with the new size, and the picture, the history
  and the selection (the float where it was) are as they were; and `/0041/memory_8000`, an 8000 x 8000 floating selection hanging past
  an edge: (a) with the history budget set below the step and the allocation limit above it, the crop is made and is the only undo
  step (REQ-0002-17), Undo giving the picture back exactly; (b) with the allocation limit below the change log, the dialog, and the
  picture byte for byte unchanged, the float where it was. (a) holds about 1.3 GB at its peak (the picture, the new buffer and the log
  of about 770 MB); if that is too much for the suite's machine the case is reported to JOD, not shrunk.
  Result:
- [ ] T009 Words in every language (REQ-0041-01; AS-1, AS-7) - done when "Cr_op to Selection" and "Crop to Selection" are in
  `po/easel.pot` and the eleven languages, with mnemonics the existing tests of 0023 and 0035 accept over the Image menu and the
  selection's menu (those tests run first with a clashing draft and fail), and Help lists the key in its twelve pages.
  Result:
- [ ] T010 Manual M1 (AS-1 to AS-8) - done when JOD has cropped a marked, a Free-Form and a floating selection (one hanging past the edge),
  undone and redone each, and tried Ctrl+Shift+K with no selection, in the real session; his notes in the Result.
  Result:
