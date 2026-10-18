/* SPDX-License-Identifier: GPL-3.0-or-later */
/* The Magnifier's rules, feature 0007 (REQ-0007-03, REQ-0007-04). */

#include <math.h>

#include "check.h"
#include "magnifier.h"
#include "view.h"

/* AS-3: the zoom a press gives. */
static void
as3_magnifier_zoom_rule (void)
{
  /* 1x, 2x, 4x, 6x and 8x (REQ-0007-02 as amended by REQ-0008-02: no reduction levels). */
  CHECK (easel_magnifier_levels[0] == 100 && easel_magnifier_levels[1] == 200 && easel_magnifier_levels[2] == 400
         && easel_magnifier_levels[3] == 600 && easel_magnifier_levels[4] == 800);
  CHECK (EASEL_MAGNIFIER_N_LEVELS == 5);
  CHECK (easel_magnifier_levels[EASEL_MAGNIFIER_DEFAULT] == 200);               /* 2x at start-up */
  CHECK (easel_magnifier_zoom (100, 200) == 200);
  CHECK (easel_magnifier_zoom (200, 200) == 100);                  /* the same level again goes back */
  CHECK (easel_magnifier_zoom (100, 400) == 400);
  CHECK (easel_magnifier_zoom (400, 400) == 100);
  CHECK (easel_magnifier_zoom (200, 400) == 400);                  /* another level: the selected one */
  CHECK (easel_magnifier_zoom (400, 200) == 200);
  CHECK (easel_magnifier_zoom (42, 200) == 200);                   /* the fit zoom of a 3000 x 2000 picture */
  CHECK (easel_magnifier_zoom (300, 400) == 400);
  CHECK (easel_magnifier_zoom (800, 800) == 100);
  CHECK (easel_magnifier_zoom (800, 200) == 200);
  CHECK (easel_magnifier_zoom (10, 600) == 600);
  CHECK (easel_magnifier_zoom (100, 100) == 100);                  /* 1x: back to 100 %, and at 100 % nothing changes */
  CHECK (easel_magnifier_zoom (400, 100) == 100);
  /* Below 100 % (a Zoom Out step, 10, 25 or 50 %, or a Fit to Window zoom): a press gives the selected level, and 1x gives
     100 % (REQ-0007-03 as amended by REQ-0008-02; there is no reduction level). */
  CHECK (easel_magnifier_zoom (25, 400) == 400 && easel_magnifier_zoom (25, 100) == 100);
  CHECK (easel_magnifier_zoom (10, 200) == 200 && easel_magnifier_zoom (50, 800) == 800 && easel_magnifier_zoom (50, 100) == 100);
  CHECK (easel_magnifier_zoom (42, 100) == 100);
}

/* AS-4: the focus goes to the center of the view as far as the edges allow. */
static void
as4_focus_scroll (void)
{
  /* A 3000 x 2000 picture at 200 % in a view of 1390 x 853: content 6000 x 4000. */
  CHECK (fabs (easel_view_focus_scroll (1000.5, 200, 3000, 1.0, 1390) - (2001 - 695)) < 1e-9);
  CHECK (fabs (easel_view_focus_scroll (700.5, 200, 2000, 1.0, 853) - (1401 - 426.5)) < 1e-9);
  /* Near the top-left corner: the edge limits it, the view shows the corner. */
  CHECK (easel_view_focus_scroll (100.5, 200, 3000, 1.0, 1390) == 0);
  CHECK (easel_view_focus_scroll (50.5, 200, 2000, 1.0, 853) == 0);
  /* Near the bottom-right corner: the last offset that still fills the view. */
  CHECK (fabs (easel_view_focus_scroll (2950.5, 200, 3000, 1.0, 1390) - (6000 - 1390)) < 1e-9);
  CHECK (fabs (easel_view_focus_scroll (1980.5, 200, 2000, 1.0, 853) - (4000 - 853)) < 1e-9);
  /* A picture that fits (600 x 400 at 200 % is 1200 x 800): no scrolling. */
  CHECK (easel_view_focus_scroll (300, 200, 600, 1.0, 1390) == 0);
  CHECK (easel_view_focus_scroll (200, 200, 400, 1.0, 853) == 0);
  /* A scale of 2: device pixels are twice the logical ones, so the offset in logical pixels is half. */
  CHECK (fabs (easel_view_focus_scroll (1000.5, 200, 3000, 2.0, 695) - (1000.5 * 2 / 2 - 347.5)) < 1e-9);
}

/* AS-5: the preview box. */
static void
as5_magnifier_box (void)
{
  int x, y, w, h;

  /* A 3000 x 2000 picture, a canvas area of 1390 x 853, 2x, the pointer over (1000, 700). */
  easel_magnifier_box (1390, 853, 200, 3000, 2000, 1000, 700, &x, &y, &w, &h);
  CHECK (x == 653 && y == 487 && w == 695 && h == 426);
  CHECK (x + w - 1 == 1347 && y + h - 1 == 912);
  easel_magnifier_box (1390, 853, 800, 3000, 2000, 1000, 700, &x, &y, &w, &h);
  CHECK (w == 173 && h == 106 && x == 1000 - 86 && y == 700 - 53);
  easel_magnifier_box (1390, 853, 400, 3000, 2000, 1000, 700, &x, &y, &w, &h);
  CHECK (w == 347 && h == 213);
  easel_magnifier_box (1390, 853, 600, 3000, 2000, 1000, 700, &x, &y, &w, &h);
  CHECK (w == 231 && h == 142);

  /* The corners: moved inside the picture. */
  easel_magnifier_box (1390, 853, 200, 3000, 2000, 0, 0, &x, &y, &w, &h);
  CHECK (x == 0 && y == 0 && w == 695 && h == 426);
  easel_magnifier_box (1390, 853, 200, 3000, 2000, 2999, 1999, &x, &y, &w, &h);
  CHECK (x == 3000 - 695 && y == 2000 - 426);
  easel_magnifier_box (1390, 853, 200, 3000, 2000, 5, 1999, &x, &y, &w, &h);
  CHECK (x == 0 && y == 2000 - 426);

  /* A picture smaller than the box: the box spans it. */
  easel_magnifier_box (1390, 853, 200, 600, 400, 300, 200, &x, &y, &w, &h);
  CHECK (x == 0 && y == 0 && w == 600 && h == 400);
  easel_magnifier_box (1390, 853, 200, 600, 1000, 300, 900, &x, &y, &w, &h);       /* only in one direction */
  CHECK (x == 0 && w == 600 && h == 426 && y == 1000 - 426);

  /* At least one pixel, for a very small canvas area. */
  easel_magnifier_box (5, 3, 800, 100, 100, 50, 50, &x, &y, &w, &h);
  CHECK (w == 1 && h == 1 && x == 50 && y == 50);
}

int
main (void)
{
  RUN (as3_magnifier_zoom_rule);
  RUN (as4_focus_scroll);
  RUN (as5_magnifier_box);
  return 0;
}
