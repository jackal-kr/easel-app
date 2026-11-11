# 0036 Wheel zoom in every tool

Status: implemented
Reviewed: spec-reviewer, 2026-11-09 (the version without the slider; its findings merged, the merged text not reviewed again)
Depends on: 0001, 0004, 0006, 0007, 0010, 0013, 0017, 0021, 0022, 0031

## Why
Zooming needs Pan today: Ctrl+wheel and the pinch work only there (REQ-0001-17, REQ-0004-08), so a user drawing with the pencil switches tools to look closer, then switches back. With this feature Ctrl+wheel and the pinch zoom with every tool, as they do in Pan. (Phase 2 row "Zoom slider and wheel zoom in every tool"; JOD, 2026-11-09: the slider in the status bar was built and then dropped, "it is too much"; the wheel zoom stays.) Before, the zoom commands stopped only during a stroke or a resize drag, and Pan's Ctrl+wheel and pinch zoomed during its drag; a fast wheel could move the view, since several zooms before one redraw each started from the view before the first.

Changes to implemented specs (each with a pointer note there): REQ-0001-16's opening "While Pan is active", REQ-0001-17 and AS-15 (Ctrl+wheel with other tools), REQ-0008-04's second sentence (the Zoom submenu while something is held), REQ-0004-03 (what is held), REQ-0004-04's last clause, REQ-0004-08's last sentence and AS-7's last clause (the pinch with other tools), 0017's Plan note that Ctrl+wheel does not zoom with Text.

## Requirements
Throughout: "the levels" are REQ-0001-16's ten zooms, 10, 25, 50, 100, 200, 300, 400, 600, 800 and 1600 %; "something is held" means a mouse button pressed in the canvas area (the picture, its margin and its handles; not its scrollbars, the thumbnail card or the font card) and not yet released, whatever the tool does with it (for example a stroke, a shape, a curve's bend, a gradient, a selection or handle drag, a text box drawn or moved, a resize drag, Pan's drag, a Fill, Pick Color or Magnifier press), or a keyboard press held (REQ-0022-06); it ends at the release, or, when the release never reaches the window, at the next pointer motion or press with no button down, as REQ-0007-18 says, for every hold named here (Pan's drag and a resize drag included). This feature adds no zoom value: every zoom it sets is a level, or a step from a Fit to Window zoom to the next level (REQ-0004-04), so the view's rules (P7.2, REQ-0001-18, REQ-0031-01) hold as for Zoom In and Zoom Out; it never changes the picture, the undo history or the Magnifier's chosen level (REQ-0008-04).

