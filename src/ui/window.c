/* SPDX-License-Identifier: GPL-3.0-or-later */

#include <glib/gstdio.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "attributes-dialog.h"
#include "grid-dialog.h"
#include "canvas.h"
#include "palette-bar.h"
#include "palette-file.h"
#include "picture-file.h"
#include "format.h"
#include "settings-file.h"
#include "view.h"
#include "window.h"

struct _EaselWindow
{
  AdwApplicationWindow parent_instance;

  GtkScrolledWindow *scroller;
  EaselCanvas *canvas;
  EaselPaletteBar *palette_bar;
  EaselStatusBar *status_bar;
  GtkWidget *toolbox_panel;   /* the tool buttons and their separator */
  GtkMenuButton *main_menu_button;
  GtkMenuButton *image_button;
  GtkMenuButton *view_button;
  GtkLabel *zoom_label;       /* the View menu's zoom row, created in code */

  EaselSettings settings;
  EaselImage *picture;
  EaselTool tool;
  uint32_t foreground;
  uint32_t background;
  EaselPalette palette;
  EaselHistory *history;
  char *path;             /* the picture's file; NULL until the first save */
  gboolean save_dialog_open;
  gboolean open_dialog_open;  /* the file dialog of Open */
  GFile *startup_file;    /* the file of the command line, opened in place of the blank picture (REQ-0005-15) */
  GFile *pending_file;    /* what Open goes on to read after the unsaved-changes prompt */
  gboolean loading;       /* a picture is being opened (REQ-0005-02) */
  GFile *loading_file;
  gboolean close_after_loading;   /* a quit request came while loading */
  gulong paint_handler;   /* the frame clock's after-paint that starts the decode */
  gint64 frame_at_request;
  char *cursor_at_decode;  /* test hook */
  int frames_before_decode;
  gboolean prompt_open;   /* a "Save Changes?" prompt, from showing it until its choice is carried out */
  void (*prompt_then) (EaselWindow *self);   /* what Discard, or a Save that worked, goes on to do */

  /* The color chooser (REQ-0003-11..14): what it edits, a swatch number or one of the CHOOSE_ values. */
  gboolean chooser_open;
  int chooser_target;
  GCancellable *chooser_cancel;
};

enum
{
  CHOOSE_FOREGROUND = -1,
  CHOOSE_BACKGROUND = -2,
};

/* Called when a save started by save () ends; saved is FALSE on failure or cancel. */
typedef void (*SaveDone) (EaselWindow *self, gboolean saved);

G_DEFINE_FINAL_TYPE (EaselWindow, easel_window, ADW_TYPE_APPLICATION_WINDOW)

static void sync_zoom_ui (EaselWindow *self);
static void update_title (EaselWindow *self);
static void show_open_error (EaselWindow *self, GFile *file, EaselOpenResult result, int width, int height);
static gboolean dialog_is_open (EaselWindow *self);
static void show_memory_error (EaselWindow *self, const char *heading, int width, int height);
static gboolean view_actions_allowed (EaselWindow *self);

static void
create_picture (EaselWindow *self, int area_width, int area_height)
{
  g_autoptr (GFile) startup = g_steal_pointer (&self->startup_file);
  EaselOpenResult opened = EASEL_OPEN_OK;
  int width, height;

  if (self->picture != NULL)
    return;
  /* The file of the command line replaces the blank picture; if it cannot be opened the blank one shows and the error
     is told over it (REQ-0005-15). There is no busy cursor here: the window is not showing anything yet. */
  if (startup != NULL)
    {
      g_autofree char *file_path = g_file_get_path (startup);

      opened = EASEL_OPEN_UNREADABLE;
      if (file_path != NULL)
        opened = easel_picture_open (file_path, &self->picture, &width, &height);
      if (opened == EASEL_OPEN_OK)
        self->path = g_steal_pointer (&file_path);
    }
  if (self->picture == NULL)
    {
      easel_settings_resolve_size (&self->settings, area_width, area_height, &width, &height);
      self->picture = easel_image_new (width, height, self->settings.background);
    }
  if (self->picture == NULL)
    {
      g_critical ("Cannot allocate a %dx%d picture", width, height);
      return;
    }
  easel_canvas_set_image (self->canvas, self->picture);
  easel_status_bar_set_size (self->status_bar, TRUE, self->picture->width, self->picture->height);
  sync_zoom_ui (self);
  update_title (self);
  if (opened != EASEL_OPEN_OK)
    show_open_error (self, startup, opened, width, height);
}

/* The monitor the window is on, or the first one while Wayland has not said yet (REQ-0001-41). */
static GdkMonitor *
window_monitor (EaselWindow *self)
{
  GdkDisplay *display = gtk_widget_get_display (GTK_WIDGET (self));
  GtkNative *native = GTK_NATIVE (self);
  GdkSurface *surface = gtk_native_get_surface (native);
  GdkMonitor *monitor = surface ? gdk_display_get_monitor_at_surface (display, surface) : NULL;

  if (monitor != NULL)
    return g_object_ref (monitor);
  return g_list_model_get_item (gdk_display_get_monitors (display), 0);
}

/* An auto size depends on the visible canvas area, known once the scrolled area is first allocated:
   the adjustments' page sizes are that area in logical pixels. On Wayland a HiDPI surface learns its
   scale only after its first frame, so the monitor's scale converts to device pixels. */
/* The visible canvas area in device pixels, from the adjustments' page sizes; false until it is known. */
static gboolean
canvas_area_device (EaselWindow *self, int *width, int *height)
{
  double w = gtk_adjustment_get_page_size (gtk_scrolled_window_get_hadjustment (self->scroller));
  double h = gtk_adjustment_get_page_size (gtk_scrolled_window_get_vadjustment (self->scroller));
  g_autoptr (GdkMonitor) monitor = NULL;
  double scale;

  if (w <= 0 || h <= 0)
    return FALSE;
  monitor = window_monitor (self);
  scale = monitor ? gdk_monitor_get_scale (monitor) : easel_canvas_get_scale (self->canvas);
  *width = (int) (w * scale);
  *height = (int) (h * scale);
  return TRUE;
}

static void
on_page_size_changed (EaselWindow *self)
{
  int w, h;

  if (self->picture != NULL || !canvas_area_device (self, &w, &h))
    return;
  create_picture (self, w, h);
}

/* The window opens at 75 % of the monitor. Before it appears, Wayland cannot say which monitor that is,
   so the first one is used (REQ-0001-41). */
static void
set_initial_size (EaselWindow *self)
{
  g_autoptr (GdkMonitor) monitor = window_monitor (self);
  GdkRectangle geometry;
  int width, height;

  if (monitor == NULL)
    return;
  gdk_monitor_get_geometry (monitor, &geometry);
  easel_view_window_size (geometry.width, geometry.height, &width, &height);
  gtk_window_set_default_size (GTK_WINDOW (self), width, height);
}

static void
update_title (EaselWindow *self)
{
  g_autofree char *name = self->path ? g_path_get_basename (self->path) : g_strdup ("Untitled");
  g_autofree char *title = g_strdup_printf (easel_history_is_modified (self->history) ? "• %s – Easel" : "%s – Easel",
                                            name);

  gtk_window_set_title (GTK_WINDOW (self), title);
}

