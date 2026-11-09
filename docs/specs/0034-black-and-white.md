# 0034 Black and white

Status: implemented
Reviewed: spec-reviewer, 2026-11-09
Depends on: 0004, 0013, 0015, 0017, 0018, 0019, 0021, 0023

## Why
XP's Image, Attributes turns a picture to pure black and white (a 1-bit look for printing, faxing or a retro style). JOD
(2026-11-04, 2026-11-09) wants it as one command beside Invert Colors, with a choice of how colors become black or white: a hard
threshold, an even dot pattern, or the photo-like error diffusion (the three compared in `docs/reference/black-and-white/conversions.png`).
Not XP's mode: no palette change, no warning, one undo step (`docs/phase2-plan.md` D).

## Requirements
Throughout: a pixel's brightness is Y = 299 R + 587 G + 114 B (0 to 255000, the weights of ITU-R BT.601 in thousandths); black is
R = G = B = 0 and white R = G = B = 255; a converted pixel keeps its alpha exactly (JOD, 2026-11-09), so a transparent pixel stays
transparent with black or white color channels.

### The command and its dialog
- REQ-0034-01 (Adds to REQ-0018-01, -02.) The Image menu shall hold Black and White… right after Invert Colors, with no shortcut; the selection menu (REQ-0013-17) holds it right after its Invert Colors. It follows REQ-0018-01's rules for when an item acts. Its strings (the item, the dialog's heading, lines, choices and buttons, the error headings) are translated in every language of 0023.
- REQ-0034-02 Choosing it shall open a modal dialog in the look of the Attributes dialog (REQ-0004-21): the heading "Black and White", the line "Turns the picture to black and white." ("Turns the selection to black and white." while a selection exists), three choices of which exactly one is chosen, each with its line under it: "Threshold" ("Hard edges, no dots"), "Ordered dither" ("Grays as an even pattern") and "Error diffusion" ("Most like a photo"), and the buttons Cancel and Convert (the suggested one). Threshold is chosen each time it opens (JOD, 2026-11-09). The focus starts on the chosen choice; the three choices are one Tab stop, and Tab and Shift+Tab move between them, Cancel and Convert; the arrow keys move the choice and choose it, and Space chooses the focused one. Enter is Convert, with the chosen conversion, unless the focus is on Cancel, where Enter and Space are Cancel; Esc is Cancel. Cancel, Esc and closing the dialog change nothing further. No new key outside the dialog.

