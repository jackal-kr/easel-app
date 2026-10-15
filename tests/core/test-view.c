/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Zoom and pointer mapping scenarios of feature 0001. */

#include "check.h"
#include "view.h"

static void
as13_zoom_steps_and_anchor (void)
{
  double sx, sy;
  int px, py;
  double acc = 0;

  CHECK (easel_view_zoom_step (100, 1) == 200);
  CHECK (easel_view_zoom_step (100, 21) == 800);
  CHECK (easel_view_zoom_step (100, -20) == 10);
  CHECK (easel_view_zoom_step (800, 1) == 800);
  CHECK (easel_view_zoom_step (10, -1) == 10);
  CHECK (easel_view_zoom_step (50, 2) == 200);
  CHECK (easel_view_zoom_step (400, -3) == 100);

  /* 2000x2000 picture, 500x500 view, scale 1, pointer over pixel (40,30) at 100 %. */
  sx = easel_view_anchor (0, 40.5, 100, 200, 2000, 1.0, 500);
  sy = easel_view_anchor (0, 30.5, 100, 200, 2000, 1.0, 500);
  easel_view_to_pixel (sx + 40.5, sy + 30.5, 200, 1.0, &px, &py);
  CHECK (px == 40 && py == 30);

  /* Same at scale 2 and from 300 % to 400 % with the view scrolled. */
  sx = easel_view_anchor (700, 123.25, 300, 400, 2000, 2.0, 500);
  easel_view_to_pixel (700 + 123.25, 0, 300, 2.0, &px, &py);
  {
    int px2, py2;

    easel_view_to_pixel (sx + 123.25, 0, 400, 2.0, &px2, &py2);
    CHECK (px2 == px);
  }

  /* At the edges the view cannot scroll past the picture. */
  CHECK (easel_view_anchor (0, 10, 200, 100, 2000, 1.0, 500) == 0);
  CHECK (easel_view_anchor (1500, 490, 100, 50, 2000, 1.0, 500) == 500);
  CHECK (easel_view_anchor (0, 10, 100, 200, 100, 1.0, 500) == 0);

  /* Touchpad: deltas add up to whole notches; wheel up (negative delta) zooms in. */
  CHECK (easel_view_take_notches (&acc, -0.4) == 0);
  CHECK (easel_view_take_notches (&acc, -0.4) == 0);
  CHECK (easel_view_take_notches (&acc, -0.4) == 1);
  CHECK (easel_view_take_notches (&acc, 2.9) == -2);
  CHECK (easel_view_take_notches (&acc, -1.0) == 1);
}

static void
as17_zoomed_click_maps_to_pixel (void)
{
  int px, py;

  for (double w = 12.0; w < 16.0; w += 0.25)
    {
      easel_view_to_pixel (w, w, 400, 1.0, &px, &py);
      CHECK (px == 3 && py == 3);
    }
  easel_view_to_pixel (11.99, 16.0, 400, 1.0, &px, &py);
  CHECK (px == 2 && py == 4);

  /* Scale 2: 400 % is 4 device = 2 logical pixels per picture pixel. */
  easel_view_to_pixel (6.0, 7.99, 400, 2.0, &px, &py);
  CHECK (px == 3 && py == 3);

  /* 100 % is one device pixel; 10 % packs ten picture pixels into one device pixel. */
  easel_view_to_pixel (5.5, 0.0, 100, 1.25, &px, &py);
  CHECK (px == 6);
  easel_view_to_pixel (3.0, 0.0, 10, 1.0, &px, &py);
  CHECK (px == 30);

  /* Left of or above the picture is negative, not 0. */
  easel_view_to_pixel (-0.5, -3.0, 100, 1.0, &px, &py);
  CHECK (px == -1 && py == -3);
}

/* REQ-0001-08, REQ-0001-16: the canvas is exactly the picture at the zoom (1 picture pixel = 1 device pixel at 100 %). */
static void
content_size_covers_picture (void)
{
  CHECK (easel_view_content_size (1920, 100, 1.0) == 1920);
  CHECK (easel_view_content_size (1920, 100, 2.0) == 960);
  CHECK (easel_view_content_size (1001, 100, 1.25) == 801);
  CHECK (easel_view_content_size (10000, 800, 1.0) == 80000);
  CHECK (easel_view_content_size (1, 10, 1.0) == 1);
}

static void
as2_window_size (void)
{
  int w, h;

  easel_view_window_size (1920, 1200, &w, &h);
  CHECK (w == 1440 && h == 900);
  easel_view_window_size (1366, 768, &w, &h);
  CHECK (w == 1024 && h == 576);
}

/* REQ-0004-04, REQ-0004-02: one level per step, and the limits where a step does nothing. */
static void
as3_zoom_levels (void)
{
  static const int levels[] = { 10, 25, 50, 100, 200, 300, 400, 600, 800 };

  for (int i = 0; i + 1 < 9; i++)
    {
      CHECK (easel_view_zoom_step (levels[i], 1) == levels[i + 1]);
      CHECK (easel_view_zoom_step (levels[i + 1], -1) == levels[i]);
    }
  CHECK (easel_view_zoom_step (800, 1) == 800);   /* Zoom In has nothing to do at 800 % */
  CHECK (easel_view_zoom_step (10, -1) == 10);    /* and Zoom Out at 10 % */
}

