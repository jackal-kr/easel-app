# 0003 The pixel contract

**Status:** Accepted (2026-10-28)

**Context:**
P4 said "hard-edged pixels by default; at 100% zoom 1 image pixel = 1 device pixel". That is too narrow. Two bugs of the Text tool
(0017), both found by JOD at 800 % on 2026-10-27, broke rules nothing stated: the same letter had different shapes in one line
(glyphs at fractional x positions and a threshold on grey coverage; the probe found 17 % of repeated letters identical), and the
grid was hidden inside the text box (the box's own pixels were drawn over it). The rules exist in pieces (P4, REQ-0017-16,
REQ-0004-12, ADR-0002) and in JOD's reference apps (XP Paint, JS Paint, Dibuja with Antialias off): the picture is a grid of squares,
tools write whole pixels, the view magnifies them and draws on top.

**Decision:**
Add principle P7, "The pixel contract" (text in `docs/constitution.md`), six points: exact whole-pixel writes; a view that only
magnifies; previews, floats and boxes that are the put-down's pixels made by the same code; the same input giving the same pixels;
overlays in screen space that never change a picture pixel; integer picture coordinates. P4 stays and keeps the default (hard edges,
smoothing opt-in); its 1:1 sentence is now point 2. Each drawing feature's spec states how it meets points 1 to 4 (a line in
/sdd-spec). The points get tests where they can fail (the small spec "pixel-contract tests").

**Consequences:**
- Text is drawn as 1-bit glyphs on the grid (antialias none, hint full, metrics on, whole-pixel positions): a change of 0017, the
  first application (REQ-0017-28).
- The text box and the floating selection are drawn in the order picture, fill, content, grid, tint, highlight, outline, handles,
  caret (REQ-0017-05, REQ-0004-12): the grid shows over everything inside the picture.
- Smooth text, lines, curves, shapes and brushes (Phase 2) stay opt-in and write whole pixels (partial alpha is a colour, not a
  sub-pixel position); they must still satisfy points 2 to 6.
- Costs: a test per drawing feature for points 3 and 4 (a two-zoom comparison), and a HiDPI check for point 2 that the project cannot
  run yet (no scale-2 screen; open since 0001, where the zoom is zoom x zoom logical pixels and fractional device sizes must be
  rounded the same way for every cell).
- Not covered: how colors are chosen, blending rules (ADR-0002 and each spec), file formats.
