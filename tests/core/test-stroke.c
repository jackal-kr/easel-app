/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Pencil stroke scenarios of feature 0001. */

#include <stdlib.h>
#include <string.h>

#include "check.h"
#include "footprint.h"
#include "image.h"
#include "history.h"
#include "stroke.h"

#define WHITE 0xFFFFFFFFu
#define BLACK 0xFF000000u

static int
count (const EaselImage *img, uint32_t color)
{
  int n = 0;

  for (int y = 0; y < img->height; y++)
    for (int x = 0; x < img->width; x++)
      n += easel_image_get (img, x, y) == color;
  return n;
}

static void
as16_click_sets_one_pixel (void)
{
  EaselImage *img = easel_image_new (100, 100, WHITE);
  EaselStroke *s = easel_stroke_begin (img, 10, 10, BLACK);

  CHECK (easel_stroke_end (s, NULL) == 1);
  CHECK (easel_image_get (img, 10, 10) == BLACK);
  CHECK (count (img, BLACK) == 1);
  easel_image_free (img);
}

/* P7.7 (2026-11-10, ADR-0008): a fully transparent color draws 0x00000000, whatever red, green and blue it holds. */
static void
transparent_color_is_zero (void)
{
  EaselImage *img = easel_image_new (20, 10, WHITE);
  EaselStroke *s = easel_stroke_begin (img, 2, 5, 0x00FF0000u);

  easel_stroke_to (s, 12, 5);
  easel_stroke_end (s, NULL);
  CHECK (easel_image_get (img, 2, 5) == 0x00000000u && easel_image_get (img, 12, 5) == 0x00000000u);
  CHECK (count (img, 0x00FF0000u) == 0);
  easel_image_free (img);
}

static void
as18_line_is_8_connected (void)
{
  EaselImage *img = easel_image_new (30, 10, WHITE);
  EaselStroke *s = easel_stroke_begin (img, 0, 0, BLACK);
  int prev_y = 0;

  easel_stroke_to (s, 20, 5);
  CHECK (easel_stroke_end (s, NULL) == 21);
  CHECK (count (img, BLACK) == 21);
  CHECK (count (img, WHITE) == 30 * 10 - 21);
  CHECK (easel_image_get (img, 0, 0) == BLACK);
  CHECK (easel_image_get (img, 20, 5) == BLACK);

  /* One pixel per column, each step moves y by at most 1: 8-connected and 1 pixel wide. */
  for (int x = 0; x <= 20; x++)
    {
      int ys = 0, y_at = -1;

      for (int y = 0; y < 10; y++)
        if (easel_image_get (img, x, y) == BLACK)
          {
            ys++;
            y_at = y;
          }
      CHECK (ys == 1);
      CHECK (abs (y_at - prev_y) <= 1);
      prev_y = y_at;
    }
  easel_image_free (img);
}

static void
as19_secondary_uses_background (void)
{
  EaselImage *img = easel_image_new (20, 5, BLACK);
  EaselStroke *s = easel_stroke_begin (img, 0, 0, WHITE);

  easel_stroke_to (s, 9, 0);
  easel_stroke_end (s, NULL);
  for (int x = 0; x <= 9; x++)
    CHECK (easel_image_get (img, x, 0) == WHITE);
  CHECK (count (img, WHITE) == 10);
  easel_image_free (img);
}

static void
as20_clipped_line (void)
{
  EaselImage *img = easel_image_new (100, 100, WHITE);
  EaselStroke *s = easel_stroke_begin (img, 50, 50, BLACK);

  easel_stroke_to (s, 150, 50);
  CHECK (easel_stroke_end (s, NULL) == 50);
  for (int x = 50; x <= 99; x++)
    CHECK (easel_image_get (img, x, 50) == BLACK);
  CHECK (count (img, BLACK) == 50);
  easel_image_free (img);

  img = easel_image_new (100, 100, WHITE);
  s = easel_stroke_begin (img, -10, 50, BLACK);
  easel_stroke_to (s, 10, 50);
  CHECK (easel_stroke_end (s, NULL) == 11);
  for (int x = 0; x <= 10; x++)
    CHECK (easel_image_get (img, x, 50) == BLACK);
  CHECK (count (img, BLACK) == 11);

  /* Leave and re-enter across a corner: only inside pixels; absurd coordinates do not hang. */
  s = easel_stroke_begin (img, 5, 5, BLACK);
  easel_stroke_to (s, -2000000000, 5);
  easel_stroke_to (s, 2000000000, -2000000000);
  easel_stroke_to (s, 99, 99);
  easel_stroke_end (s, NULL);
  CHECK (easel_image_get (img, 99, 99) == BLACK);
  easel_image_free (img);
}

