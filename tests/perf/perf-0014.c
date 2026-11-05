/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Measurements of feature 0014 (REQ-0014-16, AS-14): a smear, one pixel per pointer event, with a number of copies already left (made
   first by pointer events without waiting for a frame, the trail an earlier drag left), each further pointer event to the next painted
   frame: 150 x 110 on 2560 x 1600 with 2000, 10000 and 30000 copies, and 400 x 300 on 1920 x 1080 with 2000, at 100 % and at the fit
   zoom; and a trace of 100000 pointer positions with Free-Form Select on 8000 x 8000 at the fit zoom: each pointer event to the painted
   frame near its end, a frame with nothing changed while it is held and after the release, the release, and the peak memory. The
   budget for a pointer event is one frame (engineering.md); this prints numbers and does not fail. Run with meson test --benchmark;
   needs a display. */

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

/* The k-th step of a serpentine over the picture area in view, from (100, 100): the selection's top-left corner, one pixel at a time
   along a row, then 120 pixels down (one jump) and back along the next row. */
static void
step (int k, int cols, int rows, int *x, int *y)
{
  int r = k / cols, c = k % cols;

  *x = 100 + (r % 2 == 0 ? c : cols - 1 - c);
  *y = 100 + (r % rows) * 120;
}

static void
measure_smear (const char *config_dir, int pw, int ph, int sw, int sh, const int *counts, guint n_counts, gboolean fit)
{
  enum { N = 15 };
  EaselWindow *win;
  EaselCanvas *canvas;
  GtkWidget *view;
  double cx, cy;
  int cols, rows, k = 0, x, y;
  double z;

  write_settings (config_dir, pw, ph);
  win = open_window ();
  canvas = easel_window_get_canvas (win);
  g_action_group_activate_action (G_ACTION_GROUP (win), "tool", g_variant_new_string ("select"));
  g_action_group_activate_action (G_ACTION_GROUP (win), fit ? "zoom-fit" : "zoom-normal", NULL);
  wait_for_paint (win);
  wait_for_paint (win);
  view = gtk_widget_get_parent (GTK_WIDGET (canvas));
  z = easel_window_get_zoom (win) / 100.0 / easel_canvas_get_scale (canvas);
  cols = MAX (200, MIN (pw, (int) (gtk_widget_get_width (view) / z)) - sw - 250);
  rows = MAX (1, (MIN (ph, (int) (gtk_widget_get_height (view) / z)) - sh - 220) / 120);
  at (win, 100, 100, &cx, &cy);
  easel_canvas_press (canvas, 1, cx, cy);
  at (win, 100 + sw - 1, 100 + sh - 1, &cx, &cy);
  easel_canvas_motion (canvas, cx, cy);                       /* sw x sh */
  easel_canvas_release (canvas, 1);
  easel_canvas_set_shift (canvas, TRUE);
  at (win, 150, 150, &cx, &cy);
  easel_canvas_press (canvas, 1, cx, cy);                     /* Shift at the press on the selection: a smear */
  for (guint c = 0; c < n_counts; c++)
    {
      double ms[N];

      while (easel_canvas_get_selection_copies (canvas) < (gsize) counts[c])
        {
          step (++k, cols, rows, &x, &y);
          at (win, x + 50, y + 50, &cx, &cy);
          easel_canvas_motion (canvas, cx, cy);
        }
      wait_for_paint (win);
      for (int i = 0; i < N; i++)
        {
          gint64 start = g_get_monotonic_time ();

          step (++k, cols, rows, &x, &y);
          at (win, x + 50, y + 50, &cx, &cy);
          easel_canvas_motion (canvas, cx, cy);
          wait_for_paint (win);
          ms[i] = (g_get_monotonic_time () - start) / 1000.0;
        }
      qsort (ms, N, sizeof ms[0], cmp_double);
      g_print ("smear of %dx%d on %dx%d, %s (zoom %d%%), %zu copies: to painted frame median %.1f ms, worst %.1f ms (%d events, "
               "view %dx%d); peak memory %.0f MB\n", sw, sh, pw, ph, fit ? "fit" : "100%", easel_window_get_zoom (win),
               easel_canvas_get_selection_copies (canvas), ms[N / 2], ms[N - 1], N, gtk_widget_get_width (view),
               gtk_widget_get_height (view), peak_mb ());
    }
  easel_canvas_release (canvas, 1);
  easel_canvas_set_shift (canvas, FALSE);
  gtk_window_destroy (GTK_WINDOW (win));
}

