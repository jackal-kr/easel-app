/* SPDX-License-Identifier: GPL-3.0-or-later */
/* The path of a free-form selection as a line along the grid lines of the picture (feature 0014, REQ-0014-04, REQ-0014-05). A corner
   (x, y) is the top-left corner of the pixel (x, y). Between two corners the line follows the 8-connected line that the Pencil draws
   between them (REQ-0001-20), taken on the corners, each diagonal step made of a horizontal step and then a vertical one, so that
   every step runs along a grid line. */
#pragma once

#include <stdbool.h>
#include <stddef.h>

#include "line.h"

typedef struct
{
  EaselLine line;
  bool started, has_pending;
  int px, py, pend_x, pend_y;
} EaselLattice;

/* Starts the walk from the corner (x0, y0) to the corner (x1, y1). */
void easel_lattice_init (EaselLattice *w, int x0, int y0, int x1, int y1);

/* The next corner of the walk, in order, the first and the last included; FALSE after the last. Two corners that follow one another
   are one grid segment apart. */
bool easel_lattice_next (EaselLattice *w, int *x, int *y);

/* A black dash: a straight piece along a grid line, from (x0, y0) to (x1, y1) in picture pixels, in the way along the path. */
typedef struct
{
  double x0, y0, x1, y1;
} EaselDash;

/* The black dashes along the path through the n corners (x, y pairs), closed back to the first if closed: a dash of `dash` picture
   pixels, then as long a gap, and so on, counted from the first corner as one continuous line (a retraced segment counts again),
   black first, the last cut where the path ends; pieces that continue one another are one. `phase` is how far the pattern has
   advanced along the line, in picture pixels, so that the dashes move on as it grows (0: the first dash starts the line). Their
   number; the first max are written to out (out may be NULL). */
size_t easel_lattice_dashes (const int *xy, size_t n, bool closed, double dash, double phase, EaselDash *out, size_t max);

/* The straight runs of the path through the n corners (closed: back to the first), for its white line: the grid steps it walks, those
   that go on the same way along the same axis one after another joined into one run from (x0, y0) to (x1, y1), in the path's order.
   Their number; the first max are written to out (out may be NULL). */
size_t easel_lattice_runs (const int *xy, size_t n, bool closed, EaselDash *out, size_t max);
