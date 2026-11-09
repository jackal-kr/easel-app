/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "remove-bg.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

/* The place in a source of n samples that the sample i of m stands for, centers aligned, limited to the source; and its two
   neighbors and the weight of the second. */
static void
source_of (int i, int m, int n, int *a, int *b, float *t)
{
  float s = ((float) i + 0.5f) * (float) n / (float) m - 0.5f;

  if (s < 0)
    s = 0;
  if (s > (float) (n - 1))
    s = (float) (n - 1);
  *a = (int) s;
  *b = *a + 1 < n ? *a + 1 : *a;
  *t = s - (float) *a;
}

static bool
looked_at (const uint8_t *mask, size_t i)
{
  return mask == NULL || mask[i] != 0;
}

/* A channel (16 red, 8 green, 0 blue) of a pixel from 0 to 1, gray 0.5 where it is not looked at or fully transparent. */
static float
channel (const uint32_t *px, const uint8_t *mask, size_t i, int shift)
{
  if (!looked_at (mask, i) || (px[i] >> 24) == 0)
    return 0.5f;
  return (float) (px[i] >> shift & 0xFF) / 255.0f;
}

void
easel_rbg_input (const uint32_t *px, const uint8_t *mask, int w, int h, int side, float *out)
{
  static const int shifts[3] = { 16, 8, 0 };

  for (int y = 0; y < side; y++)
    {
      int y0, y1;
      float ty;

      source_of (y, side, h, &y0, &y1, &ty);
      for (int x = 0; x < side; x++)
        {
          int x0, x1;
          float tx;

          source_of (x, side, w, &x0, &x1, &tx);
          for (int c = 0; c < 3; c++)
            {
              float a = channel (px, mask, (size_t) y0 * w + x0, shifts[c]), b = channel (px, mask, (size_t) y0 * w + x1, shifts[c]);
              float d = channel (px, mask, (size_t) y1 * w + x0, shifts[c]), e = channel (px, mask, (size_t) y1 * w + x1, shifts[c]);
              float top = a + (b - a) * tx, bottom = d + (e - d) * tx;

              out[(size_t) c * side * side + (size_t) y * side + x] = top + (bottom - top) * ty;
            }
        }
    }
}

bool
easel_rbg_trivial (const uint32_t *px, const uint8_t *mask, int w, int h)
{
  bool seen = false;
  uint32_t first = 0;

  if (w < 16 || h < 16)
    return true;
  for (size_t i = 0; i < (size_t) w * (size_t) h; i++)
    if (looked_at (mask, i))
      {
        if (!seen)
          {
            first = px[i];
            seen = true;
          }
        else if (px[i] != first)
          return false;
      }
  return true;
}

size_t
easel_rbg_apply (uint32_t *px, const uint8_t *mask, int w, int h, const float *found, int side)
{
  float lo = found[0], hi = found[0];
  size_t changed = 0;

  for (size_t i = 1; i < (size_t) side * side; i++)
    {
      lo = fminf (lo, found[i]);
      hi = fmaxf (hi, found[i]);
    }
  if (!(hi > lo))
    return 0;                                     /* one value: nothing told apart, everything is subject */
  for (int y = 0; y < h; y++)
    {
      int y0, y1;
      float ty;

      source_of (y, h, side, &y0, &y1, &ty);
      for (int x = 0; x < w; x++)
        {
          size_t i = (size_t) y * w + x;
          int x0, x1;
          float tx, a, b, d, e, top, bottom, v;

          if (!looked_at (mask, i) || (px[i] >> 24) == 0)
            continue;
          source_of (x, w, side, &x0, &x1, &tx);
          a = found[(size_t) y0 * side + x0];
          b = found[(size_t) y0 * side + x1];
          d = found[(size_t) y1 * side + x0];
          e = found[(size_t) y1 * side + x1];
          top = a + (b - a) * tx;
          bottom = d + (e - d) * tx;
          v = (top + (bottom - top) * ty - lo) / (hi - lo);
          if (v < 0.5f)
            {
              px[i] &= 0x00FFFFFFu;                 /* removed: alpha 0, the color kept (REQ-0035-03) */
              changed++;
            }
        }
    }
  return changed;
}

EaselTransformResult
easel_image_remove_background (EaselImage *img, EaselHistory *history, const float *found, int side, size_t max_bytes)
{
  size_t total = (size_t) img->width * (size_t) img->height;
  uint32_t *px;

  if (total > SIZE_MAX / sizeof *px || total * sizeof *px > max_bytes || (px = malloc (total * sizeof *px)) == NULL)
    return EASEL_TRANSFORM_NO_MEMORY;
  memcpy (px, img->px, total * sizeof *px);
  easel_rbg_apply (px, NULL, img->width, img->height, found, side);
  return easel_image_replace_pixels (img, history, px, max_bytes);
}

EaselSelectionResult
easel_selection_remove_background (EaselFloat *f, const float *found, int side, size_t max_bytes)
{
  size_t n;
  uint32_t *px;
  EaselSelectionResult r;

  easel_selection_complete (f);
  n = (size_t) f->w * (size_t) f->h;
  if (n * sizeof *px > max_bytes || (px = malloc (n * sizeof *px)) == NULL)
    return EASEL_SELECTION_NO_MEMORY;
  memcpy (px, f->px, n * sizeof *px);
  if (easel_rbg_apply (px, f->mask, f->w, f->h, found, side) == 0)
    {
      free (px);
      return EASEL_SELECTION_NOTHING;
    }
  r = easel_selection_replace_image (f, px, max_bytes);
  free (px);
  return r;
}
