/* SPDX-License-Identifier: GPL-3.0-or-later */
/* 0039: the Brush with Smooth (REQ-0039-05 (e), REQ-0039-07, REQ-0039-11; AS-5, AS-11). */

#include <stdlib.h>
#include <string.h>

#include "check.h"
#include "cover.h"
#include "history.h"
#include "image.h"
#include "stroke.h"

#define WHITE 0xFFFFFFFFu
#define BLACK 0xFF000000u

static uint64_t rng = 0x2545F4914F6CDD1Du;

static uint32_t
next (void)
{
  rng ^= rng << 13;
  rng ^= rng >> 7;
  rng ^= rng << 17;
  return (uint32_t) rng;
}

/* ---- the reference, written again from REQ-0039-05 (e): in 1/32 pixel ---- */

/* A fraction n / d with d > 0. */
typedef struct
{
  __int128 n, d;
} Frac;

static int
frac_lt (Frac a, Frac b)
{
  return a.n * b.d < b.n * a.d;
}

/* Is there a t in [0, 1] with a . (p - t v) <= b for every half-plane (ax, ay, b)? */
static int
sweep_halfplanes (const long long (*hp)[3], int n, long long X, long long Y, long long vx, long long vy)
{
  Frac lo = { 0, 1 }, hi = { 1, 1 };

  for (int k = 0; k < n; k++)
    {
      __int128 ap = (__int128) hp[k][0] * X + (__int128) hp[k][1] * Y - hp[k][2], av = (__int128) hp[k][0] * vx + (__int128) hp[k][1] * vy;

      /* ap - t av <= 0 */
      if (av == 0)
        {
          if (ap > 0)
            return 0;
        }
      else if (av > 0)
        {
          Frac t = { ap, av };                                   /* t >= ap / av */

          if (frac_lt (lo, t))
            lo = t;
        }
      else
        {
          Frac t = { -ap, -av };                                 /* t <= ap / av */

          if (frac_lt (t, hi))
            hi = t;
        }
    }
  return !frac_lt (hi, lo);
}

/* Is the point (X, Y) in the footprint placed at pointer pixel (x0, y0) and swept to (x1, y1)? */
static int
ref_in_sweep (EaselFootprint f, int x0, int y0, int x1, int y1, long long X, long long Y)
{
  int a = (f.size - 1) / 2;
  long long L = 32LL * (x0 - a), T = 32LL * (y0 - a), S = 32LL * f.size, vx = 32LL * (x1 - x0), vy = 32LL * (y1 - y0);

  if (f.shape == EASEL_SHAPE_CIRCLE && f.size > 1)
    {
      /* within S / 2 of the segment between the two centers */
      long long cx = L + S / 2, cy = T + S / 2, px = X - cx, py = Y - cy, r = S / 2;
      __int128 l2 = (__int128) vx * vx + (__int128) vy * vy, t = (__int128) px * vx + (__int128) py * vy;

      if (l2 == 0 || t <= 0)
        return (__int128) px * px + (__int128) py * py <= (__int128) r * r;
      if (t >= l2)
        return (__int128) (px - vx) * (px - vx) + (__int128) (py - vy) * (py - vy) <= (__int128) r * r;
      {
        __int128 c = (__int128) px * vy - (__int128) py * vx;

        return c * c <= (__int128) r * r * l2;
      }
    }
  {
    long long hp[6][3] = { { -1, 0, -L }, { 1, 0, L + S }, { 0, -1, -T }, { 0, 1, T + S } };
    int n = 4;

    if (f.shape == EASEL_SHAPE_SLASH)
      {
        /* |x + y - (L + T + S)| <= 16 */
        hp[n][0] = 1, hp[n][1] = 1, hp[n++][2] = L + T + S + 16;
        hp[n][0] = -1, hp[n][1] = -1, hp[n++][2] = -(L + T + S - 16);
      }
    else if (f.shape == EASEL_SHAPE_BACKSLASH)
      {
        /* |x - y - (L - T)| <= 16 */
        hp[n][0] = 1, hp[n][1] = -1, hp[n++][2] = L - T + 16;
        hp[n][0] = -1, hp[n][1] = 1, hp[n++][2] = -(L - T - 16);
      }
    return sweep_halfplanes ((const long long (*)[3]) hp, n, X, Y, vx, vy);
  }
}

/* The points of pixel (px, py) in the stroke through the n pointer pixels xy. */
static unsigned
ref_count (EaselFootprint f, const int *xy, int n, int px, int py)
{
  unsigned c = 0;

  for (int j = 0; j < 16; j++)
    for (int i = 0; i < 16; i++)
      {
        long long X = 32LL * px + 2 * i + 1, Y = 32LL * py + 2 * j + 1;
        int in = ref_in_sweep (f, xy[0], xy[1], xy[0], xy[1], X, Y);

        for (int k = 1; k < n && !in; k++)
          in = ref_in_sweep (f, xy[2 * k - 2], xy[2 * k - 1], xy[2 * k], xy[2 * k + 1], X, Y);
        c += (unsigned) in;
      }
  return c;
}

