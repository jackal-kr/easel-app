/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Measurements of feature 0018 (T003): Flip Horizontal, Rotate 90°, Invert Colors and Clear Image on a 1920 x 1080 and an 8000 x 8000
   picture, each timed twice: the core alone (REQ-0018-12: < 500 ms, on a scratch picture with a history of its own) and the whole path,
   from the activation of the window's action to the next painted frame, which includes the canvas rebuilding its tiles (the texture
   upload); then Undo and Redo of the step to their frames, and the process's peak memory. Three runs of each, the load noted by the
   caller. Run in the real session, with no other Easel window open and the pointer outside its window: a real pointer motion with no
   button down is a lost release (REQ-0007-18). It prints numbers and does not fail.
   Usage: perf-0018 [SIZE [COMMAND]]   SIZE 1080 or 8000 (the picture is 1920 x 1080, or 8000 x 8000); COMMAND flip-h, rot90, invert, clear;
   or sel-flip-h, sel-rot90, sel-invert: the same commands on a selection of the whole picture (T004): the first run lifts the marked
   selection and makes the command, the later ones make it on the floating selection; handle-flip: a 2000 x 2000 or 8000 x 8000 (SIZE) selection of the whole
   picture stretched by its bottom-right handle across the top-left corner and back (T006, REQ-0018-11): the first event (the lift), the
   events that cross the opposite edge, and the ones that stay on one side, each to the painted frame, at 100 % and at the fit zoom. */

#include "perf-common.h"

#include <stdio.h>
#include <string.h>

#include "transform.h"

#define RUNS 3

/* The peak resident memory of the process so far, in MB (VmHWM, a high-water mark shared by the cases of one run), and the present one. */
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

/* Starts the peak (VmHWM) again from the present resident size (Linux: "5" to clear_refs), so that a peak reported afterwards is that of what
   ran after this call only. */
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

