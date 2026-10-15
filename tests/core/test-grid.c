/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Grid scenarios of feature 0004 (REQ-0004-12, REQ-0004-14). */

#include <math.h>
#include <string.h>

#include "check.h"
#include "grid.h"

typedef struct
{
  double seg[64][4];
  int n;
} Lines;

static void
collect (double x0, double y0, double x1, double y1, void *data)
{
  Lines *l = data;

  CHECK (l->n < 64);
  l->seg[l->n][0] = x0;
  l->seg[l->n][1] = y0;
  l->seg[l->n][2] = x1;
  l->seg[l->n][3] = y1;
  l->n++;
}

/* Whether the segment (x0,y0)-(x1,y1), in either direction, was emitted. */
static int
has (const Lines *l, double x0, double y0, double x1, double y1)
{
  for (int i = 0; i < l->n; i++)
    {
      const double *s = l->seg[i];

      if ((s[0] == x0 && s[1] == y0 && s[2] == x1 && s[3] == y1) || (s[0] == x1 && s[1] == y1 && s[2] == x0 && s[3] == y0))
        return 1;
    }
  return 0;
}

static int
count_counted (double x0, double y0, double x1, double y1, int w, int h, int cell, int axon)
{
  Lines l = { .n = 0 };

  easel_grid_lines (w, h, cell, axon, x0, y0, x1, y1, collect, &l);
  return l.n;
}

static void
counter (double a, double b, double c, double d, void *data)
{
  ++*(int *) data;
}

/* REQ-0004-12: a cell must be at least 4 device pixels wide on screen. */
static void
as10_grid_visibility (void)
{
  CHECK (easel_grid_visible (1, 400));
  CHECK (easel_grid_visible (1, 800));
  CHECK (!easel_grid_visible (1, 300));
  CHECK (!easel_grid_visible (1, 100));
  CHECK (!easel_grid_visible (1, 10));
  CHECK (easel_grid_visible (2, 200));       /* 2 * 200 % = 4 */
  CHECK (!easel_grid_visible (2, 100));
  CHECK (easel_grid_visible (4, 100));
  CHECK (!easel_grid_visible (3, 100));
  CHECK (easel_grid_visible (1000, 10));     /* 1000 * 10 % = 100 */
}

static void
as10_lines_inside_the_picture (void)
{
  Lines l = { .n = 0 };

  /* 10 x 6 picture, cell 2: vertical lines at x = 2, 4, 6, 8; horizontal lines at y = 2, 4; none on the edges. */
  easel_grid_lines (10, 6, 2, 0, 0, 0, 10, 6, collect, &l);
  CHECK (l.n == 6);
  for (int x = 2; x <= 8; x += 2)
    CHECK (has (&l, x, 0, x, 6));
  for (int y = 2; y <= 4; y += 2)
    CHECK (has (&l, 0, y, 10, y));
  CHECK (!has (&l, 0, 0, 0, 6) && !has (&l, 10, 0, 10, 6) && !has (&l, 0, 0, 10, 0) && !has (&l, 0, 6, 10, 6));

  /* Cell 1 on a 4 x 3 picture: x = 1, 2, 3 and y = 1, 2. */
  CHECK (count_counted (0, 0, 4, 3, 4, 3, 1, 0) == 5);
  /* A cell as large as, or larger than, the picture: no lines at all. */
  CHECK (count_counted (0, 0, 10, 6, 10, 6, 10, 0) == 0);       /* x = 10 and y = 10 are not inside the picture */
  CHECK (count_counted (0, 0, 10, 6, 10, 6, 11, 0) == 0);
}

/* REQ-0004-12: cell 16 shows every 16 picture pixels. */
static void
as11_grid_cell_16 (void)
{
  Lines l = { .n = 0 };

  easel_grid_lines (50, 40, 16, 0, 0, 0, 50, 40, collect, &l);
  CHECK (l.n == 3 + 2);
  CHECK (has (&l, 16, 0, 16, 40) && has (&l, 32, 0, 32, 40) && has (&l, 48, 0, 48, 40));
  CHECK (has (&l, 0, 16, 50, 16) && has (&l, 0, 32, 50, 32));
  CHECK (easel_grid_visible (16, 25));      /* 4 device pixels apart at 25 % */
  CHECK (!easel_grid_visible (16, 10));
}

