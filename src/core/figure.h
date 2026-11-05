/* SPDX-License-Identifier: GPL-3.0-or-later */
/* The figures of Line, Rectangle, Rounded Rectangle and Ellipse (REQ-0010-10 to REQ-0010-14): which pixels of each picture
   row a figure covers, and which of them are its outline and which its inside. One description serves the preview and the
   drawing, so they cannot differ. */
#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "history.h"
#include "image.h"

typedef enum
{
  EASEL_FIGURE_LINE,
  EASEL_FIGURE_RECTANGLE,
  EASEL_FIGURE_ROUNDED,
  EASEL_FIGURE_ELLIPSE,
  EASEL_FIGURE_CURVE,              /* 0011: the four points of pts, the curve of REQ-0011-05 */
  EASEL_FIGURE_POLYGON,            /* 0011: the n_pts corners of pts; closed, it has the last side and an inside (REQ-0011-08) */
} EaselFigureKind;

typedef enum
{
  EASEL_FIGURE_OUTLINE,            /* the outline only */
  EASEL_FIGURE_OUTLINE_AND_FILL,   /* the outline and the inside in the other color */
  EASEL_FIGURE_FILL_ONLY,          /* the whole figure in the drawing color */
} EaselFigureStyle;

/* From the start pixel (x0, y0) to the end pixel (x1, y1), both included; width is 1 to 5. */
typedef struct
{
  EaselFigureKind kind;
  int x0, y0, x1, y1;
  int width;
  EaselFigureStyle style;
  void *rows;              /* the Line's or the curve's rows, made by easel_figure_prepare; NULL until then and for the other kinds */
  const int *pts;          /* a curve: the start, the two control points and the end, as x, y pairs (8 ints); x0..y1 are unused */
  size_t n_pts;            /* the number of points in pts (4 for a curve) */
  bool closed;             /* a polygon: closed, with a last side back to the first corner */
} EaselFigure;

/* A run of pixels of one row, x0 to x1 both included: the figure's outline or, with inside set, its inside. */
typedef struct
{
  int x0, x1;
  bool inside;
} EaselFigureRun;

/* A Line needs its rows once before easel_figure_row: the Pencil's 8-connected line from the start to the end, kept for the
   picture rows y_from to y_to (plus the pen's reach), so that a row is then answered in constant time. False if the memory
   cannot be had. A no-op for the other kinds. easel_figure_release frees what prepare made; a figure that was copied
   shares it, so only one copy is released. */
bool easel_figure_prepare (EaselFigure *f, int y_from, int y_to);
void easel_figure_release (EaselFigure *f);

/* Shift (REQ-0010-14): the end (*x1, *y1) the figure takes when Shift is held, the pointer being at (*x1, *y1) on entry and
   the start at (x0, y0). A Line goes horizontally, vertically or at 45 degrees, whichever of the eight directions is nearest
   by angle to the line from the start to the pointer, the end being the start moved along it by the pointer's distance
   (for 45 degrees the smaller of the horizontal and vertical distances); the other kinds give a square or a circle of the
   smaller side of the box (both pixels included), from the start toward the pointer. */
void easel_figure_constrain (EaselFigureKind kind, int x0, int y0, int *x1, int *y1);

/* An upper bound of the runs one row of the figure can have: the size of the buffer a caller gives easel_figure_row. For a
   curve it is exact only after easel_figure_prepare, so a caller prepares first. */
size_t easel_figure_max_runs (const EaselFigure *f);

/* The box of a figure: it runs from the outer corner of the start pixel to the outer corner of the end pixel, so both
   pixels are in it (a start (10, 10) and an end (19, 14) give x 10, y 10, w 10, h 5). Not for Line. */
void easel_figure_bounds (const EaselFigure *f, int *x, int *y, int *w, int *h);

/* The runs of the row y, left to right, at most max (3 are enough: the outline's left part, the inside, the outline's
   right part; a Line has one run, all outline, its pen included); none for a row the figure does not cover. The pixels
   are not clipped to any picture. Coordinates are limited to +-1,000,000. Returns the number of runs. */
size_t easel_figure_row (const EaselFigure *f, int y, EaselFigureRun *runs, size_t max);

typedef enum
{
  EASEL_FIGURE_DONE,         /* drawn and, with a history, one undo step */
  EASEL_FIGURE_NOTHING,      /* no pixel changed (outside the picture, or already of the colors): no step */
  EASEL_FIGURE_NO_MEMORY     /* the picture is as it was and no step was added */
} EaselFigureResult;

/* Draws the figure into the picture (REQ-0010-05 to REQ-0010-07, REQ-0010-13): the outline in `draw`; the inside, by the
   style, kept (outline only), in `other` (outline and fill) or in `draw` (fill only); a Line wholly in `draw`. Only the
   pixels inside the picture; exactly the colors, no blending. With a history the changed pixels become one undo step
   (12 bytes a pixel plus 64, within REQ-0002-16; a step larger than the budget is kept, REQ-0002-17); without one they are
   just drawn. *changed, if not NULL, gets the number of pixels changed (0 unless DONE). */
EaselFigureResult easel_figure_apply (EaselImage *img, EaselHistory *history, const EaselFigure *f, uint32_t draw,
                                      uint32_t other, size_t *changed);

/* Test hook: the same, but it gives up with NO_MEMORY when the log of changed pixels would need more than max_bytes. */
EaselFigureResult easel_figure_apply_capped (EaselImage *img, EaselHistory *history, const EaselFigure *f, uint32_t draw,
                                             uint32_t other, size_t *changed, size_t max_bytes);
