/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Two processes, as Easel and another app are (feature 0016, M1: "paste into other apps does not work"): `own [WxH]` puts a picture on the
   desktop's clipboard through Easel's own provider (easel_clipboard_picture_new, the code Copy uses) and stays; `read` reads image/png from
   the clipboard as another app would and prints what arrives. Run by hand in the real session: build/tests/probe-clipboard-xproc own &
   then build/tests/probe-clipboard-xproc read. Not a test. */

#include <gtk/gtk.h>

#include "clipboard.h"

static GAsyncResult *got;

static void
on_done (GObject *source, GAsyncResult *result, gpointer data)
{
  got = g_object_ref (result);
}

static void
wait_for (double seconds)
{
  gint64 deadline = g_get_monotonic_time () + (gint64) (seconds * G_USEC_PER_SEC);

  while (got == NULL && g_get_monotonic_time () < deadline)
    g_main_context_iteration (NULL, FALSE);
}

static void
on_click (GtkButton *button, gpointer data)
{
  GdkContentProvider *provider = data;

  /* The compositor takes a selection only from a client that has just had input, so this is set from the click. */
  gdk_clipboard_set_content (gtk_widget_get_clipboard (GTK_WIDGET (button)), provider);
  g_print ("own: set from the click; now read it from another terminal: build/tests/probe-clipboard-xproc read\n");
}

static void
on_activate (GApplication *app, gpointer data)
{
  int w = 200, h = 100;
  GtkWidget *window = gtk_application_window_new (GTK_APPLICATION (app));
  GtkWidget *button = gtk_button_new_with_label ("Click: put the copy on the clipboard");
  EaselImage *copy;
  GdkContentProvider *provider;

  if (data != NULL)
    sscanf (data, "%dx%d", &w, &h);
  copy = easel_image_new (w, h, 0xFF336699u);
  for (int x = 0; x < w && x < 50; x++)
    easel_image_set (copy, x, 0, 0xFFFF0000u);
  provider = easel_clipboard_picture_new (copy);
  g_signal_connect (button, "clicked", G_CALLBACK (on_click), provider);
  gtk_window_set_title (GTK_WINDOW (window), "probe-clipboard-xproc own");
  gtk_window_set_child (GTK_WINDOW (window), button);
  gtk_window_set_default_size (GTK_WINDOW (window), 360, 80);
  gtk_window_present (GTK_WINDOW (window));
  g_print ("own: a %d x %d copy waits; click the button in its window\n", w, h);
}

int
main (int argc, char **argv)
{
  if (argc > 1 && g_strcmp0 (argv[1], "read") == 0)
    {
      GdkClipboard *clipboard;
      GtkWidget *window;
      g_autofree char *formats = NULL;
      GInputStream *stream;
      const char *mime = NULL;
      GError *error = NULL;
      gsize total = 0;
      guint8 buffer[65536];
      FILE *out = fopen (argc > 2 ? argv[2] : "/tmp/xproc-read.png", "wb");

      gtk_init ();
      window = gtk_window_new ();
      gtk_window_present (GTK_WINDOW (window));
      clipboard = gtk_widget_get_clipboard (window);
      for (gint64 d = g_get_monotonic_time () + 500000; g_get_monotonic_time () < d;)
        g_main_context_iteration (NULL, FALSE);
      formats = gdk_content_formats_to_string (gdk_clipboard_get_formats (clipboard));
      g_print ("read: formats %s, local %d\n", formats, gdk_clipboard_is_local (clipboard));
      gdk_clipboard_read_async (clipboard, (const char *[]) { "image/png", NULL }, G_PRIORITY_DEFAULT, NULL, on_done, NULL);
      wait_for (10.0);
      if (got == NULL)
        {
          g_print ("read: no answer in 10 s\n");
          return 1;
        }
      stream = gdk_clipboard_read_finish (clipboard, got, &mime, &error);
      if (stream == NULL)
        {
          g_print ("read: failed: %s\n", error ? error->message : "?");
          return 1;
        }
      for (;;)
        {
          gssize n = g_input_stream_read (stream, buffer, sizeof buffer, NULL, &error);

          if (n < 0)
            {
              g_print ("read: error after %zu bytes: %s\n", total, error->message);
              return 1;
            }
          if (n == 0)
            break;
          if (total == 0)
            g_print ("read: first bytes %02x %02x %02x %02x (a PNG starts 89 50 4e 47)\n", buffer[0], buffer[1], buffer[2], buffer[3]);
          if (out != NULL)
            fwrite (buffer, 1, (size_t) n, out);
          total += (gsize) n;
        }
      if (out != NULL)
        fclose (out);
      g_print ("read: %s, %zu bytes\n", mime, total);
      return 0;
    }
  {
    g_autoptr (GtkApplication) app = gtk_application_new ("org.easel.ProbeXproc", G_APPLICATION_NON_UNIQUE);

    g_signal_connect (app, "activate", G_CALLBACK (on_activate), argc > 2 ? argv[2] : NULL);
    return g_application_run (G_APPLICATION (app), 1, argv);
  }
}
