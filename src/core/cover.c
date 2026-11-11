/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "cover.h"

#include <stdbool.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

uint32_t
easel_cover_mix (uint32_t old, uint32_t c_out, unsigned n_out, uint32_t c_in, unsigned n_in)
{
  uint64_t rest = 256u - n_out - n_in, a = old >> 24, ao = c_out >> 24, ai = c_in >> 24;
  uint64_t m = a * rest + ao * n_out + ai * n_in, alpha = (2 * m + 256) / 512;
  uint32_t r = 0;

  if (alpha == 0)
    return 0;                                                            /* P7.7 */
  /* Divided by M, not by the rounded alpha, so that a color over a fully transparent pixel keeps its red, green and blue. */
  for (int sh = 0; sh <= 16; sh += 8)
    {
      uint64_t num = (old >> sh & 0xFF) * a * rest + (c_out >> sh & 0xFF) * ao * n_out + (c_in >> sh & 0xFF) * ai * n_in;
      uint64_t v = (2 * num + m) / (2 * m);

      r |= (uint32_t) (v > 255 ? 255 : v) << sh;
    }
  return (uint32_t) alpha << 24 | r;
}

static int64_t
floor_div16 (int64_t v)
{
  return v >= 0 ? v / 16 : -((-v + 15) / 16);
}

static int
cmp_event (const void *a, const void *b)
{
  const EaselCoverEvent *p = a, *q = b;

  return p->at < q->at ? -1 : p->at > q->at;
}

typedef struct
{
  EaselCoverRun *runs;
  size_t n, max;
  bool full;
} Out;

/* Adds pixels x0 to x1 with these counts, joined to the run before when they continue it. */
static void
emit (Out *o, int64_t x0, int64_t x1, unsigned out, unsigned in)
{
  EaselCoverRun *last = o->n > 0 ? &o->runs[o->n - 1] : NULL;

  if (out + in == 0)
    return;
  if (last != NULL && last->x1 + 1 == x0 && last->out == out && last->in == in)
    {
      last->x1 = (int) x1;
      return;
    }
  if (o->n == o->max)
    {
      o->full = true;
      return;
    }
  o->runs[o->n++] = (EaselCoverRun) { (int) x0, (int) x1, (uint16_t) out, (uint16_t) in };
}

size_t
easel_cover_count (const EaselCoverSpan *spans, size_t n, EaselCoverEvent *scratch, EaselCoverRun *runs, size_t max)
{
  Out o = { runs, 0, max, false };
  int64_t px = 0, s = 0;
  unsigned depth[2] = { 0, 0 }, acc[2] = { 0, 0 };
  bool have_px = false;

  for (size_t k = 0; k < n; k++)
    {
      scratch[2 * k] = (EaselCoverEvent) { spans[k].a, spans[k].part, 1 };
      scratch[2 * k + 1] = (EaselCoverEvent) { spans[k].b + 1, spans[k].part, -1 };
    }
  qsort (scratch, 2 * n, sizeof *scratch, cmp_event);
  for (size_t k = 0; k < 2 * n; k++)
    {
      int64_t e = scratch[k].at;

      /* The columns s to e - 1 have the depths of now: how many sample rows cover them, per part. */
      while ((depth[0] | depth[1]) != 0 && s < e)
        {
          int64_t p = floor_div16 (s), full = floor_div16 (e) - p, t;

          if (!have_px || p != px)
            {
              if (have_px)
                emit (&o, px, px, acc[0], acc[1]);
              px = p;
              have_px = true;
              acc[0] = acc[1] = 0;
            }
          if (s == 16 * p && full >= 1)
            {
              /* whole pixels: 16 columns of each covering sample row */
              emit (&o, p, p + full - 1, 16 * depth[0], 16 * depth[1]);
              have_px = false;
              s = 16 * (p + full);
              continue;
            }
          t = e < 16 * p + 16 ? e : 16 * p + 16;
          acc[0] += (unsigned) (t - s) * depth[0];
          acc[1] += (unsigned) (t - s) * depth[1];
          s = t;
        }
      s = e;
      depth[scratch[k].part] += (unsigned) scratch[k].delta;
    }
  if (have_px)
    emit (&o, px, px, acc[0], acc[1]);
  return o.full ? 0 : o.n;
}

typedef __int128 I128;

/* floor (v / d) and ceil (v / d) for d > 0. */
static I128
floor_div (I128 v, I128 d)
{
  return v >= 0 ? v / d : -((-v + d - 1) / d);
}

static I128
ceil_div (I128 v, I128 d)
{
  return -floor_div (-v, d);
}