static EaselStroke *
draw (EaselImage *img, EaselFootprint f, const int *xy, int n, uint32_t color)
{
  EaselStroke *s = easel_stroke_begin_smooth (img, xy[0], xy[1], color, f);

  CHECK (s != NULL);
  for (int k = 1; k < n; k++)
    easel_stroke_to (s, xy[2 * k], xy[2 * k + 1]);
  return s;
}

/* Every pixel of the picture against the reference: the mix of its color before the stroke and color by the union's points. */
static void
check_against_reference (const EaselImage *img, const EaselImage *before, EaselFootprint f, const int *xy, int n, uint32_t color)
{
  for (int y = 0; y < img->height; y++)
    for (int x = 0; x < img->width; x++)
      {
        unsigned c = ref_count (f, xy, n, x, y);
        uint32_t old = easel_image_get (before, x, y), want = c == 0 ? old : easel_cover_mix (old, color, c, 0, 0);

        if (easel_image_get (img, x, y) != want)
          fprintf (stderr, "shape %d size %d, pixel (%d, %d): %08X, want %08X (%u points)\n", f.shape, f.size, x, y,
                   easel_image_get (img, x, y), want, c);
        CHECK (easel_image_get (img, x, y) == want);
      }
}

/* AS-11 (REQ-0039-05 (e), REQ-0039-11): a click with the circle of 1 paints one whole pixel; the circle of 4 at (5, 5): its middle
   2 x 2 whole, its edges 235 points, its corners 81. */
static void
as11_brush_click (void)
{
  EaselImage *img = easel_image_new (20, 20, WHITE);
  EaselFootprint one = { EASEL_SHAPE_CIRCLE, 1 }, four = { EASEL_SHAPE_CIRCLE, 4 };
  static const unsigned four_counts[4][4] = { { 81, 235, 235, 81 }, { 235, 256, 256, 235 }, { 235, 256, 256, 235 }, { 81, 235, 235, 81 } };
  int xy[2] = { 5, 5 };

  CHECK (img != NULL);
  easel_stroke_end (draw (img, one, xy, 1, BLACK), NULL);
  for (int y = 0; y < 20; y++)
    for (int x = 0; x < 20; x++)
      CHECK (easel_image_get (img, x, y) == (x == 5 && y == 5 ? BLACK : WHITE));
  CHECK (easel_cover_footprint (one, 0, 0) == 256);
  easel_image_free (img);
  img = easel_image_new (20, 20, WHITE);
  easel_stroke_end (draw (img, four, xy, 1, BLACK), NULL);
  for (int row = 0; row < 4; row++)
    for (int col = 0; col < 4; col++)
      {
        CHECK (easel_cover_footprint (four, col, row) == four_counts[row][col]);
        CHECK (easel_image_get (img, 4 + col, 4 + row) == easel_cover_mix (WHITE, BLACK, four_counts[row][col], 0, 0));
      }
  CHECK (easel_image_get (img, 5, 4) == 0xFF151515u);
  CHECK (easel_image_get (img, 3, 5) == WHITE && easel_image_get (img, 8, 5) == WHITE);
  easel_image_free (img);
}

/* AS-5 (REQ-0039-05 (e), REQ-0039-07): the circle of 7 dragged (5, 5) to (25, 5) to (5, 6): each pixel by the union of the sweep,
   so where the way back overlaps the way there it is no darker than one pass. */
static void
as5_brush_union (void)
{
  EaselImage *img = easel_image_new (40, 16, WHITE), *before = easel_image_new (40, 16, WHITE);
  EaselFootprint f = { EASEL_SHAPE_CIRCLE, 7 };
  static const int xy[6] = { 5, 5, 25, 5, 5, 6 };
  int partial = 0;

  CHECK (img != NULL && before != NULL);
  easel_stroke_end (draw (img, f, xy, 3, BLACK), NULL);
  check_against_reference (img, before, f, xy, 3, BLACK);
  /* (26, 2) is covered at the turn by the way there and the way back alike: 215 points (docs/reference/proto-0039), once */
  CHECK (ref_count (f, xy, 3, 26, 2) == 215);
  CHECK (easel_image_get (img, 26, 2) == easel_cover_mix (WHITE, BLACK, 215, 0, 0));
  for (int x = 0; x < 40; x++)
    partial += easel_image_get (img, x, 2) != WHITE && easel_image_get (img, x, 2) != BLACK;
  CHECK (partial == 6);                                         /* the top row is whole but for its ends */
  easel_image_free (img);
  easel_image_free (before);
}

