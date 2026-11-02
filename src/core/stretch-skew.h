/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Stretch and Skew (0019): the numbers of the dialog and the pixels of the result. Every rounding is to the nearest, halves up. */
#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "history.h"
#include "image.h"
#include "transform.h"

#define EASEL_SS_MAX_SIZE 10000

/* pct percent of size, at least 1 and at most EASEL_SS_MAX_SIZE (REQ-0019-03, REQ-0019-04). */
int easel_ss_percent (int size, int pct);
/* The same with no upper limit: what New size asks for, 40000 for 500 % of 8000 (REQ-0019-07). */
int easel_ss_percent_unlimited (int size, int pct);
/* What percent of size px is, from 1 to 500 (REQ-0019-03, REQ-0019-04). */
int easel_ss_to_percent (int px, int size);
/* The other side when a side goes from `from` to `changed` and the ratio is kept: other x changed / from, at least 1 (REQ-0019-05). */
int easel_ss_ratio_other (int changed, int from, int other);
/* The largest value up to `changed` for which easel_ss_ratio_other stays within EASEL_SS_MAX_SIZE (REQ-0019-05). */
int easel_ss_ratio_fit (int changed, int from, int other);

/* The skew's amount S of an angle in whole degrees (-89 to 89) on n rows or columns: (n - 1) x its tangent, with its sign. From a table of
   the tangents, so that the same input gives the same S on any machine (P7.4). 0 on one row. */
int easel_ss_skew_amount (int n, int degrees);
/* The whole degrees, -89 to 89, of an amount S on n rows or columns: the arctangent of S / (n - 1). 0 on one row (REQ-0019-04). */
int easel_ss_skew_degrees (int s, int n);
/* How far row i of n goes toward larger x under the amount s (REQ-0019-10); the same for columns toward larger y. */
int easel_ss_shift (int i, int n, int s);

/* The size after a stretch to W x H, then a horizontal skew, then a vertical one (REQ-0019-08), each in degrees or in pixels, and the two
   amounts that apply, with their signs: 0 on one row or one column at that step (REQ-0019-10). */
void easel_ss_size (int W, int H, int sh, bool sh_degrees, int sv, bool sv_degrees, int *out_w, int *out_h, int *amount_h, int *amount_v);
/* Whether a size is past what a picture or a selection may have (REQ-0019-07). */
bool easel_ss_too_large (int w, int h);

/* The pixels of a w x h image stretched to W x H by the nearest pixel (REQ-0019-09), then skewed: dst holds (W + |amount_h|) x (H + |amount_v|)
   pixels (none with dst NULL: only dst_mask is made). A pixel no row or column reaches, or outside mask (w x h, 1 for a pixel of the shape;
   NULL for all), is `background`, and 0 in dst_mask where one is given, else 1. FALSE, with nothing written, if an allocation fails. */
bool easel_ss_make (const uint32_t *src, const uint8_t *mask, int w, int h, int W, int H, int amount_h, int amount_v, uint32_t background,
                    uint32_t *dst, uint8_t *dst_mask);

/* The whole picture stretched to W x H, then skewed by the amounts of easel_ss_size, as one undo step that keeps the old buffer: 4 bytes a
   pixel of it plus 64 (REQ-0019-14). What no row or column reaches is the background, exactly (REQ-0019-11). No size change and no amount
   is NOTHING, with no step. Past max_bytes, or if an allocation fails, NO_MEMORY with the picture and the history as they were
   (REQ-0019-15). W and H are from 1 to EASEL_SS_MAX_SIZE and the new size is within it. */
EaselTransformResult easel_image_stretch_skew (EaselImage *img, EaselHistory *h, int W, int H, int amount_h, int amount_v, uint32_t background, size_t max_bytes);