/* floor (sqrt (v)) for v >= 0, exact: an estimate in long double, then corrected. */
static I128
isqrt (I128 v)
{
  I128 r = (I128) sqrtl ((long double) v);

  while (r > 0 && r * r > v)
    r--;
  while ((r + 1) * (r + 1) <= v)
    r++;
  return r;
}

/* Narrows the columns [*lo, *hi] to those c whose point P = 2 c + 1 has a P + b between min and max; false if none is left. */
static bool
narrow (I128 a, I128 b, I128 min, I128 max, I128 *lo, I128 *hi)
{
  I128 l = min - b - a, h = max - b - a, d = 2 * a;

  if (d == 0)
    return l <= 0 && 0 <= h;
  if (d < 0)
    {
      I128 t = -l;

      l = -h;
      h = t;
      d = -d;
    }
  if (ceil_div (l, d) > *lo)
    *lo = ceil_div (l, d);
  if (floor_div (h, d) < *hi)
    *hi = floor_div (h, d);
  return *lo <= *hi;
}

static bool
result (I128 lo, I128 hi, int64_t *ca, int64_t *cb)
{
  if (lo > hi)
    return false;
  *ca = (int64_t) lo;
  *cb = (int64_t) hi;
  return true;
}

bool
easel_cover_band (int64_t ax, int64_t ay, int64_t bx, int64_t by, int64_t h, bool end_a, bool end_b, int64_t q, int64_t *ca,
                  int64_t *cb)
{
  I128 dx = (I128) bx - ax, dy = (I128) by - ay, vy = (I128) q - ay, l2 = dx * dx + dy * dy, k;
  I128 lo = -((I128) 1 << 100), hi = (I128) 1 << 100;

  if (l2 == 0)
    {
      if (!end_a || !end_b || vy * vy > (I128) h * h)
        return false;
      return narrow (1, -(I128) ax, -(I128) h, h, &lo, &hi) && result (lo, hi, ca, cb);
    }
  /* |v x d| <= h |d| and -h |d| <= v . d <= |d|^2 + h |d| (or 0 and |d|^2 without the ends); both sides are integers, so
     h |d| may be taken down to the whole number K. */
  k = isqrt ((I128) h * h * l2);
  /* v x d = (P - ax) dy - vy dx;  v . d = (P - ax) dx + vy dy */
  if (!narrow (dy, -(I128) ax * dy - vy * dx, -k, k, &lo, &hi))
    return false;
  if (!narrow (dx, -(I128) ax * dx + vy * dy, end_a ? -k : 0, end_b ? l2 + k : l2, &lo, &hi))
    return false;
  return result (lo, hi, ca, cb);
}

bool
easel_cover_disk (int64_t cx, int64_t cy, int64_t r, int64_t q, int64_t *ca, int64_t *cb)
{
  I128 dy = (I128) q - cy, rest = (I128) r * r - dy * dy, m, lo = -((I128) 1 << 100), hi = (I128) 1 << 100;

  if (rest < 0)
    return false;
  m = isqrt (rest);
  return narrow (1, -(I128) cx, -m, m, &lo, &hi) && result (lo, hi, ca, cb);
}

/* The least and greatest x where the sample row q crosses the convex polygon of n vertices v (x, y pairs, in order), as fractions;
   a vertex's y is a multiple of 16 and q is odd, so the row never passes through one. False if it misses the polygon. */
static bool
polygon_row (const int64_t *v, int n, int64_t q, I128 *lo_n, I128 *lo_d, I128 *hi_n, I128 *hi_d)
{
  bool any = false;

  for (int k = 0; k < n; k++)
    {
      const int64_t *a = &v[2 * k], *b = &v[2 * ((k + 1) % n)];
      I128 num, den;

      if ((a[1] < q) == (b[1] < q))
        continue;
      /* x = ax + (q - ay) (bx - ax) / (by - ay), with a positive denominator */
      den = (I128) b[1] - a[1];
      num = (I128) a[0] * den + ((I128) q - a[1]) * ((I128) b[0] - a[0]);
      if (den < 0)
        {
          num = -num;
          den = -den;
        }
      if (!any || num * *lo_d < *lo_n * den)
        *lo_n = num, *lo_d = den;
      if (!any || num * *hi_d > *hi_n * den)
        *hi_n = num, *hi_d = den;
      any = true;
    }
  return any;
}

/* Widens [*ca, *cb] (or sets it, when *hit is false) by the columns whose point 2 c + 1 lies between lo and hi. */
static void
widen (I128 lo_n, I128 lo_d, I128 hi_n, I128 hi_d, bool *hit, int64_t *ca, int64_t *cb)
{
  I128 a = ceil_div (lo_n - lo_d, 2 * lo_d), b = floor_div (hi_n - hi_d, 2 * hi_d);

  if (a > b)
    return;
  if (!*hit || a < *ca)
    *ca = (int64_t) a;
  if (!*hit || b > *cb)
    *cb = (int64_t) b;
  *hit = true;
}

