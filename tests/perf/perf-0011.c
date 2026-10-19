/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Measurements of feature 0011 (the Plan's "Measurements"): a curve's bend and the side of a polygon with 200 placed corners
   being dragged, each pointer move to the next painted frame, and the put-down of a filled 200-corner polygon over the whole
   picture (time and peak memory). The budget for a pointer event is one frame (engineering.md); this prints numbers and does
   not fail. Run with meson test --benchmark; needs a display. */

#include "perf-common.h"

#include <math.h>
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

/* The canvas position of the center of picture pixel (x, y). */
static void
at (EaselWindow *win, int x, int y, double *cx, double *cy)
{
  double z = easel_window_get_zoom (win) / 100.0 / easel_canvas_get_scale (easel_window_get_canvas (win));

  *cx = (x + 0.5) * z;
  *cy = (y + 0.5) * z;
}

static EaselWindow *
prepared_window (const char *config_dir, int w, int h, const char *tool, gboolean fit)
{
  EaselWindow *win;

  write_settings (config_dir, w, h);
  win = open_window ();
  g_action_group_activate_action (G_ACTION_GROUP (win), "tool", g_variant_new_string (tool));
  g_action_group_activate_action (G_ACTION_GROUP (win), fit ? "zoom-fit" : "zoom-normal", NULL);
  wait_for_paint (win);
  wait_for_paint (win);
  return win;
}

/* One click of the primary button at picture pixel (x, y); t is the time in ms, always far from the click before. */
static void
click (EaselWindow *win, guint32 *t, int x, int y)
{
  EaselCanvas *canvas = easel_window_get_canvas (win);
  double cx, cy;

  at (win, x, y, &cx, &cy);
  *t += 2000;
  easel_canvas_press_timed (canvas, 1, cx, cy, *t);
  easel_canvas_release (canvas, 1);
}

static void
report (const char *what, int w, int h, gboolean fit, EaselWindow *win, double *ms, int n)
{
  qsort (ms, (size_t) n, sizeof ms[0], cmp_double);
  g_print ("%s, %dx%d, %s (zoom %d%%): to painted frame median %.1f ms, worst %.1f ms (%d moves)\n", what, w, h,
           fit ? "fit" : "100%", easel_window_get_zoom (win), ms[n / 2], ms[n - 1], n);
}

/* The second bend of a curve dragged across the picture. */
static void
measure_curve (const char *config_dir, int w, int h, gboolean fit)
{
  enum { N = 30 };
  double ms[N], cx, cy;
  EaselWindow *win = prepared_window (config_dir, w, h, "curve", fit);
  EaselCanvas *canvas = easel_window_get_canvas (win);
  guint32 t = 0;

  at (win, w / 8, h / 8, &cx, &cy);
  easel_canvas_press_timed (canvas, 1, cx, cy, t += 2000);
  at (win, w * 7 / 8, h / 8, &cx, &cy);
  easel_canvas_motion (canvas, cx, cy);
  easel_canvas_release (canvas, 1);
  click (win, &t, w / 2, h / 4);                                 /* the first bend */
  at (win, w / 2, h / 4, &cx, &cy);
  easel_canvas_press_timed (canvas, 1, cx, cy, t += 2000);       /* the second bend, held and dragged */
  for (int i = 0; i < N; i++)
    {
      gint64 start = g_get_monotonic_time ();

      at (win, w / 2 + i * w / 80, h / 4 + i * h / 40, &cx, &cy);
      easel_canvas_motion (canvas, cx, cy);
      wait_for_paint (win);
      ms[i] = (g_get_monotonic_time () - start) / 1000.0;
    }
  easel_canvas_release (canvas, 1);
  report ("curve, second bend dragged", w, h, fit, win, ms, N);
  gtk_window_destroy (GTK_WINDOW (win));
}

/* A polygon of n corners on a circle around the picture's center, each put by a click. */
static void
place_circle (EaselWindow *win, guint32 *t, int w, int h, int n)
{
  double r = (w < h ? w : h) * 0.48;

  for (int i = 0; i < n; i++)
    click (win, t, (int) (w / 2 + r * cos (6.283185307 * i / n)), (int) (h / 2 + r * sin (6.283185307 * i / n)));
}

static void
measure_polygon_side (const char *config_dir, int w, int h, gboolean fit)
{
  enum { N = 30 };
  double ms[N], cx, cy;
  EaselWindow *win = prepared_window (config_dir, w, h, "polygon", fit);
  EaselCanvas *canvas = easel_window_get_canvas (win);
  guint32 t = 0;

  place_circle (win, &t, w, h, 200);
  at (win, w / 2, h / 2, &cx, &cy);
  easel_canvas_press_timed (canvas, 1, cx, cy, t += 2000);       /* the next side, held and dragged */
  for (int i = 0; i < N; i++)
    {
      gint64 start = g_get_monotonic_time ();

      at (win, w / 2 + i * w / 80, h / 2 + i * h / 80, &cx, &cy);
      easel_canvas_motion (canvas, cx, cy);
      wait_for_paint (win);
      ms[i] = (g_get_monotonic_time () - start) / 1000.0;
    }
  easel_canvas_release (canvas, 1);
  report ("polygon of 200 corners, a side dragged", w, h, fit, win, ms, N);
  gtk_window_destroy (GTK_WINDOW (win));
}

/* The put-down of the filled 200-corner polygon over the whole picture, through the window. */
static void
measure_put_down (const char *config_dir, int w, int h)
{
  EaselWindow *win = prepared_window (config_dir, w, h, "polygon", TRUE);
  EaselCanvas *canvas = easel_window_get_canvas (win);
  double before = peak_mb ();
  guint32 t = 0;
  gint64 start;

  g_action_group_change_action_state (G_ACTION_GROUP (win), "fill-style", g_variant_new_int32 (2));
  place_circle (win, &t, w, h, 200);
  wait_for_paint (win);
  start = g_get_monotonic_time ();
  easel_canvas_put_down (canvas);
  g_print ("put-down of a filled 200-corner polygon over the whole %dx%d picture: %.0f ms to return; ", w, h,
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
        measure_curve (config_dir, sizes[i][0], sizes[i][1], fit);
        measure_polygon_side (config_dir, sizes[i][0], sizes[i][1], fit);
      }
  measure_put_down (config_dir, 8000, 8000);
  g_object_unref (app);
  return 0;
}
