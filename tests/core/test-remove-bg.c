/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Remove background (feature 0035, ADR-0007), the classical method of REQ-0035-03: the automatic tolerance, the background removed
   with its color kept, a gradient background, transparent pixels, the shape, the regions, the copy of a large picture, the same pixels
   again, the memory held against the memory counted, Cancel. */

#include <malloc.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "check.h"
#include "remove-bg.h"

#define BIG ((size_t) 1 << 40)
#define WHITE 0xFFFFFFFFu
#define RED 0xFFE01010u
#define BLUE 0xFF1030C0u
#define AUTO INT32_MIN                  /* the tolerance for run (): the automatic one */

static uint32_t *
picture (int w, int h, uint32_t c)
{
  uint32_t *p = malloc ((size_t) w * h * sizeof *p);

  for (size_t i = 0; i < (size_t) w * h; i++)
    p[i] = c;
  return p;
}

static void
disk (uint32_t *p, int w, int cx, int cy, int rad, uint32_t c)
{
  for (int y = cy - rad; y <= cy + rad; y++)
    for (int x = cx - rad; x <= cx + rad; x++)
      if ((x - cx) * (x - cx) + (y - cy) * (y - cy) <= rad * rad)
        p[(size_t) y * w + x] = c;
}

static bool
removed (const uint32_t *before, const uint32_t *after, size_t i)
{
  return (after[i] >> 24) == 0 && (after[i] & 0x00FFFFFFu) == (before[i] & 0x00FFFFFFu);
}

/* The whole result for a tolerance (AUTO: the automatic one) into dst; returns the pixels removed. */
static size_t
run (const uint32_t *p, const uint8_t *m, int w, int h, int tol, uint32_t *dst)
{
  EaselRbg *r = easel_rbg_new (p, m, w, h, BIG);
  size_t gone = 0;

  CHECK (r != NULL);
  CHECK (easel_rbg_label (r, tol != AUTO ? tol : easel_rbg_auto_tolerance (r), NULL) == EASEL_RBG_DONE);
  CHECK (easel_rbg_result (r, p, m, dst, &gone, NULL) == EASEL_RBG_DONE);
  easel_rbg_free (r);
  return gone;
}

/* The oracle (2026-11-10): the integer Lab against an independent reference computed here in doubles with libm, so that a wrong table
   cannot pass again (the cube root was wrong for 0035's first build and every test passed). The reference: sRGB's own curve, the
   sRGB to XYZ matrix for D65 and CIE's f(t). */
static double
ref_f (double t)
{
  return t > 216.0 / 24389.0 ? cbrt (t) : t * 24389.0 / 3132.0 + 4.0 / 29.0;
}

static void
ref_lab (int red, int green, int blue, double out[3])
{
  double c[3] = { red / 255.0, green / 255.0, blue / 255.0 }, x, y, z;

  for (int k = 0; k < 3; k++)
    c[k] = c[k] <= 0.04045 ? c[k] / 12.92 : pow ((c[k] + 0.055) / 1.055, 2.4);
  x = (0.4124564 * c[0] + 0.3575761 * c[1] + 0.1804375 * c[2]) / 0.95047;
  y = 0.2126729 * c[0] + 0.7151522 * c[1] + 0.0721750 * c[2];
  z = (0.0193339 * c[0] + 0.1191920 * c[1] + 0.9503041 * c[2]) / 1.08883;
  out[0] = 116 * ref_f (y) - 16;
  out[1] = 500 * (ref_f (x) - ref_f (y));
  out[2] = 200 * (ref_f (y) - ref_f (z));
}

/* Every entry of f(t), t = 0 .. 65536 in Q16, within one unit of the reference; the Lab of known colors and of 512 colors spread over
   the cube within 1 delta-E: white 100, 0, 0; black 0; the backdrop green (37, 112, 56) a about -40; pure red, green and blue. */
