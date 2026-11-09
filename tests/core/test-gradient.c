/* SPDX-License-Identifier: GPL-3.0-or-later */
/* The gradient of features 0012 and 0032: its opacity at a pixel, Linear, Radial or Reflected (REQ-0012-04, REQ-0032-04 to -06),
   its blend (REQ-0012-05), its apply (REQ-0012-09). */

#include <malloc.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/resource.h>

#include "check.h"
#include "figure.h"
#include "gradient.h"
#include "history.h"
#include "image.h"

#define WHITE 0xFFFFFFFFu
#define BLACK 0xFF000000u
#define GRAY(g) (0xFF000000u | (uint32_t) (g) << 16 | (uint32_t) (g) << 8 | (uint32_t) (g))

/* Linear, the type a gradient built without one has (REQ-0032-04). */
static EaselGradient
gradient (int x0, int y0, int x1, int y1, uint32_t color)
{
  return (EaselGradient) { x0, y0, x1, y1, color };
}

static EaselGradient
typed (int x0, int y0, int x1, int y1, uint32_t color, EaselGradientType type)
{
  return (EaselGradient) { x0, y0, x1, y1, color, type };
}

/* The gradient applied over a picture of w x h of color under, without a history. */
static EaselImage *
applied (const EaselGradient *g, int w, int h, uint32_t under)
{
  EaselImage *img = easel_image_new (w, h, under);

  CHECK (img != NULL);
  easel_gradient_apply (img, NULL, g, NULL);
  return img;
}

static int
count (const EaselImage *img, uint32_t color)
{
  int n = 0;

  for (int i = 0; i < img->width * img->height; i++)
    n += img->px[i] == color;
  return n;
}

/* AS-2: the drag from (5, 5) to (15, 5), black: the opacities of a row (every row alike), and applied over white, the
   pixels, 165 of them changed in one undo step, and one Undo restoring white. */
static void
as2_gradient_pixels (void)
{
  static const unsigned want[21] = { 255, 255, 255, 255, 255, 255, 230, 204, 179, 153, 128, 102, 77, 51, 26, 0, 0, 0, 0, 0, 0 };
  static const unsigned gray[21] = { 0, 0, 0, 0, 0, 0, 25, 51, 76, 102, 127, 153, 178, 204, 229, 255, 255, 255, 255, 255, 255 };
  EaselGradient g = gradient (5, 5, 15, 5, BLACK);
  EaselImage *img = easel_image_new (21, 11, WHITE);
  EaselHistory *h = easel_history_new (16 * 1024 * 1024);
  size_t changed = 0;

  for (int y = 0; y < 11; y++)
    for (int x = 0; x < 21; x++)
      CHECK (easel_gradient_opacity (&g, x, y) == want[x]);
  CHECK (img != NULL && h != NULL);
  CHECK (easel_gradient_apply (img, h, &g, &changed) == EASEL_GRADIENT_DONE);
  for (int y = 0; y < 11; y++)
    for (int x = 0; x < 21; x++)
      CHECK (img->px[y * 21 + x] == (0xFF000000u | gray[x] << 16 | gray[x] << 8 | gray[x]));
  CHECK (changed == 165);
  CHECK (easel_history_can_undo (h) && easel_history_is_modified (h));
  CHECK (easel_history_undo (h, img));
  CHECK (count (img, WHITE) == 21 * 11);
  CHECK (!easel_history_can_undo (h));
  easel_history_free (h);
  easel_image_free (img);
}

/* REQ-0012-07: the preview and the apply lay the gradient with one function, for every type. */
static void
lay_is_the_apply (void)
{
  enum { W = 37, H = 23 };

  for (int t = 0; t < 3; t++)
    {
      EaselImage *img = easel_image_new (W, H, WHITE), *done;
      EaselGradient g = typed (30, -4, 3, 19, 0x80336699u, (EaselGradientType) t);
      uint32_t buf[W * H], part[11 * 7];

      CHECK (img != NULL);
      for (int i = 0; i < W * H; i++)                     /* opaque, transparent and half-transparent pixels */
        img->px[i] = i % 3 == 0 ? 0xFFFFFF00u : i % 3 == 1 ? 0x00000000u : 0x80FFFFFFu;
      done = easel_image_new (W, H, WHITE);
      CHECK (done != NULL);
      for (int i = 0; i < W * H; i++)
        done->px[i] = img->px[i];
      CHECK (easel_gradient_apply (done, NULL, &g, NULL) == EASEL_GRADIENT_DONE);
      easel_gradient_lay (&g, img, 0, 0, W, H, buf, W);
      for (int i = 0; i < W * H; i++)
        CHECK (buf[i] == done->px[i]);
      /* A rectangle away from the corner, written with its own stride. */
      easel_gradient_lay (&g, img, 20, 9, 11, 7, part, 11);
      for (int y = 0; y < 7; y++)
        for (int x = 0; x < 11; x++)
          {
            uint32_t src = img->px[(9 + y) * W + 20 + x];

            CHECK (part[y * 11 + x] == easel_gradient_blend (src, g.color, easel_gradient_opacity (&g, 20 + x, 9 + y)));
          }
      CHECK (easel_gradient_opacity (&g, 0, 22) == 0 && buf[22 * W] == img->px[22 * W]);    /* beyond the end: copied */
      easel_image_free (done);
      easel_image_free (img);
    }
}

