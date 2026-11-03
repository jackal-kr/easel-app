/* SPDX-License-Identifier: GPL-3.0-or-later */
#pragma once

#include <adwaita.h>

#include "clipboard.h"

#include "canvas.h"
#include "font-card.h"
#include "print.h"
#include "palette-bar.h"
#include "status-bar.h"
#include "tool-options.h"
#include "image.h"
#include "settings-file.h"
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
/* The font card over the view (tests). */
EaselFontCard *easel_window_get_font_card (EaselWindow *self);
GtkWidget *easel_window_get_thumbnail_card (EaselWindow *self);
const EaselPageSetup *easel_window_get_page_setup (EaselWindow *self);

/* Stands in for the desktop's print dialog (a test); then easel_window_print_confirmed or _cancelled answer for it. */
typedef void (*EaselPrintRunner) (EaselWindow *win, gpointer data);
void easel_window_set_print_runner (EaselWindow *self, EaselPrintRunner run, gpointer data);
/* A test: the real operation, written to path with no dialog. */
void easel_window_set_print_export (EaselWindow *self, const char *path);
/* A test: the copy of the picture refused above max_bytes. */
void easel_window_set_print_copy_limit (EaselWindow *self, gsize max_bytes);
/* The dialog confirmed a paper of w x h: the job as of now, NULL with the error shown (REQ-0026-13 to -15). */
EaselPrintJob *easel_window_print_confirmed (EaselWindow *self, double paper_w_mm, double paper_h_mm);
void easel_window_print_cancelled (EaselWindow *self);
/* What a confirm keeps: tab's choices if tab, the paper and orientation if paper (REQ-0026-04). */
void easel_window_print_remember (EaselWindow *self, GtkWidget *tab, const char *paper, gboolean landscape);
/* The job has gone, or failed; frees it (REQ-0026-15, -16). */
void easel_window_print_finished (EaselWindow *self, EaselPrintJob *job, gboolean ok);
gboolean easel_window_print_is_sending (EaselWindow *self);
/* A press on the card, with the modifier state of its event (REQ-0021-09, -10). */
void easel_window_thumbnail_press (EaselWindow *self, GdkModifierType state);
EaselPaletteBar *easel_window_get_palette_bar (EaselWindow *self);
EaselToolOptions *easel_window_get_tool_options (EaselWindow *self);
/* Preferences (0020): the settings the dialog's rows show and change; what a change does to the run, each part where it applies now. */
EaselSettings *easel_window_get_chosen_settings (EaselWindow *self);
/* The changes written to the settings file; one failure a dialog opening shows the error (REQ-0020-10). */
void easel_window_write_settings (EaselWindow *self, const EaselSettingChange *changes, gsize n);
void easel_window_apply_style (EaselWindow *self, EaselStyle style);
void easel_window_apply_tint (EaselWindow *self, int percent);
void easel_window_apply_frame_time (EaselWindow *self, gboolean shown);
void easel_window_apply_panels (EaselWindow *self, gboolean toolbox, gboolean colorbox, gboolean statusbar);
/* The chosen size, canvas color, JPEG quality and workspace colors take effect for what comes next. */
void easel_window_apply_chosen (EaselWindow *self);
void easel_window_automatic_size (EaselWindow *self, int *width, int *height);
void easel_window_reset_palette (EaselWindow *self);
void easel_window_preferences_opened (EaselWindow *self, AdwDialog *dialog);
void easel_window_preferences_closed (EaselWindow *self);
/* The Text tool's mode: TRUE for Opaque (REQ-0017-02). */
gboolean easel_window_get_text_opaque (EaselWindow *self);
EaselStatusBar *easel_window_get_status_bar (EaselWindow *self);

GtkMenuButton *easel_window_get_main_menu_button (EaselWindow *self);
GtkMenuButton *easel_window_get_image_button (EaselWindow *self);
GtkMenuButton *easel_window_get_view_button (EaselWindow *self);
/* The selection's context menu (REQ-0013-17), for tests: shown while it is open. */
GtkPopoverMenu *easel_window_get_selection_menu (EaselWindow *self);
GtkPopoverMenu *easel_window_get_canvas_menu (EaselWindow *self);
/* The text box's menu (REQ-0017-22; tests). */
GtkPopoverMenu *easel_window_get_text_menu (EaselWindow *self);
/* The clipboard the window uses (0016): NULL for the desktop's; the ui tests give theirs (the isolated session holds none). */
void easel_window_set_clipboard (EaselWindow *self, EaselClipboard *clipboard);
/* The limit of the wait for the clipboard's content, in ms (REQ-0016-15): 5000 unless a test sets it. */
void easel_window_set_clipboard_timeout (EaselWindow *self, guint ms);

/* Opens uri, and calls easel_window_help_finished at the end (REQ-0021-04); a test sets its own. */
typedef void (*EaselHelpLauncher) (EaselWindow *win, const char *uri, gpointer data);
void easel_window_set_help_launcher (EaselWindow *self, EaselHelpLauncher launch, gpointer data);
void easel_window_help_finished (EaselWindow *self, gboolean ok);
/* Tests only (REQ-0017-19, REQ-0017-20): rich text (HTML) written for Copy or read for Paste that is longer than `bytes` fails as when
   its memory cannot be had. G_MAXSIZE, the start, is off: the production path is unchanged. */
void easel_window_set_rich_text_limit (EaselWindow *self, gsize bytes);
/* Tests only (REQ-0018-10): an allocation of an Image command (the new pixels, the log of its step) above `bytes` fails as when its
   memory cannot be had. G_MAXSIZE, the start, is off: the production path is unchanged. */
void easel_window_set_transform_limit (EaselWindow *self, gsize bytes);
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
/* A file chosen for Paste From… (0016): read as Open reads it and pasted; a refusal shows its dialog. Also the tests' way to choose one. */
void easel_window_paste_from_file (EaselWindow *self, GFile *file);
/* The name chosen for Copy To… (0016): the selection's copy is written as Save As writes a picture; also the tests' way to choose one. */
void easel_window_copy_to_chosen (EaselWindow *self, const char *path);

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
