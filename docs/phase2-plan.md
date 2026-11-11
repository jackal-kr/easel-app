# Phase 2 plan: gaps carried over

Known gaps from Phase 1 specs, to take up after the core XP scope is delivered. The features of Phase 2 are in `docs/specs/INDEX.md`.

Revision 2026-11-05: JOD's Phase 2 additions A to D (cursors, brush options, gradient types, Black and white), below, docs only, no spec yet.
Revision 2026-11-07: JOD's further list sorted into E (Curve) and F (Remove background); its icon and cursor items are A, done; its gradient item is C. Every item here has a row in `docs/specs/INDEX.md`. The checks before the first beta are gathered in their own section.

## JOD's Phase 2 additions (2026-11-05)
| Item | One line | Size |
|---|---|---|
| A. Tool cursors in one style | Every tool's cursor, and the toolbox icons out of style, drawn by one set of rules | S to M |
| B. Brush options that read at a glance | The twelve tips shown so their shape and size can be read | S |
| C. Gradient types | The Gradient tool gains Linear, Radial and Reflected | M |
| D. Black and white | The picture turned to pure black and white | (below) |
| E. Curve bent on the line | A bend made by dragging a point of the line, the curve finished by a click outside it | S to M |
| F. Remove background | The background of a picture made transparent | M |

Order suggestion: A and B first, C after the graphics path audit, D with file input and output; E and F once their open questions are settled.

### A. Tool cursors in one style (spec 0029, accepted 2026-11-06, implemented 2026-11-07; black and white for every cursor over the canvas)
- Later, optional: accent-colored cursors, as a Preferences setting (JOD, 2026-11-06; the mockup `docs/reference/0029/cursors-mockup-accents.png` shows the accents losing contrast on many-colored pictures).
- The cursors of Airbrush, Pencil, Fill and Pick Color are out of style next to the rest (JOD, 2026-11-05). A screencast does not show the pointer, so they are judged from the app's own images.
- First task of the spec: draw every cursor of every tool and state (Magnifier in and out, Pan open and closed, any busy or blocked one) onto one contact sheet from the code's own cursor images, at scale 1 and 2, on light and dark backgrounds, with the hotspot marked. JOD names the odd ones from that sheet.
- Then one set of rules: size, stroke weight, a light halo, the hotspot at the exact pixel the tool acts on, one source, scale-2 variants; and a test that every tool's cursor comes from the set, with its hotspot inside the image.
- The size-matched cursors of Eraser and Brush stay. Toolbox icons out of style go in the same spec.
- P7: the spec states how the hotspot maps to the picture pixel; the cursor draws nothing.

### B. Brush options that read at a glance (spec 0030, implemented 2026-11-08; JOD: the options look ugly, a redesign of every tool's options follows)
- The twelve tip glyphs are barely understood (JOD, 2026-11-05). The twelve tips stay.
- Either each tip shown at its real size on a neutral swatch, grouped by shape with the sizes in a row, with a tooltip naming shape and size in pixels and the chosen tip also in the status bar; or two controls, shape and size. JOD sees a text layout first.
- The options of Airbrush, Eraser and the line width are checked against the same reading rule.
- The effect on Preferences' Brush row (REQ-0020-05) is stated.

### C. Gradient types
- The Gradient tool (the drawing color faded to transparent along a drag; a click or Enter applies it, Esc cancels) gains three types as an option row: Linear; Radial, from the press point, the drag's length the radius; Reflected, mirrored on both sides of the press point. Default Linear, remembered for the run.
- Open product question for JOD, in the spec: keep "to transparent", or run from the foreground to the background color, perhaps as a second row "To transparent / To background".
- The preview equals the put-down (P7.3); exact colors with a defined rounding rule; integer or checked-table arithmetic, so the same input gives the same pixels on every machine. The spec states a drag of zero length, a drag past the picture, a fill inside a selection only, and a radial edge beyond the radius.
- The math lives in `src/core`, so `--core` covers it.
- Built as 0032 (2026-11-09; to transparent kept, a blend into the background tried and withdrawn by JOD). Known gaps, tests only: per-type ui tests of Shift, a click, a moved marker and a press outside; the cells' keyboard path; the reshow of a waiting gradient after its memory returns (0032 Check F1 to F3).