/* The core rule (bounds-clip every pixel access): a rectangle partly outside the picture is laid over only where it is
   inside; the buffer's places for pixels outside the picture are left as they were. */
static void
lay_clips_its_rectangle (void)
{
  enum { W = 5, H = 4, RW = 10, RH = 7, X0 = -3, Y0 = -2 };
  EaselImage *img = easel_image_new (W, H, 0xFFFFFF00u);
  EaselGradient g = gradient (0, 0, 4, 3, BLACK);
  uint32_t buf[RW * RH];

  CHECK (img != NULL);
  for (int i = 0; i < RW * RH; i++)
    buf[i] = 0x12345678u;
  easel_gradient_lay (&g, img, X0, Y0, RW, RH, buf, RW);
  for (int j = 0; j < RH; j++)
    for (int i = 0; i < RW; i++)
      {
        int x = X0 + i, y = Y0 + j;

        if (x < 0 || y < 0 || x >= W || y >= H)
          CHECK (buf[j * RW + i] == 0x12345678u);
        else
          CHECK (buf[j * RW + i] == easel_gradient_blend (0xFFFFFF00u, BLACK, easel_gradient_opacity (&g, x, y)));
      }
  easel_image_free (img);
}

/* AS-13 (REQ-0012-09), the core part: a gradient that changes no pixel adds no step. */
static void
as13_gradient_changing_nothing (void)
{
  static const uint32_t colors[] = { WHITE, 0x00000000u, 0x80FFFFFFu, 0x00FF0000u };
  EaselImage *img = easel_image_new (21, 11, WHITE);
  EaselHistory *h = easel_history_new (16 * 1024 * 1024);
  size_t changed = 7;

  CHECK (img != NULL && h != NULL);
  for (size_t k = 0; k < sizeof colors / sizeof colors[0]; k++)
    {
      EaselGradient g = typed (5, 5, 15, 5, colors[k], (EaselGradientType) (k % 3));

      CHECK (easel_gradient_apply (img, h, &g, &changed) == EASEL_GRADIENT_NOTHING);
      CHECK (changed == 0 && count (img, WHITE) == 21 * 11);
      CHECK (!easel_history_can_undo (h) && !easel_history_is_modified (h));
    }
  easel_history_free (h);
  easel_image_free (img);
}

/* The size of the process's address space now, in bytes. */
static size_t
address_space (void)
{
  unsigned long pages = 0;
  FILE *f = fopen ("/proc/self/statm", "r");

  CHECK (f != NULL);
  CHECK (fscanf (f, "%lu", &pages) == 1);
  fclose (f);
  return (size_t) pages * 4096;
}

/* AS-14 (REQ-0012-09), the core part: with no memory for its step the gradient changes nothing and adds no step; a step
   larger than the whole budget is kept as the only step and one Undo restores the picture. */
