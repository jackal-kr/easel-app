/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Pencil stroke: 1-pixel 8-connected lines in one color, clipped to the picture, cancellable
   (REQ-0001-19..23, REQ-0001-26). */
#pragma once

#include <stddef.h>

#include "history.h"
#include "image.h"

typedef struct _EaselStroke EaselStroke;

/* Sets the pixel at (x, y) if it is inside. Returns NULL if memory runs out, or if the picture has more pixels than a
   32-bit index can address. */
EaselStroke *easel_stroke_begin (EaselImage *img, int x, int y, uint32_t color);

/* Draws the line from the previous position to (x, y). */
void easel_stroke_to (EaselStroke *s, int x, int y);

/* Frees the stroke; returns how many pixels it changed. With a history, those pixels become one undo step
   (REQ-0002-01); if the history cannot take the step (no memory) the stroke is undone and 0 is returned. */
size_t easel_stroke_end (EaselStroke *s, EaselHistory *history);

/* Restores every pixel the stroke changed and frees it. */
void easel_stroke_cancel (EaselStroke *s);