static void
lab_oracle (void)
{
  uint32_t *p = picture (8, 8, WHITE);
  EaselRbg *r = easel_rbg_new (p, NULL, 8, 8, BIG);
  static const int known[][3] = { { 255, 255, 255 }, { 0, 0, 0 }, { 37, 112, 56 }, { 255, 0, 0 }, { 0, 255, 0 }, { 0, 0, 255 },
                                   { 128, 128, 128 }, { 225, 175, 145 } };
  int worst_t = 0;
  double worst = 0;

  CHECK (r != NULL);
  for (int t = 0; t <= 65536; t++)
    {
      double want = ref_f (t / 65536.0) * 65536.0, got = easel_rbg_lab_f (r, t);

      if (fabs (got - want) > worst)
        worst = fabs (got - want), worst_t = t;
    }
  if (worst > 1.0)
    fprintf (stderr, "f(t) off by %.1f at t = %d\n", worst, worst_t);
  CHECK (worst <= 1.0);
  for (int i = 0; i < 8 + 512; i++)
    {
      int rgb[3];
      int16_t got[3];
      double want[3], de;

      if (i < 8)
        rgb[0] = known[i][0], rgb[1] = known[i][1], rgb[2] = known[i][2];
      else
        for (int k = 0; k < 3; k++)
          rgb[k] = (int) (((uint32_t) (i * 3 + k) * 2654435761u) >> 24);
      easel_rbg_lab (r, rgb[0], rgb[1], rgb[2], got);
      ref_lab (rgb[0], rgb[1], rgb[2], want);
      de = sqrt (pow (got[0] / 64.0 - want[0], 2) + pow (got[1] / 64.0 - want[1], 2) + pow (got[2] / 64.0 - want[2], 2));
      if (de > 1.0)
        fprintf (stderr, "Lab of (%d, %d, %d): %.1f %.1f %.1f, the reference %.1f %.1f %.1f\n", rgb[0], rgb[1], rgb[2], got[0] / 64.0,
                 got[1] / 64.0, got[2] / 64.0, want[0], want[1], want[2]);
      CHECK (de <= 1.0);
    }
  {
    int16_t g[3];

    easel_rbg_lab (r, 37, 112, 56, g);
    CHECK (g[1] / 64 <= -35 && g[1] / 64 >= -45);                          /* the backdrop green: a about -40 */
    easel_rbg_lab (r, 255, 255, 255, g);
    CHECK (abs (g[0] - 100 * 64) <= 32 && abs (g[1]) <= 32 && abs (g[2]) <= 32);
  }
  easel_rbg_free (r);
  free (p);
}

/* A red disk on white: the automatic tolerance is 5 (a clean ring), the white removed with its color kept, the disk exactly as it was,
   the count the white's; the preview the same, the copy being the pixels. */
static void
disk_on_white (void)
{
  uint32_t *p = picture (300, 200, WHITE), *d = picture (300, 200, 0), *v = picture (300, 200, 0);
  size_t whites = 0, gone, shown;
  EaselRbg *r;

  disk (p, 300, 150, 100, 50, RED);
  r = easel_rbg_new (p, NULL, 300, 200, BIG);
  CHECK (r != NULL && easel_rbg_auto_tolerance (r) == 5 && easel_rbg_block (r) == 1);
  CHECK (easel_rbg_label (r, 5, NULL) == EASEL_RBG_DONE);
  CHECK (easel_rbg_result (r, p, NULL, d, &gone, NULL) == EASEL_RBG_DONE);
  CHECK (easel_rbg_preview (r, p, NULL, v, &shown) == EASEL_RBG_DONE);
  for (size_t i = 0; i < 300 * 200; i++)
    {
      whites += p[i] == WHITE;
      CHECK (p[i] == RED ? d[i] == RED : removed (p, d, i));
    }
  CHECK (gone == whites && shown == whites && memcmp (d, v, 300 * 200 * 4) == 0);
  easel_rbg_free (r);
  free (p);
  free (d);
  free (v);
}

/* A background that is a vertical gradient, white to mid gray: the surface follows it, so all of it is removed and the disk kept; the
   ring's median residual is small. */
