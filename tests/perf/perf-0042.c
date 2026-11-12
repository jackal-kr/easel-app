/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Measurements of feature 0042 (T004): Invert Selection in the core on an 8000 x 8000 picture, each case three times: the shape of the inverse
   (easel_selection_invert), then its line (easel_lasso_outline), in milliseconds, with the number of corners and parts of the line and the
   process's peak resident memory (VmHWM, started again from the present size before the case) against the resident size with the selection's own
   mask. Cases: rectangle (a 4000 x 4000 rectangle in the middle: the picture less it, one hole), triangle (a 4000 x 4000 triangle's mask in the
   middle) and noise (a 1500 x 1500 box of pixels chosen by a hash, half of them: the inverse has millions of corners and parts).
   The window mode (T005, T009) is the whole path: the action Invert Selection from its activation to the next painted frame, three runs, on a
   1920 x 1080 and an 8000 x 8000 picture at the fit zoom, with a rectangle of half the sides marked by the pointer (the window case marked-half);
   the other window cases come with the floating selection (T009). Run in the real session with no Easel window open, the pointer outside the
   window and the load noted by the caller. It prints numbers and does not fail.
   Usage: perf-0042 [CASE]   CASE rectangle, triangle or noise (the core)
          perf-0042 window [SIZE]   the same through the window, SIZE 1080 or 8000 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "perf-common.h"

#include "lasso.h"
#include "selection.h"

#define RUNS 3
#define SIDE 8000

static double
now_ms (void)
{
  struct timespec t;

  clock_gettime (CLOCK_MONOTONIC, &t);
  return t.tv_sec * 1000.0 + t.tv_nsec / 1e6;
}

