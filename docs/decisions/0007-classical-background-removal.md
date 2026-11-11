# 0007 Remove background by a classical method, no model

**Status:** Accepted (2026-11-09, JOD: option A of `docs/reference/0035/methods-comparison.md`, "option A only; simple"). Supersedes
ADR-0006.

**Context:**
ADR-0006 chose a learned model (IS-Net) for 0035. Its weights carry no stated licence and its training data (DIS5K) is offered "for
non-commercial use in research or educational purpose"; the permissively tagged models that fit the budgets are trained for people
or name DIS5K too, and the one general model that is clearly permissive (BEN2 Base) needs about nine seconds and 8 GB; every model
adds ONNX Runtime, a 180 to 220 MB file, a Flatpak module and an exception to P7.4 (`docs/reference/0035/model-licences.md`,
`methods-comparison.md`, `methods-sheet.png` (kept locally, not in the repository: it shows third-party wallpapers)). A classical prototype (`docs/reference/0035/classical/`) took 0.03 s at 1920 x 1080
and 0.74 s at 8000 x 8000, passed five of the six test pictures, and gives the same pixels on every machine.

**Decision** (revised the same day after JOD's look at the improved prototype, `docs/reference/0035/methods-comparison.md`: stage 4 of
`classical-v2.c`, built as `classical-v3.c`):
1. Remove Background finds the background without a model, in `src/core`, libc and integers only: on a working copy of at most about
   2 million pixels (block means), colors in an integer Lab approximation; the background as a cubic surface per Lab channel fitted to
   the copy's border ring by fixed-point least squares (2026-11-10: a robust fit, three rounds without the samples beyond three times
   the median error, fitted once, and a colour line from black through the ring's median color for a glow or a vignette, used where it
   explains the ring at least as well as the surface and for colored cells only; and up to six color clusters of the ring, used
   where they explain it better than the surface, for a backdrop of several colors, stripes or a horizon; v6, the same day); a marker-based
   watershed on the Lab edge strength, first in first out (background seeds in the ring near the surface, subject seeds at least four
   tolerances from it), then the seam: a subject cell beside background within the tolerance of the surface becomes background (a
   step's two sides have the same edge strength); the largest subject region kept with any other at least a tenth of its size; enclosed background under 1 % of
   the picture filled; a hard mask; at full size the boundary decided by the nearest neighbouring copy cell in Lab; then the fringe
   (2026-11-10, JOD's eagle photo): boundary pixels that are blends of the background beside them and the subject inside them, the
   background's share above one half, removed in one round. Judged in RGB, where a soft
   edge's mix is a straight line; the colors taken where they have settled, so that a hard edge's subject is its own color and thin
   lines stay (`docs/reference/0035/fringe-sheet.png` (kept locally, not in the repository: it shows third-party wallpapers)).
2. (2026-11-10 note: a card in the canvas area's corner, not a dialog; the automatic tolerance from the clusters' median where they judge, else the surface's, then the stability check; the hint from the ring's worst tenth; see spec 0035, REQ-0035-04.) A small dialog with one Tolerance value that starts at an automatic one (from the ring's median distance to the surface: 5 while it
   is at most 3.05 delta-E, then 3 more per delta-E, at most 20; re-tuned 2026-11-10 when the integer Lab's cube root was found wrong), and a live preview made on the copy, OK and Cancel. Applying keeps 0035's
   worker thread, its progress dialog after 0.5 s and Cancel for large pictures.
3. No model, no ONNX Runtime, no `src/ml`, no build option. ADR-0006's exception to P7.4 is removed: the same picture and Tolerance give
   the same pixels on every machine. P7.3 gains one: on a picture or selection over about 2 million pixels the preview, made on the
   copy, may differ along the edge from the applied result, refined at full size (JOD, 2026-11-09).
4. Budgets: the preview within 0.3 s at any size; the result within 1 s at 1920 x 1080 and 2 s at 8000 x 8000, with at most 1 GB beyond
   the picture and its undo step (measured on the prototype: 47 ms and 0.71 s; about 80 MB at 8000 x 8000).
5. Its limits are documented, not hidden: it removes a background that differs in color from the subject and does not recognise
   subjects; a pale subject on a pale background and soft edges need a lower tolerance; line art on a smooth background is filled in.
   "Keep this" and "Remove this" brushes that would add seeds are an optional later item (`docs/phase2-plan.md` G). GrabCut and other
   iterated graph cuts with color models are not used (a patent is believed to cover them, status unchecked).
   Patent note (2026-11-10, JOD's request; a note, not legal advice): US 7,660,463 B2, "Foreground extraction using iterated graph
   cuts" (Microsoft; filed 2004-06-03, granted 2010-02-09), is listed by Google Patents as "Expired - Fee Related": the year-12
   maintenance fee was not paid, "Lapsed due to failure to pay maintenance fee", effective 2022-02-09 (the patent's adjusted expiration
   would otherwise have been 2026-03-10). Source: https://patents.google.com/patent/US7660463, whose status is its own assumption, not a
   legal conclusion. Related patents (continuations, other owners' graph-cut methods) were not searched. The rule above stands until JOD
   decides otherwise.

**Consequences:**
`docs/reference/0035/` keeps the licence survey, the sheets, the comparison and the prototype as a record only. Spec 0035 is revised in
place (it was not released). ADR-0006 stays, marked superseded, for the record.
