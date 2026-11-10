# Constitution

Non-negotiable principles. Specs, plans and code are checked against this file (`/sdd-build` checks
the plan against it). Changing a principle needs an ADR in `docs/decisions/`. JOD owns this file; edit freely.

## Product
- **P1** XP Paint's functionality and workflow, GNOME-native look (GTK 4 + libadwaita). No XP visuals.
- **P2** XP's hidden behaviors are features (`docs/reference/xp-behaviors.md`).
- **P3** No XP-era limits (e.g. undo depth is bounded by a memory budget only).
- **P4** Hard-edged pixels by default (no antialiasing). Smoothing is an opt-in option for text, lines, curves, shapes and brushes; the Pencil, the Eraser and Fill are always exact.
- **P5** Keep it simple: no threads, caches or fancy rendering until a measurement justifies them.
- **P6** Every user-adjustable default is a setting in a configuration file, not a constant in code; the Preferences dialog edits the same settings.
- **P7 The pixel contract** (ADR-0003).
  1. Every tool writes whole picture pixels with exact colors. Smoothing, where it exists, is opt-in and still writes whole pixels.
  2. The view only magnifies: one picture pixel is exactly zoom/100 device pixels on a side, on any screen, at every scroll position and window size. At a fractional zoom every edge is one position that depends only on k and the zoom: no gaps, no overlaps, and no cell that moves when the view scrolls. Below 100 % each device pixel shows the one picture pixel under its center, by a rule that depends only on the zoom (JOD, 2026-11-08, feature 0031).
  3. A preview, a floating selection and a text box show exactly the pixels the put-down writes, made by the same code. Exception: Remove background's live preview on a picture or selection over about 2 million pixels is made on a smaller copy, so its edge may differ from the applied result's, which is refined at full size (ADR-0007, JOD, 2026-11-09).
  4. The same input gives the same pixels: a letter is the same bitmap wherever it falls in a line, and a shape's pixels depend only on its integer coordinates, not on the zoom, the scroll or the screen scale.
  5. Overlays (outline, handles, caret, grid, tint) are drawn in screen space and never change a picture pixel. The tint and the grid let the pixels show through; the outline, the handles and the caret cover pixels only with thin lines and small marks.
  6. Coordinates are integer picture pixels, so a drag from A to B makes the same figure at any zoom.
  7. Every operation that makes a pixel fully transparent writes 0x00000000 (red, green, blue and alpha all 0); Fill treats every
     fully transparent pixel as one color; saving writes the pixels as they are, and opening reads a fully transparent pixel as 0,
     as it has since feature 0005 (ADR-0008, JOD, 2026-11-10).

## Process
- **S1** Specs are the source of truth; code is derived. When code and spec disagree, one is wrong: decide which, fix the spec first.
- **S2** No implementation without an accepted spec and its agreed plan and tasks. Exempt: typos, formatting, comment-only edits.
- **S3** Every requirement has an ID (`REQ-NNNN-xx`) and at least one scenario (`AS-n`, Given/When/Then);
  every scenario names how it is verified: an automated test or a numbered manual check.
- **S4** Specs describe observable behavior (what/why). Technology, files and structures belong in the plan.
- **S5** Ambiguity is a spec bug: stop, fix the spec, log it in `docs/learning-log.md`.
- **S6** Specs derive from observable behavior (XP Paint, jspaint, JOD's descriptions), never from the source code of an existing
  implementation. This is what makes spec gaps visible. Relax only by ADR.
- **S7** Small slices; after each task the build is warning-free and tests pass.
- **S8** Git: never commit or push unless asked; no AI co-author trailers.
- **S9** Honesty: say plainly where AI wrote code and what JOD has reviewed; no fake review traces.

## Technical (ADR-0001, accepted)
- C, GTK 4, libadwaita >= 1.8, Cairo, Meson/Ninja, Ubuntu 26.04. License GPL-3.0-or-later.
- Layering: `src/core` pure C (libc only) and headless-testable; `src/ui` turns input into core calls.
- Pixels are `uint32_t` 0xAARRGGBB with straight (non-premultiplied) alpha, so stored colors stay exact and PNG round trips are
  lossless; conversion to what GDK or Cairo expect happens only at the edges that need it (ADR-0002).
- Warnings are bugs. Tests ship with the change; a bug fix starts with a failing test.
- Details live in `docs/engineering.md` (coding standards, architecture, testing, performance budgets) and `docs/design.md` (UI and interaction).
