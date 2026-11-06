# Phase 2 plan: gaps carried over

Known gaps from Phase 1 specs, to take up after the core XP scope is delivered. The features of Phase 2 are in `docs/specs/INDEX.md`.

Revision 2026-11-05: JOD's Phase 2 additions A to D (cursors, brush options, gradient types, Black and white), below, docs only, no spec yet.

## JOD's Phase 2 additions (2026-11-05)
| Item | One line | Size |
|---|---|---|
| A. Tool cursors in one style | Every tool's cursor, and the toolbox icons out of style, drawn by one set of rules | S to M |
| B. Brush options that read at a glance | The twelve tips shown so their shape and size can be read | S |
| C. Gradient types | The Gradient tool gains Linear, Radial and Reflected | M |
| D. Black and white | The picture turned to pure black and white | (below) |

Order suggestion: A and B first, C after the graphics path audit, D with file input and output.

### A. Tool cursors in one style (spec 0029, drafted 2026-11-06)
- The cursors of Airbrush, Pencil, Fill and Pick Color are out of style next to the rest (JOD, 2026-11-05). A screencast does not show the pointer, so they are judged from the app's own images.
- First task of the spec: draw every cursor of every tool and state (Magnifier in and out, Pan open and closed, any busy or blocked one) onto one contact sheet from the code's own cursor images, at scale 1 and 2, on light and dark backgrounds, with the hotspot marked. JOD names the odd ones from that sheet.
- Then one set of rules: size, stroke weight, a light halo, the hotspot at the exact pixel the tool acts on, one source, scale-2 variants; and a test that every tool's cursor comes from the set, with its hotspot inside the image.
- The size-matched cursors of Eraser and Brush stay. Toolbox icons out of style go in the same spec.
- P7: the spec states how the hotspot maps to the picture pixel; the cursor draws nothing.

### B. Brush options that read at a glance
- The twelve tip glyphs are barely understood (JOD, 2026-11-05). The twelve tips stay.
- Either each tip shown at its real size on a neutral swatch, grouped by shape with the sizes in a row, with a tooltip naming shape and size in pixels and the chosen tip also in the status bar; or two controls, shape and size. JOD sees a text layout first.
- The options of Airbrush, Eraser and the line width are checked against the same reading rule.
- The effect on Preferences' Brush row (REQ-0020-05) is stated.

### C. Gradient types
- The Gradient tool (the drawing color faded to transparent along a drag; a click or Enter applies it, Esc cancels) gains three types as an option row: Linear; Radial, from the press point, the drag's length the radius; Reflected, mirrored on both sides of the press point. Default Linear, remembered for the run.
- Open product question for JOD, in the spec: keep "to transparent", or run from the foreground to the background color, perhaps as a second row "To transparent / To background".
- The preview equals the put-down (P7.3); exact colors with a defined rounding rule; integer or checked-table arithmetic, so the same input gives the same pixels on every machine. The spec states a drag of zero length, a drag past the picture, a fill inside a selection only, and a radial edge beyond the radius.
- The math lives in `src/core`, so `--core` covers it.

### D. Black and white
- Decided as Phase 2 on 2026-11-04; its notes and open question are in the section "Black and white" below.

## From 0025 (pixel contract tests)
- The tint, the outline, the box's frame and the picture's far-edge resize handles may lie up to 2 device pixels off the drawn cell edges at a fractional zoom (accepted, JOD, 2026-11-02). `/0025-open/overlay_sides_at_fractional_zoom` is the known-red record.
- A fractional desktop scale (125 %, 150 %) keeps the renderer's scaling and has no test: only scale 1 and scale 2 have been tried.
- A screen of scale 2 has been tried only in the headless session, never by eye.
- The frame times of fractional zooms are not reliably measured: a covered window gets no frame callbacks on Wayland. The CPU figures stand, and M1 was smooth by eye (JOD, 2026-11-02).
- `append_float_outside` builds a texture it does not use and rebuilds the cells once per band: a small waste on a rare path.
- `perf-0025` keeps its diagnostic modes (`fractional`, `pencil`, `pencil-log`).
- The rule below 100 % at scale 2 in `canvas.c` is proved by its red run and by the core rule's mutant: `tools/mutate.sh` sessions are of scale 1 and cannot run a scale 2 mutant.
- A graphics path audit was named for this phase; it has no document yet.

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

## From 0027 (Preferences buttons)
- No test of their own: Esc with a chooser, a popover or an error above Preferences; a quit with a color chooser open; a truly read-only settings file; a Tint change with a palette reset at OK; a palette file that cannot be removed; a missing settings folder; Style against a desktop theme that changes during the dialog.
- The new strings in the eleven translations are unread, as the rest.

## Black and white (JOD, 2026-11-04: moved from Phase 1)
- XP's behavior, as remembered and not checked against XP or jspaint: Image, Attributes has a Colors choice of "Black and white" or "Colors". Choosing Black and white warns that the color is lost and cannot be got back, then turns the picture to pure black and white, and the palette shows black, white and dithered patterns in place of colors. Going back to Colors does not bring the colors back.
- Open question, for its spec: the conversion. A threshold on brightness (at what value, and by which weights), or a dither (which one, and whether it is repeatable, for P7's "the same input gives the same pixels")? What happens to transparency, whether it is one undo step, and whether the palette changes or only the picture.

## From the mutation list
- REQ-0017-14, the popover's scroll guard: survives on GTK 4.24, accepted (JOD, 2026-11-02). Re-decide at packaging-release, when the minimum GTK is chosen.

## Other improvements
 - New functionlaity: Remove background of a picture.
 - Improvement: Drawing a curve line to be improved, see below the step-by-step instructions
	1. Select the Curve Tool: Go to the Home tab and click on the Curve icon (which looks like a wavy line) in the Shapes section.
	2. Draw a Straight Base Line: Click on your canvas, drag the mouse to draw a straight line, and then release the mouse button.
	3. Create the First Bend: Click anywhere on the straight line you just drew, hold down the left mouse button, and drag the pointer (upwards or downwards) to bend the line into a curve, then release the mouse button.
	4. Create a Second Bend (Optional): Click on a different part of the line and drag it again to add a second curve (Note: each line allows a maximum of two bends).
	5. Finalize the Curve: Click anywhere outside the shape to lock and finish your curve.	
- Improvement: Make toolbox icons / selection icons more consistent, readable, and user-friedly (ask for screenshots)
- Improvement: Mouse cursors consitancy - focus on Magnifier, Pick Color, Fill, modes... make them more consistent and nicer looking (ask for screenshots)
- Improvement: Gradient tool should have three options: Linear, Radial, Reflected (provide screenshots)