### D. Black and white (spec 0034, implemented 2026-11-09)
- Built 2026-11-09. Known gaps, tests only: a text box that blocks the dialog, a marked selection whose conversion fails, a free-form selection through the window, the dialog's strings in a translated session (0034 Check F1 to F4).
- JOD, 2026-11-09: an Image command, not XP's mode; Threshold, Ordered dither or Error diffusion in a dialog (Threshold first); one step, no warning; alpha kept, palette unchanged.
- Decided as Phase 2 on 2026-11-04; its notes and open question are in the section "Black and white" below.

## From 0025 (pixel contract tests)
- The tint, the outline, the box's frame and the picture's far-edge resize handles may lie up to 2 device pixels off the drawn cell edges at a fractional zoom (accepted, JOD, 2026-11-02). `/0025-open/overlay_sides_at_fractional_zoom` is the known-red record.
- A fractional desktop scale (125 %, 150 %) keeps the renderer's scaling and has no test: only scale 1 and scale 2 have been tried.
- A screen of scale 2 has been tried only in the headless session, never by eye.
- The frame times of fractional zooms are not reliably measured: a covered window gets no frame callbacks on Wayland. The CPU figures stand, and M1 was smooth by eye (JOD, 2026-11-02).
- `append_float_outside` builds a texture it does not use and rebuilds the cells once per band: a small waste on a rare path.
- `perf-0025` keeps its diagnostic modes (`fractional`, `pencil`, `pencil-log`).
- The rule below 100 % at scale 2 in `canvas.c` is proved by its red run and by the core rule's mutant: `tools/mutate.sh` sessions are of scale 1 and cannot run a scale 2 mutant.
- A graphics path audit was named for this phase: done 2026-11-08, `docs/graphics-path-audit.md`; it proposes one view path below 100 % before Gradient types.

## From 0019 (Stretch and Skew)
- Cases the code handles and no test of their own observes: Ctrl+T at the quit prompt and with the focus in the font card; the scroll kept after OK; a stretch back through the dialog, and a free-form or pasted selection skewed in the window (the core tests have them); a refused lift; AS-16 compares the pixels, not a saved file.
- The take-back of a stretch after a refused skew uses no memory limit and can itself fail, leaving the selection stretched with the error shown.

## From 0020 (Preferences)
- Cases the code handles and no test of their own observes: Ctrl+, at the quit prompt, over the color chooser and with the focus in the font card; the workspace colors actually drawn in each style (the keys are tested); JPEG quality reaching a saved file in the same run; a palette file that cannot be removed; the font row's search finding a family.
- The Font row does not show each family in its own font, as the font card does.

## From 0021 (Window Extras)
- Cases the code handles and no test of their own observes: a waiting gradient on the thumbnail; a failed put-down taking Esc; Paste From… cancelled and the oversize prompt in Pan; the quit prompt, the color chooser and an open menu as gates of Keyboard Shortcuts, Help and About Easel; no memory for the card's buffer; a press on the card released on the canvas; a theme change's workspace color on the card.
- Help under Flatpak: checked at packaging.

## From 0026 (Print)
- Cases the code handles and no test of their own observes: the document name, without the mark and " – Easel"; the quit prompt, the color chooser and an open menu as gates of Print…; a print dialog that cannot be shown; a quit during sending with a modified picture; AS-8 at another zoom and scroll (the job reads no view, by construction); the title's mark in AS-10 (the modified flag is what is tested).
- `run_print` handles GTK answering ERROR at the run with no `done`, and then keeps its references on the operation and the window. Whether GTK can answer so is unknown.
- The Picture tab under Flatpak: checked at packaging, with Sharp and Fit on one page as the fallback.

## From 0022 (Accessibility)
- Orca has not been run (JOD, 2026-11-04: not a priority): names and states are read only by GTK's test backend, announcements only from a test log. Whether Orca reads an open text box is unknown. To check with a real screen reader before release.
- The keyboard's mutants in `tests/mutants.txt` run at the phase gate, not per spec.
- Cases the code handles and no test of their own observes: "Opened NAME"; a gradient's marker taken on its own pixel by a keyboard press; the status bar's fields' names, which are their labels.
- A key before the marker was ever placed, when a mouse press outside the picture gave the canvas the focus: it starts at pixel (0, 0), not at the view's corner.
- A selection move refused by memory: the marker still moves.

