/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Measurements of feature 0034 (T002, T003, REQ-0034-09): each conversion of an 8000 x 8000 picture, and of a floating selection of it,
   in the core, three runs, against 500 ms; a
   photo-like pattern (every pixel's color different, the grays spread) so that Error diffusion carries errors everywhere. Run in the
   real session with the load noted by the caller. It prints numbers and does not fail. */

#include <stdint.h>
#include <stdio.h>
#include <time.h>

#include "black-white.h"
#include "history.h"
#include "selection.h"

#define RUNS 3
#define SIDE 8000

static double
now_ms (void)
{
  struct timespec t;

  clock_gettime (CLOCK_MONOTONIC, &t);
  return t.tv_sec * 1000.0 + t.tv_nsec / 1e6;
}

static void
pattern (EaselImage *img)
{
  for (size_t i = 0; i < (size_t) img->width * (size_t) img->height; i++)
    img->px[i] = 0xFF000000u | (uint32_t) (i * 2654435761u);
}

int
main (void)
{
  static const char *const names[] = { "threshold", "ordered", "diffusion" };
  EaselImage *img = easel_image_new (SIDE, SIDE, 0);
  EaselHistory *hist = easel_history_new ((size_t) 4 << 30);

  for (int how = 0; how < 3; how++)
    {
      double ms[RUNS];

      for (int i = 0; i < RUNS; i++)
        {
          double start;

          pattern (img);
          start = now_ms ();
          easel_image_black_white (img, hist, (EaselBw) how, (size_t) -1);
          ms[i] = now_ms () - start;
          easel_history_undo (hist, img);
        }
      printf ("picture %-9s %d x %d: core %.0f %.0f %.0f ms (%s < 500)\n", names[how], SIDE, SIDE, ms[0], ms[1], ms[2],
               (ms[0] < 500 && ms[1] < 500 && ms[2] < 500) ? "within" : "OVER");
    }
  /* A floating selection of the whole picture (T003): lifted afresh before each run, the lift not timed. */
  for (int how = 0; how < 3; how++)
    {
      double ms[RUNS];

      for (int i = 0; i < RUNS; i++)
        {
          EaselFloat f = { 0 };
          double start;

          pattern (img);
          if (easel_selection_lift (&f, img, 0, 0, SIDE, SIDE, NULL, false, 0xFFFFFFFFu, (size_t) -1) != EASEL_SELECTION_DONE)
            return 1;
          start = now_ms ();
          easel_selection_black_white (&f, (EaselBw) how, (size_t) -1);
          ms[i] = now_ms () - start;
          easel_float_free (&f);
        }
      printf ("float   %-9s %d x %d: core %.0f %.0f %.0f ms (%s < 500)\n", names[how], SIDE, SIDE, ms[0], ms[1], ms[2],
              (ms[0] < 500 && ms[1] < 500 && ms[2] < 500) ? "within" : "OVER");
    }
  easel_history_free (hist);
  easel_image_free (img);
  return 0;
}