static void
as21_cancel_restores (void)
{
  EaselImage *img = easel_image_new (40, 40, WHITE);
  EaselStroke *s = easel_stroke_begin (img, 0, 0, BLACK);
  uint32_t *before;

  easel_stroke_to (s, 39, 39);
  easel_stroke_end (s, NULL);
  before = malloc (40 * 40 * sizeof (uint32_t));
  memcpy (before, img->px, 40 * 40 * sizeof (uint32_t));

  s = easel_stroke_begin (img, 39, 0, 0xFF00FF00u);
  easel_stroke_to (s, 0, 39);
  easel_stroke_to (s, 20, 0);
  easel_stroke_cancel (s);

  CHECK (memcmp (before, img->px, 40 * 40 * sizeof (uint32_t)) == 0);
  free (before);
  easel_image_free (img);
}

static void
as23_noop_stroke_not_modified (void)
{
  EaselImage *img = easel_image_new (10, 10, WHITE);
  EaselStroke *s = easel_stroke_begin (img, 5, 5, BLACK);

  easel_stroke_end (s, NULL);
  s = easel_stroke_begin (img, 5, 5, BLACK);
  CHECK (easel_stroke_end (s, NULL) == 0);

  s = easel_stroke_begin (img, -3, -3, BLACK);
  CHECK (easel_stroke_end (s, NULL) == 0);
  easel_image_free (img);
}

static void
stroke_ends_into_history (void)
{
  EaselImage *img = easel_image_new (30, 10, WHITE);
  EaselHistory *h = easel_history_new (1 << 20);
  EaselStroke *s = easel_stroke_begin (img, 0, 0, BLACK);

  easel_stroke_to (s, 20, 0);
  /* Back over the same pixels: each pixel is recorded once. */
  easel_stroke_to (s, 0, 0);
  CHECK (easel_stroke_end (s, h) == 21);
  CHECK (easel_history_bytes (h) == 21 * 12 + 64);
  CHECK (count (img, BLACK) == 21);

  /* One stroke, one step. */
  CHECK (easel_history_undo (h, img));
  CHECK (count (img, BLACK) == 0);
  CHECK (!easel_history_can_undo (h));
  CHECK (easel_history_redo (h, img));
  CHECK (count (img, BLACK) == 21);
  CHECK (easel_image_get (img, 20, 0) == BLACK);

  easel_history_free (h);
  easel_image_free (img);
}

static void
stroke_without_change_adds_no_step (void)
{
  EaselImage *img = easel_image_new (10, 10, BLACK);
  EaselHistory *h = easel_history_new (1 << 20);
  EaselStroke *s = easel_stroke_begin (img, 5, 5, BLACK);

  easel_stroke_to (s, 8, 8);
  CHECK (easel_stroke_end (s, h) == 0);
  CHECK (!easel_history_can_undo (h));
  CHECK (!easel_history_is_modified (h));

  /* Entirely outside the picture. */
  s = easel_stroke_begin (img, -5, -5, WHITE);
  easel_stroke_to (s, -1, -9);
  CHECK (easel_stroke_end (s, h) == 0);

  /* A cancelled stroke adds nothing either (REQ-0002-02). */
  s = easel_stroke_begin (img, 1, 1, WHITE);
  easel_stroke_cancel (s);
  CHECK (!easel_history_can_undo (h));
  CHECK (easel_image_get (img, 1, 1) == BLACK);

  easel_history_free (h);
  easel_image_free (img);
}

/* A step stores 32-bit pixel indexes, so a picture with more pixels than that cannot be drawn on. */
static void
stroke_refuses_huge_picture (void)
{
  EaselImage huge = { 70000, 70000, NULL, { 0, 0, 0, 0 } };

  CHECK (easel_stroke_begin (&huge, 1, 1, BLACK) == NULL);
}

/* REQ-0003-08: a color with transparency sets pixels to exactly its value. Blending would mix it with the pixel below;
   the checkerboard behind transparency exists only on screen, never in the picture (REQ-0003-05). */
