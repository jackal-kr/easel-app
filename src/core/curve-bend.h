/* SPDX-License-Identifier: GPL-3.0-or-later */
/* The bends of feature 0033: a curve bent by dragging points of it. Where a press grabs the curve (REQ-0033-02), which bend the
   grab makes or moves (REQ-0033-04), and the control points that make the curve pass by its bends (REQ-0033-05). Every value is
   exact: a place t is a whole number of 1/4096, a point a whole number of 1/2^36 picture pixels, so the same input gives the
   same control points on every machine (P7.4). */
#pragma once

#include <stdbool.h>
#include <stdint.h>

#define EASEL_CURVE_STEPS 4096                           /* a place is t / EASEL_CURVE_STEPS */
#define EASEL_CURVE_ONE ((int64_t) 1 << 36)             /* a point's coordinate is v / EASEL_CURVE_ONE pixels */

/* A bend: its point P (x, y over EASEL_CURVE_ONE); its place comes from where P lies along the line (REQ-0033-04). */
typedef struct
{
  int64_t x, y;
} EaselCurveBend;

typedef enum
{
  EASEL_CURVE_NEW,        /* the grab makes a new bend */
  EASEL_CURVE_MOVE,       /* the grab moves the point of an existing bend */
  EASEL_CURVE_REPLACE     /* the grab replaces an existing bend */
} EaselCurveGrab;

/* The grid M of REQ-0033-02: the smallest power of two that is at least N and at least 64, at most 4096; N is REQ-0011-05's
   number for the curve pts (start, two control points, end), or, for a straight line (no bend yet), the distance from the
   start to the end rounded up. */
int easel_curve_grid (const int pts[8], bool straight);

/* The point B (t / 4096) of the curve pts, exactly (over EASEL_CURVE_ONE); for a straight line, start + t (end - start). */
void easel_curve_point (const int pts[8], bool straight, int t, int64_t *x, int64_t *y);

/* The place a press on the pixel (px, py) grabs (REQ-0033-02): of the points B (i / M), i from 0 to M, the one nearest the
   pixel's center (the smallest i on a tie), its place limited to 1/8 to 7/8: 512 to 3584. */
int easel_curve_grab (const int pts[8], bool straight, int px, int py);

/* A bend's place (REQ-0033-04), in 1/4096: where its point lies along the line from (sx, sy) to (ex, ey), rounded (a half up)
   and limited to 512 to 3584; 2048 when the start and the end are one pixel. */
int easel_curve_place (int sx, int sy, int ex, int ey, const EaselCurveBend *b);

/* The places of the n (1 or 2) bends b, in their order: two less than 512 apart are moved 512 apart round their middle (rounded
   down, limited to 768 to 3328). */
void easel_curve_places (int sx, int sy, int ex, int ey, const EaselCurveBend *b, int n, int t[2]);

/* The n bends b put in the order of their places (a tie keeps their order). */
void easel_curve_order (int sx, int sy, int ex, int ey, EaselCurveBend *b, int n);

/* What a grab at the place t does with the n (0 to 2) bends b, in their order (REQ-0033-04); *which gets the bend moved or
   replaced (0 or 1); for NEW it is n, the new bend's index before the bends are put in order. */
EaselCurveGrab easel_curve_bend_rule (int sx, int sy, int ex, int ey, const EaselCurveBend *b, int n, int t, int *which);

/* The two control points (out: C1 x, y, C2 x, y) for the start (sx, sy), the end (ex, ey) and the n (1 or 2) bends b, in their
   order (REQ-0033-05): one bend the curve's peak, two at their places; exact, then each coordinate rounded (a half toward
   larger) and limited to -1,000,000 to 1,000,000. */
void easel_curve_controls (int sx, int sy, int ex, int ey, const EaselCurveBend *b, int n, int out[4]);
