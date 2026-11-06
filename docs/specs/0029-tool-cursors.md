# 0029 Tool cursors in one style

Status: accepted
Reviewed: spec-reviewer, 2026-11-06
Depends on: 0001, 0004, 0005, 0006, 0007, 0010, 0012, 0013, 0014, 0015, 0017

## Why
Easel draws five cursors of its own (Pencil, Airbrush, Fill, Pick Color, Magnifier). They are larger than the desktop's and colored, next to the desktop theme's grey cursors that the other tools show, and two of them (Fill, Airbrush) are hard to read (JOD named Airbrush, Pencil, Fill and Pick Color on 2026-11-05, and all five on 2026-11-06 from the contact sheet `docs/reference/0029/cursors-before.png`). Nine toolbox icons are thin next to the filled ones, and two (Fill, Gradient) are unclear (`docs/reference/0029/icons-before.png`). This feature gives both one look.

## Requirements
### Cursors
- REQ-0029-01 While the pointer is over the canvas area with Pencil, Airbrush, Fill, Pick Color or the Magnifier active, the app shall show that tool's own cursor, drawn in the look of the desktop cursor theme's cursors (reference: the theme's crosshair, move and open hand on `docs/reference/0029/cursors-before.png`): a dark grey figure, a white outline all round it 1 pixel wide at the cursor size 24 and wider in proportion at larger sizes, the theme's soft shadow below and to the right, and no other color. The figures: Pencil a pencil; Airbrush a spray can with its spray; Fill a tipped paint bucket with paint spilling from it; Pick Color a dropper; the Magnifier a magnifying glass. JOD approves the drawn five on a new sheet beside the theme's cursors before the app uses them; the approved sheet is kept as `docs/reference/0029/cursors.png` and is the reference from then on. This changes the look of REQ-0001-10, REQ-0006-16 (Airbrush) and REQ-0007-12, not which cursor a tool shows.
- REQ-0029-02 Each cursor's hotspot shall be the point the tool acts on: the Pencil's tip, the center of the Airbrush's spray, the bucket's spilling tip (as REQ-0007-12), the dropper's tip, the center of the glass's lens. A press acts on the picture pixel under the hotspot, as for every tool; the cursor is not part of the picture and never changes a picture pixel.
- REQ-0029-03 The five cursors shall be the desktop's cursor size (Settings, Accessibility, Cursor Size: 24, 32, 48, 64 or 96; default 24) in logical pixels, never smaller than 24 when a smaller size is set by hand, and drawn from artwork at the device resolution at every integer screen scale (never a scale-1 image enlarged), with the same figure and outline. The hotspot sits at the same fraction of the cursor's width and height at every size, rounded to the nearest pixel. When the cursor size changes while Easel runs, the cursor over the canvas takes the new size at the latest on the next pointer motion, as the theme's cursors do.
- REQ-0029-04 Unchanged by this feature, and so stated: the Brush and Eraser outline cursors (REQ-0006-16); the theme's cursors the other tools and states show: Pan's hands (REQ-0001-10), the crosshair (REQ-0010-15, REQ-0011-03, REQ-0012-02, REQ-0013-03, REQ-0014-03, REQ-0017-01), move, the text I-beam, the resize cursors (REQ-0004-17, REQ-0015-02) and the busy cursor (REQ-0005-02); and which cursor wins over a handle, a marker, a selection or a text box (REQ-0004-17, REQ-0012-13, REQ-0013-03, REQ-0015-02, REQ-0017-01), and the busy cursor over all. When the tool changes while the pointer rests on the canvas (a key, or the return after Pick Color, REQ-0007-09), the new tool's cursor shows at once, with no pointer motion, as now.

### Toolbox icons
- REQ-0029-05 The icons of Select, Free-Form Select, Magnifier, Line, Curve, Rectangle, Rounded Rectangle, Ellipse and Polygon shall have the weight of the filled toolbox icons (Pencil, Brush, Eraser): rendered at 16 pixels, every line is at least 2 pixels wide and the figure spans at least 12 of the 16 pixels on its longer side. Select keeps a dashed rectangle and Free-Form Select a dashed loop; Rectangle and Rounded Rectangle stay told apart at 16 pixels by corners at least 3 pixels round.
- REQ-0029-06 The Fill icon shall show a tipped paint bucket with paint spilling from it; the Gradient icon a square going from solid at its smaller x to empty at its larger x in steps of one color, with no partial transparency; each in the weight of REQ-0029-05.
- REQ-0029-07 Every toolbox icon stays a symbolic icon: one color, the text color of the theme, in light and dark alike, as today.

## Scenarios
- AS-1 (REQ-0029-01, REQ-0029-02) Given each of the five tools active in turn, When the pointer is over the canvas area, inside the picture and outside it, Then the cursor is that tool's own and its hotspot is the point REQ-0029-02 names, inside the cursor's image | Verified by: automated (ui, the cursor and hotspot per tool) + manual M1 (the new sheet beside the theme's cursors, light and dark, scale 1 and 2, sizes 24 and 48, approved by JOD before any code uses it)
- AS-2 (REQ-0029-02) Given Pencil at 800 %, When the user clicks with the pencil's tip on the middle of a picture pixel, Then exactly that pixel takes the color; the same for Fill's spilling tip on a one-pixel area and Pick Color's tip on a one-pixel color | Verified by: manual M2
- AS-3 (REQ-0029-03) Given the cursor size set to 48, When Fill is active over the canvas, Then the cursor is 48 logical pixels (96 device pixels at scale 2) and its hotspot is at the same fraction of its size as at 24; Given the size set to 16 by hand, Then it is 24 | Verified by: automated (ui, the size from the setting; the scale 2 run) + manual M2 (a size changed in Settings while Easel runs, the pointer moved once)
- AS-4 (REQ-0029-04) Given Brush, Eraser, Pan, Select, Free-Form Select, Line, Gradient and Text active in turn, When the pointer is over the canvas area, and with Pencil over a picture handle and during a file load, Then the cursors are those of before this feature; When the tool changes by a key with the pointer at rest, Then the new cursor shows without a motion | Verified by: automated (the existing cursor cases of 0001, 0004, 0005, 0006, 0007, 0010, 0012, 0013, 0014, 0015 and 0017 stay green; a new case for the change at rest)
- AS-5 (REQ-0029-05, REQ-0029-06, REQ-0029-07) Given the eleven icons, When each is rendered at 16 pixels in the light and the dark theme, Then each figure spans at least 12 pixels on its longer side, no line is thinner than 2 pixels, each is one color with no partial transparency except at its edges, Select's and Free-Form Select's outlines have gaps, and Rounded Rectangle's corners are at least 3 pixels round | Verified by: automated (a render of each icon at 16 pixels) + manual M1 (the icon sheet, light and dark, 16 and 32 pixels, the bucket and the gradient judged by JOD)

## Edge cases
- A cursor theme other than Yaru or Adwaita (a white or colored theme): Easel's five stay dark grey with a white outline; they do not take the theme's colors.
- A cursor size above 96, or one set by hand between the offered ones (for example 40): drawn at that size. Below 24: drawn at 24 (REQ-0029-03).
- A fractional screen scale (125 %, 150 %, 175 %): the artwork of the next integer scale, shown by the desktop at the fractional scale; looked at in the fractional-scale check named in `docs/phase2-plan.md`, not a test of this feature.
- The pointer over a handle, a marker, a selection or an open text box, or a file loading: REQ-0029-04's order, unchanged.

## Out of scope
- The theme's own cursors (crosshair, move, hands, I-beam, resize, busy) and the Brush and Eraser outlines.
- Icons not in REQ-0029-05 and REQ-0029-06 (Pan, Eraser, Pick Color, Pencil, Brush, Airbrush, Text), and the app's own icon.
- Brush options that read at a glance (its own Phase 2 feature).

## Open questions

## Plan

## Tasks

## Check
