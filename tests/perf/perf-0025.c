/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Measurements of the view built by the edges (0025, T007): the real canvas with a 4000 x 3000 picture, the view in the middle, at 800 %
   (the renderer's scaling, as a control), 100 % (built on a screen whose scale is not whole, REQ-0025-07), 129 % and 150 %: process CPU and frame time per frame while scrolling by a pixel, while drawing
   with the Pencil, for a repaint with nothing changed, with the grid and a marked selection (tint, outline, handles) over a scroll, and
   with a floating selection over a scroll. Run in the real session with no other Easel window open and the pointer outside its window;
   it prints numbers and does not fail.
   Usage: perf-0025 [fractional | pencil | pencil-log [after-scroll]] */

#include "perf-common.h"

#include <stdio.h>
#include <time.h>

enum { N = 180, SKIP = 5 };

static double
cpu_ms (void)
{
  struct timespec t;

  clock_gettime (CLOCK_PROCESS_CPUTIME_ID, &t);
  return t.tv_sec * 1000.0 + t.tv_nsec / 1e6;
}

/* A frame's time here is the compositor's: the distance between two frames' presentation times. The time from an action to the next after-paint
   is not one: it read 20 to 33 ms, and a median of 13, while the frames reached a 60 Hz screen 16.67 ms apart (T014). */
static void
report (const char *what, int zoom, EaselCanvas *canvas, double *frame, int n_frames, double *cpu)
{
  GtkWidget *view = gtk_widget_get_parent (GTK_WIDGET (canvas));

  int over_17 = 0, over_20 = 0, over_33 = 0;

  for (int i = 0; i < n_frames; i++)
    {
      over_17 += frame[i] > 16.7;
      over_20 += frame[i] > 20;
      over_33 += frame[i] > 33;
    }
  /* A repaint with nothing changed gives the compositor nothing new: few frames or none come back with a presentation time. */
  if (n_frames == 0)
    {
      qsort (cpu, N, sizeof cpu[0], cmp_double);
      g_print ("%4d %% %-22s view %d x %d: CPU %.2f / %.2f ms, no frame presented\n", zoom, what, gtk_widget_get_width (view),
               gtk_widget_get_height (view), cpu[N / 2], cpu[N - 1]);
      return;
    }
  qsort (frame, n_frames, sizeof frame[0], cmp_double);
  qsort (cpu, N, sizeof cpu[0], cmp_double);
  g_print ("%4d %% %-22s view %d x %d: CPU %.2f / %.2f ms, frame %.2f / %.2f ms; frames over 16.7 ms %.1f %%, over 20 ms %.1f %%, over 33 ms %.1f %% (%d presented of %d)\n",
           zoom, what, gtk_widget_get_width (view), gtk_widget_get_height (view), cpu[N / 2], cpu[N - 1], frame[n_frames / 2], frame[n_frames - 1],
           100.0 * over_17 / n_frames, 100.0 * over_20 / n_frames, 100.0 * over_33 / n_frames, n_frames, N - 1);
}

typedef enum { ACT_SCROLL, ACT_PENCIL, ACT_IDLE } Act;

static void
measure (EaselWindow *win, const char *what, int zoom, Act act)
{
  EaselCanvas *canvas = easel_window_get_canvas (win);
  GtkScrolledWindow *scroller = GTK_SCROLLED_WINDOW (gtk_widget_get_ancestor (GTK_WIDGET (canvas), GTK_TYPE_SCROLLED_WINDOW));
  GtkAdjustment *ha = gtk_scrolled_window_get_hadjustment (scroller), *va = gtk_scrolled_window_get_vadjustment (scroller);
  GdkFrameClock *clock = gtk_widget_get_frame_clock (GTK_WIDGET (win));
  gint64 counter[N], presented[N];
  double frame[N], cpu[N];
  int n_frames = 0;
  double l = zoom / (100.0 * easel_canvas_get_scale (canvas));
  double x0 = gtk_adjustment_get_value (ha) + gtk_adjustment_get_page_size (ha) / 2 - EASEL_VIEW_MARGIN;
  double y0 = gtk_adjustment_get_value (va) + gtk_adjustment_get_page_size (va) / 2 - EASEL_VIEW_MARGIN;
  double base = gtk_adjustment_get_value (ha);

  if (act == ACT_PENCIL)
    {
      g_action_group_activate_action (G_ACTION_GROUP (win), "tool", g_variant_new_string ("pencil"));
      easel_canvas_press (canvas, 1, x0 + l / 2, y0 + l / 2);
    }
  for (int i = -SKIP; i < N; i++)
    {
      double c0 = cpu_ms ();

      if (act == ACT_SCROLL)
        gtk_adjustment_set_value (ha, base + (i & 1));
      else if (act == ACT_PENCIL)
        easel_canvas_motion (canvas, x0 + (i + SKIP + 1.5) * l, y0 + l / 2);
      gtk_widget_queue_draw (GTK_WIDGET (canvas));
      wait_for_paint (win);
      if (i >= 0)
        {
          cpu[i] = cpu_ms () - c0;
          counter[i] = gdk_frame_clock_get_frame_counter (clock);
          presented[i] = 0;
        }
      /* The compositor's answer comes some frames later, and the clock keeps only a few. */
      for (int b = MAX (0, i - 8); b <= i; b++)
        if (presented[b] == 0)
          {
            GdkFrameTimings *t = gdk_frame_clock_get_timings (clock, counter[b]);

            if (t != NULL && gdk_frame_timings_get_complete (t))
              presented[b] = MAX (1, gdk_frame_timings_get_presentation_time (t));
          }
    }
  for (int i = 1; i < N; i++)
    if (presented[i] > 1 && presented[i - 1] > 1)
      frame[n_frames++] = (presented[i] - presented[i - 1]) / 1000.0 / MAX (1, counter[i] - counter[i - 1]);
  if (act == ACT_PENCIL)
    {
      easel_canvas_release (canvas, 1);
      g_action_group_activate_action (G_ACTION_GROUP (win), "undo", NULL);
      wait_for_paint (win);
    }
  report (what, zoom, canvas, frame, n_frames, cpu);
}

/* perf-0025 pencil-log: one line per frame of a Pencil stroke, from three clocks: the elapsed time this program measures from before the motion to
   the next after-paint (what report () calls a frame), the frame clock's frame time, and the compositor's presentation time of that frame. */
static void
pencil_log (const char *config_dir, int zoom, gboolean scroll_first)
{
  enum { HISTORY = 8 };
  EaselWindow *win;
  EaselCanvas *canvas;
  GdkFrameClock *clock;
  GtkScrolledWindow *scroller;
  GtkAdjustment *ha, *va;
  double l, x0, y0;
  static gint64 elapsed[N + SKIP + 64], clock_time[N + SKIP + 64], presented[N + SKIP + 64], refresh[N + SKIP + 64], frame_no[N + SKIP + 64];
  gint64 first;

  write_settings (config_dir, 4000, 3000);
  win = open_window ();
  canvas = easel_window_get_canvas (win);
  clock = gtk_widget_get_frame_clock (GTK_WIDGET (win));
  scroller = GTK_SCROLLED_WINDOW (gtk_widget_get_ancestor (GTK_WIDGET (canvas), GTK_TYPE_SCROLLED_WINDOW));
  ha = gtk_scrolled_window_get_hadjustment (scroller);
  va = gtk_scrolled_window_get_vadjustment (scroller);
  gtk_window_maximize (GTK_WINDOW (win));
  wait_for_paint (win);
  easel_canvas_set_zoom (canvas, zoom);
  wait_for_paint (win);
  gtk_adjustment_set_value (ha, (gtk_adjustment_get_upper (ha) - gtk_adjustment_get_page_size (ha)) / 2);
  gtk_adjustment_set_value (va, (gtk_adjustment_get_upper (va) - gtk_adjustment_get_page_size (va)) / 2);
  wait_for_paint (win);
  g_assert_cmpint (easel_canvas_get_zoom (canvas), ==, zoom);
  if (scroll_first)
    measure (win, "scroll", zoom, ACT_SCROLL);
  l = zoom / (100.0 * easel_canvas_get_scale (canvas));
  x0 = gtk_adjustment_get_value (ha) + gtk_adjustment_get_page_size (ha) / 2 - EASEL_VIEW_MARGIN;
  y0 = gtk_adjustment_get_value (va) + gtk_adjustment_get_page_size (va) / 2 - EASEL_VIEW_MARGIN;
  g_action_group_activate_action (G_ACTION_GROUP (win), "tool", g_variant_new_string ("pencil"));
  easel_canvas_press (canvas, 1, x0 + l / 2, y0 + l / 2);
  first = gdk_frame_clock_get_frame_counter (clock);
  for (int i = 0; i < N + SKIP; i++)
    {
      gint64 t0 = g_get_monotonic_time ();
      gint64 counter;

      easel_canvas_motion (canvas, x0 + (i + 1.5) * l, y0 + l / 2);
      gtk_widget_queue_draw (GTK_WIDGET (canvas));
      wait_for_paint (win);
      counter = gdk_frame_clock_get_frame_counter (clock);
      elapsed[i] = g_get_monotonic_time () - t0;
      clock_time[i] = gdk_frame_clock_get_frame_time (clock);
      frame_no[i] = counter;
      presented[i] = -1;                                       /* the compositor's answer is not back yet */
      refresh[i] = 0;
      /* The frame HISTORY back is complete by now; the clock keeps only a few. */
      for (int b = MAX (0, i - HISTORY); b <= i; b++)
        if (refresh[b] == 0)
          {
            GdkFrameTimings *t = gdk_frame_clock_get_timings (clock, frame_no[b]);

            if (t != NULL && gdk_frame_timings_get_complete (t))
              {
                refresh[b] = MAX (1, gdk_frame_timings_get_refresh_interval (t));
                presented[b] = gdk_frame_timings_get_presentation_time (t);
              }
          }
    }
  easel_canvas_release (canvas, 1);
  g_print ("# zoom %d, first frame counter %" G_GINT64_FORMAT "\n", zoom, first);
  g_print ("# i elapsed_us frame_clock_delta_us frame_counter_delta presentation_us refresh_us (presentation -1: no answer within 8 frames; 0: answered, not presented)\n");
  for (int i = SKIP; i < N + SKIP; i++)
    g_print ("L %d %d %" G_GINT64_FORMAT " %" G_GINT64_FORMAT " %" G_GINT64_FORMAT " %" G_GINT64_FORMAT " %" G_GINT64_FORMAT "\n", zoom, i - SKIP, elapsed[i],
             clock_time[i] - clock_time[i - 1], frame_no[i] - frame_no[i - 1], presented[i], refresh[i]);
  gtk_window_destroy (GTK_WINDOW (win));
  while (g_main_context_iteration (NULL, FALSE))
    ;
}

/* A drag in the middle of the view with the Select tool: a marked rectangle; another from inside it: a floating selection. */
static void
drag (EaselCanvas *canvas, double x0, double y0, double x1, double y1)
{
  easel_canvas_press (canvas, 1, x0, y0);
  easel_canvas_motion (canvas, (x0 + x1) / 2, (y0 + y1) / 2);
  easel_canvas_motion (canvas, x1, y1);
  easel_canvas_release (canvas, 1);
}

static void
measure_zoom (const char *config_dir, int zoom, gboolean pencil_only)
{
  EaselWindow *win;
  EaselCanvas *canvas;
  GtkScrolledWindow *scroller;
  GtkAdjustment *ha, *va;
  double x0, y0;
  gboolean floating = FALSE;
  int sx, sy, sw, sh;

  write_settings (config_dir, 4000, 3000);
  win = open_window ();
  canvas = easel_window_get_canvas (win);
  scroller = GTK_SCROLLED_WINDOW (gtk_widget_get_ancestor (GTK_WIDGET (canvas), GTK_TYPE_SCROLLED_WINDOW));
  ha = gtk_scrolled_window_get_hadjustment (scroller);
  va = gtk_scrolled_window_get_vadjustment (scroller);
  gtk_window_maximize (GTK_WINDOW (win));      /* the budget's view is 1920 x 1080: as near as the screen allows; the view is printed */
  wait_for_paint (win);
  easel_canvas_set_zoom (canvas, zoom);
  wait_for_paint (win);
  gtk_adjustment_set_value (ha, (gtk_adjustment_get_upper (ha) - gtk_adjustment_get_page_size (ha)) / 2);
  gtk_adjustment_set_value (va, (gtk_adjustment_get_upper (va) - gtk_adjustment_get_page_size (va)) / 2);
  wait_for_paint (win);
  g_assert_cmpint (easel_canvas_get_zoom (canvas), ==, zoom);
  if (pencil_only)
    {
      measure (win, "pencil", zoom, ACT_PENCIL);
      gtk_window_destroy (GTK_WINDOW (win));
      while (g_main_context_iteration (NULL, FALSE))
        ;
      return;
    }
  measure (win, "scroll", zoom, ACT_SCROLL);
  measure (win, "pencil", zoom, ACT_PENCIL);
  measure (win, "nothing changed", zoom, ACT_IDLE);

  easel_canvas_set_grid (canvas, TRUE, 4, FALSE);
  x0 = gtk_adjustment_get_value (ha) + gtk_adjustment_get_page_size (ha) / 2 - EASEL_VIEW_MARGIN;
  y0 = gtk_adjustment_get_value (va) + gtk_adjustment_get_page_size (va) / 2 - EASEL_VIEW_MARGIN;
  g_action_group_activate_action (G_ACTION_GROUP (win), "tool", g_variant_new_string ("select"));
  drag (canvas, x0, y0, x0 + 300, y0 + 200);
  wait_for_paint (win);
  g_assert_true (easel_canvas_get_selection (canvas, &sx, &sy, &sw, &sh, &floating));
  g_assert_false (floating);
  measure (win, "scroll, grid, selection", zoom, ACT_SCROLL);
  easel_canvas_set_grid (canvas, FALSE, 4, FALSE);
  drag (canvas, x0 + 100, y0 + 100, x0 + 160, y0 + 140);
  wait_for_paint (win);
  g_assert_true (easel_canvas_get_selection (canvas, &sx, &sy, &sw, &sh, &floating));
  g_assert_true (floating);
  measure (win, "scroll, float", zoom, ACT_SCROLL);
  gtk_window_destroy (GTK_WINDOW (win));
  while (g_main_context_iteration (NULL, FALSE))
    ;
}

int
main (int argc, char **argv)
{
  g_autofree char *config_dir = g_dir_make_tmp ("easel-perf-XXXXXX", NULL);
  double load[3];

  g_setenv ("XDG_CONFIG_HOME", config_dir, TRUE);
  if (!gtk_init_check ())
    return 77;
  app = easel_application_new ();
  g_application_set_flags (G_APPLICATION (app), G_APPLICATION_NON_UNIQUE);
  g_application_register (G_APPLICATION (app), NULL, NULL);
  if (getloadavg (load, 3) == 3)
    g_print ("load %.2f %.2f %.2f\n", load[0], load[1], load[2]);
  if (argc > 1 && g_strcmp0 (argv[1], "pencil-log") == 0)
    {
      gboolean scroll_first = argc > 2 && g_strcmp0 (argv[2], "after-scroll") == 0;

      pencil_log (config_dir, 129, scroll_first);
      pencil_log (config_dir, 150, scroll_first);
      g_object_unref (app);
      return 0;
    }
  {
    gboolean pencil_only = argc > 1 && g_strcmp0 (argv[1], "pencil") == 0;

    if (argc <= 1 || g_strcmp0 (argv[1], "fractional") != 0)
      measure_zoom (config_dir, 800, pencil_only);
    measure_zoom (config_dir, 100, pencil_only);       /* on a screen whose scale is not whole, built by edges from 2026-11-13 (REQ-0025-07) */
    measure_zoom (config_dir, 129, pencil_only);
    measure_zoom (config_dir, 150, pencil_only);
  }
  g_object_unref (app);
  return 0;
}
