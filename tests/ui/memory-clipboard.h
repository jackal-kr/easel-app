/* SPDX-License-Identifier: GPL-3.0-or-later */
/* The ui tests' clipboard (feature 0016, Plan, "The clipboard seam"): an EaselClipboard held in memory, set by the window as the desktop's
   is, or by the test as another app would set the desktop's. A read goes through the content provider, as GDK's read of a local
   content does: its MIME types are the provider's own and those its values serialize to. */
#pragma once

#include "clipboard.h"

G_BEGIN_DECLS

#define EASEL_TYPE_MEMORY_CLIPBOARD (easel_memory_clipboard_get_type ())
G_DECLARE_FINAL_TYPE (EaselMemoryClipboard, easel_memory_clipboard, EASEL, MEMORY_CLIPBOARD, GObject)

EaselMemoryClipboard *easel_memory_clipboard_new (void);

/* Another app's content (not local): text, a picture as the bytes of mime_type, a list of files (paths), or any provider. */
void easel_memory_clipboard_set_other (EaselMemoryClipboard *self, GdkContentProvider *provider);
void easel_memory_clipboard_set_other_text (EaselMemoryClipboard *self, const char *text);
void easel_memory_clipboard_set_other_bytes (EaselMemoryClipboard *self, const char *mime_type, GBytes *bytes);
void easel_memory_clipboard_set_other_files (EaselMemoryClipboard *self, const char *const *paths);

/* While TRUE, a read never answers (an owner that hangs). */
void easel_memory_clipboard_set_never_answers (EaselMemoryClipboard *self, gboolean never);

/* While TRUE, a cancelled read or store is answered from the main loop afterwards, as GIO answers a cancelled splice from a pipe (the
   reader has returned by then); otherwise from inside the cancel. */
void easel_memory_clipboard_set_late_cancel (EaselMemoryClipboard *self, gboolean late);
/* A read answers after ms milliseconds (0: at once), as a slow owner that answers in the end. */
void easel_memory_clipboard_set_answer_delay (EaselMemoryClipboard *self, guint ms);
/* While TRUE, the store never answers until its caller cancels it (a clipboard manager that hangs). */
void easel_memory_clipboard_set_store_hangs (EaselMemoryClipboard *self, gboolean hangs);

/* How many times the store was asked for, and whether it fails at once (TRUE: as with no clipboard manager). */
guint easel_memory_clipboard_get_store_requests (EaselMemoryClipboard *self);

G_END_DECLS
