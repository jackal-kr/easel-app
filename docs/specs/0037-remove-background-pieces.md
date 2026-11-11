# 0037 Remove Background: pieces and horizon

Status: implemented
Reviewed: spec-reviewer, 2026-11-10
Depends on: 0022, 0025, 0035

## Why
Remove Background (0035) keeps every region it judges subject that is a tenth of the largest or more, and removes every region it
judges background. On JOD's eagle photo the clouds and the mountains at the bottom stay as islands; on a horizon photo a band of each
side stays; a pale glow that is not the backdrop's own color stays (JOD, 2026-11-10). A press on the preview fixes each: it flips a
piece between kept and removed. (Fitting the background in segments of the border was prototyped and left out, JOD 2026-11-10.)

## Requirements
- REQ-0037-01 (Adds to REQ-0035-04, whose canvas otherwise takes no input while the card is open.) While Remove Background's card is open, a press of the primary button on a pixel the command looks at (REQ-0035-02: the picture, or the selection's shape as shown, its part off the picture included) shall flip the piece holding it between kept and removed in the preview, at the press; moving and releasing do nothing more. A piece is a 4-connected region of pixels that the method (REQ-0035-03, all of it, the fringe included) leaves all kept or all removed, as it was before any press, the removed background itself being one, and that the flood (REQ-0035-03 b) gave one label before the regions rule (c): so an island the size rule removed is its own piece, not the background's, and a hole the rule filled is its own (2026-11-10, at T001: without this a removed island pressed back would keep the whole background, against AS-1); the pieces are the method's for the Tolerance shown, and a press does not change them. A press flips its piece; a piece pressed twice is as the method made it, so the order of presses does not matter. A flip gives no undo step and does not change the title's modified mark; OK keeps the result as shown (REQ-0035-02's one step), Cancel and Esc put everything back (REQ-0035-04). A press on a pixel the command does not look at (outside the shape, on the picture outside a float, beside the picture), the secondary and middle buttons, and a press on the card or the thumbnail do nothing to the pieces; the card and the thumbnail take their own presses as always. (Pointer note, 2026-11-10: Remove Background: Keep this and Remove this: while a brush is on, a drag paints a stroke instead, and a pressed pixel a later stroke covers is dropped.)
- REQ-0037-02 Keeping a removed piece shall give back only the pixels the method removed: pixels fully transparent before the card stay so (REQ-0035-03 e), and a piece made only of them flips nothing. If the flips leave no pixel removed, OK is as REQ-0035-05 (no step, no mark).
- REQ-0037-03 The keyboard shall reach the same (REQ-0022-05, -06): while the card is open, F6, or Tab from the card's last button, moves the focus to the canvas and shows the marker; Tab or F6 from the canvas returns it to the Tolerance field. With the focus on the canvas the arrows move the marker, Space flips the piece under it (a key repeat flips nothing), Esc is Cancel and Enter is OK; Up and Down do not change the Tolerance there; Shift+Space, Ctrl+Space, Ctrl+Shift+Space, the Menu key and Shift+F10 do nothing; F7 announces the marker's place and the color the preview shows (REQ-0022-13). Each flip announces "Removed" or "Kept" (REQ-0022-15). The card is not a box in the sense of REQ-0022-06.
- REQ-0037-04 (2026-11-10, JOD's T005: a press kept across a Tolerance change flipped the piece under it in the new preview, and at Tolerance 1, where the whole picture is one piece, a press removed everything.) A flip shall last until the Tolerance changes: a change of the number (its buttons, keys, wheel or a typed number taken) drops every press, and the new preview is the method's alone; OK straight after a change keeps no press. A press on a piece already flipped by an earlier press in this card is a press of its own that cancels the earlier one, so the piece turns back; Ctrl+Z (REQ-0038-04) takes back that later press first, turning the piece again, then the earlier one (JOD, 2026-11-11, at 0036's check; before: it removed the earlier press, so Ctrl+Z went on to the press or stroke before both). (Before: "A flip shall stay when the Tolerance changes: each press is kept as the picture pixel that was pressed, and in the new preview the piece under each such pixel is flipped once per press there. A pressed pixel that no piece of the new preview holds (fully transparent, or not shown by the copy) flips nothing. A press on a piece already flipped by an earlier press in this card removes that earlier press instead of adding one. (JOD, 2026-11-10.)") (Pointer note, 2026-11-10: Remove Background: brushes keeps its strokes through a Tolerance change and lets Ctrl+Z take back the last press or stroke.)
- REQ-0037-05 A press that takes the focus from the Tolerance field while it holds typed text shall first apply the number (REQ-0035-04) and make that preview with the presses so far, then flip the piece under the press in it; a press while a preview is still to be made acts on the preview for the number shown, once made. A press while the result is made at OK (REQ-0035-11) does nothing; Cancel there drops every press with the rest.
- REQ-0037-06 While the card is open the view shall scroll (the wheel, the scroll bars) and zoom (Ctrl+wheel, the pinch, REQ-0036-01), so that a small piece can be magnified to press it; the middle button's drag does nothing. Undo and Redo, the menus and the tools still take no input; Ctrl+Z does not undo a flip (pressing the piece again does). (Pointer note, 2026-11-10: Remove Background: brushes makes Ctrl+Z inside the card take back the last press or stroke.) (JOD, 2026-11-10.)
- REQ-0037-07 While the card is open the pointer shall be the default arrow anywhere over the canvas area (the picture, a float, the space beside them), with no tool's footprint outline (REQ-0006-16) or pointer preview. (JOD, 2026-11-10.) (Pointer note, 2026-11-10: Remove Background: Keep this and Remove this: while a brush is on, the pointer is the outline of the brush's dot.)
- REQ-0037-08 If the memory for the presses or a preview with them cannot be had, the message of REQ-0035-06 shall show and the card stay open with the preview as before the press. The presses count in REQ-0035-09's 1 GB. On the dev machine the preview after a press shall show within 0.3 s at any size, and with 1 000 presses within 0.3 s (2026-11-10: the presses no longer outlive a Tolerance change, REQ-0037-04).
- REQ-0037-09 The pixel contract (P7): (1) a flipped piece's pixels are each exactly as before the card or 0x00000000 (REQ-0035-07, P7.7, 2026-11-10; before: alpha 0 with their color kept); (2) the view only magnifies: a press is mapped to the picture pixel whose drawn square holds it (REQ-0025-02); (3) the preview and the result are made by the same code from the same pieces and presses (P7.3; over 2 000 000 pixels the pieces are the copy's regions, each copy cell standing for its block, a piece the copy does not show cannot be pressed, and at full size a pixel belongs to its own block's piece, also where the boundary refinement of REQ-0035-03 (d) or the fringe (g) changed it: a flipped piece's pixels turn to the other kind, a removed one coming back only if the method removed it; ADR-0007's exception; before 2026-11-10, at the Plan: "every pixel the refinement gives to it, with the pixels the fringe removed beside it", which the refinement does not record); (4) the same pixels, Tolerance and pressed picture pixels, in the same order, give the same pixels on every machine.

## Scenarios
- AS-1 (REQ-0037-01) Given an 80 x 60 white picture with a red rectangle, a blue disk apart from it at least a tenth of its size (kept by the method as an island) and a green disk under a tenth (removed by the method), When Remove Background and a press on the blue disk, Then it is removed in the preview, no step, no mark; When a press on it again, Then it is kept; When a press on the removed green disk, Then it is kept and the white around it still removed; When a press on the removed white, Then the white is kept; When OK, Then one step and the result as shown | Verified by: automated (ui)
- AS-2 (REQ-0037-01) Given a press that flipped a piece, When Cancel, and When Esc with the focus on the canvas, Then the picture is as before the card, no step | Verified by: automated (ui)
- AS-3 (REQ-0037-01) Given a marked rectangle selection, a free-form selection and a float stretched to twice its size, When a press inside the shape on an island, Then it flips; When a press outside the shape and When a press on the picture outside the float, Then nothing flips | Verified by: automated (ui)
- AS-4 (REQ-0037-02) Given a picture whose background was made fully transparent before, When a press on it, Then nothing flips; Given the method's only removed piece pressed to kept, When OK, Then no step and no mark | Verified by: automated (ui)
- AS-5 (REQ-0037-03) Given the card open, When F6, the marker moved onto the blue disk and Space, Then the disk flips and "Removed" is announced; a key repeat of Space flips nothing; When Tab, Then the Tolerance field has the focus; When F6 and Enter, Then OK | Verified by: automated (ui) + manual M1 (the real keyboard)
- AS-6 (REQ-0037-04) (2026-11-10: presses dropped on a Tolerance change; before: they stayed.) Given the blue disk pressed to removed, When the Tolerance changes by 1, Then it is kept as the method makes it; When the Tolerance changes back, Then it is still kept; When a press on it, Then it is removed; When the Tolerance changes by 1 and OK at once, Then the result has no press | Verified by: automated (ui)
- AS-7 (REQ-0037-05) Given 9 typed in the field, When a press on an island, Then the preview is first made for 9 and the island flipped in it | Verified by: automated (ui)
- AS-8 (REQ-0037-06, REQ-0037-07) Given the card open, When Ctrl+wheel over the picture, Then the view zooms; When the middle button's drag, Then nothing moves; When Ctrl+Z, Then nothing is undone; When the pointer moves over the picture, a float and the space beside them, Then the default arrow and no outline | Verified by: automated (ui)
- AS-9 (REQ-0037-08) Given memory made too small after the card opened, When a press, Then the message shows and the preview is as before the press; Given an 8000 x 8000 picture, When a press, Then the preview within 0.3 s; When 1 000 presses, Then the preview within 0.3 s (2026-11-10: before, 1 000 presses then a Tolerance change, which now drops them) | Verified by: automated (ui) + automated (perf, in the real session)
- AS-10 (REQ-0037-09) Given a picture over 2 000 000 pixels with an island the copy shows, When a press on it and OK, Then every pixel whose block is in the island's piece is alpha 0 with its color kept (those the method kept) and every other pixel as the method makes it without the press; the same pixels in a second process | Verified by: automated (core, ui)
- AS-11 (REQ-0037-01) Given JOD's eagle photo at its automatic tolerance, When a press on each cloud and mountain island, Then they go and the eagle stays | Verified by: manual M1 (JOD's photo, not in the repository)
- AS-12 (REQ-0037-04) Given the card open and an island removed by a press, When a second press on it, Then it is kept again; When Ctrl+Z with the focus on the canvas, Then it is removed again; When Ctrl+Z again, Then it is kept, as the method made it | Verified by: automated (ui)

