/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Prototype v6 (2026-11-10, JOD's request for complex backgrounds; one round): src/core/remove-bg.c as of 0037 with three switches:
   V6_CLUSTERS, the ring's colors in up to 6 clusters (k-means from the farthest points, 12 rounds, each 10 % of the ring or more, with
   CL_BETTER only where they explain the ring better than the surface, CL_CORNER each owning a corner); V6_HIST, colour histograms of
   16 bins a channel for the subject and the background, every cell not a seed reclassified by the better fit, V6_HIST rounds. Kept:
   the clusters with CL_BETTER (now in the app). Dropped: CL_CORNER, the histograms and their refits. Results:
   ../methods-comparison.md, "v6", and ../v6-sheet.png. Not thread-safe (a static seed buffer): a prototype only. */
#include "remove-bg.h"

#include <stdlib.h>
#include <string.h>

typedef __int128 i128;

enum { ORDER_TERMS = 10, SEED_FAR = 4, BUCKETS = 1024, AUTO_FLOOR = 5, AUTO_KNEE64 = 195, AUTO_SLOPE = 3, AUTO_MAX = 20, FIT_SAMPLES = 8192 };
enum { NONE = 0, BACK = 1, SUBJECT = 2, ABSENT = 3 };
#ifndef V6_CLUSTERS
#define V6_CLUSTERS 0
#endif
#ifndef V6_HIST
#define V6_HIST 0                  /* (2) the colour histograms' reclassification, (3) refitted V6_HIST - 1 more times */
#endif
#ifndef CL_K
#define CL_K 6
#endif
#ifndef CL_CORNER
#define CL_CORNER 0
#endif
#ifndef CL_BETTER
#define CL_BETTER 0
#endif
#ifndef CL_SHARE
#define CL_SHARE 10
#endif
/* (a) The robust fit: the ring's samples beyond ROBUST_K times their median error (at least 1 delta-E) dropped and the surface refitted,
   ROBUST_ROUNDS times. (b) The colour line: its reach toward darker past the ring's own, in % of the ring's range, and toward brighter
   the least Lab chroma of a cell that it judges (2026-11-10, ADR-0007). */
enum { ROBUST_K = 3, ROBUST_ROUNDS = 3, LINE_DARK_MARGIN = 25, LINE_BRIGHT_MARGIN = 400, LINE_MIN_CHROMA = 12 };
/* The fringe (REQ-0035-03 g): how far from the blend's line a mixed pixel may lie, as a part of the line's length; the step in color
   below which the color inward has settled, in quarters of the tolerance; its rounds, and how far inward it looks, in pixels. Chosen on the eagle photo,
   the six test pictures and the five wallpapers (docs/reference/0035/fringe-sheet.png). */
enum { FRINGE_FIT_DIV = 8, FRINGE_SETTLE_Q4 = 2, FRINGE_ROUNDS = 1, FRINGE_REACH = 16 };

/* sRGB to linear light, Q16 (from the sRGB formula, kept as integers), and XYZ from it, each row divided by the D65 white, Q16. */
static const int32_t LIN[256] = { 0, 20, 40, 60, 80, 99, 119, 139, 159, 179, 199, 219, 241, 264, 288, 313, 340, 367, 396, 427, 458, 491, 526, 562, 599, 637, 677, 718, 761, 805, 851, 898, 947, 997, 1048, 1101, 1156, 1212, 1270, 1330, 1391, 1453, 1517, 1583, 1651, 1720, 1790, 1863, 1937, 2013, 2090, 2170, 2250, 2333, 2418, 2504, 2592, 2681, 2773, 2866, 2961, 3058, 3157, 3258, 3360, 3464, 3570, 3678, 3788, 3900, 4014, 4129, 4247, 4366, 4488, 4611, 4736, 4864, 4993, 5124, 5257, 5392, 5530, 5669, 5810, 5953, 6099, 6246, 6395, 6547, 6700, 6856, 7014, 7174, 7335, 7500, 7666, 7834, 8004, 8177, 8352, 8528, 8708, 8889, 9072, 9258, 9445, 9635, 9828, 10022, 10219, 10417, 10619, 10822, 11028, 11235, 11446, 11658, 11873, 12090, 12309, 12530, 12754, 12980, 13209, 13440, 13673, 13909, 14146, 14387, 14629, 14874, 15122, 15371, 15623, 15878, 16135, 16394, 16656, 16920, 17187, 17456, 17727, 18001, 18277, 18556, 18837, 19121, 19407, 19696, 19987, 20281, 20577, 20876, 21177, 21481, 21787, 22096, 22407, 22721, 23038, 23357, 23678, 24002, 24329, 24658, 24990, 25325, 25662, 26001, 26344, 26688, 27036, 27386, 27739, 28094, 28452, 28813, 29176, 29542, 29911, 30282, 30656, 31033, 31412, 31794, 32179, 32567, 32957, 33350, 33745, 34143, 34544, 34948, 35355, 35764, 36176, 36591, 37008, 37429, 37852, 38278, 38706, 39138, 39572, 40009, 40449, 40891, 41337, 41785, 42236, 42690, 43147, 43606, 44069, 44534, 45002, 45473, 45947, 46423, 46903, 47385, 47871, 48359, 48850, 49344, 49841, 50341, 50844, 51349, 51858, 52369, 52884, 53401, 53921, 54445, 54971, 55500, 56032, 56567, 57105, 57646, 58190, 58737, 59287, 59840, 60396, 60955, 61517, 62082, 62650, 63221, 63795, 64372, 64952, 65535 };
static const int32_t XYZ[3][3] = { { 28439, 24655, 12441 }, { 13938, 46868, 4730 }, { 1164, 7174, 57198 } };

struct EaselRbg
{
  int w, h, s, cw, ch, d;          /* the pixels, the block's side, the copy, the ring's depth */
  size_t n;                        /* the copy's cells */
  int16_t *lab;                    /* L, a, b x 64 a cell */
  uint8_t *present;                /* the cell stands for pixels */
  uint8_t *rgb;                    /* the cell's mean color, 3 bytes */
  int64_t lm[3], lv[3], llo, lhi;  /* (b) the colour line: a point (x 256), its unit direction (x 65536), its reach along it (x 256) */
  bool line_ok;                    /* the line judges: a colored backdrop the line explains at least as well as the surface */
  int64_t line_lim;                /* while fitting the surface: only the ring's samples this near the line (0: all) */
  int64_t line_mad;                /* the line's median distance squared on the ring, at least 1 delta-E squared */
  int ncl;                         /* v6 (1): the ring's color clusters */
  int16_t cl[CL_K][3];
  int32_t *piece;                  /* 0037: each cell's piece in the last flips (the pixels' up to EASEL_RBG_COPY_MAX), -1 for none */
  bool pieces_made;
  uint8_t *flood;                  /* 0037: each cell's label from the flood, before the regions rule (c) */
  uint16_t *edge;                  /* the edge strength, 0 .. BUCKETS - 1 */
  uint8_t *label;                  /* NONE, BACK, SUBJECT, ABSENT */
  int32_t *next;                   /* the watershed's queues, then the regions' numbers */
  uint32_t *queue, *sizes;         /* a fill's queue; the regions' sizes */
  int32_t *ft;                     /* CIE Lab's f(t) for t = 0 .. 65536 (Q16) */
  int64_t k0[3][ORDER_TERMS], k[3][ORDER_TERMS];
  int64_t residual;
  int auto_tol;
  int tolerance;                   /* the labels' */
  uint8_t *marks;                  /* the fringe's pixels to remove in a round, a bit each */
  size_t bytes;
};

static int64_t line_d2 (const EaselRbg *r, const uint8_t *c);

