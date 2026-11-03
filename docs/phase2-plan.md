# Phase 2 plan: gaps carried over

Known gaps from Phase 1 specs, to take up after the core XP scope is delivered. The features of Phase 2 are in `docs/specs/INDEX.md`.

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

## From the mutation list
- REQ-0017-14, the popover's scroll guard: survives on GTK 4.24, accepted (JOD, 2026-11-02). Re-decide at packaging-release, when the minimum GTK is chosen.
