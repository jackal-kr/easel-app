/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "magnifier.h"

const int easel_magnifier_levels[EASEL_MAGNIFIER_N_LEVELS] = { 10, 25, 50, 100, 200, 400, 600, 800 };

int
easel_magnifier_zoom (int zoom, int level_pct)
{
  return zoom == level_pct ? 100 : level_pct;
}

/* One axis of the box: its size, and its start moved inside the picture. */
static void
box_axis (int area, int level_pct, int pic, int p, int *start, int *size)
{
  long len = (long) area * 100 / level_pct;

  if (len < 1)
    len = 1;
  if (len > pic)
    len = pic;
  *size = (int) len;
  *start = p - (int) (len / 2);
  if (*start > pic - *size)
    *start = pic - *size;
  if (*start < 0)
    *start = 0;
}

void
easel_magnifier_box (int area_w, int area_h, int level_pct, int pic_w, int pic_h, int px, int py, int *x, int *y,
                     int *w, int *h)
{
  box_axis (area_w, level_pct, pic_w, px, x, w);
  box_axis (area_h, level_pct, pic_h, py, y, h);
}
