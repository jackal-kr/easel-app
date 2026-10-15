/* SPDX-License-Identifier: GPL-3.0-or-later */
/* The Attributes dialog: the picture's width and height in pixels (REQ-0004-21). */
#pragma once

#include <adwaita.h>

G_BEGIN_DECLS

/* Called when the user chooses OK with a valid size (each 1 to 10000). */
typedef void (*EaselAttributesApplied) (int width, int height, gpointer data);

/* A modal dialog "Attributes" over parent, starting with width x height. OK stays insensitive while a field is not a
   whole number from 1 to 10000; Default puts default_width x default_height in the fields; Enter in a field is OK when
   both are valid; Esc and Cancel change nothing. */
void easel_attributes_dialog_present (GtkWidget *parent, int width, int height, int default_width, int default_height,
                                      EaselAttributesApplied applied, gpointer data);

G_END_DECLS