/* The title and the Undo and Redo actions follow the history (REQ-0002-06, REQ-0002-07, REQ-0002-12). */
static void
sync_history_ui (EaselWindow *self)
{
  GAction *undo = g_action_map_lookup_action (G_ACTION_MAP (self), "undo");
  GAction *redo = g_action_map_lookup_action (G_ACTION_MAP (self), "redo");

  if (undo != NULL)
    g_simple_action_set_enabled (G_SIMPLE_ACTION (undo), easel_history_can_undo (self->history));
  if (redo != NULL)
    g_simple_action_set_enabled (G_SIMPLE_ACTION (redo), easel_history_can_redo (self->history));
  update_title (self);
}

static void
on_stroke_finished (EaselWindow *self, guint changed)
{
  /* A stroke that changes no pixel adds no step, so the history, and the modified mark, stay as they were
     (REQ-0002-01, REQ-0002-02). */
  sync_history_ui (self);
}

static void
show_save_error (EaselWindow *self, const char *heading, const char *body)
{
  AdwDialog *dialog = adw_alert_dialog_new (heading, body);

  adw_alert_dialog_add_response (ADW_ALERT_DIALOG (dialog), "close", "Close");
  adw_dialog_present (dialog, GTK_WIDGET (self));
}

/* The format a file is written in: that of its extension, PNG if the extension says nothing (the callers have fixed the
   name or checked it). */
static EaselFormat
save_format (const char *path)
{
  EaselFormat format = easel_format_of_name (path);

  return format == EASEL_FORMAT_UNKNOWN ? EASEL_FORMAT_PNG : format;
}

gboolean
easel_window_save_to (EaselWindow *self, const char *path)
{
  g_autoptr (GError) error = NULL;

  if (self->picture == NULL)
    return FALSE;
  if (!easel_picture_save (self->picture, path, save_format (path), self->settings.jpeg_quality, &error))
    {
      g_autofree char *name = g_path_get_basename (path);
      g_autofree char *heading = g_strdup_printf ("Could Not Save “%s”", name);
      g_autofree char *body = g_strdup_printf ("“%s” could not be saved: %s", path, error->message);

      /* The picture, its title and its modified state stay as they were (REQ-0001-31). */
      show_save_error (self, heading, body);
      return FALSE;
    }
  /* path may be self->path itself, so copy before freeing. */
  {
    char *copy = g_strdup (path);

    g_free (self->path);
    self->path = copy;
  }
  easel_history_mark_saved (self->history);
  update_title (self);
  return TRUE;
}

typedef struct
{
  EaselWindow *self;      /* owned reference while the dialog is open */
  char *path;
  SaveDone done;
} SaveRequest;

static void
save_request_finish (SaveRequest *req, gboolean saved)
{
  if (req->done != NULL)
    req->done (req->self, saved);
  g_object_unref (req->self);
  g_free (req->path);
  g_free (req);
}

static void
on_replace_response (AdwAlertDialog *dialog, GAsyncResult *result, SaveRequest *req)
{
  const char *response = adw_alert_dialog_choose_finish (dialog, result);
  gboolean saved = FALSE;

  if (g_strcmp0 (response, "replace") == 0)
    saved = easel_window_save_to (req->self, req->path);
  save_request_finish (req, saved);
}

/* The dialog confirmed the name as typed; once .png is added, an existing file needs its own question
   (REQ-0001-42). */
static void
ask_replace (SaveRequest *req)
{
  g_autofree char *name = g_path_get_basename (req->path);
  g_autofree char *heading = g_strdup_printf ("Replace “%s”?", name);
  AdwDialog *dialog = adw_alert_dialog_new (heading, "A file with this name already exists.");

  adw_alert_dialog_add_responses (ADW_ALERT_DIALOG (dialog), "cancel", "_Cancel", "replace", "_Replace", NULL);
  adw_alert_dialog_set_response_appearance (ADW_ALERT_DIALOG (dialog), "replace", ADW_RESPONSE_DESTRUCTIVE);
  adw_alert_dialog_set_default_response (ADW_ALERT_DIALOG (dialog), "cancel");
  adw_alert_dialog_set_close_response (ADW_ALERT_DIALOG (dialog), "cancel");
  adw_alert_dialog_choose (ADW_ALERT_DIALOG (dialog), GTK_WIDGET (req->self), NULL,
                           (GAsyncReadyCallback) on_replace_response, req);
}

/* The name the user typed or chose, with the rules of REQ-0005-10 and REQ-0001-42; ends the request. */
static void
save_chosen (SaveRequest *req, const char *chosen)
{
  char *fixed = NULL;
  EaselFormat format;

  switch (easel_format_fix_name (chosen, &fixed, &format))
    {
    case EASEL_FORMAT_UNKNOWN_EXTENSION:
      show_save_error (req->self, "Cannot Save This Kind of File", "Pictures can be saved as PNG, JPEG, BMP, TIFF or WebP.");
      save_request_finish (req, FALSE);
      return;
    case EASEL_FORMAT_NO_MEMORY:
      g_critical ("Out of memory choosing a file name");
      save_request_finish (req, FALSE);
      return;
    case EASEL_FORMAT_OK:
      break;
    }

  req->path = g_strdup (fixed);
  free (fixed);
  /* The system's dialog confirmed the name as typed; once .png is added, an existing file needs its own question
     (REQ-0001-42). */
  if (strcmp (req->path, chosen) != 0 && g_file_test (req->path, G_FILE_TEST_EXISTS))
    {
      ask_replace (req);
      return;
    }
  save_request_finish (req, easel_window_save_to (req->self, req->path));
}

static void
on_save_dialog_done (GtkFileDialog *dialog, GAsyncResult *result, SaveRequest *req)
{
  g_autoptr (GFile) file = gtk_file_dialog_save_finish (dialog, result, NULL);
  g_autofree char *chosen = file ? g_file_get_path (file) : NULL;

  req->self->save_dialog_open = FALSE;

  /* Cancelled: nothing is saved and nothing changes (REQ-0001-32). */
  if (chosen == NULL)
    {
      save_request_finish (req, FALSE);
      return;
    }
  save_chosen (req, chosen);
}

void
easel_window_save_chosen (EaselWindow *self, const char *path)
{
  SaveRequest *req = g_new0 (SaveRequest, 1);

  req->self = g_object_ref (self);
  save_chosen (req, path);
}

/* The name the dialog suggests for a file that cannot be saved in place: its name without the extension, plus .png
   (REQ-0005-13). */
static char *
png_name_for (const char *path)
{
  const char *slash = strrchr (path, '/');
  const char *name = slash ? slash + 1 : path;
  const char *dot = strrchr (name, '.');
  gsize stem = (dot == NULL || dot == name) ? strlen (path) : (gsize) (dot - path);

  return g_strdup_printf ("%.*s.png", (int) stem, path);
}

/* Saves to the picture's file, or asks for one first (REQ-0001-27, REQ-0001-29, REQ-0005-13). With as_new, always asks
   (Save As). A file whose extension cannot be saved is asked about too, suggesting a .png. done may be NULL. */
