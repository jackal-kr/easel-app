# 0040 Smooth text

Status: implemented
Reviewed: spec-reviewer, 2026-11-11 (9 findings merged; one asked: the highlight under a soft edge)
Depends on: 0017, 0020, 0022, 0030, 0039

## Why
Text is written in whole hard-edged pixels (REQ-0017-16, P4), as XP Paint does, so letters show stairs on their curves and slants.
For a label, a poster or a diagram, JOD wants an opt-in smoothing of text, as 0039 gave lines, shapes and brushes: off by default so
the app stays XP-faithful (`docs/product.md`, `docs/reference/new-behaviors.md`, ADR-0003), its own choice, its own default.

## Requirements
Pointer notes added at acceptance: REQ-0039-01 (Text now shows its own well), REQ-0017-02 (the panel holds the Smooth well),
REQ-0017-16 and REQ-0017-05 (with Smooth, REQ-0040-04 and REQ-0040-06), REQ-0017-24 and REQ-0020-05 (the new key), REQ-0020-11 (the
created file's `[text] smooth=false`), `docs/reference/new-behaviors.md`.

- REQ-0040-01 (Adds to REQ-0017-02's panel; changes REQ-0039-01's "Text shows no such well".) While Text is active, the options panel shall show, below the Opaque | Transparent well, a well of two cells side by side, Sharp then Smooth, drawn, named and with the tooltips of 0039's well (REQ-0039-01: "Sharp edges", "Smooth edges"). One of the two is chosen. It is Text's own choice: choosing it changes nothing of Line, Curve, the shapes or Brush, and choosing theirs changes nothing of Text (JOD, 2026-11-11).
- REQ-0040-02 The choice shall be changed by a click on a cell, or from the keyboard as the mode cells of REQ-0017-02 are reached and chosen (REQ-0022-04), and by nothing else: no shortcut. While a button is held on the canvas (making, moving or resizing the box, or selecting its text), choosing is ignored, as a choice in the panel is then (REQ-0006-03). While a box is open, a choice changes that box at once: it shows, and is put down, in the new choice (JOD, 2026-11-11), as the mode does (REQ-0017-02). The choice is no edit of the box: Undo does not take it back (REQ-0017-08). Sharp and Smooth give the same line breaks, box height and caret places (every glyph and advance on whole pixels, REQ-0017-28), so a change never meets REQ-0017-07's bottom edge.
- REQ-0040-03 (Adds to REQ-0017-24 and REQ-0020-05.) At start-up the choice shall be Smooth when the settings file's `[text]` key `smooth` is `true`, in any case as the file's other values (REQ-0020-11), and Sharp otherwise (missing, `false` or any other value, silently). Preferences shall show a switch "Smooth text" in its Text group, after Strikethrough, written as `smooth=true` or `smooth=false` at OK or Apply (REQ-0027-07); like the other rows of that group it sets the start-up choice and leaves the current session's choice as it is. A settings file the app creates holds `smooth=false` in `[text]`. A choice made in the options panel lasts for the session and is not written to the file.
- REQ-0040-04 (Changes REQ-0017-16 while Smooth is chosen.) With Smooth, the text shall be drawn with the look of `docs/reference/proto-0040/sheet.png`, "Smooth, hint full" (JOD, 2026-11-11): the same letter shapes, places and sizes as Sharp (REQ-0017-28), with blended edges instead of whole-or-nothing pixels, and no colored (subpixel) fringes. Each pixel gets, for each run of characters of one style, a coverage c from 0 (none) to 255 (all) by the run's glyphs, underline and strikethrough taken together as one shape: where a glyph meets its underline, or one character's underline the next one's, the pixel is no darker than either alone. The pixel then takes the mix of its color before that run and the run's color by c: with old (r, g, b, a) and the run's (r_t, g_t, b_t, a_t), M = a (255 − c) + a_t c, the alpha is round (M / 255) and the red, when M is not 0, round ((r a (255 − c) + r_t a_t c) / M), at most 255, green and blue alike; an alpha of 0 gives 0x00000000 (P7.7); halves round up (the mix of REQ-0039-06 with c in 255ths). So c = 255 gives the run's color exactly and c = 0 leaves the pixel. A color glyph's pixel mixes its own color at that pixel by its own coverage there.
- REQ-0040-05 The runs of a box shall be laid in the order of the text, each over what the earlier ones and the box's ground left, so where two characters overlap the later is on top, as with Sharp (REQ-0017-16). In Opaque mode the box is first filled with the background color and the text is mixed over it; in Transparent mode over the picture's pixels. A foreground with an alpha below 255 is mixed by REQ-0040-04 (a pixel it covers wholly takes it exactly).
- REQ-0040-06 (Changes REQ-0017-05's highlight while Smooth is chosen.) While a box is open with Smooth, its selected text's highlight shall be drawn under every pixel the selected text covers at all, and such a pixel shows the mix of REQ-0040-04 of the text's color over the highlight's color by its coverage (a whole pixel shows the letter's own color, as with Sharp) (JOD, 2026-11-11).
- REQ-0040-07 With Smooth chosen, everything else shall be as with Sharp (REQ-0017-03 to REQ-0017-28): the box, its outline and tint, the font card, editing, Undo inside the box and the one undo step of the put-down, the modified mark, a box with no characters or one that changes no pixel adding no step, the pixels inside the picture only, the memory failure of REQ-0017-18, Cut, Copy, Paste, and what puts a box down. If the memory to show the open box in the new choice cannot be had, the choice stays as it was and the error bell sounds, as a refused edit does (REQ-0017-07). With Sharp chosen, text is written exactly as before this feature.
- REQ-0040-08 The pixel contract (P7): (1) every pixel is whole, its color a run's exact color (or a color glyph's own), the old color, or the mix of REQ-0040-04: smoothing, opt-in, as P7.1 allows; (2) the view only magnifies; (3) an open box shows exactly the pixels its put-down writes, under the selection's tint as with Sharp (REQ-0017-16, AS-16); (4) the pixels depend only on the text, its styles and fonts, the box's place and the mode, never on the zoom, the scroll or the screen scale; the same character in the same font, size and style has the same coverage wherever it falls in a line (REQ-0017-28), so its pixels are the same wherever no other character's coverage reaches them (a smooth letter's soft edge can reach a neighbour's place, `docs/reference/proto-0040/same_letter.py`).

## Scenarios
- AS-1 (REQ-0040-01, REQ-0040-02) Given Text active, Then a well Sharp | Smooth shows below Opaque | Transparent, Sharp chosen, with 0039's tooltips and names; When Smooth is chosen there, Then Line, Ellipse and Brush still show Sharp; When Smooth is chosen for Line, Then Text still shows Sharp; Tab reaches the cells and Space chooses; while a box is being made, and while text in a box is selected by a drag (button held), a click on Smooth changes nothing | Verified by: automated (ui) + manual M1 (the look, JOD)
- AS-2 (REQ-0040-03) Given `smooth=true` in `[text]`, When the app starts, Then Text's Smooth is chosen and 0039's choice is Sharp; `smooth=TRUE` the same; `smooth=yes` or no key, Sharp; When Preferences' "Smooth text" is turned on and Apply pressed, Then the file holds `smooth=true` in `[text]` and the panel's choice is unchanged; on and Cancel, the file is unchanged; a created file holds `smooth=false` in `[text]`; choosing in the panel writes nothing | Verified by: automated (core, ui)
- AS-3 (REQ-0040-04, REQ-0040-05, REQ-0040-08) Given a white picture, black, Transparent, Smooth, When a box with "Hello" in DejaVu Sans at 24 points is put down, Then every changed pixel is #000000FF or a gray with R = G = B from #010101FF to #FEFEFEFF, and there are gray pixels; a box with "l   l   l" (spaced): every "l" has the same pixels; with Sharp the same "Hello" gives only #FFFFFFFF and #000000FF | Verified by: automated (ui)
- AS-4 (REQ-0040-04, REQ-0040-05) Given a fully transparent picture, red #FF0000FF, Transparent, Smooth, When "o" at 24 points is put down, Then every changed pixel is red #FF0000 with an alpha from 1 to 255 (a mix over transparency keeps the red); given Opaque with white as the background on the same picture, Then the box is #FFFFFFFF where no letter is and each pixel of the "o" is the mix of REQ-0040-04 of white and red by its coverage | Verified by: automated (ui)
- AS-5 (REQ-0040-02, REQ-0040-08) Given an open box with "Hi" and Sharp, When Smooth is chosen, Then the box shows smoothed at once, with the same height and caret place, rendered at 100 % and at 800 % equal to its put-down under the tint (REQ-0017-16, AS-16 of 0017), and Undo inside the box does not bring Sharp back; When the box is put down, Then one step is added, and one Undo restores the picture; at scale 2 the put-down's pixels equal scale 1's | Verified by: automated (ui, rendering the window; the scale 2 suite)
- AS-6 (REQ-0040-07) Given Smooth and memory made too small for the put-down's step, When the box is put down, Then the dialog of REQ-0017-18 shows and the picture is unchanged | Verified by: automated (ui)
- AS-7 (REQ-0040-05) Given a box in Opaque mode, a foreground #00000080, Smooth, When "x" is put down over a white picture, Then a pixel the "x" covers wholly is #00000080 and a partly covered one the mix of REQ-0040-04 of the white box and #00000080 | Verified by: automated (ui)
- AS-8 (REQ-0040-04) Given underlined "ab" and struck-through "cd", Smooth, When put down, Then along the underline's and the strike's rows no pixel at the joint between two letters is darker than the line's pixels on either side | Verified by: automated (ui)
- AS-9 (REQ-0040-04, REQ-0040-08) Given a color emoji in a box with Smooth over white, When put down, Then its fully covered pixels are its own colors and its edge pixels mixes of them with white; with Sharp, as REQ-0017-16 has them | Verified by: automated (ui) when the system has a color emoji font, else manual M1
- AS-10 (REQ-0040-06) Given an open box with "Hi" in Smooth and "H" selected, When the canvas is drawn, Then the highlight lies under every pixel the "H" covers at all, a whole pixel shows the letter's color, and a partial one the mix of the letter's color over the highlight's | Verified by: automated (ui, rendering the window)

## Edge cases
- A box partly outside the picture: only the pixels inside change (REQ-0040-07); the coverage is as if the picture went on.
- Two characters overlapping (a kerned pair, an italic's overhang) in different runs: the later on top (REQ-0040-05); in one run, one shape (REQ-0040-04).
- A box of spaces in Transparent mode: no pixel changes, no step (REQ-0040-07).
- The choice changed while the font card's field has the focus: it applies to the box at once.
- A font missing on another machine changes the pixels as it does with Sharp (REQ-0017-24); the pixels never depend on the screen.
- A smooth letter's soft edge reaches about a pixel into the next letter's place (`docs/reference/proto-0040/same_letter.py`): touching letters share edge pixels.

## Out of scope
- Colored (subpixel) smoothing, other hinting, a gamma for the mix.
- A shared choice with 0039's (JOD, 2026-11-11: Text's own); a shortcut for the choice.
- Smooth Pencil, Airbrush, Eraser, Fill, Gradient and selections (0039's out of scope).

## Test plan notes (JOD, 2026-11-11, for the Plan)
- (a) A test that fails if text is ever drawn with colored (subpixel, LCD) smoothing: every Smooth pixel of black text on white has
  R = G = B.
- (b) The Sharp and Smooth layouts' equality on the 40 font and size cases of `docs/reference/proto-0040/same_layout.py` as a
  permanent test of the app, not only a prototype.
- (c) No golden-pixel comparisons for Smooth text (they would change with the fonts and the font engine): invariants only.

## Plan
(Opus, 2026-11-11.) Read with `docs/engineering.md`, `docs/design.md`, 0017's Plan (the text is Easel's own, laid out by Pango at
picture scale, its ink rasterized per style run into an 8-bit coverage surface, a pixel set where the coverage is at least 128).

Approach.
- One layout, two inks. The layout stays Sharp's (antialias none, hint full, hint metrics on): `docs/reference/proto-0040/same_layout.py`
  found Sharp and Smooth giving the same glyph positions, line breaks and height on 40 font and size cases, and the permanent test (b)
  keeps it so. Only the ink differs: with Smooth each run is rasterized with gray antialiasing (hint full) and its coverage kept, 0 to 255,
  instead of the threshold at 128. Since Pango's context options override the drawing surface's, the smooth ink is drawn through a
  layout made on a second context with gray antialiasing, from the same text, width and attributes (risk 1).
- The ink holds, for Smooth, one layer per style run: its rectangle (the run's coverage bounds), its coverage bytes, and its color, or for
  a run with color glyphs a straight color per pixel (the glyph's own where it differs from the run's by more than 2, as Sharp decides).
  Layers are laid in run order (REQ-0040-05).
- One function lays an ink over a buffer of the picture's pixels, for both modes: Sharp's masked pixels replaced as today, Smooth's
  layers mixed in order by REQ-0040-04's mix in 255ths, an Opaque box's ground first. The put-down (`easel_text_put_down`, which every way
  of putting a box down reaches through the canvas's `text_put_down`) and the box's view (`append_text_box`) both call it (P7.3).
- The highlight (REQ-0040-06): with Smooth, every selected pixel whose layers cover it at all gets the accent under it and the text's
  color at its coverage over it; whole pixels as Sharp has them.
- The choice: `win.text-smooth`, Text's own (not `win.smooth`), ignored while stroking; it re-makes the open box's ink at once, and if the
  memory cannot be had the state and the ink stay as they were and the bell sounds (REQ-0040-07).

Files and interfaces.
- `src/core/cover.{c,h}`: `easel_cover_mix255 (old, color, c)`, REQ-0040-04's formula (0039's mix with c in 255ths).
- `src/core/text-put.{c,h}`: `EaselTextLayer { int x, y, w, h; uint32_t color; const uint8_t *cover; const uint32_t *colors; }`
  (`colors` NULL unless the run has color glyphs); `EaselTextInk` gains `const EaselTextLayer *layers; size_t n_layers` (Smooth; `color`
  and `mask` NULL then); `easel_text_ink_lay (ink, box, opaque, background, x0, y0, w, h, px, stride)`; `easel_text_put_down` lays
  through it.
- `src/ui/text-render.{c,h}`: `easel_text_render_ink` takes `smooth`; a gray context (`easel_text_render_smooth_context`) for the smooth
  pass; the layers' memory counted in `max_bytes` (1 byte a covered pixel, 4 more where a run has color glyphs).
- `src/ui/canvas.{c,h}`: `text_smooth`, `gboolean easel_canvas_set_text_smooth (self, smooth)` (FALSE without memory). Every reader of
  the ink and so of the flag: the relayout (`text_relayout`, canvas.c ~872), an edit tried on a copy (~999), a resize (~1596, ~1616),
  `text_box_pixel` and `append_text_box` (the view), `append_text_highlight`, `text_put_down`, and the ink's free; tests that read
  `text_ink.mask` (grep at T003).
- `src/ui/window.c`: the action `text-smooth` (boolean, ignored while stroking, the bell when the canvas refuses), start state from
  the settings. `src/ui/tool-options.c`: `new_smooth_well (action)` takes the action's name; the Text page gets one with `win.text-smooth`.
- `src/core/settings.{c,h}`: `EaselSettings.text_smooth`, `[text] smooth`, `smooth=false` in a created file's `[text]`.
  `src/ui/preferences-dialog.c`: the switch "Smooth text" after Strikethrough.
- Strings: "Smooth text" in 11 languages; the cells reuse 0039's "Sharp edges", "Smooth edges"; Help's sentence on Sharp and Smooth
  names Text too, in 12 pages.
- What grows: an open box's smooth ink, per style run 1 byte per pixel of the run's coverage rectangle (5 with color glyphs), made anew
  at each relayout as the Sharp ink is; nothing in the history beyond the put-down's step as with Sharp.

As built (2026-11-11), where it differs from the above: the one laying function is the per-pixel `easel_text_ink_value (ink, x, y,
under, box, opaque, background)` (the put-down already asked per pixel; the view's `text_box_pixel` asks it too); `text-render` has
`easel_text_render_context_for (smooth)`, `easel_text_render_layout_for (b, width, smooth)`, `easel_text_render_ink (..., smooth, ink)`,
`easel_text_render_ink_free` and `easel_text_render_ink_move` (a moved box's layers); `easel_cover_mix255` is in `cover`; the canvas
adds `easel_canvas_set_text_smooth` (the bell on a refusal) and `easel_canvas_is_selecting_text` (risk 3: a text selection drag is not
in `easel_canvas_is_stroking`); `new_smooth_well (action)`; the cells drawn as four magnified pixels (JOD, idea D, for 0039's too).

Tests (JOD's notes): (a) `/0040/as3_gray_and_same_letters` asserts R = G = B for every changed pixel of black text on white, and so
does the headless `text-smooth-layout` test on the ink itself, with a mutant that turns on colored (subpixel) antialiasing; (b)
`text-smooth-layout`, a test program of its own (Pango only, no display), compares Sharp's and Smooth's glyph positions, line breaks and
height on the 40 font and size cases of the prototype, skipping (77) a font not installed and failing when none is; (c) no golden pixels:
every Smooth assertion is an invariant (R = G = B, an alpha range, the same letter's pixels, no darker joint, preview equals put-down,
the mix's formula on the coverage the test reads back), never a stored image or a count of gray pixels.

| Scenario | Test |
|---|---|
| AS-1 | ui `/0040/as1_own_choice`, `/0040/as1_keyboard`, `/0040/as1_ignored_while_held` (making a box, selecting by a drag), `/0040-a11y/as1_names` + manual M1 |
| AS-2 | core `settings_text_smooth` + ui `/0040/as2_preferences` |
| AS-3 | ui `/0040/as3_gray_and_same_letters` + `text-smooth-layout` (a) |
| AS-4 | ui `/0040/as4_red_over_transparent`, `/0040/as4_opaque_box` |
| AS-5 | ui `/0040/as5_switch_open_box` (rendered at 100 and 800 %, under the tint) + `/0040/as5_moved_box` + `/0040-scale2/as5_scale2` |
| AS-6 | ui `/0040/as6_no_memory`, `/0040/as6_switch_without_memory` (the choice kept, the bell) |
| AS-7 | ui `/0040/as7_half_alpha` + core `ink_lay_layers` |
| AS-8 | ui `/0040/as8_underline_joint` |
| AS-9 | ui `/0040/as9_color_emoji` (skipped with a message where no color emoji font is installed; then M1) |
| AS-10 | ui `/0040/as10_highlight` (rendered) |

Core oracles: `mix255_oracle` (the formula in doubles over a sweep, c = 0 and 255 exact, P7.7's zero); `ink_lay_layers` (layers in
order, Opaque's ground, a pixel no layer covers unchanged, Sharp's mask exactly as before, the empty ink and a one-pixel layer).

Constitution: P4 and P7 as REQ-0040-08 (smoothing opt-in, P7.1); layering: the mix and the laying in core, Pango in ui; no threads, no
caches; Sharp text's pixels unchanged (every 0017 test passes as it is).

Risks:
- 1. Pango's context font options override the surface's (from the pangocairo documentation): probed in T001 by rendering one layout's
  text through a gray context and reading back partial coverage; fallback: set the options on the layout's own context for the smooth
  pass and set them back.
- 2. The layout's sameness in the app (not only the prototype's): T001's `text-smooth-layout` runs before any ui code relies on it.
- 3. `easel_canvas_is_stroking` names `text_drag`; whether making a box and selecting by a drag set it is probed in the ui task of the
  choice (T004) before REQ-0040-02's ignore relies on it.
- 4. The color emoji font may be missing in the isolated session: AS-9's test skips with a message there, and M1 covers it.
- 5. The highlight's partial pixels are drawn by GTK's color nodes (accent, then the text's color at the coverage), so they equal REQ-0040-06's
  mix within 1 a channel (as 0039's cells): T006's test allows 1.

## Tasks
(Opus, 2026-11-11; JOD's go to the Plan, 2026-11-11.)

Group A, core and the ink
- [x] T001 Probes and the layout's sameness (risks 1, 2; REQ-0040-02, REQ-0040-08; JOD's note b) - done when `text-smooth-layout`
  (its own test program, Pango only) compares Sharp's and Smooth's glyph positions, line breaks and height on the prototype's 40 font
  and size cases and passes, and a gray context's coverage is read back partial (risk 1), with a mutant that makes the two layouts differ
  Result: new `text-smooth-layout` (`tests/ui/test-text-layout.c`, Pango only): `/0040/same_layout` (the prototype's 40 cases, five
  fonts by eight sizes, every byte's position and width, the line count and the size equal for Sharp and Smooth; a font not installed
  skipped, none installed 77; all 40 ran here) and `/0040/smooth_context_draws_partial` (Sharp draws no partial pixel, Smooth more than
  100: risk 1 settled, the context's options decide) failed first on stubs at `assertion failed (partial_pixels (smooth) > 100): (0 >
  100)`; with `easel_text_render_context_for (smooth)` (gray antialiasing, nothing else changed) and `easel_text_render_layout_for` they
  pass. 2 mutants killed (Smooth with hint metrics off: `(hs == hm): (30 == 28)`; Smooth drawn 1-bit); 0017's antialias mutant repointed
  (`--lint` 1428, 0 stale) and killed. Core, io and ui-0017 pass.
- [x] T002 The mix in 255ths (REQ-0040-04) - done when core `mix255_oracle` (a sweep against the formula in doubles; c = 0 and 255 exact;
  P7.7's zero) fails first and passes, with mutants (256ths, halves down, no zero)
  Result: core `mix255_oracle` (3000 color pairs, alphas 0, 1, 128 and 255 forced often, every coverage 0 to 255, against the formula
  in doubles; c = 255 the color, c = 0 the old, red at half over transparency #40FF0000, a transparent color over black 0) failed first
  on a stub returning the old color at `test-cover.c:869: CHECK failed: got == want`; `easel_cover_mix255` in 64-bit integers passes.
  3 mutants killed (256ths, halves down, no zero); 0039's alpha mutant made unique again (`--lint` 1431, 0 stale) and killed.
- [x] T003 The ink's layers and the one laying function (REQ-0040-04, REQ-0040-05, REQ-0040-07; AS-7 core) - done when core
  `ink_lay_layers` (layers in order, Opaque's ground first, an uncovered pixel unchanged, the empty ink, a one-pixel layer) fails first and
  passes, the put-down lays through it, every 0017 test passes unchanged (Sharp's pixels), and the tests that read `text_ink.mask` are
  listed and still pass
  Result: core `ink_lay_layers` (a 10 x 6 picture of opaque and transparent colors, a box (1, 1, 7, 4), Transparent and Opaque: two
  overlapping layers mixed in order by `easel_cover_mix255`, a color glyph's own color, a one-pixel layer, uncovered pixels kept or the
  ground; the same through the put-down with one step and Undo; an empty ink: nothing in Transparent, the box in Opaque) failed first on
  a stub at `test-text-put.c:207: CHECK failed: easel_text_ink_value (...) == want`; with `EaselTextLayer`, `EaselTextInk`'s `layers`
  and `n_layers`, and `easel_text_ink_value` (the Opaque ground, Sharp's mask as before, then the layers), which the put-down now asks
  for every pixel, it passes. The Plan's buffer-wide `easel_text_ink_lay` became this per-pixel function (the put-down already asked per
  pixel; the canvas's view, T005, asks the same). Readers of `ink.mask` in the tests: `/0017` cases calling `easel_text_render_ink`
  directly (lines ~18800 to 19110), Sharp ink, unchanged here, updated with T004's flag. 3 mutants killed (layers reversed, a color glyph
  in the run's color, the ground over the text); 0017's Opaque mutant repointed (`--lint` 1434, 0 stale). Core, io and ui-0017 pass.
- [x] T004 The smooth ink (REQ-0040-04, REQ-0040-08; AS-3, JOD's note a) - done when `text-smooth-layout`'s ink case (black text on white:
  every partial pixel R = G = B; the same letter's coverage in "l   l   l") fails first and passes, with a mutant that turns on colored
  (subpixel) antialiasing
  Result: `text-smooth-layout`'s `/0040/smooth_ink_gray_and_same_letters` ("Hello" and "l   l   l" in DejaVu Sans 24, black, the ink laid
  over white by `easel_text_ink_value`: no mask, one layer; every changed pixel opaque with R = G = B, more than 20 grays; each "l" from a
  pixel before its place to one after its advance equal to the first's) failed first at `'ink.mask' should be NULL` (the flag ignored);
  with `smooth_ink` (each run drawn alone through `easel_text_render_layout_for (b, width, TRUE)`, its coverage kept over its non-zero
  rectangle, a color glyph's run with each pixel's color by `glyph_color`, Sharp's rule; the bytes counted against `max_bytes`) and
  `easel_text_render_ink_free` it passes. Every caller of `easel_text_render_ink` passes the flag (FALSE until T005) and frees through
  the new function (canvas, `perf-0017`, two `/0017` cases). JOD's note a: the tint mutant (a partial pixel 0x10 off the run's color) is
  killed by `assertion failed (g == bl)`; a mutant turning on subpixel antialiasing survived and was removed as equivalent: the coverage
  is read from an 8-bit, one-channel surface, so Cairo cannot draw colored fringes into it, and the R = G = B check on the final pixels
  stays as the guard for any later path. The test's `g_error` reports became assertions (an abort counts as a test gap). 0017's
  edit-without-memory mutant repointed (`--lint` 1435, 0 stale). Core, io, `text-smooth-layout` and ui-0017 pass.

Group B, the window
- [x] T005 The choice on the canvas (REQ-0040-02, REQ-0040-03 core part, REQ-0040-05, REQ-0040-07; AS-3 to AS-9 ui) - done when core
  `settings_text_smooth` and ui `/0040/as3_gray_and_same_letters`, `/0040/as4_red_over_transparent`, `/0040/as4_opaque_box`,
  `/0040/as5_switch_open_box` (rendered at 100 and 800 %, under the tint; Undo keeps the choice), `/0040-scale2/as5_scale2`,
  `/0040/as6_no_memory`, `/0040/as6_switch_without_memory`, `/0040/as7_half_alpha`, `/0040/as8_underline_joint` and
  `/0040/as9_color_emoji` fail first and pass through `win.text-smooth` (no well yet), and risk 3 is probed (making a box and selecting
  by a drag hold the choice)
  Result: core `settings_text_smooth` (true and TRUE Smooth; yes, false, a bad repeat, none Sharp; apart from `[drawing] smooth`; the
  created file's `strikethrough=false\nsmooth=false\n`) failed first at `test-settings.c:412`; ui `/0040/as3_gray_and_same_letters`
  (put down with Smooth: every changed pixel black or a gray with R = G = B, more than 20 grays; "l   l   l": three groups of inked
  columns with the same pixels; with Sharp only white and black), `/0040/as4_red_over_transparent`, `/0040/as4_opaque_box` (each pixel
  white, red or the mix by some coverage from 1 to 254, found by trying each), `/0040/as5_switch_open_box` (Sharp then Smooth: the same
  height and caret, no mask, the view at 100 % and 800 % within 2 of the ink's value under the tint, Undo in the box keeps Smooth, the
  put-down equal to the ink's values, one step), `/0040-scale2/as5_scale2` (the put-down equal to an ink made with no window), the added
  `/0040/as5_moved_box` (Alt with the arrows: the layers move with the box), `/0040/as6_no_memory`, `/0040/as6_switch_without_memory`
  (Sharp kept, the bell, the mask still there), `/0040/as7_half_alpha`, `/0040/as8_underline_joint` (the longest black row of "ab" and of
  "cd" even between its ends) and `/0040/as9_color_emoji` (Noto Color Emoji installed here: colored, opaque pixels) failed first at
  `text_smooth_state: 'v' should not be NULL`. With `win.text-smooth`, `easel_canvas_set_text_smooth` (the open box's ink made again at
  once, or the choice kept and the bell), the flag passed at the three places the ink is made, the view asking `easel_text_ink_value`,
  `easel_canvas_text_ink` answering for layers, `easel_text_render_ink_move` for a moved box, and `[text] smooth` they pass. My tests had
  two faults, not the code: the foreground set after the box was made (a box takes it when made, REQ-0017-12), and "cd" looked for in
  the box's right half though it follows "ab". Risk 3, probed: making a box sets `selecting`, which `easel_canvas_is_stroking` counts;
  selecting text by a drag sets only `text_selecting`, which it does not: T006 makes the choice ignore it too. 4 mutants killed (an
  edited box's ink Sharp, layers left behind, a refused switch taken, the view without layers); the start-up mutant survives until T008's
  test. Three 0017 mutants repointed at the view's new call (`--lint` 1440, 0 stale), killed. Core, io, ui-0017, ui-0022, ui-0040 and
  its scale 2 pass.
- [x] T006 The well (REQ-0040-01, REQ-0040-02; AS-1) - done when ui `/0040/as1_own_choice`, `/0040/as1_keyboard`,
  `/0040/as1_ignored_while_held` and `/0040-a11y/as1_names` fail first and pass, the window's least height with Text equal to Pan's;
  manual M1a: JOD looks at Text's panel (light and dark)
  Result: ui `/0040/as1_own_choice` (Text's Sharp and Smooth side by side in an `options-well` below Opaque, Sharp chosen; Smooth there
  leaves Line, Ellipse and Brush at Sharp, and Smooth for Line leaves Text at Sharp; the toolbox's width and the window's least height as
  Pan's), `/0040/as1_keyboard` (Tab from Transparent to Sharp, then Smooth; the activation chooses), `/0040/as1_ignored_while_held`
  (a drag selecting text in the box and a box being made: a click on Smooth changes nothing; after the release it chooses) and
  `/0040-a11y/as1_names` (a new suite `ui-0040-a11y`) failed first at `text_cells: 'GTK_IS_TOGGLE_BUTTON (*sharp) && ...' should be
  TRUE`; with `new_smooth_well (action)` (the Text page's well bound to `win.text-smooth`) and the handler also refusing during
  `easel_canvas_is_selecting_text` (risk 3) they pass. Two earlier tests checked what 0040 changes, each updated with its pointer note:
  `/0039/as1_well_shared` no longer lists Text as showing no well, and `/0017/as1_text_tool_and_options` counts 4 toggles, not 2.
  4 mutants killed (no well, the well bound to `win.smooth`, a choice taken while selecting, or while making a box); 0039's Brush mutant
  repointed (`--lint` 1444, 0 stale), killed. ui-0008, -0013, -0017, -0030, -0039, -0040 and its a11y suite pass.
  M1a (JOD, 2026-11-11): Text's panel, the two cells, the separate choices, a smooth word and an open box switched, light and dark: "ok".
- [x] T007 The highlight (REQ-0040-06; AS-10) - done when ui `/0040/as10_highlight` (rendered; within 1 a channel, risk 5) fails first
  and passes
  Result: ui `/0040/as10_highlight` ("Hi" at 48 points with Smooth, "H" selected, rendered: every whole pixel of the "H" within 2 of the
  letter's color under the tint, as Sharp; every partly covered one within 2 of the letter's color mixed by its coverage over the
  highlight's color, read as the commonest color of the uncovered pixels in the rows the "H" inks) failed first at `assert_near: (0xff664472
  == 0xff3a303d)` (the highlight over a whole pixel). Built in two steps: whole pixels left out of the highlight as Sharp's are; then the
  partial pixels, which first showed the letter twice (the box's drawing already mixes it over the ground, and a translucent letter laid
  again over the highlight darkened it to #1C1320), are drawn whole: the ground (the picture or an Opaque box's background), the 12 %
  tint and the 35 % highlight computed as GTK lays them, then the letter by `easel_cover_mix255`, within 2 of the render (risk 5 said 1;
  2 is the 0017 tests' bound). My test's first reference was wrong twice (the box's top corner, then rows below the line, which the
  highlight does not reach). 3 mutants killed (the highlight over whole pixels, the letter left out, the partial pixels left as the box
  draws them). ui-0017 (Sharp's highlight) and ui-0040 pass.

Group C, settings and words
- [x] T008 Preferences (REQ-0040-03; AS-2) - done when ui `/0040/as2_preferences` (Apply, Cancel, the panel writing nothing, the row after
  Strikethrough) fails first and passes
  Result: ui `/0040/as2_preferences` (`[text] smooth=true` and `TRUE` start Text at Smooth, 0039's choice at Sharp; `yes` and no key Sharp;
  the panel's choice writes nothing; "Smooth text" right after Strikethrough; on and Cancel writes nothing; on and Apply writes
  `smooth=true` under `[text]` and leaves the panel at Sharp) failed first at `pref_row: 'row' should not be NULL` (the start-up part
  passed already, from T005); the switch in the Text group passes it. 2 mutants killed (the start-up choice not read, which survived
  until this test, and another key). ui-0020, -0023, -0027 and -0040 pass.
- [x] T009 Strings and Help (REQ-0040-01, REQ-0040-03) - done when "Smooth text" is in the 11 catalogs (by hand, as they are kept), the
  template regenerated, Help's sentence names Text in 12 pages, `translations` passes and was seen red
  Result: the template regenerated ("Smooth text" added, "Undo memory" moved); `translations` seen red (`po/cs.po: missing: 'Smooth
  text'`, the 11 catalogs), then green with the string added by hand to each; Help's tools paragraph gains a sentence on Text's own choice
  and switch in the 12 pages, each naming Text and Preferences by its catalog's words; `docs/reference/translations-to-review.md` lists
  the string (the review parked by JOD). ui-0021 passes.
- [x] T010 Gate - done when the full suite and the scale 2 list pass through `tools/isolated.sh` with nothing built beside them,
  `tools/mutate.sh --core` is clean or its survivors are in Check, and manual M1: JOD types with Smooth (a word at 12 and 48 points,
  underlined, an emoji, Opaque and Transparent, selected), switches an open box, turns the Preferences switch and restarts
  Result (machine part, 2026-11-11): `meson test` (isolated) 140 of 140, 0 warnings; the scale 2 list with `ui-0040-scale2` 7 of 7;
  `tools/mutate.sh --core` 551 mutants, 548 killed, 2 survived (REQ-0037-02 and REQ-0038-03, equivalent since 0038's check), 1 accepted
  (209 s). M1 (JOD, 2026-11-11): smooth words at 12 and 48 points, underlined, an emoji, Opaque and
  Transparent, selected, an open box switched, the Preferences switch and a restart: "ok".

## Open questions
- None. Asked of JOD, 2026-11-11: the look (Smooth, hint full, from the sheet), the control (own Sharp | Smooth well), Preferences (own switch in Text), an open box (changes at once), the highlight (mixed over it). The layout's sameness checked on 40 font and size cases (`docs/reference/proto-0040/same_layout.py`).

## Check
- Date and commit: 2026-11-11, on `13204ae` with the uncommitted work of 0040.
- Tests: `meson test` (isolated) 140 of 140 at T010's gate and again after the check's fixes (below); the scale 2 list 7 of 7 both times; the cases of
  this feature: `text-smooth-layout` (3), core `mix255_oracle`, `ink_lay_layers`, `settings_text_smooth`, ui `/0040/*` (17),
  `/0040-scale2/as5_scale2`, `/0040-a11y/as1_names`.
- Mutants: `tools/mutate.sh --core` at T010: 551 mutants, 548 killed, 0 by crash, 0 inconclusive, 2 survived, 1 accepted (209 s); the
  0040 ui mutants run by REQ id during the tasks and the check, all killed; `--lint` 1450, 0 stale.
- Survivors, accepted (0038's check, unchanged): REQ-0037-02 "a fully transparent pixel comes back with a kept piece" and REQ-0038-03
  "the seam changes a stroke's cell", equivalent.
- Removed as equivalent (T004): a mutant turning on subpixel antialiasing: the coverage is read from a one-channel surface, which Cairo
  cannot draw colored fringes into; JOD's note a is kept by R = G = B on the final pixels (`smooth_ink_gray_and_same_letters`,
  `as3_gray_and_same_letters`), which a tint mutant fails.
- Warnings: 0. Lint and `--names 0040` clean.
- Scenarios: AS-1 `as1_own_choice`, `as1_keyboard`, `as1_ignored_while_held`, `as1_names` + M1a; AS-2 `settings_text_smooth` +
  `as2_preferences`; AS-3 `as3_gray_and_same_letters` + `smooth_ink_gray_and_same_letters`; AS-4 `as4_red_over_transparent`,
  `as4_opaque_box`; AS-5 `as5_switch_open_box`, `as5_moved_box`, `as5_overhang_shown` (the check's), `as5_scale2`; AS-6 `as6_no_memory`,
  `as6_switch_without_memory`; AS-7 `as7_half_alpha` + `ink_lay_layers`; AS-8 `as8_underline_joint`; AS-9 `as9_color_emoji` (ran: Noto
  Color Emoji installed) + M1; AS-10 `as10_highlight`, `as10_highlight_transparent` (the check's). Every task has its `Result:` with a red
  run; no characterization. M1 (JOD, T010): "ok".

Findings (a review agent read the diff and the spec blind; each finding checked in the code before it was settled):
- F1 (fixed): Smooth ink reaching past the box (an italic's overhang) was put down but not shown: the view widened to the ink only for
  Sharp's mask. `/0040/as5_overhang_shown` (an italic "Wf" at 48 points in a narrow box: every inked pixel outside the box shows the ink's
  value) failed first (`0xffffffff == 0xff000000`); the view widens for the layers too; a mutant kills it.
- F2 (fixed): a selected, partly covered pixel was drawn opaque, from a ground read without its alpha: on a transparent picture the
  checkerboard went dark under a selection. Now the box's drawing leaves such a pixel without its letter (`text_selected_pixels`, shared
  with the highlight), the highlight lies over it, and the letter's color is laid over at its coverage: the mix of REQ-0040-06 for an
  opaque pixel, the checkerboard showing through over a transparent one. `/0040/as10_highlight_transparent` (P = H (1 - c / 255) within 3
  beside an uncovered selected pixel) was written after the code, against the rule: its red run is the mutant that puts the letter
  back in the box's drawing (the letter twice), killed, with the mutant leaving the letter out.
- F3 (known gap): those pixels lie over the grid at 800 % with a selection: the grid shows faintly through the letter's color there.
- F4 (known gap): the highlight's letter color comes from the one layer covering a pixel most; where two style runs' soft edges overlap
  the selected view can differ by a little from the put-down (the put-down is right).
- F5 (known gap): a selected letter's soft edge reaching into the next letter's advance gets no highlight under it.
- F6 (known gap, probe at need): the smooth pass draws into Sharp's ink rectangle; a gray outline reaching a pixel past it would be cut.
  Not seen in the tests' fonts.
- F7 (known gap): `glyph_color` un-premultiplies pixels of a few 255ths of alpha, so a plain letter's faintest edge in a run with an emoji
  could take an odd color; the ARGB surface's status is unchecked (as in Sharp's path).
- Found in the tasks and fixed: a moved Smooth box left its layers behind (T005, `as5_moved_box`); a text selection drag was not "a button
  held" (risk 3, T006).
- Decided with JOD during the build: the cells' look, idea D (four magnified pixels) for 0039's and 0040's; REQ-0039-01 reworded.
- Not of this spec, done during the check at JOD's request: About Easel names the developer JOD and the website (REQ-0021-05, AS-5,
  `/0021/as5_about` red first, a mutant).

Known gaps: F3 to F7 (also in `docs/phase2-plan.md`).
