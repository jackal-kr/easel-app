/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Performance budgets of feature 0001 (docs/engineering.md). Run with meson test --benchmark; needs a display.
   Prints numbers; it does not fail on a missed budget, because the budgets are judged by JOD. */

#include "perf-common.h"

/* Pointer event to pixels on screen: one pencil motion, then the next painted frame. */
static void
measure_stroke (const char *config_dir, int w, int h)
{
  enum { N = 60 };
  double ms[N], work[N];
  EaselWindow *win;
  EaselCanvas *canvas;
  gint64 t0;

  write_settings (config_dir, w, h);
  t0 = g_get_monotonic_time ();
  win = open_window ();
  g_print ("open %dx%d to first frame: %.1f ms\n", w, h, (g_get_monotonic_time () - t0) / 1000.0);

  canvas = easel_window_get_canvas (win);
  g_action_group_activate_action (G_ACTION_GROUP (win), "tool", g_variant_new_string ("pencil"));
  easel_canvas_press (canvas, 1, 10.5, 10.5);
  for (int i = 0; i < N; i++)
    {
      gint64 start = g_get_monotonic_time ();

      easel_canvas_motion (canvas, 10.5 + 5 * i, 10.5 + 3 * (i % 7));
      work[i] = (g_get_monotonic_time () - start) / 1000.0;
      wait_for_paint (win);
      ms[i] = (g_get_monotonic_time () - start) / 1000.0;
    }
  easel_canvas_release (canvas, 1);
  qsort (ms, N, sizeof ms[0], cmp_double);
  qsort (work, N, sizeof work[0], cmp_double);
  g_print ("pencil motion work (core stroke + texture update), %dx%d: median %.2f ms, worst %.2f ms\n",
           w, h, work[N / 2], work[N - 1]);
  g_print ("pencil motion to painted frame, %dx%d: median %.1f ms, worst %.1f ms (%d motions)\n",
           w, h, ms[N / 2], ms[N - 1], N);
  gtk_window_destroy (GTK_WINDOW (win));
}

int
main (int argc, char **argv)
{
  gint64 t_main = g_get_monotonic_time ();
  g_autofree char *config_dir = g_dir_make_tmp ("easel-perf-XXXXXX", NULL);
  EaselWindow *win;

  g_setenv ("XDG_CONFIG_HOME", config_dir, TRUE);
  if (!gtk_init_check ())
    return 77;

  app = easel_application_new ();
  g_application_set_flags (G_APPLICATION (app), G_APPLICATION_NON_UNIQUE);
  g_application_register (G_APPLICATION (app), NULL, NULL);

  write_settings (config_dir, 1024, 768);
  win = open_window ();
  g_print ("start (main to first frame, 1024x768): %.1f ms\n", (g_get_monotonic_time () - t_main) / 1000.0);
  gtk_window_destroy (GTK_WINDOW (win));

  measure_stroke (config_dir, 1920, 1080);
  measure_stroke (config_dir, 8000, 8000);

  g_object_unref (app);
  return 0;
}
