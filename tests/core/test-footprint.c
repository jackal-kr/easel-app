/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Footprints and presets of feature 0006 (REQ-0006-04, REQ-0006-05, REQ-0006-07, REQ-0006-10, REQ-0006-14). */

#include <string.h>

#include "check.h"
#include "footprint.h"

/* The mask as text, one line per row, # for a pixel of the footprint and . for none, rows joined by '/'. */
static const char *
mask (EaselShape shape, int size)
{
  static char text[24 * 25 + 1];
  EaselFootprint f = { shape, size };
  int n = 0;

  for (int row = 0; row < size; row++)
    {
      for (int col = 0; col < size; col++)
        text[n++] = easel_footprint_has (f, col, row) ? '#' : '.';
      if (row + 1 < size)
        text[n++] = '/';
    }
  text[n] = '\0';
  return text;
}

#define CHECK_MASK(shape, size, want) \
  do { \
    if (strcmp (mask (shape, size), want) != 0) \
      { \
        fprintf (stderr, "%s:%d: mask of %d is %s, want %s\n", __FILE__, __LINE__, size, mask (shape, size), want); \
        exit (1); \
      } \
  } while (0)

/* AS-3: the masks and the anchor of REQ-0006-05. */
static void
as3_footprint_masks (void)
{
  CHECK_MASK (EASEL_SHAPE_CIRCLE, 7, "..###../.#####./#######/#######/#######/.#####./..###..");
  CHECK_MASK (EASEL_SHAPE_CIRCLE, 4, ".##./####/####/.##.");
  CHECK_MASK (EASEL_SHAPE_CIRCLE, 1, "#");
  CHECK_MASK (EASEL_SHAPE_SQUARE, 2, "##/##");
  CHECK_MASK (EASEL_SHAPE_SQUARE, 8, "########/########/########/########/########/########/########/########");
  CHECK_MASK (EASEL_SHAPE_SLASH, 5, "....#/...#./..#../.#.../#....");
  CHECK_MASK (EASEL_SHAPE_BACKSLASH, 5, "#..../.#.../..#../...#./....#");
  CHECK_MASK (EASEL_SHAPE_SLASH, 2, ".#/#.");
  CHECK_MASK (EASEL_SHAPE_BACKSLASH, 1, "#");

  /* Outside the box nothing is part of it. */
  {
    EaselFootprint c4 = { EASEL_SHAPE_CIRCLE, 4 };

    CHECK (!easel_footprint_has (c4, -1, 1));
    CHECK (!easel_footprint_has (c4, 1, -1));
    CHECK (!easel_footprint_has (c4, 4, 1));
    CHECK (!easel_footprint_has (c4, 1, 4));
  }

  /* The pixel under the pointer is at column and row (size - 1) / 2, rounded down. */
  CHECK (easel_footprint_anchor (1) == 0);
  CHECK (easel_footprint_anchor (2) == 0);
  CHECK (easel_footprint_anchor (4) == 1);
  CHECK (easel_footprint_anchor (5) == 2);
  CHECK (easel_footprint_anchor (7) == 3);
  CHECK (easel_footprint_anchor (8) == 3);
  CHECK (easel_footprint_anchor (9) == 4);
  CHECK (easel_footprint_anchor (10) == 4);
  CHECK (easel_footprint_anchor (16) == 7);
  CHECK (easel_footprint_anchor (24) == 11);

  /* Every circle of the presets is the same under a half turn and under mirroring: it has no lean. */
  for (int size = 1; size <= 24; size++)
    {
      EaselFootprint c = { EASEL_SHAPE_CIRCLE, size };

      for (int row = 0; row < size; row++)
        for (int col = 0; col < size; col++)
          {
            CHECK (easel_footprint_has (c, col, row) == easel_footprint_has (c, size - 1 - col, size - 1 - row));
            CHECK (easel_footprint_has (c, col, row) == easel_footprint_has (c, size - 1 - col, row));
            CHECK (easel_footprint_has (c, col, row) == easel_footprint_has (c, row, col));
          }
    }
}

