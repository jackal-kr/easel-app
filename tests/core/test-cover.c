/* SPDX-License-Identifier: GPL-3.0-or-later */
/* 0039 Smooth shapes and brushes: the coverage of the figures and the mix of REQ-0039-06. */

#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "check.h"
#include "cover.h"
#include "figure.h"
#include "history.h"
#include "image.h"

/* REQ-0039-06 in doubles, written again from the spec's words: the reference of the oracle. */
static uint32_t
mix_reference (uint32_t old, uint32_t c_out, unsigned n_out, uint32_t c_in, unsigned n_in)
{
  double a = old >> 24, rest = 256.0 - n_out - n_in, ao = c_out >> 24, ai = c_in >> 24;
  double m = a * rest + ao * n_out + ai * n_in, alpha = floor (m / 256.0 + 0.5);
  uint32_t r = 0;

  if (alpha == 0)
    return 0;
  for (int sh = 0; sh <= 16; sh += 8)
    {
      double v = ((old >> sh & 0xFF) * a * rest + (c_out >> sh & 0xFF) * ao * n_out + (c_in >> sh & 0xFF) * ai * n_in) / m;
      double c = floor (v + 0.5);

      r |= (uint32_t) (c > 255 ? 255 : c) << sh;
    }
  return (uint32_t) alpha << 24 | r;
}

static uint64_t rng = 88172645463325252u;

static uint32_t
next (void)
{
  rng ^= rng << 13;
  rng ^= rng >> 7;
  rng ^= rng << 17;
  return (uint32_t) rng;
}

/* The mix against the formula over a sweep: random colors (and alphas of 0, 1, 128, 255 often), every count pair on a grid. */
static void
mix_oracle (void)
{
  static const uint32_t alphas[] = { 0, 1, 128, 255 };

  for (int k = 0; k < 4000; k++)
    {
      uint32_t old = next (), co = next (), ci = next ();

      if (k % 2 == 0)
        old = (old & 0xFFFFFF) | alphas[k / 2 % 4] << 24;
      if (k % 3 == 0)
        co = (co & 0xFFFFFF) | alphas[k / 3 % 4] << 24;
      if (old >> 24 == 0)
        old = 0;
      for (unsigned no = 0; no <= 256; no += 16)
        for (unsigned ni = 0; no + ni <= 256; ni += 16 + (k % 5))
          {
            uint32_t got = easel_cover_mix (old, co, no, ci, ni), want = mix_reference (old, co, no, ci, ni);

            if (got != want)
              fprintf (stderr, "old %08X out %08X x%u in %08X x%u: %08X, want %08X\n", old, co, no, ci, ni, got, want);
            CHECK (got == want);
          }
    }
  /* the spec's ends: all points of one part give its color exactly; none give the old */
  CHECK (easel_cover_mix (0xFFFFFFFF, 0x80FF0000, 256, 0, 0) == 0x80FF0000);
  CHECK (easel_cover_mix (0x12345678, 0xFF000000, 0, 0, 0) == 0x12345678);
  CHECK (easel_cover_mix (0xFF000000, 0x00000000, 256, 0, 0) == 0);
}

/* AS-6: the counts of the Line from (2, 2) to (12, 7), width 3, in #FF000080. */
static void
as6_mix_transparent (void)
{
  CHECK (easel_cover_mix (0, 0x80FF0000, 176, 0, 0) == 0x58FF0000);
  CHECK (easel_cover_mix (0, 0x80FF0000, 49, 0, 0) == 0x19FF0000);
  CHECK (easel_cover_mix (0, 0x80FF0000, 256, 0, 0) == 0x80FF0000);
  CHECK (easel_cover_mix (0, 0x80FF0000, 0, 0, 0) == 0);
}

static void
as6_mix_white (void)
{
  CHECK (easel_cover_mix (0xFFFFFFFF, 0x80FF0000, 176, 0, 0) == 0xA8FF7979);
  CHECK (easel_cover_mix (0xFFFFFFFF, 0x80FF0000, 49, 0, 0) == 0xE7FFE4E4);
  CHECK (easel_cover_mix (0xFFFFFFFF, 0x80FF0000, 256, 0, 0) == 0x80FF0000);
  CHECK (easel_cover_mix (0xFFFFFFFF, 0x80FF0000, 0, 0, 0) == 0xFFFFFFFF);
  /* AS-4's two-part pixel: 28 outline points black, 228 inside red, over white */
  CHECK (easel_cover_mix (0xFFFFFFFF, 0xFF000000, 28, 0xFFFF0000, 228) == 0xFFE30000);
}

/* The counts of pixel (x, y) in the figure's cover rows (0, 0 for a pixel in no run); the runs are checked on the way: left to right,
   apart, no empty run, out + in at most 256. */
