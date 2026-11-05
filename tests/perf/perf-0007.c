/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Measurements of feature 0007 (the Plan's "Measurements"): whole-picture fills in the core (time and peak memory), a Fill
   press and a Magnifier press to the next painted frame. engineering.md has no budget for Fill; this prints numbers and
   does not fail, because the budget is JOD's to set. Run with meson test --benchmark; needs a display. */

#include "perf-common.h"

#include <stdio.h>
#include <string.h>

#include "fill.h"
#include "history.h"
#include "image.h"

/* The peak resident memory of the process so far, in MB (VmHWM). */
static double
peak_mb (void)
{
  FILE *f = fopen ("/proc/self/status", "r");
  char line[256];
  long kb = 0;

  while (f != NULL && fgets (line, sizeof line, f))
    if (sscanf (line, "VmHWM: %ld kB", &kb) == 1)
      break;
  if (f != NULL)
    fclose (f);
  return kb / 1024.0;
}

static void
measure_core_fill (int w, int h, gboolean comb)
{
  EaselImage *img = easel_image_new (w, h, 0xFFFFFFFFu);
  EaselHistory *history = easel_history_new ((size_t) 16 << 20);
  size_t changed = 0;
  double before = peak_mb ();
  gint64 start;
  EaselFillResult r;

  if (comb)
    for (int x = 1; x < w; x += 2)
      for (int y = 1; y < h; y++)
        easel_image_set (img, x, y, 0xFF000000u);
  start = g_get_monotonic_time ();
  r = easel_fill (img, history, 0, 0, 0xFF0000FFu, &changed);
  g_print ("core fill, %s %dx%d: result %d, %zu pixels in %.1f ms; peak memory %.0f MB (was %.0f MB before the fill)\n",
           comb ? "comb" : "blank", w, h, r, changed, (g_get_monotonic_time () - start) / 1000.0, peak_mb (), before);
  easel_history_free (history);
  easel_image_free (img);
}

/* One press and release at canvas point (x, y) with the tool, then the next painted frame. */
static void
measure_press (const char *config_dir, int w, int h, const char *tool, const char *what)
{
  enum { N = 20 };
  double ms[N];
  EaselWindow *win;
  EaselCanvas *canvas;

  write_settings (config_dir, w, h);
  win = open_window ();
  canvas = easel_window_get_canvas (win);
  g_action_group_activate_action (G_ACTION_GROUP (win), "tool", g_variant_new_string (tool));
  if (strcmp (tool, "fill") == 0)
    {
      /* A walled 10 x 10 area (100 pixels) to fill. */
      EaselImage *img = (EaselImage *) easel_window_get_picture (win);

      for (int i = 0; i < 12; i++)
        {
          easel_image_set (img, 100 + i, 100, 0xFF000000u);
          easel_image_set (img, 100 + i, 111, 0xFF000000u);
          easel_image_set (img, 100, 100 + i, 0xFF000000u);
          easel_image_set (img, 111, 100 + i, 0xFF000000u);
        }
    }
  for (int i = 0; i < N; i++)
    {
      gint64 start = g_get_monotonic_time ();

      if (strcmp (tool, "fill") == 0)
        {
          /* Alternate the colors so that each press has something to fill. */
          easel_canvas_set_colors (canvas, i % 2 ? 0xFF00FF00u : 0xFFFF0000u, 0xFFFFFFFFu);
          easel_canvas_press (canvas, 1, 105.5, 105.5);
        }
      else
        easel_canvas_press (canvas, 1, 105.5, 105.5);
      easel_canvas_release (canvas, 1);
      wait_for_paint (win);
      ms[i] = (g_get_monotonic_time () - start) / 1000.0;
    }
  qsort (ms, N, sizeof ms[0], cmp_double);
  g_print ("%s press, %dx%d%s: to painted frame median %.1f ms, worst %.1f ms (%d presses)\n", what, w, h,
           strcmp (tool, "magnifier") == 0 ? " (alternating zoom in and out)" : "", ms[N / 2], ms[N - 1], N);
  gtk_window_destroy (GTK_WINDOW (win));
}

int
main (int argc, char **argv)
{
  g_autofree char *config_dir = g_dir_make_tmp ("easel-perf-XXXXXX", NULL);

  g_setenv ("XDG_CONFIG_HOME", config_dir, TRUE);
  if (!gtk_init_check ())
    return 77;

  measure_core_fill (1920, 1080, FALSE);
  measure_core_fill (1800, 1800, TRUE);
  measure_core_fill (8000, 8000, FALSE);

  app = easel_application_new ();
  g_application_set_flags (G_APPLICATION (app), G_APPLICATION_NON_UNIQUE);
  g_application_register (G_APPLICATION (app), NULL, NULL);
  measure_press (config_dir, 1920, 1080, "fill", "Fill (100 pixels)");
  measure_press (config_dir, 8000, 8000, "fill", "Fill (100 pixels)");
  measure_press (config_dir, 1920, 1080, "magnifier", "Magnifier");
  measure_press (config_dir, 8000, 8000, "magnifier", "Magnifier");
  g_object_unref (app);
  return 0;
}
