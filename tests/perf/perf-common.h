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

static void
write_settings (const char *dir, int w, int h)
{
  g_autofree char *path = g_build_filename (dir, "easel", "settings.ini", NULL);
  g_autofree char *parent = g_path_get_dirname (path);
  g_autofree char *text = g_strdup_printf ("[new-picture]\nwidth=%d\nheight=%d\n", w, h);

  g_mkdir_with_parents (parent, 0700);
  g_file_set_contents (path, text, -1, NULL);
}

static EaselWindow *
open_window (void)
{
  EaselWindow *win;
  GdkFrameClock *clock;

  g_application_activate (G_APPLICATION (app));
  win = EASEL_WINDOW (gtk_application_get_active_window (app));
  while (!gtk_widget_get_realized (GTK_WIDGET (win)))
    g_main_context_iteration (NULL, TRUE);
  clock = gtk_widget_get_frame_clock (GTK_WIDGET (win));
  g_signal_connect (clock, "after-paint", G_CALLBACK (on_after_paint), NULL);
  wait_for_paint (win);
  return win;
}

static int
cmp_double (const void *a, const void *b)
{
  double x = *(const double *) a, y = *(const double *) b;

  return (x > y) - (x < y);
}
