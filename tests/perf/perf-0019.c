/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Measurements of Stretch and Skew (0019, REQ-0019-16), in the core: an 8000 x 8000 picture and a floating selection of that size, up to the
   largest new size, and the process's peak memory with the step kept for Undo. Budgets: 500 ms up to 64 million pixels, 1 s at 10000 x 10000.
   It prints numbers and does not fail. Run in the real session with no other Easel window open. Usage: perf-0019 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "history.h"
#include "image.h"
#include "selection.h"
#include "stretch-skew.h"

enum { RUNS = 5 };

static double
now_ms (void)
{
  struct timespec t;

  clock_gettime (CLOCK_MONOTONIC, &t);
  return t.tv_sec * 1000.0 + t.tv_nsec / 1e6;
}

static int
cmp_double (const void *a, const void *b)
{
  double x = *(const double *) a, y = *(const double *) b;

  return x < y ? -1 : x > y;
}

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

static EaselImage *
picture (void)
{
  EaselImage *img = easel_image_new (8000, 8000, 0xFFFFFFFFu);

  for (size_t i = 0; i < (size_t) 8000 * 8000; i++)
    img->px[i] = 0xFF000000u | (uint32_t) (i * 2654435761u >> 8);
  return img;
}

static void
print_times (const char *what, double *ms, double budget)
{
  qsort (ms, RUNS, sizeof *ms, cmp_double);
  printf ("%-52s median %7.1f ms, worst %7.1f ms, budget %5.0f ms: %s\n", what, ms[RUNS / 2], ms[RUNS - 1], budget,
          ms[RUNS - 1] <= budget ? "within" : "OVER");
  fflush (stdout);
}

/* The whole picture to W x H and the amounts, then Undo. */
static void
on_picture (EaselImage *img, const char *what, int W, int H, int sh, int sv, double budget)
{
  double ms[RUNS];

  for (int r = 0; r < RUNS; r++)
    {
      EaselHistory *h = easel_history_new ((size_t) 4 << 30);
      double start = now_ms ();

      if (easel_image_stretch_skew (img, h, W, H, sh, sv, 0xFFFFFFFFu, SIZE_MAX) != EASEL_TRANSFORM_DONE)
        {
          printf ("%s: refused\n", what);
          exit (1);
        }
      ms[r] = now_ms () - start;
      easel_history_undo (h, img);
      easel_history_free (h);
    }
  print_times (what, ms, budget);
}

/* A float of the whole picture stretched, or skewed. */
static void
on_float (EaselImage *img, const char *what, int W, int H, int sh, int sv, double budget)
{
  double ms[RUNS];

  for (int r = 0; r < RUNS; r++)
    {
      EaselFloat f = { 0 };
      double start;

      if (easel_selection_lift (&f, img, 0, 0, img->width, img->height, NULL, false, 0xFFFFFFFFu, SIZE_MAX) != EASEL_SELECTION_DONE)
        exit (1);
      start = now_ms ();
      if (W != f.w || H != f.h)
        {
          if (easel_selection_stretch (&f, 0, 0, W, H, SIZE_MAX) != EASEL_SELECTION_DONE)
            exit (1);
          easel_selection_complete (&f);
        }
      if ((sh != 0 || sv != 0) && easel_selection_skew (&f, sh, sv, SIZE_MAX) != EASEL_SELECTION_DONE)
        exit (1);
      ms[r] = now_ms () - start;
      easel_float_free (&f);
    }
  print_times (what, ms, budget);
}

int
main (void)
{
  EaselImage *img = picture ();

  printf ("resident %.0f MB with the 8000 x 8000 picture\n", status_mb ("VmRSS"));
  on_picture (img, "picture, stretch to 50 % (4000 x 4000)", 4000, 4000, 0, 0, 500);
  on_picture (img, "picture, stretch to 50 % and skew 1 degree (4070 x 4000)", 4000, 4000, easel_ss_skew_amount (4000, 1), 0, 500);
  on_picture (img, "picture, stretch and skew to 8000 x 8000 (64 M)", 7900, 8000, 100, 0, 500);
  on_picture (img, "picture, skew vertical to 8000 x 8000 (64 M)", 8000, 7900, 0, 100, 500);
  on_picture (img, "picture, skew 2000 pixels (10000 x 8000, 80 M)", 8000, 8000, 2000, 0, 1000);
  on_picture (img, "picture, stretch to 10000 x 10000 (100 M)", 10000, 10000, 0, 0, 1000);
  on_float (img, "float, stretch to 50 % and skew 1 degree", 4000, 4000, easel_ss_skew_amount (4000, 1), 0, 500);
  on_float (img, "float, stretch and skew to 8000 x 8000 (64 M)", 7900, 8000, 100, 0, 500);
  on_float (img, "float, skew 2000 pixels (10000 x 8000, 80 M)", 8000, 8000, 2000, 0, 1000);
  on_float (img, "float, stretch to 10000 x 10000 (100 M)", 10000, 10000, 0, 0, 1000);
  printf ("resident %.0f MB, peak so far %.0f MB (8000 x 8000 cases)\n", status_mb ("VmRSS"), status_mb ("VmHWM"));
  /* Peak with the step kept, in a process of its own size: the new 10000 x 10000 picture and the old buffer held for Undo. */
  {
    EaselHistory *h = easel_history_new ((size_t) 4 << 30);
    FILE *refs = fopen ("/proc/self/clear_refs", "w");           /* the peak starts again from what is resident now */

    if (refs != NULL)
      {
        fputs ("5", refs);
        fclose (refs);
      }
    if (easel_image_stretch_skew (img, h, 10000, 10000, 0, 0, 0xFFFFFFFFu, SIZE_MAX) != EASEL_TRANSFORM_DONE)
      exit (1);
    printf ("10000 x 10000 with its undo step: history %.0f MB, resident %.0f MB, peak %.0f MB\n", (double) easel_history_bytes (h) / (1024 * 1024),
            status_mb ("VmRSS"), status_mb ("VmHWM"));
    easel_history_free (h);
  }
  easel_image_free (img);
  return 0;
}
