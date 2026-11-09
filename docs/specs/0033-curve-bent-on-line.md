# 0033 Curve bent on the line

Status: implemented
Reviewed: spec-reviewer, 2026-11-09
Depends on: 0010, 0011, 0031

## Why
XP's Curve bends the line by a click anywhere, and its second click puts the curve down: the bend lands away from where the user
pointed, and there is no second try. JOD wants to take the line where it should bend and drag that point, as drawing programs do
(`docs/phase2-plan.md` E). Decided by JOD on 2026-11-07: this replaces XP's way, with no option (a named exception to
"XP-faithful by default", `docs/product.md`); on 2026-11-09: each bent point stays where it is dropped, a press away from the
curve finishes it, further presses on the curve keep reshaping it, Esc drops it.

## Requirements
Throughout: picture pixels are taken at their centers; S and E are the curve's start and end (the line's two ends, 0011); a
point of the curve is B (t) for t from 0 (S) to 1 (E): the cubic Bézier curve of REQ-0011-05 with the control points C1 and C2,
and, before any bend, the straight line B (t) = S + t (E − S); N is the number of REQ-0011-05 for the curve as shown (before any
bend, the distance from S to E rounded up), and M is the smallest power of two that is at least N and at least 64, at most 4096 (JOD, 2026-11-09, at the Plan); round (v) is the whole number nearest v, a half rounded
toward larger x or y. Every value here except N (REQ-0011-05's own) is computed exactly (as fractions), never approximated (JOD, 2026-11-09, at the check), so the same input gives the same
pixels (P7.4). Everything of 0011 about Curve holds unless a requirement here says otherwise: the line (its drag, Shift), the
pixels of REQ-0011-05, pending and its preview (REQ-0011-09), the colors and the one button (REQ-0011-10), the drops
(REQ-0011-11), the put-downs (REQ-0011-12), a held line or grab (REQ-0011-13), the step (REQ-0011-14) and the failures
(REQ-0011-15). This replaces, in 0011: REQ-0011-04's two bend stages and "the release of the second bend puts the curve down";
REQ-0011-13's "the next stage" for Curve (a press acts as REQ-0033-01 says); REQ-0011-03's hint for Curve; AS-2 and AS-3's clicks
and AS-4 and AS-9's "bends it twice"; the edge case "after one bend: the curve with both control points there"; and "XP's bends"
in 0011's Open questions. 0011 has pointer notes on REQ-0011-03 and -04. The exception to P1 is ADR-0005.

### On the curve or off it
- REQ-0033-01 (Changes REQ-0011-04.) While a curve waits (its line drawn, no button held), a press of its button in the canvas area shall be on the curve when the straight-line distance on screen, in logical pixels, from the pointer to the nearest point of the square of a pixel the curve draws inside the picture (at the width chosen; 0 inside the square) is at most 4, at any zoom, and off the curve otherwise. Pixels of the curve outside the picture, which are not shown, do not count. A press on a resize handle is the handle's (REQ-0011-12 puts the curve down first), whatever the curve under it.
- REQ-0033-02 A press of its button on the curve shall grab it: of the M + 1 points B (i / M), i from 0 to M, the one nearest the center of the pixel under the press (the smallest i on a tie) gives the grabbed place t = i / M, limited to the range 1/8 to 7/8, and the grabbed point is B (t), exactly. REQ-0033-04 says which bend the grab makes or moves. Until the pointer leaves the press's pixel the curve does not change; then, while the button is held, the moving point (the grabbed point, or the point of the bend REQ-0033-04 names) moves by the pointer's movement in picture pixels since the press (the pixel under the pointer less the pixel under the press), and the curve follows at once, shown as it will be put down; the release fixes it and the curve waits again. A press and release on the press's pixel leaves the curve as it was, waiting. Shift has no effect on a grab. (Changed 2026-11-09 at M1, see REQ-0033-05: with one bend the curve's peak is its point, so the grabbed point is under the pointer as the peak.)
- REQ-0033-03 A press of its button off the curve shall put the curve down as shown (one step, REQ-0011-14), at the press. If the pointer has not left the press's pixel by the release, nothing more happens; once it has, that press is the start of a new curve's line (REQ-0011-04's line, in the press's button's colors), which ends at the release wherever the pointer is (a one-pixel line if it is back on the press's pixel) and then waits. Esc, Undo or the other button while that press is held, before or after the pointer leaves its pixel, cancel only the new line (REQ-0010-08: no line starts, the release does nothing more): the curve put down stays, and that Undo does not undo it. If the put-down needs memory that cannot be had, the dialog of REQ-0011-15 appears at once (as for Gradient, REQ-0012-08; JOD, 2026-11-09, at the Plan), the curve is dropped and no new line starts. Further presses of the same multi-click (the second press of a double-click) after a press off the curve are ignored, with their releases, as REQ-0011-07 does for Polygon.

