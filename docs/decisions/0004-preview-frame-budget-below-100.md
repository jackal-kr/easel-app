# 0004 A frame budget for previews below 100 %

**Status:** Accepted (2026-11-08, JOD: the budget of spec 0031)

**Context:**
`docs/engineering.md` changes a budget number only by ADR. Below 100 % a waiting gradient cost the picture's size per frame: 375 to 379 ms
a frame at Fit to Window on 8000 × 8000, and 448 MB more held while it waited (`docs/graphics-path-audit.md`, perf-0012). Gradient types
(Radial, Reflected) cost more per pixel. Spec 0031 builds the view below 100 % one device pixel each, so that a preview costs the screen's
size, and JOD set its budget at two frames.

**Decision:**
A new row in the performance budgets: a gradient waiting or a selection moved below 100 %, at Fit to Window on 8000 × 8000 in a 1920 × 1080
window, pointer event to painted frame, median at most 33 ms; the memory the view holds for it at most 8 bytes per device pixel shown.
Measured by perf-0012 and perf-0013 (Fit to Window, 1920 × 1080, 2000 × 2000 and 8000 × 8000), three runs, in the real session.

**Consequences:**
Measured after 0031 (2026-11-08, three rounds interleaved with a build of `843562e`, load 7.8 falling to 3.0): the gradient 16.6 to 16.7 ms
at 8000 × 8000 (375 to 379 ms before), 16.6 to 16.9 ms at 2000 × 2000 (33.5 to 34.7 ms before), with no memory added; the selection one
frame before and after (16.4 to 17.0 ms), its worst frame 26 to 32 ms (19 ms before). Gradient types are measured against this row.