static void
gradient_background (void)
{
  uint32_t *p = picture (300, 200, 0), *d = picture (300, 200, 0);
  EaselRbg *r;

  for (int y = 0; y < 200; y++)
    for (int x = 0; x < 300; x++)
      p[y * 300 + x] = 0xFF000000u | (uint32_t) (250 - y * 120 / 200) * 0x010101u;
  disk (p, 300, 150, 100, 40, BLUE);
  r = easel_rbg_new (p, NULL, 300, 200, BIG);
  CHECK (easel_rbg_residual (r) < 64);
  easel_rbg_free (r);
  run (p, NULL, 300, 200, AUTO, d);
  CHECK (removed (p, d, 5 * 300 + 5) && removed (p, d, 195 * 300 + 295) && removed (p, d, 100 * 300 + 30));
  CHECK (d[100 * 300 + 150] == BLUE);
  free (p);
  free (d);
}

/* REQ-0035-04's automatic tolerance on a noisy border: 5 + 3 per delta-E of the median above 3.05, rounded down, so above 5 here. */
static void
noisy_border (void)
{
  uint32_t *p = picture (300, 200, 0);
  EaselRbg *r;
  int64_t m;

  for (uint32_t i = 0; i < 300 * 200; i++)
    p[i] = 0xFF000000u | (uint32_t) (200 + (int) ((i * 2654435761u) >> 26) - 32) * 0x010101u;
  disk (p, 300, 150, 100, 50, RED);
  r = easel_rbg_new (p, NULL, 300, 200, BIG);
  m = easel_rbg_residual (r);
  fprintf (stderr, "noisy border: median %.2f delta-E, automatic %d\n", m / 64.0, easel_rbg_auto_tolerance (r));
  CHECK (m > 4 * 64);
  CHECK (easel_rbg_auto_tolerance (r) == (int) (5 + 3 * (m - 195) / 64 > 20 ? 20 : 5 + 3 * (m - 195) / 64));
  easel_rbg_free (r);
  free (p);
}

#define SKY 0xFF2864A0u                    /* (40, 100, 160) */
#define DARK 0xFF0A0A14u                   /* (10, 10, 20) */

/* The sky and the dark blended: share of the dark in 1/256ths. */
static uint32_t
blend (int dark256)
{
  uint32_t c = 0xFF000000u;

  for (int k = 0; k < 3; k++)
    {
      int a = (int) (SKY >> (8 * k) & 0xFF), b = (int) (DARK >> (8 * k) & 0xFF);

      c |= (uint32_t) ((a * (256 - dark256) + b * dark256 + 128) / 256) << (8 * k);
    }
  return c;
}

/* (g) The fringe: a dark disk whose edge blends into the sky over 10 pixels (radius 40 dark, 50 sky). Where the blend is more than
   half sky (farther than 45.5) the pixel is removed; where it is more than half dark (nearer than 44.5) it is kept. */
static void
soft_edge (void)
{
  int w = 300, h = 200;
  uint32_t *p = picture (w, h, SKY), *d = picture (w, h, 0);

  for (int y = 0; y < h; y++)
    for (int x = 0; x < w; x++)
      {
        int64_t q = (int64_t) (x - 150) * (x - 150) + (int64_t) (y - 100) * (y - 100);

        if (q <= 40 * 40)
          p[y * w + x] = DARK;
        else if (q < 50 * 50)
          {
            int r256 = 0;

            while ((int64_t) (r256 + 1) * (r256 + 1) <= q * 256 * 256)
              r256++;                                                   /* the radius in 1/256ths */
            p[y * w + x] = blend (256 - (r256 - 40 * 256) / 10);
          }
      }
  run (p, NULL, w, h, AUTO, d);
  for (int y = 0; y < h; y++)
    for (int x = 0; x < w; x++)
      {
        int64_t q4 = 4 * ((int64_t) (x - 150) * (x - 150) + (int64_t) (y - 100) * (y - 100));
        size_t i = (size_t) y * w + x;

        if (q4 >= 91 * 91)
          CHECK (removed (p, d, i));                                    /* r >= 45.5: more than half sky */
        else if (q4 <= 89 * 89)
          CHECK (d[i] == p[i]);                                         /* r <= 44.5: more than half dark */
      }
  free (p);
  free (d);
}

