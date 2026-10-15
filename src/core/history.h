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

/* Write the step's pixels back through easel_image_set, so the screen update sees them. False: nothing to do. */
bool easel_history_undo (EaselHistory *h, EaselImage *img);
bool easel_history_redo (EaselHistory *h, EaselImage *img);
/* True if the most recent undo or redo was refused for lack of memory (REQ-0004-26); then the picture and the history
   position are unchanged, w and h get the size that could not be allocated, and the same call can be tried again.
   Any later successful undo, redo or edit clears it. */
bool easel_history_refused (const EaselHistory *h, int *w, int *h_out);
bool easel_history_can_undo (const EaselHistory *h);
bool easel_history_can_redo (const EaselHistory *h);

/* What the history stores: per step 12 bytes per changed pixel plus 64 bytes for the step itself (REQ-0002-16). */
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
