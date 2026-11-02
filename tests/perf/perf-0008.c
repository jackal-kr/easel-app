/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Measurements of the 1600 % zoom (0008's change of 2026-10-28, T010): an 8000 x 8000 picture at 800 % and at 1600 %, the view in the
   middle: a scroll by a tenth of the page to the painted frame, a pencil motion to the painted frame, a frame with the 1-pixel grid on,
   and the process's memory (resident now and its peak). The view is drawn by the tiles in view, so 1600 % should cost what 800 % costs.
   Run in the real session with no other Easel window open and the pointer outside its window; it prints numbers and does not fail.
   Usage: perf-0008 */

#include "perf-common.h"

#include <stdio.h>
#include <string.h>

/* The process's memory in MB: "VmRSS" now, "VmHWM" its peak. */
static double
status_mb (const char *key)
{
  FILE *f = fopen ("/proc/self/status", "r");
  char line[256];
  long kb = 0;
  gsize n = strlen (key);

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

static void
median_worst (double *ms, int n, double *median, double *worst)
{
  qsort (ms, n, sizeof ms[0], cmp_double);
  *median = ms[n / 2];
  *worst = ms[n - 1];
}

static void
measure_zoom (EaselWindow *win, int zoom)
{
  enum { N = 30 };
  EaselCanvas *canvas = easel_window_get_canvas (win);
  GtkScrolledWindow *scroller = GTK_SCROLLED_WINDOW (gtk_widget_get_ancestor (GTK_WIDGET (canvas), GTK_TYPE_SCROLLED_WINDOW));
  GtkAdjustment *ha = gtk_scrolled_window_get_hadjustment (scroller), *va = gtk_scrolled_window_get_vadjustment (scroller);
  double scroll[N], motion[N], grid[N], sm, sw, mm, mw, gm, gw;
  double l;

  g_action_group_activate_action (G_ACTION_GROUP (win), "zoom-to", g_variant_new_int32 (zoom));
  wait_for_paint (win);
  gtk_adjustment_set_value (ha, (gtk_adjustment_get_upper (ha) - gtk_adjustment_get_page_size (ha)) / 2);
  gtk_adjustment_set_value (va, (gtk_adjustment_get_upper (va) - gtk_adjustment_get_page_size (va)) / 2);
  wait_for_paint (win);
  for (int i = 0; i < N; i++)
    {
      gint64 start = g_get_monotonic_time ();

      gtk_adjustment_set_value (ha, gtk_adjustment_get_value (ha) + (i % 2 ? -1 : 1) * gtk_adjustment_get_page_size (ha) / 10);
      wait_for_paint (win);
      scroll[i] = (g_get_monotonic_time () - start) / 1000.0;
    }
  /* a pencil stroke in the middle of the view, one picture pixel a motion */
  l = zoom / (100.0 * easel_canvas_get_scale (canvas));
  {
    double x0 = gtk_adjustment_get_value (ha) + gtk_adjustment_get_page_size (ha) / 2 - EASEL_VIEW_MARGIN;
    double y0 = gtk_adjustment_get_value (va) + gtk_adjustment_get_page_size (va) / 2 - EASEL_VIEW_MARGIN;

    g_action_group_activate_action (G_ACTION_GROUP (win), "tool", g_variant_new_string ("pencil"));
    easel_canvas_press (canvas, 1, x0 + l / 2, y0 + l / 2);
    for (int i = 0; i < N; i++)
      {
        gint64 start = g_get_monotonic_time ();

        easel_canvas_motion (canvas, x0 + (i + 1.5) * l, y0 + l / 2);
        wait_for_paint (win);
        motion[i] = (g_get_monotonic_time () - start) / 1000.0;
      }
    easel_canvas_release (canvas, 1);
    g_action_group_activate_action (G_ACTION_GROUP (win), "undo", NULL);
    wait_for_paint (win);
  }
  easel_canvas_set_grid (canvas, TRUE, 1, FALSE);
  wait_for_paint (win);
  for (int i = 0; i < N; i++)
    {
      gint64 start = g_get_monotonic_time ();

      gtk_widget_queue_draw (GTK_WIDGET (canvas));
      wait_for_paint (win);
      grid[i] = (g_get_monotonic_time () - start) / 1000.0;
    }
  easel_canvas_set_grid (canvas, FALSE, 1, FALSE);
  median_worst (scroll, N, &sm, &sw);
  median_worst (motion, N, &mm, &mw);
  median_worst (grid, N, &gm, &gw);
  g_print ("8000 x 8000 at %4d %%: a scroll to the frame median %.1f ms, worst %.1f; a pencil motion median %.1f, worst %.1f; a frame "
           "with the 1-pixel grid median %.1f, worst %.1f; resident %.0f MB, peak %.0f MB\n",
           zoom, sm, sw, mm, mw, gm, gw, status_mb ("VmRSS"), status_mb ("VmHWM"));
}

int
main (int argc, char **argv)
{
  g_autofree char *config_dir = g_dir_make_tmp ("easel-perf-XXXXXX", NULL);
  EaselWindow *win;

  g_setenv ("XDG_CONFIG_HOME", config_dir, TRUE);
  if (!gtk_init_check ())
    return 77;
  app = easel_application_new ();
  g_application_set_flags (G_APPLICATION (app), G_APPLICATION_NON_UNIQUE);
  g_application_register (G_APPLICATION (app), NULL, NULL);
  write_settings (config_dir, 8000, 8000);
  win = open_window ();
  measure_zoom (win, 800);
  measure_zoom (win, 1600);
  measure_zoom (win, 800);
  gtk_window_destroy (GTK_WINDOW (win));
  g_object_unref (app);
  return 0;
}
