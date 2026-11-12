/* SPDX-License-Identifier: GPL-3.0-or-later */
/* The shapes of Shapes (0043): Rectangle, Rounded Rectangle and Ellipse, drawn by their own figures (REQ-0043-18), and 16 shapes
   drawn as the polygon through their corners (REQ-0043-04, REQ-0043-05), whose table is docs/reference/proto-0043/corners.md. A
   shape and its box give the EaselFigure that draws it, so the preview and the put-down are figure.c's (P7.3). */
#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "figure.h"

/* In the order of the popover (REQ-0043-02). */
typedef enum
{
  EASEL_SHAPE_KIND_RECTANGLE,
  EASEL_SHAPE_KIND_ROUNDED_RECTANGLE,
  EASEL_SHAPE_KIND_ELLIPSE,
  EASEL_SHAPE_KIND_TRIANGLE,
  EASEL_SHAPE_KIND_RIGHT_TRIANGLE,
  EASEL_SHAPE_KIND_DIAMOND,
  EASEL_SHAPE_KIND_PENTAGON,
  EASEL_SHAPE_KIND_HEXAGON,
  EASEL_SHAPE_KIND_RIGHT_ARROW,
  EASEL_SHAPE_KIND_LEFT_ARROW,
  EASEL_SHAPE_KIND_UP_ARROW,
  EASEL_SHAPE_KIND_DOWN_ARROW,
  EASEL_SHAPE_KIND_STAR_4,
  EASEL_SHAPE_KIND_STAR_5,
  EASEL_SHAPE_KIND_STAR_6,
  EASEL_SHAPE_KIND_ROUNDED_CALLOUT,
  EASEL_SHAPE_KIND_OVAL_CALLOUT,
  EASEL_SHAPE_KIND_HEART,
  EASEL_SHAPE_KIND_LIGHTNING_BOLT,
  EASEL_SHAPE_KIND_COUNT
} EaselShapeKind;

/* The most corners a shape has (the Heart's). */
#define EASEL_SHAPE_MAX_CORNERS 96
/* The largest side of a shape's box, in pixels (REQ-0043-04). */
#define EASEL_SHAPE_MAX_SIDE 10000

/* The shape's name as the win.shape action's target ("rectangle", "rounded-rectangle", "ellipse", "triangle", "right-triangle",
   "diamond", "pentagon", "hexagon", "right-arrow", "left-arrow", "up-arrow", "down-arrow", "4-point-star", "5-point-star",
   "6-point-star", "rounded-rectangle-callout", "oval-callout", "heart", "lightning-bolt"); NULL outside the list. */
const char *easel_shape_name (EaselShapeKind shape);
/* The shape of a name, false for a name not in the list. */
bool easel_shape_from_name (const char *name, EaselShapeKind *shape);

/* The corners of one of the 16 polygon shapes, as (u, v) pairs in thousandths of the box (u from its left, v from its top), in
   corners.md's order: *uv points at 2 n values; 0 and NULL for Rectangle, Rounded Rectangle and Ellipse. */
size_t easel_shape_corners (EaselShapeKind shape, const int16_t **uv);

/* The box of a drag from the start pixel (x0, y0) to the end pixel (x1, y1), both inside it (REQ-0043-04): with shift a square of
   side the smaller of its width and height, from the start toward the end; each side at most EASEL_SHAPE_MAX_SIDE, the end
   stopping there. A drag toward smaller x or y gives the same box as the other way. */
void easel_shape_box (int x0, int y0, int x1, int y1, bool shift, int *x, int *y, int *w, int *h);

/* The corner's place on a side of n pixels (REQ-0043-04): p (u, n) = floor ((u (n - 1) + 500) / 1000) for u <= 500, else
   (n - 1) - floor (((1000 - u) (n - 1) + 500) / 1000); u 0 to 1000, n 1 to EASEL_SHAPE_MAX_SIDE. */
int easel_shape_place (int u, int n);

/* The figure that draws the shape in the box (x, y, w, h) (w, h 1 to EASEL_SHAPE_MAX_SIDE) with the line width (1 to 5), the fill
   style and Smooth: Rectangle, Rounded Rectangle and Ellipse their own kind from (x, y) to (x + w - 1, y + h - 1), the mirror flags
   ignored (REQ-0043-18); the others a closed polygon through their corners placed in the box made smaller by the width
   (REQ-0043-04), u read as 1000 - u when mirror_x, v as 1000 - v when mirror_y, written to pts (2 * EASEL_SHAPE_MAX_CORNERS ints),
   which the figure points at. */
EaselFigure easel_shape_figure (EaselShapeKind shape, int x, int y, int w, int h, int width, EaselFigureStyle style, bool smooth,
                                bool mirror_x, bool mirror_y, int *pts);
