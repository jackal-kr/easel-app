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

/* REQ-0035-04's automatic tolerance: 5 while the ring's median distance to the surface is at most 3.05 delta-E, then 3 more for each
   delta-E above (rounded down), at most 20. */
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

/* A press on the preview (0037): a pixel of px, as the caller looks at it. */
typedef struct
{
  int x, y;
} EaselRbgPress;

/* After easel_rbg_preview or easel_rbg_result into dst (REQ-0037-01, -02, -09): each piece holding an odd number of the n presses turns
   to the other kind. A piece is a 4-connected run of dst's kept, or removed, pixels that are looked at and were not fully transparent in
   px (up to EASEL_RBG_COPY_MAX pixels), or a run of the copy's labels whose blocks the pixels follow (over that). A removed pixel comes
   back only if the method removed it. A press on no piece does nothing. *removed (may be NULL), the pixels removed before it (as the
   preview or the result gave it), becomes the count after it. */
void easel_rbg_flips (EaselRbg *r, const uint32_t *px, const uint8_t *mask, uint32_t *dst, const EaselRbgPress *presses, size_t n,
                      size_t *removed);

/* Whether two pixels lie in one piece of the last easel_rbg_flips (REQ-0037-04); FALSE when either lies in none. */
bool easel_rbg_same_piece (const EaselRbg *r, int x1, int y1, int x2, int y2);

/* Whether the card shows its second line (REQ-0035-04): the ring's 90th percentile distance to the background as judged above 6
   delta-E, a border the background's model explains poorly in part. */
bool easel_rbg_complex (const EaselRbg *r);

/* Test hooks: the bytes the analysis holds (all of it is counted against max_bytes), the ring's median residual (delta-E x 64); the
   CIE Lab function f(t) of the table, t and the result in Q16 (t from 0 to 65536); the integer Lab of an sRGB color (L, a, b x 64). */
size_t easel_rbg_bytes (const EaselRbg *r);
int64_t easel_rbg_residual (const EaselRbg *r);
/* Test hook: the median (delta-E x 64) the automatic tolerance comes from: the surface's, or the clusters' where they judge. */
int64_t easel_rbg_judged (const EaselRbg *r);
/* Test hook: the automatic tolerance from the ring's median, before the stability check (REQ-0035-04). */
int easel_rbg_ring_tolerance (const EaselRbg *r);
int32_t easel_rbg_lab_f (const EaselRbg *r, int t);
void easel_rbg_lab (const EaselRbg *r, int red, int green, int blue, int16_t out[3]);
