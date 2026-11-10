/* SPDX-License-Identifier: GPL-3.0-or-later */

#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "image.h"

EaselImage *
easel_image_new (int width, int height, uint32_t fill)
{
  EaselImage *img;
  size_t n;

  if (width <= 0 || height <= 0)
    return NULL;
  n = (size_t) width * (size_t) height;
  if (n > SIZE_MAX / sizeof (uint32_t))
    return NULL;

  img = malloc (sizeof *img);
  if (img == NULL)
    return NULL;
  img->px = malloc (n * sizeof (uint32_t));
  if (img->px == NULL)
    {
      free (img);
      return NULL;
    }
  img->width = width;
  img->height = height;
  img->dirty = (EaselRect) { 0, 0, 0, 0 };
  fill = easel_px_canon (fill);               /* P7.7 */
  for (size_t i = 0; i < n; i++)
    img->px[i] = fill;
  return img;
}

void
easel_image_free (EaselImage *img)
{
  if (img == NULL)
    return;
  free (img->px);
  free (img);
}

uint32_t
easel_image_get (const EaselImage *img, int x, int y)
{
  if (x < 0 || y < 0 || x >= img->width || y >= img->height)
    return 0;
  return img->px[(size_t) y * (size_t) img->width + (size_t) x];
}

void
easel_image_set (EaselImage *img, int x, int y, uint32_t color)
{
  EaselRect *d = &img->dirty;

  if (x < 0 || y < 0 || x >= img->width || y >= img->height)
    return;
  img->px[(size_t) y * (size_t) img->width + (size_t) x] = color;

  if (d->x1 <= d->x0)
    {
      *d = (EaselRect) { x, y, x + 1, y + 1 };
      return;
    }
  if (x < d->x0)
    d->x0 = x;
  if (y < d->y0)
    d->y0 = y;
  if (x >= d->x1)
    d->x1 = x + 1;
  if (y >= d->y1)
    d->y1 = y + 1;
}

bool
easel_image_take_dirty (EaselImage *img, EaselRect *r)
{
  if (img->dirty.x1 <= img->dirty.x0)
    return false;
  *r = img->dirty;
  img->dirty = (EaselRect) { 0, 0, 0, 0 };
  return true;
}

EaselResizeResult
easel_image_resize (EaselImage *img, int width, int height, uint32_t fill)
{
  size_t n;
  uint32_t *px;
  int kept_w, kept_h;

  if (width < 1 || height < 1 || width > EASEL_IMAGE_MAX_SIZE || height > EASEL_IMAGE_MAX_SIZE)
    return EASEL_RESIZE_BAD_SIZE;
  n = (size_t) width * (size_t) height;
  px = malloc (n * sizeof (uint32_t));
  if (px == NULL)
    return EASEL_RESIZE_NO_MEMORY;

  /* The part both sizes share stays, in the top-left corner; the rest is new. */
  kept_w = width < img->width ? width : img->width;
  kept_h = height < img->height ? height : img->height;
  for (int y = 0; y < height; y++)
    {
      uint32_t *row = px + (size_t) y * (size_t) width;
      int from = 0;

      if (y < kept_h)
        {
          memcpy (row, img->px + (size_t) y * (size_t) img->width, (size_t) kept_w * sizeof (uint32_t));
          from = kept_w;
        }
      for (int x = from; x < width; x++)
        row[x] = easel_px_canon (fill);        /* P7.7 */
    }
  free (img->px);
  img->px = px;
  img->width = width;
  img->height = height;
  img->dirty = (EaselRect) { 0, 0, 0, 0 };
  return EASEL_RESIZE_OK;
}

void
easel_flatten_on_white (const uint32_t *src, uint8_t *rgb, int n)
{
  for (int i = 0; i < n; i++)
    {
      uint32_t c = src[i], a = c >> 24, rest = 255 * (255 - a);

      rgb[3 * i + 0] = (uint8_t) ((((c >> 16) & 0xFF) * a + rest + 127) / 255);
      rgb[3 * i + 1] = (uint8_t) ((((c >> 8) & 0xFF) * a + rest + 127) / 255);
      rgb[3 * i + 2] = (uint8_t) (((c & 0xFF) * a + rest + 127) / 255);
    }
}

EaselImage *
easel_image_from_bytes (const uint8_t *src, int w, int h, int stride, int channels, int orientation)
{
  EaselImage *img;
  int out_w, out_h;
  size_t row_step, col_step;      /* where a source pixel goes: dest index = base + x * col_step + y * row_step */
  size_t base;

  if (w <= 0 || h <= 0 || (channels != 3 && channels != 4) || (size_t) stride < (size_t) w * (size_t) channels)
    return NULL;
  if (orientation < 1 || orientation > 8)
    orientation = 1;
  out_w = orientation >= 5 ? h : w;
  out_h = orientation >= 5 ? w : h;
  img = easel_image_new (out_w, out_h, 0);
  if (img == NULL)
    return NULL;

  /* Source (x, y) lands at (dx, dy); each case is dx = fx + gx * x + hx * y, and the same for dy, written as steps in the
     flat destination array so the loop below is the same for all eight. */
  {
    size_t ow = (size_t) out_w;
    size_t W = (size_t) w, H = (size_t) h;

    switch (orientation)
      {
      case 1: base = 0;                         col_step = 1;           row_step = ow; break;
      case 2: base = W - 1;                     col_step = (size_t) -1; row_step = ow; break;
      case 3: base = (H - 1) * ow + (W - 1);    col_step = (size_t) -1; row_step = (size_t) -(ptrdiff_t) ow; break;
      case 4: base = (H - 1) * ow;              col_step = 1;           row_step = (size_t) -(ptrdiff_t) ow; break;
      case 5: base = 0;                         col_step = ow;          row_step = 1; break;
      case 6: base = H - 1;                     col_step = ow;          row_step = (size_t) -1; break;
      case 7: base = (W - 1) * ow + (H - 1);    col_step = (size_t) -(ptrdiff_t) ow; row_step = (size_t) -1; break;
      default: base = (W - 1) * ow;             col_step = (size_t) -(ptrdiff_t) ow; row_step = 1; break;
      }
  }

  for (int y = 0; y < h; y++)
    {
      const uint8_t *p = src + (size_t) y * (size_t) stride;
      size_t d = base + (size_t) y * row_step;

      for (int x = 0; x < w; x++, p += channels, d += col_step)
        {
          uint32_t a = channels == 4 ? p[3] : 255;

          img->px[d] = a == 0 ? 0 : (a << 24) | ((uint32_t) p[0] << 16) | ((uint32_t) p[1] << 8) | p[2];
        }
    }
  return img;
}