static void
save (EaselWindow *self, SaveDone done, gboolean as_new)
{
  g_autoptr (GtkFileDialog) dialog = NULL;
  g_autoptr (GFile) suggestion = NULL;
  const char *pictures = g_get_user_special_dir (G_USER_DIRECTORY_PICTURES);
  SaveRequest *req;

  /* A stroke in progress is ended first and saved with the picture (REQ-0001-33). */
  easel_canvas_end_stroke (self->canvas);
  easel_canvas_cancel_resize (self->canvas);

  if (!as_new && self->path != NULL && easel_format_of_name (self->path) != EASEL_FORMAT_UNKNOWN)
    {
      gboolean saved = easel_window_save_to (self, self->path);

      if (done != NULL)
        done (self, saved);
      return;
    }
  /* A save already in progress keeps going; this request counts as not saved, so a waiting quit prompt
     ends as Cancel (REQ-0001-43). */
  if (self->save_dialog_open || self->picture == NULL)
    {
      if (done != NULL)
        done (self, FALSE);
      return;
    }

  req = g_new0 (SaveRequest, 1);
  req->self = g_object_ref (self);
  req->done = done;

  dialog = gtk_file_dialog_new ();
  if (self->path != NULL)
    {
      /* The file's own name, or its name as a .png when it cannot be written as it is. */
      g_autofree char *suggested = easel_format_of_name (self->path) != EASEL_FORMAT_UNKNOWN ? g_strdup (self->path)
                                                                                              : png_name_for (self->path);

      suggestion = g_file_new_for_path (suggested);
      gtk_file_dialog_set_title (dialog, "Save Picture As");
      gtk_file_dialog_set_initial_file (dialog, suggestion);
    }
  else
    {
      suggestion = g_file_new_for_path (pictures ? pictures : g_get_home_dir ());
      gtk_file_dialog_set_initial_folder (dialog, suggestion);
      gtk_file_dialog_set_initial_name (dialog, "Untitled.png");
      if (as_new)
        gtk_file_dialog_set_title (dialog, "Save Picture As");
    }
  self->save_dialog_open = TRUE;
  gtk_file_dialog_save (dialog, GTK_WINDOW (self), NULL, (GAsyncReadyCallback) on_save_dialog_done, req);
}

static void
on_prompt_save_done (EaselWindow *self, gboolean saved)
{
  void (*then) (EaselWindow *) = self->prompt_then;

  self->prompt_open = FALSE;
  self->prompt_then = NULL;
  /* Go on only if the save succeeded (REQ-0001-36, REQ-0005-04). */
  if (saved)
    then (self);
}

static void
on_prompt_response (AdwAlertDialog *dialog, const char *response, EaselWindow *self)
{
  void (*then) (EaselWindow *) = self->prompt_then;

  if (g_strcmp0 (response, "save") == 0)
    {
      save (self, on_prompt_save_done, FALSE);
      return;
    }
  self->prompt_open = FALSE;
  self->prompt_then = NULL;
  /* Discard only means "go on": nothing is lost until a picture really replaces this one (REQ-0005-04). */
  if (g_strcmp0 (response, "discard") == 0)
    then (self);
}

/* The question of the quit prompt (REQ-0001-35), also asked before New and Open (REQ-0005-04); then is what a
   Discard, or a Save that worked, goes on to do. */
static void
ask_unsaved (EaselWindow *self, void (*then) (EaselWindow *self))
{
  g_autofree char *name = self->path ? g_path_get_basename (self->path) : g_strdup ("Untitled");
  g_autofree char *body = g_strdup_printf ("“%s” has unsaved changes.", name);
  AdwDialog *dialog = adw_alert_dialog_new ("Save Changes?", body);

  adw_alert_dialog_add_responses (ADW_ALERT_DIALOG (dialog),
                                  "cancel", "_Cancel", "discard", "_Discard", "save", "_Save", NULL);
  adw_alert_dialog_set_response_appearance (ADW_ALERT_DIALOG (dialog), "discard", ADW_RESPONSE_DESTRUCTIVE);
  adw_alert_dialog_set_response_appearance (ADW_ALERT_DIALOG (dialog), "save", ADW_RESPONSE_SUGGESTED);
  adw_alert_dialog_set_default_response (ADW_ALERT_DIALOG (dialog), "save");
  adw_alert_dialog_set_close_response (ADW_ALERT_DIALOG (dialog), "cancel");
  g_signal_connect_object (dialog, "response", G_CALLBACK (on_prompt_response), self, G_CONNECT_DEFAULT);
  self->prompt_open = TRUE;
  self->prompt_then = then;
  adw_dialog_present (dialog, GTK_WIDGET (self));
}

static void
quit_now (EaselWindow *self)
{
  gtk_window_destroy (GTK_WINDOW (self));
}

/* Ctrl+Q, Ctrl+W and the window's close button all end here. */
static gboolean
easel_window_close_request (GtkWindow *window)
{
  EaselWindow *self = EASEL_WINDOW (window);

  /* A stroke in progress is ended first (REQ-0001-33); an open color chooser closes as Cancel (REQ-0003-14). */
  easel_canvas_end_stroke (self->canvas);
  easel_canvas_cancel_resize (self->canvas);
  easel_window_finish_color_choice (self, NULL);

  /* A quit during loading waits for the picture, then goes on; while the Open dialog is open it does nothing
     (REQ-0005-02). */
  if (self->loading)
    {
      self->close_after_loading = TRUE;
      return TRUE;
    }
  if (self->open_dialog_open)
    return TRUE;
  /* One prompt at a time (REQ-0001-38). */
  if (self->prompt_open)
    return TRUE;
  if (!easel_history_is_modified (self->history))
    return FALSE;
  ask_unsaved (self, quit_now);
  return TRUE;
}

/* ---- New (REQ-0005-03, REQ-0005-04) ---- */

/* Puts img in place of the picture, with a new, empty history whose first state is the saved one, the file path (taken
   over; NULL for none), the view at its start, and everything that shows the picture brought up to date. The old picture
   and history go last, so that nothing is lost if this fails. Returns FALSE, with img freed and nothing changed, if the
   history cannot be had. */
static gboolean
replace_picture (EaselWindow *self, EaselImage *img, char *path)
{
  EaselHistory *history = easel_history_new ((size_t) self->settings.undo_budget_mb << 20);
  EaselImage *old_picture = self->picture;
  EaselHistory *old_history = self->history;

  if (history == NULL)
    {
      easel_image_free (img);
      g_free (path);
      return FALSE;
    }
  self->picture = img;
  self->history = history;
  g_free (self->path);
  self->path = path;
  easel_canvas_set_history (self->canvas, history);
  easel_canvas_set_image (self->canvas, img);
  easel_canvas_reset_view (self->canvas);
  easel_status_bar_set_size (self->status_bar, TRUE, img->width, img->height);
  sync_zoom_ui (self);
  sync_history_ui (self);
  easel_image_free (old_picture);
  easel_history_free (old_history);
  return TRUE;
}

/* A picture as at start-up: the size and background of the settings read when the app started (REQ-0005-03). */
static void
new_picture (EaselWindow *self)
{
  int area_w = 0, area_h = 0, w, h;
  EaselImage *img;

  canvas_area_device (self, &area_w, &area_h);
  easel_settings_resolve_size (&self->settings, area_w, area_h, &w, &h);
  img = easel_image_new (w, h, self->settings.background);
  if (img == NULL || !replace_picture (self, img, NULL))
    show_memory_error (self, "Could Not Create a New Picture", w, h);
}

/* New, Open, Save and Save As: nothing before the start-up picture exists or while a dialog or prompt is open; a stroke
   in progress ends first and a resize drag is cancelled (REQ-0005-02). */
static gboolean
file_action_allowed (EaselWindow *self)
{
  if (self->picture == NULL || dialog_is_open (self))
    return FALSE;
  easel_canvas_end_stroke (self->canvas);
  easel_canvas_cancel_resize (self->canvas);
  return TRUE;
}

static void
new_activated (GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
  EaselWindow *self = user_data;

  if (!file_action_allowed (self))
    return;
  if (easel_history_is_modified (self->history))
    ask_unsaved (self, new_picture);
  else
    new_picture (self);
}