static void
as14_gradient_memory_and_budget (void)
{
  enum { N = 1800 };
  EaselImage *img = easel_image_new (N, N, WHITE);
  EaselHistory *h = easel_history_new (16 * 1024 * 1024);
  EaselGradient g = gradient (0, 0, 2 * N, 0, BLACK);         /* every pixel of the picture has an opacity above 0 */
  size_t changed = 0;
  struct rlimit old, lim;

  CHECK (img != NULL && h != NULL);
  CHECK (getrlimit (RLIMIT_AS, &old) == 0);
  lim = old;
  lim.rlim_cur = address_space () + 4 * 1024 * 1024;
  CHECK (setrlimit (RLIMIT_AS, &lim) == 0);
  CHECK (easel_gradient_apply (img, h, &g, &changed) == EASEL_GRADIENT_NO_MEMORY);
  CHECK (setrlimit (RLIMIT_AS, &old) == 0);
  CHECK (changed == 0 && count (img, WHITE) == N * N);                       /* every pixel back */
  CHECK (!easel_history_can_undo (h) && !easel_history_is_modified (h));
  /* The test hook with a cap does the same without limiting the process. */
  CHECK (easel_gradient_apply_capped (img, h, &g, &changed, 1024 * 1024) == EASEL_GRADIENT_NO_MEMORY);
  CHECK (changed == 0 && count (img, WHITE) == N * N && !easel_history_can_undo (h));
  /* With memory: a step of 3.24 million pixels, 38.9 MB against a budget of 16 MB: the only step. */
  CHECK (easel_gradient_apply (img, h, &g, &changed) == EASEL_GRADIENT_DONE);
  CHECK (changed == (size_t) N * N && count (img, WHITE) == 0);
  CHECK (easel_history_bytes (h) == (size_t) N * N * 12 + 64 && easel_history_bytes (h) > 16 * 1024 * 1024);
  CHECK (easel_history_undo (h, img));
  CHECK (count (img, WHITE) == N * N);
  CHECK (!easel_history_can_undo (h));
  easel_history_free (h);
  easel_image_free (img);
}

/* The heap memory in use now, as glibc counts it: the small blocks and the mapped ones (a big log is a mapped block). */
static size_t
heap_in_use (void)
{
  struct mallinfo2 mi = mallinfo2 ();

  return mi.uordblks + mi.hblkhd;
}

/* AS-14, REQ-0002-16: the memory a gradient's step holds is the memory the budget counts. A gradient that changes just more
   pixels than a power of two of log entries must not leave the log's spare half in the history. */
static void
as14_step_holds_what_it_counts (void)
{
  EaselImage *img = easel_image_new (513, 512, WHITE);
  EaselHistory *h = easel_history_new ((size_t) 512 << 20);
  EaselGradient g = gradient (0, 0, 2000, 0, BLACK);
  size_t changed = 0, before, held, counted;

  CHECK (img != NULL && h != NULL);
  before = heap_in_use ();
  CHECK (easel_gradient_apply (img, h, &g, &changed) == EASEL_GRADIENT_DONE);
  CHECK (changed == 513u * 512u);
  held = heap_in_use () - before;
  counted = easel_history_bytes (h);
  printf ("  changed %zu pixels: held %zu bytes, counted %zu\n", changed, held, counted);
  CHECK (held <= counted + counted / 100 + 4096);
  easel_history_free (h);
  easel_image_free (img);
}

/* AS-3: Shift from (5, 5) toward (14, 8) gives the end (14, 5) (the Line's constraint), and these opacities. */
static void
as3_shift_gradient_pixels (void)
{
  static const unsigned want[12] = { 255, 255, 227, 198, 170, 142, 113, 85, 57, 28, 0, 0 };     /* columns 4 to 15 */
  int x1 = 14, y1 = 8;
  EaselGradient g;

  easel_figure_constrain (EASEL_FIGURE_LINE, 5, 5, &x1, &y1);
  CHECK (x1 == 14 && y1 == 5);
  g = gradient (5, 5, x1, y1, 0xFF000000u);
  for (int x = 4; x <= 15; x++)
    CHECK (easel_gradient_opacity (&g, x, 5) == want[x - 4]);
}

/* AS-4: the pixels at the same distance along a diagonal drag have the same opacity. */
static void
as4_equal_opacity_lines (void)
{
  EaselGradient g = gradient (0, 0, 10, 10, 0xFF000000u);

  CHECK (easel_gradient_opacity (&g, 5, 5) == 128);
  CHECK (easel_gradient_opacity (&g, 0, 10) == 128);
  CHECK (easel_gradient_opacity (&g, 10, 0) == 128);
  CHECK (easel_gradient_opacity (&g, 3, 7) == 128);
}

/* AS-6: a drawing color of alpha 128 starts the ramp at 128. */
static void
as6_transparent_color (void)
{
  EaselGradient g = gradient (5, 5, 15, 5, 0x80000000u);

  CHECK (easel_gradient_opacity (&g, 5, 5) == 128);
  CHECK (easel_gradient_opacity (&g, 0, 5) == 128);
  CHECK (easel_gradient_opacity (&g, 10, 5) == 64);
  CHECK (easel_gradient_opacity (&g, 14, 5) == 13);
  CHECK (easel_gradient_opacity (&g, 15, 5) == 0);
}

