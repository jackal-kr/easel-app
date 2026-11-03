# 0007 Fill, Pick Color and Magnifier

Status: implemented
Depends on: 0001, 0002, 0003, 0004, 0006

## Why
The last three tools of the first drawing milestone. Fill paints a whole area in one click, Pick Color takes a color from the
picture instead of the palette, and the Magnifier zooms to a chosen level with a click, the way XP Paint does. With them the
app is a usable paint program (milestone M1).

## Requirements
- REQ-0007-01 The toolbox shall show Magnifier, Pick Color and Fill with the tooltips "Magnifier", "Pick Color" and "Fill", in the final order of REQ-0006-01, so that the toolbox is, in two columns, row by row: Pan, Magnifier / Eraser, Pick Color / Fill, Pencil / Brush, Airbrush. Clicking one makes it the active tool, as for the other tools (REQ-0001-09). (0012 adds Gradient after Fill, which moves Pencil and the tools after it one place on: REQ-0012-01.)
- REQ-0007-02 (Pointer note, Preferences, 2026-11-03: the start-up level comes from `[drawing] magnifier`, REQ-0020-05.) While the Magnifier is active, the options panel (REQ-0006-02) shall show six toggle buttons labelled "1x", "2x", "4x", "6x", "8x" and "16x" in two columns, row by row (1x 2x / 4x 6x / 8x 16x), one selected at a time, with the tooltips "100 %", "200 %", "400 %", "600 %", "800 %" and "1600 %". The selected level is 2x at start-up and each later time the app starts, and the choice is kept for the rest of the session when the user switches tools and back (as REQ-0006-03: it is not a setting in this feature; Preferences will make it one). Choosing a level only selects it: the zoom does not change. Pick Color and Fill have no options: the panel and its line are hidden for them, as for Pan and Pencil (this amends REQ-0006-02, which names only Pan and Pencil). (Amended by 0008, REQ-0008-02, 2026-10-18: five levels, no reductions; 16x added, JOD, 2026-10-27.)
- REQ-0007-03 When the user presses the primary or the secondary button on a picture pixel with the Magnifier, the app shall set the zoom to the selected level, unless the zoom already is that level, in which case it sets the zoom to 100 % (when the view is already at the selected level, the press returns to 100 %, at every level: 8x at 800 %, 16x at 1600 %) (so with 1x, a press at any other zoom returns to 100 % and a press at 100 % changes nothing; at a zoom below 100 %, reached by a Zoom Out step or by Fit to Window, a press gives the selected level, and with 1x it gives 100 %). (Amended by 0008, REQ-0008-02, 2026-10-18: five levels, no reductions; 16x added, JOD, 2026-10-27.) The zoom happens at the press: moving the pointer or pressing the other button while it is held does nothing more. The middle button does nothing, and the Magnifier stays the active tool. A press on a resize handle starts a resize (REQ-0004-16). A press outside the picture does nothing (the grey area around a small picture does not zoom). The point of the picture that is the focus (below) shall be at the center of the canvas area as far as the view's edges allow (as REQ-0004-06); a picture smaller than the canvas area stays at its top-left (REQ-0001-11). The focus is the pixel under the pointer, except at 100 % with the preview box shown (REQ-0007-04), where it is the center of that box; where that box touches an edge of the picture, the view goes to that edge (so a press near a corner shows the corner exactly, whether or not a scrollbar appears with the new zoom). (0009: the focus scroll counts the margin of REQ-0009-01.)
- REQ-0007-04 While the Magnifier is active, the zoom is 100 % and the pointer is over a picture pixel, the app shall show a box over the picture: the part of the picture that a press would make fill the canvas area, that is, the canvas area's width and height in device pixels (no scrollbars counted, as REQ-0004-07) divided by the selected level (as a whole number of picture pixels, rounded down, at least 1), with its top-left corner at the pointer's pixel minus half its size (rounded down) and moved as far as needed to lie inside the picture; where the picture is smaller than the box, the box spans the whole picture in that direction. It is drawn as the cursors of REQ-0006-16 are, a black line with a white line beside it, so that it shows on any color. At any other zoom, and with 1x selected (a press would not zoom in), no box is shown (amended by 0008, REQ-0008-02: there is no reduction level); when the zoom becomes 100 % again (by a press or by any zoom command) with the pointer over a picture pixel, the box is shown at once. The center of the box, as placed, is the focus of REQ-0007-03.
- REQ-0007-05 When the user presses the primary button on a picture pixel with Fill, the app shall set to the foreground color every pixel that is connected to it through pixels equal to it in all four channels (connected by edges, not by corners: a diagonal line of different pixels stops the fill), and with the secondary button to the background color. The new value is exactly the color, with no blending (REQ-0003-08), also when it has transparency. The fill happens at the press, is one undo step (REQ-0002-01) and marks the picture modified; the pointer moving or the buttons being held or released afterwards does nothing more.
- REQ-0007-06 If the pixel pressed already has exactly the color to fill with (all four channels), then Fill shall change nothing: no pixel, no undo step, no modified mark. A press outside the picture does nothing and the middle button does nothing.
- REQ-0007-07 If the memory for the fill or for its undo step cannot be had, then the app shall show an error dialog with the heading "Could Not Fill" and the body "There is not enough memory to fill this area.", and change nothing: neither the picture nor the history. The fill's undo step costs at most 16 bytes per changed pixel plus 64 (REQ-0002-16); one that is larger than the whole budget is kept as the only step (REQ-0002-17). A fill is not interruptible; how long a whole-picture fill takes is measured in the Plan.
- REQ-0007-08 When the user presses the primary button on a picture pixel with Pick Color, the app shall make that pixel's color, all four channels exactly, the foreground color, and with the secondary button the background color. While the button is held and the pointer moves, the color shall follow the pixel under the pointer (the color box shows it at once); while the pointer is outside the picture the color of the last pixel it was over stays. A press outside the picture does nothing. The middle button does nothing.
- REQ-0007-09 When the user releases the button after a pick, the app shall make the tool that was active when Pick Color was chosen the active tool again (Pan, if Pick Color was chosen right after start-up; the Magnifier, if that was it), and the options panel, the cursor and the hint follow at once. If nothing was picked (the press was outside the picture), Pick Color stays active. Clicking the Pick Color button while it is active changes nothing, so the tool to return to stays the same.
- REQ-0007-10 If the user presses the other mouse button or presses Undo during a pick, then the app shall cancel it (REQ-0001-23, REQ-0002-09): the colors are as they were before the press, Pick Color stays active, further Undo and Redo are ignored until the button is released, and the release changes nothing. If the window loses focus, the button is released outside the window, or Save or Quit is asked during a pick, then the pick ends and keeps its color, as a stroke does (REQ-0001-25, REQ-0001-33), and the tool returns as in REQ-0007-09.
- REQ-0007-11 Picking a color, choosing a Magnifier level and zooming with the Magnifier shall not change the picture, add an undo step or change the modified mark (adds to REQ-0002-02 and REQ-0003-09).
- REQ-0007-12 While the pointer is over the canvas area with Fill, Pick Color or the Magnifier active, the cursor shall be a paint bucket, a dropper or a magnifying glass; the pixel the tool acts on is the one under the bucket's spilling tip, the dropper's tip and the glass's lens center. Over a resize handle the resize cursor wins (REQ-0004-17), and the handles behave as for the Pencil (REQ-0004-16).
- REQ-0007-13 The status bar hint (REQ-0004-09) shall be, for Fill: "Fills an area with the current drawing color."; for Pick Color: "Picks up a color from the picture for drawing."; for the Magnifier: "Changes the magnification."
- REQ-0007-14 Numpad + and − (this amends REQ-0006-14, which names only Pan and Pencil) shall do nothing with Fill, Pick Color and the Magnifier; Ctrl+Numpad + and − keep zooming (REQ-0004-04). (Pointer note, Text, 2026-10-26: While a text box is open, Numpad + and − type characters into it, REQ-0017-06.)
- REQ-0007-16 While a pick is in progress or a resize drag is, choosing another tool shall be ignored (adds to REQ-0001-09, which names only strokes).
- REQ-0007-15 A press with Fill, Pick Color or the Magnifier is ignored, as a stroke is (REQ-0001-39, REQ-0002-09 and the dialogs of REQ-0004-03), before the picture exists and while a dialog, the quit prompt or the color chooser is open or a resize drag is in progress.
- REQ-0007-18 If the pointer moves, or a button is pressed, while the window sees no mouse button down, then a press that is still held in the app (a Fill, Pick Color or Magnifier press, a stroke) shall end as if its button had been released: the release did not reach the window (a menu or a popover took it), and the tools and the options panel must not stay blocked.
- REQ-0007-17 A zoom by the Magnifier shall take effect at once, within the next frame drawn, with no animation, and every tool shall keep acting on the picture pixel under the pointer at the new zoom (REQ-0001-19).

