/* SPDX-License-Identifier: GPL-3.0-or-later */
/* The gradient of feature 0012: its opacity at a pixel (REQ-0012-04), its blend (REQ-0012-05), its apply (REQ-0012-09). */

#include <malloc.h>
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

static EaselGradient
gradient (int x0, int y0, int x1, int y1, uint32_t color)
{
  return (EaselGradient) { x0, y0, x1, y1, color };
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

/* REQ-0012-07: the preview and the apply lay the gradient over the pixels with one function. Laid over any rectangle of the
   picture, each pixel is the blend of the picture's pixel at its opacity; laid over the whole picture, it is what the apply
   leaves; pixels of opacity 0 are copied as they are. */
static void
lay_is_the_apply (void)
{
  enum { W = 37, H = 23 };
  EaselImage *img = easel_image_new (W, H, WHITE), *applied;
  EaselGradient g = gradient (30, -4, 3, 19, 0x80336699u);
  uint32_t buf[W * H], part[11 * 7];

  CHECK (img != NULL);
  for (int i = 0; i < W * H; i++)                     /* opaque, transparent and half-transparent pixels */
    img->px[i] = i % 3 == 0 ? 0xFFFFFF00u : i % 3 == 1 ? 0x00000000u : 0x80FFFFFFu;
  applied = easel_image_new (W, H, WHITE);
  CHECK (applied != NULL);
  for (int i = 0; i < W * H; i++)
    applied->px[i] = img->px[i];
  CHECK (easel_gradient_apply (applied, NULL, &g, NULL) == EASEL_GRADIENT_DONE);
  easel_gradient_lay (&g, img, 0, 0, W, H, buf, W);
  for (int i = 0; i < W * H; i++)
    CHECK (buf[i] == applied->px[i]);
  /* A rectangle away from the corner, written with its own stride. */
  easel_gradient_lay (&g, img, 20, 9, 11, 7, part, 11);
  for (int y = 0; y < 7; y++)
    for (int x = 0; x < 11; x++)
      {
        uint32_t src = img->px[(9 + y) * W + 20 + x];

        CHECK (part[y * 11 + x] == easel_gradient_blend (src, g.color, easel_gradient_opacity (&g, 20 + x, 9 + y)));
      }
  CHECK (easel_gradient_opacity (&g, 0, 22) == 0 && buf[22 * W] == img->px[22 * W]);    /* beyond the end: copied */
  easel_image_free (applied);
  easel_image_free (img);
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
      EaselGradient g = gradient (5, 5, 15, 5, colors[k]);

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

/* ---- REQ-0012-04 again, in the test's own words: count the whole numbers k with k + 1/2 <= A (L - d) / L ---- */

static unsigned
ref_opacity (long long x0, long long y0, long long x1, long long y1, unsigned alpha, long long x, long long y)
{
  __int128 dx = x1 - x0, dy = y1 - y0, L = dx * dx + dy * dy, d = (x - x0) * dx + (y - y0) * dy;
  unsigned k = 0;

  if (L == 0)
    return 0;
  if (d < 0)
    d = 0;
  if (d > L)
    d = L;
  while (k < 255 && (__int128) (2 * k + 1) * L <= (__int128) 2 * alpha * (L - d))
    k++;
  return k;
}

static void
gradient_matches_the_formula (void)
{
  unsigned int seed = 12;

  for (int n = 0; n < 200000; n++)
    {
      int r = n % 4 == 0 ? 1000000 : n % 4 == 1 ? 2000 : 60;          /* small, middle and up to the coordinate limit */
      int c[6];
      unsigned alpha;
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
      g = gradient (c[0], c[1], c[2], c[3], (uint32_t) alpha << 24 | 0x123456u);
      CHECK (easel_gradient_opacity (&g, c[4], c[5]) == ref_opacity (c[0], c[1], c[2], c[3], alpha, c[4], c[5]));
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
  return 0;
}
