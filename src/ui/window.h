/* SPDX-License-Identifier: GPL-3.0-or-later */
#pragma once

#include <adwaita.h>

#include "canvas.h"
#include "palette-bar.h"
#include "status-bar.h"
#include "tool-options.h"
#include "image.h"
#include "tool.h"

G_BEGIN_DECLS

#define EASEL_TYPE_WINDOW (easel_window_get_type ())
G_DECLARE_FINAL_TYPE (EaselWindow, easel_window, EASEL, WINDOW, AdwApplicationWindow)

EaselWindow *easel_window_new (GtkApplication *app);

/* A window whose start-up picture is the file (REQ-0005-15). If the file cannot be opened, the window starts with the
   blank picture and tells why, as Open does. */
EaselWindow *easel_window_new_with_file (GtkApplication *app, GFile *file);

/* NULL until the start-up picture exists (it may wait for the window's monitor, REQ-0001-02). */
const EaselImage *easel_window_get_picture (EaselWindow *self);
int easel_window_get_zoom (EaselWindow *self);
EaselTool easel_window_get_tool (EaselWindow *self);
uint32_t easel_window_get_foreground (EaselWindow *self);
uint32_t easel_window_get_background (EaselWindow *self);
gboolean easel_window_get_modified (EaselWindow *self);
EaselCanvas *easel_window_get_canvas (EaselWindow *self);
EaselPaletteBar *easel_window_get_palette_bar (EaselWindow *self);
EaselToolOptions *easel_window_get_tool_options (EaselWindow *self);
EaselStatusBar *easel_window_get_status_bar (EaselWindow *self);

GtkMenuButton *easel_window_get_main_menu_button (EaselWindow *self);
GtkMenuButton *easel_window_get_image_button (EaselWindow *self);
GtkMenuButton *easel_window_get_view_button (EaselWindow *self);
/* The selection's context menu (REQ-0013-17), for tests: shown while it is open. */
GtkPopoverMenu *easel_window_get_selection_menu (EaselWindow *self);
/* The zoom as the View menu's row shows it, for example "100%" (REQ-0004-01). */
const char *easel_window_get_zoom_text (EaselWindow *self);

/* The color chooser is open (REQ-0003-14). */
gboolean easel_window_is_choosing_color (EaselWindow *self);

/* Ends the open color choice: rgba is the confirmed color, NULL means Cancel. The chooser's callback calls it; tests
   call it in place of GTK's dialog. */
void easel_window_finish_color_choice (EaselWindow *self, const GdkRGBA *rgba);

/* Opens the picture in file as the Open command does after the file dialog (REQ-0005-07..09), asking first when the
   picture is modified (REQ-0005-04). Nothing is decoded in this call: the window shows the busy cursor and ignores
   input, and the picture is read after the next painted frame, so the cursor is on screen before the wait starts. The
   result, or the error dialog, follows from the main loop. Does nothing before the start-up picture exists or while a
   dialog is open. */
void easel_window_open_file (EaselWindow *self, GFile *file);

/* A picture is being opened: from the request until it is shown or the error is. */
gboolean easel_window_is_loading (EaselWindow *self);

/* Test hooks: the name of the window's cursor at the moment the last decode started (NULL if none yet), and how many
   frames were painted between the request and that moment (-1 until it starts). */
const char *easel_window_get_cursor_at_decode (EaselWindow *self);
int easel_window_get_frames_before_decode (EaselWindow *self);
/* Test-only getter: whether the last picture decode was started by a frame's after-paint (not by an idle). */
gboolean easel_window_get_decode_after_paint (EaselWindow *self);

/* What happens when the user has typed or chosen path in the save dialog (REQ-0005-10, REQ-0005-12): a name without an
   extension gets .png, an extension that cannot be saved gives the error "Cannot Save This Kind of File", an existing
   file whose name had .png added is asked about (REQ-0001-42), and otherwise the picture is written in the format of the
   extension and becomes the picture's file. Test hook for the part of Save As that follows the system's dialog. */
void easel_window_save_chosen (EaselWindow *self, const char *path);

/* Saves the picture at path without asking, in the format of its extension (PNG when the extension says nothing); on success it becomes the picture's file.
   On failure shows an error and returns FALSE (REQ-0001-30, REQ-0001-31). */
gboolean easel_window_save_to (EaselWindow *self, const char *path);

/* Resizes the picture with the current background color as the fill, as one undo step (REQ-0004-20, REQ-0004-22), and
   makes the canvas and the status bar show it. UNCHANGED (same size) adds no step. If the memory is not there it shows
   the error dialog of REQ-0004-26 and changes nothing. */
EaselResizeResult easel_window_resize_picture (EaselWindow *self, int width, int height);

G_END_DECLS