/* ---- Open (REQ-0005-04 to REQ-0005-09) ---- */

static void
show_open_error (EaselWindow *self, GFile *file, EaselOpenResult result, int width, int height)
{
  g_autofree char *name = g_file_get_basename (file);
  g_autofree char *heading = g_strdup_printf ("Could Not Open “%s”", name);

  switch (result)
    {
    case EASEL_OPEN_UNREADABLE:
      show_save_error (self, heading, "The file cannot be read.");
      break;
    case EASEL_OPEN_NOT_PICTURE:
      show_save_error (self, heading, "This is not a picture the app can open.");
      break;
    case EASEL_OPEN_TOO_BIG:
      show_save_error (self, heading, "The picture is bigger than 10000 pixels in width or height.");
      break;
    case EASEL_OPEN_NO_MEMORY:
      show_memory_error (self, heading, width, height);
      break;
    case EASEL_OPEN_OK:
      break;
    }
}

/* The picture is read here, after a frame was painted with the busy cursor (Plan, "The busy cursor shows before
   decoding starts"), and then shown or refused. */
static void
decode_now (EaselWindow *self)
{
  g_autoptr (GFile) file = g_steal_pointer (&self->loading_file);
  g_autofree char *path = g_file_get_path (file);
  GdkCursor *cursor = gtk_widget_get_cursor (GTK_WIDGET (self));
  GdkFrameClock *clock = gtk_widget_get_frame_clock (GTK_WIDGET (self));
  EaselOpenResult result = EASEL_OPEN_UNREADABLE;
  EaselImage *img = NULL;
  int w = 0, h = 0;

  g_free (self->cursor_at_decode);
  self->cursor_at_decode = cursor ? g_strdup (gdk_cursor_get_name (cursor)) : NULL;
  self->frames_before_decode = clock ? (int) (gdk_frame_clock_get_frame_counter (clock) - self->frame_at_request) : 0;

  if (path != NULL)
    result = easel_picture_open (path, &img, &w, &h);

  /* Back to the tool's cursor and to taking input, whatever the result. */
  self->loading = FALSE;
  gtk_widget_set_cursor (GTK_WIDGET (self), NULL);
  easel_canvas_set_busy (self->canvas, FALSE);
  easel_canvas_set_input_blocked (self->canvas, FALSE);

  if (result == EASEL_OPEN_OK)
    {
      if (!replace_picture (self, img, g_steal_pointer (&path)))
        show_open_error (self, file, EASEL_OPEN_NO_MEMORY, w, h);
    }
  else
    show_open_error (self, file, result, w, h);
  if (self->close_after_loading)
    {
      self->close_after_loading = FALSE;
      gtk_window_close (GTK_WINDOW (self));
    }
}

static void
on_loading_painted (GdkFrameClock *clock, EaselWindow *self)
{
  g_clear_signal_handler (&self->paint_handler, clock);
  decode_now (self);
}

static gboolean
decode_idle (gpointer data)
{
  EaselWindow *self = data;

  if (self->loading)
    decode_now (self);
  g_object_unref (self);
  return G_SOURCE_REMOVE;
}

/* Shows the busy cursor and ignores input, then reads the file once a frame has been painted. */
static void
begin_loading (EaselWindow *self, GFile *file)
{
  GdkFrameClock *clock = gtk_widget_get_mapped (GTK_WIDGET (self)) ? gtk_widget_get_frame_clock (GTK_WIDGET (self)) : NULL;

  g_set_object (&self->loading_file, file);
  self->loading = TRUE;
  self->frames_before_decode = -1;
  gtk_widget_set_cursor_from_name (GTK_WIDGET (self), "wait");
  easel_canvas_set_busy (self->canvas, TRUE);
  easel_canvas_set_input_blocked (self->canvas, TRUE);
  self->frame_at_request = clock ? gdk_frame_clock_get_frame_counter (clock) : 0;
  if (clock != NULL)
    {
      self->paint_handler = g_signal_connect_object (clock, "after-paint", G_CALLBACK (on_loading_painted), self,
                                                     G_CONNECT_DEFAULT);
      gtk_widget_queue_draw (GTK_WIDGET (self));
    }
  else
    g_idle_add (decode_idle, g_object_ref (self));   /* a window that is not shown has no frames */
}

static void
on_open_dialog_done (GtkFileDialog *dialog, GAsyncResult *result, EaselWindow *self)
{
  g_autoptr (GFile) file = gtk_file_dialog_open_finish (dialog, result, NULL);

  self->open_dialog_open = FALSE;
  /* Cancelled: nothing changes (REQ-0005-05). */
  if (file != NULL)
    begin_loading (self, file);
  g_object_unref (self);
}

static void
show_open_dialog (EaselWindow *self)
{
  g_autoptr (GtkFileDialog) dialog = gtk_file_dialog_new ();
  g_autoptr (GListStore) filters = g_list_store_new (GTK_TYPE_FILE_FILTER);
  g_autoptr (GtkFileFilter) pictures = gtk_file_filter_new ();
  g_autoptr (GtkFileFilter) all = gtk_file_filter_new ();
  g_autoptr (GFile) folder = NULL;
  const char *pictures_dir = g_get_user_special_dir (G_USER_DIRECTORY_PICTURES);
  static const char *const mime_types[] = { "image/png", "image/jpeg", "image/bmp", "image/tiff", "image/webp", "image/gif" };

  /* The folder of the current picture's file, else the Pictures folder, else home (REQ-0005-05). */
  if (self->path != NULL)
    {
      g_autoptr (GFile) current = g_file_new_for_path (self->path);

      folder = g_file_get_parent (current);
    }
  if (folder == NULL || !g_file_query_exists (folder, NULL))
    {
      g_clear_object (&folder);
      folder = g_file_new_for_path (pictures_dir && g_file_test (pictures_dir, G_FILE_TEST_IS_DIR) ? pictures_dir
                                                                                                     : g_get_home_dir ());
    }
  gtk_file_filter_set_name (pictures, "Pictures");
  for (gsize i = 0; i < G_N_ELEMENTS (mime_types); i++)
    gtk_file_filter_add_mime_type (pictures, mime_types[i]);
  gtk_file_filter_set_name (all, "All Files");
  gtk_file_filter_add_pattern (all, "*");
  g_list_store_append (filters, pictures);
  g_list_store_append (filters, all);
  gtk_file_dialog_set_title (dialog, "Open Picture");
  gtk_file_dialog_set_filters (dialog, G_LIST_MODEL (filters));
  gtk_file_dialog_set_default_filter (dialog, pictures);
  gtk_file_dialog_set_initial_folder (dialog, folder);
  self->open_dialog_open = TRUE;
  gtk_file_dialog_open (dialog, GTK_WINDOW (self), NULL, (GAsyncReadyCallback) on_open_dialog_done, g_object_ref (self));
}

static void
open_pending (EaselWindow *self)
{
  g_autoptr (GFile) file = g_steal_pointer (&self->pending_file);

  begin_loading (self, file);
}

void
easel_window_open_file (EaselWindow *self, GFile *file)
{
  if (!file_action_allowed (self))
    return;
  if (easel_history_is_modified (self->history))
    {
      g_set_object (&self->pending_file, file);
      ask_unsaved (self, open_pending);
    }
  else
    begin_loading (self, file);
}

static void
open_activated (GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
  EaselWindow *self = user_data;

  if (!file_action_allowed (self))
    return;
  if (easel_history_is_modified (self->history))
    ask_unsaved (self, show_open_dialog);
  else
    show_open_dialog (self);
}

