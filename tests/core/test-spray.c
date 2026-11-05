/* SPDX-License-Identifier: GPL-3.0-or-later */
/* The Airbrush's spray of feature 0006 (REQ-0006-08, REQ-0006-09), with a fixed random sequence and a simulated clock. */

#include <stdlib.h>
#include <string.h>

#include "check.h"
#include "footprint.h"
#include "history.h"
#include "image.h"
#include "stroke.h"

#define WHITE 0xFFFFFFFFu
#define BLACK 0xFF000000u

/* Is (x, y) in the disc of the given diameter placed around the pointer at (px, py)? Written out here, not taken from the
   core: the box's top-left is at the pointer minus (diameter - 1) / 2, a pixel is in the disc if its center is within
   diameter / 2 of the box's center. */
static int
in_disc (int diameter, int px, int py, int x, int y)
{
  int a = (diameter - 1) / 2, col = x - (px - a), row = y - (py - a);
  long dx = 2L * col + 1 - diameter, dy = 2L * row + 1 - diameter;

  return col >= 0 && row >= 0 && col < diameter && row < diameter && dx * dx + dy * dy < (long) diameter * diameter;
}

static int
count (const EaselImage *img, uint32_t color)
{
  int n = 0;

  for (int y = 0; y < img->height; y++)
    for (int x = 0; x < img->width; x++)
      n += easel_image_get (img, x, y) == color;
  return n;
}

/* The count of sprays due by a time held: one at the press and one for every full 5 ms. */
static void
as6_sprays_due (void)
{
  CHECK (easel_spray_due (0) == 1);
  CHECK (easel_spray_due (4999) == 1);
  CHECK (easel_spray_due (5000) == 2);
  CHECK (easel_spray_due (9999) == 2);
  CHECK (easel_spray_due (10000) == 3);
  CHECK (easel_spray_due (1000000) == 201);
  CHECK (easel_spray_due (-5) == 1);                        /* a clock that went back: still the press */
  CHECK (easel_spray_pixels (9) == 1 && easel_spray_pixels (16) == 1 && easel_spray_pixels (24) == 3);
}

/* AS-6: one second held still on the disc of 16: exactly 201 sprays of one pixel, all in the disc. */
static void
as6_spray_held (void)
{
  EaselImage *img = easel_image_new (100, 100, WHITE);
  EaselStroke *s = easel_stroke_begin_spray (img, BLACK, 16, 1, 12345);
  int made = 0, painted;

  /* The clock ticks every 5 ms exactly, with the press at 0. */
  for (int64_t t_us = 0; t_us <= 1000000; t_us += 5000)
    {
      int due = easel_spray_due (t_us);

      easel_stroke_spray (s, 50, 50, due - made);
      made = due;
    }
  CHECK (made == 201);
  painted = (int) easel_stroke_end (s, NULL);
  CHECK (painted == count (img, BLACK));
  CHECK (painted > 100 && painted <= 201);                  /* 201 picks with repeats: about 127 different pixels */
  for (int y = 0; y < 100; y++)
    for (int x = 0; x < 100; x++)
      if (easel_image_get (img, x, y) == BLACK)
        CHECK (in_disc (16, 50, 50, x, y));
  easel_image_free (img);
}

/* A late timer catches up: the count follows the time held, not the number of ticks. */
static void
as6_late_ticks_catch_up (void)
{
  static const int64_t ticks_us[] = { 0, 5000, 17000, 18000, 400000, 1000000 };
  int made = 0;

  for (size_t i = 0; i < sizeof ticks_us / sizeof ticks_us[0]; i++)
    {
      int due = easel_spray_due (ticks_us[i]);

      CHECK (due >= made);                                  /* never fewer than made, so never a negative spray */
      made = due;
    }
  CHECK (made == 201);                                      /* however late the ticks were */
  CHECK (easel_spray_due (17000) == 4 && easel_spray_due (400000) == 81);
}