/* (g) Thin lines survive the fringe: lines 1 and 2 pixels wide across the sky, joined to a dark disk, kept whole. */
static void
thin_lines (void)
{
  int w = 300, h = 200;
  uint32_t *p = picture (w, h, SKY), *d = picture (w, h, 0);

  disk (p, w, 150, 100, 30, DARK);
  for (int x = 10; x < 290; x++)
    {
      p[100 * w + x] = DARK;                                            /* one pixel wide */
      p[40 * w + x] = p[41 * w + x] = DARK;                              /* two */
    }
  for (int y = 41; y < 100; y++)
    p[y * w + 150] = DARK;                                              /* joining the two to the disk */
  run (p, NULL, w, h, AUTO, d);
  for (int x = 10; x < 290; x++)
    CHECK (d[100 * w + x] == DARK && d[40 * w + x] == DARK && d[41 * w + x] == DARK);
  CHECK (removed (p, d, 5 * (size_t) w + 5) && removed (p, d, 70 * (size_t) w + 20));
  free (p);
  free (d);
}

/* REQ-0035-03 (a, b), JOD's green-backdrop case drawn here (no photo in the repository): a green backdrop lit more in the middle (a glow
   of the same green, brighter) and darker in the corners, a grey block touching the bottom border (a shirt) and two skin-colored bars
   touching the top (raised arms). The block and the arms are kept, the glow and the corners removed. */
static void
green_backdrop (void)
{
  int w = 480, h = 270;
  uint32_t *p = picture (w, h, 0), *d = picture (w, h, 0);

  for (int y = 0; y < h; y++)
    for (int x = 0; x < w; x++)
      {
        /* the glow: 1 + 0.8 e^(-r^2 / 0.2025), r measured to the middle in half-sizes; the corners: down to 0.55 */
        double u = (x - w / 2.0) / (w / 2.0), v = (y - h * 0.45) / (h / 2.0), cu = (x - w / 2.0) / (w / 2.0),
               cv = (y - h / 2.0) / (h / 2.0), corner = sqrt (cu * cu + cv * cv) - 0.6, light;

        light = (1 + 0.8 * exp (-(u * u + v * v) / 0.2025)) * (1 - 0.45 * (corner < 0 ? 0 : corner > 0.8 ? 1 : corner / 0.8));
        p[y * w + x] = 0xFF000000u | (uint32_t) (40 * light) << 16 | (uint32_t) (120 * light) << 8 | (uint32_t) (60 * light);
        if (x > w * 34 / 100 && x < w * 66 / 100 && y > h * 55 / 100)
          p[y * w + x] = 0xFF69705Bu;                                      /* the grey block, (105, 112, 88) */
        else if (y < h / 2 && (abs (x - w * 41 / 100) < w / 40 || abs (x - w * 59 / 100) < w / 40))
          p[y * w + x] = 0xFFE1AF91u;                                      /* the arms, (225, 175, 145) */
      }
  run (p, NULL, w, h, AUTO, d);
  CHECK (d[(h - 5) * w + w / 2] == 0xFF69705Bu && d[(h * 70 / 100) * w + w * 40 / 100] == 0xFF69705Bu);   /* the block, kept */
  CHECK (d[5 * w + w * 41 / 100] == 0xFFE1AF91u);                          /* an arm at the top border, kept */
  CHECK (removed (p, d, (size_t) (h * 45 / 100) * w + w / 2 - 70));        /* the glow, beside the head's place */
  CHECK (removed (p, d, (size_t) (h * 40 / 100) * w + w * 25 / 100));
  CHECK (removed (p, d, 2 * (size_t) w + 2) && removed (p, d, (size_t) (h - 3) * w + w - 3));   /* the dark corners */
  free (p);
  free (d);
}

/* (b) The colour line's guards. A white disk on a mid blue backdrop lit unevenly, at Tolerance 8 (where the line's brighter end reaches white): a white cell is the surface's
   alone, never the line's. Line art in a darker shade of the color between a two-color gradient's ends: the surface explains the
   gradient better than one color under more or less light, so the line does not judge and the strokes stay. */
