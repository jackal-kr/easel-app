/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Performance budget of feature 0006 (docs/engineering.md, "Pointer event to pixels on screen (pencil, brush)"): a motion of
   the Brush or the Eraser to the next painted frame, and one second of spray. Run with meson test --benchmark; needs a
   display. Prints numbers; it does not fail on a missed budget, because the budgets are judged by JOD. */

#include "perf-common.h"

/* One motion at a time with the given footprint, then the next painted frame. */
static void
measure_motions (const char *config_dir, int w, int h, const char *tool, EaselFootprint fp, const char *name)
{
  enum { N = 60 };
  double ms[N], work[N];
  EaselWindow *win;
  EaselCanvas *canvas;

  write_settings (config_dir, w, h);
  win = open_window ();
  canvas = easel_window_get_canvas (win);
  g_action_group_activate_action (G_ACTION_GROUP (win), "tool", g_variant_new_string (tool));
  easel_canvas_set_footprint (canvas, fp);
  easel_canvas_press (canvas, 1, 20.5, 20.5);
  for (int i = 0; i < N; i++)
    {
      gint64 start = g_get_monotonic_time ();

      easel_canvas_motion (canvas, 20.5 + 12 * i, 20.5 + 9 * (i % 7));
      work[i] = (g_get_monotonic_time () - start) / 1000.0;
      wait_for_paint (win);
      ms[i] = (g_get_monotonic_time () - start) / 1000.0;
    }
  easel_canvas_release (canvas, 1);
  qsort (ms, N, sizeof ms[0], cmp_double);
  qsort (work, N, sizeof work[0], cmp_double);
  g_print ("%s motion, %dx%d: work (stroke + texture) median %.2f ms, worst %.2f ms; to painted frame median %.1f ms, "
           "worst %.1f ms (%d motions)\n", name, w, h, work[N / 2], work[N - 1], ms[N / 2], ms[N - 1], N);
  gtk_window_destroy (GTK_WINDOW (win));
}

/* One second of the Airbrush (24) held still: the frames painted meanwhile and the sprays made. */
static void
measure_spray (const char *config_dir, int w, int h)
{
  enum { MAX = 400 };
  double frame[MAX];
  int n = 0;
  EaselWindow *win;
  EaselCanvas *canvas;
  gint64 end;

  write_settings (config_dir, w, h);
  win = open_window ();
  canvas = easel_window_get_canvas (win);
  g_action_group_activate_action (G_ACTION_GROUP (win), "tool", g_variant_new_string ("airbrush"));
  easel_canvas_set_footprint (canvas, easel_airbrush_presets[2]);
  end = g_get_monotonic_time () + G_USEC_PER_SEC;
  easel_canvas_press (canvas, 1, 100.5, 100.5);
  while (g_get_monotonic_time () < end && n < MAX)
    {
      gint64 start = g_get_monotonic_time ();

      wait_for_paint (win);
      frame[n++] = (g_get_monotonic_time () - start) / 1000.0;
    }
  {
    int sprays = easel_canvas_get_spray_count (canvas);

    easel_canvas_release (canvas, 1);
    qsort (frame, n, sizeof frame[0], cmp_double);
    g_print ("spray (24) held 1 s, %dx%d: %d sprays, %d frames, frame interval median %.1f ms, worst %.1f ms\n",
             w, h, sprays, n, frame[n / 2], frame[n - 1]);
  }
  gtk_window_destroy (GTK_WINDOW (win));
}

int
main (int argc, char **argv)
{
  g_autofree char *config_dir = g_dir_make_tmp ("easel-perf-XXXXXX", NULL);
  int sizes[2][2] = { { 1920, 1080 }, { 8000, 8000 } };

  g_setenv ("XDG_CONFIG_HOME", config_dir, TRUE);
  if (!gtk_init_check ())
    return 77;

  app = easel_application_new ();
  g_application_set_flags (G_APPLICATION (app), G_APPLICATION_NON_UNIQUE);
  g_application_register (G_APPLICATION (app), NULL, NULL);

  for (int i = 0; i < 2; i++)
    {
      measure_motions (config_dir, sizes[i][0], sizes[i][1], "brush", (EaselFootprint) { EASEL_SHAPE_CIRCLE, 7 }, "Brush circle 7");
      measure_motions (config_dir, sizes[i][0], sizes[i][1], "brush", (EaselFootprint) { EASEL_SHAPE_SLASH, 8 }, "Brush slash 8");
      measure_motions (config_dir, sizes[i][0], sizes[i][1], "eraser", (EaselFootprint) { EASEL_SHAPE_SQUARE, 10 }, "Eraser 10");
      measure_spray (config_dir, sizes[i][0], sizes[i][1]);
    }

  g_object_unref (app);
  return 0;
}