/* Every pixel different (the worst case of a count of the pixels a command changes, and nothing for a flip to leave as it is). */
static void
pattern (EaselImage *img)
{
  for (gsize i = 0; i < (gsize) img->width * (gsize) img->height; i++)
    img->px[i] = 0xFF000000u | (guint32) (i * 2654435761u);
  img->dirty = (EaselRect) { 0, 0, img->width, img->height };
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

static const struct { const char *name, *action; EaselOp op; } commands[] = {
  { "flip-h", "flip-horizontal", EASEL_OP_FLIP_H },
  { "rot90", "rotate-90", EASEL_OP_ROTATE_90 },
  { "invert", "invert", EASEL_OP_INVERT },
  { "clear", "clear-image", EASEL_OP_CLEAR },
};

static void
measure (const char *config_dir, int w, int h, guint c)
{
  double core[RUNS], path[RUNS], undo_ms, redo_ms, picture_mb, before, peak, peak_first = 0;
  EaselWindow *win;
  EaselImage *img;
  EaselHistory *hist;

  /* the core alone, on a scratch picture, patterned again before each run (a clear of a cleared picture is no work) */
  img = easel_image_new (w, h, 0);
  hist = easel_history_new ((gsize) 4 << 30);
  for (int i = 0; i < RUNS; i++)
    {
      gint64 start;

      pattern (img);
      start = g_get_monotonic_time ();
      easel_image_transform (img, hist, commands[c].op, 0xFF00FF00u, (gsize) -1);
      core[i] = (g_get_monotonic_time () - start) / 1000.0;
      easel_history_undo (hist, img);
    }
  easel_history_free (hist);
  easel_image_free (img);

  /* the whole path, in a window on a picture of the size */
  write_settings (config_dir, w, h);
  win = open_window ();
  g_action_group_activate_action (G_ACTION_GROUP (win), "zoom-normal", NULL);
  wait_for_paint (win);
  img = (EaselImage *) easel_window_get_picture (win);
  pattern (img);
  easel_canvas_update (easel_window_get_canvas (win));
  wait_for_paint (win);
  reset_peak ();                                           /* the core phase above is not part of the window's peak */
  picture_mb = status_mb ("VmRSS");
  before = status_mb ("VmHWM");
  for (int i = 0; i < RUNS; i++)
    {
      pattern (img);
      easel_canvas_update (easel_window_get_canvas (win));
      wait_for_paint (win);
      path[i] = timed (win, commands[c].action);
      if (i == 0)
        {
          peak_first = status_mb ("VmHWM");                  /* one command, no redoable step yet */
          undo_ms = timed (win, "undo");
          redo_ms = timed (win, "redo");
        }
      g_action_group_activate_action (G_ACTION_GROUP (win), "undo", NULL);    /* back to the patterned picture, off the clock */
      wait_for_paint (win);
    }
  peak = status_mb ("VmHWM");
  g_print ("%-7s %d x %d: core %.0f %.0f %.0f ms (%s < 500) | whole path %.0f %.0f %.0f ms | undo %.0f redo %.0f ms | "
           "resident %.0f MB with the picture shown; peak %.0f MB after the first run, %.0f MB after three (the later runs find a redoable step, "
           "the peak was %.0f at the start)\n",
           commands[c].name, w, h, core[0], core[1], core[2],
           MAX (core[0], MAX (core[1], core[2])) < 500 ? "within" : "OVER", path[0], path[1], path[2], undo_ms, redo_ms, picture_mb,
           peak_first, peak, before);
  gtk_window_destroy (GTK_WINDOW (win));
  while (g_main_context_pending (NULL))
    g_main_context_iteration (NULL, FALSE);
}

static const struct { const char *name, *action; } sel_commands[] = {
  { "sel-flip-h", "flip-horizontal" },
  { "sel-rot90", "rotate-90" },
  { "sel-invert", "invert" },
};

/* A command on a selection of the whole picture, three runs, the whole path each: the first includes the lift. */
static void
measure_selection (const char *config_dir, int w, int h, guint c)
{
  double path[RUNS], picture_mb, peak_first = 0, peak;
  EaselWindow *win;
  EaselImage *img;

  write_settings (config_dir, w, h);
  win = open_window ();
  g_action_group_activate_action (G_ACTION_GROUP (win), "zoom-normal", NULL);
  wait_for_paint (win);
  img = (EaselImage *) easel_window_get_picture (win);
  pattern (img);
  easel_canvas_update (easel_window_get_canvas (win));
  wait_for_paint (win);
  g_action_group_activate_action (G_ACTION_GROUP (win), "select-all", NULL);
  wait_for_paint (win);
  reset_peak ();
  picture_mb = status_mb ("VmRSS");
  for (int i = 0; i < RUNS; i++)
    {
      path[i] = timed (win, sel_commands[c].action);
      if (i == 0)
        peak_first = status_mb ("VmHWM");
    }
  peak = status_mb ("VmHWM");
  g_print ("%-12s %d x %d: whole path %.0f (with the lift) %.0f %.0f ms | resident %.0f MB with the picture shown; peak %.0f MB after the "
           "first run, %.0f MB after three\n",
           sel_commands[c].name, w, h, path[0], path[1], path[2], picture_mb, peak_first, peak);
  gtk_window_destroy (GTK_WINDOW (win));
  while (g_main_context_pending (NULL))
    g_main_context_iteration (NULL, FALSE);
}

/* One event of the drag to the painted frame, milliseconds. */
static double
drag_event (EaselWindow *win, EaselCanvas *canvas, double x, double y)
{
  gint64 start = g_get_monotonic_time ();

  easel_canvas_motion (canvas, x, y);
  wait_for_paint (win);
  return (g_get_monotonic_time () - start) / 1000.0;
}

/* A stretch by the bottom-right handle of a selection of the whole of a size x size picture that goes across the opposite corner and back
   (REQ-0018-11): the pointer 1 pixel in (the lift), then to 3 pixels past the top-left corner (mirrored in both axes), a few events on that
   side, and back across to the other side. The peak memory is that of the drag from the marked selection on. */
static void
measure_handle_flip (const char *config_dir, int size, gboolean fit)
{
  enum { N = 10 };
  EaselWindow *win;
  EaselCanvas *canvas;
  graphene_rect_t r;
  double x, y, l, first, cross, same[N], back, release, before, peak;
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
  l = easel_window_get_zoom (win) / 100.0 / easel_canvas_get_scale (canvas);
  g_assert (easel_canvas_get_selection_handle_rect (canvas, EASEL_SEL_HANDLE_BR, &r));
  x = r.origin.x + r.size.width / 2;
  y = r.origin.y + r.size.height / 2;
  reset_peak ();
  before = status_mb ("VmRSS");
  easel_canvas_press (canvas, 1, x, y);
  first = drag_event (win, canvas, x + l, y + l);
  cross = drag_event (win, canvas, x - (size + 3) * l, y - (size + 3) * l);
  for (int i = 0; i < N; i++)
    same[i] = drag_event (win, canvas, x - (size + 4 + i) * l, y - (size + 4 + i) * l);
  back = drag_event (win, canvas, x - (size * 3 / 4) * l, y - (size * 3 / 4) * l);
  start = g_get_monotonic_time ();
  easel_canvas_release (canvas, 1);
  wait_for_paint (win);
  release = (g_get_monotonic_time () - start) / 1000.0;
  peak = status_mb ("VmHWM");
  qsort (same, N, sizeof same[0], cmp_double);
  g_print ("handle-flip %d x %d, %s (zoom %d%%): first event (lift) %.0f ms; the event across the opposite corner %.0f ms; "
           "the next %d events on that side median %.1f ms, worst %.1f ms; the event back %.0f ms; release to the painted frame %.0f ms; resident %.0f MB before the drag, peak %.0f MB\n",
           size, size, fit ? "fit" : "100%", easel_window_get_zoom (win), first, cross, N, same[N / 2], same[N - 1], back, release, before, peak);
  gtk_window_destroy (GTK_WINDOW (win));
  while (g_main_context_pending (NULL))
    g_main_context_iteration (NULL, FALSE);
}

int
main (int argc, char **argv)
{
  g_autofree char *config_dir = g_dir_make_tmp ("easel-perf-XXXXXX", NULL);
  int size = argc > 1 ? atoi (argv[1]) : 0;

  g_setenv ("XDG_CONFIG_HOME", config_dir, TRUE);
  if (!gtk_init_check ())
    return 77;
  app = easel_application_new ();
  g_application_set_flags (G_APPLICATION (app), G_APPLICATION_NON_UNIQUE);
  g_application_register (G_APPLICATION (app), NULL, NULL);
  for (int pass = 0; pass < 2; pass++)
    {
      int w = pass == 0 ? 1920 : 8000, h = pass == 0 ? 1080 : 8000;

      if (size != 0 && size != h)
        continue;
      for (guint c = 0; c < G_N_ELEMENTS (commands); c++)
        if (argc <= 2 || strcmp (argv[2], commands[c].name) == 0)
          measure (config_dir, w, h, c);
      for (guint c = 0; c < G_N_ELEMENTS (sel_commands); c++)
        if (argc > 2 && strcmp (argv[2], sel_commands[c].name) == 0)
          measure_selection (config_dir, w, h, c);
    }
  if (argc > 2 && strcmp (argv[2], "handle-flip") == 0)
    for (int pass = 0; pass < 2; pass++)
      for (int fit = 0; fit <= 1; fit++)
        if (size == 0 || size == (pass == 0 ? 2000 : 8000))
          measure_handle_flip (config_dir, pass == 0 ? 2000 : 8000, fit);
  g_object_unref (app);
  return 0;
}
