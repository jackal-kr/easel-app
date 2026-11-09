# 0032 Gradient types

Status: implemented
Reviewed: spec-reviewer, 2026-11-08
Depends on: 0012, 0030, 0031

## Why
The Gradient tool fades the drawing color to transparent along a straight drag only (0012). A glow, a spot of light or a band
of color needs a fade all round a point, or both ways from a line. This feature gives Gradient three types, Linear, Radial
and Reflected, as an option row (`docs/phase2-plan.md` C, JOD 2026-11-05; the types and their look decided by JOD,
2026-11-08). It comes after the one view path below 100 % (Feature 0031), so the new types cost the screen's size at every zoom.

## Requirements
Throughout, as in 0012: colors are 0xAARRGGBB with straight channels (ADR-0002); round (x / y), for whole numbers x >= 0 and
y > 0, is (2x + y) div 2y; a picture pixel p, the start S and the end E are taken at pixel centers; L is the square of the
distance from S to E in pixels; A is the drawing color's own opacity (its alpha, 0 to 255) and the drawing color is the one of REQ-0012-04 (the foreground for the primary button, the background for the secondary). Each type below defines the gradient's opacity at p, from 0 to A, in place of REQ-0012-04's; the gradient is laid over the picture at that opacity by REQ-0012-05, and a pixel of opacity 0 is left alone. E differs from S (a click makes no gradient, REQ-0012-06), so L > 0.