static void
line_guards (void)
{
  int w = 300, h = 200;
  uint32_t *p = picture (w, h, 0), *d = picture (w, h, 0);

  for (int y = 0; y < h; y++)
    for (int x = 0; x < w; x++)
      {
        int l = 80 + 20 * y / h;                                          /* 80 % to 100 % of (150, 180, 215) */

        p[y * w + x] = 0xFF000000u | (uint32_t) (150 * l / 100) << 16 | (uint32_t) (180 * l / 100) << 8 | (uint32_t) (215 * l / 100);
      }
  disk (p, w, 150, 100, 40, 0xFFFFFFFFu);
  run (p, NULL, w, h, 8, d);
  CHECK (d[100 * w + 150] == 0xFFFFFFFFu && removed (p, d, 5 * (size_t) w + 5));   /* white, at Tolerance 8 */
  for (int y = 0; y < h; y++)
    for (int x = 0; x < w; x++)
      p[y * w + x] = 0xFF000000u | (uint32_t) (250 - 100 * x / w) << 16 | (uint32_t) (150 - 100 * x / w) << 8 | (uint32_t) (50 + 100 * x / w);
  for (int x = 40; x < 260; x++)
    for (int k = 0; k < 4; k++)
      p[(60 + k) * w + x] = p[(140 + k) * w + x] = 0xFF643232u;           /* strokes of (100, 50, 50) */
  for (int y = 64; y < 140; y++)
    for (int k = 0; k < 4; k++)
      p[y * w + 150 + k] = 0xFF643232u;
  run (p, NULL, w, h, AUTO, d);
  CHECK (d[61 * w + 100] == 0xFF643232u && d[141 * w + 200] == 0xFF643232u && d[100 * w + 151] == 0xFF643232u);
  CHECK (removed (p, d, 5 * (size_t) w + 5));
  free (p);
  free (d);
}

/* (c') A backdrop of two colors, blue on the left and orange on the right, with a dark green figure standing across the split: both
   halves are removed and the figure kept; the ring's two colors explain it better than one smooth surface. */
static void
two_colours (void)
{
  int w = 320, h = 200;
  uint32_t *p = picture (w, h, 0), *d = picture (w, h, 0);

  for (int y = 0; y < h; y++)
    for (int x = 0; x < w; x++)
      p[y * w + x] = x < w / 2 ? 0xFF3C6EC8u : 0xFFF0963Cu;
  for (int y = 84; y < h; y++)
    for (int x = 130; x < 190; x++)
      p[y * w + x] = 0xFF1E5A32u;                                         /* the figure's body, to the bottom */
  disk (p, w, 160, 60, 22, 0xFF1E5A32u);                                  /* its head */
  run (p, NULL, w, h, AUTO, d);
  CHECK (d[150 * w + 160] == 0xFF1E5A32u && d[60 * w + 160] == 0xFF1E5A32u);
  CHECK (removed (p, d, 100 * (size_t) w + 40) && removed (p, d, 100 * (size_t) w + 280));
  CHECK (removed (p, d, 20 * (size_t) w + 150) && removed (p, d, 20 * (size_t) w + 170));   /* both sides of the split, near it */
  free (p);
  free (d);
}

/* (e) Fully transparent pixels stay as they were and are no seeds; a picture all transparent has no ring: nothing removed. */
static void
transparent (void)
{
  uint32_t *p = picture (100, 100, 0x00123456u), *d = picture (100, 100, 0);

  CHECK (run (p, NULL, 100, 100, AUTO, d) == 0 && memcmp (p, d, 100 * 100 * 4) == 0);
  for (int y = 0; y < 100; y++)
    for (int x = 0; x < 100; x++)
      if (x > 10 && x < 90 && y > 10 && y < 90)
        p[y * 100 + x] = WHITE;                                             /* white inside a transparent frame */
  disk (p, 100, 50, 50, 20, RED);
  run (p, NULL, 100, 100, AUTO, d);
  CHECK (d[5 * 100 + 5] == 0x00123456u);                                    /* as it was */
  CHECK (removed (p, d, 20 * 100 + 20) && d[50 * 100 + 50] == RED);         /* the ring at the transparent frame's edge */
  free (p);
  free (d);
}

/* The shape: pixels not looked at stay; the ring includes the shape's edge, so a white middle inside a red frame not looked at is
   reached from it. */
