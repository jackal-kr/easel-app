/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Measurements of feature 0043 (the Plan's Risk 3, T007): a waiting Heart and Oval Callout (96 and 92 corners), width 5, Outline and
   fill, Sharp and Smooth, their bottom-right handle dragged in 30 steps from the picture's own box to 10000 x 10000, each motion to the
   next painted frame, at 1920 x 1080 (100 % and fitted) and 8000 x 8000 (fitted); then the put-down by Enter. The budget for a pointer
   event is one frame (engineering.md). Run in the real session with no Easel window open and the pointer outside the window; it prints
   numbers and does not fail; with PERF_0043_WORST set it also says which motion was each case's slowest. */

#include "perf-common.h"

#include <stdio.h>
#include <string.h>

static int
cmp_ms (const void *a, const void *b)
{
  double x = *(const double *) a, y = *(const double *) b;

  return x < y ? -1 : x > y;
}

static void
measure (const char *config_dir, int w, int h, const char *shape, gboolean smooth, gboolean fit)
{
  enum { N = 30 };
  double ms[N], put;
  EaselWindow *win;
  EaselCanvas *canvas;
  graphene_rect_t r;
  double zoom, hx, hy;
  gint64 start;
  int bx = w - 1, by = h - 1;

  write_settings (config_dir, w, h);
  win = open_window ();
  canvas = easel_window_get_canvas (win);
  g_action_group_change_action_state (G_ACTION_GROUP (win), "shape", g_variant_new_string (shape));
  g_action_group_activate_action (G_ACTION_GROUP (win), "tool", g_variant_new_string ("shapes"));
  g_action_group_change_action_state (G_ACTION_GROUP (win), "line-width", g_variant_new_int32 (5));
  g_action_group_change_action_state (G_ACTION_GROUP (win), "fill-style", g_variant_new_int32 (1));
  g_action_group_change_action_state (G_ACTION_GROUP (win), "smooth", g_variant_new_boolean (smooth));
  g_action_group_activate_action (G_ACTION_GROUP (win), fit ? "zoom-fit" : "zoom-normal", NULL);
  wait_for_paint (win);
  wait_for_paint (win);
  zoom = easel_window_get_zoom (win) / 100.0 / easel_canvas_get_scale (canvas);
  /* the first drag: the whole picture (at 100 % the part the view shows) */
  if (!fit)
    {
      bx = 600;
      by = 400;
    }
  easel_canvas_press (canvas, 1, 0.5 * zoom, 0.5 * zoom);
  easel_canvas_motion (canvas, (bx + 0.5) * zoom, (by + 0.5) * zoom);
  easel_canvas_release (canvas, 1);
  wait_for_paint (win);
  if (!easel_canvas_get_selection_handle_rect (canvas, EASEL_SEL_HANDLE_BR, &r))
    {
      g_print ("%s: no waiting shape\n", shape);
      return;
    }
  hx = r.origin.x + r.size.width / 2;
  hy = r.origin.y + r.size.height / 2;
  easel_canvas_press (canvas, 1, hx, hy);
  for (int i = 0; i < N; i++)
    {
      /* toward a box of 10000 x 10000 (at 100 %, 3 times the first box) */
      double tx = fit ? (10000.0 - (bx + 1)) * (i + 1) / N : 2.0 * (bx + 1) * (i + 1) / N;
      double ty = fit ? (10000.0 - (by + 1)) * (i + 1) / N : 2.0 * (by + 1) * (i + 1) / N;

      start = g_get_monotonic_time ();
      easel_canvas_motion (canvas, hx + tx * zoom, hy + ty * zoom);
      wait_for_paint (win);
      ms[i] = (g_get_monotonic_time () - start) / 1000.0;
    }
  easel_canvas_release (canvas, 1);
  wait_for_paint (win);
  start = g_get_monotonic_time ();
  easel_canvas_enter (canvas);
  wait_for_paint (win);
  put = (g_get_monotonic_time () - start) / 1000.0;
  {
    int worst = 0;

    for (int i = 1; i < N; i++)
      if (ms[i] > ms[worst])
        worst = i;
    if (g_getenv ("PERF_0043_WORST") != NULL)
      g_print ("  worst event: motion %d of %d\n", worst + 1, N);
  }
  qsort (ms, N, sizeof ms[0], cmp_ms);
  g_print ("%4dx%-4d %s %-26s %-6s handle drag: median %.1f ms, 90th %.1f, worst %.1f (to the painted frame) | put-down %.0f ms\n", w, h,
           fit ? "fit " : "100%", shape, smooth ? "smooth" : "sharp", ms[N / 2], ms[N * 9 / 10], ms[N - 1], put);
  gtk_window_destroy (GTK_WINDOW (win));
  while (g_main_context_pending (NULL))
    g_main_context_iteration (NULL, FALSE);
}

int
main (int argc, char **argv)
{
  g_autofree char *config_dir = g_dir_make_tmp ("easel-perf-XXXXXX", NULL);
  static const char *const shapes[] = { "heart", "oval-callout" };

  g_setenv ("XDG_CONFIG_HOME", config_dir, TRUE);
  if (!gtk_init_check ())
    return 77;
  app = easel_application_new ();
  g_application_set_flags (G_APPLICATION (app), G_APPLICATION_NON_UNIQUE);
  g_application_register (G_APPLICATION (app), NULL, NULL);
  for (int k = 0; k < 2; k++)
    for (int sm = 0; sm < 2; sm++)
      {
        measure (config_dir, 1920, 1080, shapes[k], sm, FALSE);
        measure (config_dir, 1920, 1080, shapes[k], sm, TRUE);
        measure (config_dir, 8000, 8000, shapes[k], sm, TRUE);
      }
  g_object_unref (app);
  return 0;
}
