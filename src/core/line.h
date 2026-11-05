/* SPDX-License-Identifier: GPL-3.0-or-later */
/* The Pencil's line (REQ-0001-20): the 8-connected line between two pixels, both ends included, one pixel per step along
   the major axis. The path of a free-form selection (REQ-0014-04) and the trail of a smear (REQ-0014-11) walk it. */
#pragma once

#include <stdbool.h>

typedef struct
{
  int x, y, x1, y1, dx, dy, sx, sy;
  long long err;
  bool done;
} EaselLine;

/* Starts the walk from (x0, y0) to (x1, y1). */
void easel_line_init (EaselLine *l, int x0, int y0, int x1, int y1);

/* The next pixel of the line, in order: FALSE after the last one. */
bool easel_line_next (EaselLine *l, int *x, int *y);
