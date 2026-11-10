# 0038 Remove Background: Keep this and Remove this

Status: implemented
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
- REQ-0038-03 The method (REQ-0035-03) shall take the strokes as the user's word, the later stroke winning where two cover the same pixel: (b) a copy cell holding a pixel covered by a stroke takes the kind of the latest stroke covering any of its pixels: Keep this a subject seed, Remove this a background seed, in place of what its color would make it, and the seam does not change it; a cell no stroke covers that is joined to a Keep this stroke is a subject seed too, never a background seed, also in the ring (JOD, 2026-11-10: the border's background seeds joined to a Keep stroke stop being background seeds; made subject seeds at the spec's writing, since unseeding alone kept 69 % of the synthetic's shirt, the fringe eating the rest). Joined: reachable from a cell the stroke covers by a path of 4-connected copy cells, each within the Tolerance shown (delta-E) of the stroke's color, the mean Lab of the copy cells the stroke covers (each Keep this stroke its own; the color is the stroke's, not the previous cell's, so the path cannot drift along a gradient); (c) a subject region holding a Keep this cell is kept whatever its size, and an enclosed background region holding a Remove this cell is not made subject; (d) at full size a pixel covered by a stroke takes the stroke's kind; (g) the fringe does not remove a pixel covered by Keep this; (e) a fully transparent pixel stays as it is under either. A piece of REQ-0037-01 is made from the method with the strokes; the presses of REQ-0037-04 then flip as there, except that a press whose pixel a later stroke covers is dropped, and a pixel of a flipped piece that a stroke made after its press covers keeps that stroke's kind: each pixel takes the latest word on it, a stroke covering it or a press whose piece holds it (JOD, 2026-11-10, at the check; before: only the press's own pixel was looked at, so a later stroke over the rest of a pressed piece did nothing; over 2 000 000 pixels "made after" is read per copy cell, its latest stroke, as REQ-0038-09 (3) allows).
- REQ-0038-04 After a stroke ends (the button released, or its release lost, REQ-0007-18: the stroke ends at the last pointer place), the preview shall be made with all strokes, then the presses, within 0.3 s on the dev machine at any size; during the drag the preview stays as it was and the band of the stroke in progress grows under the pointer. A stroke begun while the field holds typed text first applies the number (as REQ-0037-05); one begun while a preview is still to be made paints over the preview for the number shown, once made; one begun while the result is made at OK (REQ-0035-11) paints nothing. The strokes stay when the Tolerance changes (each new preview uses them). A stroke gives no undo step of the picture and does not change the modified mark; OK keeps the result as shown (REQ-0035-02's one step), Cancel and Esc drop the strokes with everything else (REQ-0035-04). Ctrl+Z while the card is open removes the last press or stroke, whichever came last, and makes the preview without it, again for the one before, and does nothing when none is left; it never changes the Tolerance; with the focus in the Tolerance field it is the field's own (its typed text); there is no redo. A Tolerance change drops the presses (REQ-0037-04) and keeps the strokes, so Ctrl+Z after it takes back strokes only. (JOD, 2026-11-10, after T005; before: the last stroke only, never a press; in the draft: no taking back.)
- REQ-0038-05 While a brush is on, the strokes shall show over the picture as translucent bands made of the covered pixels' squares at the current zoom: Keep this in the style's success color (green), Remove this in its destructive color (red), at 40 % opacity, a later stroke drawn over an earlier one, each stroke's covered area outlined with a black line and a white line beside it as the footprint outline of REQ-0006-16 is; with no brush on they are hidden, so the preview shows bare. The bands are not part of the picture. While a brush is on the pointer over the canvas area is the outline of the dot a press would paint there (black and white as REQ-0006-16, with no minimum size); with none on, the default arrow (REQ-0037-07).
- REQ-0038-06 The keyboard shall reach the same (REQ-0022-05): with the focus on the canvas (REQ-0037-03) and a brush on, Space pressed paints a dot at the marker's pixel and begins a stroke; while Space is held each arrow (REQ-0037-03's marker step) moves the marker and extends the stroke to its new pixel; Space released ends the stroke as a release does (REQ-0038-04); key repeats of Space do nothing; Esc while Space is held drops that stroke (REQ-0038-07). With no brush on, Space flips the piece under the marker as REQ-0037-03 says. (JOD, 2026-11-10; before, in the draft: open.) Turning a brush on or off announces "Keep this on", "Remove this on" or "Brushes off" (REQ-0022-15).
- REQ-0038-07 The view shall scroll and zoom during a stroke as REQ-0037-06 says; the stroke goes on under the pointer at its picture pixel (REQ-0025-02), its width as at the press. A window resize during a stroke changes nothing of it. Esc or a press of the secondary button during a stroke drops that stroke only, as REQ-0022-10 drops a drag; Enter during a stroke does nothing; a lost release (REQ-0007-18) or the window losing the focus ends the stroke at its last place, as a release does; Ctrl+Q, Ctrl+W and a close request are as REQ-0035-12. The wheel over the Tolerance field does nothing while a stroke is held.
- REQ-0038-08 If the memory for a stroke (at its press or as it grows) or for its preview cannot be had, the message of REQ-0035-06 shall show, the stroke be dropped (the rest of the drag paints nothing) and the card stay open with the preview as before it. The strokes' total memory counts in REQ-0035-09's 1 GB. With 100 strokes of 1 000 pointer places each, a preview shall show within 0.3 s on the dev machine at 8000 x 8000.
- REQ-0038-10 (Changes REQ-0035-04's second line.) The card's second line for a complex background shall read "This background is complex. Lower the Tolerance, then use Keep this and Remove this." in every language (before: "... then click pieces to fix them."); it changes with the brushes, not before (JOD, 2026-11-10).
- REQ-0038-09 The pixel contract (P7): (1) every pixel is exactly as before the card or 0x00000000 (REQ-0035-07, P7.7, 2026-11-10; before: alpha 0 with its color kept); (2) the view only magnifies: each pointer place is mapped to its picture pixel as REQ-0025-02 maps a press, and a stroke is whole picture pixels and a whole width (P7.4 and P7.6 exception: the width depends on the zoom at the press, by the design of a brush sized on the screen; once set, the stroke is the same at any zoom); (3) the preview and the result are made by the same code from the same strokes (P7.3; over 2 000 000 pixels a copy cell stands for its block, as ADR-0007 allows); (4) the same pixels, Tolerance, strokes (their kinds, widths and places, in order) and presses give the same pixels on every machine.

## Scenarios
- AS-1 (REQ-0038-01) Given the card open, Then Keep this and Remove this are off, side by side between the Tolerance row and the buttons, with their tooltips; When Keep this, Then it is on; When Remove this, Then it is on and Keep this off; When Remove this again, Then both are off; Tab goes Tolerance, Keep this, Remove this, Cancel, OK; their mnemonics differ from each other and from the card's other mnemonics in every language; a click on a brush during a stroke does nothing | Verified by: automated (ui)
- AS-2 (REQ-0038-02, REQ-0038-03, REQ-0038-04) (2026-11-10, T001, JOD's yes: re-measured on the current core; before: "Given AS-11's picture of 0035 (the selfie-like synthetic, whose face and shirt the method removes ...), When Keep this and a dot at (120, 290) on the shirt at 100 %, Then the shirt is kept (at least 95 %) and the sky and foliage removed (at least 95 %) ...; When a second dot on the face, Then the face is kept too") Given 0035 AS-11's picture at its automatic tolerance (5), whose shirt the method removes and whose face it keeps, When Keep this and a dot at (120, 290) at 100 %, Then after the release the whole shirt is kept and no other pixel changes, no step, no mark; When Remove this and a dot at (33, 120) at 100 % on the foliage the method leaves kept, Then the pixels the dot covers are removed and the face and the shirt are unchanged; When OK, Then one step and the result as shown | Verified by: automated (ui, core)
- AS-3 (REQ-0038-03) Given an 80 x 60 white picture with a red rectangle from (20, 15) to (59, 44), at 100 %, When Remove this and a stroke from (25, 30) to (35, 30) (width 16), Then the red pixels it covers are removed and every other red pixel kept; When Keep this on the dot at (30, 30), Then the pixels of that dot are kept and the rest of the first stroke still removed (the later stroke wins); Given a 200 x 200 red square with a white square hole 8 x 8 in its middle (filled by the method, under 1 %), When Remove this on the hole, Then the hole is removed; Given a press that removed an island, When Keep this over it, Then it is kept (the press dropped); Given a fully transparent pixel under Keep this, Then it stays transparent | Verified by: automated (core, ui)
- AS-4 (REQ-0038-02) Given a brush on, When a press with no motion at 100 %, Then the 197 pixels within 8 of the pressed pixel's centre are covered and no piece flips; at 3200 %, Then the pressed pixel alone; When a drag beside the picture, Then nothing is painted; Given a marked free-form selection, When a stroke across its edge, Then only pixels inside the shape are covered; When both brushes are off, Then a press flips a piece as in 0037 | Verified by: automated (ui)
- AS-5 (REQ-0038-04, REQ-0037-04) (2026-11-10: the press is dropped; before: it stayed too.) Given a stroke and a press on an island, When the Tolerance changes by 1 and back, Then the stroke's pixels are as before and the island as the method makes it (the press dropped); When Cancel, Then the picture as before the card, no step | Verified by: automated (ui)
- AS-6 (REQ-0038-05, REQ-0038-07) Given Keep this on and a stroke, Then the band shows green with its outline at 100 % and at 400 %; When both off, Then no band; the pointer is the brush's outline with a brush on and the arrow with none; When Ctrl+wheel during a stroke, Then the view zooms and the stroke goes on under the pointer at the width it began with; When Esc during a stroke, Then that stroke is dropped and the card stays; When the button's release is lost, Then the stroke ends as released; When Ctrl+Q during a stroke on a modified picture, Then the card closes as Cancel and the Save prompt shows | Verified by: automated (ui, rendering the window) + manual M2 (JOD's look at the card and the bands)
- AS-7 (REQ-0038-06) Given the focus on the canvas and Keep this on, When Space is pressed at the marker, Right is pressed 10 times while Space is held and Space is released, Then the stroke covers what a mouse drag along the same 11 pixels covers, and key repeats of Space change nothing; Given no brush on, When Space, Then the piece under the marker flips; "Keep this on" is announced when it is turned on | Verified by: automated (ui) + manual M1 (the real keyboard)
- AS-8 (REQ-0038-08) Given memory made too small after the card opened, When a stroke, Then the message shows and the preview is as before it; Given an 8000 x 8000 picture with 100 strokes of 1 000 places, When a Tolerance change, Then the preview within 0.3 s | Verified by: automated (ui) + automated (perf, in the real session)
- AS-9 (REQ-0038-09) Given a picture over 2 000 000 pixels and a stroke, When OK, Then every pixel the stroke covers is of its kind and the same pixels in a second process | Verified by: automated (core)
- AS-11 (REQ-0038-03) Given a 200 x 100 picture: a grey ramp from (60, 60, 60) at the left edge to (200, 200, 200) at the right, its rows the same, a red disk of radius 20 at (100, 50), When Keep this and a dot at (5, 50) at 100 % and Tolerance 5, Then the cells joined to it are only those within 5 delta-E of the dot's mean color (the ramp's left part, not all of it) and the ramp's right end is removed | Verified by: automated (core)
- AS-14 (REQ-0038-03, REQ-0038-04) (JOD, 2026-11-10, at the check) Given a press that removed an island, When Keep this over a part of the island away from the pressed pixel, Then that part is kept and the rest of the island stays removed; When Ctrl+Z, Then the whole island is removed again; Given a Keep stroke over part of an island, When a press on the island, Then the whole island is removed (the press is later) | Verified by: automated (core, ui)
- AS-12 (REQ-0038-04) Given a press on an island, then two strokes, then a second press, When Ctrl+Z with the focus on the canvas, Then the second press is taken back; When Ctrl+Z again, Then the preview is as with the first stroke and the first press; When Ctrl+Z twice more, Then as with none; When Ctrl+Z again, Then nothing changes; Given the focus in the Tolerance field with typed text, When Ctrl+Z, Then the field's text is undone and the strokes and presses stay | Verified by: automated (ui)
- AS-13 (REQ-0038-10) Given AS-11's picture of 0035, When Remove Background, Then the second line reads "This background is complex. Lower the Tolerance, then use Keep this and Remove this."; its strings in every language | Verified by: automated (ui) + the translation check
- AS-10 (REQ-0038-03) (2026-11-10, T001, JOD's yes: the automatic tolerance is now 7; before: "(12) ... the shirt, the beard, the face and the hat kept ... 59 % of the picture kept") Given JOD's selfie at its automatic tolerance (7), whose face and beard the method keeps and whose shirt it removes, When Keep this and one dot on the shirt, Then the shirt comes back with the face and the beard, and the garden is as the method leaves it (T001's prototype: 52.9 % of the picture kept before the dot, 65.9 % after) | Verified by: manual M1 (JOD's photo, not in the repository)

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
(Written with the spec at JOD's request, 2026-11-10; JOD's go to build, 2026-11-10.) Spec reviewer, 2026-11-10: fifteen findings merged routinely (integer strokes with the P7 exception stated, the cell under both kinds, a press against a later stroke, Esc and the lost release during a stroke, toggles held, the wheel over the field, typed text and pending previews, memory as a stroke grows, the dot's minimum, concrete AS-3 and AS-4, the bands' look, mnemonics, Help) and one asked (JOD's answer 5). Pointer notes added at acceptance to REQ-0035-04, REQ-0037-01 and REQ-0037-07, and after T005 (2026-11-10) to REQ-0037-04 (presses dropped on a Tolerance change, strokes kept) and REQ-0037-06 (Ctrl+Z takes back the last press or stroke).

(Interfaces brought to the names as built, 2026-11-10.)
- Core `src/core/remove-bg.{c,h}`: `EaselRbgStroke { bool keep; int width; size_t n; const int32_t *xy; }` (whole picture
  pixels, P7.4). `easel_rbg_set_strokes (r, strokes, n)` builds `pxmap` (each pixel's stroke kind, 2 bits; the later stroke wins),
  `cmark` and `cowner` (each copy cell's kind and stroke) and `smean` (each stroke's mean Lab), all counted in the analysis's bytes;
  each row's covered run is found by halving (`cover_run`, exact). `easel_rbg_label` makes a Keep stroke's joined cells (4-connected,
  within the Tolerance of its mean) subject seeds, skips marked cells in the seam and exempts a region holding a stroke from the size
  and hole rules; `apply_strokes` sets each covered pixel's kind after the labels or the refinement, before the fringe, which stops at
  Keep pixels. Coverage `easel_rbg_covers (x, y, ax, ay, bx, by, width)`: a pixel's centre within width/2 of the segment, in 64-bit
  integers; `easel_rbg_stroke_kind (r, x, y)` is the test hook. Pieces (0037) follow from the labels with no change.
- Canvas `src/ui/canvas.{c,h}`: `easel_canvas_set_paint_hook (self, hook, width_logical, data)` with `EaselPaintEvent` (begin,
  extend, end, drop) takes the primary drag while a brush is on; `easel_canvas_is_painting`, `easel_canvas_drop_paint`;
  `easel_canvas_reset_pointer` ends a stroke (lost focus). `easel_canvas_set_bands (self, kind, data)` and
  `easel_canvas_bands_changed (self, x0, y0, x1, y1)` give it the bands, drawn as a texture with outline edges after the grid and
  before the gradient markers; a dot cursor of the brush's width.
- Card `src/ui/remove-bg-card.{c,h}`: two ungrouped `GtkToggleButton`s (`rbg-keep`, `rbg-remove`) with mnemonics and a colour dot
  each (success, error), `toggled` turning the other off; `easel_remove_bg_card_on_brush`, `easel_remove_bg_card_brush`,
  `easel_remove_bg_card_hold_brushes`; both lines capped at 40 characters' width.
- Window `src/ui/window.c`: the session keeps `struct RbgStroke {keep, seq, width, xy, n, cap}` (growing, limit-checked in
  `rbg_stroke_add`, shrunk at the end) and a band map (`rbg_band_paint`, `rbg_bands_rebuild`); `on_rbg_paint` sets the strokes, labels,
  previews and flips after each stroke end and each Tolerance change, OK's worker the same at full size, Cancel frees them;
  `on_rbg_brush` announces; `rbg_undo_last` takes back the press or stroke with the highest sequence number unless the field has the
  focus; Space and the arrows at the marker in the key handler and `on_key_released`, Esc drops.
- What grows: per stroke 24 bytes plus 8 bytes per pointer place (two coordinates), a pixel repeated not stored again; the marks, 1 byte a copy cell (at most 2 MB).
- Strings: "Keep this", "Remove this", the two tooltips, the three announcements, the new hint line (REQ-0038-10, the old one
  dropped), the Help paragraph, in 11 languages; the mnemonic check of 0023.

| Scenario | Test |
|---|---|
| AS-1 | ui `/0038/as1_toggles` + `/0023-lang/as1_card_mnemonics_differ` |
| AS-2 | core `strokes_selfie`, `strokes_dot`, `strokes_runs`, `strokes_fringe` + ui `/0038/as2_keep_stroke` |
| AS-3 | core `strokes_rules` + ui `/0038/as3_remove_and_order` |
| AS-4 | ui `/0038/as4_dot_and_shape` |
| AS-5 | ui `/0038/as5_tolerance_and_cancel` |
| AS-6 | ui `/0038/as6_bands_and_zoom` (rendered) + manual M2 |
| AS-7 | ui `/0038/as7_keyboard` + manual M1 |
| AS-8 | ui `/0038/as8_memory` + core `strokes_memory` + `perf-0035` (100 strokes) |
| AS-9 | core `strokes_large` |
| AS-10 | manual M1 |
| AS-11 | core `strokes_joined` |
| AS-12 | ui `/0038/as12_ctrl_z` |
| AS-13 | ui `/0038/as13_hint` + `translations` |
| AS-14 | core `strokes_after_press` + ui `/0038/as14_stroke_after_press` |

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
- Measured in T005 on a quiet machine (2026-11-10): 100 strokes set with their preview 316 to 325 ms, then 274 to 282 ms with the rows' runs; measured in T003: at 8000 x 8000 the strokes set with their preview 280 to 283 ms, a Tolerance change with them 178 to 182 ms, the result 869 to 873 ms; the first is near the 0.3 s budget, the set's per-pixel map and the preview's pass over every pixel being its cost.
- The preview within 0.3 s at 8000 x 8000 with strokes: the label alone takes 144 to 149 ms there now; the rasterizing of 100 x 1 000
  places into 2 000 000 cells is measured in the core task that adds it, before the ui relies on it.

## Tasks
JOD, 2026-11-10: build (the Plan as written; the answers: 16 logical pixels, Ctrl+Z to the last press or stroke, Space held at the marker,
the hint line in all 11 languages in the same commit, joined border seeds).
- [x] T001 Re-measure AS-2, AS-10 and AS-11 with the current core (the tolerance from the clusters, the stability check) on a prototype of the
  stroke rule (REQ-0038-03; AS-2, AS-10, AS-11) - done when the numbers are measured and any scenario they change is shown to JOD.
  Result: a prototype of the stroke rule on the current core (the stroke's cells and the cells joined to them made subject seeds), the automatic tolerance taken without the stroke: AS-2's picture opens at 5 (was 20), the face kept and the shirt removed, the backdrop 81.7 % removed with or without the stroke; one Keep dot at (120, 290) keeps all 15 360 shirt pixels and changes no other; the selfie opens at 7 (was 12), 52.9 % kept, 65.9 % with one dot on the shirt; AS-11 holds (kept from x 0 to 29, the right end removed). AS-2 and AS-10 rewritten with JOD's yes, a Remove this dot added to AS-2 (at (33, 120), where the foliage the method keeps fills the dot).
- [x] T002 Core: strokes, coverage, marks, joined seeds, the result's override and the fringe's shield (REQ-0038-03, -09; AS-2's core, AS-3's
  core, AS-9, AS-11) - done when the core cases pass, red first, with their mutants.
  Result: `easel_rbg_set_strokes` (a 2-bit kind a pixel, a kind and a stroke number a copy cell, each stroke's mean Lab; counted with the
  analysis). Red first on a stub that set nothing: `strokes_dot` failed ("count_removed (p, d, 60 * 60, RED) == want[k]"); then
  `strokes_dot` (197, 1, 5 pixels), `strokes_rules`, `strokes_joined`, `strokes_selfie`, `strokes_large`, `strokes_memory` pass.
  `strokes_fringe` passed at its first run (the code existed): its mutant (the fringe's shield taken out) killed it. Mutants: 9, 8
  killed; the seam's survives, equivalent (see Check); the "earlier wins" mutant rewritten once (it read the old map).
- [x] T003 Perf: 100 strokes of 1 000 places at 8000 x 8000, a stroke's preview and a Tolerance change (REQ-0038-04, -08; AS-8's perf) - done
  when measured in the real session and the numbers are in the Plan.
  Result: `perf-0035` extended (100 strokes of 1 000 places, width 16, half of them Keep this), real session, the load average 10.5
  and falling after mutation runs: 8000 x 8000: the strokes set with their preview 280 to 283 ms (within 0.3 s, near it), a Tolerance
  change with them 178 to 182 ms, the result 869 to 873 ms; 1920 x 1080: 112 to 132 ms, 14 to 15 ms, 27 ms.
- [x] T004 Card: the two toggles, tooltips, mnemonics, focus order, the announcements of the brush (REQ-0038-01, REQ-0038-06's announcements;
  AS-1) - done when the ui case passes and JOD has looked at the card in the app.
  Result (JOD's look, 2026-11-10: accepted, the card capped near 380 wide with the hint wrapping and a color dot on each toggle;
  `as11_complex_hint` checks the width and the wrap where the hint shows, failing at 516 wide without the cap): `as1_toggles` failed first ("'keep' should not be NULL"); then passes: the toggles found by their real
  labels and clicked, off at the start, one on turns the other off, clicked again both off, the announcements in order, the tooltips,
  laid out between the row and the buttons and side by side, the Tab order Tolerance, Keep this, Remove this, Cancel, OK (read from the
  window's focus: `gtk_widget_has_focus` needs an active window, which the isolated session's is not). Broken once (the other toggle not
  turned off): "'gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (keep))' should be FALSE"; restored.
- [x] T005 Canvas and window: a real drag paints a stroke with a brush on, the bands and the brush's outline, the preview after the release,
  presses and strokes together, a typed number, a pending preview, the work at OK, the Tolerance keeping strokes and dropping presses, Esc,
  a lost release, zoom during a stroke, Cancel and OK (REQ-0038-02, -04, -05, -07; AS-2, AS-3, AS-4, AS-5, AS-6) - done when the ui cases
  pass on the real path and JOD has painted with the real mouse (M2).
  Result (JOD's real-mouse check, 2026-11-10: "accepted"): the canvas's paint mode (a press begins, a move extends, a release or a lost one ends,
  the other button or Esc drops), the window's strokes and 2-bit band map, the bands drawn with their outline, the dot's outline as the
  pointer. `as2_keep_stroke` failed first (a press with a brush on still flipped a piece: "img->px[i] == before[i]"), then passes with
  `as3_remove_and_order`, `as4_dot_and_shape`, `as5_tolerance_and_cancel`, `as6_bands_and_zoom`; each broken once (no stroke begun:
  as3 to as6 fail; Esc not dropping, a lost release not ending, the width taken at the end, the toggles not held: as6 fails at the
  matching assert) and restored. Two test mistakes fixed on the way (the original copied after the card opened; a dot outside the
  disk at 400 %). Quiet machine (16 cores, load 2.5): 100 strokes set with their preview at 8000 x 8000 were 316 to 325 ms, over 0.3 s;
  each row's covered run found by halving (`cover_run`, the same pixels: `strokes_runs` compares with a test of each pixel) brought it
  to 274 to 282 ms (load 3.9), one stroke 199 to 206 ms, a Tolerance change 194 to 197 ms; no worker needed. AS-4's free-form shape is
  checked with a rectangle selection only.
- [x] T006 Ctrl+Z to the last press or stroke, and Space held at the marker (REQ-0038-04, -06; AS-7, AS-12) - done when the ui cases pass
  and JOD has tried the real keyboard.
  Result (JOD's real keyboard still to try, with T009): each press and stroke takes a number in their common order; Ctrl+Z on the canvas
  takes back the highest (with the focus in the field it is the field's own); Space with a brush on begins a stroke at the marker, the
  arrows extend it while it is held, its release ends it, Esc drops it, a repeat does nothing; a lost focus ends a stroke as released.
  `as7_keyboard` failed first (Space flipped a piece: "img->px[...] == ... 0xFFFF0000u", the stroke's pixel kept) and `as12_ctrl_z`
  ("'pc_removed (img, 120, 100) && pc_removed (img, 60, 70)' should be TRUE"), then pass; broken once each (Ctrl+Z taking a press first:
  as12 fails; the arrows not extending: as7 fails) and restored.
- [x] T007 Memory: a stroke or its preview without memory (REQ-0038-08; AS-8's ui) - done when the ui case passes, held and counted compared.
  Result: characterization (T005 wrote both checks): `as8_memory` passed at its first run; each check broken once: the places' limit
  gone (way 1: "top_heading (win) == 'Could Not Remove the Background'": NULL); the bands' limit gone first went unseen (a limit of 64
  bytes failed the places' first 512 too), so way 0's limit became 600 (the places fit, the bands' 1 200 do not), and then it failed;
  restored. Known gap: each allocation is held to the limit alone, not added to the analysis's (as 0037's F4).
- [x] T008 Strings: the hint line naming the brushes, the toggles, tooltips and announcements in 11 languages, the Help paragraph, the
  mnemonic test first (REQ-0038-10, REQ-0038-01's mnemonics; AS-13, AS-1's mnemonic part) - done when `translations`, the mnemonic case and
  `as13_hint` pass.
  Result: `/0023-lang/as1_card_mnemonics_differ` (Keep this, Remove this, Cancel, OK) written before the strings, passed in English;
  `/0038/as13_hint` (AS-11's case under AS-13's name, the new line in `RBG_HINT`) failed with the old line in the card ("'hint' should
  not be NULL"), then passes; 8 strings in 11 catalogs, the old hint dropped; the Help's Remove Background paragraph names the
  clicks, the brushes and Ctrl+Z in 12 pages; ru, uk, pl added to `translations-to-review.md`. `translations`, ui-0023 in every
  language with `-long` and `-rtl`, ui-0035/0037/0038, core and io green; the mnemonic case broken once (ru Keep as "_Оставить"
  meets ru's OK: ui-0023-ru fails) and restored.
- [x] T009 Manual M1 on JOD's selfie: one Keep stroke on the shirt (AS-10) - done when JOD has tried it.
  Result: JOD tried the selfie and the keyboard painting in the real session and said "go" (2026-11-10).
- Gate: `tools/mutate.sh --core` once, in the background, after T008.

## Check
- Date and commit: 2026-11-10, on `54fb0f3` with the uncommitted work of 0038 (nothing of it committed yet).
- Tests: `meson test` (isolated) 129 of 129 before the check's fixes, 130 of 130 after them; the scale 2 list 6 of 6 (with the new
  `ui-0038-scale2`); the cases of this feature: core 9 (`strokes_*`), ui 14 (`/0038/*`, `/0038-scale2/as4_dot_scale2`), the mnemonic case.
- Mutants: the gate `tools/mutate.sh --core`: 514 mutants, 511 killed, 0 by crash, 0 inconclusive, 2 survived, 1 accepted (218 s); after
  the check's core change `tools/mutate.sh REQ-0038-03`: 9 mutants, 8 killed, 1 survived (the seam's, below), the two new ones killed.
  `--lint`: 1384 mutants, 0 stale (three repointed after the check's edits).
- Warnings: 0. Lint and `--names 0038` clean.
- Survivor, accepted: REQ-0037-02 "a fully transparent pixel comes back with a kept piece": equivalent since P7.7; the mutant writes
  an alpha-0 pixel's own value or 0, and in the app every alpha-0 pixel is already 0x00000000 (the loader zeroes them, every operation
  writes 0), so no picture a user has shows a difference.
- Survivor, accepted: REQ-0038-03 "the seam changes a stroke's cell": equivalent; every pixel a stroke covers takes the stroke's kind after the labels (`apply_strokes`), in the preview and the result, so a cell the seam changes under a stroke shows no pixel of it.
- Scenarios: AS-1 `as1_toggles` + `as1_card_mnemonics_differ`; AS-2 `strokes_selfie` + `as2_keep_stroke`; AS-3 `strokes_rules` +
  `as3_remove_and_order`; AS-4 `strokes_dot` + `as4_dot_and_shape` + `as4_dot_scale2`; AS-5 `as5_tolerance_and_cancel`; AS-6
  `as6_bands_and_zoom` + M2; AS-7 `as7_keyboard` + M1; AS-8 `as8_memory` (C: T007, broken by hand) + `strokes_memory` + perf; AS-9
  `strokes_large`; AS-10 M1 (JOD, T009); AS-11 `strokes_joined`; AS-12 `as12_ctrl_z` + `as12_ctrl_z_with_history` +
  `as12_undo_brings_press_back`; AS-13 `as13_hint` + `translations`; AS-14 `strokes_after_press` + `as14_stroke_after_press`.
- Task results: T001 to T009 have their `Result:` lines (above); T007 is a characterization (its checks written in T005, each broken by
  hand); T006's "the real keyboard still to try" was done at T009.

Findings (a review agent read the diff blind; each finding below checked in the code before it was settled):
- F1 (user-visible, fixed): Ctrl+Z on the canvas with the card open did nothing when the picture had undo steps: Ctrl+Z is the window's
  Undo accelerator, which takes the key before the key handler once Undo is enabled, and Undo does nothing while a dialog is open.
  AS-12's picture had no steps, so its test passed. Fixed in `undo_activated`; `as12_ctrl_z_with_history` failed first ("memcmp ...
  (-255 == 0)": the stroke stayed), then passes.
- F2 (user-visible, fixed): the brush was 16 device pixels, not 16 logical, so 8 logical on a scale 2 screen: the zoom counts device
  pixels. One rule now, `easel_canvas_paint_width`, for the stroke and the dot cursor (drawn in logical pixels). `as4_dot_scale2` failed
  first (49 pixels, not 197), then passes; registered as `ui-0038-scale2`.
- F3 (user-visible, fixed): Ctrl+Z of a stroke did not bring back the presses that stroke had put out of force (REQ-0038-04: "makes the
  preview without it"). The stroke now keeps them and gives them back in their order; a Tolerance change forgets them with the presses.
  `as12_undo_brings_press_back` failed first ("memcmp ... (192 == 0)"), then passes.
- F4 (user-visible, decided by JOD, fixed): a press flipped its whole piece after the strokes, so a later Keep stroke over a pressed
  island away from the pressed pixel did nothing. JOD: each pixel takes the latest word on it. REQ-0038-03 changed, AS-14 added, learning
  log; `easel_rbg_flips` keeps a later stroke's pixels (`EaselRbgPress.strokes_after`, the cell's latest stroke over 2 000 000 pixels).
  `strokes_after_press` failed first, then passes; `as14_stroke_after_press` passed at its first run (the code existed) and failed with
  the window's count broken ("'pc_removed (img, 110, 60)' should be FALSE"); two mutants killed.
- F5 (test gaps, open): clauses with no case of their own: REQ-0038-07 the secondary button dropping a stroke, Enter doing nothing,
  the window losing the focus, a resize, the wheel over the field during a stroke, Ctrl+Q during a stroke (named in AS-6); REQ-0038-04
  a typed number, a pending preview and a stroke during OK's work; AS-6's bands at 400 % and their outline; AS-4's free-form shape
  (a rectangle only). The review found the focus loss, the wheel and Ctrl+Q right by reading; no user-visible bug known.
- F6 (open, low): a press of the mouse while Space holds a keyboard stroke joins that stroke (its motion extends it, its release ends
  it), and a secondary press does not drop a keyboard stroke.
- F7 (open, low): if the memory for the strokes cannot be had when Ctrl+Z takes one back (it is made new before the old is freed), the
  failure is ignored: the core keeps the stroke while the bands drop it, with no message.
- F8 (open, low): over 2 000 000 pixels the core's strokes ignore the selection's shape inside a copy cell that straddles its edge
  (the bands follow it); the cell's covered pixels outside still make it a seed and count in the stroke's color.
- F9 (open, low): `covers` can overflow 64 bits for places about 70 000 pixels from the picture (a drag far outside at a low zoom).
- F10 (open, low): during a drag each motion repaints its segment's whole bounding box and rebuilds the whole band texture; a fast drag
  at a low zoom on 8000 x 8000 may lag. JOD's real-mouse check (T005) was at ordinary sizes.
- F11 (open, spec wording): the joined cells are claimed first-come, so a cell joined to two Keep strokes is reached from one only;
  overlapping strokes of one kind share one outline (REQ-0038-05 says each stroke's area). Neither changes a kept pixel's kind.
- Known gaps (also in `docs/phase2-plan.md`): F5 to F11; each allocation held to the limit alone, not added to the analysis's (as
  0037's F4); the dropped presses a stroke keeps are held to the limit without a memory test of their own.
