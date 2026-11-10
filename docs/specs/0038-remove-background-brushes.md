# 0038 Remove Background: Keep this and Remove this

Status: accepted
Reviewed: spec-reviewer, 2026-11-10
Depends on: 0006, 0022, 0025, 0035, 0037

## Why
Remove Background (0035) finds the background from the picture's border. When the subject fills a side of the border (a selfie
whose shirt fills the bottom edge, JOD 2026-11-10), it takes the subject for background, and a press on a piece (0037) cannot help:
the face, the shirt and the garden are one removed piece, so a press brings back all of it or none (JOD's T005, 2026-11-10: pieces
are all or nothing on such a photo). Two brushes let the user say it directly: paint over what to keep and
over what to remove, and the method grows those marks out to the edges it finds (`docs/phase2-plan.md` G; JOD's design notes of
2026-11-10). A centre seed and a rule on the border's sides were tried and dropped (`docs/reference/0035/methods-comparison.md`).

## Requirements
- REQ-0038-01 (Adds to REQ-0035-04.) Remove Background's card shall have, between the Tolerance row and the buttons, two toggle buttons side by side, "Keep this" and "Remove this", with the tooltips "Paint over parts to keep" and "Paint over parts to remove", both off each time the card opens. Turning one on turns the other off; pressing the one that is on turns it off, so at most one is on. The focus order is the Tolerance field, Keep this, Remove this, Cancel, OK (OK stays the last, REQ-0037-03). Their mnemonics differ from each other and from every other mnemonic of the card (Tolerance's, Cancel's, OK's) in every language. While a stroke is held, turning a brush on or off (a click, its mnemonic) does nothing. (New.)
- REQ-0038-02 (Changes REQ-0037-01 while a brush is on.) While Keep this or Remove this is on, a drag of the primary button over the canvas area shall paint a stroke instead of flipping a piece: the stroke covers every picture pixel the command looks at (REQ-0035-02) whose centre lies within half the brush's width of the path joining the centres of the picture pixels the pointer passes (each mapped as REQ-0025-02 maps a press, the path between them straight). The width is set at the press in whole picture pixels: 16 logical pixels on the screen divided by the zoom then (JOD, 2026-11-10: fixed, no size control), rounded to the nearest, at least 1, and kept for the stroke (a later zoom does not change it). A press with no motion paints a round dot, which always covers the pressed pixel (at 100 % and a width of 16: the 197 pixels within 8 of its centre). Parts of the path beside the picture or outside the selection's shape cover nothing. While no brush is on, a press flips a piece as REQ-0037-01 says. Neither paints with the secondary or middle button.
- REQ-0038-03 The method (REQ-0035-03) shall take the strokes as the user's word, the later stroke winning where two cover the same pixel: (b) a copy cell holding a pixel covered by a stroke takes the kind of the latest stroke covering any of its pixels: Keep this a subject seed, Remove this a background seed, in place of what its color would make it, and the seam does not change it; a cell no stroke covers that is joined to a Keep this stroke is a subject seed too, never a background seed, also in the ring (JOD, 2026-11-10: the border's background seeds joined to a Keep stroke stop being background seeds; made subject seeds at the spec's writing, since unseeding alone kept 69 % of the synthetic's shirt, the fringe eating the rest). Joined: reachable from a cell the stroke covers by a path of 4-connected copy cells, each within the Tolerance shown (delta-E) of the stroke's color, the mean Lab of the copy cells the stroke covers (each Keep this stroke its own; the color is the stroke's, not the previous cell's, so the path cannot drift along a gradient); (c) a subject region holding a Keep this cell is kept whatever its size, and an enclosed background region holding a Remove this cell is not made subject; (d) at full size a pixel covered by a stroke takes the stroke's kind; (g) the fringe does not remove a pixel covered by Keep this; (e) a fully transparent pixel stays as it is under either. A piece of REQ-0037-01 is made from the method with the strokes; the presses of REQ-0037-04 then flip as there, except that a press whose pixel a later stroke covers is dropped (the later word wins, a press or a stroke).
- REQ-0038-04 After a stroke ends (the button released, or its release lost, REQ-0007-18: the stroke ends at the last pointer place), the preview shall be made with all strokes, then the presses, within 0.3 s on the dev machine at any size; during the drag the preview stays as it was and the band of the stroke in progress grows under the pointer. A stroke begun while the field holds typed text first applies the number (as REQ-0037-05); one begun while a preview is still to be made paints over the preview for the number shown, once made; one begun while the result is made at OK (REQ-0035-11) paints nothing. The strokes stay when the Tolerance changes (each new preview uses them). A stroke gives no undo step of the picture and does not change the modified mark; OK keeps the result as shown (REQ-0035-02's one step), Cancel and Esc drop the strokes with everything else (REQ-0035-04). Ctrl+Z while the card is open removes the last press or stroke, whichever came last, and makes the preview without it, again for the one before, and does nothing when none is left; it never changes the Tolerance; with the focus in the Tolerance field it is the field's own (its typed text); there is no redo. A Tolerance change drops the presses (REQ-0037-04) and keeps the strokes, so Ctrl+Z after it takes back strokes only. (JOD, 2026-11-10, after T005; before: the last stroke only, never a press; in the draft: no taking back.)
- REQ-0038-05 While a brush is on, the strokes shall show over the picture as translucent bands made of the covered pixels' squares at the current zoom: Keep this in the style's success color (green), Remove this in its destructive color (red), at 40 % opacity, a later stroke drawn over an earlier one, each stroke's covered area outlined with a black line and a white line beside it as the footprint outline of REQ-0006-16 is; with no brush on they are hidden, so the preview shows bare. The bands are not part of the picture. While a brush is on the pointer over the canvas area is the outline of the dot a press would paint there (black and white as REQ-0006-16, with no minimum size); with none on, the default arrow (REQ-0037-07).
- REQ-0038-06 The keyboard shall reach the same (REQ-0022-05): with the focus on the canvas (REQ-0037-03) and a brush on, Space pressed paints a dot at the marker's pixel and begins a stroke; while Space is held each arrow (REQ-0037-03's marker step) moves the marker and extends the stroke to its new pixel; Space released ends the stroke as a release does (REQ-0038-04); key repeats of Space do nothing; Esc while Space is held drops that stroke (REQ-0038-07). With no brush on, Space flips the piece under the marker as REQ-0037-03 says. (JOD, 2026-11-10; before, in the draft: open.) Turning a brush on or off announces "Keep this on", "Remove this on" or "Brushes off" (REQ-0022-15).
- REQ-0038-07 The view shall scroll and zoom during a stroke as REQ-0037-06 says; the stroke goes on under the pointer at its picture pixel (REQ-0025-02), its width as at the press. A window resize during a stroke changes nothing of it. Esc or a press of the secondary button during a stroke drops that stroke only, as REQ-0022-10 drops a drag; Enter during a stroke does nothing; a lost release (REQ-0007-18) or the window losing the focus ends the stroke at its last place, as a release does; Ctrl+Q, Ctrl+W and a close request are as REQ-0035-12. The wheel over the Tolerance field does nothing while a stroke is held.
- REQ-0038-08 If the memory for a stroke (at its press or as it grows) or for its preview cannot be had, the message of REQ-0035-06 shall show, the stroke be dropped (the rest of the drag paints nothing) and the card stay open with the preview as before it. The strokes' total memory counts in REQ-0035-09's 1 GB. With 100 strokes of 1 000 pointer places each, a preview shall show within 0.3 s on the dev machine at 8000 x 8000.
- REQ-0038-10 (Changes REQ-0035-04's second line.) The card's second line for a complex background shall read "This background is complex. Lower the Tolerance, then use Keep this and Remove this." in every language (before: "... then click pieces to fix them."); it changes with the brushes, not before (JOD, 2026-11-10).
- REQ-0038-09 The pixel contract (P7): (1) every pixel is exactly as before the card or 0x00000000 (REQ-0035-07, P7.7, 2026-11-10; before: alpha 0 with its color kept); (2) the view only magnifies: each pointer place is mapped to its picture pixel as REQ-0025-02 maps a press, and a stroke is whole picture pixels and a whole width (P7.4 and P7.6 exception: the width depends on the zoom at the press, by the design of a brush sized on the screen; once set, the stroke is the same at any zoom); (3) the preview and the result are made by the same code from the same strokes (P7.3; over 2 000 000 pixels a copy cell stands for its block, as ADR-0007 allows); (4) the same pixels, Tolerance, strokes (their kinds, widths and places, in order) and presses give the same pixels on every machine.

## Scenarios
- AS-1 (REQ-0038-01) Given the card open, Then Keep this and Remove this are off, side by side between the Tolerance row and the buttons, with their tooltips; When Keep this, Then it is on; When Remove this, Then it is on and Keep this off; When Remove this again, Then both are off; Tab goes Tolerance, Keep this, Remove this, Cancel, OK; their mnemonics differ from each other and from the card's other mnemonics in every language; a click on a brush during a stroke does nothing | Verified by: automated (ui)
- AS-2 (REQ-0038-02, REQ-0038-03, REQ-0038-04) Given AS-11's picture of 0035 (the selfie-like synthetic, whose face and shirt the method removes, which AS-2's test asserts first), When Keep this and a dot at (120, 290) on the shirt at 100 %, Then after the release the shirt is kept (its pixels at least 95 %, counted away from the true edge as AS-2 of 0035 counts) and the sky and foliage removed (at least 95 %), no step, no mark; the face, joined to nothing (sky lies between), stays as the method makes it (the prototype, 2026-11-10: shirt 100 %, background 97.6 %); When a second dot on the face, Then the face is kept too; When OK, Then one step and the result as shown | Verified by: automated (ui, core)
- AS-3 (REQ-0038-03) Given an 80 x 60 white picture with a red rectangle from (20, 15) to (59, 44), at 100 %, When Remove this and a stroke from (25, 30) to (35, 30) (width 16), Then the red pixels it covers are removed and every other red pixel kept; When Keep this on the dot at (30, 30), Then the pixels of that dot are kept and the rest of the first stroke still removed (the later stroke wins); Given a 200 x 200 red square with a white square hole 8 x 8 in its middle (filled by the method, under 1 %), When Remove this on the hole, Then the hole is removed; Given a press that removed an island, When Keep this over it, Then it is kept (the press dropped); Given a fully transparent pixel under Keep this, Then it stays transparent | Verified by: automated (core, ui)
- AS-4 (REQ-0038-02) Given a brush on, When a press with no motion at 100 %, Then the 197 pixels within 8 of the pressed pixel's centre are covered and no piece flips; at 3200 %, Then the pressed pixel alone; When a drag beside the picture, Then nothing is painted; Given a marked free-form selection, When a stroke across its edge, Then only pixels inside the shape are covered; When both brushes are off, Then a press flips a piece as in 0037 | Verified by: automated (ui)
- AS-5 (REQ-0038-04, REQ-0037-04) (2026-11-10: the press is dropped; before: it stayed too.) Given a stroke and a press on an island, When the Tolerance changes by 1 and back, Then the stroke's pixels are as before and the island as the method makes it (the press dropped); When Cancel, Then the picture as before the card, no step | Verified by: automated (ui)
- AS-6 (REQ-0038-05, REQ-0038-07) Given Keep this on and a stroke, Then the band shows green with its outline at 100 % and at 400 %; When both off, Then no band; the pointer is the brush's outline with a brush on and the arrow with none; When Ctrl+wheel during a stroke, Then the view zooms and the stroke goes on under the pointer at the width it began with; When Esc during a stroke, Then that stroke is dropped and the card stays; When the button's release is lost, Then the stroke ends as released; When Ctrl+Q during a stroke on a modified picture, Then the card closes as Cancel and the Save prompt shows | Verified by: automated (ui, rendering the window) + manual M2 (JOD's look at the card and the bands)
- AS-7 (REQ-0038-06) Given the focus on the canvas and Keep this on, When Space is pressed at the marker, Right is pressed 10 times while Space is held and Space is released, Then the stroke covers what a mouse drag along the same 11 pixels covers, and key repeats of Space change nothing; Given no brush on, When Space, Then the piece under the marker flips; "Keep this on" is announced when it is turned on | Verified by: automated (ui) + manual M1 (the real keyboard)
- AS-8 (REQ-0038-08) Given memory made too small after the card opened, When a stroke, Then the message shows and the preview is as before it; Given an 8000 x 8000 picture with 100 strokes of 1 000 places, When a Tolerance change, Then the preview within 0.3 s | Verified by: automated (ui) + automated (perf, in the real session)
- AS-9 (REQ-0038-09) Given a picture over 2 000 000 pixels and a stroke, When OK, Then every pixel the stroke covers is of its kind and the same pixels in a second process | Verified by: automated (core)
- AS-11 (REQ-0038-03) Given a 200 x 100 picture: a grey ramp from (60, 60, 60) at the left edge to (200, 200, 200) at the right, its rows the same, a red disk of radius 20 at (100, 50), When Keep this and a dot at (5, 50) at 100 % and Tolerance 5, Then the cells joined to it are only those within 5 delta-E of the dot's mean color (the ramp's left part, not all of it) and the ramp's right end is removed | Verified by: automated (core)
- AS-12 (REQ-0038-04) Given a press on an island, then two strokes, then a second press, When Ctrl+Z with the focus on the canvas, Then the second press is taken back; When Ctrl+Z again, Then the preview is as with the first stroke and the first press; When Ctrl+Z twice more, Then as with none; When Ctrl+Z again, Then nothing changes; Given the focus in the Tolerance field with typed text, When Ctrl+Z, Then the field's text is undone and the strokes and presses stay | Verified by: automated (ui)
- AS-13 (REQ-0038-10) Given AS-11's picture of 0035, When Remove Background, Then the second line reads "This background is complex. Lower the Tolerance, then use Keep this and Remove this."; its strings in every language | Verified by: automated (ui) + the translation check
- AS-10 (REQ-0038-03) Given JOD's selfie at its automatic tolerance (12), When Keep this and one dot on the shirt, Then the shirt, the beard, the face and the hat are kept and most of the garden removed (the prototype, 2026-11-10: 59 % of the picture kept, the face back) | Verified by: manual M1 (JOD's photo, not in the repository)

