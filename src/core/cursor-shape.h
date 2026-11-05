/* SPDX-License-Identifier: GPL-3.0-or-later */
/* The cursor of the Brush and the Eraser: the outline of the footprint at the current zoom (REQ-0006-16). */
#pragma once

#include <stdint.h>

#include "footprint.h"

typedef struct
{
  int width, height;    /* of the whole image, in device pixels */
  int hot_x, hot_y;     /* the center of the pixel under the pointer */
} EaselCursorShape;

/* The image's size and hotspot for a footprint at zoom percent with the given screen scale. The black outline is the
   footprint's size in device pixels (rounded), never less than 5 x 5 (then a plain square), and the white line one device
   pixel outside it, so the image is 2 larger. The hotspot is the center of the footprint's anchor pixel. */
EaselCursorShape easel_cursor_shape (EaselFootprint f, int zoom, int scale);

/* Fills width * height ARGB pixels (0xAARRGGBB, straight alpha): black where a pixel of the footprint has a neighbor
   (left, right, above, below) outside it, white outside the footprint next to a black pixel (also diagonally),
   transparent elsewhere. */
void easel_cursor_render (EaselFootprint f, int zoom, int scale, uint32_t *argb);