## Scenarios
- AS-1 (REQ-0007-01, REQ-0007-02, REQ-0007-12, REQ-0007-13) Given the app just started, Then the toolbox shows Pan, Magnifier / Eraser, Pick Color / Fill, Pencil / Brush, Airbrush with their tooltips; When the user clicks Magnifier, Then the options panel shows 1x 2x (selected) / 4x 6x / 8x 16x with the tooltips "100 %" to "1600 %" (amended by 0008, REQ-0008-02), the status bar says "Changes the magnification." and the cursor is a magnifying glass; When the user clicks Fill, Then the panel and its line are gone, the hint is "Fills an area with the current drawing color." and the cursor is a paint bucket; Pick Color: hint "Picks up a color from the picture for drawing.", cursor a dropper | Verified by: automated (ui, toolbox, panel, hints) + manual M1 (the cursors)
- AS-2 (REQ-0007-02) Given the Magnifier with 2x chosen at 100 %, When the user clicks the 8x option, Then the zoom is still 100 % and 8x is selected; the panel has 1x 2x / 4x 6x / 8x 16x (amended by 0008, REQ-0008-02); When the user switches to Pencil and back, Then 8x is still selected | Verified by: automated (ui)
- AS-3 (REQ-0007-03) Given a 3000 × 2000 picture at 100 %, the Magnifier with 2x, a canvas area of 1390 × 853 and the pointer at pixel (1000, 700), When the user presses the primary button, Then the zoom is 200 % and picture pixel (1000, 700) is at the center of the canvas area (within one device pixel); When the user presses again (anywhere on the picture), Then the zoom is 100 %; with 4x chosen, a press at 100 % gives 400 %, and a press at 400 % gives 100 %; at 200 % with 4x chosen a press gives 400 %; with 1x chosen a press at 400 % gives 100 %, at 42 % gives 100 %, and at 100 % changes nothing; at 25 % (two Zoom Out steps from 100 %) with 4x chosen a press gives 400 %, and with 1x chosen a press at 25 % gives 100 % (amended by 0008, REQ-0008-02, which removed the reduction levels); at the fit zoom 42 % with 2x chosen a press gives 200 %; the secondary button does the same; a press with the middle button does nothing; the Magnifier stays active, and no pixel, undo step or modified mark changed (REQ-0007-11) | Verified by: automated (core, the focus and the level rule) + automated (ui)
- AS-4 (REQ-0007-03) Given a 3000 × 2000 picture, the Magnifier with 2x, When the user presses at pixel (100, 50) at 100 %, Then the view shows the picture's top-left corner at 200 % (the edges limit it); at pixel (2950, 1980) the view shows the bottom-right corner; Given a 600 × 400 picture, a press zooms to 200 % and the picture, 1200 × 800 device pixels, fits the canvas area, so it stays at its top-left | Verified by: automated (core)
- AS-5 (REQ-0007-04) Given the data of AS-3 at 100 % with 2x, When the pointer is over pixel (1000, 700), Then the box covers picture pixels x 653 to 1347 and y 487 to 912 (695 × 426) and is drawn black and white; with 8x it is 173 × 106; at pixel (0, 0) it covers x 0 to 694 and y 0 to 425; at pixel (0, 0) its center, the focus, is pixel (347, 213); Given a 600 × 400 picture, 2x, Then the box covers the whole picture; at 200 % no box is shown, and none with 1x chosen (amended by 0008, REQ-0008-02); after a press at 200 % returns to 100 % with the pointer still over the picture, the box is back at once | Verified by: automated (core, geometry) + manual M1 (the look)
- AS-6 (REQ-0007-05) Given a picture of a red ring (one pixel wide, closed, 8-connected diagonals included) on white and the foreground blue, When the user presses the primary button inside the ring with Fill, Then every white pixel inside becomes blue and nothing outside changes; the red pixels stay; one Undo restores it exactly; the secondary button fills with the background color; Given a closed ring made of diagonal steps only, Then the fill stays inside (it does not leak through the corners) | Verified by: automated (core) + automated (ui)
- AS-7 (REQ-0007-05) Given a picture with a #00000000 area and a foreground #FF000080, When the user fills it, Then those pixels are exactly #FF000080 (no blending); a pixel #00FF0000 next to #00000000 pixels is not part of the area (all four channels must be equal) | Verified by: automated (core)
- AS-8 (REQ-0007-06) Given a white picture and a white background color, When the user presses the secondary button with Fill, Then no pixel changes, Undo stays insensitive and the title has no modified mark; a press outside the picture does the same | Verified by: automated (core) + automated (ui)
- AS-9 (REQ-0007-07) Given a fill that cannot get its memory, Then the dialog "Could Not Fill" appears with the body of REQ-0007-07 and the picture, the title and Undo are unchanged; Given a 16 MB undo budget and a fill whose undo step is larger than the budget (the worst case for a fill is a region of one-pixel pieces: a 1800 × 1800 picture of one-pixel-wide columns joined by their top row), Then the fill is the only step and one Undo restores the picture | Verified by: automated (core, refusal and budget) + automated (ui, dialog)
- AS-10 (REQ-0007-08, REQ-0007-09, REQ-0007-11) Given Pencil was the active tool, then Pick Color was chosen, foreground black, When the user presses the primary button on a red pixel, Then the foreground is that red at once, and When the user drags to a green pixel, Then it is green; When the pointer leaves the picture, Then it stays green; When the user releases, Then Pencil is the active tool again, the foreground is green, no pixel changed, Undo is insensitive and the title has no modified mark; the secondary button sets the background color | Verified by: automated (ui)
- AS-11 (REQ-0007-08) Given a transparent pixel #00000000, When the user picks it, Then the foreground is exactly #00000000 and the indicator shows a checkerboard; a pixel #80FF8000 gives exactly that color | Verified by: automated (core, ui)
- AS-12 (REQ-0007-08, REQ-0007-09) Given Pick Color active at start-up (Pan was active), When the user presses outside the picture and releases, Then nothing changes and Pick Color stays active; When the user picks a pixel, Then Pan is active again | Verified by: automated (ui)
- AS-13 (REQ-0007-10) Given a pick in progress that changed the foreground from black to red, When the user presses the other mouse button (or Ctrl+Z), Then the foreground is black again, Pick Color is still active, a second Ctrl+Z before the release does nothing, and the release changes nothing; When instead the window loses focus (or the button is released outside the window), Then the foreground stays red and the previous tool is active again | Verified by: automated (ui)
- AS-14 (REQ-0007-14) Given Fill active, When the user presses Numpad +, Then nothing changes; Ctrl+Numpad + zooms in | Verified by: automated (ui)
- AS-15 (REQ-0007-15) Given the color chooser open, a resize drag in progress or a window whose picture does not exist yet, When the user presses with any of the three tools, Then nothing changes | Verified by: automated (ui)
- AS-16 (REQ-0007-16) Given a pick in progress (button held), When the user clicks another tool's button (or presses its shortcut), Then Pick Color stays active; the same during a resize drag; after the release or the end of the drag a tool can be chosen again | Verified by: automated (ui)
- AS-17 (REQ-0007-17) Given the Magnifier at 100 % with 4x, When the user presses on pixel (1000, 700), Then the zoom is 400 % within the next frame; When the user chooses Pencil and presses at the center of the canvas area, Then the pixel painted is the one that was at that point, (1000, 700) within one pixel, and Fill at the same point fills the area of that pixel | Verified by: automated (ui)
- AS-18 (REQ-0007-18) Given a Fill press whose release never arrives, When the window then sees the pointer move with no button down, Then the next press fills again; Given a Pencil stroke whose release never arrives, Then the same move ends the stroke, keeps what it drew, and the options panel is usable again; Given a Pick Color press without its release, Then the move ends the pick and keeps its color | Verified by: automated (ui, with the canvas entry point that the window's event handler calls)

- AS-19 (REQ-0007-03) Given a 3000 × 2000 picture, the Magnifier with 8x at 800 %, When the user presses on the picture, Then the zoom is 100 %; Given 16x at 1600 %, Then a press gives 100 %; Given 16x at 100 %, Then a press gives 1600 %, the pixel under the pointer at the center as far as the edges allow | Verified by: automated (core, ui)
## Edge cases
- A fill of the whole picture (a blank 8000 × 8000): works within the undo budget rule REQ-0002-17, or shows the memory dialog of REQ-0007-07; it is never half done.
- Fill on a one-pixel picture, and on a pixel at each edge or corner: the area does not wrap and has no neighbors outside the picture.
- Fill connects by edges only: a closed ring of pixels that touch each other only by their corners (a diagonal line) keeps the fill inside, and a gap of one pixel in a ring lets it out.
- A press, then a drag, with Fill: nothing more happens; pressing the other button while the first is held does nothing (the fill is already done, there is no stroke to cancel).
- Magnifier: the preview box follows the pointer only while it is over a picture pixel; the zoom keys, the zoom row and the wheel (Pan only) work as before and change the level the next press compares with.
- At 800 % with 8x, and at 1600 % with 16x, a press returns to 100 %; at 1600 % with 2x selected it zooms out to 200 %; with 1x selected a press never zooms in; below 100 % (a Zoom Out step or Fit to Window) any other level zooms in to it, centered on the pixel under the pointer (no preview box); Fit to Window still reaches 10 %, where an 8000 × 8000 picture fits a 1390 × 853 area (amended by 0008, REQ-0008-02).
- Pick Color while the pointer is outside the picture but the button is held: the last color stays (REQ-0007-08); a pick that starts on a resize handle starts a resize instead (REQ-0004-16).
- Choosing a tool during a pick or a resize is ignored (REQ-0007-16).

## Out of scope
- Tolerance, gradient and pattern fills, and fills limited to a selection (Selection adds the selection).
- Picking a color from outside the picture or from other windows; a picked color on the color box's swatches.
- The Magnifier's rectangle zoom by dragging; Magnifier levels beyond 1x, 2x, 4x, 6x, 8x and 16x (the zoom steps keep all the levels of REQ-0001-16; amended by 0008, REQ-0008-02).
- Keyboard-only use of the three tools (Accessibility).
- A setting for the Magnifier's default level (Preferences).

## Open questions
None.

## Plan
### Approach
Fill and the Magnifier's rules are pure functions of pixels and numbers, so they go in the core and are tested without a
window: a scanline flood fill that, because it replaces only pixels equal to the seed in all four channels, records one old
color, one new color and the spans `(y, x0, x1)` it filled, and hands them to the history as one step of a new kind (so a
whole-picture fill costs a few kilobytes, not 12 bytes a pixel, and undo, the budget and "modified" follow the existing
rules), and the Magnifier's rules
(which zoom a press gives, the preview box, the scroll that puts a focus point at the center of the view). The ui adds
three tools to the one tool list, three toolbox buttons with icons and cursors, a fourth group of buttons in the options
panel (1/10x to 8x), and the canvas input: a Fill press runs the core fill; a Magnifier press asks the core for the zoom
and the scroll and uses the existing zoom-commit path of 0004; a Pick Color press is a "pending" gesture that the canvas
treats like a stroke (a held button, cancelled by the other button or Undo, ended by focus loss), so every existing rule for
strokes (tool change ignored, zoom actions ignored, Undo cancels, further Undo ignored until release) applies without a copy.
The picked color travels to the window by a signal, and the window sets the colors and, at the end, returns to the previous
tool. No threads, no new settings (REQ-0007-02: the level is a session choice, as REQ-0006-03).

### Files
- `src/core/fill.{c,h}` (new), `src/core/magnifier.{c,h}` (new), `src/core/view.{c,h}` (one function added), and
  `src/core/history.{c,h}`: a third step kind and one push function (below; about 60 lines in `history.c`, 10 in the header).
- `src/ui/tool.{c,h}` (three tools), `src/ui/window.ui` (toolbox grid), `src/ui/tool-options.{c,h}` (the Magnifier group),
  `src/ui/canvas.{c,h}` (input, the preview box, cursors, the pick gesture, signals), `src/ui/window.{c,h}` (hints, colors,
  return to the previous tool, the memory dialog), `src/ui/easel.gresource.xml`, icons `easel-tool-{magnifier,pick,fill}-symbolic.svg`
  and cursors `cursors/{magnifier,pick,fill}.svg`.
- Tests: `tests/core/test-fill.c`, `tests/core/test-magnifier.c` (new), `tests/ui/test-window.c` (a 0007 block), `tests/perf/perf-0007.c`,
  mutants in `tests/mutants.txt`.

### Key interfaces
- `EaselFillResult easel_fill (EaselImage *img, EaselHistory *history, int x, int y, uint32_t color)`: result `EASEL_FILL_DONE`,
  `EASEL_FILL_NOTHING` (outside, or the pixel already has `color` in all four channels: no step), `EASEL_FILL_NO_MEMORY` (the
  picture is as it was, no step). Returns through an out parameter the number of pixels changed.
- History: `typedef struct { uint32_t y, x0, x1; } EaselSpan;` (x1 inclusive) and `bool easel_history_push_fill (EaselHistory *h,
  uint32_t old_value, uint32_t new_value, EaselSpan *spans, size_t n)`, with the ownership rule of `easel_history_push` (on
  success it takes the spans; false means no memory and the caller keeps them). Undo writes `old_value` into every pixel of
  every span, redo `new_value`, both through `easel_image_set`. A step counts 12 bytes per span plus 64: a span has at least
  one pixel, so this is within REQ-0002-16's 16 bytes per changed pixel, and no existing function or test changes.
- `int easel_magnifier_zoom (int zoom, int level_pct)`: the zoom a press gives (level, or 100 when already at it).
  `void easel_magnifier_box (int area_w, int area_h, int level_pct, int pic_w, int pic_h, int px, int py, int *x, int *y, int *w, int *h)`:
  the preview box of REQ-0007-04 in picture pixels. `double easel_view_focus_scroll (double focus, int pct, int picture_size, double scale, double view_size)`:
  the scroll offset that puts the picture coordinate `focus` at the center of a view, clamped to the content (0 if it fits).
- Canvas: `easel_canvas_set_magnifier_level (int pct)`; signals `color-picked (button, color)` (at the press and at each pixel
  change), `pick-ended (cancelled)`, `fill-failed` (memory). `easel_canvas_is_stroking` is true during a pick.
- Tool options: `easel_tool_options_get_magnifier_level` (200, 400, 600 or 800); `preset-changed` also fires for it.

### How each rule is kept
- **Fill (REQ-0007-05..07):** scanline fill; no visited bitmap, because the new color differs from the target (the no-op case
  returns first), so a filled pixel no longer matches. Work spans wait on a stack and filled spans go to a list, both grown by
  realloc; any failure restores every filled span to the old color and returns `NO_MEMORY`. At the end the list goes to
  `easel_history_push_fill`, which keeps a step that is bigger than the budget (REQ-0002-17) and refuses only for lack of
  memory (then the fill is undone).
- **Magnifier press (REQ-0007-03, -04, -17):** the focus is the box center when the zoom is 100 % and the box is shown, else the
  pixel under the pointer; the new scroll comes from `easel_view_focus_scroll` per axis with the view's page size, and goes through
  the existing `commit_zoom` (applied at the next allocation, as 0004's anchors are). The preview box is drawn by the canvas in
  its snapshot as a black rectangle with a white one outside it; it needs the pixel under the pointer, which the canvas
  now keeps (and clears when the pointer leaves the area).
- **Pick (REQ-0007-08..10, -16):** press stores the button, emits `color-picked`; motion over a different pixel emits it again;
  the other button or Undo cancel through `easel_canvas_cancel_stroke` (which emits `pick-ended (TRUE)`), focus loss and release
  go through `easel_canvas_end_stroke` (`pick-ended (FALSE)`). The window keeps the colors from before the press and restores
  them on a cancel; on an end it activates the tool that was active when Pick Color was chosen (kept in the window).
- **Options, hints, cursors, keys (REQ-0007-02, -12..14):** one more group in `tool-options.c`; `tool_hint` gets three cases;
  cursors are SVG textures like the Pencil's, with the hotspots of REQ-0007-12; `step_preset` names the three tools that have presets.

### What grows
The fill's undo step: 12 bytes per span (y, x0, x1) plus the two colors, kept as the undo step, nothing copied (the history
takes the buffer). A whole blank 8000 × 8000 fill is 8000 spans, 96 KB. The worst case is a region of one-pixel spans, 12 bytes
per pixel, as the old per-pixel log (an 1800 × 1800 comb, every other column joined by its top row, has 1,619,101 spans, 19.4 MB).
The work stack: 16 bytes per waiting span, freed at the end. Nothing else grows: the Magnifier and Pick keep a few integers.

### Measurements
A task adds `perf-0007`: a whole-picture fill of a blank 1920 × 1080 and 8000 × 8000 picture and of the 1800 × 1800 comb
(time, and the peak memory from `/proc/self/status`), a Fill press on a 100-pixel area at 8000 × 8000 to the next painted frame, and a Magnifier press to the
painted frame at both sizes. `docs/engineering.md` has no budget for Fill; the numbers go here and the budget is JOD's to
set. If a whole-picture fill at 8000 × 8000 takes longer than a second, a fix is proposed then (no threads before a decision).

### Scenario-to-test map
| AS | Test |
|---|---|
| 1 | ui `as1_new_tools_toolbox_hints` (order, tooltips, panel, hints) and `as1_cursors_follow_tool` (the cursor changes with the tool) + manual M1 (icons, panel) and M4 (the cursors) |
| 2 | ui `as2_magnifier_level_kept` |
| 3 | core `as3_magnifier_zoom_rule`, ui `as3_magnifier_click` (the zoom, the focus, the rendered picture) |
| 4 | core `as4_focus_scroll` (the corners, a picture that fits), ui part of `as3_magnifier_click` |
| 5 | core `as5_magnifier_box` (the numbers of the scenario, the corner, a small picture, the minimum), ui `as5_box_follows_pointer` + manual M3 |
| 6 | core `as6_fill_ring` (a ring, diagonal ring, a gap, a secondary-button color) and `as6_fill_step_undo_redo` (the new history step: undo and redo restore exactly, the byte count), ui `as6_fill_on_canvas` (one step, Undo) |
| 7 | core `as7_fill_exact` (alpha, #00FF0000 against #00000000) |
| 8 | core `as8_fill_nothing`, ui `as8_fill_nothing_on_canvas` |
| 9 | core `as9_fill_memory_and_budget` (the 1800 × 1800 comb under a 16 MB budget: its step is 19.4 MB, so it is kept alone and one Undo restores the picture; an address-space limit as `as27_error_dialog` does makes the fill refuse and change nothing), ui `as9_fill_memory_dialog` (the failure is made with a test hook that caps the fill's work buffers; the dialog is shown from an idle callback) |
| 10 | ui `as10_pick_follows_and_returns` |
| 11 | ui `as11_pick_exact_colors` (transparent, #80FF8000) |
| 12 | ui `as12_pick_outside_picture` |
| 13 | ui `as13_pick_cancel_and_focus` (other button, Undo, second Undo ignored, focus loss, release outside) |
| 14 | ui `as14_numpad_ignored` |
| 15 | ui `as15_new_tools_ignored_when_blocked` (chooser open, resize drag, before the picture exists) |
| 16 | ui `as16_tool_change_ignored_during_pick` |
| 17 | ui `as17_pencil_after_magnifier` |
| 18 | ui `as18_lost_release_ends_the_press` (Fill, Pencil stroke, Pick Color) |
| 19 | core `as3_magnifier_zoom_rule` (16x from 100 % and back), ui `as19_magnifier_16x` (the change of 2026-10-28, 0008's T009) |

### Constitution check
Layering holds (the fill and the Magnifier rules are libc-only core; the canvas turns input into core calls); no new
threads or caches (P5); the three tools' defaults are session choices, not settings (P6 is kept for Preferences, as in 0006);
every REQ has a scenario and every scenario a test or a manual check (S3); the pointer notes on 0001 and 0006 are written.

### Risks
- **The fill's memory and time:** the undo step is small for a normal fill (spans), 12 bytes a pixel in the worst case; the
  failure path is tested with an address-space limit like REQ-0004-26's; the time is measured in `perf-0007` and nothing is
  optimized before the numbers.
- **The pointer-leave probe (T009):** the preview box relies on the canvas area reporting that the pointer left it (the
  window's existing leave event). This cannot be shown headless; it was confirmed in the M3 check on a real session.
- **A new history step kind** touches `history.c`, which every edit goes through: the existing core tests of 0002 and 0004 run
  unchanged, and the mutants of REQ-0002-01, -16 and -17 run after the task (and the new step gets mutants of its own).
- **The Magnifier's scroll depends on the allocation after `commit_zoom`** (the family of the stale-layout flakes of 0005 and 0006):
  the ui test waits for the layout and checks where the picture is drawn, by rendering; no fallback is planned.
- **The preview box needs "the pointer is over a picture pixel" and "the pointer left"**: the canvas has no leave handler today;
  a probe in the box task checks that GTK's motion controller reports the leave on this session before the box relies on it.
- **Cursors:** hotspots are chosen by eye; M1 checks them. The scale-2 question of 0006 applies to the three new textures too.
- **A tool list that grows** (Lines, Curves and Shapes, Selection and Text add more): the toolbox grid is a template; each feature adds its buttons in the
  order of REQ-0006-01.


### Numbers (T012, `tests/perf/perf-0007.c`, 2026-10-18, this machine)
| Measure | Result |
|---|---|
| Core fill, blank 1920 × 1080 (2.07 M pixels) | 8.3 ms |
| Core fill, blank 8000 × 8000 (64 M pixels) | 160 ms; peak memory unchanged (the picture's 256 MB; the 8000 spans are 96 KB) |
| Core fill, 1800 × 1800 comb (1.62 M one-pixel spans) | 21.9 ms; peak memory +25 MB (the 19.4 MB step and the work stack) |
| Fill press on a 100-pixel area to the painted frame, 1920 × 1080 and 8000 × 8000 | median 16.5 / 16.7 ms, worst 20 / 23 ms (one frame) |
| Magnifier press to the painted frame, 1920 × 1080 and 8000 × 8000 | median 16.6 / 16.5 ms, worst 22 / 24 ms (one frame) |

No whole-picture fill takes a second, so no fix is proposed (no threads). JOD set the budgets on 2026-10-18 (`docs/engineering.md`): a whole 8000 × 8000 fill under 500 ms (measured 160 ms), a Fill or Magnifier press to the painted frame under 100 ms (measured one frame); the comb has no time budget. They are benchmark budgets, reported by `/sdd-check`, not pass/fail tests.

## Tasks
Groups end with a full `meson test`; M1 to M4 are manual checks (the look and the feel cannot be tested headless).

A. The core
- [x] T001 A history step for a fill (REQ-0007-05, REQ-0007-07; AS-6, AS-9) - done when core `as6_fill_step_undo_redo` (undo and redo restore exactly; 12 bytes per span plus 64; the spans are owned on success and kept by the caller on failure; no spans adds no step) passes, the core tests of 0002 and 0004 pass unchanged, and mutants of the new step (undo writes the new color, the byte count, the ownership) are in `tests/mutants.txt` and killed by assertions
- [x] T002 The core fill (REQ-0007-05, REQ-0007-06, REQ-0007-07; AS-6, AS-7, AS-8, AS-9) - done when core `as6_fill_ring` (a ring, a diagonal-only ring that does not leak, a one-pixel gap that does, the secondary color), `as7_fill_exact` (alpha, #00FF0000 against #00000000, transparent fill color), `as8_fill_nothing` (same color, outside, no step) and `as9_fill_memory_and_budget` (the 1800 x 1800 comb under 16 MB is the only step; an address-space limit makes the fill refuse and change nothing) pass, each fails first, and the mutants of the fill (4-connected to 8-connected, a channel ignored, no restore on failure) are killed
- [x] T003 The Magnifier rules (REQ-0007-03, REQ-0007-04; AS-3, AS-4, AS-5) - done when core `as3_magnifier_zoom_rule` (100 gives the level, the level gives 100, 42 % and 300 % give the level), `as4_focus_scroll` (the focus at the center, the corners, a picture that fits stays at 0) and `as5_magnifier_box` (695 x 426 at (653, 487), 173 x 106 for 8x, the corner, a picture smaller than the box, the one-pixel minimum) pass, each fails first, and their mutants are killed

B. The tools in the window
- [x] T004 Three tools in the toolbox, with icons, hints and no options (REQ-0007-01, REQ-0007-13; AS-1) - done when ui `as1_new_tools_toolbox_hints` (the order Pan, Magnifier / Eraser, Pick Color / Fill, Pencil / Brush, Airbrush, the tooltips, the hints, the panel and its line hidden for Pick Color and Fill) passes and the toolbox tests of 0001 and 0006 still pass
- [x] T005 The Magnifier's options (REQ-0007-02; AS-1, AS-2) - done when ui `as2_magnifier_level_kept` (1/10x, 1/4x / 1/2x, 1x / 2x, 4x / 6x, 8x with the tooltips, 2x selected, choosing changes no zoom, kept after another tool) and the Magnifier part of `as1_new_tools_toolbox_hints` pass, and manual M1 (the icons and the options in light and dark themes) passes
- [x] T006 Fill on the canvas and its memory dialog (REQ-0007-05, REQ-0007-06, REQ-0007-07; AS-6, AS-8, AS-9) - done when ui `as6_fill_on_canvas` (one step, Undo, the modified mark, the secondary button), `as8_fill_nothing_on_canvas` and `as9_fill_memory_dialog` ("Could Not Fill", nothing changed) pass

C. Picking and zooming
- [x] T007 Pick Color (REQ-0007-08, REQ-0007-09, REQ-0007-10, REQ-0007-11, REQ-0007-16; AS-10, AS-11, AS-12, AS-13, AS-16) - done when ui `as10_pick_follows_and_returns`, `as11_pick_exact_colors`, `as12_pick_outside_picture`, `as13_pick_cancel_and_focus` and `as16_tool_change_ignored_during_pick` pass, and manual M2 (Fill and Pick Color on a real picture) passes
- [x] T008 The Magnifier's click (REQ-0007-03, REQ-0007-11, REQ-0007-17; AS-3, AS-4, AS-17) - done when ui `as3_magnifier_click` (the zoom, the focus at the center of the view checked by rendering the window, the same level returns to 100 %, the secondary button, middle button and outside do nothing, no step and no modified mark) and `as17_pencil_after_magnifier` pass
- [x] T009 The preview box (REQ-0007-04; AS-5) - done when a probe shows that GTK reports the pointer leaving the canvas area on this session (recorded in the Plan's risks), ui `as5_box_follows_pointer` (the box's place and size, hidden outside the picture and at other zooms, back at once after returning to 100 %) passes, and manual M3 (the Magnifier's feel and the box) passes

D. Cursors, keys, blocked input and the numbers
- [x] T010 The three cursors (REQ-0007-12; AS-1) - done when ui `as1_cursors_follow_tool` passes and manual M4 (the bucket, the dropper and the glass with their hotspots, the resize cursor still wins) passes
- [x] T011 The keys and blocked input (REQ-0007-14, REQ-0007-15; AS-14, AS-15) - done when ui `as14_numpad_ignored` and `as15_new_tools_ignored_when_blocked` pass
- [x] T012 (numbers in the Plan; budgets set by JOD 2026-10-18: whole 8000 × 8000 fill under 500 ms, a Fill or Magnifier press to the painted frame under 100 ms, reported by `/sdd-check`) Measure Fill and the Magnifier (the Fill has no budget in engineering.md; the Magnifier is judged by one frame; REQ-0007-05, REQ-0007-03) - done when `perf-0007` records the whole-picture fills, the comb and the Magnifier press, and the numbers are in the Plan, and the owner has been asked whether a budget is needed
- [x] T013 A release that never arrives (REQ-0007-18; AS-18) - done when ui `as18_lost_release_ends_the_press` passes (a Fill press, a Pencil stroke and a pick, each without its release, end at the next pointer move with no button down) and manual M5 (View > Fit to Window with the Magnifier, then the options and a click) passes

## Check
- Date and commit: 2026-10-18, `f7213cb` plus the uncommitted work of 0007
- Tests: `meson test` 26 of 26 pass (the cases of this feature: 18 ui `/0007/...`, `core-fill`, `core-magnifier`, and the new history step in `core-history`)
- Mutants: 136 of 136 killed (621 s); none killed only by a crash
- Warnings: 0
- Budgets (`perf-0007`, reported, not tests): whole 8000 × 8000 fill 208 ms (budget < 500 ms); Fill and Magnifier press to the painted frame, median 16.4 to 16.7 ms, worst 21 ms (budget < 100 ms): within budget
- Findings:
  1. Low: `tools/spec.sh lint --names 0007` fails: the scenario map (AS-9) names `easel_canvas_set_fill_memory_limit`, a function, in the place of a test. Open: JOD to accept rewording the map without the identifier in backticks (the test is `as9_fill_memory_dialog`).
  2. Low: the comment on `Work.max_bytes` in `src/core/fill.c` says the two buffers together, but `grow` checks each buffer alone. The hook still works; open: fix the comment, or check the sum.
  3. Low: T009's probe (GTK reports the pointer leaving the canvas area) was not recorded in the Plan's risks as the task says; JOD's M3 check on a real session covered it. Open: add one line to the Plan, or accept as is.
  4. Low: T010 and T011 tests passed on their first run (the code was written with or before them), so the "each fails first" rule was not shown for them; T013's test was shown failing with the fix removed. No mutants exist for the preview box's show rules, the cursors or the blocked input; none of them guards against data loss, so none is required. Left open.
- Manual checks (all reported "ok" by JOD; he gave no detail beyond that, and the steps of M5 after its second were not walked through one at a time): M1 (the icons and the options, light and dark themes) ok; M2 (Fill and Pick Color on a real picture) ok; M3 (the Magnifier's feel and the box; this also confirmed that the pointer leaving the canvas area is reported) ok; M4 (the bucket, the dropper and the glass with their hotspots, the resize cursor still wins) ok; M5 (View > Fit to Window with the Magnifier, then the options and a click) ok.
- Settled after the check (2026-10-18): findings 1, 2 and 3 fixed (scenario map reworded, comment in `fill.c` corrected, one line added to the Plan's risks; `lint --names 0007` and `lint` pass); finding 4 left open on purpose, since T010, T011 and T013 stay characterization tests without a red run.
- Verdict: clean apart from the four low findings; no drift, no layering break, one undo step per fill, no AI trailers (nothing committed yet).
