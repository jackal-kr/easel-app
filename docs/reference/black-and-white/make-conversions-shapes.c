/* SPDX-License-Identifier: GPL-3.0-or-later */
/* 0034 reference (2026-11-11): a synthetic 320 x 200 picture (a gray ramp, sky, grass, a sun, an ellipse, a triangle, a square) and its
   three conversions by the app's own easel_bw_convert, written as four PPM files for make-conversions-shapes.py. Synthetic only, so the
   sheet can be in the repository (the photo half of the earlier sheet is kept locally). Built by make-conversions-shapes.py. */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "black-white.h"

enum { W = 320, H = 200 };

static uint32_t
pixel (int x, int y)
{
  double dx, dy;

  if (y < 40)
    {
      unsigned v = (unsigned) (x * 255 / (W - 1));

      return 0xFF000000u | v << 16 | v << 8 | v;                         /* the ramp, black to white */
    }
  dx = x - 260.0, dy = y - 80.0;
  if (dx * dx + dy * dy <= 30.0 * 30.0)
    return 0xFFF8D800u;                                                  /* the sun */
  dx = (x - 95.0) / 55.0, dy = (y - 145.0) / 50.0;
  if (dx * dx + dy * dy <= 1.0)
    return 0xFFD82828u;                                                  /* the ellipse */
  if (y >= 110 && y < 190 && x >= 210 - (y - 110) / 2 && x <= 210 + (y - 110) / 2)
    return 0xFF8B5A2Bu;                                                  /* the triangle */
  if (x >= 260 && x < 310 && y >= 140 && y < 190)
    return 0xFFF4A0B4u;                                                  /* the square */
  return y < 120 ? 0xFF78B4F0u : 0xFF3FA03Fu;                            /* sky, grass */
}

static void
write_ppm (const char *path, const uint32_t *px)
{
  FILE *f = fopen (path, "wb");

  if (f == NULL)
    exit (1);
  fprintf (f, "P6\n%d %d\n255\n", W, H);
  for (int i = 0; i < W * H; i++)
    {
      unsigned char rgb[3] = { (unsigned char) (px[i] >> 16), (unsigned char) (px[i] >> 8), (unsigned char) px[i] };

      fwrite (rgb, 1, 3, f);
    }
  fclose (f);
}

int
main (int argc, char **argv)
{
  static const char *const names[4] = { "original", "threshold", "ordered", "diffusion" };
  static uint32_t px[W * H];
  char path[4096];

  if (argc != 2)
    return 2;
  for (int k = 0; k < 4; k++)
    {
      for (int y = 0; y < H; y++)
        for (int x = 0; x < W; x++)
          px[y * W + x] = pixel (x, y);
      if (k > 0 && !easel_bw_convert (px, NULL, W, H, (EaselBw) (k - 1)))
        return 1;
      snprintf (path, sizeof path, "%s/%s.ppm", argv[1], names[k]);
      write_ppm (path, px);
    }
  return 0;
}
