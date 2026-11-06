# 0028 Phase 1 acceptance

Status: implemented
Reviewed: spec-reviewer, 2026-11-04
Depends on: every feature of Phase 1 built so far (0001 to 0023, 0025 to 0027)

## Why
Each feature is tested alone, so nothing yet tests the seams between them. These fifteen journeys walk through the app as a person does, across features, and each ends in the place that matters: the saved file, read back. The spec adds no behavior to the app. What a journey finds wrong is a bug against the feature that owns it.

## Requirements
### How every journey runs
- REQ-0028-01 Each journey shall be one automated case and one numbered step of a manual walkthrough with the real mouse and keys. The automated case drives the app through what a person uses: its actions, its keys and its press, motion and release on the canvas. Where a person answers a system dialog (a file chooser, a color chooser) or uses the desktop's clipboard, the case may use the seam the feature's own tests use, and the manual step covers the real one. Each journey starts from its own empty configuration, and the walkthrough runs with a throwaway configuration folder, not the owner's. A restart in a case is a new window reading the files, in the walkthrough a real one.
- REQ-0028-02 Each journey shall end by saving the picture, opening that file again, and comparing every pixel with the picture as the app held it before the save, by the sameness of REQ-0005-06. A journey says what differs if its format cannot hold the picture, and says so if it ends otherwise (journeys 11 and 13). Pixel equality is the automated case's; the manual step looks by eye.
- REQ-0028-03 All fifteen automated cases together shall take at most 2 minutes under the software renderer in the isolated session on the owner's machine, in the normal test run; going over is reported to JOD and fails nothing. They shall run once more under Vulkan at the phase gate, and journeys 1 and 4 also on a screen of scale 2, under the software renderer. They have no mutants of their own: their job is to catch what falls between features, not to prove a rule.
- REQ-0028-04 A journey that fails shall be fixed in the feature that owns the behavior, with a test there as well if the feature's own tests should have seen it, and a line in `docs/learning-log.md`. The journey's expected result is changed only if the owner's spec says the journey was wrong.

