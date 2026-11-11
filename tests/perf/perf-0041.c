/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Measurements of feature 0041 (T004): Crop to Selection in the core, on a 1920 x 1080 and an 8000 x 8000 picture, each case three times:
   the crop, its Undo and its Redo, in milliseconds, and the process's peak resident memory (VmHWM, started again from the present size
   before the case) against the resident size with the picture alone. Cases: marked-half (a marked rectangle of half the sides in the
   middle), marked-small (100 x 100: nearly every pixel is removed and kept by the step), float-half (a floating selection of half the sides
   moved so that it hangs a quarter of its size past the right and bottom edges) and float-full (a floating selection of the whole picture
   moved by half the sides: the worst log, nearly every pixel of the new picture differs from the cut). The whole path, from the window's
   action to the painted frame, is measured with the window's action (T006). Run in the real session with no Easel window open and the load
   noted by the caller. It prints numbers and does not fail.
   Usage: perf-0041 [SIZE [CASE]]   SIZE 1080 or 8000 (1920 x 1080 or 8000 x 8000); CASE a name above. */

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "history.h"
#include "selection.h"

#define RUNS 3

static double
now_ms (void)
{
  struct timespec t;

  clock_gettime (CLOCK_MONOTONIC, &t);
  return t.tv_sec * 1000.0 + t.tv_nsec / 1e6;
}

/* A number of /proc/self/status (VmRSS, VmHWM) in MB. */
static double
status_mb (const char *key)
{
  FILE *f = fopen ("/proc/self/status", "r");
  char line[256];
  long kb = 0;
  size_t n = strlen (key);

  while (f != NULL && fgets (line, sizeof line, f))
    if (strncmp (line, key, n) == 0 && line[n] == ':')
      {
        kb = atol (line + n + 1);
        break;
      }
  if (f != NULL)
    fclose (f);
  return kb / 1024.0;
}

/* The peak (VmHWM) starts again from the present resident size (Linux: "5" to clear_refs). */
static void
reset_peak (void)
{
  FILE *f = fopen ("/proc/self/clear_refs", "w");

  if (f != NULL)
    {
      fputs ("5", f);
      fclose (f);
    }
}

static void
pattern (EaselImage *img)
{
  for (size_t i = 0; i < (size_t) img->width * (size_t) img->height; i++)
    img->px[i] = 0xFF000000u | (uint32_t) (i * 2654435761u);
}

static const char *const cases[] = { "marked-half", "marked-small", "float-half", "float-full" };

static void
measure (int w, int h, int c)
{
  EaselImage *img = easel_image_new (w, h, 0xFFFFFFFFu);
  EaselHistory *hist = easel_history_new ((size_t) 8 << 30);
  EaselFloat f = { 0 };
  double crop[RUNS], undo[RUNS], redo[RUNS], base, peak, peak_first = 0;
  int x, y, cw, ch;
  bool floating = c >= 2;

  if (img == NULL || hist == NULL)
    {
      printf ("%s %d x %d: no memory for the picture\n", cases[c], w, h);
      return;
    }
  pattern (img);
  switch (c)
    {
    case 0: x = w / 4; y = h / 4; cw = w / 2; ch = h / 2; break;
    case 1: x = w / 2; y = h / 2; cw = 100; ch = 100; break;
    case 2: x = w * 3 / 4; y = h * 3 / 4; cw = w / 2; ch = h / 2; break;
    default: x = w / 2; y = h / 2; cw = w; ch = h; break;
    }
  base = status_mb ("VmRSS");
  reset_peak ();
  if (floating)
    {
      /* lifted at the top-left as the user lifts it, then moved: the lift is the move's cost, not the crop's */
      if (easel_selection_lift (&f, img, 0, 0, cw, ch, NULL, false, 0xFFFFFF00u, SIZE_MAX) != EASEL_SELECTION_DONE)
        {
          printf ("%s %d x %d: no memory for the lift\n", cases[c], w, h);
          return;
        }
      f.x = x;
      f.y = y;
    }
  for (int i = 0; i < RUNS; i++)
    {
      double t0 = now_ms (), t1, t2, t3;
      EaselSelectionResult r = easel_selection_crop (floating ? &f : NULL, img, hist, x, y, cw, ch, 0xFFFF0000u, SIZE_MAX);

      t1 = now_ms ();
      if (r != EASEL_SELECTION_DONE)
        {
          printf ("%s %d x %d: the crop answered %d\n", cases[c], w, h, (int) r);
          return;
        }
      easel_history_undo (hist, img);
      t2 = now_ms ();
      easel_history_redo (hist, img);
      t3 = now_ms ();
      easel_history_undo (hist, img);                      /* back to the picture, for the next run */
      if (i == 0)
        peak_first = status_mb ("VmHWM");                   /* one crop, undone, redone and undone: no earlier redoable step held */
      crop[i] = t1 - t0;
      undo[i] = t2 - t1;
      redo[i] = t3 - t2;
    }
  peak = status_mb ("VmHWM");
  printf ("%-12s %4d x %-4d -> %4d x %-4d: crop %.0f %.0f %.0f ms | undo %.0f %.0f %.0f ms | redo %.0f %.0f %.0f ms | resident %.0f MB with the "
          "picture, peak %.0f MB after the first run, %.0f MB after three (the later crops find the redoable step of the one before; the step counts %.0f MB)\n",
          cases[c], w, h, cw, ch, crop[0], crop[1], crop[2], undo[0], undo[1], undo[2], redo[0], redo[1], redo[2], base, peak_first, peak,
          (double) easel_history_bytes (hist) / (1024.0 * 1024.0));
  easel_float_free (&f);
  easel_history_free (hist);
  easel_image_free (img);
}

int
main (int argc, char **argv)
{
  int size = argc > 1 ? atoi (argv[1]) : 0;

  for (int pass = 0; pass < 2; pass++)
    {
      int w = pass == 0 ? 1920 : 8000, h = pass == 0 ? 1080 : 8000;

      if (size != 0 && size != h)
        continue;
      for (int c = 0; c < 4; c++)
        if (argc <= 2 || strcmp (argv[2], cases[c]) == 0)
          measure (w, h, c);
    }
  return 0;
}
