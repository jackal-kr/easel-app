/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Performance budget of feature 0002 (docs/engineering.md, "Undo / redo"): an undo or redo shows on screen at the next
   refresh on a 1920x1080 picture. Run with meson test --benchmark; needs a display. Prints numbers; it does not fail
   on a missed budget, because the budgets are judged by JOD. */

#include "perf-common.h"

/* A stroke of exactly 50,000 changed pixels: 50 zigzag rows of 1,000 pixels at the picture's top-left. */
static void
draw_50000_pixel_stroke (EaselCanvas *canvas)
{
  easel_canvas_press (canvas, 1, 0.5, 0.5);
  easel_canvas_motion (canvas, 999.5, 0.5);
  for (int r = 1; r < 50; r++)
    {
      easel_canvas_motion (canvas, r % 2 ? 999.5 : 0.5, r + 0.5);
      easel_canvas_motion (canvas, r % 2 ? 0.5 : 999.5, r + 0.5);
    }
  easel_canvas_release (canvas, 1);
}

static void
report (const char *what, int w, int h, double *ms, double *work, int n)
{
  qsort (ms, n, sizeof ms[0], cmp_double);
  qsort (work, n, sizeof work[0], cmp_double);
  g_print ("%s of a 50,000-pixel stroke, %dx%d: action %.2f ms median (%.2f worst); on screen %.1f ms median "
           "(%.1f worst) (%d runs)\n",
           what, w, h, work[n / 2], work[n - 1], ms[n / 2], ms[n - 1], n);
}

static void
measure_undo_redo (const char *config_dir, int w, int h)
{
  enum { N = 20 };
  double undo_ms[N], undo_work[N], redo_ms[N], redo_work[N];
  EaselWindow *win;
  EaselCanvas *canvas;

  write_settings (config_dir, w, h);
  win = open_window ();
  canvas = easel_window_get_canvas (win);
  g_action_group_activate_action (G_ACTION_GROUP (win), "tool", g_variant_new_string ("pencil"));
  draw_50000_pixel_stroke (canvas);
  wait_for_paint (win);

  for (int i = 0; i < N; i++)
    {
      gint64 start = g_get_monotonic_time ();

      g_action_group_activate_action (G_ACTION_GROUP (win), "undo", NULL);
      undo_work[i] = (g_get_monotonic_time () - start) / 1000.0;
      wait_for_paint (win);
      undo_ms[i] = (g_get_monotonic_time () - start) / 1000.0;

      start = g_get_monotonic_time ();
      g_action_group_activate_action (G_ACTION_GROUP (win), "redo", NULL);
      redo_work[i] = (g_get_monotonic_time () - start) / 1000.0;
      wait_for_paint (win);
      redo_ms[i] = (g_get_monotonic_time () - start) / 1000.0;
    }
  report ("undo", w, h, undo_ms, undo_work, N);
  report ("redo", w, h, redo_ms, redo_work, N);
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

  measure_undo_redo (config_dir, 1920, 1080);
  measure_undo_redo (config_dir, 8000, 8000);

  g_object_unref (app);
  return 0;
}
