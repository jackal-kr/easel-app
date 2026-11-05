/* SPDX-License-Identifier: GPL-3.0-or-later */
/* The cursor of the Brush and the Eraser (REQ-0006-16, AS-12): size, hotspot and pixels. */

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "check.h"
#include "cursor-shape.h"

#define BLACK 0xFF000000u
#define WHITE 0xFFFFFFFFu

/* The image as text, rows joined by '/': # black, o white, . transparent. */
static const char *
picture (EaselFootprint f, int zoom, int scale)
{
  static char text[200 * 201 + 1];
  EaselCursorShape s = easel_cursor_shape (f, zoom, scale);
  uint32_t *px = calloc ((size_t) s.width * s.height, sizeof *px);
  int n = 0;

  easel_cursor_render (f, zoom, scale, px);
  for (int y = 0; y < s.height; y++)
    {
      for (int x = 0; x < s.width; x++)
        {
          uint32_t c = px[y * s.width + x];

          text[n++] = c == BLACK ? '#' : c == WHITE ? 'o' : c == 0 ? '.' : '?';
        }
      if (y + 1 < s.height)
        text[n++] = '/';
    }
  text[n] = '\0';
  free (px);
  return text;
}

static void
as12_cursor_geometry (void)
{
  EaselCursorShape s;

  /* The Eraser (10) at 400 %: a 40 x 40 black outline with the white line outside it (42 x 42 in all); the pointer's
     pixel is column 4 of the footprint, its center 4 * 4 + 2 = 18 device pixels in, plus the white line. */
  s = easel_cursor_shape ((EaselFootprint) { EASEL_SHAPE_SQUARE, 10 }, 400, 1);
  CHECK (s.width == 42 && s.height == 42);
  CHECK (s.hot_x == 19 && s.hot_y == 19);

  /* The same on a screen with scale 2: device pixels, so twice as many. */
  s = easel_cursor_shape ((EaselFootprint) { EASEL_SHAPE_SQUARE, 10 }, 400, 2);
  CHECK (s.width == 82 && s.height == 82);
  CHECK (s.hot_x == 1 + 4 * 8 + 4 && s.hot_y == 1 + 4 * 8 + 4);

  /* The Brush "\" 8 at 100 %: the footprint's own size, the anchor is column 3 (its center 3.5, rounded down). */
  s = easel_cursor_shape ((EaselFootprint) { EASEL_SHAPE_BACKSLASH, 8 }, 100, 1);
  CHECK (s.width == 10 && s.height == 10);
  CHECK (s.hot_x == 4 && s.hot_y == 4);

  /* Never smaller than 5 x 5: the Eraser (8) at 10 % is 0.8 device pixels, so a 5 x 5 outline (7 x 7 with the white line)
     around the pointer's pixel, the hotspot at its center. */
  s = easel_cursor_shape ((EaselFootprint) { EASEL_SHAPE_SQUARE, 8 }, 10, 1);
  CHECK (s.width == 7 && s.height == 7);
  CHECK (s.hot_x == 3 && s.hot_y == 3);
  /* The edge: 4 device pixels get the minimum, 5 and 6 are their own size. */
  CHECK (easel_cursor_shape ((EaselFootprint) { EASEL_SHAPE_SQUARE, 4 }, 100, 1).width == 7);
  CHECK (easel_cursor_shape ((EaselFootprint) { EASEL_SHAPE_SQUARE, 5 }, 100, 1).width == 7);
  CHECK (easel_cursor_shape ((EaselFootprint) { EASEL_SHAPE_SQUARE, 6 }, 100, 1).width == 8);
  CHECK (easel_cursor_shape ((EaselFootprint) { EASEL_SHAPE_CIRCLE, 1 }, 400, 1).width == 7);   /* 4 device pixels */
  CHECK (easel_cursor_shape ((EaselFootprint) { EASEL_SHAPE_CIRCLE, 1 }, 800, 1).width == 10);  /* 8 */

  /* The pixels: black on the boundary of the footprint, white outside it next to the black, transparent elsewhere. */
  CHECK (strcmp (picture ((EaselFootprint) { EASEL_SHAPE_SQUARE, 5 }, 100, 1),
                 "ooooooo/o#####o/o#...#o/o#...#o/o#...#o/o#####o/ooooooo") == 0);
  CHECK (strcmp (picture ((EaselFootprint) { EASEL_SHAPE_SQUARE, 8 }, 10, 1),      /* the minimum, a plain square */
                 "ooooooo/o#####o/o#...#o/o#...#o/o#...#o/o#####o/ooooooo") == 0);
  CHECK (strcmp (picture ((EaselFootprint) { EASEL_SHAPE_SQUARE, 2 }, 400, 1),
                 "oooooooooo/o########o/o#......#o/o#......#o/o#......#o/o#......#o/o#......#o/o#......#o/o########o/oooooooooo") == 0);
  /* A staircase: at 100 % each pixel of the diagonal is its own boundary. */
  CHECK (strcmp (picture ((EaselFootprint) { EASEL_SHAPE_BACKSLASH, 8 }, 100, 1),
                 "ooo......./o#oo....../oo#oo...../.oo#oo..../..oo#oo.../...oo#oo../....oo#oo./.....oo#oo/......oo#o/.......ooo") == 0);
  /* At 200 % each pixel is 2 x 2 device pixels, the slash 5 is a staircase of squares. */
  CHECK (strcmp (picture ((EaselFootprint) { EASEL_SHAPE_SLASH, 5 }, 200, 1),
                 "........oooo/........o##o/......ooo##o/......o##ooo/....ooo##o../....o##ooo../..ooo##o..../..o##ooo..../"
                 "ooo##o....../o##ooo....../o##o......../oooo........") == 0);
}

int
main (void)
{
  RUN (as12_cursor_geometry);
  return 0;
}
