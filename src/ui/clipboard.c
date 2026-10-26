/* SPDX-License-Identifier: GPL-3.0-or-later */

#include "clipboard.h"
#include "picture-file.h"

G_DEFINE_INTERFACE (EaselClipboard, easel_clipboard, G_TYPE_OBJECT)

static void
easel_clipboard_default_init (EaselClipboardInterface *iface)
{
  g_signal_new ("changed", G_TYPE_FROM_INTERFACE (iface), G_SIGNAL_RUN_LAST, 0, NULL, NULL, NULL, G_TYPE_NONE, 0);
}

GdkContentFormats *
easel_clipboard_get_formats (EaselClipboard *self)
{
  return EASEL_CLIPBOARD_GET_IFACE (self)->get_formats (self);
}

void
easel_clipboard_read_async (EaselClipboard *self, const char **mime_types, GCancellable *cancellable, GAsyncReadyCallback callback,
                            gpointer user_data)
{
  EASEL_CLIPBOARD_GET_IFACE (self)->read_async (self, mime_types, cancellable, callback, user_data);
}

GInputStream *
easel_clipboard_read_finish (EaselClipboard *self, GAsyncResult *result, const char **out_mime_type, GError **error)
{
  return EASEL_CLIPBOARD_GET_IFACE (self)->read_finish (self, result, out_mime_type, error);
}

void
easel_clipboard_read_files_async (EaselClipboard *self, GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
  EASEL_CLIPBOARD_GET_IFACE (self)->read_files_async (self, cancellable, callback, user_data);
}

GSList *
easel_clipboard_read_files_finish (EaselClipboard *self, GAsyncResult *result, GError **error)
{
  return EASEL_CLIPBOARD_GET_IFACE (self)->read_files_finish (self, result, error);
}

void
easel_clipboard_set_content (EaselClipboard *self, GdkContentProvider *provider)
{
  EASEL_CLIPBOARD_GET_IFACE (self)->set_content (self, provider);
}

gboolean
easel_clipboard_is_local (EaselClipboard *self)
{
  return EASEL_CLIPBOARD_GET_IFACE (self)->is_local (self);
}

GdkContentProvider *
easel_clipboard_get_content (EaselClipboard *self)
{
  return EASEL_CLIPBOARD_GET_IFACE (self)->get_content (self);
}

void
easel_clipboard_store_async (EaselClipboard *self, GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
  EASEL_CLIPBOARD_GET_IFACE (self)->store_async (self, cancellable, callback, user_data);
}

gboolean
easel_clipboard_store_finish (EaselClipboard *self, GAsyncResult *result, GError **error)
{
  return EASEL_CLIPBOARD_GET_IFACE (self)->store_finish (self, result, error);
}

/* ---- EaselGdkClipboard: the display's clipboard, each call forwarded ---- */

struct _EaselGdkClipboard
{
  GObject parent_instance;
  GdkClipboard *clipboard;
};

static void easel_gdk_clipboard_iface_init (EaselClipboardInterface *iface);

G_DEFINE_FINAL_TYPE_WITH_CODE (EaselGdkClipboard, easel_gdk_clipboard, G_TYPE_OBJECT,
                               G_IMPLEMENT_INTERFACE (EASEL_TYPE_CLIPBOARD, easel_gdk_clipboard_iface_init))

static GdkContentFormats *
gdk_get_formats (EaselClipboard *self)
{
  return gdk_clipboard_get_formats (EASEL_GDK_CLIPBOARD (self)->clipboard);
}

/* GDK's callbacks get the GdkClipboard as their source object; the caller expects this clipboard, so a task carries the result. */
static void
on_read_done (GObject *source, GAsyncResult *result, gpointer data)
{
  g_autoptr (GTask) task = data;
  const char *mime = NULL;
  GError *error = NULL;
  GInputStream *stream = gdk_clipboard_read_finish (GDK_CLIPBOARD (source), result, &mime, &error);

  if (stream == NULL)
    g_task_return_error (task, error);
  else
    {
      g_task_set_task_data (task, g_strdup (mime), g_free);
      g_task_return_pointer (task, stream, g_object_unref);
    }
}

static void
gdk_read_async (EaselClipboard *self, const char **mime_types, GCancellable *cancellable, GAsyncReadyCallback callback,
                gpointer user_data)
{
  GTask *task = g_task_new (self, cancellable, callback, user_data);

  gdk_clipboard_read_async (EASEL_GDK_CLIPBOARD (self)->clipboard, mime_types, G_PRIORITY_DEFAULT, cancellable, on_read_done, task);
}

static GInputStream *
gdk_read_finish (EaselClipboard *self, GAsyncResult *result, const char **out_mime_type, GError **error)
{
  GInputStream *stream = g_task_propagate_pointer (G_TASK (result), error);

  if (out_mime_type != NULL)
    *out_mime_type = stream != NULL ? g_task_get_task_data (G_TASK (result)) : NULL;
  return stream;
}

