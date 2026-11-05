# 0016 Clipboard

Status: implemented
Depends on: 0002 (undo), 0004 (the picture's size, REQ-0004-20 to -23; zoom), 0005 (Open and Save: formats, errors, the 10000
limit), 0011 and 0012 (pending curve, polygon, gradient), 0013 (Select, the floating selection, the menus), 0014 (Free-Form Select),
0015 (Selection handles)

## Why
A selection can be moved and stretched inside one picture, but not taken to another picture or another app, and a screenshot cannot
be brought in. XP Paint's Cut, Copy and Paste, with Copy To… and Paste From… for files, close that gap; a picture bigger than the
canvas asks first instead of being cut off (`docs/reference/xp-behaviors.md`; `docs/reference/new-behaviors.md`, Editing, as this
spec settles it).

## Requirements
In this spec "the clipboard" is the desktop's clipboard, shared with other apps. "The copy of the selection" is the picture made of
the selection's bounding rectangle: the pixels of its shape as they are (a marked selection: the picture's pixels; a floating one: its
own pixels as they float, stretched or not, also the part outside the picture), every other pixel #00000000 (fully transparent). A
selection marked with Select, and a pasted one (REQ-0016-06), has every pixel of its rectangle in its shape.

### Commands and where they are
- REQ-0016-01 The app shall offer Cut (Ctrl+X), Copy (Ctrl+C) and Paste (Ctrl+V), for Select and Free-Form Select alike: in the Edit
  submenu of the primary menu (REQ-0013-16), whose order becomes Cut, Copy, Paste; a line; Select All, Clear Selection; and in the
  first section of the selection's context menu (REQ-0013-17, JOD's mock-up of 2026-10-20). Copy To… shall be in the selection's
  context menu after Select All (the mock-up's second section) and in the primary menu after Edit ›, followed by Paste From… (the
  mock-up's primary menu: Edit ›, Copy To…, Paste From…; REQ-0005-01). (Pointer note, Text, 2026-10-26: While a text box is open, Cut, Copy and Paste act on its text, REQ-0017-19.)
- REQ-0016-02 Cut, Copy and Copy To… shall be insensitive (and their shortcuts not taken) while there is no selection. Paste shall be
  insensitive, and Ctrl+V not taken, while the clipboard holds neither a picture nor a copied file (as Files' Copy puts one); it
  follows the clipboard as it changes, also when another app changes it. Paste From… is sensitive while the picture exists. All five
  do nothing before the picture exists; their shortcuts are not taken while a dialog, a prompt, a menu or the color chooser is open
  (choosing the item in an open menu acts), and they are ignored while a mouse button is held for a stroke or a drag, as a tool
  change is (REQ-0001-09, REQ-0013-18, REQ-0014-14, REQ-0015-08). (Pointer note, Text, 2026-10-26: While a text box is open, Cut and Copy follow its selected text, Paste follows text on the clipboard, and Copy To… is insensitive, REQ-0017-19, -20.)

### Copy and Cut
- REQ-0016-03 When the user chooses Copy, the app shall put the copy of the selection on the clipboard as a picture with its
  transparency (other apps read it as PNG), replacing what the clipboard held. The clipboard holds the selection as it was at Copy:
  later moves, clears, stretches, New or Open do not change it. The picture, the selection (marked or floating, where it is), the undo
  history and the title's modified mark are unchanged.
- REQ-0016-04 When the user chooses Cut, the app shall copy the selection as REQ-0016-03 says and then clear it as Clear Selection does
  (REQ-0013-14, and REQ-0016-05 for a paste): one undo step, or none if no pixel changes. If the copy fails (REQ-0016-11) nothing is
  cleared. If the clear fails (REQ-0013-19's "Could Not Clear the Selection"), the clipboard keeps the copy and the selection is as it
  was.

### Paste
- REQ-0016-05 When the user chooses Paste and the clipboard holds a picture, or a copied file that is a picture (read by its content as
  Open reads it, REQ-0005-06), the app shall read it first (REQ-0016-15) and then: put down a floating selection and remove a marked
  one, and apply or drop what is pending as a tool change does (REQ-0013-11, REQ-0011-12, REQ-0012-07); make Select the active tool
  (`docs/design.md`: after any paste the Select tool is active); and show the pasted picture as a floating selection whose top-left
  corner is the picture pixel at the top-left corner of the view (the pixel that holds the top-left point of the part of the picture in
  view; (0, 0) whenever the picture's top-left corner is in view). A paste may run past the picture's edges; it floats there as a
  moved selection does and only its pixels inside the picture are put down (REQ-0013-11). A paste has no lift and no hole: everything
  done to it until it is put down (moves, stretches, Ctrl copies, smears, stamps) is one undo step with its put-down; if no pixel of
  the picture differs then, no step is added, no mark, and the redo steps stay (REQ-0013-11). Undo while it floats removes it, and
  what was done to it, with no step added (REQ-0013-12). Clear Selection (or Cut) on a floating paste discards it and keeps the copies
  it left; if the paste grew the picture (REQ-0016-07), the growth stays, as one undo step with the copies' put-down, and Undo takes
  the picture back to its size (JOD, 2026-10-22, as XP). If the put-down or the apply that Paste starts with fails for memory (REQ-0013-19, REQ-0011-12), its dialog shows after
  the paste, which goes on.
- REQ-0016-06 (Pointer note, Stretch and Skew, 2026-11-02: a skewed paste has a shape, REQ-0019-12.) A pasted picture shall be a rectangle selection of its size (JOD, 2026-10-22): its outline is the rectangle of
  REQ-0013-05 round it, its size is shown in the status bar (REQ-0013-06), it is tinted and pressed as a rectangle is, and its handles
  stretch it as a rectangle (0015), from the pixels as pasted (after Fit, the fitted ones), also when it was copied from a free-form
  selection. Its fully transparent pixels (alpha 0) leave the picture as it is: they are not laid over the picture while it floats and
  are not put down; every other pixel is put down as it is, partly transparent ones replacing all four channels (REQ-0013-11). So a
  free-form selection copied and pasted back changes only the pixels of its shape that are not fully transparent. A picture of only
  fully transparent pixels pastes as any other: a floating selection of its size, whose put-down changes nothing and adds no undo step,
  with no dialog.
- REQ-0016-07 If the picture to paste is wider or taller than the picture, then the app shall first ask what to do (REQ-0016-13),
  before anything is put down, applied or changed:
  - Resize Canvas to Picture makes the picture as wide as the larger of the two widths and as tall as the larger of the two heights
    (1200 × 500 pasted onto 1024 × 768 gives 1200 × 768), the new area filled as REQ-0004-20 says with the background color current
    when the answer is given, and pastes the picture as REQ-0016-05 says at (0, 0);
  - Fit Picture to Canvas makes the picture to paste as large as fits in the picture with its ratio kept, by the nearest pixel
    (REQ-0015-05): the side that limits takes the picture's size and the other is rounded to the nearest whole pixel, halves up, at
    least 1 (1920 × 1080 onto 1024 × 768 gives 1024 × 576), and pastes it as REQ-0016-05 says at (0, 0);
  - after either, the view scrolls so the picture's top-left corner is in view, the zoom kept (JOD, 2026-10-22);
  - Cancel changes nothing: a floating selection stays floating, a marked one stays marked, nothing pending is applied.
  With Resize Canvas to Picture the growth is part of the paste's step: it is one undo step with the put-down (always added, as the
  size changed), costing what REQ-0004-23 counts for the growth plus what REQ-0002-16 counts for the pixels put down (after Clear Selection or Cut,
  the copies' only, REQ-0016-05); while the paste
  floats the picture has its new size (the status bar shows it), the title has no modified mark yet and the redo steps stay; Undo while
  it floats removes the paste and the growth (the picture back to its size and pixels, no step added); if the put-down's step cannot
  get its memory, the picture is as before the paste, its size too, and REQ-0013-19's dialog shows.
- REQ-0016-13 The prompt of REQ-0016-07 shall look as in JOD's mock-up of 2026-10-22: an alert with the heading "Pasted Picture Is
  Larger Than the Canvas" (title case, as the app's other headings; JOD, 2026-10-22, the mock-up has it in sentence case), the body "The picture is W × H pixels; the canvas is W × H." (the pasted picture's size, then the
  picture's, e.g. "The picture is 7353 × 1024 pixels; the canvas is 800 × 600."), and three buttons one above the other: Resize Canvas
  to Picture (the suggested one, in the accent color, and the default: Enter), Fit Picture to Canvas, Cancel. Esc and closing it mean
  Cancel. While it is open the window waits for the answer, with no time limit; a lost focus, a window resize or a zoom change leaves
  it open, and the answer pastes at (0, 0) whatever the view is then. A quit request while it is open closes it as Cancel (as for the
  New and Open prompts, REQ-0005-02).
- REQ-0016-08 When the user chooses Paste From…, the app shall show the system file dialog titled "Paste From", in the folders and with
  the filters of Open (REQ-0005-05); nothing is put down, removed or applied before a file is chosen and read; then the file is read as
  Open reads it (REQ-0005-06, REQ-0005-17) and pasted as REQ-0016-05 and REQ-0016-07 say. Cancel changes nothing. The picture's file
  and title are not changed. While its dialog is open a quit request does nothing, as for Open's (REQ-0005-02).
- REQ-0016-15 From Paste until the clipboard's content has arrived, the app shall wait on the spot as while a file is read
  (REQ-0005-02): input waits and the busy cursor shows, and a quit request is handled after it; a picture that arrives after a quit
  request is dropped, and nothing is pasted (JOD, 2026-10-26, check F2). If nothing has arrived within 5
  seconds, the read has failed (REQ-0016-10). When the clipboard offers both a picture and copied files, the picture is pasted; with
  copied files only, the first file of the list is read (JOD, 2026-10-22).

### Copy To
- REQ-0016-09 (Pointer note, Packaging and release: the dialog also has a file type list; a chosen format decides what is written, and By extension is this rule, REQ-0024-12.) When the user chooses Copy To…, the app shall show the system save dialog titled "Copy To", suggesting `Untitled.png`
  in the folder of the picture's file or, if it has none, the Pictures folder, and write the copy of the selection (REQ-0016-03's
  picture) to the chosen file as Save As writes the picture (REQ-0005-12): the format from the extension (REQ-0005-10; `.gif` or an
  unknown extension is that requirement's error), `.png` added to a name without one and the replace question (REQ-0001-28,
  REQ-0001-42), JPEG and BMP flattened on white (REQ-0005-11), JPEG at the quality of REQ-0005-14, the memory checked before encoding
  (REQ-0005-18). The picture's file, title, modified mark and undo history are unchanged, and the selection stays as it is (marked or
  floating). Cancel changes nothing. While its dialog is open a quit request behaves as for Save As (REQ-0001-43).

### The canvas menu
- REQ-0016-14 When the user presses the secondary button with Select or Free-Form Select in the canvas area while no button is held,
  not on the selection (a pixel of its shape; a pasted selection's fully transparent pixels are on it, REQ-0016-06) and not on a
  handle (the selection's, REQ-0015-02, or the picture's, REQ-0004-16), the app shall open a context menu at the pointer, as in JOD's
  mock-up of 2026-10-22: a GNOME popover menu with, in its first section, Paste (Ctrl+V), Paste From…, Select All (Ctrl+A); in its
  second, Zoom In, Zoom Out, Normal Size (Ctrl+PageUp, as the app and XP have it; the mock-up shows Ctrl+Page Down by mistake, JOD,
  2026-10-22). Each item is sensitive as its counterpart elsewhere is (REQ-0016-02, REQ-0004-02) and acts as it does: Paste lands at
  the top-left in view, not at the pointer; a zoom keeps the center of the canvas area still (REQ-0004-06). Opening and closing the
  menu puts nothing down and changes nothing. (This replaces REQ-0013-17's "a secondary press elsewhere with Select does nothing".)
  The mock-up's third section holds Clear Image (Shift+Ctrl+N) (JOD, 2026-10-22); the command comes with Image Transforms, which
  adds that section, as REQ-0013-17 leaves later features' items to them; this feature shows the first two sections. (Pointer note, Image Transforms, 2026-10-27: the third section with Clear Image (Shift+Ctrl+N) is added, REQ-0018-02.)

### Failures
- REQ-0016-10 If the clipboard's picture or the file to paste cannot be read, is not a picture, or is bigger than 10000 pixels in
  width or height, then the app shall show an error dialog and change nothing (nothing put down, removed or applied): for the
  clipboard's picture the heading "Could Not Paste" with the body "The clipboard's picture cannot be read.", "This is not a picture
  the app can open." or "The picture is bigger than 10000 pixels in width or height."; for a file (Paste From…, or a copied file) the
  heading "Could Not Paste “<name>”" with the body of REQ-0005-09 that fits (a folder: "The file cannot be read.").
- REQ-0016-11 If the memory to make the copy of the selection cannot be had, then the app shall show the error dialog "Could Not Copy
  the Selection", "There is not enough memory to copy this selection.", and leave the clipboard, the picture and the selection as they
  were (for Cut, nothing is cleared; for Copy To…, nothing is written). If the memory to paste (the pasted picture, its fitted size,
  or the picture's growth) cannot be had, the app shall show "Could Not Paste", "There is not enough memory to paste this picture.",
  and change nothing. Each is checked for the one thing being made (as REQ-0013-19 and REQ-0015-09).
- REQ-0016-12 If writing the file of Copy To… fails, the app shall show the save error of REQ-0001-31 (heading "Could Not Save
  “<name>”", the system's reason) and leave any existing file at that path with its old content; the picture and the selection are
  unchanged.

## Scenarios
The picture of the scenarios, unless said otherwise, is that of 0013: 21 × 11 pixels of #FFFFFFFF (white) with a red block
#FF0000FF at the columns 2 to 4 of the rows 2 and 3; the foreground #000000FF, the background #FFFF00FF (yellow); 100 %; Opaque mode.
"Marked" is the selection (1, 1) to (5, 4); "the triangle" is 0014's (the path (1, 1), (7, 1), (1, 7); its shape the pixels
i + j ≤ 4 of its 5 × 5 box at (1, 1), 15 pixels).

- AS-1 (REQ-0016-01, REQ-0016-02) Given no selection, Then the Edit submenu shows Cut, Copy, Paste, a line, Select All, Clear Selection, Cut and Copy insensitive, and the primary menu shows Copy To… (insensitive) and Paste From… after Edit ›; Ctrl+X and Ctrl+C are not taken; Given marked, Then the selection's context menu shows Cut, Copy, Paste in its first section and Copy To… after Select All, all sensitive but Paste while the clipboard holds no picture; Given the clipboard holds only text, Then Paste is insensitive and Ctrl+V not taken; When another app copies a picture, Then Paste becomes sensitive without any action in Easel; before the picture exists all five do nothing | Verified by: automated (ui) + manual M1 (the menus' look, another app's copy)
- AS-2 (REQ-0016-03) Given marked, When the user presses Ctrl+C, Then the clipboard holds a 5 × 4 picture equal to the picture's columns 1 to 5 of the rows 1 to 4 (red at its columns 1 to 3 of its rows 1 and 2), the picture, the selection and the history are unchanged; When the user then presses Delete and Ctrl+V, Then the paste shows the copied pixels (red), not the cleared ones; Given the triangle floating, When Copy, Then the clipboard holds 5 × 5 with the 15 pixels of the shape and the other 10 #00000000 | Verified by: automated (ui)
- AS-3 (REQ-0016-04) Given marked, When Ctrl+X, Then the clipboard holds AS-2's picture and the columns 1 to 5 of the rows 1 to 4 are yellow (Opaque), one undo step; Undo restores them and the clipboard keeps its picture; Given the same in Transparent mode, Then they are #00000000; Given the clear's step refused for memory, Then "Could Not Clear the Selection", the clipboard holds the copy and the selection is as it was | Verified by: automated (ui)
- AS-4 (REQ-0016-05, REQ-0016-06) Given AS-2's copy on the clipboard and the Pencil active, When Ctrl+V, Then Select is the active tool, a 5 × 4 floating selection at (0, 0) shows the copy, "5 × 4" in the status bar, the picture unchanged; When the user drags it from (1, 1) to (11, 6) and presses Esc, Then it is put down at (10, 5) to (14, 8), red at the columns 11 to 13 of the rows 6 and 7: 6 pixels changed, one step; Given the triangle copied, When Ctrl+V and at once Esc (put down at (0, 0)), Then 4 pixels changed, (1, 1), (2, 1), (3, 1) and (1, 2) now red, and the red pixels (2, 3), (3, 2), (3, 3), (4, 2) and (4, 3), under the copy's transparent pixels, stay red; the pasted selection's outline was the 5 × 5 rectangle | Verified by: automated (ui)
- AS-5 (REQ-0016-05) Given a 300 × 200 picture at 400 % scrolled so the top-left pixel in view is (100, 50) (partly shown), When pasting AS-2's copy, Then the floating selection's top-left is (100, 50) and it is drawn there; at 100 % unscrolled it is (0, 0); Given a 250 × 180 picture on the clipboard and the 300 × 200 picture scrolled so (100, 50) is the top-left in view, When Paste, Then no prompt, the paste floats at (100, 50) running past the right and bottom edges, and after Esc only its 200 × 150 pixels inside the picture are put down | Verified by: automated (ui, rendering where it is drawn)
- AS-6 (REQ-0016-05) Given a pasted selection floating, When Undo, Then the picture is as before, no selection, no step added, the redo steps kept; Given a floating selection moved by (3, 0), When Ctrl+V, Then it is first put down (one step), then the paste floats; When Ctrl+V again, Then the first paste is put down at (0, 0) as a step of its own and the second floats; Given a curve pending, When Ctrl+V, Then the curve is applied (one step) and Select is active; Given AS-2's copy pasted and moved to (1, 1), where its pixels came from, When Esc, Then no pixel differs and no step is added | Verified by: automated (ui)
- AS-7 (REQ-0016-07) Given a 30 × 20 picture on the clipboard, When Paste, Then the prompt asks first and nothing is put down yet; Fit Picture to Canvas pastes a 17 × 11 selection at (0, 0) (30 × 20 scaled by 11 / 20, 16.5 rounded up); Resize Canvas to Picture makes the picture 30 × 20 and pastes 30 × 20 at (0, 0), and Undo before the put-down gives back 21 × 11, every pixel, no step added and the redo steps kept; after Esc one Undo gives back 21 × 11 and every pixel; Cancel changes nothing, a floating selection stays floating and a pending curve stays pending; Given a 25 × 11 picture, Fit gives 21 × 9; Given 22 × 5 (wider only), Resize gives 22 × 11 and, after Esc, the column 21 of the rows 5 to 10 is yellow, and Fit gives 21 × 5; Given 21 × 11, no prompt; 22 × 11, the prompt; Given 10000 × 1, Fit gives 21 × 1; Given a 1920 × 1080 screenshot on a 1024 × 768 picture, Fit gives 1024 × 576 and Resize 1920 × 1080; Given the view scrolled away from (0, 0) at 400 %, After Fit or Resize the view shows the picture's top-left corner, still at 400 % | Verified by: automated (core, the sizes) + automated (ui)
- AS-8 (REQ-0016-08, REQ-0016-10, REQ-0016-15) Given a PNG file of 3 × 2 pixels, When Paste From… chooses it, Then it floats at (0, 0), 3 × 2, and the title and file are unchanged; Given a floating selection, When Paste From… is cancelled, Then it still floats; Given a text file named `a.png`, Then "Could Not Paste “a.png”" with "This is not a picture the app can open." and nothing changes; Given a PNG copied in Files, When Ctrl+V, Then it pastes; Given a copied text file `n.txt`, Then "Could Not Paste “n.txt”"; Given a clipboard offering a picture and a file, Then the picture is pasted; Given a 10001 × 1 picture on the clipboard, Then "Could Not Paste" with the 10000 body | Verified by: automated (ui) + manual M2 (the dialog, Files' copy)
- AS-9 (REQ-0016-09, REQ-0016-12) Given the triangle marked, When Copy To… writes `t.png`, Then `t.png` is 5 × 5 with the 15 shape pixels and 10 transparent ones, the picture keeps its file, title and modified state; to `t.jpg` the 10 are white; into a read-only folder the save error shows and nothing else changes | Verified by: automated (io, the file) + automated (ui) + manual M2
- AS-10 (REQ-0016-11) Given memory made too small for the copy, When Ctrl+X, Then "Could Not Copy the Selection" shows, the clipboard keeps what it held, nothing is cleared; When Copy To…, Then the same dialog and no file; Given memory too small for a paste, Then "Could Not Paste" with the memory body, and the floating selection that was there still floats; the same for the growth of Resize and for Fit | Verified by: automated (ui)
- AS-11 (REQ-0016-02) Given a pencil stroke held, When Ctrl+V, Then nothing happens and the stroke goes on; Given a rectangle dragged out with Select, When Ctrl+C, Then nothing is copied; Given the Attributes dialog open, When Ctrl+C, Then the app copies nothing (a field of the dialog may copy its own text) | Verified by: automated (ui)
- AS-12 (REQ-0016-13) Given a 30 × 20 picture on the clipboard, When Paste, Then the prompt's heading is "Pasted Picture Is Larger Than the Canvas", its body "The picture is 30 × 20 pixels; the canvas is 21 × 11.", its buttons from the top Resize Canvas to Picture (suggested), Fit Picture to Canvas, Cancel; Enter gives Resize; Esc and closing give Cancel; with the prompt open, a window resize leaves it open; a quit request closes it as Cancel; the window stays | Verified by: automated (ui) + manual M1 (its look)
- AS-13 (REQ-0016-14) Given no selection with Select, When the secondary button is pressed at (15, 8), Then the canvas menu opens there: Paste, Paste From…, Select All; Zoom In, Zoom Out, Normal Size (Ctrl+PageUp), Normal Size insensitive at 100 %; at 800 % Zoom In insensitive; Given marked and floating, When the secondary button is pressed at (15, 8), Then the canvas menu opens and closing it leaves the selection floating where it was; Given the triangle marked, When pressed at (5, 5) (in its box, outside its shape), Then the canvas menu opens; When pressed on a selection handle or a picture resize handle, or during a held primary drag, Then no menu (the handle and drag rules apply); When Paste is chosen from the menu, Then it lands at the top-left in view | Verified by: automated (ui) + manual M1
- AS-15 (REQ-0016-05, REQ-0016-07) Given a 30 × 20 picture on the clipboard, When Paste, Resize Canvas to Picture, then Delete, Then the picture is 30 × 20 with no pasted pixel (the columns 0 to 20 of the rows 0 to 10 as before, the new pixels yellow), one undo step; Undo gives back 21 × 11 and every pixel; Given the same paste moved by Ctrl-drag by (2, 0) before the Delete (a copy left at (0, 0)), Then after Delete the copy is in the picture at (0, 0), the picture 30 × 20, one step, and Undo gives back 21 × 11 and every pixel | Verified by: automated (ui)
- AS-14 (REQ-0016-15) Given a clipboard owner that never answers, When Ctrl+V, Then the window waits with the busy cursor and after 5 s "Could Not Paste" with "The clipboard's picture cannot be read." shows and nothing has changed; Ctrl+V pressed three times quickly pastes once per press, one after the other; Given an owner that answers after a quit request made during the wait, Then nothing is pasted and the window closes, or asks to save a modified picture. | Verified by: automated (ui, a test clipboard owner)

## Edge cases
- The clipboard changes while the oversize prompt is open: the answer pastes the picture read when Paste was chosen.
- Paste of the selection's own copy while it floats: the floating one is put down first, then the copy floats at the top-left in view.
- A copy of a floating selection partly outside the picture holds all its pixels, those outside the picture too (REQ-0016-03).
- A copy of a selection whose pixels are all #00000000 (a Transparent-mode hole, say) is a fully transparent picture; it pastes as
  REQ-0016-06's last sentence says.
- Fit may make a picture whose only opaque pixels were skipped by the nearest pixel: it pastes as a fully transparent one.
- A paste larger than the picture in both directions on a picture of exactly its size: no prompt (not wider, not taller).
- Easel quits: whether the clipboard still holds the copy afterwards is the desktop's matter (a clipboard manager may keep it).

## Out of scope
- The Pan tool's right-click menu (Paste, Paste From…, Select All, zoom, Clear Image): Window Extras.
- Text in the clipboard (pasting text into a text box): Text.
- Ctrl+Insert, Shift+Insert and Shift+Delete as alternative shortcuts.
- Dragging pictures in from other apps or Files (drag and drop).
- A pasted picture's shape from its alpha channel (its fully transparent pixels outside the selection, for the outline, the tint and
  presses): a possible later improvement (JOD, 2026-10-22); here a paste is a rectangle (REQ-0016-06).

## Open questions
None.

## Plan
### Approach
The core gets three pure pieces: the copy of a selection as a picture (REQ-0016-03), a floating selection made from a pasted
picture (no lift, no hole; its fully transparent pixels skipped where the float is laid, which is also where it is put down and
stamped, REQ-0016-06), and the fit size (REQ-0016-07); the history gets one new step kind, a growth of the picture joined to the
pixels of a put-down (REQ-0016-07). The io layer opens a picture from bytes as it opens one from a file (the same sniff, size and
memory checks, REQ-0016-10) and encodes a picture to PNG bytes. The window owns the five actions, the clipboard and the menus; the
canvas takes a finished picture to paste (`easel_canvas_paste`) and opens the canvas menu by a signal, as it does the selection's.

- **Copy** puts on the clipboard a content provider of our own that holds the copy of the selection as a picture, made at Copy
  (the snapshot of REQ-0016-03), and offers `image/png`: the PNG is encoded only when a reader asks for it (another app). The
  encoding runs on a worker thread (a `GTask` in the provider's `write_mime_type_async`), never on the main thread, so Easel's window
  stays responsive while another app pastes a big copy: that app's read waits until the last byte is written to the stream GDK hands
  over (its own wait, as for any slow owner); Easel's input does not wait. The thread reads only the snapshot, which nothing changes
  (a new Copy makes a new snapshot; the task keeps a reference to the old one until it ends). Its memory while it runs: gdk-pixbuf's
  copy of the pixels (4 bytes a pixel) and the encoder's small buffers, the PNG being written to the stream as it is made, not held
  whole; the snapshot is already held (4 bytes a pixel). Easel's own
  Paste of its own copy takes the held picture directly (`gdk_clipboard_is_local` and our provider), with no encode and decode, so the
  pixels come back exact, partly transparent ones too.
- **Paste** from another app reads the first image MIME type the clipboard offers (`image/png` first, then the other formats Open
  reads), else the file list (`GDK_TYPE_FILE_LIST`, its first file, REQ-0016-15), then opens it by its content (io). The read waits on
  the spot: input blocked and the busy cursor as for Open (REQ-0005-02), a nested main loop that ends at the answer or at 5 s.
- **The clipboard seam** (JOD, 2026-10-22, after T005's first run: in the isolated session no window gets the focus, and the
  compositor drops a clipboard set from a window without it, so no ui test can hold the desktop's clipboard). The window talks to the
  clipboard only through `EaselClipboard`, an interface that mirrors `GdkClipboard`'s: the formats offered, an asynchronous read of a
  picture (a stream of one of the MIME types asked for, with the type chosen) and of a list of files, setting content with a lazy
  `GdkContentProvider`, "is the content ours" (with our provider back), the store at quit, and a "changed" notification. In the app it
  is `EaselGdkClipboard`, a wrapper over the display's `GdkClipboard` that forwards each call and the signal and has no logic of its
  own. The ui tests give the window `EaselMemoryClipboard` (in `tests/ui`): its content is set by the test as another app would set the
  desktop's (text, a picture as PNG bytes, files, an owner that never answers or answers late) or by the window's own provider, and a
  read of the window's own content goes through that provider's `write_mime_type_async` and `get_value`, as GDK's own read does.
  Automated through the seam: Easel's logic (sensitivity, what Copy holds and offers, the paste flow, the 5 s limit, the choice between a
  picture and files, the errors, the store asked at quit). Not automated: what only the desktop does (the compositor taking our content,
  another app reading our PNG, Files' copy, a clipboard manager keeping it); that is checked by `probe-clipboard` in the real session
  and by the manual steps M1 and M2 of the spec (T016). No spec wording changes: AS-1 and AS-14 already name a manual step or a test
  owner.
- **The oversize prompt** is an `AdwAlertDialog` (REQ-0016-13) asked before anything is put down; its answer goes on.
- **The paste itself** (canvas): put down or remove the selection and apply what is pending as a tool change does (the window's
  one path for a tool change, then Select), grow the picture if asked (keeping the picture as it was, so that Undo while it floats and
  a failed put-down give it back without allocating), make the float at the top-left pixel in view (or (0, 0) and scroll there after
  the prompt).
- **Clipboard store at quit** (JOD, 2026-10-22; the spec stays as written, its edge case "Easel quits"): when the window closes and
  the clipboard is ours (`gdk_clipboard_is_local`), the app asks the desktop's clipboard manager to keep it
  (`gdk_clipboard_store_async`), waits on the spot for the answer for at most 2 seconds, then quits whatever the answer. When no
  manager keeps it (GNOME on Wayland has none by default: the call fails at once or the time runs out), nothing is shown and the copy
  is gone after the quit, as the edge case says; a later version may run without a window for that, not planned. Known note (Check
  2026-10-26, JOD: no change): input is not blocked during the store's wait, so with a clipboard manager that answers slowly a stroke
  made in those 2 seconds would be lost with the window, which found the picture unmodified before the wait; GDK on Wayland answers
  the store at once (no manager to ask), so there the wait is not felt.

### Files
- `src/core/selection.{c,h}`: `easel_selection_copy_image`, `easel_selection_paste`, the float's `skip_clear` (the alpha-0 rule in
  `lay_float_at`), `easel_fit_size`.
- `src/core/history.{c,h}`: `easel_history_push_grow`.
- `src/io/picture-file.{c,h}`: `easel_picture_open_bytes`, `easel_picture_encode`; `easel_picture_save` writes what
  `easel_picture_encode` makes.
- `src/ui/window.{c,h}`, `window.ui`: the actions `win.cut`, `win.copy`, `win.paste`, `win.copy-to`, `win.paste-from`, their
  accelerators, the menus (`selection_menu`'s sections, the Edit submenu, the primary menu, the new `canvas_menu`), the clipboard
  provider (`EaselClipboardPicture`, a `GdkContentProvider` subclass in `src/ui/clipboard.{c,h}`), the read with its time limit, the
  prompt, Paste From and Copy To dialogs, the store at quit.
- `src/ui/clipboard.{c,h}`: the `EaselClipboard` interface and `EaselGdkClipboard`, the wrapper over `GdkClipboard` (the provider
  `EaselClipboardPicture` lives here too).
- `tests/ui/memory-clipboard.{c,h}`: `EaselMemoryClipboard`, the test's clipboard; `tests/probe/probe-clipboard.c`: the probe for the
  real session, built by `meson` but not a test (run by hand, as `perf-*`).
- `src/ui/canvas.{c,h}`: `easel_canvas_paste`, the canvas-menu signal, the picture kept while a grown paste floats, test hooks.
- Tests: `tests/core/test-selection.c`, `tests/core/test-history.c`, `tests/io/test-picture-file.c`, `tests/ui/test-window.c`
  (`/0016/`, and the cases the menus move, below), `tests/perf/perf-0016.c`, `tests/meson.build` (ui-0016, perf-0016),
  `tests/mutants.txt`.

### Key interfaces
- `EaselImage *easel_selection_copy_image (const EaselImage *img, const EaselFloat *f, int x, int y, int w, int h, const uint8_t
  *mask, size_t max_bytes)`: the copy of a marked selection (img, rectangle, mask) or, with f, of the floating one; NULL when its
  memory cannot be had (REQ-0016-11).
- `EaselSelectionResult easel_selection_paste (EaselFloat *f, const EaselImage *pic, int x, int y, size_t max_bytes)`: f (empty)
  becomes a float of pic's pixels at (x, y), no hole, `skip_clear` set; NO_MEMORY with f empty.
- `void easel_fit_size (int w, int h, int W, int H, int *nw, int *nh)` (REQ-0016-07's rule; pure).
- `bool easel_history_push_grow (EaselHistory *h, int old_w, int old_h, int new_w, int new_h, uint32_t fill, EaselChange *changes,
  size_t n)`: one step that crops back to old_w x old_h and writes the changes' old values on undo, and grows with fill and writes the
  new values on redo; counts 64 bytes plus 12 per change (a growth removes no pixel, REQ-0004-23); added even with no change.
  `EaselSelectionResult easel_selection_put_down_grown (f, with_float, img, history, old_w, old_h, fill, max_bytes)` puts a grown paste
  down as that step.
- `EaselOpenResult easel_picture_open_bytes (GBytes *bytes, EaselImage **img, int *width, int *height)`; `GBytes
  *easel_picture_encode (const EaselImage *img, EaselFormat format, int jpeg_quality, GError **error)`; `GBytes
  *easel_picture_encode_fast_png (const EaselImage *img, GError **error)` (the clipboard's PNG, compression level 1: 113 ms against
  2189 ms for a 3178 x 3099 photo; M1 with JS Paint).
- `EaselClipboard` (a GObject interface; vfuncs as GDK's): `get_formats`, `read_async`/`read_finish` (MIME types in, a stream and the
  chosen type out), `read_files_async`/`read_files_finish` (a `GSList` of `GFile`), `set_content` (a `GdkContentProvider`), `is_local`,
  `get_content`, `store_async`/`store_finish`, signal `changed`. `EaselClipboardPicture` (a provider of the copy, `image/png` encoded on
  a worker thread and written whole) in `src/ui/clipboard.c`. Hooks `easel_window_set_clipboard (win, clipboard)` (the tests' memory
  clipboard; the window uses `EaselGdkClipboard` otherwise), `easel_window_set_clipboard_timeout (win, ms)` (the 5 s),
  `easel_window_paste_from_file (win, file)` and `easel_window_copy_to_chosen (win, path)` (the choice made in a dialog),
  `easel_window_get_canvas_menu (win)`.
- Canvas: `EaselImage *easel_canvas_copy_selection (self)`, `void easel_canvas_top_left_in_view (self, &x, &y)`, `gboolean
  easel_canvas_paste (self, pic, x, y)` (takes pic; FALSE with nothing changed), `gboolean easel_canvas_paste_grown (self, pic, width,
  height, fill)` (the picture grown first, the old pixels kept as `paste_base` for Undo and a failed put-down),
  `easel_canvas_can_paste (self, pic_w, pic_h, grown_w, grown_h)` (the memory looked for before the tool changes),
  `easel_canvas_get_selection_limit (self)`; signals `canvas-menu (x, y)` and `picture-replaced`.

### Where it is put down, removed or cancelled
A paste is a floating selection, so the functions of 0013 and 0015 apply: `finish_selection` puts it down; for a grown paste it pushes
`easel_history_push_grow` instead of a plain step (always, as the size changed) and frees the kept picture; `cancel_float` (Undo while
it floats) also swaps the kept picture back; a failed put-down's step does the same and shows REQ-0013-19's dialog. Clear Selection on
a paste discards it as on any float lifted with Ctrl (no hole); on a grown paste it pushes the joint step with the copies' pixels
(the growth stays, REQ-0016-05) and frees the kept picture. Copy and Cut never put anything down; Cut clears through Clear
Selection's own function.

### Every reader of what changes
- **The float's pixels** (`lay_float_at`): the tiles' preview, the trail of a smear, the stamps, the put-down (all through the lay),
  `append_float_outside` (draws pixels outside the picture; an alpha-0 pixel there draws nothing either way), `easel_selection_bounds`
  (unchanged: a skipped pixel still bounds the float). Stretch keeps `skip_clear` (`stretch_to`).
- **The picture's size while a grown paste floats**: the canvas's size and scroll range, the status bar's picture size, the picture
  handles, `to_pixel`, the grid, the tiles (all as after a resize, `easel_canvas_update`), the title (no mark: no step yet), Save
  while it floats (puts it down first, REQ-0013-11).
- **Waiting on the spot**: the window's `loading` flag is Open's; the clipboard read gets its own `reading_clipboard`, and the one
  question every gate asks (`input_waits`) answers for both: the shortcuts, the tool change, the quit request, the canvas's input.
- **The secondary press with Select or Free-Form Select**: `easel_canvas_press_timed` (3473, 3501) now emits `canvas-menu` where it
  did nothing; the press on a handle and during a drag keep their branches before it.
- **The menus**: `selection_menu` gets a new first section, so its "Clear Selection, Select All" moves to the second; the Edit submenu
  gets a first section; the primary menu's Edit section gets Copy To… and Paste From….
- **Tests the menus change, moved in T005 (the menus' content) and T006 (the secondary presses)**: 0013 `as11_context_menu` (its section 0 becomes Cut, Copy, Paste; the secondary press
  at (40, 40) now opens the canvas menu), `as10_select_all_edit_menu` (the Edit submenu's sections), `as12_drags_that_do_not_finish`
  (the secondary press that cancels a drag opens no canvas menu either); 0014 `as17_free_context_menu` (the presses at (5, 5) and
  (40, 40) now open the canvas menu), `as11_traces_that_do_not_finish` (no menu after the cancelling press); 0015
  `as8_selection_handle_wins` (no canvas menu on a handle), `as9_stretch_that_does_not_finish` (no menu after the cancelling press);
  0005 `as1_file_menu` (the Edit section's items). T006 greps every secondary press of the ui tests made with Select or Free-Form
  Select (55 presses with button 3 in all) and asserts the canvas menu's state where the press is not on the selection.

### What grows
- The clipboard provider holds one copy of the selection (4 bytes per pixel of its rectangle) until another copy or another app
  replaces it; the PNG it encodes on a request is freed after the write.
- A paste holds its pixels (4 bytes a pixel) as any float; a Fit paste only the fitted ones; a grown paste also keeps the picture as it
  was (4 bytes a pixel of the old size) until it is put down or undone.
- The joint step stores 12 bytes per changed pixel plus 64 (REQ-0004-23, REQ-0002-16).
- Each allocation is checked on its own against its limit (the selection's for the copy and the paste, the history's budget for
  the step), as the spec says (REQ-0016-11).

### Constitution check
P4: the nearest pixel for Fit. P5: no cache; the PNG is encoded only when another app asks for it, so a Copy costs a copy of its pixels;
this is deferring work, not a cache. The worker thread of the encode is P5's "until a measurement justifies them": T009 records the
encode time; if 8000 x 8000 encodes within one display frame, the thread is dropped and the encode runs on the main thread. Core pure, io
display-free, ui only turns input into calls.

### Risks
- **The isolated session holds no clipboard** (found in T005's first run, 2026-10-22): its windows never become active, and the
  compositor drops a set from an unfocused window (`gdk_clipboard_is_local` false again, no formats). Hence the seam above.
- **GDK's behaviour the code relies on, probed by hand in the real session (`probe-clipboard`, T007, replacing the probes once
  planned there):** after a set, the content is ours (`is_local`, `get_content` gives our provider) and stays ours; the formats offered
  for our provider (`image/png`) and for a texture another app sets; a read of our own content goes through our provider's
  `write_mime_type_async`; a file list (`GDK_TYPE_FILE_LIST`) read back with `read_value_async`; `store_async` with no clipboard
  manager (does it fail at once, or wait). Its output goes into this Plan before the code relies on it; the memory clipboard is made to
  do what the probe saw. The fallback for the store is the 2 s limit alone.
  Run 2026-10-22 (T007; real session, GNOME 50 on Wayland, load 0.2): the probe's window active at once; after a set the content is
  ours (`is_local` 1, `get_content` our provider) and still ours a second later; the formats GDK reports for our PNG provider are
  `GdkTexture GdkPixbuf image/png` (it adds the types its deserializers make from the MIME type); a texture's are `GdkTexture GdkPixbuf`
  and every image type gdk-pixbuf writes (`image/png`, `image/tiff`, `image/jpeg`, `image/webp`, `image/bmp`, ...); a file list's are
  `GdkFileList GFile gchararray text/plain;charset=utf-8 text/uri-list` and the portal's two (so copied files also offer text); a read of
  our own content gives a stream of `image/png` through our provider's `write_mime_type_async` (its writes went from 1 to 2: something
  on the desktop had read our content once right after the set, so a big copy may be encoded at Copy whether or not anyone pastes);
  the file list reads back as one `GFile`; `store_async` is refused at once ("This clipboard cannot store data.": no clipboard manager).
  Two lessons for the code: a provider writes without blocking (the probe's first version wrote synchronously and the read hung: the
  reader of a local read is the same main loop), and the memory clipboard reports formats as GDK does (the serialized MIME types and the
  deserialized types of the provider's formats).
- **The nested main loop** of the wait: events that arrive while it runs (a key, a close request) must wait; the gate is the same
  `input_waits` Open uses, and T012's test presses keys during a slow owner.
- **Speed:** no requirement budgets a paste; JOD's rule for 0016 is one display frame from Ctrl+V (or the prompt's answer) to the
  painted frame, else the numbers go to him before any fix. Reading and decoding an 8000 x 8000 PNG from another app takes a second
  or more; measured in T009 (paste, Copy, Cut and the encode) and T011 (Fit, Resize) right after the code that produces the cost.
- **Memory, in numbers:** an 8000 x 8000 paste from another app holds the PNG bytes (up to about 256 MB), the decoded picture (256
  MB) and the float (256 MB) at the peak; a grown paste adds the kept picture (up to 256 MB); Fit holds the decoded picture and the
  fitted one. perf-0016 records the peak.
- **Clear on a grown paste** (a spec gap found while planning, settled by JOD, 2026-10-22, REQ-0016-05): the growth stays, as one
  step with the copies' put-down; `clear_selection` on a grown paste pushes `easel_history_push_grow` with the copies' pixels only.

### Measurements
`perf-0016` (T009), real session, no Easel window open, the pointer parked outside its window (JOD), load average 0.3, 60 Hz display (a
refresh is 16.7 ms), the view at the fit zoom (as the benchmark opens it), three runs (2026-10-23). Each time is from the command to the
painted frame; the encode is timed apart (the worker thread's run, which the window does not wait for); the pasted pictures are of one
color, so their PNG is small (0.3 MB at 8000 x 8000) and quick to decode, which a photo would not be.

| Selection / picture | Copy | Paste of Easel's own copy | Paste of another app's PNG | Cut | PNG encode (thread) | Peak memory (before) |
|---|---|---|---|---|---|---|
| 2000 x 2000 | 14.2 / 16.4 / 14.0 ms | 44.0 / 34.6 / 42.6 ms | 50.9 / 41.2 / 40.5 ms | 18.6 / 15.0 / 15.2 ms | 48.9 / 34.2 / 44.5 ms | 283 MB (127) |
| 8000 x 8000 | 111.6 / 92.3 / 90.1 ms | 385.6 / 371.9 / 365.9 ms | 363.7 / 353.8 / 358.5 ms | 132.0 / 148.8 / 133.1 ms | 546.1 / 567.8 / 527.7 ms | 2363 MB (498) |

Over one display frame: at 2000 x 2000 both pastes (about 2 to 3 frames) and the Cut once; at 8000 x 8000 every case (Copy 6 frames,
Cut 8, the pastes about 22). Reported to JOD before any fix (his rule for 0016); T010 and later wait for his decision.

JOD's two follow-ups (2026-10-23, same conditions, three runs each, load 0.1 to 0.7): `perf-0016 zoom100` (8000 x 8000 at 100 %) and
`perf-0016 photo` (another app's photo-like PNG, smooth light and color with grain, pasted at 100 %, the decode alone timed apart):

| Case | Copy | Paste of Easel's own copy | Paste of another app's PNG | Cut | PNG encode (thread) | Peak memory (before) |
|---|---|---|---|---|---|---|
| 8000 x 8000, one color, 100 % | 123.2 / 94.2 / 87.5 ms | 137.3 / 107.9 / 111.6 ms | 271.8 / 271.6 / 259.0 ms | 158.7 / 155.9 / 161.3 ms | 606.6 / 600.7 / 579.2 ms | 1580 MB (356) |

| Photo-like PNG | PNG size | Paste of another app's PNG | of which the decode alone | Peak memory (before) |
|---|---|---|---|---|
| 2000 x 2000, 100 % | 5.0 MB | 56.8 / 58.8 / 55.4 ms | 50.1 / 56.5 / 47.2 ms | 175 MB (136) |
| 8000 x 8000, 100 % | 63.7 MB | 684.6 / 657.2 / 661.9 ms | 540.7 / 541.8 / 553.5 ms | 1150 MB (1150: the photo was made and encoded before the baseline) |

Reading: at 100 % the paste of Easel's own copy at 8000 x 8000 falls from about 370 ms to 108 to 137 ms, near the Copy (88 to 123 ms):
drawing at the fit zoom was its cost, the ruling JOD named (0015's accepted fit-zoom case). A photo's paste is the decode's: 47 to 57 of
its 55 to 59 ms at 2000 x 2000, 541 to 554 of its 657 to 685 ms at 8000 x 8000 (gdk-pixbuf's PNG decode, done on the spot as REQ-0016-15
says). Still over one frame at 100 %: Copy and Cut at 8000 x 8000, the own paste at 8000 x 8000, and every photo paste. Sent to JOD.

Accepted slow cases (JOD, 2026-10-23): a photo's paste from another app, about 50 ms at 2000 x 2000 and about 550 ms at 8000 x 8000,
decode-bound in gdk-pixbuf, 1150 MB peak at 8000 x 8000, with the same wait on the spot as Open (REQ-0016-15); the own paste at the fit
zoom falls under 0015's fit-zoom ruling. Copy and Cut get a copy by rows (JOD's decision), measured again below.

After the copy by rows (2026-10-23, `perf-0016 zoom100`, both sizes at 100 %, three runs, load 0.23, the pointer parked):

| Selection | Copy | Paste of Easel's own copy | Cut | Peak memory (before) |
|---|---|---|---|---|
| 2000 x 2000 | 7.1 / 20.1 / 11.2 ms | 28.2 / 19.4 / 25.3 ms | 16.9 / 17.2 / 16.1 ms | 232 MB (127) |
| 8000 x 8000 | 73.5 / 59.0 / 69.1 ms (was 88 to 123) | 132.4 / 110.7 / 111.2 ms | 130.1 / 131.9 / 128.5 ms (was 156 to 161) | 1622 MB (402) |

Accepted slow cases with these numbers (JOD, 2026-10-23: moving about 256 MB takes some tens of ms): at 8000 x 8000 Copy (about 70 ms),
Cut (about 130 ms: the copy and the clear's undo step) and the own paste (about 110 to 130 ms: the floating pixels and the first frame);
at 2000 x 2000 the own paste (about 20 to 28 ms) and Cut (about 16 to 17 ms), at or just over one frame. The own paste already copied
in one block (the clipboard's picture taken with one copy, the float made with one), so it had no loop to replace.

The prompt's answer (T011, 2026-10-24, `perf-0016 oversize`: an 8000 x 8000 picture of another app, one color, pasted onto a 2000 x 2000
picture at 100 %; from the answer to the painted frame; the read and the decode happen at Paste, before the prompt, and are not in it;
three runs, load 0.28, the pointer parked):

| Answer | Time | Peak memory |
|---|---|---|
| Fit Picture to Canvas (8000 x 8000 scaled to 2000 x 2000: the loop runs over the 4 M output pixels, each reading its one source pixel, so 4 M of the 64 M are read) | 29.8 / 31.3 / 31.3 ms | 1022 to 1024 MB (the same as before the paste: the 8000 x 8000 picture and its PNG were made before the baseline, so the peak is theirs) |
| Resize Canvas to Picture (the picture grown to 8000 x 8000, 256 MB made and the old pixels copied, the paste of 256 MB floating) | 94.0 / 94.8 / 95.6 ms | the same |

Both are over one display frame: Fit about 2 frames, Resize about 6. Accepted slow cases (JOD, 2026-10-24): Fit about 30 ms and Resize
about 95 ms, 8000 x 8000 pasted onto 2000 x 2000 at 100 %. Both are memory-bound: Fit makes and fills the 2000 x 2000 float (the
resample already reads only the output pixels, which is JOD's question answered: `scale_region` loops over the 4 M outputs, not the 64 M
sources); Resize makes the 256 MB grown picture and copies the old pixels, and it cannot skip filling the new area even where the paste
covers it, because the paste's transparent pixels (REQ-0016-06) show what is under them: the background (REQ-0004-20).

### Scenario-to-test map
| AS | Test |
|---|---|
| 1 | ui `as1_clipboard_menus` + manual M1 |
| 2 | core `as2_copy_image`, `copy_by_rows_matches_oracle`, ui `as2_copy`, `as2_copy_big`, io `as9_encode_png_fast` |
| 3 | ui `as3_cut` |
| 4 | core `as4_paste_float_skips_clear`, ui `as4_paste_and_put_down` |
| 5 | ui `as5_paste_top_left_in_view` |
| 6 | ui `as6_paste_undo_and_order` |
| 7 | core `as7_fit_size`, `as7_put_down_grown`, `as7_grown_step_holds_what_it_counts` (selection), `as7_grow_step` (history), ui `as7_oversize_paste` |
| 8 | io `as8_open_bytes`, `as8_open_bytes_without_memory`, ui `as8_paste_from_and_files`, `as8_paste_from_dialog` + manual M2 |
| 9 | io `as9_encode_png`, ui `as9_copy_to` + manual M2 |
| 10 | ui `as10_clipboard_memory` |
| 11 | ui `as11_ignored_during_drags` |
| 12 | ui `as12_oversize_prompt` + manual M1 |
| 13 | ui `as13_canvas_menu` + manual M1 |
| 14 | ui `as14_clipboard_wait`, `as14_late_callback_after_timeout`, `as14_quit_during_read_that_succeeds` (the quit with an owner that answers after it) + manual M2 |
| 15 | core `as7_grow_step` (with no pasted pixel), ui `as15_clear_grown_paste` |
| (store at quit) | ui `store_at_quit`, `store_at_quit_late_callback` (Plan item, no AS) |
| perf | `perf-0016` (real session, pointer parked; modes: none, `zoom100`, `photo`, `oversize`) |
| probes (by hand, real session) | `probe-clipboard`, `probe-clipboard-xproc`, `slow-clipboard-owner` |

## Tasks
Every task gets a `Result:` line when it is ticked: the test that failed first and its message, or `characterization: <why there
was no red run>` with the mutant that proves it; a manual step, one line of what JOD saw.

Group A, the core and io (no ui change)
- [x] T001 The fit size and a float from a pasted picture (REQ-0016-06, REQ-0016-07; AS-4, AS-7) - done when core `as7_fit_size` (17 x 11, 21 x 9, 21 x 5, 21 x 1, 1024 x 576, halves up, at least 1) and `as4_paste_float_skips_clear` (no hole; the alpha-0 pixels neither laid, put down nor stamped, a stretched paste keeps the rule, partly transparent ones replace all four channels; a wholly transparent paste puts nothing down) pass after a red run, each test and its function in two edits
  Result: red by build first (`implicit declaration of function 'easel_fit_size'` and `'easel_selection_paste'`: core `as7_fit_size` and `as4_paste_float_skips_clear` written in one edit, the functions in the next), then green: `easel_fit_size` (the limiting side by `W * h <= H * w` in whole numbers, the other rounded halves up, at least 1; 17 x 11, 21 x 9, 21 x 5, 21 x 1, 1 x 11, 1024 x 576, 800 x 111, the same size) and `easel_selection_paste` (a float of the picture's pixels, no hole, the new `skip_clear`), whose rule lives in `lay_float_at`, the one function that lays the float and its copies for the preview and the put-down, through the image views (current, original, kept) that now carry the flag, so a copy made before a stretch keeps it. The test: no hole; at (2, 2) over the red block the alpha-0 pixels show red, a partly transparent one replaces; a copy and the float moved, then stretched to 6 x 4, skip theirs; the put-down changes only the non-clear pixels and Undo restores every pixel; a wholly transparent paste puts nothing down (NOTHING, the redo step kept); 23 bytes refused for 24. I corrected one check of my own test before the code: after the Undo the redo step stays, so a put-down that adds nothing leaves Redo available. Mutants: 9 new killed (the clear pixels laid, partly transparent ones skipped, the flag not set, a copy's image without the flag, the limit ignored, the fit's rounding and its minimum, twice); two mutants of 0013 and 0014 that quoted the lay's line rewritten (15 of 15 REQ-0013-11 and REQ-0014-08 killed). `mutate.sh --lint` 610, 0 stale.
- [x] T002 The joint growth step (REQ-0016-05, REQ-0016-07; AS-7, AS-15) - done when core `as7_grow_step` (undo crops back and restores every pixel; redo grows with the same fill and writes the changes; a step with no changed pixel, the growth alone; the bytes counted 64 + 12 per change, and held against counted) passes after a red run
  Result: red by build first (`implicit declaration of function 'easel_history_push_grow'`), then green: a step kind `STEP_GROW` (the old and new sizes, the fill, the changes, whose indexes are of the grown size); redo grows with the fill and writes the new values, undo crops back and writes the old values of the changes inside the old size; the size goes first both ways, so a buffer that cannot be had changes nothing (I had written undo the other way round first and turned it before ticking: a failed crop would have left the old values in a grown picture). It counts 12 bytes per change plus 64 (`step_bytes` as for a change step) and is added even with no change. The interface has the new size too: `easel_history_push_grow (h, old_w, old_h, new_w, new_h, fill, changes, n)` (the Plan's key interfaces to be brought to it at the gate). Core `as7_grow_step`: undo back to 21 x 11 with every pixel, redo to 30 x 20 with the yellow area and the three pixels, the step before stays undoable through it, the redo step it discards, the bytes 64 + 3 x 12, the growth alone 64. Mutants: 5 new, 9 of 9 REQ-0016-07 killed (the growth alone adds no step, undo keeps the grown size, redo's fill, undo writes the new values, redo leaves them out); two orders that no test without a failed allocation can tell apart (undo writing before cropping; the check of the old size, which `easel_image_set`'s clipping makes equivalent) are not listed. `mutate.sh --lint` 615, 0 stale.
- [x] T003 The copy of a selection (REQ-0016-03, REQ-0016-11; AS-2) - done when core `as2_copy_image` (marked rectangle, the triangle with 10 transparent pixels, a floating stretched selection partly outside the picture, the memory refused with NULL) passes after a red run
  Result: red by build first (`implicit declaration of function 'easel_selection_copy_image'`), then green: a new picture of the rectangle, filled with #00000000, the shape's pixels copied from the picture (marked) or from the float's own image (floating; the caller completes a lazy image first, as for a lay). Core `as2_copy_image`: marked 5 x 4 equal to the picture with red at its columns 1 to 3 of rows 1 and 2; the marked triangle with its 10 other pixels #00000000; 99 bytes refused for 5 x 5, 100 enough (the exact case added before the mutants ran); a floating selection stretched to 10 x 4 at (-3, 1), partly outside the picture, equal to its own pixels; a floating triangle transparent outside its shape. Mutants: 5 new killed (the pixels outside the shape copied, a float copied from the picture, a float's shape ignored, the limit ignored, the exact fit refused); 6 of 6 REQ-0016-03 and -11 killed. `mutate.sh --lint` 620, 0 stale.
- [x] T004 Open from bytes, encode to bytes (REQ-0016-03, REQ-0016-09, REQ-0016-10; AS-8, AS-9) - done when io `as8_open_bytes` (PNG, JPEG, a text, an empty, a 10001 x 1 header, too little memory) and `as9_encode_png` (round trip exact with partly transparent pixels; `easel_picture_save` unchanged in what it writes) pass after a red run; group A gate: full suite, `mutate.sh` for the REQ ids so far, `--lint`
  Result: red by build first (`implicit declaration of function 'easel_picture_open_bytes'` and `'easel_picture_encode'`), then green: the sniffer works on a stream (`sniff_stream`, a file's or bytes' through `fmemopen`), `easel_picture_open_bytes` makes the same checks in the same order as a file's (what and how big from the header, then the memory) and decodes through `gdk_pixbuf_new_from_stream`; both opens end in one `from_pixbuf` (upright, 8-bit RGBA); `easel_picture_encode` is the old body of `easel_picture_save`, which now writes what it returns. io `as8_open_bytes`: ten fixtures (PNG, JPEG, a rotated JPEG, BMP, a PNG named `.dat`, a GIF, 10001 wide, empty, a cut header, a JPEG with no size) give the same result, size and pixels from bytes as from the file; a text and no bytes are not pictures; `as8_open_bytes_without_memory` (a subprocess, the address space limited): NO_MEMORY with 6000 x 6000 before decoding; `as9_encode_png`: PNG bytes open again to the same pixels, partly transparent ones exact, a fully transparent one still fully transparent; JPEG bytes opaque. The 0005 tests pass unchanged (17 of 17 in `test-picture-file`). Mutants: 4 new killed (the memory check, the 10000 limit, the sizes of a refusal, the transparency lost in the encode).

Group B, the window
- [x] T005 The actions and the menus with their sensitivity (REQ-0016-01, REQ-0016-02; AS-1, AS-11) - done when ui `as1_clipboard_menus` (the five actions, their accelerators, the Edit submenu's two sections, the selection menu's first section, Copy To… and Paste From… in the primary menu, sensitivity with and without a selection and with the clipboard's content changing) and `as11_ignored_during_drags` pass after a red run through the seam (`EaselClipboard`, `EaselGdkClipboard` with no logic, and the tests' `EaselMemoryClipboard`, which the test fills with text, a picture or files as another app would), and the tests of the menus' content are moved in this task and pass (0013 `as11_context_menu` and `as10_select_all_edit_menu`, 0005 `as1_file_menu`)
  Result: red first: ui `as1_clipboard_menus` (`describe_section: assertion failed (section < g_menu_model_get_n_items (model)): (1 < 1)`: the selection menu had one section). Its first run had segfaulted inside GMenu (an index past the end); `describe_section` now asserts the index and that the item is a section, so a menu test fails with a message. Then the seam: the first runs with the actions built showed that the isolated session holds no clipboard (the window is never active; the compositor drops the set: `gdk_clipboard_is_local` false, no formats), JOD chose the seam (option 1), and `EaselClipboard` (an interface mirroring GdkClipboard's: formats, a read of a picture and of files, content with a provider, is-local, get-content, store, `changed`), `EaselGdkClipboard` (forwards each call and the signal, no logic of its own) and the tests' `EaselMemoryClipboard` (filled by the test as another app would; a read goes through the provider, as GDK's local read does; the store fails at once, as with no clipboard manager) were built; the window gets its clipboard through `easel_window_set_clipboard` (NULL: the desktop's). The actions `win.cut`, `win.copy`, `win.paste`, `win.copy-to`, `win.paste-from` (their behaviour comes with T007 and later; they do nothing yet), Ctrl+X, Ctrl+C, Ctrl+V, the menus' new sections, and `update_clipboard_actions` (Cut, Copy and Copy To… with a selection, Paste while the clipboard holds a picture by its type or MIME type or files, Paste From… with the picture, none while a button is held), called at a change of the selection, the clipboard, the picture and a held button. `as1_clipboard_menus` and `as11_ignored_during_drags` (a pencil stroke, a rectangle dragged out, a move, a stretch: all five insensitive while held) pass through the seam; `as11` had no red run of its own behaviour (its first run failed on the dropped clipboard, before the seam), so it is proven by the mutants "the commands stay sensitive while a button is held" and "Paste From is sensitive while a button is held". Moved: 0013 `as11_context_menu` (its second section now), `as10_select_all_edit_menu` (the Edit submenu's sections, Copy To… and Paste From… after Edit); 0005 `as1_file_menu` passes unchanged. Mutants: 7 new, all killed, run twice (each under 5 s, as ui-0016 has two cases); `mutate.sh --lint` 631, 0 stale; ui-0005, -0013, -0014, -0015, -0016 and the full isolated suite (45 of 45) pass, warning-free.
- [x] T006 The canvas menu (REQ-0016-14; AS-13) - done when ui `as13_canvas_menu` passes after a red run, the tests whose secondary press now opens the canvas menu or must show that it does not are moved in this task and pass (0013 `as11_context_menu`'s press at (40, 40), `as12_drags_that_do_not_finish`; 0014 `as17_free_context_menu`, `as11_traces_that_do_not_finish`; 0015 `as8_selection_handle_wins`, `as9_stretch_that_does_not_finish`; every other secondary press with a select tool found by the grep), and it ends with manual M1 of the menus' look (the selection menu, the Edit submenu, the primary menu, the canvas menu, at 100 %)
  Result: red by build first (`implicit declaration of function 'easel_window_get_canvas_menu'`), then green: the canvas's secondary press with Select or Free-Form Select emits `canvas-menu` where it did nothing (not on the selection: the selection's `context-menu` as before; the handle presses and the held drags keep their earlier branches), and the window opens `canvas_menu` (Paste, Paste From…, Select All; Zoom In, Zoom Out, Normal Size) at the pointer, counted by `menu_is_open` so the window takes none of its keys. ui `as13_canvas_menu`: the sections and actions, Normal Size on Ctrl+PageUp and insensitive at 100 %, Zoom In insensitive at 800 %, the menu at (15, 8) with no selection, marked and floating (Esc, Enter, Delete and the arrows not taken while it is open, the selection still floating after), the selection's own menu on the selection, no menu on a selection handle, on a picture handle or during a held drag, and on the triangle's box outside its shape. Moved: 0013 `as11_context_menu` (the press at (40, 40) now opens the canvas menu), 0014 `as17_free_context_menu` ((5, 5) and (40, 40) now open it); a `g_assert_false (canvas_menu_shown (win))` after each secondary press of 0013 `as12_drags_that_do_not_finish` and `as14_selection_blocked`, 0014 `as11_traces_that_do_not_finish`, 0015 `as8_selection_handle_wins` and `as9_stretch_that_does_not_finish` (13 presses; the grep found no other secondary press made with a select tool). Mutants: 4 new killed (nothing opens outside with Select, the same with Free-Form Select, the canvas menu on the selection, the window taking the keys while it is open); two mutants of 0013 and 0014 that quoted the old press code rewritten; 11 of 11 REQ-0016-14, REQ-0013-17 and REQ-0014-07 killed, run twice. Full isolated suite 45 of 45; `mutate.sh --lint` 635, 0 stale. Manual M1 (2026-10-22): JOD looked at the primary menu, the Edit submenu, the selection's menu and the canvas menu with Select and Free-Form Select at 100 %: "M1: ok".
- [x] T007 Copy and Cut (REQ-0016-03, REQ-0016-04; AS-2, AS-3) - done when `probe-clipboard` has run by hand in the real session and its results are in the Plan, and ui `as2_copy` (through the seam: the memory clipboard holds the window's provider; read back by the own path and by the PNG path, as another app reads it; the snapshot after Delete) and `as3_cut` (Opaque, Transparent, Undo, the clear refused) pass after a red run
  Result: `probe-clipboard` written and run by hand in the real session first (its results in the Plan's Risks; its first version hung, its provider writing synchronously to a stream whose reader was the same main loop, so every provider writes without blocking; the memory clipboard now reports formats as GDK does: serialized MIME types and deserialized types). Then red by build (`implicit declaration of function 'easel_clipboard_picture_get_image'`, `EASEL_IS_CLIPBOARD_PICTURE`: ui `as2_copy` and `as3_cut` written first), then green: `EaselClipboardPicture`, a provider holding the copy (the snapshot), offering `image/png`, encoded by `easel_picture_encode` on a worker thread (`g_task_run_in_thread`) when a reader asks and written with `g_output_stream_write_bytes_async`; `easel_canvas_copy_selection` (a marked selection's copy from the picture, a floating one's from its own pixels, the image completed first); `copy_selection` and Cut (the copy, then Clear Selection's own clear, nothing cleared if the copy fails). Through the seam: `as2_copy` (marked 5 x 4, read back by the own path and by the PNG path exactly; the selection, picture and history unchanged; Delete after Copy leaves the copy; the floating triangle 5 x 5 with its 10 other pixels transparent), `as3_cut` (Opaque: 20 pixels cyan, one step, Undo restores and the clipboard keeps the copy; Transparent: #00000000; the clear's step refused: "Could Not Clear the Selection", the clipboard holds the copy, the selection as it was). Mutants: 5 new killed (nothing set, the PNG not a PNG with transparency, another MIME type, a float copied from the picture, Cut not clearing); 9 of 9 REQ-0016-03 and -04 killed. The rule "if the copy fails nothing is cleared" needs a refused copy: its test and mutant come with T015's memory cases. Full isolated suite 45 of 45; `mutate.sh --lint` 640, 0 stale.
- [x] T008 Paste of a picture no bigger than the picture (REQ-0016-05, REQ-0016-06; AS-4, AS-5, AS-6) - done when ui `as4_paste_and_put_down`, `as5_paste_top_left_in_view` (rendered at 400 % scrolled) and `as6_paste_undo_and_order` pass after a red run
  Result: ui `as4_paste_and_put_down`, `as5_paste_top_left_in_view` and `as6_paste_undo_and_order` written first; red (`assertion failed (easel_window_get_tool (win) == EASEL_TOOL_SELECT): (1 == 15)`: Paste did nothing), then green: `read_clipboard_picture` (Easel's own copy taken as it is; another app's read on the spot as PNG first, then the types Open reads, and opened by `easel_picture_open_bytes`), Paste applying what is pending as a tool change to Select does, `easel_canvas_top_left_in_view` (the pixel under the top-left point of the part of the picture in view, (0, 0) while its corner is in view), and `easel_canvas_paste` (the float made first, so a refusal changes nothing; then a floating selection put down or a marked one removed; the paste floats with no hole). Tests through the seam: Ctrl+V with the Pencil: Select, 5 x 4 floating at (0, 0), "5 × 4", a rectangle; dragged by (10, 5) and put down, 6 pixels, one step; the triangle's copy put down at (0, 0): 4 pixels, the red under its transparent pixels kept; at 800 % scrolled to the far corner the paste lands at the partly shown pixel and is drawn there; a 250 x 180 paste from there runs past the right and bottom edges and only its part inside is put down; Undo while floating keeps the redo step; a floating selection and a floating paste are each put down as a step of their own by the next Ctrl+V; a pending curve is applied; a paste put down where its pixels already are adds no step. I changed one test before the code: AS-5's 400 % cannot scroll a 300 x 200 picture in the test window (1200 logical pixels fit it), so the test scrolls at 800 %, which also lets a paste smaller than the picture run past its edges. Not yet: the oversize prompt (T010), the time limit, copied files and the errors (T012); a picture bigger than the picture is not pasted for now. Mutants: 4 new killed (always (0, 0), the partly shown pixel skipped, the tool kept, the floating selection dropped instead of put down). Full isolated suite 45 of 45; `mutate.sh --lint` 644, 0 stale. Addendum (2026-10-23, found by `perf-0016`'s first runs in the real session, which hung): the window read the clipboard's stream with a blocking read, and for a content of this process GDK fills that stream from the same main loop, so it waited for ever; the memory clipboard had hidden it by handing back a stream already full. The memory clipboard now does as GDK does (the read end of a pipe at once, the provider writing it only when the main loop runs), the window and the tests' PNG helper read the stream with `g_output_stream_splice_async` while the loop runs; checked by hand: the old blocking read put back makes `as5_paste_top_left_in_view` hang (cut off after 60 s), the fix passes all eight 0016 cases.
- [x] T009 Measure a paste, Copy and Cut, right after T008 (REQ-0016-03, REQ-0016-04, REQ-0016-05; AS-2, AS-3, AS-4) - done when `perf-0016` (its header: keep the pointer outside the window) has run three times in the real session with no Easel window open and the load average noted: a paste of 2000 x 2000 and of 8000 x 8000, from Easel's own copy and from another app's PNG, and a Copy and a Cut of a 2000 x 2000 and an 8000 x 8000 selection, each timed from the key to the painted frame, with the peak memory and its baseline, and the PNG encode for another app timed apart (the worker thread's run, which the window does not wait for); the numbers are in the Plan; a case over one display frame goes to JOD before any fix and the tasks below wait
  Result: `perf-0016` written (header: keep the pointer outside the window) and run in the real session with the pointer parked: its first three runs hung, which found the blocking read of the clipboard's stream (T008's addendum); then three runs at the fit zoom, over one frame in most cases (the Plan's Measurements), sent to JOD before any fix. JOD's follow-ups: at 100 % the own paste at 8000 x 8000 falls from about 370 to about 110 to 137 ms (drawing at the fit zoom was the cost: 0015's ruling); a photo-like PNG's paste is decode-bound (about 50 ms at 2000 x 2000, about 550 ms at 8000 x 8000), accepted. JOD's decision: a copy by rows for Copy and Cut, test first with the pixel-by-pixel copy as the oracle (core `copy_by_rows_matches_oracle`: a marked rectangle, at the edges, the whole picture, one pixel wide and high, a free-form shape, a floating selection partly outside the picture with and without a shape; it passed against the old code and after the change), 4 mutants for the rows killed (the wrong stride, one row too few, an offset at the edge, a floating copy short of its last row), one older mutant rewritten; again three runs at 100 %: Copy at 8000 x 8000 about 70 ms (was 88 to 123), Cut about 130 ms (was 156 to 161), the own paste unchanged (it copied in one block already). What stays over a frame is accepted with these numbers (the Plan's Measurements). The PNG encode on the worker thread takes 530 to 570 ms at 8000 x 8000, which keeps the thread (P5). Full isolated suite 45 of 45; `mutate.sh --lint` 648, 0 stale.
- [x] T010 The oversize prompt, Fit and Resize (REQ-0016-05, REQ-0016-07, REQ-0016-13; AS-7, AS-12, AS-15) - done when ui `as7_oversize_paste` (Fit, Resize with the joint step, Undo while floating, Cancel with a floating selection and a pending curve, the scroll to (0, 0) at 400 %, 21 x 11 no prompt, 22 x 11 prompt) `as12_oversize_prompt` (heading, body, buttons, Enter, Esc, a window resize, a quit request) and `as15_clear_grown_paste` (Delete on a grown paste: 30 x 20, one step, Undo to 21 x 11; with a Ctrl copy left) pass after a red run
  Result: core `as7_put_down_grown` written first, red by build (`implicit declaration of function 'easel_selection_put_down_grown'`), then green: `easel_selection_put_down_grown` (the put-down's pixels with the growth as one `easel_history_push_grow` step, added even when no pixel changes or when there is nothing to lay: a cleared paste with no copy); ui `as7_oversize_paste` red first (`paste_prompt (win)` should not be NULL: Paste pasted nothing bigger than the picture), `as12_oversize_prompt` and `as15_clear_grown_paste`, then green with `ask_paste_size` (the prompt: heading, body with both sizes, Resize the suggested default, Fit, Cancel; Esc and closing are Cancel), `on_paste_prompt_answered` (Resize: `easel_canvas_paste_grown`, the picture grown to the larger of each side in the window's background color; Fit: `easel_fit_size` and `easel_selection_scale`, the paste at (0, 0); both scroll the view to the picture's corner, zoom kept; Cancel changes nothing), and in the canvas the picture as it was kept while a grown paste floats (`paste_base`; Undo while it floats and a failed put-down give it back with no allocation; the put-down and a Clear Selection push the joint step; a new `picture-replaced` signal tells the window the size changed). Tests: the prompt asks first and nothing is changed; Fit 282 x 200, 300 x 94 and 67 x 200; Resize 310 x 220, 320 x 200 (the new strip in the background color, the pasted rows blue) and 300 x 300; the status bar's size follows; Undo while floating gives 300 x 200 and every pixel with no step and the redo step kept, after the put-down one Undo does; Cancel leaves a floating selection floating and a pending curve pending; the view scrolls to (0, 0) at 800 %; 300 x 200 asks nothing, 301 x 200 asks; AS-15: Delete on a grown paste leaves 310 x 220 with no pasted pixel and one step, with a Ctrl copy left the copy is in the picture, Undo gives 300 x 200 and every pixel. Spec gap found: AS-12 says a quit request closes the prompt as Cancel "and goes on", but REQ-0016-13 and REQ-0005-02 (the New and Open prompts, as33) say only that it closes as Cancel, and libadwaita closes an open dialog as its close response and the window stays; the test follows the requirement (the prompt closes, nothing is pasted, the window stays, a second quit request quits). The wording fix of AS-12 ("closes it as Cancel; the window stays") was made with JOD's yes (2026-10-24). Mutants: 12 new (Resize's size and fill, Cancel fitting, no scroll, the size test at equal size and at height only, Undo keeping the growth, a plain put-down, Clear dropping the growth, the growth alone and the empty put-down adding no step, the crop size), two survived first (a taller-only paste and the empty grown put-down: both tests added) and are now killed; three older mutants of 0013 quoting the put-down code rewritten; 49 of 49 REQ-0016-05, -07 and REQ-0013-11, -14, -19 killed. Not yet tested: a put-down that fails for memory with a grown paste (the picture as before, its size too): T015's memory cases. Full isolated suite 45 of 45; `mutate.sh --lint` 660, 0 stale.
- [x] T011 Measure Fit and Resize, right after T010 (REQ-0016-07) - done when `perf-0016` also times the prompt's answer to the painted frame for Fit and Resize of an 8000 x 8000 paste on a 2000 x 2000 picture, three runs, the numbers in the Plan; over one frame goes to JOD first
  Result: `perf-0016` gained the mode `oversize` (an 8000 x 8000 picture of another app pasted onto a 2000 x 2000 picture at 100 %, each answer timed from the answer to the painted frame; the read and decode are at Paste, before the prompt) and ran three times in the real session with the pointer parked, load 0.28: Fit 29.8 / 31.3 / 31.3 ms, Resize Canvas to Picture 94.0 / 94.8 / 95.6 ms, both over one frame, sent to JOD before any fix; JOD accepted both as slow cases (memory-bound; Resize cannot skip filling the new area because a paste's transparent pixels show the background), recorded with the reason in the Plan's Measurements. JOD's question, whether Fit's resample runs over the sources: it runs over the 4 M output pixels (`scale_region`), so no fix and no re-measure.
- [x] T012 Reading the clipboard: wait, time limit, files, errors (REQ-0016-10, REQ-0016-15; AS-8, AS-14) - done when ui `as14_clipboard_wait` (through the seam: a memory clipboard whose owner never answers: the busy cursor, keys waiting, the dialog after the limit; three quick Ctrl+V) and the clipboard part of `as8_paste_from_and_files` (through the seam: a copied PNG file, a copied text file, a picture and a file offered, 10001 x 1; Files' own copy is M2) pass after a red run
  Result: ui `as14_clipboard_wait` and `as8_paste_from_and_files` (its clipboard part) written first and red by build (`implicit declaration of function 'easel_window_set_clipboard_timeout'`), then green with `read_clipboard_picture` rebuilt: the window waits on the spot (`reading_clipboard` joins `dialog_is_open`'s gate, so the commands, keys and Select All act on nothing; the busy cursor; the canvas's input blocked; a quit request deferred by `close_after_reading` and handled after the read), the limit (5 s, `easel_window_set_clipboard_timeout` for the tests; the read is cancelled), the choice (a picture offered, by an `image/` type or a texture, wins over copied files; with files only the first file of the list is opened as Open opens it), and `paste_error` ("Could Not Paste" with "The clipboard's picture cannot be read.", "This is not a picture the app can open.", the 10000 body or the memory body; for a file "Could Not Paste “<name>”" with "The file cannot be read." and the others). Through the seam: an owner that never answers: the busy cursor, a key, Select All and another Paste doing nothing meanwhile, after 400 ms the dialog and nothing changed, input taken again, the paste works when the owner answers; three quick pastes of different pictures give three steps; a quit request during the wait closes the window after it, with no error dialog (a dialog would take the quit); a copied PNG file pastes (3 x 2 at (0, 0)), a copied text file and a gone file give their dialogs, a picture and a file offered together paste the picture, two files paste the first (a text second: the PNG; the text first: its error), 10001 x 1 and bytes that are no picture give theirs. Test-side fixes before the run was green, all mine: `gdk_cursor_get_name` on the window's NULL cursor; the memory clipboard's hung read left its task unfinished when cancelled (it now ends with the cancellation, as GDK's read does, and when the clipboard goes); the union provider takes its providers. Mutants: 13 new (the busy cursor, the gate, the deferred quit, the forgotten quit, the dialog with a quit waiting, input left blocked, files over a picture, three wrong bodies, a file's heading, the last file) and 11 killed in the run; one survived first (all my files tests used one file: the two-file case added) and one was killed only by a crash (a window destroyed inside the wait: the probe now asserts the window is still there); two older 0005 mutants of the busy cursor, whose find text the new read repeats, were made unique; 24 of 24 REQ-0005-02, REQ-0016-10 and -15 killed. Full isolated suite 45 of 45; `mutate.sh --lint` 672, 0 stale.
- [x] T013 Paste From… (REQ-0016-08; AS-8) - done when the Paste From part of `as8_paste_from_and_files` (3 x 2 at (0, 0), title kept, Cancel keeps a floating selection, `a.png` text) passes after a red run
  Result: ui `as8_paste_from_dialog` written first and red by build (`implicit declaration of function 'easel_window_paste_from_file'`), then green: `paste-from` opens the file dialog (`make_picture_dialog`, the folders and filters of Open, now shared with it; titled "Paste From") with `open_dialog_open` set, so a quit request does nothing meanwhile; Cancel changes nothing; a chosen file goes to `easel_window_paste_from_file` (read as Open reads it by `easel_picture_open`; a refusal shows `paste_error` with the file's name; else `paste_picture`, the same path as Ctrl+V: put down what floats, the prompt for a bigger one, Select, floating at the top-left in view). Tests: the dialog's title with a floating selection that stays floating and the picture unchanged, a quit request doing nothing (window visible, selection still floating), Cancel (Esc) leaving it floating, then a file chosen in the real dialog (the in-process `GtkFileChooserDialog` of `GDK_DEBUG=no-portals`, driven by its deprecated interface inside the ignore-deprecation macros: its folder, a moment to read it, `set_file`, the `response`): the floating move put down (a step: the title gets the modified mark, keeps its name) and the 3 x 2 file floating at (0, 0), Undo twice gives every pixel back; a text file named a.png: "Could Not Paste “a.png”", nothing changed; a 310 x 220 file asks first and Cancel changes nothing. Run through meson (`ui-0016`), not the bare test binary: the bare binary lacks `GDK_DEBUG=no-portals`, so the dialog was never found. Found by the test and fixed: `close_request` put a floating selection down before it noticed the file dialog was open, so a quit request during Open's or Paste From's dialog put it down; REQ-0005-02 says such a quit does nothing and REQ-0016-08 that nothing is put down, so the check of the dialog is now first (it also changes Open's behaviour, which only the new test with a floating selection could see). Mutants: 5 new killed (the title, the quit putting the selection down, the quit not held back, the file's name left out of the dialog, the chosen file ignored: the last needed the real dialog in the test, the hook `easel_window_paste_from_file` alone could not tell); one 0005 mutant rewritten for the new place of the check; 21 of 21 REQ-0016-08, -10 and REQ-0005-02 killed. Full isolated suite 45 of 45; `mutate.sh --lint` 677, 0 stale.
- [x] T014 Copy To… (REQ-0016-09, REQ-0016-12; AS-9) - done when ui `as9_copy_to` (`t.png` exact, `t.jpg` white, the save error, the picture's file and title kept, the selection kept) passes after a red run
  Result: ui `as9_copy_to` written first and red by build (`implicit declaration of function 'easel_window_copy_to_chosen'`), then green: `copy-to` opens a save dialog titled "Copy To" (suggesting Untitled.png in the folder of the picture's file or the Pictures folder; `save_dialog_open` set, so a quit request behaves as for Save As); a chosen name goes to `easel_window_copy_to_chosen`, which makes the selection's copy at once and runs Save As's own name rules (`save_chosen`: the extension decides the format, `.png` added, an unknown one is "Cannot Save This Kind of File", and "Replace “t.png”?" when `.png` was added to an existing name) through the same `SaveRequest`, which now carries the copy; `write_copy` writes it with `easel_picture_save` at the settings' JPEG quality and shows the save error of REQ-0001-31 on a failure; the picture, its file, title, modified mark and history are not touched. Tests: the dialog's title and its cancel (after a moment: closing a chooser that is still reading its folder makes GTK show an error of its own; nothing written); t.png is 5 x 5 with the triangle's 15 pixels and 10 fully transparent; t.jpg opaque with the 10 white; no extension asks to replace t.png (Cancel keeps the old bytes, Replace writes the copy of the floating selection, which stays floating); t.gif is the kind-of-file error; a folder made read-only gives "Could Not Save “t.png”" and no file; title, modified mark and the picture as they were, history unchanged (Undo only forgets the float); a modified picture stays modified with its mark. Test-side fixes, mine: the answer to the replace question reaches the write on the next iteration (as in 0001's test); Undo is always there while a selection floats. Mutants: 7 new (the whole picture written, the title, no replace question, no save error, the picture marked saved, a JPEG quality of its own; the cancel guard's removal only crashes on a NULL name, so that mutant was dropped, its guard being exercised by the cancel step); two crashes became clean failures (the dialog asserted present before its heading is read), one survivor closed (a modified picture); two older mutants made unique (0005's JPEG quality, whose text the copy's write now repeats): 6 of 6 REQ-0016-09 and -12 killed in the last run. Full isolated suite 45 of 45; `mutate.sh --lint` 683, 0 stale.
- [x] T015 Memory failures and the store at quit (REQ-0016-11; AS-10; the Plan's store) - done when ui `as10_clipboard_memory` (Copy, Cut, Copy To, a paste, Resize's growth, Fit) and `store_at_quit` (through the seam: the store asked of the memory clipboard only when the content is ours, the quit going on after a failure or the 2 s; what a real clipboard manager does is `probe-clipboard`'s) pass after a red run; group B gate: full isolated suite, `mutate.sh` for every REQ-0016 id and the touched REQs of 0013 to 0015, `--lint`
  Result: ui `as10_clipboard_memory` and `store_at_quit` written first; red (`assert_alert: 'dialog' should not be NULL`: Copy with no memory showed nothing), then green: Copy, Cut and Copy To… with no memory for the copy show "Could Not Copy the Selection" (Cut clears nothing, Copy To… writes nothing); `easel_canvas_can_paste` looks for the memory of a paste and of the grown picture (the selection's limit and the system, each on its own) before the tool changes, so a refused paste, Fit or Resize changes nothing, the tool and a floating selection included, and shows "Could Not Paste" with the memory body; a put-down that cannot get the step's memory after a growth gives the picture back as before the paste, its size too, with 0013's dialog; `store_clipboard_for_quit` asks the clipboard manager to keep our copy when the window closes (an unmodified picture, or the quit prompt's Discard), waits up to 2 s and goes on whatever the answer (none keeps it here: the memory clipboard's store fails at once). Test-side fixes of mine: the Pencil and a floating selection cannot exist together (a tool change puts it down), so two cases; `respond` closes a prompt whose window the quit destroyed. A test gap found by the mutants and closed: the refusal of a Resize and of a Fit was tried with Select already the tool, which hides whether the tool changed first: it now uses the Pencil, with the mutant "a refused Resize has already changed the tool"; a second gap at the group gate: the MIME-type test of Paste's sensitivity had become invisible once the memory clipboard also reported the deserialized picture type (as GDK does), so `as1_clipboard_menus` now offers `image/x-fake`. Mutants: 9 new killed; one removed as equivalent (the picture-type test in the store: the check and the code), the equivalence notes of Fit's twice-made limit are in the Check section. Group B gate (2026-10-24): full isolated suite 45 of 45; `mutate.sh` for the ids of REQ-0016, REQ-0015, REQ-0014, REQ-0013 and REQ-0005: 411 mutants, 409 killed, 0 killed by crash, 0 inconclusive, 1 survived (the MIME test above, closed and killed in a second run with REQ-0016-11's new mutant: 16 of 16), 1 long-only not run (0014's, run at the check); `mutate.sh --lint` 693, 0 stale.
- [x] T016 Manual M1 and M2 with the real mouse and other apps (REQ-0016-01, REQ-0016-13, REQ-0016-14, REQ-0016-15; AS-1, AS-8, AS-9, AS-12, AS-13) - done when JOD has copied from Easel into another app (a browser or an image viewer) and back, copied a screenshot and a file in Files and pasted them, seen the prompt for a big screenshot with Fit and Resize, used Copy To and Paste From, and right-clicked outside a selection with Select and Free-Form Select, at 100 % and 400 %, and, as M2's step to judge by eye, with a clipboard owner that answers late (a small helper offering a picture after a few seconds, `tests/probe/slow-clipboard-owner.c`, built in this task), pressed Ctrl+V and judged by eye the busy cursor while it waits, keys doing nothing meanwhile, and the dialog "Could Not Paste" when it never answers (REQ-0016-15; AS-14's manual part)
  Result: manual M1 and M2 with the real mouse and other apps (JOD, 2026-10-25). M1: the menus, the prompt with Fit and Resize, Copy To and Paste From and the canvas menu worked at once; pasting Easel's copy into another app did not, and the two checks found two real bugs and a helper fault, all fixed and checked again by JOD. (1) The provider wrote the encoded PNG with `g_output_stream_write_bytes_async`, which writes only what a pipe takes at once (64 KiB) and reports success, so a copy whose PNG was over 64 KB reached other apps cut off at 65536 bytes (found by reading the clipboard from outside: a PNG of 65536 bytes whose header claimed 2685 x 2160); my tests used small pictures. Fixed with `g_output_stream_write_all_async`; ui `as2_copy_big` (a noisy full picture copied and read back exact, red first: the bytes were not a picture), mutant "the PNG offered to another app is cut off at what a pipe takes at once"; Writer and Dibuja then took the paste. (2) JS Paint still refused a big copy (3178 x 3099 of a photo) and took a small one: served at once from a prepared file it took 0.6, 3.2, 12.8 and 25.5 MB PNGs (`slow-clipboard-owner 0 FILE`), so size was not the cause; Easel's default PNG encoding of that picture takes 2.2 s after the other app asks (measured: 2189 ms, 12 MB; at compression 1: 113 ms, 24 MB) and the browser gave up waiting. Fixed with `easel_picture_encode_fast_png` for the clipboard's PNG (Save As keeps the default level); io `as9_encode_png_fast` (exact round trip, larger than the default's on noise), ui `as2_copy_big` checks the bytes read are the fast encoding's, two mutants (the default level used; the fast level not asked), all 17 REQ-0016-03 mutants killed; the big copy then pasted into JS Paint, also an even bigger one. (3) A fault of my helper, not of Easel: M2's first run showed Paste greyed because `slow-clipboard-owner` set the clipboard with no input, and the compositor takes a clipboard only from a client that has just had one, so Easel still saw an old screenshot; the helper and `probe-clipboard-xproc` now set it from a button click. M2: with the helper answering after 4 s the busy cursor showed, keys did nothing meanwhile and the picture arrived; with `never` the dialog "Could Not Paste" appeared after about 5 s and Easel worked again. Also written for the check: `tests/probe/probe-clipboard-xproc.c` (two processes: `own` puts Easel's copy on the clipboard on a click, `read` reads it back through GTK; with `xclip` it shows what an X app sees) and `slow-clipboard-owner [SECONDS | never [PNG-FILE]]`. Full isolated suite 45 of 45; `mutate.sh --lint` 696, 0 stale.
  Again on the build after T018 (JOD, 2026-10-26): M1 all five steps ok (Writer, Dibuja and JS Paint, a big photo rectangle too; a screenshot and a Files picture pasted; Fit, Resize, Cancel and Undo of Resize; Copy To and Paste From; the canvas menu at 100 % and 400 %); M2: `slow-clipboard-owner 3`, the busy cursor and the picture after about 3 s; `never`, "Could Not Paste" after about 5 s and Easel working again; a quit within the 3 s, nothing pasted and the window closed.
- [x] T017 Gate (all) - done when the full isolated `meson test` passes, `mutate.sh` for every REQ-0016 id and `--lint` are clean, `tools/spec.sh lint --names 0016` is clean, and the Plan's interfaces and map match the names as built
  Result: full isolated `meson test` 45 of 45, warning-free; `mutate.sh` for every REQ-0016 id (2026-10-25, after the fixes of M1): 95 mutants, 95 killed, 0 killed by crash, 0 inconclusive, 0 survived, 0 errors (912 s); the earlier group B gate (REQ-0016, -0015, -0014, -0013, -0005: 411 mutants, one survivor since closed) stands for the other features; `mutate.sh --lint` 696, 0 stale; `tools/spec.sh lint` and `lint --names 0016` clean; the Plan's key interfaces (the history step's new size, `easel_selection_put_down_grown`, `easel_picture_encode_fast_png`, the clipboard provider and the canvas and window hooks) and its scenario-to-test map brought to the names as built, with the probes and `perf-0016`'s modes listed.
- [x] T018 Check findings F1-F3 (REQ-0016-15, REQ-0016-07; Check 2026-10-26) - done when: before fixing, every wait that returns on a
  time-out or a cancel while a callback can still arrive is listed with a finding for each (Open's read, the decode, the store at
  quit, the clipboard read, any other a grep finds); F1 red first: the memory clipboard gets an option to answer a cancel late, as
  GIO does, and a ui test has the callback arrive after the time-out (also run once under an address sanitizer or valgrind, how
  reported), then fixed with the wait data on the heap, freed by the last of the callback and the wait, in `read_clipboard_picture` and
  `store_clipboard_for_quit`; F2 red first: a quit during a read that then succeeds, with an unmodified and with a modified picture:
  nothing pasted, the window closes or asks to save, no crash; F3: a held-versus-counted test for the grown step and a mutant that
  removes the shrink in `push_step`; mutants for each run against the short suite with counts, the full isolated suite, the lints, and
  the Check section updated.
  Result: the waits that can give up while a callback can still arrive (a grep of `g_main_context_iteration`, `g_cancellable_cancel`
  and the asynchronous calls of `window.c`): (1) the clipboard read, `read_clipboard_picture`: a nested wait with a 5 s limit, the
  result kept on the stack: F1; (2) the store at quit, `store_clipboard_for_quit`: a nested wait with a 2 s limit, the same: F1;
  (3) Open's read (the file dialog's callback): not a wait that gives up, the callback holds a reference to the window and the window
  cannot close while the dialog is open (`open_dialog_open`): no finding; (4) the decode: synchronous on the main loop after a painted
  frame (`after-paint`, connected with `g_signal_connect_object`) or from an idle that holds a reference: nothing to give up on, no
  finding; (5) the color chooser: cancelled when the choice ends, its callback holds a reference and finds the choice already finished
  (`chooser_open`), and a quit closes it first: no finding; (6) Save, Copy To… and Paste From… dialogs: callbacks hold a reference, the
  window cannot close while they are open: no finding. A note, not a finding: during the store's wait input is not blocked, so in a
  slow clipboard manager's 2 s a stroke could change a picture the quit has found unmodified; on Wayland GDK's store answers at once
  (no manager to ask), so it cannot happen there; left as it is unless JOD wants input blocked. F1: the memory clipboard got
  `set_late_cancel` (a cancelled read or store answered from an idle, as the probe showed GIO does), `set_store_hangs` and
  `set_answer_delay`; red first: ui `as14_late_callback_after_timeout` (`assertion failed (late * 10 + (weak != NULL) == late * 10):
  (11 == 10)`: with the late cancel the clipboard stayed alive, the result's reference taken by the late callback; the control run
  with an owner that answers let it go) and `store_at_quit_late_callback` (`'weak' should be NULL`). The red test under AddressSanitizer
  (a second build, `meson setup .scratch/build-asan -Db_sanitize=address -Db_lundef=false -Dbuildtype=debug`, `test-window` run alone
  through `tools/isolated.sh` with `GDK_DEBUG=no-portals ASAN_OPTIONS=detect_leaks=0:detect_stack_use_after_return=1:halt_on_error=1`):
  `ERROR: AddressSanitizer: stack-use-after-return`, `WRITE of size 8` in `on_clipboard_read` (window.c:2062) into the frame of
  `read_clipboard_picture`; its control run first needed the normal 5 s limit (ASan made the 300 ms one too short for an answer).
  Fixed with `Answer` on the heap, shared by the wait and the callback, freed by the last of them (`on_answer`, `answer_take`), for the
  three calls of the read and the store; both tests then passed, and under ASan with no report. F2: red first: ui
  `as14_quit_during_read_that_succeeds` (a read that answers after 300 ms, a quit at 100 ms): unmodified, a segmentation fault (the
  paste into the destroyed window); modified, alone, `'easel_canvas_get_selection (...)' should be FALSE` (pasted under the save
  prompt); fixed by dropping the picture in `read_clipboard_picture` when a quit request came; then green: unmodified, the window
  closes; modified, the save prompt, the picture as it was, no selection, one step. Test-side fixes of mine: `paint_red_block` makes
  no step (a Delete does); Undo waits while the prompt is open (Cancel first); the modified case runs first so that the mutant is
  killed by an assertion and not by the crash. F3: core `as7_grown_step_holds_what_it_counts` (held 3153440 bytes, counted 3151936),
  characterization (the shrink was there): proved by the mutant "the grown step keeps its log's spare capacity". Mutants, five new:
  the read's and the store's wait keeping a late callback's result, the late callback keeping its result, a picture pasted after a
  quit request, the grown step's spare capacity; one older mutant's find text brought to the new code ("copied files win over a
  picture on the clipboard"); against the short suite `REQ-0016-15` and `REQ-0016-07`: 34 mutants, 33 killed, 0 by crash, 0
  inconclusive, 0 survived, 1 error (that stale mutant), then `REQ-0016-15` again: 12 of 12 killed, 0 errors; each new one killed by
  its own test's assertion (31-35 s, the core one 1 s). Full isolated suite 45 of 45, warning-free; `mutate.sh --lint` 701, 0 stale;
  `spec.sh lint` and `lint --names 0016` clean; every REQ-0016 id after it: 100 of 100 killed, 0 by crash, 0 inconclusive, 0 survived, 0 errors (1047 s).

## Check
Repeated check (2026-10-26, at `2d119f0`, the committed 0016 with its retrospective; JOD chose a repeated check of 0016 at HEAD):
- Tests: `meson test` 45 of 45 through `tools/isolated.sh`, the long suite included, warning-free.
- Mutants: the whole list, 701 mutants: 700 killed, 0 killed by crash (test gap), 0 inconclusive (no snapshot), 0 survived, 0
  errors, 1 long-only (not run) in 7241 s, as `tools/mutate.sh` reports (isolated). The long-only mutant (REQ-0014-16) applied by hand
  against `ui-0014-long`: killed twice by `as14_long_trace`'s assertion (0.8 s each); the source restored and the suite green again.
- Lints: `tools/spec.sh lint`, `lint --names 0016` and `mutate.sh --lint` (701, 0 stale) clean; `spec.sh status` 18 of 18 tasks; no
  AI trailer in the commits since `1ef33de`.
- Coverage: every requirement has a scenario and a test; REQ-0016-01 (the menus) and REQ-0016-13 (the prompt's look) have no mutant
  and guard against no data loss (`as1_clipboard_menus` and `as12_oversize_prompt` assert them; Cancel has its mutant under -07).
  Two characterizations, each proven by a mutant: `as11_ignored_during_drags` (T005) and `as7_grown_step_holds_what_it_counts` (T018).
- Known note F4 (low, JOD 2026-10-26: recorded, not fixed, since no real user action on the target desktop reaches it): during the
  store's wait at quit a second close request would re-enter `close_request` and start a second store inside the first, and through
  `quit_now` could destroy the window twice. The wait never happens on GNOME 50, which has only the Wayland session: there GDK's store
  fails at once ("This clipboard cannot store data", measured 0.0 ms on JOD's desktop, 2026-10-26). It needs Easel run with
  `GDK_BACKEND=x11` under an X clipboard manager that answers slowly; a test could reach it with the memory clipboard's hanging store
  and a second close request during the wait. A flag while storing that makes `close_request` return TRUE would close it, if X11 ever
  matters.
- Verdict: still `implemented`.

Earlier check:
- Date and commit: 2026-10-26, `1ef33de` (0015's retrospective) plus the uncommitted 0016 work tree (spec, Plan, Tasks T001-T018, code)
- Tests: `meson test` 45 of 45 pass through `tools/isolated.sh`, the long suite `ui-0014-long` included; the cases of this feature: core
  `test-selection` 5 (paste, copy by rows, fit, grown put-down), `test-history` 1 (`as7_grow_step`), io `test-picture-file` 6, ui
  `/0016/` 18; `perf-0016` in the real session (the Plan's Measurements); the probes by hand (T016)
- Mutants: the whole list, 696 mutants: 695 killed, 0 killed by crash (test gap), 0 inconclusive (no snapshot), 0 survived, 0 errors,
  1 long-only (not run) in 6755 s, as `tools/mutate.sh` reports (isolated). The long-only mutant (REQ-0014-16, "a long trace keeps only
  its first 65536 corners") applied by hand and run against `ui-0014-long`: killed twice by `as14_long_trace`'s assertion
  (`selection_is (win, 10, 10, 280, 180)`), 0.8 s each; the source restored and `ui-0014-long` green again (5.4 s)
- Warnings: 0; `tools/spec.sh lint`, `lint --names 0016` and `mutate.sh --lint` (696, 0 stale) clean; no AI trailer in the commits
- Scenarios: the scenario-to-test map of the Plan, each test registered (`lint --names 0016`); no task is a characterization; every
  task and the manual steps M1 and M2 (T016) have a `Result:`
- Findings (2026-10-26), settled in T018 (all three fixed; the waits of the window listed there, none other found):
  - F1 (high, REQ-0016-15, Plan "Clipboard store at quit"): `read_clipboard_picture` and `store_clipboard_for_quit` wait for a callback
    that writes its result into a local variable, and on a time-out cancel and return without waiting for it. A probe (a splice from a
    pipe whose writer never writes, cancelled) called back 5 main-loop iterations after the cancel, so on the desktop the callback of a
    timed-out read writes into a stack slot no longer the function's and leaks a reference. The memory clipboard answers a cancel from
    inside `g_cancellable_cancel`, which hid it; M2's "never" showed no crash. JOD: fix, the wait data on the heap. Fixed in T018:
    the answer on the heap, freed by the last of the wait and the callback; red first by the memory clipboard's late cancel (the
    clipboard kept alive by the late callback's reference) and by AddressSanitizer (`stack-use-after-return`, a WRITE of 8 bytes in
    `on_clipboard_read`), green after and clean under ASan; three mutants killed.
  - F2 (high, REQ-0016-15): a quit request during a read that then succeeds is handled inside `read_clipboard_picture`, before
    `paste_activated` pastes, so the paste goes to a window already destroyed (dispose cleared its canvas) or, with a modified picture,
    lands under the quit prompt. `as14_clipboard_wait` tried the quit only with a read that fails. A spec gap ("handled after it" did
    not say whether the paste is made): JOD chose (a), the picture that arrives is dropped and nothing is pasted; REQ-0016-15 worded so.
    Fixed in T018: red first (a segmentation fault unmodified, a paste under the prompt modified), green after; one mutant killed.
  - F3 (medium, REQ-0016-07, REQ-0002-16): `push_step` in `selection.c` shrinks the grown step's log to its length before
    `easel_history_push_grow` (a copy of the change log's own shrink), and no test compares the memory held with the memory counted for
    that step, and no mutant removes the shrink. JOD: a held-versus-counted test and the mutant. Done in T018:
    `as7_grown_step_holds_what_it_counts` (a characterization) and the mutant that removes the shrink, killed.
- After T018: full isolated suite 45 of 45, warning-free; `REQ-0016-15` and `REQ-0016-07` against the short suite, 34 mutants then the
  stale one fixed and `REQ-0016-15` again 12 of 12, 0 by crash, 0 inconclusive, 0 survived; `mutate.sh --lint` 701, 0 stale;
  `spec.sh lint` and `lint --names 0016` clean; then every REQ-0016 id: 100 mutants: 100 killed, 0 killed by crash (test gap), 0
  inconclusive (no snapshot), 0 survived, 0 errors in 1047 s, as `tools/mutate.sh` reports (isolated).
- After T018, the mutants of other requirements (JOD, 2026-10-26): none lies in the code T018 changed (`window.c` only; `selection.c`
  and the clipboard seam unchanged by it; the 9 mutants inside the changed functions are all REQ-0016-15). As a margin, every id of
  REQ-0005 and REQ-0013 with a mutant in `window.c` (20 ids, the quit path shared): 80 mutants: 79 killed, 1 killed by crash (test
  gap), 0 inconclusive (no snapshot), 0 survived, 0 errors in 606 s. The crash: REQ-0005-15 "a second launch replaces the window of
  the first" (`application.c`, not touched by 0016's T018), `not ok /0005/as16_quality_setting_reaches_save - GLib-GObject-FATAL-CRITICAL:
  object_ref: assertion '!object_already_finalized' failed`, in a case registered before `as13_command_line`, the one that kills it;
  run again alone three times: killed 3 of 3 by `as13_command_line`'s assertion (`launch_with (again, 1) == win`), as in the check's
  whole-list run. An intermittent crash of `as16` under that mutant, seen once in five runs; its saved log was cleared by the re-runs
  (`mutate.sh` empties its folder), so the stack is not kept. Not hunted further: it needs the mutant to happen, and it belongs to 0005.
- Final numbers, for the code as it is after the fixes of M1 and M2 (the PNG written whole, `easel_picture_encode_fast_png`, the
  `slow-clipboard-owner` helper) and T018, 2026-10-26: `meson test` 45 of 45 through `tools/isolated.sh`, the long suite included,
  warning-free; `mutate.sh` for every REQ-0016 id and for REQ-0005-10, -11, -14 and -18 (their mutants quote `encode_with` of
  `picture-file.c`, the hunk the fast PNG changed; a grep of every mutant's find text against the changed functions of `picture-file.c`,
  `clipboard.c` and `window.c` found no other requirement): 109 mutants: 109 killed, 0 killed by crash (test gap), 0 inconclusive (no
  snapshot), 0 survived, 0 errors in 1083 s, as `tools/mutate.sh` reports (isolated); `mutate.sh --lint` 701, 0 stale; `tools/spec.sh
  lint` and `lint --names 0016` clean.
- Verdict: clean after T018 and JOD's M1 and M2 on that build; `Status: implemented` (JOD, 2026-10-26).

Equivalence notes (T015, 2026-10-24; not findings): one check of the code is made twice on purpose and no input can tell either of the two
mutants apart, so neither is in `tests/mutants.txt`:
- Fit's memory limit is checked twice, by the pre-check `easel_canvas_can_paste (self->canvas, nw, nh, 0, 0)` of `on_paste_prompt_answered`
  and by the limit handed to `easel_selection_scale`. Each refusal gives the same dialog and the same state (nothing changed, the tool
  included, because the fitted picture is made before the tool changes), so removing either leaves the other. Find
  `if (easel_canvas_can_paste (self->canvas, nw, nh, 0, 0)\n          && easel_selection_scale (pic->px, NULL, pic->width, pic->height, nw, nh, &px, NULL,\n                                    easel_canvas_get_selection_limit (self->canvas)) == EASEL_SELECTION_DONE)`;
  replace 1 `if (easel_selection_scale (…, easel_canvas_get_selection_limit (self->canvas)) == EASEL_SELECTION_DONE)` (the pre-check
  gone); replace 2 `if (easel_canvas_can_paste (self->canvas, nw, nh, 0, 0) && easel_selection_scale (…, SIZE_MAX) == EASEL_SELECTION_DONE)`
  (the limit of the scale gone).
- The growth is NOT such a pair (JOD's note counted it as one): the pre-check of `easel_canvas_can_paste` and the real allocation in
  `easel_canvas_paste_grown` both refuse it, but the tool is switched to Select between them, so removing the pre-check leaves the Pencil
  changed to Select after a refused Resize. The first version of `as10_clipboard_memory` refused it with Select already active and could
  not see that; it now uses the Pencil and has the mutant "a refused Resize has already changed the tool".
- The picture-type test in the store at quit (`if (!EASEL_IS_CLIPBOARD_PICTURE (content)) return;`) was removed, as it was observable by
  no input (the window only ever sets pictures) and would have been wrong once 0017 puts text there.