static void
counts_at (const EaselFigure *f, int x, int y, int *out, int *in)
{
  size_t m = easel_figure_cover_max_runs (f), n;
  EaselCoverRun *runs = malloc (m * sizeof *runs);

  CHECK (runs != NULL);
  n = easel_figure_cover_row (f, y, runs, m);
  *out = *in = 0;
  for (size_t k = 0; k < n; k++)
    {
      CHECK (runs[k].x0 <= runs[k].x1 && runs[k].out + runs[k].in > 0 && runs[k].out + runs[k].in <= 256);
      CHECK (k == 0 || runs[k].x0 > runs[k - 1].x1);
      if (x >= runs[k].x0 && x <= runs[k].x1)
        *out = runs[k].out, *in = runs[k].in;
    }
  free (runs);
}

/* The counts of the pixels x0 to x0 + nx - 1 of row y, from one call of the cover row. */
static void
row_counts (const EaselFigure *f, int y, int x0, int nx, int *out, int *in)
{
  size_t m = easel_figure_cover_max_runs (f), n;
  EaselCoverRun *runs = malloc (m * sizeof *runs);

  CHECK (runs != NULL);
  n = easel_figure_cover_row (f, y, runs, m);
  for (int x = 0; x < nx; x++)
    out[x] = in[x] = 0;
  for (size_t k = 0; k < n; k++)
    {
      CHECK (runs[k].x0 <= runs[k].x1 && runs[k].out + runs[k].in > 0 && runs[k].out + runs[k].in <= 256);
      CHECK (k == 0 || runs[k].x0 > runs[k - 1].x1);
      for (int x = runs[k].x0; x <= runs[k].x1; x++)
        if (x >= x0 && x < x0 + nx)
          out[x - x0] = runs[k].out, in[x - x0] = runs[k].in;
    }
  free (runs);
}

static EaselFigure
smooth_box (EaselFigureKind kind, int x0, int y0, int x1, int y1, int width)
{
  EaselFigure f = { kind, x0, y0, x1, y1, width, EASEL_FIGURE_OUTLINE_AND_FILL };

  f.smooth = true;
  return f;
}

/* The brute force of REQ-0039-05 (a), written again: is the point (P, Q), in 1/32 pixel, in the shape of this kind in the box
   (x, y, w, h) with corner radius r, all in pixels? Sample points are never on a box edge (odd against multiples of 32). */
static int
in_box_shape (EaselFigureKind kind, long long P, long long Q, long long x, long long y, long long w, long long h, long long r)
{
  if (w <= 0 || h <= 0 || P < 32 * x || P > 32 * (x + w) || Q < 32 * y || Q > 32 * (y + h))
    return 0;
  if (kind == EASEL_FIGURE_ELLIPSE)
    {
      __int128 dx = 2 * P - 32 * (2 * x + w), dy = 2 * Q - 32 * (2 * y + h);

      return dx * dx * h * h + dy * dy * w * w <= (__int128) 1024 * w * w * h * h;
    }
  if (kind == EASEL_FIGURE_ROUNDED)
    {
      long long cx = P < 32 * (x + r) ? 32 * (x + r) : P > 32 * (x + w - r) ? 32 * (x + w - r) : P;
      long long cy = Q < 32 * (y + r) ? 32 * (y + r) : Q > 32 * (y + h - r) ? 32 * (y + h - r) : Q;

      return (P - cx) * (P - cx) + (Q - cy) * (Q - cy) <= 1024 * r * r;
    }
  return 1;
}

static void
box_reference (EaselFigureKind kind, int x0, int y0, int x1, int y1, int wd, int px, int py, int *out, int *in)
{
  long long x = x0 < x1 ? x0 : x1, y = y0 < y1 ? y0 : y1, w = abs (x1 - x0) + 1, h = abs (y1 - y0) + 1;
  long long r = (w < h ? w : h) / 2;

  if (r > 8)
    r = 8;
  if (kind != EASEL_FIGURE_ROUNDED)
    r = 0;
  *out = *in = 0;
  for (int j = 0; j < 16; j++)
    for (int i = 0; i < 16; i++)
      {
        long long P = 32LL * px + 2 * i + 1, Q = 32LL * py + 2 * j + 1;

        if (!in_box_shape (kind, P, Q, x, y, w, h, r))
          continue;
        if (in_box_shape (kind, P, Q, x + wd, y + wd, w - 2 * wd, h - 2 * wd, r > wd ? r - wd : 0))
          ++*in;
        else
          ++*out;
      }
}

/* AS-4: the ellipse in the box (2, 2) to (17, 11), width 2. */
static void
as4_ellipse_points (void)
{
  EaselFigure f = smooth_box (EASEL_FIGURE_ELLIPSE, 2, 2, 17, 11, 2);
  int o, i;

  counts_at (&f, 3, 4, &o, &i);
  CHECK (o == 211 && i == 0);
  counts_at (&f, 4, 6, &o, &i);
  CHECK (o == 28 && i == 228);
  counts_at (&f, 9, 6, &o, &i);
  CHECK (o == 0 && i == 256);
  counts_at (&f, 1, 6, &o, &i);
  CHECK (o == 0 && i == 0);
}

