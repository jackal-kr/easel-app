/* SPDX-License-Identifier: GPL-3.0-or-later */
#include <limits.h>
#include <math.h>
#include <stdint.h>
#include <stdlib.h>

#include "figure.h"

#define COORD_LIMIT 1000000
#define ROUND_RADIUS 8

static int
clamp_coord (int v)
{
  return v < -COORD_LIMIT ? -COORD_LIMIT : v > COORD_LIMIT ? COORD_LIMIT : v;
}

/* A box of pixels: its left and top pixel, its size. */
typedef struct
{
  int x, y, w, h;
  int r;                   /* the corner radius of a rounded rectangle */
} Box;

/* The Line's rows: for each picture row from first_row, the leftmost and rightmost pixel of the 8-connected line in it. */
typedef struct
{
  int first_row, n_rows;
  int *xmin, *xmax;        /* n_rows each; xmin > xmax for a row the line does not touch */
} LineRows;

/* Bresenham, as the Pencil draws between two pointer positions (REQ-0001-20): 8-connected, one pixel per step along the
   major axis, both ends included. The same tie-breaking as `line` in stroke.c, which a core test checks pixel by pixel. */
static void
line_rows_fill (LineRows *r, int x0, int y0, int x1, int y1)
{
  int dx = x1 > x0 ? x1 - x0 : x0 - x1, dy = -(y1 > y0 ? y1 - y0 : y0 - y1);
  int sx = x0 < x1 ? 1 : -1, sy = y0 < y1 ? 1 : -1;
  long long err = (long long) dx + dy;

  for (;;)
    {
      long long e2 = 2 * err;

      if (y0 >= r->first_row && y0 < r->first_row + r->n_rows)
        {
          int k = y0 - r->first_row;

          if (x0 < r->xmin[k])
            r->xmin[k] = x0;
          if (x0 > r->xmax[k])
            r->xmax[k] = x0;
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

bool
easel_figure_prepare (EaselFigure *f, int y_from, int y_to)
{
  LineRows *r;
  int reach = f->width;

  f->rows = NULL;
  if (f->kind != EASEL_FIGURE_LINE)
    return true;
  r = malloc (sizeof *r);
  if (r == NULL)
    return false;
  r->first_row = y_from - reach;
  r->n_rows = y_to - y_from + 1 + 2 * reach;
  if (r->n_rows < 1)
    r->n_rows = 1;
  r->xmin = malloc ((size_t) r->n_rows * sizeof *r->xmin);
  r->xmax = malloc ((size_t) r->n_rows * sizeof *r->xmax);
  if (r->xmin == NULL || r->xmax == NULL)
    {
      free (r->xmin);
      free (r->xmax);
      free (r);
      return false;
    }
  for (int k = 0; k < r->n_rows; k++)
    {
      r->xmin[k] = INT_MAX;
      r->xmax[k] = INT_MIN;
    }
  line_rows_fill (r, clamp_coord (f->x0), clamp_coord (f->y0), clamp_coord (f->x1), clamp_coord (f->y1));
  f->rows = r;
  return true;
}

void
easel_figure_release (EaselFigure *f)
{
  LineRows *r = f->rows;

  if (r != NULL)
    {
      free (r->xmin);
      free (r->xmax);
      free (r);
    }
  f->rows = NULL;
}

/* The row y of a Line: the pen is the w x w square stamped on every pixel of the line, the line's pixel at column and row
   (w - 1) / 2 of it (REQ-0010-10), so the row y is touched by the line's rows y + a - w + 1 to y + a, a = (w - 1) / 2. */
static size_t
line_row (const EaselFigure *f, int y, EaselFigureRun *runs)
{
  const LineRows *r = f->rows;
  int w = f->width, a = (w - 1) / 2, lo = INT_MAX, hi = INT_MIN;

  if (r == NULL)
    return 0;
  for (int py = y + a - w + 1; py <= y + a; py++)
    {
      int k = py - r->first_row;

      if (k < 0 || k >= r->n_rows || r->xmin[k] > r->xmax[k])
        continue;
      if (r->xmin[k] - a < lo)
        lo = r->xmin[k] - a;
      if (r->xmax[k] - a + w - 1 > hi)
        hi = r->xmax[k] - a + w - 1;
    }
  if (lo > hi)
    return 0;
  runs[0] = (EaselFigureRun) { lo, hi, false };
  return 1;
}

void
easel_figure_constrain (EaselFigureKind kind, int x0, int y0, int *x1, int *y1)
{
  int dx, dy, ax, ay, sx, sy;

  x0 = clamp_coord (x0);
  y0 = clamp_coord (y0);
  dx = clamp_coord (*x1) - x0;
  dy = clamp_coord (*y1) - y0;
  ax = dx < 0 ? -dx : dx;
  ay = dy < 0 ? -dy : dy;
  sx = dx < 0 ? -1 : 1;
  sy = dy < 0 ? -1 : 1;
  if (kind == EASEL_FIGURE_LINE)
    {
      /* Nearer to horizontal when the angle is below 22.5 degrees: |dx| > (1 + sqrt 2) |dy|; nearer to vertical in the
         mirror case; else the diagonal. Integers cannot sit on the boundary, since tan 22.5 degrees is irrational. */
      const double k = 1.0 + sqrt (2.0);

      if (ax == 0 && ay == 0)
        return;
      if ((double) ax > k * ay)
        *y1 = y0;
      else if ((double) ay > k * ax)
        *x1 = x0;
      else
        {
          int d = ax < ay ? ax : ay;

          *x1 = x0 + sx * d;
          *y1 = y0 + sy * d;
        }
      return;
    }
  {
    int side = (ax < ay ? ax : ay);                              /* the smaller of the box's sides, less one */

    *x1 = x0 + sx * side;
    *y1 = y0 + sy * side;
  }
}

void
easel_figure_bounds (const EaselFigure *f, int *x, int *y, int *w, int *h)
{
  int x0 = clamp_coord (f->x0), y0 = clamp_coord (f->y0), x1 = clamp_coord (f->x1), y1 = clamp_coord (f->y1);

  *x = x0 < x1 ? x0 : x1;
  *y = y0 < y1 ? y0 : y1;
  *w = (x0 < x1 ? x1 - x0 : x0 - x1) + 1;
  *h = (y0 < y1 ? y1 - y0 : y0 - y1) + 1;
}

/* 128-bit sums of products, so that the ellipse's test is exact for any box (dx^2 * h^2 reaches 10^26 at the limit). */
typedef struct
{
  uint64_t hi, lo;
} U128;

static U128
mul128 (uint64_t a, uint64_t b)
{
  uint64_t a_lo = a & 0xFFFFFFFFu, a_hi = a >> 32, b_lo = b & 0xFFFFFFFFu, b_hi = b >> 32;
  uint64_t ll = a_lo * b_lo, lh = a_lo * b_hi, hl = a_hi * b_lo, hh = a_hi * b_hi;
  uint64_t mid = (ll >> 32) + (lh & 0xFFFFFFFFu) + (hl & 0xFFFFFFFFu);
  U128 r;

  r.lo = (ll & 0xFFFFFFFFu) | (mid << 32);
  r.hi = hh + (lh >> 32) + (hl >> 32) + (mid >> 32);
  return r;
}

static U128
add128 (U128 a, U128 b)
{
  U128 r = { a.hi + b.hi, a.lo + b.lo };

  if (r.lo < a.lo)
    r.hi++;
  return r;
}

static bool
le128 (U128 a, U128 b)
{
  return a.hi < b.hi || (a.hi == b.hi && a.lo <= b.lo);
}

/* Is the center of the pixel (x, y) in the figure of this kind in the box (its edge counts)? Coordinates are doubled, so
   a pixel's center is (2x + 1) and a box's center (2 bx + bw). */
static bool
member (EaselFigureKind kind, const Box *b, int x, int y)
{
  if (b->w <= 0 || b->h <= 0 || x < b->x || x >= b->x + b->w || y < b->y || y >= b->y + b->h)
    return false;
  if (kind == EASEL_FIGURE_RECTANGLE)
    return true;
  if (kind == EASEL_FIGURE_ELLIPSE)
    {
      int64_t dx = (2 * (int64_t) x + 1) - (2 * (int64_t) b->x + b->w);
      int64_t dy = (2 * (int64_t) y + 1) - (2 * (int64_t) b->y + b->h);
      U128 lhs = add128 (mul128 ((uint64_t) (dx * dx), (uint64_t) b->h * (uint64_t) b->h),
                         mul128 ((uint64_t) (dy * dy), (uint64_t) b->w * (uint64_t) b->w));
      U128 rhs = mul128 ((uint64_t) b->w * (uint64_t) b->w, (uint64_t) b->h * (uint64_t) b->h);

      return le128 (lhs, rhs);
    }
  {
    /* Rounded: inside the box, and in a corner zone inside the quarter circle. */
    int64_t px = 2 * (int64_t) x + 1, py = 2 * (int64_t) y + 1, cx2 = 0, cy2 = 0;
    bool in_x = false, in_y = false;

    if (px < 2 * ((int64_t) b->x + b->r))
      {
        cx2 = 2 * ((int64_t) b->x + b->r);
        in_x = true;
      }
    else if (px > 2 * ((int64_t) b->x + b->w - b->r))
      {
        cx2 = 2 * ((int64_t) b->x + b->w - b->r);
        in_x = true;
      }
    if (py < 2 * ((int64_t) b->y + b->r))
      {
        cy2 = 2 * ((int64_t) b->y + b->r);
        in_y = true;
      }
    else if (py > 2 * ((int64_t) b->y + b->h - b->r))
      {
        cy2 = 2 * ((int64_t) b->y + b->h - b->r);
        in_y = true;
      }
    if (in_x && in_y)
      return (px - cx2) * (px - cx2) + (py - cy2) * (py - cy2) <= 4 * (int64_t) b->r * b->r;
    return true;
  }
}

/* The x range of the row y in the figure of this kind in the box: an estimate from the formula, widened, then cut back to
   exactly the pixels that are members (the members of a row are one run). False if the row has none. */
static bool
extent (EaselFigureKind kind, const Box *b, int y, int *xa, int *xb)
{
  double lo = b->x, hi = b->x + b->w;
  int left, right;

  if (b->w <= 0 || b->h <= 0 || y < b->y || y >= b->y + b->h)
    return false;
  if (kind == EASEL_FIGURE_ELLIPSE)
    {
      double cy = b->y + b->h / 2.0, v = (y + 0.5 - cy) / (b->h / 2.0), t = 1.0 - v * v, h;

      h = (b->w / 2.0) * sqrt (t > 0 ? t : 0);
      lo = b->x + b->w / 2.0 - h;
      hi = b->x + b->w / 2.0 + h;
    }
  else if (kind == EASEL_FIGURE_ROUNDED && b->r > 0)
    {
      double py = y + 0.5, dy = 0;

      if (py < b->y + b->r)
        dy = b->y + b->r - py;
      else if (py > b->y + b->h - b->r)
        dy = py - (b->y + b->h - b->r);
      if (dy > 0)
        {
          double t = (double) b->r * b->r - dy * dy, inset = b->r - sqrt (t > 0 ? t : 0);

          lo = b->x + inset;
          hi = b->x + b->w - inset;
        }
    }
  left = (int) floor (lo) - 1;
  right = (int) ceil (hi) + 1;
  if (left < b->x)
    left = b->x;
  if (right > b->x + b->w - 1)
    right = b->x + b->w - 1;
  while (left <= right && !member (kind, b, left, y))
    left++;
  while (right >= left && !member (kind, b, right, y))
    right--;
  if (left > right)
    return false;
  *xa = left;
  *xb = right;
  return true;
}

static Box
outer_box (const EaselFigure *f)
{
  Box b;

  easel_figure_bounds (f, &b.x, &b.y, &b.w, &b.h);
  b.r = b.w < b.h ? b.w / 2 : b.h / 2;
  if (b.r > ROUND_RADIUS)
    b.r = ROUND_RADIUS;
  return b;
}

size_t
easel_figure_row (const EaselFigure *f, int y, EaselFigureRun *runs, size_t max)
{
  Box outer, inner;
  int xa, xb, ia, ib, w = f->width;
  size_t n = 0;

  if (max < 3)
    return 0;
  if (f->kind == EASEL_FIGURE_LINE)
    return line_row (f, y, runs);
  outer = outer_box (f);
  if (!extent (f->kind, &outer, y, &xa, &xb))
    return 0;
  /* The same figure in the box made smaller by w on each side (the corner radius by w, not below 0). */
  inner.x = outer.x + w;
  inner.y = outer.y + w;
  inner.w = outer.w - 2 * w;
  inner.h = outer.h - 2 * w;
  inner.r = outer.r > w ? outer.r - w : 0;
  if (!extent (f->kind, &inner, y, &ia, &ib))
    {
      runs[n++] = (EaselFigureRun) { xa, xb, false };
      return n;
    }
  if (ia > xa)
    runs[n++] = (EaselFigureRun) { xa, ia - 1, false };
  runs[n++] = (EaselFigureRun) { ia, ib, true };
  if (ib < xb)
    runs[n++] = (EaselFigureRun) { ib + 1, xb, false };
  return n;
}

/* The log of the pixels a figure changes, and what restoring them takes. */
typedef struct
{
  EaselChange *log;
  size_t n, cap;
  size_t max_bytes;
} Log;

static bool
log_add (Log *l, uint32_t index, uint32_t old_value, uint32_t new_value)
{
  if (l->n == l->cap)
    {
      size_t cap = l->cap ? l->cap * 2 : 1024;
      EaselChange *p;

      if (cap > SIZE_MAX / sizeof *l->log || cap * sizeof *l->log > l->max_bytes)
        return false;
      p = realloc (l->log, cap * sizeof *l->log);
      if (p == NULL)
        return false;
      l->log = p;
      l->cap = cap;
    }
  l->log[l->n++] = (EaselChange) { index, old_value, new_value };
  return true;
}

/* Puts every logged pixel back, last first, through easel_image_set so that the screen update sees them. */
static void
log_restore (const Log *l, EaselImage *img)
{
  for (size_t i = l->n; i > 0; i--)
    easel_image_set (img, (int) (l->log[i - 1].index % (uint32_t) img->width),
                     (int) (l->log[i - 1].index / (uint32_t) img->width), l->log[i - 1].old_value);
}

/* Sets the pixels x_from to x_to of the row y (inside the picture) to color, logging each one that changes. */
static bool
paint_run (EaselImage *img, Log *l, int y, int x_from, int x_to, uint32_t color)
{
  if (x_from < 0)
    x_from = 0;
  if (x_to > img->width - 1)
    x_to = img->width - 1;
  for (int x = x_from; x <= x_to; x++)
    {
      uint32_t old = img->px[(size_t) y * (size_t) img->width + (size_t) x];

      if (old != color)
        {
          if (!log_add (l, (uint32_t) ((size_t) y * (size_t) img->width + (size_t) x), old, color))
            return false;
          easel_image_set (img, x, y, color);          /* through easel_image_set: the screen update sees it */
        }
    }
  return true;
}

EaselFigureResult
easel_figure_apply_capped (EaselImage *img, EaselHistory *history, const EaselFigure *figure, uint32_t draw,
                           uint32_t other, size_t *changed, size_t max_bytes)
{
  EaselFigure f = *figure;
  Log l = { NULL, 0, 0, max_bytes };
  int y_from = 0, y_to = img->height - 1;
  bool ok = true;

  if (changed != NULL)
    *changed = 0;
  if (f.kind != EASEL_FIGURE_LINE)
    {
      int bx, by, bw, bh;

      easel_figure_bounds (&f, &bx, &by, &bw, &bh);
      if (by > y_from)
        y_from = by;
      if (by + bh - 1 < y_to)
        y_to = by + bh - 1;
    }
  if (y_from > y_to)
    return EASEL_FIGURE_NOTHING;
  if (!easel_figure_prepare (&f, y_from, y_to))
    return EASEL_FIGURE_NO_MEMORY;
  for (int y = y_from; ok && y <= y_to; y++)
    {
      EaselFigureRun runs[4];
      size_t n = easel_figure_row (&f, y, runs, 4);

      for (size_t i = 0; ok && i < n; i++)
        {
          if (!runs[i].inside)
            ok = paint_run (img, &l, y, runs[i].x0, runs[i].x1, draw);
          else if (f.style == EASEL_FIGURE_OUTLINE_AND_FILL)
            ok = paint_run (img, &l, y, runs[i].x0, runs[i].x1, other);
          else if (f.style == EASEL_FIGURE_FILL_ONLY)
            ok = paint_run (img, &l, y, runs[i].x0, runs[i].x1, draw);
        }
    }
  easel_figure_release (&f);
  if (!ok)
    {
      log_restore (&l, img);
      free (l.log);
      return EASEL_FIGURE_NO_MEMORY;
    }
  if (l.n == 0)
    {
      free (l.log);
      return EASEL_FIGURE_NOTHING;
    }
  if (changed != NULL)
    *changed = l.n;
  if (history == NULL)
    {
      free (l.log);
      return EASEL_FIGURE_DONE;
    }
  /* The log doubled as it grew: the history keeps it at its length, so the memory held is the memory the budget counts
     (REQ-0002-16). A shrink that fails keeps the bigger block, which is still valid. */
  if (l.cap > l.n)
    {
      EaselChange *p = realloc (l.log, l.n * sizeof *l.log);

      if (p != NULL)
        l.log = p;
    }
  if (!easel_history_push (history, l.log, l.n))
    {
      log_restore (&l, img);
      free (l.log);
      if (changed != NULL)
        *changed = 0;
      return EASEL_FIGURE_NO_MEMORY;
    }
  return EASEL_FIGURE_DONE;            /* the history took the log */
}

EaselFigureResult
easel_figure_apply (EaselImage *img, EaselHistory *history, const EaselFigure *f, uint32_t draw, uint32_t other,
                    size_t *changed)
{
  return easel_figure_apply_capped (img, history, f, draw, other, changed, SIZE_MAX);
}
