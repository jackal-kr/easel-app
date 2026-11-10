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

/* P7.7 (ADR-0008): the value an operation writes for color: 0x00000000 when color is fully transparent, whatever red, green and blue
   it holds. */
static inline uint32_t
easel_px_canon (uint32_t color)
{
  return (color >> 24) != 0 ? color : 0;
}

/* Fill's sameness (REQ-0007-05, P7.7): equal in all four channels, or both fully transparent. */
static inline bool
easel_px_same (uint32_t a, uint32_t b)
{
  return a == b || ((a >> 24) == 0 && (b >> 24) == 0);
}

/* Returns NULL if a size is not positive or memory runs out. */
EaselImage *easel_image_new (int width, int height, uint32_t fill);
void easel_image_free (EaselImage *img);

/* The largest width or height of a picture (REQ-0001-02, REQ-0004-18). */
#define EASEL_IMAGE_MAX_SIZE 10000

typedef enum
{
  EASEL_RESIZE_OK,
  EASEL_RESIZE_UNCHANGED,   /* easel_resize_edit only: the picture already has that size */
  EASEL_RESIZE_BAD_SIZE,    /* a side below 1 or above EASEL_IMAGE_MAX_SIZE */
  EASEL_RESIZE_NO_MEMORY
} EaselResizeResult;

/* Gives the picture a new size: the pixels that stay (the top-left part both sizes share) are unchanged, new pixels take
   fill, removed ones are gone (REQ-0004-20). On BAD_SIZE or NO_MEMORY the picture is untouched. The dirty area is
   cleared: the caller rebuilds whatever shows it. */
EaselResizeResult easel_image_resize (EaselImage *img, int width, int height, uint32_t fill);

/* Outside the image: 0 (transparent). */
uint32_t easel_image_get (const EaselImage *img, int x, int y);

/* The one dirty-marking path for pixel writes. Outside the image: ignored. */
void easel_image_set (EaselImage *img, int x, int y, uint32_t color);

/* Copies the area changed since the last call into *r and clears it; false if nothing changed. */
bool easel_image_take_dirty (EaselImage *img, EaselRect *r);

/* Flattens n pixels on white for the formats without transparency (REQ-0005-11): every channel c of a pixel with alpha a
   becomes round ((c * a + 255 * (255 - a)) / 255); rgb gets three bytes per pixel, red first. The only copy of the
   formula. */
void easel_flatten_on_white (const uint32_t *src, uint8_t *rgb, int n);

/* Makes a picture from decoded bytes in one pass, upright as the EXIF orientation says (REQ-0005-06): src holds h rows
   of w pixels, stride bytes apart, each 3 bytes (red, green, blue: opaque) or 4 (with alpha). orientation is the EXIF
   value 1 to 8 (1 as stored, 2 mirrored left to right, 3 turned 180 degrees, 4 mirrored top to bottom, 5 transposed,
   6 turned 90 degrees clockwise, 7 transversed, 8 turned 90 degrees counter-clockwise); anything else counts as 1. For
   5 to 8 the picture is h wide and w high. A fully transparent pixel becomes 0x00000000. NULL if the size is not
   positive, channels is not 3 or 4, or memory runs out; w and h are the stored size. */
EaselImage *easel_image_from_bytes (const uint8_t *src, int w, int h, int stride, int channels, int orientation);
