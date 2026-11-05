/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Window Extras (0021): frame time's text and its pace, its key, and the thumbnail's first pixel. */

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "check.h"
#include "frame-time.h"
#include "settings.h"
#include "view.h"

/* AS-12 (REQ-0021-12): one decimal, a value at most once per 250 ms, and the key. */
static void
as12_frame_time_text (void)
{
  char text[32];
  EaselSettings s;
  const char *file;

  easel_frame_time_format (6.24, text, sizeof text);
  CHECK (strcmp (text, "6.2 ms") == 0);
  easel_frame_time_format (0, text, sizeof text);
  CHECK (strcmp (text, "0.0 ms") == 0);
  easel_frame_time_format (123.456, text, sizeof text);
  CHECK (strcmp (text, "123.5 ms") == 0);
  CHECK (easel_frame_time_due (1000000, 0));
  CHECK (easel_frame_time_due (250000, 0));
  CHECK (!easel_frame_time_due (249999, 0));
  CHECK (easel_frame_time_wait_ms (100000, 0) == 150);
  CHECK (easel_frame_time_wait_ms (249999, 0) == 1);
  CHECK (easel_frame_time_wait_ms (250000, 0) == 0);

  easel_settings_defaults (&s);
  CHECK (!s.frame_time);
  file = "[view]\nframe-time=on\n";
  easel_settings_parse (file, strlen (file), &s);
  CHECK (s.frame_time);
  file = "[view]\nframe-time=maybe\n";
  easel_settings_parse (file, strlen (file), &s);
  CHECK (!s.frame_time);
  CHECK (strstr (easel_settings_default_text, "frame-time=off\n") != NULL);
}

/* AS-7 (REQ-0021-07): the first pixel is the one whose drawn square holds the view's top-left device pixel, 0 while the margin is at that edge. */
static void
as7_first_cell (void)
{
  CHECK (easel_view_first_cell (0, EASEL_VIEW_MARGIN, 800, 1) == 0);
  CHECK (easel_view_first_cell (EASEL_VIEW_MARGIN, EASEL_VIEW_MARGIN, 800, 1) == 0);
  /* Eight device pixels a square at 800 %: 7 past the margin is still square 0, 8 is square 1. */
  CHECK (easel_view_first_cell (EASEL_VIEW_MARGIN + 7, EASEL_VIEW_MARGIN, 800, 1) == 0);
  CHECK (easel_view_first_cell (EASEL_VIEW_MARGIN + 8, EASEL_VIEW_MARGIN, 800, 1) == 1);
  /* The margin counts in logical pixels, the rest in device pixels: at scale 2 one logical pixel is two. */
  CHECK (easel_view_first_cell (EASEL_VIEW_MARGIN + 4, EASEL_VIEW_MARGIN, 800, 2) == 1);
  CHECK (easel_view_first_cell (EASEL_VIEW_MARGIN + 3.5, EASEL_VIEW_MARGIN, 800, 2) == 0);
  /* Before the first square the margin holds the edge. */
  CHECK (easel_view_first_cell (3, EASEL_VIEW_MARGIN, 800, 2) == 0);
  /* 129 %: the square that holds each device pixel, by the drawn edges. */
  for (int d = 0; d < 400; d++)
    {
      int k = easel_view_first_cell (EASEL_VIEW_MARGIN + d, EASEL_VIEW_MARGIN, 129, 1);

      CHECK (easel_view_edge (k, 129) <= d && d < easel_view_edge (k + 1, 129));
    }
}

/* AS-11 (REQ-0021-11): the card needs 172 + 2 x 12 by 132 + 2 x 12. */
static void
as11_card_fits (void)
{
  CHECK (easel_view_thumbnail_fits (196, 156));
  CHECK (!easel_view_thumbnail_fits (195, 156));
  CHECK (!easel_view_thumbnail_fits (196, 155));
  CHECK (easel_view_thumbnail_fits (1000, 1000));
}

int
main (void)
{
  RUN (as12_frame_time_text);
  RUN (as7_first_cell);
  RUN (as11_card_fits);
  return 0;
}