## From 0023 (Languages)
- JOD's thorough reading of ru, uk and pl and their Help pages (JOD, 2026-11-04: later); then the mark and the date. The eight other drafts are read by nobody.
- No test of their own: the Language row in Russian; a test translation in the ui (core only); the announcements in the walk for English; the alerts raised by their real failures (built from their strings).
- The layout test runs only the long language, which takes de and ru: a wording long only in another language needs a one-off run over all of them. `easel.pot` going stale fails no test.

## From JOD's use, 2026-11-04
- Ctrl+Shift with an arrow (REQ-0013-10) leaves the hole of a plain move, a blank strip beside the selection at its first move. JOD wants it fixed, in phase 2: the 10-pixel key should not show that gap. The likely form is a moved copy, as Ctrl with a drag does (the original stays, no trail, still 10 pixels), which would leave the keyboard no plain 10-pixel move; settle it in a spec then.

## From 0035's opening test (2026-11-10)
- A marked selection should survive a cancelled quit: closing a modified picture, then Cancel on the Save prompt, loses the marking,
  since the quit ends the selection first (REQ-0013-11, the whole app; found by 0035's AS-14).

## From 0027 (Preferences buttons)
- No test of their own: Esc with a chooser, a popover or an error above Preferences; a quit with a color chooser open; a truly read-only settings file; a Tint change with a palette reset at OK; a palette file that cannot be removed; a missing settings folder; Style against a desktop theme that changes during the dialog.
- The new strings in the eleven translations are unread, as the rest.

## Black and white (JOD, 2026-11-04: moved from Phase 1)
- XP's behavior, as remembered and not checked against XP or jspaint: Image, Attributes has a Colors choice of "Black and white" or "Colors". Choosing Black and white warns that the color is lost and cannot be got back, then turns the picture to pure black and white, and the palette shows black, white and dithered patterns in place of colors. Going back to Colors does not bring the colors back.
- Open question, for its spec: the conversion. A threshold on brightness (at what value, and by which weights), or a dither (which one, and whether it is repeatable, for P7's "the same input gives the same pixels")? What happens to transparency, whether it is one undo step, and whether the palette changes or only the picture.

## From 0036's check (2026-11-11)
- A Magnifier zoom waiting for its allocation, then Ctrl+Wheel in the same frame, can anchor around a stale point (F5).
- One finger on a touch screen may pan the view by the scrolled window's kinetic scrolling (REQ-0036-03 says no pan): to try on a
  touch screen (F6, M2 not run).

## From the mutation list
- REQ-0017-14, the popover's scroll guard: survives on GTK 4.24, accepted (JOD, 2026-11-02). Re-decide at packaging-release, when the minimum GTK is chosen.

## JOD's further list (2026-11-07)
### E. Curve bent on the line (spec 0033, implemented 2026-11-09)
- Built 2026-11-09 (the peak under the pointer after JOD's M1). Known gaps, tests only: Esc and the other button while a press off the curve is held; a handle pressed over a waiting curve; the hit test without memory (0033 Check F1, F3, F5).
- JOD's steps: drag a straight line; press on a point of the line and drag to bend it there; optionally a second bend from another point (two at most); a click outside the curve finishes it.
- Today (REQ-0011-04, XP's way): a press anywhere starts the next bend, and the second bend puts the curve down.
- Decided (JOD, 2026-11-07): this replaces XP's way, no option; REQ-0011-04 changes in its spec, and `docs/product.md`'s "XP-faithful by default" gets this as a named exception. Left for the spec: what a press that is not on the line does, and what Enter and Esc do with one bend made.

### F. Remove background (spec 0035, revised 2026-11-09: the classical method of ADR-0007, no model; ADR-0006's IS-Net superseded)
- New, beyond XP Paint (a modern extra). Decided (JOD, 2026-11-07): Windows 11 Paint's one command; with a selection, only its area; one undo step; saving needs nothing new (PNG, TIFF and WebP keep the transparency, JPEG and BMP flatten it on white, REQ-0005-11).
- Revised (JOD, 2026-11-09): no model (its weights' licence and training data unclear, the permissive one over budget; `docs/reference/0035/`). The background found by its colors and edges, integers only, the same pixels on every machine; a dialog with one Tolerance that starts at an automatic value, a live preview, OK and Cancel; hard edges; the progress dialog and Cancel kept for the result. Limits (in the dialog's line and in Help): it does not recognise subjects; pale on pale, soft edges and line drawings need a lower Tolerance or the brushes of G.
- Built 2026-11-09, the model version first (commit 5c1cb53), then the classical one; JOD's M1 and M2 to come. The older menu mnemonics, fixed in every language with 0035's first check, are kept by `/0023-lang/as14_menu_mnemonics_differ`.

### G. "Keep this" and "Remove this" brushes for Remove Background (optional, later; JOD, 2026-11-09)
- Two brushes in Remove Background's card: what the user paints with "Keep this" becomes subject seeds, with "Remove this" background
  seeds, for the same watershed (ADR-0007), so a subject the colors alone miss (the stingray, line art) can be kept by a few strokes.
- First design notes (JOD, 2026-11-10):
  - Two toggle buttons in the card, "Keep this" and "Remove this", none on at the start; with one on, a drag on the picture paints a
    stroke of a fixed size in screen pixels (shown as a translucent band, green and red, both outlined for contrast) instead of the
    piece presses of Remove Background: pieces; with none on, presses flip pieces as before.
  - The strokes become the watershed's markers (ADR-0007): every copy cell a "Keep" stroke covers is a subject seed, every "Remove"
    cell a background seed, overriding the color seeds; the flood then runs as now, so one stroke across a subject the colors miss
    keeps all of it up to its edges. A cell under both: the later stroke wins.
  - Kept, like the presses, as picture pixels, so a Tolerance change keeps them; the preview after a stroke within 0.3 s at any size.
- Built 2026-11-10 (spec 0038). Open from its check (0038 Check F5 to F11): test cases for the secondary button, Enter, focus loss,
  resize, the wheel and Ctrl+Q during a stroke, a typed number, a pending preview, a stroke during OK, the bands at 400 % and a free-form
  shape; a mouse press joining a keyboard stroke; Ctrl+Z's memory failure ignored; the selection's shape inside a copy cell over
  2 000 000 pixels; `covers` overflow far outside; a drag's band repaint at 8000 x 8000; joined cells first-come and one outline for
  overlapping strokes of a kind.
  - No undo step inside the card; Cancel drops them, OK keeps the result as one step. Open: an eraser for strokes, the stroke's size
    control, and the keyboard path (the marker painting with Space held).

### H. Soft edges for Remove Background (optional, later; JOD, 2026-11-10)
- Partial transparency from the color-mixing estimate of the fringe (REQ-0035-03 g): a boundary pixel's alpha from its share of the
  subject, instead of kept or removed whole. Off by default (a setting in the card); P7.1 would then need its own wording.

### I. A texture channel for Remove Background (optional, later; JOD, 2026-11-10)
- Besides color, a local texture measure (how much the color varies around a cell) in the edge strength and the distance to the
  background, so a textured ground (grass, gravel) is told from a smooth subject without a high Tolerance (0037's horizon prototype:
  the ground stayed mostly for its texture).

### Done or merged
- Toolbox icons and selection icons more consistent and readable: A, spec 0029, implemented 2026-11-07.
- Mouse cursors consistent (Magnifier, Pick Color, Fill): A, spec 0029, implemented 2026-11-07.
- Gradient with Linear, Radial and Reflected: C.

## Before the first beta
Gathered from the sections above; none has written steps yet.
- Orca on the app, an open text box included (From 0022).
- A fractional desktop scale (125 %, 150 %) and a scale 2 screen by eye (From 0025, 0029's edge case).
- JOD's reading of ru, uk and pl and their Help pages (From 0023).
- REQ-0017-14's scroll guard re-decided with the minimum GTK (From the mutation list; in 0024's row).