## Edge cases
- A piece of one pixel: flips like any other.
- Many presses on one piece: each press there flips it again (REQ-0037-04 counts the ones in force).
- All or nothing on a busy photo (JOD's selfie, 2026-11-10, T005): at the automatic tolerance the face, beard, shirt, sky and garden were one removed piece, so a press brought back all of it or none; pieces cannot split what the method joined (Remove Background: brushes).
- A drag begun elsewhere while the card is open: none can begin, the canvas taking no other input.

## Out of scope
- Brushes that paint kept or removed (`docs/phase2-plan.md` G), soft edges (H), a texture channel (I), any graph cut.

## Open questions
None. Settled by JOD on 2026-11-10: pressed pixels kept across a Tolerance change; the background in segments of the border left out;
the default arrow; a piece is the method's region before any press, each press flipping it; Ctrl+Z does not undo a flip; the view
scrolls and zooms while the card is open. The spec reviewer's findings settled routinely (2026-11-10): the selection's pixels, the
keyboard path and its keys, the fully transparent pixels, typed text and pending previews, the work at OK, memory and time, the order
of the method's steps, the copy's pieces.

## Plan
JOD, 2026-11-10: accepted, built without stopping after the Plan and the Tasks; stop at the manual check.

- Core `src/core/remove-bg.{c,h}`: `easel_rbg_flips (r, px, mask, dst, presses, n, &removed)` applies the presses (picture or float
  pixels, as given by the caller) to a preview or a result already made into dst: it finds the pieces (at 2 000 000 pixels and under,
  4-connected runs of dst's kept or removed pixels the command looks at and that were not fully transparent before; over that, runs of
  the copy's labels), counts the presses per piece and turns each piece with an odd count to the other kind, a removed pixel coming
  back only if the method removed it. `easel_rbg_same_piece (r, x1, y1, x2, y2)` answers from the last flips call. The pieces' numbers,
  4 bytes a cell, are held with the analysis and counted (REQ-0037-08); the fill reuses the analysis's queue.
- UI `window.c`: the session keeps the presses (picture pixels of the float or the picture, as the command looks at them) in a growing
  array counted against the transform limit; a press asks `same_piece` against each kept one and removes it instead of adding (REQ-0037-04);
  every preview and the result at OK run `easel_rbg_flips` after the method. Canvas: `easel_canvas_set_press_hook` takes the primary press
  (mouse, or Space at the marker) while the card is open, mapped by `to_pixel` (REQ-0025-02), before the input block; the pointer is the
  default arrow and no outline is drawn while the hook is set. The window's key handler gives the canvas the arrows and Space when it has
  the focus, makes Esc Cancel and Enter OK there, and F6 and Tab move between the card and the canvas; zoom and scroll pass (REQ-0037-06).
- Memory: presses 8 bytes each; the pieces 4 bytes a cell.

| Scenario | Test |
|---|---|
| AS-1, AS-2, AS-3, AS-4, AS-6, AS-7, AS-8 | ui `/0037/...` |
| AS-5 | ui `/0037/as5_keyboard`, `/0037/as5_keys` (F7 and the keys that do nothing) + manual M1 |
| AS-9 | ui memory case + `perf-0035` (a press, 1 000 presses) |
| AS-10 | core `pieces_*` + ui `/0037/as10_large` |
| AS-11 | manual M1 |

Constitution: P7 as REQ-0037-09; the exception of ADR-0007 over 2 000 000 pixels unchanged.

## Tasks
- [x] T001 Core: pieces and flips (REQ-0037-01, -02, -04, -09; AS-10) - done when the core cases pass, red first, and the mutants are killed. Result: `pieces_small` failed first on the stubs (line 542: the island not removed); then on the green disk the method removed (pressed, it brought back the whole white: a removed island is one run with the background), which split the pieces by the flood's labels before (c) and changed REQ-0037-01's wording; `pieces_large` passed with them. Mutants: parity, the flood's split, same-piece, the copy's pieces killed; the transparent pixel's survives, equivalent (see Check).
- [x] T002 UI: the press hook, the presses kept and replayed, OK and Cancel, same-piece removal, pointer, zoom and scroll, memory (REQ-0037-01, -02, -04 to -08; AS-1 to AS-4, AS-6 to AS-8, AS-9's memory case) - done when the ui cases pass. Result: `as1_presses` to `as9_memory` written before the code ran; `as8_view` failed first (Ctrl+wheel did not zoom: the wheel's own gate, not the zoom commands', had to let the card through). Mutants: the result at OK without the presses, no hook, no zoom killed; same-piece removal survives (see Check).
- [x] T003 UI: the keyboard path and the announcements (REQ-0037-03; AS-5) - done when the ui case passes. Result: `as5_keyboard`; its first build passed the arrows and Space on to a canvas that has no key handler of its own (the window's handler drives the marker), so the card's branch calls the marker and the press itself; "Kept" and "Removed" translated, ru, uk, pl for JOD's review.
- [x] T004 Perf (REQ-0037-08; AS-9) - done when within 0.3 s. Result: first over (8000 x 8000: 275 ms a press, 330 ms with 1 000 presses after a Tolerance change: the flips visited every pixel); visiting only the flipped pieces' blocks: 153 ms and 244 to 271 ms; 1920 x 1080: 10 ms and 26 ms.
- [x] T005 Manual M1: JOD presses the eagle's islands and tries the keyboard (AS-5, AS-11).
  Result (first round, 2026-11-10, JOD's clip on his selfie): the card opened at 12 and lost the lower face, beard and shirt (fixed in 0035's stability check, T110); the pieces were all or nothing on that photo (a known limit, left to the brushes); presses kept across a Tolerance change made Tolerance 1 remove everything (REQ-0037-04 changed: presses dropped on a Tolerance change; `pc_as6_tolerance` failed first at "'pc_removed (img, 120, 60)' should be FALSE", then passes; a mutant for the drop added and killed). Second round (2026-11-10): the eagle's islands and the keyboard, after the fixes: "T005: ok" (JOD).

- [x] T006 The Check's F1, F2 and F3 (REQ-0037-03, -09; AS-5, AS-10), JOD 2026-11-10 - done when the cases pass, red first. Result: `as5_keys`
  failed first at F7 ("'strstr (said, want)' should not be NULL": nothing announced, then, with F7 passed on, still nothing: the
  keyboard pointer's keys refuse while the card counts as an open dialog), then passes with the announcement shared by both paths; its
  other keys passed at once, so the rule for Space with Shift was broken once (Shift+Space flipped the island: "'pc_removed (img, 120,
  60)' should be FALSE") and restored; `as10_large` passed at once (the code existed), so the large picture's pieces were broken once
  (every block no piece: "'pc_removed (img, 500, 200)' should be TRUE") and restored; a mutant for F7 added and killed.

## Check
- Date and commit: 2026-11-10, `996db2d` (the run below); the transparent-pixel change of ADR-0008 came after it, uncommitted.
- Tests: `tools/isolated.sh meson test` 128 of 128 pass (the cases of this feature: ui-0037's 9 and core `pieces_small`, `pieces_large`);
  scale 2: 5 of 5; after ADR-0008, 128 of 128 again.
- Mutants: `tools/mutate.sh --core`: 499 mutants, 496 killed, 0 killed by crash, 2 survived, 1 accepted (149 s).
- Warnings: 0. Commits: no AI trailers since `5c1cb53`.
- Scenarios: AS-1 to AS-4, AS-6 to AS-8 ui `/0037/...`; AS-5 ui `as5_keyboard` + T005; AS-9 ui `as9_memory` + `perf-0035`; AS-10 core
  `pieces_large` (no ui case, F3); AS-11 T005 ("T005: ok", JOD).
- Task results: T001 to T004 as written above, none a characterization; T005: first round on JOD's selfie (three changes, see its
  Result), second round "T005: ok" (JOD).
- F1 (user-visible): with the card open and the focus on the canvas, F7 did nothing: the card's key branch took every key it did not
  handle before the keyboard pointer's keys, so REQ-0037-03's "F7 announces the marker's place and the color the preview shows" was not
  met. Fixed in T006 (the announcement shared by both paths; a mutant for it, killed).
- F2: REQ-0037-03's keys with no test: Shift+Space, Ctrl+Space, Ctrl+Shift+Space, the Menu key and Shift+F10 doing nothing, Up and
  Down not changing the Tolerance on the canvas, F7 (F1). Fixed in T006: `as5_keys`.
- F3: AS-10 named a ui verification; only the core test checked a picture over 2 000 000 pixels. Fixed in T006: `as10_large`.
- F4: REQ-0037-08 counts the presses in REQ-0035-09's 1 GB; the code compares the presses' array alone with the transform limit, not
  added to the analysis. 8 bytes a press; not user-visible. Known gap.
- Survivor, accepted: REQ-0037-04 "a press on a pressed piece adds a press instead of removing the earlier": equivalent since the presses are dropped on a Tolerance change (2026-11-10; two presses on one piece flip it twice, as none); before: the pixels differed only when a later Tolerance split that piece (two presses on one piece cancel either way, three leave one flip either way).
- Survivor, accepted: REQ-0037-02 "a fully transparent pixel comes back with a kept piece": up to 2 000 000 pixels such a pixel is in no piece; over that, with ADR-0008, the mutant writes 0 over a pixel that was transparent before the card instead of leaving it, which is invisible either way (2026-11-10; before: both kinds gave it back as it was).
- Verdict (2026-11-10, after T006): implemented; F4 stays a known gap. ui-0037 (11 cases), core and io (47) pass; the REQ-0037-03 mutant
  killed; no full mutation run after T006 (JOD).