/* AS-4: a Rectangle with Smooth is Sharp's: every pixel of Sharp's runs whole, in its part, and no other. */
static void
as4_rectangle_is_sharp (void)
{
  for (int k = 0; k < 300; k++)
    {
      int x0 = (int) (next () % 30) - 5, y0 = (int) (next () % 30) - 5, x1 = (int) (next () % 30) - 5, y1 = (int) (next () % 30) - 5;
      int w = 1 + (int) (next () % 5);
      EaselFigure f = smooth_box (EASEL_FIGURE_RECTANGLE, x0, y0, x1, y1, w);

      for (int y = -8; y < 28; y++)
        {
          EaselFigureRun sharp[4];
          size_t n = easel_figure_row (&f, y, sharp, 4);

          for (int x = -8; x < 28; x++)
            {
              int o, i, so = 0, si = 0;

              for (size_t q = 0; q < n; q++)
                if (x >= sharp[q].x0 && x <= sharp[q].x1)
                  *(sharp[q].inside ? &si : &so) = 256;
              counts_at (&f, x, y, &o, &i);
              CHECK (o == so && i == si);
            }
        }
    }
}

/* The oracle: random boxes and widths of the three kinds, every pixel against the brute force. */
static void
cover_oracle_boxes (void)
{
  static const EaselFigureKind kinds[3] = { EASEL_FIGURE_RECTANGLE, EASEL_FIGURE_ROUNDED, EASEL_FIGURE_ELLIPSE };

  for (int k = 0; k < 600; k++)
    {
      EaselFigureKind kind = kinds[k % 3];
      int x0 = (int) (next () % 40) - 4, y0 = (int) (next () % 40) - 4, x1 = (int) (next () % 40) - 4, y1 = (int) (next () % 40) - 4;
      int w = 1 + (int) (next () % 5);
      EaselFigure f = smooth_box (kind, x0, y0, x1, y1, w);

      for (int y = -6; y < 38; y++)
        for (int x = -6; x < 38; x++)
          {
            int o, i, ro, ri;

            counts_at (&f, x, y, &o, &i);
            box_reference (kind, x0, y0, x1, y1, w, x, y, &ro, &ri);
            if (o != ro || i != ri)
              fprintf (stderr, "kind %d box (%d, %d) to (%d, %d) width %d, pixel (%d, %d): %d/%d, want %d/%d\n", kind, x0, y0, x1, y1,
                       w, x, y, o, i, ro, ri);
            CHECK (o == ro && i == ri);
          }
    }
}

static EaselFigure
smooth_line (int x0, int y0, int x1, int y1, int width)
{
  EaselFigure f = { EASEL_FIGURE_LINE, x0, y0, x1, y1, width, EASEL_FIGURE_OUTLINE };

  f.smooth = true;
  return f;
}

/* REQ-0039-05 (b), written again: the points of pixel (px, py) in the rectangle of width w along the centers of the end pixels,
   reaching w / 2 beyond each, moved by 1/2 for an even w; start = end is the w x w square. In 1/32 pixel, squared, in 128 bits. */
static int
line_reference (int x0, int y0, int x1, int y1, int w, int px, int py)
{
  long long sh = w % 2 == 0 ? 16 : 0, ax = 32LL * x0 + 16 + sh, ay = 32LL * y0 + 16 + sh, dx = 32LL * (x1 - x0), dy = 32LL * (y1 - y0);
  __int128 l2 = (__int128) dx * dx + (__int128) dy * dy, hh = (__int128) (16 * w) * (16 * w);
  int n = 0;

  for (int j = 0; j < 16; j++)
    for (int i = 0; i < 16; i++)
      {
        long long vx = 32LL * px + 2 * i + 1 - ax, vy = 32LL * py + 2 * j + 1 - ay;
        __int128 t = (__int128) vx * dx + (__int128) vy * dy, c = (__int128) vx * dy - (__int128) vy * dx;

        if (l2 == 0)
          n += vx * vx <= hh && vy * vy <= hh;
        else
          n += c * c <= hh * l2 && (t >= 0 || t * t <= hh * l2) && (t <= l2 || (t - l2) * (t - l2) <= hh * l2);
      }
  return n;
}

/* AS-3: the Line from (2, 2) to (12, 7), width 1, and a level one. */
static void
as3_line_points (void)
{
  EaselFigure f = smooth_line (2, 2, 12, 7, 1);
  static const int want[4][3] = { { 2, 2, 229 }, { 3, 2, 144 }, { 4, 3, 238 }, { 13, 7, 10 } };
  static const uint32_t colors[4] = { 0xFF1B1B1B, 0xFF707070, 0xFF121212, 0xFFF5F5F5 };
  int o, i;

  for (int k = 0; k < 4; k++)
    {
      counts_at (&f, want[k][0], want[k][1], &o, &i);
      CHECK (o == want[k][2] && i == 0);
      CHECK (easel_cover_mix (0xFFFFFFFF, 0xFF000000, (unsigned) o, 0, 0) == colors[k]);
    }
}