static void
as6_transparent_stroke_exact (void)
{
  static const uint32_t colors[] = { 0x00000000u, 0x80FF0000u };   /* #00000000 and #FF000080 */
  EaselImage *img = easel_image_new (20, 4, 0xFF336699u);

  for (int k = 0; k < 2; k++)
    {
      EaselStroke *s = easel_stroke_begin (img, 0, k, colors[k]);

      easel_stroke_to (s, 19, k);
      CHECK (easel_stroke_end (s, NULL) == 20);
      for (int x = 0; x < 20; x++)
        CHECK (easel_image_get (img, x, k) == colors[k]);
    }
  /* The row in between is untouched; a second pass in the same color changes nothing. */
  for (int x = 0; x < 20; x++)
    CHECK (easel_image_get (img, x, 2) == 0xFF336699u);
  {
    EaselStroke *s = easel_stroke_begin (img, 0, 1, colors[1]);

    easel_stroke_to (s, 19, 1);
    CHECK (easel_stroke_end (s, NULL) == 0);
  }
  easel_image_free (img);
}

/* ---- 0006: stamping strokes (Brush, Eraser) ---- */

#define RED 0xFFFF0000u
#define BLUE 0xFF0000FFu

/* A picture of w x h pixels of one color, and a mask of the pixels the test expects to have been painted. */
typedef struct { int w, h; unsigned char *on; } Expect;

static Expect
expect_new (int w, int h)
{
  return (Expect) { w, h, calloc ((size_t) w * (size_t) h, 1) };
}

static void
expect_stamp (Expect *e, EaselFootprint f, int x, int y)
{
  int a = easel_footprint_anchor (f.size);

  for (int row = 0; row < f.size; row++)
    for (int col = 0; col < f.size; col++)
      {
        int px = x - a + col, py = y - a + row;

        if (easel_footprint_has (f, col, row) && px >= 0 && py >= 0 && px < e->w && py < e->h)
          e->on[py * e->w + px] = 1;
      }
}

/* The picture has `color` exactly where the expectation is set, and `other` everywhere else. */
static void
check_painted (const EaselImage *img, const Expect *e, uint32_t color, uint32_t other)
{
  for (int y = 0; y < img->height; y++)
    for (int x = 0; x < img->width; x++)
      CHECK (easel_image_get (img, x, y) == (e->on[y * e->w + x] ? color : other));
}

/* AS-4: a click paints one footprint, a drag the footprint at every pixel of the line, with either button's color, clipped. */
static void
as4_brush_stroke (void)
{
  EaselFootprint c7 = { EASEL_SHAPE_CIRCLE, 7 };
  EaselImage *img = easel_image_new (100, 100, WHITE);
  Expect e = expect_new (100, 100);
  EaselStroke *s;

  /* A click: exactly the mask of AS-3, 37 pixels, with the pointer pixel at column 3, row 3. */
  s = easel_stroke_begin_stamp (img, 20, 20, BLACK, c7, NULL);
  expect_stamp (&e, c7, 20, 20);
  CHECK (easel_stroke_end (s, NULL) == 37);
  check_painted (img, &e, BLACK, WHITE);
  CHECK (easel_image_get (img, 20, 17) == BLACK && easel_image_get (img, 20, 23) == BLACK);
  CHECK (easel_image_get (img, 17, 20) == BLACK && easel_image_get (img, 23, 20) == BLACK);
  CHECK (easel_image_get (img, 17, 17) == WHITE);                       /* a corner of the box is not in the circle */
  free (e.on);

  /* A drag from (10, 60) to (40, 60). */
  easel_image_free (img);
  img = easel_image_new (100, 100, WHITE);
  e = expect_new (100, 100);
  s = easel_stroke_begin_stamp (img, 10, 60, RED, c7, NULL);
  easel_stroke_to (s, 40, 60);
  for (int x = 10; x <= 40; x++)
    expect_stamp (&e, c7, x, 60);
  easel_stroke_end (s, NULL);
  check_painted (img, &e, RED, WHITE);
  free (e.on);
  easel_image_free (img);

  /* Two far positions with nothing between: a stamp at every pixel of the line, no gap (the footprint's own pixel included). */
  img = easel_image_new (100, 100, WHITE);
  s = easel_stroke_begin_stamp (img, 10, 10, BLACK, c7, NULL);
  easel_stroke_to (s, 90, 80);
  easel_stroke_end (s, NULL);
  for (int i = 0; i <= 80; i++)
    {
      int x = 10 + (80 * i + 40) / 80, y = 10 + (70 * i + 40) / 80;       /* the line, rounded to the nearest pixel */

      CHECK (easel_image_get (img, x, y) == BLACK);
    }
  easel_image_free (img);

  /* A stroke that starts outside the picture paints only the part inside. */
  img = easel_image_new (100, 100, WHITE);
  e = expect_new (100, 100);
  s = easel_stroke_begin_stamp (img, -5, 20, BLUE, c7, NULL);
  easel_stroke_to (s, 5, 20);
  for (int x = -5; x <= 5; x++)
    expect_stamp (&e, c7, x, 20);
  easel_stroke_end (s, NULL);
  check_painted (img, &e, BLUE, WHITE);
  free (e.on);
  easel_image_free (img);
}

