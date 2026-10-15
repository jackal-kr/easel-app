/* SPDX-License-Identifier: GPL-3.0-or-later */

#include <stdlib.h>

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