/* AS-8: the opacity does not depend on the color's channels, only on its alpha. */
static void
as8_secondary_color (void)
{
  EaselGradient g = gradient (5, 5, 15, 5, 0xFFFF0000u);

  CHECK (easel_gradient_opacity (&g, 10, 5) == 128);
}

/* AS-9: a drag from outside the picture to outside it. */
static void
as9_gradient_from_outside (void)
{
  EaselGradient g = gradient (-10, 5, 30, 5, 0xFF000000u);

  CHECK (easel_gradient_opacity (&g, 0, 5) == 191);
  CHECK (easel_gradient_opacity (&g, 10, 5) == 128);
  CHECK (easel_gradient_opacity (&g, 20, 5) == 64);
}

/* AS-18: drags of one pixel, to a diagonal and to a horizontal neighbour. */
static void
as18_one_pixel_drags (void)
{
  static const unsigned diagonal[4][4] = { { 255, 255, 255, 128 }, { 255, 255, 128, 0 }, { 255, 128, 0, 0 }, { 128, 0, 0, 0 } };
  EaselGradient g = gradient (1, 1, 2, 2, 0xFF000000u), h = gradient (1, 1, 2, 1, 0xFF000000u);

  for (int y = 0; y < 4; y++)
    for (int x = 0; x < 4; x++)
      CHECK (easel_gradient_opacity (&g, x, y) == diagonal[y][x]);
  CHECK (easel_gradient_opacity (&h, 0, 1) == 255 && easel_gradient_opacity (&h, 1, 1) == 255);
  CHECK (easel_gradient_opacity (&h, 2, 1) == 0 && easel_gradient_opacity (&h, 3, 1) == 0);
  /* A drag of no length is no gradient. */
  g = gradient (4, 4, 4, 4, 0xFF000000u);
  CHECK (easel_gradient_opacity (&g, 4, 4) == 0 && easel_gradient_opacity (&g, 0, 0) == 0);
}

/* AS-5: the drag of AS-2, black, over a fully transparent picture, and over white at half opacity. */
static void
as5_blend_over_transparent (void)
{
  static const struct { int x; uint32_t want; } cols[] = {
    { 0, 0xFF000000u }, { 5, 0xFF000000u }, { 6, 0xE6000000u }, { 10, 0x80000000u }, { 14, 0x1A000000u }, { 15, 0x00000000u },
    { 20, 0x00000000u } };
  EaselGradient g = gradient (5, 5, 15, 5, 0xFF000000u);

  for (size_t k = 0; k < sizeof cols / sizeof cols[0]; k++)
    CHECK (easel_gradient_blend (0x00000000u, g.color, easel_gradient_opacity (&g, cols[k].x, 5)) == cols[k].want);
  CHECK (easel_gradient_opacity (&g, 10, 5) == 128);
  CHECK (easel_gradient_blend (0x80FFFFFFu, g.color, 128) == 0xC0555555u);
  /* A pixel of opacity 0 is left as it is, a transparent one with its color channels too. */
  CHECK (easel_gradient_blend (0x00123456u, g.color, 0) == 0x00123456u);
  CHECK (easel_gradient_blend (0x80FFFFFFu, g.color, 0) == 0x80FFFFFFu);
}

/* AS-7: the drag of AS-2 applied twice over white: the second lays over the first. */
static void
as7_second_gradient (void)
{
  static const struct { int x; uint32_t want; } cols[] = { { 6, 0xFF020202u }, { 10, 0xFF3F3F3Fu }, { 14, 0xFFCECECEu } };
  EaselGradient g = gradient (5, 5, 15, 5, 0xFF000000u);

  for (size_t k = 0; k < sizeof cols / sizeof cols[0]; k++)
    {
      unsigned a = easel_gradient_opacity (&g, cols[k].x, 5);
      uint32_t once = easel_gradient_blend (0xFFFFFFFFu, g.color, a);

      CHECK (once == (k == 0 ? 0xFF191919u : k == 1 ? 0xFF7F7F7Fu : 0xFFE5E5E5u));     /* gray 25, 127, 229 of AS-2 */
      CHECK (easel_gradient_blend (once, g.color, a) == cols[k].want);
    }
}

