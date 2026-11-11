/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Pencil stroke: 1-pixel 8-connected lines in one color, clipped to the picture, cancellable
   (REQ-0001-19..23, REQ-0001-26). */
#pragma once

#include <stddef.h>
#include <stdint.h>

#include "footprint.h"
#include "history.h"
#include "image.h"

typedef struct _EaselStroke EaselStroke;

/* Sets the pixel at (x, y) if it is inside. Returns NULL if memory runs out, or if the picture has more pixels than a
   32-bit index can address. */
EaselStroke *easel_stroke_begin (EaselImage *img, int x, int y, uint32_t color);

/* A stroke that stamps a footprint (the Brush and the Eraser, REQ-0006-06, REQ-0006-11, REQ-0006-12) instead of one pixel:
   the footprint of f is painted with color at (x, y) and, by easel_stroke_to, at every pixel of the 8-connected line to
   each new position. If only is not NULL, only pixels equal to *only in all four channels are changed (the color eraser).
   Same end and cancel as the Pencil's; a pixel is changed and logged at most once. */
EaselStroke *easel_stroke_begin_stamp (EaselImage *img, int x, int y, uint32_t color, EaselFootprint f,
                                       const uint32_t *only);

/* The Airbrush's spray (REQ-0006-08, REQ-0006-09). The stroke paints `color` at random pixels of the disc of the given
   diameter (the circle footprint of that size) around the positions it is told of, per_spray pixels at each spray, chosen
   independently and with equal chance (a pixel can be chosen twice); seed starts the random sequence (any value, also 0).
   Same end and cancel as the others. */
EaselStroke *easel_stroke_begin_spray (EaselImage *img, uint32_t color, int diameter, int per_spray, uint64_t seed);

/* Makes `sprays` sprays with the pointer at (x, y); a spray whose pixel is outside the picture paints nothing. Does
   nothing for a stroke that is not a spray; easel_stroke_to does nothing for a spray. */
void easel_stroke_spray (EaselStroke *s, int x, int y, int sprays);

/* How many sprays are due when the button has been held for held_us microseconds: one at the press and one for every full
   5 ms since, so that a late timer catches up (REQ-0006-08). The caller makes due - made sprays. */
int easel_spray_due (int64_t held_us);

/* Pixels per spray for a disc: 1 for 9 and 16, 3 for 24 (REQ-0006-08). */
int easel_spray_pixels (int diameter);

/* The Brush with Smooth (REQ-0039-05 (e), REQ-0039-07): the footprint's shape at (x, y) and, by easel_stroke_to, swept along the
   straight segment to each new position; each pixel takes the mix (REQ-0039-06) of its color before the stroke and `color` by the
   points the whole path so far covers. Same end and cancel as the others. NULL if memory runs out. */
EaselStroke *easel_stroke_begin_smooth (EaselImage *img, int x, int y, uint32_t color, EaselFootprint f);

/* Test hook: a smooth stroke may hold at most max_tiles of its 16 x 16 tiles of points from now on; the pixels of a tile it
   cannot have are left as they are, as a pixel whose log entry cannot be had is. */
void easel_stroke_limit_tiles (EaselStroke *s, size_t max_tiles);

/* Draws the line from the previous position to (x, y). */
void easel_stroke_to (EaselStroke *s, int x, int y);

/* Frees the stroke; returns how many pixels it changed. With a history, those pixels become one undo step
   (REQ-0002-01); if the history cannot take the step (no memory) the stroke is undone and 0 is returned. */
size_t easel_stroke_end (EaselStroke *s, EaselHistory *history);

/* Restores every pixel the stroke changed and frees it. */
void easel_stroke_cancel (EaselStroke *s);