/* The footprint's polygon (not a circle of more than 1) placed with its box's top left at (l, t), side s: its vertices in order. */
static int
footprint_polygon (EaselFootprint f, int64_t l, int64_t t, int64_t s, int64_t *v)
{
  if (f.shape == EASEL_SHAPE_SLASH)
    {
      /* the points of the box within 1/2 along x of the diagonal from its bottom-left corner to its top-right one */
      const int64_t p[12] = { l, t + s - 16, l + s - 16, t, l + s, t, l + s, t + 16, l + 16, t + s, l, t + s };

      memcpy (v, p, sizeof p);
      return 6;
    }
  if (f.shape == EASEL_SHAPE_BACKSLASH)
    {
      const int64_t p[12] = { l, t, l + 16, t, l + s, t + s - 16, l + s, t + s, l + s - 16, t + s, l, t + 16 };

      memcpy (v, p, sizeof p);
      return 6;
    }
  {
    const int64_t p[8] = { l, t, l + s, t, l + s, t + s, l, t + s };

    memcpy (v, p, sizeof p);
    return 4;
  }
}

bool
easel_cover_sweep (EaselFootprint f, int x0, int y0, int x1, int y1, int64_t q, int64_t *ca, int64_t *cb)
{
  int64_t a = (f.size - 1) / 2, s = 32 * (int64_t) f.size;
  int64_t l = 32 * ((int64_t) x0 - a), t = 32 * ((int64_t) y0 - a), vx = 32 * ((int64_t) x1 - x0), vy = 32 * ((int64_t) y1 - y0);
  int64_t v[12], piece[12], pa, pb;
  I128 ln, ld, hn, hd;
  bool hit = false;
  int n;

  if (f.size < 1)
    return false;
  if (f.shape == EASEL_SHAPE_CIRCLE && f.size > 1)
    {
      /* a capsule: the disks at both ends and the band between them; convex, so the row is one interval */
      int64_t cx = l + s / 2, cy = t + s / 2;

      if (easel_cover_disk (cx, cy, s / 2, q, &pa, &pb))
        widen (2 * (I128) pa + 1, 1, 2 * (I128) pb + 1, 1, &hit, ca, cb);
      if (easel_cover_disk (cx + vx, cy + vy, s / 2, q, &pa, &pb))
        widen (2 * (I128) pa + 1, 1, 2 * (I128) pb + 1, 1, &hit, ca, cb);
      if (easel_cover_band (cx, cy, cx + vx, cy + vy, s / 2, false, false, q, &pa, &pb))
        widen (2 * (I128) pa + 1, 1, 2 * (I128) pb + 1, 1, &hit, ca, cb);
      return hit;
    }
  /* the polygon at both ends and each of its edges swept: together the convex hull of the two, so the row is one interval */
  n = footprint_polygon (f, l, t, s, v);
  if (polygon_row (v, n, q, &ln, &ld, &hn, &hd))
    widen (ln, ld, hn, hd, &hit, ca, cb);
  if (vx == 0 && vy == 0)
    return hit;
  for (int k = 0; k < n; k++)
    piece[2 * k] = v[2 * k] + vx, piece[2 * k + 1] = v[2 * k + 1] + vy;
  if (polygon_row (piece, n, q, &ln, &ld, &hn, &hd))
    widen (ln, ld, hn, hd, &hit, ca, cb);
  for (int k = 0; k < n; k++)
    {
      const int64_t *p = &v[2 * k], *r = &v[2 * ((k + 1) % n)];
      int64_t e[8] = { p[0], p[1], r[0], r[1], r[0] + vx, r[1] + vy, p[0] + vx, p[1] + vy };

      if (polygon_row (e, 4, q, &ln, &ld, &hn, &hd))
        widen (ln, ld, hn, hd, &hit, ca, cb);
    }
  return hit;
}

unsigned
easel_cover_footprint (EaselFootprint f, int col, int row)
{
  int a = (f.size - 1) / 2;
  unsigned n = 0;

  for (int j = 0; j < 16; j++)
    {
      int64_t ca, cb, lo = 16 * (int64_t) col, hi = lo + 15;

      if (easel_cover_sweep (f, a, a, a, a, 32 * (int64_t) row + 2 * j + 1, &ca, &cb) && ca <= hi && cb >= lo)
        n += (unsigned) ((cb < hi ? cb : hi) - (ca > lo ? ca : lo) + 1);
    }
  return n;
}
