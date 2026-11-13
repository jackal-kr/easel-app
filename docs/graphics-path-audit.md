# Graphics path audit (Phase 2, before Gradient types)

2026-11-08, on `843562e`. A read of how the canvas draws the picture and what lies on it, at every zoom and screen
scale, so that Gradient types (`docs/phase2-plan.md` C) builds on a path whose cost and exactness are known. Named in
`docs/phase2-plan.md` (From 0025) and in the roadmap note of `docs/specs/INDEX.md`. Read-only: no code changed.

## The paths today (`src/ui/canvas.c`, `canvas_snapshot_frame`)

| Zoom, screen scale | The picture | A gradient or a moved selection over it |
|---|---|---|
| 100 % and up, whole zooms, scale 1 | Tiles of 256 × 256 picture pixels, each a texture over the picture's own memory (no copy), scaled by the renderer, NEAREST | Each tile that shows is replaced by a texture of the preview's pixels, made for the frame |
| Fractional zooms (150 %, 250 % …), and any zoom at scale 2 | Built by the view's edges (0025): only what shows, one device pixel each, from the tiles or the preview | The same, from the preview's textures |
| Below 100 %, scale 1 | The tiles, scaled down by the renderer, NEAREST: the renderer picks the picture pixel of each device pixel | **Every tile that shows gets a preview texture of all its picture pixels**, then the renderer scales it down |
| A fractional screen scale (125 %, 150 %) | As scale 1 with the division; never tried (0025's known gap) | Same |

Over the picture: the shape being dragged (runs of rectangles, one row in step below 100 %, so its cost is the view's),
the text box (its own pixels, the box's size), the float's part outside the picture, the tint, the outlines, the handles,
the grid, the markers, the keyboard marker. All of them are drawn after the picture, by the same `l` (logical pixels per
picture pixel).

## Findings

1. **The gradient and the selection previews cost the picture's size below 100 %, not the screen's** (high, the one the
   roadmap note names). `gradient_preview` and `selection_preview` make a full-resolution texture for every tile the view
   touches. At the fit zoom of an 8000 × 8000 picture that is every tile: 64 million pixels laid per frame for a view of
   about 1 million device pixels. Measured before (INDEX roadmap note, 0012, 0013): a gradient dragged at the fit zoom
   about 390 ms a frame with the picture's size held in memory; a selection moved at that zoom 165 to 173 ms a move and
   470 MB; the put-down of the whole picture 380 ms and 1.8 GB. Gradient types (Radial, Reflected) cost more per pixel than
   Linear, so this gets worse with C unless the path changes first.
2. **Below 100 % the picture's pixel for each device pixel is chosen by the renderer, and a preview's by the same
   renderer** (low, holds today). P7.3 (the preview is the put-down) holds because both go through one scaled texture
   per tile. A new path for previews must keep the same choice of pixel, or a preview frame and the frame after the
   put-down would show different pixels at the same zoom.
3. **Two rules choose the pixel of a device pixel**: the renderer's below 100 % and at whole zooms at scale 1, and
   `easel_view_cell_at` (the edges) at fractional zooms and at scale 2. They agree where both are tested; below 100 %
   only the renderer's is used, and the pointer maps by the division there (`easel_view_to_pixel`).
4. **`append_float_outside` builds a texture it does not use, and rebuilds the cells once per band** (low, known from
   0025): a small waste on a rare path.
5. **A fractional desktop scale is untried** (known gap, in `docs/phase2-plan.md` "Before the next release").

## Proposal

One view path below 100 %, as 0025 built one for fractional zooms: the canvas builds the view it shows, one device pixel
each, choosing each pixel by one rule (the edges' rule, `easel_view_cell_at`, extended below 100 %), and for each pixel
asks the picture, or the preview when one is shown, for that one pixel only (`compose_pixels` already gives the put-down's
pixels for any rectangle). The cost of a frame is then the view's, at every zoom; the preview and the picture go through
the same choice of pixel, so P7.3 holds by construction; the pointer maps by the same rule. A feature spec of its own,
before Gradient types, with the measurements above taken again as its acceptance (perf-0012 and perf-0013 at the fit zoom).

Out of it: the put-down's own cost (it writes every pixel, as it must), the fractional desktop scale (its own check).
