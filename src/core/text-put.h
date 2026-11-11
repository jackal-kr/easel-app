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
/* Smooth text (0040): one style run's coverage over its rectangle, 0 to 255 a pixel, and its color; a run with color glyphs has a
   straight color for each pixel instead (`colors`, else NULL). */
typedef struct
{
  int x, y, w, h;
  uint32_t color;
  const uint8_t *cover;
  const uint32_t *colors;
} EaselTextLayer;

/* Sharp text has color and mask; Smooth text has layers, in the order of the text (REQ-0040-05), and color and mask NULL. */
typedef struct
{
  int x, y, w, h;
  const uint32_t *color;
  const uint8_t *mask;
  const EaselTextLayer *layers;
  size_t n_layers;
} EaselTextInk;

/* The value the box gives the pixel (x, y) whose color is `under`: the background inside an Opaque box (bx, by, bw, bh) first; then with
   Sharp the ink's color where masked; with Smooth each layer that covers it mixed over in order by easel_cover_mix255 (REQ-0040-04,
   REQ-0040-05). The put-down and the box's view both ask it (REQ-0040-08). */
uint32_t easel_text_ink_value (const EaselTextInk *ink, int x, int y, uint32_t under, int bx, int by, int bw, int bh, bool opaque,
                               uint32_t background);

/* Writes the box into the picture: in Opaque mode the box (x, y, w, h) filled with `background` first, then the ink's masked pixels;
   only pixels inside the picture change. One step of `history` (none when it is NULL); NOTHING when no pixel differs (no step);
   NO_MEMORY, with the picture as it was, when the step's memory cannot be had (max_bytes for the log, or the history's). */
EaselSelectionResult easel_text_put_down (EaselImage *img, EaselHistory *history, int x, int y, int w, int h, const EaselTextInk *ink,
                                          bool opaque, uint32_t background, size_t max_bytes);