gboolean
easel_window_is_loading (EaselWindow *self)
{
  return self->loading;
}

const char *
easel_window_get_cursor_at_decode (EaselWindow *self)
{
  return self->cursor_at_decode;
}

int
easel_window_get_frames_before_decode (EaselWindow *self)
{
  return self->frames_before_decode;
}

static void
close_activated (GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
  gtk_window_close (GTK_WINDOW (user_data));
}

static void
save_activated (GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
  EaselWindow *self = user_data;

  if (file_action_allowed (self))
    save (self, NULL, FALSE);
}

static void
save_as_activated (GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
  EaselWindow *self = user_data;

  if (file_action_allowed (self))
    save (self, NULL, TRUE);
}

/* The shortcuts do nothing while the quit prompt or a save, replace or error dialog is open (REQ-0002-11). */
static gboolean
dialog_is_open (EaselWindow *self)
{
  return self->prompt_open || self->save_dialog_open || self->open_dialog_open || self->loading || self->chooser_open
         || adw_application_window_get_visible_dialog (ADW_APPLICATION_WINDOW (self)) != NULL;
}

/* Colors are not edits: no undo step, no modified mark (REQ-0003-09). */
static void
set_colors (EaselWindow *self, uint32_t foreground, uint32_t background)
{
  self->foreground = foreground;
  self->background = background;
  easel_canvas_set_colors (self->canvas, foreground, background);
  easel_palette_bar_set_colors (self->palette_bar, foreground, background);
}

static void open_chooser (EaselWindow *self, int target, uint32_t color);

static uint32_t
from_rgba (const GdkRGBA *c)
{
  return ((uint32_t) (c->alpha * 255 + 0.5f) << 24) | ((uint32_t) (c->red * 255 + 0.5f) << 16)
         | ((uint32_t) (c->green * 255 + 0.5f) << 8) | (uint32_t) (c->blue * 255 + 0.5f);
}

gboolean
easel_window_is_choosing_color (EaselWindow *self)
{
  return self->chooser_open;
}

void
easel_window_finish_color_choice (EaselWindow *self, const GdkRGBA *rgba)
{
  uint32_t color;

  if (!self->chooser_open)
    return;
  self->chooser_open = FALSE;
  easel_canvas_set_input_blocked (self->canvas, FALSE);
  /* Closes GTK's dialog if it is still open; its callback then finds the choice already finished. */
  g_cancellable_cancel (self->chooser_cancel);
  g_clear_object (&self->chooser_cancel);
  if (rgba == NULL)
    return;

  color = from_rgba (rgba);
  if (self->chooser_target == CHOOSE_FOREGROUND)
    set_colors (self, color, self->background);
  else if (self->chooser_target == CHOOSE_BACKGROUND)
    set_colors (self, self->foreground, color);
  else
    {
      /* A confirmed swatch color is also the new foreground (REQ-0003-12). */
      g_autofree char *path = easel_palette_file_path ();

      self->palette.colors[self->chooser_target - 1] = color;
      easel_palette_bar_set_swatch (self->palette_bar, self->chooser_target, color);
      set_colors (self, color, self->background);
      /* Remembered at once; if writing fails the swatch still changes for this run, silently (REQ-0003-15,
         REQ-0003-16). */
      easel_palette_file_save (path, &self->palette);
    }
}

static void
on_color_chosen (GObject *source, GAsyncResult *result, gpointer data)
{
  EaselWindow *self = data;
  GdkRGBA *rgba = gtk_color_dialog_choose_rgba_finish (GTK_COLOR_DIALOG (source), result, NULL);

  easel_window_finish_color_choice (self, rgba);
  if (rgba != NULL)
    gdk_rgba_free (rgba);
  g_object_unref (self);
}

/* The system color chooser, modal and with transparency. The window also blocks its own input while it is open,
   since a compositor may not (REQ-0003-14). */
static void
open_chooser (EaselWindow *self, int target, uint32_t color)
{
  g_autoptr (GtkColorDialog) dialog = NULL;
  GdkRGBA initial = { ((color >> 16) & 0xFF) / 255.0f, ((color >> 8) & 0xFF) / 255.0f, (color & 0xFF) / 255.0f,
                      (color >> 24) / 255.0f };

  if (dialog_is_open (self))
    return;
  self->chooser_open = TRUE;
  self->chooser_target = target;
  self->chooser_cancel = g_cancellable_new ();
  easel_canvas_set_input_blocked (self->canvas, TRUE);

  dialog = gtk_color_dialog_new ();
  gtk_color_dialog_set_modal (dialog, TRUE);
  gtk_color_dialog_set_with_alpha (dialog, TRUE);
  gtk_color_dialog_choose_rgba (dialog, GTK_WINDOW (self), &initial, self->chooser_cancel, on_color_chosen,
                                g_object_ref (self));
}

static void
on_swatch_pressed (EaselWindow *self, int n, guint button, int n_press)
{
  uint32_t color = self->palette.colors[n - 1];

  if (dialog_is_open (self))
    return;
  if (button == GDK_BUTTON_PRIMARY)
    set_colors (self, color, self->background);
  else
    set_colors (self, self->foreground, color);

  /* A primary double-click on one swatch edits it; the first click has already picked it (REQ-0003-11). */
  if (button == GDK_BUTTON_PRIMARY && n_press == 2)
    open_chooser (self, n, color);
}

static void
on_swatch_edit (EaselWindow *self, int n)
{
  open_chooser (self, n, self->palette.colors[n - 1]);
}

static void
on_indicator_pressed (EaselWindow *self, gboolean front)
{
  if (front)
    open_chooser (self, CHOOSE_FOREGROUND, self->foreground);
  else
    open_chooser (self, CHOOSE_BACKGROUND, self->background);
}

static void
swap_colors_activated (GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
  EaselWindow *self = EASEL_WINDOW (user_data);

  /* Like undo and redo, not while a dialog is open (REQ-0003-07). A stroke in progress keeps its color
     (REQ-0003-08). */
  if (dialog_is_open (self))
    return;
  set_colors (self, self->background, self->foreground);
}

/* ---- the status bar (REQ-0004-09..11) ---- */

static const char *
tool_hint (EaselTool tool)
{
  return tool == EASEL_TOOL_PENCIL ? "Draw with the foreground color. The secondary button uses the background color."
                                   : "Drag to pan. Ctrl+wheel zooms.";
}

static void
on_pointer_moved (EaselWindow *self, int x, int y)
{
  easel_status_bar_set_position (self->status_bar, TRUE, x, y);
}

static void
on_pointer_left (EaselWindow *self)
{
  easel_status_bar_set_position (self->status_bar, FALSE, 0, 0);
}

/* ---- panels (REQ-0004-27) ---- */

static void
set_panel_state (EaselWindow *self, const char *action, GtkWidget *panel, gboolean shown)
{
  GAction *a = g_action_map_lookup_action (G_ACTION_MAP (self), action);

  gtk_widget_set_visible (panel, shown);
  g_simple_action_set_state (G_SIMPLE_ACTION (a), g_variant_new_boolean (shown));
}

/* Tool Box, Color Box and Status Bar toggle their panel. Like the other View actions, they do nothing while a dialog
   is open or a button is held (REQ-0004-03); the check item then stays as it is. */
