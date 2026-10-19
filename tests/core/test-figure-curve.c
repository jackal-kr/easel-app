/* SPDX-License-Identifier: GPL-3.0-or-later */
/* The curve of feature 0011 (REQ-0011-05): the pixels of the cubic Bézier curve through two control points, one pixel thin,
   with the square pen. */

#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "check.h"
#include "figure.h"
#include "history.h"
#include "image.h"

#define WHITE 0xFFFFFFFFu
#define BLACK 0xFF000000u
#define W 100
#define H 100

static EaselFigure
curve (const int *pts, int width)
{
  return (EaselFigure) { EASEL_FIGURE_CURVE, 0, 0, 0, 0, width, EASEL_FIGURE_OUTLINE, NULL, pts, 4, false };
}

static EaselFigure
line (int x0, int y0, int x1, int y1, int width)
{
  return (EaselFigure) { EASEL_FIGURE_LINE, x0, y0, x1, y1, width, EASEL_FIGURE_OUTLINE, NULL, NULL, 0, false };
}

/* The pixels the figure draws on a white W x H picture: set[y * W + x] is 1 where it is black. */
static int
draw (const EaselFigure *f, unsigned char *set)
{
  EaselImage *img = easel_image_new (W, H, WHITE);
  size_t changed = 0;
  int n = 0;

  CHECK (img != NULL);
  easel_figure_apply (img, NULL, f, BLACK, WHITE, &changed);
  for (int i = 0; i < W * H; i++)
    {
      set[i] = img->px[i] == BLACK;
      n += set[i];
    }
  CHECK ((size_t) n == changed);
  easel_image_free (img);
  return n;
}

/* Every set pixel has a set neighbour (8-connected), a weak test of a path in one piece. */
static bool
has_neighbours (const unsigned char *set)
{
  for (int y = 0; y < H; y++)
    for (int x = 0; x < W; x++)
      if (set[y * W + x])
        {
          int near = 0;

          for (int dy = -1; dy <= 1; dy++)
            for (int dx = -1; dx <= 1; dx++)
              if ((dx || dy) && x + dx >= 0 && x + dx < W && y + dy >= 0 && y + dy < H && set[(y + dy) * W + x + dx])
                near++;
          if (near == 0)
            return false;
        }
  return true;
}

static void
as2_curve_pixels (void)
{
  static const int pts[8] = { 10, 40, 30, 20, 30, 20, 50, 40 };
  EaselFigure f = curve (pts, 1);
  unsigned char set[W * H];

  CHECK (draw (&f, set) == 41);
  CHECK (set[40 * W + 10] && set[40 * W + 50]);
  for (int x = 0; x < W; x++)
    CHECK ((x >= 28 && x <= 32) == (set[25 * W + x] != 0));      /* the top: row 25, exactly 28 to 32 */
  for (int y = 0; y < 25; y++)
    for (int x = 0; x < W; x++)
      CHECK (!set[y * W + x]);                                   /* nothing above row 25 */
  CHECK (has_neighbours (set));
}

static void
as3_straight_curve_is_the_line (void)
{
  static const int on_segment[8] = { 10, 10, 25, 10, 25, 10, 40, 10 };
  static const int overshoot[8] = { 10, 10, 60, 10, 60, 10, 40, 10 };
  unsigned char a[W * H], b[W * H];
  EaselFigure c, l;

  for (int w = 1; w <= 5; w += 2)
    {
      c = curve (on_segment, w);
      l = line (10, 10, 40, 10, w);
      draw (&c, a);
      draw (&l, b);
      CHECK (memcmp (a, b, sizeof a) == 0);
    }
  c = curve (on_segment, 1);
  CHECK (draw (&c, a) == 31);
  /* Control points on the segment's line but beyond an end: the curve overshoots past (40, 10). */
  c = curve (overshoot, 1);
  draw (&c, a);
  {
    bool beyond = false;

    for (int i = 0; i < W * H; i++)
      beyond |= a[i] && i % W > 40;
    CHECK (beyond);
  }
}

static void
as4_s_curve (void)
{
  static const int pts[8] = { 10, 40, 30, 20, 50, 80, 50, 40 };
  EaselFigure f = curve (pts, 1);
  unsigned char set[W * H];
  bool above_left = false, below_right = false;
  int lowest = 0;

  CHECK (draw (&f, set) == 52);
  for (int y = 0; y < H; y++)
    for (int x = 0; x < W; x++)
      if (set[y * W + x])
        {
          above_left |= y < 40 && x < 30;
          below_right |= y > 40 && x > 30;
          if (y > lowest)
            lowest = y;
        }
  CHECK (above_left && below_right);
  CHECK (lowest == 54);
  CHECK (has_neighbours (set));
}

/* ---- REQ-0011-05 again, in the test's own words ---- */

typedef struct
{
  int x, y;
} P;

static bool
on_closed_segment (const int p[8], int qx, int qy)
{
  long long cross = (long long) (p[6] - p[0]) * (qy - p[1]) - (long long) (p[7] - p[1]) * (qx - p[0]);

  return cross == 0 && qx >= (p[0] < p[6] ? p[0] : p[6]) && qx <= (p[0] < p[6] ? p[6] : p[0])
         && qy >= (p[1] < p[7] ? p[1] : p[7]) && qy <= (p[1] < p[7] ? p[7] : p[1]);
}

