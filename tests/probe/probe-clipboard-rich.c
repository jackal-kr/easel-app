/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Probe of feature 0017's risk R6 (the Plan of docs/specs/0017-text.md): what other apps do with rich text on the clipboard.
   Run by hand in the real session, two processes, each acting from a click (the compositor takes a clipboard only after input):
     build/tests/probe-clipboard-rich own    a window whose button puts on the clipboard the HTML fragment Easel would offer (inline
                                            styles: family, size in points, color, bold, italic, underline, line-through) and the
                                            same text as plain UTF-8; paste it into LibreOffice Writer and into a rich-text field of
                                            Firefox and look at the formatting
     build/tests/probe-clipboard-rich read NAME   reads what the clipboard offers now (its MIME types; text/html and text/plain whole)
                                            and saves them as tests/data/rich/NAME.html and NAME.txt in the source tree, from
                                            whatever folder it is started in; run it after copying
                                            formatted text in Writer or Firefox
   Not a test: it prints what it does. */

#include <gtk/gtk.h>

/* The fragment as Easel would write it: one span per run. */
static const char html[] =
  "<meta charset=\"utf-8\">"
  "<span style=\"font-family:'DejaVu Sans';font-size:12pt;color:#000000\">plain </span>"
  "<span style=\"font-family:'DejaVu Serif';font-size:18pt;color:#ff0000;font-weight:bold\">bold red serif </span>"
  "<span style=\"font-family:'DejaVu Sans Mono';font-size:10pt;color:#0000ff;font-style:italic\">italic blue mono </span>"
  "<span style=\"font-family:'DejaVu Sans';font-size:14pt;color:#008000;text-decoration:underline line-through\">underlined struck green</span>"
  "<br><span style=\"font-family:'DejaVu Sans';font-size:12pt;color:#000000\">second line</span>";
static const char plain[] = "plain bold red serif italic blue mono underlined struck green\nsecond line";

static void
on_click (GtkButton *button, gpointer data)
{
  GdkContentProvider *providers[2] = {
    gdk_content_provider_new_for_bytes ("text/html", g_bytes_new_static (html, sizeof html - 1)),
    gdk_content_provider_new_for_bytes ("text/plain;charset=utf-8", g_bytes_new_static (plain, sizeof plain - 1)),
  };
  GdkContentProvider *union_ = gdk_content_provider_new_union (providers, 2);

  gdk_clipboard_set_content (gtk_widget_get_clipboard (GTK_WIDGET (button)), union_);
  g_object_unref (union_);
  g_print ("own: text/html (%zu bytes) and text/plain on the clipboard; paste into Writer and Firefox now\n", sizeof html - 1);
}

static void
on_activate (GApplication *app, gpointer data)
{
  GtkWidget *window = gtk_application_window_new (GTK_APPLICATION (app));
  GtkWidget *button = gtk_button_new_with_label ("Click: put Easel's rich text on the clipboard");

  g_signal_connect (button, "clicked", G_CALLBACK (on_click), NULL);
  gtk_window_set_title (GTK_WINDOW (window), "probe-clipboard-rich own");
  gtk_window_set_child (GTK_WINDOW (window), button);
  gtk_window_set_default_size (GTK_WINDOW (window), 420, 80);
  gtk_window_present (GTK_WINDOW (window));
}

static GAsyncResult *got;

static void
on_done (GObject *source, GAsyncResult *result, gpointer data)
{
  got = g_object_ref (result);
}

/* Reads one MIME type whole, waiting at most 10 s; NULL if it is not there. */
static GBytes *
read_type (GdkClipboard *clipboard, const char *mime)
{
  GInputStream *stream;
  GOutputStream *sink;
  gint64 deadline = g_get_monotonic_time () + 10 * G_USEC_PER_SEC;
  GBytes *bytes;

  got = NULL;
  gdk_clipboard_read_async (clipboard, (const char *[]) { mime, NULL }, G_PRIORITY_DEFAULT, NULL, on_done, NULL);
  while (got == NULL && g_get_monotonic_time () < deadline)
    g_main_context_iteration (NULL, FALSE);
  if (got == NULL)
    return NULL;
  stream = gdk_clipboard_read_finish (clipboard, got, NULL, NULL);
  g_clear_object (&got);
  if (stream == NULL)
    return NULL;
  sink = g_memory_output_stream_new_resizable ();
  g_output_stream_splice_async (sink, stream, G_OUTPUT_STREAM_SPLICE_CLOSE_SOURCE | G_OUTPUT_STREAM_SPLICE_CLOSE_TARGET,
                                G_PRIORITY_DEFAULT, NULL, on_done, NULL);
  while (got == NULL && g_get_monotonic_time () < deadline)
    g_main_context_iteration (NULL, FALSE);
  if (got == NULL)
    return NULL;
  g_output_stream_splice_finish (sink, got, NULL);
  g_clear_object (&got);
  bytes = g_memory_output_stream_steal_as_bytes (G_MEMORY_OUTPUT_STREAM (sink));
  g_object_unref (sink);
  g_object_unref (stream);
  return bytes;
}

int
main (int argc, char **argv)
{
  if (argc > 2 && g_strcmp0 (argv[1], "read") == 0)
    {
      const char *exts[] = { "html", "txt" }, *types[] = { "text/html", "text/plain;charset=utf-8" };
      GtkWidget *window;
      GdkClipboard *clipboard;
      g_autofree char *formats = NULL;

      gtk_init ();
      window = gtk_window_new ();
      gtk_window_present (GTK_WINDOW (window));
      clipboard = gtk_widget_get_clipboard (window);
      for (gint64 d = g_get_monotonic_time () + 500000; g_get_monotonic_time () < d;)
        g_main_context_iteration (NULL, FALSE);
      formats = gdk_content_formats_to_string (gdk_clipboard_get_formats (clipboard));
      g_print ("read: the clipboard offers %s\n", formats);
      g_autofree char *dir = g_build_filename (EASEL_SOURCE_DIR, "tests", "data", "rich", NULL);

      g_mkdir_with_parents (dir, 0755);
      for (int i = 0; i < 2; i++)
        {
          g_autoptr (GBytes) b = read_type (clipboard, types[i]);
          g_autofree char *name = g_strdup_printf ("%s.%s", argv[2], exts[i]);
          g_autofree char *path = g_build_filename (dir, name, NULL);

          if (b == NULL)
            {
              g_print ("read: no %s\n", types[i]);
              continue;
            }
          g_file_set_contents (path, g_bytes_get_data (b, NULL), (gssize) g_bytes_get_size (b), NULL);
          g_print ("read: %s, %zu bytes, saved as %s\n", types[i], g_bytes_get_size (b), path);
        }
      return 0;
    }
  {
    g_autoptr (GtkApplication) app = gtk_application_new ("org.easel.ProbeClipboardRich", G_APPLICATION_NON_UNIQUE);

    g_signal_connect (app, "activate", G_CALLBACK (on_activate), NULL);
    return g_application_run (G_APPLICATION (app), 1, argv);
  }
}