static void
show_panel_state_changed (GSimpleAction *action, GVariant *value, gpointer user_data)
{
  EaselWindow *self = user_data;
  const char *name = g_action_get_name (G_ACTION (action));
  GtkWidget *panel = g_strcmp0 (name, "show-toolbox") == 0    ? self->toolbox_panel
                     : g_strcmp0 (name, "show-colorbox") == 0 ? GTK_WIDGET (self->palette_bar)
                                                               : GTK_WIDGET (self->status_bar);

  if (!view_actions_allowed (self))
    return;
  set_panel_state (self, name, panel, g_variant_get_boolean (value));
}

/* ---- the grid (REQ-0004-12, REQ-0004-15) ---- */

/* Show Grid and Ctrl+G toggle the grid. Like the other View actions they do nothing while a dialog is open or a button
   is held (REQ-0004-03). Not an edit: nothing to undo, no modified mark. */
static void
grid_state_changed (GSimpleAction *action, GVariant *value, gpointer user_data)
{
  EaselWindow *self = user_data;

  if (!view_actions_allowed (self))
    return;
  easel_canvas_set_grid (self->canvas, g_variant_get_boolean (value), easel_canvas_get_grid_cell (self->canvas),
                         easel_canvas_get_grid_axonometric (self->canvas));
  g_simple_action_set_state (action, value);
}

/* Grid…: the dialog sets the cell size and the style; it never switches the grid on or off (REQ-0004-13). */
static void
grid_dialog_applied (int cell, gboolean axonometric, gpointer user_data)
{
  EaselWindow *self = user_data;

  easel_canvas_set_grid (self->canvas, easel_canvas_get_grid_on (self->canvas), cell, axonometric);
}

static void
grid_dialog_activated (GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
  EaselWindow *self = user_data;

  if (!view_actions_allowed (self))
    return;
  easel_grid_dialog_present (GTK_WIDGET (self), easel_canvas_get_grid_cell (self->canvas),
                             easel_canvas_get_grid_axonometric (self->canvas), grid_dialog_applied, self);
}

/* ---- zoom commands (REQ-0004-04..07) ---- */

/* The View and Image actions do nothing before the picture exists, while a dialog is open, or while a mouse button is
   held for a stroke (REQ-0004-03). */
static gboolean
view_actions_allowed (EaselWindow *self)
{
  return self->picture != NULL && !dialog_is_open (self) && !easel_canvas_is_stroking (self->canvas)
         && !easel_canvas_is_resizing (self->canvas) && !easel_canvas_is_waiting_for_release (self->canvas);
}

/* The row's label and which zoom actions can do something (REQ-0004-02). */
static void
sync_zoom_ui (EaselWindow *self)
{
  int zoom = easel_canvas_get_zoom (self->canvas);
  g_autofree char *text = g_strdup_printf ("%d%%", zoom);
  struct { const char *name; gboolean enabled; } states[] = {
    { "zoom-in", easel_view_zoom_step (zoom, 1) != zoom },
    { "zoom-out", easel_view_zoom_step (zoom, -1) != zoom },
    { "zoom-normal", zoom != 100 },
    { "zoom-large", zoom != 400 },
  };

  gtk_label_set_text (self->zoom_label, text);
  easel_status_bar_set_zoom (self->status_bar, self->picture != NULL, zoom);
  for (guint i = 0; i < G_N_ELEMENTS (states); i++)
    {
      GAction *action = g_action_map_lookup_action (G_ACTION_MAP (self), states[i].name);

      if (action != NULL)
        g_simple_action_set_enabled (G_SIMPLE_ACTION (action), states[i].enabled);
    }
}

const char *
easel_window_get_zoom_text (EaselWindow *self)
{
  return gtk_label_get_text (self->zoom_label);
}

GtkMenuButton *
easel_window_get_main_menu_button (EaselWindow *self)
{
  return self->main_menu_button;
}

GtkMenuButton *
easel_window_get_image_button (EaselWindow *self)
{
  return self->image_button;
}

GtkMenuButton *
easel_window_get_view_button (EaselWindow *self)
{
  return self->view_button;
}

static void
zoom_in_activated (GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
  EaselWindow *self = user_data;

  if (view_actions_allowed (self))
    easel_canvas_set_zoom (self->canvas, easel_view_zoom_step (easel_canvas_get_zoom (self->canvas), 1));
}

static void
zoom_out_activated (GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
  EaselWindow *self = user_data;

  if (view_actions_allowed (self))
    easel_canvas_set_zoom (self->canvas, easel_view_zoom_step (easel_canvas_get_zoom (self->canvas), -1));
}

static void
zoom_normal_activated (GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
  EaselWindow *self = user_data;

  if (view_actions_allowed (self))
    easel_canvas_set_zoom (self->canvas, 100);
}

static void
zoom_large_activated (GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
  EaselWindow *self = user_data;

  if (view_actions_allowed (self))
    easel_canvas_set_zoom (self->canvas, 400);
}

static void
zoom_fit_activated (GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
  EaselWindow *self = user_data;

  if (view_actions_allowed (self))
    easel_canvas_fit_to_window (self->canvas);
}

static void
on_zoom_changed (EaselWindow *self, int pct)
{
  sync_zoom_ui (self);
}

/* The View menu's first item: Zoom Out, the zoom as N%, Zoom In. */
static GtkWidget *
make_zoom_row (EaselWindow *self)
{
  GtkWidget *row = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
  GtkWidget *zoom_out = gtk_button_new_from_icon_name ("zoom-out-symbolic");
  GtkWidget *zoom_in = gtk_button_new_from_icon_name ("zoom-in-symbolic");

  gtk_widget_set_margin_start (row, 6);
  gtk_widget_set_margin_end (row, 6);
  gtk_widget_add_css_class (zoom_out, "flat");
  gtk_widget_add_css_class (zoom_in, "flat");
  gtk_actionable_set_action_name (GTK_ACTIONABLE (zoom_out), "win.zoom-out");
  gtk_actionable_set_action_name (GTK_ACTIONABLE (zoom_in), "win.zoom-in");
  gtk_widget_set_tooltip_text (zoom_out, "Zoom Out (Ctrl+-)");
  gtk_widget_set_tooltip_text (zoom_in, "Zoom In (Ctrl+=)");
  self->zoom_label = GTK_LABEL (gtk_label_new ("100%"));
  gtk_widget_set_hexpand (GTK_WIDGET (self->zoom_label), TRUE);
  gtk_widget_add_css_class (GTK_WIDGET (self->zoom_label), "numeric");
  gtk_box_append (GTK_BOX (row), zoom_out);
  gtk_box_append (GTK_BOX (row), GTK_WIDGET (self->zoom_label));
  gtk_box_append (GTK_BOX (row), zoom_in);
  return row;
}

/* ---- the picture's size (REQ-0004-20..26) ---- */

static void
show_memory_error (EaselWindow *self, const char *heading, int width, int height)
{
  g_autofree char *body = g_strdup_printf ("There is not enough memory for a picture of %d × %d pixels.", width, height);
  AdwDialog *dialog = adw_alert_dialog_new (heading, body);

  adw_alert_dialog_add_response (ADW_ALERT_DIALOG (dialog), "close", "Close");
  adw_dialog_present (dialog, GTK_WIDGET (self));
}

static void
show_resize_error (EaselWindow *self, int width, int height)
{
  show_memory_error (self, "Could Not Resize the Picture", width, height);
}

/* The pixel buffer was replaced: the canvas builds its tiles again (zoom and scroll stay, GTK clamps the scroll to the
   new size, REQ-0004-25) and the status bar shows the size. */
static void
picture_size_changed (EaselWindow *self)
{
  easel_canvas_set_image (self->canvas, self->picture);
  easel_status_bar_set_size (self->status_bar, TRUE, self->picture->width, self->picture->height);
  sync_zoom_ui (self);
}

