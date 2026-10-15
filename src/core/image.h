/* SPDX-License-Identifier: GPL-3.0-or-later */
/* The picture: straight-alpha 0xAARRGGBB pixels, row by row (ADR-0002). */
#pragma once

#include <stdint.h>

#include <stdbool.h>

/* Half-open: x0 <= x < x1, y0 <= y < y1. */
typedef struct
{
  int x0, y0, x1, y1;
} EaselRect;

typedef struct
{
  int width;
  int height;
  uint32_t *px;
  EaselRect dirty;        /* empty when x1 <= x0 */
} EaselImage;

/* Returns NULL if a size is not positive or memory runs out. */
EaselImage *easel_image_new (int width, int height, uint32_t fill);
void easel_image_free (EaselImage *img);

/* Outside the image: 0 (transparent). */
uint32_t easel_image_get (const EaselImage *img, int x, int y);

/* The one dirty-marking path for pixel writes. Outside the image: ignored. */
void easel_image_set (EaselImage *img, int x, int y, uint32_t color);

/* Copies the area changed since the last call into *r and clears it; false if nothing changed. */
bool easel_image_take_dirty (EaselImage *img, EaselRect *r);