/* ---- Lab ---- */

static uint64_t
icbrt (uint64_t v)
{
  uint64_t r = 0;

  for (int sh = 63; sh >= 0; sh -= 3)
    {
      uint64_t b;

      r <<= 1;
      b = 3 * r * (r + 1) + 1;            /* the next root bit's test, after the shift (2026-11-10: was 3 r (2 r + 1) + 1 before it) */
      if ((v >> sh) >= b)
        {
          v -= b << sh;
          r++;
        }
    }
  return r;
}

static uint64_t
isqrt64 (uint64_t v)
{
  uint64_t r = 0, b = (uint64_t) 1 << 62;

  while (b > v)
    b >>= 2;
  while (b != 0)
    {
      if (v >= r + b)
        {
          v -= r + b;
          r = (r >> 1) + b;
        }
      else
        r >>= 1;
      b >>= 2;
    }
  return r;
}

static int32_t
f_of (const EaselRbg *r, int64_t t)
{
  return r->ft[t < 0 ? 0 : t > 65536 ? 65536 : t];
}

static void
to_lab (const EaselRbg *r, int red, int green, int blue, int16_t out[3])
{
  int64_t lr = LIN[red], lg = LIN[green], lb = LIN[blue];
  int32_t fx = f_of (r, (XYZ[0][0] * lr + XYZ[0][1] * lg + XYZ[0][2] * lb) >> 16);
  int32_t fy = f_of (r, (XYZ[1][0] * lr + XYZ[1][1] * lg + XYZ[1][2] * lb) >> 16);
  int32_t fz = f_of (r, (XYZ[2][0] * lr + XYZ[2][1] * lg + XYZ[2][2] * lb) >> 16);

  out[0] = (int16_t) ((116 * (int64_t) fy - 16 * 65536) >> 10);
  out[1] = (int16_t) ((500 * (int64_t) (fx - fy)) >> 10);
  out[2] = (int16_t) ((200 * (int64_t) (fy - fz)) >> 10);
}

static int64_t
de2 (const int16_t *a, const int16_t *b)
{
  int64_t d0 = a[0] - b[0], d1 = a[1] - b[1], d2 = a[2] - b[2];

  return d0 * d0 + d1 * d1 + d2 * d2;
}

/* ---- the surface: per channel sum k_j phi_j, phi = 1, u, v, u^2, uv, v^2, u^3, u^2 v, u v^2, v^3; u, v in -1 .. 1 (Q8) ---- */

static void
basis (const EaselRbg *r, int x, int y, int64_t phi[ORDER_TERMS])
{
  int64_t u = r->cw > 1 ? ((int64_t) x * 512 / (r->cw - 1)) - 256 : 0, v = r->ch > 1 ? ((int64_t) y * 512 / (r->ch - 1)) - 256 : 0;

  phi[0] = 256, phi[1] = u, phi[2] = v, phi[3] = u * u >> 8, phi[4] = u * v >> 8, phi[5] = v * v >> 8;
  phi[6] = phi[3] * u >> 8, phi[7] = phi[3] * v >> 8, phi[8] = phi[5] * u >> 8, phi[9] = phi[5] * v >> 8;
}

static void
surface_at (const EaselRbg *r, const int64_t (*k)[ORDER_TERMS], int x, int y, int16_t out[3])
{
  int64_t phi[ORDER_TERMS];

  basis (r, x, y, phi);
  for (int c = 0; c < 3; c++)
    {
      int64_t s = 0;

      for (int j = 0; j < ORDER_TERMS; j++)
        s += k[c][j] * phi[j];
      s >>= 24;
      out[c] = (int16_t) (s < -32768 ? -32768 : s > 32767 ? 32767 : s);
    }
}

static bool
in_ring (const EaselRbg *r, int x, int y)
{
  size_t i = (size_t) y * r->cw + x;

  if (!r->present[i])
    return false;
  if (x < r->d || y < r->d || x >= r->cw - r->d || y >= r->ch - r->d)
    return true;
  return !r->present[i - 1] || !r->present[i + 1] || !r->present[i - r->cw] || !r->present[i + r->cw];
}

static int
ring_stride (const EaselRbg *r)
{
  size_t ring = 2 * ((size_t) r->cw + r->ch) * (size_t) r->d;

  return (int) (ring / FIT_SAMPLES) + 1;
}

/* A least-squares fit to the ring's cells (every ring_stride-th), leaving out those farther than limit2 from the first fit when
   refitting. Gaussian elimination in 128-bit integers, the pivot the largest in its column (ties: the first). */
static void
fit (const EaselRbg *r, bool refit, int64_t limit2, int64_t (*k)[ORDER_TERMS])
{
  int64_t a[ORDER_TERMS][ORDER_TERMS] = { { 0 } }, rhs[3][ORDER_TERMS] = { { 0 } }, n = 0, phi[ORDER_TERMS];
  int stride = ring_stride (r), count = 0;

  memset (k, 0, 3 * sizeof *k);
  for (int y = 0; y < r->ch; y++)
    for (int x = 0; x < r->cw; x++)
      {
        const int16_t *p = r->lab + 3 * ((size_t) y * r->cw + x);
        int16_t s[3];

        if (!in_ring (r, x, y) || (count++ % stride) != 0)
          continue;
        if (r->line_lim > 0 && line_d2 (r, r->rgb + 3 * ((size_t) y * r->cw + x)) > r->line_lim)
          continue;                    /* (b) off the colour line: not the background's, not fitted */
        if (refit)
          {
            surface_at (r, (const int64_t (*)[ORDER_TERMS]) r->k0, x, y, s);
            if (de2 (p, s) > limit2)
              continue;
          }
        basis (r, x, y, phi);
        for (int i = 0; i < ORDER_TERMS; i++)
          {
            for (int j = 0; j < ORDER_TERMS; j++)
              a[i][j] += phi[i] * phi[j];
            for (int c = 0; c < 3; c++)
              rhs[c][i] += phi[i] * p[c];
          }
        n++;
      }
  if (n < ORDER_TERMS)
    {
      /* too few cells for the cubic: a flat surface, the mean of those there are (none: zero) */
      for (int c = 0; c < 3 && n > 0; c++)
        k[c][0] = (int64_t) (((i128) rhs[c][0] << 24) / (n * 256 * 256));
      return;
    }
  for (int c = 0; c < 3; c++)
    {
      i128 m[ORDER_TERMS][ORDER_TERMS + 1];

      for (int i = 0; i < ORDER_TERMS; i++)
        {
          for (int j = 0; j < ORDER_TERMS; j++)
            m[i][j] = a[i][j] / n;
          m[i][ORDER_TERMS] = ((i128) rhs[c][i] << 24) / n;
        }
      for (int col = 0; col < ORDER_TERMS; col++)
        {
          int piv = col;

          for (int i = col + 1; i < ORDER_TERMS; i++)
            if ((m[i][col] < 0 ? -m[i][col] : m[i][col]) > (m[piv][col] < 0 ? -m[piv][col] : m[piv][col]))
              piv = i;
          if (piv != col)
            for (int j = 0; j <= ORDER_TERMS; j++)
              {
                i128 t = m[col][j];

                m[col][j] = m[piv][j];
                m[piv][j] = t;
              }
          if (m[col][col] == 0)
            continue;
          for (int i = col + 1; i < ORDER_TERMS; i++)
            {
              i128 f = m[i][col], d = m[col][col];

              for (int j = col; j <= ORDER_TERMS; j++)
                m[i][j] -= m[col][j] * f / d;
            }
        }
      for (int i = ORDER_TERMS - 1; i >= 0; i--)
        {
          i128 s = m[i][ORDER_TERMS];

          for (int j = i + 1; j < ORDER_TERMS; j++)
            s -= m[i][j] * (i128) k[c][j];
          k[c][i] = m[i][i] != 0 ? (int64_t) (s / m[i][i]) : 0;
        }
    }
}

