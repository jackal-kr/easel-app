/* SPDX-License-Identifier: GPL-3.0-or-later */
/* The pixel grid: when it is shown and where its lines are (REQ-0004-12, REQ-0004-14). */
#pragma once

#include <stdbool.h>

/* The grid shows only while one cell is at least 4 device pixels wide on screen: cell * zoom_pct / 100 >= 4. Zoom is
   in percent of device pixels, so the 1-pixel grid first shows at 400 %. */
bool easel_grid_visible (int cell, int zoom_pct);

/* Called for each line segment, in picture coordinates (x to the right, y down, in picture pixels). */
typedef void (*EaselGridLineFunc) (double x0, double y0, double x1, double y1, void *data);

/* The lines of a w x h picture with the given cell size, that reach the area (cx0, cy0)-(cx1, cy1) (picture coordinates;
   pass 0, 0, w, h for all of them), cut to the picture and to that area.
   - Vertical lines at every positive multiple of cell inside the picture; the picture's own edges get none.
   - Not axonometric: horizontal lines likewise.
   - Axonometric: instead of the horizontals, the lines y = x / 2 + k * cell and y = -x / 2 + k * cell for every whole k.
   Zero-length pieces are not produced. A cell below 1 gives no lines. */
void easel_grid_lines (int w, int h, int cell, bool axonometric, double cx0, double cy0, double cx1, double cy1,
                       EaselGridLineFunc fn, void *data);
