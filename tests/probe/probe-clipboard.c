/* SPDX-License-Identifier: GPL-3.0-or-later */
/* The GDK clipboard behaviour feature 0016 relies on (Plan, "The clipboard seam" and Risks), probed in the real session: the isolated
   session holds no clipboard. Run by hand, as the benchmarks are, with no other Easel window open and the pointer outside the probe's
   window: build/tests/probe-clipboard. It opens a small window, waits for it to have the focus (a window may set the clipboard only
   then), and prints one line per behaviour. Not a test: it prints what it sees and does not fail. */

#include <gtk/gtk.h>

/* A provider of PNG bytes that counts the writes GDK asks of it. */
#define PROBE_TYPE_PROVIDER (probe_provider_get_type ())
G_DECLARE_FINAL_TYPE (ProbeProvider, probe_provider, PROBE, PROVIDER, GdkContentProvider)

struct _ProbeProvider
{
  GdkContentProvider parent_instance;
  GBytes *png;
  int writes;
};

G_DEFINE_FINAL_TYPE (ProbeProvider, probe_provider, GDK_TYPE_CONTENT_PROVIDER)

static GdkContentFormats *
probe_ref_formats (GdkContentProvider *provider)
{
  return gdk_content_formats_new ((const char *[]) { "image/png" }, 1);
}

static void
on_written (GObject *stream, GAsyncResult *result, gpointer data)
{
  g_autoptr (GTask) task = data;
  GError *error = NULL;

  if (g_output_stream_write_bytes_finish (G_OUTPUT_STREAM (stream), result, &error) < 0)
    g_task_return_error (task, error);
  else
    g_task_return_boolean (task, TRUE);
}

static void
probe_write_async (GdkContentProvider *provider, const char *mime_type, GOutputStream *stream, int io_priority,
                   GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
  ProbeProvider *self = PROBE_PROVIDER (provider);
  GTask *task = g_task_new (provider, cancellable, callback, user_data);

  self->writes++;
  /* Written without blocking: the reader of a local read is this same main loop. */
  g_output_stream_write_bytes_async (stream, self->png, io_priority, cancellable, on_written, task);
}

static gboolean
probe_write_finish (GdkContentProvider *provider, GAsyncResult *result, GError **error)
{
  return g_task_propagate_boolean (G_TASK (result), error);
}

static void
probe_provider_class_init (ProbeProviderClass *klass)
{
  GdkContentProviderClass *provider = GDK_CONTENT_PROVIDER_CLASS (klass);

  provider->ref_formats = probe_ref_formats;
  provider->write_mime_type_async = probe_write_async;
  provider->write_mime_type_finish = probe_write_finish;
}

static void
probe_provider_init (ProbeProvider *self)
{
}

static GAsyncResult *got;

static void
on_done (GObject *source, GAsyncResult *result, gpointer data)
{
  got = g_object_ref (result);
}

/* Runs the main loop until an asynchronous call answers or the seconds run out; the result, or NULL. */
static GAsyncResult *
wait_result (double seconds)
{
  gint64 deadline = g_get_monotonic_time () + (gint64) (seconds * G_USEC_PER_SEC);

  g_clear_object (&got);
  while (got == NULL && g_get_monotonic_time () < deadline)
    g_main_context_iteration (NULL, FALSE);
  return got;
}

static void
spin (double seconds)
{
  gint64 deadline = g_get_monotonic_time () + (gint64) (seconds * G_USEC_PER_SEC);

  while (g_get_monotonic_time () < deadline)
    g_main_context_iteration (NULL, FALSE);
}

static GBytes *
small_png (void)
{
  g_autoptr (GdkPixbuf) pb = gdk_pixbuf_new (GDK_COLORSPACE_RGB, TRUE, 8, 2, 2);
  char *buffer = NULL;
  gsize size = 0;

  gdk_pixbuf_fill (pb, 0xFF000080);
  gdk_pixbuf_save_to_buffer (pb, &buffer, &size, "png", NULL, NULL);
  return g_bytes_new_take (buffer, size);
}

