/* SPDX-License-Identifier: GPL-3.0-or-later */
/* What Easel's Paste of text sees on the desktop's clipboard (feature 0017, M2: rich text from Writer and Firefox did not paste): click
   its button after copying in another app; it prints the formats offered, whether they count as text for a box's Paste, and reads
   text/html (else plain text) through Easel's clipboard seam as Paste does, then what the HTML reader makes of it. Wayland gives the
   clipboard only to the app with the keyboard focus, hence the click. Not a test: it prints what it finds.
   Run by hand in the real session: build/tests/probe-paste-text */

#include <stdio.h>

#include "clipboard.h"
#include "rich-text.h"

static EaselClipboard *seam;

static void
on_spliced (GObject *sink, GAsyncResult *result, gpointer data)
{
  gboolean html = GPOINTER_TO_INT (data);
  GError *error = NULL;
  gssize n = g_output_stream_splice_finish (G_OUTPUT_STREAM (sink), result, &error);
  g_autoptr (GBytes) bytes = NULL;
  gsize len = 0;
  const char *text;

  if (n < 0)
    {
      printf ("the read failed: %s\n", error->message);
      g_clear_error (&error);
      return;
    }
  bytes = g_memory_output_stream_steal_as_bytes (G_MEMORY_OUTPUT_STREAM (sink));
  text = g_bytes_get_data (bytes, &len);
  printf ("read %zu bytes; the first 300:\n%.*s\n", len, (int) MIN (len, 300), text != NULL ? text : "");
  if (html)
    {
      EaselRichText rt;

      if (!easel_rich_text_from_html (text != NULL ? text : "", len, &rt))
        printf ("the HTML reader: no memory\n");
      else
        {
          printf ("the HTML reader: %zu bytes of text in %zu runs: [%s]\n", rt.len, rt.n, rt.text);
          for (size_t i = 0; i < rt.n; i++)
            printf ("  run at %zu: family %s, %d pt, flags %u, color %s%08X\n", rt.runs[i].start,
                    rt.runs[i].family != NULL ? rt.runs[i].family : "(none)", rt.runs[i].size_pt, rt.runs[i].flags,
                    rt.runs[i].has_color ? "#" : "(none) ", rt.runs[i].color);
          easel_rich_text_free (&rt);
        }
    }
  fflush (stdout);
}

static void
on_read (GObject *source, GAsyncResult *result, gpointer data)
{
  const char *mime = NULL;
  GError *error = NULL;
  GInputStream *stream = easel_clipboard_read_finish (seam, result, &mime, &error);
  GOutputStream *sink;

  if (stream == NULL)
    {
      printf ("the read did not start: %s\n", error != NULL ? error->message : "(no error)");
      g_clear_error (&error);
      fflush (stdout);
      return;
    }
  printf ("reading as %s\n", mime);
  sink = g_memory_output_stream_new_resizable ();
  g_output_stream_splice_async (sink, stream, G_OUTPUT_STREAM_SPLICE_CLOSE_SOURCE | G_OUTPUT_STREAM_SPLICE_CLOSE_TARGET,
                                G_PRIORITY_DEFAULT, NULL, on_spliced, GINT_TO_POINTER (g_strcmp0 (mime, "text/html") == 0));
  g_object_unref (stream);
}

static void
on_click (GtkButton *button, gpointer data)
{
  static const char *html_types[] = { "text/html", NULL };
  static const char *text_types[] = { "text/plain;charset=utf-8", "text/plain", "UTF8_STRING", NULL };
  GdkContentFormats *formats = easel_clipboard_get_formats (seam);
  g_autofree char *listed = gdk_content_formats_to_string (formats);
  gboolean html = gdk_content_formats_contain_mime_type (formats, "text/html");
  gboolean text = gdk_content_formats_contain_gtype (formats, G_TYPE_STRING)
                  || gdk_content_formats_contain_mime_type (formats, "text/plain;charset=utf-8")
                  || gdk_content_formats_contain_mime_type (formats, "text/plain")
                  || gdk_content_formats_contain_mime_type (formats, "UTF8_STRING");

  printf ("\nformats: %s\nlocal: %d; text/html: %d; plain text: %d\n", listed, easel_clipboard_is_local (seam), html, text);
  fflush (stdout);
  easel_clipboard_read_async (seam, html ? html_types : text_types, NULL, on_read, NULL);
}

static void
on_activate (GApplication *app, gpointer data)
{
  GtkWidget *window = gtk_application_window_new (GTK_APPLICATION (app));
  GtkWidget *button = gtk_button_new_with_label ("Click: read the clipboard as Easel's Paste does");

  seam = easel_gdk_clipboard_new (gtk_widget_get_clipboard (window));
  g_signal_connect (button, "clicked", G_CALLBACK (on_click), NULL);
  gtk_window_set_title (GTK_WINDOW (window), "probe-paste-text");
  gtk_window_set_child (GTK_WINDOW (window), button);
  gtk_window_set_default_size (GTK_WINDOW (window), 420, 80);
  gtk_window_present (GTK_WINDOW (window));
}

int
main (int argc, char **argv)
{
  g_autoptr (GtkApplication) app = gtk_application_new ("org.easel.ProbePasteText", G_APPLICATION_NON_UNIQUE);

  g_signal_connect (app, "activate", G_CALLBACK (on_activate), NULL);
  return g_application_run (G_APPLICATION (app), 1, argv);
}