### The journeys
Each journey below shall run as written and shall give the result written. Colors and sizes not named are the Plan's, stated before the case is written; each picture is asymmetric where a flip or a turn is tested.
- REQ-0028-05 Journey 1, First picture. From a first start, with a picture size the settings fix: draw with the Pencil, a Line and a Rectangle with Outline and fill, in two colors, Save As PNG, quit, start again with that file. Every pixel is the same, the document is not modified and Undo is off (0001, 0003, 0005, 0010).
- REQ-0028-06 Journey 2, Poster. Grow the picture by Attributes, change the background color, crop it by a handle, draw filled shapes, type a text box with one bold word by Ctrl+B, put it down, undo every step to the first state and redo them all. The picture at the end equals the one before the undoing, also where Redo refills a grown area in the old background color (0002, 0004, 0010, 0017).
- REQ-0028-07 Journey 3, Fix a screenshot. Open a PNG with some partly transparent pixels, magnify a part, pick a color from it, fill an area of another color with it, edit swatch 1 to an opaque color and paint with it, Save As BMP. The window keeps the partly transparent pixels until the file is read back; there each is flattened on white by REQ-0005-11's formula and every other pixel is the same (0003, 0005, 0007).
- REQ-0028-08 Journey 4, Pixel art. At 800 % with the grid on, draw single pixels with the Pencil; then at a fractional zoom and at 100 % the picture's pixels are unchanged. At 800 % each cell is 8 × 8 device pixels of its color, except its grid lines; at the fractional zoom each cell has the sides and edges of REQ-0025-02; at 100 % each cell is one device pixel; no grid line shows at either; no grid line is in the file (0004, 0006, 0008, 0025).
- REQ-0028-09 Journey 5, Rectangle selection. Each of these with a new mark, ended by Enter: a drag (the hole takes the background color); Ctrl and a drag (no hole); Shift and a drag (no hole, a trail); Shift+Right three times (the same trail as a Shift drag by 3); in Transparent mode a drag over a colored ground (the hole is #00000000, and the selection's background-colored pixels cover the ground, as in Opaque mode, as REQ-0013-13 says; JOD, 2026-11-04: Easel's rule stays). After the last, Undo gives the picture before the lift, with no selection; Redo gives the put-down picture, and that is saved (0013, 0014).
- REQ-0028-10 Journey 6, Free-form cut. Trace an asymmetric shape, Cut (its pixels take the background color), Paste (a rectangle whose outside pixels leave the picture as it is, at the top-left in view), stretch it to twice its size by a handle, drag the right handle past the left edge, put it down. The picture equals the bounding rectangle's copy scaled by the nearest pixel and mirrored (0014, 0015, 0016, 0018).
- REQ-0028-11 Journey 7, Oversize paste. On a 400 × 300 asymmetric opaque picture: Select All, Copy To big.png, Attributes to 200 × 150. Paste From big.png, Fit Picture to Canvas, Enter: the picture stays 200 × 150 and holds big.png scaled by the nearest pixel. Undo restores the 200 × 150 picture. Paste From again, Resize Canvas to Picture, Enter: the picture is 400 × 300 and the saved file equals big.png, pixel for pixel. The prompt's Cancel is 0016's own test (0004, 0005, 0016).
- REQ-0028-12 Journey 8, Rich text. Paste formatted text in an installed family into a text box, restyle part of it, zoom to a fractional zoom and to 800 % while the box is open, put it down. It equals a box made at 100 % by the same drag in the same mode, with the same text and restyle (0004, 0016, 0017).
- REQ-0028-13 Journey 9, Transforms. On the picture: Rotate 90°, Flip Horizontal, Invert Colors, Stretch and Skew: four steps. Mark a rectangle, apply the same four, Enter: one step. Five Undos give the first picture and its size, exactly; that picture is saved (0018, 0019).
- REQ-0028-14 Journey 10, Interrupted tools. For each of a curve after its first stage, a polygon with two sides and a gradient: a zoom leaves it waiting; then, in three runs, Esc drops it with no step, Enter puts it down with one step, another tool puts it down with one step. The saved file holds the six put down and none of those dropped (0004, 0011, 0012).
- REQ-0028-15 Journey 11, Don't lose work. On an existing file, edited, close: Cancel at the prompt stays; a Save into a folder that cannot be written stays, with its error, the picture and the old file as they were. In a second run on a writable file a Save closes with the new file; in a third, Discard closes with the file byte for byte as it was. It ends with the file read back in each (0002, 0005).
- REQ-0028-16 Journey 12, Big picture. With `[history] memory=16` set before the start and an 8000 × 8000 picture made by Attributes: at 1600 % mark a 40 × 20 rectangle, move it, Enter; at 100 % (JOD, 2026-11-04: a hand cannot do this much at 1600 %), four times at different places, mark a 1000 × 700 rectangle, Invert Colors, Enter; Undo, pressed to its end, no longer reaches the move (a filled rectangle's step counts only its spans, so it would take thousands to pass the budget, and a put-down selection counts each pixel, by REQ-0002-16). The move's step went first; undo stops at the oldest step kept, on the picture from before that step, with Undo off; the title stays modified if the saved state was dropped; the picture saves and reads back (0002, 0004, 0005, 0008, 0013, 0025).
- REQ-0028-17 Journey 13, Preferences. With swatch 1 edited beforehand and a settings file holding a comment: set the new-picture size to 300 × 200 and a canvas color, change Workspace color, press Reset Palette, Apply; change Tint strength, Cancel; restart. The kept values show and are used, New has the kept size and color, the classic colors show, the cancelled tint does not, and the comment stays. New's picture is drawn on, saved and read back (0003, 0009, 0020, 0027).
- REQ-0028-18 Journey 14, Keyboard only. Started with a file as argument, with no pointer used: F6 to the toolbox, arrows and Space to choose the Pencil, on to a swatch and choose it, F6 to the canvas, draw by Space, arrows, Space; choose Select, mark by Space, arrows, Space; once released, arrows move it and Shift with an arrow smears it; Enter puts it down; Ctrl+S saves (0005, 0013, 0014, 0022).
- REQ-0028-19 Journey 15, Another language. Journey 1 with the same input and picture size, from a settings file holding `language=uk`, under `LC_ALL=C`, with the translated suggested file name: a menu item reads in Ukrainian and the file equals journey 1's, pixel for pixel (0023).

## Scenarios
- AS-1 (REQ-0028-01, REQ-0028-02) Given each journey, Then it has one automated case and one step in the walkthrough, starts from an empty configuration, and its case ends by reading the saved file back or says why it ends otherwise | Verified by: automated (the list of cases) + manual (the walkthrough, JOD)
- AS-2 (REQ-0028-03) Given the fifteen cases, When they run in the isolated session under the software renderer, Then they pass and their time is reported against 2 minutes; When the phase gate runs, Then they pass under Vulkan, and journeys 1 and 4 at scale 2; no mutant in the list names them | Verified by: automated (the run's time, the list)
- AS-3 (REQ-0028-04) Given a journey that fails, Then the fix is in the feature that owns the behavior and the log has its line | Verified by: manual (the spec's Check)
- AS-4 (REQ-0028-05) Given a first start, When journey 1 runs, Then the reopened file equals the picture, pixel for pixel, the title has no mark and Undo is off | Verified by: automated (ui, with a new window) + manual step 1, with a real restart
- AS-5 (REQ-0028-06) Given journey 2, When every step is undone, Then the picture is the first one; When all are redone, Then it equals the one before the undoing, in the old background color in the grown area; Then the file reads back the same | Verified by: automated (ui) + manual step 2
- AS-6 (REQ-0028-07) Given a PNG with partly transparent pixels, When journey 3 runs, Then the BMP reads back with those pixels flattened on white and every other pixel the same, and the filled area and the painted pixels are in it | Verified by: automated (ui, the color chooser's seam) + manual step 3
- AS-7 (REQ-0028-08) Given journey 4, Then the rendered window shows the cells as the requirement says at each zoom, the picture's pixels are unchanged, and the file holds no grid | Verified by: automated (ui, a rendered window, the fractional zoom from Fit to Window at a chosen window size) + manual step 4, at whatever zoom Fit to Window gives
- AS-8 (REQ-0028-09) Given journey 5, Then each of the five gives what the requirement says, and the file reads back the same | Verified by: automated (ui) + manual step 5
- AS-9 (REQ-0028-10) Given journey 6, Then the picture equals the copy scaled and mirrored, and the file reads back the same | Verified by: automated (ui, the clipboard's seam) + manual step 6
- AS-10 (REQ-0028-11) Given journey 7, Then the sizes and pixels are as the requirement says, and the saved file equals big.png | Verified by: automated (ui, the file chooser's seam) + manual step 7
- AS-11 (REQ-0028-12) Given journey 8, Then the put-down after zooming equals the box made at 100 %, and the file reads back the same | Verified by: automated (ui, the clipboard's seam) + manual step 8
- AS-12 (REQ-0028-13) Given an asymmetric picture and journey 9, When five Undos are done, Then every pixel is the first picture's, and its size; the file reads back the same | Verified by: automated (ui) + manual step 9
- AS-13 (REQ-0028-14) Given journey 10, Then each of the nine ends as the requirement says, with its count of steps, and the file holds the six | Verified by: automated (ui) + manual step 10
- AS-14 (REQ-0028-15) Given journey 11, Then Cancel leaves the window and the edit; the failed Save leaves them, with the error and the old file byte for byte; the working Save closes with the new file; Discard closes with the old file byte for byte | Verified by: automated (ui, a folder that cannot be written; skipped as root) + manual step 11
- AS-15 (REQ-0028-16) Given journey 12, Then the move's step is dropped first, undo stops at the oldest step kept, on the picture from before it, and the file reads back the same | Verified by: automated (ui) + manual step 12
- AS-16 (REQ-0028-17) Given journey 13, When the app starts again, Then the kept values are shown and used and the cancelled one is not; the comment stays; New's picture reads back the same | Verified by: automated (ui, a new window reading the files) + manual step 13, with a real restart
- AS-17 (REQ-0028-18) Given journey 14, Then the case sends no pointer event, and in the manual step the mouse is not touched; the file reads back the same | Verified by: automated (ui) + manual step 14
- AS-18 (REQ-0028-19) Given journey 15, Then a menu item reads in Ukrainian and the file equals journey 1's, pixel for pixel | Verified by: automated (ui, its own run) + manual step 15, started from a terminal under `LC_ALL=C`, looked at by eye

## Edge cases
- The manual walkthrough is one sitting of about half an hour; a step that fails is reported and the rest go on.
- If the fifteen pass 2 minutes because of journey 12, its size changes only with JOD's word, as its point is the size.
- The app's order of OK and the like, the other dialogs' prompts and their third answers stay with their features' tests.

## Out of scope
- New behavior; mutants of the journeys; Black and white (Phase 2, `docs/phase2-plan.md`); the fractional desktop scale and the phone photo, separate checks after this spec; packaging.

## Open questions

## Plan
**Approach.** No code in the app: fifteen cases in the ui test program, `/0028/journey_N_name`, built on the helpers the features' own tests use, with `journey_end`, which saves, opens the file again and compares every pixel with the picture held before the save. A failure is read first: a mistake in a case is the case's, and a real bug goes to the feature that owns it, its spec old and new, test first.

**Runs.** `ui-0028` in the normal suite, the software renderer. Journey 15 in `ui-0028-uk`, its own process, with `language=uk` from the settings file and `LC_ALL=C`. Journeys 1 and 4 again in `ui-0028-scale2`, which skips off a screen of scale 2, like the other scale 2 tests. Vulkan is a command of the gate and not a test of the normal suite, which `docs/engineering.md` names: `tools/isolated.sh env GSK_RENDERER=vulkan ... build/tests/test-window -p /0028`.

**Files.** `tests/ui/test-window.c`, `tests/meson.build`, `docs/engineering.md`, `docs/walkthrough-phase1.md`.

**Oracles.** Journey 1 and 15's picture is a model written from REQ-0010's rules, so that two processes can be compared with one thing and neither with itself. Journey 3's flatten is REQ-0005-11's formula. Journey 5 uses the models of 0013's and 0014's tests. Journey 7 is big.png scaled by the nearest pixel. Journey 8 compares with the same box made at 100 %.

**Risks.** R1 Journey 12's 8000 × 8000 save and read may take most of the budget: measured at T002, and its size moves only at JOD's word. R2 Vulkan in the headless session: `ui-0025-vulkan` runs there today.

**Scenario map.** AS-1 the list of cases and `docs/walkthrough-phase1.md`; AS-2 the times in the Check; AS-3 the Check; AS-4 to AS-18 `/0028/journey_1` to `journey_14` and `/0028-uk/journey_15`, each with its step of the walkthrough.

Constitution: no app code, so P1 to P7 are untouched unless a journey finds a bug.

## Tasks
- [x] T001 The fifteen journeys and their runs (REQ-0028-01, -02, -05 to -19; AS-1, AS-4 to AS-18) - done when all pass, each first run reported
  Result: first runs: 7 passed (1, 3, 4, 5, 8, 10, 13) plus 15 in its own process, 7 failed. All seven failures were mistakes in the cases, none in the app: colors set on the canvas and not the palette (2), Pan has no handles (2), an off-by-one column (6), the nearest pixel taken at a corner and not a cell's middle (7), Save on an untitled picture opening its chooser (9), the same stroke again changing nothing and a wait for no dialog (11), a walk that could not change column (14). And one in the spec: journey 12's rectangles never passed the budget, as a filled rectangle's step counts its spans, so its step changed, old and new in T003. Journey 14's smear first looked wrong, a 3-row selection covering its own copies: that is REQ-0014-11, and the journey marks one row. As the passing ones had shown nothing, breaks: one pixel changed at every save failed 12 of 14 (not 9 and 11, which do not go that way); an Undo step skipped and the quit prompt gone failed 7, 9, 10 and 11. Sources restored and the suite green again.
- [x] T002 The time, Vulkan and scale 2 (REQ-0028-03; AS-2) - done when the times are here
  Result: software renderer in the isolated session: `ui-0028` 25.85 s and `ui-0028-uk` 0.39 s, 26.2 s of the 120 s budget. Vulkan: 15 of 15, 26.6 s. Scale 2: journeys 1 and 4 pass, `ui-0028-scale2` 2.81 s.
- [x] T003 What the journeys find (REQ-0028-04; AS-3) - done when each is fixed in its feature, or there is none
  Result: the journeys found no bug in the app, so no feature's spec changed. One change here, after acceptance (JOD, 2026-11-04, shown): REQ-0028-16 old, "draw filled 500 × 500 rectangles in turn until Undo, pressed to its end, no longer reaches the move"; new, four times mark a 1000 × 700 rectangle, Invert Colors, Enter.
- [x] T004 The full mutation run, its versions in the INDEX - done when its summary line is here and each survivor has its line
  Result: at `2e6954e`, 8 workers: `1258 mutants: 1247 killed, 0 killed by crash (test gap), 0 inconclusive (no snapshot or skipped), 8 survived, 0 errors, 2 accepted, 1 long-only (not run) in 735 s`. The long-only one, REQ-0014-16, killed by hand: `as14_long_trace`. GTK 4.24.1, Pango 1.58.0, Cairo 1.18.4, GNOME Shell 51.0, the same as at the last full run. Survivors, each accepted, none showing a bug a user can see:
  - REQ-0023-06, Romanian's few stopping before 19: AS-5's numbers have no 19, and the rule as written is right.
  - REQ-0023-02, an empty translation showing as nothing: `translations` refuses an empty message before it ships.
  - REQ-0023-06, a file with fewer forms than the rule: `translations` refuses it before it ships, and no test feeds the reader one.
  - REQ-0023-05, a cut file read past its end: only a catalog damaged after install reaches it, and no test feeds the reader one.
  - REQ-0013-10, the arrows moving a selection while a button is held: the canvas's own keys stop them first, so this guard, with the focus off the canvas, is not reached by a test.
  - REQ-0013-10, the arrows taken without a selection, with the focus off the canvas: the code is right, and since 0022 no test presses an arrow there with no selection.
  - REQ-0022-07, a keyboard press counting as a click of a double click: the code is right, and no test presses Space twice quickly on a polygon.
  - REQ-0014-11, Shift with an arrow lifting with a hole: equivalent, as the copy it leaves at the place it came from covers that hole exactly.
- [x] T005 `docs/walkthrough-phase1.md` - done when each step has its action and result, in a throwaway settings folder
  Result: fifteen steps, in a throwaway settings folder with a command that builds it; its setup was run once, and the three pictures it makes open.
- [x] T006 Manual: the walkthrough (JOD) - done when JOD has walked it
  Result: walked by JOD, 2026-11-06: every step as expected, no comments.

## Check
- Date and commit: 2026-11-06, `eb4df93` (0024's packaging on top of the gate's `2e6954e`; its save path changed `src/ui/window.c`, so everything below was run again here).
- Tests: `meson test` 112 of 112 through `tools/isolated.sh`; `ui-0028` 25.70 s and `ui-0028-uk` 0.37 s, 26.1 s of the 120 s budget. Scale 2: the four scale 2 runs and `ui-0028-scale2` (journeys 1 and 4) pass, 2.78 s. Vulkan, the command in `docs/engineering.md`: `/0028` 14 of 14 and `/0028-uk` 1 of 1, about 29 s.
- Mutants, the full list: `1262 mutants: 1251 killed, 0 killed by crash (test gap), 0 inconclusive (no snapshot or skipped), 8 survived, 0 errors, 2 accepted, 1 long-only (not run) in 841 s`. The long-only one, REQ-0014-16, killed by hand by `as14_long_trace` in a separate worktree. GTK 4.24.1, Pango 1.58.0, Cairo 1.18.4, GNOME Shell 51.0: none changed since the gate. No mutant names a journey (REQ-0028-03).
- Survivors: the same eight as T004's, each accepted there with its reason, none a bug a user can see (REQ-0023-06 twice, REQ-0023-02, REQ-0023-05, REQ-0013-10 twice, REQ-0022-07, REQ-0014-11).
- Warnings: 0.
- Scenarios: AS-1, AS-4 to AS-18 have their cases, `/0028/journey_1` to `journey_14` and `/0028-uk/journey_15`, each proven by T001's breaks (a pixel changed at every save failed 12 of 14; an Undo step skipped and the quit prompt gone failed 7, 9, 10 and 11). AS-2 by the times above; AS-3 by T003 (no app bug found). Results T001 to T005 are in each task; none a characterization.
- F1, fixed (spec wording, JOD, 2026-11-06): REQ-0028-18 old "Tab to the toolbox", new "F6 to the toolbox", as the case and walkthrough step 14 do (F6 is 0022's panel key).
- F2, fixed (walkthrough, JOD, 2026-11-06): step 8 zoomed to 800 % and 100 %; REQ-0028-12 names a fractional zoom and 800 %. Step 8 now reads "View, Fit to Window; then View, Zoom, 800 %".
- F3, noted: AS-17's "no pointer event" holds by construction (the case calls only key helpers), not by an assertion; Ctrl+S in journey 14 is the action and a check of its accelerator, the real key is walkthrough step 14's.
- F4: T006 walked by JOD, 2026-11-06, no comments. Open: the fractional desktop scale and the phone photo (Out of scope) have no written steps yet.
- Verdict: clean after F1 and F2. `Status: implemented` (JOD, 2026-11-06). Next: JOD tests the app in daily use over the coming weeks, before a first beta.
