/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "lasso.h"

#include <limits.h>
#include <stdlib.h>

#include "lattice.h"
#include "stretch-skew.h"

static inline int
imax (int a, int b)
{
  return a > b ? a : b;
}

static inline int
imin (int a, int b)
{
  return a < b ? a : b;
}

bool
easel_path_add (EaselPath *p, int x, int y, size_t max_bytes)
{
  if (p->n == p->cap)
    {
      /* Doubling first; if that is over the limit, only what is needed. */
      size_t cap = p->cap ? p->cap * 2 : 16;
      int *xy;

      if (cap > max_bytes / (2 * sizeof *xy))
        cap = p->n + 1;
      if (cap > max_bytes / (2 * sizeof *xy))
        return false;
      xy = realloc (p->xy, cap * 2 * sizeof *xy);
      if (xy == NULL)
        return false;
      p->xy = xy;
      p->cap = cap;
    }
  p->xy[2 * p->n] = x;
  p->xy[2 * p->n + 1] = y;
  p->n++;
  return true;
}

void
easel_path_free (EaselPath *p)
{
  free (p->xy);
  *p = (EaselPath) { 0 };
}

/* One coordinate: u from the old origin, old size s, new origin o and size n. */
static int
scale_corner (int v, int s0, int s, int o, int n)
{
  long long u = (long long) v - s0;

  if (u < 0)
    return (int) (o + u);
  if (u > s)
    return (int) (o + n + (u - s));
  return (int) (o + (2 * u * n + s) / (2 * (long long) s));
}

bool
easel_lasso_orient_path (const EaselPath *src, int x, int y, int w, int h, EaselOrient o, EaselPath *out, size_t max_bytes)
{
  EaselPath p = { 0 };
  bool transposed = easel_orient_swaps (o);
  long long pw = transposed ? h : w, ph = transposed ? w : h;      /* the box after the transpose */

  for (size_t i = 0; i < src->n; i++)
    {
      long long u = (long long) src->xy[2 * i] - x, v = (long long) src->xy[2 * i + 1] - y;

      if (src->xy[2 * i] == EASEL_PATH_BREAK)
        {
          if (!easel_path_add (&p, EASEL_PATH_BREAK, EASEL_PATH_BREAK, max_bytes))
            {
              easel_path_free (&p);
              return false;
            }
          continue;
        }

      if (transposed)
        {
          long long t = u;

          u = v;
          v = t;
        }
      if (o & EASEL_ORIENT_FLIP_X)
        u = pw - u;
      if (o & EASEL_ORIENT_FLIP_Y)
        v = ph - v;
      if (!easel_path_add (&p, (int) (x + u), (int) (y + v), max_bytes))
        {
          easel_path_free (&p);
          return false;
        }
    }
  *out = p;
  return true;
}

bool
easel_lasso_scale_path (const EaselPath *src, int sx, int sy, int sw, int sh, int nx, int ny, int nw, int nh, EaselPath *out,
                        size_t max_bytes)
{
  EaselPath p = { 0 };

  if (sw <= 0 || sh <= 0 || nw <= 0 || nh <= 0)
    return false;
  for (size_t i = 0; i < src->n; i++)
    if (!(src->xy[2 * i] == EASEL_PATH_BREAK ? easel_path_add (&p, EASEL_PATH_BREAK, EASEL_PATH_BREAK, max_bytes)
          : easel_path_add (&p, scale_corner (src->xy[2 * i], sx, sw, nx, nw), scale_corner (src->xy[2 * i + 1], sy, sh, ny, nh), max_bytes)))
      {
        easel_path_free (&p);
        return false;
      }
  *out = p;
  return true;
}

const int *
easel_path_next_part (const EaselPath *p, size_t *at, size_t *n)
{
  size_t first = *at, i = first;

  if (first >= p->n)
    return NULL;
  while (i < p->n && p->xy[2 * i] != EASEL_PATH_BREAK)
    i++;
  *n = i - first;
  *at = i + 1;
  return p->xy + 2 * first;
}

/* Whether (x, y) of the mask is a pixel of the shape; outside the mask it is not. */
static bool
in_mask (const uint8_t *mask, int w, int h, int x, int y)
{
  return x >= 0 && x < w && y >= 0 && y < h && mask[(size_t) y * w + x];
}

