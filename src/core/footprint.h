/* SPDX-License-Identifier: GPL-3.0-or-later */
/* The footprints of the Brush, the Airbrush and the Eraser, and their presets (REQ-0006-04, REQ-0006-05, REQ-0006-07,
   REQ-0006-10, REQ-0006-14). */
#pragma once

#include <stdbool.h>

typedef enum
{
  EASEL_SHAPE_CIRCLE,
  EASEL_SHAPE_SQUARE,
  EASEL_SHAPE_SLASH,       /* the diagonal from bottom-left to top-right */
  EASEL_SHAPE_BACKSLASH,   /* the diagonal from top-left to bottom-right */
} EaselShape;

typedef struct
{
  EaselShape shape;
  int size;                /* the side of the box, in picture pixels */
} EaselFootprint;

/* Is the pixel at column col, row row of the size x size box part of the footprint? A circle is the pixels whose centers
   lie within size / 2 of the box's center; a square is the whole box; a slash is row = size - 1 - col; a backslash is
   row = col. Outside the box: false. */
bool easel_footprint_has (EaselFootprint f, int col, int row);

/* The footprint is placed with the pixel under the pointer at this column and row of its box: (size - 1) / 2, rounded down. */
int easel_footprint_anchor (int size);

/* The presets, in the order of the options panel (two columns, row by row). */
enum
{
  EASEL_BRUSH_N_PRESETS = 12,
  EASEL_BRUSH_DEFAULT = 1,       /* the 4-pixel circle */
  EASEL_AIRBRUSH_N_PRESETS = 3,
  EASEL_AIRBRUSH_DEFAULT = 1,    /* the disc of 16 */
  EASEL_ERASER_N_SIZES = 4,
  EASEL_ERASER_DEFAULT = 2,      /* the square of 8 */
};
extern const EaselFootprint easel_brush_presets[EASEL_BRUSH_N_PRESETS];
extern const EaselFootprint easel_airbrush_presets[EASEL_AIRBRUSH_N_PRESETS];
extern const EaselFootprint easel_eraser_presets[EASEL_ERASER_N_SIZES];

/* Numpad + (direction > 0) or - (direction < 0): the index of the next larger or smaller preset of the same shape in the
   table of n presets, or index itself if there is none (the ends stay), if direction is 0 or index is not in the table. */
int easel_preset_step (const EaselFootprint *table, int n, int index, int direction);
