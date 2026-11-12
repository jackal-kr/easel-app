/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Measurements of feature 0039 (REQ-0039-10, AS-8): with Smooth, an Ellipse (outline and fill, width 2) being dragged, each pointer
   move to the next painted frame, at 1920 x 1080 at 100 % and at 8000 x 8000 fitted, where the last moves make it as large as the
   picture; the release of a whole-picture smooth Ellipse; and a smooth Brush stroke with the circle of 7 at 1920 x 1080, each move to
   the next painted frame (the pointer-to-screen budget, one frame). Budgets: a frame (16 ms) at 1920 x 1080, 100 ms at 8000 x 8000. This
   prints numbers and does not fail. Run with meson test --benchmark; needs a display. */

#include "perf-common.h"

#include <stdio.h>

static void
measure_drag (const char *config_dir, int w, int h, gboolean fit)
{
  enum { N = 30 };
  double ms[N], last = 0;
  EaselWindow *win;
  EaselCanvas *canvas;
  double scale, zoom, span;

  write_settings (config_dir, w, h);
  win = open_window ();
  canvas = easel_window_get_canvas (win);
  g_action_group_change_action_state (G_ACTION_GROUP (win), "shape", g_variant_new_string ("ellipse"));   /* 0043 */
  g_action_group_activate_action (G_ACTION_GROUP (win), "tool", g_variant_new_string ("shapes"));
  g_action_group_change_action_state (G_ACTION_GROUP (win), "line-width", g_variant_new_int32 (2));
  g_action_group_change_action_state (G_ACTION_GROUP (win), "fill-style", g_variant_new_int32 (1));
  g_action_group_change_action_state (G_ACTION_GROUP (win), "smooth", g_variant_new_boolean (TRUE));
  g_action_group_activate_action (G_ACTION_GROUP (win), fit ? "zoom-fit" : "zoom-normal", NULL);
  wait_for_paint (win);
  wait_for_paint (win);
  scale = easel_canvas_get_scale (canvas);
  zoom = easel_window_get_zoom (win) / 100.0 / scale;
  span = fit ? (w < h ? w : h) - 1 : 600;
  easel_canvas_press (canvas, 1, 0.5 * zoom, 0.5 * zoom);
  for (int i = 0; i < N; i++)
    {
      double t = span * (i + 1) / N;
      gint64 start = g_get_monotonic_time ();

      easel_canvas_motion (canvas, (0.5 + t) * zoom, (0.5 + t * (fit ? 1.0 : 0.6)) * zoom);
      wait_for_paint (win);
      ms[i] = (g_get_monotonic_time () - start) / 1000.0;
    }
  last = ms[N - 1];
  easel_canvas_release (canvas, 1);
  qsort (ms, N, sizeof ms[0], cmp_double);
  g_print ("smooth ellipse drag, %dx%d, %s (zoom %d%%): to painted frame median %.1f ms, worst %.1f ms, the largest %.1f ms (%d moves)\n",
           w, h, fit ? "fit" : "100%", easel_window_get_zoom (win), ms[N / 2], ms[N - 1], last, N);
  gtk_window_destroy (GTK_WINDOW (win));
}

static void
measure_release (const char *config_dir, int w, int h)
{
  EaselWindow *win;
  EaselCanvas *canvas;
  double scale, zoom;
  gint64 start;

  write_settings (config_dir, w, h);
  win = open_window ();
  canvas = easel_window_get_canvas (win);
  g_action_group_change_action_state (G_ACTION_GROUP (win), "shape", g_variant_new_string ("ellipse"));   /* 0043 */
  g_action_group_activate_action (G_ACTION_GROUP (win), "tool", g_variant_new_string ("shapes"));
  g_action_group_change_action_state (G_ACTION_GROUP (win), "fill-style", g_variant_new_int32 (1));
  g_action_group_change_action_state (G_ACTION_GROUP (win), "smooth", g_variant_new_boolean (TRUE));
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
  g_print ("release of a smooth whole-picture Ellipse, %dx%d: %.0f ms to return; ", w, h, (g_get_monotonic_time () - start) / 1000.0);
  wait_for_paint (win);
  g_print ("%.0f ms to the next painted frame\n", (g_get_monotonic_time () - start) / 1000.0);
  gtk_window_destroy (GTK_WINDOW (win));
}

/* A Brush stroke with the circle of 7, Smooth, at 100 %: N moves of 23 pixels in a zigzag, each to the next painted frame. */
static void
measure_brush (const char *config_dir, int w, int h)
{
  enum { N = 60 };
  double ms[N];
  EaselWindow *win;
  EaselCanvas *canvas;
  double scale, zoom, x = 40, y = 40;

  write_settings (config_dir, w, h);
  win = open_window ();
  canvas = easel_window_get_canvas (win);
  g_action_group_activate_action (G_ACTION_GROUP (win), "tool", g_variant_new_string ("brush"));
  g_action_group_change_action_state (G_ACTION_GROUP (win), "smooth", g_variant_new_boolean (TRUE));
  easel_canvas_set_footprint (canvas, (EaselFootprint) { EASEL_SHAPE_CIRCLE, 7 });
  g_action_group_activate_action (G_ACTION_GROUP (win), "zoom-normal", NULL);
  wait_for_paint (win);
  wait_for_paint (win);
  scale = easel_canvas_get_scale (canvas);
  zoom = easel_window_get_zoom (win) / 100.0 / scale;
  easel_canvas_press (canvas, 1, x * zoom, y * zoom);
  for (int i = 0; i < N; i++)
    {
      gint64 start = g_get_monotonic_time ();

      x += 20;
      y += i % 2 == 0 ? 11 : -11;
      easel_canvas_motion (canvas, x * zoom, y * zoom);
      wait_for_paint (win);
      ms[i] = (g_get_monotonic_time () - start) / 1000.0;
    }
  easel_canvas_release (canvas, 1);
  qsort (ms, N, sizeof ms[0], cmp_double);
  g_print ("smooth brush, circle of 7, %dx%d, 100%%: to painted frame median %.1f ms, worst %.1f ms (%d moves)\n", w, h, ms[N / 2],
           ms[N - 1], N);
  gtk_window_destroy (GTK_WINDOW (win));
}

int
main (int argc, char **argv)
{
  g_autofree char *config_dir = g_dir_make_tmp ("easel-perf-XXXXXX", NULL);

  g_setenv ("XDG_CONFIG_HOME", config_dir, TRUE);
  if (!gtk_init_check ())
    return 77;
  app = easel_application_new ();
  g_application_set_flags (G_APPLICATION (app), G_APPLICATION_NON_UNIQUE);
  g_application_register (G_APPLICATION (app), NULL, NULL);
  measure_drag (config_dir, 1920, 1080, FALSE);
  measure_drag (config_dir, 1920, 1080, TRUE);
  measure_drag (config_dir, 8000, 8000, TRUE);
  measure_release (config_dir, 1920, 1080);
  measure_release (config_dir, 8000, 8000);
  measure_brush (config_dir, 1920, 1080);
  g_object_unref (app);
  return 0;
}
