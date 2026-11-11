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
- REQ-0041-04 A crop shall be exactly one undo step (REQ-0002-01): with a floating selection, the lift, its moves and copies and the crop are that one step, so Undo returns to the picture before the lift (REQ-0013-11); Undo restores the previous size and every pixel exactly (also those removed) and no selection; Redo gives the same picture, with the background color used at the crop even if it has changed since (as REQ-0004-22). It counts for the memory budget (REQ-0002-16) as 4 bytes per pixel of the old picture not in the new one, plus 16 bytes per pixel the put-down of a floating selection changed, plus 64. The title gets the modified mark, as after any edit.
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