/* The Pencil's line from a to b (REQ-0001-20), both ends, appended to out; the Pencil's tie-breaking, which test-figure
   checks against the Pencil itself. */
static size_t
ref_line (P a, P b, P *out, size_t n)
{
  int dx = abs (b.x - a.x), dy = -abs (b.y - a.y), sx = a.x < b.x ? 1 : -1, sy = a.y < b.y ? 1 : -1;
  int err = dx + dy;

  for (;;)
    {
      int e2 = 2 * err;

      out[n++] = a;
      if (a.x == b.x && a.y == b.y)
        break;
      if (e2 >= dy)
        {
          err += dy;
          a.x += sx;
        }
      if (e2 <= dx)
        {
          err += dx;
          a.y += sy;
        }
    }
  return n;
}

/* The pixel of the curve at i / n: the nearest, a half rounded up, in exact integer arithmetic. */
static int
ref_coord (int p0, int p1, int p2, int p3, long long n, long long i)
{
  unsigned __int128 d = (unsigned __int128) n * n * n, u = n - i, t = i;
  unsigned __int128 num = u * u * u * (unsigned) (p0 + 1000000) + 3 * u * u * t * (unsigned) (p1 + 1000000)
                          + 3 * u * t * t * (unsigned) (p2 + 1000000) + t * t * t * (unsigned) (p3 + 1000000);

  return (int) ((num * 2 + d) / (2 * d)) - 1000000;
}

/* The curve's 1-pixel path by REQ-0011-05; returns its length. */
static size_t
ref_path (const int p[8], P *path)
{
  P samples[2000], joined[8000];
  size_t ns = 0, nj = 0, n_out = 0;
  double len = 0;

  if (on_closed_segment (p, p[2], p[3]) && on_closed_segment (p, p[4], p[5]))
    return ref_line ((P) { p[0], p[1] }, (P) { p[6], p[7] }, path, 0);
  for (int k = 0; k < 3; k++)
    len += sqrt ((double) (p[2 * k + 2] - p[2 * k]) * (p[2 * k + 2] - p[2 * k])
                 + (double) (p[2 * k + 3] - p[2 * k + 1]) * (p[2 * k + 3] - p[2 * k + 1]));
  long long n = (long long) ceil (len) < 1 ? 1 : (long long) ceil (len);

  for (long long i = 0; i <= n; i++)
    {
      P q = { ref_coord (p[0], p[2], p[4], p[6], n, i), ref_coord (p[1], p[3], p[5], p[7], n, i) };

      if (ns == 0 || samples[ns - 1].x != q.x || samples[ns - 1].y != q.y)
        samples[ns++] = q;
    }
  joined[nj++] = samples[0];
  for (size_t k = 1; k < ns; k++)
    {
      P seg[64];
      size_t m = ref_line (samples[k - 1], samples[k], seg, 0);

      for (size_t j = 1; j < m; j++)
        joined[nj++] = seg[j];
    }
  path[n_out++] = joined[0];
  for (size_t i = 1; i + 1 < nj; i++)
    {
      P a = path[n_out - 1], c = joined[i + 1];

      if (abs (a.x - c.x) <= 1 && abs (a.y - c.y) <= 1)
        continue;
      path[n_out++] = joined[i];
    }
  if (nj > 1)
    path[n_out++] = joined[nj - 1];
  return n_out;
}

static void
as4_curve_matches_the_rule (void)
{
  unsigned int seed = 99;
  unsigned char want[W * H], got[W * H];

  for (int n = 0; n < 400; n++)
    {
      int pts[8], w = 1 + n % 5, a = (w - 1) / 2;
      static P path[8000];
      size_t len;
      EaselFigure f;

      for (int k = 0; k < 8; k++)
        {
          seed = seed * 1103515245u + 12345u;
          pts[k] = (int) ((seed >> 8) % 100);
        }
      if (n % 7 == 0)                                  /* some with control points on the line between the ends */
        {
          pts[2] = pts[4] = pts[0];
          pts[3] = pts[5] = pts[1];
        }
      len = ref_path (pts, path);
      memset (want, 0, sizeof want);
      for (size_t i = 0; i < len; i++)
        {
          if (i > 0)                                   /* one piece: consecutive pixels touch */
            CHECK (abs (path[i].x - path[i - 1].x) <= 1 && abs (path[i].y - path[i - 1].y) <= 1);
          for (int sy = 0; sy < w; sy++)
            for (int sx = 0; sx < w; sx++)
              {
                int x = path[i].x - a + sx, y = path[i].y - a + sy;

                if (x >= 0 && x < W && y >= 0 && y < H)
                  want[y * W + x] = 1;
              }
        }
      f = curve (pts, w);
      draw (&f, got);
      CHECK (memcmp (want, got, sizeof want) == 0);
    }
}

int
main (void)
{
  RUN (as2_curve_pixels);
  RUN (as3_straight_curve_is_the_line);
  RUN (as4_s_curve);
  RUN (as4_curve_matches_the_rule);
  return 0;
}
