/* SPDX-License-Identifier: GPL-3.0-or-later */
#pragma once

#include <adwaita.h>

#include "canvas.h"
#include "palette-bar.h"
#include "status-bar.h"
#include "image.h"
#include "tool.h"

G_BEGIN_DECLS

#define EASEL_TYPE_WINDOW (easel_window_get_type ())
G_DECLARE_FINAL_TYPE (EaselWindow, easel_window, EASEL, WINDOW, AdwApplicationWindow)

EaselWindow *easel_window_new (GtkApplication *app);

/* NULL until the start-up picture exists (it may wait for the window's monitor, REQ-0001-02). */
const EaselImage *easel_window_get_picture (EaselWindow *self);
int easel_window_get_zoom (EaselWindow *self);
EaselTool easel_window_get_tool (EaselWindow *self);
uint32_t easel_window_get_foreground (EaselWindow *self);
uint32_t easel_window_get_background (EaselWindow *self);
gboolean easel_window_get_modified (EaselWindow *self);
EaselCanvas *easel_window_get_canvas (EaselWindow *self);
EaselPaletteBar *easel_window_get_palette_bar (EaselWindow *self);
EaselStatusBar *easel_window_get_status_bar (EaselWindow *self);

GtkMenuButton *easel_window_get_image_button (EaselWindow *self);
GtkMenuButton *easel_window_get_view_button (EaselWindow *self);
/* The zoom as the View menu's row shows it, for example "100%" (REQ-0004-01). */
const char *easel_window_get_zoom_text (EaselWindow *self);

/* The color chooser is open (REQ-0003-14). */
gboolean easel_window_is_choosing_color (EaselWindow *self);

/* Ends the open color choice: rgba is the confirmed color, NULL means Cancel. The chooser's callback calls it; tests
   call it in place of GTK's dialog. */
void easel_window_finish_color_choice (EaselWindow *self, const GdkRGBA *rgba);

/* Saves the picture as PNG at path without asking; on success it becomes the picture's file.
   On failure shows an error and returns FALSE (REQ-0001-30, REQ-0001-31). */
gboolean easel_window_save_to (EaselWindow *self, const char *path);

G_END_DECLS
