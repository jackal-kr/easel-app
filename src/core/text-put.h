/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Putting a text box down (feature 0017, REQ-0017-16): its ink written into the picture as one undo step. */
#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "history.h"
#include "image.h"
#include "selection.h"

/* The text as pixels, made by the ui from the box's layout: a rectangle of the picture (it may reach outside the picture), each pixel's
   color and whether it is written (the mask: a character covers at least half of it). The colors are written as they are, alpha
   included. */
typedef struct
{
  int x, y, w, h;
  const uint32_t *color;
  const uint8_t *mask;
} EaselTextInk;

/* Writes the box into the picture: in Opaque mode the box (x, y, w, h) filled with `background` first, then the ink's masked pixels;
   only pixels inside the picture change. One step of `history` (none when it is NULL); NOTHING when no pixel differs (no step);
   NO_MEMORY, with the picture as it was, when the step's memory cannot be had (max_bytes for the log, or the history's). */
EaselSelectionResult easel_text_put_down (EaselImage *img, EaselHistory *history, int x, int y, int w, int h, const EaselTextInk *ink,
                                          bool opaque, uint32_t background, size_t max_bytes);
