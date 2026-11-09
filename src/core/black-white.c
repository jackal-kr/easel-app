/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "black-white.h"

#include <stdlib.h>
#include <string.h>

/* The 8 x 8 matrix of REQ-0034-05. */
static const uint8_t matrix[8][8] = { { 0, 32, 8, 40, 2, 34, 10, 42 }, { 48, 16, 56, 24, 50, 18, 58, 26 },
                                      { 12, 44, 4, 36, 14, 46, 6, 38 }, { 60, 28, 52, 20, 62, 30, 54, 22 },
                                      { 3, 35, 11, 43, 1, 33, 9, 41 },  { 51, 19, 59, 27, 49, 17, 57, 25 },
                                      { 15, 47, 7, 39, 13, 45, 5, 37 }, { 63, 31, 55, 23, 61, 29, 53, 21 } };

/* Y = 299 R + 587 G + 114 B, 0 to 255000. */
static int32_t
brightness (uint32_t p)
{
  return 299 * (int32_t) (p >> 16 & 0xFF) + 587 * (int32_t) (p >> 8 & 0xFF) + 114 * (int32_t) (p & 0xFF);
}

static uint32_t
black_or_white (uint32_t p, bool white)
{
  return (p & 0xFF000000u) | (white ? 0x00FFFFFFu : 0);
}

static bool
in_shape (const uint8_t *mask, size_t i)
{
  return mask == NULL || mask[i] != 0;
}

/* Error diffusion (REQ-0034-06) with two rows of shares, each one pixel wider on both sides so that no share needs a bounds test
   along x: cur holds what the row being gone through has received, next what the row after it receives. A value stays within
   -255000 to 510000 and a share within 7 x 510000 / 16 of it: 32 bits suffice. */
static bool
diffuse (uint32_t *px, const uint8_t *mask, int w, int h)
{
  int32_t *rows = calloc (2 * ((size_t) w + 2), sizeof *rows), *cur, *next;

  if (rows == NULL)
    return false;
  cur = rows + 1;
  next = rows + (w + 2) + 1;
  for (int y = 0; y < h; y++)
    {
      uint32_t *row = px + (size_t) y * (size_t) w;
      const uint8_t *mrow = mask != NULL ? mask + (size_t) y * (size_t) w : NULL;
      const uint32_t *below = y + 1 < h ? row + w : NULL;
      const uint8_t *mbelow = mask != NULL && below != NULL ? mrow + w : NULL;

      for (int x = 0; x < w; x++)
        {
          int32_t v, e;
          bool white;

          if (!in_shape (mrow, (size_t) x))
            continue;
          if ((row[x] >> 24) == 0)
            {
              row[x] = black_or_white (row[x], brightness (row[x]) >= 128000);   /* by its own Y, no share in or out */
              continue;
            }
          v = brightness (row[x]) + cur[x];
          white = v >= 128000;
          e = v - (white ? 255000 : 0);
          row[x] = black_or_white (row[x], white);
          /* A share goes only to a pixel in the picture (or the buffer), in the shape, and not fully transparent. */
          if (x + 1 < w && in_shape (mrow, (size_t) x + 1) && (row[x + 1] >> 24) != 0)
            cur[x + 1] += 7 * e / 16;
          if (below != NULL)
            {
              if (x > 0 && in_shape (mbelow, (size_t) x - 1) && (below[x - 1] >> 24) != 0)
                next[x - 1] += 3 * e / 16;
              if (in_shape (mbelow, (size_t) x) && (below[x] >> 24) != 0)
                next[x] += 5 * e / 16;
              if (x + 1 < w && in_shape (mbelow, (size_t) x + 1) && (below[x + 1] >> 24) != 0)
                next[x + 1] += e / 16;
            }
        }
      {
        int32_t *t = cur;

        cur = next;
        next = t;
        memset (next - 1, 0, ((size_t) w + 2) * sizeof *next);
      }
    }
  free (rows);
  return true;
}

bool
easel_bw_convert (uint32_t *px, const uint8_t *mask, int w, int h, EaselBw how)
{
  if (w <= 0 || h <= 0)
    return true;
  if (how == EASEL_BW_DIFFUSION)
    return diffuse (px, mask, w, h);
  for (int y = 0; y < h; y++)
    {
      uint32_t *row = px + (size_t) y * (size_t) w;
      const uint8_t *mrow = mask != NULL ? mask + (size_t) y * (size_t) w : NULL;

      for (int x = 0; x < w; x++)
        if (in_shape (mrow, (size_t) x))
          {
            int32_t yv = brightness (row[x]);
            bool white = how == EASEL_BW_THRESHOLD ? yv >= 128000
                                                   : 128 * (int64_t) yv >= 255000 * (int64_t) (2 * matrix[y & 7][x & 7] + 1);

            row[x] = black_or_white (row[x], white);
          }
    }
  return true;
}

EaselTransformResult
easel_image_black_white (EaselImage *img, EaselHistory *history, EaselBw how, size_t max_bytes)
{
  size_t total = (size_t) img->width * (size_t) img->height;
  uint32_t *px;

  if (total > SIZE_MAX / sizeof *px || total * sizeof *px > max_bytes || (px = malloc (total * sizeof *px)) == NULL)
    return EASEL_TRANSFORM_NO_MEMORY;
  memcpy (px, img->px, total * sizeof *px);
  if (!easel_bw_convert (px, NULL, img->width, img->height, how))
    {
      free (px);
      return EASEL_TRANSFORM_NO_MEMORY;
    }
  return easel_image_replace_pixels (img, history, px, max_bytes);       /* the step, as Invert's (REQ-0018-09) */
}