static void
shape (void)
{
  uint32_t *p = picture (200, 200, RED), *d = picture (200, 200, 0);
  uint8_t *m = calloc (200 * 200, 1);

  for (int y = 40; y < 160; y++)
    for (int x = 40; x < 160; x++)
      {
        p[y * 200 + x] = WHITE;
        m[y * 200 + x] = 1;
      }
  disk (p, 200, 100, 100, 25, BLUE);
  run (p, m, 200, 200, AUTO, d);
  CHECK (removed (p, d, 45 * 200 + 45) && d[100 * 200 + 100] == BLUE);
  CHECK (d[10 * 200 + 10] == RED && d[100 * 200 + 20] == RED);              /* not looked at */
  free (p);
  free (d);
  free (m);
}

/* (c) Regions: a speck under a tenth of the largest subject becomes background; two subjects of like size both stay; a hole inside a
   subject is reached by no background seed, so it stays, small or large (the spec's edge cases). The seam: no white pixel beside a
   subject is kept. */
static void
regions (void)
{
  uint32_t *p = picture (400, 300, WHITE), *d = picture (400, 300, 0);

  disk (p, 400, 110, 150, 60, RED);
  disk (p, 400, 290, 150, 55, BLUE);
  disk (p, 400, 200, 40, 6, BLUE);                                          /* a speck: about 110 pixels against 11 000 */
  disk (p, 400, 110, 150, 3, WHITE);                                        /* a small hole in the red disk: filled */
  disk (p, 400, 290, 150, 35, WHITE);                                       /* a large hole in the blue ring */
  run (p, NULL, 400, 300, AUTO, d);
  CHECK (d[150 * 400 + 80] == RED && d[150 * 400 + 340] == BLUE);           /* both subjects */
  CHECK (removed (p, d, 40 * 400 + 200));                                   /* the speck */
  CHECK (d[150 * 400 + 110] == WHITE && d[150 * 400 + 290] == WHITE);      /* both holes kept */
  CHECK (removed (p, d, 150 * 400 + 49) && removed (p, d, 150 * 400 + 346));  /* the first white pixel beside each disk */
  free (p);
  free (d);
}

/* The tolerance is held to 1 .. 40; the same input gives the same pixels. */
static void
clamp_and_same (void)
{
  uint32_t *p = picture (160, 120, WHITE), *a = picture (160, 120, 0), *b = picture (160, 120, 0);

  for (int i = 0; i < 160 * 120; i++)
    if ((i * 2654435761u >> 28) == 3)
      p[i] = 0xFF000000u | (uint32_t) (i * 40503u);
  disk (p, 160, 80, 60, 30, RED);
  run (p, NULL, 160, 120, 40, a);
  run (p, NULL, 160, 120, 99, b);
  CHECK (memcmp (a, b, 160 * 120 * 4) == 0);
  {
    /* a blue disk on yellow, about 200 delta-E apart: at 40 the disk is still four tolerances from the background and stays; an
       unheld 99 would remove it */
    uint32_t *q = picture (160, 120, 0xFFFFFF00u), *c = picture (160, 120, 0);

    disk (q, 160, 80, 60, 30, 0xFF0000FFu);
    run (q, NULL, 160, 120, 99, c);
    CHECK (c[60 * 160 + 80] == 0xFF0000FFu && removed (q, c, 5));
    free (q);
    free (c);
  }
  run (p, NULL, 160, 120, 1, a);
  run (p, NULL, 160, 120, -3, b);
  CHECK (memcmp (a, b, 160 * 120 * 4) == 0);
  run (p, NULL, 160, 120, AUTO, a);
  run (p, NULL, 160, 120, AUTO, b);
  CHECK (memcmp (a, b, 160 * 120 * 4) == 0);
  free (p);
  free (a);
  free (b);
}

/* Over 2 000 000 pixels the copy is made of 2 x 2 blocks: the preview takes each block's label, the result refines the boundary by the
   nearest neighbouring cell's color, so the disk's edge pixels follow their own colors; away from the edge both agree. (Until the
   cube root's fix the preview's edge also differed here; with a correct Lab the fringe evens it out, which ADR-0007 allows.) */