/* REQ-0004-06: a zoom command keeps the center of the canvas area still. */
static void
as5_zoom_anchor_center (void)
{
  /* 2000x2000 picture at 100 %, canvas area 1000x800 scrolled to (500, 400): the center is picture pixel (1000, 800). */
  double sx = easel_view_anchor (500, 500, 100, 200, 2000, 1.0, 1000);
  double sy = easel_view_anchor (400, 400, 100, 200, 2000, 1.0, 800);

  CHECK (sx == 1500 && sy == 1200);       /* (1000, 800) * 2 - (500, 400) */
  CHECK (easel_view_anchor (500, 500, 200, 100, 2000, 1.0, 1000) == 0);   /* zooming out: the picture fits, no scroll */
  /* A picture smaller than the canvas area stays at the top-left, at any zoom. */
  CHECK (easel_view_anchor (0, 500, 100, 200, 300, 1.0, 1000) == 0);
  CHECK (easel_view_anchor (0, 400, 100, 800, 100, 1.0, 1000) == 0);   /* 100 pixels at 800 % are 800 < 1000 */
  /* Where the anchor would leave the picture, the edge wins: zooming out at the right end (200 % scrolled to 3000). */
  CHECK (easel_view_anchor (3000, 500, 200, 100, 2000, 1.0, 1000) == 1000);
  CHECK (easel_view_anchor (1000, 1000, 100, 200, 2000, 1.0, 1000) == 3000);   /* zooming in at the right end */
}

/* REQ-0004-07: the largest level at which the whole picture fits the canvas area; device pixels. */
static void
as6_fit_level (void)
{
  CHECK (easel_view_fit_level (1390, 853, 800, 600) == 100);
  CHECK (easel_view_fit_level (1390, 853, 3000, 2000) == 25);
  CHECK (easel_view_fit_level (1390, 853, 10000, 10000) == 10);   /* none fits: 10 % */
  CHECK (easel_view_fit_level (1390, 853, 1, 1) == 800);          /* smaller than the area even at 800 % */
  CHECK (easel_view_fit_level (1390, 853, 100, 100) == 800);
  CHECK (easel_view_fit_level (1390, 853, 174, 100) == 600);      /* 174 * 8 = 1392 > 1390; 174 * 6 = 1044 */
  /* Fitting exactly counts as fitting; either direction can be the limit. */
  CHECK (easel_view_fit_level (1000, 800, 1000, 800) == 100);
  CHECK (easel_view_fit_level (2000, 800, 1000, 800) == 100);     /* the height limits it */
  CHECK (easel_view_fit_level (1000, 2000, 1000, 800) == 100);    /* the width limits it */
  CHECK (easel_view_fit_level (2000, 1600, 1000, 800) == 200);
  CHECK (easel_view_fit_level (0, 0, 800, 600) == 10);            /* no room at all */
}

/* REQ-0004-08: one level per 1.5x of the pinch since the last level change; the scale starts again after each change,
   at the limits, and when a pinch ends. */
static void
as7_pinch_accumulator (void)
{
  double base = 1.0;

  CHECK (easel_view_pinch (&base, 1.2, 100) == 0 && base == 1.0);
  CHECK (easel_view_pinch (&base, 1.5, 100) == 1 && base == 1.5);         /* in, and the scale starts again */
  CHECK (easel_view_pinch (&base, 2.0, 200) == 0 && base == 1.5);
  CHECK (easel_view_pinch (&base, 2.25, 200) == 1 && base == 2.25);       /* 2.25 / 1.5 */
  CHECK (easel_view_pinch (&base, 1.5, 300) == -1 && base == 1.5);        /* out, 1 / 1.5 of the last change */
  CHECK (easel_view_pinch (&base, 1.2, 200) == 0 && base == 1.5);
  /* A fast pinch moves one level per call, not several. */
  base = 1.0;
  CHECK (easel_view_pinch (&base, 4.0, 100) == 1 && base == 4.0);

  /* At 800 % a further pinch out does nothing and starts the scale again, so reversing needs a normal 1.5x. */
  base = 1.0;
  CHECK (easel_view_pinch (&base, 1.6, 800) == 0 && base == 1.6);
  CHECK (easel_view_pinch (&base, 1.2, 800) == 0 && base == 1.6);
  CHECK (easel_view_pinch (&base, 1.0, 800) == -1 && base == 1.0);
  /* Same at 10 %: a pinch in does nothing there and restarts the scale; 1.5x from that point zooms in. */
  base = 1.0;
  CHECK (easel_view_pinch (&base, 0.6, 10) == 0 && base == 0.6);
  CHECK (easel_view_pinch (&base, 0.8, 10) == 0 && base == 0.6);
  CHECK (easel_view_pinch (&base, 0.9, 10) == 1 && base == 0.9);
}

int
main (void)
{
  RUN (as2_window_size);
  RUN (as13_zoom_steps_and_anchor);
  RUN (as3_zoom_levels);
  RUN (as5_zoom_anchor_center);
  RUN (as6_fit_level);
  RUN (as7_pinch_accumulator);
  RUN (as17_zoomed_click_maps_to_pixel);
  RUN (content_size_covers_picture);
  return 0;
}