/* The 24 disc sprays 3 pixels at a time; every pixel is in its disc. */
static void
as6_big_disc (void)
{
  EaselImage *img = easel_image_new (100, 100, WHITE);
  EaselStroke *s = easel_stroke_begin_spray (img, BLACK, 24, 3, 7);
  int painted;

  easel_stroke_spray (s, 40, 40, 201);
  painted = (int) easel_stroke_end (s, NULL);
  CHECK (painted > 300 && painted <= 603);                  /* 603 picks from 452 pixels: about 330 different */
  for (int y = 0; y < 100; y++)
    for (int x = 0; x < 100; x++)
      if (easel_image_get (img, x, y) == BLACK)
        CHECK (in_disc (24, 40, 40, x, y));
  easel_image_free (img);
}

/* REQ-0006-09: the random choice depends on the seed; the same seed gives the same pixels. */
static void
as6_seed_decides (void)
{
  EaselImage *a = easel_image_new (60, 60, WHITE), *b = easel_image_new (60, 60, WHITE), *c = easel_image_new (60, 60, WHITE);
  EaselStroke *sa = easel_stroke_begin_spray (a, BLACK, 16, 1, 99), *sb = easel_stroke_begin_spray (b, BLACK, 16, 1, 99);
  EaselStroke *sc = easel_stroke_begin_spray (c, BLACK, 16, 1, 100);

  easel_stroke_spray (sa, 30, 30, 201);
  easel_stroke_spray (sb, 30, 30, 201);
  easel_stroke_spray (sc, 30, 30, 201);
  easel_stroke_end (sa, NULL);
  easel_stroke_end (sb, NULL);
  easel_stroke_end (sc, NULL);
  CHECK (memcmp (a->px, b->px, 60 * 60 * sizeof (uint32_t)) == 0);
  CHECK (memcmp (a->px, c->px, 60 * 60 * sizeof (uint32_t)) != 0);
  /* A seed of 0 works too (the generator never gets stuck on it). */
  {
    EaselStroke *sz = easel_stroke_begin_spray (a, BLACK, 16, 1, 0);

    easel_stroke_spray (sz, 30, 30, 50);
    CHECK (easel_stroke_end (sz, NULL) > 0);
  }
  easel_image_free (a);
  easel_image_free (b);
  easel_image_free (c);
}

/* Equal chance: over many sprays every pixel of the disc is reached, and none outside. */
static void
as6_every_pixel_of_the_disc_is_reached (void)
{
  EaselImage *img = easel_image_new (40, 40, WHITE);
  EaselStroke *s = easel_stroke_begin_spray (img, BLACK, 9, 1, 2024);
  int inside = 0;

  easel_stroke_spray (s, 20, 20, 5000);                     /* 5000 picks from 69 pixels: all of them with near certainty */
  easel_stroke_end (s, NULL);
  for (int y = 0; y < 40; y++)
    for (int x = 0; x < 40; x++)
      {
        int d = in_disc (9, 20, 20, x, y);

        inside += d;
        CHECK ((easel_image_get (img, x, y) == BLACK) == d);
      }
  CHECK (inside == 69);                                        /* counted by hand: 9 + 18 + 18 + 14 + 10 */
  easel_image_free (img);
}

/* AS-14: a spray lands where the pointer is at that moment: a fast move leaves gaps between the positions. */
static void
as14_spray_gaps (void)
{
  EaselImage *img = easel_image_new (200, 60, WHITE);
  EaselStroke *s = easel_stroke_begin_spray (img, BLACK, 9, 1, 5);
  /* 100 pixels to the right in 10 ms: the pointer is at x = 20 at 0 ms, 70 at 5 ms and 120 at 10 ms. */
  static const int64_t t_us[] = { 0, 5000, 10000 };
  static const int x_at[] = { 20, 70, 120 };
  int made = 0;

  for (int i = 0; i < 3; i++)
    {
      int due = easel_spray_due (t_us[i]);

      easel_stroke_spray (s, x_at[i], 30, due - made);
      made = due;
    }
  CHECK (made == 3);
  CHECK (easel_stroke_end (s, NULL) <= 3);
  for (int y = 0; y < 60; y++)
    for (int x = 0; x < 200; x++)
      if (easel_image_get (img, x, y) == BLACK)
        CHECK (in_disc (9, 20, 30, x, y) || in_disc (9, 70, 30, x, y) || in_disc (9, 120, 30, x, y));
  CHECK (count (img, BLACK) >= 1);
  /* Nothing between the positions. */
  for (int y = 0; y < 60; y++)
    for (int x = 40; x < 60; x++)
      CHECK (easel_image_get (img, x, y) == WHITE);
  easel_image_free (img);
}

