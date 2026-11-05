/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Image Transforms (feature 0018): the eight ways a picture can be turned and flipped, Invert Colors and Clear Image. An orientation
   is a transpose (swapping x and y) followed by a mirror of x and a mirror of y; the five commands of the Image menu are five of the
   eight. The whole picture and a selection use the same maps (REQ-0018-04, REQ-0018-07). */
#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "history.h"
#include "image.h"

typedef uint8_t EaselOrient;

enum
{
  EASEL_ORIENT_IDENTITY = 0,
  EASEL_ORIENT_TRANSPOSE = 1,   /* x and y swapped first: a quarter turn has it */
  EASEL_ORIENT_FLIP_X = 2,      /* then x mirrored in the size after the transpose */
  EASEL_ORIENT_FLIP_Y = 4       /* then y mirrored */
};

/* The commands (REQ-0018-01). */
typedef enum
{
  EASEL_OP_FLIP_H,        /* the pixel at x goes to width - 1 - x */
  EASEL_OP_FLIP_V,        /* y goes to height - 1 - y */
  EASEL_OP_ROTATE_90,     /* clockwise: the old (x, y) is the new (H - 1 - y, x) */
  EASEL_OP_ROTATE_180,
  EASEL_OP_ROTATE_270,    /* clockwise three quarters: the old (x, y) is the new (y, W - 1 - x) */
  EASEL_OP_INVERT,        /* red, green and blue become 255 minus themselves, alpha is kept */
  EASEL_OP_CLEAR          /* every pixel takes the background */
} EaselOp;

/* The orientation of one of the first five commands. */
EaselOrient easel_op_orient (EaselOp op);

/* The orientation that does `first` and then `then`, and the one that takes `o` back. */
EaselOrient easel_orient_compose (EaselOrient first, EaselOrient then);
EaselOrient easel_orient_inverse (EaselOrient o);

/* Whether it swaps width and height (a quarter turn), and the size of a w x h picture after it. */
bool easel_orient_swaps (EaselOrient o);
void easel_orient_size (EaselOrient o, int w, int h, int *ow, int *oh);

/* The pixel of a w x h source that lands at (dx, dy) of the oriented picture. */
void easel_orient_source (EaselOrient o, int w, int h, int dx, int dy, int *sx, int *sy);
/* Where the pixel (x, y) of a w x h source lands in the oriented picture (the inverse of easel_orient_source). */
void easel_orient_target (EaselOrient o, int w, int h, int x, int y, int *tx, int *ty);

/* The pixels of src (w x h) oriented into dst (easel_orient_size's size, which the caller allocates), every pixel moved exactly. */
void easel_orient_pixels (const uint32_t *src, int w, int h, EaselOrient o, uint32_t *dst);

typedef enum
{
  EASEL_TRANSFORM_DONE,       /* done, as one undo step */
  EASEL_TRANSFORM_NOTHING,    /* no pixel and no size differs: no step */
  EASEL_TRANSFORM_NO_MEMORY   /* nothing changed */
} EaselTransformResult;

/* A command on the whole picture as one undo step (REQ-0018-09), the cheaper of two kinds: a log of the changed pixels (12 bytes
   each plus 64) while that is not more than 4 bytes a pixel of the picture, else the picture's old buffer kept whole (4 bytes a
   pixel plus 64), which a quarter turn always does. The background is Clear Image's color (exactly, in all four channels). Every
   allocation (the new pixels, the log) above max_bytes, or that cannot be had, is NO_MEMORY with the picture and the history as
   they were. After a quarter turn the picture has its new size and its whole area is dirty. */
EaselTransformResult easel_image_transform (EaselImage *img, EaselHistory *h, EaselOp op, uint32_t background, size_t max_bytes);
