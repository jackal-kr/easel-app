# 0044 Idle redraw cost

Status: implemented
Reviewed: spec reviewer pass, 13 findings merged
Depends on: 0013, 0014, 0017, 0025, 0043 (the moving dashes and what lies under them)

## Why
While a selection, an open text box or a waiting shape shows its moving dashes, Easel draws the whole canvas again 12 times a second,
also when nothing else changes: at rest it uses 14 % of a core for a marked selection and up to 32 % for a waiting Smooth Heart (0043's
check of 2026-11-13, F2), which a user feels as a warm laptop, a fan and a shorter battery. The dashes are the only thing that moves.
The same spec measures again, on a quiet machine, the one handle drag that read over a frame under load (0043, 2026-11-13).

## Requirements
"At rest" (all of this spec): no input event reaches the app; the pointer is outside the window (AS-1 has one case with it resting over
a selection); the window keeps its focus, size and scale; no setting, file or clipboard changes; anything that still moves on its own in
the window (a text field's blinking caret) is part of what is measured. "Shown from nothing": the pixels the app shows when the window is
first drawn in that state. A CPU share is the CPU time of all threads of the app's process over the wall time measured.
- REQ-0044-01 (Change of 2026-11-13, JOD, after T001 and T001b: the 5 % and the 0 % are measured with GTK's default renderer, in the
  isolated test session (Vulkan on the GPU) and confirmed once by hand in JOD's real session at T005; with the Cairo renderer forced, as
  the ui tests run, the numbers are a guard only, no worse than the same-day baseline measured before the change (T002: 33.4, 19.9, 14.6,
  50.6, 23.6, 18.1 and 18.3 %; JOD, 2026-11-13: not T001's numbers), with no bound: the Cairo renderer
  draws in software, and its frame alone costs about 10 % there (the Plan's tables, T001 and T001b). The dashes keep 12 steps a second.)
  While the canvas is at rest with moving dashes (a marked or floating selection, an open text box or a waiting shape, with
  animations on: REQ-0014-17 (c), REQ-0017-05, REQ-0043-19), the app shall use at most 5 % of one CPU core, measured over 3 seconds after
  half a second at rest, in the isolated test session at scale 1, on the cases of AS-1, the smallest of them included. At scales 2 and
  1.25 the same cases are measured and recorded, not bound. The table of performance budgets gains the row "Idle with moving dashes:
  at most 5 % of a core (isolated test session, scale 1)", by an ADR.
- REQ-0044-02 With animations off (the dashes stand still, REQ-0014-17 (c)), the canvas at rest shall not be painted again, and the app
  shall use 0 % of a core, read as at most 50 ms of CPU time over 10 seconds (the measurement's resolution), as the budget "Idle: 0% CPU
  when nothing is happening" says.
- REQ-0044-03 The dashes shall move as REQ-0014-17 (c) says (12 logical pixels a second along the line; standing still while a button is
  held and with animations off), and at every moment the canvas area shall show exactly the pixels shown from nothing in that state (P7;
  REQ-0025-02 to -05 and -07): the picture, the previews, the tint, the grid, the line, the handles, the caret, the keyboard marker and
  the Magnifier's box. Anything that changes what the canvas area shows shows at its next frame.
- REQ-0044-04 If memory runs short, the app shall still show at every frame the pixels shown from nothing, with no dialog; only its CPU
  use may then be higher.
- REQ-0044-07 (JOD, 2026-11-13) While only the dashes advance, at rest, in every case of REQ-0044-01, the app shall build nothing of
  what lies under the line again: a count of those builds, kept for the tests, stays 0 from one frame to the next.
- REQ-0044-06 (JOD, 2026-11-13) While the window is minimized or wholly hidden, the moving dashes shall cause no redraw and no CPU use
  (as REQ-0044-02); shown again, they move on from where the clock puts them.
- REQ-0044-05 (a measurement, from 0043's check) 0043's benchmark of a waiting Smooth Heart, width 5, Outline and fill, on a 1920 × 1080
  picture at Fit to Window, its bottom-right handle dragged, shall be measured in three runs in JOD's real session (its default renderer,
  its monitor's refresh rate), each run starting with a 1-minute load average under 0.5, and the medians from pointer event to painted
  frame recorded with each run's load. A median above one refresh period of the screen (16.7 ms at 60 Hz) is reported to JOD as a
  finding, not fixed in this spec.

## Scenarios
Unless said otherwise: the isolated test session at scale 1, its window as it opens (its size in the Plan), animations on, at rest
(above); "a waiting Heart" is a Heart of Shapes, width 5, Outline and fill, dragged from (2, 2) to (W − 3, H − 3) of a W × H picture and
released; "Fit" is View › Zoom › Fit to Window.
- AS-1 (REQ-0044-01) Given a 1920 × 1080 picture at Fit, When a Smooth waiting Heart is at rest, Then the app uses at most 5 % of a core (32 % before this change); the same for a Sharp waiting Heart (19 % before), a marked Select rectangle from (2, 2) to (1917, 1077) (14 % before), that rectangle with the pointer resting over its inside, that selection floating after a move by (10, 10), a Free-Form selection traced through 400 corners round the picture, and an open text box with "Hello" from (100, 100) to (700, 300); Given a 200 × 150 picture at Fit, When a Smooth waiting Heart from (2, 2) to (197, 147) is at rest (the smallest case; 17 % before), Then at most 5 %; Given scales 2 and 1.25, Then the same cases are measured and recorded | Verified by: automated (ui, isolated, a timing case outside the normal suite) + the numbers recorded in the Plan
- AS-2 (REQ-0044-02) Given each case of AS-1 at scale 1 with animations off, Then the canvas is not painted again while at rest and the app uses at most 50 ms of CPU time over 10 s | Verified by: automated (ui, isolated)
- AS-3 (REQ-0044-03) Given each case of AS-1 at rest, When the dashes have moved by 1 logical pixel, Then the canvas area equals the pixels shown from nothing at the same moment, and the dashes have moved; When, at rest, any of these happens, Then the next frame equals the pixels shown from nothing: Undo, Redo, Paste and Clear Image; a scroll by 1 logical pixel; a zoom change; the window resized by 1 pixel; the grid turned on; the selection's handles hidden; Opaque and Transparent switched on a floating selection; the keyboard marker moved; a palette color, a width, a fill style and Smooth changed on a waiting shape; a letter typed and the font size changed in an open text box; the tint changed in Preferences; the accent color, the dark style and the high-contrast style switched; the window losing focus and getting it back; the animations setting switched off (the first dash back to the start of the line at once) and on (the dashes start); the pointer moved over a handle; the Magnifier chosen and its box shown over a marked selection; and the screen's scale changed from 1 to 2 at rest; the whole at scales 1, 2 and 1.25 | Verified by: automated (ui, a rendered window, isolated at scales 1, 2 and 1.25)
- AS-4 (REQ-0044-04) Given memory made too short for what the app keeps between frames, When the dashes move at rest, Then each frame equals the pixels shown from nothing, no dialog shows, and the dashes go on moving | Verified by: automated (ui)
- AS-7 (REQ-0044-07) Given each case of AS-1 at rest, When the dashes advance by 10 steps, Then the count of builds of what lies under the line is the same before and after, while the frames drawn went up by at least 10; When the picture changes (Undo), Then the next frame builds it once | Verified by: automated (ui, the normal suite, deterministic)
- AS-6 (REQ-0044-06) Given a marked selection at rest with animations on, When the window is minimized, Then the canvas is not painted while it stays minimized and the app uses at most 50 ms of CPU time over 10 s; When it is shown again, Then the dashes move and the frame equals the pixels shown from nothing | Verified by: automated (ui, isolated, the timing case) + T001's check of what happens today
- AS-5 (REQ-0044-05) Given JOD's real session and a quiet machine, When 0043's benchmark drags the fitted 1920 × 1080 Smooth Heart's handle, three runs, Then the three medians and loads are recorded | Verified by: measurement recorded in the Plan

## Edge cases
- Input (a press, a key, a pointer motion, a hover change, a scroll, a zoom) is not at rest: the frames it causes are drawn as needed and
  are not counted in REQ-0044-01.
- Two things with moving dashes cannot show at once: a waiting shape is put down when another tool is chosen (REQ-0043-10), a selection
  is put down when Shapes or Text is chosen (REQ-0013-11), and an open text box is put down when another tool is chosen (REQ-0017-17).
- The caret of a text box blinks (found at T003: the first wording said it did not), and a text field's own caret (the font card's size
  field) blinks as GTK makes it; both are part of what is measured, and the box's caret, drawn over the line, redraws only what lies over
  the line (REQ-0044-07 holds with a box open).
- A window not in focus but visible: the dashes keep moving, as today and as a selection does in XP (REQ-0014-17 (c) says nothing of
  focus); the bound holds the same; AS-3 checks the frames on losing and getting back the focus (JOD, 2026-11-13).
- A minimized or wholly hidden window (JOD, 2026-11-13): the canvas is not drawn again and the dashes' redraws stop while it is not shown;
  on showing again the dashes are where the clock puts them. (REQ-0044-06.)
- A screen whose scale is not whole (REQ-0025-07): the frames equal the pixels shown from nothing there too (AS-3 at 1.25).
- The thumbnail (REQ-0021-06, -07, -14) shows any change as today; at rest nothing in it changes, and it is not drawn again then (as
  today).

## Known gaps
- The handle drag of a fitted 1920 x 1080 Smooth shape drops about one frame in ten (T006: median 17.3 / 18.8 / 17.7 ms, 90th percentile
  33.1 / 36.6 / 33.1 ms, worst 33.4 / 48.7 / 33.6 ms on a quiet machine); not caused by this feature (1.0.3 reads the same); accepted as a
  known case (JOD, 2026-11-13), in `docs/phase2-plan.md`.

## Out of scope
- The cost of a frame while something moves (a drag, a scroll, a zoom): the existing budgets cover it.
- The real session's renderers: REQ-0044-01 is measured in the isolated test session; the real session is looked at in M1.
- The session's compositor's own work for each frame: not the app's process.
- Any change to how the dashes look or move.

## Open questions
None.

## Plan
(Opus, 2026-11-13.) Read with `docs/engineering.md` (Performance budgets), 0014 (the moving dashes), 0025 (the pixel contract) and the code
of the canvas's frame (`easel_canvas_snapshot`, `canvas_snapshot_frame`, `ants_tick_cb`, `ants_phase`).

**What happens today.** The dashes' timer (`ants_tick_cb`, 12 a second) calls `gtk_widget_queue_draw` on the canvas; GTK then asks the
canvas for its whole frame (`easel_canvas_snapshot`), which builds every node again: the picture's tiles (textures kept, nodes new), the
shape's preview (rows rasterized again: the Smooth Heart's coverage at every frame, which is why it costs most), the grid, the tint, the
line, the handles; GSK then compares the new tree with the last and draws what differs. After each frame the canvas emits `painted`,
and the window updates the text box's accessible value (a copy of the whole text) and the frame-time readout.

**The approach to prove or reject: keep what lies under the moving line, draw only the line and what lies over it.**
- A frame is two parts in the snapshot's order: the **base**, everything drawn before the line (the tiles, the previews, the grid, the
  bands, the gradient's markers, the picture's handles, the float outside the picture, the tint, the text's highlight), and the **top**,
  the line and what lies over it (`append_selection_outline`, `append_selection_handles`, the caret, the Magnifier's box, the keyboard
  marker). The base is snapshotted into its own node (`gtk_snapshot_push`/pop into a container, or a `GtkSnapshot` of its own turned into
  a node) and kept with a **key**; at a frame whose key is the kept one, the kept node is appended as it is and only the top is built.
- **The key** says what the base depends on: a generation counter, the zoom, the screen's scale and the device shift (REQ-0025-07), the
  visible area (`seen_in_drawing`), the canvas's size, the accent color, the dark and high-contrast styles, the tint. The generation goes
  up at every `gtk_widget_queue_draw` of the canvas except the dashes' own (`ants_tick_cb`): the canvas's ~100 calls go through one
  `canvas_redraw (self)` that bumps it, and the dashes' timer alone calls `gtk_widget_queue_draw` bare. The key's other fields catch the
  frames GTK asks for by itself (a style, a scale, a resize).
- **Why it should help:** GSK compares render nodes by identity first: the kept base node is the same node, so its subtree is not
  compared again nor drawn again; the damage of a dashes-only frame is the line's rectangles, and the renderer draws only that region
  (Cairo clips to it). Risk 1 says what is not known.
- **The dashes' clock** (JOD, 2026-11-13: a minimized or hidden window must not keep redrawing, REQ-0044-06; the frame clock preferred
  "if that does not complicate the Plan"). Decided at T003: **the timer stays**, and stops itself when the redraw it last asked was never
  drawn (the window draws nothing: minimized, wholly hidden); the next frame drawn starts it again, as drawing the line always did. Why not
  the frame clock: 0014's check (F8, T024) decided that the dashes' callback runs once a step, 11 to 13 times a second, not once a frame,
  and `/0014/as25_dashes_timer` holds it; a tick callback runs 60 times a second. And the minimized state cannot be read here: the
  minimized surface reported `0xaa00` in T001 (no minimized or suspended flag); only the frames stopping show it, which is what the timer
  now watches. Measured (T003's probe, second by second after minimizing): the minimize's own frames in the first second, then 0 redraws
  asked and 0 frames for 4 s; the session draws a minimized window about once in 5 s, after which the dashes ask once and stop.
- **Memory (REQ-0044-04):** the kept node references the tiles and preview nodes that exist anyway; nothing large is allocated. A test hook
  (`easel_canvas_set_keep_base (FALSE)`) makes every frame be built from nothing: it is the reference of AS-3 and AS-4's "memory made too
  short" (keeping refused, every frame from nothing, no dialog).
- **The work after each frame:** `on_canvas_painted` sets the text box's accessible value only when the text changed (a revision kept
  with the buffer), not at every frame.
- **Animations off (REQ-0044-02):** nothing is drawn at rest today (0 % measured); a test asserts the canvas is not painted (a frame
  counter, `frames_drawn`) and the CPU over 10 s.

**Where the bound is measured (JOD, 2026-11-13, after T001 and T001b).** GTK's default renderer: the timing case runs in
`tools/isolated.sh` without `GSK_RENDERER` (GTK then picks Vulkan on the GPU), and the result is confirmed once in JOD's real session at
T005 (his "ready"). The same case under `GSK_RENDERER=cairo` is a regression guard: no case worse than T002's same-day baseline (a tenth over it
allowed for noise), no bound. If the
isolated default-renderer measurement is too noisy to automate (T002 runs it five times and says), the real-session measurement is the
check, recorded in the ADR. AS-7's count of base builds is the deterministic test in the normal suite.

**Measuring.** A timing case `/0044-open/idle_cpu` (under `-open`, which no meson test runs): for each case of AS-1, the window as it
opens (1440 x 900 logical at scale 1), half a second at rest, then 3 s (10 s for AS-2) in a blocking `GMainLoop` (a busy wait would count
itself), `getrusage (RUSAGE_SELF)`, the share printed and asserted. Run by `tools/isolated.sh` (and `--scale 2`, `--scale 1.25`), under
`GSK_RENDERER=cairo` as the ui tests. The pointer outside the window is the isolated session's default (no pointer motion is sent).

**Tests.** ui `/0044/as3_kept_frame_equals_fresh` (each case and each change of AS-3: the frame with the base kept against the frame with
`keep_base` off, at scales 1, 2 and 1.25 by the scale suites); ui `/0044/as4_keep_refused`; the timing case for AS-1 and AS-2.

**Per task:** the ui suites that compare screen captures (`ui-0010`, `ui-0013`, `ui-0014`, `ui-0017`, `ui-0022`, `ui-0025`, `ui-0031`,
`ui-0039`, `ui-0043`, `ui-0007`), the scale 2 list and the 1.25 and 1.5 suites, core and io.

**Mutants:** a change that does not bump the generation (the base kept over a changed picture); the key without the scale, the shift, the
visible area or the accent; the dashes' timer bumping the generation (the base never kept); the top drawn under the kept base.

Constitution: no core rule changes (the frame's composition is the ui's); the pixels shown are the same by construction and by AS-3
(P7.2, P7.3); a new budget row by ADR (REQ-0044-01). Risks:
1. **The floor** (decided by JOD, 2026-11-13, if it is near or over 5 %: first the dashes step less often, 6 to 8 steps a second in place
   of 12 (a change to REQ-0014-17 (c) then, by the spec loop); second, the bound raised by an ADR; the measurement is not redefined). Whether a frame that changes only the line costs under 5 % of a core in the isolated session at all: GTK's frame clock,
   the snapshot of the whole window (the other widgets keep their nodes), the node diff and the Cairo renderer's work on the damage, and
   copying the frame out. The line of a selection round the whole picture spans its whole border, so its damage is long. **T001 measures
   the floor first** (a window whose canvas changes only a 1-pixel area 12 times a second, and the line alone over a kept base, by a
   quick prototype not kept), and the work stops for JOD if the floor is near or over 5 %: then a decision of his (the bound by ADR, the
   dashes stepping less often, or the bound counting the canvas's own work only).
2. **GSK keeps no node across frames as we expect** (a container node copied, or the diff walking it anyway): T001's prototype measures it.
3. **A change that does not go through `canvas_redraw` and is not in the key** shows a stale base: AS-3's list and the mutants; a grep at
   T003 that no `gtk_widget_queue_draw (GTK_WIDGET (self))` is left in `canvas.c` but the timer's.
4. **REQ-0044-05** is a measurement in JOD's real session on a quiet machine, after his "ready" (T006); its result is recorded, not fixed.

**Measured (T001, 2026-11-13, the isolated session at scale 1, Cairo, the window as it opens; 3 s at rest after 0.5 s; a throwaway
prototype, not kept):** CPU share of a core; frames drawn in the 3 s; the canvas's own snapshot time a frame.

| Case | Today | The base kept, the top drawn | The floor: the base kept, one pixel changed |
|---|---|---|---|
| 1920 x 1080 Smooth Heart, Fit | 32.5 %, 36 frames, 11.39 ms | 22.4 %, 0.25 ms | 14.7 %, 0.22 ms |
| 1920 x 1080 Sharp Heart, Fit | 16.8 %, 1.30 ms | 17.8 %, 0.13 ms | 10.0 %, 0.07 ms |
| 1920 x 1080 marked selection, Fit | 12.5 %, 0.10 ms | 12.9 %, 0.08 ms | 10.3 %, 0.01 ms |
| 200 x 150 Smooth Heart, Fit | 17.4 %, 4.28 ms | 12.4 %, 0.10 ms | 10.5 %, 0.04 ms |

Keeping the base removes the canvas's own work (11.4 ms to 0.25 ms a frame for the Smooth Heart) but not the frame's: a frame in which
the canvas changes one pixel costs about 10 % of a core at 12 frames a second here, about 8 ms a frame outside the canvas's snapshot
(GTK's frame and the Cairo renderer's work on the window). The floor is twice the bound. A minimized window (the surface's state
minimized, the widget still mapped): 1 frame drawn in 3 s, 0.4 % of a core, while the dashes' timer still asked 36 redraws; GTK draws
nothing for them, so REQ-0044-06 holds today in the CPU it costs, with a timer waking 12 times a second for nothing. The frame clock's
tick callback ran once in those 3 s (it stops with the surface). After `gtk_window_unminimize` the headless session drew no frame (not
read further).

**Measured (T001b, 2026-11-13, JOD's real session):** the same four cases, today's build (no change), Easel's own process from outside
(`top -b -d 1`, 30 samples of 1 s after half a second at rest), animations on, scale 1.25, GTK's default renderer there
(`GskVulkanRenderer`, the AMD Radeon 860M), the window as it opens (1152 x 720 logical; Fit gives 66 % and, for 200 x 150, 475 %), three
runs, load 0.83 to 1.40 (`.scratch/t001b.sh`).

| Case | Real session, Vulkan (3 runs) | Isolated session, Cairo (T001, today) |
|---|---|---|
| 1920 x 1080 Smooth Heart | 24.6 / 24.7 / 24.8 % | 32.5 % |
| 1920 x 1080 Sharp Heart | 5.1 / 5.2 / 5.3 % | 16.8 % |
| 1920 x 1080 marked selection | 2.4 / 2.4 / 2.5 % | 12.5 % |
| 200 x 150 Smooth Heart | 7.2 / 7.2 / 7.4 % | 17.4 % |

**The renderers.** The ui tests set `GSK_RENDERER=cairo`: Cairo draws on the CPU, so T001's isolated numbers are software rendering.
With nothing set, GTK picks Vulkan in the isolated session too, on device 0, the AMD Radeon 860M (`radv`; lavapipe's llvmpipe is installed
and listed, not picked); in JOD's real session it is `GskVulkanRenderer`. So the isolated floor of about 10 % (T001) is the Cairo
renderer's frame, not GTK's on a GPU: in the real session a marked selection, a frame of the line alone over an unchanged picture, costs
2.4 %. What costs there is the shape's preview built again at each frame (the Smooth Heart 24.7 % against the selection's 2.4 %), which
is what keeping the base removes (T001's prototype: the Smooth Heart's own snapshot 11.4 to 0.25 ms a frame).

**Measured (T002, 2026-11-13, the isolated session, scale 1, today's code, `/0044-open/idle_cpu`):** five runs under GTK's default
renderer (`GskVulkanRenderer`, the GPU) and one under Cairo (the guard's baseline); share of a core at rest.

| Case | Default renderer, 5 runs, animations on | Cairo, animations on |
|---|---|---|
| 1920 x 1080 Smooth Heart | 20.7 / 18.8 / 24.9 / 19.1 / 19.8 % | 33.4 % |
| 1920 x 1080 Sharp Heart | 4.3 / 5.0 / 4.9 / 5.2 / 4.9 % | 19.9 % |
| 1920 x 1080 marked selection | 1.6 / 2.0 / 1.9 / 1.9 / 1.7 % | 14.6 % |
| 1920 x 1080 floating selection | 16.6 / 18.5 / 19.1 / 18.1 / 18.2 % | 50.6 % |
| 1920 x 1080 Free-Form selection, 400 corners | 6.3 / 6.8 / 7.2 / 6.2 / 6.7 % | 23.6 % |
| 1920 x 1080 open text box | 2.8 / 3.1 / 2.9 / 3.0 / 3.0 % | 18.1 % |
| 200 x 150 Smooth Heart | 7.1 / 7.0 / 10.5 / 6.9 / 7.2 % | 18.3 % |

With animations off every case reads 0.0 % (0 ms in 10 s) but the open text box, 0.4 % under the default renderer (40 ms in 10 s,
inside the 50 ms) and 2.1 % under Cairo: something still draws there at rest, likely the font card's size field's blinking caret, which
the spec counts as measured. Minimized: 0.1 to 0.2 %. **Steady enough?** The runs agree within one or two points, with one outlier in a
case (the Smooth Heart 24.9 % against 18.8 to 20.7 %): enough for a record, not for a normal-suite test that must not fail by chance
near 5 %. So the deterministic check in the normal suite is AS-7's count; the timing case stays under `/0044-open/`, run at T005 and at
the gate, its numbers recorded, and the real-session measurement of T005 is the budget's check, recorded in the ADR.

**Measured (T005, isolated, 2026-11-13, the change built, `/0044-open/idle_cpu`):** share of a core at rest, animations on.

| Case | Default renderer, scale 1 (the bound) | Scale 1.25 | Scale 2 (recorded) | Cairo, scale 1 (guard: T002) |
|---|---|---|---|---|
| 1920 x 1080 Smooth Heart | 1.7 % | 2.4 % | 1.9 % | 21.1 % (33.4) |
| 1920 x 1080 Sharp Heart | 3.6 % | 3.9 % | 3.8 % | 18.8 % (19.9) |
| 1920 x 1080 marked selection | 1.8 % | 2.3 % | 1.7 % | 15.3 % (14.6) |
| 1920 x 1080 floating selection | 1.7 % | 2.3 % | 1.6 % | 45.7 % (50.6) |
| 1920 x 1080 Free-Form selection | 4.8 % | 4.4 % | 5.4 % | 22.5 % (23.6) |
| 1920 x 1080 open text box | 1.6 % | 2.0 % | 1.7 % | 17.7 % (18.1) |
| 200 x 150 Smooth Heart | 1.7 % | 1.6 % | 1.8 % | 19.2 % (18.3) |

Animations off and minimized: 0 to 23 ms in 10 s at every scale (the text box's caret the most); under Cairo the box's caret 184 ms
(T002: 210). The 1.25 result is far under JOD's "about 10 %". The Free-Form selection of 400 corners is the nearest the bound at scale 1
(4.8 %) and over it at scale 2 (5.4 %, recorded, no bound there).

**Measured (T005, JOD's real session, 2026-11-13, after his "ready"):** scale 1.25, Vulkan, Easel's own CPU from outside, three runs:
Smooth Heart 2.5 / 2.6 / 2.5 % (24.7 before), Sharp Heart 4.1 / 3.9 / 4.0 % (5.2), marked selection 2.5 / 2.4 / 2.5 % (2.4), 200 x 150
Smooth Heart 2.0 / 2.0 / 2.0 % (7.3): every case at most 5 % (ADR-0009). **The texture of a smooth preview against the drags (JOD's
condition (d)):** `perf-0043` and `perf-0039`, 1.0.3 (a worktree of `v1.0.3`) against the change, interleaved, three runs each, load 0.3
to 1.6; medians of a drag's events to the painted frame, ms:

| Case | 1.0.3 | The change |
|---|---|---|
| perf-0043, 1920 x 1080 at 100 %, Heart, Sharp / Smooth | 16.6-16.8 / 16.7-17.1 | 16.4-16.8 / 16.8-16.9 |
| perf-0043, 1920 x 1080 fitted, Heart, Sharp / Smooth | 16.6-16.7 / 16.8-17.8 | 16.5-16.6 / 17.4-17.8 |
| perf-0043, 8000 x 8000 fitted, Heart, Sharp / Smooth | 16.6-16.8 / 30.2-33.4 | 16.5-16.8 / 32.0-33.4 |
| perf-0043, 1920 x 1080 at 100 %, Oval Callout, Sharp / Smooth | 16.7-17.0 / 16.7-16.8 | 16.5-16.9 / 16.5-16.7 |
| perf-0043, 1920 x 1080 fitted, Oval Callout, Sharp / Smooth | 16.7 / 16.7-16.8 | 16.5-16.7 / 16.6-17.0 |
| perf-0043, 8000 x 8000 fitted, Oval Callout, Sharp / Smooth | 16.6-16.8 / 26.8-32.5 | 16.6 / 27.3-28.8 |
| perf-0039, Smooth Ellipse drag 1920 x 1080 at 100 % / fitted / 8000 x 8000 fitted | 16.8-16.9 / 16.8-17.0 / 17.2-17.4 | 16.8 / 16.9-17.1 / 17.4 |
| perf-0039, Smooth Brush, circle of 7, 1920 x 1080 at 100 % | 16.7-16.9 | 16.7 |
| perf-0039, release of a Smooth whole-picture Ellipse, next frame, 1920 x 1080 / 8000 x 8000 | 16-47 / 32-36 | 18 / 33 |

The put-downs of `perf-0043` the same (8000 x 8000 Smooth 415 to 428 ms both). The two builds overlap in every case: the texture costs a
drag nothing measurable. The fitted 1920 x 1080 Smooth Heart is just over a frame in both (REQ-0044-05's case; T006 measures it on a
quiet machine, load under 0.5, which this run was not throughout).

### Scenario-to-test map
| AS | Test |
|---|---|
| AS-1 | `/0044-open/idle_cpu` (isolated, default renderer, scales 1, 2, 1.25; Cairo as the guard), numbers in the Plan (T002, T003, T005); JOD's real session by hand (T001b, T005) |
| AS-2 | `/0044-open/idle_cpu`, animations off (10 s) |
| AS-3 | ui `/0044/as3_kept_frame_equals_fresh` (the dashes alone, the seven cases) and `/0044/as3_changes` (36 changes and the eight setters; `ui-0044`, `ui-0044-scale2`, `ui-0044-scale125`) |
| AS-4 | ui `/0044/as4_keep_refused` |
| AS-5 | `perf-0043` in JOD's real session on a quiet machine (T006), recorded here |
| AS-6 | ui `/0044/as6_minimized_no_redraw`; `/0044-open/idle_cpu` (minimized) |
| AS-7 | ui `/0044/as7_no_base_build_at_rest` (normal suite, deterministic) |

## Tasks
- [x] T001 The floor and a prototype (REQ-0044-01, -06; Risks 1 and 2) - done when what happens today when the window is minimized is
  measured (redraws and CPU) and whether a frame-clock tick callback stops there; and the idle CPU of each case of AS-1 is broken down (the canvas's
  snapshot, the rest of the frame, the window's work after it) and measured with a quick prototype that keeps the base and builds only the
  top, not kept in the tree; the floor and the prototype's numbers are in the Plan; if either is near or over 5 % the work stops for JOD
  Result (2026-11-13): measured, the numbers above; the prototype (a kept base node and the top drawn alone, behind environment
  variables, with a case under `/0044-open/`) was thrown away: `src/ui/canvas.c` and `tests/ui/test-window.c` are as committed. **Stop
  (Risk 1): the floor, a one-pixel change 12 times a second, is about 10 % of a core (10.0 to 14.7 %), twice the bound.** The kept base
  alone brings the Smooth Heart from 32.5 to 22.4 % and leaves the others where they were. A minimized window draws nothing (0.4 %); the
  frame clock's tick callback stops there. Waiting for JOD before T002.
- [x] T001b The real session (REQ-0044-01; JOD, 2026-11-13) - done when the four cases are measured in JOD's real session (Easel's own
  CPU from outside, 30 s, scale 1.25, three runs) and the renderers of both sessions are said
  Result (2026-11-13): the table above. Real session, Vulkan: Smooth Heart 24.7 %, Sharp Heart 5.2 %, marked selection 2.4 %, 200 x 150
  Smooth Heart 7.3 %; the isolated numbers of T001 are Cairo's, in software. A temporary case in `tests/ui/test-window.c` set up each
  scene; the file is as committed again. A first run measured nothing (the script looked for `^READY` where GLib's test output writes
  `# READY`), fixed and run again.
- [x] T002 The failing tests (REQ-0044-01 to -07; AS-1 to AS-4, AS-6, AS-7) - done when ui `/0044/as7_no_base_build_at_rest` (a test hook
  counting builds of the base) and the minimized check (the dashes ask no redraw while minimized) are red today; `/0044-open/idle_cpu` is
  written, red today under the default renderer (the Smooth Heart over 5 %), run five times to say if it is steady enough to automate,
  and run under Cairo for the guard's baseline; AS-3 and AS-4's cases written (green today: no base is kept yet, their breaks named in T004);
  AS-2 green today (characterization); no product code changed
  Result (2026-11-13): two test hooks added so the cases build (`easel_canvas_get_base_builds`, counted at every build of the base, today
  every frame; `easel_canvas_set_keep_base`, stored, without effect yet); no other product code. Red today: `/0044/as7_no_base_build_at_rest`
  (`b1 - b0 == 0`: 12 builds in a second of the Smooth Heart, one a frame) and `/0044/as6_minimized_no_redraw` (`r1 - r0 == 0`: the dashes
  asked 12 redraws in a second minimized); `/0044-open/idle_cpu` red under the default renderer (the Smooth Heart, the floating
  selection, the Free-Form one and the small Heart over 5 %) and under Cairo (the floating selection had no baseline yet: T002's 50.6 % is
  now the guard). Green today, characterization, as planned: `/0044/as3_kept_frame_equals_fresh` (each of the seven cases: the dashes moved
  8244 device pixels on 1920 x 1080, 0 pixels differ kept against fresh) and `/0044/as4_keep_refused`; AS-2's part of the timing case
  (0 to 40 ms in 10 s). New meson test `ui-0044` (`-p /0044`). The numbers above.
- [x] T003 The kept base and the frame clock (REQ-0044-01, -03, -04, -06, -07) - done when `canvas_redraw`, the key, the base and top
  split, the base-build counter and `keep_base` are in, the dashes are driven by the widget's tick callback (no timer), no bare
  `gtk_widget_queue_draw` of the canvas is left, `on_canvas_painted` sets the accessible value only on a change, AS-7 and the minimized
  check pass, AS-3 and AS-4 pass, the timing case under the default renderer is under 5 % at scale 1 and the Cairo guard holds, the
  mutants are killed, and the per-task list is green
  Result (2026-11-13): built in three steps, each measured.
  (1) The kept base: `canvas_redraw` (93 calls and the scroll's) marks a change; the dashes' step, the caret's blink and the clock's test
  hook ask a redraw without one; the frame is split (`canvas_snapshot_frame` what lies under the line, `canvas_snapshot_top` the line and
  over it, `canvas_snapshot_parts` keeps the base node while its key holds); the dashes' timer stops itself in a window that draws nothing
  (the frame clock not taken: the Plan, "The dashes' clock"); `on_canvas_painted` sets the accessible value only on a change. Found on the
  way, each a redraw that the dashes had hidden by drawing everything 12 times a second, fixed: eight setters asked no redraw (the picture,
  the guides, the view's, preview's, shape's and text's memory limits, the selection's Transparent mode, the bands); a color change redrew
  a waiting gradient or shape only, not an open Opaque box's fill (`/0017/as13_text_color` red) nor a floating selection's hole; the caret
  blinks (the spec said not: corrected). Smooth Heart then 7.4 % (JOD, 2026-11-13: option 1).
  (2) A smooth figure's preview as one texture of its own box (`append_cover_texture`: its opaque spans laid into a buffer of their box,
  drawn by `append_cells_by_edges` where the view is built on the edges, else scaled as the tiles; a span with transparency, a sampled
  step or no memory under the preview's limit: the rectangles as before, no dialog). Exactness: `/0039/as7_preview_is_put_down` (the scaled
  texture, 100 % and 800 %) and `/0043/as11_shown_is_put_down` (by edges at 25 %, and scrolled 400 %); a break (each texture pixel one
  step off) made ui-0039 and ui-0043 red, reverted. A try that drew the whole kept base into a texture rounded the tint one step off
  (2148 pixels) and was taken out.
  (3) The line's white kept with the base, its black dashes alone drawn at each step (`line_part`): the Free-Form line of 400 corners is
  thousands of grid steps.
  Red first: AS-7 (12 builds a second) and the minimized check (36 redraws asked in 3 s against 1 frame); green now, and AS-3, AS-4.
  The timing case, default renderer, scale 1, two runs: Smooth Heart 2.1 / 1.8 %, Sharp Heart 3.6 / 3.6, marked selection 1.9 / 1.8,
  floating 2.0 / 2.0, Free-Form 4.9 / 4.8 (closest to the bound), text box 2.1 / 2.0, 200 x 150 Smooth Heart 1.9 / 1.9; animations off and
  minimized 0 to 18 ms in 10 s. Under Cairo (the guard, against T002's same-day run): 20.6, 19.2, 15.8, 45.1, 22.3, 18.0, 19.5 %; off: the
  text box 2.0 % (its caret; T002 2.1). Mutants by id: all of 0044's and the thirteen repointed killed; a survivor of REQ-0043-19 (the line
  over the shape's handles) showed `as15_line_pixels` read one pixel of the handle where the line happened to be white: it reads the
  handle's five now, killed. REQ-0032-08 and REQ-0004-12 survive as at the 1.0.0 gate; one REQ-0014-17 not repeated (a flake). Per-task
  list green: ui 17 of 17, core and io 52 of 52, scale 2 5 of 5, 1.25 and 1.5 1 of 1.
- [x] T004 Every change of AS-3 at scales 1, 2 and 1.25 (REQ-0044-03; AS-3) - done when the case covers the whole list at the three scales and
  each of the Plan's mutants and a manual break (the key without the accent) makes it red
  Result (2026-11-13): `/0044/as3_changes` (a 400 x 300 picture at 200 %, animations off so the dashes stand still between the frames
  compared; each change made at rest, then the canvas area drawn with the base kept against the same frame built from nothing,
  `kept_against_fresh`): Undo, Redo, a scroll by 1 logical pixel, a zoom change, the window 1 pixel wider, the grid on, the handles hidden,
  the keyboard marker moved, the tint in Preferences, the accent and high-contrast notified (a real change cannot be made in the test
  session: the session's settings do not reach libadwaita there; M1 is JOD's look), the dark style, the focus lost and back, animations on
  (a fixed clock) and off, the pointer over a handle, the screen's scale from 1 to 2 and back (Mutter's display configuration from the
  test, in the isolated session at scale 1), the Magnifier's box, Transparent and Opaque on a floating selection, a background color under
  its hole, Paste, Clear Image, a palette color, a width, a fill style and Sharp on a waiting Smooth Heart, a letter typed, Opaque and a
  background color in an open box; and the eight setters of T003, each its own line: the guides, the bands, the view's memory limit (at
  129 %), the preview's (the rectangles in place of the texture, no dialog), the shape's, the text's, Transparent, the picture (New). Every
  one 0 device pixels differ, at scale 1 (`ui-0044`), 2 (`ui-0044-scale2`) and 1.25 (`ui-0044-scale125`). Red as a whole while built: the
  focus case first destroyed the window (a plain window takes the focus now), and a preview limit of 64 bytes dropped the shape with its
  dialog fading in between the two frames (70000 bytes refuses only the texture). Mutants of REQ-0044-03: the line drawn under the base
  killed; the key's five fields (size, part in view, accent, scale and shift, styles) survive: survivor, accepted, equivalent: each change
  that alters one of them also marks a change (zoom and picture changes, the scroll's handler, the scale's resize, the style's redraw), and
  the accent's handler now marks one too (it asked a bare redraw before T004); a manual break (the key without the scale) left the case
  green with the scale changed at rest, which shows it. The fields stay as a guard against a frame GTK asks by itself. Per-task list
  green: ui 17 of 17, core and io 52 of 52, scale 2 6 of 6, 1.25 2 of 2, 1.5 1 of 1.
- [x] T005 Measure and record (REQ-0044-01, -02, -06; AS-1, AS-2, AS-6) - done when the timing case's numbers under the default renderer
  at scales 1, 2 and 1.25 and the Cairo guard's are in the Plan against today's (the scale 1.25 result said to JOD if above about 10 %),
  the ADR and the budget row are written, and, after JOD's "ready", the four cases measured by hand in his real session (as T001b) are
  each at most 5 % (expected 2.5 to 4 %); a case over 5 % stops the work and is reported, the budget unchanged; M1: JOD sees the dashes
  move as before
  Result (2026-11-13): the measurements above (isolated at scales 1, 1.25 and 2, the Cairo guard against T002, JOD's real session, the two
  benchmarks against 1.0.3); every case at most 5 % at scale 1 in isolation (the Free-Form selection 4.8 %, the nearest) and in JOD's real
  session (2.0 to 4.1 %); 1.25 at most 4.4 % (JOD's "about 10 %" not reached); ADR-0009 and the budget row written. M1: JOD, 2026-11-13,
  the real session: "ok. movement visible" (a selection, a text box and a waiting Smooth Heart at rest, the dashes moving as before).
- [x] T006 The Heart's handle drag on a quiet machine (REQ-0044-05; AS-5) - done when, after JOD's "ready" with the load under 0.5, 0043's
  benchmark has run three times and the medians and loads are in the Plan; a median over a refresh period is reported to JOD
  Result (2026-11-13): JOD's real session, scale 1.25, `perf-0043` of the change, three runs, each started at a 1-minute load under 0.5
  (0.34, 0.47, 0.48; the first attempt refused at once: JOD's M1 window was still open). The fitted 1920 x 1080 Smooth Heart's handle drag,
  medians 17.3, 18.8 and 17.7 ms (90th 33.1 to 36.6, worst 33.4 to 48.7; put-down 21 to 22 ms): **over one refresh period (16.7 ms) in all
  three: a finding for JOD**, not fixed in this spec (REQ-0044-05). Settled (JOD, 2026-11-13): accepted as a known case. Median 17.3 / 18.8
  / 17.7 ms, 90th percentile 33.1 / 36.6 / 33.1 ms, worst 33.4 / 48.7 / 33.6 ms: about one frame in ten is dropped in a handle drag of a
  fitted 1920 x 1080 Smooth shape; not caused by 0044 (1.0.3 reads the same); no new spec now (`docs/phase2-plan.md`, the Heart item). The same case read 16.7 to 16.8 ms on 2026-11-12 (0043's T007) and
  16.8 to 17.8 (1.0.3) against 17.4 to 17.8 (this change) under load at T005: it sits on the edge of a frame, the change adds nothing
  measurable to it (T005's table).
- [x] T007 Gate - done when one `tools/mutate.sh --core`, `spec.sh lint` and `--names 0044`, the per-task list, the map by the names as built,
  and the spec and the INDEX marked implemented
  Result (2026-11-13): `tools/mutate.sh --core`: 626 mutants, 622 killed, 0 killed by crash, 0 inconclusive, 3 survived, 0 errors, 1
  accepted (214 s); the survivors the three accepted before (REQ-0037-02, REQ-0038-03, REQ-0041-06). `spec.sh lint` and `--names 0044` ok;
  `--lint` 1647, 0 stale (REQ-0014-17's accent mutant repointed to `canvas_redraw`, its 25 killed). Per-task list: ui 17 of 17 (`ui-0044`
  and every suite that compares screen captures), core and io 52 of 52, 1.25 2 of 2 (with `ui-0044-scale125`), 1.5 1 of 1; scale 2 5 of 6
  at first (`ui-0044-scale2` killed after the isolated display went away, no assertion of ours: a flake, logged in the INDEX), then 6 of 6
  twice; warning-free. The map by the names as built; implemented here and in the INDEX.

## Check