static void
as3_level_line_is_sharp (void)
{
  for (int w = 1; w <= 5; w++)
    {
      EaselFigure f = smooth_line (2, 15, 17, 15, w), up = smooth_line (6, 3, 6, 18, w), dot = smooth_line (9, 9, 9, 9, w);
      EaselFigure *each[3] = { &f, &up, &dot };

      for (int k = 0; k < 3; k++)
        {
          EaselFigure sharp = *each[k];

          sharp.smooth = false;
          CHECK (easel_figure_prepare (&sharp, -10, 40));
          for (int y = -5; y < 30; y++)
            {
              EaselFigureRun runs[4];
              size_t n = easel_figure_row (&sharp, y, runs, 4);

              for (int x = -5; x < 30; x++)
                {
                  int o, in, so = 0;

                  for (size_t q = 0; q < n; q++)
                    if (x >= runs[q].x0 && x <= runs[q].x1)
                      so = 256;
                  counts_at (each[k], x, y, &o, &in);
                  CHECK (o == so && in == 0);
                }
            }
          easel_figure_release (&sharp);
        }
    }
}

/* AS-6's counts: width 3 from (2, 2) to (12, 7). */
static void
as6_line_counts (void)
{
  EaselFigure f = smooth_line (2, 2, 12, 7, 3);
  static const int want[4][3] = { { 5, 2, 176 }, { 4, 5, 49 }, { 2, 2, 256 }, { 3, 5, 0 } };
  int o, i;

  for (int k = 0; k < 4; k++)
    {
      counts_at (&f, want[k][0], want[k][1], &o, &i);
      CHECK (o == want[k][2] && i == 0);
    }
}

static void
cover_oracle_lines (void)
{
  for (int k = 0; k < 1500; k++)
    {
      int x0 = (int) (next () % 30), y0 = (int) (next () % 30), x1 = (int) (next () % 30), y1 = (int) (next () % 30);
      int w = 1 + (int) (next () % 5);
      EaselFigure f;

      if (k % 10 == 0)
        x1 = x0, y1 = y0;
      f = smooth_line (x0, y0, x1, y1, w);
      for (int y = -4; y < 34; y++)
        {
          int ro[38], ri[38];

          row_counts (&f, y, -4, 38, ro, ri);
          for (int x = -4; x < 34; x++)
            {
              int o = ro[x + 4], i = ri[x + 4], r = line_reference (x0, y0, x1, y1, w, x, y);

              if (o != r || i != 0)
                fprintf (stderr, "line (%d, %d) to (%d, %d) width %d, pixel (%d, %d): %d, want %d\n", x0, y0, x1, y1, w, x, y, o, r);
              CHECK (o == r && i == 0);
            }
        }
    }
  /* far from the origin, near the coordinate limit: the same counts as at the origin, moved */
  for (int k = 0; k < 50; k++)
    {
      int dx = (int) (next () % 60) - 30, dy = (int) (next () % 60) - 30, w = 1 + (int) (next () % 5), o, i, o2, i2;
      EaselFigure near = smooth_line (0, 0, dx, dy, w), far = smooth_line (999000, -999000, 999000 + dx, -999000 + dy, w);

      for (int y = -34; y < 34; y++)
        for (int x = -34; x < 34; x++)
          {
            counts_at (&near, x, y, &o, &i);
            counts_at (&far, 999000 + x, -999000 + y, &o2, &i2);
            CHECK (o == o2 && i == i2);
          }
    }
}

/* ---- Curve and Polygon (REQ-0039-05 (c), (d)): the reference, written again ---- */

typedef struct
{
  long long x, y;
} Pt;

static int
ref_band (long long P, long long Q, Pt a, Pt b, long long h, int ea, int eb)
{
  __int128 dx = b.x - a.x, dy = b.y - a.y, vx = P - a.x, vy = Q - a.y, l2 = dx * dx + dy * dy, hh = (__int128) h * h, t, c;

  if (l2 == 0)
    return ea && eb && vx * vx <= hh && vy * vy <= hh;
  t = vx * dx + vy * dy;
  c = vx * dy - vy * dx;
  if (c * c > hh * l2)
    return 0;
  if (t < 0 && !(ea && t * t <= hh * l2))
    return 0;
  if (t > l2 && !(eb && (t - l2) * (t - l2) <= hh * l2))
    return 0;
  return 1;
}

/* The path of n points (1/32 pixel) as a list of pieces: its sides as bands, round joins, square ends at the two ends of an open
   one; a lone point is the square. */
typedef struct
{
  Pt a, b;
  int disk, ea, eb;
} RefPiece;

static int
ref_pieces (const Pt *v, int n, int closed, RefPiece *out)
{
  int sides = closed ? n : n - 1, m = 0;

  if (n == 1)
    {
      out[m++] = (RefPiece) { v[0], v[0], 0, 1, 1 };
      return m;
    }
  for (int k = 0; k < sides; k++)
    out[m++] = (RefPiece) { v[k], v[(k + 1) % n], 0, !closed && k == 0, !closed && k == sides - 1 };
  for (int k = closed ? 0 : 1; k < (closed ? n : n - 1); k++)
    out[m++] = (RefPiece) { v[k], v[k], 1, 0, 0 };
  return m;
}

