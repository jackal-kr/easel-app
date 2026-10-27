/* SPDX-License-Identifier: GPL-3.0-or-later */

#include "memory-clipboard.h"

#include <fcntl.h>
#include <gio/gunixinputstream.h>
#include <gio/gunixoutputstream.h>
#include <glib-unix.h>

struct _EaselMemoryClipboard
{
  GObject parent_instance;
  GdkContentProvider *content;   /* NULL: empty */
  gboolean local;                /* set by the window */
  gboolean never_answers;
  gboolean late_cancel;          /* a cancelled read or store ends from the main loop, after the reader's cancel has returned */
  guint answer_delay_ms;         /* a read answers this late (0: at once) */
  gboolean store_hangs;          /* the store never answers, until its caller cancels it */
  guint store_requests;
  GSList *hung;                  /* the tasks of reads that never answer, until their reader cancels them */
};

static void easel_memory_clipboard_iface_init (EaselClipboardInterface *iface);

G_DEFINE_FINAL_TYPE_WITH_CODE (EaselMemoryClipboard, easel_memory_clipboard, G_TYPE_OBJECT,
                               G_IMPLEMENT_INTERFACE (EASEL_TYPE_CLIPBOARD, easel_memory_clipboard_iface_init))

/* A read that never answers (an owner that hangs): its task waits until the reader cancels it, as GDK's read does, and then ends with
   the cancellation; a task nobody cancels ends when the clipboard is disposed. */
static gboolean
return_cancelled (gpointer data)
{
  GTask *task = data;

  g_task_return_new_error (task, G_IO_ERROR, G_IO_ERROR_CANCELLED, "Operation was cancelled");
  g_object_unref (task);
  return G_SOURCE_REMOVE;
}

/* With late_cancel the cancellation is answered from the main loop afterwards, as a cancelled splice from a pipe is (check F1 of 0016,
   2026-10-26: its callback came 5 main-loop iterations after the cancel), so the reader has gone on by then. */
static void
end_hung_task (GCancellable *cancellable, gpointer data)
{
  GTask *task = data;
  EaselMemoryClipboard *self = g_task_get_source_object (task);

  if (g_slist_find (self->hung, task) == NULL)
    return;
  self->hung = g_slist_remove (self->hung, task);
  if (self->late_cancel)
    g_idle_add (return_cancelled, task);
  else
    return_cancelled (task);
}

static void
hang (EaselMemoryClipboard *self, GTask *task, GCancellable *cancellable)
{
  self->hung = g_slist_prepend (self->hung, task);
  if (cancellable != NULL)
    g_cancellable_connect (cancellable, G_CALLBACK (end_hung_task), task, NULL);
}

static void
set (EaselMemoryClipboard *self, GdkContentProvider *provider, gboolean local)
{
  g_set_object (&self->content, provider);
  self->local = local && provider != NULL;
  g_signal_emit_by_name (self, "changed");
}

/* What the content offers, with the MIME types its values serialize to and the types those deserialize to, as GdkClipboard reports a
   content's formats (probe-clipboard, 2026-10-22). */
static GdkContentFormats *
mem_get_formats (EaselClipboard *clipboard)
{
  EaselMemoryClipboard *self = EASEL_MEMORY_CLIPBOARD (clipboard);
  GdkContentFormats *formats;

  if (self->content == NULL)
    formats = gdk_content_formats_new (NULL, 0);
  else
    formats = gdk_content_formats_union_deserialize_gtypes (
      gdk_content_formats_union_serialize_mime_types (gdk_content_provider_ref_formats (self->content)));
  g_object_set_data_full (G_OBJECT (self), "formats", formats, (GDestroyNotify) gdk_content_formats_unref);
  return formats;
}

/* The provider has written all: the write end is closed, so the reader sees the end of the stream. */
static void
on_written (GObject *source, GAsyncResult *result, gpointer data)
{
  g_autoptr (GOutputStream) out = data;

  gdk_content_provider_write_mime_type_finish (GDK_CONTENT_PROVIDER (source), result, NULL);
  g_output_stream_close (out, NULL, NULL);
}