/* (b) The distance squared, delta-E x 64, of an RGB color to the colour line: the line's nearest point (held to its reach) and the color
   compared in Lab. */
static int64_t
line_d2 (const EaselRbg *r, const uint8_t *c)
{
  int64_t t = 0;
  int q[3];
  int16_t lq[3], lc[3];

  for (int k = 0; k < 3; k++)
    t += ((int64_t) c[k] * 256 - r->lm[k]) * r->lv[k];
  t >>= 16;                                   /* along the line, x 256 */
  if (t < r->llo)
    t = r->llo;
  if (t > r->lhi)
    t = r->lhi;
  for (int k = 0; k < 3; k++)
    {
      int64_t v = (r->lm[k] + (t * r->lv[k] >> 16)) / 256;

      q[k] = (int) (v < 0 ? 0 : v > 255 ? 255 : v);
    }
  to_lab (r, q[0], q[1], q[2], lq);
  to_lab (r, c[0], c[1], c[2], lc);
  return de2 (lc, lq);
}

/* The distance squared of a cell's color to the background (REQ-0035-03 a): to the surface there, or to the colour line when it
   judges and the cell is colored (a grey or white cell is the surface's alone), whichever is less. */
static int64_t
bg_d2 (const EaselRbg *r, int cx, int cy)
{
  size_t c = (size_t) cy * r->cw + cx;
  int16_t sf[3];
  int64_t d;

  surface_at (r, (const int64_t (*)[ORDER_TERMS]) r->k, cx, cy, sf);
  d = de2 (r->lab + 3 * c, sf);
  if (r->line_ok
      && (int64_t) r->lab[3 * c + 1] * r->lab[3 * c + 1] + (int64_t) r->lab[3 * c + 2] * r->lab[3 * c + 2]
           >= (int64_t) LINE_MIN_CHROMA * 64 * LINE_MIN_CHROMA * 64)
    {
      int64_t l = line_d2 (r, r->rgb + 3 * c);

      if (l < d)
        d = l;
    }
  for (int k = 0; V6_CLUSTERS && k < r->ncl; k++)
    {
      int64_t e = de2 (r->lab + 3 * c, r->cl[k]);

      if (e < d)
        d = e;
    }
  return d;
}

static int
cmp64 (const void *a, const void *b)
{
  int64_t x = *(const int64_t *) a, y = *(const int64_t *) b;

  return (x > y) - (x < y);
}

/* The ring's median distance to the first fit (delta-E x 64); -1 when its sample list cannot get memory. */
static int64_t
median_residual (const EaselRbg *r, const int64_t (*k)[ORDER_TERMS])
{
  int stride = ring_stride (r), count = 0;
  size_t cap = 2 * ((size_t) r->cw + r->ch) * (size_t) r->d / (size_t) stride + 2, n = 0;
  int64_t *d = malloc (cap * sizeof *d), med;

  if (d == NULL)
    return -1;
  for (int y = 0; y < r->ch; y++)
    for (int x = 0; x < r->cw; x++)
      {
        int16_t s[3];

        if (!in_ring (r, x, y) || (count++ % stride) != 0 || n >= cap)
          continue;
        surface_at (r, k, x, y, s);
        d[n++] = de2 (r->lab + 3 * ((size_t) y * r->cw + x), s);
      }
  qsort (d, n, sizeof *d, cmp64);
  med = n ? (int64_t) isqrt64 ((uint64_t) d[n / 2]) : 0;
  free (d);
  return med;
}

/* REQ-0035-04's mapping: 5 while the median is at most 2 delta-E, then 2 more for each whole delta-E above, at most 20. */
/* v6 (1): the ring's colors in up to CL_K clusters, k-means from the farthest points, 12 rounds; a cluster under CL_SHARE % of the
   ring dropped */
static void
fit_clusters (EaselRbg *r)
{
  int stride = ring_stride (r), count = 0;
  size_t cap = 2 * ((size_t) r->cw + r->ch) * (size_t) r->d / (size_t) stride + 2, n = 0, cnt[CL_K];
  int16_t (*p)[3] = malloc (cap * sizeof *p), c[CL_K][3];
  int64_t mean[3] = { 0, 0, 0 };
  int K = 0;

  r->ncl = 0;
  if (p == NULL)
    return;
  for (int y = 0; y < r->ch; y++)
    for (int x = 0; x < r->cw; x++)
      if (in_ring (r, x, y) && (count++ % stride) == 0 && n < cap)
        memcpy (p[n++], r->lab + 3 * ((size_t) y * r->cw + x), sizeof p[0]);
  if (n < 30)
    {
      free (p);
      return;
    }
  for (size_t i = 0; i < n; i++)
    for (int k = 0; k < 3; k++)
      mean[k] += p[i][k];
  {
    int16_t m[3] = { (int16_t) (mean[0] / (int64_t) n), (int16_t) (mean[1] / (int64_t) n), (int16_t) (mean[2] / (int64_t) n) };

    for (K = 0; K < CL_K; K++)
      {
        int64_t far = -1;
        size_t at = 0;

        for (size_t i = 0; i < n; i++)
          {
            int64_t d = K == 0 ? de2 (p[i], m) : INT64_MAX;

            for (int q = 0; q < K; q++)
              d = de2 (p[i], c[q]) < d ? de2 (p[i], c[q]) : d;
            if (d > far)
              far = d, at = i;
          }
        memcpy (c[K], p[at], sizeof c[K]);
      }
  }
  for (int it = 0; it < 12; it++)
    {
      int64_t acc[CL_K][3] = { { 0 } };

      memset (cnt, 0, sizeof cnt);
      for (size_t i = 0; i < n; i++)
        {
          int b = 0;

          for (int q = 1; q < K; q++)
            if (de2 (p[i], c[q]) < de2 (p[i], c[b]))
              b = q;
          cnt[b]++;
          for (int k = 0; k < 3; k++)
            acc[b][k] += p[i][k];
        }
      for (int q = 0; q < K; q++)
        for (int k = 0; k < 3 && cnt[q]; k++)
          c[q][k] = (int16_t) (acc[q][k] / (int64_t) cnt[q]);
    }
  {
    /* a backdrop's color reaches a corner of the copy; a subject standing on an edge seldom does: a cluster must hold the most of the
       cells in a square of 3 d at one corner at least (CL_CORNER) */
    bool owns[CL_K] = { false };

    for (int corner = 0; corner < 4; corner++)
      {
        int side = 3 * r->d, x0 = corner & 1 ? r->cw - side : 0, y0 = corner & 2 ? r->ch - side : 0;
        size_t votes[CL_K] = { 0 };
        int best = 0;

        for (int y = y0 < 0 ? 0 : y0; y < y0 + side && y < r->ch; y++)
          for (int x = x0 < 0 ? 0 : x0; x < x0 + side && x < r->cw; x++)
            if (r->present[(size_t) y * r->cw + x])
              {
                const int16_t *lab = r->lab + 3 * ((size_t) y * r->cw + x);
                int b = 0;

                for (int q = 1; q < K; q++)
                  if (de2 (lab, c[q]) < de2 (lab, c[b]))
                    b = q;
                votes[b]++;
              }
        for (int q = 1; q < K; q++)
          if (votes[q] > votes[best])
            best = q;
        if (votes[best] > 0)
          owns[best] = true;
      }
    for (int q = 0; q < K; q++)
      if (cnt[q] * 100 >= n * CL_SHARE && (!CL_CORNER || owns[q]))
        memcpy (r->cl[r->ncl++], c[q], sizeof c[q]);
  }
  free (p);
}

