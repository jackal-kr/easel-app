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

/* 0008 AS-1 (REQ-0008-01): one list of presets, in order, and the index of a zoom in it (-1 when it is not a preset). */
static void
as1_presets_list_and_index (void)
{
  static const int expected[] = { 100, 200, 400, 600, 800 };

  CHECK (EASEL_VIEW_N_PRESETS == 5);
  for (int i = 0; i < 5; i++)
    {
      CHECK (easel_view_presets[i] == expected[i]);
      CHECK (easel_view_preset_index (expected[i]) == i);
    }
  CHECK (easel_view_preset_index (300) == -1);        /* a step, not a preset */
  CHECK (easel_view_preset_index (10) == -1);         /* steps below the first preset */
  CHECK (easel_view_preset_index (25) == -1);
  CHECK (easel_view_preset_index (50) == -1);
  CHECK (easel_view_preset_index (42) == -1);         /* Fit to Window's zoom */
  CHECK (easel_view_preset_index (0) == -1);
  CHECK (easel_view_preset_index (9) == -1);
  CHECK (easel_view_preset_index (801) == -1);
}

/* 0008 AS-5 (REQ-0008-06): every preset is a zoom step, so Zoom In from 100 % reaches every preset in order, with 300 %
   between 200 % and 400 % as the only step that is not a preset; 10, 25 and 50 % are steps below the first preset. */