bool
easel_lasso_outline (const uint8_t *mask, int w, int h, int x, int y, EaselPath *out, size_t max_bytes)
{
  enum { EAST, SOUTH, WEST, NORTH };
  size_t total = (size_t) w * (size_t) h;
  uint8_t *seen;                               /* the pixels whose top edge a line has taken */
  EaselPath p = { 0 };
  bool first_part = true;

  if (total > max_bytes || (seen = calloc (total, 1)) == NULL)
    return false;
  for (int y0 = 0; y0 < h; y0++)
    for (int x0 = 0; x0 < w; x0++)
      {
        int cx = x0, cy = y0, dir = EAST;

        if (seen[(size_t) y0 * w + x0] || !in_mask (mask, w, h, x0, y0) || in_mask (mask, w, h, x0, y0 - 1))
          continue;
        if ((!first_part && !easel_path_add (&p, EASEL_PATH_BREAK, EASEL_PATH_BREAK, max_bytes))
            || !easel_path_add (&p, x + cx, y + cy, max_bytes))
          goto no_memory;
        first_part = false;
        /* Along the edge with the shape on the right, so that at a corner it turns to stay with the pixel it follows: a part that touches
           only at that corner is another line. */
        do
          {
            bool ahead_right, ahead_left;
            int turned;

            if (dir == EAST)
              {
                seen[(size_t) cy * w + cx] = 1;
                cx++;
                ahead_right = in_mask (mask, w, h, cx, cy);
                ahead_left = in_mask (mask, w, h, cx, cy - 1);
              }
            else if (dir == SOUTH)
              {
                cy++;
                ahead_right = in_mask (mask, w, h, cx - 1, cy);
                ahead_left = in_mask (mask, w, h, cx, cy);
              }
            else if (dir == WEST)
              {
                cx--;
                ahead_right = in_mask (mask, w, h, cx - 1, cy - 1);
                ahead_left = in_mask (mask, w, h, cx - 1, cy);
              }
            else
              {
                cy--;
                ahead_right = in_mask (mask, w, h, cx, cy - 1);
                ahead_left = in_mask (mask, w, h, cx - 1, cy - 1);
              }
            turned = !ahead_right ? (dir + 1) % 4 : ahead_left ? (dir + 3) % 4 : dir;
            if (turned != dir && !(cx == x0 && cy == y0 && turned == EAST)
                && !easel_path_add (&p, x + cx, y + cy, max_bytes))
              goto no_memory;
            dir = turned;
          }
        while (!(cx == x0 && cy == y0 && dir == EAST));
      }
  free (seen);
  *out = p;
  return true;
no_memory:
  free (seen);
  easel_path_free (&p);
  return false;
}

bool
easel_lasso_skew_line (const EaselFloat *f, int amount_h, int amount_v, EaselPath *out, size_t max_bytes)
{
  int out_w = f->w + abs (amount_h), out_h = f->h + abs (amount_v);
  size_t n = (size_t) out_w * (size_t) out_h;
  uint8_t *shape;
  bool ok;

  if (n > max_bytes || (shape = malloc (n)) == NULL)
    return false;
  ok = easel_ss_make (NULL, f->mask, f->w, f->h, f->w, f->h, amount_h, amount_v, 0, NULL, shape)
       && easel_lasso_outline (shape, out_w, out_h, f->x, f->y, out, max_bytes);
  free (shape);
  return ok;
}

/* A grid segment: for a horizontal one, the lattice row y and the column x it covers; for a vertical one, the pixel row y it is in
   and the lattice column x. */
typedef struct
{
  int y, x;
} Seg;

/* A run of pixels of one row between two vertical segments: lo to hi, both included (INT_MIN and INT_MAX for the ends of a row). */
typedef struct
{
  int y, lo, hi;
} Run;

/* What the work holds at once, against max. */
typedef struct
{
  size_t used, max;
} Budget;

/* Grows *arr, of *cap elements of elem bytes, to hold need: by doubling if the budget allows, else exactly. FALSE if not. */
static bool
grow (void **arr, size_t *cap, size_t elem, size_t need, Budget *b)
{
  size_t ncap = *cap ? *cap * 2 : 64;
  void *p;

  if (need <= *cap)
    return true;
  if (ncap < need)
    ncap = need;
  if (ncap > SIZE_MAX / elem || b->used + (ncap - *cap) * elem > b->max)
    ncap = need;
  if (ncap > SIZE_MAX / elem || b->used + (ncap - *cap) * elem > b->max)
    return false;
  p = realloc (*arr, ncap * elem);
  if (p == NULL)
    return false;
  b->used += (ncap - *cap) * elem;
  *arr = p;
  *cap = ncap;
  return true;
}

static int
seg_cmp (const void *a, const void *b)
{
  const Seg *p = a, *q = b;

  if (p->y != q->y)
    return p->y < q->y ? -1 : 1;
  return p->x < q->x ? -1 : p->x > q->x;
}

