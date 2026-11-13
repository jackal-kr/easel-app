# ADR-0009 Idle with moving dashes: at most 5 % of a core

Status: accepted (JOD, 2026-11-13; the real-session confirmation of T005 below)
Feature: 0044 Idle redraw cost

## Context
While a selection, an open text box or a waiting shape shows its moving dashes, Easel used to draw its whole canvas again 12 times a
second at rest. Measured on 2026-11-13: in JOD's real session (Vulkan on the GPU, scale 1.25) a waiting 1920 x 1080 Smooth Heart took
24.7 % of a core and a marked selection 2.4 %; in the isolated test session with the Cairo renderer forced, as the ui tests run, the same
cases took 32.5 % and 12.5 %, of which about 10 % is the Cairo renderer's own frame, in software, whatever the canvas draws (0044's T001
and T001b). The budget table said "Idle: 0% CPU when nothing is happening", which the moving dashes, by design (REQ-0014-17 (c)), cannot
meet.

## Decision
- A new budget row: **Idle with moving dashes: at most 5 % of a core**, at rest, animations on, the dashes at 12 steps a second, measured
  with GTK's default renderer: in the isolated test session (`/0044-open/idle_cpu`, which picks Vulkan on the GPU there) on 0044's cases
  at scale 1, and confirmed once by hand in JOD's real session (T005). At scales 2 and 1.25 the numbers are recorded, not bound.
- With animations off the "Idle: 0%" row holds, read as at most 50 ms of CPU time in 10 s.
- With the Cairo renderer forced the numbers are a regression guard only: no case worse than the same-day baseline measured before 0044
  (T002), no bound.
- The deterministic check in the normal suite is a count, not a time: at rest nothing under the moving line is built again
  (`/0044/as7_no_base_build_at_rest`).

## Consequences
- Measured after 0044 (isolated, default renderer, scale 1): 1.6 to 4.8 % in every case; the Free-Form selection of 400 corners is the
  nearest the bound (4.8 %; 5.4 % at scale 2, recorded).
- A change that alters what lies under the moving line must mark it (`canvas_redraw`), or the kept frame shows the state before it:
  AS-3 of 0044 lists the changes and compares each kept frame with one built from nothing.

## Real-session confirmation (T005, 2026-11-13)
JOD's real session after his "ready", scale 1.25, `GskVulkanRenderer` (the AMD Radeon 860M), the window as it opens (1152 x 720), Easel's
own process from outside (`top -b -d 1`, 30 samples after half a second at rest), three runs, load 0.11 to 1.01:

| Case | Before (T001b) | After |
|---|---|---|
| 1920 x 1080 Smooth Heart at Fit | 24.6 / 24.7 / 24.8 % | 2.5 / 2.6 / 2.5 % |
| 1920 x 1080 Sharp Heart at Fit | 5.1 / 5.2 / 5.3 % | 4.1 / 3.9 / 4.0 % |
| 1920 x 1080 marked selection at Fit | 2.4 / 2.4 / 2.5 % | 2.5 / 2.4 / 2.5 % |
| 200 x 150 Smooth Heart at Fit | 7.2 / 7.2 / 7.4 % | 2.0 / 2.0 / 2.0 % |

Every case at most 5 %: the bound holds in the real session.