typedef struct
{
  GdkContentProvider *content;
  const char *mime;
  GOutputStream *out;
} Writing;

static void
on_serialized (GObject *source, GAsyncResult *result, gpointer data)
{
  g_autoptr (GOutputStream) out = data;

  gdk_content_serialize_finish (result, NULL);
  g_output_stream_close (out, NULL, NULL);
}

/* As GdkClipboard writes a local content: a MIME type the provider offers itself, written by it; one that a value it holds serializes
   to (a G_TYPE_STRING to text/plain;charset=utf-8, say), serialized from that value by GDK's serializers (0017 T017: the typed text of
   a provider read empty without this). */
static void
start_writing (gpointer data)
{
  Writing *w = data;
  g_autoptr (GdkContentFormats) own = gdk_content_provider_ref_formats (w->content);
  gsize n = 0;
  const GType *gtypes = gdk_content_formats_get_gtypes (own, &n);
  gboolean done = FALSE;

  if (gdk_content_formats_contain_mime_type (own, w->mime))
    {
      gdk_content_provider_write_mime_type_async (w->content, w->mime, w->out, G_PRIORITY_DEFAULT, NULL, on_written, w->out);
      done = TRUE;
    }
  for (gsize i = 0; i < n && !done; i++)
    {
      g_autoptr (GdkContentFormats) one = gdk_content_formats_new_for_gtype (gtypes[i]);
      g_autoptr (GdkContentFormats) to = gdk_content_formats_union_serialize_mime_types (gdk_content_formats_ref (one));
      GValue value = G_VALUE_INIT;

      if (!gdk_content_formats_contain_mime_type (to, w->mime))
        continue;
      g_value_init (&value, gtypes[i]);
      if (gdk_content_provider_get_value (w->content, &value, NULL))
        {
          gdk_content_serialize_async (w->out, w->mime, &value, G_PRIORITY_DEFAULT, NULL, on_serialized, w->out);
          done = TRUE;
        }
      g_value_unset (&value);
    }
  if (!done)
    {
      g_output_stream_close (w->out, NULL, NULL);
      g_object_unref (w->out);
    }
  g_object_unref (w->content);
  g_free (w);
}

static void answer_read (EaselMemoryClipboard *self, GTask *task, const char *chosen);

/* A read that answers late (a slow owner that answers in the end). */
static gboolean
answer_late (gpointer data)
{
  GTask *task = data;

  answer_read (g_task_get_source_object (task), task, g_object_get_data (G_OBJECT (task), "mime"));
  return G_SOURCE_REMOVE;
}

/* As GDK's read of a local content: the reader gets the read end of a pipe at once, and the provider fills the write end from the main
   loop, so a reader that blocks on the stream before the loop runs waits for ever, as it would on the desktop's clipboard
   (perf-0016, 2026-10-23: the window's first read did, and an in-memory stream here had hidden it). */
static void
mem_read_async (EaselClipboard *clipboard, const char **mime_types, GCancellable *cancellable, GAsyncReadyCallback callback,
                gpointer user_data)
{
  EaselMemoryClipboard *self = EASEL_MEMORY_CLIPBOARD (clipboard);
  GTask *task = g_task_new (self, cancellable, callback, user_data);
  GdkContentFormats *formats = mem_get_formats (clipboard);
  const char *chosen = NULL;

  for (int i = 0; mime_types[i] != NULL && chosen == NULL; i++)
    if (gdk_content_formats_contain_mime_type (formats, mime_types[i]))
      chosen = g_intern_string (mime_types[i]);
  if (self->never_answers)
    {
      hang (self, task, cancellable);
      return;
    }
  if (self->answer_delay_ms > 0 && chosen != NULL)
    {
      g_object_set_data (G_OBJECT (task), "mime", (gpointer) chosen);
      g_timeout_add (self->answer_delay_ms, answer_late, task);
      return;
    }
  answer_read (self, task, chosen);
}