/* (b) The colour line: the ray from black through the ring's median color (more or less light on one color; a median, so a subject on
   up to half the ring does not move it). Its reach: the ring's samples within ROBUST_K times their median distance, then
   LINE_DARK_MARGIN % of that range toward darker and LINE_BRIGHT_MARGIN % toward brighter. FALSE when memory cannot be had. */
static bool
fit_line (EaselRbg *r)
{
  int stride = ring_stride (r), count = 0;
  size_t cap = 2 * ((size_t) r->cw + r->ch) * (size_t) r->d / (size_t) stride + 2, n = 0, nd = 0;
  uint8_t (*p)[3] = malloc (cap * sizeof *p), *use = malloc (cap);
  int64_t *dist = malloc (cap * sizeof *dist), med[3], len, lo = INT64_MAX, hi = INT64_MIN;
  r->line_ok = false;
  if (p == NULL || use == NULL || dist == NULL)
    {
      free (p), free (use), free (dist);
      return false;
    }
  for (int y = 0; y < r->ch; y++)
    for (int x = 0; x < r->cw; x++)
      if (in_ring (r, x, y) && (count++ % stride) == 0 && n < cap)
        memcpy (p[n++], r->rgb + 3 * ((size_t) y * r->cw + x), 3);
  for (int k = 0; k < 3; k++)
    {
      for (size_t i = 0; i < n; i++)
        dist[i] = p[i][k];
      qsort (dist, n, sizeof *dist, cmp64);
      med[k] = n ? dist[n / 2] : 0;
    }
  if (n == 0 || med[0] + med[1] + med[2] == 0)
    {
      free (p), free (use), free (dist);
      return true;                             /* no ring, or a black one: no line */
    }
  len = (int64_t) isqrt64 ((uint64_t) (med[0] * med[0] + med[1] * med[1] + med[2] * med[2]));
  for (int k = 0; k < 3; k++)
    r->lm[k] = med[k] * 256, r->lv[k] = med[k] * 65536 / len;
  r->llo = INT64_MIN / 4, r->lhi = INT64_MAX / 4;
  for (size_t i = 0; i < n; i++)
    dist[i] = line_d2 (r, p[i]);
  qsort (dist, n, sizeof *dist, cmp64);
  {
    int64_t md = dist[n / 2], lim = (md < 64 * 64 ? 64 * 64 : md) * ROBUST_K * ROBUST_K;

    for (size_t i = 0; i < n; i++)
      use[i] = line_d2 (r, p[i]) <= lim;
  }
  for (size_t i = 0; i < n; i++)
    if (use[i])
      {
        int64_t t = 0;

        for (int k = 0; k < 3; k++)
          t += ((int64_t) p[i][k] * 256 - r->lm[k]) * r->lv[k];
        t >>= 16;
        lo = t < lo ? t : lo, hi = t > hi ? t : hi;
      }
  if (lo > hi)
    lo = hi = 0;
  r->llo = lo - (hi - lo) * LINE_DARK_MARGIN / 100;
  r->lhi = hi + (hi - lo) * LINE_BRIGHT_MARGIN / 100;
  for (size_t i = 0; i < n; i++)
    if (use[i])
      dist[nd++] = line_d2 (r, p[i]);
  qsort (dist, nd, sizeof *dist, cmp64);
  r->line_mad = nd && dist[nd / 2] > 64 * 64 ? dist[nd / 2] : 64 * 64;
  r->line_ok = true;
  free (p), free (use), free (dist);
  return true;
}

static int
auto_from (int64_t median64)
{
  /* REQ-0035-04 (2026-11-10, on a correct Lab): 5 while the median is at most 3.05 delta-E, then 3 more per delta-E above, rounded
     down; chosen over the whole test set for the worst subject kept, the knee centred in the window picture 04 needs
     (docs/reference/0035/methods-comparison.md, the re-tune) */
  int64_t t = AUTO_FLOOR + (median64 > AUTO_KNEE64 ? AUTO_SLOPE * (median64 - AUTO_KNEE64) / 64 : 0);

  return (int) (t > AUTO_MAX ? AUTO_MAX : t);
}

/* ---- the analysis ---- */

static bool
looked_at (const uint8_t *mask, size_t i)
{
  return mask == NULL || mask[i] != 0;
}

void
easel_rbg_free (EaselRbg *r)
{
  if (r == NULL)
    return;
  free (r->lab);
  free (r->present);
  free (r->rgb);
  free (r->piece);
  free (r->flood);
  free (r->edge);
  free (r->label);
  free (r->next);
  free (r->queue);
  free (r->sizes);
  free (r->ft);
  free (r->marks);
  free (r);
}