/* Edges: only pixels inside the picture are painted, also for a spray that is far outside; a 1 x 1 picture. */
static void
as6_spray_is_clipped (void)
{
  EaselImage *img = easel_image_new (20, 20, WHITE);
  EaselStroke *s = easel_stroke_begin_spray (img, BLACK, 24, 3, 3);

  easel_stroke_spray (s, -3, -3, 400);                      /* the disc reaches only the top-left corner */
  easel_stroke_spray (s, 1000, 1000, 50);                   /* far outside: nothing */
  easel_stroke_spray (s, -1000000, 5, 50);
  CHECK (easel_stroke_end (s, NULL) > 0);
  for (int y = 0; y < 20; y++)
    for (int x = 0; x < 20; x++)
      if (easel_image_get (img, x, y) == BLACK)
        CHECK (in_disc (24, -3, -3, x, y));
  easel_image_free (img);

  img = easel_image_new (1, 1, WHITE);
  s = easel_stroke_begin_spray (img, BLACK, 24, 3, 3);
  easel_stroke_spray (s, 0, 0, 201);
  CHECK (easel_stroke_end (s, NULL) == 1);
  CHECK (easel_image_get (img, 0, 0) == BLACK);
  easel_image_free (img);
}

/* A spray is one stroke: one undo step with the pixels it changed, and a cancel puts them all back. */
static void
as9_spray_is_one_step (void)
{
  EaselImage *img = easel_image_new (60, 60, WHITE);
  EaselHistory *h = easel_history_new (1 << 20);
  EaselStroke *s = easel_stroke_begin_spray (img, BLACK, 16, 1, 11);
  size_t n;

  easel_stroke_spray (s, 30, 30, 100);
  n = easel_stroke_end (s, h);
  CHECK (n > 0);
  CHECK (easel_history_bytes (h) == n * 12 + 64);
  CHECK (easel_history_undo (h, img));
  CHECK (count (img, BLACK) == 0);
  CHECK (!easel_history_can_undo (h));

  s = easel_stroke_begin_spray (img, BLACK, 16, 1, 12);
  easel_stroke_spray (s, 30, 30, 100);
  CHECK (count (img, BLACK) > 0);
  easel_stroke_cancel (s);
  CHECK (count (img, BLACK) == 0);

  /* Painting a color that is already there changes nothing: no step. */
  {
    EaselImage *black = easel_image_new (20, 20, BLACK);
    EaselHistory *h2 = easel_history_new (1 << 20);

    s = easel_stroke_begin_spray (black, BLACK, 9, 1, 13);
    easel_stroke_spray (s, 10, 10, 50);
    CHECK (easel_stroke_end (s, h2) == 0);
    CHECK (!easel_history_can_undo (h2));
    easel_history_free (h2);
    easel_image_free (black);
  }
  easel_history_free (h);
  easel_image_free (img);
}

int
main (void)
{
  RUN (as6_sprays_due);
  RUN (as6_spray_held);
  RUN (as6_late_ticks_catch_up);
  RUN (as6_big_disc);
  RUN (as6_seed_decides);
  RUN (as6_every_pixel_of_the_disc_is_reached);
  RUN (as14_spray_gaps);
  RUN (as6_spray_is_clipped);
  RUN (as9_spray_is_one_step);
  return 0;
}