/* The read answered: the read end of a pipe the provider fills from the main loop. Takes task. */
static void
answer_read (EaselMemoryClipboard *self, GTask *task, const char *chosen)
{
  GError *error = NULL;
  int fds[2];

  if (chosen == NULL || self->content == NULL)
    {
      g_task_return_new_error (task, G_IO_ERROR, G_IO_ERROR_NOT_SUPPORTED, "No compatible formats to transfer clipboard contents.");
      g_object_unref (task);
      return;
    }
  if (!g_unix_open_pipe (fds, O_CLOEXEC, &error) || !g_unix_set_fd_nonblocking (fds[1], TRUE, &error))
    {
      g_task_return_error (task, error);
      g_object_unref (task);
      return;
    }
  g_object_set_data (G_OBJECT (task), "mime", (gpointer) chosen);
  {
    Writing *w = g_new0 (Writing, 1);

    w->content = g_object_ref (self->content);
    w->mime = chosen;
    w->out = g_unix_output_stream_new (fds[1], TRUE);
    g_idle_add_once (start_writing, w);         /* the writer runs only when the main loop does, as GDK's in-process pipe */
  }
  g_task_return_pointer (task, g_unix_input_stream_new (fds[0], TRUE), g_object_unref);
  g_object_unref (task);
}

static GInputStream *
mem_read_finish (EaselClipboard *clipboard, GAsyncResult *result, const char **out_mime_type, GError **error)
{
  GInputStream *stream = g_task_propagate_pointer (G_TASK (result), error);

  if (out_mime_type != NULL)
    *out_mime_type = stream != NULL ? g_object_get_data (G_OBJECT (result), "mime") : NULL;
  return stream;
}

/* A copy of a list of files holds its own references. */
static gpointer
ref_object (gconstpointer object, gpointer unused)
{
  return g_object_ref ((gpointer) object);
}

static void
mem_read_files_async (EaselClipboard *clipboard, GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
  EaselMemoryClipboard *self = EASEL_MEMORY_CLIPBOARD (clipboard);
  GTask *task = g_task_new (self, cancellable, callback, user_data);
  GValue value = G_VALUE_INIT;
  GError *error = NULL;

  if (self->never_answers)
    {
      hang (self, task, cancellable);
      return;
    }
  g_value_init (&value, GDK_TYPE_FILE_LIST);
  if (self->content == NULL || !gdk_content_provider_get_value (self->content, &value, &error))
    {
      if (error == NULL)
        error = g_error_new_literal (G_IO_ERROR, G_IO_ERROR_NOT_SUPPORTED, "No compatible formats to transfer clipboard contents.");
      g_task_return_error (task, error);
    }
  else
    g_task_return_pointer (task, g_slist_copy_deep (g_value_get_boxed (&value), ref_object, NULL), NULL);
  g_value_unset (&value);
  g_object_unref (task);
}

static GSList *
mem_read_files_finish (EaselClipboard *clipboard, GAsyncResult *result, GError **error)
{
  return g_task_propagate_pointer (G_TASK (result), error);
}

static void
mem_set_content (EaselClipboard *clipboard, GdkContentProvider *provider)
{
  set (EASEL_MEMORY_CLIPBOARD (clipboard), provider, TRUE);
}

static gboolean
mem_is_local (EaselClipboard *clipboard)
{
  return EASEL_MEMORY_CLIPBOARD (clipboard)->local;
}

static GdkContentProvider *
mem_get_content (EaselClipboard *clipboard)
{
  EaselMemoryClipboard *self = EASEL_MEMORY_CLIPBOARD (clipboard);

  return self->local ? self->content : NULL;
}

/* As with no clipboard manager: the store fails at once. */
static void
mem_store_async (EaselClipboard *clipboard, GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
  EaselMemoryClipboard *self = EASEL_MEMORY_CLIPBOARD (clipboard);
  g_autoptr (GTask) task = g_task_new (self, cancellable, callback, user_data);

  self->store_requests++;
  if (self->store_hangs)
    {
      hang (self, g_steal_pointer (&task), cancellable);
      return;
    }
  g_task_return_new_error (task, G_IO_ERROR, G_IO_ERROR_NOT_SUPPORTED, "No clipboard manager is active.");
}