EaselResizeResult
easel_window_resize_picture (EaselWindow *self, int width, int height)
{
  EaselResizeResult r;

  if (self->picture == NULL)
    return EASEL_RESIZE_BAD_SIZE;
  r = easel_resize_edit (self->picture, self->history, width, height, self->background);
  if (r == EASEL_RESIZE_NO_MEMORY)
    show_resize_error (self, width, height);
  if (r == EASEL_RESIZE_OK)
    {
      picture_size_changed (self);
      sync_history_ui (self);
    }
  return r;
}

static void
attributes_applied (int width, int height, gpointer data)
{
  easel_window_resize_picture (data, width, height);
}

static void
attributes_activated (GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
  EaselWindow *self = user_data;
  int area_w = 0, area_h = 0, default_w, default_h;

  if (!view_actions_allowed (self))
    return;
  canvas_area_device (self, &area_w, &area_h);
  easel_settings_resolve_size (&self->settings, area_w, area_h, &default_w, &default_h);
  easel_attributes_dialog_present (GTK_WIDGET (self), self->picture->width, self->picture->height, default_w, default_h,
                                   attributes_applied, self);
}

/* Undo and redo change only the picture: the view, tool and colors stay (REQ-0002-08). */
static void
step_history (EaselWindow *self, gboolean undo)
{
  int old_w, old_h;

  if (self->picture == NULL || dialog_is_open (self) || easel_canvas_is_resizing (self->canvas))
    return;

  /* Undo during a stroke cancels it, as the other mouse button does; until the button is released, further
     undo and redo are ignored (REQ-0002-09). */
  if (easel_canvas_is_stroking (self->canvas))
    {
      if (undo)
        easel_canvas_cancel_stroke (self->canvas);
      return;
    }
  if (easel_canvas_is_waiting_for_release (self->canvas))
    return;
  old_w = self->picture->width;
  old_h = self->picture->height;
  if (!(undo ? easel_history_undo (self->history, self->picture) : easel_history_redo (self->history, self->picture)))
    {
      int w, h;

      /* Not enough memory for the size the step restores: nothing changed (AS-30). */
      if (easel_history_refused (self->history, &w, &h))
        show_resize_error (self, w, h);
      return;
    }
  if (self->picture->width != old_w || self->picture->height != old_h)
    picture_size_changed (self);
  else
    easel_canvas_update (self->canvas);
  sync_history_ui (self);
}

static void
undo_activated (GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
  step_history (EASEL_WINDOW (user_data), TRUE);
}

static void
redo_activated (GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
  step_history (EASEL_WINDOW (user_data), FALSE);
}

/* A handle drag ended on a new size (REQ-0004-18). */
static void
on_resize_requested (EaselWindow *self, int width, int height)
{
  easel_window_resize_picture (self, width, height);
}

/* Esc cancels a handle drag (REQ-0004-19); at any other time it is not ours. */
static gboolean
on_key_pressed (EaselWindow *self, guint keyval, guint keycode, GdkModifierType state, GtkEventControllerKey *controller)
{
  if (keyval != GDK_KEY_Escape || !easel_canvas_is_resizing (self->canvas))
    return GDK_EVENT_PROPAGATE;
  easel_canvas_cancel_resize (self->canvas);
  return GDK_EVENT_STOP;
}

/* Losing focus ends a stroke and keeps it (REQ-0001-25), and stops waiting for a release that may never come
   (REQ-0002-09). */
static void
on_active_changed (EaselWindow *self)
{
  if (!gtk_window_is_active (GTK_WINDOW (self)))
    easel_canvas_reset_pointer (self->canvas);
}

static void
change_tool_state (GSimpleAction *action, GVariant *value, gpointer user_data)
{
  EaselWindow *self = user_data;
  EaselTool tool;

  /* A tool cannot change during a stroke (REQ-0001-09) or a handle drag. */
  if (easel_canvas_is_stroking (self->canvas) || easel_canvas_is_resizing (self->canvas))
    return;
  if (!easel_tool_from_name (g_variant_get_string (value, NULL), &tool))
    return;
  self->tool = tool;
  easel_canvas_set_tool (self->canvas, tool);
  easel_status_bar_set_hint (self->status_bar, tool_hint (tool));
  g_simple_action_set_state (action, value);
}

static const GActionEntry win_actions[] = {
  { .name = "tool", .parameter_type = "s", .state = "'pan'", .change_state = change_tool_state },
  { .name = "new", .activate = new_activated },
  { .name = "open", .activate = open_activated },
  { .name = "save", .activate = save_activated },
  { .name = "save-as", .activate = save_as_activated },
  { .name = "undo", .activate = undo_activated },
  { .name = "redo", .activate = redo_activated },
  { .name = "swap-colors", .activate = swap_colors_activated },
  { .name = "zoom-in", .activate = zoom_in_activated },
  { .name = "zoom-out", .activate = zoom_out_activated },
  { .name = "zoom-normal", .activate = zoom_normal_activated },
  { .name = "zoom-large", .activate = zoom_large_activated },
  { .name = "zoom-fit", .activate = zoom_fit_activated },
  { .name = "grid", .state = "false", .change_state = grid_state_changed },
  { .name = "grid-dialog", .activate = grid_dialog_activated },
  { .name = "attributes", .activate = attributes_activated },
  { .name = "show-toolbox", .state = "true", .change_state = show_panel_state_changed },
  { .name = "show-colorbox", .state = "true", .change_state = show_panel_state_changed },
  { .name = "show-statusbar", .state = "true", .change_state = show_panel_state_changed },
  { .name = "close", .activate = close_activated },
};

const EaselImage *
easel_window_get_picture (EaselWindow *self)
{
  return self->picture;
}

int
easel_window_get_zoom (EaselWindow *self)
{
  return easel_canvas_get_zoom (self->canvas);
}

EaselTool
easel_window_get_tool (EaselWindow *self)
{
  return self->tool;
}

uint32_t
easel_window_get_foreground (EaselWindow *self)
{
  return self->foreground;
}

uint32_t
easel_window_get_background (EaselWindow *self)
{
  return self->background;
}

gboolean
easel_window_get_modified (EaselWindow *self)
{
  return easel_history_is_modified (self->history);
}

EaselCanvas *
easel_window_get_canvas (EaselWindow *self)
{
  return self->canvas;
}

EaselPaletteBar *
easel_window_get_palette_bar (EaselWindow *self)
{
  return self->palette_bar;
}

EaselStatusBar *
easel_window_get_status_bar (EaselWindow *self)
{
  return self->status_bar;
}

static void
easel_window_dispose (GObject *object)
{
  EaselWindow *self = EASEL_WINDOW (object);

  if (self->chooser_cancel != NULL)
    g_cancellable_cancel (self->chooser_cancel);
  g_clear_object (&self->chooser_cancel);
  g_clear_object (&self->pending_file);
  g_clear_object (&self->startup_file);
  g_clear_object (&self->loading_file);
  g_clear_pointer (&self->cursor_at_decode, g_free);
  if (self->canvas != NULL)
    {
      easel_canvas_set_history (self->canvas, NULL);
      easel_canvas_set_image (self->canvas, NULL);
    }
  gtk_widget_dispose_template (GTK_WIDGET (self), EASEL_TYPE_WINDOW);
  self->canvas = NULL;
  self->scroller = NULL;
  self->palette_bar = NULL;
  self->status_bar = NULL;
  self->toolbox_panel = NULL;
  self->image_button = NULL;
  self->main_menu_button = NULL;
  self->view_button = NULL;
  G_OBJECT_CLASS (easel_window_parent_class)->dispose (object);
}

