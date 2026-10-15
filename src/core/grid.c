/* SPDX-License-Identifier: GPL-3.0-or-later */

#include <math.h>

#include "grid.h"

bool
easel_grid_visible (int cell, int zoom_pct)
{
  return (long) cell * zoom_pct >= 4 * 100;
}

/* The line y = m * x + b cut to the rectangle (x0, y0)-(x1, y1); nothing if it only touches a corner. */
static void
emit_diagonal (double m, double b, double x0, double y0, double x1, double y1, EaselGridLineFunc fn, void *data)
{
  double xa, xb;

  if (m > 0)
    {
      xa = fmax (x0, (y0 - b) / m);
      xb = fmin (x1, (y1 - b) / m);
    }
  else
    {
      /* Going right the line goes up, so it is at the bottom edge first. */
      xa = fmax (x0, (y1 - b) / m);
      xb = fmin (x1, (y0 - b) / m);
    }
  if (xb - xa > 0)
    fn (xa, m * xa + b, xb, m * xb + b, data);
}

void
easel_grid_lines (int w, int h, int cell, bool axonometric, double cx0, double cy0, double cx1, double cy1,
                  EaselGridLineFunc fn, void *data)
{
  /* Only what is inside the picture and inside the area asked for. */
  double x0 = fmax (0, cx0), y0 = fmax (0, cy0), x1 = fmin (w, cx1), y1 = fmin (h, cy1);

  if (cell < 1 || x0 >= x1 || y0 >= y1)
    return;

  /* Vertical lines at the positive multiples of cell inside the picture; its own edges get none. */
  for (double x = fmax (1, ceil (x0 / cell)) * cell; x <= x1 && x < w; x += cell)
    fn (x, y0, x, y1, data);

  if (!axonometric)
    {
      for (double y = fmax (1, ceil (y0 / cell)) * cell; y <= y1 && y < h; y += cell)
        fn (x0, y, x1, y, data);
      return;
    }

  /* y = x / 2 + k * cell crosses the area for the k whose b = k * cell lies in [y0 - x1 / 2, y1 - x0 / 2]. */
  for (double b = ceil ((y0 - x1 / 2) / cell) * cell; b <= y1 - x0 / 2; b += cell)
    emit_diagonal (0.5, b, x0, y0, x1, y1, fn, data);
  /* y = -x / 2 + k * cell: b in [y0 + x0 / 2, y1 + x1 / 2]. */
  for (double b = ceil ((y0 + x0 / 2) / cell) * cell; b <= y1 + x1 / 2; b += cell)
    emit_diagonal (-0.5, b, x0, y0, x1, y1, fn, data);
}
