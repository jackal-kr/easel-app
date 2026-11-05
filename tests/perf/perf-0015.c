/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Measurements of feature 0015 (REQ-0015-10, AS-12): a selection of the whole picture, 2000 x 2000 and 8000 x 8000, stretched by its
   bottom-right handle, one picture pixel per pointer event, at 100 % and at the fit zoom: the first event (the lift, the first image) and
   the median of the next 15 to the painted frame, with the peak memory. The budget for a pointer event is one frame (engineering.md);
   this prints numbers and does not fail. Run with meson test --benchmark; needs a display.
   Keep the pointer outside the window while it runs: the stretch is driven by calls, so a real pointer motion over the window, with no
   button down, is a lost release (REQ-0007-18) and ends the held stretch early, making the whole image at that event (a worst event of
   150 to 195 ms and a release of 0 ms in place of 30 ms and 133 to 136 ms at 8000 x 8000, 100 %). */

#include "perf-common.h"

#include <stdio.h>

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

static void
measure_stretch (const char *config_dir, int size, gboolean fit)
{
  enum { N = 15 };
  EaselWindow *win;
  EaselCanvas *canvas;
  graphene_rect_t r;
  double ms[N], first, rel_return, rel_painted, x, y, l, before;
  gint64 start;

  write_settings (config_dir, size, size);
  win = open_window ();
  canvas = easel_window_get_canvas (win);
  g_action_group_activate_action (G_ACTION_GROUP (win), "tool", g_variant_new_string ("select"));
  g_action_group_activate_action (G_ACTION_GROUP (win), fit ? "zoom-fit" : "zoom-normal", NULL);
  wait_for_paint (win);
  wait_for_paint (win);
  g_action_group_activate_action (G_ACTION_GROUP (win), "select-all", NULL);
  wait_for_paint (win);
  before = peak_mb ();
  l = easel_window_get_zoom (win) / 100.0 / easel_canvas_get_scale (canvas);
  g_assert (easel_canvas_get_selection_handle_rect (canvas, EASEL_SEL_HANDLE_BR, &r));
  x = r.origin.x + r.size.width / 2;
  y = r.origin.y + r.size.height / 2;
  easel_canvas_press (canvas, 1, x, y);
  start = g_get_monotonic_time ();
  easel_canvas_motion (canvas, x + l, y + l);                     /* the lift and the first image */
  wait_for_paint (win);
  first = (g_get_monotonic_time () - start) / 1000.0;
  for (int i = 0; i < N; i++)
    {
      start = g_get_monotonic_time ();
      easel_canvas_motion (canvas, x + (i + 2) * l, y + (i + 2) * l);
      wait_for_paint (win);
      ms[i] = (g_get_monotonic_time () - start) / 1000.0;
    }
  start = g_get_monotonic_time ();
  easel_canvas_release (canvas, 1);                               /* the whole image is made at the release */
  rel_return = (g_get_monotonic_time () - start) / 1000.0;
  wait_for_paint (win);
  rel_painted = (g_get_monotonic_time () - start) / 1000.0;
  qsort (ms, N, sizeof ms[0], cmp_double);
  g_print ("stretch of %dx%d, %s (zoom %d%%): first event (lift) %.0f ms; next events to painted frame median %.1f ms, worst %.1f ms (%d events); "
           "release %.0f ms to return, %.0f ms to the painted frame; peak memory %.0f MB (was %.0f MB)\n", size, size, fit ? "fit" : "100%",
           easel_window_get_zoom (win), first, ms[N / 2], ms[N - 1], N, rel_return, rel_painted, peak_mb (), before);
  gtk_window_destroy (GTK_WINDOW (win));
}

int
main (int argc, char **argv)
{
  g_autofree char *config_dir = g_dir_make_tmp ("easel-perf-XXXXXX", NULL);
  static const int sizes[] = { 2000, 8000 };

  g_setenv ("XDG_CONFIG_HOME", config_dir, TRUE);
  if (!gtk_init_check ())
    return 77;
  app = easel_application_new ();
  g_application_set_flags (G_APPLICATION (app), G_APPLICATION_NON_UNIQUE);
  g_application_register (G_APPLICATION (app), NULL, NULL);
  for (guint i = 0; i < G_N_ELEMENTS (sizes); i++)
    for (int fit = 0; fit <= 1; fit++)
      measure_stretch (config_dir, sizes[i], fit);
  g_object_unref (app);
  return 0;
}