/* REQ-0004-14: vertical lines stay; diagonals at 1 down per 2 across replace the horizontals. */
static void
as13_axonometric_lines (void)
{
  Lines l = { .n = 0 };

  easel_grid_lines (12, 8, 4, 1, 0, 0, 12, 8, collect, &l);
  CHECK (l.n == 8);
  /* The vertical lines. */
  CHECK (has (&l, 4, 0, 4, 8) && has (&l, 8, 0, 8, 8));
  /* y = x / 2 + 4k, clipped to the picture. */
  CHECK (has (&l, 8, 0, 12, 2));
  CHECK (has (&l, 0, 0, 12, 6));
  CHECK (has (&l, 0, 4, 8, 8));
  /* y = -x / 2 + 4k, clipped to the picture. */
  CHECK (has (&l, 0, 4, 8, 0));
  CHECK (has (&l, 0, 8, 12, 2));
  CHECK (has (&l, 8, 8, 12, 6));
  /* No horizontal line. */
  for (int i = 0; i < l.n; i++)
    CHECK (!(l.seg[i][1] == l.seg[i][3] && l.seg[i][0] != l.seg[i][2]));
  /* Every segment lies inside the picture and has the slope 1/2, -1/2 or is vertical. */
  for (int i = 0; i < l.n; i++)
    {
      double dx = l.seg[i][2] - l.seg[i][0], dy = l.seg[i][3] - l.seg[i][1];

      CHECK (l.seg[i][0] >= 0 && l.seg[i][2] <= 12 && l.seg[i][1] >= 0 && l.seg[i][3] <= 8);
      CHECK (dx == 0 || fabs (dy / dx) == 0.5);
    }
}

/* Only the lines that reach the visible area are produced: an 8000 x 8000 picture at cell 1 must not walk 16,000 lines. */
static void
grid_lines_are_limited_to_the_visible_area (void)
{
  int n = 0;
  Lines l = { .n = 0 };

  easel_grid_lines (8000, 8000, 1, 0, 100, 200, 200, 260, counter, &n);
  CHECK (n >= 100 + 60 - 2 && n <= 100 + 60 + 2);                /* about 100 vertical + 60 horizontal */
  n = 0;
  easel_grid_lines (8000, 8000, 1, 1, 100, 200, 200, 260, counter, &n);
  CHECK (n > 100 && n < 400);                                    /* the verticals and the diagonals that cross it */

  /* The segments are cut to the visible area as well. */
  easel_grid_lines (8000, 8000, 50, 0, 90, 90, 160, 160, collect, &l);
  CHECK (has (&l, 100, 90, 100, 160) && has (&l, 150, 90, 150, 160));
  CHECK (has (&l, 90, 100, 160, 100) && has (&l, 90, 150, 160, 150));
  CHECK (l.n == 4);

  /* An empty or inverted area, and an area outside the picture, give nothing. */
  CHECK (count_counted (10, 10, 10, 10, 100, 100, 1, 0) == 0);
  CHECK (count_counted (50, 50, 20, 20, 100, 100, 1, 0) == 0);
  CHECK (count_counted (200, 200, 300, 300, 100, 100, 1, 0) == 0);
  CHECK (count_counted (0, 0, 100, 100, 100, 100, 0, 0) == 0);        /* a cell of 0 is not a grid */
}

int
main (void)
{
  RUN (as10_grid_visibility);
  RUN (as10_lines_inside_the_picture);
  RUN (as11_grid_cell_16);
  RUN (as13_axonometric_lines);
  RUN (grid_lines_are_limited_to_the_visible_area);
  return 0;
}
