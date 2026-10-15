/* SPDX-License-Identifier: GPL-3.0-or-later */

#include <glib/gstdio.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "canvas.h"
#include "png.h"
#include "savename.h"
#include "settings-file.h"
#include "view.h"
#include "window.h"

struct _EaselWindow
{
  AdwApplicationWindow parent_instance;

  GtkScrolledWindow *scroller;
  EaselCanvas *canvas;

  EaselSettings settings;
  EaselImage *picture;
  EaselTool tool;
  uint32_t foreground;
  uint32_t background;
  EaselHistory *history;
  char *path;             /* the picture's file; NULL until the first save */
  gboolean save_dialog_open;
  gboolean quit_prompt_open;  /* from the prompt until its choice is carried out */
};

/* Called when a save started by save () ends; saved is FALSE on failure or cancel. */
typedef void (*SaveDone) (EaselWindow *self, gboolean saved);

G_DEFINE_FINAL_TYPE (EaselWindow, easel_window, ADW_TYPE_APPLICATION_WINDOW)

static void
create_picture (EaselWindow *self, int area_width, int area_height)
{
  int width, height;

  if (self->picture != NULL)
    return;
  easel_settings_resolve_size (&self->settings, area_width, area_height, &width, &height);
  self->picture = easel_image_new (width, height, self->settings.background);
  if (self->picture == NULL)
    {
      g_critical ("Cannot allocate a %dx%d picture", width, height);
      return;
    }
  easel_canvas_set_image (self->canvas, self->picture);
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
static void
on_page_size_changed (EaselWindow *self)
{
  double w = gtk_adjustment_get_page_size (gtk_scrolled_window_get_hadjustment (self->scroller));
  double h = gtk_adjustment_get_page_size (gtk_scrolled_window_get_vadjustment (self->scroller));
  g_autoptr (GdkMonitor) monitor = NULL;
  double scale;

  if (self->picture != NULL || w <= 0 || h <= 0)
    return;
  monitor = window_monitor (self);
  scale = monitor ? gdk_monitor_get_scale (monitor) : easel_canvas_get_scale (self->canvas);
  create_picture (self, (int) (w * scale), (int) (h * scale));
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

gboolean
easel_window_save_to (EaselWindow *self, const char *path)
{
  g_autoptr (GError) error = NULL;

  if (self->picture == NULL)
    return FALSE;
  if (!easel_png_save (self->picture, path, &error))
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

static void
on_save_dialog_done (GtkFileDialog *dialog, GAsyncResult *result, SaveRequest *req)
{
  g_autoptr (GFile) file = gtk_file_dialog_save_finish (dialog, result, NULL);
  g_autofree char *chosen = file ? g_file_get_path (file) : NULL;
  char *fixed = NULL;

  req->self->save_dialog_open = FALSE;

  /* Cancelled: nothing is saved and nothing changes (REQ-0001-32). */
  if (chosen == NULL)
    {
      save_request_finish (req, FALSE);
      return;
    }

  switch (easel_savename_fix (chosen, &fixed))
    {
    case EASEL_SAVENAME_NOT_PNG:
      show_save_error (req->self, "Only PNG Can Be Saved",
                       "This version of Easel saves pictures as PNG only. Choose a name ending in .png.");
      save_request_finish (req, FALSE);
      return;
    case EASEL_SAVENAME_NO_MEMORY:
      g_critical ("Out of memory choosing a file name");
      save_request_finish (req, FALSE);
      return;
    case EASEL_SAVENAME_OK:
      break;
    }

  req->path = g_strdup (fixed);
  free (fixed);
  if (strcmp (req->path, chosen) != 0 && g_file_test (req->path, G_FILE_TEST_EXISTS))
    {
      ask_replace (req);
      return;
    }
  save_request_finish (req, easel_window_save_to (req->self, req->path));
}

/* Saves to the picture's file, or asks for one first (REQ-0001-27, REQ-0001-29). done may be NULL. */
static void
save (EaselWindow *self, SaveDone done)
{
  g_autoptr (GtkFileDialog) dialog = NULL;
  g_autoptr (GFile) folder = NULL;
  const char *pictures = g_get_user_special_dir (G_USER_DIRECTORY_PICTURES);
  SaveRequest *req;

  /* A stroke in progress is ended first and saved with the picture (REQ-0001-33). */
  easel_canvas_end_stroke (self->canvas);

  if (self->path != NULL)
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
  folder = g_file_new_for_path (pictures ? pictures : g_get_home_dir ());
  gtk_file_dialog_set_initial_folder (dialog, folder);
  gtk_file_dialog_set_initial_name (dialog, "Untitled.png");
  self->save_dialog_open = TRUE;
  gtk_file_dialog_save (dialog, GTK_WINDOW (self), NULL, (GAsyncReadyCallback) on_save_dialog_done, req);
}

static void
on_quit_save_done (EaselWindow *self, gboolean saved)
{
  self->quit_prompt_open = FALSE;
  /* Close only if the save succeeded (REQ-0001-36). */
  if (saved)
    gtk_window_destroy (GTK_WINDOW (self));
}

static void
on_quit_response (AdwAlertDialog *dialog, const char *response, EaselWindow *self)
{
  if (g_strcmp0 (response, "save") == 0)
    {
      save (self, on_quit_save_done);
      return;
    }
  self->quit_prompt_open = FALSE;
  if (g_strcmp0 (response, "discard") == 0)
    gtk_window_destroy (GTK_WINDOW (self));
}

static void
ask_quit (EaselWindow *self)
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
  g_signal_connect_object (dialog, "response", G_CALLBACK (on_quit_response), self, G_CONNECT_DEFAULT);
  self->quit_prompt_open = TRUE;
  adw_dialog_present (dialog, GTK_WIDGET (self));
}

/* Ctrl+Q, Ctrl+W and the window's close button all end here. */
static gboolean
easel_window_close_request (GtkWindow *window)
{
  EaselWindow *self = EASEL_WINDOW (window);

  /* A stroke in progress is ended first (REQ-0001-33). */
  easel_canvas_end_stroke (self->canvas);

  /* One prompt at a time (REQ-0001-38). */
  if (self->quit_prompt_open)
    return TRUE;
  if (!easel_history_is_modified (self->history))
    return FALSE;
  ask_quit (self);
  return TRUE;
}

static void
close_activated (GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
  gtk_window_close (GTK_WINDOW (user_data));
}

static void
save_activated (GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
  save (EASEL_WINDOW (user_data), NULL);
}

/* The shortcuts do nothing while the quit prompt or a save, replace or error dialog is open (REQ-0002-11). */
static gboolean
dialog_is_open (EaselWindow *self)
{
  return self->quit_prompt_open || self->save_dialog_open
         || adw_application_window_get_visible_dialog (ADW_APPLICATION_WINDOW (self)) != NULL;
}

/* Undo and redo change only the picture: the view, tool and colors stay (REQ-0002-08). */
static void
step_history (EaselWindow *self, gboolean undo)
{
  if (self->picture == NULL || dialog_is_open (self))
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
  if (!(undo ? easel_history_undo (self->history, self->picture) : easel_history_redo (self->history, self->picture)))
    return;
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

  /* A tool cannot change during a stroke (REQ-0001-09). */
  if (easel_canvas_is_stroking (self->canvas))
    return;
  if (!easel_tool_from_name (g_variant_get_string (value, NULL), &tool))
    return;
  self->tool = tool;
  easel_canvas_set_tool (self->canvas, tool);
  g_simple_action_set_state (action, value);
}

static const GActionEntry win_actions[] = {
  { .name = "tool", .parameter_type = "s", .state = "'pan'", .change_state = change_tool_state },
  { .name = "save", .activate = save_activated },
  { .name = "undo", .activate = undo_activated },
  { .name = "redo", .activate = redo_activated },
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

static void
easel_window_dispose (GObject *object)
{
  EaselWindow *self = EASEL_WINDOW (object);

  if (self->canvas != NULL)
    {
      easel_canvas_set_history (self->canvas, NULL);
      easel_canvas_set_image (self->canvas, NULL);
    }
  gtk_widget_dispose_template (GTK_WIDGET (self), EASEL_TYPE_WINDOW);
  self->canvas = NULL;
  self->scroller = NULL;
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

static void
easel_window_class_init (EaselWindowClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->dispose = easel_window_dispose;
  object_class->finalize = easel_window_finalize;
  GTK_WINDOW_CLASS (klass)->close_request = easel_window_close_request;

  g_type_ensure (EASEL_TYPE_CANVAS);
  gtk_widget_class_set_template_from_resource (widget_class, RESOURCE_PATH "/window.ui");
  gtk_widget_class_bind_template_child (widget_class, EaselWindow, scroller);
  gtk_widget_class_bind_template_child (widget_class, EaselWindow, canvas);
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
  easel_canvas_set_history (self->canvas, self->history);
  easel_canvas_listen_on (self->canvas, GTK_WIDGET (self->scroller));
  easel_canvas_set_tool (self->canvas, self->tool);
  easel_canvas_set_colors (self->canvas, self->foreground, self->background);
  g_signal_connect_object (self->canvas, "stroke-finished", G_CALLBACK (on_stroke_finished), self, G_CONNECT_SWAPPED);
  g_signal_connect (self, "notify::is-active", G_CALLBACK (on_active_changed), NULL);

  sync_history_ui (self);
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

EaselWindow *
easel_window_new (GtkApplication *app)
{
  return g_object_new (EASEL_TYPE_WINDOW, "application", app, NULL);
}
