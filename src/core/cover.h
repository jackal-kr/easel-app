/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Smooth edges (0039): how many of a pixel's 256 sample points a figure's parts cover, and the color a partly covered pixel
   takes (REQ-0039-04, REQ-0039-06). */
#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "footprint.h"

/* The color of a pixel whose old color is old (0xAARRGGBB, straight alpha) when n_out of its 256 points are covered by a part of
   color c_out and n_in by a part of color c_in (n_out + n_in <= 256): each color weighted by its points, the old one by the rest, in
   premultiplied alpha, halves rounded up; an alpha of 0 gives 0x00000000 (REQ-0039-06, P7.7). */
uint32_t easel_cover_mix (uint32_t old, uint32_t c_out, unsigned n_out, uint32_t c_in, unsigned n_in);

/* The same mix for a coverage c from 0 to 255 by one color (REQ-0040-04, smooth text): with old (r, g, b, a) and color (r_t, g_t, b_t,
   a_t), M = a (255 - c) + a_t c, the alpha round (M / 255), the red round ((r a (255 - c) + r_t a_t c) / M), at most 255, halves up;
   an alpha of 0 gives 0x00000000. c = 255 gives color, c = 0 old. */
uint32_t easel_cover_mix255 (uint32_t old, uint32_t color, unsigned c);

/* A run of pixels x0 to x1 of one row, each with out of its points in the figure's outline and in in its inside (0 to 256 each,
   out + in at most 256); a pixel no part covers is in no run. */
typedef struct
{
  int x0, x1;
  uint16_t out, in;
} EaselCoverRun;

/* Sample columns a to b, both included, of one sample row: column c is the point 16 x + i of pixel x (i from 0 to 15), at
   (32 x + 2 i + 1) / 32. part is 0 for the outline, 1 for the inside. */
typedef struct
{
  int64_t a, b;
  int part;
} EaselCoverSpan;

/* Where a span starts or ends, for easel_cover_count's sort. */
typedef struct
{
  int64_t at;
  int part, delta;
} EaselCoverEvent;

/* The runs of one picture row from the spans of its 16 sample rows (n spans in all; within one sample row the spans do not
   overlap, the outline's and the inside's included). scratch has room for 2 n events. At most max runs (2 n + 1 are enough);
   returns their number, or 0 if max is too small. */
size_t easel_cover_count (const EaselCoverSpan *spans, size_t n, EaselCoverEvent *scratch, EaselCoverRun *runs, size_t max);

/* The pieces figures are made of, in 1/32 pixel (a pixel's center at 32 x + 16), for the sample row whose points are at q (odd):
   true and the sample columns *ca to *cb of the points of that row inside the piece or on its edge, false if none.
   A band: the points within h of the segment from a to b whose foot falls on it; end_a, end_b add the square ends reaching h beyond
   a and b (REQ-0039-05 (b)); a segment of no length with both ends is the square of side 2 h, without them nothing. */
bool easel_cover_band (int64_t ax, int64_t ay, int64_t bx, int64_t by, int64_t h, bool end_a, bool end_b, int64_t q, int64_t *ca,
                       int64_t *cb);
/* A disk: the points within r of c. */
bool easel_cover_disk (int64_t cx, int64_t cy, int64_t r, int64_t q, int64_t *ca, int64_t *cb);

/* The Brush with Smooth (REQ-0039-05 (e)): the footprint's shape placed at pointer pixel (x0, y0) as REQ-0006-05 places it and swept
   in a straight line to (x1, y1) (the same pixel: one stamp); true and the sample columns *ca to *cb of the sample row q (1/32 pixel)
   inside it, false if none. The circle of 1 is its square. */
bool easel_cover_sweep (EaselFootprint f, int x0, int y0, int x1, int y1, int64_t q, int64_t *ca, int64_t *cb);

/* The points, 0 to 256, of the pixel at column col and row row of the footprint's box that one click covers (REQ-0039-11). */
unsigned easel_cover_footprint (EaselFootprint f, int col, int row);