/* The points of pixel (px, py) in the pieces' union: only the pieces whose box, widened by 2 h, meets the pixel are tried. */
static int
ref_path_count (const RefPiece *pc, int m, long long h, int px, int py, unsigned char *hit)
{
  static RefPiece near[40000];
  int nn = 0, count = 0;
  long long x0 = 32LL * px, x1 = x0 + 32, y0 = 32LL * py, y1 = y0 + 32;

  for (int k = 0; k < m; k++)
    {
      long long lx = pc[k].a.x < pc[k].b.x ? pc[k].a.x : pc[k].b.x, hx = pc[k].a.x < pc[k].b.x ? pc[k].b.x : pc[k].a.x;
      long long ly = pc[k].a.y < pc[k].b.y ? pc[k].a.y : pc[k].b.y, hy = pc[k].a.y < pc[k].b.y ? pc[k].b.y : pc[k].a.y;

      if (lx - 2 * h <= x1 && hx + 2 * h >= x0 && ly - 2 * h <= y1 && hy + 2 * h >= y0)
        near[nn++] = pc[k];
    }
  for (int j = 0; j < 16; j++)
    for (int i = 0; i < 16; i++)
      {
        long long P = x0 + 2 * i + 1, Q = y0 + 2 * j + 1;
        int in = 0;

        for (int k = 0; k < nn && !in; k++)
          in = near[k].disk ? (P - near[k].a.x) * (P - near[k].a.x) + (Q - near[k].a.y) * (Q - near[k].a.y) <= h * h
                            : ref_band (P, Q, near[k].a, near[k].b, h, near[k].ea, near[k].eb);
        hit[16 * j + i] = (unsigned char) in;
        count += in;
      }
  return count;
}

/* The alternate rule: sides crossing the row Q (never through a corner: Q is odd, corners even) to the point's larger x. */
static int
ref_region (long long P, long long Q, const Pt *v, int n)
{
  int c = 0;

  for (int k = 0; k < n; k++)
    {
      Pt a = v[k], b = v[(k + 1) % n];

      if ((a.y < Q) != (b.y < Q))
        {
          /* x at the crossing > P, with the denominator b.y - a.y made positive */
          __int128 num = (__int128) a.x * (b.y - a.y) + (__int128) (Q - a.y) * (b.x - a.x), den = b.y - a.y;

          if (den < 0)
            num = -num, den = -den;
          c += num > (__int128) P * den;
        }
    }
  return c % 2;
}

/* The curve's points of REQ-0011-05 at pixel centers, taken to the nearest 1/32 (a half up), repeats dropped; n from the
   control polygon's length, rounded up, as the Sharp curve. */
static int
ref_curve_points (const int p[8], int w, Pt *out)
{
  long long sh = w % 2 == 0 ? 16 : 0, n;
  double len = 0;
  int m = 0;

  for (int k = 0; k < 3; k++)
    len += sqrt ((double) (p[2 * k + 2] - p[2 * k]) * (p[2 * k + 2] - p[2 * k]) + (double) (p[2 * k + 3] - p[2 * k + 1]) * (p[2 * k + 3] - p[2 * k + 1]));
  n = (long long) ceil (len);
  if (n < 1)
    n = 1;
  for (long long i = 0; i <= n; i++)
    {
      __int128 d = (__int128) n * n * n, u = n - i, t = i;
      long long c[2];

      for (int k = 0; k < 2; k++)
        {
          __int128 num = u * u * u * p[k] + 3 * u * u * t * p[2 + k] + 3 * u * t * t * p[4 + k] + t * t * t * p[6 + k];
          __int128 v = 64 * num + d, q = v >= 0 ? v / (2 * d) : -((-v + 2 * d - 1) / (2 * d));

          c[k] = (long long) q + 16 + sh;
        }
      if (m > 0 && out[m - 1].x == c[0] && out[m - 1].y == c[1])
        continue;
      out[m++] = (Pt) { c[0], c[1] };
    }
  return m;
}

static int
on_segment (const int p[8], int qx, int qy)
{
  long long cross = (long long) (p[6] - p[0]) * (qy - p[1]) - (long long) (p[7] - p[1]) * (qx - p[0]);

  return cross == 0 && qx >= (p[0] < p[6] ? p[0] : p[6]) && qx <= (p[0] < p[6] ? p[6] : p[0])
         && qy >= (p[1] < p[7] ? p[1] : p[7]) && qy <= (p[1] < p[7] ? p[7] : p[1]);
}

static void
curve_reference (const int p[8], int w, int px, int py, int *out)
{
  static Pt v[20000];
  static RefPiece pc[40000];
  unsigned char hit[256];
  int n;

  if (on_segment (p, p[2], p[3]) && on_segment (p, p[4], p[5]))
    {
      *out = line_reference (p[0], p[1], p[6], p[7], w, px, py);
      return;
    }
  n = ref_curve_points (p, w, v);
  *out = ref_path_count (pc, ref_pieces (v, n, 0, pc), 16LL * w, px, py, hit);
}