/* The presets, in the order of the requirements, and their defaults. */
static void
presets_are_the_ones_of_the_spec (void)
{
  static const struct { EaselShape shape; int size; } brush[12] = {
    { EASEL_SHAPE_CIRCLE, 7 }, { EASEL_SHAPE_CIRCLE, 4 }, { EASEL_SHAPE_CIRCLE, 1 },
    { EASEL_SHAPE_SQUARE, 8 }, { EASEL_SHAPE_SQUARE, 5 }, { EASEL_SHAPE_SQUARE, 2 },
    { EASEL_SHAPE_SLASH, 8 }, { EASEL_SHAPE_SLASH, 5 }, { EASEL_SHAPE_SLASH, 2 },
    { EASEL_SHAPE_BACKSLASH, 8 }, { EASEL_SHAPE_BACKSLASH, 5 }, { EASEL_SHAPE_BACKSLASH, 2 },
  };
  static const int airbrush[3] = { 9, 16, 24 }, eraser[4] = { 4, 6, 8, 10 };

  CHECK (EASEL_BRUSH_N_PRESETS == 12 && EASEL_AIRBRUSH_N_PRESETS == 3 && EASEL_ERASER_N_SIZES == 4);
  for (int i = 0; i < 12; i++)
    CHECK (easel_brush_presets[i].shape == brush[i].shape && easel_brush_presets[i].size == brush[i].size);
  for (int i = 0; i < 3; i++)
    CHECK (easel_airbrush_presets[i].shape == EASEL_SHAPE_CIRCLE && easel_airbrush_presets[i].size == airbrush[i]);
  for (int i = 0; i < 4; i++)
    CHECK (easel_eraser_presets[i].shape == EASEL_SHAPE_SQUARE && easel_eraser_presets[i].size == eraser[i]);
  CHECK (easel_brush_presets[EASEL_BRUSH_DEFAULT].shape == EASEL_SHAPE_CIRCLE
         && easel_brush_presets[EASEL_BRUSH_DEFAULT].size == 4);
  CHECK (easel_airbrush_presets[EASEL_AIRBRUSH_DEFAULT].size == 16);
  CHECK (easel_eraser_presets[EASEL_ERASER_DEFAULT].size == 8);
}

/* AS-10: Numpad + and - go to the next larger or smaller preset of the same shape; the ends stay. */
static void
as10_preset_steps (void)
{
  const EaselFootprint *b = easel_brush_presets;

  /* Circles: 7 (0), 4 (1), 1 (2). */
  CHECK (easel_preset_step (b, 12, 1, +1) == 0);
  CHECK (easel_preset_step (b, 12, 0, +1) == 0);
  CHECK (easel_preset_step (b, 12, 1, -1) == 2);
  CHECK (easel_preset_step (b, 12, 2, -1) == 2);
  /* Squares: 8 (3), 5 (4), 2 (5); the shape never changes. */
  CHECK (easel_preset_step (b, 12, 4, +1) == 3);
  CHECK (easel_preset_step (b, 12, 3, +1) == 3);
  CHECK (easel_preset_step (b, 12, 4, -1) == 5);
  CHECK (easel_preset_step (b, 12, 5, -1) == 5);
  /* Slashes (6, 7, 8) and backslashes (9, 10, 11). */
  CHECK (easel_preset_step (b, 12, 7, +1) == 6);
  CHECK (easel_preset_step (b, 12, 7, -1) == 8);
  CHECK (easel_preset_step (b, 12, 10, +1) == 9);
  CHECK (easel_preset_step (b, 12, 10, -1) == 11);
  CHECK (easel_preset_step (b, 12, 8, -1) == 8);
  CHECK (easel_preset_step (b, 12, 9, +1) == 9);

  /* The airbrush: 9 (0), 16 (1), 24 (2), larger to the right. */
  CHECK (easel_preset_step (easel_airbrush_presets, 3, 1, +1) == 2);
  CHECK (easel_preset_step (easel_airbrush_presets, 3, 2, +1) == 2);
  CHECK (easel_preset_step (easel_airbrush_presets, 3, 1, -1) == 0);
  CHECK (easel_preset_step (easel_airbrush_presets, 3, 0, -1) == 0);

  /* The eraser: 4 (0), 6 (1), 8 (2), 10 (3). */
  CHECK (easel_preset_step (easel_eraser_presets, 4, 2, +1) == 3);
  CHECK (easel_preset_step (easel_eraser_presets, 4, 3, +1) == 3);
  CHECK (easel_preset_step (easel_eraser_presets, 4, 2, -1) == 1);
  CHECK (easel_preset_step (easel_eraser_presets, 4, 0, -1) == 0);

  /* A step is to the NEXT preset, not to the end of the row. */
  CHECK (easel_preset_step (b, 12, 2, +1) == 1);
  CHECK (easel_preset_step (b, 12, 0, -1) == 1);
  CHECK (easel_preset_step (b, 12, 11, +1) == 10);
  CHECK (easel_preset_step (easel_airbrush_presets, 3, 0, +1) == 1);
  CHECK (easel_preset_step (easel_airbrush_presets, 3, 2, -1) == 1);
  CHECK (easel_preset_step (easel_eraser_presets, 4, 0, +1) == 1);
  CHECK (easel_preset_step (easel_eraser_presets, 4, 3, -1) == 2);

  /* Nonsense stays where it is. */
  CHECK (easel_preset_step (b, 12, 1, 0) == 1);
  CHECK (easel_preset_step (b, 12, -1, +1) == -1);
  CHECK (easel_preset_step (b, 12, 12, -1) == 12);
}

int
main (void)
{
  RUN (as3_footprint_masks);
  RUN (presets_are_the_ones_of_the_spec);
  RUN (as10_preset_steps);
  return 0;
}