EaselRbg *
easel_rbg_new (const uint32_t *px, const uint8_t *mask, int w, int h, size_t max_bytes)
{
  EaselRbg *r;
  int s;
  size_t per_cell = 3 * sizeof (int16_t) + 1 + sizeof (uint16_t) + 1 + sizeof (int32_t) + 2 * sizeof (uint32_t) + 3 + sizeof (int32_t) + 1;

  if (w <= 0 || h <= 0)
    return NULL;
  for (s = 1; (size_t) ((w + s - 1) / s) * (size_t) ((h + s - 1) / s) > EASEL_RBG_COPY_MAX; s++)
    ;
  r = calloc (1, sizeof *r);
  if (r == NULL)
    return NULL;
  r->w = w, r->h = h, r->s = s;
  r->cw = (w + s - 1) / s, r->ch = (h + s - 1) / s;
  r->n = (size_t) r->cw * r->ch;
  r->d = (r->cw < r->ch ? r->cw : r->ch) / 100;
  if (r->d < 2)
    r->d = 2;
  r->bytes = sizeof *r + r->n * per_cell + 65537 * sizeof (int32_t) + ((size_t) w * (size_t) h + 7) / 8;
  if (r->bytes > max_bytes)
    {
      free (r);
      return NULL;
    }
  r->lab = malloc (r->n * 3 * sizeof *r->lab);
  r->present = malloc (r->n);
  r->rgb = malloc (r->n * 3);
  r->piece = malloc (r->n * sizeof *r->piece);
  r->flood = malloc (r->n);
  r->edge = malloc (r->n * sizeof *r->edge);
  r->label = malloc (r->n);
  r->next = malloc (r->n * sizeof *r->next);
  r->queue = malloc (r->n * sizeof *r->queue);
  r->sizes = malloc (r->n * sizeof *r->sizes);
  r->ft = malloc (65537 * sizeof *r->ft);
  r->marks = malloc (((size_t) w * (size_t) h + 7) / 8);
  if (r->marks == NULL || r->rgb == NULL || r->piece == NULL || r->flood == NULL || r->lab == NULL || r->present == NULL || r->edge == NULL || r->label == NULL || r->next == NULL || r->queue == NULL || r->sizes == NULL || r->ft == NULL)
    {
      easel_rbg_free (r);
      return NULL;
    }
  for (int t = 0; t <= 65536; t++)
    r->ft[t] = t > 580 ? (int32_t) icbrt ((uint64_t) t << 32)
                       : (int32_t) (((int64_t) t * 841 * 29 + (int64_t) 4 * 65536 * 108 + 108 * 29 / 2) / (108 * 29));   /* rounded once */

  /* the copy: each block's mean over its pixels looked at and not fully transparent */
  for (int cy = 0; cy < r->ch; cy++)
    for (int cx = 0; cx < r->cw; cx++)
      {
        uint64_t sum[3] = { 0, 0, 0 }, cnt = 0;
        size_t c = (size_t) cy * r->cw + cx;

        for (int y = cy * s; y < cy * s + s && y < h; y++)
          for (int x = cx * s; x < cx * s + s && x < w; x++)
            {
              size_t i = (size_t) y * w + x;

              if (!looked_at (mask, i) || (px[i] >> 24) == 0)
                continue;
              sum[0] += px[i] >> 16 & 0xFF, sum[1] += px[i] >> 8 & 0xFF, sum[2] += px[i] & 0xFF;
              cnt++;
            }
        r->present[c] = cnt > 0;
        r->rgb[3 * c] = (uint8_t) (cnt ? sum[0] / cnt : 0), r->rgb[3 * c + 1] = (uint8_t) (cnt ? sum[1] / cnt : 0);
        r->rgb[3 * c + 2] = (uint8_t) (cnt ? sum[2] / cnt : 0);
        if (cnt > 0)
          to_lab (r, (int) (sum[0] / cnt), (int) (sum[1] / cnt), (int) (sum[2] / cnt), r->lab + 3 * c);
        else
          memset (r->lab + 3 * c, 0, 3 * sizeof *r->lab);
      }
  /* the edge strength, on cells whose four neighbours are there */
  for (int cy = 0; cy < r->ch; cy++)
    for (int cx = 0; cx < r->cw; cx++)
      {
        size_t c = (size_t) cy * r->cw + cx;
        int64_t g = 0;

        if (cx > 0 && cy > 0 && cx < r->cw - 1 && cy < r->ch - 1 && r->present[c] && r->present[c - 1] && r->present[c + 1]
            && r->present[c - r->cw] && r->present[c + r->cw])
          for (int k = 0; k < 3; k++)
            {
              int64_t dx = r->lab[3 * (c + 1) + k] - r->lab[3 * (c - 1) + k];
              int64_t dy = r->lab[3 * (c + r->cw) + k] - r->lab[3 * (c - r->cw) + k];

              g += (dx < 0 ? -dx : dx) + (dy < 0 ? -dy : dy);
            }
        g >>= 6;
        r->edge[c] = (uint16_t) (g >= BUCKETS ? BUCKETS - 1 : g);
      }
  /* (b) The colour line first; the surface is then fitted to the ring's samples on it, so that a subject crossing the ring does not
     pull it. */
  if (!fit_line (r))
    {
      easel_rbg_free (r);
      return NULL;
    }
  r->line_lim = r->line_ok ? r->line_mad * ROBUST_K * ROBUST_K : 0;
  if (V6_CLUSTERS)
    fit_clusters (r);
  /* (a) The surface: fitted, then ROBUST_ROUNDS times refitted without the samples beyond ROBUST_K times the median error (at least
     1 delta-E); the automatic tolerance (REQ-0035-04) from the last median. Fitted once: the Tolerance does not refit it. */
  fit (r, false, 0, r->k0);
  {
    int64_t med = median_residual (r, (const int64_t (*)[ORDER_TERMS]) r->k0);

    for (int round = 0; round < ROBUST_ROUNDS && med >= 0; round++)
      {
        int64_t lim = (med < 64 ? 64 : med) * ROBUST_K;

        fit (r, true, lim * lim, r->k);
        memcpy (r->k0, r->k, sizeof r->k);
        med = median_residual (r, (const int64_t (*)[ORDER_TERMS]) r->k);
      }
    if (med < 0)
      {
        easel_rbg_free (r);
        return NULL;
      }
    r->residual = med;
    r->auto_tol = auto_from (r->residual);
  }
  if (V6_CLUSTERS && CL_BETTER && r->ncl > 0)
    {
      /* v6 (1'): the clusters judge only where they explain the ring better than the surface (their median distance less) */
      int stride = ring_stride (r), count = 0;
      size_t cap = 2 * ((size_t) r->cw + r->ch) * (size_t) r->d / (size_t) stride + 2, n = 0;
      int64_t *d = malloc (cap * sizeof *d);

      if (d != NULL)
        {
          for (int y = 0; y < r->ch; y++)
            for (int x = 0; x < r->cw; x++)
              if (in_ring (r, x, y) && (count++ % stride) == 0 && n < cap)
                {
                  int64_t best = INT64_MAX;

                  for (int k = 0; k < r->ncl; k++)
                    {
                      int64_t e = de2 (r->lab + 3 * ((size_t) y * r->cw + x), r->cl[k]);

                      best = e < best ? e : best;
                    }
                  d[n++] = best;
                }
          qsort (d, n, sizeof *d, cmp64);
          if (n == 0 || (int64_t) isqrt64 ((uint64_t) d[n / 2]) >= r->residual)
            r->ncl = 0;
          free (d);
        }
    }
  /* (b) The line judges only where it explains the ring at least as well as the surface (one color under more or less light); where
     the surface fits better (a gradient between colors, line art on it) a subject of the backdrop's own colors would go with it. */
  if (r->line_ok && r->line_mad > (r->residual < 64 ? 64 : r->residual) * (r->residual < 64 ? 64 : r->residual))
    r->line_ok = false;
  return r;
}

int
easel_rbg_auto_tolerance (const EaselRbg *r)
{
  return r->auto_tol;
}

int
easel_rbg_block (const EaselRbg *r)
{
  return r->s;
}

size_t
easel_rbg_copy_cells (const EaselRbg *r)
{
  return r->n;
}

size_t
easel_rbg_bytes (const EaselRbg *r)
{
  return r->bytes;
}

/* 0037: a cell's kind for the pieces: 1 kept, 0 removed, -1 none. Up to EASEL_RBG_COPY_MAX the cells are the pixels and their kind is
   dst's (the method's result, the fringe included); a pixel not looked at, or fully transparent before, is none. Over that, the copy's
   labels. */
static int
piece_kind (const EaselRbg *r, const uint32_t *px, const uint8_t *mask, const uint32_t *dst, size_t g)
{
  /* the kind kept or removed, and apart by the flood's label before (c): an island the size rule removed is its own piece, not the
     background's, and so is a hole it filled (2 x flood subject + kept) */
  if (r->flood[g] == ABSENT)
    return -1;
  if (r->s == 1)
    return !looked_at (mask, g) || (px[g] >> 24) == 0 ? -1 : 2 * (r->flood[g] == SUBJECT) + ((dst[g] >> 24) != 0);
  return 2 * (r->flood[g] == SUBJECT) + (r->label[g] == SUBJECT);
}

