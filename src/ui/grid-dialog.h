/* SPDX-License-Identifier: GPL-3.0-or-later */
/* The Grid dialog: the grid's cell size and its axonometric style (REQ-0004-13). */
#pragma once

#include <adwaita.h>

G_BEGIN_DECLS

/* Called when the user applies: the new cell size (1 to 1000) and whether the grid is axonometric. */
typedef void (*EaselGridDialogApplied) (int cell, gboolean axonometric, gpointer data);

/* A modal dialog "Grid" over parent, starting with the given values. Apply stays insensitive while the cell size is not
   a whole number from 1 to 1000; Enter in the field applies a valid value; Esc and Cancel change nothing. */
void easel_grid_dialog_present (GtkWidget *parent, int cell, gboolean axonometric, EaselGridDialogApplied applied,
                                gpointer data);

G_END_DECLS