## Edge cases
- A stroke over pixels the command does not look at only: covers nothing; still a stroke for the order of later ones.
- A stroke entirely of one kind over the whole picture: Keep this keeps all (OK then is as REQ-0035-05 if nothing is removed); Remove this removes every pixel not fully transparent.
- A press on a piece (brushes off) after strokes: the piece is the method's with the strokes, flipped as 0037 says.
- A brush turned on or off during a stroke (a click, a mnemonic): does nothing (REQ-0038-01).
- A selection float moved: cannot happen while the card is open (the canvas takes no other input).

- Help: the Image menu section's Remove Background paragraph names the two brushes, in every language (REQ-0035-10), decided routinely.

## Out of scope
- An eraser for strokes, a size control (JOD: fixed width), a redo of Ctrl+Z, soft edges (H), a texture channel (I), any graph cut.

## Open questions
None. Settled by JOD on 2026-11-10: a fixed width of 16 logical pixels; Ctrl+Z inside the card removes the last stroke (after T005: the last press or stroke); Space held
paints at the marker while the arrows move it, and flips a piece with no brush on; the hint line names the brushes, changed with this
feature; the border's background seeds joined to a Keep stroke stop being seeds (defined in REQ-0038-03; made subject seeds, and the
synthetic's face left out of AS-2, both from the prototype at the spec's writing, `docs/learning-log.md`). The spec reviewer's
fifteen findings settled routinely (2026-11-10), listed under the Plan.