### Bends
- REQ-0033-04 A bend is a point P. Its place is the fraction of the way from S to E at which P lies when measured along the line (the dot product of P − S and E − S over the square of the distance from S to E; 1/2 when S = E), rounded to the nearest 1/4096 and limited to 1/8 to 7/8; with two bends whose places are less than 1/8 apart, the two places are moved apart to 1/8, evenly round their middle (the middle limited to 3/16 to 13/16). A curve shall have at most two bends, kept in the order of their places. With no bend, a grab makes the first bend, its point the grabbed point as moved. With one bend, a grab whose t is less than 1/8 from the bend's place concerns that bend: its point moves by the pointer's movement; any other grab makes the second bend. With two bends, the grab concerns the bend whose place is nearer the grab's t (the first on a tie): less than 1/8 from it, that bend's point moves by the pointer's movement; otherwise that bend is replaced by the grabbed point as moved.
- REQ-0033-05 The control points shall be: with one bend P, those that make P the curve's peak, the point of the curve farthest from the line S E, at t = 1/2: with d = E − S, n = d turned a quarter turn (−dy, dx), and f and g the numbers with P = S + f d + g n, C1 = S + u d + (4g / 3) n and C2 = S + v d + (4g / 3) n, where u = 0 and v = (8f − 1) / 3 when f <= 1/2, and u = (8f − 4) / 3 and v = 1 otherwise (so both control points lie at the same distance from the line and B (1/2) = P); when S = E, C1 = C2 = (4P − S) / 3; with two bends P1 and P2 at the places t1 and t2 (REQ-0033-04), the points C1 and C2 with B (t1) = P1 and B (t2) = P2; each coordinate then rounded by round () and limited to −1,000,000 to 1,000,000. The curve's pixels are those of REQ-0011-05 for S, these control points, E and the width, and the same S, E, bends and width always give the same pixels. (Note: with the control points rounded, the curve passes through or next to the pixel of each bend in the cases tried, one step in x, y or both; this is not a guarantee, and not where a control point was limited.) (Changed 2026-11-09 at M1, JOD: "the mouse pointer is not in the center for the curve angle"; before, one bend had C1 = C2 with B (t) = P at the grabbed place, so a bend grabbed away from the middle had its peak elsewhere, and a bend kept its grabbed place.)
- REQ-0033-06 Enter (keypad Enter too) shall put a waiting curve down as shown, with no bend, one or two (REQ-0011-12); Esc, Undo and a press of the other button drop it whole (REQ-0011-11), and the middle button does nothing (REQ-0011-11). No new key or control.

### Unchanged and changed in 0011
- REQ-0033-07 The hint of REQ-0011-03 shall become, for Curve, "Draws a curved line: drag a line, then drag points of it to bend it. A click away from it finishes it." The cursor stays the crosshair (REQ-0010-15), also over the curve.
- REQ-0033-08 (P7) The curve writes whole pixels in the exact drawing color (REQ-0011-14, P7.1); the curve shown while it waits or while a grab is held is exactly the pixels the put-down writes, made by the same rule, at every zoom, at scale 1 and 2, below 100 % by 0031's rule (P7.2, P7.3); its pixels depend only on S, E, the bends and the width, from whole picture pixels and the places i / M (P7.4, P7.6).

