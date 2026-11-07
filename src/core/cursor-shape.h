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

/* The own cursors of Pencil, Airbrush, Fill, Pick Color and the Magnifier (REQ-0029-02, REQ-0029-03). */
typedef struct
{
  int size;             /* logical pixels, square */
  int device;           /* device pixels of its image, square */
  int hot_x, hot_y;     /* logical pixels */
} EaselToolCursor;

/* The cursor for the desktop's cursor size and the screen scale: the size is the desktop's, never less than 24; the image
   is size x scale device pixels, rounded up; the hotspot, given as a pixel of the 24-pixel artwork, is the pixel of the
   cursor that holds the same fraction of its width and height (that pixel's center scaled), inside the image. */
EaselToolCursor easel_tool_cursor_size (int desktop_size, double scale, int hot_x24, int hot_y24);
