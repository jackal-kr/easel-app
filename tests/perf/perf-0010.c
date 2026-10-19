/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Measurements of feature 0010 (the Plan's "Measurements"): a filled Ellipse and a 5-pixel Line being dragged, each pointer
   move to the next painted frame, and the release of a whole-picture filled Ellipse (time and peak memory). The budget for a
   pointer event is one frame (engineering.md); this prints numbers and does not fail. Run with meson test --benchmark;
   needs a display. */

#include "perf-common.h"

#include <stdio.h>
#include <string.h>

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

/* A drag from near the picture's top left toward its bottom right, as a growing shape: each move to the next painted
   frame. */
static void
measure_drag (const char *config_dir, int w, int h, const char *tool, int width, int style, gboolean fit)
{
  enum { N = 30 };
  double ms[N];
  EaselWindow *win;
  EaselCanvas *canvas;
  double scale, zoom;

  write_settings (config_dir, w, h);
  win = open_window ();
  canvas = easel_window_get_canvas (win);
  g_action_group_activate_action (G_ACTION_GROUP (win), "tool", g_variant_new_string (tool));
  g_action_group_change_action_state (G_ACTION_GROUP (win), "line-width", g_variant_new_int32 (width));
  g_action_group_change_action_state (G_ACTION_GROUP (win), "fill-style", g_variant_new_int32 (style));
  g_action_group_activate_action (G_ACTION_GROUP (win), fit ? "zoom-fit" : "zoom-normal", NULL);
  wait_for_paint (win);
  wait_for_paint (win);
  scale = easel_canvas_get_scale (canvas);
  zoom = easel_window_get_zoom (win) / 100.0 / scale;           /* canvas points per picture pixel */
  easel_canvas_press (canvas, 1, 2.5 * zoom, 2.5 * zoom);
  for (int i = 0; i < N; i++)
    {
      /* The end moves over what the view shows: up to 600 px of the picture at 100 %, the whole of it when fitted. */
      double span = fit ? (w < h ? w : h) - 10 : 600;
      double t = span * (i + 1) / N;
      gint64 start = g_get_monotonic_time ();

      easel_canvas_motion (canvas, (2.5 + t) * zoom, (2.5 + t * 0.6) * zoom);
      wait_for_paint (win);
      ms[i] = (g_get_monotonic_time () - start) / 1000.0;
    }
  easel_canvas_release (canvas, 1);
  qsort (ms, N, sizeof ms[0], cmp_double);
  g_print ("%s drag, width %d, %s, %dx%d, %s (zoom %d%%): to painted frame median %.1f ms, worst %.1f ms (%d moves)\n", tool,
           width, style == 2 ? "filled" : style == 1 ? "outline and fill" : "outline", w, h, fit ? "fit" : "100%",
           easel_window_get_zoom (win), ms[N / 2], ms[N - 1], N);
  gtk_window_destroy (GTK_WINDOW (win));
}

/* The release of a filled Ellipse over a whole picture, in one go, through the window. */
static void
measure_release (const char *config_dir, int w, int h)
{
  EaselWindow *win;
  EaselCanvas *canvas;
  double before = peak_mb ();
  double scale, zoom;
  gint64 start;

  write_settings (config_dir, w, h);
  win = open_window ();
  canvas = easel_window_get_canvas (win);
  g_action_group_activate_action (G_ACTION_GROUP (win), "tool", g_variant_new_string ("ellipse"));
  g_action_group_change_action_state (G_ACTION_GROUP (win), "fill-style", g_variant_new_int32 (2));
  g_action_group_activate_action (G_ACTION_GROUP (win), "zoom-fit", NULL);
  wait_for_paint (win);
  wait_for_paint (win);
  scale = easel_canvas_get_scale (canvas);
  zoom = easel_window_get_zoom (win) / 100.0 / scale;
  easel_canvas_press (canvas, 1, 0.5 * zoom, 0.5 * zoom);
  easel_canvas_motion (canvas, (w - 0.5) * zoom, (h - 0.5) * zoom);
  wait_for_paint (win);
  start = g_get_monotonic_time ();
  easel_canvas_release (canvas, 1);
  g_print ("release of a filled whole-picture Ellipse, %dx%d: %.0f ms to return; ", w, h,
           (g_get_monotonic_time () - start) / 1000.0);
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
      {
        measure_drag (config_dir, sizes[i][0], sizes[i][1], "ellipse", 1, 2, fit);
        measure_drag (config_dir, sizes[i][0], sizes[i][1], "line", 5, 0, fit);
      }
  measure_release (config_dir, 8000, 8000);
  g_object_unref (app);
  return 0;
}