/* AS-5: "/" of 8 dragged across its own direction leaves no empty diagonal between stamps: the band between is whole. */
static void
as5_slash_no_gaps (void)
{
  EaselImage *img = easel_image_new (40, 40, WHITE), *before = easel_image_new (40, 40, WHITE);
  EaselFootprint f = { EASEL_SHAPE_SLASH, 8 };
  static const int xy[4] = { 10, 10, 30, 30 };

  CHECK (img != NULL && before != NULL);
  easel_stroke_end (draw (img, f, xy, 2, BLACK), NULL);
  check_against_reference (img, before, f, xy, 2, BLACK);
  for (int k = 12; k <= 28; k++)
    CHECK (easel_image_get (img, k, k) == BLACK && easel_image_get (img, k + 1, k) == BLACK);
  easel_image_free (img);
  easel_image_free (before);
}

/* The oracle: random strokes of every preset shape, over a picture of mixed opaque and transparent colors. */
static void
cover_oracle_strokes (void)
{
  static const EaselFootprint shapes[] = { { EASEL_SHAPE_CIRCLE, 7 }, { EASEL_SHAPE_CIRCLE, 4 }, { EASEL_SHAPE_CIRCLE, 1 },
                                           { EASEL_SHAPE_SQUARE, 8 }, { EASEL_SHAPE_SQUARE, 2 }, { EASEL_SHAPE_SLASH, 8 },
                                           { EASEL_SHAPE_SLASH, 2 }, { EASEL_SHAPE_BACKSLASH, 5 }, { EASEL_SHAPE_BACKSLASH, 2 } };

  for (int k = 0; k < 180; k++)
    {
      EaselFootprint f = shapes[k % 9];
      int n = 1 + (int) (next () % 4), xy[8];
      EaselImage *img = easel_image_new (30, 30, 0), *before = easel_image_new (30, 30, 0);
      uint32_t color = k % 4 == 0 ? 0x80FF2000u : 0xFF000000u | (next () & 0xFFFFFF);

      CHECK (img != NULL && before != NULL);
      for (int i = 0; i < 30 * 30; i++)
        img->px[i] = before->px[i] = i % 5 == 0 ? 0 : 0xFF000000u | (uint32_t) (i * 2654435761u >> 8);
      for (int q = 0; q < 2 * n; q++)
        xy[q] = (int) (next () % 34) - 2;
      if (n > 1 && k % 7 == 0)
        xy[2] = xy[0], xy[3] = xy[1];                             /* a position repeated */
      easel_stroke_end (draw (img, f, xy, n, color), NULL);
      check_against_reference (img, before, f, xy, n, color);
      easel_image_free (img);
      easel_image_free (before);
    }
}

/* REQ-0039-08: cancel puts every pixel back; the end makes one step that Undo takes back. */
static void
smooth_stroke_cancel_and_undo (void)
{
  EaselImage *img = easel_image_new (40, 30, WHITE);
  EaselHistory *h = easel_history_new (1 << 20);
  EaselFootprint f = { EASEL_SHAPE_CIRCLE, 7 };
  static const int xy[6] = { 3, 3, 35, 20, 5, 25 };
  size_t changed;

  CHECK (img != NULL && h != NULL);
  easel_stroke_cancel (draw (img, f, xy, 3, BLACK));
  for (int i = 0; i < 40 * 30; i++)
    CHECK (img->px[i] == WHITE);
  changed = easel_stroke_end (draw (img, f, xy, 3, BLACK), h);
  CHECK (changed > 100);
  CHECK (easel_history_undo (h, img) && !easel_history_can_undo (h));
  for (int i = 0; i < 40 * 30; i++)
    CHECK (img->px[i] == WHITE);
  easel_history_free (h);
  easel_image_free (img);
}

/* REQ-0039-08: a tile of points that cannot be had leaves its pixels as they are, and the rest is drawn. */
static void
smooth_stroke_tiles_capped (void)
{
  EaselImage *img = easel_image_new (100, 40, WHITE);
  EaselFootprint f = { EASEL_SHAPE_SQUARE, 2 };
  EaselStroke *s;
  int far = 0;

  CHECK (img != NULL);
  s = easel_stroke_begin_smooth (img, 5, 5, BLACK, f);
  CHECK (s != NULL);
  easel_stroke_limit_tiles (s, 1);
  easel_stroke_to (s, 90, 30);
  CHECK (easel_image_get (img, 5, 5) == BLACK && easel_image_get (img, 10, 7) != WHITE);
  for (int y = 16; y < 40; y++)
    for (int x = 16; x < 100; x++)
      far += easel_image_get (img, x, y) != WHITE;
  CHECK (far == 0);
  easel_stroke_end (s, NULL);
  easel_image_free (img);
}

int
main (void)
{
  RUN (as11_brush_click);
  RUN (as5_brush_union);
  RUN (as5_slash_no_gaps);
  RUN (cover_oracle_strokes);
  RUN (smooth_stroke_cancel_and_undo);
  RUN (smooth_stroke_tiles_capped);
  return 0;
}