/* Sorts a[0..n) and takes out the repeats; the new length. */
static size_t
sort_unique (Seg *a, size_t n)
{
  size_t m = 0;

  if (n > 1)
    qsort (a, n, sizeof *a, seg_cmp);             /* an empty list may be NULL */
  for (size_t i = 0; i < n; i++)
    if (m == 0 || seg_cmp (&a[m - 1], &a[i]) != 0)
      a[m++] = a[i];
  return m;
}

/* The first index of a[0..n) not below (y, x). */
static size_t
lower_bound (const Seg *a, size_t n, int y, int x)
{
  size_t lo = 0, hi = n;
  Seg key = { y, x };

  while (lo < hi)
    {
      size_t mid = (lo + hi) / 2;

      if (seg_cmp (&a[mid], &key) < 0)
        lo = mid + 1;
      else
        hi = mid;
    }
  return lo;
}

static int
find (int *parent, int i)
{
  while (parent[i] != i)
    {
      parent[i] = parent[parent[i]];
      i = parent[i];
    }
  return i;
}

static void
join (int *parent, int a, int b)
{
  a = find (parent, a);
  b = find (parent, b);
  if (a != b)
    parent[a < b ? b : a] = a < b ? a : b;          /* the smaller index is the root: node 0 is "outside" */
}

/* The state of the work: the grid segments of the closed path, and the runs between its vertical segments. */
typedef struct
{
  Seg *h, *v;
  size_t n_h, n_v, cap_h, cap_v;
  Run *run;
  int *parent;
  size_t n_run, cap_run, cap_parent;
  Budget b;
} Work;

static void
work_free (Work *w)
{
  free (w->h);
  free (w->v);
  free (w->run);
  free (w->parent);
}

static bool
push_run (Work *w, int y, int lo, int hi, bool outside)
{
  if (!grow ((void **) &w->run, &w->cap_run, sizeof *w->run, w->n_run + 1, &w->b)
      || !grow ((void **) &w->parent, &w->cap_parent, sizeof *w->parent, w->n_run + 1, &w->b))
    return false;
  w->run[w->n_run] = (Run) { y, lo, hi };
  w->parent[w->n_run] = (int) w->n_run;
  if (outside)
    join (w->parent, (int) w->n_run, 0);
  w->n_run++;
  return true;
}

/* Is every column lo..hi of the lattice row y covered by a horizontal segment, so that nothing passes between the two pixel rows? */
static bool
blocked (const Work *w, int y, int lo, int hi)
{
  size_t a, b;

  if (lo == INT_MIN || hi == INT_MAX)
    return false;
  a = lower_bound (w->h, w->n_h, y, lo);
  b = lower_bound (w->h, w->n_h, y, hi + 1);
  return b - a == (size_t) hi - (size_t) lo + 1;
}

