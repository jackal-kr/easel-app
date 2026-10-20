/* SPDX-License-Identifier: GPL-3.0-or-later */
#include <limits.h>
#include <math.h>
#include <stdint.h>
#include <stdlib.h>

#include "change-log.h"
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

/* A list of pixels, for the path of a curve. */
typedef struct
{
  int x, y;
} Px;

typedef struct
{
  Px *v;
  size_t n, cap;
} PxList;

static bool
pxl_add (PxList *l, int x, int y)
{
  if (l->n == l->cap)
    {
      size_t cap = l->cap ? l->cap * 2 : 256;
      Px *p;

      if (cap > SIZE_MAX / sizeof *l->v)
        return false;
      p = realloc (l->v, cap * sizeof *l->v);
      if (p == NULL)
        return false;
      l->v = p;
      l->cap = cap;
    }
  l->v[l->n++] = (Px) { x, y };
  return true;
}

/* The Pencil's line from (x0, y0) to (x1, y1) appended to the list, both ends (the first one only if with_first). */
static bool
bres_append (PxList *l, int x0, int y0, int x1, int y1, bool with_first)
{
  int dx = x1 > x0 ? x1 - x0 : x0 - x1, dy = -(y1 > y0 ? y1 - y0 : y0 - y1);
  int sx = x0 < x1 ? 1 : -1, sy = y0 < y1 ? 1 : -1;
  long long err = (long long) dx + dy;
  bool first = true;

  for (;;)
    {
      long long e2 = 2 * err;

      if ((!first || with_first) && !pxl_add (l, x0, y0))
        return false;
      first = false;
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
  return true;
}

/* The coordinate of the curve's point at i / n, the nearest pixel (a half rounded up), in exact integer arithmetic: the
   Bernstein weights are (n - i)^3, 3 (n - i)^2 i, 3 (n - i) i^2 and i^3 over n^3; the coordinates are shifted by the
   limit so that nothing is negative. */
static int
curve_coord (int p0, int p1, int p2, int p3, long long n, long long i)
{
  unsigned __int128 d = (unsigned __int128) n * n * n, u = (unsigned __int128) (n - i), t = (unsigned __int128) i;
  unsigned __int128 num = u * u * u * (unsigned) (p0 + COORD_LIMIT) + 3 * u * u * t * (unsigned) (p1 + COORD_LIMIT)
                          + 3 * u * t * t * (unsigned) (p2 + COORD_LIMIT) + t * t * t * (unsigned) (p3 + COORD_LIMIT);

  return (int) ((num * 2 + d) / (2 * d)) - COORD_LIMIT;
}

static bool
on_closed_segment (const int p[8], int qx, int qy)
{
  long long cross = (long long) (p[6] - p[0]) * (qy - p[1]) - (long long) (p[7] - p[1]) * (qx - p[0]);

  return cross == 0 && qx >= (p[0] < p[6] ? p[0] : p[6]) && qx <= (p[0] < p[6] ? p[6] : p[0])
         && qy >= (p[1] < p[7] ? p[1] : p[7]) && qy <= (p[1] < p[7] ? p[7] : p[1]);
}

/* The 1-pixel path of the curve of REQ-0011-05, appended to an empty list. */
static bool
curve_path (const int pts[8], PxList *out)
{
  int p[8];
  PxList samples = { NULL, 0, 0 }, joined = { NULL, 0, 0 };
  double len = 0;
  long long n;
  bool ok = false;

  for (int k = 0; k < 8; k++)
    p[k] = clamp_coord (pts[k]);
  if (on_closed_segment (p, p[2], p[3]) && on_closed_segment (p, p[4], p[5]))
    return bres_append (out, p[0], p[1], p[6], p[7], true);
  for (int k = 0; k < 3; k++)
    {
      double dx = (double) (p[2 * k + 2] - p[2 * k]), dy = (double) (p[2 * k + 3] - p[2 * k + 1]);

      len += sqrt (dx * dx + dy * dy);
    }
  n = (long long) ceil (len);
  if (n < 1)
    n = 1;
  for (long long i = 0; i <= n; i++)
    {
      int x = curve_coord (p[0], p[2], p[4], p[6], n, i), y = curve_coord (p[1], p[3], p[5], p[7], n, i);

      if (samples.n > 0 && samples.v[samples.n - 1].x == x && samples.v[samples.n - 1].y == y)
        continue;
      if (!pxl_add (&samples, x, y))
        goto done;
    }
  if (!pxl_add (&joined, samples.v[0].x, samples.v[0].y))
    goto done;
  for (size_t k = 1; k < samples.n; k++)
    if (!bres_append (&joined, samples.v[k - 1].x, samples.v[k - 1].y, samples.v[k].x, samples.v[k].y, false))
      goto done;
  /* One pixel thin: a pixel goes when the one kept before it and the one after it touch. */
  if (!pxl_add (out, joined.v[0].x, joined.v[0].y))
    goto done;
  for (size_t i = 1; i + 1 < joined.n; i++)
    {
      Px a = out->v[out->n - 1], c = joined.v[i + 1];

      if (abs (a.x - c.x) <= 1 && abs (a.y - c.y) <= 1)
        continue;
      if (!pxl_add (out, joined.v[i].x, joined.v[i].y))
        goto done;
    }
  if (joined.n > 1 && !pxl_add (out, joined.v[joined.n - 1].x, joined.v[joined.n - 1].y))
    goto done;
  ok = true;
done:
  free (samples.v);
  free (joined.v);
  return ok;
}

/* The rows of a path stamped with the w x w pen: for each picture row from first_row, its runs of pixels, sorted and merged. */
typedef struct
{
  int first_row, n_rows;
  size_t *start;           /* n_rows + 1: where the intervals of a row begin in iv */
  int *cnt;                /* the runs of a row */
  int *iv;                 /* x0, x1 pairs */
  int max_runs;            /* the most runs any row has */
} PathRows;

static int
cmp_interval (const void *a, const void *b)
{
  const int *x = a, *y = b;

  return (x[0] > y[0]) - (x[0] < y[0]);
}

static void
path_rows_free (PathRows *r)
{
  if (r != NULL)
    {
      free (r->start);
      free (r->cnt);
      free (r->iv);
      free (r);
    }
}

static PathRows *
path_rows_build (const Px *v, size_t n, int w, int first_row, int n_rows)
{
  PathRows *r = calloc (1, sizeof *r);
  int a = (w - 1) / 2;
  size_t total = 0, *pos;

  if (r == NULL)
    return NULL;
  r->first_row = first_row;
  r->n_rows = n_rows;
  r->start = calloc ((size_t) n_rows + 1, sizeof *r->start);
  r->cnt = calloc ((size_t) n_rows, sizeof *r->cnt);
  pos = calloc ((size_t) n_rows, sizeof *pos);
  if (r->start == NULL || r->cnt == NULL || pos == NULL)
    goto fail;
  for (size_t i = 0; i < n; i++)
    for (int k = 0; k < w; k++)
      {
        long long row = (long long) v[i].y - a + k - first_row;

        if (row >= 0 && row < n_rows)
          r->cnt[row]++;
      }
  for (int k = 0; k < n_rows; k++)
    {
      r->start[k] = total;
      pos[k] = total;
      total += (size_t) r->cnt[k];
    }
  r->start[n_rows] = total;
  if (total > SIZE_MAX / (2 * sizeof *r->iv))
    goto fail;
  r->iv = malloc ((total ? total : 1) * 2 * sizeof *r->iv);
  if (r->iv == NULL)
    goto fail;
  for (size_t i = 0; i < n; i++)
    for (int k = 0; k < w; k++)
      {
        long long row = (long long) v[i].y - a + k - first_row;

        if (row >= 0 && row < n_rows)
          {
            r->iv[2 * pos[row]] = v[i].x - a;
            r->iv[2 * pos[row] + 1] = v[i].x - a + w - 1;
            pos[row]++;
          }
      }
  for (int k = 0; k < n_rows; k++)
    {
      int *s = r->iv + 2 * r->start[k], m = 0;

      qsort (s, (size_t) r->cnt[k], 2 * sizeof *s, cmp_interval);
      for (int j = 0; j < r->cnt[k]; j++)
        {
          if (m > 0 && s[2 * j] <= s[2 * (m - 1) + 1] + 1)
            {
              if (s[2 * j + 1] > s[2 * (m - 1) + 1])
                s[2 * (m - 1) + 1] = s[2 * j + 1];
            }
          else
            {
              s[2 * m] = s[2 * j];
              s[2 * m + 1] = s[2 * j + 1];
              m++;
            }
        }
      r->cnt[k] = m;
      if (m > r->max_runs)
        r->max_runs = m;
    }
  free (pos);
  return r;
fail:
  free (pos);
  path_rows_free (r);
  return NULL;
}

/* A prepared polygon: the rows of its outline, and room for the work of one row of its region. */
typedef struct
{
  PathRows *outline;
  int n_sides;
  int *scratch;            /* the crossings of a row, then the intervals of its region */
} Poly;

static void
poly_free (Poly *p)
{
  if (p != NULL)
    {
      path_rows_free (p->outline);
      free (p->scratch);
      free (p);
    }
}

/* The outline path of a polygon: the Pencil's line of every side, the closing one when closed; one corner is one pixel. */
static bool
polygon_path (const EaselFigure *f, PxList *out)
{
  size_t n = f->n_pts;
  int p[2];

  if (n == 0)
    return true;
  p[0] = clamp_coord (f->pts[0]);
  p[1] = clamp_coord (f->pts[1]);
  if (!pxl_add (out, p[0], p[1]))
    return false;
  for (size_t i = 1; i <= n; i++)
    {
      int x0, y0, x1, y1;

      if (i == n && !f->closed)
        break;
      x0 = clamp_coord (f->pts[2 * (i - 1)]);
      y0 = clamp_coord (f->pts[2 * (i - 1) + 1]);
      x1 = clamp_coord (f->pts[2 * (i % n)]);
      y1 = clamp_coord (f->pts[2 * (i % n) + 1]);
      if (!bres_append (out, x0, y0, x1, y1, false))
        return false;
    }
  return true;
}

static bool
poly_prepare (EaselFigure *f, int y_from, int y_to)
{
  Poly *poly = calloc (1, sizeof *poly);
  PxList path = { NULL, 0, 0 };
  int reach = f->width, n_rows = y_to - y_from + 1 + 2 * reach;
  size_t sides = f->closed ? f->n_pts : (f->n_pts > 0 ? f->n_pts - 1 : 0);

  if (poly == NULL)
    return false;
  poly->n_sides = (int) sides;
  if (f->pts == NULL || !polygon_path (f, &path))
    {
      free (path.v);
      poly_free (poly);
      return false;
    }
  poly->outline = path_rows_build (path.v, path.n, f->width, y_from - reach, n_rows < 1 ? 1 : n_rows);
  free (path.v);
  if (poly->outline != NULL)
    {
      /* The crossings of a row, its region's intervals (up to 2 sides + 1) and the inside's pieces (those and the outline's runs). */
      size_t room = sides + 2 * (2 * sides + 2) + 2 * (2 * sides + 2 + (size_t) poly->outline->max_runs);

      poly->scratch = malloc (room * sizeof *poly->scratch);
    }
  if (poly->outline == NULL || poly->scratch == NULL)
    {
      poly_free (poly);
      return false;
    }
  f->rows = poly;
  return true;
}

/* The outline's runs of the row y of a prepared path, as a pointer into its intervals and a count. */
static int
path_runs_at (const PathRows *r, int y, const int **iv)
{
  long long k = (long long) y - r->first_row;

  if (k < 0 || k >= r->n_rows)
    {
      *iv = NULL;
      return 0;
    }
  *iv = r->iv + 2 * r->start[k];
  return r->cnt[k];
}

/* The row y of a polygon (REQ-0011-08): the outline's runs, and, when closed, the inside: the pixels whose center is on a
   side or has an odd number of crossing sides to its right (a side crosses when one end has a larger y than the row and the
   other has not), less the outline. Runs come left to right. */
static size_t
polygon_row (const EaselFigure *f, int y, EaselFigureRun *runs)
{
  const Poly *poly = f->rows;
  const int *ov;
  int n_out, m = 0, n_reg = 0, n_in = 0;
  int *r, *reg, *in;
  size_t n = 0;

  if (poly == NULL)
    return 0;
  n_out = path_runs_at (poly->outline, y, &ov);
  if (!f->closed || poly->n_sides == 0)
    {
      for (int i = 0; i < n_out; i++)
        runs[n++] = (EaselFigureRun) { ov[2 * i], ov[2 * i + 1], false };
      return n;
    }
  r = poly->scratch;
  reg = r + poly->n_sides;                       /* up to 2 sides + 1 intervals, a pair each */
  in = reg + 2 * (2 * poly->n_sides + 2);
  for (int i = 0; i < poly->n_sides; i++)
    {
      int x0 = clamp_coord (f->pts[2 * i]), y0 = clamp_coord (f->pts[2 * i + 1]);
      int x1 = clamp_coord (f->pts[2 * ((i + 1) % f->n_pts)]), y1 = clamp_coord (f->pts[2 * ((i + 1) % f->n_pts) + 1]);
      long long num, den, q, rem;

      if (y0 == y1)
        {
          if (y0 == y)                             /* a horizontal side on this row: its pixels are on it */
            {
              reg[2 * n_reg] = x0 < x1 ? x0 : x1;
              reg[2 * n_reg + 1] = x0 < x1 ? x1 : x0;
              n_reg++;
            }
          continue;
        }
      if (y < (y0 < y1 ? y0 : y1) || y > (y0 < y1 ? y1 : y0))
        continue;
      num = (long long) (y - y0) * (x1 - x0);
      den = y1 - y0;
      if (den < 0)
        {
          num = -num;
          den = -den;
        }
      q = num / den;
      rem = num - q * den;
      if (rem < 0)
        {
          q--;
          rem += den;
        }
      if (rem == 0)                                /* a lattice point exactly on the side */
        {
          reg[2 * n_reg] = reg[2 * n_reg + 1] = (int) (x0 + q);
          n_reg++;
        }
      if ((y0 > y) != (y1 > y))                    /* crossing: the largest x strictly left of the crossing */
        {
          int v = (int) (x0 + q - (rem == 0 ? 1 : 0)), j = m++;

          while (j > 0 && r[j - 1] > v)
            {
              r[j] = r[j - 1];
              j--;
            }
          r[j] = v;
        }
    }
  for (int k = m - 2; k >= 0; k -= 2)              /* the odd stretches, from the right */
    if (r[k] + 1 <= r[k + 1])
      {
        reg[2 * n_reg] = r[k] + 1;
        reg[2 * n_reg + 1] = r[k + 1];
        n_reg++;
      }
  /* Sort the region's intervals by their start and merge the ones that touch. */
  for (int i = 1; i < n_reg; i++)
    {
      int a = reg[2 * i], b = reg[2 * i + 1], j = i;

      while (j > 0 && reg[2 * (j - 1)] > a)
        {
          reg[2 * j] = reg[2 * (j - 1)];
          reg[2 * j + 1] = reg[2 * (j - 1) + 1];
          j--;
        }
      reg[2 * j] = a;
      reg[2 * j + 1] = b;
    }
  {
    int k = 0;

    for (int i = 0; i < n_reg; i++)
      {
        if (k > 0 && reg[2 * i] <= reg[2 * (k - 1) + 1] + 1)
          {
            if (reg[2 * i + 1] > reg[2 * (k - 1) + 1])
              reg[2 * (k - 1) + 1] = reg[2 * i + 1];
          }
        else
          {
            reg[2 * k] = reg[2 * i];
            reg[2 * k + 1] = reg[2 * i + 1];
            k++;
          }
      }
    n_reg = k;
  }
  /* The inside: the region less the outline's runs (both sorted, the outline's merged). */
  for (int i = 0, o = 0; i < n_reg; i++)
    {
      int lo = reg[2 * i], hi = reg[2 * i + 1];

      while (o < n_out && ov[2 * o + 1] < lo)
        o++;
      for (int p = o; lo <= hi; p++)
        {
          if (p >= n_out || ov[2 * p] > hi)
            {
              in[2 * n_in] = lo;
              in[2 * n_in + 1] = hi;
              n_in++;
              break;
            }
          if (ov[2 * p] > lo)
            {
              in[2 * n_in] = lo;
              in[2 * n_in + 1] = ov[2 * p] - 1;
              n_in++;
            }
          lo = ov[2 * p + 1] + 1;
        }
    }
  /* Outline and inside runs, left to right. */
  for (int i = 0, j = 0; i < n_out || j < n_in;)
    {
      if (j >= n_in || (i < n_out && ov[2 * i] < in[2 * j]))
        {
          runs[n++] = (EaselFigureRun) { ov[2 * i], ov[2 * i + 1], false };
          i++;
        }
      else
        {
          runs[n++] = (EaselFigureRun) { in[2 * j], in[2 * j + 1], true };
          j++;
        }
    }
  return n;
}

bool
easel_figure_prepare (EaselFigure *f, int y_from, int y_to)
{
  LineRows *r;
  int reach = f->width;

  f->rows = NULL;
  if (f->kind == EASEL_FIGURE_POLYGON)
    return poly_prepare (f, y_from, y_to);
  if (f->kind == EASEL_FIGURE_CURVE)
    {
      PxList path = { NULL, 0, 0 };
      int n_rows = y_to - y_from + 1 + 2 * reach;

      if (f->pts == NULL || f->n_pts < 4 || !curve_path (f->pts, &path))
        {
          free (path.v);
          return false;
        }
      f->rows = path_rows_build (path.v, path.n, f->width, y_from - reach, n_rows < 1 ? 1 : n_rows);
      free (path.v);
      return f->rows != NULL;
    }
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

  if (f->kind == EASEL_FIGURE_POLYGON)
    {
      poly_free (f->rows);
      f->rows = NULL;
      return;
    }
  if (f->kind == EASEL_FIGURE_CURVE)
    {
      path_rows_free (f->rows);
      f->rows = NULL;
      return;
    }
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

/* The row y of a curve: the runs of its prepared rows, all outline. */
static size_t
path_row (const EaselFigure *f, int y, EaselFigureRun *runs)
{
  const PathRows *r = f->rows;
  long long k = r != NULL ? (long long) y - r->first_row : -1;

  if (r == NULL || k < 0 || k >= r->n_rows)
    return 0;
  for (int j = 0; j < r->cnt[k]; j++)
    runs[j] = (EaselFigureRun) { r->iv[2 * (r->start[k] + (size_t) j)], r->iv[2 * (r->start[k] + (size_t) j) + 1], false };
  return (size_t) r->cnt[k];
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
easel_figure_max_runs (const EaselFigure *f)
{
  /* A curve's rows are known once it is prepared (so ask after easel_figure_prepare); before that one run. */
  if (f->kind == EASEL_FIGURE_CURVE)
    return f->rows != NULL && ((const PathRows *) f->rows)->max_runs > 0 ? (size_t) ((const PathRows *) f->rows)->max_runs : 1;
  if (f->kind == EASEL_FIGURE_POLYGON)
    {
      const Poly *poly = f->rows;

      /* The outline's runs, the inside's (a piece for each interval of the region and each outline run) and room to spare. */
      return poly != NULL ? (size_t) (2 * poly->outline->max_runs + 2 * poly->n_sides + 2) : 1;
    }
  /* A Line's row is one range; a shape's row is its left edge, its inside and its right edge. */
  return 3;
}

size_t
easel_figure_row (const EaselFigure *f, int y, EaselFigureRun *runs, size_t max)
{
  Box outer, inner;
  int xa, xb, ia, ib, w = f->width;
  size_t n = 0;

  if (max < easel_figure_max_runs (f))
    return 0;
  if (f->kind == EASEL_FIGURE_LINE)
    return line_row (f, y, runs);
  if (f->kind == EASEL_FIGURE_CURVE)
    return path_row (f, y, runs);
  if (f->kind == EASEL_FIGURE_POLYGON)
    return polygon_row (f, y, runs);
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

/* Sets the pixels x_from to x_to of the row y (inside the picture) to color, logging each one that changes. */
static bool
paint_run (EaselImage *img, EaselChangeLog *l, int y, int x_from, int x_to, uint32_t color)
{
  if (x_from < 0)
    x_from = 0;
  if (x_to > img->width - 1)
    x_to = img->width - 1;
  for (int x = x_from; x <= x_to; x++)
    if (!easel_change_log_set (l, img, x, y, color))
      return false;
  return true;
}

EaselFigureResult
easel_figure_apply_capped (EaselImage *img, EaselHistory *history, const EaselFigure *figure, uint32_t draw,
                           uint32_t other, size_t *changed, size_t max_bytes)
{
  EaselFigure f = *figure;
  EaselChangeLog l;
  int y_from = 0, y_to = img->height - 1;
  size_t max_runs;
  EaselFigureRun *runs;
  bool ok = true;

  if (changed != NULL)
    *changed = 0;
  easel_change_log_init (&l, max_bytes);
  if (f.kind == EASEL_FIGURE_CURVE || f.kind == EASEL_FIGURE_POLYGON)
    {
      /* The figure lies within the hull of its points, a pixel off, and the pen reaches the width further. */
      int lo, hi;
      size_t np = f.kind == EASEL_FIGURE_CURVE ? 4 : f.n_pts;

      if (f.pts == NULL || np == 0)
        return EASEL_FIGURE_NOTHING;
      lo = hi = clamp_coord (f.pts[1]);
      for (size_t k = 1; k < np; k++)
        {
          int v = clamp_coord (f.pts[2 * k + 1]);

          lo = v < lo ? v : lo;
          hi = v > hi ? v : hi;
        }
      if (lo - f.width - 2 > y_from)
        y_from = lo - f.width - 2;
      if (hi + f.width + 2 < y_to)
        y_to = hi + f.width + 2;
    }
  else if (f.kind != EASEL_FIGURE_LINE)
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
  max_runs = easel_figure_max_runs (&f);
  runs = malloc (max_runs * sizeof *runs);
  if (runs == NULL)
    {
      easel_figure_release (&f);
      return EASEL_FIGURE_NO_MEMORY;
    }
  for (int y = y_from; ok && y <= y_to; y++)
    {
      size_t n = easel_figure_row (&f, y, runs, max_runs);

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
  free (runs);
  if (!ok)
    {
      easel_change_log_restore (&l, img);
      easel_change_log_free (&l);
      return EASEL_FIGURE_NO_MEMORY;
    }
  if (l.n == 0)
    {
      easel_change_log_free (&l);
      return EASEL_FIGURE_NOTHING;
    }
  if (changed != NULL)
    *changed = l.n;
  if (history == NULL)
    {
      easel_change_log_free (&l);
      return EASEL_FIGURE_DONE;
    }
  if (!easel_change_log_push (&l, history))
    {
      easel_change_log_restore (&l, img);
      easel_change_log_free (&l);
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