void
easel_rbg_flips (EaselRbg *r, const uint32_t *px, const uint8_t *mask, uint32_t *dst, const EaselRbgPress *presses, size_t n,
                 size_t *removed)
{
  int32_t pieces = 0;
  size_t gone = 0;                         /* the change of the removed count (wraps for a decrease, as size_t does) */

  /* the pieces: 4-connected runs of one kind; r->sizes counts the presses per piece (parity), r->next keeps each piece's kind */
  for (size_t g = 0; g < r->n; g++)
    r->piece[g] = -1;
  for (size_t g0 = 0; g0 < r->n; g0++)
    {
      int kind = piece_kind (r, px, mask, dst, g0);
      size_t qh = 0, qt = 0;

      if (kind < 0 || r->piece[g0] >= 0)
        continue;
      r->piece[g0] = pieces;
      r->queue[qt++] = (uint32_t) g0;
      while (qh < qt)
        {
          size_t g = r->queue[qh++];
          int gx = (int) (g % (size_t) r->cw), gy = (int) (g / (size_t) r->cw);
          size_t nb[4];
          int k = 0;

          if (gx > 0)
            nb[k++] = g - 1;
          if (gx + 1 < r->cw)
            nb[k++] = g + 1;
          if (gy > 0)
            nb[k++] = g - (size_t) r->cw;
          if (gy + 1 < r->ch)
            nb[k++] = g + (size_t) r->cw;
          for (int j = 0; j < k; j++)
            if (r->piece[nb[j]] < 0 && piece_kind (r, px, mask, dst, nb[j]) == kind)
              {
                r->piece[nb[j]] = pieces;
                r->queue[qt++] = (uint32_t) nb[j];
              }
        }
      r->sizes[pieces] = 0;
      r->next[pieces] = kind;
      pieces++;
    }
  r->pieces_made = true;
  for (size_t i = 0; i < n; i++)
    {
      int x = presses[i].x, y = presses[i].y;
      int32_t c;

      if (x < 0 || y < 0 || x >= r->w || y >= r->h)
        continue;
      c = r->piece[(size_t) (y / r->s) * r->cw + x / r->s];
      if (c >= 0)
        r->sizes[c] ^= 1;
    }
  /* each piece with an odd count: its pixels take the other kind (a removed pixel back only if the method removed it); only the
     blocks of such pieces are visited, and the count before is adjusted by what changes */
  for (size_t g = 0; g < r->n; g++)
    {
      int32_t c = r->piece[g];
      int gx = (int) (g % (size_t) r->cw) * r->s, gy = (int) (g / (size_t) r->cw) * r->s;

      if (c < 0 || !(r->sizes[c] & 1))
        continue;
      for (int y = gy; y < gy + r->s && y < r->h; y++)
        for (int x = gx; x < gx + r->s && x < r->w; x++)
          {
            size_t i = (size_t) y * r->w + x;
            bool was = (dst[i] >> 24) == 0;

            if (!looked_at (mask, i) || (px[i] >> 24) == 0)
              continue;
            dst[i] = (r->next[c] & 1) ? px[i] & 0x00FFFFFFu : px[i];
            gone += ((dst[i] >> 24) == 0) - was;
          }
    }
  if (removed != NULL)
    *removed += gone;
}

bool
easel_rbg_same_piece (const EaselRbg *r, int x1, int y1, int x2, int y2)
{
  int32_t a, b;

  if (!r->pieces_made || x1 < 0 || y1 < 0 || x2 < 0 || y2 < 0 || x1 >= r->w || y1 >= r->h || x2 >= r->w || y2 >= r->h)
    return false;
  a = r->piece[(size_t) (y1 / r->s) * r->cw + x1 / r->s];
  b = r->piece[(size_t) (y2 / r->s) * r->cw + x2 / r->s];
  return a >= 0 && a == b;
}

int32_t
easel_rbg_lab_f (const EaselRbg *r, int t)
{
  return f_of (r, t);
}

void
easel_rbg_lab (const EaselRbg *r, int red, int green, int blue, int16_t out[3])
{
  to_lab (r, red & 0xFF, green & 0xFF, blue & 0xFF, out);
}

int64_t
easel_rbg_residual (const EaselRbg *r)
{
  return r->residual;
}

static bool
cancelled (const volatile int *cancel)
{
  return cancel != NULL && *cancel != 0;
}

/* The 4-neighbours of cell i of a cw x ch grid into nb; returns how many. */
static int
neighbours (const EaselRbg *r, size_t i, size_t nb[4])
{
  int x = (int) (i % (size_t) r->cw), y = (int) (i / (size_t) r->cw), k = 0;

  if (x > 0) nb[k++] = i - 1;
  if (x < r->cw - 1) nb[k++] = i + 1;
  if (y > 0) nb[k++] = i - r->cw;
  if (y < r->ch - 1) nb[k++] = i + r->cw;
  return k;
}

static EaselRbgResult
regions (EaselRbg *r, const volatile int *cancel)
{

  /* (c) the largest subject region and those a tenth of it or more; then enclosed background under 1 % of the copy filled. next holds
     each cell's region number, sizes each region's size (bit 31: a background region touching the copy's edge or a cell standing for
     no pixel, so no hole). */
  for (int pass = 0; pass < 2; pass++)
    {
      uint8_t want = pass == 0 ? SUBJECT : BACK, other = pass == 0 ? BACK : SUBJECT;
      uint32_t nregions = 0, largest = 0;

      for (size_t c = 0; c < r->n; c++)
        r->next[c] = -1;
      for (size_t s0 = 0; s0 < r->n; s0++)
        {
          size_t qh = 0, qt = 0;
          bool touches = false;

          if (r->label[s0] != want || r->next[s0] >= 0)
            continue;
          r->next[s0] = (int32_t) nregions;
          r->queue[qt++] = (uint32_t) s0;
          while (qh < qt)
            {
              size_t c = r->queue[qh++], nb[4];
              int x = (int) (c % (size_t) r->cw), y = (int) (c / (size_t) r->cw);
              int k = neighbours (r, c, nb);

              if (x == 0 || y == 0 || x == r->cw - 1 || y == r->ch - 1)
                touches = true;
              for (int j = 0; j < k; j++)
                {
                  if (r->label[nb[j]] == ABSENT)
                    touches = true;
                  if (r->label[nb[j]] == want && r->next[nb[j]] < 0)
                    {
                      r->next[nb[j]] = (int32_t) nregions;
                      r->queue[qt++] = (uint32_t) nb[j];
                    }
                }
            }
          r->sizes[nregions] = (uint32_t) qt | (touches ? (uint32_t) 1 << 31 : 0);
          if (qt > largest)
            largest = (uint32_t) qt;
          nregions++;
        }
      if (cancelled (cancel))
        return EASEL_RBG_CANCELLED;
      for (size_t c = 0; c < r->n; c++)
        if (r->label[c] == want)
          {
            uint32_t size = r->sizes[r->next[c]] & 0x7FFFFFFFu;
            bool edge = (r->sizes[r->next[c]] >> 31) != 0;

            if (pass == 0 ? (uint64_t) size * 10 < largest : (!edge && (uint64_t) size * 100 < r->n))
              r->label[c] = other;
          }
    }
  return EASEL_RBG_DONE;
}

EaselRbgResult
easel_rbg_label (EaselRbg *r, int tolerance, const volatile int *cancel)
{
  int64_t t2;
  int32_t head[BUCKETS], tail[BUCKETS];
  int level = 0;
  size_t popped = 0;

  static uint8_t *seed;                      /* v6 prototype only: the seeds of the last labels (not thread-safe) */
  static size_t seed_n;

  if (seed_n < r->n)
    {
      free (seed);
      seed = malloc (r->n);
      seed_n = seed ? r->n : 0;
      if (seed == NULL)
        return EASEL_RBG_NO_MEMORY;
    }
  if (tolerance < EASEL_RBG_TOLERANCE_MIN)
    tolerance = EASEL_RBG_TOLERANCE_MIN;
  if (tolerance > EASEL_RBG_TOLERANCE_MAX)
    tolerance = EASEL_RBG_TOLERANCE_MAX;
  r->tolerance = tolerance;
  t2 = (int64_t) tolerance * 64 * tolerance * 64;

  /* (b) the seeds, then the flood in order of edge strength, first in first out within a level */
  for (int b = 0; b < BUCKETS; b++)
    head[b] = tail[b] = -1;
#define PUSH(idx, pr) do { int pr_ = (pr) < level ? level : (pr); r->next[idx] = -1;                                      \
    if (tail[pr_] < 0) head[pr_] = (int32_t) (idx); else r->next[tail[pr_]] = (int32_t) (idx); tail[pr_] = (int32_t) (idx); } while (0)
  for (int cy = 0; cy < r->ch; cy++)
    for (int cx = 0; cx < r->cw; cx++)
      {
        size_t c = (size_t) cy * r->cw + cx;
        int64_t d;

        if (!r->present[c])
          {
            r->label[c] = ABSENT;
            continue;
          }
        d = bg_d2 (r, cx, cy);
        r->label[c] = NONE;
        if (in_ring (r, cx, cy) && d <= t2)
          r->label[c] = BACK;
        else if (d >= (int64_t) SEED_FAR * SEED_FAR * t2)
          r->label[c] = SUBJECT;
        if (r->label[c] != NONE)
          PUSH (c, 0);
        seed[c] = r->label[c] != NONE;
      }
  for (level = 0; level < BUCKETS; level++)
    while (head[level] >= 0)
      {
        size_t c = (size_t) head[level], nb[4];
        int k = neighbours (r, c, nb);

        if ((++popped & 0xFFFF) == 0 && cancelled (cancel))
          return EASEL_RBG_CANCELLED;
        head[level] = r->next[c];
        if (head[level] < 0)
          tail[level] = -1;
        for (int j = 0; j < k; j++)
          if (r->label[nb[j]] == NONE)
            {
              r->label[nb[j]] = r->label[c];
              PUSH (nb[j], r->edge[nb[j]]);
            }
      }
