/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Undo history: one step per user-visible edit, each holding only the pixels it changed (REQ-0002-01..05, -12..14). */
#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "image.h"

/* One changed pixel: its index (y * width + x) and its value before and after the edit. */
typedef struct
{
  uint32_t index;
  uint32_t old_value;
  uint32_t new_value;
} EaselChange;

typedef struct _EaselHistory EaselHistory;

EaselHistory *easel_history_new (size_t budget_bytes);
void easel_history_free (EaselHistory *h);

/* Adds a finished edit as one step and discards every redoable step. On success it takes ownership of changes
   (malloc'd, n entries); with n == 0 nothing is added and changes is freed. Returns false if memory ran out: nothing
   was added and the caller still owns changes. */
bool easel_history_push (EaselHistory *h, EaselChange *changes, size_t n);

/* A run of pixels in one row, x0 to x1 inclusive: a piece of a fill's area (REQ-0007-05). */
typedef struct
{
  uint32_t y, x0, x1;
} EaselSpan;

/* A fill as one step: every pixel of the n spans had old_value and now has new_value (a fill replaces only pixels equal in
   all four channels). Same ownership as easel_history_push: on success it takes spans (malloc'd); with n == 0 nothing is
   added and spans is freed; false means no memory and the caller still owns spans. A step counts 12 bytes per span plus 64,
   which is within the 16 bytes per pixel of REQ-0002-16, since a span has at least one pixel. */
bool easel_history_push_fill (EaselHistory *h, uint32_t old_value, uint32_t new_value, EaselSpan *spans, size_t n);

/* A growth of the picture from old_w x old_h to its present new_w x new_h (the new area filled with fill) joined to the pixels a put-down
   then changed (REQ-0016-07: a paste with Resize Canvas to Picture), as one step, which is added even with no change (the growth alone).
   The changes' indexes are of the grown size. Undo writes their old values and crops back; redo grows with fill and writes the new
   values. It counts 12 bytes per change plus 64 (a growth removes no pixel). Same ownership as easel_history_push; false: no memory, the
   caller still owns changes. */
bool easel_history_push_grow (EaselHistory *h, int old_w, int old_h, int new_w, int new_h, uint32_t fill, EaselChange *changes,
                              size_t n);

/* A step that keeps a whole picture buffer (0018: a turn, a flip, an invert or a clear of the picture): px, w x hgt, is the picture's
   buffer as it was before the command, which the caller has just replaced by new pixels of its own (and size); on success the history
   owns px. Undo and Redo swap the picture's buffer and size with the step's, allocating nothing, and mark the whole picture dirty. It
   counts 4 bytes a pixel plus 64, and its pixel count never changes. False, with px still the caller's: no memory. */
bool easel_history_push_buffer (EaselHistory *h, int w, int hgt, uint32_t *px);

/* Write the step's pixels back through easel_image_set, so the screen update sees them. False: nothing to do. */
bool easel_history_undo (EaselHistory *h, EaselImage *img);
bool easel_history_redo (EaselHistory *h, EaselImage *img);
/* True if the most recent undo or redo was refused for lack of memory (REQ-0004-26); then the picture and the history
   position are unchanged, w and h get the size that could not be allocated, and the same call can be tried again.
   Any later successful undo, redo or edit clears it. */
bool easel_history_refused (const EaselHistory *h, int *w, int *h_out);
bool easel_history_can_undo (const EaselHistory *h);
bool easel_history_can_redo (const EaselHistory *h);

/* What the history stores: per step 12 bytes per changed pixel (per span, for a fill) plus 64 bytes for the step itself
   (REQ-0002-16). */
size_t easel_history_bytes (const EaselHistory *h);

/* A resize of the picture as one undo step (REQ-0004-22..24, REQ-0004-26), with everything it needs:
   - allocates what the step needs first, then resizes img (easel_image_resize), then records the step, so a failure at
     any point changes neither the picture nor the history;
   - returns UNCHANGED, with no step, if the picture already has that size;
   - a step counts 4 bytes per removed pixel plus 64 for the budget; adding pixels needs no storage.
   Undo gives back the old size and every pixel, including the removed ones; redo resizes again with the same fill.
   Undo and redo of a resize allocate the new pixel buffer; if that fails they return false and change nothing. */
EaselResizeResult easel_resize_edit (EaselImage *img, EaselHistory *h, int width, int height, uint32_t fill);

/* The current state becomes the saved one. Modified means the current state differs from the saved one, or the saved
   one can no longer be reached (REQ-0002-12, REQ-0002-14). */
void easel_history_mark_saved (EaselHistory *h);
bool easel_history_is_modified (const EaselHistory *h);