/* A copy of a list of files holds its own references. */
static gpointer
ref_object (gconstpointer object, gpointer unused)
{
  return g_object_ref ((gpointer) object);
}

static void
on_files_done (GObject *source, GAsyncResult *result, gpointer data)
{
  g_autoptr (GTask) task = data;
  GError *error = NULL;
  const GValue *value = gdk_clipboard_read_value_finish (GDK_CLIPBOARD (source), result, &error);

  if (value == NULL)
    g_task_return_error (task, error);
  else
    g_task_return_pointer (task, g_slist_copy_deep (g_value_get_boxed (value), ref_object, NULL), NULL);
}

static void
gdk_read_files_async (EaselClipboard *self, GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
  GTask *task = g_task_new (self, cancellable, callback, user_data);

  gdk_clipboard_read_value_async (EASEL_GDK_CLIPBOARD (self)->clipboard, GDK_TYPE_FILE_LIST, G_PRIORITY_DEFAULT, cancellable,
                                  on_files_done, task);
}

static GSList *
gdk_read_files_finish (EaselClipboard *self, GAsyncResult *result, GError **error)
{
  return g_task_propagate_pointer (G_TASK (result), error);
}

static void
gdk_set_content (EaselClipboard *self, GdkContentProvider *provider)
{
  gdk_clipboard_set_content (EASEL_GDK_CLIPBOARD (self)->clipboard, provider);
}

static gboolean
gdk_is_local (EaselClipboard *self)
{
  return gdk_clipboard_is_local (EASEL_GDK_CLIPBOARD (self)->clipboard);
}

static GdkContentProvider *
gdk_get_content (EaselClipboard *self)
{
  return gdk_clipboard_get_content (EASEL_GDK_CLIPBOARD (self)->clipboard);
}

static void
on_store_done (GObject *source, GAsyncResult *result, gpointer data)
{
  g_autoptr (GTask) task = data;
  GError *error = NULL;

  if (gdk_clipboard_store_finish (GDK_CLIPBOARD (source), result, &error))
    g_task_return_boolean (task, TRUE);
  else
    g_task_return_error (task, error);
}

static void
gdk_store_async (EaselClipboard *self, GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
  GTask *task = g_task_new (self, cancellable, callback, user_data);

  gdk_clipboard_store_async (EASEL_GDK_CLIPBOARD (self)->clipboard, G_PRIORITY_DEFAULT, cancellable, on_store_done, task);
}

static gboolean
gdk_store_finish (EaselClipboard *self, GAsyncResult *result, GError **error)
{
  return g_task_propagate_boolean (G_TASK (result), error);
}

static void
easel_gdk_clipboard_iface_init (EaselClipboardInterface *iface)
{
  iface->get_formats = gdk_get_formats;
  iface->read_async = gdk_read_async;
  iface->read_finish = gdk_read_finish;
  iface->read_files_async = gdk_read_files_async;
  iface->read_files_finish = gdk_read_files_finish;
  iface->set_content = gdk_set_content;
  iface->is_local = gdk_is_local;
  iface->get_content = gdk_get_content;
  iface->store_async = gdk_store_async;
  iface->store_finish = gdk_store_finish;
}

static void
easel_gdk_clipboard_dispose (GObject *object)
{
  EaselGdkClipboard *self = EASEL_GDK_CLIPBOARD (object);

  if (self->clipboard != NULL)
    g_signal_handlers_disconnect_by_data (self->clipboard, self);
  g_clear_object (&self->clipboard);
  G_OBJECT_CLASS (easel_gdk_clipboard_parent_class)->dispose (object);
}

static void
easel_gdk_clipboard_class_init (EaselGdkClipboardClass *klass)
{
  G_OBJECT_CLASS (klass)->dispose = easel_gdk_clipboard_dispose;
}

static void
easel_gdk_clipboard_init (EaselGdkClipboard *self)
{
}

static void
on_gdk_changed (GdkClipboard *clipboard, EaselGdkClipboard *self)
{
  g_signal_emit_by_name (self, "changed");
}

EaselClipboard *
easel_gdk_clipboard_new (GdkClipboard *clipboard)
{
  EaselGdkClipboard *self = g_object_new (EASEL_TYPE_GDK_CLIPBOARD, NULL);

  self->clipboard = g_object_ref (clipboard);
  g_signal_connect (clipboard, "changed", G_CALLBACK (on_gdk_changed), self);
  return EASEL_CLIPBOARD (self);
}

/* ---- EaselClipboardPicture: the window's copy on the clipboard ---- */

struct _EaselClipboardPicture
{
  GdkContentProvider parent_instance;
  EaselImage *copy;
};

