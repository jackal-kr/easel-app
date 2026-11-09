/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Black and White (feature 0034): a picture or a selection turned to pure black and white by Threshold, Ordered dither or Error
   diffusion (REQ-0034-04 to -06), each pixel's alpha kept, in whole numbers so the same pixels always give the same result. */
#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "history.h"
#include "image.h"
#include "transform.h"

typedef enum
{
  EASEL_BW_THRESHOLD,     /* white when Y >= 128000 (REQ-0034-04) */
  EASEL_BW_ORDERED,       /* the 8 x 8 matrix (REQ-0034-05) */
  EASEL_BW_DIFFUSION      /* Floyd-Steinberg, shares rounded toward zero (REQ-0034-06) */
} EaselBw;

/* Converts the w x h pixels px in place, rows of w pixels, x and y counted from their first pixel. With a mask (w x h bytes, 1 in
   the shape) only the shape's pixels change and Error diffusion's shares to pixels outside it are dropped; a fully transparent
   pixel is converted by its own brightness and takes and passes no share. False, px untouched, if Error diffusion cannot get the
   two rows it works in (8 bytes a pixel of a row). */
bool easel_bw_convert (uint32_t *px, const uint8_t *mask, int w, int h, EaselBw how);

/* The whole picture converted as one undo step (REQ-0034-03), as Invert Colors makes its step (REQ-0018-09): a log of the changed
   pixels while they are at most a third of the picture, else its old buffer kept whole. The converted picture is first made in a
   buffer of its own (4 bytes a pixel). NOTHING when no pixel changes; NO_MEMORY, the picture and the history as they were, when an
   allocation above max_bytes, or one that cannot be had, is needed. */
EaselTransformResult easel_image_black_white (EaselImage *img, EaselHistory *history, EaselBw how, size_t max_bytes);