static double
status_mb (const char *key)
{
  FILE *f = fopen ("/proc/self/status", "r");
  char line[256];
  long kb = 0;
  size_t n = strlen (key);

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
reset_peak (void)
{
  FILE *f = fopen ("/proc/self/clear_refs", "w");

  if (f != NULL)
    {
      fputs ("5", f);
      fclose (f);
    }
}

static const char *const cases[] = { "rectangle", "triangle", "noise" };

static void
measure (int c)
{
  int sx = 2000, sy = 2000, sw = 4000, sh = 4000;
  uint8_t *mask = NULL;
  double shape[RUNS], line[RUNS], base, peak;
  size_t corners = 0, parts = 0;

  if (c == 2)
    {
      sx = sy = 3250;
      sw = sh = 1500;
    }
  if (c >= 1)
    {
      mask = malloc ((size_t) sw * (size_t) sh);
      if (mask == NULL)
        return;
      for (int j = 0; j < sh; j++)
        for (int i = 0; i < sw; i++)
          {
            uint32_t v = (uint32_t) (j * sw + i) * 2654435761u;

            mask[(size_t) j * sw + i] = c == 1 ? i <= j : (v >> 15) & 1;
          }
    }
  base = status_mb ("VmRSS");
  reset_peak ();
  for (int r = 0; r < RUNS; r++)
    {
      uint8_t *out = NULL;
      EaselPath p = { 0 };
      int x, y, w, h;
      double t0 = now_ms (), t1, t2;

      if (easel_selection_invert (mask, sx, sy, sw, sh, SIDE, SIDE, &x, &y, &w, &h, &out, SIZE_MAX) != EASEL_SELECTION_DONE)
        {
          printf ("%s: the shape was not made\n", cases[c]);
          free (mask);
          return;
        }
      t1 = now_ms ();
      if (!easel_lasso_outline (out, w, h, x, y, &p, SIZE_MAX))
        {
          printf ("%s: the line was not made\n", cases[c]);
          free (out);
          free (mask);
          return;
        }
      t2 = now_ms ();
      shape[r] = t1 - t0;
      line[r] = t2 - t1;
      corners = p.n;
      parts = 1;
      for (size_t i = 0; i < p.n; i++)
        parts += p.xy[2 * i] == EASEL_PATH_BREAK;
      easel_path_free (&p);
      free (out);
    }
  peak = status_mb ("VmHWM");
  printf ("%-9s the picture less a %d x %d %s: shape %.0f %.0f %.0f ms | line %.0f %.0f %.0f ms | %zu corners, %zu parts | resident %.0f MB with "
          "the selection, peak %.0f MB\n",
          cases[c], sw, sh, c == 0 ? "rectangle" : c == 1 ? "triangle" : "box of noise", shape[0], shape[1], shape[2], line[0], line[1], line[2], corners, parts, base,
          peak);
  free (mask);
}

/* The action, then the painted frame: milliseconds. */
static double
timed (EaselWindow *win, const char *action)
{
  gint64 start = g_get_monotonic_time ();

  g_action_group_activate_action (G_ACTION_GROUP (win), action, NULL);
  wait_for_paint (win);
  return (g_get_monotonic_time () - start) / 1000.0;
}

/* A drag of the primary button from the picture pixel (x0, y0) to (x1, y1) at the zoom the window has. */
static void
drag (EaselWindow *win, double x0, double y0, double x1, double y1)
{
  EaselCanvas *canvas = easel_window_get_canvas (win);
  double l = easel_window_get_zoom (win) / 100.0 / easel_canvas_get_scale (canvas);

  easel_canvas_press (canvas, 1, (x0 + 0.5) * l, (y0 + 0.5) * l);
  easel_canvas_motion (canvas, (x1 + 0.5) * l, (y1 + 0.5) * l);
  easel_canvas_release (canvas, 1);
  wait_for_paint (win);
}

static void
measure_window (const char *config_dir, int w, int h)
{
  double invert[RUNS], again[RUNS], base, peak, peak_first = 0;
  EaselWindow *win;
  EaselCanvas *canvas;
  int sx, sy, sw, sh;
  gboolean floating;

  write_settings (config_dir, w, h);
  win = open_window ();
  canvas = easel_window_get_canvas (win);
  g_action_group_activate_action (G_ACTION_GROUP (win), "zoom-fit", NULL);
  wait_for_paint (win);
  wait_for_paint (win);
  base = status_mb ("VmRSS");
  reset_peak ();
  for (int i = 0; i < RUNS; i++)
    {
      g_action_group_activate_action (G_ACTION_GROUP (win), "tool", g_variant_new_string ("select"));
      drag (win, w / 4, h / 4, w / 4 + w / 2 - 1, h / 4 + h / 2 - 1);
      if (!easel_canvas_get_selection (canvas, &sx, &sy, &sw, &sh, &floating))
        {
          g_print ("window %d x %d: no selection was marked\n", w, h);
          return;
        }
      invert[i] = timed (win, "invert-selection");                  /* a rectangle's inverse: one hole */
      again[i] = timed (win, "invert-selection");                   /* and the inverse of that: the rectangle as a shape */
      if (i == 0)
        peak_first = status_mb ("VmHWM");
    }
  peak = status_mb ("VmHWM");
  g_print ("window %4d x %-4d at %d%%, a rectangle of half the sides marked: invert %.0f %.0f %.0f ms, invert again %.0f %.0f %.0f ms (each to the painted frame) | resident %.0f MB "
           "with the picture shown; peak %.0f MB after the first run, %.0f MB after three\n",
           w, h, easel_window_get_zoom (win), invert[0], invert[1], invert[2], again[0], again[1], again[2], base, peak_first, peak);
  gtk_window_destroy (GTK_WINDOW (win));
  while (g_main_context_pending (NULL))
    g_main_context_iteration (NULL, FALSE);
}

/* The floating case (a rectangle of half the sides moved by a few device pixels: Invert Selection puts it down, then selects the rest) and the jagged case (a Free-Form zigzag of 200 teeth over the
   picture's middle half: the inverse has the same jagged edge, drawn every frame), each to the painted frame, three runs. */
static void
measure_window_extra (const char *config_dir, int w, int h)
{
  double floating_ms[RUNS], jagged[RUNS], jagged_again[RUNS], base, peak;
  EaselWindow *win;
  EaselCanvas *canvas;
  int sx, sy, sw, sh;
  gboolean floating;
  const int teeth = 200;

  write_settings (config_dir, w, h);
  win = open_window ();
  canvas = easel_window_get_canvas (win);
  g_action_group_activate_action (G_ACTION_GROUP (win), "zoom-fit", NULL);
  wait_for_paint (win);
  wait_for_paint (win);
  base = status_mb ("VmRSS");
  reset_peak ();
  for (int i = 0; i < RUNS; i++)
    {
      double l = easel_window_get_zoom (win) / 100.0 / easel_canvas_get_scale (canvas);

      g_action_group_activate_action (G_ACTION_GROUP (win), "tool", g_variant_new_string ("select"));
      drag (win, w / 4, h / 4, w / 4 + w / 2 - 1, h / 4 + h / 2 - 1);
      drag (win, w / 2, h / 2, w / 2 + (int) (8 / l) + 3, h / 2 + (int) (8 / l) + 2);   /* moved by a few device pixels: floating */
      if (!easel_canvas_get_selection (canvas, &sx, &sy, &sw, &sh, &floating) || !floating)
        {
          g_print ("window %d x %d: no floating selection\n", w, h);
          return;
        }
      floating_ms[i] = timed (win, "invert-selection");
      g_action_group_activate_action (G_ACTION_GROUP (win), "tool", g_variant_new_string ("free-select"));
      easel_canvas_press (canvas, 1, (w / 4) * l, (h / 4) * l);
      for (int k = 0; k <= teeth; k++)
        easel_canvas_motion (canvas, (w / 4 + (double) k * (w / 2) / teeth) * l, (k % 2 ? h / 4 + h / 2 : h / 4) * l);
      easel_canvas_motion (canvas, (w / 4 + w / 2) * l, (h / 4 + h / 2 + h / 8) * l);
      easel_canvas_motion (canvas, (w / 4) * l, (h / 4 + h / 2 + h / 8) * l);
      easel_canvas_release (canvas, 1);
      wait_for_paint (win);
      if (!easel_canvas_get_selection (canvas, &sx, &sy, &sw, &sh, &floating))
        {
          g_print ("window %d x %d: no zigzag was marked\n", w, h);
          return;
        }
      jagged[i] = timed (win, "invert-selection");
      jagged_again[i] = timed (win, "invert-selection");
    }
  peak = status_mb ("VmHWM");
  g_print ("window %4d x %-4d at %d%%: a floating rectangle of half the sides, moved: invert (put down, then select the rest) %.0f %.0f %.0f ms | a zigzag of %d teeth: invert %.0f %.0f %.0f ms, "
           "invert again %.0f %.0f %.0f ms (each to the painted frame) | resident %.0f MB with the picture shown, peak %.0f MB\n",
           w, h, easel_window_get_zoom (win), floating_ms[0], floating_ms[1], floating_ms[2], teeth, jagged[0], jagged[1], jagged[2], jagged_again[0], jagged_again[1], jagged_again[2], base,
           peak);
  gtk_window_destroy (GTK_WINDOW (win));
  while (g_main_context_pending (NULL))
    g_main_context_iteration (NULL, FALSE);
}

int
main (int argc, char **argv)
{
  if (argc > 1 && strcmp (argv[1], "window") == 0)
    {
      g_autofree char *config_dir = g_dir_make_tmp ("easel-perf-XXXXXX", NULL);
      int size = argc > 2 ? atoi (argv[2]) : 0;

      g_setenv ("XDG_CONFIG_HOME", config_dir, TRUE);
      if (!gtk_init_check ())
        return 77;
      app = easel_application_new ();
      g_application_set_flags (G_APPLICATION (app), G_APPLICATION_NON_UNIQUE);
      g_application_register (G_APPLICATION (app), NULL, NULL);
      for (int pass = 0; pass < 2; pass++)
        if (size == 0 || size == (pass == 0 ? 1080 : 8000))
          {
            measure_window (config_dir, pass == 0 ? 1920 : 8000, pass == 0 ? 1080 : 8000);
            measure_window_extra (config_dir, pass == 0 ? 1920 : 8000, pass == 0 ? 1080 : 8000);
          }
      g_object_unref (app);
      return 0;
    }
  for (int c = 0; c < 3; c++)
    if (argc <= 1 || strcmp (argv[1], cases[c]) == 0)
      measure (c);
  return 0;
}