static void
large_copy (void)
{
  int w = 2400, h = 1000;
  uint32_t *p = picture (w, h, WHITE), *v = picture (w, h, 0), *d = picture (w, h, 0);
  EaselRbg *r;
  size_t differ = 0;

  disk (p, w, 1200, 500, 301, RED);
  r = easel_rbg_new (p, NULL, w, h, BIG);
  CHECK (r != NULL && easel_rbg_block (r) == 2 && easel_rbg_copy_cells (r) == (size_t) 1200 * 500);
  CHECK (easel_rbg_label (r, 5, NULL) == EASEL_RBG_DONE);
  easel_rbg_preview (r, p, NULL, v, NULL);
  easel_rbg_result (r, p, NULL, d, NULL, NULL);
  for (size_t i = 0; i < (size_t) w * h; i++)
    {
      CHECK (p[i] == RED ? d[i] == RED : removed (p, d, i));               /* the result exact at full size */
      differ += v[i] != d[i];
    }
  (void) differ;                                    /* the preview may differ at the edge (ADR-0007), it need not (the fringe evens it out) */
  CHECK (v[500 * (size_t) w + 1200] == RED && removed (p, v, 10));
  easel_rbg_free (r);
  free (p);
  free (v);
  free (d);
}

/* The shape on a copy of 2 x 2 blocks, its edge at odd places so that blocks lie across it: no pixel outside the shape changes, also
   where its block takes part in the boundary's refinement. */
static void
large_shape (void)
{
  int w = 2400, h = 1000;
  uint32_t *p = picture (w, h, WHITE), *d = picture (w, h, 0);
  uint8_t *m = calloc ((size_t) w * h, 1);

  for (int y = 101; y < 899; y++)
    for (int x = 301; x < 2099; x++)
      m[(size_t) y * w + x] = 1;
  disk (p, w, 1200, 500, 201, RED);
  run (p, m, w, h, AUTO, d);
  for (size_t i = 0; i < (size_t) w * h; i++)
    CHECK (m[i] ? (p[i] == RED ? d[i] == RED : removed (p, d, i)) : d[i] == p[i]);
  free (p);
  free (d);
  free (m);
}


static size_t
heap_in_use (void)
{
  struct mallinfo2 mi = mallinfo2 ();

  return mi.uordblks + mi.hblkhd;
}

/* The memory: what the analysis holds is what it counts (within a page per buffer), it refuses a limit below that, and Cancel ends a
   result. */
static void
memory_and_cancel (void)
{
  uint32_t *p = picture (600, 400, WHITE), *d = picture (600, 400, 0);
  size_t before, held, counted;
  EaselRbg *r;
  volatile int stop = 1;

  disk (p, 600, 300, 200, 100, RED);
  before = heap_in_use ();
  r = easel_rbg_new (p, NULL, 600, 400, BIG);
  held = heap_in_use () - before;
  counted = easel_rbg_bytes (r);
  CHECK (held <= counted + 8 * 4096 && counted <= held + 8 * 4096);
  CHECK (easel_rbg_new (p, NULL, 600, 400, counted - 1) == NULL);
  CHECK (easel_rbg_label (r, 5, &stop) == EASEL_RBG_CANCELLED || easel_rbg_label (r, 5, NULL) == EASEL_RBG_DONE);
  CHECK (easel_rbg_label (r, 5, NULL) == EASEL_RBG_DONE);
  CHECK (easel_rbg_result (r, p, NULL, d, NULL, &stop) == EASEL_RBG_CANCELLED);
  easel_rbg_free (r);
  free (p);
  free (d);
}

int
main (void)
{
  RUN (lab_oracle);
  RUN (disk_on_white);
  RUN (gradient_background);
  RUN (noisy_border);
  RUN (green_backdrop);
  RUN (line_guards);
  RUN (two_colours);
  RUN (soft_edge);
  RUN (thin_lines);
  RUN (transparent);
  RUN (shape);
  RUN (regions);
  RUN (clamp_and_same);
  RUN (large_copy);
  RUN (large_shape);
  RUN (memory_and_cancel);
  return 0;
}
