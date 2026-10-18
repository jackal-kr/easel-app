/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Fill (REQ-0007-05, REQ-0007-06, REQ-0007-07): the pixels connected to one pixel through pixels equal to it in all four
   channels become one color, as one undo step. */
#pragma once

#include <stddef.h>
#include <stdint.h>

#include "history.h"
#include "image.h"

typedef enum
{
  EASEL_FILL_DONE,         /* the area was filled and, with a history, is one undo step */
  EASEL_FILL_NOTHING,      /* (x, y) is outside the picture, or the pixel already has the color: no change, no step */
  EASEL_FILL_NO_MEMORY     /* the picture is as it was and no step was added */
} EaselFillResult;

/* Fills with color the area around (x, y): the pixels reachable from it by steps left, right, up and down (not
   diagonally) through pixels equal to the one at (x, y) in all four channels. The new value is exactly color (no
   blending). With a history, the area becomes one step (kept even if it is larger than the budget, REQ-0002-17); without
   one, the pixels are just filled. *changed, if not NULL, gets the number of pixels filled (0 unless DONE). */
EaselFillResult easel_fill (EaselImage *img, EaselHistory *history, int x, int y, uint32_t color, size_t *changed);

/* Test hook: the same, but the fill gives up with NO_MEMORY when its work buffers would need more than max_bytes, so that
   a test can reach the failure path without exhausting the machine (the core test also does it with a real limit). */
EaselFillResult easel_fill_capped (EaselImage *img, EaselHistory *history, int x, int y, uint32_t color, size_t *changed,
                                   size_t max_bytes);
