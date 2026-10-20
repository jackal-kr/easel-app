/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Measurements of feature 0013 (the Plan's "Measurements"): a floating selection of 400 x 300 being dragged, each pointer
   move to the next painted frame, with the peak memory the drag adds, at 1920 x 1080 and 8000 x 8000, at 100 % and at the
   fit zoom; and the put-down of the whole 8000 x 8000 picture moved by one pixel (time and peak memory). The budget for a
   pointer event is one frame (engineering.md); this prints numbers and does not fail. Run with meson test --benchmark; needs
   a display. */

#include "perf-common.h"

#include <stdio.h>

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

/* The canvas position of the center of picture pixel (x, y). */
static void
at (EaselWindow *win, int x, int y, double *cx, double *cy)
{
  double z = easel_window_get_zoom (win) / 100.0 / easel_canvas_get_scale (easel_window_get_canvas (win));

  *cx = (x + 0.5) * z;
  *cy = (y + 0.5) * z;
}

static EaselWindow *
prepared_window (const char *config_dir, int w, int h, gboolean fit)
{
  EaselWindow *win;

  write_settings (config_dir, w, h);
  win = open_window ();
  g_action_group_activate_action (G_ACTION_GROUP (win), "tool", g_variant_new_string ("select"));
  g_action_group_activate_action (G_ACTION_GROUP (win), fit ? "zoom-fit" : "zoom-normal", NULL);
  wait_for_paint (win);
  wait_for_paint (win);
  return win;
}

/* A selection of 400 x 300 marked, then dragged across the picture. */
static void
measure_drag (const char *config_dir, int w, int h, gboolean fit)
{
  enum { N = 30 };
  double ms[N], cx, cy, before;
  EaselWindow *win = prepared_window (config_dir, w, h, fit);
  EaselCanvas *canvas = easel_window_get_canvas (win);

  before = peak_mb ();
  at (win, 100, 100, &cx, &cy);
  easel_canvas_press (canvas, 1, cx, cy);
  at (win, 499, 399, &cx, &cy);
  easel_canvas_motion (canvas, cx, cy);
  easel_canvas_release (canvas, 1);
  at (win, 300, 250, &cx, &cy);
  easel_canvas_press (canvas, 1, cx, cy);                     /* on the selection: a move */
  for (int i = 0; i < N; i++)
    {
      gint64 start = g_get_monotonic_time ();

      at (win, 300 + (i + 1) * (w / 4 - 300) / N, 250 + (i + 1) * (h / 4 - 250) / N, &cx, &cy);
      easel_canvas_motion (canvas, cx, cy);
      wait_for_paint (win);
      ms[i] = (g_get_monotonic_time () - start) / 1000.0;
    }
  easel_canvas_release (canvas, 1);
  qsort (ms, N, sizeof ms[0], cmp_double);
  g_print ("selection dragged, %dx%d, %s (zoom %d%%): to painted frame median %.1f ms, worst %.1f ms (%d moves); "
           "peak memory %.0f MB (was %.0f MB)\n", w, h, fit ? "fit" : "100%", easel_window_get_zoom (win), ms[N / 2],
           ms[N - 1], N, peak_mb (), before);
  easel_canvas_put_down (canvas);
  gtk_window_destroy (GTK_WINDOW (win));
}

/* The whole picture selected, moved by one pixel and put down: every pixel changes (the picture holds two colors in
   stripes first), so the step is as large as it gets. */
static void
measure_put_down (const char *config_dir, int w, int h)
{
  EaselWindow *win = prepared_window (config_dir, w, h, TRUE);
  EaselCanvas *canvas = easel_window_get_canvas (win);
  EaselImage *img = (EaselImage *) easel_window_get_picture (win);
  double before;
  gint64 start;

  for (int y = 0; y < h; y++)
    for (int x = 0; x < w; x++)
      easel_image_set (img, x, y, (x & 1) ? 0xFFFF0000u : 0xFFFFFFFFu);
  easel_canvas_update (canvas);
  g_action_group_activate_action (G_ACTION_GROUP (win), "select-all", NULL);
  wait_for_paint (win);
  before = peak_mb ();
  start = g_get_monotonic_time ();
  easel_canvas_move_selection (canvas, 1, 0);                  /* the lift: the whole picture copied */
  g_print ("lift of the whole %dx%d picture: %.0f ms to return; ", w, h, (g_get_monotonic_time () - start) / 1000.0);
  wait_for_paint (win);
  g_print ("%.0f ms to the next painted frame; ", (g_get_monotonic_time () - start) / 1000.0);
  start = g_get_monotonic_time ();
  easel_canvas_put_down (canvas);
  g_print ("put-down: %.0f ms to return; ", (g_get_monotonic_time () - start) / 1000.0);
  wait_for_paint (win);
  g_print ("%.0f ms to the next painted frame; peak memory %.0f MB (was %.0f MB)\n",
           (g_get_monotonic_time () - start) / 1000.0, peak_mb (), before);
  gtk_window_destroy (GTK_WINDOW (win));
}

int
main (int argc, char **argv)
{
  g_autofree char *config_dir = g_dir_make_tmp ("easel-perf-XXXXXX", NULL);
  static const int sizes[][2] = { { 1920, 1080 }, { 8000, 8000 } };

  g_setenv ("XDG_CONFIG_HOME", config_dir, TRUE);
  if (!gtk_init_check ())
    return 77;
  app = easel_application_new ();
  g_application_set_flags (G_APPLICATION (app), G_APPLICATION_NON_UNIQUE);
  g_application_register (G_APPLICATION (app), NULL, NULL);
  for (guint i = 0; i < G_N_ELEMENTS (sizes); i++)
    for (int fit = 0; fit <= 1; fit++)
      measure_drag (config_dir, sizes[i][0], sizes[i][1], fit);
  measure_put_down (config_dir, 8000, 8000);
  g_object_unref (app);
  return 0;
}