/* ---- The opacities again, in the test's own words (R3: not the code's formula); A is the color's alpha ---- */

/* Linear and Reflected: count the whole numbers k with k + 1/2 <= span (L - d) / L, d limited to 0 to L (for Reflected, its
   absolute value first). */
static unsigned
ref_along (long long x0, long long y0, long long x1, long long y1, unsigned span, long long x, long long y, int mirrored)
{
  __int128 dx = x1 - x0, dy = y1 - y0, L = dx * dx + dy * dy, d = (x - x0) * dx + (y - y0) * dy;
  unsigned k = 0;

  if (L == 0)
    return 0;
  if (mirrored && d < 0)
    d = -d;
  if (d < 0)
    d = 0;
  if (d > L)
    d = L;
  while (k < span && (__int128) (2 * k + 1) * L <= (__int128) 2 * span * (L - d))
    k++;
  return k;
}

/* Radial: floor (span (R - r) / R + 1/2) for r <= R, 0 beyond, with r and R as long double square roots. Where the value lies
   within 1e-9 of a whole number n (a half in the rounding), the exact equality 4 span^2 D = L (2 span - 2n + 1)^2 decides: equal
   is the tie, rounded up to n; otherwise the long double side is far enough from the tie to be right (the gap is at least
   1 / (2 sqrt (4 span^2 D)), about 1e-8 here, against a long double error of about 1e-16 of the value). */
static unsigned
ref_radial (long long x0, long long y0, long long x1, long long y1, unsigned span, long long x, long long y)
{
  long long dx = x1 - x0, dy = y1 - y0, px = x - x0, py = y - y0;
  __int128 L = (__int128) dx * dx + (__int128) dy * dy, D = (__int128) px * px + (__int128) py * py;
  long double R, r, v, n;

  if (L == 0 || span == 0)
    return 0;
  R = sqrtl ((long double) L);
  r = sqrtl ((long double) D);
  if (r > R)
    return 0;
  v = span * (R - r) / R + 0.5L;
  n = roundl (v);
  if (fabsl (v - n) < 1e-9L && n >= 1)
    {
      __int128 m = 2 * (__int128) span - 2 * (__int128) n + 1;

      if (4 * (__int128) span * span * D == L * m * m)
        return (unsigned) n;
    }
  return (unsigned) floorl (v);
}

static void
gradient_matches_the_formula (void)
{
  unsigned int seed = 12;

  for (int n = 0; n < 200000; n++)
    {
      int r = n % 4 == 0 ? 1000000 : n % 4 == 1 ? 2000 : 60;          /* small, middle and up to the coordinate limit */
      int c[6];
      unsigned alpha, want;
      EaselGradient g;

      for (int k = 0; k < 6; k++)
        {
          seed = seed * 1103515245u + 12345u;
          c[k] = (int) ((seed >> 4) % (unsigned) (2 * r + 1)) - r;
        }
      seed = seed * 1103515245u + 12345u;
      alpha = (seed >> 8) % 256;
      if (n % 3 == 0)
        alpha = 255;
      g = typed (c[0], c[1], c[2], c[3], (uint32_t) alpha << 24 | 0x123456u, (EaselGradientType) (n / 3 % 3));
      want = g.type == EASEL_GRADIENT_RADIAL ? ref_radial (c[0], c[1], c[2], c[3], alpha, c[4], c[5])
             : ref_along (c[0], c[1], c[2], c[3], alpha, c[4], c[5], g.type == EASEL_GRADIENT_REFLECTED);
      CHECK (easel_gradient_opacity (&g, c[4], c[5]) == want);
    }
}

/* ---- REQ-0012-05 again, in the test's own words: round (p / q) is the count of the whole numbers k with k + 1/2 <= p / q ---- */

static unsigned
ref_round (long long p, long long q)
{
  unsigned k = 0;

  while (k < 255 && (2 * k + 1) * q <= 2 * p)
    k++;
  return k;
}

static uint32_t
ref_blend (uint32_t picture, uint32_t color, unsigned a)
{
  long long da = picture >> 24, m = 255 * (long long) a + da * (255 - a);
  uint32_t out;

  if (a == 0)
    return picture;
  out = (uint32_t) ref_round (m, 255) << 24;
  for (int shift = 0; shift <= 16; shift += 8)
    {
      long long s = color >> shift & 0xFF, d = picture >> shift & 0xFF;

      out |= (uint32_t) ref_round (s * a * 255 + d * da * (255 - a), m) << shift;
    }
  return out;
}

