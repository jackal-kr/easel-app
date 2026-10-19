/* SPDX-License-Identifier: GPL-3.0-or-later */
/* The polygon of feature 0011 (REQ-0011-08): its outline, its region by the alternate rule, and the fill styles. */

#include <stdlib.h>
#include <string.h>

#include "check.h"
#include "figure.h"
#include "history.h"
#include "image.h"

#define WHITE 0xFFFFFFFFu
#define BLACK 0xFF000000u
#define RED 0xFFFF0000u
#define W 100
#define H 100

static EaselFigure
polygon (const int *pts, size_t n, bool closed, int width, EaselFigureStyle style)
{
  return (EaselFigure) { EASEL_FIGURE_POLYGON, 0, 0, 0, 0, width, style, NULL, pts, n, closed };
}

/* Draws the figure on a white W x H picture, with BLACK as the drawing color and RED as the other; returns the picture. */
static EaselImage *
draw (const EaselFigure *f)
{
  EaselImage *img = easel_image_new (W, H, WHITE);

  CHECK (img != NULL);
  easel_figure_apply (img, NULL, f, BLACK, RED, NULL);
  return img;
}

static int
count (const EaselImage *img, uint32_t color)
{
  int n = 0;

  for (int i = 0; i < W * H; i++)
    n += img->px[i] == color;
  return n;
}

static void
as5_triangle_fill_styles (void)
{
  static const int tri[6] = { 10, 10, 30, 10, 10, 30 };
  EaselFigure f = polygon (tri, 3, true, 1, EASEL_FIGURE_OUTLINE);
  EaselImage *img = draw (&f);

  CHECK (count (img, BLACK) == 60 && count (img, WHITE) == W * H - 60);
  easel_image_free (img);
  f = polygon (tri, 3, true, 1, EASEL_FIGURE_OUTLINE_AND_FILL);
  img = draw (&f);
  CHECK (count (img, BLACK) == 60 && count (img, RED) == 171);
  easel_image_free (img);
  f = polygon (tri, 3, true, 1, EASEL_FIGURE_FILL_ONLY);
  img = draw (&f);
  CHECK (count (img, BLACK) == 231 && count (img, RED) == 0);
  easel_image_free (img);
  f = polygon (tri, 3, true, 3, EASEL_FIGURE_OUTLINE);
  img = draw (&f);
  CHECK (count (img, BLACK) == 214);
  easel_image_free (img);
  /* Not closed: only the two sides placed, no fill, whatever the style. */
  f = polygon (tri, 3, false, 1, EASEL_FIGURE_FILL_ONLY);
  img = draw (&f);
  CHECK (count (img, BLACK) == 41 && count (img, RED) == 0);
  easel_image_free (img);
  /* Fewer than three distinct corners: no inside. */
  f = polygon (tri, 2, true, 1, EASEL_FIGURE_OUTLINE_AND_FILL);
  img = draw (&f);
  CHECK (count (img, BLACK) == 21 && count (img, RED) == 0);
  easel_image_free (img);
}

static void
as7_star_alternate (void)
{
  static const int star[10] = { 50, 10, 62, 46, 31, 24, 69, 24, 38, 46 };
  EaselFigure f = polygon (star, 5, true, 1, EASEL_FIGURE_FILL_ONLY);
  EaselImage *img = draw (&f);

  CHECK (count (img, BLACK) == 396);
  CHECK (easel_image_get (img, 50, 30) == WHITE);          /* the center pentagon is not filled */
  CHECK (easel_image_get (img, 50, 15) == BLACK);          /* a point is */
  easel_image_free (img);
}

/* ---- REQ-0011-08, in the test's own words ---- */

/* Is the center (x, y) on the side from (x0, y0) to (x1, y1)? */
static bool
on_side (int x0, int y0, int x1, int y1, int x, int y)
{
  long long cross = (long long) (x1 - x0) * (y - y0) - (long long) (y1 - y0) * (x - x0);

  return cross == 0 && x >= (x0 < x1 ? x0 : x1) && x <= (x0 < x1 ? x1 : x0) && y >= (y0 < y1 ? y0 : y1)
         && y <= (y0 < y1 ? y1 : y0);
}

/* Is the center (x, y) in the region of the polygon: on a side, or with an odd number of crossing sides to its right? */
static bool
in_region (const int *p, size_t n, int x, int y)
{
  int crossings = 0;

  for (size_t i = 0; i < n; i++)
    {
      int x0 = p[2 * i], y0 = p[2 * i + 1], x1 = p[2 * ((i + 1) % n)], y1 = p[2 * ((i + 1) % n) + 1];
      long long lhs, rhs;

      if (on_side (x0, y0, x1, y1, x, y))
        return true;
      if ((y0 > y) == (y1 > y))
        continue;
      /* x < the side's x at this row, by cross-multiplication in exact integers */
      lhs = (long long) (x - x0) * (y1 - y0);
      rhs = (long long) (y - y0) * (x1 - x0);
      if (y1 > y0 ? lhs < rhs : lhs > rhs)
        crossings++;
    }
  return crossings % 2 == 1;
}

static void
as5_polygon_matches_the_rule (void)
{
  unsigned int seed = 31;

  for (int round = 0; round < 300; round++)
    {
      int pts[80], w = 1 + round % 5, n = 2 + round % 39;
      EaselFigureStyle style = (EaselFigureStyle) (round % 3);
      bool closed = round % 5 != 0;
      static unsigned char outline[W * H];
      EaselFigure f;
      EaselImage *img;

      for (int k = 0; k < 2 * n; k++)
        {
          seed = seed * 1103515245u + 12345u;
          pts[k] = (int) ((seed >> 8) % 100);
        }
      memset (outline, 0, sizeof outline);
      for (int i = 0; i < (closed ? n : n - 1); i++)
        {
          int x0 = pts[2 * i], y0 = pts[2 * i + 1], x1 = pts[2 * ((i + 1) % n)], y1 = pts[2 * ((i + 1) % n) + 1];
          int dx = abs (x1 - x0), dy = -abs (y1 - y0), sx = x0 < x1 ? 1 : -1, sy = y0 < y1 ? 1 : -1, err = dx + dy;

          for (;;)
            {
              int e2 = 2 * err, a = (w - 1) / 2;

              for (int sy2 = 0; sy2 < w; sy2++)
                for (int sx2 = 0; sx2 < w; sx2++)
                  {
                    int x = x0 - a + sx2, y = y0 - a + sy2;

                    if (x >= 0 && x < W && y >= 0 && y < H)
                      outline[y * W + x] = 1;
                  }
              if (x0 == x1 && y0 == y1)
                break;
              if (e2 >= dy)
                {
                  err += dy;
                  x0 += sx;
                }
              if (e2 <= dx)
                {
                  err += dx;
                  y0 += sy;
                }
            }
        }
      f = polygon (pts, (size_t) n, closed, w, style);
      img = draw (&f);
      for (int y = 0; y < H; y++)
        for (int x = 0; x < W; x++)
          {
            bool region = closed && in_region (pts, (size_t) n, x, y);
            uint32_t want = WHITE;

            if (outline[y * W + x])
              want = BLACK;
            else if (region && style == EASEL_FIGURE_OUTLINE_AND_FILL)
              want = RED;
            else if (region && style == EASEL_FIGURE_FILL_ONLY)
              want = BLACK;
            CHECK (easel_image_get (img, x, y) == want);
          }
      easel_image_free (img);
    }
}

int
main (void)
{
  RUN (as5_triangle_fill_styles);
  RUN (as7_star_alternate);
  RUN (as5_polygon_matches_the_rule);
  return 0;
}
