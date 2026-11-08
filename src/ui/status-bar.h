/* SPDX-License-Identifier: GPL-3.0-or-later */
/* The status bar below the palette bar: pointer position, picture size, zoom and a hint for the active tool
   (REQ-0004-09..11). A field is written only when its text changes. */
#pragma once

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define EASEL_TYPE_STATUS_BAR (easel_status_bar_get_type ())
G_DECLARE_FINAL_TYPE (EaselStatusBar, easel_status_bar, EASEL, STATUS_BAR, GtkBox)

typedef enum
{
  EASEL_STATUS_POSITION,
  EASEL_STATUS_SIZE,
  EASEL_STATUS_ZOOM,
  EASEL_STATUS_HINT,
  EASEL_STATUS_SELECTION,
  EASEL_STATUS_FRAME_TIME,
  EASEL_STATUS_PRESET,
  EASEL_STATUS_N_FIELDS
} EaselStatusField;

/* inside FALSE (or exists FALSE) empties the field. */
void easel_status_bar_set_position (EaselStatusBar *self, gboolean inside, int x, int y);
void easel_status_bar_set_size (EaselStatusBar *self, gboolean exists, int width, int height);
void easel_status_bar_set_zoom (EaselStatusBar *self, gboolean exists, int percent);
void easel_status_bar_set_hint (EaselStatusBar *self, const char *hint);
/* The selection's size "W × H" just before the picture size, or nothing (and no room taken) without one (REQ-0013-06). */
void easel_status_bar_set_selection (EaselStatusBar *self, gboolean exists, int width, int height);
/* The Frame time part (REQ-0021-12), left of the position; hidden, it takes no room. NULL text keeps what it shows. */
void easel_status_bar_show_frame_time (EaselStatusBar *self, gboolean shown);
void easel_status_bar_set_frame_time (EaselStatusBar *self, const char *text);
/* The selected size preset (REQ-0030-08), as wide as the longest of names (NULL-terminated); NULL text hides it. */
void easel_status_bar_set_preset (EaselStatusBar *self, const char *text, const char *const *names);

/* Test hooks: a field's text, how many times its label has been written, and the label (for its ellipsis). */
const char *easel_status_bar_get_text (EaselStatusBar *self, EaselStatusField field);
guint easel_status_bar_get_writes (EaselStatusBar *self, EaselStatusField field);
GtkLabel *easel_status_bar_get_label (EaselStatusBar *self, EaselStatusField field);

G_END_DECLS