#undef PUSH
  for (size_t c = 0; c < r->n; c++)
    if (r->label[c] == NONE)
      r->label[c] = BACK;                       /* no seed reached it */

  /* The seam: a step's two sides have the same edge strength, so the subject's flood takes the first background cell beside it. A
     subject cell beside background whose color is within the tolerance of the surface goes to the background; all such cells are
     found before any is changed, so the order of the scan does not matter. NONE marks them meanwhile. */
  for (int cy = 0; cy < r->ch; cy++)
    for (int cx = 0; cx < r->cw; cx++)
      {
        size_t c = (size_t) cy * r->cw + cx, nb[4];
        int k;
        bool beside = false;

        if (r->label[c] != SUBJECT)
          continue;
        k = neighbours (r, c, nb);
        for (int j = 0; j < k; j++)
          beside = beside || r->label[nb[j]] == BACK;
        if (!beside)
          continue;
        if (bg_d2 (r, cx, cy) <= t2)
          r->label[c] = NONE;
      }
  for (size_t c = 0; c < r->n; c++)
    if (r->label[c] == NONE)
      r->label[c] = BACK;

  memcpy (r->flood, r->label, r->n);         /* 0037: the flood's own regions, before (c) */
  if (regions (r, cancel) != EASEL_RBG_DONE)
    return EASEL_RBG_CANCELLED;
  if (V6_HIST)
    {
      /* v6 (2), (3): colour histograms of 16 bins a channel for the subject and the background from the labels so far; every cell
         that was no seed takes the label that explains its color better; V6_HIST rounds; then the regions rule again */
      uint32_t *hs = calloc (2 * 4096, sizeof *hs), *hb;

      if (hs == NULL)
        return EASEL_RBG_NO_MEMORY;
      hb = hs + 4096;
      for (int round = 0; round < V6_HIST; round++)
        {
          uint64_t ns = 0, nb = 0;

          memset (hs, 0, 2 * 4096 * sizeof *hs);
          for (size_t c = 0; c < r->n; c++)
            {
              unsigned bin = (unsigned) (r->rgb[3 * c] >> 4) << 8 | (unsigned) (r->rgb[3 * c + 1] >> 4) << 4 | (unsigned) (r->rgb[3 * c + 2] >> 4);

              if (r->label[c] == SUBJECT)
                hs[bin]++, ns++;
              else if (r->label[c] == BACK)
                hb[bin]++, nb++;
            }
          for (size_t c = 0; c < r->n; c++)
            {
              unsigned bin = (unsigned) (r->rgb[3 * c] >> 4) << 8 | (unsigned) (r->rgb[3 * c + 1] >> 4) << 4 | (unsigned) (r->rgb[3 * c + 2] >> 4);

              if (r->label[c] == ABSENT || seed[c])
                continue;
              r->label[c] = (uint64_t) (hs[bin] + 1) * (nb + 4096) > (uint64_t) (hb[bin] + 1) * (ns + 4096) ? SUBJECT : BACK;
            }
        }
      free (hs);
      if (regions (r, cancel) != EASEL_RBG_DONE)
        return EASEL_RBG_CANCELLED;
    }
  return EASEL_RBG_DONE;
}

/* The fringe (REQ-0035-03 g): where the background and the subject mix along the boundary. For each of the four directions in which
   a kept pixel has a removed neighbour on one side and a kept one on the other, the subject is the color inward where it has
   settled and the background the color outward where it has settled (each step of a walk more than FRINGE_SETTLE_Q4 / 4 tolerances,
   up to FRINGE_REACH pixels); the run of kept pixels inward from it that are blends of the two with the background's share above
   one half is removed. A blend is judged in the pixels' own RGB, where a soft edge's mix is a straight line (in Lab it bends): the
   subject's share, dot / sl, below one half, and the distance to the line, squared (pp sl - dot^2) / sl, within a FRINGE_FIT_DIV-th
   of the line's length, squared. A line one pixel wide has no kept pixel inward and stays. One round marks, then removes, so the
   order of the scan does not matter; the rounds go on while one removes, at most FRINGE_ROUNDS. */
static bool
removed_at (const EaselRbg *r, const uint8_t *mask, const uint32_t *dst, int x, int y)
{
  size_t i;

  if (x < 0 || y < 0 || x >= r->w || y >= r->h)
    return false;
  i = (size_t) y * r->w + x;
  return looked_at (mask, i) && (dst[i] >> 24) == 0;
}

static bool
kept_at (const EaselRbg *r, const uint8_t *mask, const uint32_t *dst, int x, int y)
{
  size_t i;

  if (x < 0 || y < 0 || x >= r->w || y >= r->h)
    return false;
  i = (size_t) y * r->w + x;
  return looked_at (mask, i) && (dst[i] >> 24) != 0;
}

/* From (x, y) in steps of (dx, dy) over kept pixels (kept) or removed ones: the last before the color settles; its index, or -1. */
static ptrdiff_t
settled (const EaselRbg *r, const uint32_t *px, const uint8_t *mask, const uint32_t *dst, int x, int y, int dx, int dy, bool kept,
         int64_t t2)
{
  ptrdiff_t at = -1;
  int16_t prev[3];

  for (int k = 1; k <= FRINGE_REACH; k++)
    {
      int qx = x + k * dx, qy = y + k * dy;
      uint32_t c;
      int16_t lab[3];

      if (kept ? !kept_at (r, mask, dst, qx, qy) : !removed_at (r, mask, dst, qx, qy))
        break;
      c = px[(size_t) qy * r->w + qx];
      to_lab (r, (int) (c >> 16 & 0xFF), (int) (c >> 8 & 0xFF), (int) (c & 0xFF), lab);
      if (k > 1 && de2 (lab, prev) * 16 <= t2 * FRINGE_SETTLE_Q4 * FRINGE_SETTLE_Q4)
        break;
      at = (ptrdiff_t) qy * r->w + qx;
      memcpy (prev, lab, sizeof prev);
    }
  return at;
}