## Scenarios
Colors: the picture white #FFFFFFFF, the foreground black #000000FF, Curve with 1 pixel, at 100 %, presses at pixel centers,
unless a scenario says otherwise. The numbers are computed with a script from REQ-0033-02 to -05 and the app's REQ-0011-05 pixels
(the first set checked by the review with a second script; recomputed on 2026-11-09 after M1's change of REQ-0033-05).
- AS-1 (REQ-0033-02, REQ-0033-04, REQ-0033-05, REQ-0033-08) Given a 300 × 200 picture, When the user drags the line from (10, 40) to (50, 40), presses at (30, 40) and drags to (30, 20), Then while the button is held the curve is shown with its peak at (30, 20), the picture unchanged; When the user releases, Then the curve waits; When the user clicks at (150, 150), Then the curve is put down: 59 black pixels, from (10, 40) to (50, 40), the smallest y 20 with the pixels (24, 20) to (35, 20), as one undo step, and nothing else starts; pressing at (20, 40) instead and dragging to (20, 20) gives the peak at (20, 20): 54 pixels, the smallest y 20 with (17, 20) to (24, 20); the window shown while it waited equals the window after | Verified by: automated (core, the pixels) + automated (ui, the stages, the preview, the click away)
- AS-2 (REQ-0033-04, REQ-0033-05) Given the line from (10, 40) to (70, 40), When the user presses at (25, 40) and drags to (25, 20) (the peak there: 72 pixels, the smallest y 20), then presses at (44, 25) (on the curve) and drags to (48, 48), and clicks away, Then the curve is an S of 82 pixels that has the pixels (25, 20) and (48, 48), the smallest y 20 and the largest y 56 | Verified by: automated (core) + automated (ui)
- AS-3 (REQ-0033-04) Given the curve of AS-2 waiting with its two bends, When the user presses at (25, 20) and drags to (25, 15), Then the first bend's point moves by (0, −5): the curve has the pixels (25, 15) and (48, 48), 94 pixels after a click away; Given the curve of AS-2 waiting instead, When the user presses at (40, 37) (on the curve, its place a little nearer the second bend's) and drags to (40, 30), Then the second bend is replaced: the curve has the pixels (40, 30) and (25, 20), 69 pixels after a click away | Verified by: automated (core) + automated (ui)
- AS-4 (REQ-0033-02) Given the line from (10, 40) to (90, 40), When the user presses at (11, 40) (near the start) and drags to (11, 30), Then the grabbed place is t = 1/8, its point (20, 40) moves to (20, 30), the peak: the curve has the pixel (20, 30) and the smallest y 30 (87 pixels after a click away); pressing at (89, 40) and dragging to (89, 30) instead grabs t = 7/8 and gives the peak (80, 30), 87 pixels; a press on the curve and a release on the same pixel change nothing, on this line and on the curve of AS-2 | Verified by: automated (core) + automated (ui)
- AS-5 (REQ-0033-04) Given a curve with one bend at the place t1, When a grab's place is exactly 1/8 from t1, Then it makes the second bend; When it is one step of the grid (1 / M) nearer, Then it moves the first bend's point; Given two bends, When a grab's place is as far from both, Then it concerns the first; two bends whose points lie less than 1/8 apart along the line get places 1/8 apart round their middle | Verified by: automated (core)
- AS-6 (REQ-0033-01, REQ-0033-03) Given the line from (10, 40) to (50, 40) waiting, When the user presses at (30, 43), Then the press grabs the curve; at (30, 46), Then it is off the curve; at 400 % a press exactly 4 logical pixels from the line's pixels grabs it and one 5 away does not, and the same at 25 % and at scale 2; with 3 pixels the distance counts from the wider line; a curve whose pixels near the press lie outside the picture is off there; When the user clicks off the curve, Then it is put down as one step and nothing else starts, also with a double-click (its second press is ignored); When the user instead presses off the curve at (100, 100) and drags to (140, 120), Then the first curve is put down and a new line from (100, 100) to (140, 120) is shown, waiting after the release; Esc then drops the new line only, the first curve staying | Verified by: automated (ui)
- AS-7 (REQ-0033-06, REQ-0011-11, REQ-0011-12) Given a curve waiting with one bend, When Enter is pressed, Then it is put down as shown; Given it waiting with two bends, When Esc is pressed, Then it is dropped and the picture is unchanged; the same with Undo and with a press of the other button; a middle press keeps it waiting; choosing the Pencil puts it down | Verified by: automated (ui)
- AS-8 (REQ-0011-10, REQ-0033-02) Given a line drawn with the secondary button (the background red #FF0000FF), When the user grabs and bends it with the secondary button, Then it is red when put down; When the user presses the primary button on the curve instead, Then the curve is dropped (the other button, REQ-0011-11) | Verified by: automated (ui)
- AS-9 (REQ-0011-13, REQ-0033-02, REQ-0033-03) Given a grab held, When the release never arrives and the next press comes with no button down, Then the bend ends as last shown and that press acts as a press of its own (on or off the curve); When the window loses focus instead, Then the bend ends as last shown and the curve waits; When the window is resized while the grab is held, Then the bend still follows the pointer's movement in picture pixels; Enter while the grab is held does nothing; Given memory made too small, When a press off the curve puts it down, Then at once the dialog "Could Not Draw the Shape" appears, the picture is unchanged and no new line starts | Verified by: automated (ui)
- AS-10 (REQ-0033-07) Given the app started, When the user chooses Curve, Then the hint is "Draws a curved line: drag a line, then drag points of it to bend it. A click away from it finishes it." and the cursor over the curve is the crosshair | Verified by: automated (ui) + manual M1 (JOD bends curves with the mouse)

## Edge cases
- A line of one pixel (a click as the line, 0011's loop): every press within 4 logical pixels of it grabs it; the grabbed place is 1/8 to 7/8; the bends make a loop from that pixel.
- A curve wholly outside the picture: no pixel of it counts for a press, so every press is off it and puts it down (no step, REQ-0011-14).
- A grab dragged far away (outside the window): the control points are limited to ±1,000,000 (REQ-0033-05); a curve too large for memory follows REQ-0011-15 (the whole curve dropped, every bend with it); a preview of such a curve may take longer than a frame (0011's shape budgets hold for curves within the window).
- A press where the curve crosses itself: the grabbed place is the nearest point's, the smallest i on a tie.
- Two bends: a grab between them, as near one as the other: it concerns the first (REQ-0033-04).
- A resize handle under the curve: the handle wins (REQ-0033-01).
- The width or the colors changed while the curve waits: as REQ-0011-12 and REQ-0011-10 (the width applies at once; the colors stay the first press's).

## Out of scope
- XP's click-anywhere bends (replaced, no option, ADR-0005); more than two bends; moving the ends of the line after it is drawn; a cursor or a marker over the curve or its bend points; smoothing (Smooth shapes and brushes).

## Open questions
None. Settled by JOD on 2026-11-07 (replaces XP's way, no option) and 2026-11-09 (each bent point stays; a press off the curve finishes it and a drag from it starts a new line; further presses on the curve reshape it; Esc drops it, Enter puts it down). At M1 (JOD, 2026-11-09): with one bend the curve's peak is at the pointer (REQ-0033-05); with two, each bend's place is where its point lies along the line (decided then, routine: it keeps an S clean where the grabbed places made it loop). Decided in the draft and the review (routine): the hit distance of 4 logical pixels, as Polygon's close (REQ-0011-07), counted on the pixels shown inside the picture; the grabbed place on a grid of M steps (the curve's length, at least 64) limited to 1/8 to 7/8, so a grab near an end does not throw the curve; bends at least 1/8 apart; nothing changes until the pointer leaves the press's pixel; a double-click away puts the curve down once; the control points rounded to whole pixels (P7.6); the crosshair kept; ADR-0005 for the exception to P1.

## Plan
**Approach.** The bend rules are pure core code: a new `src/core/curve-bend.{c,h}` computes the grid, the grab, the bend rule and
the control points in whole numbers (every place is a multiple of 1/4096 and every point a multiple of 1/2^36, so the solve fits
in 128 bits and is exact, P7.4). The curve's pixels stay the existing REQ-0011-05 code (`easel_figure_*` with integer control
points): preview and put-down are unchanged and shared (P7.3). The canvas keeps the curve's line and its bends, turns a press on a
waiting curve into a grab or a put-down by REQ-0033-01's hit test, and recomputes the four points of `pend_pts` from the bends at
each pointer move. The put-down stays `put_down_pending`, the one function every path goes through (Enter, a tool change, Save,
the off-curve press); the drop stays `easel_canvas_drop` (Esc, Undo, the other button).

**Files.** `src/core/curve-bend.{c,h}` (new), `src/core/meson.build`; `src/ui/canvas.c` (the curve's state, the press, the follow,
the release); `src/ui/window.c` (the hint); `po/*.po`, `po/easel.pot` (the hint); `tests/core/test-curve-bend.c` (new),
`tests/meson.build`; `tests/ui/test-window.c` (0011's curve cases and the new `/0033` ones); `tests/mutants.txt`.

**Interfaces.** (As built, 2026-11-09.)
- core `src/core/curve-bend.h`: `EASEL_CURVE_STEPS` (4096) and `EASEL_CURVE_ONE` (2^36); `EaselCurveBend { int64_t x, y; }` (a
  bend is its point); `easel_curve_grid`, `easel_curve_point`, `easel_curve_grab` (the grabbed place, 512 to 3584);
  `easel_curve_place`, `easel_curve_places` (two close places spread 1/8 apart), `easel_curve_order`; `easel_curve_bend_rule
  (sx, sy, ex, ey, b, n, t, &which)` (NEW, MOVE, REPLACE); `easel_curve_controls (sx, sy, ex, ey, b, n, out[4])` (one bend the peak,
  two at their places; exact in 128 bits, rounded, limited).
- ui (canvas): `curve_n`, `curve_bend[2]`, the grab (`grab_kind`, `grab_which`, `grab_px/py`, `grab_x/y`, `grab_t`, `grab_n`,
  `grab_base[2]`, `grab_left`), the press off the curve (`off_press`, `off_button`, `off_px/py`, `off_closed`); `curve_line_start`,
  `curve_hit` (the rows near the press from `easel_figure_row`, clipped to the picture, each run's square measured with
  `view_edge`), `curve_grab_start`, `curve_grab_follow`, `curve_set_controls`, `off_press_end`. `pend_step` is 0 for the line
  and 1 for a grab.

**Readers of what changes.** `pend_step` is read by `pend_follow`, `end_pending_stage` (the second bend's put-down goes), the
press (the next stage goes), and by tests through the stages. 0011's ui cases that click bends: `as2_curve_stages_and_preview`
(8535), the lost release and focus case (9287), the memory case (9400), `as9` colors (9126 path), the shape list of 0025's
figure test (26001: "curve, the first bend", "the second bend"), the hints table (8451), and every `start_pending (win, "curve")`
user; T004 runs every ui suite that names "curve" and updates them with old/new.

**Scenario to test.** (As built.) AS-1 to AS-5: core `test-curve-bend` (`grid`, `grab_place`, `bend_rule`, `controls`, `as1_one_bend`,
`as2_as3_two_bends`, `as4_near_the_ends`); AS-1 to AS-4 also ui `/0033/as1_one_bend` (the window rendered while held, waiting and
after), `as2_two_bends`, `as4_ends_and_still_click`. AS-6: ui `/0033/as6_on_or_off` (100 %, 400 %, 25 %) and
`/0033-scale2/as6_on_or_off_scale2`. AS-7: ui `as7_keys_and_buttons`. AS-8, AS-9: ui `as8_buttons_and_held`,
`as9_off_without_memory`. AS-10: the hints table of 0011's ui test + M1 (T005).

**Constitution.** P1's exception is ADR-0005. P7 by the shared figure code and the exact core; core libc only; no new key.

**Risks.**
- R1 128-bit range: the solve's numerators reach about 2^94 with the coordinates and the control points within ±2^20 (the
  limit) and places in 1/4096; a core test at the limits (T001) checks it, with `__int128` (GCC, as the gradient test uses).
- R2 Spec gap, settled by JOD at the Plan: M had no upper bound; exact math needs one. Now: "M is the smallest power of
  two that is at least N and at least 64, at most 4096" (a 4096 grid on an 8000-pixel curve still puts a point every 2 pixels).
- R3 Spec gap, settled by JOD at the Plan: the memory dialog of a put-down at a press comes at once (the window shows it at idle,
  as for Gradient, REQ-0012-08), not at the release.
- R4 The hit test below 100 %: measured with the view's edges, which only that path uses; T003's 25 % case renders the window and
  presses at a computed device pixel.
- R5 0025's pointer-to-figure test (26001) drives the curve's stages by clicks: it changes to grabs, with old/new (T004).
- R6 Cost: a grab computes M + 1 ≤ 4097 points once at the press; the preview is 0011's, so no budget changes and no benchmark task.

## Tasks
- [x] T001 The bend rules in the core (REQ-0033-02, -04, -05, -08; AS-1 to AS-5) - done when `tests/core/test-curve-bend.c` checks the grid, the grab (the nearest point, the tie, the 1/8 to 7/8 limit), the bend rule (new, move, replace, the 1/8 boundary and the tie), the control points (one and two bends, rounding, the ±1,000,000 limit, the 128-bit range at the limits, R1) and the scenarios' pixel counts through `easel_figure_apply`, red first, then green; the scenario numbers recomputed for M as a power of two and corrected in the spec if they moved
  Result: red by link (`undefined reference to 'easel_curve_grab'` and the four others), then 7 of 7 after one test fix of mine (3584 is exactly 1/8 from 3072, so the spec replaces, not moves). The scenario numbers are the same with M a power of two (47; 92; 110 and 70; 81 and 81). Hand breaks, each red: no 1/8 limit (`grab (far, 11, 40) == 512`), the 1/8 boundary moving (`2048 + 512 ... NEW`), truncation for floor (the exact round trip at the limits), the tie to the second bend, the tie to the largest i (the loop).
- [x] T002 Grabs in the canvas, no put-down after the second bend, the hint (REQ-0033-02, -04 to -08; AS-1 to AS-4, AS-7, AS-10) - done when ui `/0033` cases press, drag and release on the curve (the window while waiting equals the window after, at 100 % and 400 %), a press and release on one pixel changes nothing, Enter, Esc, Undo and the other button act as REQ-0033-06, the hint is the new one in all eleven languages, and 0011's and 0025's curve cases are updated with old/new; red first
  Result: ui `/0033/as1_one_bend` failed first (`count_color (img, 0xFF000000u) == 47`: 41, XP's bend), then the four `/0033` cases passed (AS-1, AS-2/-3, AS-4 with the still clicks, AS-7). 0011's and 0025's curve cases updated, each with its old text in a comment: `as2_curve_stages_and_preview` (two clicks and 41 pixels, now a grab, Enter and 47; the S-curve case dropped, AS-2 of 0033 covers it), `as9_curve_colors_of_the_first_press` (41 to 47), `as10_lost_release_focus_save` (a lost release of the second bend put the curve down; now the grab ends and the curve waits), `as12_nothing_memory_blocked` (the dialog at Enter, not at the second bend), 0025's `run_figure` and its pointer-shape stages (grabs, then Enter or another tool). The hint is new in the code, the hints table and the eleven .po files, the template regenerated. Full suite 120 of 121: `ui-0020` `as5_panel_switches` crashed once (`object_already_finalized`), passed alone twice, logged in the learning log. Shown at 100 % only: the zooms of the preview are 0011's path, unchanged.
- [x] T003 On or off the curve, and the press off it (REQ-0033-01, -03; AS-6, AS-9's memory case) - done when ui `/0033/as6_on_or_off` checks the hit distance at 100 %, exactly 4 at 400 %, 25 % (by rendering and a computed device pixel), width 3 and pixels outside the picture, and `/0033-scale2` at scale 2; a click away puts the curve down once (a double-click too), a drag away starts a new line that Esc drops alone, and a put-down without memory shows the dialog at once and starts nothing; red first
  Result: ui `/0033/as6_on_or_off` failed first (`curve_still_after_click (win, 30.5, 41 + 5 / l)` should be FALSE: every press grabbed), then `/0033` 7 of 7 and `/0033-scale2` 1 of 1 at scale 2; 0010, 0007, 0011, 0025 and 0028 green. Hand breaks, each red in `as6_on_or_off`: the reach 5.5 for 4 (the press 5 away grabbed), the multi-click not ignored (a dot left waiting), pixels outside the picture counted (the press at (320, 40) grabbed). `as9_off_without_memory` shows the dialog at once and starts nothing.
- [x] T004 The held grab's other events (REQ-0011-13, REQ-0033-02; AS-8, AS-9) - done when ui cases for the secondary button's color, the primary press dropping, a lost release, a lost focus, a window resize while held and Enter while held pass. Likely to pass through 0011's shared code: each is made red by a named break (the grab's follow reading the pointer's position instead of its movement; the lost release not ending the grab)
  Result: characterization: ui `/0033/as8_buttons_and_held` passed at its first run (0011's shared paths end a held grab as they ended a held stage). Made red by two breaks, each `'easel_canvas_is_stroking (canvas)' should be FALSE`: the lost release not ending the grab (`pend_held` taken out of `easel_canvas_buttons_released`), and the lost focus not ending it (`easel_canvas_reset_pointer` skipping a held grab). The window resized while held and Enter while held change nothing; the secondary button's curve is red, 47 pixels; the primary press drops it.
- [x] T005 JOD bends curves with the mouse (M1; AS-10) - done when JOD has drawn a line, bent it once and twice, reshaped it, clicked away, dragged away into a new line, and used Enter and Esc, at 100 % and another zoom, and the Result says what JOD saw
  Result (first look, 2026-11-09): JOD: "works but when i drag the line the mouse pointer is not in the center for the curve angle, should be in the mid of the shape"; asked, JOD chose the peak under the pointer: REQ-0033-02, -04, -05 changed and AS-1 to AS-5 recomputed, built in T007; a second look follows.
  Result (second look, 2026-11-09, after T007): JOD: "ok".
- [x] T007 The peak under the pointer (REQ-0033-04, -05 as changed at M1; AS-1 to AS-5) - done when the core test has the new rule (bends as points, places from their position along the line, the one-bend peak, the spread places) and the new numbers, red first, and the ui `/0033` cases and 0011's and 0025's curve cases pass with the new numbers
  Result: core red by build (`too many arguments to function 'easel_curve_bend_rule'`, `easel_curve_order` and `easel_curve_place` undeclared), then 7 of 7 after two slips of mine in the test (C2 of a far bend lies over E, x 10; AS-2's largest y is 56, misread as 58 from the probe's columns, fixed in the spec too). Hand break: the first bend's u taken as v (`u = 8a - d` when 2a <= d): red, `out[0] == 10 && out[1] == 13 && out[2] == 50`. ui `/0033`, 0011, 0025 and 0028 green with the new numbers (59, 54; 72, 82, 94, 69; 87); 0025's second-bend press moved to (110, 130), on the new curve.
- [x] T006 Gate - done when the full suite and the scale-2 runs pass through `tools/isolated.sh`, `tools/mutate.sh --core` has run once (any survivor reported), the mutants of REQ-0033-02 to -05 are in `tests/mutants.txt`, and the Plan's names match the code
  Result (2026-11-09): full suite through `tools/isolated.sh` 122 of 122, scale-2 runs 9 of 9 (`/0033-scale2` among them). Mutants by REQ (0033-01 to -05, 0011-03, -10, -13): `23 mutants: 22 killed, ... 1 survived`, the survivor mine: I had pointed 0011-13's edge mutant at the new off-curve branch, which no test drags outside the picture; pointed back at the held stage's motion, `3 mutants: 3 killed`. Three 0011-04 mutants of XP's bend stages removed (the behavior is gone), 16 new for 0033; lint 1296, 0 stale. `--core`: `459 mutants: 454 killed, 0 killed by crash (test gap), 0 inconclusive (no snapshot or skipped), 4 survived, 0 errors, 1 accepted in 169 s`, the four 0023's, accepted at its check. The Plan's names as built.

## Check
- Date and commit: 2026-11-09, `843562e` plus the uncommitted 0031 to 0033 work; no code changed after T006's runs (only
  `tests/mutants.txt`, re-linted).
- Tests: `tools/isolated.sh meson test` 122 of 122 (this feature: core `test-curve-bend` 7 cases, ui `/0033` 7 cases, 0011's and
  0025's curve cases updated); scale-2 runs 9 of 9 (`/0033-scale2/as6_on_or_off_scale2`); warnings 0.
- Mutants: by REQ (0033-01 to -05, 0011-03, -10, -13) 22 of 23 killed, the survivor a mutant I had moved to the wrong place,
  moved back: 3 of 3; `--core` `459 mutants: 454 killed, 0 killed by crash (test gap), 0 inconclusive (no snapshot or skipped),
  4 survived, 0 errors, 1 accepted in 169 s`, the four 0023's (accepted at its check). Lint 1296, 0 stale.
- Scenarios: AS-1 core `as1_one_bend` (break: the peak's u, T007) + ui `/0033/as1_one_bend` (red first: 41 for 47, T002) and the
  click away in `as6_on_or_off`; AS-2, AS-3 core `as2_as3_two_bends` + ui `as2_two_bends`; AS-4 core `as4_near_the_ends` (break:
  no 1/8 limit) + ui `as4_ends_and_still_click`; AS-5 core `bend_rule` (breaks: the 1/8 boundary, the ties, the spread mutant);
  AS-6 ui `as6_on_or_off` (breaks: reach 5.5, pixels outside counted, the multi-click), scale 2; AS-7 ui `as7_keys_and_buttons`;
  AS-8, AS-9 ui `as8_buttons_and_held`, C (T004: passed first; breaks: the lost release and the lost focus not ending the grab)
  and `as9_off_without_memory`; AS-10 the hints table + M1 (T005: "ok" after T007). REQ-0033-08 below 100 % and at scale 2:
  0025's `as4b_preview_equals_release` (the curve's stages, as grabs, at its zooms) and `as2_same_figure_at_any_zoom`.
- Task Results: T001 to T007 each have one.
- F1 (low) REQ-0033-03 names Esc, Undo and the other button while the press off the curve is held; only Undo (before the pointer
  moves) has a test, and Esc after the release. Esc and the other button reach the same `off_press_end` / `easel_canvas_drop`
  paths. Left open: Known gaps.
- F2 (low, wording, for JOD) "Every value here is computed exactly" includes N, which REQ-0011-05 computes from a floating
  length (the same in the curve's pixels, deterministic on IEEE doubles). Proposed: "Every value here except N (REQ-0011-05's
  own) is computed exactly". Applied with JOD's yes, 2026-11-09.
- F3 (info) If the hit test cannot get the memory to read the curve's rows, the press counts as off the curve (and the
  put-down then follows REQ-0011-15). Not in the spec; a memory edge no user meets. Known gaps.
- F4 (info) T004 is a characterization, proven by its two breaks.
- F5 (info) A resize handle wins over the curve by 0011's existing check before the hit test; no 0033 case presses a handle with
  a curve under it (0011's AS-8 covers the handle with a polygon).
- F6 (info) The one-off crash of `ui-0020` `as5_panel_switches` during T002's full run is logged in the learning log.
- Verdict: clean for a user; two low gaps and a wording proposal.

### Known gaps
- Tests for Esc and the other button while a press off the curve is held (F1); a handle pressed over a waiting curve (F5); the
  hit test without memory (F3).
