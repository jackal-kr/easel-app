/* SPDX-License-Identifier: GPL-3.0-or-later */
/* A clipboard owner that answers late, for the manual step M2 of feature 0016 (REQ-0016-15: Paste waits for the clipboard with the busy
   cursor): it puts a picture on the desktop's clipboard and, when another app reads it, answers only after a delay, or never.
   Run by hand in the real session: build/tests/slow-clipboard-owner [SECONDS | never [PNG-FILE | text]]   (default 4 seconds; with a file,
   that PNG is what is offered, to tell what a browser does with a big picture served at once from what it does with a late one; with
   "text", a line of UTF-8 text is offered instead, for M2 of feature 0017: REQ-0017-27, Paste in a text box waits for it)
   Click its button (the compositor takes a clipboard only from a click), then press Ctrl+V in Easel and watch the busy cursor; "never" shows the dialog "Could Not Paste" after Easel's 5 seconds.
   Not a test: it prints what it does. */

#include <gtk/gtk.h>

#define SLOW_TYPE_PROVIDER (slow_provider_get_type ())
G_DECLARE_FINAL_TYPE (SlowProvider, slow_provider, SLOW, PROVIDER, GdkContentProvider)

struct _SlowProvider
{
  GdkContentProvider parent_instance;
  GBytes *png;                   /* what is offered: the picture, or the text */
  int delay_ms;                  /* < 0: never answers */
  gboolean text;                 /* text/plain;charset=utf-8 instead of image/png */
};

G_DEFINE_FINAL_TYPE (SlowProvider, slow_provider, GDK_TYPE_CONTENT_PROVIDER)

static GdkContentFormats *
slow_ref_formats (GdkContentProvider *provider)
{
  return gdk_content_formats_new ((const char *[]) { SLOW_PROVIDER (provider)->text ? "text/plain;charset=utf-8" : "image/png" }, 1);
}

typedef struct
{
  GTask *task;
  GOutputStream *stream;
  GBytes *png;
} Pending;

static void
on_written (GObject *stream, GAsyncResult *result, gpointer data)
{
  GTask *task = data;
  GError *error = NULL;

  if (!g_output_stream_write_all_finish (G_OUTPUT_STREAM (stream), result, NULL, &error))
    g_task_return_error (task, error);
  else
    g_task_return_boolean (task, TRUE);
  g_object_unref (task);
}

static gboolean
answer_now (gpointer data)
{
  Pending *p = data;

  g_print ("answering now\n");
  g_task_set_task_data (p->task, p->png, (GDestroyNotify) g_bytes_unref);
  g_output_stream_write_all_async (p->stream, g_bytes_get_data (p->png, NULL), g_bytes_get_size (p->png), G_PRIORITY_DEFAULT, NULL, on_written,
                                   p->task);
  g_object_unref (p->stream);
  g_free (p);
  return G_SOURCE_REMOVE;
}

static void
slow_write_async (GdkContentProvider *provider, const char *mime_type, GOutputStream *stream, int io_priority,
                  GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
  SlowProvider *self = SLOW_PROVIDER (provider);
  Pending *p = g_new0 (Pending, 1);

  g_print ("asked for %s; %s\n", mime_type, self->delay_ms < 0 ? "never answering" : "answering after the delay");
  p->task = g_task_new (provider, cancellable, callback, user_data);
  p->stream = g_object_ref (stream);
  p->png = g_bytes_ref (self->png);
  if (self->delay_ms >= 0)
    g_timeout_add (self->delay_ms, answer_now, p);
}

static gboolean
slow_write_finish (GdkContentProvider *provider, GAsyncResult *result, GError **error)
{
  return g_task_propagate_boolean (G_TASK (result), error);
}

static void
slow_provider_class_init (SlowProviderClass *klass)
{
  GdkContentProviderClass *provider = GDK_CONTENT_PROVIDER_CLASS (klass);

  provider->ref_formats = slow_ref_formats;
  provider->write_mime_type_async = slow_write_async;
  provider->write_mime_type_finish = slow_write_finish;
}

static void
slow_provider_init (SlowProvider *self)
{
}

/* A 200 x 100 picture, a gradient, so that the paste is easy to see. */
static GBytes *
make_png (void)
{
  g_autoptr (GdkPixbuf) pb = gdk_pixbuf_new (GDK_COLORSPACE_RGB, TRUE, 8, 200, 100);
  guint8 *px = gdk_pixbuf_get_pixels (pb);
  int stride = gdk_pixbuf_get_rowstride (pb);
  char *buffer = NULL;
  gsize size = 0;

  for (int y = 0; y < 100; y++)
    for (int x = 0; x < 200; x++)
      {
        guint8 *p = px + (gsize) y * stride + (gsize) x * 4;

        p[0] = (guint8) (x * 255 / 199);
        p[1] = (guint8) (y * 255 / 99);
        p[2] = 128;
        p[3] = 255;
      }
  gdk_pixbuf_save_to_buffer (pb, &buffer, &size, "png", NULL, NULL);
  return g_bytes_new_take (buffer, size);
}

static void
on_click (GtkButton *button, gpointer data)
{
  SlowProvider *provider = data;

  /* The compositor takes a selection only from a client that has just had input: it is set from the click. */
  gdk_clipboard_set_content (gtk_widget_get_clipboard (GTK_WIDGET (button)), GDK_CONTENT_PROVIDER (provider));
  g_print ("on the clipboard: %s, %s\n", provider->text ? "a line of text" : "a 200 x 100 PNG",
           provider->delay_ms < 0 ? "never answering" : "answering after the delay");
}

static void
on_activate (GApplication *app, gpointer data)
{
  SlowProvider *provider = g_object_new (SLOW_TYPE_PROVIDER, NULL);
  GtkWidget *window = gtk_application_window_new (GTK_APPLICATION (app));
  GtkWidget *button = gtk_button_new_with_label ("Click: put the slow content on the clipboard");
  const char *arg = data, *file = g_object_get_data (G_OBJECT (app), "png-file");

  provider->png = make_png ();
  if (g_strcmp0 (file, "text") == 0)
    {
      provider->text = TRUE;
      g_bytes_unref (provider->png);
      provider->png = g_bytes_new_static ("Slow text from another app", 26);
      file = NULL;
    }
  if (file != NULL)
    {
      char *contents = NULL;
      gsize len = 0;

      if (g_file_get_contents (file, &contents, &len, NULL))
        {
          g_bytes_unref (provider->png);
          provider->png = g_bytes_new_take (contents, len);
          g_print ("serving %s: %.1f MB\n", file, len / 1048576.0);
        }
    }
  provider->delay_ms = arg != NULL && g_strcmp0 (arg, "never") == 0 ? -1 : (arg != NULL ? (int) (atof (arg) * 1000) : 4000);
  g_signal_connect (button, "clicked", G_CALLBACK (on_click), provider);
  gtk_window_set_title (GTK_WINDOW (window), "slow-clipboard-owner");
  gtk_window_set_child (GTK_WINDOW (window), button);
  gtk_window_set_default_size (GTK_WINDOW (window), 360, 80);
  gtk_window_present (GTK_WINDOW (window));
}

int
main (int argc, char **argv)
{
  g_autoptr (GtkApplication) app = gtk_application_new ("org.easel.SlowClipboardOwner", G_APPLICATION_NON_UNIQUE);

  if (argc > 2)
    g_object_set_data (G_OBJECT (app), "png-file", argv[2]);
  g_signal_connect (app, "activate", G_CALLBACK (on_activate), argc > 1 ? argv[1] : NULL);
  return g_application_run (G_APPLICATION (app), 1, argv);
}
