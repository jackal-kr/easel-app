/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "cursor-shape.h"

#define MIN_OUTLINE 5

/* The side of the black outline in device pixels, and whether it is the plain-square minimum. */
static int
outline_side (EaselFootprint f, int zoom, int scale, bool *plain)
{
  long side = ((long) f.size * zoom * scale + 50) / 100;

  *plain = side < MIN_OUTLINE;
  return *plain ? MIN_OUTLINE : (int) side;
}

/* Is the device pixel (x, y) of the outline's box part of the footprint? */
static bool
covered (EaselFootprint f, int zoom, int scale, int side, bool plain, int x, int y)
{
  if (x < 0 || y < 0 || x >= side || y >= side)
    return false;
  if (plain)
    return true;
  return easel_footprint_has (f, (int) ((long) x * 100 / ((long) zoom * scale)), (int) ((long) y * 100 / ((long) zoom * scale)));
}

static bool
on_boundary (EaselFootprint f, int zoom, int scale, int side, bool plain, int x, int y)
{
  return covered (f, zoom, scale, side, plain, x, y)
         && (!covered (f, zoom, scale, side, plain, x + 1, y) || !covered (f, zoom, scale, side, plain, x - 1, y)
             || !covered (f, zoom, scale, side, plain, x, y + 1) || !covered (f, zoom, scale, side, plain, x, y - 1));
}

EaselCursorShape
easel_cursor_shape (EaselFootprint f, int zoom, int scale)
{
  bool plain;
  int side = outline_side (f, zoom, scale, &plain);
  EaselCursorShape s = { side + 2, side + 2, 0, 0 };
  int hot = plain ? side / 2 : (int) ((long) (2 * easel_footprint_anchor (f.size) + 1) * zoom * scale / 200);

  s.hot_x = s.hot_y = 1 + hot;
  return s;
}

void
easel_cursor_render (EaselFootprint f, int zoom, int scale, uint32_t *argb)
{
  bool plain;
  int side = outline_side (f, zoom, scale, &plain);

  for (int y = 0; y < side + 2; y++)
    for (int x = 0; x < side + 2; x++)
      {
        int bx = x - 1, by = y - 1;
        uint32_t c = 0;

        if (on_boundary (f, zoom, scale, side, plain, bx, by))
          c = 0xFF000000u;
        else if (!covered (f, zoom, scale, side, plain, bx, by))
          for (int dy = -1; dy <= 1; dy++)
            for (int dx = -1; dx <= 1; dx++)
              if ((dx != 0 || dy != 0) && on_boundary (f, zoom, scale, side, plain, bx + dx, by + dy))
                c = 0xFFFFFFFFu;
        argb[y * (side + 2) + x] = c;
      }
}