/* The k-th corner of a walk round the rectangle of corners (m, m) to (w - m, h - m), one corner a step. */
static void
round_rect (long k, int w, int h, int m, int *x, int *y)
{
  long side_w = w - 2 * m, side_h = h - 2 * m, p = k % (2 * (side_w + side_h));

  if (p < side_w)
    *x = m + (int) p, *y = m;
  else if (p < side_w + side_h)
    *x = w - m, *y = m + (int) (p - side_w);
  else if (p < 2 * side_w + side_h)
    *x = w - m - (int) (p - side_w - side_h), *y = h - m;
  else
    *x = m, *y = h - m - (int) (p - 2 * side_w - side_h);
}

/* The k-th of n positions of a hand-drawn loop: an ellipse round (650, 380) with radii 450 and 250 picture pixels, which lies in the view
   at 100 %, with a wobble of a few pixels, as a hand gives it. */
static void
hand_loop (long k, long n, int *x, int *y)
{
  double t = 2 * G_PI * (double) k / (double) n;

  *x = (int) lround (650 + 450 * cos (t) + 3 * sin (17 * t));
  *y = (int) lround (380 + 250 * sin (t) + 2 * cos (23 * t));
}

typedef void (*PathStep) (long k, long n, int *x, int *y);

static void
extreme_step (long k, long n, int *x, int *y)
{
  round_rect (k, 8000, 8000, 100, x, y);
}

/* The pointer to the corner (x, y) of the grid. */
static void
to_corner (EaselWindow *win, gboolean press, int x, int y)
{
  EaselCanvas *canvas = easel_window_get_canvas (win);
  double z = easel_window_get_zoom (win) / 100.0 / easel_canvas_get_scale (canvas);

  if (press)
    easel_canvas_press (canvas, 1, x * z, y * z);
  else
    easel_canvas_motion (canvas, x * z, y * z);
}

/* The median of n frames with nothing changed, each to the next painted frame. */
static double
steady (EaselWindow *win, double *worst)
{
  enum { N = 30 };
  double ms[N];

  for (int i = 0; i < N; i++)
    {
      gint64 start = g_get_monotonic_time ();

      wait_for_paint (win);
      ms[i] = (g_get_monotonic_time () - start) / 1000.0;
    }
  qsort (ms, N, sizeof ms[0], cmp_double);
  *worst = ms[N - 1];
  return ms[N / 2];
}

/* A trace of total pointer positions with Free-Form Select on a pw x ph picture: each of the last 30 pointer events to the painted
   frame, a frame with nothing changed while it is held (with the line, and with nothing), the release, and a frame with nothing changed
   after it with the line and the tint, the line only, the tint only and neither (the probe of the frame's cost). */
