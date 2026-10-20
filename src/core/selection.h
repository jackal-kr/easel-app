/* SPDX-License-Identifier: GPL-3.0-or-later */
/* The floating selection of feature 0013. The picture is not touched while a selection floats: the float holds the lifted
   pixels, its place, the hole it left (REQ-0013-08) and the copies that Ctrl leaves (REQ-0013-09); one function lays the
   result over picture pixels, for the preview and for the put-down, which is one undo step (REQ-0013-11). */
#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "history.h"
#include "image.h"

/* A copy left in the picture by a Ctrl drag of a float: where. */
typedef struct
{
  int x, y;
} EaselStamp;

/* The lifted pixels (w x h, 0xAARRGGBB straight alpha) at the place (x, y) of the picture, which may be partly or wholly
   outside it; the hole at (hx, hy), w x h, in hole_color (none after a lift with Ctrl); the stamps, in their order. */
typedef struct
{
  int x, y, w, h;
  uint32_t *px;
  bool hole;
  int hx, hy;
  uint32_t hole_color;
  EaselStamp *stamps;
  size_t n_stamps, cap;
} EaselFloat;

typedef enum
{
  EASEL_SELECTION_DONE,       /* done; for a put-down, one undo step (with a history) */
  EASEL_SELECTION_NOTHING,    /* no pixel of the picture differs: no step */
  EASEL_SELECTION_NO_MEMORY   /* nothing changed */
} EaselSelectionResult;

/* Lifts the rectangle (x, y, w, h), inside the picture, into f (which must be empty): its pixels are copied, and unless copy
   it leaves a hole in hole_color. NO_MEMORY, with f empty, if its w x h x 4 bytes exceed max_bytes or cannot be had. */
EaselSelectionResult easel_selection_lift (EaselFloat *f, const EaselImage *img, int x, int y, int w, int h, bool copy,
                                           uint32_t hole_color, size_t max_bytes);

/* Leaves a copy of the float where it is now, as it would be put down there (REQ-0013-09); it belongs to the float's one
   step. FALSE, with nothing added, if the stamps would need more than max_bytes or cannot grow. */
bool easel_selection_stamp (EaselFloat *f, size_t max_bytes);

/* Lays the result over the rectangle of w x h picture pixels at (x0, y0), written to dst (stride: dst_stride pixels): the
   picture, the hole, each stamp, then, if with_float, the float at its place; a float or stamp pixel replaces what is under
   it in all four channels, whatever its color (REQ-0013-13). Only places of picture pixels are written. */
void easel_selection_lay (const EaselFloat *f, bool with_float, const EaselImage *img, int x0, int y0, int w, int h,
                          uint32_t *dst, size_t dst_stride);

/* Writes the result into the picture as one undo step (REQ-0013-11): DONE, NOTHING if no pixel differs, or NO_MEMORY with
   the picture as before if the step's log would exceed max_bytes or cannot be had. The float is kept (the caller frees it). */
EaselSelectionResult easel_selection_put_down (const EaselFloat *f, bool with_float, EaselImage *img,
                                               EaselHistory *history, size_t max_bytes);

/* Clear Selection on a marked selection (REQ-0013-14): the rectangle (x, y, w, h), inside the picture, takes color as one
   undo step; NOTHING if no pixel changes, NO_MEMORY with the picture as before. A float is cleared by easel_selection_put_down
   without the float. */
EaselSelectionResult easel_selection_clear (EaselImage *img, EaselHistory *history, int x, int y, int w, int h, uint32_t color,
                                            size_t max_bytes);

/* Frees the float's memory and empties it. */
void easel_float_free (EaselFloat *f);