### The option row
- REQ-0032-01 (Changes REQ-0012-01: the options panel now shows the types for Gradient.) While Gradient is active, the options panel shall show one well (REQ-0030-01's look) with three cells side by side, Linear, Radial, Reflected, exactly one of them chosen, its tint REQ-0030-17's. Each cell draws its type in the theme's text color made opaque, on whole pixels, as REQ-0030-02 draws a footprint; the fade is drawn as a falling density of opaque pixels (all, then fewer in a regular pattern, then none), so only fully opaque and fully transparent pixels are drawn (REQ-0030-07): Linear a square going from full at its smaller x to empty at its larger x; Radial full at its center falling to empty at a circle touching the square's sides; Reflected full along its middle column falling to empty at both its smaller and its larger x. The sheet sets the drawings' size and their steps. At scale 2 each drawn pixel is two device pixels on a side, sharp (REQ-0030-07). The three drawings are shown to JOD on a sheet (light and dark, scale 1 and 2) and approved before the app uses them; the approved sheet is kept as `docs/reference/0032/types.png` and is the reference from then on.
- REQ-0032-02 The cells' tooltips and accessible names shall be "Linear", "Radial" and "Reflected". They are reached from the keyboard as the cells of every other well are (Tab to the well, the arrow keys between its cells); Space on a cell chooses it, and Enter too, except while a gradient waits or its button is held, when Enter acts as REQ-0012-11 says (applies it, or nothing while held). No new shortcut. Numpad + and Numpad − still do nothing with Gradient (REQ-0012-01).
- REQ-0032-03 The type shall be Linear at each start of the app and stay as last chosen for the rest of the run, as the panel's other options do. It is not written to the settings file. Choosing a type is not an undo step and does not mark the picture modified.

- REQ-0032-13 (JOD, 2026-11-08, at the Plan: adds a case to REQ-0030-08.) While Gradient is active, the status bar shall show the chosen type in the words of its tooltip ("Linear", "Radial", "Reflected") as the size part of REQ-0030-08, in its place and with its width rule; it follows a change of type at once.

### The types
- REQ-0032-04 Linear shall be the gradient of REQ-0012-04 (its opacity round (A (L − d) / L)), unchanged.
- REQ-0032-05 Radial shall give the pixel p the opacity a, the largest whole number from 0 to A with 4 A² D <= L (2A − 2a + 1)², where D is the square of the distance from S to p, and 0 when no such number exists. For r the distance from S to p and R the distance from S to E, this is floor (A (R − r) / R + 1/2), halves rounded up, for r <= R, and 0 for r > R: the color's own opacity at S, falling in a straight line with the distance to 0 on the circle through E, and 0 (the picture left alone) there and everywhere beyond it. The lines of equal opacity are circles round S: a disc.
- REQ-0032-06 Reflected shall give p the opacity round (A (L − e) / L), where e is the absolute value of the dot product of p − S and E − S, limited to L. The color's own opacity is on the line through S perpendicular to the drag; it falls in a straight line to 0 at the line through E perpendicular to the drag, and is 0 there and beyond; behind S the same fall is mirrored: the pixel 2S − p has p's opacity. A band.
- REQ-0032-07 Every type shall be applied by REQ-0012-05's rule: laid over the picture at its opacity, so gradients stack: one applied where an earlier one lies is laid over it, and the earlier one stays where the new one's opacity is 0; the same drag applied again darkens toward the color. Where 0012 names the opacity of REQ-0012-04 (REQ-0012-03, -05) it means the chosen type's (REQ-0032-04 to -06); the status bar hint of REQ-0012-02 ("Fades the drawing color to transparent along a drag. A click or Enter applies it, Esc cancels it.") is the same for every type. Everything else of 0012 holds for every type as written: the color, its button and its following the palette while the gradient waits or is dragged (REQ-0012-04), Shift (REQ-0012-03), the click (REQ-0012-06), waiting, applying and dropping (REQ-0012-07, -08, -11, -12), the markers and the guide line (REQ-0012-13), the failure dialog and the step (REQ-0012-09), and the press rules (REQ-0012-10).

### Changing the type
- REQ-0032-08 When the user chooses another type while a gradient waits, the waiting gradient shall take the new type at once, with the same ends, colors and button, and keep waiting; the shown pixels are the new type's, and Enter applies the new type. Choosing the type already chosen changes nothing. A type chosen while no gradient waits is the type of the next drag.
- REQ-0032-09 A gradient's drag, or a marker's move, that is held while the type changes (the press takes the keyboard to the canvas, REQ-0022-02; only Tab to the panel and Space on a cell can change it then) shall take the new type at once, as REQ-0032-08, and go on. A marker's move cancelled after the change (Esc, Undo, the other button) or ended on the other end's pixel puts the end back (REQ-0012-13) and keeps the new type: the type is the panel's, not part of the move.
- REQ-0032-10 If the memory to show a gradient of the new type cannot be had, then a waiting one shall stay waiting, unshown, with no dialog, as REQ-0012-09 says, and be shown again when the view allows it; a held drag or move goes on unshown, and its release follows REQ-0012-09. Below 100 % a frame without memory is drawn as REQ-0031-05 says.

### The pixel contract (P7)
- REQ-0032-11 Every type writes whole picture pixels with the exact colors of REQ-0012-05 at the type's opacity (P7.1). The shown gradient, at every zoom, at scale 1 and 2, below 100 % by 0031's rule, is exactly the pixels the put-down writes, made by the same rule (P7.2, P7.3). The pixels depend only on S, E, the color, the type and the picture (P7.4), and are the same on every machine, also for ends far outside the window. Along any straight line away from the place of the full opacity the opacity never rises and changes by at most 1 from one pixel to the next on a long drag (no grain).

### Cost
- REQ-0032-12 A Radial or a Reflected gradient, waiting, dragged or with an end moved, shall meet REQ-0031-04 below 100 % (a frame within ADR-0004's budget, 33 ms median, at most 8 bytes per device pixel shown, nothing of the picture's size) and 0012's frame budgets from 100 % up, as Linear does.

## Scenarios
Colors: gray g is #ggggggFF (in the tests 0xFFgggggg).
- AS-1 (REQ-0032-01, REQ-0032-02, REQ-0032-03, REQ-0032-13) Given the app just started, When the user clicks Gradient, Then the options panel shows one well of three cells with the tooltips "Linear", "Radial", "Reflected", Linear chosen, with the accent tint; When the user clicks Radial, Then Radial alone is chosen, no step is added and the title has no modified mark; When the user chooses Pencil and Gradient again, Then Radial is still chosen; When the app starts again, Then Linear is chosen and the settings file has no new key; the cells' accessible names are the tooltips; When the user moves the focus to the well with Tab, an arrow key to Reflected and presses Space, Then it is chosen; with no gradient waiting, Enter on Radial chooses it; with one waiting, Enter on a cell applies the gradient and does not change the type; Numpad + changes nothing; the status bar shows "Linear", then "Radial" after the click, and no size part with Pencil (REQ-0032-13) | Verified by: automated (ui) + manual M1 (the sheet of REQ-0032-01, approved by JOD before code uses it)
- AS-2 (REQ-0032-05, REQ-0032-07) Given a white 21 x 11 picture, Radial and black, When the user drags from (10, 5) to (15, 5) and presses Enter, Then row 5 from column 4 to 16 is gray 255, 255, 204, 153, 102, 51, 0, 51, 102, 153, 204, 255, 255; column 10 from row 0 to 10 is 255, 204, 153, 102, 51, 0, 51, 102, 153, 204, 255; (13, 8) is gray 216 and (13, 9) and (14, 8) white (left alone, past the circle); one undo step | Verified by: automated (core, the pixels) + automated (ui, the step)
- AS-3 (REQ-0032-06, REQ-0032-07) Given the picture of AS-2, Reflected and black, When the user drags from (10, 5) to (15, 5) and presses Enter, Then row 5 from column 4 to 16 is the row of AS-2, every row is the same, and column 7 is gray 153 in every row | Verified by: automated (core)
- AS-4 (REQ-0032-05, REQ-0032-07) Given the picture of AS-2, Radial and #00000080 (black, alpha 128), When the user drags from (10, 5) to (15, 5) and presses Enter, Then the opacities of the columns 10 to 15 of row 5 are 128, 102, 77, 51, 26 and 0, and the pixels gray 127, 153, 178, 204, 229 and 255; Given a white 4 x 4 picture, Radial and black, When the user drags from (1, 1) to (2, 1) and presses Enter, Then only (1, 1) changes, to black; When the drag is from (1, 1) to (2, 2) instead, Then (1, 1) is black, (0, 1), (2, 1), (1, 0) and (1, 2) are gray 180, and every other pixel stays white | Verified by: automated (core)
- AS-5 (REQ-0032-05, REQ-0032-06, REQ-0032-11) Given every S and E in an 11 x 11 picture with E different from S, and A in {0, 1, 127, 128, 254, 255}, and every A for three fixed pairs, Then the Radial opacity of every pixel equals floor (A (R − r) / R + 1/2) for r <= R and 0 beyond, computed by a reference that does not use the inequality of REQ-0032-05 (rational bounds on the square roots, exact on a tie), the Linear and Reflected opacities equal a reference that counts the halves, and the Reflected opacity at 2S − p, when inside the picture, equals that at p; Given a white 21 x 11 picture and black, When a Radial drag from (-5, 5) to (-3, 5) is applied, Then every opacity is 0, so no pixel changes and no step is added; When a Reflected drag from (10, 5) to (40, 5) is applied, Then the columns 0, 10 and 20 of row 5 are gray 85, 0 and 85; Given S at (10, 5) and E 100000 pixels away at (100010, 5), Then the opacities match the reference | Verified by: automated (core)
- AS-6 (REQ-0032-04, REQ-0032-08, REQ-0032-10) Given the Linear drag of AS-2's ends waiting in black, When the user clicks Radial, Then the shown pixels are AS-2's at once, the markers stay on (10, 5) and (15, 5), the picture is unchanged and nothing is applied; When Enter is pressed, Then the pixels are AS-2's, one step; Given it waiting again, When the user clicks Radial again, Then nothing changes; When Reflected and then Linear, Then the shown pixels follow each, Linear's being REQ-0012-04's for these ends, columns 0 to 10 of row 5 black and 11 to 14 gray 51, 102, 153 and 204 (REQ-0032-04); Given the view's memory refused, When the type changes, Then the picture shows without the gradient, its markers shown, no dialog; with memory again the next frame shows the new type | Verified by: automated (ui, rendered and applied)
- AS-7 (REQ-0032-11) Given a waiting gradient of each type over a white picture, at 100 %, 50 % and 200 %, and at scale 2, Then the window rendered while it waits equals the window rendered after Enter, except the markers and the guide line | Verified by: automated (ui, rendering; scale 2 in the scale-2 run)
- AS-8 (REQ-0032-07) Given each type, Then a drag with the secondary button is in the background color (red over white: the core's red gradient of that type), Shift constrains the end (Radial from (5, 5) toward (14, 8) with Shift ends at (14, 5), and after Enter with black over white the columns 5 to 15 of row 5 are gray 0, 28, 57, 85, 113, 142, 170, 198, 227, 255 and 255), a click with none waiting makes no gradient, a marker moved re-shapes the type around the new ends, Esc drops it, and a press outside the markers applies it and starts a new drag of the chosen type | Verified by: automated (ui)
- AS-9 (REQ-0032-09) Given a Radial drag held from (10, 5), When Tab moves the focus to the panel and Space chooses Linear, Then the shown gradient is Linear at once and the release leaves a Linear gradient waiting; Given a Radial gradient waiting and its end's marker held, When Tab and Space choose Linear and Esc is pressed, Then the end is back where it was and the gradient waits as Linear | Verified by: automated (ui)
- AS-10 (REQ-0032-12) Given an 8000 x 8000 picture at Fit to Window, When a Radial and a Reflected gradient are dragged over it, Then the median frame is within 33 ms (ADR-0004) and the memory held while dragging is at most 8 bytes per device pixel shown; at 100 % each type meets 0012's budget | Verified by: automated (perf, in the real session)

- AS-11 (REQ-0032-07, REQ-0032-11, REQ-0012-04, REQ-0012-13) Given the yellow start-up picture, black and each type, When the user presses at (8, 8) and drags to (20, 12), Then the window shows the type's black gradient while the button is held; When the user releases, Then the gradient waits with a mark on each end, (8, 8) and (20, 12), and the picture is unchanged; When the user clicks red on the palette, Then the shown gradient is red at once and the picture is unchanged; When Enter is pressed, Then the picture is the red gradient as shown; When the user drags from (24, 6) to (14, 10) and presses Enter, Then the second gradient is laid over the first: where both reach, the pixel is the second laid over the first, elsewhere each is as it was alone; When the user drags again and presses Esc, Then the picture is as before that drag; two Undos give back the yellow picture | Verified by: automated (core `as11_types_stack`, ui `as11_types_recolor_stack`)
- AS-12 (REQ-0032-11) Given a Radial drag from (700, 400) to (1900, 1300), a Linear one from (100, 300) to (1700, 300) and a Reflected one from (900, 300) to (1700, 300), Then along 64 rays from S (Radial), along rows (Linear) and outward both ways from S (Reflected) the opacity never rises, changes by at most 1 a pixel and, for Radial, reaches 0 | Verified by: automated (core)

## Edge cases
- A drag of zero length (a click): no gradient, as REQ-0012-06, for every type.
- A Radial drag of 1 pixel: only S is laid over, at the color's own opacity; every other pixel is left alone (S's neighbours lie on the circle). To a diagonal neighbour: S at the color's opacity, its four side neighbours at round (A (√2 − 1) / √2), 75 for A = 255 (gray 180 for black over white), the rest left alone (AS-4).
- A Radial drag whose circle reaches past the picture: the part of the disc over the picture is laid over it, the rest of the picture is left alone. A circle wholly outside the picture (S far outside): no pixel changes and no step is added.
- A Reflected drag whose two end lines both lie outside the picture: the whole picture is between them and gets the fade.
- A press outside the picture (REQ-0012-03): S may lie outside; Radial then shows only the part of the disc over the picture.
- A selection: none exists while Gradient is active (choosing a tool puts a selection down, REQ-0013-21), so a gradient never applies inside a selection only, as in 0012.
- The colors swapped while a gradient of any type waits: it takes its button's new color (REQ-0012-04).
- The same drag applied again and again: each lays over the last, so the pixels darken toward the color (REQ-0032-07); this is stacking, not a fault.
- The largest picture under a Radial gradient that covers it: as REQ-0012-09 and 0012's edge case of 10000 x 10000.

## Out of scope
- A gradient from one color into another that replaces the picture (built 2026-11-08 from a misreading of JOD's screencasts, withdrawn by JOD on 2026-11-09); more stops; Diamond, Angle or other types; dithering; smoothing.
- A Preferences row or settings key for the type.

## Open questions
Settled by JOD on 2026-11-08: to transparent only (a blend of the foreground into the background was built the same evening from his screencasts, and withdrawn by JOD on 2026-11-09: every type fades the drawing color to transparent and stacks over the picture, 0012 unchanged); Linear, Radial and Reflected; a waiting gradient takes a new type at once;
the cells draw their type, approved on a sheet before code. Decided in the review (routine): the fade in a cell is a density of opaque pixels (REQ-0030-07 holds); a cancelled marker move keeps a type chosen during it; the hint stays. At the Plan (JOD's go, 2026-11-08): the status bar shows the type (REQ-0032-13, reversing the Out of scope line); AS-5's reference is independent of the code's formula (R3); the correction step is core code and `--core` must leave no survivor of it.

## Plan
**Approach.** The type becomes a field of the core's gradient (zero is Linear, so every caller that builds one today keeps
its pixels), and `easel_gradient_opacity` picks the rule by it; `easel_gradient_lay`, the apply and every preview already go
through that one function, so P7.3 holds by construction. The canvas holds the chosen type and `shown_gradient` reads it at
each call, so a waiting gradient, a held drag and a marker's move take a new type at once (REQ-0032-08, -09) and a cancelled
move, which puts back only the end, keeps it. The options panel gains a ninth group, one well of three cells drawn as the
size presets are; the window passes the choice to the canvas as it passes the presets.

**Files.** `src/core/gradient.{c,h}`; `src/ui/tool-options.{c,h}` (the group, the cells' drawing, the getter); `src/ui/canvas.{c,h}`
(the type, its setter); `src/ui/window.c` (`sync_canvas_options`); `po/*.po` (three strings); `tests/core/test-gradient.c`;
`tests/ui/test-window.c` (`/0032` under the a11y test environment, `/0032-scale2`, the panel sheet's tool list); `tests/meson.build`; `tests/perf/perf-0012.c`
(the types); `tests/mutants.txt`.

**Interfaces.** (As built, 2026-11-09, after the withdrawal.)
- core: `EaselGradientType` (LINEAR, RADIAL, REFLECTED); `EaselGradient { x0, y0, x1, y1, color, type }` (0012's struct with the
  type added: zero is Linear); `unsigned easel_gradient_opacity (g, x, y)` picks the type's rule; `easel_gradient_blend` and
  `easel_gradient_lay` are 0012's, unchanged. Inside: a `Frame` (the clamped drag, L, the type and the color's alpha) made once
  per lay, `opacity_in` per pixel. Radial: a first opacity one below the floor of the double estimate, raised while the exact test
  `4 A² D <= L (2A − 2a + 1)²` holds, in 64 bits (both sides below 2.1 × 10¹⁸ with the coordinates limited to ±1,000,000).
- ui: `easel_tool_options_get_gradient_type`; `easel_canvas_set_gradient_type` (a redraw when a gradient waits); the cells emit
  `preset-changed`; `easel_tool_options_size_names` and `size_index` give the types for the status bar (REQ-0032-13).

**Readers of the state this extends.** `EaselGradient` is built in `shown_gradient` only (canvas) and in the core and perf tests;
its readers: the sampled view below 100 % (`append_view_by_edges`), `gradient_preview`, the put-down (`put_down_pending` and the
paths into it), the markers. REQ-0012-01's empty panel for Gradient is read by 0012 `as1_gradient_toolbox_hints` and by the
panel tests of 0006, 0008 and 0030 that list the groups; T003 updates them. The panel's width: three cells fit the default
width (REQ-0030-09); checked by rendering in T001.

**Scenario to test.** (As built.) AS-1: ui `as1_gradient_types_panel` + M1 (the sheet). AS-2, AS-3, AS-4, AS-5: core
`as2_radial`, `as3_reflected`, `as4_radial_alpha_and_small`, `as5_types_against_reference`, `gradient_matches_the_formula` (the
three types), plus ui `as2_types_user_path`. AS-6: ui `as6_type_change_waiting`. AS-7: ui `as2_types_user_path` (100 %, 50 %, 200 %) and
`/0032-scale2/as7_types_scale2`. AS-8: ui `as8_types_keep_0012_rules`. AS-9: ui `as9_type_change_held`. AS-10: `perf-0012` with the
types. AS-11: core `as11_types_stack`, ui `as11_types_recolor_stack`. AS-12: core `as12_no_grain`. M2: JOD with the mouse.

**Status bar.** The window sets the preset part from `easel_tool_options_get_gradient_type` with the three names as the width's set (`easel_status_bar_set_preset`), as for the sizes.

**Measurements (T005, 2026-11-09, of the withdrawn blend; measured again in T009).** `perf-0012` in the real session, no other Easel window, load 1.1, three runs interleaved with a build of HEAD (843562e) in a scratch worktree; each move to the painted frame, median of 30 moves, ms.

| gradient dragged | this build, runs 1/2/3 | HEAD (Linear fade), runs 1/2/3 |
|---|---|---|
| Linear, 1920 x 1080, 100 % | 17.6 / 16.8 / 17.1 | 17.1 / 17.7 / 16.6 |
| Linear, 8000 x 8000, 100 % | 17.2 / 17.0 / 17.2 | 18.9 / 18.2 / 18.0 |
| Linear, 8000 x 8000, fit (10 %) | 19.9 / 18.7 / 19.4 | 388 (0031 not in HEAD) |
| Reflected, 1920 x 1080, 100 % | 16.9 / 16.9 / 16.6 | - |
| Reflected, 8000 x 8000, fit | 19.6 / 19.5 / 18.7 | - |
| **Radial, 1920 x 1080, 100 %** | **33.5 / 26.4 / 26.6** | - |
| Radial, 8000 x 8000, fit | 33.3 / 24.7 / 24.5 | - |

- The apply over the whole 8000 x 8000 picture: 804 to 860 ms to return (HEAD 853 to 864), peak 1111 to 1113 MB (HEAD 1347).
- The first build of the blend computed the drag's geometry once per channel: Linear 21 to 23 ms and the apply 1.07 s, against
  HEAD's 17 ms and 0.86 s. Fixed before this table: the drag and the spans worked out once per lay, the pixel's place once for its
  four channels, and Radial's exact test in 64 bits (both sides below 2.1 x 10^18).
- **Over budget:** Radial at 100 % takes about two frames a move (26 to 34 ms against one frame); at the fit zoom it is within
  ADR-0004's 33 ms. The fallback of R1 (a per-row bound in place of the square root per pixel) is not built: for JOD.

**Measurements again (T009, 2026-11-09, the fade with the types).** `perf-0012` in the real session, no Easel window, load 2.7, two runs interleaved with HEAD (843562e) in a scratch worktree; median of 30 moves, ms.

| gradient dragged | this build, runs 1/2 | HEAD, runs 1/2 |
|---|---|---|
| Linear, 1920 x 1080, 100 % | 16.5 / 16.9 | 17.4 / 17.2 |
| Linear, 8000 x 8000, 100 % | 16.7 / 16.7 | 16.8 / 16.8 |
| Linear, 8000 x 8000, fit (10 %) | 16.8 / 16.7 | 436 / 423 (0031 not in HEAD) |
| Radial, 1920 x 1080, 100 % | 17.0 / 17.0 | - |
| Radial, 8000 x 8000, fit | 16.7 / 16.8 | - |
| Reflected, 1920 x 1080, 100 % | 16.7 / 16.7 | - |
| Reflected, 8000 x 8000, fit | 16.7 / 16.7 | - |

- The apply over the whole 8000 x 8000 picture: 706 to 709 ms (HEAD 821 to 836), peak 1113 to 1122 MB (HEAD 1347 to 1351).
- Every type within one frame at 100 % and within ADR-0004 at the fit zoom: the fade weighs the alpha once a pixel, where the
  blend weighed four channels, so R1's fallback is not needed.

**Constitution.** P7 by the one opacity function (REQ-0032-11); core in `src/core` with only libc; no ADR (no choice against
the constitution).

**Risks.**
- R1 Radial's square root per pixel at 100 % on 1920 × 1080 and below 100 % at the fit zoom: measured in T004 against 0012's
  frame and ADR-0004's 33 ms; fallback, a per-row incremental bound (D grows by 2 dx + 1 along a row), only if T004 is over.
- R2 The double estimate is off by more than one on a long drag: the correction loops until the exact test holds, so the
  result is exact whatever the estimate; AS-5's far case (T002) checks it. Each branch of the correction (up, down, A = 0,
  beyond R) gets a mutant in `tests/mutants.txt` (T002), and the `--core` run at the end (T008) reports any survivor of it.
- R3 (JOD: independent of the code's formula, not the same formula under another name) The reference of AS-5 must not be the inequality: it uses long double r and R and, where A (R − r) / R lies within 10⁻⁹
  of a half, decides by the exact equality 4A²D = L (2A − 2a + 1)² (T002).
- R4 Enter on a cell while a gradient waits: the window takes Enter before the focused button (REQ-0012-11, existing); T003's
  `as1` checks it with the focus on a cell.
- R5 The cells' density pattern at scale 2: drawn on a grid of logical pixels, each two device pixels, as the footprints are
  (REQ-0030-07); checked on the sheet (T001) and by `as7`'s scale-2 run reading the panel's pixels for partial alpha (T005).

## Tasks
Reworked 2026-11-08 after JOD's screencasts into a blend of the foreground into the background that replaced the picture; JOD withdrew that on 2026-11-09 (T009): the Results of T002 to T006 below describe the blend's build, and T009 says what was put back.
- [x] T001 The three cells drawn, and their sheet (REQ-0032-01, REQ-0032-02; AS-1) - done when the panel shows the Gradient well of three cells with their tooltips and accessible names, the panel sheet dump includes Gradient (light, dark; scale 1 and 2), and JOD approves the sheet (M1), kept as `docs/reference/0032/types.png`
  Result: ui `as1_gradient_types_panel` failed first (`'cells[i]' should not be NULL`), then passed (ui-0032 runs with the a11y test environment, as 0022, for the accessible names). Sheet made from the app's own panel (light, dark, scale 1 and 2); JOD approved it with T007 on 2026-11-09, kept as `docs/reference/0032/types.png`.
- [x] T002 The two-color blend and the three types in the core (REQ-0012-04, -05; REQ-0032-04 to -07, REQ-0032-11; 0012 AS-2 to AS-9, AS-13, AS-18; 0032 AS-2 to AS-5) - done when the core tests, rewritten for the blend and the types, fail first and pass, with the independent reference of R3, and the correction's mutants are in `tests/mutants.txt`
  Result: red by build (`'EaselGradient' has no member named 'color'`), then 19 of 19. Old/new of the core tests: AS-5 (blend over transparent, 0xE6000000 at column 6) now replaced pixels (0xFF191919); AS-6 (opacities 128/64/13) now the pixels #00000080, #7F7F7FBF, #E5E5E5F2; AS-7 (two layers, gray 2/63/206) now a second apply changing nothing; AS-8 (opacity only) now red into black; AS-13 (alpha-0 and half colors over white) now both colors equal to the picture; `blend_matches_the_formula` dropped with the blend; `gradient_matches_the_formula` became `pixel_matches_the_formula` for the three types. Hand breaks: the radial up-correction removed: red at `as5_types_against_reference`. Two breaks survived and changed the code: `+0.4999` in the estimate (equivalent: the up loop corrects it) and the down-correction removed (no input in 2 x 10^9 tries made the double estimate overshoot); the estimate now starts one below its floor and only the up loop remains, so no branch is unreachable.
- [x] T003 The type chosen, shown and applied; 0012's tests to the blend (REQ-0032-01 to -04, -07, -08, -10, -13; REQ-0012-02, -04, -05; AS-1, AS-6, AS-8) - done when ui `as1_gradient_types_panel` (the status bar, the kept choice, Enter while waiting), `as6_type_change_waiting` and `as8_types_keep_0012_rules` fail first and pass, and the 0012 tests are updated with old/new
  Result: red by link (`easel_tool_options_get_gradient_type` undefined), then green. Hand break: the canvas reading Linear always: `as2_types_user_path` red (`0xff000000 == 0xffffffff`). 0012 ui tests, old/new: `as1_gradient_toolbox_hints` (hint "Fades the drawing color to transparent..." and no options) now the blend's hint and the types' well; `as7_two_gradients_two_steps` (two steps, gray 2/63/206) now one step, gray 25/127/229; `as8_secondary_button` (bg faded over yellow) now bg into fg; `as10_new_press_applies` ((10, 5) 0x3F3F00, (10, 10) 0x7F7F00) now 0x7F7F00 and yellow, and after the background turns olive the primary's gradient is black into olive; `as11_drops_and_put_down` the same after its palette right-click; `as13_gradient_changing_nothing` (fg yellow, alpha 0, half) now both colors yellow, either button; `as17_handle_presses` each button's two colors; `as19_recolor` rewritten: a change of either color recolors, Ctrl+Shift+X trades start and end; the helpers `gradient_over_yellow` and `gradient_over` blend into yellow, the start-up background. 0006, 0008 and 0030 did not count Gradient's empty panel (green unchanged). New strings in all eleven .po files, the template regenerated.
- [x] T004 JOD's path per type (JOD's item 3; REQ-0032-11; AS-2, AS-3, AS-7) - done when ui `as2_types_user_path` (press, drag, release, Enter per type at 100 %, 50 % and 200 %: the window while waiting equals the window after Enter, the picture is the core's, Radial a disc, Reflected a mirrored band) and `/0032-scale2/as7_types_scale2` pass
  Result: red by link with T003, then green; broken by hand (the canvas reading Linear always): red, `0xff000000 == 0xffffffff`. Preview against put-down, every type at 100 %, 50 % and 200 %: 0 device pixels differ, at scale 1 and 2 (JOD's item 2: no grain on the user's path; the grain of the video came from stacked fades, gone with the blend). A test slip of mine: at 50 % only odd pixels are under a device pixel's center (0031), so the first drag from (100, 100) landed on (101, 101); the drag now uses odd pixels. Journey 10 of 0028 slanted its gradient: two along one horizontal drag are now one picture and one step.
- [x] T005 Measure (REQ-0032-12; AS-10) - done when `perf-0012` runs the three types at 100 % on 1920 × 1080 and at the fit zoom on 8000 × 8000, three runs each with the load noted, the numbers in the Plan (R1)
  Result: the Plan's Measurements. A regression of my first blend (Linear 21 to 23 ms, the apply 1.07 s) found by the interleaved run against HEAD and fixed; Radial at 100 % over one frame, reported to JOD.
- [x] T006 A type changed while held (REQ-0032-09; AS-9) - done when ui `as9_type_change_held` passes and is broken by hand (the type read at the press) with that run reported
  Result: passed at its first run (characterization: the canvas reads the type at each use since T003); broken by hand, a type change ignored while the button is held (`|| self->pend_held` in the setter): red, `differing == 0` (302 == 0). A first break on `action_button` survived: that flag is not the drag's.
- [x] T009 The blend withdrawn, the types kept (JOD, 2026-11-09; REQ-0012-02, -04, -05 as accepted; REQ-0032-04 to -07, -11; AS-2 to AS-8, AS-11, AS-12) - done when the core and the 0012 ui tests are HEAD's again with the types added, the 0032 tests are in one color, a test from the user's path per type covers the live preview, the marks, a recolor while waiting, Enter, stacking and Esc, and each new test is made red by a break
  Result: core tests rewritten from HEAD's (opacity, blend) with the types: red by build (`implicit declaration of function 'easel_gradient_opacity'`), then 25 of 25. New core `as11_types_stack` red when the lay ignores the picture under it (`img->px[...] == easel_gradient_blend (first, ...)`); new `as12_no_grain` red with a grain added on odd columns, to Linear and to Radial (`o <= prev && prev - o <= 1`). The 0012 ui tests and 0028's journey 10 are HEAD's again (git: the 30 hunks of the blend reversed; the types' well check in `as1_gradient_toolbox_hints` kept). New ui `as11_types_recolor_stack` red when the canvas keeps the old foreground while a gradient waits (`0xff000000 == 0xffff0000`). The hint and its eleven translations are HEAD's; Linear, Radial and Reflected kept. Mutants: 0012's gradient blocks back from HEAD, 6 finds moved to the new code, lint 1282, 0 stale. Full suite through `tools/isolated.sh` 119 of 119, scale-2 runs 8 of 8; the benchmark in the Plan's Measurements again. `--core`: `450 mutants: 445 killed, 0 killed by crash (test gap), 0 inconclusive (no snapshot or skipped), 4 survived, 0 errors, 1 accepted in 133 s`, the four 0023's, accepted at its check; no gradient survivor.
- [x] T007 JOD looks at the three types once (JOD's item 5; M1 sheet and M2) - done when JOD has tried each type with the mouse and the Result says what JOD saw
  Result (2026-11-09, after T009): JOD tried the three types with the mouse, the cells and the status bar: "all is good now".
- [x] T008 Gate - done when the full suite and the scale-2 runs pass through `tools/isolated.sh`, `tools/mutate.sh --core` has run once with no survivor of the correction step (any survivor reported), and the Plan's names match the code
  Result (2026-11-09, before JOD's look): full suite through `tools/isolated.sh` 119 of 119 (one slip of mine: the scale-2 case ran at scale 1 in the full suite; it now returns early there, as 0031's does); scale-2 runs 7 of 7; `--core`: `450 mutants: 445 killed, 0 killed by crash (test gap), 0 inconclusive (no snapshot or skipped), 4 survived, 0 errors, 1 accepted in 170 s`, the four survivors 0023's, accepted at its check; no survivor in the gradient's correction or types; lint 1282 mutants, 0 stale; the Plan's names as built.

## Check
- Date and commit: 2026-11-09, `843562e` plus the uncommitted 0031 and 0032 work (no code changed after the runs below).
- Tests: `tools/isolated.sh meson test` 119 of 119 (this feature: core-gradient 25 cases, ui-0032 7 cases with 0012's 25 updated
  for the types' well); scale-2 runs 8 of 8 (`/0032-scale2/as7_types_scale2` among them); warnings 0.
- Mutants: `--core` `450 mutants: 445 killed, 0 killed by crash (test gap), 0 inconclusive (no snapshot or skipped), 4 survived, 0
  errors, 1 accepted in 133 s`; the 4 survivors are 0023's (REQ-0023-02, -05, -06 twice), accepted at its check; no gradient
  survivor. Lint 1282, 0 stale. ui mutants of 0032: REQ-0032-08 (twice), REQ-0032-13, in `tests/mutants.txt`.
- Scenarios: AS-1 ui `as1_gradient_types_panel` + M1 (the sheet, approved); AS-2 to AS-5 core `as2_radial`, `as3_reflected`,
  `as4_radial_alpha_and_small`, `as5_types_against_reference` (the mutants of REQ-0032-05, -06) + ui `as2_types_user_path` (break:
  the canvas reading Linear always); AS-6 ui `as6_type_change_waiting` (mutant REQ-0032-08 no redraw); AS-7 ui
  `as2_types_user_path` and its scale-2 run; AS-8 ui `as8_types_keep_0012_rules`; AS-9 ui `as9_type_change_held`, C (T006: passed
  first, made red by ignoring a change while held, 302 pixels); AS-10 `perf-0012` (Plan, Measurements again); AS-11 core
  `as11_types_stack` (break: the lay ignoring the picture) + ui `as11_types_recolor_stack` (break: the foreground kept while
  waiting); AS-12 core `as12_no_grain` (breaks: grain on odd columns, Linear and Radial). M2: JOD, "all is good now".
- Task Results: T001 to T009 each have one (T002 to T006 describe the withdrawn blend's build; T009 what was put back).
- F1 (low) AS-8 names six cases; `as8_types_keep_0012_rules` tests two (the secondary button's color, Esc). Shift on Radial, a
  click with none waiting, a moved marker and a press outside the markers per type have no 0032 test: 0012 tests them for Linear
  and the type enters only in the pixels (`shown_gradient`), not in the ends or presses. Left open: Known gaps.
- F2 (low) AS-1's keyboard clauses (Tab to the well, arrows, Space; Enter choosing a cell with none waiting) have no test; GTK's
  toggle buttons give them, as in every other well. Left open: Known gaps.
- F3 (low) AS-6's "with memory again the next frame shows the new type" is not asserted (the test applies with Enter instead).
  Left open: Known gaps.
- F4 (info) AS-7 was narrowed on 2026-11-09 to black over white at 100, 50 and 200 %; the half-transparent color and picture
  are covered for every type by core `lay_is_the_apply` (the preview's and the apply's one function), and through the window for
  Linear by 0012's preview cases.
- F5 (info) T006 is a characterization, proven by its break (above).
- Verdict: clean for a user; three test gaps, none user-visible.

### Known gaps
- Per-type ui tests for AS-8's Shift, click, marker move and press outside (F1); AS-1's keyboard path (F2); AS-6's reshow
  after memory returns (F3).