G_DEFINE_FINAL_TYPE (EaselClipboardPicture, easel_clipboard_picture, GDK_TYPE_CONTENT_PROVIDER)

static GdkContentFormats *
picture_ref_formats (GdkContentProvider *provider)
{
  return gdk_content_formats_new ((const char *[]) { "image/png" }, 1);
}

/* The PNG, made on a worker thread from the copy, which nothing changes while the provider lives (the task holds the provider). */
static void
encode_in_thread (GTask *task, gpointer source, gpointer data, GCancellable *cancellable)
{
  GError *error = NULL;
  GBytes *png = easel_picture_encode_fast_png (EASEL_CLIPBOARD_PICTURE (source)->copy, &error);

  if (png == NULL)
    g_task_return_error (task, error);
  else
    g_task_return_pointer (task, png, (GDestroyNotify) g_bytes_unref);
}

typedef struct
{
  GTask *task;            /* the write's, given back to GDK */
  GOutputStream *stream;
} Write;

/* All of the PNG has been written, or the stream failed. (g_output_stream_write_bytes_async writes only what the pipe takes at once, 64 KiB:
   another app got a PNG cut off there and reported it as no picture, M1 of 0016, 2026-10-25.) */
static void
on_png_written (GObject *stream, GAsyncResult *result, gpointer data)
{
  GTask *task = data;
  GError *error = NULL;
  gsize written = 0;

  if (!g_output_stream_write_all_finish (G_OUTPUT_STREAM (stream), result, &written, &error))
    g_task_return_error (task, error);
  else
    g_task_return_boolean (task, TRUE);
  g_object_unref (task);
}

static void
on_encoded (GObject *source, GAsyncResult *result, gpointer data)
{
  Write *w = data;
  GError *error = NULL;
  GBytes *png = g_task_propagate_pointer (G_TASK (result), &error);

  if (png == NULL)
    {
      g_task_return_error (w->task, error);
      g_object_unref (w->task);
    }
  else
    {
      /* Written without blocking, all of it: the reader of a local read is this main loop (probe-clipboard), and a pipe takes 64 KiB at a
         time. The bytes live as long as the task. */
      g_task_set_task_data (w->task, png, (GDestroyNotify) g_bytes_unref);
      g_output_stream_write_all_async (w->stream, g_bytes_get_data (png, NULL), g_bytes_get_size (png), G_PRIORITY_DEFAULT,
                                       g_task_get_cancellable (w->task), on_png_written, w->task);
    }
  g_object_unref (w->stream);
  g_free (w);
}

static void
picture_write_async (GdkContentProvider *provider, const char *mime_type, GOutputStream *stream, int io_priority,
                     GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
  GTask *task = g_task_new (provider, cancellable, callback, user_data);
  g_autoptr (GTask) encode = NULL;
  Write *w;

  if (g_strcmp0 (mime_type, "image/png") != 0)
    {
      g_task_return_new_error (task, G_IO_ERROR, G_IO_ERROR_NOT_SUPPORTED, "Cannot provide contents as %s", mime_type);
      g_object_unref (task);
      return;
    }
  w = g_new0 (Write, 1);
  w->task = task;
  w->stream = g_object_ref (stream);
  encode = g_task_new (provider, cancellable, on_encoded, w);
  g_task_run_in_thread (encode, encode_in_thread);
}

static gboolean
picture_write_finish (GdkContentProvider *provider, GAsyncResult *result, GError **error)
{
  return g_task_propagate_boolean (G_TASK (result), error);
}

static void
easel_clipboard_picture_finalize (GObject *object)
{
  easel_image_free (EASEL_CLIPBOARD_PICTURE (object)->copy);
  G_OBJECT_CLASS (easel_clipboard_picture_parent_class)->finalize (object);
}

static void
easel_clipboard_picture_class_init (EaselClipboardPictureClass *klass)
{
  GdkContentProviderClass *provider = GDK_CONTENT_PROVIDER_CLASS (klass);

  G_OBJECT_CLASS (klass)->finalize = easel_clipboard_picture_finalize;
  provider->ref_formats = picture_ref_formats;
  provider->write_mime_type_async = picture_write_async;
  provider->write_mime_type_finish = picture_write_finish;
}

static void
easel_clipboard_picture_init (EaselClipboardPicture *self)
{
}

GdkContentProvider *
easel_clipboard_picture_new (EaselImage *copy)
{
  EaselClipboardPicture *self = g_object_new (EASEL_TYPE_CLIPBOARD_PICTURE, NULL);

  self->copy = copy;
  return GDK_CONTENT_PROVIDER (self);
}

const EaselImage *
easel_clipboard_picture_get_image (EaselClipboardPicture *self)
{
  return self->copy;
}