static void
polygon_reference (const int *c, int n, int closed, int w, int px, int py, int *out, int *in)
{
  Pt v[64], r[64];
  int m = 0, distinct = 0;
  long long sh = w % 2 == 0 ? 16 : 0;

  for (int k = 0; k < n; k++)
    {
      Pt q = { 32LL * c[2 * k] + 16 + sh, 32LL * c[2 * k + 1] + 16 + sh };

      if (m > 0 && v[m - 1].x == q.x && v[m - 1].y == q.y)
        continue;
      r[m] = (Pt) { 32LL * c[2 * k] + 16, 32LL * c[2 * k + 1] + 16 };
      v[m++] = q;
    }
  if (closed && m > 1 && v[m - 1].x == v[0].x && v[m - 1].y == v[0].y)
    m--;
  for (int a = 0; a < m; a++)
    {
      int seen = 0;

      for (int b = 0; b < a; b++)
        seen |= r[a].x == r[b].x && r[a].y == r[b].y;
      distinct += !seen;
    }
  {
    RefPiece pc[130];
    unsigned char hit[256];

    *out = ref_path_count (pc, ref_pieces (v, m, closed, pc), 16LL * w, px, py, hit);
    *in = 0;
    if (closed && distinct >= 3)
      for (int j = 0; j < 16; j++)
        for (int i = 0; i < 16; i++)
          *in += !hit[16 * j + i] && ref_region (32LL * px + 2 * i + 1, 32LL * py + 2 * j + 1, r, m);
  }
}

static EaselFigure
smooth_points (EaselFigureKind kind, const int *pts, size_t n, bool closed, int width)
{
  EaselFigure f = { kind, 0, 0, 0, 0, width, EASEL_FIGURE_OUTLINE_AND_FILL, NULL, pts, n, closed };

  f.smooth = true;
  return f;
}

/* AS-9: the bow tie (2, 2), (14, 12), (14, 2), (2, 12), width 1, closed: the two side triangles filled, the middle above and
   below the crossing empty, the crossing and the corners covered once. Numbers from the prototype (docs/reference/proto-0039/curve.py). */
static void
as9_bow_tie (void)
{
  static const int c[8] = { 2, 2, 14, 12, 14, 2, 2, 12 };
  static const int want[10][4] = { { 4, 7, 0, 256 }, { 12, 7, 0, 256 }, { 8, 4, 0, 0 }, { 8, 9, 0, 0 }, { 8, 7, 256, 0 },
                                   { 2, 2, 233, 0 }, { 14, 12, 233, 0 }, { 5, 5, 165, 91 }, { 3, 7, 0, 256 }, { 8, 6, 92, 0 } };
  EaselFigure f = smooth_points (EASEL_FIGURE_POLYGON, c, 4, true, 1);
  int o, i;

  CHECK (easel_figure_prepare (&f, 0, 20));
  for (int k = 0; k < 10; k++)
    {
      counts_at (&f, want[k][0], want[k][1], &o, &i);
      if (o != want[k][2] || i != want[k][3])
        fprintf (stderr, "pixel (%d, %d): %d/%d\n", want[k][0], want[k][1], o, i);
      CHECK (o == want[k][2] && i == want[k][3]);
    }
  easel_figure_release (&f);
}

/* AS-9: a Rounded Rectangle (2, 2) to (21, 13), width 2 (radius 6), and a Curve with one bend: start (2, 10), both control
   points (10, 2), end (18, 10), width 2. Numbers from the prototype (docs/reference/proto-0039/rounded.py and curve.py). */
static void
as9_rounded_and_curve (void)
{
  static const int rr[10][4] = { { 2, 2, 0, 0 }, { 3, 3, 28, 0 }, { 4, 3, 213, 0 }, { 5, 2, 114, 0 }, { 2, 5, 114, 0 },
                                 { 3, 5, 256, 0 }, { 4, 4, 256, 0 }, { 8, 2, 256, 0 }, { 8, 4, 0, 256 }, { 10, 8, 0, 256 } };
  static const int cv[10][3] = { { 2, 10, 211 }, { 3, 9, 211 }, { 5, 7, 204 }, { 6, 6, 188 }, { 9, 5, 256 }, { 10, 5, 256 },
                                 { 10, 4, 246 }, { 14, 7, 230 }, { 18, 10, 256 }, { 19, 11, 211 } };
  static const int p[8] = { 2, 10, 10, 2, 10, 2, 18, 10 };
  EaselFigure r = smooth_box (EASEL_FIGURE_ROUNDED, 2, 2, 21, 13, 2), f = smooth_points (EASEL_FIGURE_CURVE, p, 4, false, 2);
  int o, i;

  for (int k = 0; k < 10; k++)
    {
      counts_at (&r, rr[k][0], rr[k][1], &o, &i);
      CHECK (o == rr[k][2] && i == rr[k][3]);
    }
  CHECK (easel_figure_prepare (&f, 0, 20));
  for (int k = 0; k < 10; k++)
    {
      counts_at (&f, cv[k][0], cv[k][1], &o, &i);
      if (o != cv[k][2] || i != 0)
        fprintf (stderr, "curve pixel (%d, %d): %d/%d\n", cv[k][0], cv[k][1], o, i);
      CHECK (o == cv[k][2] && i == 0);
    }
  easel_figure_release (&f);
}