- REQ-0036-01 (Changes REQ-0001-17, REQ-0004-04's last clause and REQ-0004-08's last sentence.) Ctrl+wheel (REQ-0001-16) and the pinch (REQ-0004-08) shall zoom with every tool exactly as they do in Pan: one level per notch, or per pinch scale of 1.5, wheel up larger, the picture point under the pointer, or the pinch's center, kept still as far as the view's edges allow; at 10 % and 1600 % further notches that way do nothing; a touchpad's Ctrl+scroll counts in whole notches. Ctrl+Shift+wheel zooms as Ctrl+wheel does (as in Pan); a horizontal scroll with Ctrl scrolls the view sideways as without Ctrl. They work anywhere over the canvas area (the picture, its margin, its handles and its scrollbars), the thumbnail card and the font card included (REQ-0021-07, REQ-0017-14), the font card's size field too, whatever has the focus (JOD, 2026-11-09); the wheel without Ctrl keeps its rules there, and a zoom leaves the size field's text and focus as a scroll by the wheel does. An open text box stays open, and a pending curve, polygon or gradient stays pending, as with Zoom In and Zoom Out. While a dialog, the quit prompt, the color chooser, a menu or a popover is open, they do nothing (REQ-0004-03).
- REQ-0036-02 (Changes REQ-0004-03.) While something is held, nothing shall zoom (JOD, 2026-11-09): not Ctrl+wheel, which then scrolls the view as the wheel without Ctrl does (REQ-0001-15; a shape's end moves at the next pointer motion, REQ-0010-21); not the pinch, its scale starting again when nothing is held; and not the zoom commands of REQ-0004-03 (Zoom In, Zoom Out, Normal Size, Large Size, Fit to Window and the Zoom submenu). A touchpad's partial notch is dropped when something comes to be held. The other View and Image actions keep REQ-0004-03 as it was.
- REQ-0036-03 (JOD, 2026-11-09.) On a touch screen a one-finger touch in the canvas area shall not draw, select, pan, pick or press anything, with any tool, as before this feature; two fingers pinch-zoom with every tool (REQ-0036-01).
- REQ-0036-04 However quickly notches of Ctrl+wheel, levels of a pinch or zoom commands follow each other, the view shall end where it would if each were shown before the next: the picture point under the pointer kept for the wheel, the pinch's center for the pinch, the picture point at the center of the canvas area for a command, as far as the view's edges allow.

## Scenarios
- AS-1 (REQ-0036-01) Given Pencil, Select with a floating selection, an open text box and a waiting curve, each in turn, at 100 %, When Ctrl+wheel turns one notch up with the pointer over pixel (40, 30), Then 200 % with pixel (40, 30) under the pointer, the picture unchanged, the box open and the curve waiting; When one notch down, Then 100 %; When Ctrl+Shift+wheel turns one notch up, Then 200 %; Given 1600 %, When one notch up, Then nothing changes, and at 10 % one notch down nothing; Given Pencil, When Ctrl+wheel turns with the pointer over the margin and over a resize handle, Then it zooms; When a touchpad's Ctrl+scroll gives 0.6 of a notch twice, Then one level larger; When a touchpad pinch reaches 1.5, Then one level larger around its center; Given the Zoom submenu open, When Ctrl+wheel turns, Then the zoom stays | Verified by: automated (ui) + manual M1 (a real wheel and touchpad)
- AS-2 (REQ-0036-01) Given Pencil with the thumbnail shown, When Ctrl+wheel turns over the thumbnail card, Then it zooms around the canvas point under the pointer; Given an open text box, When Ctrl+wheel turns over the font card and over its size field, with and without the field's focus, Then it zooms, and the field keeps its text and focus; When the wheel without Ctrl turns over the focused size field, Then the size steps as before | Verified by: automated (ui)
- AS-3 (REQ-0036-02) Given Pencil with the primary button held in a stroke, When Ctrl+wheel turns one notch down, Then the zoom stays and the view scrolls as without Ctrl; When Ctrl+= is pressed, or Fit to Window or the Zoom submenu's 400 % is chosen, Then the zoom stays; Given a line being dragged, a selection being moved, a text box being drawn, Pan's drag, a resize drag and a held Space press, each in turn, Then the same; Given a pinch at scale 1.3 when the primary button is pressed, When its scale reaches 1.6 during the stroke, Then the zoom stays, and When the button is released and the scale then grows by 1.5, Then one level larger; Given a stroke and Pan's drag whose release is lost, When the pointer moves with no button down and Ctrl+wheel then turns, Then it zooms; Given Pan's drag held, When Flip Horizontal is chosen, Then it acts as before (REQ-0004-03 unchanged for it) | Verified by: automated (ui)
- AS-4 (REQ-0036-03) Given Pencil on a touch screen, When one finger touches the picture and moves, Then nothing is drawn; When two fingers pinch, Then the zoom changes and nothing is drawn | Verified by: manual M2 (a touch screen; without one, recorded as not run in Check)
- AS-5 (REQ-0036-04) Given an 8000 × 8000 picture at 100 % in a canvas area of 1390 × 853 logical pixels at scale 1, the center of pixel (4000, 4000) at the area's center and the pointer there, When two notches of Ctrl+wheel up come at once, Then 300 % and the picture point that was under the pointer is still under it, within one device pixel; When two notches up and three down come at once, Then 50 % and the picture point that was under the pointer is still under it, within one device pixel (at 50 % that is two picture pixels); When the pointer then moves to the area's top left quarter and Zoom In is chosen twice at once, Then 200 % with the picture point that was at the area's center still there, within one device pixel | Verified by: automated (ui)

## Edge cases
- The picture smaller than the view: a zoom keeps it at the margin's top left (REQ-0004-06).
- No picture yet (REQ-0001-39): Ctrl+wheel and the pinch do nothing.
- Ctrl+wheel over the toolbox, the palette, the options panel, the status bar or the header bar: not over the canvas area, so no zoom (each keeps its own wheel, if any).
- A pinch that started while nothing is held, then a mouse button pressed on the canvas: from then on nothing zooms; the pinch's scale starts again at the release.
- Pan's hint "Drag to pan. Ctrl+wheel zooms." stays as it is; no other hint names the wheel.
- What a hold does when the view scrolls under it by the wheel, and the status bar's position after a zoom with the pointer still: as they already are with the wheel without Ctrl (REQ-0001-15, REQ-0010-21), nothing new.

## Out of scope
- A zoom slider or zoom buttons in the status bar (built and dropped by JOD, 2026-11-09: "too much").
- Zooms other than the levels; a zoom field to type a percent; Ctrl+wheel over other panels.

## Open questions
None. Settled by JOD on 2026-11-09: Ctrl+wheel and the pinch with every tool, as in Pan; nothing zooms while something is held; Ctrl+wheel zooms over the font card's size field too; a finger draws nothing on a touch screen and two pinch; the slider removed.

## Plan
**Approach.** The canvas decides every wheel and pinch zoom, so the change sits there: the Pan-only checks of `easel_canvas_wheel`,
`easel_canvas_zoom_at` and `easel_canvas_pinch` go, and one predicate, "something is held", stops them; the window's zoom actions
(Zoom In, Zoom Out, Normal Size, Large Size, Fit to Window, the Zoom submenu) check it too. The window gives the canvas a gate for
the wheel and the pinch (the picture exists, no dialog, no menu or popover open: `view_actions_allowed` and `menu_is_open`), so
the rule is in code and testable, not only in the input grab. The font card forwards Ctrl+wheel to `easel_canvas_wheel` as the
thumbnail already does. A lost release ends Pan's drag and a text box's drag as it ends the other holds (REQ-0007-18's
`easel_canvas_buttons_released`). Several zooms before one allocation start from the scroll the previous one set, not from the
adjustments (`zoom_around`, the one function the wheel, the pinch and the commands' `easel_canvas_set_zoom` go through).

**Files.** `src/ui/canvas.{c,h}`, `src/ui/window.c`, `tests/ui/test-window.c` (`/0036`, and the cases below), `tests/meson.build`
(`ui-0036`), `tests/mutants.txt`. No core change, no string.

**Interfaces** (as built).
- canvas: `gboolean easel_canvas_is_holding (EaselCanvas *self)`: `easel_canvas_is_stroking` (strokes, shapes, picks, selection
  and handle drags, the text drag), panning, resizing, a cancelled button still down, a Fill, Pick Color or Magnifier press
  (`action_button`), a keyboard press held; `typedef gboolean (*EaselZoomGate) (gpointer data)` and
  `easel_canvas_set_zoom_gate (self, gate, data)`; `easel_canvas_wheel` zooms on Ctrl (with or without Shift) with any tool when
  nothing is held and the gate allows, lets the wheel scroll while something is held, and does nothing while the gate says no, each
  time dropping a partial notch; `easel_canvas_pinch` zooms the same way and, while something is held, moves its base to the scale
  now, so the count starts again from the release; `easel_canvas_zoom_at` does nothing while something is held
  (`easel_canvas_set_zoom` and `fit_to_window` do not check: the window's actions do, and the fit zoom's follow-up of a window resize
  may change the zoom during a held stretch, REQ-0015-05); `easel_canvas_buttons_released` also ends panning; `zoom_around` starts
  from the scroll a zoom still waiting for its allocation will set.
- window: `zoom_allowed (self)` = `view_actions_allowed` and not holding, used by the six zoom actions (`zoom-in`, `-out`,
  `-normal`, `-large`, `-fit`, `zoom-to`); `wheel_zoom_gate` (the picture exists, no dialog, no menu or popover, the font card's
  family list closed); `easel_window_card_wheel (self, over, state, notches, x, y)`, what the thumbnail's scroll controller and the
  font card's new capture-phase one (`on_card_scroll`) call, with `scroll_point` reading the event's place.

**Readers of what changes.** The Pan-only rule: 0001's AS-13 and AS-15 cases, 0004's pinch case (AS-7), 0017's card wheel cases,
0021's thumbnail wheel and pinch cases; REQ-0004-03's rule: 0004's and 0008's cases of zoom during a stroke; each changed with an
old/new comment. `easel_canvas_buttons_released`'s callers (the canvas's event handler, the window's).

**Scenario to test.** AS-1, AS-2, AS-3, AS-5: ui `/0036`, through `easel_canvas_wheel` and `easel_canvas_pinch` (the functions
the controllers call) and the window's actions; AS-5 calls the wheel twice before `wait_for_layout`. M1 a real wheel and
touchpad; M2 a touch screen (AS-4).

**Constitution.** No zoom value added, no picture change (P7); no core change.

**Risks.**
- R1 A modal popover takes the pointer, so the canvas may never see the wheel while a menu is open; the gate makes the rule hold
  either way. T004 checks the gate through `easel_canvas_wheel`.
- R2 The font card is the scroller's sibling in the overlay: its events never reach the canvas's controller; forwarded with the
  pointer turned into canvas coordinates, as `on_thumbnail_scroll` does (T002's case reads the pixel under the pointer after the zoom).
- R3 The fast-zoom fix changes `zoom_around`, which every zoom uses: 0004's, 0007's (the Magnifier's own `anchor_focus` path is
  left out of the fix) and 0008's cases rerun in T005.

## Tasks
- [x] T001 Ctrl+wheel and the pinch with every tool (REQ-0036-01; AS-1 without the submenu clause) - done when ui `/0036` covers Pencil, Select with a floating selection, an open box and a waiting curve (the pixel under the pointer kept, the box open, the curve waiting), Ctrl+Shift+wheel, the ends at 1600 % and 10 %, the margin and a handle, a touchpad's 0.6 + 0.6 of a notch, a pinch with Pencil, red first; 0001's, 0004's and 0021's cases that expected Pan only changed with old/new comments
  Result: ui `/0036/as1_every_tool` red first (`wz_wheel (...) == GDK_EVENT_STOP` false with Pencil), then green: Pencil, Select with an inverted (floating) selection, an open box and a waiting curve, each 100 -> 200 -> 100 % with the picture point under (140.5, 90.5) kept within a pixel and the picture unchanged, the selection floating, the box open, the curve waiting; Ctrl+Shift+wheel; 1600 % up and 10 % down change nothing; 0.6 + 0.6 of a touchpad notch is one level; a pinch at 1.5 with Pencil, around its center; the margin and the corner resize handle. The Pan checks of `easel_canvas_wheel`, `easel_canvas_zoom_at` and `easel_canvas_pinch` replaced by `easel_canvas_is_holding`, here still only the strokes and drags `easel_canvas_is_stroking` counts (0010's case of Ctrl+wheel during a line drag, which would have zoomed, kept it red until then; T003 adds the rest). Old cases changed with old/new comments: 0001's zoom-at case (a notch with Pencil now 10 -> 25 %), 0004's pinch case (Pencil now one level an event, 100 -> 200 %). ui 0001, 0004, 0007, 0008, 0010 to 0013, 0017, 0021, 0022 green.
- [x] T002 The cards (REQ-0036-01; AS-2) - done when ui `/0036` covers Ctrl+wheel over the thumbnail card (the canvas point under the pointer kept) and over the font card and its size field with and without its focus (zooms; the field's text and focus kept), and the plain wheel over the focused field (steps the size), red first; 0017's card wheel cases green or changed with old/new
  Result: ui `/0036/as2_cards` red by build (`implicit declaration of function 'easel_window_card_wheel'`), then green: Ctrl+wheel over the thumbnail card at 200 % gives 300 % with the canvas point under the card's (20, 20) kept; over the font card and over its size field without the focus, then with it, it zooms, the field keeping its text and the focus; without Ctrl it is not a zoom. `easel_window_card_wheel` is what the thumbnail's and the font card's scroll controllers call (the font card's, new, in the capture phase over the whole card, so Ctrl+wheel never reaches the size field's own wheel). Seam: the test calls that function, so the font card's controller being installed is not observed (a "scroll" emitted on a controller carries no Ctrl state): the real wheel over the card is T006's M1. ui 0017 and 0021 green.
- [x] T003 Nothing zooms while something is held (REQ-0036-02; AS-3) - done when ui `/0036` covers a stroke, a line drag, a selection move, a text box drawn, Pan's drag, a resize drag and a held Space press, each with Ctrl+wheel (no zoom, the view scrolls), Ctrl+=, Fit to Window and the Zoom submenu (no zoom); the pinch during a stroke and its count from the release; a lost release of a stroke and of Pan's drag, then a zoom; Flip Horizontal during Pan's drag as before, red first; 0004's and 0008's cases green or changed with old/new; the held predicate and the lost-release ends get mutants
  Result: ui `/0036/as3_held` red first at Pan's drag (`easel_canvas_is_holding` FALSE during it), then green: a stroke, a line drag, a selection move, a text box drawn, Pan's drag, a resize drag, a held Space press and a Fill press, each with Ctrl+wheel not handled, Zoom In, Fit to Window, the Zoom submenu's 400 % and a pinch at 1.6 leaving 100 %, and after each release Ctrl+wheel zooming; the pinch at 1.3, then 1.6 during a stroke (no zoom), then 2.3 (under 1.5 times 1.6, none) and 2.4 after the release (one level); a lost release of a stroke and of Pan's drag (`easel_canvas_buttons_released`) ending them, then a zoom; Flip Horizontal during Pan's drag doing what it did before (nothing until the release, the Image actions' own panning check). `easel_canvas_is_holding`: strokes and drags, panning, resizing, a cancelled button, a Fill, Pick Color or Magnifier press (`action_button`, found missing by a survivor) and a keyboard hold. `easel_canvas_set_zoom` and `fit_to_window` do not check it (0015's case changes the zoom during a held stretch, as the fit zoom's follow-up of a window resize does): the window's six zoom actions do, through `zoom_allowed`. Mutants: `10 mutants: 10 killed` (REQ-0036-02 with 0008-04 and 0010-21, two of them refreshed for the new lines); one dropped as equivalent: a keyboard hold left out of the predicate (a keyboard hold is always a stroke: `easel_canvas_is_key_held` clears itself otherwise). ui 0001, 0004, 0007, 0008, 0010 to 0015, 0017, 0021, 0022 green.
- [x] T004 No zoom while a dialog, menu or popover is open (REQ-0036-01's last clause; AS-1's submenu clause) - done when ui `/0036` covers Ctrl+wheel and the pinch with the Zoom submenu open, a dialog open and the font card's family popover open (no zoom), through `easel_canvas_wheel` and `easel_canvas_pinch`, red first
  Result: ui `/0036/as1_open` red first (`easel_window_get_zoom (win) == 100`: 300 with the View menu open, the direct calls zooming), then green: with the View menu open, the Attributes dialog open and the font card's family popover open, Ctrl+wheel and a pinch at 1.6 leave 100 %; closed again, Ctrl+wheel zooms. The canvas asks the window's gate (`easel_canvas_set_zoom_gate`, `wheel_zoom_gate`: the picture exists, no dialog, no menu or popover, the family list closed) before the wheel or the pinch zooms. Mutants: the dialog left out and the family list left out, each killed; the gate ignored in the wheel came back INCONCLUSIVE three times (ui-0036 exited 77 in 0.02 s, the tool's session giving no display, only for this mutant, with 8 workers and with 1); applied by hand it is killed (`as1_open` fails at the View menu, 200 against 100). ui 0001, 0004, 0013, 0016, 0017, 0021 green.
- [x] T005 Fast zooms (REQ-0036-04; AS-5) - done when ui `/0036` covers two notches up and three down at once on an 8000 × 8000 picture (the point under the pointer kept) and Zoom In twice at once with the pointer elsewhere (the center kept), red first; 0004's, 0007's and 0008's zoom cases green; a mutant on the fix
  Result: ui `/0036/as5_fast` red first (two notches before a redraw: the point under the pointer moved from 4000 to 2000), then green with `zoom_around` starting from the scroll a zoom still waiting for its allocation will set: on an 8000 x 8000 picture in a 1390 x 853 area, two notches up, and two up then three down, end at the same picture point as one notch at a time (4000.333, 4000.5 at 300 %; 4000, 3999 at 50 %, exactly the same both ways); Zoom In twice keeps the area's center (4000.5, 4000.5). At 50 % the view moves in whole device pixels of two picture pixels, so AS-5's "within a quarter of a picture pixel" could not hold; JOD changed it after the gate (2026-11-09; old: "the same point under the pointer (within a quarter of a picture pixel)", new: "the picture point that was under the pointer is still under it, within one device pixel (at 50 % that is two picture pixels)"), and the case now asserts that in device pixels (P7.2): 1.00 apart after two notches up, 0.75 after two up and three down, the center after Zoom In twice within one; the fix's mutant killed again against it. Mutant: `1 mutants: 1 killed`. ui 0001, 0004, 0007 to 0009, 0015, 0025, 0031 green.
- [x] T006 Manual M1 and M2 (REQ-0036-01, REQ-0036-03; AS-1, AS-4) - done when JOD has tried a real wheel (Ctrl+wheel with Pencil, Select, Text and a waiting curve; during a stroke; a fast spin), a real touchpad (Ctrl+scroll and the pinch with Pencil), and a touch screen if one is at hand (else recorded as not run), and the Result says what he saw
  Result: JOD, 2026-11-09, in his build with a real mouse and touchpad: Ctrl+wheel with Pencil, Select, Text (over the font card and its size field too) and a waiting curve, the scroll instead during a stroke, a fast spin, the touchpad's Ctrl+scroll and pinch: "all works". This also covers the font card's own controller that T002's seam could not see. The touch screen (M2) was not reported separately: recorded as not run.
- [x] T007 Gate - done when the full suite and the scale-2 runs pass through `tools/isolated.sh`, `tools/mutate.sh --core` has run once (no core change: for the record), the 0036 mutants by REQ have run, and the Plan's names match the code
  Result (2026-11-09): through `tools/isolated.sh`, the full suite 128 of 128 and the scale-2 runs 5 of 5. Mutants by REQ (0036-01, -02, -04 with 0001-16, -17, 0004-08, 0008-04, 0010-21): `16 mutants: 15 killed, 0 killed by crash (test gap), 1 inconclusive (no snapshot or skipped), 0 survived, 0 errors in 111 s`, the inconclusive one the gate ignored in the wheel, as at T004 (ui-0036 exits 77 in the tool's session, only for this mutant; killed by hand). `--core` (no core change, for the record): `473 mutants: 468 killed, 0 killed by crash (test gap), 0 inconclusive (no snapshot or skipped), 4 survived, 0 errors, 1 accepted in 175 s`, the four 0023's accepted at its check. Lint 1329, 0 stale. After the gate, AS-5's wording (JOD) and its test in device pixels, ui-0036 green and REQ-0036-04's mutant killed again. The Plan's names as built.

## Check
- Date and commit: 2026-11-11, on `f4478f6` with the beta gate's uncommitted fixes (0036 itself is in `5c1cb53`, a restore point
  shared with Remove Background, so its diff was read from the code that names it).
- Tests: the full suite (isolated) 130 of 130 on 2026-11-10 before the check's fixes; after them ui-0007, ui-0017, ui-0021, ui-0035,
  ui-0036, ui-0037, ui-0038, core and io green; the cases of this feature: `/0036/as1_every_tool`, `as1_open`, `as2_cards`, `as3_held`,
  `as3_lost_release_box_move` and `as3_partial_dropped_at_press` (new), `as5_fast`. Under the sanitizers (ASan, UBSan) all 83 ui
  suites pass with no report.
- Mutants: the full list (2026-11-10, the beta gate) left REQ-0036-01's one inconclusive; run again alone with REQ-0036-02 and -04:
  all killed (77 mutants with the gate's new survivors: 71 killed, 5 survived, 1 accepted; 0036's none).
- Warnings: 0. Lint and `--names 0036` clean.
- Scenarios: AS-1 `as1_every_tool`, `as1_open` + M1; AS-2 `as2_cards` + M1 (the font card's own controller); AS-3 `as3_held`,
  `as3_lost_release_box_move`, `as3_partial_dropped_at_press`; AS-4 M2, not run (no touch screen); AS-5 `as5_fast`.
- Task results: T001 to T007 have their `Result:` lines; none is a characterization; T006's touch screen (M2) not run.

Findings (a review agent read the code blind; each checked in the code before it counted):
- F1 (user-visible, fixed): a lost release during a text box's move or resize left the canvas held: Ctrl+Wheel scrolled instead of
  zooming, the zoom commands were refused until the next click, and the pointer moving with no button down still moved the box.
  `easel_canvas_buttons_released` now ends it as released. `as3_lost_release_box_move` failed first ("is_holding ... should be FALSE").
- F2 (user-visible, fixed): a touchpad's partial notch was not dropped when something came to be held (0.6, a click, 0.6 zoomed), and a
  pinch counted from its old base when no scale came during the hold. Both are reset at every press, the one place every hold begins.
  `as3_partial_dropped_at_press` failed first (200 for 100).
- F3 (user-visible, fixed, manual check): Ctrl with a sideways scroll over the thumbnail card was swallowed (no sideways scroll), and
  over the font card too. Now only the vertical part zooms. No headless test can give a scroll signal the Ctrl state: step 7 of
  `docs/walkthrough-phase2.md`.
- F4 (not a defect): a fast pinch moves one level per event: REQ-0004-08 and its core test say so.
- F5 (open, low): a Magnifier zoom waiting for its allocation, then Ctrl+Wheel in the same frame, can anchor around a stale point.
- F6 (open, needs M2): one finger on a touch screen may pan through the scrolled window's kinetic scrolling (REQ-0036-03 says no
  pan); no touch screen at hand.
- From the gate's mutants (seen here): REQ-0037-04's survivor showed a user-visible gap of 0037 with 0038's Ctrl+Z: a second press on
  a pressed piece erased the first, so Ctrl+Z skipped both. JOD (2026-11-11): each press is kept and two cancel; REQ-0037-04 changed,
  0037's AS-12 added, `/0037/as12_second_press_undone` red first, a new mutant killed.
- Known gaps (also in `docs/phase2-plan.md`): F5, F6.