static size_t
fringe_round (const EaselRbg *r, const uint32_t *px, const uint8_t *mask, uint32_t *dst, const volatile int *cancel, bool *stop)
{
  static const int dirs[4][2] = { { 1, 0 }, { -1, 0 }, { 0, 1 }, { 0, -1 } };
  int w = r->w, h = r->h;
  int64_t t2 = (int64_t) r->tolerance * 64 * r->tolerance * 64;
  size_t marked = 0;

  memset (r->marks, 0, ((size_t) w * (size_t) h + 7) / 8);
  for (int y = 0; y < h; y++)
    {
      if (cancelled (cancel))
        {
          *stop = true;
          return 0;
        }
      const uint32_t *row = dst + (size_t) y * w;

      for (int x = 0; x < w; x++)
        {
          size_t i = (size_t) y * w + x;

          /* the quick test first: a kept pixel with a fully transparent 4-neighbour (most pixels have none) */
          if ((row[x] >> 24) == 0 || !looked_at (mask, i)
              || !((x > 0 && (row[x - 1] >> 24) == 0) || (x + 1 < w && (row[x + 1] >> 24) == 0)
                   || (y > 0 && (row[x - w] >> 24) == 0) || (y + 1 < h && (row[x + w] >> 24) == 0)))
            continue;
          for (int d = 0; d < 4; d++)
          {
            int ix = dirs[d][0], iy = dirs[d][1];  /* inward */
            ptrdiff_t si, bi;
            uint32_t sc, bc;
            int16_t sl_lab[3], bl_lab[3];

            if (!removed_at (r, mask, dst, x - ix, y - iy) || !kept_at (r, mask, dst, x + ix, y + iy))
              continue;
            si = settled (r, px, mask, dst, x, y, ix, iy, true, t2);
            bi = settled (r, px, mask, dst, x, y, -ix, -iy, false, t2);
            if (si < 0 || bi < 0)
              continue;
            sc = px[si], bc = px[bi];
            to_lab (r, (int) (sc >> 16 & 0xFF), (int) (sc >> 8 & 0xFF), (int) (sc & 0xFF), sl_lab);
            to_lab (r, (int) (bc >> 16 & 0xFF), (int) (bc >> 8 & 0xFF), (int) (bc & 0xFF), bl_lab);
            if (de2 (sl_lab, bl_lab) < 4 * t2)
              continue;                  /* the inside is the background's color: nothing to tell apart */
            for (int k = 0; k < FRINGE_REACH; k++)
              {
                int qx = x + k * ix, qy = y + k * iy;
                size_t q = (size_t) qy * w + qx;
                int64_t dot = 0, pp = 0, sl = 0;

                if (!kept_at (r, mask, dst, qx, qy) || (ptrdiff_t) q == si)
                  break;
                for (int c = 0; c < 3; c++)
                  {
                    int64_t bv = bc >> (8 * c) & 0xFF, pb = (int64_t) (px[q] >> (8 * c) & 0xFF) - bv;
                    int64_t sbb = (int64_t) (sc >> (8 * c) & 0xFF) - bv;

                    dot += pb * sbb;
                    pp += pb * pb;
                    sl += sbb * sbb;
                  }
                if (2 * dot >= sl
                    || (dot >= 0 ? (pp * sl - dot * dot) * FRINGE_FIT_DIV * FRINGE_FIT_DIV > sl * sl
                                 : pp * FRINGE_FIT_DIV * FRINGE_FIT_DIV > sl))
                  break;
                if (!(r->marks[q >> 3] >> (q & 7) & 1))
                  {
                    r->marks[q >> 3] |= (uint8_t) (1u << (q & 7));
                    marked++;
                  }
              }
          }
        }
    }
  for (size_t byte = 0; marked > 0 && byte < ((size_t) w * (size_t) h + 7) / 8; byte++)
    if (r->marks[byte] != 0)
      for (size_t i = byte * 8; i < byte * 8 + 8 && i < (size_t) w * (size_t) h; i++)
        if (r->marks[byte] >> (i & 7) & 1)
          dst[i] = px[i] & 0x00FFFFFFu;
  return marked;
}

static EaselRbgResult
fringe (const EaselRbg *r, const uint32_t *px, const uint8_t *mask, uint32_t *dst, size_t *gone, const volatile int *cancel, int rounds)
{
  bool stop = false;

  for (int round = 0; round < rounds; round++)
    {
      size_t n = fringe_round (r, px, mask, dst, cancel, &stop);

      if (stop)
        return EASEL_RBG_CANCELLED;
      *gone += n;
      if (n == 0)
        break;
    }
  return EASEL_RBG_DONE;
}

EaselRbgResult
easel_rbg_preview (const EaselRbg *r, const uint32_t *px, const uint8_t *mask, uint32_t *dst, size_t *removed)
{
  size_t gone = 0;

  for (int y = 0; y < r->h; y++)
    {
      const uint8_t *row = r->label + (size_t) (y / r->s) * r->cw;     /* the block's label, its column counted, not divided */

      for (int x = 0, cx = 0, in = 0; x < r->w; x++)
        {
          size_t i = (size_t) y * r->w + x;

          dst[i] = px[i];
          if (row[cx] == BACK && (px[i] >> 24) != 0 && looked_at (mask, i))
            {
              dst[i] = px[i] & 0x00FFFFFFu;
              gone++;
            }
          if (++in == r->s)
            {
              in = 0;
              cx++;
            }
        }
    }
  fringe (r, px, mask, dst, &gone, NULL, FRINGE_ROUNDS);
  if (removed != NULL)
    *removed = gone;
  return EASEL_RBG_DONE;
}

EaselRbgResult
easel_rbg_result (const EaselRbg *r, const uint32_t *px, const uint8_t *mask, uint32_t *dst, size_t *removed,
                  const volatile int *cancel)
{
  size_t gone = 0;

  for (int y = 0; y < r->h; y++)
    {
      if (cancelled (cancel))
        return EASEL_RBG_CANCELLED;
      for (int x = 0; x < r->w; x++)
        {
          size_t i = (size_t) y * r->w + x;
          int cx = x / r->s, cy = y / r->s;
          uint8_t own = r->label[(size_t) cy * r->cw + cx], lab = own;
          bool same = true;

          dst[i] = px[i];
          if (!looked_at (mask, i) || (px[i] >> 24) == 0)
            continue;
          if (r->s > 1)
            {
              for (int dy = -1; dy <= 1 && same; dy++)
                for (int dx = -1; dx <= 1 && same; dx++)
                  {
                    int nx = cx + dx, ny = cy + dy;
                    uint8_t l;

                    if (nx < 0 || ny < 0 || nx >= r->cw || ny >= r->ch)
                      continue;
                    l = r->label[(size_t) ny * r->cw + nx];
                    if (l != ABSENT && l != own)
                      same = false;
                  }
              if (!same || own == ABSENT)
                {
                  int16_t me[3];
                  int64_t best = INT64_MAX;

                  to_lab (r, (int) (px[i] >> 16 & 0xFF), (int) (px[i] >> 8 & 0xFF), (int) (px[i] & 0xFF), me);
                  for (int dy = -1; dy <= 1; dy++)
                    for (int dx = -1; dx <= 1; dx++)
                      {
                        int nx = cx + dx, ny = cy + dy;
                        size_t c;
                        int64_t dd;

                        if (nx < 0 || ny < 0 || nx >= r->cw || ny >= r->ch)
                          continue;
                        c = (size_t) ny * r->cw + nx;
                        if (r->label[c] == ABSENT)
                          continue;
                        dd = de2 (me, r->lab + 3 * c);
                        if (dd < best || (dd == best && r->label[c] == SUBJECT))
                          {
                            best = dd;
                            lab = r->label[c];
                          }
                      }
                }
            }
          if (lab == BACK)
            {
              dst[i] = px[i] & 0x00FFFFFFu;
              gone++;
            }
        }
    }
  if (fringe (r, px, mask, dst, &gone, cancel, FRINGE_ROUNDS) != EASEL_RBG_DONE)
    return EASEL_RBG_CANCELLED;
  if (removed != NULL)
    *removed = gone;
  return EASEL_RBG_DONE;
}
