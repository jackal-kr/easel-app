/* SPDX-License-Identifier: GPL-3.0-or-later */

#include "footprint.h"

const EaselFootprint easel_brush_presets[EASEL_BRUSH_N_PRESETS] = {
  { EASEL_SHAPE_CIRCLE, 7 }, { EASEL_SHAPE_CIRCLE, 4 }, { EASEL_SHAPE_CIRCLE, 1 },
  { EASEL_SHAPE_SQUARE, 8 }, { EASEL_SHAPE_SQUARE, 5 }, { EASEL_SHAPE_SQUARE, 2 },
  { EASEL_SHAPE_SLASH, 8 }, { EASEL_SHAPE_SLASH, 5 }, { EASEL_SHAPE_SLASH, 2 },
  { EASEL_SHAPE_BACKSLASH, 8 }, { EASEL_SHAPE_BACKSLASH, 5 }, { EASEL_SHAPE_BACKSLASH, 2 },
};

const EaselFootprint easel_airbrush_presets[EASEL_AIRBRUSH_N_PRESETS] = {
  { EASEL_SHAPE_CIRCLE, 9 }, { EASEL_SHAPE_CIRCLE, 16 }, { EASEL_SHAPE_CIRCLE, 24 },
};

const EaselFootprint easel_eraser_presets[EASEL_ERASER_N_SIZES] = {
  { EASEL_SHAPE_SQUARE, 4 }, { EASEL_SHAPE_SQUARE, 6 }, { EASEL_SHAPE_SQUARE, 8 }, { EASEL_SHAPE_SQUARE, 10 },
};

bool
easel_footprint_has (EaselFootprint f, int col, int row)
{
  if (col < 0 || row < 0 || col >= f.size || row >= f.size)
    return false;
  switch (f.shape)
    {
    case EASEL_SHAPE_SQUARE:
      return true;
    case EASEL_SHAPE_SLASH:
      return row == f.size - 1 - col;
    case EASEL_SHAPE_BACKSLASH:
      return row == col;
    case EASEL_SHAPE_CIRCLE:
      {
        /* Pixel centers are at (col + 0.5, row + 0.5) and the box's center at (size / 2, size / 2); scaled by 2 so that
           everything stays in whole numbers: no center lies exactly on the circle, so < or <= give the same masks. */
        long dx = 2L * col + 1 - f.size, dy = 2L * row + 1 - f.size;

        return dx * dx + dy * dy < (long) f.size * f.size;
      }
    }
  return false;
}

int
easel_footprint_anchor (int size)
{
  return size > 0 ? (size - 1) / 2 : 0;
}

int
easel_preset_step (const EaselFootprint *table, int n, int index, int direction)
{
  int best = index;

  if (index < 0 || index >= n || direction == 0)
    return index;
  for (int i = 0; i < n; i++)
    {
      if (table[i].shape != table[index].shape)
        continue;
      if (direction > 0 && table[i].size > table[index].size
          && (best == index || table[i].size < table[best].size))
        best = i;
      if (direction < 0 && table[i].size < table[index].size
          && (best == index || table[i].size > table[best].size))
        best = i;
    }
  return best;
}
