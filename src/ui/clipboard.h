/* SPDX-License-Identifier: GPL-3.0-or-later */
/* The clipboard as the window sees it (feature 0016, Plan, "The clipboard seam"): an interface that mirrors GdkClipboard's, so that the
   ui tests can give the window a clipboard of their own (the isolated session holds none). In the app it is EaselGdkClipboard, which
   forwards every call to the display's GdkClipboard and has no logic of its own. */
#pragma once

#include <gtk/gtk.h>

#include "image.h"

G_BEGIN_DECLS

#define EASEL_TYPE_CLIPBOARD (easel_clipboard_get_type ())
G_DECLARE_INTERFACE (EaselClipboard, easel_clipboard, EASEL, CLIPBOARD, GObject)

struct _EaselClipboardInterface
{
  GTypeInterface parent_iface;

  /* What the content offers (gdk_clipboard_get_formats). */
  GdkContentFormats *(*get_formats) (EaselClipboard *self);
  /* A stream of the content in the first of mime_types it offers (gdk_clipboard_read_async, read_finish). */
  void (*read_async) (EaselClipboard *self, const char **mime_types, GCancellable *cancellable, GAsyncReadyCallback callback,
                      gpointer user_data);
  GInputStream *(*read_finish) (EaselClipboard *self, GAsyncResult *result, const char **out_mime_type, GError **error);
  /* The copied files, a list of GFile (gdk_clipboard_read_value_async with GDK_TYPE_FILE_LIST). */
  void (*read_files_async) (EaselClipboard *self, GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data);
  GSList *(*read_files_finish) (EaselClipboard *self, GAsyncResult *result, GError **error);
  /* The window's own content (gdk_clipboard_set_content, is_local, get_content). */
  void (*set_content) (EaselClipboard *self, GdkContentProvider *provider);
  gboolean (*is_local) (EaselClipboard *self);
  GdkContentProvider *(*get_content) (EaselClipboard *self);
  /* Ask the desktop's clipboard manager to keep the content (gdk_clipboard_store_async, store_finish). */
  void (*store_async) (EaselClipboard *self, GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data);
  gboolean (*store_finish) (EaselClipboard *self, GAsyncResult *result, GError **error);
};

/* Each call is its vfunc; the interface also has the signal "changed" (no arguments), as GdkClipboard's. */
GdkContentFormats *easel_clipboard_get_formats (EaselClipboard *self);
void easel_clipboard_read_async (EaselClipboard *self, const char **mime_types, GCancellable *cancellable, GAsyncReadyCallback callback,
                                 gpointer user_data);
GInputStream *easel_clipboard_read_finish (EaselClipboard *self, GAsyncResult *result, const char **out_mime_type, GError **error);
void easel_clipboard_read_files_async (EaselClipboard *self, GCancellable *cancellable, GAsyncReadyCallback callback,
                                       gpointer user_data);
GSList *easel_clipboard_read_files_finish (EaselClipboard *self, GAsyncResult *result, GError **error);
void easel_clipboard_set_content (EaselClipboard *self, GdkContentProvider *provider);
gboolean easel_clipboard_is_local (EaselClipboard *self);
GdkContentProvider *easel_clipboard_get_content (EaselClipboard *self);
void easel_clipboard_store_async (EaselClipboard *self, GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data);
gboolean easel_clipboard_store_finish (EaselClipboard *self, GAsyncResult *result, GError **error);

#define EASEL_TYPE_GDK_CLIPBOARD (easel_gdk_clipboard_get_type ())
G_DECLARE_FINAL_TYPE (EaselGdkClipboard, easel_gdk_clipboard, EASEL, GDK_CLIPBOARD, GObject)

/* The app's clipboard: the display's GdkClipboard, every call forwarded. */
EaselClipboard *easel_gdk_clipboard_new (GdkClipboard *clipboard);

#define EASEL_TYPE_CLIPBOARD_PICTURE (easel_clipboard_picture_get_type ())
G_DECLARE_FINAL_TYPE (EaselClipboardPicture, easel_clipboard_picture, EASEL, CLIPBOARD_PICTURE, GdkContentProvider)

/* The window's content on the clipboard (REQ-0016-03): the copy of a selection, taken (the snapshot at Copy). It offers image/png,
   encoded only when a reader asks for it, on a worker thread (Plan, "Copy"), and written without blocking. */
GdkContentProvider *easel_clipboard_picture_new (EaselImage *copy);
/* The copy it holds (Easel's own paste takes it, with no PNG). */
const EaselImage *easel_clipboard_picture_get_image (EaselClipboardPicture *self);

G_END_DECLS