### What is changed
- REQ-0034-03 Opening the dialog shall do what opening Stretch and Skew does to what waits (REQ-0019-06): a pending curve, polygon or gradient and an open text box are put down and stay put down if the dialog is cancelled; a text box whose put-down cannot get its memory stays open and the dialog does not open. Convert then acts on the selection while one exists and on the whole picture otherwise, as Invert Colors does (REQ-0018-03, REQ-0018-08): a marked selection is lifted first (REQ-0018-07), also when the conversion changes none of its pixels, and floats afterwards, joining its one undo step; on a floating one the conversion is made on its pixels as shown (REQ-0018-07: as lifted, scaled to its size, turned and flipped; for a pasted one the pixels as pasted), which then become its pixels at its shown size with no flip or turn left over, so a later stretch starts from them (JOD, 2026-11-09); only a free-form selection's shape's pixels change, and a pasted selection's fully transparent pixels stay not put down. On the whole picture it is one undo step that costs what REQ-0002-16 allows for the pixels it changes (as Invert Colors, REQ-0018-09); one that changes no pixel adds no step and no mark.
- REQ-0034-04 Threshold shall make a pixel white when Y >= 128000 and black otherwise.
- REQ-0034-05 Ordered dither shall make the pixel at (x, y) white when 128 Y >= 255000 (2 b + 1), b being the value at row y mod 8 and column x mod 8 of the matrix 0 32 8 40 2 34 10 42 / 48 16 56 24 50 18 58 26 / 12 44 4 36 14 46 6 38 / 60 28 52 20 62 30 54 22 / 3 35 11 43 1 33 9 41 / 51 19 59 27 49 17 57 25 / 15 47 7 39 13 45 5 37 / 63 31 55 23 61 29 53 21 (rows separated by "/"), black otherwise; x and y are the pixel's own coordinates in the picture, or in the selection as shown counted from its own top-left pixel, so a pixel's result depends only on itself and its place there, and two selections or a selection and the whole picture may show the pattern at different offsets.
- REQ-0034-06 Error diffusion shall go through the pixels row by row from the smallest y, each row from the smallest x, each pixel's value v starting as its Y: the pixel is white when v >= 128000, black otherwise; its error e = v − 255000 (white) or v (black) is added to the values of the pixels not yet gone through: 7e / 16 to (x + 1, y), 3e / 16 to (x − 1, y + 1), 5e / 16 to (x, y + 1) and e / 16 to (x + 1, y + 1), each share rounded toward zero. On the whole picture a share whose pixel is outside the picture is dropped; on a selection the rows and columns are those of the selection as shown, and a share whose pixel is outside it (or outside a free-form selection's shape) is dropped, wherever the selection floats. A fully transparent pixel (alpha 0, also a pasted selection's pixel that stays not put down) is converted by its own Y alone, takes no share and passes none on, so what cannot be seen does not move the dots that can (decided in the review, 2026-11-09).
- REQ-0034-07 (P7) The conversion writes whole pixels of exactly black or white with their alpha (P7.1); the same pixels, the same choice and, for a selection, the same shown pixels and shape always give the same result (P7.4); a floating selection then shows its converted pixels one to one (P7.3).

### Failures and cost
- REQ-0034-08 If its memory cannot be had, the app shall show REQ-0018-10's dialog with the heading "Could Not Convert the Picture" (or "Could Not Convert the Selection") and the body "There is not enough memory for this change.", and change nothing, as REQ-0018-10 says (a selection as before the command, marked again if the command lifted it).
- REQ-0034-10 Ctrl+Q, Ctrl+W or a close request while Black and White is open shall close it as Cancel does, then go on as REQ-0001-34 and REQ-0001-35 say (as REQ-0021-03).
- REQ-0034-09 Each conversion of an 8000 × 8000 picture, and of an 8000 × 8000 floating selection, shall take less than 500 ms in the core (REQ-0018-12's budget), the result painted at the next refresh after it.

## Scenarios
Colors are #RRGGBBAA. The numbers are computed with a script from REQ-0034-04 to -06.
- AS-1 (REQ-0034-01, REQ-0034-02) Given a picture, When the user opens the Image menu, Then Black and White… follows Invert Colors with no shortcut; the selection menu has it after its Invert Colors; When the user chooses it, Then the dialog shows the heading, the line about the picture, Threshold chosen and focused, Cancel and Convert; with a selection the line says the selection; the arrow keys choose Ordered dither then Error diffusion; Tab goes to Cancel, then Convert, and Shift+Tab back; Enter on Cancel cancels; Enter elsewhere converts with the chosen conversion; Esc closes it with nothing changed and no step; reopened, Threshold is chosen again; the strings show in German too | Verified by: automated (ui) + manual M1 (the dialog's look and keys)
- AS-2 (REQ-0034-04) Given a 6 × 1 picture #7F7F7FFF, #808080FF, #FF0000FF, #FFFF00FF, #00FF00FF, #0000FF80, When Threshold converts it, Then it is #000000FF, #FFFFFFFF, #000000FF, #FFFFFFFF, #FFFFFFFF, #00000080 (Y 127000, 128000, 76245, 225930, 149685, 29070) | Verified by: automated (core)
- AS-3 (REQ-0034-05) Given an 8 × 8 picture of #808080FF, When Ordered dither converts it, Then exactly the 32 pixels whose matrix value is below 32 are white; a 16 × 16 picture of it gives the same 8 × 8 pattern four times; #000000FF gives all black and #FFFFFFFF all white | Verified by: automated (core)
- AS-4 (REQ-0034-06) Given a 4 × 2 picture of #808080FF, When Error diffusion converts it, Then the rows are white, black, white, black and black, white, black, white; a 4 × 4 picture of #404040FF gives, row by row, B B B B / B W B W / B B B B / B W B B | Verified by: automated (core)
- AS-5 (REQ-0034-03) Given a marked selection, When the user converts it with Threshold, Then the selection floats with its pixels converted, the pixels outside it unchanged, and one Undo gives back the picture as before the lift; the same for a marked selection already black and white (it is lifted and floats, nothing else changes); Given no selection, Then the whole picture is converted as one step and Undo gives it back exactly; a picture already black and white adds no step; Given a pending curve and the dialog opened, When Cancel, Then the curve is put down and nothing else changes | Verified by: automated (ui)
- AS-6 (REQ-0034-06, REQ-0034-07) Given a picture of #808080FF and a selection of its part from (2, 1) to (5, 3), When Error diffusion converts the selection, Then the selection's pixels are those the same 4 × 3 picture alone gives; Given a free-form selection whose shape, in its 3 × 3 box of #808080FF, is the L (0, 0), (0, 1), (0, 2), (1, 2), (2, 2), Then those pixels are W, B, W, B, W in that order and the box's other pixels are unchanged; Given a 4 × 1 picture #808080FF, #80808000, #808080FF, #808080FF, Then Error diffusion gives W, W, W, B (the transparent pixel takes and passes no error; all opaque it would be W, B, W, B) | Verified by: automated (core) + automated (ui)
- AS-7 (REQ-0034-08) Given memory made too small, When the user converts the picture, Then the dialog "Could Not Convert the Picture" appears and nothing changes; the same for a selection | Verified by: automated (ui)
- AS-8 (REQ-0034-09) Given an 8000 × 8000 picture, and an 8000 × 8000 floating selection, When each conversion runs, Then the core takes less than 500 ms | Verified by: automated (perf, in the real session)
- AS-9 (REQ-0034-03, REQ-0034-05) Given a 4 × 4 selection of #808080FF floating, stretched to 8 × 8, When Ordered dither converts it, Then its 8 × 8 pixels are those an 8 × 8 picture of #808080FF gives (single-pixel dots: row 0 W B W B W B W B), and a stretch back to 4 × 4 starts from them | Verified by: automated (ui)
- AS-10 (REQ-0034-10) Given a modified picture and the dialog open, When Ctrl+Q, Then the dialog is gone, the picture unchanged and the Save prompt shows; the same with Ctrl+W | Verified by: automated (ui)

## Edge cases
- A one-pixel picture or selection: converted by its own rule; Error diffusion has nowhere to send the error.
- A fully transparent pixel: its color channels become black or white by its own color; its alpha stays 0, so nothing shows; in Error diffusion it takes and passes no error.
- A selection floating partly outside the picture: converted whole, its off-picture part too (it may be moved back).
- A text box open: put down when the dialog opens (REQ-0034-03); if that cannot get its memory, the box stays open and the dialog does not open.
- While the dialog is open the canvas and the window's keys are inert (modal), so the selection Convert acts on is the one its line named; left open, it blocks the window, and nothing changes until it is answered.
- Converting twice: Threshold and Ordered dither change nothing the second time (black and white are kept); Error diffusion likewise (a black or white pixel has no error).

## Out of scope
- XP's black-and-white mode (the palette and Attributes' Colors choice); a threshold the user sets; other dithers; a preview in the dialog (JOD chose the dialog without previews, 2026-11-09).

## Open questions
None. Settled by JOD on 2026-11-09: a command in the Image menu, on the selection too; three conversions in a dialog, Threshold the default each time; one undo step, no warning; alpha kept, palette unchanged; the dialog in words, without previews; on a floating selection, what is shown is converted. Decided in the review (routine): the put-downs when the dialog opens (as Stretch and Skew); Ctrl+Q and Ctrl+W close it as Cancel (as REQ-0021-03); the dialog's Tab order and Enter on Cancel; a marked selection is lifted even when nothing changes (as Invert); fully transparent pixels take and pass no error; the pattern's origin is the selection's own; the budget covers a floating selection too.

## Plan
**Approach.** The three conversions are one pure core function over a buffer with an optional shape mask, in whole numbers
(`src/core/black-white.{c,h}`). The whole picture goes through a new `easel_image_black_white`: the converted picture is made in
a buffer of its own (error diffusion is not a per-pixel rule, so `easel_image_transform`'s per-pixel path cannot serve), then, as
Invert's step does, a log of the changed pixels when they are at most a third of the picture, else the old buffer kept whole
(REQ-0018-09's two kinds). A floating selection goes through a new `easel_selection_black_white`: its shown image is made whole,
converted in copies, and becomes its lifted image with the orientation started again, by the same bookkeeping as Skew (the hole,
the copies made before and their kept images), moved out of `easel_selection_skew` into one helper both call. The canvas's
`easel_canvas_black_white_selection` lifts a marked selection as `easel_canvas_transform_selection` does. The window adds the item to
both menus, the action that puts down what waits and opens the dialog (as `stretch_skew_activated`), the dialog
(`src/ui/black-white-dialog.{c,h}`, the Attributes look), and `black_white_applied`, the one function every Convert goes through.

**Files.** `src/core/black-white.{c,h}` (new), `src/core/selection.{c,h}` (the helper, the new function), `src/meson.build`;
`src/ui/black-white-dialog.{c,h}` (new), `src/ui/canvas.{c,h}`, `src/ui/window.c`, `src/ui/window.ui` (two menu items); `po/*.po`,
`po/easel.pot`, `po/POTFILES` (the new file); `tests/core/test-black-white.c` (new), `tests/meson.build`; `tests/ui/test-window.c`
(`/0034`, and the menu cases that list the Image and selection menus); `tests/perf/perf-0034.c` (new); `tests/mutants.txt`.

**Interfaces.** (As built, 2026-11-09.)
- core: `typedef enum { EASEL_BW_THRESHOLD, EASEL_BW_ORDERED, EASEL_BW_DIFFUSION } EaselBw;`
  `void easel_bw_convert (uint32_t *px, const uint8_t *mask, int w, int h, EaselBw how)` (in place; mask NULL or w x h, 1 in the
  shape); `EaselTransformResult easel_image_black_white (EaselImage *, EaselHistory *, EaselBw, size_t max_bytes)`;
  `EaselSelectionResult easel_selection_black_white (EaselFloat *, EaselBw, size_t max_bytes)`.
- ui: `AdwDialog *easel_black_white_dialog_new (gboolean selection, EaselBlackWhiteApplied applied, gpointer data)`, presented by
  the window as its extra dialog (`present_extra_dialog`), so a quit closes it first (REQ-0034-10); `black_white_activated` and
  `black_white_applied` in the window; `gboolean easel_canvas_black_white_selection (EaselCanvas *, EaselBw)` ("selection-failed"
  kind 8, its dialog "Could Not Convert the Selection"). In the core, Skew's bookkeeping is `replace_image (f, w, h, max, make,
  data)` in `selection.c`, with `make_skewed` and `make_black_white`.

**What grows.** A conversion of the whole picture holds the converted buffer (4 bytes a pixel) while it runs, then either the log
(12 bytes a changed pixel, its exact length) and the buffer is freed, or the old buffer (4 bytes a pixel plus 64) in the step; of a
float, a copy of its shown image and mask (5 bytes a pixel) that becomes its image, the old one freed or kept for its copies as
Skew keeps it. Each allocation is checked against the limit before it is made.

**Readers of what changes.** The Image menu's sections: 0018's `as1_image_menus` (section 1) and the selection menu case (23844),
0028's table of menu items (28081), 0023's check of English words in every language, 0022's mnemonics if it lists the menu. Skew's
bookkeeping: 0019's selection skew cases and its core tests (they prove the moved helper). T003 runs them.

**Scenario to test.** (As built.) AS-2 to AS-4, AS-6's numbers: core `test-black-white` (`as2_threshold`, `as3_ordered`,
`as4_diffusion`, `as6_shape_and_transparent`, `matches_the_reference`, `picture_step`, `picture_undo_nothing_memory`, and the floats:
`float_stretched`, `float_turned_shaped_pasted`, `float_copies_and_memory`). AS-1: ui `/0034/as1_menus_and_dialog`, `as1_keys`; AS-5,
AS-6 on a selection: `as5_convert`; AS-7: `as7_memory`; AS-9: `as9_stretched`; AS-10: `as10_quit`. AS-8: `perf-0034` in the real
session. M1: JOD (T004).

**Measurements (T002, 2026-11-09).** `perf-0034` in the real session, load 1.7, the core alone on a patterned 8000 x 8000 picture
(every pixel a different color), three runs each: Threshold 141, 144, 143 ms; Ordered dither 174, 184, 176 ms; Error diffusion
379, 384, 384 ms; all within REQ-0034-09's 500 ms. Error diffusion works in two rows of 32-bit shares from the start (R1's
fallback), so no other change was needed.
- T003, the floating selection of the whole 8000 x 8000 picture (lifted afresh before each run, the lift not timed), two rounds at
  load 1.9 falling: Threshold 140 to 153 ms, Ordered dither 153 to 165 ms, Error diffusion 360 to 427 ms; the picture in the same
  rounds 141 to 164, 165 to 188 and 373 to 459 ms. A first round at load 6.8 (another program's tests running) had Error diffusion
  at 621 ms once on the picture and 520 ms on the float; not counted. Error diffusion's margin is the smallest (the worst run 459 ms).

**Constitution.** Core in `src/core`, libc only; P7 by the one convert function and whole numbers; no new key outside the dialog;
no ADR.

**Risks.**
- R1 Error diffusion is serial: 64 million pixels, each with four shares, against 500 ms. Measured in T002 right after the core;
  fallback, two rows of errors in 32-bit integers in place of a full-size value buffer (the value range fits: |e| <= 255000 and a
  pixel's value stays within -255000 to 510000).
- R2 Moving Skew's bookkeeping into a helper can change Skew; 0019's tests and mutants are run in T003, before the selection code
  relies on it.
- R3 The dialog's focus and keys (Enter on Cancel, Tab as one stop for the choices) are GTK's: a ui test checks what it can, and
  where the keys go after the dialog closes is a manual check in the real session (T004).

## Tasks
- [x] T001 The three conversions and the whole picture in the core (REQ-0034-04 to -07, -03's whole-picture step, -08's NO_MEMORY; AS-2, AS-3, AS-4, AS-6's L and transparent row, AS-5's no-step case) - done when `tests/core/test-black-white.c` checks the scenarios' pixels, the mask (free-form), the transparent pixels in the error flow, a reference written from the spec's words for random pictures, the one step (log or buffer, the memory held against the memory counted), no step for an unchanged picture and NO_MEMORY under a limit, red first
  Result: red by link (`undefined reference to 'easel_bw_convert'`), then 7 of 7. Hand breaks: red for the matrix transposed (the reference), Threshold strict (`px[i] == want[i]`), never a buffer step (`counted == ... + 64`); a share floored for rounded toward zero survived the random reference (a value 1 off flips a pixel only at exactly 128000), so a case was built where it decides, gray 128 then (29, 249, 252): red then. Two breaks are equivalent and stay unobservable: a share written to a fully transparent or an out-of-shape pixel is never read (such a pixel is converted by its own Y). The memory held equals the memory counted for both kinds of step.
- [x] T002 Measure (REQ-0034-09; AS-8, the whole picture) - done when `perf-0034` times each conversion of an 8000 x 8000 picture in the real session, three runs, the numbers in the Plan; R1's fallback if over 500 ms
  Result: the Plan's Measurements: 143, 178 and 382 ms medians, within 500 ms; `perf-0034` (core, no GLib) added.
- [x] T003 The floating selection in the core (REQ-0034-03's float, -05, -06 on a selection, -07; AS-6, AS-9's core part) - done when Skew's bookkeeping is one helper with 0019's core tests and selection mutants still green, and `easel_selection_black_white` converts what is shown (a stretched 2x, a turned and a free-form float, a pasted one with transparent pixels, its copies keeping their images), red first; the float's time in `perf-0034` (AS-8)
  Result: Skew's bookkeeping moved into `replace_image` (Skew now calls it with `make_skewed`): core 46 of 46, 0019's mutants REQ-0019-12 to -14 13 of 13 killed, ui 0013 to 0016, 0018, 0019 green. Float tests red by link (`undefined reference to 'easel_selection_black_white'`), then 10 of 10: a 2x stretched float gives one-pixel dots and later stretches start from them, a turned, a free-form and a pasted float are converted as shown (the shape kept, transparent pixels left out), a copy keeps its colors, NO_MEMORY leaves the float. Hand breaks, each red: the shape ignored, nothing converted. A rectangle passes no shape to the conversion (the same result, fewer tests a pixel). Timings in the Plan.
- [x] T004 The command, the dialog and the window (REQ-0034-01 to -03, -08, -10; AS-1, AS-5, AS-6, AS-7, AS-9, AS-10) - done when ui `/0034` cases (the menus, the dialog's strings, keys and default, the put-downs on opening, Convert on the picture and on marked, floating, free-form and stretched selections, the memory dialogs, Ctrl+Q and Ctrl+W) pass, red first, the menu cases of 0018 and 0028 and 0023's languages updated with old/new, the strings in all eleven .po files; ends with JOD's look at the dialog and its keys in the real session (M1), and where the keys go after it closes
  Result (before JOD's look): ui `/0034/as1_menus_and_dialog` failed first (the Image menu's section without Black and White…), then `/0034` 6 of 6. My test slips on the way: `easel_canvas_deselect` puts nothing floating down (now `easel_canvas_put_down`); a drag of Select includes its end pixel (2, 1 to 6, 4 is 5 x 4); the lift and the conversion's copy are the same size, so a selection without memory is tested floating; the Save prompt reused the freed dialog's address (now a weak pointer). One fix in the code: the dialog is the window's extra dialog, so a quit closes it first (REQ-0034-10, as About); `as10_quit` was red before it. The keys (`as1_keys`: the focus on Threshold, Down chooses the next, Tab as one stop to Cancel, Enter there cancels) passed at their first run: GTK's own (characterization; the break is T005's mutant). Convert's mnemonic is Con_vert (Cancel has C). 0018's two menu cases updated with old/new; the thirteen strings in all eleven languages, the template regenerated; full suite 124 of 124.
  Result (M1, 2026-11-09): JOD tried the three conversions on the picture and on selections, a stretched one, the dialog's keys and the focus after it: "all is good".
- [x] T005 Gate - done when the full suite and the scale-2 runs pass through `tools/isolated.sh`, the mutants of REQ-0034-03 to -08 are in `tests/mutants.txt`, `tools/mutate.sh --core` has run once (any survivor reported), and the Plan's names match the code
  Result (2026-11-09): full suite 124 of 124 (T004; since then only canvas comments and `as7_memory`, with ui-0034 and ui-0018 green again), scale-2 runs 9 of 9. Fifteen mutants for 0034 added and eight of 0018 kept unique (its finds now met my new function's lines twice; my function's comments made them differ, and the two menu finds take the new item in); lint 1311, 0 stale. By REQ (0034-02 to -10 with 0018-01, -02, -07, -08, -10): `85 mutants: 84 killed, 1 killed by crash (test gap)`: the picture's missing memory dialog crashed `as7_memory` on a NULL dialog (log kept as `crash34/crash-1309.log` in the session's scratch), so the test now asserts the dialog first; rerun `2 mutants: 2 killed`. `--core`: `467 mutants: 462 killed, 0 killed by crash (test gap), 0 inconclusive (no snapshot or skipped), 4 survived, 0 errors, 1 accepted in 183 s`, the four 0023's. The Plan's names as built.

## Check
- Date and commit: 2026-11-09, `843562e` plus the uncommitted 0031 to 0034 work; no code changed after T005's runs.
- Tests: `tools/isolated.sh meson test` 124 of 124 at T004, ui-0034 and ui-0018 again after the last test edit (this feature:
  core `test-black-white` 10 cases, ui `/0034` 6 cases, 0018's two menu cases updated); scale-2 runs 9 of 9; warnings 0.
- Mutants: 0034's 15 with 0018's touched ones `85 mutants: 84 killed, 1 killed by crash (test gap)`, the crash a NULL dialog read
  in `as7_memory` (log `crash-1309.log` kept in the session's scratch), the test fixed to assert the dialog first and the two of
  REQ-0034-08 then `2 killed`; `--core` `467 mutants: 462 killed, 0 killed by crash (test gap), 0 inconclusive (no snapshot or
  skipped), 4 survived, 0 errors, 1 accepted in 183 s`, the four 0023's, accepted at its check. Lint 1311, 0 stale. Two hand
  breaks are equivalent and have no mutant: a share written to a fully transparent or an out-of-shape pixel is never read.
- Scenarios: AS-1 ui `as1_menus_and_dialog` (red first: the menu without the item) and `as1_keys`, C (GTK's; proven by the
  mutants "the focus does not start on the chosen choice" and "the dialog opens on Ordered dither", both killed); AS-2 to AS-4 core
  (mutants: Threshold strict, the matrix transposed, the share rounded down, the share below dropped); AS-5 ui `as5_convert` (mutant:
  the selection ignored) and core `picture_step`, `picture_undo_nothing_memory` (mutants: always a log, a step for no change); AS-6
  core `as6_shape_and_transparent` (mutant: a transparent pixel takes shares) and ui `as5_convert`'s selection rows; AS-7 ui
  `as7_memory` (mutants: no picture dialog, Invert's selection dialog); AS-8 `perf-0034` (Plan's Measurements); AS-9 core
  `float_stretched` and ui `as9_stretched`; AS-10 ui `as10_quit` (red before the extra-dialog fix; mutant: a quit leaves the dialog
  open). M1: JOD, "all is good".
- Task Results: T001 to T005 each have one.
- F1 (low) REQ-0034-03's text box that cannot be put down (the dialog does not open) has no 0034 test; it is the shared
  `put_down_for_action` path that Stretch and Skew's REQ-0019-06 tests. Known gaps.
- F2 (low) REQ-0034-08's "marked again if the command lifted it" is tested for a floating selection only; the marked path is
  `transform_refused`, shared with Invert and tested by 0018. Known gaps.
- F3 (low) A free-form selection converted through the window has no ui case; the core covers the shape (`float_turned_shaped_pasted`,
  mutant: the shape ignored). Known gaps.
- F4 (low) AS-1's "the strings show in German too" has no test: 0023's check reads what the window shows in ru and uk, not the
  dialog; the thirteen strings are in all eleven files and `msgfmt -c` passes. Known gaps.
- F5 (info) REQ-0034-01's "when an item acts" (a held button, the font card's field) is the same guard as Stretch and Skew's action,
  not tested again for this item.
- F6 (info) Error diffusion's two rows of shares (8 bytes a pixel of a row) are not checked against the memory limit, which the Plan's
  "What grows" says of each allocation; a refusal still gives NO_MEMORY with nothing changed. Routine: the Plan's line names the
  converted buffer, the log and the float's copy, the rows being small.
- F7 (info) T004's keys are a characterization (F5 above in Scenarios).
- Verdict: clean for a user; four low test gaps.

### Known gaps
- Tests for the text box that blocks the dialog (F1), a marked selection whose conversion fails (F2), a free-form selection through the
  window (F3), and the dialog's strings in a translated session (F4).
