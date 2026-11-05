/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Helpers shared by the performance benchmarks: a window on a picture of a given size, and waiting for a painted frame. */
#pragma once

#include <adwaita.h>
#include <stdlib.h>

#include "application.h"
#include "window.h"

static GtkApplication *app;
static gboolean painted;

static void
on_after_paint (GdkFrameClock *clock, gpointer data)
{
  painted = TRUE;
}

/* Runs the main loop until the window paints a frame (at most 5 s). */
static void
wait_for_paint (EaselWindow *win)
{
  gint64 deadline = g_get_monotonic_time () + 5 * G_USEC_PER_SEC;

  painted = FALSE;
  gtk_widget_queue_draw (GTK_WIDGET (easel_window_get_canvas (win)));
  while (!painted && g_get_monotonic_time () < deadline)
    g_main_context_iteration (NULL, TRUE);
}

G_GNUC_UNUSED static void
write_settings (const char *dir, int w, int h)
{
  g_autofree char *path = g_build_filename (dir, "easel", "settings.ini", NULL);
  g_autofree char *parent = g_path_get_dirname (path);
  g_autofree char *text = g_strdup_printf ("[new-picture]\nwidth=%d\nheight=%d\n", w, h);

  g_mkdir_with_parents (parent, 0700);
  g_file_set_contents (path, text, -1, NULL);
}

/* A second Easel or test window changes the layout of the benchmark's window and its timings, so a benchmark refuses to run
   with one open, as the ui tests do. */
static void
refuse_if_other_easel (void)
{
  g_autoptr (GDir) proc = g_dir_open ("/proc", 0, NULL);
  const char *entry;

  if (g_getenv ("EASEL_ISOLATED") != NULL)
    {
      g_printerr ("Benchmarks stay in the real session: the isolated session (tools/isolated.sh) has a virtual monitor and its "
                  "own frame clock, so its timings are not comparable.\n");
      exit (1);
    }

  while (proc != NULL && (entry = g_dir_read_name (proc)) != NULL)
    {
      g_autofree char *path = g_strdup_printf ("/proc/%s/comm", entry);
      g_autofree char *comm = NULL;
      int pid = atoi (entry);

      if (pid > 0 && pid != getpid () && g_file_get_contents (path, &comm, NULL, NULL)
          && (g_str_has_prefix (comm, "easel\n") || g_str_has_prefix (comm, "test-window\n")))
        {
          g_printerr ("Close every Easel window first: process %d is an Easel or a ui test, and a second window changes the "
                      "timings of this benchmark.\n", pid);
          exit (1);
        }
    }
}

static EaselWindow *
open_window (void)
{
  EaselWindow *win;
  GdkFrameClock *clock;
  static gboolean checked, checked_pointer;

  if (!checked)
    refuse_if_other_easel ();
  checked = TRUE;

  if (!checked_pointer)
    g_printerr ("Keep the pointer outside the window while this runs: a real motion with no button down ends a held drag (REQ-0007-18).\n");
  checked_pointer = TRUE;
  g_application_activate (G_APPLICATION (app));
  win = EASEL_WINDOW (gtk_application_get_active_window (app));
  while (!gtk_widget_get_realized (GTK_WIDGET (win)))
    g_main_context_iteration (NULL, TRUE);
  clock = gtk_widget_get_frame_clock (GTK_WIDGET (win));
  g_signal_connect (clock, "after-paint", G_CALLBACK (on_after_paint), NULL);
  wait_for_paint (win);
  return win;
}

G_GNUC_UNUSED static int
cmp_double (const void *a, const void *b)
{
  double x = *(const double *) a, y = *(const double *) b;

  return (x > y) - (x < y);
}