/* AS-5: slanted brushes, exact pixels, and the stripes of XP. */
static void
as5_slanted_brush (void)
{
  EaselFootprint slash8 = { EASEL_SHAPE_SLASH, 8 };
  EaselImage *img = easel_image_new (100, 100, WHITE);
  Expect e = expect_new (100, 100);
  EaselStroke *s;

  /* Horizontally, a band 8 pixels tall. */
  s = easel_stroke_begin_stamp (img, 10, 20, BLACK, slash8, NULL);
  easel_stroke_to (s, 30, 20);
  for (int x = 10; x <= 30; x++)
    expect_stamp (&e, slash8, x, 20);
  easel_stroke_end (s, NULL);
  check_painted (img, &e, BLACK, WHITE);
  CHECK (easel_image_get (img, 20, 17) == BLACK && easel_image_get (img, 20, 24) == BLACK);
  CHECK (easel_image_get (img, 20, 16) == WHITE && easel_image_get (img, 20, 25) == WHITE);
  free (e.on);
  easel_image_free (img);

  /* Along its own direction, from (20, 40) to (30, 30): one diagonal, x + y = 61, from (17, 44) to (34, 27). */
  img = easel_image_new (100, 100, WHITE);
  s = easel_stroke_begin_stamp (img, 20, 40, BLACK, slash8, NULL);
  easel_stroke_to (s, 30, 30);
  CHECK (easel_stroke_end (s, NULL) == 18);
  for (int y = 0; y < 100; y++)
    for (int x = 0; x < 100; x++)
      CHECK ((easel_image_get (img, x, y) == BLACK) == (x + y == 61 && x >= 17 && x <= 34));
  easel_image_free (img);

  /* Across its direction, along "\": the stamps leave the diagonals between them empty (XP's stripes): only the
     diagonals x + y = 71, 73, ..., 91 are painted. */
  img = easel_image_new (100, 100, WHITE);
  s = easel_stroke_begin_stamp (img, 10, 60, BLACK, slash8, NULL);
  easel_stroke_to (s, 20, 70);
  easel_stroke_end (s, NULL);
  for (int y = 0; y < 100; y++)
    for (int x = 0; x < 100; x++)
      if (easel_image_get (img, x, y) == BLACK)
        CHECK ((x + y) % 2 == 1 && x + y >= 71 && x + y <= 91);
  CHECK (easel_image_get (img, 14, 57) == BLACK);                        /* x + y = 71: a stripe */
  CHECK (easel_image_get (img, 15, 57) == WHITE);                        /* x + y = 72, 74: between the stripes */
  CHECK (easel_image_get (img, 20, 54) == WHITE);
  easel_image_free (img);
}

/* AS-7: the eraser paints its square with the background color, or with transparency. */
static void
as7_eraser (void)
{
  EaselFootprint sq8 = { EASEL_SHAPE_SQUARE, 8 };
  EaselImage *img = easel_image_new (100, 100, BLACK);
  Expect e = expect_new (100, 100);
  EaselStroke *s;
  uint32_t yellow = 0xFFFFFF00u;

  /* Opaque: a click is one 8 x 8 square, the pointer pixel at column 3, row 3; a drag a path 8 wide. */
  s = easel_stroke_begin_stamp (img, 30, 30, yellow, sq8, NULL);
  CHECK (easel_stroke_end (s, NULL) == 64);
  CHECK (easel_image_get (img, 27, 27) == yellow && easel_image_get (img, 34, 34) == yellow);
  CHECK (easel_image_get (img, 26, 30) == BLACK && easel_image_get (img, 35, 30) == BLACK);
  s = easel_stroke_begin_stamp (img, 10, 70, yellow, sq8, NULL);
  easel_stroke_to (s, 50, 70);
  for (int x = 10; x <= 50; x++)
    expect_stamp (&e, sq8, x, 70);
  easel_stroke_end (s, NULL);
  for (int y = 40; y < 100; y++)
    for (int x = 0; x < 100; x++)
      CHECK (easel_image_get (img, x, y) == (e.on[y * 100 + x] ? yellow : BLACK));
  free (e.on);

  /* Transparent: the pixels become #00000000. */
  s = easel_stroke_begin_stamp (img, 80, 10, 0x00000000u, sq8, NULL);
  easel_stroke_to (s, 90, 10);
  easel_stroke_end (s, NULL);
  CHECK (easel_image_get (img, 85, 7) == 0x00000000u && easel_image_get (img, 85, 14) == 0x00000000u);   /* rows 7 to 14 */
  CHECK (easel_image_get (img, 85, 6) == BLACK && easel_image_get (img, 85, 15) == BLACK);
  easel_image_free (img);
}