static void
blend_matches_the_formula (void)
{
  unsigned int seed = 5;

  for (int n = 0; n < 100000; n++)
    {
      uint32_t picture, color;
      unsigned a;

      seed = seed * 1103515245u + 12345u;
      picture = seed ^ seed << 13;
      seed = seed * 1103515245u + 12345u;
      color = seed ^ seed >> 7;
      seed = seed * 1103515245u + 12345u;
      a = (seed >> 8) % 256;
      if (n % 3 == 0)                                   /* opaque, fully transparent and half-transparent picture pixels */
        picture |= 0xFF000000u;
      else if (n % 3 == 1)
        picture &= n % 2 ? 0x00FFFFFFu : 0x80FFFFFFu;
      if (n % 5 == 0)
        a = n % 10 ? 255 : 0;
      CHECK (easel_gradient_blend (picture, color, a) == ref_blend (picture, color, a));
    }
}

/* ---- 0032: Radial and Reflected ---- */

/* 0032 AS-2: Radial from (10, 5) to (15, 5), black over white: a disc, white beyond its circle. */
static void
as2_radial (void)
{
  static const unsigned row[13] = { 255, 255, 204, 153, 102, 51, 0, 51, 102, 153, 204, 255, 255 };     /* columns 4 to 16 */
  static const unsigned col[11] = { 255, 204, 153, 102, 51, 0, 51, 102, 153, 204, 255 };              /* rows 0 to 10 */
  EaselGradient g = typed (10, 5, 15, 5, BLACK, EASEL_GRADIENT_RADIAL);
  EaselImage *img = applied (&g, 21, 11, WHITE);

  for (int x = 4; x <= 16; x++)
    CHECK (img->px[5 * 21 + x] == GRAY (row[x - 4]));
  for (int y = 0; y <= 10; y++)
    CHECK (img->px[y * 21 + 10] == GRAY (col[y]));
  CHECK (img->px[8 * 21 + 13] == GRAY (216));
  CHECK (img->px[9 * 21 + 13] == WHITE && img->px[8 * 21 + 14] == WHITE);
  easel_image_free (img);
}

/* 0032 AS-3: Reflected from (10, 5) to (15, 5): the row of AS-2 in every row, column 7 gray 153, white past both lines. */
static void
as3_reflected (void)
{
  static const unsigned row[13] = { 255, 255, 204, 153, 102, 51, 0, 51, 102, 153, 204, 255, 255 };
  EaselGradient g = typed (10, 5, 15, 5, BLACK, EASEL_GRADIENT_REFLECTED);
  EaselImage *img = applied (&g, 21, 11, WHITE);

  for (int y = 0; y < 11; y++)
    {
      for (int x = 4; x <= 16; x++)
        CHECK (img->px[y * 21 + x] == GRAY (row[x - 4]));
      CHECK (img->px[y * 21 + 7] == GRAY (153));
      CHECK (img->px[y * 21 + 0] == WHITE && img->px[y * 21 + 20] == WHITE);
    }
  easel_image_free (img);
}

/* 0032 AS-4: Radial in black of alpha 128 over white, and the drags of one pixel. */
static void
as4_radial_alpha_and_small (void)
{
  static const unsigned want[6] = { 128, 102, 77, 51, 26, 0 };
  EaselGradient g = typed (10, 5, 15, 5, 0x80000000u, EASEL_GRADIENT_RADIAL);
  EaselImage *img = applied (&g, 21, 11, WHITE);

  for (int x = 10; x <= 15; x++)
    {
      CHECK (easel_gradient_opacity (&g, x, 5) == want[x - 10]);
      CHECK (img->px[5 * 21 + x] == GRAY (255 - want[x - 10]));
    }
  easel_image_free (img);
  g = typed (1, 1, 2, 1, BLACK, EASEL_GRADIENT_RADIAL);
  img = applied (&g, 4, 4, WHITE);
  CHECK (img->px[1 * 4 + 1] == BLACK && count (img, WHITE) == 15);
  easel_image_free (img);
  g = typed (1, 1, 2, 2, BLACK, EASEL_GRADIENT_RADIAL);
  img = applied (&g, 4, 4, WHITE);
  CHECK (img->px[1 * 4 + 1] == BLACK);
  CHECK (img->px[1 * 4 + 0] == GRAY (180) && img->px[1 * 4 + 2] == GRAY (180));
  CHECK (img->px[0 * 4 + 1] == GRAY (180) && img->px[2 * 4 + 1] == GRAY (180));
  CHECK (count (img, WHITE) == 11);
  easel_image_free (img);
}

