# 0008 Zoom Presets

Status: accepted
Depends on: 0001, 0004, 0007

## Why
The Magnifier's options and the zoom commands show two different lists, and the View menu cannot jump to a zoom level at
all, only step. JOD's screencast of 2026-10-18 showed the gap: reaching 8x meant the Magnifier or many steps. One list of
zoom presets, shown in the Magnifier's options and in a View > Zoom submenu, with a check mark on the current one, makes
the same levels reachable and visible in both places.

## Requirements
- REQ-0008-01 The app shall have one list of zoom presets, in percent and in this order: 10, 25, 50, 100, 200, 400, 600, 800. The 300 % of the zoom steps (REQ-0001-16) is not a preset.
- REQ-0008-02 The Magnifier's options (REQ-0007-02) shall show the presets, in the order and layout REQ-0007-02 already gives; nothing about them changes for the user (this amends REQ-0007-02 only to say that its eight levels are the presets).
- REQ-0008-03 The View menu shall hold, between the zoom row's group and Normal Size, a submenu "Zoom" that holds the presets only, one item each, in the order of REQ-0008-01, labelled "10 %", "25 %", "50 %", "100 %", "200 %", "400 %", "600 %" and "800 %". Normal Size, Large Size and Fit to Window stay where they are in the menu, with their shortcuts (REQ-0004-05, REQ-0004-07). The label `100%` of the zoom row stays a label and does not open the list (this amends REQ-0004-01, the list of the View menu's items).
- REQ-0008-04 When the user chooses a preset in the Zoom submenu, the app shall set the zoom to it, with any tool active, keeping the point at the center of the canvas area still as far as the view's edges allow (REQ-0004-06). It does nothing where REQ-0004-03 ignores the View actions (before the picture exists, while a dialog is open, during a stroke or a resize drag). It does not change the level selected in the Magnifier's options.
- REQ-0008-05 The Zoom submenu shall show a check mark on the item whose preset equals the current zoom, and no check mark when the zoom is not a preset (Fit to Window's 42 % for example, or 300 %). The mark follows every change of zoom, however it came: the submenu, the zoom row, Normal and Large Size, Fit to Window, the wheel, the pinch, the Magnifier.
- REQ-0008-06 Zoom In and Zoom Out (REQ-0004-04) shall keep their own step list, 10, 25, 50, 100, 200, 300, 400, 600 and 800 %; every preset is a step, so that the steps reach every preset.

- REQ-0008-07 The toolbox shall have the same width for every tool, whether the tool has options or not: choosing another tool never changes it. At the default text size the width is 87 pixels, that of the two-column grid of tool buttons (reference measurements from JOD, 2026-10-18). To keep every tool's options inside it, the Brush, Airbrush and Eraser presets and the Magnifier's eight level buttons shall be compact: the same shapes, sizes, labels, tooltips, order and two columns as REQ-0006-04, REQ-0006-07, REQ-0006-10 and REQ-0007-02, drawn or labelled smaller, with less padding. At a larger text size the width is that of the widest content the toolbox then holds (the labels grow, the tool buttons do not), still the same for every tool, and no label is cut (this amends the look of the presets of 0006 and the buttons of REQ-0007-02 only).

## Scenarios
- AS-1 (REQ-0008-01, REQ-0008-02, REQ-0008-03) Given the app just started, When the user opens View > Zoom, Then the items are 10 %, 25 %, 50 %, 100 %, 200 %, 400 %, 600 % and 800 %, and the Magnifier's options, when the Magnifier is chosen, show 1/10x, 1/4x / 1/2x, 1x / 2x, 4x / 6x, 8x with the tooltips 10 % to 800 %: the same eight levels | Verified by: automated (ui)
- AS-2 (REQ-0008-04) Given a 3000 × 2000 picture at 100 % with Pencil active and a canvas area of 1390 × 853, When the user chooses 800 % in View > Zoom, Then the zoom is 800 % and the picture point that was at the center of the canvas area is still there; When the user chooses 100 %, Then the zoom is 100 %; the choice in the Magnifier's options is unchanged, and no pixel, undo step or modified mark changed | Verified by: automated (ui)
- AS-3 (REQ-0008-05) Given 100 %, Then 100 % is checked and only it; When the user presses Ctrl+PageDown (400 %), Then 400 % is checked; When the user zooms in once from 400 %, Then 600 % is checked; When the user zooms out from 400 % (to 300 %), Then no item is checked; Given Fit to Window gave 42 %, Then no item is checked; When the user then presses the Magnifier on the picture with 4x chosen, Then 400 % is checked | Verified by: automated (ui)
- AS-4 (REQ-0008-04) Given the color chooser open, a stroke in progress or a window whose picture does not exist yet, When the user chooses a preset, Then nothing changes | Verified by: automated (ui)
- AS-5 (REQ-0008-06) Given any preset below 800 %, When the user zooms in, Then the zoom is the next step, which is the next preset, except from 200 % where it is 300 %; and zooming out from 400 % gives 300 %, then 200 % | Verified by: automated (core)
- AS-6 (REQ-0008-07) Given the app at the default text size, When the user chooses each tool in turn (Pan, Pencil, Brush, Airbrush, Eraser, Magnifier, Pick Color, Fill), Then the toolbox is 87 pixels wide each time, and every preset of the Brush, Airbrush and Eraser and the eight level buttons of the Magnifier are fully shown; Given the text size at 150 % and at 200 %, Then the toolbox is the same width for every tool at each size, wider than 87 pixels only as far as its widest content needs, and no label is cut | Verified by: automated (ui, `as6_toolbox_width_is_the_same_for_every_tool`, which measures the toolbox after each choice at three text sizes) + manual M1 (the look of the compact presets and buttons)

## Edge cases
- A zoom that is not a preset (42 %, 300 %): no check mark; choosing any preset still works.
- Choosing the preset that is already the zoom changes nothing (no scroll, no redraw needed).
- The submenu is open while the zoom changes by the wheel or a shortcut: not possible with the menu's popover modal, so no rule.
- At 800 % Zoom In is insensitive (REQ-0004-02) and the 800 % item is still shown, checked.

## Out of scope
- A Custom Zoom… entry or any list the user edits (Preferences).
- Keyboard shortcuts for single presets.
- A margin around a small picture and the workspace color (the view polish feature).
- Changing the Magnifier's levels or the zoom steps (only the check that every preset is a step is new).

## Open questions
None. Settled by JOD on 2026-10-18: the items are labelled "10 %" … "800 %"; the submenu holds the presets only, and Normal Size, Large Size and Fit to Window stay where they are; the `100%` label of the zoom row does not open the list. The submenu's place among the View menu's items (after the zoom row, before Normal Size) was the draft's choice and is confirmed by the acceptance of the spec.

## Plan
(added by /sdd-build after the spec is accepted: approach, files, key interfaces, scenario-to-test map, risks)

## Tasks
(added by /sdd-build after the plan is agreed: `- [ ] T001 title (REQ-..., AS-...) - done when ...`)
(filled in when the task is ticked, a line under it: `  Result: ...`. A code task: the test that failed first and its message,
or `characterization: <why there was no red run>`. A manual step: one line of what JOD saw.)

## Check
(written by /sdd-check as its last step, about ten lines)
- Date and commit: YYYY-MM-DD, `abc1234`
- Tests: `meson test` N of N pass (the cases of this feature: M)
- Mutants: K of K killed (seconds); none killed only by a crash
- Warnings: 0
- Findings, each with how it was settled (fixed in T0NN, decided by the owner, left open and why)