static void
measure_path (const char *config_dir, const char *name, int pw, int ph, gboolean fit, PathStep next, long total)
{
  enum { N = 30 };
  EaselWindow *win;
  EaselCanvas *canvas;
  double ms[N], before, worst, held_line, held_none, w2;
  int x, y, sx, sy, sw, sh;
  gint64 start;
  gboolean floating;
  static const struct { gboolean line, tint; const char *name; } probes[] = {
    { TRUE, TRUE, "line and tint" }, { TRUE, FALSE, "line only" }, { FALSE, TRUE, "tint only" }, { FALSE, FALSE, "neither" } };

  write_settings (config_dir, pw, ph);
  win = open_window ();
  canvas = easel_window_get_canvas (win);
  g_action_group_activate_action (G_ACTION_GROUP (win), "tool", g_variant_new_string ("free-select"));
  g_action_group_activate_action (G_ACTION_GROUP (win), fit ? "zoom-fit" : "zoom-normal", NULL);
  wait_for_paint (win);
  wait_for_paint (win);
  before = peak_mb ();
  next (0, total, &x, &y);
  to_corner (win, TRUE, x, y);
  for (long k = 1; k < total - N; k++)
    {
      next (k, total, &x, &y);
      to_corner (win, FALSE, x, y);
    }
  wait_for_paint (win);
  for (int i = 0; i < N; i++)
    {
      start = g_get_monotonic_time ();
      next (total - N + i, total, &x, &y);
      to_corner (win, FALSE, x, y);
      wait_for_paint (win);
      ms[i] = (g_get_monotonic_time () - start) / 1000.0;
    }
  qsort (ms, N, sizeof ms[0], cmp_double);
  held_line = steady (win, &worst);
  easel_canvas_set_overlay_probe (canvas, FALSE, FALSE);
  held_none = steady (win, &w2);
  easel_canvas_set_overlay_probe (canvas, TRUE, TRUE);
  g_print ("%s, %ld positions on %dx%d, %s (zoom %d%%): tracing, pointer event to painted frame median %.1f ms (worst %.1f); "
           "frame while held: line %.1f ms, nothing %.1f ms; ", name, total, pw, ph, fit ? "fit" : "100%", easel_window_get_zoom (win),
           ms[N / 2], ms[N - 1], held_line, held_none);
  start = g_get_monotonic_time ();
  easel_canvas_release (canvas, 1);
  g_print ("release %.0f ms to return, ", (g_get_monotonic_time () - start) / 1000.0);
  wait_for_paint (win);
  g_print ("%.0f ms to the painted frame; selection %s; finished, frame with nothing changed:", (g_get_monotonic_time () - start) / 1000.0,
           easel_canvas_get_selection (canvas, &sx, &sy, &sw, &sh, &floating) ? "made" : "NOT made");
  for (guint p = 0; p < G_N_ELEMENTS (probes); p++)
    {
      double m;

      easel_canvas_set_overlay_probe (canvas, probes[p].line, probes[p].tint);
      wait_for_paint (win);
      m = steady (win, &worst);
      g_print (" %s %.1f ms (worst %.1f)%s", probes[p].name, m, worst, p + 1 < G_N_ELEMENTS (probes) ? ";" : "");
    }
  easel_canvas_set_overlay_probe (canvas, TRUE, TRUE);
  g_print ("; peak memory %.0f MB (was %.0f MB)\n", peak_mb (), before);
  gtk_window_destroy (GTK_WINDOW (win));
}

/* With the argument "trace", only the traces are measured; with "extreme", only the extreme trace. */
int
main (int argc, char **argv)
{
  g_autofree char *config_dir = g_dir_make_tmp ("easel-perf-XXXXXX", NULL);
  static const int many[] = { 2000, 10000, 30000 }, two_thousand[] = { 2000 };
  gboolean extreme_only = argc > 1 && strcmp (argv[1], "extreme") == 0;
  gboolean traces_only = extreme_only || (argc > 1 && strcmp (argv[1], "trace") == 0);

  g_setenv ("XDG_CONFIG_HOME", config_dir, TRUE);
  if (!gtk_init_check ())
    return 77;
  app = easel_application_new ();
  g_application_set_flags (G_APPLICATION (app), G_APPLICATION_NON_UNIQUE);
  g_application_register (G_APPLICATION (app), NULL, NULL);
  if (!traces_only)
    {
      for (int fit = 0; fit <= 1; fit++)
        measure_smear (config_dir, 2560, 1600, 150, 110, many, G_N_ELEMENTS (many), fit);
      for (int fit = 0; fit <= 1; fit++)
        measure_smear (config_dir, 1920, 1080, 400, 300, two_thousand, G_N_ELEMENTS (two_thousand), fit);
    }
  for (int fit = 0; fit <= 1 && !extreme_only; fit++)
    measure_path (config_dir, "typical hand-drawn loop", 2560, 1600, fit, hand_loop, 2000);
  measure_path (config_dir, "extreme trace", 8000, 8000, TRUE, extreme_step, 100000);
  g_object_unref (app);
  return 0;
}