static void
as5_every_preset_is_a_step (void)
{
  int zoom = 100, next = 1;

  CHECK (easel_view_presets[0] == 100);
  CHECK (easel_view_zoom_step (100, -1) == 50 && easel_view_zoom_step (50, -1) == 25 && easel_view_zoom_step (25, -1) == 10);
  for (int n = 0; n < 20 && next < EASEL_VIEW_N_PRESETS; n++)
    {
      zoom = easel_view_zoom_step (zoom, 1);
      if (zoom == easel_view_presets[next])
        next++;
      else
        CHECK (zoom == 300 && easel_view_presets[next] == 400);
    }
  CHECK (next == EASEL_VIEW_N_PRESETS);
  CHECK (easel_view_zoom_step (200, 1) == 300);
  CHECK (easel_view_zoom_step (400, -1) == 300);
  CHECK (easel_view_zoom_step (300, -1) == 200);
  /* Each preset is also reached by its own step from the one below it. */
  for (int i = 0; i < EASEL_VIEW_N_PRESETS; i++)
    CHECK (easel_view_zoom_step (easel_view_zoom_step (easel_view_presets[i], -1), 1) == easel_view_presets[i]);
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
as6_fit_zoom (void)
{
  /* The largest whole percent that fits, 10 to 800 (REQ-0004-07). */
  CHECK (easel_view_fit_zoom (1390, 853, 800, 600) == 142);        /* 853 / 600 = 1.421 */
  CHECK (easel_view_fit_zoom (1390, 853, 3000, 2000) == 42);       /* 853 / 2000 = 0.426 */
  CHECK (easel_view_fit_zoom (1390, 853, 5840, 5184) == 16);       /* the photo: 853 / 5184 = 0.1645 */
  CHECK (easel_view_fit_zoom (1390, 853, 10000, 10000) == 10);     /* 8.5 %: not below 10 % */
  CHECK (easel_view_fit_zoom (1390, 853, 1, 1) == 800);            /* smaller than the area even at 800 % */
  CHECK (easel_view_fit_zoom (1390, 853, 100, 100) == 800);
  CHECK (easel_view_fit_zoom (1390, 853, 174, 100) == 798);        /* 1390 / 174 = 7.988 */
  CHECK (easel_view_fit_zoom (1390, 853, 173, 100) == 800);        /* 8.035: capped */
  /* Fitting exactly counts as fitting; either direction can be the limit. */
  CHECK (easel_view_fit_zoom (1000, 800, 1000, 800) == 100);
  CHECK (easel_view_fit_zoom (2000, 800, 1000, 800) == 100);       /* the height limits it */
  CHECK (easel_view_fit_zoom (1000, 2000, 1000, 800) == 100);      /* the width limits it */
  CHECK (easel_view_fit_zoom (2000, 1600, 1000, 800) == 200);
  CHECK (easel_view_fit_zoom (0, 0, 800, 600) == 10);              /* no room at all */
  /* The result fits, and one percent more would not (except at the ends). */
  for (int w = 50; w <= 9000; w += 437)
    for (int h = 50; h <= 9000; h += 611)
      {
        int z = easel_view_fit_zoom (1390, 853, w, h);

        if (z > 10)
          CHECK ((long) w * z <= 1390L * 100 && (long) h * z <= 853L * 100);
        if (z > 10 && z < 800)
          CHECK ((long) w * (z + 1) > 1390L * 100 || (long) h * (z + 1) > 853L * 100);
      }
}

/* REQ-0004-04, REQ-0004-07: from a zoom between two levels, Zoom In and Zoom Out go to the next level above or below. */
static void
as6_levels_from_a_fit_zoom (void)
{
  CHECK (easel_view_zoom_step (16, 1) == 25);
  CHECK (easel_view_zoom_step (16, -1) == 10);
  CHECK (easel_view_zoom_step (142, 1) == 200);
  CHECK (easel_view_zoom_step (142, -1) == 100);
  CHECK (easel_view_zoom_step (798, 1) == 800);
  CHECK (easel_view_zoom_step (798, -1) == 600);
  CHECK (easel_view_zoom_step (42, 1) == 50);
  CHECK (easel_view_zoom_step (42, -1) == 25);
  CHECK (easel_view_zoom_step (25, 1) == 50);                      /* on a level: the next one */
  CHECK (easel_view_zoom_step (10, -1) == 10);
  CHECK (easel_view_zoom_step (800, 1) == 800);
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

/* AS-17: the size a handle drag gives, from the movement since the press (REQ-0004-18). */
static void
as17_size_from_movement (void)
{
  int w, h;

  w = 300; h = 200;
  easel_view_resize_by (EASEL_HANDLE_RIGHT, 100.4, 77, &w, &h);       /* the other axis is ignored */
  CHECK (w == 400 && h == 200);
  w = 300; h = 200;
  easel_view_resize_by (EASEL_HANDLE_BOTTOM, 55, -50, &w, &h);
  CHECK (w == 300 && h == 150);
  w = 300; h = 200;
  easel_view_resize_by (EASEL_HANDLE_CORNER, 200, 100, &w, &h);
  CHECK (w == 500 && h == 300);
  /* Nearest boundary: .5 goes up, as the pointer is as close to either. */
  w = 300; h = 200;
  easel_view_resize_by (EASEL_HANDLE_CORNER, 0.49, -0.51, &w, &h);
  CHECK (w == 300 && h == 199);
  w = 300; h = 200;
  easel_view_resize_by (EASEL_HANDLE_CORNER, 0.5, 0.5, &w, &h);
  CHECK (w == 301 && h == 201);
  /* No movement: the same size, which adds no step. */
  w = 300; h = 200;
  easel_view_resize_by (EASEL_HANDLE_CORNER, 0, 0, &w, &h);
  CHECK (w == 300 && h == 200);
  /* Limits 1 to 10000. */
  w = 300; h = 200;
  easel_view_resize_by (EASEL_HANDLE_CORNER, -5000, -1e9, &w, &h);
  CHECK (w == 1 && h == 1);
  w = 300; h = 200;
  easel_view_resize_by (EASEL_HANDLE_CORNER, 20000, 1e12, &w, &h);
  CHECK (w == 10000 && h == 10000);
  w = 300; h = 200;
  easel_view_resize_by (EASEL_HANDLE_RIGHT, -299.4, 0, &w, &h);
  CHECK (w == 1 && h == 200);
  /* Not a handle: nothing changes. */
  w = 300; h = 200;
  easel_view_resize_by (EASEL_HANDLE_NONE, 50, 50, &w, &h);
  CHECK (w == 300 && h == 200);
}

int
main (void)
{
  RUN (as2_window_size);
  RUN (as13_zoom_steps_and_anchor);
  RUN (as3_zoom_levels);
  RUN (as1_presets_list_and_index);
  RUN (as5_every_preset_is_a_step);
  RUN (as5_zoom_anchor_center);
  RUN (as6_fit_zoom);
  RUN (as6_levels_from_a_fit_zoom);
  RUN (as7_pinch_accumulator);
  RUN (as17_zoomed_click_maps_to_pixel);
  RUN (as17_size_from_movement);
  RUN (content_size_covers_picture);
  return 0;
}