/* AS-8: the color eraser changes only pixels equal to the foreground color in all four channels. */
static void
as8_color_eraser (void)
{
  EaselFootprint sq8 = { EASEL_SHAPE_SQUARE, 8 };
  EaselImage *img = easel_image_new (40, 40, BLUE);
  EaselStroke *s;
  uint32_t red = RED;

  for (int y = 0; y < 40; y++)
    for (int x = 0; x < 20; x++)
      easel_image_set (img, x, y, red);

  /* Red becomes white where the footprint passes; blue stays. */
  s = easel_stroke_begin_stamp (img, 5, 20, WHITE, sq8, &red);
  easel_stroke_to (s, 30, 20);
  easel_stroke_end (s, NULL);
  CHECK (easel_image_get (img, 10, 20) == WHITE && easel_image_get (img, 19, 20) == WHITE);
  CHECK (easel_image_get (img, 25, 20) == BLUE);                         /* in the path, but not red */
  CHECK (easel_image_get (img, 10, 10) == red);                           /* outside the path */
  easel_image_free (img);

  /* Transparent mode: the same pixels become #00000000. */
  img = easel_image_new (40, 40, BLUE);
  easel_image_set (img, 10, 10, red);
  s = easel_stroke_begin_stamp (img, 10, 10, 0x00000000u, sq8, &red);
  CHECK (easel_stroke_end (s, NULL) == 1);
  CHECK (easel_image_get (img, 10, 10) == 0x00000000u && easel_image_get (img, 11, 10) == BLUE);
  easel_image_free (img);

  /* All four channels: a fully transparent pixel with other color channels is not the transparent foreground. */
  img = easel_image_new (20, 20, 0);
  for (int y = 0; y < 20; y++)
    for (int x = 0; x < 20; x++)
      easel_image_set (img, x, y, 0x00FF0000u);              /* hidden colors, as an older picture may hold */
  easel_image_set (img, 5, 5, 0x00000000u);
  {
    uint32_t clear = 0x00000000u;

    s = easel_stroke_begin_stamp (img, 5, 5, WHITE, sq8, &clear);
    CHECK (easel_stroke_end (s, NULL) == 1);
    CHECK (easel_image_get (img, 5, 5) == WHITE && easel_image_get (img, 6, 5) == 0x00FF0000u);
  }
  easel_image_free (img);

  /* Foreground and background equal: nothing changes and no step is made. */
  img = easel_image_new (20, 20, RED);
  {
    EaselHistory *h = easel_history_new (1 << 20);

    s = easel_stroke_begin_stamp (img, 10, 10, RED, sq8, &red);
    CHECK (easel_stroke_end (s, h) == 0);
    CHECK (!easel_history_can_undo (h));
    easel_history_free (h);
  }
  CHECK (count (img, RED) == 400);
  easel_image_free (img);
}

int
main (void)
{
  RUN (as16_click_sets_one_pixel);
  RUN (as18_line_is_8_connected);
  RUN (as19_secondary_uses_background);
  RUN (as20_clipped_line);
  RUN (as21_cancel_restores);
  RUN (as23_noop_stroke_not_modified);
  RUN (stroke_ends_into_history);
  RUN (stroke_without_change_adds_no_step);
  RUN (stroke_refuses_huge_picture);
  RUN (as6_transparent_stroke_exact);
  RUN (as4_brush_stroke);
  RUN (as5_slanted_brush);
  RUN (as7_eraser);
  RUN (as8_color_eraser);
  RUN (transparent_color_is_zero);
  return 0;
}