## Plan
(Written with the spec at JOD's request, 2026-11-10; JOD's go on the Plan and the Tasks still due before building.) Spec reviewer, 2026-11-10: fifteen findings merged routinely (integer strokes with the P7 exception stated, the cell under both kinds, a press against a later stroke, Esc and the lost release during a stroke, toggles held, the wheel over the field, typed text and pending previews, memory as a stroke grows, the dot's minimum, concrete AS-3 and AS-4, the bands' look, mnemonics, Help) and one asked (JOD's answer 5). Pointer notes added at acceptance to REQ-0035-04, REQ-0037-01 and REQ-0037-07, and after T005 (2026-11-10) to REQ-0037-04 (presses dropped on a Tolerance change, strokes kept) and REQ-0037-06 (Ctrl+Z takes back the last press or stroke).

- Core `src/core/remove-bg.{c,h}`: `EaselRbgStroke { bool keep; int width; size_t n; const int32_t *xy; }` (whole picture
  pixels, P7.4). `easel_rbg_set_strokes (r, strokes, n)` rasterizes them into a mark per copy cell (1 byte: none,
  keep, remove; the later stroke wins), counted in the analysis's bytes; `easel_rbg_label` seeds from the marks first (b), skips
  marked cells in the seam, keeps marked regions in (c); `easel_rbg_result` sets a covered pixel's kind after the refinement (d) and
  shields Keep pixels from the fringe (g), testing coverage per pixel only inside each stroke's bounding box. Coverage: a pixel's
  centre within width/2 of a segment, in 64-bit integers (squared distances, no square root). Pieces (0037) follow from the labels
  with no change.
- Canvas `src/ui/canvas.{c,h}`: the press hook of 0037 grows into a stroke hook (begin, extend, end, lost release) taking the primary
  drag while a brush is on, mapped by `to_pixel`; `easel_canvas_set_marks` gives it the bands to draw over the
  picture in its snapshot (after the picture, before the selection outline), and the round brush cursor outline drawn as the
  footprint outline is (REQ-0006-16).
- Card `src/ui/remove-bg-card.{c,h}`: two `GtkToggleButton`s with mnemonics, not in a GTK group (a group cannot be all off);
  `toggled` turns the other off; a callback tells the window the brush (none, keep, remove).
- Window `src/ui/window.c`: the session keeps the strokes (growing arrays counted against the limit, shrunk at stroke end), calls
  `easel_rbg_set_strokes` then label (the joined cells found per Keep stroke by a fill over the copy within the Tolerance of the
  stroke's mean, in the label step since they depend on the Tolerance), preview and flips after each stroke end and each Tolerance change; OK's worker does the same at
  full size; Cancel frees them. Keys: Space down/up and the arrows at the marker (REQ-0038-06), Ctrl+Z to the
  last press or stroke (one list of both, in order) unless the field has the focus; the announcements.
- What grows: per stroke 24 bytes plus 8 bytes per pointer place (two coordinates), a pixel repeated not stored again; the marks, 1 byte a copy cell (at most 2 MB).
- Strings: "Keep this", "Remove this", the two tooltips, the three announcements, the new hint line (REQ-0038-10, the old one
  dropped), the Help paragraph, in 11 languages; the mnemonic check of 0023.

| Scenario | Test |
|---|---|
| AS-1 | ui `/0038/as1_toggles` + `/0023-lang` mnemonic case |
| AS-2 | core `strokes_selfie` + ui `/0038/as2_keep_stroke` |
| AS-3 | core `strokes_rules` + ui `/0038/as3_remove_and_order` |
| AS-4 | ui `/0038/as4_dot_and_shape` |
| AS-5 | ui `/0038/as5_tolerance_and_cancel` |
| AS-6 | ui `/0038/as6_bands_and_zoom` (rendered) + manual M2 |
| AS-7 | ui `/0038/as7_keyboard` + manual M1 |
| AS-8 | ui memory case + `perf-0035` (100 strokes) |
| AS-9 | core `strokes_large` |
| AS-10 | manual M1 |
| AS-11 | core `strokes_joined` |
| AS-12 | ui `/0038/as12_ctrl_z` |
| AS-13 | ui `/0038/as13_hint` + `translations` |

Constitution: P7 as REQ-0038-09, ADR-0007's exception over 2 000 000 pixels unchanged; no graph cut (JOD's rule); layering: the
coverage and the seeds in core, the drag and the bands in ui.

Risks:
- Two toggle buttons that may both be off: GTK's grouped toggles cannot; probe ungrouped ones with a `toggled` handler in the first
  task.
- The canvas's drag gesture and the press hook of 0037 on one button: the first canvas task probes that a drag with a brush on never
  flips a piece and a press with none never paints.
- Space held on the canvas: GTK delivers key repeats as presses; the keyboard task checks that a repeat neither ends nor restarts the
  stroke (REQ-0038-06).
- The numbers in AS-2 were measured with the core before the automatic tolerance came from the clusters (0035, 2026-11-10): the selfie-like synthetic's automatic tolerance is now 5, not 20; T001 measures AS-2 again before its test relies on them.
- The preview within 0.3 s at 8000 x 8000 with strokes: the label alone takes 144 to 149 ms there now; the rasterizing of 100 x 1 000
  places into 2 000 000 cells is measured in the core task that adds it, before the ui relies on it.

## Tasks
(added after the Plan is agreed)

## Check