EaselSelectionResult
easel_lasso_shape (const EaselPath *p, int img_w, int img_h, int *x, int *y, int *w, int *h, uint8_t **mask, size_t max_bytes)
{
  Work k = { .b = { 0, max_bytes } };
  bool same = true;
  size_t row_v = 0, prev_start = 0, prev_end = 0;
  int ymin = INT_MAX, ymax = INT_MIN;
  int min_x = INT_MAX, min_y = INT_MAX, max_x = INT_MIN, max_y = INT_MIN;
  uint8_t *out;
  EaselSelectionResult result = EASEL_SELECTION_NO_MEMORY;

  *mask = NULL;
  for (size_t i = 1; i < p->n; i++)
    same = same && p->xy[2 * i] == p->xy[0] && p->xy[2 * i + 1] == p->xy[1];
  if (p->n == 0 || same)
    return EASEL_SELECTION_NOTHING;

  /* 1. The grid segments of the closed path: each walk from one corner to the next, and from the last to the first. */
  for (size_t i = 0; i < p->n; i++)
    {
      size_t j = (i + 1) % p->n;
      EaselLattice l;
      int lx, ly, px = 0, py = 0;
      bool first = true;

      easel_lattice_init (&l, p->xy[2 * i], p->xy[2 * i + 1], p->xy[2 * j], p->xy[2 * j + 1]);
      while (easel_lattice_next (&l, &lx, &ly))
        {
          if (!first)
            {
              if (ly == py)
                {
                  if (!grow ((void **) &k.h, &k.cap_h, sizeof *k.h, k.n_h + 1, &k.b))
                    goto done;
                  k.h[k.n_h++] = (Seg) { ly, lx < px ? lx : px };
                }
              else
                {
                  if (!grow ((void **) &k.v, &k.cap_v, sizeof *k.v, k.n_v + 1, &k.b))
                    goto done;
                  k.v[k.n_v++] = (Seg) { ly < py ? ly : py, lx };
                }
            }
          first = false;
          px = lx;
          py = ly;
        }
    }
  k.n_h = sort_unique (k.h, k.n_h);
  k.n_v = sort_unique (k.v, k.n_v);
  /* The rows the path covers: the pixel rows from ymin to ymax - 1. */
  for (size_t i = 0; i < k.n_h; i++)
    {
      ymin = k.h[i].y < ymin ? k.h[i].y : ymin;
      ymax = k.h[i].y > ymax ? k.h[i].y : ymax;
    }
  for (size_t i = 0; i < k.n_v; i++)
    {
      ymin = k.v[i].y < ymin ? k.v[i].y : ymin;
      ymax = k.v[i].y + 1 > ymax ? k.v[i].y + 1 : ymax;
    }
  if (ymin >= ymax || k.n_v == 0)
    {
      result = EASEL_SELECTION_NOTHING;               /* a path with no vertical segment, or no height, encloses nothing */
      goto done;
    }

  /* 2. Row by row, the runs of pixels between the vertical segments, and the rows above and below the path. A run is joined to the
     runs of the row before it that it shares a column with and that no horizontal segment cuts off: node 0 is outside, to which the
     two rows beyond the path and the ends of every row belong. */
  if (!push_run (&k, 0, 1, 0, false))
    goto done;
  if (!push_run (&k, ymin - 1, INT_MIN, INT_MAX, true))
    goto done;
  prev_start = 1;
  prev_end = 2;
  for (int r = ymin; r <= ymax; r++)
    {
      size_t cur_start = k.n_run, cur_end;

      if (r == ymax)
        {
          if (!push_run (&k, r, INT_MIN, INT_MAX, true))
            goto done;
        }
      else
        {
          size_t first_v = row_v;

          while (row_v < k.n_v && k.v[row_v].y == r)
            row_v++;
          if (row_v == first_v)
            {
              if (!push_run (&k, r, INT_MIN, INT_MAX, true))
                goto done;
            }
          else
            {
              if (!push_run (&k, r, INT_MIN, k.v[first_v].x - 1, true))
                goto done;
              for (size_t i = first_v; i + 1 < row_v; i++)
                if (!push_run (&k, r, k.v[i].x, k.v[i + 1].x - 1, false))
                  goto done;
              if (!push_run (&k, r, k.v[row_v - 1].x, INT_MAX, true))
                goto done;
            }
        }
      cur_end = k.n_run;
      {
        size_t a = prev_start, c = cur_start;

        while (a < prev_end && c < cur_end)
          {
            int lo = k.run[a].lo > k.run[c].lo ? k.run[a].lo : k.run[c].lo;
            int hi = k.run[a].hi < k.run[c].hi ? k.run[a].hi : k.run[c].hi;

            if (lo <= hi && !blocked (&k, r, lo, hi))
              join (k.parent, (int) a, (int) c);
            if (k.run[a].hi < k.run[c].hi)
              a++;
            else
              c++;
          }
      }
      prev_start = cur_start;
      prev_end = cur_end;
    }

  /* 3. The bounding rectangle of the enclosed pixels that are inside the picture. */
  for (size_t i = 2; i < k.n_run; i++)
    {
      int lo = k.run[i].lo > 0 ? k.run[i].lo : 0, hi = k.run[i].hi < img_w - 1 ? k.run[i].hi : img_w - 1;

      if (find (k.parent, (int) i) == 0 || k.run[i].y < 0 || k.run[i].y >= img_h || lo > hi)
        continue;
      min_x = lo < min_x ? lo : min_x;
      max_x = hi > max_x ? hi : max_x;
      min_y = k.run[i].y < min_y ? k.run[i].y : min_y;
      max_y = k.run[i].y > max_y ? k.run[i].y : max_y;
    }
  if (min_x > max_x)
    {
      result = EASEL_SELECTION_NOTHING;
      goto done;
    }
  *x = min_x;
  *y = min_y;
  *w = max_x - min_x + 1;
  *h = max_y - min_y + 1;
  if ((size_t) *w > SIZE_MAX / (size_t) *h || k.b.used + (size_t) *w * (size_t) *h > max_bytes)
    goto done;
  out = calloc ((size_t) *w * (size_t) *h, 1);
  if (out == NULL)
    goto done;

  /* 4. The mask. */
  for (size_t i = 2; i < k.n_run; i++)
    {
      int lo = k.run[i].lo > min_x ? k.run[i].lo : min_x, hi = k.run[i].hi < max_x ? k.run[i].hi : max_x;

      if (find (k.parent, (int) i) == 0 || k.run[i].y < min_y || k.run[i].y > max_y)
        continue;
      for (int c = lo; c <= hi; c++)
        out[(size_t) (k.run[i].y - min_y) * (size_t) *w + (size_t) (c - min_x)] = 1;
    }
  *mask = out;
  result = EASEL_SELECTION_DONE;
done:
  work_free (&k);
  return result;
}
