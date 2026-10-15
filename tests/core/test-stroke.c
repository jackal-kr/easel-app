/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Pencil stroke scenarios of feature 0001. */

#include <stdlib.h>
#include <string.h>

#include "check.h"
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
  return 0;
}