int
main (int argc, char **argv)
{
  GtkWidget *window;
  GdkClipboard *clipboard;
  g_autoptr (ProbeProvider) provider = NULL;
  gint64 start;

  gtk_init ();
  window = gtk_window_new ();
  gtk_window_set_title (GTK_WINDOW (window), "probe-clipboard");
  gtk_window_set_default_size (GTK_WINDOW (window), 200, 100);
  gtk_window_present (GTK_WINDOW (window));
  clipboard = gtk_widget_get_clipboard (window);
  start = g_get_monotonic_time ();
  while (!gtk_window_is_active (GTK_WINDOW (window)) && g_get_monotonic_time () - start < 5 * G_USEC_PER_SEC)
    g_main_context_iteration (NULL, FALSE);
  g_print ("focus: the window is %s after %.1f s\n", gtk_window_is_active (GTK_WINDOW (window)) ? "active" : "NOT active",
           (g_get_monotonic_time () - start) / 1e6);

  /* 1. Ours after a set, and still ours a second later. */
  provider = g_object_new (PROBE_TYPE_PROVIDER, NULL);
  provider->png = small_png ();
  gdk_clipboard_set_content (clipboard, GDK_CONTENT_PROVIDER (provider));
  g_print ("is-local: right after the set %d, get_content is ours %d\n", gdk_clipboard_is_local (clipboard),
           gdk_clipboard_get_content (clipboard) == GDK_CONTENT_PROVIDER (provider));
  spin (1.0);
  g_print ("is-local: one second later %d, get_content is ours %d\n", gdk_clipboard_is_local (clipboard),
           gdk_clipboard_get_content (clipboard) == GDK_CONTENT_PROVIDER (provider));

  /* 2. The formats offered for our provider. */
  {
    g_autofree char *f = gdk_content_formats_to_string (gdk_clipboard_get_formats (clipboard));

    g_print ("formats of our PNG provider: %s\n", f);
  }

  /* 3. A read of our own content goes through our provider. */
  {
    GAsyncResult *r;
    const char *mime = NULL;
    g_autoptr (GError) error = NULL;
    g_autoptr (GInputStream) stream = NULL;
    int writes = provider->writes;

    gdk_clipboard_read_async (clipboard, (const char *[]) { "image/png", NULL }, G_PRIORITY_DEFAULT, NULL, on_done, NULL);
    r = wait_result (5.0);
    if (r != NULL)
      stream = gdk_clipboard_read_finish (clipboard, r, &mime, &error);
    g_print ("read of our content: %s, mime %s, our provider's writes %d -> %d%s%s\n", stream != NULL ? "a stream" : "none",
             mime ? mime : "(none)", writes, provider->writes, error ? ", error: " : "", error ? error->message : "");
  }

  /* 4. A texture's formats, as another app's picture would look. */
  {
    g_autoptr (GBytes) bytes = small_png ();
    g_autoptr (GdkTexture) texture = gdk_texture_new_from_bytes (bytes, NULL);
    g_autofree char *f = NULL;

    gdk_clipboard_set_texture (clipboard, texture);
    f = gdk_content_formats_to_string (gdk_clipboard_get_formats (clipboard));
    g_print ("formats of a texture: %s\n", f);
  }

  /* 5. A list of files, read back as GDK_TYPE_FILE_LIST. */
  {
    g_autoptr (GFile) file = g_file_new_for_path ("/tmp/probe-clipboard.png");
    GSList *list = g_slist_append (NULL, file);
    g_autofree char *f = NULL;
    GAsyncResult *r;
    const GValue *value = NULL;
    g_autoptr (GError) error = NULL;

    gdk_clipboard_set (clipboard, GDK_TYPE_FILE_LIST, list);
    g_slist_free (list);
    f = gdk_content_formats_to_string (gdk_clipboard_get_formats (clipboard));
    g_print ("formats of a file list: %s\n", f);
    gdk_clipboard_read_value_async (clipboard, GDK_TYPE_FILE_LIST, G_PRIORITY_DEFAULT, NULL, on_done, NULL);
    r = wait_result (5.0);
    if (r != NULL)
      value = gdk_clipboard_read_value_finish (clipboard, r, &error);
    if (value != NULL)
      {
        GSList *files = g_value_get_boxed (value);
        g_autofree char *path = files ? g_file_get_path (files->data) : NULL;

        g_print ("file list read back: %u file(s), the first %s\n", g_slist_length (files), path ? path : "(none)");
      }
    else
      g_print ("file list read back: none%s%s\n", error ? ", error: " : "", error ? error->message : "");
  }

  /* 6. The store with whatever clipboard manager the desktop has. */
  {
    GAsyncResult *r;
    g_autoptr (GError) error = NULL;
    gboolean stored = FALSE;

    gdk_clipboard_set_content (clipboard, GDK_CONTENT_PROVIDER (provider));
    start = g_get_monotonic_time ();
    gdk_clipboard_store_async (clipboard, G_PRIORITY_DEFAULT, NULL, on_done, NULL);
    r = wait_result (5.0);
    if (r != NULL)
      stored = gdk_clipboard_store_finish (clipboard, r, &error);
    g_print ("store: %s after %.2f s%s%s\n", r == NULL ? "no answer" : stored ? "kept" : "refused", (g_get_monotonic_time () - start) / 1e6,
             error ? ", error: " : "", error ? error->message : "");
  }

  g_bytes_unref (provider->png);
  gtk_window_destroy (GTK_WINDOW (window));
  return 0;
}