static void
cover_oracle_paths (void)
{
  for (int k = 0; k < 400; k++)
    {
      int p[8], w = 1 + (int) (next () % 5), o, i, r;
      EaselFigure f;

      for (int q = 0; q < 8; q++)
        p[q] = (int) (next () % 26);
      if (k % 8 == 0)
        p[2] = p[0], p[3] = p[1], p[4] = p[6], p[5] = p[7];       /* before any bend: the Line */
      f = smooth_points (EASEL_FIGURE_CURVE, p, 4, false, w);
      CHECK (easel_figure_prepare (&f, -10, 40));
      for (int y = -4; y < 30; y++)
        {
          int ro[34], ri[34];

          row_counts (&f, y, -4, 34, ro, ri);
          for (int x = -4; x < 30; x++)
            {
              o = ro[x + 4], i = ri[x + 4];
              curve_reference (p, w, x, y, &r);
              if (o != r || i != 0)
                fprintf (stderr, "curve %d %d %d %d %d %d %d %d width %d, pixel (%d, %d): %d, want %d\n", p[0], p[1], p[2], p[3], p[4],
                         p[5], p[6], p[7], w, x, y, o, r);
              CHECK (o == r && i == 0);
            }
        }
      easel_figure_release (&f);
    }
  for (int k = 0; k < 600; k++)
    {
      int c[16], n = 1 + (int) (next () % 7), w = 1 + (int) (next () % 5), closed = (int) (next () % 3) != 0, o, i, ro, ri;
      EaselFigure f;

      for (int q = 0; q < 2 * n; q++)
        c[q] = (int) (next () % 24);
      if (k % 9 == 0 && n > 1)
        c[2] = c[0], c[3] = c[1];                                  /* a corner twice */
      f = smooth_points (EASEL_FIGURE_POLYGON, c, (size_t) n, closed, w);
      CHECK (easel_figure_prepare (&f, -10, 40));
      for (int y = -4; y < 28; y++)
        {
          int go[32], gi[32];

          row_counts (&f, y, -4, 32, go, gi);
          for (int x = -4; x < 28; x++)
            {
              o = go[x + 4], i = gi[x + 4];
              polygon_reference (c, n, closed, w, x, y, &ro, &ri);
              if (o != ro || i != ri)
                fprintf (stderr, "polygon of %d (closed %d) width %d, pixel (%d, %d): %d/%d, want %d/%d\n", n, closed, w, x, y, o, i, ro, ri);
              CHECK (o == ro && i == ri);
            }
        }
      easel_figure_release (&f);
    }
}

/* REQ-0039-04, REQ-0039-08: the put-down mixes each covered pixel by the style's parts, in one undo step, inside the picture only. */
static void
smooth_apply_styles (void)
{
  static const int poly[8] = { -3, 4, 20, 2, 26, 27, 3, 20 };
  EaselFigure figs[4] = { smooth_box (EASEL_FIGURE_ELLIPSE, -4, 3, 17, 26, 3), smooth_line (-2, 5, 27, 19, 2),
                          smooth_box (EASEL_FIGURE_ROUNDED, 4, -3, 27, 15, 4),
                          smooth_points (EASEL_FIGURE_POLYGON, poly, 4, true, 2) };
  static const EaselFigureStyle styles[3] = { EASEL_FIGURE_OUTLINE, EASEL_FIGURE_OUTLINE_AND_FILL, EASEL_FIGURE_FILL_ONLY };
  const uint32_t draw = 0xC0102030, other = 0xFFF0E0D0;

  for (int k = 0; k < 4; k++)
    for (int st = 0; st < 3; st++)
      {
        EaselImage *img = easel_image_new (24, 24, 0), *before = easel_image_new (24, 24, 0);
        EaselHistory *h = easel_history_new (1 << 20);
        EaselFigure f = figs[k];
        size_t changed = 0, want_changed = 0;

        CHECK (img != NULL && before != NULL && h != NULL);
        for (int i = 0; i < 24 * 24; i++)
          img->px[i] = before->px[i] = i % 7 == 0 ? 0 : 0xFF000000u | (uint32_t) (i * 2654435761u >> 8);
        f.style = styles[st];
        CHECK (easel_figure_apply (img, h, &f, draw, other, &changed) == EASEL_FIGURE_DONE);
        if (f.kind == EASEL_FIGURE_POLYGON)
          CHECK (easel_figure_prepare (&f, -10, 40));
        for (int y = 0; y < 24; y++)
          for (int x = 0; x < 24; x++)
            {
              int o, i;
              uint32_t old = before->px[y * 24 + x], want;

              counts_at (&f, x, y, &o, &i);
              want = f.style == EASEL_FIGURE_OUTLINE ? easel_cover_mix (old, draw, (unsigned) o, 0, 0)
                     : f.style == EASEL_FIGURE_FILL_ONLY ? easel_cover_mix (old, draw, (unsigned) (o + i), 0, 0)
                     : easel_cover_mix (old, draw, (unsigned) o, other, (unsigned) i);
              if (o + i == 0)
                want = old;
              want_changed += want != old;
              if (img->px[y * 24 + x] != want)
                fprintf (stderr, "figure %d style %d pixel (%d, %d): %08X, want %08X\n", k, st, x, y, img->px[y * 24 + x], want);
              CHECK (img->px[y * 24 + x] == want);
            }
        if (f.kind == EASEL_FIGURE_POLYGON)
          easel_figure_release (&f);
        CHECK (changed == want_changed && changed > 0);
        CHECK (easel_history_undo (h, img) && !easel_history_can_undo (h));
        CHECK (memcmp (img->px, before->px, sizeof (uint32_t) * 24 * 24) == 0);
        easel_history_free (h);
        easel_image_free (img);
        easel_image_free (before);
      }
}

