/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Performance budgets of feature 0004 (docs/engineering.md): a pencil stroke motion, an undo and a redo shown at the next
   refresh on 1920x1080, and drawing at 8000x8000 without freezing. Run with meson test --benchmark; needs a display, and
   the window must stay visible while it runs (a hidden window gets no frames). Prints numbers; it does not fail on a
   missed budget, because the budgets are judged by JOD. */

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
  g_print ("%s, %dx%d: action %.2f ms median (%.2f worst); on screen %.1f ms median (%.1f worst) (%d runs); bytes copied "
           "by the canvas so far: see the last line\n",
           what, w, h, work[n / 2], work[n - 1], ms[n / 2], ms[n - 1], n);
}

static void
measure (const char *config_dir, int w, int h)
{
  enum { N_MOTION = 60, N_PAIRS = 20 };
  double ms[N_MOTION], work[N_MOTION];
  double undo_ms[N_PAIRS], undo_work[N_PAIRS], redo_ms[N_PAIRS], redo_work[N_PAIRS];
  EaselWindow *win;
  EaselCanvas *canvas;
  gint64 t0;

  write_settings (config_dir, w, h);
  t0 = g_get_monotonic_time ();
  win = open_window ();
  g_print ("open %dx%d to first frame: %.1f ms (renderer %s)\n", w, h, (g_get_monotonic_time () - t0) / 1000.0,
           G_OBJECT_TYPE_NAME (gtk_native_get_renderer (GTK_NATIVE (win))));
  canvas = easel_window_get_canvas (win);
  g_action_group_activate_action (G_ACTION_GROUP (win), "tool", g_variant_new_string ("pencil"));

  /* One long stroke, one motion at a time: pointer event to the next painted frame. */
  easel_canvas_press (canvas, 1, 10.5, 10.5);
  for (int i = 0; i < N_MOTION; i++)
    {
      gint64 start = g_get_monotonic_time ();

      easel_canvas_motion (canvas, 10.5 + 5 * i, 10.5 + 3 * (i % 7));
      work[i] = (g_get_monotonic_time () - start) / 1000.0;
      wait_for_paint (win);
      ms[i] = (g_get_monotonic_time () - start) / 1000.0;
    }
  easel_canvas_release (canvas, 1);
  report ("pencil motion", w, h, ms, work, N_MOTION);

  /* Undo and redo of a 50,000-pixel stroke. */
  draw_50000_pixel_stroke (canvas);
  wait_for_paint (win);
  for (int i = 0; i < N_PAIRS; i++)
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
  report ("undo of a 50,000-pixel stroke", w, h, undo_ms, undo_work, N_PAIRS);
  report ("redo of a 50,000-pixel stroke", w, h, redo_ms, redo_work, N_PAIRS);
  g_print ("bytes of the picture copied by the canvas, %dx%d: %" G_GSIZE_FORMAT "\n", w, h,
           easel_canvas_get_bytes_copied (canvas));
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

  measure (config_dir, 1920, 1080);
  measure (config_dir, 4000, 4000);
  measure (config_dir, 8000, 8000);

  g_object_unref (app);
  return 0;
}