static gboolean
mem_store_finish (EaselClipboard *clipboard, GAsyncResult *result, GError **error)
{
  return g_task_propagate_boolean (G_TASK (result), error);
}

static void
easel_memory_clipboard_iface_init (EaselClipboardInterface *iface)
{
  iface->get_formats = mem_get_formats;
  iface->read_async = mem_read_async;
  iface->read_finish = mem_read_finish;
  iface->read_files_async = mem_read_files_async;
  iface->read_files_finish = mem_read_files_finish;
  iface->set_content = mem_set_content;
  iface->is_local = mem_is_local;
  iface->get_content = mem_get_content;
  iface->store_async = mem_store_async;
  iface->store_finish = mem_store_finish;
}

static void
easel_memory_clipboard_dispose (GObject *object)
{
  EaselMemoryClipboard *self = EASEL_MEMORY_CLIPBOARD (object);

  for (GSList *l = self->hung; l != NULL; l = l->next)
    {
      g_task_return_new_error (l->data, G_IO_ERROR, G_IO_ERROR_CANCELLED, "The clipboard went away");
      g_object_unref (l->data);
    }
  g_clear_pointer (&self->hung, g_slist_free);
  g_clear_object (&self->content);
  G_OBJECT_CLASS (easel_memory_clipboard_parent_class)->dispose (object);
}

static void
easel_memory_clipboard_class_init (EaselMemoryClipboardClass *klass)
{
  G_OBJECT_CLASS (klass)->dispose = easel_memory_clipboard_dispose;
}

static void
easel_memory_clipboard_init (EaselMemoryClipboard *self)
{
}

EaselMemoryClipboard *
easel_memory_clipboard_new (void)
{
  return g_object_new (EASEL_TYPE_MEMORY_CLIPBOARD, NULL);
}

void
easel_memory_clipboard_set_other (EaselMemoryClipboard *self, GdkContentProvider *provider)
{
  set (self, provider, FALSE);
}

void
easel_memory_clipboard_set_other_text (EaselMemoryClipboard *self, const char *text)
{
  g_autoptr (GdkContentProvider) provider = gdk_content_provider_new_typed (G_TYPE_STRING, text);

  set (self, provider, FALSE);
}

void
easel_memory_clipboard_set_other_bytes (EaselMemoryClipboard *self, const char *mime_type, GBytes *bytes)
{
  g_autoptr (GdkContentProvider) provider = gdk_content_provider_new_for_bytes (mime_type, bytes);

  set (self, provider, FALSE);
}

void
easel_memory_clipboard_set_other_files (EaselMemoryClipboard *self, const char *const *paths)
{
  GSList *list = NULL;
  g_autoptr (GdkContentProvider) provider = NULL;

  for (int i = 0; paths[i] != NULL; i++)
    list = g_slist_append (list, g_file_new_for_path (paths[i]));
  provider = gdk_content_provider_new_typed (GDK_TYPE_FILE_LIST, list);
  g_slist_free_full (list, g_object_unref);
  set (self, provider, FALSE);
}

void
easel_memory_clipboard_set_never_answers (EaselMemoryClipboard *self, gboolean never)
{
  self->never_answers = never;
}

void
easel_memory_clipboard_set_late_cancel (EaselMemoryClipboard *self, gboolean late)
{
  self->late_cancel = late;
}

void
easel_memory_clipboard_set_answer_delay (EaselMemoryClipboard *self, guint ms)
{
  self->answer_delay_ms = ms;
}

void
easel_memory_clipboard_set_store_hangs (EaselMemoryClipboard *self, gboolean hangs)
{
  self->store_hangs = hangs;
}

guint
easel_memory_clipboard_get_store_requests (EaselMemoryClipboard *self)
{
  return self->store_requests;
}