/* REQ-0039-08: without the memory for the step's log, nothing changes and no step is added. */
static void
smooth_apply_no_memory (void)
{
  EaselImage *img = easel_image_new (40, 40, 0xFFFFFFFFu);
  EaselHistory *h = easel_history_new (1 << 20);
  EaselFigure f = smooth_box (EASEL_FIGURE_ELLIPSE, 2, 2, 37, 30, 2);

  CHECK (img != NULL && h != NULL);
  CHECK (easel_figure_apply_capped (img, h, &f, 0xFF000000u, 0xFFFF0000u, NULL, 12 * 100) == EASEL_FIGURE_NO_MEMORY);
  for (int i = 0; i < 40 * 40; i++)
    CHECK (img->px[i] == 0xFFFFFFFFu);
  CHECK (!easel_history_can_undo (h));
  easel_history_free (h);
  easel_image_free (img);
}

/* 0039 Check: a polygon with no corner yet (a pending one is shown at once), open or closed, prepares and covers nothing. */
static void
smooth_polygon_no_corner (void)
{
  static const int none[2] = { 0, 0 };

  for (int closed = 0; closed <= 1; closed++)
    {
      EaselFigure f = smooth_points (EASEL_FIGURE_POLYGON, none, 0, closed, 3);
      EaselCoverRun *runs;

      CHECK (easel_figure_prepare (&f, 0, 20));
      runs = malloc (easel_figure_cover_max_runs (&f) * sizeof *runs);
      CHECK (runs != NULL);
      for (int y = -2; y < 4; y++)
        CHECK (easel_figure_cover_row (&f, y, runs, easel_figure_cover_max_runs (&f)) == 0);
      free (runs);
      easel_figure_release (&f);
    }
}

/* 0040 REQ-0040-04: the mix in 255ths against the formula in doubles, written again from the spec: a sweep of colors (alphas 0, 1, 128
   and 255 forced often) and every coverage from 0 to 255; c = 255 the color exactly, c = 0 the old color, an alpha of 0 the zero. */
static uint32_t
mix255_reference (uint32_t old, uint32_t color, unsigned c)
{
  double a = old >> 24, at = color >> 24, m = a * (255.0 - c) + at * c, alpha = floor (m / 255.0 + 0.5);
  uint32_t r = 0;

  if (alpha == 0)
    return 0;
  for (int sh = 0; sh <= 16; sh += 8)
    {
      double v = floor (((old >> sh & 0xFF) * a * (255.0 - c) + (color >> sh & 0xFF) * at * c) / m + 0.5);

      r |= (uint32_t) (v > 255 ? 255 : v) << sh;
    }
  return (uint32_t) alpha << 24 | r;
}

static void
mix255_oracle (void)
{
  static const uint32_t alphas[] = { 0, 1, 128, 255 };

  for (int k = 0; k < 3000; k++)
    {
      uint32_t old = next (), color = next ();

      if (k % 2 == 0)
        old = (old & 0xFFFFFF) | alphas[k / 2 % 4] << 24;
      if (k % 3 == 0)
        color = (color & 0xFFFFFF) | alphas[k / 3 % 4] << 24;
      if (old >> 24 == 0)
        old = 0;
      for (unsigned c = 0; c <= 255; c++)
        {
          uint32_t got = easel_cover_mix255 (old, color, c), want = mix255_reference (old, color, c);

          if (got != want)
            fprintf (stderr, "old %08X color %08X c %u: %08X, want %08X\n", old, color, c, got, want);
          CHECK (got == want);
        }
    }
  CHECK (easel_cover_mix255 (0xFFFFFFFF, 0xFF000000, 255) == 0xFF000000);
  CHECK (easel_cover_mix255 (0x12345678, 0xFF000000, 0) == 0x12345678);
  CHECK (easel_cover_mix255 (0, 0x80FF0000, 128) == 0x40FF0000);            /* over transparency the red is kept */
  CHECK (easel_cover_mix255 (0xFF000000, 0x00000000, 255) == 0);
}

int
main (void)
{
  RUN (as6_mix_transparent);
  RUN (as6_mix_white);
  RUN (mix_oracle);
  RUN (mix255_oracle);
  RUN (as4_ellipse_points);
  RUN (as4_rectangle_is_sharp);
  RUN (cover_oracle_boxes);
  RUN (as3_line_points);
  RUN (as3_level_line_is_sharp);
  RUN (as6_line_counts);
  RUN (cover_oracle_lines);
  RUN (as9_bow_tie);
  RUN (as9_rounded_and_curve);
  RUN (cover_oracle_paths);
  RUN (smooth_apply_styles);
  RUN (smooth_apply_no_memory);
  RUN (smooth_polygon_no_corner);
  return 0;
}
