/* SPDX-License-Identifier: GPL-3.0-or-later */
/* The path of a free-form selection and its shape (feature 0014, REQ-0014-04, -06, -15): the corners of the picture's grid nearest to
   the pointer positions are joined by lines along the grid lines (lattice.h) and closed from the last to the first; the shape is the
   pixels that the path encloses (the union of its loops), limited to the picture. */
#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "selection.h"

/* The corners of the grid the pointer reached (x, y pairs), in order; 8 bytes each. */
typedef struct
{
  int *xy;
  size_t n, cap;
} EaselPath;

/* Adds a position. FALSE, with the path as it was, if it would need more than max_bytes or cannot grow. */
bool easel_path_add (EaselPath *p, int x, int y, size_t max_bytes);

void easel_path_free (EaselPath *p);

/* The path of a shape stretched (feature 0015, REQ-0015-06): the corners of src, which traced the shape box (sx, sy, sw, sh), as they lie
   when the shape is the box (nx, ny, nw, nh). A corner inside the box [sx, sx + sw] keeps its relative place (halves go to the larger
   x or y), one outside keeps its distance from the nearest edge. FALSE, with out untouched, if the copy would need more than max_bytes. */
bool easel_lasso_scale_path (const EaselPath *src, int sx, int sy, int sw, int sh, int nx, int ny, int nw, int nh, EaselPath *out,
                             size_t max_bytes);

/* Makes the shape of the closed path over a picture of img_w x img_h: the bounding rectangle (x, y, w, h) of the pixels of the
   shape that are inside the picture, and their mask (w x h bytes, 1 inside the shape; the caller frees it with free ()). NOTHING,
   with no mask, for a path of one corner (a click), a path that encloses no pixel (a straight drag, an out and back) or a shape with
   no pixel inside the picture; NO_MEMORY, with no mask, if the
   memory the work holds at once (the path's pixels and rows, the mask) would exceed max_bytes or cannot be had. */
EaselSelectionResult easel_lasso_shape (const EaselPath *p, int img_w, int img_h, int *x, int *y, int *w, int *h,
                                        uint8_t **mask, size_t max_bytes);