static void
easel_window_finalize (GObject *object)
{
  EaselWindow *self = EASEL_WINDOW (object);

  easel_image_free (self->picture);
  easel_history_free (self->history);
  g_free (self->path);
  G_OBJECT_CLASS (easel_window_parent_class)->finalize (object);
}

enum
{
  PROP_STARTUP_FILE = 1,
  N_PROPS
};

static void
easel_window_set_property (GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
  EaselWindow *self = EASEL_WINDOW (object);

  if (prop_id == PROP_STARTUP_FILE)
    self->startup_file = g_value_dup_object (value);
  else
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
}

/* The picture is made here, not in init, because the construct-only startup file is set after init (REQ-0005-15). */
static void
easel_window_constructed (GObject *object)
{
  EaselWindow *self = EASEL_WINDOW (object);

  G_OBJECT_CLASS (easel_window_parent_class)->constructed (object);
  set_initial_size (self);
  if (self->settings.width != EASEL_SIZE_AUTO && self->settings.height != EASEL_SIZE_AUTO)
    create_picture (self, 0, 0);
  else
    {
      g_signal_connect_object (gtk_scrolled_window_get_hadjustment (self->scroller), "notify::page-size",
                               G_CALLBACK (on_page_size_changed), self, G_CONNECT_SWAPPED);
      g_signal_connect_object (gtk_scrolled_window_get_vadjustment (self->scroller), "notify::page-size",
                               G_CALLBACK (on_page_size_changed), self, G_CONNECT_SWAPPED);
    }
}

static void
easel_window_class_init (EaselWindowClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->constructed = easel_window_constructed;
  object_class->set_property = easel_window_set_property;
  object_class->dispose = easel_window_dispose;
  g_object_class_install_property (object_class, PROP_STARTUP_FILE,
                                   g_param_spec_object ("startup-file", NULL, NULL, G_TYPE_FILE,
                                                        G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS));
  object_class->finalize = easel_window_finalize;
  GTK_WINDOW_CLASS (klass)->close_request = easel_window_close_request;

  g_type_ensure (EASEL_TYPE_CANVAS);
  g_type_ensure (EASEL_TYPE_PALETTE_BAR);
  g_type_ensure (EASEL_TYPE_STATUS_BAR);
  gtk_widget_class_set_template_from_resource (widget_class, RESOURCE_PATH "/window.ui");
  gtk_widget_class_bind_template_child (widget_class, EaselWindow, scroller);
  gtk_widget_class_bind_template_child (widget_class, EaselWindow, canvas);
  gtk_widget_class_bind_template_child (widget_class, EaselWindow, palette_bar);
  gtk_widget_class_bind_template_child (widget_class, EaselWindow, status_bar);
  gtk_widget_class_bind_template_child (widget_class, EaselWindow, toolbox_panel);
  gtk_widget_class_bind_template_child (widget_class, EaselWindow, image_button);
  gtk_widget_class_bind_template_child (widget_class, EaselWindow, main_menu_button);
  gtk_widget_class_bind_template_child (widget_class, EaselWindow, view_button);
}

static void
easel_window_init (EaselWindow *self)
{
  g_autofree char *settings_path = easel_settings_file_path ();

  gtk_widget_init_template (GTK_WIDGET (self));

  easel_settings_file_load (settings_path, &self->settings);
  self->tool = EASEL_TOOL_PAN;
  self->foreground = 0xFF000000u;
  self->background = self->settings.background;

  self->history = easel_history_new ((size_t) self->settings.undo_budget_mb << 20);
  if (self->history == NULL)
    g_error ("Out of memory creating the undo history");

  g_action_map_add_action_entries (G_ACTION_MAP (self), win_actions, G_N_ELEMENTS (win_actions), self);
  gtk_popover_menu_add_child (GTK_POPOVER_MENU (gtk_menu_button_get_popover (self->view_button)), make_zoom_row (self),
                              "zoom");
  g_signal_connect_object (self->canvas, "zoom-changed", G_CALLBACK (on_zoom_changed), self, G_CONNECT_SWAPPED);
  g_signal_connect_object (self->canvas, "pointer-moved", G_CALLBACK (on_pointer_moved), self, G_CONNECT_SWAPPED);
  g_signal_connect_object (self->canvas, "pointer-left", G_CALLBACK (on_pointer_left), self, G_CONNECT_SWAPPED);
  easel_status_bar_set_hint (self->status_bar, tool_hint (self->tool));
  sync_zoom_ui (self);
  easel_canvas_set_history (self->canvas, self->history);
  easel_canvas_listen_on (self->canvas, GTK_WIDGET (self->scroller));
  easel_canvas_set_tool (self->canvas, self->tool);
  {
    g_autofree char *palette_path = easel_palette_file_path ();

    easel_palette_file_load (palette_path, &self->palette);
  }
  easel_palette_bar_set_palette (self->palette_bar, &self->palette);
  /* The grid and the panels start as the settings file says (REQ-0004-15, REQ-0004-27). */
  easel_canvas_set_grid (self->canvas, self->settings.grid, self->settings.grid_size, self->settings.grid_axonometric);
  g_simple_action_set_state (G_SIMPLE_ACTION (g_action_map_lookup_action (G_ACTION_MAP (self), "grid")),
                             g_variant_new_boolean (self->settings.grid));
  set_panel_state (self, "show-toolbox", self->toolbox_panel, self->settings.show_toolbox);
  set_panel_state (self, "show-colorbox", GTK_WIDGET (self->palette_bar), self->settings.show_colorbox);
  set_panel_state (self, "show-statusbar", GTK_WIDGET (self->status_bar), self->settings.show_statusbar);
  set_colors (self, self->foreground, self->background);
  g_signal_connect_object (self->palette_bar, "swatch-pressed", G_CALLBACK (on_swatch_pressed), self,
                           G_CONNECT_SWAPPED);
  g_signal_connect_object (self->palette_bar, "swatch-edit", G_CALLBACK (on_swatch_edit), self, G_CONNECT_SWAPPED);
  g_signal_connect_object (self->palette_bar, "indicator-pressed", G_CALLBACK (on_indicator_pressed), self,
                           G_CONNECT_SWAPPED);
  g_signal_connect_object (self->canvas, "resize-requested", G_CALLBACK (on_resize_requested), self, G_CONNECT_SWAPPED);
  {
    GtkEventController *keys = gtk_event_controller_key_new ();

    gtk_event_controller_set_propagation_phase (keys, GTK_PHASE_CAPTURE);
    g_signal_connect_swapped (keys, "key-pressed", G_CALLBACK (on_key_pressed), self);
    gtk_widget_add_controller (GTK_WIDGET (self), keys);
  }
  g_signal_connect_object (self->canvas, "stroke-finished", G_CALLBACK (on_stroke_finished), self, G_CONNECT_SWAPPED);
  g_signal_connect (self, "notify::is-active", G_CALLBACK (on_active_changed), NULL);

  sync_history_ui (self);
}

EaselWindow *
easel_window_new (GtkApplication *app)
{
  return g_object_new (EASEL_TYPE_WINDOW, "application", app, NULL);
}

EaselWindow *
easel_window_new_with_file (GtkApplication *app, GFile *file)
{
  return g_object_new (EASEL_TYPE_WINDOW, "application", app, "startup-file", file, NULL);
}