/* 0032 AS-5: every S and E in an 11 x 11 picture, some alphas, and every alpha for three pairs, against the references; the
   Reflected opacity at 2S - p equals that at p; ends far away; and the pictures of AS-5. */
static void
as5_types_against_reference (void)
{
  static const unsigned alphas[] = { 0, 1, 127, 128, 254, 255 };
  static const int pairs[3][4] = { { 5, 5, 8, 9 }, { 0, 0, 10, 3 }, { 7, 2, 1, 10 } };

  for (int s = 0; s < 121; s++)
    for (int e = 0; e < 121; e++)
      if (s != e)
        for (size_t k = 0; k < sizeof alphas / sizeof alphas[0]; k++)
          {
            int sx = s % 11, sy = s / 11, ex = e % 11, ey = e / 11;
            uint32_t c = (uint32_t) alphas[k] << 24;
            EaselGradient r = typed (sx, sy, ex, ey, c, EASEL_GRADIENT_RADIAL);
            EaselGradient f = typed (sx, sy, ex, ey, c, EASEL_GRADIENT_REFLECTED);
            EaselGradient l = typed (sx, sy, ex, ey, c, EASEL_GRADIENT_LINEAR);

            for (int y = 0; y < 11; y++)
              for (int x = 0; x < 11; x++)
                {
                  int mx = 2 * sx - x, my = 2 * sy - y;

                  CHECK (easel_gradient_opacity (&r, x, y) == ref_radial (sx, sy, ex, ey, alphas[k], x, y));
                  CHECK (easel_gradient_opacity (&f, x, y) == ref_along (sx, sy, ex, ey, alphas[k], x, y, 1));
                  CHECK (easel_gradient_opacity (&l, x, y) == ref_along (sx, sy, ex, ey, alphas[k], x, y, 0));
                  if (mx >= 0 && mx < 11 && my >= 0 && my < 11)
                    CHECK (easel_gradient_opacity (&f, mx, my) == easel_gradient_opacity (&f, x, y));
                }
          }
  for (int p = 0; p < 3; p++)
    for (unsigned alpha = 0; alpha <= 255; alpha++)
      {
        EaselGradient r = typed (pairs[p][0], pairs[p][1], pairs[p][2], pairs[p][3], alpha << 24, EASEL_GRADIENT_RADIAL);

        for (int y = 0; y < 11; y++)
          for (int x = 0; x < 11; x++)
            CHECK (easel_gradient_opacity (&r, x, y)
                   == ref_radial (pairs[p][0], pairs[p][1], pairs[p][2], pairs[p][3], alpha, x, y));
      }
  {
    EaselGradient r = typed (10, 5, 100010, 5, BLACK, EASEL_GRADIENT_RADIAL);

    for (int x = -300; x <= 300; x += 7)
      for (int y = -50000; y <= 50000; y += 9973)
        CHECK (easel_gradient_opacity (&r, x, y) == ref_radial (10, 5, 100010, 5, 255, x, y));
    r = typed (-999999, 999999, 999999, -999999, BLACK, EASEL_GRADIENT_RADIAL);    /* the coordinate limits */
    for (int x = -1000000; x <= 1000000; x += 99991)
      CHECK (easel_gradient_opacity (&r, x, x / 3) == ref_radial (-999999, 999999, 999999, -999999, 255, x, x / 3));
  }
  {
    EaselGradient r = typed (-5, 5, -3, 5, BLACK, EASEL_GRADIENT_RADIAL);
    EaselGradient f = typed (10, 5, 40, 5, BLACK, EASEL_GRADIENT_REFLECTED);
    EaselImage *img = easel_image_new (21, 11, WHITE);

    CHECK (img != NULL && easel_gradient_apply (img, NULL, &r, NULL) == EASEL_GRADIENT_NOTHING);   /* the disc wholly outside */
    easel_image_free (img);
    img = applied (&f, 21, 11, WHITE);
    CHECK (img->px[5 * 21 + 0] == GRAY (85) && img->px[5 * 21 + 10] == BLACK && img->px[5 * 21 + 20] == GRAY (85));
    easel_image_free (img);
  }
}

/* 0032 AS-11: two gradients of each type stack: where they overlap the second lays over the first, elsewhere each is as alone;
   and the same drag applied again darkens, it does not replace. */
