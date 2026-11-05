/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "line.h"

#include <stdlib.h>

void
easel_line_init (EaselLine *l, int x0, int y0, int x1, int y1)
{
  l->x = x0;
  l->y = y0;
  l->x1 = x1;
  l->y1 = y1;
  l->dx = abs (x1 - x0);
  l->dy = -abs (y1 - y0);
  l->sx = x0 < x1 ? 1 : -1;
  l->sy = y0 < y1 ? 1 : -1;
  l->err = (long long) l->dx + l->dy;
  l->done = false;
}

bool
easel_line_next (EaselLine *l, int *x, int *y)
{
  long long e2;

  if (l->done)
    return false;
  *x = l->x;
  *y = l->y;
  if (l->x == l->x1 && l->y == l->y1)
    {
      l->done = true;
      return true;
    }
  e2 = 2 * l->err;
  if (e2 >= l->dy)
    {
      l->err += l->dy;
      l->x += l->sx;
    }
  if (e2 <= l->dx)
    {
      l->err += l->dx;
      l->y += l->sy;
    }
  return true;
}
