/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Remove background (feature 0035, ADR-0007): the background found by its colors and edges, no model (REQ-0035-03). A working copy of
   at most EASEL_RBG_COPY_MAX pixels (block means); on it an integer Lab approximation, a cubic background surface fitted to the ring,
   a marker-based watershed on the edge strength, the largest regions kept and small holes filled; the boundary refined at full size.
   Integers only, so the same pixels and tolerance give the same result on every machine. docs/reference/0035/classical/classical-v3.c
   is the same method for a whole opaque picture. */
#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* The Tolerance's range in the dialog, and the copy's most cells (REQ-0035-04, the copy of the spec's "Throughout"). */
enum { EASEL_RBG_TOLERANCE_MIN = 1, EASEL_RBG_TOLERANCE_MAX = 40, EASEL_RBG_COPY_MAX = 2000000 };

typedef struct EaselRbg EaselRbg;

typedef enum
{
  EASEL_RBG_DONE,
  EASEL_RBG_NO_MEMORY,          /* a buffer above the limit, or one that cannot be had: nothing written */
  EASEL_RBG_CANCELLED           /* *cancel became nonzero: dst undefined */
} EaselRbgResult;

/* The analysis of w x h pixels px, looked at where mask (NULL for all, or w x h bytes, 1 in the shape) says: the copy, its colors, its
   edges, the first fit and the automatic tolerance. NULL when it would need more than max_bytes, or memory cannot be had. */
EaselRbg *easel_rbg_new (const uint32_t *px, const uint8_t *mask, int w, int h, size_t max_bytes);
void easel_rbg_free (EaselRbg *r);

/* REQ-0035-04's automatic tolerance: 5 while the ring's median distance to the first fit is at most 2 delta-E, then 2 more for each
   whole delta-E above, at most 20. */
int easel_rbg_auto_tolerance (const EaselRbg *r);

/* The side of a copy block in pixels (1 when the copy is the pixels themselves), and the copy's size. */
int easel_rbg_block (const EaselRbg *r);
size_t easel_rbg_copy_cells (const EaselRbg *r);

/* The copy's labels for a tolerance (clamped to the range): the refit, the watershed, the regions. */
EaselRbgResult easel_rbg_label (EaselRbg *r, int tolerance, const volatile int *cancel);

/* After easel_rbg_label: dst (w x h, not px) is px with the background removed, alpha 0, the color kept; a pixel not looked at or
   already fully transparent as it was. The preview takes each pixel's block's label; the result refines the boundary (REQ-0035-03 d).
   *removed (may be NULL) gets the pixels removed. cancel may be NULL. */
EaselRbgResult easel_rbg_preview (const EaselRbg *r, const uint32_t *px, const uint8_t *mask, uint32_t *dst, size_t *removed);
EaselRbgResult easel_rbg_result (const EaselRbg *r, const uint32_t *px, const uint8_t *mask, uint32_t *dst, size_t *removed,
                                 const volatile int *cancel);

/* Test hooks: the bytes the analysis holds (all of it is counted against max_bytes), the ring's median residual (delta-E x 64). */
size_t easel_rbg_bytes (const EaselRbg *r);
int64_t easel_rbg_residual (const EaselRbg *r);