static void
as11_types_stack (void)
{
  for (int t = 0; t < 3; t++)
    {
      EaselGradient a = typed (6, 5, 12, 5, BLACK, (EaselGradientType) t), b = typed (14, 5, 8, 5, 0xFFFF0000u, (EaselGradientType) t);
      EaselImage *img = easel_image_new (21, 11, WHITE), *one = applied (&a, 21, 11, WHITE);

      CHECK (img != NULL);
      CHECK (easel_gradient_apply (img, NULL, &a, NULL) == EASEL_GRADIENT_DONE);
      CHECK (easel_gradient_apply (img, NULL, &b, NULL) == EASEL_GRADIENT_DONE);
      for (int y = 0; y < 11; y++)
        for (int x = 0; x < 21; x++)
          {
            uint32_t first = easel_gradient_blend (WHITE, BLACK, easel_gradient_opacity (&a, x, y));

            CHECK (one->px[y * 21 + x] == first);
            CHECK (img->px[y * 21 + x] == easel_gradient_blend (first, 0xFFFF0000u, easel_gradient_opacity (&b, x, y)));
          }
      CHECK (img->px[5 * 21 + 10] != one->px[5 * 21 + 10] && img->px[5 * 21 + 10] != WHITE);    /* both show in the overlap */
      CHECK (easel_gradient_apply (one, NULL, &a, NULL) == EASEL_GRADIENT_DONE);                   /* again: darker */
      CHECK (one->px[5 * 21 + 8] < easel_gradient_blend (WHITE, BLACK, easel_gradient_opacity (&a, 8, 5)));
      easel_image_free (one);
      easel_image_free (img);
    }
}

/* 0032 AS-12 (no grain): on a long drag the opacity never rises going away from the start color's place, in steps of at most
   one, for every type; along a ray from S for Radial, along the drag for Linear, outward on both sides for Reflected. */
static void
as12_no_grain (void)
{
  EaselGradient r = typed (700, 400, 1900, 1300, BLACK, EASEL_GRADIENT_RADIAL);
  EaselGradient l = typed (100, 300, 1700, 300, BLACK, EASEL_GRADIENT_LINEAR);
  EaselGradient f = typed (900, 300, 1700, 300, BLACK, EASEL_GRADIENT_REFLECTED);

  for (int k = 0; k < 64; k++)
    {
      double a = k * 2 * 3.14159265358979 / 64;
      unsigned prev = 255;

      for (int s = 0; s <= 1600; s++)
        {
          unsigned o = easel_gradient_opacity (&r, 700 + (int) lround (s * cos (a)), 400 + (int) lround (s * sin (a)));

          CHECK (o <= prev && prev - o <= 1);
          prev = o;
        }
      CHECK (prev == 0);
    }
  for (int y = 0; y < 600; y += 37)
    {
      unsigned prev = 255, up = 255, down = 255;

      for (int x = 0; x <= 1800; x++)
        {
          unsigned o = easel_gradient_opacity (&l, x, y);

          CHECK (o <= prev && prev - o <= 1);
          prev = o;
        }
      for (int s = 0; s <= 900; s++)
        {
          unsigned o = easel_gradient_opacity (&f, 900 + s, y), m = easel_gradient_opacity (&f, 900 - s, y);

          CHECK (o == m && o <= up && up - o <= 1 && m <= down);
          up = o, down = m;
        }
    }
}

int
main (void)
{
  RUN (as2_gradient_pixels);
  RUN (as3_shift_gradient_pixels);
  RUN (as4_equal_opacity_lines);
  RUN (as6_transparent_color);
  RUN (as8_secondary_color);
  RUN (as9_gradient_from_outside);
  RUN (as18_one_pixel_drags);
  RUN (gradient_matches_the_formula);
  RUN (as5_blend_over_transparent);
  RUN (as7_second_gradient);
  RUN (blend_matches_the_formula);
  RUN (lay_is_the_apply);
  RUN (lay_clips_its_rectangle);
  RUN (as13_gradient_changing_nothing);
  RUN (as14_gradient_memory_and_budget);
  RUN (as14_step_holds_what_it_counts);
  RUN (as2_radial);
  RUN (as3_reflected);
  RUN (as4_radial_alpha_and_small);
  RUN (as5_types_against_reference);
  RUN (as11_types_stack);
  RUN (as12_no_grain);
  return 0;
}
