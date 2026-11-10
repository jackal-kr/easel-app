/* SPDX-License-Identifier: GPL-3.0-or-later */

#include <glib/gstdio.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "attributes-dialog.h"
#include "frame-time.h"
#include "help.h"
#include "preferences-dialog.h"
#include "print.h"
#include "thumbnail-card.h"
#include "stretch-skew-dialog.h"
#include "black-white-dialog.h"
#include "remove-bg.h"
#include "remove-bg-card.h"
#include "stretch-skew.h"
#include "grid-dialog.h"
#include "magnifier.h"
#include "canvas.h"
#include "clipboard.h"
#include "palette-bar.h"
#include "palette-file.h"
#include "picture-file.h"
#include "format.h"
#include "settings-file.h"
#include "tool-options.h"
#include "view.h"
#include "text-render.h"
#include "transform.h"
#include "window.h"

#include "font-card.h"
#include "rich-text.h"
#include "i18n.h"

struct _EaselWindow
{
  AdwApplicationWindow parent_instance;

  EaselClipboard *clipboard;  /* the desktop's, or a test's (Plan of 0016, "The clipboard seam") */
  GtkScrolledWindow *scroller;
  GtkOverlay *canvas_overlay;
  EaselFontCard *font_card;                     /* over the view while a text box is open (REQ-0017-14) */
  EaselThumbnailCard *thumbnail_card;           /* over the view while Show Thumbnail is checked and it can show (REQ-0021-06, -11) */
  GString *announced;                           /* a test's log of what was announced, NULL if none */
  uint32_t said_foreground, said_background;
  int said_zoom;
  gboolean said_selection, said_floating, said_box;
  GtkWidget *header_bar, *toolbox;
  GtkWidget *last_toolbox, *last_options, *last_palette;   /* the control each panel last had the focus on (REQ-0022-01) */
  gboolean panel_focus_by_key;                  /* the focus came by Tab, F6 or an arrow: a panel takes its keys from an open box (REQ-0022-04) */
  gboolean grabbing_by_key;
  gboolean key_releasing, said_key_held;
  GtkWidget *text_access;                       /* unseen, while a box is open: its name and text for a screen reader (REQ-0022-14) */
  guint card_idle;
  guint card_focus_idle;
  gint64 tab_time;                              /* when Tab or Shift+Tab was last pressed: keyboard navigation */
  gsize rich_limit;                             /* tests: rich text longer than this fails as for memory; G_MAXSIZE is off */
  EaselSettings chosen;                         /* what Preferences shows: as read at start, as changed there (REQ-0020-02) */
  AdwDialog *preferences;                       /* Preferences while it is open */
  AdwDialog *extra_dialog;                      /* Keyboard Shortcuts, About Easel, Help's or a print's error, or Black and White, while it is open */
  /* Remove background (0035): from its dialog's opening to the end of its work; the input held meanwhile; the progress dialog. */
  struct RbgSession *rbg;
  GtkWidget *rbg_card;                          /* Remove Background's card while it is open (REQ-0035-04) */
  gboolean removing;
  guint rbg_timer, rbg_pulse;
  AdwDialog *rbg_progress;
  EaselPageSetup page_setup;                    /* kept until the app quits (REQ-0026-04) */
  gboolean print_dialog_open;
  gboolean put_down_failed;                     /* a shape's or a gradient's, whose dialog is on its way */
  EaselPrintRunner print_run;
  gpointer print_run_data;
  char *print_export;
  gsize print_copy_limit;
  int print_sending;                            /* jobs confirmed and not yet gone; each holds the application */
  int print_errors;                             /* failures not yet shown, one dialog each */
  guint print_error_timer;
  GtkPrintSettings *print_settings;             /* the print dialog's printer and choices, kept until the app quits (REQ-0026-13) */
  EaselHelpLauncher help_launch;
  gpointer help_launch_data;
  gboolean help_starting;                       /* the browser was asked and has not answered */
  gboolean help_dropped;                        /* a quit has begun: Help's failure is not shown (REQ-0021-04) */
  guint help_error_timer;                       /* Help's failure waits for a free moment */
  guint key_down;                               /* the key last pressed and not yet released: another press of it is a repeat */
  AdwDialog *settings_error;                    /* the error of a failed OK or Apply, while it shows (REQ-0027-12) */
  guint closed_key;                             /* the Enter or Space held as Preferences closed: its repeats do nothing */
  gint64 frame_time_shown;                      /* when the Frame time part last changed (REQ-0021-12) */
  guint frame_time_timer;
  gsize transform_limit;                        /* tests: an Image command's allocation above this fails as for memory; G_MAXSIZE is off */
  EaselCanvas *canvas;
  EaselPaletteBar *palette_bar;
  EaselStatusBar *status_bar;
  GtkWidget *toolbox_panel;   /* the tool buttons, the options below them, and their separator */
  GtkWidget *tools_column;    /* the tool grid, and below it the options panel */
  GtkWidget *toolbox_edge;    /* the band at the canvas area's left edge that widens the toolbox (REQ-0030-09) */
  int toolbox_kept;           /* the kept width, in pixels at the default text size; 0 for the default (REQ-0030-12) */
  int toolbox_shown;          /* the width the toolbox has now, in logical pixels */
  int toolbox_drag_start;     /* the shown width at the press of a drag on the edge, or -1 without one */
  gboolean toolbox_drag_moved;
  int toolbox_window_width;   /* the window's width the shown width was worked out for */
  int toolbox_default;        /* the default width, measured with the options in two columns */
  guint toolbox_follow;       /* the idle that looks at the window's new width, or 0 */
  EaselToolOptions *tool_options;
  EaselTool pick_return_tool;   /* the tool that was active when Pick Color was chosen (REQ-0007-09) */
  gboolean pick_running;        /* a pick is in progress; the two colors below are those before its press (REQ-0007-10) */
  uint32_t pick_old_foreground, pick_old_background;
  GtkMenuButton *main_menu_button;
  GMenuModel *selection_menu_model;   /* the template's selection_menu (REQ-0013-17) */
  GtkWidget *selection_menu;          /* its popover, parented to the canvas */
  GMenuModel *canvas_menu_model;      /* the template's canvas_menu (REQ-0016-14) */
  GtkWidget *canvas_menu;             /* its popover, parented to the canvas */
  GMenuModel *text_menu_model;        /* the template's text_menu (REQ-0017-22) */
  GtkWidget *text_menu;               /* its popover, parented to the canvas */
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
  gboolean reading_clipboard;     /* Paste is waiting for the clipboard (REQ-0016-15) */
  gboolean close_after_reading;   /* a quit request came while it waited */
  guint clipboard_timeout_ms;     /* the limit of that wait: 5000 (a test hook sets it) */
  gulong paint_handler;   /* the frame clock's after-paint that starts the decode */
  gint64 frame_at_request;
  char *cursor_at_decode;  /* test hook */
  int frames_before_decode;
  gboolean decode_after_paint;   /* test hook: the last decode was started by a frame's after-paint */
  gboolean prompt_open;   /* a "Save Changes?" prompt, from showing it until its choice is carried out */
  AdwDialog *paste_prompt;   /* the oversize-paste prompt while it is open (REQ-0016-13) */
  EaselImage *paste_pending; /* the picture it asks about, read when Paste was chosen */
  void (*prompt_then) (EaselWindow *self);   /* what Discard, or a Save that worked, goes on to do */

  /* The color chooser (REQ-0003-11..14): what it edits, a swatch number or one of the CHOOSE_ values. */
  gboolean chooser_open;
  int chooser_target;
  gboolean chooser_to_background;   /* a swatch's chooser opened by a secondary double-click (REQ-0003-12) */
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

static void update_clipboard_actions (EaselWindow *self);

static void sync_zoom_ui (EaselWindow *self);
static void update_title (EaselWindow *self);
static void show_open_error (EaselWindow *self, GFile *file, EaselOpenResult result, int width, int height);
static gboolean dialog_is_open (EaselWindow *self);
static void show_memory_error (EaselWindow *self, const char *heading, int width, int height);
static gboolean view_actions_allowed (EaselWindow *self);
static gboolean zoom_allowed (EaselWindow *self);
static void scroll_view_as_scroller (EaselWindow *self, GtkEventControllerScroll *controller, double dx, double dy);

static gboolean widget_within (GtkWidget *w, GtkWidget *panel);

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
  update_clipboard_actions (self);
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
/* The visible canvas area less its two margins, in device pixels, from the adjustments' page sizes; false until it
   is known. */
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
  /* Less the two margins around the picture, so that an auto picture still shows without scrollbars (REQ-0001-02 as
     amended by REQ-0009-01). */
  *width = (int) ((w - 2 * EASEL_VIEW_MARGIN) * scale);
  *height = (int) ((h - 2 * EASEL_VIEW_MARGIN) * scale);
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
  g_autofree char *name = self->path ? g_path_get_basename (self->path) : g_strdup (_ ("Untitled"));
  g_autofree char *title = g_strdup_printf (easel_history_is_modified (self->history) ? _ ("• %s – Easel") : _ ("%s – Easel"),
                                            name);

  gtk_window_set_title (GTK_WINDOW (self), title);
}

/* The card's text field that has the focus (the size, the family's search), or NULL. */
static GtkWidget *
card_field_focus (EaselWindow *self)
{
  GtkWidget *focus = gtk_root_get_focus (GTK_ROOT (self));

  return focus != NULL && GTK_IS_TEXT (focus) && gtk_widget_is_ancestor (focus, GTK_WIDGET (self->font_card)) ? focus : NULL;
}

/* While a field of the card has the focus, Undo, Redo, Select All, Cut, Copy and Paste act on that field (REQ-0017-14): TRUE if one
   took the action. */
static gboolean
card_field_action (EaselWindow *self, const char *action)
{
  GtkWidget *field = card_field_focus (self);

  return field != NULL && gtk_widget_activate_action (field, action, NULL);
}

/* The card's place in the overlay (REQ-0017-14): above the box with a gap of 8 logical pixels, else below it, else at the view's top
   edge; shifted sideways to stay in the view. FALSE with no box; `in_view` FALSE when the box is wholly out of the view. */
static gboolean
card_place (EaselWindow *self, GdkRectangle *place, gboolean *in_view)
{
  GtkWidget *card = GTK_WIDGET (self->font_card), *overlay = GTK_WIDGET (self->canvas_overlay);
  double l = easel_window_get_zoom (self) / 100.0 / easel_canvas_get_scale (self->canvas);
  graphene_rect_t view;
  graphene_point_t a, b;
  int x, y, w, h, cw, ch;
  double cx, cy, above, below;

  if (!easel_canvas_text_box (self->canvas, &x, &y, &w, &h)
      || !gtk_widget_compute_bounds (GTK_WIDGET (self->scroller), overlay, &view)
      || !gtk_widget_compute_point (GTK_WIDGET (self->canvas), overlay, &GRAPHENE_POINT_INIT ((float) (x * l), (float) (y * l)), &a)
      || !gtk_widget_compute_point (GTK_WIDGET (self->canvas), overlay,
                                    &GRAPHENE_POINT_INIT ((float) ((x + w) * l), (float) ((y + h) * l)), &b))
    return FALSE;
  gtk_widget_measure (card, GTK_ORIENTATION_HORIZONTAL, -1, NULL, &cw, NULL, NULL);
  gtk_widget_measure (card, GTK_ORIENTATION_VERTICAL, cw, NULL, &ch, NULL, NULL);
  *in_view = b.x > view.origin.x && a.x < view.origin.x + view.size.width && b.y > view.origin.y
             && a.y < view.origin.y + view.size.height;
  cx = MAX (MIN (a.x, view.origin.x + view.size.width - cw), view.origin.x);
  above = a.y - 8 - ch;
  below = b.y + 8;
  cy = above >= view.origin.y ? above : below + ch <= view.origin.y + view.size.height ? below : view.origin.y;
  *place = (GdkRectangle) { (int) floor (cx + 0.5), (int) floor (cy + 0.5), cw, ch };
  return TRUE;
}

static gboolean
on_card_position (GtkOverlay *overlay, GtkWidget *widget, GdkRectangle *allocation, EaselWindow *self)
{
  gboolean in_view;

  return widget == GTK_WIDGET (self->font_card) && card_place (self, allocation, &in_view);
}

/* Whether the focus is in the font card where the keys must not stay (REQ-0017-14; JOD, 2026-10-27: the card's controls take no
   focus): on a control that is not a text field, or on a text field it reached by Tab, with the family popover closed. A text field the
   user clicked into keeps it while the user types there. */
static gboolean
focus_stuck_in_card (EaselWindow *self)
{
  GtkWidget *focus = gtk_root_get_focus (GTK_ROOT (self));
  gboolean by_tab = g_get_monotonic_time () - self->tab_time < G_USEC_PER_SEC / 2;

  return focus != NULL && gtk_widget_is_ancestor (focus, GTK_WIDGET (self->font_card)) && (!GTK_IS_TEXT (focus) || by_tab)
         && !easel_font_card_family_is_open (self->font_card);
}

static gboolean
card_focus_off (gpointer data)
{
  EaselWindow *self = data;

  self->card_focus_idle = 0;
  if (focus_stuck_in_card (self))
    gtk_root_set_focus (GTK_ROOT (self), NULL);
  return G_SOURCE_REMOVE;
}

/* The focus came where it must not stay in the card: GTK gives it back to the family button's side when the popover closes, and on the
   desktop also later, when the window gets the keys back from the popover, and Tab can carry it in; then the keys go to the box again and
   no control keeps a focus ring (REQ-0017-14; JOD's M1 recording, 2026-10-27). */
static void
on_focus_widget_changed (EaselWindow *self)
{
  {
    GtkWidget *focus = gtk_root_get_focus (GTK_ROOT (self));

    if (widget_within (focus, self->toolbox))
      self->last_toolbox = focus;
    else if (widget_within (focus, GTK_WIDGET (self->tool_options)))
      self->last_options = focus;
    else if (widget_within (focus, GTK_WIDGET (self->palette_bar)))
      self->last_palette = focus;
    if (!self->grabbing_by_key)
      self->panel_focus_by_key = FALSE;
    /* The marker shows when the keyboard brings the focus to the canvas (REQ-0022-05), at the view's top left pixel the first time. */
    if (focus == GTK_WIDGET (self->canvas) && self->grabbing_by_key && self->picture != NULL)
      {
        if (!easel_canvas_marker_is_set (self->canvas))
          {
            int zoom = easel_canvas_get_zoom (self->canvas);
            double scale = easel_canvas_get_scale (self->canvas);

            easel_canvas_marker_set (self->canvas,
                                     MIN (easel_view_first_cell (gtk_adjustment_get_value (gtk_scrolled_window_get_hadjustment (self->scroller)), EASEL_VIEW_MARGIN, zoom, scale), self->picture->width - 1),
                                     MIN (easel_view_first_cell (gtk_adjustment_get_value (gtk_scrolled_window_get_vadjustment (self->scroller)), EASEL_VIEW_MARGIN, zoom, scale), self->picture->height - 1));
          }
        easel_canvas_marker_show (self->canvas, TRUE);
      }
    /* A menu of the canvas holds the focus inside it: the marker stays through it. */
    else if (!widget_within (focus, GTK_WIDGET (self->canvas)))
      easel_canvas_marker_show (self->canvas, FALSE);
  }
  if (focus_stuck_in_card (self) && self->card_focus_idle == 0)
    self->card_focus_idle = g_idle_add (card_focus_off, self);
}

/* After the layout: the card shown only while the box is open and in the view (REQ-0017-14), and placed again. */
static gboolean
card_after_layout (gpointer data)
{
  EaselWindow *self = data;
  GdkRectangle place;
  gboolean in_view = FALSE;

  self->card_idle = 0;
  gtk_widget_set_child_visible (GTK_WIDGET (self->font_card), card_place (self, &place, &in_view) && in_view);
  gtk_widget_queue_allocate (GTK_WIDGET (self->canvas_overlay));
  return G_SOURCE_REMOVE;
}

/* The card follows the box: its style, and its place after the layout (a scroll, a zoom, a move or a resize of the box). */
static void
update_card (EaselWindow *self)
{
  int x, y, w, h;
  gboolean open = self->tool == EASEL_TOOL_TEXT && easel_canvas_text_box (self->canvas, &x, &y, &w, &h);

  gtk_widget_set_visible (GTK_WIDGET (self->font_card), open);
  if (open)
    {
      EaselTextStyle st = { 0 };
      unsigned mixed = 0;

      easel_canvas_text_selection_style (self->canvas, &st, &mixed);
      easel_font_card_show_style (self->font_card, &st, mixed);
    }
  if (self->card_idle == 0)
    self->card_idle = g_idle_add (card_after_layout, self);
}

/* The title and the Undo and Redo actions follow the history (REQ-0002-06, REQ-0002-07, REQ-0002-12). */
static void
sync_history_ui (EaselWindow *self)
{
  GAction *undo = g_action_map_lookup_action (G_ACTION_MAP (self), "undo");
  GAction *redo = g_action_map_lookup_action (G_ACTION_MAP (self), "redo");
  gboolean pending = easel_canvas_is_pending (self->canvas);
  int x, y, w, h;
  gboolean box = easel_canvas_text_box (self->canvas, &x, &y, &w, &h);

  /* While a curve or polygon is pending, Undo is there to drop it, even with nothing to undo, and Redo is not (REQ-0011-11).
     While a text box is open, Undo takes back its edits or closes it, and Redo redoes its edits only (REQ-0017-08). */
  if (undo != NULL)
    g_simple_action_set_enabled (G_SIMPLE_ACTION (undo), pending || box || easel_history_can_undo (self->history));
  if (redo != NULL)
    g_simple_action_set_enabled (G_SIMPLE_ACTION (redo),
                                 box ? easel_canvas_text_can_redo (self->canvas) : !pending && easel_history_can_redo (self->history));
  update_title (self);
}

/* Puts down what is open (a text box, a floating selection, a pending curve or polygon). FALSE only when a text box could not be put
   down: it stays open and the action that asked for it is not done (REQ-0017-18); a selection that could not be put down lets the
   action go on (REQ-0013-19). */
static gboolean
put_down_for_action (EaselWindow *self, gboolean keep_marked)
{
  int x, y, w, h;
  gboolean ok = keep_marked ? easel_canvas_put_down_keeping_marked (self->canvas) : easel_canvas_put_down (self->canvas);

  return ok || !easel_canvas_text_box (self->canvas, &x, &y, &w, &h);
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

  adw_alert_dialog_add_response (ADW_ALERT_DIALOG (dialog), "close", _ ("Close"));
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
      g_autofree char *heading = g_strdup_printf (_ ("Could Not Save “%s”"), name);
      g_autofree char *body = g_strdup_printf (_ ("“%s” could not be saved: %s"), path, error->message);

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
  {
    g_autofree char *name = g_path_get_basename (self->path);
    g_autofree char *text = g_strdup_printf (_ ("Saved %s"), name);

    easel_window_announce (self, text);
  }
  return TRUE;
}

typedef struct
{
  EaselWindow *self;      /* owned reference while the dialog is open */
  char *path;
  SaveDone done;
  EaselImage *copy;       /* Copy To… (REQ-0016-09): the selection's copy to write, instead of the picture; owned */
} SaveRequest;

static void
save_request_finish (SaveRequest *req, gboolean saved)
{
  if (req->done != NULL)
    req->done (req->self, saved);
  g_object_unref (req->self);
  g_free (req->path);
  easel_image_free (req->copy);
  g_free (req);
}

/* The copy of the selection written to path as Save As writes the picture (REQ-0016-09, -12); the picture, its file, title, modified
   mark and history are not touched; a failure is the save error of REQ-0001-31. */
static gboolean
write_copy (EaselWindow *self, const EaselImage *copy, const char *path)
{
  g_autoptr (GError) error = NULL;

  if (!easel_picture_save (copy, path, save_format (path), self->settings.jpeg_quality, &error))
    {
      g_autofree char *name = g_path_get_basename (path);
      g_autofree char *heading = g_strdup_printf (_ ("Could Not Save “%s”"), name);
      g_autofree char *body = g_strdup_printf (_ ("“%s” could not be saved: %s"), path, error->message);

      show_save_error (self, heading, body);
      return FALSE;
    }
  return TRUE;
}

/* What a save request writes: the picture, or for Copy To… the copy. */
static gboolean
write_request (SaveRequest *req)
{
  return req->copy != NULL ? write_copy (req->self, req->copy, req->path) : easel_window_save_to (req->self, req->path);
}

static void
on_replace_response (AdwAlertDialog *dialog, GAsyncResult *result, SaveRequest *req)
{
  const char *response = adw_alert_dialog_choose_finish (dialog, result);
  gboolean saved = FALSE;

  if (g_strcmp0 (response, "replace") == 0)
    saved = write_request (req);
  save_request_finish (req, saved);
}

/* The dialog confirmed the name as typed; once .png is added, an existing file needs its own question
   (REQ-0001-42). */
static void
ask_replace (SaveRequest *req)
{
  g_autofree char *name = g_path_get_basename (req->path);
  g_autofree char *heading = g_strdup_printf (_ ("Replace “%s”?"), name);
  AdwDialog *dialog = adw_alert_dialog_new (heading, _ ("A file with this name already exists."));

  adw_alert_dialog_add_responses (ADW_ALERT_DIALOG (dialog), "cancel", _ ("_Cancel"), "replace", _ ("_Replace"), NULL);
  adw_alert_dialog_set_response_appearance (ADW_ALERT_DIALOG (dialog), "replace", ADW_RESPONSE_DESTRUCTIVE);
  adw_alert_dialog_set_default_response (ADW_ALERT_DIALOG (dialog), "cancel");
  adw_alert_dialog_set_close_response (ADW_ALERT_DIALOG (dialog), "cancel");
  adw_alert_dialog_choose (ADW_ALERT_DIALOG (dialog), GTK_WIDGET (req->self), NULL,
                           (GAsyncReadyCallback) on_replace_response, req);
}

static void open_save_chooser (EaselWindow *self, SaveRequest *req, const char *title, const char *path, EaselFormat format);

/* Only the name the dialog returned can be written in the sandbox: the portal grants that file and no other (REQ-0024-10). */
static gboolean
in_sandbox (void)
{
  return g_getenv ("EASEL_TEST_SANDBOX") != NULL || g_file_test ("/.flatpak-info", G_FILE_TEST_EXISTS);
}

/* The name the user typed or chose, with the rules of REQ-0024-12, REQ-0005-10 and REQ-0001-42; ends the request, unless the dialog
   has to open again. EASEL_FORMAT_UNKNOWN is By extension. */
static void
save_chosen (SaveRequest *req, const char *chosen, EaselFormat chosen_format)
{
  char *fixed = NULL;
  EaselFormat format;

  if (chosen_format != EASEL_FORMAT_UNKNOWN)
    {
      if (!easel_format_name_for (chosen, chosen_format, &fixed))
        {
          g_critical ("Out of memory choosing a file name");
          save_request_finish (req, FALSE);
          return;
        }
    }
  else
    switch (easel_format_fix_name (chosen, &fixed, &format))
      {
      case EASEL_FORMAT_UNKNOWN_EXTENSION:
        show_save_error (req->self, _ ("Cannot Save This Kind of File"), _ ("Pictures can be saved as PNG, JPEG, BMP, TIFF or WebP."));
        save_request_finish (req, FALSE);
        return;
      case EASEL_FORMAT_NO_MEMORY:
        g_critical ("Out of memory choosing a file name");
        save_request_finish (req, FALSE);
        return;
      case EASEL_FORMAT_OK:
        break;
      }

  g_free (req->path);
  req->path = g_strdup (fixed);
  free (fixed);
  if (strcmp (req->path, chosen) != 0)
    {
      if (in_sandbox ())
        {
          g_autofree char *again = g_strdup (req->path);

          open_save_chooser (req->self, req, req->copy != NULL ? _ ("Copy To") : _ ("Save Picture As"), again, chosen_format);
          return;
        }
      /* The system's dialog confirmed the name as typed; once it has changed, an existing file needs its own question
         (REQ-0001-42). */
      if (g_file_test (req->path, G_FILE_TEST_EXISTS))
        {
          ask_replace (req);
          return;
        }
    }
  save_request_finish (req, write_request (req));
}

void
easel_window_save_chosen_as (EaselWindow *self, const char *path, EaselFormat format)
{
  SaveRequest *req = g_new0 (SaveRequest, 1);

  req->self = g_object_ref (self);
  save_chosen (req, path, format);
}

void
easel_window_save_chosen (EaselWindow *self, const char *path)
{
  easel_window_save_chosen_as (self, path, EASEL_FORMAT_UNKNOWN);
}

static void show_copy_error (EaselWindow *self);

/* Copy To… with a name chosen (REQ-0016-09): the copy is made now, so the selection may change while a replace question waits. */
void
easel_window_copy_to_chosen_as (EaselWindow *self, const char *path, EaselFormat format)
{
  SaveRequest *req;
  EaselImage *copy = self->picture != NULL ? easel_canvas_copy_selection (self->canvas) : NULL;

  if (copy == NULL)
    {
      int x, y, w, h;
      gboolean floating;

      if (self->picture != NULL && easel_canvas_get_selection (self->canvas, &x, &y, &w, &h, &floating))
        show_copy_error (self);                  /* a selection but no memory for its copy: nothing is written (REQ-0016-11) */
      return;
    }
  req = g_new0 (SaveRequest, 1);
  req->self = g_object_ref (self);
  req->copy = copy;
  save_chosen (req, path, format);
}

void
easel_window_copy_to_chosen (EaselWindow *self, const char *path)
{
  easel_window_copy_to_chosen_as (self, path, EASEL_FORMAT_UNKNOWN);
}

/* The save dialog, with its file type list (REQ-0024-12). GtkFileDialog does not say which entry was chosen (a probe, 2026-11-05), and
   this one does; it is deprecated, so all of it is here. */
G_GNUC_BEGIN_IGNORE_DEPRECATIONS

typedef struct
{
  EaselWindow *self;
  SaveRequest *req;                              /* NULL for Copy To's first answer: the copy is made then */
} SaveChooser;

static gboolean
release_native (gpointer native)
{
  g_object_unref (native);
  return G_SOURCE_REMOVE;
}

static void
on_save_chooser_response (GtkNativeDialog *native, int response, SaveChooser *c)
{
  EaselWindow *self = c->self;
  SaveRequest *req = c->req;
  g_autoptr (GFile) file = response == GTK_RESPONSE_ACCEPT ? gtk_file_chooser_get_file (GTK_FILE_CHOOSER (native)) : NULL;
  g_autofree char *chosen = file ? g_file_get_path (file) : NULL;
  GtkFileFilter *filter = gtk_file_chooser_get_filter (GTK_FILE_CHOOSER (native));
  /* A dialog that reports no choice is By extension. */
  EaselFormat format = filter != NULL ? (EaselFormat) GPOINTER_TO_INT (g_object_get_data (G_OBJECT (filter), "easel-format")) : EASEL_FORMAT_UNKNOWN;

  self->save_dialog_open = FALSE;
  g_free (c);
  g_idle_add_full (G_PRIORITY_DEFAULT_IDLE, release_native, native, NULL);     /* not from inside its own response */
  /* Cancelled: nothing is saved and nothing changes (REQ-0001-32). */
  if (chosen == NULL)
    {
      if (req != NULL)
        save_request_finish (req, FALSE);
    }
  else if (req != NULL)
    save_chosen (req, chosen, format);
  else
    easel_window_copy_to_chosen_as (self, chosen, format);
  g_object_unref (self);
}

/* Opens the dialog on path, with format chosen in its list; the answer goes on with req. */
static void
open_save_chooser (EaselWindow *self, SaveRequest *req, const char *title, const char *path, EaselFormat format)
{
  static const struct { const char *label; EaselFormat format; const char *suffix[2]; } kinds[] = {
    { N_ ("By extension"), EASEL_FORMAT_UNKNOWN, { NULL, NULL } },
    { "BMP (*.bmp)", EASEL_FORMAT_BMP, { "bmp", NULL } },
    { "JPEG (*.jpg, *.jpeg)", EASEL_FORMAT_JPEG, { "jpg", "jpeg" } },
    { "PNG (*.png)", EASEL_FORMAT_PNG, { "png", NULL } },
    { "TIFF (*.tif, *.tiff)", EASEL_FORMAT_TIFF, { "tif", "tiff" } },
    { "WebP (*.webp)", EASEL_FORMAT_WEBP, { "webp", NULL } },
  };
  static const char *const every[] = { "bmp", "jpg", "jpeg", "png", "tif", "tiff", "webp" };
  GtkFileChooserNative *native = gtk_file_chooser_native_new (title, GTK_WINDOW (self), GTK_FILE_CHOOSER_ACTION_SAVE, NULL, NULL);
  g_autofree char *folder_path = g_path_get_dirname (path);
  g_autofree char *name = g_path_get_basename (path);
  g_autoptr (GFile) folder = g_file_new_for_path (folder_path);
  SaveChooser *c = g_new0 (SaveChooser, 1);

  for (gsize i = 0; i < G_N_ELEMENTS (kinds); i++)
    {
      g_autoptr (GtkFileFilter) filter = gtk_file_filter_new ();

      gtk_file_filter_set_name (filter, kinds[i].format == EASEL_FORMAT_UNKNOWN ? _ (kinds[i].label) : kinds[i].label);
      for (gsize k = 0; k < 2 && kinds[i].suffix[k] != NULL; k++)
        gtk_file_filter_add_suffix (filter, kinds[i].suffix[k]);
      if (kinds[i].format == EASEL_FORMAT_UNKNOWN)
        for (gsize k = 0; k < G_N_ELEMENTS (every); k++)
          gtk_file_filter_add_suffix (filter, every[k]);
      g_object_set_data (G_OBJECT (filter), "easel-format", GINT_TO_POINTER (kinds[i].format));
      gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (native), filter);
      if (kinds[i].format == format)
        gtk_file_chooser_set_filter (GTK_FILE_CHOOSER (native), filter);
    }
  gtk_native_dialog_set_modal (GTK_NATIVE_DIALOG (native), TRUE);
  gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (native), folder, NULL);
  gtk_file_chooser_set_current_name (GTK_FILE_CHOOSER (native), name);
  c->self = g_object_ref (self);
  c->req = req;
  self->save_dialog_open = TRUE;                 /* a quit request behaves as for Save As (REQ-0001-43) */
  g_signal_connect (native, "response", G_CALLBACK (on_save_chooser_response), c);
  gtk_native_dialog_show (GTK_NATIVE_DIALOG (native));
}

G_GNUC_END_IGNORE_DEPRECATIONS

/* Copy To… (REQ-0016-09): a save dialog titled "Copy To", suggesting Untitled.png in the folder of the picture's file or the Pictures
   folder. */
static void
copy_to_activated (GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
  EaselWindow *self = user_data;
  g_autoptr (GFile) folder = NULL;
  const char *pictures = g_get_user_special_dir (G_USER_DIRECTORY_PICTURES);
  g_autofree char *start = NULL, *in = NULL;

  if (self->picture == NULL || dialog_is_open (self) || easel_canvas_is_stroking (self->canvas)
      || easel_canvas_is_resizing (self->canvas) || !g_action_group_get_action_enabled (G_ACTION_GROUP (self), "copy-to"))
    return;
  if (self->path != NULL)
    {
      g_autoptr (GFile) current = g_file_new_for_path (self->path);

      folder = g_file_get_parent (current);
    }
  if (folder == NULL || !g_file_query_exists (folder, NULL))
    {
      g_clear_object (&folder);
      folder = g_file_new_for_path (pictures && g_file_test (pictures, G_FILE_TEST_IS_DIR) ? pictures : g_get_home_dir ());
    }
  in = g_file_get_path (folder);
  start = g_build_filename (in, _ ("Untitled.png"), NULL);
  open_save_chooser (self, NULL, _ ("Copy To"), start, EASEL_FORMAT_UNKNOWN);
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
  g_autofree char *start = NULL;
  const char *pictures = g_get_user_special_dir (G_USER_DIRECTORY_PICTURES);
  SaveRequest *req;

  /* A stroke in progress is ended first and saved with the picture (REQ-0001-33); a pending curve or polygon was put down
     by file_action_allowed, which every way into this function goes through (REQ-0011-12). */
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

  /* The file's own name, or its name as a .png when it cannot be written as it is; else Untitled.png in the Pictures folder. */
  if (self->path != NULL)
    start = easel_format_of_name (self->path) != EASEL_FORMAT_UNKNOWN ? g_strdup (self->path) : png_name_for (self->path);
  else
    start = g_build_filename (pictures ? pictures : g_get_home_dir (), _ ("Untitled.png"), NULL);
  open_save_chooser (self, req, _ ("Save Picture As"), start, EASEL_FORMAT_UNKNOWN);
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
  g_autofree char *name = self->path ? g_path_get_basename (self->path) : g_strdup (_ ("Untitled"));
  g_autofree char *body = g_strdup_printf (_ ("“%s” has unsaved changes."), name);
  AdwDialog *dialog = adw_alert_dialog_new (_ ("Save Changes?"), body);

  adw_alert_dialog_add_responses (ADW_ALERT_DIALOG (dialog),
                                  "cancel", _ ("_Cancel"), "discard", _ ("_Discard"), "save", _ ("_Save"), NULL);
  adw_alert_dialog_set_response_appearance (ADW_ALERT_DIALOG (dialog), "discard", ADW_RESPONSE_DESTRUCTIVE);
  adw_alert_dialog_set_response_appearance (ADW_ALERT_DIALOG (dialog), "save", ADW_RESPONSE_SUGGESTED);
  adw_alert_dialog_set_default_response (ADW_ALERT_DIALOG (dialog), "save");
  adw_alert_dialog_set_close_response (ADW_ALERT_DIALOG (dialog), "cancel");
  g_signal_connect_object (dialog, "response", G_CALLBACK (on_prompt_response), self, G_CONNECT_DEFAULT);
  self->prompt_open = TRUE;
  self->prompt_then = then;
  adw_dialog_present (dialog, GTK_WIDGET (self));
}

/* The answer of a call that a wait may give up on (Check F1 of 0016): a cancelled call can still call back from the main loop after the
   wait has returned, so the answer lives on the heap and the last of the callback and the wait frees it. */
typedef struct
{
  GAsyncResult *result;
  gboolean given_up;
} Answer;

static void
on_answer (GObject *source, GAsyncResult *result, gpointer data)
{
  Answer *answer = data;

  if (answer->given_up)
    g_free (answer);                            /* the wait has gone: nothing to keep */
  else
    answer->result = g_object_ref (result);
}

/* The wait is over: the result if it came (the caller's to unref) and the answer freed; otherwise NULL, the answer left to its callback. */
static GAsyncResult *
answer_take (Answer *answer)
{
  GAsyncResult *result = answer->result;

  if (result == NULL)
    answer->given_up = TRUE;
  else
    g_free (answer);
  return result;
}

/* The store at quit (Plan of 0016): when the window's own copy is on the clipboard, the desktop's clipboard manager is asked to keep it,
   waited for on the spot for at most 2 seconds; the quit goes on whatever the answer (with none, the copy is gone after it). */
static void
store_clipboard_for_quit (EaselWindow *self)
{
  g_autoptr (GCancellable) cancellable = NULL;
  Answer *answer;
  GAsyncResult *result;
  gint64 deadline;

  /* Whatever the window put there is kept (a picture now, text and rich text with 0017); another app's content is not ours to store. */
  if (self->clipboard == NULL || !easel_clipboard_is_local (self->clipboard))
    return;
  cancellable = g_cancellable_new ();
  answer = g_new0 (Answer, 1);
  easel_clipboard_store_async (self->clipboard, cancellable, on_answer, answer);
  deadline = g_get_monotonic_time () + 2 * G_USEC_PER_SEC;
  while (answer->result == NULL && g_get_monotonic_time () < deadline)
    g_main_context_iteration (NULL, FALSE);
  result = answer_take (answer);
  if (result == NULL)
    g_cancellable_cancel (cancellable);
  else
    {
      easel_clipboard_store_finish (self->clipboard, result, NULL);
      g_object_unref (result);
    }
}

static void
quit_now (EaselWindow *self)
{
  store_clipboard_for_quit (self);
  gtk_window_destroy (GTK_WINDOW (self));
}

/* An emission hook, since the handler libadwaita connects at the window's start stops a close request while a dialog is open, ahead of any handler of ours.
   Preferences closes, what is not kept dropped (REQ-0027-11), so do Keyboard Shortcuts, About Easel and Help's error (REQ-0021-03), and the quit goes on;
   a failure of Help not yet shown is dropped (REQ-0021-04). */
static void rbg_cancel (EaselWindow *self);

static gboolean
close_request_hook (GSignalInvocationHint *hint, guint n_values, const GValue *values, gpointer data)
{
  GObject *window = g_value_get_object (&values[0]);

  if (EASEL_IS_WINDOW (window))
    {
      EaselWindow *self = EASEL_WINDOW (window);

      if (self->settings_error != NULL)
        adw_dialog_force_close (self->settings_error);
      if (self->preferences != NULL)
        adw_dialog_force_close (self->preferences);
      if (self->extra_dialog != NULL)
        adw_dialog_force_close (self->extra_dialog);
      if (self->rbg_card != NULL)
        easel_remove_bg_card_answer (self->rbg_card, FALSE);   /* Remove Background's card closes as Cancel (REQ-0035-12) */
      rbg_cancel (self);                          /* Remove background: its work ends, nothing applied (REQ-0035-11) */
      self->help_dropped = TRUE;
      g_clear_handle_id (&self->help_error_timer, g_source_remove);
    }
  return TRUE;
}

/* Ctrl+Q, Ctrl+W and the window's close button all end here. */
static gboolean
easel_window_close_request (GtkWindow *window)
{
  EaselWindow *self = EASEL_WINDOW (window);

  /* While Open's or Paste From's file dialog is open a quit request does nothing, and so puts nothing down (REQ-0005-02, REQ-0016-08); so
     with the print dialog (REQ-0026-16). */
  if (self->open_dialog_open || self->print_dialog_open)
    return TRUE;
  /* A stroke in progress is ended first (REQ-0001-33), a pending curve or polygon put down (REQ-0011-12); an open color
     chooser closes as Cancel (REQ-0003-14). */
  easel_canvas_end_stroke (self->canvas);
  if (!easel_canvas_put_down (self->canvas))
    return TRUE;                                /* a selection could not be put down: the quit stops, the dialog tells why (REQ-0013-19) */
  easel_canvas_cancel_resize (self->canvas);
  easel_window_finish_color_choice (self, NULL);

  /* A quit during loading waits for the picture, then goes on; while the Open dialog is open it does nothing
     (REQ-0005-02). */
  if (self->loading)
    {
      self->close_after_loading = TRUE;
      return TRUE;
    }
  if (self->reading_clipboard)
    {
      self->close_after_reading = TRUE;           /* handled after the read (REQ-0016-15) */
      return TRUE;
    }
  /* One prompt at a time (REQ-0001-38). */
  if (self->prompt_open)
    return TRUE;
  if (!easel_history_is_modified (self->history))
    {
      store_clipboard_for_quit (self);
      return FALSE;
    }
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
  if (path != NULL)
    {
      g_autofree char *name = g_path_get_basename (path);
      g_autofree char *text = g_strdup_printf (_ ("Opened %s"), name);

      easel_window_announce (self, text);
    }
  easel_canvas_set_history (self->canvas, history);
  easel_canvas_set_image (self->canvas, img);
  easel_canvas_marker_reset (self->canvas);
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
    show_memory_error (self, _ ("Could Not Create a New Picture"), w, h);
}

/* New, Open, Save and Save As: nothing before the start-up picture exists or while a dialog or prompt is open; a stroke
   in progress ends first and a resize drag is cancelled (REQ-0005-02). */
static gboolean
file_action_allowed_keeping (EaselWindow *self, gboolean keep_marked)
{
  if (self->picture == NULL || dialog_is_open (self))
    return FALSE;
  easel_canvas_end_stroke (self->canvas);
  /* A pending curve or polygon too (REQ-0011-12); Save and Save As keep a marked selection (REQ-0013-12). */
  if (!put_down_for_action (self, keep_marked))
    return FALSE;                               /* a text box that could not be put down: no dialog, no prompt (REQ-0017-18) */
  easel_canvas_cancel_resize (self->canvas);
  return TRUE;
}

static gboolean
file_action_allowed (EaselWindow *self)
{
  return file_action_allowed_keeping (self, FALSE);
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
  g_autofree char *heading = g_strdup_printf (_ ("Could Not Open “%s”"), name);

  switch (result)
    {
    case EASEL_OPEN_UNREADABLE:
      show_save_error (self, heading, _ ("The file cannot be read."));
      break;
    case EASEL_OPEN_NOT_PICTURE:
      show_save_error (self, heading, _ ("This is not a picture the app can open."));
      break;
    case EASEL_OPEN_TOO_BIG:
      show_save_error (self, heading, _ ("The picture is bigger than 10000 pixels in width or height."));
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
  self->decode_after_paint = TRUE;
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
  self->decode_after_paint = FALSE;
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

/* A file dialog for pictures with Open's folders and filters (REQ-0005-05), titled title: Open's, and Paste From's (REQ-0016-08). */
GtkFileDialog *
easel_window_picture_dialog (EaselWindow *self, const char *title)
{
  GtkFileDialog *dialog = gtk_file_dialog_new ();
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
  gtk_file_filter_set_name (pictures, _ ("Pictures"));
  for (gsize i = 0; i < G_N_ELEMENTS (mime_types); i++)
    gtk_file_filter_add_mime_type (pictures, mime_types[i]);
  gtk_file_filter_set_name (all, _ ("All Files"));
  gtk_file_filter_add_pattern (all, "*");
  g_list_store_append (filters, pictures);
  g_list_store_append (filters, all);
  gtk_file_dialog_set_title (dialog, title);
  gtk_file_dialog_set_filters (dialog, G_LIST_MODEL (filters));
  gtk_file_dialog_set_default_filter (dialog, pictures);
  gtk_file_dialog_set_initial_folder (dialog, folder);
  return dialog;
}

static void
show_open_dialog (EaselWindow *self)
{
  g_autoptr (GtkFileDialog) dialog = easel_window_picture_dialog (self, _ ("Open Picture"));

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

/* Test-only getter (see window.h). */
gboolean
easel_window_get_decode_after_paint (EaselWindow *self)
{
  return self->decode_after_paint;
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

  if (file_action_allowed_keeping (self, TRUE))
    save (self, NULL, FALSE);
}

static void
save_as_activated (GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
  EaselWindow *self = user_data;

  if (file_action_allowed_keeping (self, TRUE))
    save (self, NULL, TRUE);
}

/* The shortcuts do nothing while the quit prompt or a save, replace or error dialog is open (REQ-0002-11). */
static gboolean
dialog_is_open (EaselWindow *self)
{
  return self->prompt_open || self->save_dialog_open || self->open_dialog_open || self->loading || self->reading_clipboard
         || self->chooser_open || self->print_dialog_open || self->removing || self->rbg_card != NULL
         || adw_application_window_get_visible_dialog (ADW_APPLICATION_WINDOW (self)) != NULL;
}

/* While a dialog is open, a press on the canvas does nothing (REQ-0010-17; the dialog is modal for the pointer, this holds
   it for a press that reaches the canvas anyway). */
static void
on_visible_dialog_changed (EaselWindow *self)
{
  easel_canvas_set_input_blocked (self->canvas, self->chooser_open || self->loading || self->removing || self->rbg_card != NULL
                                  || adw_application_window_get_visible_dialog (ADW_APPLICATION_WINDOW (self)) != NULL);
}

/* Colors are not edits: no undo step, no modified mark (REQ-0003-09). */
static void announce_colors (EaselWindow *self);

static void
set_colors (EaselWindow *self, uint32_t foreground, uint32_t background)
{
  self->foreground = foreground;
  self->background = background;
  easel_canvas_set_colors (self->canvas, foreground, background);
  easel_palette_bar_set_colors (self->palette_bar, foreground, background);
  announce_colors (self);
}

static void open_chooser (EaselWindow *self, int target, uint32_t color);

/* A color pick (REQ-0017-13): the foreground color, and with a text box open the color of its selected text (one edit), or with nothing
   selected of the text typed next at its caret. A swap or a background change is not a pick. */
static void
pick_foreground (EaselWindow *self, uint32_t color)
{
  EaselTextStyle v = { 0 };

  set_colors (self, color, self->background);
  v.color = color;
  if (self->tool == EASEL_TOOL_TEXT)
    easel_canvas_text_apply (self->canvas, EASEL_TEXT_COLOR, &v);
}

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
    pick_foreground (self, color);
  else if (self->chooser_target == CHOOSE_BACKGROUND)
    set_colors (self, self->foreground, color);
  else
    {
      /* A confirmed swatch color is also the new foreground, or the background after a secondary double-click
         (REQ-0003-12). */
      g_autofree char *path = easel_palette_file_path ();

      self->palette.colors[self->chooser_target - 1] = color;
      easel_palette_bar_set_swatch (self->palette_bar, self->chooser_target, color);
      if (self->chooser_to_background)
        set_colors (self, self->foreground, color);
      else
        pick_foreground (self, color);
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
  self->chooser_to_background = FALSE;
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
    pick_foreground (self, color);
  else
    set_colors (self, self->foreground, color);

  /* A double-click on one swatch, with either button, edits it; the first click has already picked it (REQ-0003-11). */
  if (n_press == 2)
    {
      open_chooser (self, n, color);
      self->chooser_to_background = button == GDK_BUTTON_SECONDARY;
    }
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

/* ---- the Text tool's starting style (REQ-0017-24) ---- */

/* The family of the desktop's interface font (`auto`, REQ-0017-24), or NULL where the desktop has none. */
static char *
desktop_interface_family (void)
{
  GSettingsSchema *schema = g_settings_schema_source_lookup (g_settings_schema_source_get_default (), "org.gnome.desktop.interface", TRUE);
  g_autoptr (GSettings) settings = NULL;
  g_autofree char *name = NULL;
  PangoFontDescription *fd;
  char *family;

  if (schema == NULL || !g_settings_schema_has_key (schema, "font-name"))
    {
      g_clear_pointer (&schema, g_settings_schema_unref);
      return NULL;
    }
  g_settings_schema_unref (schema);
  settings = g_settings_new ("org.gnome.desktop.interface");
  name = g_settings_get_string (settings, "font-name");
  fd = pango_font_description_from_string (name);
  family = g_strdup (pango_font_description_get_family (fd));
  pango_font_description_free (fd);
  return family;
}

/* `[text]` as the canvas's starting style: the family the settings name if it is installed (matched without regard to case), else the
   desktop's interface font; the size and the styles as read (REQ-0017-24). */
static void
apply_text_settings (EaselWindow *self)
{
  const char *family = self->settings.text_font[0] != '\0' ? easel_text_render_find_family (self->settings.text_font) : NULL;
  g_autofree char *desktop = family == NULL ? desktop_interface_family () : NULL;
  guint8 flags = (guint8) ((self->settings.text_bold ? EASEL_TEXT_BOLD : 0) | (self->settings.text_italic ? EASEL_TEXT_ITALIC : 0)
                           | (self->settings.text_underline ? EASEL_TEXT_UNDERLINE : 0)
                           | (self->settings.text_strike ? EASEL_TEXT_STRIKE : 0));

  /* a generic name, in the settings or from the desktop, becomes the installed family it stands for (REQ-0017-24) */
  easel_canvas_set_text_start_style (self->canvas,
                                     easel_text_render_resolve_family (family != NULL ? family : desktop != NULL ? desktop : "Sans"),
                                     self->settings.text_size, flags);
}

/* ---- the status bar (REQ-0004-09..11) ---- */

static const char *
tool_hint (EaselTool tool, gboolean transparent)
{
  switch (tool)
    {
    case EASEL_TOOL_PENCIL:
      return _ ("Draw with the foreground color. The secondary button uses the background color.");
    case EASEL_TOOL_BRUSH:
      return _ ("Paint with the foreground color. The secondary button uses the background color.");
    case EASEL_TOOL_AIRBRUSH:
      return _ ("Spray with the foreground color. The secondary button uses the background color.");
    case EASEL_TOOL_ERASER:
      return transparent ? _ ("Erase to transparency. The secondary button erases only the foreground color.")
                         : _ ("Erase to the background color. The secondary button changes only the foreground color.");
    case EASEL_TOOL_MAGNIFIER:
      return _ ("Changes the magnification.");
    case EASEL_TOOL_LINE:
      return _ ("Draws a straight line with the selected line width.");
    case EASEL_TOOL_RECTANGLE:
      return _ ("Draws a rectangle with the selected fill style.");
    case EASEL_TOOL_ROUNDED_RECTANGLE:
      return _ ("Draws a rounded rectangle with the selected fill style.");
    case EASEL_TOOL_ELLIPSE:
      return _ ("Draws an ellipse with the selected fill style.");
    case EASEL_TOOL_CURVE:
      return _ ("Draws a curved line: drag a line, then drag points of it to bend it. A click away from it finishes it.");
    case EASEL_TOOL_POLYGON:
      return _ ("Draws a polygon with the selected fill style.");
    case EASEL_TOOL_SELECT:
      return _ ("Selects a rectangular part of the picture to move, copy, or edit.");
    case EASEL_TOOL_FREE_SELECT:
      return _ ("Selects a free-form part of the picture to move, copy, or edit.");
    case EASEL_TOOL_GRADIENT:
      return _ ("Fades the drawing color to transparent along a drag. A click or Enter applies it, Esc cancels it.");
    case EASEL_TOOL_PICK:
      return _ ("Picks up a color from the picture for drawing.");
    case EASEL_TOOL_FILL:
      return _ ("Fills an area with the current drawing color.");
    case EASEL_TOOL_TEXT:
      return _ ("Inserts text into the picture.");
    default:
      return _ ("Drag to pan. Ctrl+wheel zooms.");
    }
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
  if (panel == self->toolbox_panel && self->toolbox_edge != NULL)
    gtk_widget_set_visible (self->toolbox_edge, shown);           /* hidden with the toolbox, and takes no presses (REQ-0030-14) */
  g_simple_action_set_state (G_SIMPLE_ACTION (a), g_variant_new_boolean (shown));
}

/* ---- the toolbox's width (REQ-0030-09 to -14) ---- */

/* The text scale factor: the desktop's text size against the default 96 dpi (REQ-0008-07's text sizes). */
static double
text_factor (void)
{
  int dpi = 0;

  g_object_get (gtk_settings_get_default (), "gtk-xft-dpi", &dpi, NULL);
  return dpi > 0 ? dpi / (96.0 * 1024.0) : 1.0;
}

/* The default width: what the toolbox's own content asks for, the options in two columns (REQ-0008-07). */
static int
toolbox_default_width (EaselWindow *self)
{
  int nat = 0, sep = 0;
  GtkWidget *line = gtk_widget_get_last_child (self->toolbox_panel);

  gtk_widget_measure (self->tools_column, GTK_ORIENTATION_HORIZONTAL, -1, NULL, &nat, NULL, NULL);
  if (line != NULL && line != self->tools_column)
    gtk_widget_measure (line, GTK_ORIENTATION_HORIZONTAL, -1, NULL, &sep, NULL, NULL);
  return nat + sep;
}

/* The widest the toolbox may be now: the limit, and half the window (REQ-0030-09), never under the default. */
static int
toolbox_widest (EaselWindow *self, int def)
{
  int limit = (int) lround (EASEL_TOOLBOX_WIDTH_MAX * text_factor ());
  int window = gtk_widget_get_width (GTK_WIDGET (self));

  if (window > 0)
    limit = MIN (limit, window / 2);
  return MAX (def, limit);
}

/* Shows the kept width as far as the window allows, and gives the options panel its form (REQ-0030-10, -13). */
static void
apply_toolbox_width (EaselWindow *self)
{
  int def, widest, want;

  def = toolbox_default_width (self);
  self->toolbox_default = def;
  widest = toolbox_widest (self, def);
  want = self->toolbox_kept > 0 ? (int) lround (self->toolbox_kept * text_factor ()) : def;
  want = CLAMP (want, def, widest);
  self->toolbox_shown = want;
  self->toolbox_window_width = gtk_widget_get_width (GTK_WIDGET (self));
  gtk_widget_set_size_request (self->toolbox_panel, want > def ? want : -1, -1);
}

static void
write_toolbox_width (EaselWindow *self, int value)
{
  g_autofree char *text = g_strdup_printf ("%d", value);
  EaselSettingChange change = { "view", "toolbox-width", text };

  /* A file that cannot be written: nothing shown, the width applies for this run, and the next drag tries again
     (REQ-0030-12). */
  if (!easel_window_write_settings (self, &change, 1))
    g_warning ("The toolbox width could not be written to settings.ini");
}

void
easel_window_drag_toolbox_edge (EaselWindow *self, double dx, gboolean end)
{
  int def = self->toolbox_default, want;          /* measured in two columns, not in the form shown now */

  if (self->toolbox_drag_start < 0)
    {
      self->toolbox_drag_start = self->toolbox_shown;
      self->toolbox_drag_moved = FALSE;
    }
  if (dx != 0)
    self->toolbox_drag_moved = TRUE;
  /* The width moves by the pointer's movement since the press (REQ-0030-09). */
  want = CLAMP (self->toolbox_drag_start + (int) lround (dx), def, toolbox_widest (self, def));
  self->toolbox_kept = (int) lround (want / text_factor ());
  apply_toolbox_width (self);
  if (end)
    {
      self->toolbox_drag_start = -1;
      if (self->toolbox_drag_moved)
        write_toolbox_width (self, self->toolbox_kept);
    }
}

void
easel_window_cancel_toolbox_drag (EaselWindow *self)
{
  /* A lost focus or a release outside the window ends the drag as a stroke ends: the width kept and written (REQ-0030-13). */
  if (self->toolbox_drag_start < 0)
    return;
  self->toolbox_drag_start = -1;
  if (self->toolbox_drag_moved)
    write_toolbox_width (self, self->toolbox_kept);
}

void
easel_window_reset_toolbox_width (EaselWindow *self)
{
  self->toolbox_drag_start = -1;
  self->toolbox_kept = 0;
  apply_toolbox_width (self);
  write_toolbox_width (self, (int) lround (self->toolbox_default / text_factor ()));
}

int
easel_window_get_toolbox_width (EaselWindow *self)
{
  return self->toolbox_shown;
}

GtkWidget *
easel_window_get_toolbox_edge (EaselWindow *self)
{
  return self->toolbox_edge;
}

static void
edge_drag_update (GtkGestureDrag *gesture, double dx, double dy, EaselWindow *self)
{
  easel_window_drag_toolbox_edge (self, dx, FALSE);
}

static void
edge_drag_end (GtkGestureDrag *gesture, double dx, double dy, EaselWindow *self)
{
  /* Nothing held: a press without movement, or a drag a cancel already ended (REQ-0030-12, -13). */
  if (self->toolbox_drag_start < 0)
    return;
  easel_window_drag_toolbox_edge (self, dx, TRUE);
}

/* A lost focus or a release outside the window cancels the gesture: the drag ends with its last width, kept and
   written (REQ-0030-13). */
static void
edge_drag_cancel (GtkGesture *gesture, GdkEventSequence *sequence, EaselWindow *self)
{
  easel_window_cancel_toolbox_drag (self);
}

static void
edge_pressed (GtkGestureClick *gesture, int n_press, double x, double y, EaselWindow *self)
{
  if (n_press == 2)
    easel_window_reset_toolbox_width (self);                        /* REQ-0030-11 */
}

/* The window's width changed: the toolbox shows as much of its kept width as half the window allows (REQ-0030-13).
   Looked at once after the window's surface is laid out at a new size, when the window has its new width; nothing runs
   while the size stays. */
static void
toolbox_follow_window (gpointer data)
{
  EaselWindow *self = data;

  self->toolbox_follow = 0;
  if (gtk_widget_get_width (GTK_WIDGET (self)) != self->toolbox_window_width)
    apply_toolbox_width (self);
}

static void
on_surface_layout (GdkSurface *surface, int width, int height, EaselWindow *self)
{
  if (self->toolbox_follow == 0)
    self->toolbox_follow = g_idle_add_once (toolbox_follow_window, self);
}

static void
on_window_realize (EaselWindow *self)
{
  g_signal_connect_object (gtk_native_get_surface (GTK_NATIVE (self)), "layout", G_CALLBACK (on_surface_layout), self, 0);
}

static void
make_toolbox_edge (EaselWindow *self)
{
  GtkGesture *drag = gtk_gesture_drag_new (), *click = gtk_gesture_click_new ();

  self->toolbox_drag_start = -1;
  self->toolbox_edge = gtk_drawing_area_new ();
  gtk_widget_set_size_request (self->toolbox_edge, 6, -1);
  gtk_widget_set_halign (self->toolbox_edge, GTK_ALIGN_START);
  gtk_widget_set_valign (self->toolbox_edge, GTK_ALIGN_FILL);
  gtk_widget_set_cursor_from_name (self->toolbox_edge, "ew-resize");
  gtk_gesture_single_set_button (GTK_GESTURE_SINGLE (drag), GDK_BUTTON_PRIMARY);      /* other buttons do nothing */
  gtk_gesture_single_set_button (GTK_GESTURE_SINGLE (click), GDK_BUTTON_PRIMARY);
  g_signal_connect (drag, "drag-update", G_CALLBACK (edge_drag_update), self);
  g_signal_connect (drag, "drag-end", G_CALLBACK (edge_drag_end), self);
  g_signal_connect (drag, "cancel", G_CALLBACK (edge_drag_cancel), self);
  g_signal_connect (click, "pressed", G_CALLBACK (edge_pressed), self);
  gtk_widget_add_controller (self->toolbox_edge, GTK_EVENT_CONTROLLER (drag));
  gtk_widget_add_controller (self->toolbox_edge, GTK_EVENT_CONTROLLER (click));
  gtk_overlay_add_overlay (self->canvas_overlay, self->toolbox_edge);
  g_signal_connect (self, "realize", G_CALLBACK (on_window_realize), NULL);
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

static gboolean menu_is_open (EaselWindow *self);

/* The canvas's question before the wheel or the pinch zooms (REQ-0036-01): not before the picture exists, nor while a dialog, a
   menu or a popover is open (the font card's family list among them). */
static gboolean
wheel_zoom_gate (gpointer data)
{
  EaselWindow *self = data;

  return self->picture != NULL && !dialog_is_open (self) && !menu_is_open (self)
         && !easel_font_card_family_is_open (self->font_card);
}

/* The zoom commands besides: nothing zooms while anything is held on the canvas (REQ-0036-02), Pan's drag, a selection or handle drag
   and a held key press included. */
static gboolean
zoom_allowed (EaselWindow *self)
{
  return view_actions_allowed (self) && !easel_canvas_is_holding (self->canvas);
}

/* ---- the thumbnail (0021) ---- */

/* With no button held a drag whose release got lost ends as last shown (REQ-0021-10); a focused field of the font card applies its value, as at a press
   elsewhere (REQ-0021-09). The press does nothing more. */
void
easel_window_thumbnail_press (EaselWindow *self, GdkModifierType state)
{
  if (!(state & (GDK_BUTTON1_MASK | GDK_BUTTON2_MASK | GDK_BUTTON3_MASK)))
    easel_canvas_buttons_released (self->canvas);
  if (card_field_focus (self) != NULL)
    gtk_root_set_focus (GTK_ROOT (self), NULL);
}

static void
on_thumbnail_pressed (GtkGestureClick *gesture, int n_press, double x, double y, EaselWindow *self)
{
  gtk_gesture_set_state (GTK_GESTURE (gesture), GTK_EVENT_SEQUENCE_CLAIMED);
  easel_window_thumbnail_press (self, gtk_event_controller_get_current_event_state (GTK_EVENT_CONTROLLER (gesture)));
}

/* A point of the card in the canvas area's coordinates. */
static graphene_point_t
thumbnail_point_in_area (EaselWindow *self, double x, double y)
{
  graphene_point_t p = GRAPHENE_POINT_INIT (0, 0);

  if (!gtk_widget_compute_point (GTK_WIDGET (self->thumbnail_card), GTK_WIDGET (self->scroller), &GRAPHENE_POINT_INIT ((float) x, (float) y), &p))
    p = GRAPHENE_POINT_INIT (0, 0);
  return p;
}

/* The wheel, Shift+wheel and Ctrl+wheel act as over the canvas at that point (REQ-0021-09): the card is the scrolled window's sibling, so the wheel would not reach it. */
/* Ctrl+wheel over a card that floats over the canvas area (the thumbnail, the font card and its size field): the canvas's own wheel,
   at the canvas area's point under the pointer (REQ-0036-01). The cards are the scrolled window's siblings in the overlay, so the
   canvas's controller never sees their events. TRUE when it zoomed; x and y in over's coordinates, notches as the canvas counts them. */
gboolean
easel_window_card_wheel (EaselWindow *self, GtkWidget *over, GdkModifierType state, double notches, double x, double y)
{
  graphene_point_t p = GRAPHENE_POINT_INIT (0, 0);

  if (!(state & GDK_CONTROL_MASK))
    return FALSE;
  if (!gtk_widget_compute_point (over, GTK_WIDGET (self->scroller), &GRAPHENE_POINT_INIT ((float) x, (float) y), &p))
    return FALSE;
  return easel_canvas_wheel (self->canvas, state, notches, p.x, p.y) == GDK_EVENT_STOP;
}

/* The event's position in the controller's widget, and the wheel's notches. */
static void
scroll_point (GtkEventControllerScroll *controller, double dy, double *x, double *y, double *notches)
{
  GtkWidget *w = gtk_event_controller_get_widget (GTK_EVENT_CONTROLLER (controller));
  GdkEvent *event = gtk_event_controller_get_current_event (GTK_EVENT_CONTROLLER (controller));
  GtkNative *native = gtk_widget_get_native (w);
  double ex = 0, ey = 0, nx = 0, ny = 0;
  graphene_point_t p = GRAPHENE_POINT_INIT (0, 0);

  *notches = gtk_event_controller_scroll_get_unit (controller) == GDK_SCROLL_UNIT_SURFACE ? dy / EASEL_CANVAS_PIXELS_PER_NOTCH : dy;
  if (event != NULL && native != NULL && gdk_event_get_position (event, &ex, &ey))
    {
      gtk_native_get_surface_transform (native, &nx, &ny);
      if (!gtk_widget_compute_point (GTK_WIDGET (native), w, &GRAPHENE_POINT_INIT ((float) (ex - nx), (float) (ey - ny)), &p))
        p = GRAPHENE_POINT_INIT (0, 0);
    }
  *x = p.x;
  *y = p.y;
}

/* Over the whole font card, before its parts: Ctrl+wheel zooms (REQ-0036-01); anything else goes on to them. */
static gboolean
on_card_scroll (GtkEventControllerScroll *controller, double dx, double dy, EaselWindow *self)
{
  GdkModifierType state = gtk_event_controller_get_current_event_state (GTK_EVENT_CONTROLLER (controller));
  double x, y, notches;

  if (!(state & GDK_CONTROL_MASK))
    return GDK_EVENT_PROPAGATE;
  scroll_point (controller, dy, &x, &y, &notches);
  if (easel_window_card_wheel (self, gtk_event_controller_get_widget (GTK_EVENT_CONTROLLER (controller)), state, notches, x, y))
    return GDK_EVENT_STOP;
  return GDK_EVENT_PROPAGATE;
}

static gboolean
on_thumbnail_scroll (GtkEventControllerScroll *controller, double dx, double dy, EaselWindow *self)
{
  GdkModifierType state = gtk_event_controller_get_current_event_state (GTK_EVENT_CONTROLLER (controller));
  double x, y, notches;

  scroll_point (controller, dy, &x, &y, &notches);
  if (easel_window_card_wheel (self, gtk_event_controller_get_widget (GTK_EVENT_CONTROLLER (controller)), state, notches, x, y))
    return GDK_EVENT_STOP;
  scroll_view_as_scroller (self, controller, dx, dy);
  return GDK_EVENT_STOP;
}

static void
on_thumbnail_pinch_begin (GtkGesture *gesture, GdkEventSequence *sequence, EaselWindow *self)
{
  easel_canvas_pinch_begin (self->canvas);
}

static void
on_thumbnail_pinch_end (GtkGesture *gesture, GdkEventSequence *sequence, EaselWindow *self)
{
  easel_canvas_pinch_end (self->canvas);
}

static void
on_thumbnail_pinch (GtkGestureZoom *gesture, double scale, EaselWindow *self)
{
  double x = 0, y = 0;
  graphene_point_t p;

  gtk_gesture_get_bounding_box_center (GTK_GESTURE (gesture), &x, &y);
  p = thumbnail_point_in_area (self, x, y);
  easel_canvas_pinch (self->canvas, scale, p.x, p.y);
}

/* The card's own input. A drag begun on the canvas keeps its grab, so none of this hears it (REQ-0021-10). */
static void
thumbnail_connect_input (EaselWindow *self)
{
  GtkWidget *card = GTK_WIDGET (self->thumbnail_card);
  GtkGesture *click = gtk_gesture_click_new ();
  GtkGesture *pinch = gtk_gesture_zoom_new ();
  GtkEventController *wheel = gtk_event_controller_scroll_new (GTK_EVENT_CONTROLLER_SCROLL_BOTH_AXES);

  gtk_gesture_single_set_button (GTK_GESTURE_SINGLE (click), 0);
  g_signal_connect_object (click, "pressed", G_CALLBACK (on_thumbnail_pressed), self, G_CONNECT_DEFAULT);
  gtk_widget_add_controller (card, GTK_EVENT_CONTROLLER (click));
  g_signal_connect_object (wheel, "scroll", G_CALLBACK (on_thumbnail_scroll), self, G_CONNECT_DEFAULT);
  gtk_widget_add_controller (card, wheel);
  g_signal_connect_object (pinch, "begin", G_CALLBACK (on_thumbnail_pinch_begin), self, G_CONNECT_DEFAULT);
  g_signal_connect_object (pinch, "end", G_CALLBACK (on_thumbnail_pinch_end), self, G_CONNECT_DEFAULT);
  g_signal_connect_object (pinch, "scale-changed", G_CALLBACK (on_thumbnail_pinch), self, G_CONNECT_DEFAULT);
  gtk_widget_add_controller (card, GTK_EVENT_CONTROLLER (pinch));
}

/* The card shows while the item is checked, above 100 % and with room for it (REQ-0021-06, -11); the item is sensitive above 100 %. */
static void
sync_thumbnail (EaselWindow *self)
{
  GAction *action = g_action_map_lookup_action (G_ACTION_MAP (self), "thumbnail");
  g_autoptr (GVariant) state = g_action_get_state (action);
  gboolean above = self->picture != NULL && easel_canvas_get_zoom (self->canvas) > 100;
  GtkAdjustment *h = gtk_scrolled_window_get_hadjustment (self->scroller), *v = gtk_scrolled_window_get_vadjustment (self->scroller);

  g_simple_action_set_enabled (G_SIMPLE_ACTION (action), above);
  gtk_widget_set_visible (GTK_WIDGET (self->thumbnail_card), g_variant_get_boolean (state) && above
                          && easel_view_thumbnail_fits ((int) gtk_adjustment_get_page_size (h), (int) gtk_adjustment_get_page_size (v)));
}

static void
thumbnail_state_changed (GSimpleAction *action, GVariant *value, gpointer user_data)
{
  g_simple_action_set_state (action, value);
  sync_thumbnail (user_data);
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
  sync_thumbnail (self);
  /* View > Zoom marks the current zoom when it is a preset, and nothing otherwise (REQ-0008-05). */
  {
    GAction *zoom_to = g_action_map_lookup_action (G_ACTION_MAP (self), "zoom-to");

    if (zoom_to != NULL)
      g_simple_action_set_state (G_SIMPLE_ACTION (zoom_to),
                                 g_variant_new_int32 (easel_view_preset_index (zoom) >= 0 ? zoom : 0));
  }
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

void
easel_window_set_clipboard_timeout (EaselWindow *self, guint ms)
{
  self->clipboard_timeout_ms = ms;
}

void
easel_window_set_clipboard (EaselWindow *self, EaselClipboard *clipboard)
{
  if (self->clipboard != NULL)
    g_signal_handlers_disconnect_by_data (self->clipboard, self);
  g_clear_object (&self->clipboard);
  g_clear_pointer (&self->paste_pending, easel_image_free);
  self->clipboard = clipboard != NULL ? g_object_ref (clipboard) : easel_gdk_clipboard_new (gtk_widget_get_clipboard (GTK_WIDGET (self)));
  g_signal_connect_swapped (self->clipboard, "changed", G_CALLBACK (update_clipboard_actions), self);
  update_clipboard_actions (self);
}

GtkPopoverMenu *
easel_window_get_selection_menu (EaselWindow *self)
{
  return GTK_POPOVER_MENU (self->selection_menu);
}

/* A secondary press on the selection: its context menu at the pointer (REQ-0013-17). */
static void
on_context_menu (EaselWindow *self, double x, double y)
{
  /* With Text, the box's menu (REQ-0017-22); else the selection's */
  GtkWidget *menu = self->tool == EASEL_TOOL_TEXT ? self->text_menu : self->selection_menu;

  gtk_popover_set_pointing_to (GTK_POPOVER (menu), &(GdkRectangle) { (int) x, (int) y, 1, 1 });
  gtk_popover_popup (GTK_POPOVER (menu));
}

GtkPopoverMenu *
easel_window_get_text_menu (EaselWindow *self)
{
  return GTK_POPOVER_MENU (self->text_menu);
}

void
easel_window_set_rich_text_limit (EaselWindow *self, gsize bytes)
{
  self->rich_limit = bytes;
}

void
easel_window_set_transform_limit (EaselWindow *self, gsize bytes)
{
  self->transform_limit = bytes;
}

/* A secondary press with a select tool not on the selection: the canvas menu at the pointer (REQ-0016-14). */
static void
on_canvas_menu (EaselWindow *self, double x, double y)
{
  gtk_popover_set_pointing_to (GTK_POPOVER (self->canvas_menu), &(GdkRectangle) { (int) x, (int) y, 1, 1 });
  gtk_popover_popup (GTK_POPOVER (self->canvas_menu));
}

GtkPopoverMenu *
easel_window_get_canvas_menu (EaselWindow *self)
{
  return GTK_POPOVER_MENU (self->canvas_menu);
}

/* A menu or popover is open: its keys are its own (REQ-0013-17). */
static gboolean
menu_is_open (EaselWindow *self)
{
  GtkMenuButton *buttons[] = { self->main_menu_button, self->image_button, self->view_button };

  if (self->selection_menu != NULL && gtk_widget_get_visible (self->selection_menu))
    return TRUE;
  if (self->text_menu != NULL && gtk_widget_get_visible (self->text_menu))
    return TRUE;
  if (self->canvas_menu != NULL && gtk_widget_get_visible (self->canvas_menu))
    return TRUE;
  for (guint i = 0; i < G_N_ELEMENTS (buttons); i++)
    if (buttons[i] != NULL && gtk_menu_button_get_popover (buttons[i]) != NULL
        && gtk_widget_get_visible (GTK_WIDGET (gtk_menu_button_get_popover (buttons[i]))))
      return TRUE;
  return FALSE;
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

  if (zoom_allowed (self))
    easel_canvas_set_zoom (self->canvas, easel_view_zoom_step (easel_canvas_get_zoom (self->canvas), 1));
}

static void
zoom_out_activated (GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
  EaselWindow *self = user_data;

  if (zoom_allowed (self))
    easel_canvas_set_zoom (self->canvas, easel_view_zoom_step (easel_canvas_get_zoom (self->canvas), -1));
}

static void
zoom_normal_activated (GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
  EaselWindow *self = user_data;

  if (zoom_allowed (self))
    easel_canvas_set_zoom (self->canvas, 100);
}

static void
zoom_large_activated (GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
  EaselWindow *self = user_data;

  if (zoom_allowed (self))
    easel_canvas_set_zoom (self->canvas, 400);
}

static void
zoom_fit_activated (GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
  EaselWindow *self = user_data;

  if (zoom_allowed (self))
    easel_canvas_fit_to_window (self->canvas);
}

/* The workspace colors of the settings file, when valid, over the defaults of style.css (REQ-0009-04, REQ-0009-06): in
   the light theme `workspace-color`; in the dark theme `workspace-color-dark`, else `workspace-color`. One provider for
   the app, set again by each new window from the file it read, and again when the theme changes (a media query in a
   provider loaded from a string is not re-evaluated on a change of theme, so the color for the theme now is chosen
   here). */
static EaselSettings workspace_settings;

static void on_dark_changed (AdwStyleManager *manager, GParamSpec *pspec, gpointer data);

static void
load_workspace_color (void)
{
  static GtkCssProvider *provider;
  const EaselSettings *s = &workspace_settings;
  gboolean dark = adw_style_manager_get_dark (adw_style_manager_get_default ());
  gboolean set = dark ? s->has_workspace_color_dark || s->has_workspace_color : s->has_workspace_color;
  uint32_t color = dark && s->has_workspace_color_dark ? s->workspace_color_dark : s->workspace_color;
  g_autofree char *css = NULL;

  if (provider == NULL)
    {
      provider = gtk_css_provider_new ();
      gtk_style_context_add_provider_for_display (gdk_display_get_default (), GTK_STYLE_PROVIDER (provider),
                                                  GTK_STYLE_PROVIDER_PRIORITY_USER);
      g_signal_connect (adw_style_manager_get_default (), "notify::dark", G_CALLBACK (on_dark_changed), NULL);
    }
  if (set)
    css = g_strdup_printf ("scrolledwindow.workspace > viewport, .thumbnail-area { background-color: #%06X; }", (unsigned) color);
  gtk_css_provider_load_from_string (provider, css != NULL ? css : "");
}

/* The theme changed: the workspace color for the new theme (REQ-0009-06). */
static void
on_dark_changed (AdwStyleManager *manager, GParamSpec *pspec, gpointer data)
{
  load_workspace_color ();
}

static void
apply_workspace_color (const EaselSettings *settings)
{
  workspace_settings = *settings;
  load_workspace_color ();
}

/* The Magnifier's level (REQ-0007-02, REQ-0008-02): the lens buttons set this state; it only selects, it never zooms. */
static void
magnifier_level_changed (GSimpleAction *action, GVariant *value, gpointer user_data)
{
  EaselWindow *self = user_data;

  if (easel_view_preset_index (g_variant_get_int32 (value)) < 0)
    return;
  g_simple_action_set_state (action, value);
  easel_canvas_set_magnifier_level (self->canvas, g_variant_get_int32 (value));
}

/* The line width and the fill style of the shape tools (REQ-0010-03): one state each, shared by every tool that shows it. */
static void update_preset_part (EaselWindow *self);

static void
line_width_changed (GSimpleAction *action, GVariant *value, gpointer user_data)
{
  EaselWindow *self = user_data;
  int w = g_variant_get_int32 (value);
  g_autoptr (GVariant) style = g_action_group_get_action_state (G_ACTION_GROUP (self), "fill-style");

  if (w >= 1 && w <= 5 && !easel_canvas_is_stroking (self->canvas))      /* ignored during a drag (REQ-0010-08) */
    {
      g_simple_action_set_state (action, value);
      easel_canvas_set_shape_options (self->canvas, w, g_variant_get_int32 (style));
      update_preset_part (self);
    }
}

static void
fill_style_changed (GSimpleAction *action, GVariant *value, gpointer user_data)
{
  EaselWindow *self = user_data;
  int style = g_variant_get_int32 (value);
  g_autoptr (GVariant) width = g_action_group_get_action_state (G_ACTION_GROUP (self), "line-width");

  if (style >= 0 && style <= 2 && !easel_canvas_is_stroking (self->canvas))
    {
      g_simple_action_set_state (action, value);
      easel_canvas_set_shape_options (self->canvas, g_variant_get_int32 (width), style);
    }
}

/* View > Zoom (REQ-0008-04): a preset, through the same path as the other zoom commands, which keeps the center still. */
static void
zoom_to_activated (GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
  EaselWindow *self = user_data;

  if (zoom_allowed (self) && easel_view_preset_index (g_variant_get_int32 (parameter)) >= 0)
    easel_canvas_set_zoom (self->canvas, g_variant_get_int32 (parameter));
}

/* The View menu's Zoom submenu, one item per preset (REQ-0008-03), first in the section of Normal Size. */
static void
add_zoom_submenu (EaselWindow *self)
{
  GMenuModel *menu = gtk_menu_button_get_menu_model (self->view_button);
  g_autoptr (GMenuModel) section = g_menu_model_get_item_link (menu, 1, G_MENU_LINK_SECTION);
  g_autoptr (GMenu) sub = g_menu_new ();

  for (int i = 0; i < EASEL_VIEW_N_PRESETS; i++)
    {
      g_autofree char *label = g_strdup_printf ("%d %%", easel_view_presets[i]);
      g_autoptr (GMenuItem) item = g_menu_item_new (label, NULL);

      g_menu_item_set_action_and_target_value (item, "win.zoom-to", g_variant_new_int32 (easel_view_presets[i]));
      g_menu_append_item (sub, item);
    }
  g_menu_insert_submenu (G_MENU (section), 0, _ ("Zoom"), G_MENU_MODEL (sub));
}

static void
on_zoom_changed (EaselWindow *self, int pct)
{
  sync_zoom_ui (self);
  if (pct != self->said_zoom)
    {
      g_autofree char *text = g_strdup_printf ("%d %%", pct);

      easel_window_announce (self, text);
    }
  self->said_zoom = pct;
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
  gtk_widget_set_tooltip_text (zoom_out, _ ("Zoom Out (Ctrl+-)"));
  gtk_widget_set_tooltip_text (zoom_in, _ ("Zoom In (Ctrl+=)"));
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
  g_autofree char *body = g_strdup_printf (_n ("There is not enough memory for a picture of %d × %d pixel.", "There is not enough memory for a picture of %d × %d pixels.", (unsigned long) height), width, height);
  AdwDialog *dialog = adw_alert_dialog_new (heading, body);

  adw_alert_dialog_add_response (ADW_ALERT_DIALOG (dialog), "close", _ ("Close"));
  adw_dialog_present (dialog, GTK_WIDGET (self));
}

static void
show_resize_error (EaselWindow *self, int width, int height)
{
  show_memory_error (self, _ ("Could Not Resize the Picture"), width, height);
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
  /* an Image action puts a pending curve or polygon down (REQ-0011-12), and a text box, or is not done (REQ-0017-18) */
  if (!put_down_for_action (self, FALSE))
    return;
  canvas_area_device (self, &area_w, &area_h);
  easel_settings_resolve_size (&self->settings, area_w, area_h, &default_w, &default_h);
  easel_attributes_dialog_present (GTK_WIDGET (self), self->picture->width, self->picture->height, default_w, default_h,
                                   attributes_applied, self);
}

/* ---- Image Transforms on the whole picture (REQ-0018-01 to REQ-0018-06, REQ-0018-09, REQ-0018-10) ---- */

/* The dialog of a command whose memory could not be had, with its action's heading (REQ-0018-10). */
static void
show_transform_error (EaselWindow *self, EaselOp op)
{
  const char *heading = op == EASEL_OP_INVERT ? _ ("Could Not Invert the Colors")
                        : op == EASEL_OP_CLEAR ? _ ("Could Not Clear the Picture")
                        : (op == EASEL_OP_FLIP_H || op == EASEL_OP_FLIP_V) ? _ ("Could Not Flip the Picture")
                        : _ ("Could Not Rotate the Picture");

  show_save_error (self, heading, _ ("There is not enough memory for this change."));
}

/* The one function every Image command goes through: nothing before the picture exists, with a dialog open or while a button is held
   (REQ-0004-03); what is open is put down first (a pending curve, polygon or gradient, a floating selection, a text box, or the
   command is not done when the box cannot be put down, REQ-0017-18); then the command on the whole picture as one step. */
static void
image_action (EaselWindow *self, EaselOp op)
{
  EaselTransformResult r;
  int old_w, old_h;

  if (!view_actions_allowed (self) || easel_canvas_is_panning (self->canvas))
    return;
  if (card_field_focus (self) != NULL)
    return;                                     /* the focus is in a field of the font card: the key is the field's (REQ-0018-01) */
  if (op != EASEL_OP_CLEAR && easel_canvas_transform_selection (self->canvas, op))
    return;                                     /* a selection: the command is its own, it is not put down (REQ-0018-03) */
  if (!put_down_for_action (self, FALSE))
    return;
  old_w = self->picture->width;
  old_h = self->picture->height;
  r = easel_image_transform (self->picture, self->history, op, self->background, self->transform_limit);
  if (r == EASEL_TRANSFORM_NO_MEMORY)
    show_transform_error (self, op);
  if (r != EASEL_TRANSFORM_DONE)
    return;
  if (self->picture->width != old_w || self->picture->height != old_h)
    picture_size_changed (self);
  else
    easel_canvas_update (self->canvas);
  sync_history_ui (self);
}

static void
flip_horizontal_activated (GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
  image_action (user_data, EASEL_OP_FLIP_H);
}

static void
flip_vertical_activated (GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
  image_action (user_data, EASEL_OP_FLIP_V);
}

static void
rotate_90_activated (GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
  image_action (user_data, EASEL_OP_ROTATE_90);
}

static void
rotate_180_activated (GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
  image_action (user_data, EASEL_OP_ROTATE_180);
}

static void
rotate_270_activated (GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
  image_action (user_data, EASEL_OP_ROTATE_270);
}

/* ---- Frame time (0021) ---- */

static void
show_frame_time (EaselWindow *self)
{
  char text[32];

  easel_frame_time_format (easel_canvas_get_frame_ms (self->canvas), text, sizeof text);
  easel_status_bar_set_frame_time (self->status_bar, text);
  self->frame_time_shown = g_get_monotonic_time ();
}

static gboolean
frame_time_timer_fired (gpointer data)
{
  EaselWindow *self = data;

  self->frame_time_timer = 0;
  show_frame_time (self);
  return G_SOURCE_REMOVE;
}

/* A frame was painted: its value shows now, or as soon as the pace lets it, so the last one always shows within 250 ms (REQ-0021-12). */
static void
on_canvas_painted (EaselWindow *self)
{
  gint64 now = g_get_monotonic_time ();
  EaselTextBuffer *text = easel_canvas_text_buffer (self->canvas);

  /* A frame follows every change of the box: its text for a screen reader goes with it. */
  gtk_widget_set_visible (self->text_access, text != NULL);
  if (text != NULL)
    {
      size_t len;
      const char *utf8 = easel_text_buffer_text (text, &len);
      g_autofree char *value = g_strndup (utf8, len);

      gtk_accessible_update_property (GTK_ACCESSIBLE (self->text_access), GTK_ACCESSIBLE_PROPERTY_VALUE_TEXT, value, -1);
    }
  if (!self->settings.frame_time || self->frame_time_timer != 0)
    return;
  if (easel_frame_time_due (now, self->frame_time_shown))
    show_frame_time (self);
  else
    self->frame_time_timer = g_timeout_add (easel_frame_time_wait_ms (now, self->frame_time_shown), frame_time_timer_fired, self);
}

void
easel_window_apply_frame_time (EaselWindow *self, gboolean shown)
{
  self->settings.frame_time = shown;
  g_clear_handle_id (&self->frame_time_timer, g_source_remove);
  easel_status_bar_show_frame_time (self->status_bar, shown);
  self->frame_time_shown = 0;
}

/* ---- Preferences (0020) ---- */

static void
preferences_activated (GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
  EaselWindow *self = user_data;

  /* Opening it changes nothing: what waits stays (REQ-0020-01). */
  if (view_actions_allowed (self) && !easel_canvas_is_panning (self->canvas) && card_field_focus (self) == NULL)
    easel_preferences_dialog_present (self);
}

/* ---- Panels and their keys (0022) ---- */

static gboolean
widget_within (GtkWidget *w, GtkWidget *panel)
{
  return w != NULL && (w == panel || gtk_widget_is_ancestor (w, panel));
}

static gboolean
widget_can_take_focus (GtkWidget *w)
{
  return w != NULL && gtk_widget_get_mapped (w) && gtk_widget_get_sensitive (w) && gtk_widget_get_focusable (w);
}

/* The focusable controls under w, leaves first: a menu button's own toggle, not the menu button. */
static void
collect_focusable (GtkWidget *w, GPtrArray *out)
{
  guint before = out->len;

  /* The title buttons take no focus (the desktop has their keys). */
  if (!gtk_widget_get_mapped (w) || GTK_IS_WINDOW_CONTROLS (w))
    return;
  for (GtkWidget *c = gtk_widget_get_first_child (w); c != NULL; c = gtk_widget_get_next_sibling (c))
    collect_focusable (c, out);
  if (out->len == before && widget_can_take_focus (w))
    g_ptr_array_add (out, w);
}

/* The first focus of a panel: its chosen toggle, else its first control. */
static GtkWidget *
panel_first (GtkWidget *panel)
{
  g_autoptr (GPtrArray) all = g_ptr_array_new ();

  collect_focusable (panel, all);
  for (guint i = 0; i < all->len; i++)
    if (GTK_IS_TOGGLE_BUTTON (all->pdata[i]) && gtk_toggle_button_get_active (all->pdata[i]))
      return all->pdata[i];
  return all->len > 0 ? all->pdata[0] : NULL;
}

/* Where a panel's focus goes (REQ-0022-01): what it last had while that is still there. */
static GtkWidget *
panel_target (GtkWidget *panel, GtkWidget *last)
{
  return widget_can_take_focus (last) && widget_within (last, panel) ? last : panel_first (panel);
}

static GtkWidget *
palette_swatch_target (EaselWindow *self)
{
  GtkWidget *last = self->last_palette;

  return widget_can_take_focus (last) && EASEL_IS_SWATCH (last) ? last : GTK_WIDGET (easel_palette_bar_get_swatch (self->palette_bar, 1));
}

/* One stop each of Tab and Shift+Tab, in order (REQ-0022-01). */
static GPtrArray *
panel_stops (EaselWindow *self)
{
  GPtrArray *stops = g_ptr_array_new ();
  GtkWidget *w;

  collect_focusable (self->header_bar, stops);
  if ((w = panel_target (self->toolbox, self->last_toolbox)) != NULL)
    g_ptr_array_add (stops, w);
  if ((w = panel_target (GTK_WIDGET (self->tool_options), self->last_options)) != NULL)
    g_ptr_array_add (stops, w);
  if (widget_can_take_focus (GTK_WIDGET (self->canvas)))
    g_ptr_array_add (stops, self->canvas);
  if (gtk_widget_get_mapped (GTK_WIDGET (self->palette_bar)))
    {
      g_ptr_array_add (stops, easel_palette_bar_get_square (self->palette_bar, TRUE));
      g_ptr_array_add (stops, easel_palette_bar_get_square (self->palette_bar, FALSE));
      g_ptr_array_add (stops, easel_palette_bar_get_swap (self->palette_bar));
      g_ptr_array_add (stops, palette_swatch_target (self));
    }
  return stops;
}

/* One stop each of F6 and Shift+F6. */
static GPtrArray *
panel_groups (EaselWindow *self)
{
  GPtrArray *groups = g_ptr_array_new ();
  GtkWidget *w;

  if ((w = panel_first (self->header_bar)) != NULL)
    g_ptr_array_add (groups, w);
  if ((w = panel_target (self->toolbox, self->last_toolbox)) != NULL)
    g_ptr_array_add (groups, w);
  if ((w = panel_target (GTK_WIDGET (self->tool_options), self->last_options)) != NULL)
    g_ptr_array_add (groups, w);
  if (widget_can_take_focus (GTK_WIDGET (self->canvas)))
    g_ptr_array_add (groups, self->canvas);
  if (gtk_widget_get_mapped (GTK_WIDGET (self->palette_bar)))
    g_ptr_array_add (groups, widget_can_take_focus (self->last_palette) ? self->last_palette : palette_swatch_target (self));
  return groups;
}

static void
grab_by_key (EaselWindow *self, GtkWidget *w)
{
  self->grabbing_by_key = TRUE;
  gtk_widget_grab_focus (w);
  self->grabbing_by_key = FALSE;
  self->panel_focus_by_key = TRUE;
  gtk_window_set_focus_visible (GTK_WINDOW (self), TRUE);
}

/* The panel that holds w. */
static GtkWidget *
panel_of (EaselWindow *self, GtkWidget *w)
{
  GtkWidget *panels[] = { self->header_bar, self->toolbox, GTK_WIDGET (self->tool_options), GTK_WIDGET (self->canvas), GTK_WIDGET (self->palette_bar) };

  for (guint i = 0; i < G_N_ELEMENTS (panels); i++)
    if (widget_within (w, panels[i]))
      return panels[i];
  return NULL;
}

/* The next stop from the focus, forward or back; with the focus nowhere, the first or the last. */
static void
walk_stops (EaselWindow *self, GPtrArray *stops, gboolean by_panel, gboolean back)
{
  GtkWidget *focus = gtk_root_get_focus (GTK_ROOT (self));
  GtkWidget *panel = panel_of (self, focus);
  int at = -1, n = (int) stops->len;

  for (int i = 0; i < n && at < 0; i++)
    if (stops->pdata[i] == focus || (by_panel && panel != NULL && panel_of (self, stops->pdata[i]) == panel))
      at = i;
  if (n == 0)
    return;
  /* A focus between stops (in a panel, on another control of it) counts as its panel's stop. */
  if (at < 0 && panel != NULL)
    for (int i = 0; i < n && at < 0; i++)
      if (panel_of (self, stops->pdata[i]) == panel)
        at = i;
  grab_by_key (self, stops->pdata[at < 0 ? (back ? n - 1 : 0) : (at + (back ? n - 1 : 1)) % n]);
}

/* The nearest control of panel in that direction, by their bounds; none at the ends (REQ-0022-02, -03). */
static GtkWidget *
panel_neighbor (EaselWindow *self, GtkWidget *panel, GtkWidget *focus, int dx, int dy)
{
  g_autoptr (GPtrArray) all = g_ptr_array_new ();
  graphene_rect_t from;
  GtkWidget *best = NULL;
  double best_distance = G_MAXDOUBLE;

  if (!gtk_widget_compute_bounds (focus, panel, &from))
    return NULL;
  collect_focusable (panel, all);
  for (guint i = 0; i < all->len; i++)
    {
      GtkWidget *w = all->pdata[i];
      graphene_rect_t r;
      double along, overlap;

      /* In the palette the arrows are the swatches' (the squares and the swap button are stops of their own). */
      if (w == focus || (panel == GTK_WIDGET (self->palette_bar) && !EASEL_IS_SWATCH (w)) || !gtk_widget_compute_bounds (w, panel, &r))
        continue;
      if (dx != 0)
        {
          along = dx > 0 ? r.origin.x - (from.origin.x + from.size.width) : from.origin.x - (r.origin.x + r.size.width);
          overlap = MIN (r.origin.y + r.size.height, from.origin.y + from.size.height) - MAX (r.origin.y, from.origin.y);
        }
      else
        {
          along = dy > 0 ? r.origin.y - (from.origin.y + from.size.height) : from.origin.y - (r.origin.y + r.size.height);
          overlap = MIN (r.origin.x + r.size.width, from.origin.x + from.size.width) - MAX (r.origin.x, from.origin.x);
        }
      if (along >= -0.5 && overlap > 0 && along < best_distance)
        {
          best = w;
          best_distance = along;
        }
    }
  return best;
}

/* The keys of the panels: TRUE if taken (REQ-0022-01 to -04). */
static gboolean
panel_key (EaselWindow *self, guint keyval, GdkModifierType state, gboolean repeat, gboolean *handled)
{
  GtkWidget *focus = gtk_root_get_focus (GTK_ROOT (self));
  gboolean shift = (state & GDK_SHIFT_MASK) != 0;
  gboolean plain = (state & (GDK_CONTROL_MASK | GDK_ALT_MASK)) == 0;
  gboolean held = easel_canvas_is_stroking (self->canvas) || easel_canvas_is_resizing (self->canvas) || easel_canvas_is_panning (self->canvas)
                  || easel_canvas_is_waiting_for_release (self->canvas);
  gboolean box = easel_canvas_text_buffer (self->canvas) != NULL;
  GtkWidget *panel = panel_of (self, focus);
  int dx = 0, dy = 0;

  if (!plain || dialog_is_open (self))
    return FALSE;
  if (keyval == GDK_KEY_Tab || keyval == GDK_KEY_ISO_Left_Tab || keyval == GDK_KEY_KP_Tab || keyval == GDK_KEY_F6)
    {
      *handled = TRUE;
      if (!held)
        {
          g_autoptr (GPtrArray) stops = keyval == GDK_KEY_F6 ? panel_groups (self) : panel_stops (self);

          walk_stops (self, stops, keyval == GDK_KEY_F6, shift);
        }
      return TRUE;
    }
  /* A click on a control with a box open leaves the keys with the box (REQ-0017-06). */
  if (panel == NULL || panel == self->header_bar || panel == GTK_WIDGET (self->canvas) || (box && !self->panel_focus_by_key))
    return FALSE;
  switch (keyval)
    {
    case GDK_KEY_Left: case GDK_KEY_KP_Left: dx = -1; break;
    case GDK_KEY_Right: case GDK_KEY_KP_Right: dx = 1; break;
    case GDK_KEY_Up: case GDK_KEY_KP_Up: dy = -1; break;
    case GDK_KEY_Down: case GDK_KEY_KP_Down: dy = 1; break;
    default: break;
    }
  if (dx != 0 || dy != 0)
    {
      GtkWidget *next = panel_neighbor (self, panel, focus, dx, dy);

      *handled = TRUE;
      if (next != NULL)
        grab_by_key (self, next);
      return TRUE;
    }
  if (keyval == GDK_KEY_space || keyval == GDK_KEY_KP_Space || keyval == GDK_KEY_Return || keyval == GDK_KEY_KP_Enter)
    {
      gboolean enter = keyval == GDK_KEY_Return || keyval == GDK_KEY_KP_Enter;
      int w, h;

      /* Enter acts on what waits for it (REQ-0011-12, REQ-0013-11). */
      if (enter && (easel_canvas_is_pending (self->canvas) || easel_canvas_get_selection_size (self->canvas, &w, &h)))
        return FALSE;
      /* A held key chooses once: a repeat would swap the colors back and forth. */
      if (repeat)
        {
          *handled = TRUE;
          return TRUE;
        }
      if (EASEL_IS_SWATCH (focus))
        easel_palette_bar_press (self->palette_bar, easel_swatch_get_number (EASEL_SWATCH (focus)), shift ? GDK_BUTTON_SECONDARY : GDK_BUTTON_PRIMARY, 1);
      else if (focus == easel_palette_bar_get_square (self->palette_bar, TRUE) || focus == easel_palette_bar_get_square (self->palette_bar, FALSE))
        easel_palette_bar_press_indicator (self->palette_bar, focus == easel_palette_bar_get_square (self->palette_bar, TRUE));
      else if (GTK_IS_BUTTON (focus))
        g_signal_emit_by_name (focus, "clicked");
      else
        return FALSE;
      *handled = TRUE;
      return TRUE;
    }
  return FALSE;
}

/* ---- The keyboard pointer (0022) ---- */

/* A keyboard press's hint and its announcements, as it starts and ends (REQ-0022-11, -15). */
static void
sync_key_hold (EaselWindow *self)
{
  gboolean held = easel_canvas_is_key_held (self->canvas);

  if (held == self->said_key_held)
    return;
  self->said_key_held = held;
  easel_status_bar_set_hint (self->status_bar, held ? _ ("Space to finish, Esc to cancel")
                                                    : tool_hint (self->tool, easel_tool_options_get_transparent (self->tool_options)));
  easel_window_announce (self, held ? _ ("Drawing") : self->key_releasing ? _ ("Done") : _ ("Cancelled"));
}

/* The view scrolls by the least to show the marker's square; a square wider than the view shows its start (REQ-0022-06, -08). */
static void
marker_into_view (EaselWindow *self)
{
  GtkAdjustment *h = gtk_scrolled_window_get_hadjustment (self->scroller), *v = gtk_scrolled_window_get_vadjustment (self->scroller);
  double x0, y0, x1, y1;

  easel_canvas_marker_bounds (self->canvas, &x0, &y0, &x1, &y1);
  if (gtk_adjustment_get_value (h) < x1 + EASEL_VIEW_MARGIN - gtk_adjustment_get_page_size (h))
    gtk_adjustment_set_value (h, x1 + EASEL_VIEW_MARGIN - gtk_adjustment_get_page_size (h));
  if (gtk_adjustment_get_value (h) > x0 + EASEL_VIEW_MARGIN)
    gtk_adjustment_set_value (h, x0 + EASEL_VIEW_MARGIN);
  if (gtk_adjustment_get_value (v) < y1 + EASEL_VIEW_MARGIN - gtk_adjustment_get_page_size (v))
    gtk_adjustment_set_value (v, y1 + EASEL_VIEW_MARGIN - gtk_adjustment_get_page_size (v));
  if (gtk_adjustment_get_value (v) > y0 + EASEL_VIEW_MARGIN)
    gtk_adjustment_set_value (v, y0 + EASEL_VIEW_MARGIN);
}

/* The keys of the keyboard pointer, with the focus on the canvas: TRUE if taken (REQ-0022-06, -08). */
static gboolean
pointer_key (EaselWindow *self, guint keyval, GdkModifierType state, gboolean repeat)
{
  gboolean shift = (state & GDK_SHIFT_MASK) != 0, ctrl = (state & GDK_CONTROL_MASK) != 0, alt = (state & GDK_ALT_MASK) != 0;
  gboolean held, mouse_held, box, moves_selection;
  int dx = 0, dy = 0, w, h, step;

  if (gtk_root_get_focus (GTK_ROOT (self)) != GTK_WIDGET (self->canvas) || dialog_is_open (self) || self->picture == NULL)
    return FALSE;
  /* F7 says the marker's place and color (REQ-0022-13). */
  if (keyval == GDK_KEY_F7)
    {
      if (!repeat)
        {
          easel_canvas_marker_show (self->canvas, TRUE);
          int x, y;
          uint32_t c = easel_canvas_marker_color (self->canvas);
          g_autofree char *text = NULL;

          easel_canvas_marker_get (self->canvas, &x, &y);
          text = g_strdup_printf ("%d, %d: #%06X%02X", x, y, (unsigned) (c & 0xFFFFFF), (unsigned) (c >> 24));
          easel_window_announce (self, text);
        }
      return TRUE;
    }
  held = easel_canvas_is_key_held (self->canvas);
  /* Esc cancels a keyboard press with any tool: the keyboard has no other button (REQ-0022-10). */
  if (keyval == GDK_KEY_Escape && held)
    {
      easel_canvas_key_hold_event (self->canvas, GDK_BUTTON_PRESS, GDK_BUTTON_PRIMARY);
      sync_key_hold (self);
      return TRUE;
    }
  mouse_held = !held && (easel_canvas_is_stroking (self->canvas) || easel_canvas_is_resizing (self->canvas) || easel_canvas_is_panning (self->canvas));
  if (keyval == GDK_KEY_Menu || (keyval == GDK_KEY_F10 && shift && !ctrl && !alt))
    {
      if (!held && !mouse_held)
        {
          easel_canvas_marker_show (self->canvas, TRUE);
          marker_into_view (self);
          easel_canvas_key_menu (self->canvas);
        }
      return TRUE;
    }
  box = easel_canvas_text_buffer (self->canvas) != NULL;
  /* Alt with an arrow moves an open box, Alt+Shift its far corner (REQ-0022-12); else a box takes its keys (REQ-0017-06). */
  if (box && !held && !(alt && !ctrl && (keyval == GDK_KEY_Left || keyval == GDK_KEY_Right || keyval == GDK_KEY_Up || keyval == GDK_KEY_Down)))
    return FALSE;
  if (box && !held)
    {
      easel_canvas_text_nudge (self->canvas, shift, keyval == GDK_KEY_Left ? -1 : keyval == GDK_KEY_Right ? 1 : 0, keyval == GDK_KEY_Up ? -1 : keyval == GDK_KEY_Down ? 1 : 0);
      return TRUE;
    }
  if ((keyval == GDK_KEY_space || keyval == GDK_KEY_KP_Space) && !alt)
    {
      if (repeat || mouse_held)
        return TRUE;
      easel_canvas_marker_show (self->canvas, TRUE);
      if (held)
        {
          /* Any Space releases what is held, with the Shift really held counting (REQ-0022-06). */
          easel_canvas_set_shift (self->canvas, shift);
          easel_canvas_set_ctrl (self->canvas, ctrl);
          self->key_releasing = TRUE;
          easel_canvas_key_release (self->canvas);
          sync_key_hold (self);
          self->key_releasing = FALSE;
          return TRUE;
        }
      marker_into_view (self);
      easel_canvas_key_press (self->canvas, shift && !ctrl ? GDK_BUTTON_SECONDARY : GDK_BUTTON_PRIMARY, ctrl && !shift, ctrl && shift);
      sync_key_hold (self);
      return TRUE;
    }
  switch (keyval)
    {
    case GDK_KEY_Left: case GDK_KEY_KP_Left: dx = -1; break;
    case GDK_KEY_Right: case GDK_KEY_KP_Right: dx = 1; break;
    case GDK_KEY_Up: case GDK_KEY_KP_Up: dy = -1; break;
    case GDK_KEY_Down: case GDK_KEY_KP_Down: dy = 1; break;
    default: return FALSE;
    }
  moves_selection = !held && !alt && easel_canvas_get_selection_size (self->canvas, &w, &h);
  if ((ctrl && !(shift && moves_selection)) || mouse_held)
    return mouse_held;
  /* With a selection, Shift smears by 1 and Ctrl+Shift moves by 10 (REQ-0013-10); else Shift is the marker's 10. */
  step = shift && (!moves_selection || ctrl) ? 10 : 1;
  easel_canvas_marker_show (self->canvas, TRUE);
  if (held)
    easel_canvas_set_shift (self->canvas, shift);
  /* With a selection and nothing held, an arrow moves it and the marker goes along; Alt moves the marker alone (REQ-0022-08). */
  if (moves_selection && shift && !ctrl)
    easel_canvas_smear_selection (self->canvas, dx, dy);
  else if (moves_selection)
    easel_canvas_move_selection (self->canvas, dx * step, dy * step);
  else if (alt && !easel_canvas_get_selection_size (self->canvas, &w, &h))
    return FALSE;
  easel_canvas_key_move (self->canvas, dx * step, dy * step);
  marker_into_view (self);
  return TRUE;
}

gboolean
easel_window_double_click_ignored (EaselWindow *self, GtkWidget *bar, double x, double y)
{
  /* The pick passes over a disabled button, down to what holds it. */
  GtkWidget *picked = gtk_widget_pick (bar, x, y, GTK_PICK_DEFAULT);

  if (picked == NULL)
    return TRUE;
  if (gtk_widget_get_ancestor (picked, GTK_TYPE_BUTTON) != NULL || gtk_widget_get_ancestor (picked, GTK_TYPE_MENU_BUTTON) != NULL
      || EASEL_IS_SWATCH (picked) || EASEL_IS_COLOR_INDICATOR (picked))
    return FALSE;
  /* The title keeps what the desktop gives it. */
  return !(bar == self->header_bar && GTK_IS_LABEL (picked));
}

/* Before the bar's window handle sees it (REQ-0021-15). */
static void
on_bar_pressed (GtkGestureClick *gesture, int n_press, double x, double y, EaselWindow *self)
{
  if (n_press == 2 && easel_window_double_click_ignored (self, gtk_event_controller_get_widget (GTK_EVENT_CONTROLLER (gesture)), x, y))
    gtk_gesture_set_state (GTK_GESTURE (gesture), GTK_EVENT_SEQUENCE_CLAIMED);
}

/* ---- Announcements (0022) ---- */

/* Nothing while the app is starting: what it starts with is not news. */
void
easel_window_announce (EaselWindow *self, const char *text)
{
  if (self->picture == NULL)
    return;
  /* GTK's test backend, which reads names for the tests, crashes on an announcement (GTK 4.24). */
  if (g_strcmp0 (g_getenv ("GTK_A11Y"), "test") != 0)
    gtk_accessible_announce (GTK_ACCESSIBLE (self), text, GTK_ACCESSIBLE_ANNOUNCEMENT_PRIORITY_MEDIUM);
  if (self->announced != NULL)
    g_string_append_printf (self->announced, "%s%s", self->announced->len > 0 ? "|" : "", text);
}

void
easel_window_log_announcements (EaselWindow *self, gboolean on)
{
  if (self->announced != NULL)
    g_string_free (self->announced, TRUE);
  self->announced = on ? g_string_new (NULL) : NULL;
}

char *
easel_window_take_announcements (EaselWindow *self)
{
  char *said = g_strdup (self->announced->str);

  g_string_truncate (self->announced, 0);
  return said;
}

static void
announce_color (EaselWindow *self, gboolean foreground, uint32_t c)
{
  g_autofree char *hex = (c >> 24) == 0xFF ? g_strdup_printf ("#%06X", (unsigned) (c & 0xFFFFFF))
                                           : g_strdup_printf ("#%06X%02X", (unsigned) (c & 0xFFFFFF), (unsigned) (c >> 24));
  g_autofree char *text = g_strdup_printf (foreground ? _ ("Foreground %s") : _ ("Background %s"), hex);

  easel_window_announce (self, text);
}

/* The colors, unless a pick is going on: that says its color at the release (REQ-0022-15). */
static void
announce_colors (EaselWindow *self)
{
  if (self->tool == EASEL_TOOL_PICK && easel_canvas_is_stroking (self->canvas))
    return;
  if (self->foreground != self->said_foreground)
    announce_color (self, TRUE, self->foreground);
  if (self->background != self->said_background)
    announce_color (self, FALSE, self->background);
  self->said_foreground = self->foreground;
  self->said_background = self->background;
}

static void
announce_selection (EaselWindow *self)
{
  int x, y, w = 0, h = 0;
  gboolean floating = FALSE, box = easel_canvas_text_buffer (self->canvas) != NULL;
  gboolean exists = !box && !easel_canvas_is_stroking (self->canvas) && easel_canvas_get_selection (self->canvas, &x, &y, &w, &h, &floating);

  if (box != self->said_box)
    easel_window_announce (self, box ? _ ("Text box open") : _ ("Text box closed"));
  self->said_box = box;
  if (easel_canvas_is_stroking (self->canvas))
    return;
  if (exists && !self->said_selection)
    {
      g_autofree char *text = g_strdup_printf (_ ("Selection %d by %d"), w, h);

      easel_window_announce (self, text);
    }
  else if (!exists && self->said_selection)
    easel_window_announce (self, self->said_floating ? _ ("Selection put down") : _ ("Selection removed"));
  self->said_selection = exists;
  self->said_floating = exists && floating;
}

static void
on_polygon_closed (EaselWindow *self)
{
  easel_window_announce (self, _ ("Polygon closed"));
}

static const char *
tool_title (EaselTool tool)
{
  static const char *const titles[] = {
    [EASEL_TOOL_PAN] = N_ ("Pan"), [EASEL_TOOL_PENCIL] = N_ ("Pencil"), [EASEL_TOOL_BRUSH] = N_ ("Brush"), [EASEL_TOOL_AIRBRUSH] = N_ ("Airbrush"), [EASEL_TOOL_ERASER] = N_ ("Eraser"),
    [EASEL_TOOL_MAGNIFIER] = N_ ("Magnifier"), [EASEL_TOOL_PICK] = N_ ("Pick Color"), [EASEL_TOOL_FILL] = N_ ("Fill"), [EASEL_TOOL_LINE] = N_ ("Line"), [EASEL_TOOL_RECTANGLE] = N_ ("Rectangle"),
    [EASEL_TOOL_ROUNDED_RECTANGLE] = N_ ("Rounded Rectangle"), [EASEL_TOOL_ELLIPSE] = N_ ("Ellipse"), [EASEL_TOOL_CURVE] = N_ ("Curve"), [EASEL_TOOL_POLYGON] = N_ ("Polygon"),
    [EASEL_TOOL_GRADIENT] = N_ ("Gradient"), [EASEL_TOOL_SELECT] = N_ ("Select"), [EASEL_TOOL_FREE_SELECT] = N_ ("Free-Form Select"), [EASEL_TOOL_TEXT] = N_ ("Text"),
  };

  return _ (titles[tool]);
}

/* A button with no words of its own is named by its tooltip (REQ-0022-14). */
static void
name_by_tooltip (GtkWidget *w)
{
  if (GTK_IS_BUTTON (w) || GTK_IS_MENU_BUTTON (w))
    {
      const char *tip = gtk_widget_get_tooltip_text (w);

      if (tip != NULL)
        gtk_accessible_update_property (GTK_ACCESSIBLE (w), GTK_ACCESSIBLE_PROPERTY_LABEL, tip, -1);
    }
  for (GtkWidget *c = gtk_widget_get_first_child (w); c != NULL; c = gtk_widget_get_next_sibling (c))
    name_by_tooltip (c);
}

/* ---- Keyboard Shortcuts, Help, About Easel (0021) ---- */

/* Opening any of them changes nothing, and each has the same gates (REQ-0021-01). */
static gboolean
extras_allowed (EaselWindow *self)
{
  return view_actions_allowed (self) && !easel_canvas_is_panning (self->canvas) && card_field_focus (self) == NULL;
}

static void
on_extra_dialog_closed (EaselWindow *self)
{
  self->extra_dialog = NULL;
}

static void
present_extra_dialog (EaselWindow *self, AdwDialog *dialog)
{
  self->extra_dialog = dialog;
  g_signal_connect_object (dialog, "closed", G_CALLBACK (on_extra_dialog_closed), self, G_CONNECT_SWAPPED);
  adw_dialog_present (dialog, GTK_WIDGET (self));
}

static void
shortcuts_activated (GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
  EaselWindow *self = user_data;

  if (extras_allowed (self))
    present_extra_dialog (self, easel_shortcuts_dialog_new ());
}

static void
about_activated (GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
  EaselWindow *self = user_data;

  if (extras_allowed (self))
    present_extra_dialog (self, easel_about_dialog_new ());
}

/* No button is held and no dialog is open. */
static gboolean
extras_can_show_error (EaselWindow *self)
{
  return view_actions_allowed (self) && !easel_canvas_is_panning (self->canvas);
}

static void
show_help_error (EaselWindow *self)
{
  AdwDialog *dialog = adw_alert_dialog_new (_ ("Could Not Open Help"), _ ("The help page could not be opened."));

  adw_alert_dialog_add_response (ADW_ALERT_DIALOG (dialog), "ok", _ ("OK"));
  adw_alert_dialog_set_default_response (ADW_ALERT_DIALOG (dialog), "ok");
  adw_alert_dialog_set_close_response (ADW_ALERT_DIALOG (dialog), "ok");
  present_extra_dialog (self, dialog);
}

/* A failure reported late waits until no button is held and no dialog is open (REQ-0021-04). */
static gboolean
help_error_timer_fired (gpointer data)
{
  EaselWindow *self = data;

  if (!extras_can_show_error (self))
    return G_SOURCE_CONTINUE;
  self->help_error_timer = 0;
  show_help_error (self);
  return G_SOURCE_REMOVE;
}

void
easel_window_help_finished (EaselWindow *self, gboolean ok)
{
  self->help_starting = FALSE;
  if (ok || self->help_dropped)
    return;
  if (extras_can_show_error (self))
    show_help_error (self);
  else if (self->help_error_timer == 0)
    self->help_error_timer = g_timeout_add (100, help_error_timer_fired, self);
}

static void
help_launched (GObject *source, GAsyncResult *result, gpointer data)
{
  g_autoptr (EaselWindow) self = data;
  g_autoptr (GError) error = NULL;

  easel_window_help_finished (self, g_app_info_launch_default_for_uri_finish (result, &error));
}

static void
launch_in_browser (EaselWindow *win, const char *uri, gpointer data)
{
  g_autoptr (GdkAppLaunchContext) context = gdk_display_get_app_launch_context (gtk_widget_get_display (GTK_WIDGET (win)));

  /* GLib's own launch: GTK's URI launcher refuses a file:// page and its file launcher asks "Open With" through the portal (JOD's M1, 2026-11-03). The window
     stays until the answer; a window gone has dropped the failure. */
  g_app_info_launch_default_for_uri_async (uri, G_APP_LAUNCH_CONTEXT (context), NULL, help_launched, g_object_ref (win));
}

void
easel_window_set_help_launcher (EaselWindow *self, EaselHelpLauncher launch, gpointer data)
{
  self->help_launch = launch;
  self->help_launch_data = data;
}

static void
help_activated (GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
  EaselWindow *self = user_data;
  const char *page = easel_help_page_path ();
  g_autofree char *uri = NULL;

  if (!extras_allowed (self) || self->help_starting)
    return;
  self->help_dropped = FALSE;
  /* A missing page needs no browser to be seen. */
  if (!g_file_test (page, G_FILE_TEST_IS_REGULAR) || (uri = g_filename_to_uri (page, NULL, NULL)) == NULL)
    {
      show_help_error (self);
      return;
    }
  self->help_starting = TRUE;
  self->help_launch (self, uri, self->help_launch_data);
}

/* ---- Print (0026) ---- */

const EaselPageSetup *
easel_window_get_page_setup (EaselWindow *self)
{
  return &self->page_setup;
}

static gboolean
print_error_timer_fired (gpointer data)
{
  EaselWindow *self = data;

  /* Nothing left to show, or a window closed during sending, with no one to tell (REQ-0026-16). */
  if (self->print_errors == 0 || gtk_window_get_application (GTK_WINDOW (self)) == NULL)
    {
      self->print_error_timer = 0;
      return G_SOURCE_REMOVE;
    }
  if (extras_can_show_error (self))
    {
      AdwDialog *dialog = adw_alert_dialog_new (_ ("Could Not Print"), _ ("The picture could not be printed."));

      adw_alert_dialog_add_response (ADW_ALERT_DIALOG (dialog), "ok", _ ("OK"));
      adw_alert_dialog_set_default_response (ADW_ALERT_DIALOG (dialog), "ok");
      adw_alert_dialog_set_close_response (ADW_ALERT_DIALOG (dialog), "ok");
      self->print_errors--;
      present_extra_dialog (self, dialog);
    }
  return G_SOURCE_CONTINUE;
}

/* One dialog for each failed print, as soon as no button is held and no dialog is open (REQ-0026-15). */
static void
print_failed (EaselWindow *self)
{
  /* A window closed during sending has no one to tell (REQ-0026-16). */
  if (gtk_window_get_application (GTK_WINDOW (self)) == NULL)
    return;
  self->print_errors++;
  if (self->print_error_timer == 0)
    self->print_error_timer = g_timeout_add (50, print_error_timer_fired, self);
  print_error_timer_fired (self);
}

void
easel_window_set_print_runner (EaselWindow *self, EaselPrintRunner run, gpointer data)
{
  self->print_run = run;
  self->print_run_data = data;
}

void
easel_window_set_print_export (EaselWindow *self, const char *path)
{
  g_free (self->print_export);
  self->print_export = g_strdup (path);
}

void
easel_window_set_print_copy_limit (EaselWindow *self, gsize max_bytes)
{
  self->print_copy_limit = max_bytes;
}

gboolean
easel_window_print_is_sending (EaselWindow *self)
{
  return self->print_sending > 0;
}

void
easel_window_print_cancelled (EaselWindow *self)
{
  self->print_dialog_open = FALSE;
}

EaselPrintJob *
easel_window_print_confirmed (EaselWindow *self, double paper_w_mm, double paper_h_mm)
{
  EaselPrintPage page;
  EaselPrintJob *job = NULL;

  self->print_dialog_open = FALSE;
  easel_page_setup_page (&self->page_setup, self->picture->width, self->picture->height, &page);
  page.paper_w_mm = paper_w_mm;
  page.paper_h_mm = paper_h_mm;
  if (easel_print_area_ok (paper_w_mm - 2 * page.margins_mm, paper_h_mm - 2 * page.margins_mm, page.fit, page.percent))
    job = easel_print_job_new_limited (self->picture, &page, self->print_copy_limit);
  if (job == NULL)
    {
      print_failed (self);
      return NULL;
    }
  self->print_sending++;
  g_application_hold (g_application_get_default ());
  return job;
}

void
easel_window_print_finished (EaselWindow *self, EaselPrintJob *job, gboolean ok)
{
  if (job != NULL)
    {
      easel_print_job_free (job);
      self->print_sending--;
      g_application_release (g_application_get_default ());
    }
  if (!ok)
    print_failed (self);
}

/* What a confirm keeps for the next print: the tab's choices, the paper, and the orientation, which stops following the picture once it differs (REQ-0026-04). */
void
easel_window_print_remember (EaselWindow *self, GtkWidget *tab, const char *paper, gboolean landscape)
{
  if (tab != NULL)
    easel_print_tab_read (tab, &self->page_setup);
  if (paper == NULL)
    return;
  if (paper != self->page_setup.paper)
    {
      g_free (self->page_setup.paper);
      self->page_setup.paper = g_strdup (paper);
    }
  if (landscape != easel_page_setup_landscape (&self->page_setup, self->picture->width, self->picture->height))
    {
      self->page_setup.orientation_set = TRUE;
      self->page_setup.landscape = landscape;
    }
}

/* The real operation: GTK's dialog, then a job per confirm. */
static GObject *
on_create_custom_widget (GtkPrintOperation *op, EaselWindow *self)
{
  return G_OBJECT (easel_print_tab_new (&self->page_setup, self->picture->width, self->picture->height));
}

static void
on_update_custom_widget (GtkPrintOperation *op, GtkWidget *tab, GtkPageSetup *setup, GtkPrintSettings *settings, EaselWindow *self)
{
  easel_print_tab_set_paper (tab, gtk_page_setup_get_paper_width (setup, GTK_UNIT_MM), gtk_page_setup_get_paper_height (setup, GTK_UNIT_MM));
}

static void
on_custom_widget_apply (GtkPrintOperation *op, GtkWidget *tab, EaselWindow *self)
{
  easel_window_print_remember (self, tab, NULL, FALSE);
}

static void
on_begin_print (GtkPrintOperation *op, GtkPrintContext *context, EaselWindow *self)
{
  GtkPageSetup *setup = gtk_print_context_get_page_setup (context);
  EaselPrintJob *job;

  easel_window_print_remember (self, NULL, gtk_paper_size_get_name (gtk_page_setup_get_paper_size (setup)),
                               gtk_page_setup_get_orientation (setup) == GTK_PAGE_ORIENTATION_LANDSCAPE);
  job = easel_window_print_confirmed (self, gtk_page_setup_get_paper_width (setup, GTK_UNIT_MM),
                                                     gtk_page_setup_get_paper_height (setup, GTK_UNIT_MM));

  g_object_set_data (G_OBJECT (op), "easel-print-job", job);
  if (job != NULL)
    gtk_print_operation_set_n_pages (op, easel_print_job_sheets (job));
  else
    gtk_print_operation_cancel (op);
}

static void
on_draw_page (GtkPrintOperation *op, GtkPrintContext *context, int page, EaselWindow *self)
{
  EaselPrintJob *job = g_object_get_data (G_OBJECT (op), "easel-print-job");

  if (job != NULL)
    easel_print_job_draw_sheet (job, gtk_print_context_get_cairo_context (context), page, gtk_print_context_get_dpi_x (context));
}

/* Ends what run_print began: the job, if there was one, and the reference on the window. */
static void
on_print_done (GtkPrintOperation *op, GtkPrintOperationResult result, EaselWindow *self)
{
  EaselPrintJob *job = g_object_get_data (G_OBJECT (op), "easel-print-job");

  if (result == GTK_PRINT_OPERATION_RESULT_APPLY)
    g_set_object (&self->print_settings, gtk_print_operation_get_print_settings (op));
  if (job == NULL && result != GTK_PRINT_OPERATION_RESULT_ERROR)
    easel_window_print_cancelled (self);       /* cancelled at the dialog, or the failure is already told */
  else
    {
      self->print_dialog_open = FALSE;
      easel_window_print_finished (self, job, result != GTK_PRINT_OPERATION_RESULT_ERROR);
    }
  g_object_set_data (G_OBJECT (op), "easel-print-job", NULL);
  g_object_unref (self);
  g_object_unref (op);
}

static void
run_print (EaselWindow *self, gpointer data)
{
  GtkPrintOperation *op = gtk_print_operation_new ();
  g_autoptr (GtkPageSetup) setup = gtk_page_setup_new ();
  g_autoptr (GtkPaperSize) paper = gtk_paper_size_new (self->page_setup.paper);
  const char *title = gtk_window_get_title (GTK_WINDOW (self));
  g_autofree char *name = g_strdup (g_str_has_prefix (title, "• ") ? title + strlen ("• ") : title);
  char *suffix = g_strrstr (name, " – Easel");
  GtkPrintOperationResult result;
  g_autoptr (GError) error = NULL;

  if (suffix != NULL)
    *suffix = '\0';
  gtk_page_setup_set_paper_size (setup, paper);
  gtk_page_setup_set_orientation (setup, easel_page_setup_landscape (&self->page_setup, self->picture->width, self->picture->height)
                                         ? GTK_PAGE_ORIENTATION_LANDSCAPE : GTK_PAGE_ORIENTATION_PORTRAIT);
  gtk_print_operation_set_default_page_setup (op, setup);
  gtk_print_operation_set_use_full_page (op, TRUE);
  gtk_print_operation_set_allow_async (op, TRUE);
  gtk_print_operation_set_embed_page_setup (op, TRUE);       /* so that the dialog's Orientation shows what the page is (a probe, T009) */
  gtk_print_operation_set_custom_tab_label (op, _ ("Picture"));
  g_signal_connect (op, "create-custom-widget", G_CALLBACK (on_create_custom_widget), self);
  g_signal_connect (op, "update-custom-widget", G_CALLBACK (on_update_custom_widget), self);
  g_signal_connect (op, "custom-widget-apply", G_CALLBACK (on_custom_widget_apply), self);
  gtk_print_operation_set_job_name (op, name);
  if (self->print_settings != NULL)
    gtk_print_operation_set_print_settings (op, self->print_settings);
  g_signal_connect (op, "begin-print", G_CALLBACK (on_begin_print), self);
  g_signal_connect (op, "draw-page", G_CALLBACK (on_draw_page), self);
  g_signal_connect (op, "done", G_CALLBACK (on_print_done), self);
  g_object_ref (self);
  self->print_dialog_open = TRUE;
  if (self->print_export != NULL)
    gtk_print_operation_set_export_filename (op, self->print_export);
  result = gtk_print_operation_run (op, self->print_export != NULL ? GTK_PRINT_OPERATION_ACTION_EXPORT : GTK_PRINT_OPERATION_ACTION_PRINT_DIALOG,
                                    GTK_WINDOW (self), &error);
  /* An answer at once, with no "done" to come: GTK emits it only for a run in progress. */
  if (result == GTK_PRINT_OPERATION_RESULT_ERROR && g_object_get_data (G_OBJECT (op), "easel-print-job") == NULL)
    {
      self->print_dialog_open = FALSE;
      print_failed (self);
    }
}

/* Past the prompt: what is held is put down, and if that fails its dialog shows and there is no print (REQ-0026-12). */
static void
print_go (EaselWindow *self)
{
  self->put_down_failed = FALSE;
  /* A shape's or a gradient's dialog comes after the action, so it is the signal that says it failed. */
  if (!easel_canvas_put_down_keeping_marked (self->canvas) || self->put_down_failed || dialog_is_open (self))
    return;
  self->print_dialog_open = TRUE;
  self->print_run (self, self->print_run_data);
}

static void
print_activated (GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
  EaselWindow *self = user_data;

  if (extras_allowed (self))
    print_go (self);
}

EaselSettings *
easel_window_get_chosen_settings (EaselWindow *self)
{
  return &self->chosen;
}

void
easel_window_preferences_opened (EaselWindow *self, AdwDialog *dialog)
{
  self->preferences = dialog;
}

void
easel_window_preferences_closed (EaselWindow *self)
{
  self->preferences = NULL;
  if (self->key_down == GDK_KEY_Return || self->key_down == GDK_KEY_KP_Enter || self->key_down == GDK_KEY_ISO_Enter || self->key_down == GDK_KEY_space)
    self->closed_key = self->key_down;
}

gboolean
easel_window_write_settings (EaselWindow *self, const EaselSettingChange *changes, gsize n)
{
  g_autofree char *path = easel_settings_file_path ();

  return easel_settings_file_change (path, changes, n);
}

void
easel_window_show_language_notice (EaselWindow *self)
{
  AdwDialog *dialog = adw_alert_dialog_new (_ ("Language Changes at Next Start"), _ ("Your changes are kept. The new language shows the next time Easel starts."));

  adw_alert_dialog_add_response (ADW_ALERT_DIALOG (dialog), "ok", _ ("_OK"));
  adw_dialog_present (dialog, GTK_WIDGET (self));
}

void
easel_window_show_settings_error (EaselWindow *self)
{
  AdwDialog *dialog = adw_alert_dialog_new (_ ("Could Not Save the Preferences"),
                                            _ ("The changes apply until Easel closes where they can, but settings.ini could not be written."));

  adw_alert_dialog_add_response (ADW_ALERT_DIALOG (dialog), "close", _ ("Close"));
  g_set_weak_pointer (&self->settings_error, dialog);
  adw_dialog_present (dialog, GTK_WIDGET (self));
}

void
easel_window_preview_workspace (EaselWindow *self, const EaselSettings *draft)
{
  self->settings.has_workspace_color = draft->has_workspace_color;
  self->settings.workspace_color = draft->workspace_color;
  self->settings.has_workspace_color_dark = draft->has_workspace_color_dark;
  self->settings.workspace_color_dark = draft->workspace_color_dark;
  apply_workspace_color (&self->settings);
}

void
easel_window_apply_style (EaselWindow *self, EaselStyle style)
{
  self->settings.style = style;
  adw_style_manager_set_color_scheme (adw_style_manager_get_default (), style == EASEL_STYLE_LIGHT ? ADW_COLOR_SCHEME_FORCE_LIGHT
                                      : style == EASEL_STYLE_DARK ? ADW_COLOR_SCHEME_FORCE_DARK : ADW_COLOR_SCHEME_DEFAULT);
}

void
easel_window_apply_tint (EaselWindow *self, int percent)
{
  self->settings.tint = percent;
  easel_canvas_set_tint (self->canvas, percent);
}

void
easel_window_apply_panels (EaselWindow *self, gboolean toolbox, gboolean colorbox, gboolean statusbar)
{
  set_panel_state (self, "show-toolbox", self->toolbox_panel, toolbox);
  set_panel_state (self, "show-colorbox", GTK_WIDGET (self->palette_bar), colorbox);
  set_panel_state (self, "show-statusbar", GTK_WIDGET (self->status_bar), statusbar);
}

void
easel_window_apply_chosen (EaselWindow *self)
{
  self->settings.width = self->chosen.width;
  self->settings.height = self->chosen.height;
  self->settings.background = self->chosen.background;
  self->settings.jpeg_quality = self->chosen.jpeg_quality;
  self->settings.has_workspace_color = self->chosen.has_workspace_color;
  self->settings.workspace_color = self->chosen.workspace_color;
  self->settings.has_workspace_color_dark = self->chosen.has_workspace_color_dark;
  self->settings.workspace_color_dark = self->chosen.workspace_color_dark;
  apply_workspace_color (&self->settings);
}

void
easel_window_automatic_size (EaselWindow *self, int *width, int *height)
{
  EaselSettings automatic = self->settings;
  int area_w = 0, area_h = 0;

  automatic.width = automatic.height = EASEL_SIZE_AUTO;
  canvas_area_device (self, &area_w, &area_h);
  easel_settings_resolve_size (&automatic, area_w, area_h, width, height);
}

const EaselPalette *
easel_window_get_palette (EaselWindow *self)
{
  return &self->palette;
}

void
easel_window_set_palette (EaselWindow *self, const EaselPalette *palette)
{
  self->palette = *palette;
  easel_palette_bar_set_palette (self->palette_bar, &self->palette);
}

void
easel_window_remove_palette_file (EaselWindow *self)
{
  g_autofree char *path = easel_palette_file_path ();

  if (g_remove (path) != 0)
    g_debug ("Cannot remove %s", path);          /* the swatches keep the classic colors for this run, silently (REQ-0027-12) */
}

/* ---- Stretch and Skew (0019) ---- */

/* OK: the one function every Stretch and Skew goes through; the dialog has closed. */
static void
stretch_skew_applied (int W, int H, int amount_h, int amount_v, gpointer data)
{
  EaselWindow *self = data;
  EaselTransformResult r;

  if (easel_canvas_stretch_skew_selection (self->canvas, W, H, amount_h, amount_v))
    {
      sync_history_ui (self);
      return;
    }
  r = easel_image_stretch_skew (self->picture, self->history, W, H, amount_h, amount_v, self->background, self->transform_limit);
  if (r == EASEL_TRANSFORM_NO_MEMORY)
    show_save_error (self, _ ("Could Not Stretch or Skew the Picture"), _ ("There is not enough memory for this change."));
  if (r != EASEL_TRANSFORM_DONE)
    return;
  picture_size_changed (self);
  sync_history_ui (self);
}

/* The item and Ctrl+T (REQ-0019-01): opening the dialog puts down what waits, as Attributes does, and leaves a selection as it is (REQ-0019-06). */
static void
stretch_skew_activated (GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
  EaselWindow *self = user_data;
  int x, y, w, h;
  gboolean floating;

  if (!view_actions_allowed (self) || easel_canvas_is_panning (self->canvas) || card_field_focus (self) != NULL)
    return;
  if (!easel_canvas_get_selection (self->canvas, &x, &y, &w, &h, &floating))
    {
      if (!put_down_for_action (self, FALSE))
        return;
      w = self->picture->width;
      h = self->picture->height;
    }
  easel_stretch_skew_dialog_present (GTK_WIDGET (self), w, h, stretch_skew_applied, self);
}

/* ---- Black and White (0034) ---- */

/* Convert: the one function every conversion goes through; the dialog has closed. On the selection while one exists, else on the whole
   picture as one step (REQ-0034-03); without the memory, its dialog and nothing changed (REQ-0034-08). */
static void
black_white_applied (EaselBw how, gpointer data)
{
  EaselWindow *self = data;
  EaselTransformResult r;

  if (easel_canvas_black_white_selection (self->canvas, how))
    {
      sync_history_ui (self);
      return;
    }
  r = easel_image_black_white (self->picture, self->history, how, self->transform_limit);
  if (r == EASEL_TRANSFORM_NO_MEMORY)
    show_save_error (self, _ ("Could Not Convert the Picture"), _ ("There is not enough memory for this change."));
  if (r != EASEL_TRANSFORM_DONE)
    return;
  easel_canvas_update (self->canvas);
  sync_history_ui (self);
}

/* The item (REQ-0034-01): opening the dialog puts down what waits, as Stretch and Skew does, and leaves a selection as it is
   (REQ-0034-03). */
static void
black_white_activated (GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
  EaselWindow *self = user_data;
  int x, y, w, h;
  gboolean floating, selection;

  if (!view_actions_allowed (self) || easel_canvas_is_panning (self->canvas) || card_field_focus (self) != NULL)
    return;
  selection = easel_canvas_get_selection (self->canvas, &x, &y, &w, &h, &floating);
  if (!selection && !put_down_for_action (self, FALSE))
    return;
  /* As an extra dialog, so that a quit closes it as Cancel and goes on (REQ-0034-10, as REQ-0021-03). */
  present_extra_dialog (self, easel_black_white_dialog_new (selection, black_white_applied, self));
}

/* ---- Remove background (0035, ADR-0007) ---- */

/* One use of the command: what it acts on (the picture's pixels or the float's, as shown), their copy as before, the analysis, then the
   work at OK. The window holds it from the dialog's opening; the worker owns it while it runs. */
struct RbgSession
{
  EaselWindow *win;                      /* a reference, held until the end */
  gboolean selection, lifted_here;
  uint32_t *live;                        /* where the preview is written: the picture's or the float's own pixels */
  uint32_t *orig;                        /* those pixels as before the dialog */
  uint8_t *mask;                         /* the float's shape (a copy), NULL for a rectangle or the picture */
  int w, h;
  EaselRbg *rbg;
  int tolerance;                         /* the labels' tolerance */
  int shown;                             /* the dialog's number */
  guint idle;                            /* a preview due */
  gboolean restored;                     /* put back as before: the work's end changes nothing */
  uint32_t *dst;                         /* the result at OK */
  size_t removed;
  int cancel;                            /* asked to stop (any thread) */
  EaselRbgResult outcome;
  int slow_ms;                           /* the test hook EASEL_TEST_RBG_SLOW_MS: the work made that much slower (AS-9) */
};

static void
rbg_session_free (struct RbgSession *s)
{
  g_clear_handle_id (&s->idle, g_source_remove);
  easel_rbg_free (s->rbg);
  free (s->orig);
  free (s->mask);
  free (s->dst);
  g_object_unref (s->win);
  g_free (s);
}

/* A message the quit closes as Cancel, then goes on (REQ-0035-06, REQ-0035-12). */
static void
rbg_message (EaselWindow *self)
{
  AdwDialog *dialog = adw_alert_dialog_new (_ ("Could Not Remove the Background"), _ ("There is not enough memory for this change."));

  adw_alert_dialog_add_response (ADW_ALERT_DIALOG (dialog), "close", _ ("Close"));
  present_extra_dialog (self, dialog);
}

/* The pixels it acts on changed: shown again. */
static void
rbg_show (struct RbgSession *s)
{
  if (s->selection)
    easel_canvas_rbg_float_changed (s->win->canvas);
  else
    {
      s->win->picture->dirty = (EaselRect) { 0, 0, s->w, s->h };
      easel_canvas_update (s->win->canvas);
    }
}

/* Everything as before the dialog (REQ-0035-04's Cancel): the pixels, a selection the dialog lifted marked again. Once only. */
static void
rbg_restore (struct RbgSession *s)
{
  if (s->restored)
    return;
  s->restored = TRUE;
  memcpy (s->live, s->orig, (size_t) s->w * (size_t) s->h * sizeof *s->orig);
  rbg_show (s);
  if (s->lifted_here)
    easel_canvas_rbg_unlift (s->win->canvas);
}

/* The preview for the number shown (REQ-0035-04): the labels on the copy, each cell standing for its block. */
static void
rbg_preview (struct RbgSession *s)
{
  s->tolerance = s->shown;
  easel_rbg_label (s->rbg, s->tolerance, NULL);
  easel_rbg_preview (s->rbg, s->orig, s->mask, s->live, NULL);
  rbg_show (s);
}

static gboolean
on_rbg_idle (gpointer data)
{
  struct RbgSession *s = data;

  s->idle = 0;
  rbg_preview (s);
  return G_SOURCE_REMOVE;
}

/* A change of the number: its preview when the main loop is next idle, so that a run of changes makes one. */
static void
on_rbg_changed (int tolerance, gpointer data)
{
  struct RbgSession *s = data;

  s->shown = tolerance;
  if (s->idle == 0)
    s->idle = g_idle_add (on_rbg_idle, s);
}

/* The worker thread: the labels for the number shown, then the result at full size (REQ-0035-03). */
static void
rbg_work (GTask *task, gpointer source, gpointer data, GCancellable *cancellable)
{
  struct RbgSession *s = data;

  for (int waited = 0; waited < s->slow_ms && !__atomic_load_n (&s->cancel, __ATOMIC_SEQ_CST); waited += 10)
    g_usleep (10000);
  s->outcome = easel_rbg_label (s->rbg, s->shown, &s->cancel);
  if (s->outcome == EASEL_RBG_DONE)
    s->outcome = easel_rbg_result (s->rbg, s->orig, s->mask, s->dst, &s->removed, &s->cancel);
  if (__atomic_load_n (&s->cancel, __ATOMIC_SEQ_CST))
    s->outcome = EASEL_RBG_CANCELLED;
  g_task_return_boolean (task, TRUE);
}

/* Stops the work in progress, if any, from the main thread, or ends the dialog's preview: everything as before the dialog at once,
   nothing of it applied (REQ-0035-11, -12). */
static void
rbg_cancel (EaselWindow *self)
{
  if (self->rbg == NULL)
    return;                              /* while the dialog is open too: a quit puts the pixels back before it goes on */
  __atomic_store_n (&self->rbg->cancel, 1, __ATOMIC_SEQ_CST);
  rbg_restore (self->rbg);
}

static void
on_rbg_progress_closed (EaselWindow *self)
{
  self->rbg_progress = NULL;
  g_clear_handle_id (&self->rbg_pulse, g_source_remove);
  rbg_cancel (self);                     /* Cancel, Esc, or a quit closing it: a closing after the end does nothing */
}

static gboolean
on_rbg_pulse (gpointer data)
{
  gtk_progress_bar_pulse (GTK_PROGRESS_BAR (data));
  return G_SOURCE_CONTINUE;
}

/* After 0.5 s of work, the progress dialog (REQ-0035-11): its heading, a pulsing bar and Cancel. */
static gboolean
rbg_show_progress (gpointer data)
{
  EaselWindow *self = data;
  AdwDialog *dialog = adw_alert_dialog_new (_ ("Removing the Background"), NULL);
  GtkWidget *bar = gtk_progress_bar_new ();

  self->rbg_timer = 0;
  adw_alert_dialog_set_extra_child (ADW_ALERT_DIALOG (dialog), bar);
  adw_alert_dialog_add_response (ADW_ALERT_DIALOG (dialog), "cancel", _ ("_Cancel"));
  adw_alert_dialog_set_close_response (ADW_ALERT_DIALOG (dialog), "cancel");
  self->rbg_pulse = g_timeout_add_full (G_PRIORITY_DEFAULT, 100, on_rbg_pulse, g_object_ref (bar), g_object_unref);
  g_signal_connect_object (dialog, "closed", G_CALLBACK (on_rbg_progress_closed), self, G_CONNECT_SWAPPED);
  self->rbg_progress = dialog;
  present_extra_dialog (self, dialog);
  return G_SOURCE_REMOVE;
}

/* The one function every end of the work goes through, on the main thread: the progress dialog closed, the pixels put back as before,
   then the result kept as one step (REQ-0035-02), or no step when nothing is removed (REQ-0035-05), or the memory's message
   (REQ-0035-06), or nothing (cancelled, already put back). */
static void
rbg_finished (GObject *source, GAsyncResult *result, gpointer data)
{
  struct RbgSession *s = data;
  EaselWindow *self = s->win;
  AdwDialog *progress = self->rbg_progress;
  gboolean cancelled = s->restored || s->outcome != EASEL_RBG_DONE;

  self->rbg = NULL;
  self->removing = FALSE;
  g_clear_handle_id (&self->rbg_timer, g_source_remove);
  g_clear_handle_id (&self->rbg_pulse, g_source_remove);
  if (progress != NULL)
    {
      self->rbg_progress = NULL;
      g_signal_handlers_disconnect_by_func (progress, on_rbg_progress_closed, self);
      adw_dialog_force_close (progress);
    }
  on_visible_dialog_changed (self);
  if (cancelled)
    rbg_restore (s);
  else
    {
      s->restored = TRUE;                /* the pixels as before, then the result kept by the step's own path */
      memcpy (s->live, s->orig, (size_t) s->w * (size_t) s->h * sizeof *s->orig);
      if (s->removed == 0)
        rbg_show (s);                    /* no step; a selection the dialog lifted stays floating (REQ-0035-05) */
      else if (s->selection)
        {
          easel_canvas_rbg_float_replace (self->canvas, s->dst, s->lifted_here);
          sync_history_ui (self);
        }
      else
        {
          EaselTransformResult r = easel_image_replace_pixels (self->picture, self->history, s->dst, self->transform_limit);

          s->dst = NULL;                 /* taken */
          rbg_show (s);
          if (r == EASEL_TRANSFORM_NO_MEMORY)
            rbg_message (self);
          sync_history_ui (self);
        }
    }
  rbg_session_free (s);
}

/* The dialog closed. Cancel: everything as before. OK: the work in a worker thread, the window's input held, the progress dialog
   after 0.5 s (REQ-0035-11). */
static void
on_rbg_closed (gboolean ok, int tolerance, gpointer data)
{
  struct RbgSession *s = data;
  EaselWindow *self = s->win;
  size_t n = (size_t) s->w * (size_t) s->h, held = easel_rbg_bytes (s->rbg) + n * (sizeof *s->orig + (s->mask != NULL));
  GTask *task;

  g_clear_handle_id (&s->idle, g_source_remove);
  s->shown = tolerance;
  if (self->rbg_card != NULL)
    {
      gtk_overlay_remove_overlay (self->canvas_overlay, self->rbg_card);
      self->rbg_card = NULL;
      on_visible_dialog_changed (self);
      gtk_widget_grab_focus (GTK_WIDGET (self->canvas));   /* the keys go back to the canvas */
    }
  if (!ok)
    {
      rbg_restore (s);
      self->rbg = NULL;
      rbg_session_free (s);
      return;
    }
  if (held > self->transform_limit || n * sizeof *s->dst > self->transform_limit - held
      || (s->dst = malloc (n * sizeof *s->dst)) == NULL)
    {
      rbg_restore (s);
      self->rbg = NULL;
      rbg_session_free (s);
      rbg_message (self);
      return;
    }
  self->removing = TRUE;
  on_visible_dialog_changed (self);
  self->rbg_timer = g_timeout_add (500, rbg_show_progress, self);
  task = g_task_new (NULL, NULL, rbg_finished, s);
  g_task_set_task_data (task, s, NULL);
  g_task_run_in_thread (task, rbg_work);
  g_object_unref (task);
}

/* The item (REQ-0035-01, -02, -04): what waits is put down, a marked selection lifted, the pixels as before copied, the analysis
   made; then the dialog with the preview for the automatic tolerance; or the memory's message and nothing changed (REQ-0035-06). */
static void
remove_background_activated (GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
  EaselWindow *self = user_data;
  struct RbgSession *s;
  int x, y, w, h;
  gboolean floating;
  const uint8_t *shape = NULL;
  const char *slow = g_getenv ("EASEL_TEST_RBG_SLOW_MS");
  size_t n, held;
  GtkWidget *card;

  if (!view_actions_allowed (self) || easel_canvas_is_panning (self->canvas) || card_field_focus (self) != NULL || self->rbg != NULL)
    return;
  s = g_new0 (struct RbgSession, 1);
  s->win = g_object_ref (self);
  s->slow_ms = slow != NULL ? atoi (slow) : 0;
  s->selection = easel_canvas_get_selection (self->canvas, &x, &y, &w, &h, &floating);
  if (s->selection)
    {
      s->live = easel_canvas_rbg_float (self->canvas, &s->lifted_here, &shape, &s->w, &s->h);
      if (s->live == NULL)
        {
          rbg_session_free (s);          /* the lift's failure: its message shown, the selection marked (REQ-0035-06) */
          return;
        }
    }
  else
    {
      if (!put_down_for_action (self, FALSE))
        {
          rbg_session_free (s);
          return;
        }
      s->live = self->picture->px;
      s->w = self->picture->width;
      s->h = self->picture->height;
    }
  n = (size_t) s->w * (size_t) s->h;
  held = n * (sizeof *s->orig + (shape != NULL));
  if (held <= self->transform_limit && (s->orig = malloc (n * sizeof *s->orig)) != NULL
      && (shape == NULL || (s->mask = malloc (n)) != NULL))
    {
      memcpy (s->orig, s->live, n * sizeof *s->orig);
      if (shape != NULL)
        memcpy (s->mask, shape, n);
      s->rbg = easel_rbg_new (s->orig, s->mask, s->w, s->h, self->transform_limit - held);
    }
  if (s->rbg == NULL)
    {
      s->restored = TRUE;                /* nothing written yet */
      if (s->lifted_here)
        easel_canvas_rbg_unlift (self->canvas);
      rbg_session_free (s);
      rbg_message (self);
      return;
    }
  self->rbg = s;
  s->shown = easel_rbg_auto_tolerance (s->rbg);
  /* The card in the canvas area's bottom right corner, over the view, as the thumbnail is in its top right (REQ-0035-04). */
  card = easel_remove_bg_card_new (s->shown, easel_rbg_complex (s->rbg), on_rbg_changed, on_rbg_closed, s);
  gtk_widget_set_halign (card, GTK_ALIGN_END);
  gtk_widget_set_valign (card, GTK_ALIGN_END);
  gtk_widget_set_margin_end (card, 12);
  gtk_widget_set_margin_bottom (card, 12);
  self->rbg_card = card;
  gtk_overlay_add_overlay (self->canvas_overlay, card);
  on_visible_dialog_changed (self);
  gtk_widget_grab_focus (GTK_WIDGET (easel_remove_bg_card_row (card)));
  rbg_preview (s);
}

gboolean
easel_window_is_removing_background (EaselWindow *self)
{
  return self->removing;
}

/* Remove Background's card while it is open, for the tests; NULL otherwise. */
GtkWidget *
easel_window_remove_background_card (EaselWindow *self)
{
  return self->rbg_card;
}

static void
add_remove_background_action (EaselWindow *self)
{
  GSimpleAction *action = g_simple_action_new ("remove-background", NULL);

  g_signal_connect (action, "activate", G_CALLBACK (remove_background_activated), self);
  g_action_map_add_action (G_ACTION_MAP (self), G_ACTION (action));
  g_object_unref (action);
}

static void
invert_activated (GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
  image_action (user_data, EASEL_OP_INVERT);
}

static void
clear_image_activated (GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
  image_action (user_data, EASEL_OP_CLEAR);
}

/* Undo and redo change only the picture: the view, tool and colors stay (REQ-0002-08). */
static void
step_history (EaselWindow *self, gboolean undo)
{
  int old_w, old_h;

  if (self->picture == NULL || dialog_is_open (self) || easel_canvas_is_resizing (self->canvas))
    return;

  /* A pending curve or polygon: Undo drops it and undoes nothing else, Redo does nothing (REQ-0011-11). */
  if (easel_canvas_is_pending (self->canvas))
    {
      if (undo)
        easel_canvas_drop (self->canvas);
      return;
    }
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
  /* An open text box: Undo and Redo act on its edits, not on the picture's steps (REQ-0017-08); the box's change syncs the
     actions through selection-changed. */
  if (easel_canvas_text_step (self->canvas, undo))
    {
      easel_window_announce (self, undo ? _ ("Undone") : _ ("Redone"));
      return;
    }
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
  easel_window_announce (self, undo ? _ ("Undone") : _ ("Redone"));
  easel_canvas_deselect (self->canvas);           /* Undo or Redo removes a marked selection (REQ-0013-12) */
  if (self->picture->width != old_w || self->picture->height != old_h)
    picture_size_changed (self);
  else
    easel_canvas_update (self->canvas);
  sync_history_ui (self);
}

static void
undo_activated (GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
  if (!card_field_action (EASEL_WINDOW (user_data), "text.undo"))
    step_history (EASEL_WINDOW (user_data), TRUE);
}

static void
redo_activated (GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
  if (!card_field_action (EASEL_WINDOW (user_data), "text.redo"))
    step_history (EASEL_WINDOW (user_data), FALSE);
}

/* A handle drag ended on a new size (REQ-0004-18). */
static void
on_resize_requested (EaselWindow *self, int width, int height)
{
  easel_window_resize_picture (self, width, height);
}

/* The wheel moves the view as GTK's scrolled window does (Shift for across). */
static void
scroll_view_as_scroller (EaselWindow *self, GtkEventControllerScroll *controller, double dx, double dy)
{
  GdkModifierType state = gtk_event_controller_get_current_event_state (GTK_EVENT_CONTROLLER (controller));
  gboolean across = (state & GDK_SHIFT_MASK) || (dy == 0 && dx != 0);
  GtkAdjustment *adj = across ? gtk_scrolled_window_get_hadjustment (self->scroller) : gtk_scrolled_window_get_vadjustment (self->scroller);
  double d = across ? (dx != 0 ? dx : dy) : dy;

  if (gtk_event_controller_scroll_get_unit (controller) == GDK_SCROLL_UNIT_WHEEL)
    d *= pow (gtk_adjustment_get_page_size (adj), 2.0 / 3.0);
  gtk_adjustment_set_value (adj, gtk_adjustment_get_value (adj) + d);
}

/* The wheel over the card's size field: with the focus in the field, the field's own (a step of the size, REQ-0017-25); without it,
   the view's, as anywhere over the canvas area. The card is the scrolled window's sibling in the overlay, so the wheel would not reach
   it: the window scrolls the view as GTK's scrolled window does (Shift for across). Ctrl+wheel never gets here: the card's own
   controller zooms first (REQ-0036-01). */
static gboolean
on_card_size_scroll (GtkEventControllerScroll *controller, double dx, double dy, EaselWindow *self)
{
  GtkWidget *field = gtk_event_controller_get_widget (GTK_EVENT_CONTROLLER (controller));
  GtkWidget *focus = gtk_root_get_focus (GTK_ROOT (self));

  if (focus != NULL && (focus == field || gtk_widget_is_ancestor (focus, field)))
    return GDK_EVENT_PROPAGATE;
  scroll_view_as_scroller (self, controller, dx, dy);
  return GDK_EVENT_STOP;
}

/* The open box's keys that the window owns (REQ-0017-12, REQ-0017-14): Ctrl+Shift+5 toggles Strikethrough, matched by the key that
   writes 5, whatever Shift makes it report ("%" on a US or Polish layout); Ctrl+Shift+F opens the font card's family popover, the keys
   in its search. */
static gboolean
format_key (EaselWindow *self, guint keyval, guint keycode, GdkModifierType state)
{
  g_autofree GdkKeymapKey *keys = NULL;
  int n = 0;

  if ((state & (GDK_CONTROL_MASK | GDK_SHIFT_MASK | GDK_ALT_MASK)) != (GDK_CONTROL_MASK | GDK_SHIFT_MASK))
    return FALSE;
  if (keyval == GDK_KEY_F || keyval == GDK_KEY_f)
    {
      easel_font_card_open_family (self->font_card);
      return TRUE;
    }
  if (keycode != 0 && gdk_display_map_keyval (gtk_widget_get_display (GTK_WIDGET (self)), GDK_KEY_5, &keys, &n))
    for (int i = 0; i < n; i++)
      if (keys[i].keycode == keycode)
        {
          easel_canvas_text_toggle (self->canvas, EASEL_TEXT_STRIKE);
          return TRUE;
        }
  return FALSE;
}

/* Whether a key writes a letter of a script that is not Latin (Cyrillic, Greek, ...): only those are mapped to the Latin key at their place;
   a Latin letter with a diacritic (a Czech ú, ě, š), a symbol and a dead key are not (REQ-0018-01, the check's F4). */
static gboolean
keyval_of_non_latin_script (guint keyval)
{
  gunichar c = gdk_keyval_to_unicode (keyval);
  GUnicodeScript script = c != 0 ? g_unichar_get_script (c) : G_UNICODE_SCRIPT_UNKNOWN;

  return script != G_UNICODE_SCRIPT_LATIN && script != G_UNICODE_SCRIPT_COMMON && script != G_UNICODE_SCRIPT_INHERITED
         && script != G_UNICODE_SCRIPT_UNKNOWN;
}

/* With Ctrl held, a key that writes a letter of a script that is not Latin counts as the key at the same place that writes a Latin letter
   or symbol in a layout the user also has: on a Ukrainian or Russian layout the key of Ctrl+I reports "ш", and the shortcuts are still
   those of the Latin keys (REQ-0018-01; also Ctrl+B, U, [, ] and F of the text box, REQ-0017-12). The Latin keyval of the key's level,
   else the one given. */
static guint
shortcut_keyval (EaselWindow *self, guint keyval, guint keycode, GdkModifierType state)
{
  GdkKeymapKey *keys = NULL;
  guint *keyvals = NULL;
  guint found = keyval;
  int n = 0;

  if (!(state & GDK_CONTROL_MASK) || (state & GDK_ALT_MASK) || keyval < 0x80 || keycode == 0
      || !keyval_of_non_latin_script (keyval)
      || !gdk_display_map_keycode (gtk_widget_get_display (GTK_WIDGET (self)), keycode, &keys, &keyvals, &n))
    return keyval;
  for (int i = 0; i < n; i++)
    if (keys[i].level == ((state & GDK_SHIFT_MASK) ? 1 : 0) && keyvals[i] != 0 && keyvals[i] < 0x80)
      {
        found = keyvals[i];
        break;
      }
  g_free (keys);
  g_free (keyvals);
  return found;
}

/* Ctrl+? by the key's place: with Shift, a layout that is not Latin writes something else on that key, such as the comma of the Ukrainian layout, and
   the key still means Ctrl+? when a layout the user also has writes "?" there (REQ-0021-01, as REQ-0018-01). */
static gboolean
key_is_shortcuts (EaselWindow *self, guint keyval, guint keycode, GdkModifierType state)
{
  g_autofree GdkKeymapKey *keys = NULL;
  g_autofree guint *keyvals = NULL;
  int n = 0;

  if ((state & (GDK_CONTROL_MASK | GDK_SHIFT_MASK | GDK_ALT_MASK)) != (GDK_CONTROL_MASK | GDK_SHIFT_MASK) || keyval == GDK_KEY_question || keycode == 0
      || !gdk_display_map_keycode (gtk_widget_get_display (GTK_WIDGET (self)), keycode, &keys, &keyvals, &n))
    return FALSE;
  for (int i = 0; i < n; i++)
    if (keyvals[i] == GDK_KEY_question)
      return TRUE;
  return FALSE;
}

/* Esc cancels a handle drag (REQ-0004-19); at any other time it is not ours. */
static gboolean
on_key_pressed (EaselWindow *self, guint keyval, guint keycode, GdkModifierType state, GtkEventControllerKey *controller)
{
  gboolean repeat, panel_handled = FALSE;

  keyval = shortcut_keyval (self, keyval, keycode, state);
  repeat = keyval == self->key_down;
  /* The Enter or Space that closed Preferences does nothing more while it is held (REQ-0027's edge cases). */
  if (repeat && keyval == self->closed_key)
    return GDK_EVENT_STOP;
  self->closed_key = 0;
  /* A key repeat of F1 does nothing (REQ-0021-04). */
  if (keyval == GDK_KEY_F1 && repeat && (state & GDK_CONTROL_MASK) == 0)
    return GDK_EVENT_STOP;
  self->key_down = keyval;
  if (self->rbg_card != NULL)
    {
      /* Remove Background's card (REQ-0035-04): Esc is its Cancel; its field and buttons take their own keys; nothing else acts. */
      if (keyval == GDK_KEY_Escape)
        {
          if (!repeat)
            easel_remove_bg_card_answer (self->rbg_card, FALSE);
          return GDK_EVENT_STOP;
        }
      return gtk_root_get_focus (GTK_ROOT (self)) != NULL
                 && gtk_widget_is_ancestor (gtk_root_get_focus (GTK_ROOT (self)), self->rbg_card)
               ? GDK_EVENT_PROPAGATE : GDK_EVENT_STOP;
    }
  if (key_is_shortcuts (self, keyval, keycode, state))
    {
      g_action_group_activate_action (G_ACTION_GROUP (self), "shortcuts", NULL);
      return GDK_EVENT_STOP;
    }
  if (keyval == GDK_KEY_Tab || keyval == GDK_KEY_ISO_Left_Tab || keyval == GDK_KEY_KP_Tab)
    self->tab_time = g_get_monotonic_time ();   /* a focus Tab carries into the card does not stay there (REQ-0017-14) */
  if (menu_is_open (self))
    return GDK_EVENT_PROPAGATE;                 /* a menu's keys are its own (REQ-0013-17) */
  if (gtk_root_get_focus (GTK_ROOT (self)) != NULL
      && gtk_widget_is_ancestor (gtk_root_get_focus (GTK_ROOT (self)), GTK_WIDGET (self->font_card)))
    return GDK_EVENT_PROPAGATE;                 /* the card's fields and its family list take their own keys (REQ-0017-14) */
  if (panel_key (self, keyval, state, repeat, &panel_handled) && panel_handled)
    return GDK_EVENT_STOP;
  if (pointer_key (self, keyval, state, repeat))
    return GDK_EVENT_STOP;
  /* Ctrl+P by the place of its key on a layout that is not Latin, ahead of an open box; its repeat does nothing (REQ-0026-01). */
  if ((keyval == GDK_KEY_p || keyval == GDK_KEY_P) && (state & (GDK_CONTROL_MASK | GDK_SHIFT_MASK | GDK_ALT_MASK)) == GDK_CONTROL_MASK)
    {
      if (!repeat)
        g_action_group_activate_action (G_ACTION_GROUP (self), "print", NULL);
      return GDK_EVENT_STOP;
    }
  if (self->tool == EASEL_TOOL_TEXT && !dialog_is_open (self) && easel_canvas_text_buffer (self->canvas) != NULL)
    {
      GdkEvent *event = gtk_event_controller_get_current_event (GTK_EVENT_CONTROLLER (controller));
      gboolean level3 = event != NULL && gdk_event_get_event_type (event) == GDK_KEY_PRESS && gdk_key_event_get_level (event) >= 2;
      gboolean shortcut = !level3 && (state & (GDK_CONTROL_MASK | GDK_ALT_MASK)) != 0;

      if (keyval == GDK_KEY_Escape && !easel_canvas_is_stroking (self->canvas))
        {
          easel_canvas_put_down (self->canvas);   /* Esc puts the box down, or the dialog says why not (REQ-0017-17, -18) */
          return GDK_EVENT_STOP;
        }
      if (format_key (self, keyval, keycode, state))
        return GDK_EVENT_STOP;
      /* Keys go to the open box wherever the focus is in the window (REQ-0017-06): an input method first, then the box's own keys. A
         key with Ctrl or Alt is a shortcut and goes to the box's keys only (JOD's M1, 2026-10-26: the input method typed Ctrl+B as "b"),
         unless it is on the ISO Level 3 shift (AltGr, which some setups report as Ctrl+Alt): the Polish ą ć ę ł ń ó ś ź ż are typed so */
      if ((!shortcut && easel_canvas_text_filter (self->canvas, event)) || easel_canvas_text_key (self->canvas, keyval, state))
        return GDK_EVENT_STOP;
    }
  if ((keyval == GDK_KEY_i || keyval == GDK_KEY_I) && (state & (GDK_CONTROL_MASK | GDK_SHIFT_MASK | GDK_ALT_MASK)) == GDK_CONTROL_MASK)
    {
      /* Ctrl+I is Invert Colors unless an open text box took it for Italic above (REQ-0017-12, REQ-0018-01) */
      image_action (self, EASEL_OP_INVERT);
      return GDK_EVENT_STOP;
    }
  if ((keyval == GDK_KEY_Delete || keyval == GDK_KEY_KP_Delete) && !dialog_is_open (self)
      && easel_canvas_clear_selection (self->canvas))
    return GDK_EVENT_STOP;                      /* Delete clears a selection wherever the focus is (REQ-0013-14) */
  if ((keyval == GDK_KEY_Left || keyval == GDK_KEY_Right || keyval == GDK_KEY_Up || keyval == GDK_KEY_Down)
      && (state & GDK_ALT_MASK) == 0 && (state & (GDK_CONTROL_MASK | GDK_SHIFT_MASK)) != GDK_CONTROL_MASK && !dialog_is_open (self))
    {
      /* The arrows move the selection wherever the focus is, Shift smears it by 1, Ctrl+Shift moves it by 10 (REQ-0013-10);
         without one, or while a button is held, they are not taken. */
      gboolean shift = (state & GDK_SHIFT_MASK) != 0, ctrl = (state & GDK_CONTROL_MASK) != 0;
      int step = ctrl ? 10 : 1;
      int dx = keyval == GDK_KEY_Left ? -step : keyval == GDK_KEY_Right ? step : 0;
      int dy = keyval == GDK_KEY_Up ? -step : keyval == GDK_KEY_Down ? step : 0;

      if (shift && !ctrl ? easel_canvas_smear_selection (self->canvas, dx, dy) : easel_canvas_move_selection (self->canvas, dx, dy))
        return GDK_EVENT_STOP;
    }
  if (keyval == GDK_KEY_Escape && !dialog_is_open (self) && easel_canvas_finish_selection (self->canvas))
    return GDK_EVENT_STOP;                      /* Esc puts a selection down, or removes a marked one (REQ-0013-11) */
  if (keyval == GDK_KEY_Escape && !dialog_is_open (self) && (easel_tool_selects (self->tool) || self->tool == EASEL_TOOL_TEXT)
      && easel_canvas_is_stroking (self->canvas))
    {
      easel_canvas_cancel_stroke (self->canvas);   /* Esc during a drag of Select or of a text box cancels it (REQ-0013-18, -0017-04) */
      return GDK_EVENT_STOP;
    }
  if (keyval == GDK_KEY_Escape && easel_canvas_is_pending (self->canvas) && !dialog_is_open (self))
    {
      /* Esc drops a pending curve or polygon (REQ-0011-11); with a dialog, the quit prompt or the color chooser open it acts on
         that and not on the shape (REQ-0011-12). */
      easel_canvas_drop (self->canvas);
      return GDK_EVENT_STOP;
    }
  if (keyval == GDK_KEY_Escape && easel_tool_draws_shape (self->tool) && easel_canvas_is_stroking (self->canvas))
    {
      /* Esc cancels a shape drag (REQ-0010-08). */
      easel_canvas_cancel_stroke (self->canvas);
      return GDK_EVENT_STOP;
    }
  if ((keyval == GDK_KEY_Return || keyval == GDK_KEY_KP_Enter) && !dialog_is_open (self) && easel_canvas_enter (self->canvas))
    return GDK_EVENT_STOP;
  if (keyval == GDK_KEY_Escape && easel_canvas_is_resizing (self->canvas))
    {
      easel_canvas_cancel_resize (self->canvas);
      return GDK_EVENT_STOP;
    }
  /* Last of all, with nothing else to take it and no button held, Esc closes the thumbnail; its repeat does not (REQ-0021-08). */
  if (keyval == GDK_KEY_Escape && !repeat && gtk_widget_get_visible (GTK_WIDGET (self->thumbnail_card)) && view_actions_allowed (self)
      && !easel_canvas_is_panning (self->canvas))
    {
      g_action_group_change_action_state (G_ACTION_GROUP (self), "thumbnail", g_variant_new_boolean (FALSE));
      return GDK_EVENT_STOP;
    }
  return GDK_EVENT_PROPAGATE;
}

static void
on_key_released (EaselWindow *self, guint keyval, guint keycode, GdkModifierType state, GtkEventControllerKey *controller)
{
  if (keyval == self->key_down)
    self->key_down = 0;
}

/* Shift going down or up changes a shape being dragged at once (REQ-0010-14); the state is the new one. */
static gboolean
on_modifiers_changed (EaselWindow *self, GdkModifierType state, GtkEventControllerKey *controller)
{
  easel_canvas_set_shift (self->canvas, (state & GDK_SHIFT_MASK) != 0);
  easel_canvas_set_ctrl (self->canvas, (state & GDK_CONTROL_MASK) != 0);
  return GDK_EVENT_PROPAGATE;
}

/* Losing focus ends a stroke and keeps it (REQ-0001-25), and stops waiting for a release that may never come
   (REQ-0002-09). */
static void
on_active_changed (EaselWindow *self)
{
  self->key_down = 0;                           /* the release of a key held as the window went to the back never arrives */
  if (!gtk_window_is_active (GTK_WINDOW (self)))
    easel_canvas_reset_pointer (self->canvas);
}

/* What the canvas paints with follows the options panel (REQ-0006-03). */
/* The status bar's part for the selected size preset, or none for a tool without sizes (REQ-0030-08). */
static void
update_preset_part (EaselWindow *self)
{
  g_auto (GStrv) names = easel_tool_options_size_names (self->tool);
  g_autoptr (GVariant) width = g_action_group_get_action_state (G_ACTION_GROUP (self), "line-width");

  if (names == NULL)
    {
      easel_status_bar_set_preset (self->status_bar, NULL, NULL);
      return;
    }
  easel_status_bar_set_preset (self->status_bar,
                               names[easel_tool_options_size_index (self->tool_options, self->tool, g_variant_get_int32 (width))],
                               (const char *const *) names);
}

static void
sync_canvas_options (EaselWindow *self)
{
  if (self->tool == EASEL_TOOL_BRUSH || self->tool == EASEL_TOOL_AIRBRUSH || self->tool == EASEL_TOOL_ERASER)
    easel_canvas_set_footprint (self->canvas, easel_tool_options_get_footprint (self->tool_options, self->tool));
  easel_canvas_set_eraser_transparent (self->canvas, easel_tool_options_get_transparent (self->tool_options));
  easel_canvas_set_selection_transparent (self->canvas, easel_tool_options_get_select_transparent (self->tool_options));
  easel_canvas_set_text_opaque (self->canvas, !easel_tool_options_get_text_transparent (self->tool_options));
  easel_canvas_set_gradient_type (self->canvas, easel_tool_options_get_gradient_type (self->tool_options));
  easel_status_bar_set_hint (self->status_bar, tool_hint (self->tool, easel_tool_options_get_transparent (self->tool_options)));
  update_preset_part (self);
}

/* Pick Color (REQ-0007-08..10): the picked color is the foreground (primary button) or the background (secondary) at
   once, so that the color box shows it while the button is held; a cancel puts the old colors back, an end goes back to
   the tool that was active when Pick Color was chosen. */
static void
on_color_picked (EaselWindow *self, guint button, guint color)
{
  if (!self->pick_running)
    {
      self->pick_running = TRUE;
      self->pick_old_foreground = self->foreground;
      self->pick_old_background = self->background;
    }
  if (button == GDK_BUTTON_PRIMARY)
    set_colors (self, color, self->background);
  else
    set_colors (self, self->foreground, color);
}

static void
on_pick_ended (EaselWindow *self, gboolean cancelled)
{
  announce_colors (self);
  self->pick_running = FALSE;
  if (cancelled)
    set_colors (self, self->pick_old_foreground, self->pick_old_background);
  else
    g_action_group_activate_action (G_ACTION_GROUP (self), "tool",
                                    g_variant_new_string (easel_tool_name (self->pick_return_tool)));
}

/* REQ-0007-07: shown from an idle callback, so that the press that failed has returned first. */
static gboolean
show_fill_error (gpointer data)
{
  EaselWindow *self = data;

  show_save_error (self, _ ("Could Not Fill"), _ ("There is not enough memory to fill this area."));
  g_object_unref (self);
  return G_SOURCE_REMOVE;
}

static void
on_fill_failed (EaselWindow *self)
{
  g_idle_add (show_fill_error, g_object_ref (self));
}

/* REQ-0010-20: the same, for a shape. */
static gboolean
show_shape_error (gpointer data)
{
  EaselWindow *self = data;

  show_save_error (self, _ ("Could Not Draw the Shape"), _ ("There is not enough memory to draw this shape."));
  g_object_unref (self);
  return G_SOURCE_REMOVE;
}

static void
on_shape_failed (EaselWindow *self)
{
  self->put_down_failed = TRUE;
  g_idle_add (show_shape_error, g_object_ref (self));
}

/* REQ-0012-09: the same, for a gradient (its apply, or its preview at the release of the drag). */
static gboolean
show_gradient_error (gpointer data)
{
  EaselWindow *self = data;

  show_save_error (self, _ ("Could Not Apply the Gradient"), _ ("There is not enough memory to apply this gradient."));
  g_object_unref (self);
  return G_SOURCE_REMOVE;
}

static void
on_gradient_failed (EaselWindow *self)
{
  self->put_down_failed = TRUE;
  g_idle_add (show_gradient_error, g_object_ref (self));
}

/* REQ-0013-19: the memory dialogs of the selection, from an idle callback so that the action that put the selection down (Save,
   New, Open, a tool change) goes on first. */
static gboolean
show_selection_error (gpointer data)
{
  EaselWindow *self = data;
  int kind = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (self), "selection-error-kind"));

  if (kind == 1)
    show_save_error (self, _ ("Could Not Clear the Selection"), _ ("There is not enough memory to clear this selection."));
  else if (kind == 2)
    show_save_error (self, _ ("Could Not Make the Selection"), _ ("There is not enough memory to make this selection."));
  else if (kind == 3)
    show_save_error (self, _ ("Could Not Stretch the Selection"), _ ("There is not enough memory to stretch this selection."));
  else if (kind == 4)
    show_save_error (self, _ ("Could Not Flip the Selection"), _ ("There is not enough memory for this change."));
  else if (kind == 5)
    show_save_error (self, _ ("Could Not Rotate the Selection"), _ ("There is not enough memory for this change."));
  else if (kind == 6)
    show_save_error (self, _ ("Could Not Invert the Selection"), _ ("There is not enough memory for this change."));
  else if (kind == 7)
    show_save_error (self, _ ("Could Not Stretch or Skew the Selection"), _ ("There is not enough memory for this change."));
  else if (kind == 8)
    show_save_error (self, _ ("Could Not Convert the Selection"), _ ("There is not enough memory for this change."));
  else if (kind == 9)
    show_save_error (self, _ ("Could Not Remove the Background"), _ ("There is not enough memory for this change."));
  else
    show_save_error (self, _ ("Could Not Move the Selection"), _ ("There is not enough memory to move this selection."));
  g_object_unref (self);
  return G_SOURCE_REMOVE;
}

/* A box's first text could not be shown: the box is gone, nothing put down (REQ-0017-07). */
static void
on_text_failed (EaselWindow *self, int kind)
{
  /* kind 0: the box's text could not be shown (REQ-0017-07); 1: the put-down's step could not be had (REQ-0017-18) */
  show_save_error (self, _ ("Could Not Add the Text"),
                   kind == 0 ? _ ("There is not enough memory to show this text.") : _ ("There is not enough memory to add this text."));
}

static void
on_selection_failed (EaselWindow *self, int kind)
{
  g_object_set_data (G_OBJECT (self), "selection-error-kind", GINT_TO_POINTER (kind));
  g_idle_add (show_selection_error, g_object_ref (self));
}

/* The status bar's selection size follows the selection and the rectangle being dragged out (REQ-0013-06); Clear Selection
   is there only while a selection exists (REQ-0013-14). */
/* Whether the clipboard holds something Paste takes (REQ-0016-02): a picture, or copied files (as Files' Copy puts them). */
static gboolean
clipboard_has_picture (EaselClipboard *clipboard)
{
  GdkContentFormats *formats = easel_clipboard_get_formats (clipboard);
  const char *const *types;
  gsize n = 0;

  if (gdk_content_formats_contain_gtype (formats, GDK_TYPE_TEXTURE) || gdk_content_formats_contain_gtype (formats, GDK_TYPE_FILE_LIST))
    return TRUE;
  types = gdk_content_formats_get_mime_types (formats, &n);
  for (gsize i = 0; i < n; i++)
    if (g_str_has_prefix (types[i], "image/"))
      return TRUE;
  return FALSE;
}

/* Whether the clipboard holds text a box's Paste takes (REQ-0017-20): plain or rich text, or copied files (their paths). */
static gboolean
clipboard_has_text (EaselClipboard *clipboard)
{
  GdkContentFormats *formats = easel_clipboard_get_formats (clipboard);

  return gdk_content_formats_contain_gtype (formats, G_TYPE_STRING) || gdk_content_formats_contain_gtype (formats, GDK_TYPE_FILE_LIST)
         || gdk_content_formats_contain_mime_type (formats, "text/html")
         || gdk_content_formats_contain_mime_type (formats, "text/plain;charset=utf-8")
         || gdk_content_formats_contain_mime_type (formats, "text/plain") || gdk_content_formats_contain_mime_type (formats, "UTF8_STRING");
}

/* The five clipboard commands' sensitivity (REQ-0016-02): Cut, Copy and Copy To… with a selection, Paste while the clipboard holds a
   picture or files, Paste From… while the picture exists; none while a button is held for a stroke or a drag. Kept up to date at every
   change of the selection, the clipboard, the picture and a held button. */
static void
set_action_enabled (EaselWindow *self, const char *name, gboolean enabled)
{
  GAction *action = g_action_map_lookup_action (G_ACTION_MAP (self), name);

  if (action != NULL)
    g_simple_action_set_enabled (G_SIMPLE_ACTION (action), enabled);
}

static void
update_clipboard_actions (EaselWindow *self)
{
  int x, y, w, h;
  gboolean floating;
  gboolean idle = self->picture != NULL && !easel_canvas_is_stroking (self->canvas) && !easel_canvas_is_resizing (self->canvas);
  gboolean box = self->tool == EASEL_TOOL_TEXT && easel_canvas_text_box (self->canvas, &x, &y, &w, &h);
  gboolean selection = idle && easel_canvas_get_selection (self->canvas, &x, &y, &w, &h, &floating);
  gboolean picture = idle && self->clipboard != NULL && clipboard_has_picture (self->clipboard);
  static const char *selection_actions[] = { "cut", "copy", "copy-to" };

  if (box)
    {
      /* A box open: Cut and Copy with text selected, Paste and Paste as Plain Text with text on the clipboard, no Copy To…
         (REQ-0017-19, REQ-0017-20) */
      size_t from, to;
      gboolean text_selected = idle && easel_text_buffer_selection (easel_canvas_text_buffer (self->canvas), &from, &to);
      gboolean text = idle && self->clipboard != NULL && clipboard_has_text (self->clipboard);

      set_action_enabled (self, "cut", text_selected);
      set_action_enabled (self, "copy", text_selected);
      set_action_enabled (self, "copy-to", FALSE);
      set_action_enabled (self, "paste", text);
      set_action_enabled (self, "paste-plain", text);
      set_action_enabled (self, "paste-from", idle);
      return;
    }
  set_action_enabled (self, "paste-plain", FALSE);
  for (guint i = 0; i < G_N_ELEMENTS (selection_actions); i++)
    g_simple_action_set_enabled (G_SIMPLE_ACTION (g_action_map_lookup_action (G_ACTION_MAP (self), selection_actions[i])), selection);
  g_simple_action_set_enabled (G_SIMPLE_ACTION (g_action_map_lookup_action (G_ACTION_MAP (self), "paste")), picture);
  g_simple_action_set_enabled (G_SIMPLE_ACTION (g_action_map_lookup_action (G_ACTION_MAP (self), "paste-from")), idle);
}

static void
on_selection_changed (EaselWindow *self)
{
  int w = 0, h = 0, x, y;
  gboolean exists = easel_canvas_get_selection_size (self->canvas, &w, &h), floating;
  GAction *clear = g_action_map_lookup_action (G_ACTION_MAP (self), "clear-selection");

  easel_status_bar_set_selection (self->status_bar, exists, w, h);
  announce_selection (self);
  sync_history_ui (self);                          /* a text box opened, edited or closed (REQ-0017-08) */
  update_card (self);
  if (clear != NULL)
    g_simple_action_set_enabled (G_SIMPLE_ACTION (clear),
                                 easel_canvas_get_selection (self->canvas, &x, &y, &w, &h, &floating));
  update_clipboard_actions (self);
}

/* Select All (REQ-0013-15): ignored when a tool change is (before the picture exists, with a dialog open, during a stroke or
   a resize drag); otherwise Select becomes the tool (another tool's pending shape put down as a tool change does) and the
   whole picture is marked, a floating selection put down first. */
static void
select_all_activated (GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
  EaselWindow *self = user_data;

  if (card_field_action (self, "selection.select-all"))
    return;
  if (self->picture == NULL || dialog_is_open (self) || easel_canvas_is_stroking (self->canvas)
      || easel_canvas_is_resizing (self->canvas))
    return;
  if (self->tool == EASEL_TOOL_TEXT && easel_canvas_text_select_all (self->canvas))
    return;                                     /* an open box: all its text selected, the box kept (REQ-0017-17) */
  g_action_group_change_action_state (G_ACTION_GROUP (self), "tool", g_variant_new_string ("select"));
  easel_canvas_select_all (self->canvas);
}

static void
clear_selection_activated (GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
  EaselWindow *self = user_data;

  if (!dialog_is_open (self))
    easel_canvas_clear_selection (self->canvas);
}

/* Choosing a preset is not possible while a stroke is in progress (REQ-0006-03). */
static void
on_stroking_changed (EaselWindow *self, gboolean stroking)
{
  sync_key_hold (self);
  easel_tool_options_set_busy (self->tool_options, stroking);
  update_clipboard_actions (self);
}

/* The memory for the copy of the selection could not be had: the dialog, and nothing else changed (REQ-0016-11). */
static void
show_copy_error (EaselWindow *self)
{
  show_save_error (self, _ ("Could Not Copy the Selection"), _ ("There is not enough memory to copy this selection."));
}

/* Copy (REQ-0016-03): the selection's copy on the clipboard, replacing what it held. FALSE if there was nothing to copy or no memory for
   the copy (its dialog: 0016 T015). */
static gboolean
copy_selection (EaselWindow *self)
{
  EaselImage *copy;
  g_autoptr (GdkContentProvider) provider = NULL;

  if (self->picture == NULL || dialog_is_open (self) || easel_canvas_is_stroking (self->canvas))
    return FALSE;
  copy = easel_canvas_copy_selection (self->canvas);
  if (copy == NULL)
    {
      int x, y, w, h;
      gboolean floating;

      if (easel_canvas_get_selection (self->canvas, &x, &y, &w, &h, &floating))
        show_copy_error (self);                  /* a selection but no memory for its copy (REQ-0016-11) */
      return FALSE;
    }
  provider = easel_clipboard_picture_new (copy);
  easel_clipboard_set_content (self->clipboard, provider);
  return TRUE;
}

/* The picture types a paste reads, in the order it prefers them: PNG first (exact), then the others Open reads (REQ-0016-05). */
static const char *const paste_mime_types[] = { "image/png", "image/webp", "image/tiff", "image/bmp", "image/jpeg", "image/gif", NULL };

/* A read that waits for the clipboard: input waits and the busy cursor shows (REQ-0016-15). */
typedef struct
{
  EaselWindow *self;
  GCancellable *cancellable;
  gboolean timed_out;
  guint timer;
} ClipboardWait;

static gboolean
on_clipboard_timeout (gpointer data)
{
  ClipboardWait *wait = data;

  wait->timed_out = TRUE;
  wait->timer = 0;
  g_cancellable_cancel (wait->cancellable);
  return G_SOURCE_REMOVE;
}

/* The main loop runs until the call answers or the limit passes; the result (the caller's to unref), or NULL on a time-out (the call is
   cancelled, and its late callback frees the answer). */
static GAsyncResult *
wait_for_clipboard_result (ClipboardWait *wait, Answer *answer)
{
  while (answer->result == NULL && !wait->timed_out)
    g_main_context_iteration (NULL, TRUE);
  return answer_take (answer);
}

/* The window waits on the spot for the clipboard (REQ-0016-15, REQ-0017-27): the busy cursor, input held, and the limit's timer. */
static void
clipboard_wait_begin (EaselWindow *self, ClipboardWait *wait)
{
  self->reading_clipboard = TRUE;
  gtk_widget_set_cursor_from_name (GTK_WIDGET (self), "wait");
  easel_canvas_set_busy (self->canvas, TRUE);
  easel_canvas_set_input_blocked (self->canvas, TRUE);
  wait->timer = g_timeout_add (self->clipboard_timeout_ms, on_clipboard_timeout, wait);
}

/* The wait is over: back to the tool's cursor and to taking input. TRUE when a quit request came meanwhile: the quit goes on (the window
   may be gone after), and the caller drops what it read and touches the window no more. */
static gboolean
clipboard_wait_end (EaselWindow *self, ClipboardWait *wait)
{
  g_clear_handle_id (&wait->timer, g_source_remove);
  g_object_unref (wait->cancellable);
  self->reading_clipboard = FALSE;
  gtk_widget_set_cursor (GTK_WIDGET (self), NULL);
  easel_canvas_set_busy (self->canvas, FALSE);
  easel_canvas_set_input_blocked (self->canvas, FALSE);
  if (!self->close_after_reading)
    return FALSE;
  self->close_after_reading = FALSE;
  gtk_window_close (GTK_WINDOW (self));
  return TRUE;
}

/* A refusal of the paste: the error dialog and nothing changed (REQ-0016-10). name is the file's, for a file. */
static void
paste_error (EaselWindow *self, const char *name, EaselOpenResult why)
{
  g_autofree char *heading = name != NULL ? g_strdup_printf (_ ("Could Not Paste “%s”"), name) : g_strdup (_ ("Could Not Paste"));
  const char *body;

  if (self->close_after_reading)
    return;       /* a quit request waits for this read: nothing was changed, and a dialog would take the quit (REQ-0016-15) */

  switch (why)
    {
    case EASEL_OPEN_TOO_BIG:
      body = _ ("The picture is bigger than 10000 pixels in width or height.");
      break;
    case EASEL_OPEN_NO_MEMORY:
      body = _ ("There is not enough memory to paste this picture.");
      break;
    case EASEL_OPEN_NOT_PICTURE:
      body = _ ("This is not a picture the app can open.");
      break;
    case EASEL_OPEN_UNREADABLE:
    default:
      body = name != NULL ? _ ("The file cannot be read.") : _ ("The clipboard's picture cannot be read.");
      break;
    }
  show_save_error (self, heading, body);
}

/* The clipboard's picture (REQ-0016-05, -15): Easel's own copy taken as it is; another app's picture read as PNG first and then the
   types Open reads; copied files only: the first file, opened as Open does. NULL if there is none (a refusal shows its dialog), or if a
   quit request came during the wait (the window may be gone when this returns). */
static EaselImage *
read_clipboard_picture (EaselWindow *self)
{
  GdkContentProvider *content = easel_clipboard_get_content (self->clipboard);
  GdkContentFormats *formats = easel_clipboard_get_formats (self->clipboard);
  ClipboardWait wait = { self, g_cancellable_new (), FALSE, 0 };
  Answer *answer;
  GAsyncResult *result = NULL;
  g_autoptr (GInputStream) stream = NULL;
  g_autoptr (GOutputStream) sink = NULL;
  g_autoptr (GBytes) bytes = NULL;
  const char *const *types;
  gboolean has_picture = FALSE;
  EaselImage *img = NULL;
  EaselOpenResult opened = EASEL_OPEN_UNREADABLE;
  gsize n = 0;
  int w = 0, h = 0;

  if (easel_clipboard_is_local (self->clipboard) && EASEL_IS_CLIPBOARD_PICTURE (content))
    {
      const EaselImage *own = easel_clipboard_picture_get_image (EASEL_CLIPBOARD_PICTURE (content));

      img = easel_image_new (own->width, own->height, 0);
      if (img == NULL)
        paste_error (self, NULL, EASEL_OPEN_NO_MEMORY);
      else
        memcpy (img->px, own->px, (size_t) own->width * (size_t) own->height * sizeof *img->px);
      g_object_unref (wait.cancellable);
      return img;
    }
  /* From here the window waits on the spot: the busy cursor, input blocked, a quit request handled after (REQ-0016-15). */
  clipboard_wait_begin (self, &wait);
  types = gdk_content_formats_get_mime_types (formats, &n);
  for (gsize i = 0; i < n && !has_picture; i++)
    has_picture = g_str_has_prefix (types[i], "image/");
  has_picture = has_picture || gdk_content_formats_contain_gtype (formats, GDK_TYPE_TEXTURE);
  if (has_picture)
    {
      answer = g_new0 (Answer, 1);
      easel_clipboard_read_async (self->clipboard, (const char **) paste_mime_types, wait.cancellable, on_answer, answer);
      result = wait_for_clipboard_result (&wait, answer);
      stream = result != NULL ? easel_clipboard_read_finish (self->clipboard, result, NULL, NULL) : NULL;
      g_clear_object (&result);
      if (stream != NULL)
        {
          /* Read without blocking: when the content is of this process its writer is this main loop (perf-0016, 2026-10-23). */
          sink = g_memory_output_stream_new_resizable ();
          answer = g_new0 (Answer, 1);
          g_output_stream_splice_async (sink, stream, G_OUTPUT_STREAM_SPLICE_CLOSE_SOURCE | G_OUTPUT_STREAM_SPLICE_CLOSE_TARGET,
                                        G_PRIORITY_DEFAULT, wait.cancellable, on_answer, answer);
          result = wait_for_clipboard_result (&wait, answer);
          if (result != NULL && g_output_stream_splice_finish (sink, result, NULL) >= 0)
            {
              bytes = g_memory_output_stream_steal_as_bytes (G_MEMORY_OUTPUT_STREAM (sink));
              opened = easel_picture_open_bytes (bytes, &img, &w, &h);
            }
          g_clear_object (&result);
        }
      if (img == NULL)
        paste_error (self, NULL, opened);
    }
  else
    {
      GSList *files = NULL;

      answer = g_new0 (Answer, 1);
      easel_clipboard_read_files_async (self->clipboard, wait.cancellable, on_answer, answer);
      result = wait_for_clipboard_result (&wait, answer);
      if (result != NULL)
        files = easel_clipboard_read_files_finish (self->clipboard, result, NULL);
      g_clear_object (&result);
      if (files != NULL)
        {
          g_autofree char *path = g_file_get_path (files->data);
          g_autofree char *name = g_file_get_basename (files->data);

          opened = path != NULL ? easel_picture_open (path, &img, &w, &h) : EASEL_OPEN_UNREADABLE;
          if (img == NULL)
            paste_error (self, name, opened);
        }
      else
        paste_error (self, NULL, EASEL_OPEN_UNREADABLE);
      g_slist_free_full (files, g_object_unref);
    }
  /* Back to the tool's cursor and to taking input, a quit request that came meanwhile handled now: a picture that arrives after it is
     dropped, and nothing is pasted (REQ-0016-15). */
  if (clipboard_wait_end (self, &wait))
    g_clear_pointer (&img, easel_image_free);
  return img;
}

/* Select made the tool for a paste, as a tool change: FALSE when an open text box could not be put down, Text staying the tool; the
   read picture is then dropped (REQ-0017-18). */
static gboolean
select_for_paste (EaselWindow *self)
{
  g_action_group_change_action_state (G_ACTION_GROUP (self), "tool", g_variant_new_string ("select"));
  return self->tool == EASEL_TOOL_SELECT;
}

/* The view scrolled so the picture's top-left corner is in view, the zoom kept (REQ-0016-07). */
static void
scroll_to_origin (EaselWindow *self)
{
  GtkAdjustment *h = gtk_scrolled_window_get_hadjustment (self->scroller), *v = gtk_scrolled_window_get_vadjustment (self->scroller);

  gtk_adjustment_set_value (h, gtk_adjustment_get_lower (h));
  gtk_adjustment_set_value (v, gtk_adjustment_get_lower (v));
}

/* The answer of the oversize-paste prompt (REQ-0016-07): Resize grows the picture to the larger of each side and pastes at (0, 0);
   Fit makes the picture as large as fits, by the nearest pixel, and pastes it at (0, 0); Cancel, Esc or closing changes nothing. */
static void
on_paste_prompt_answered (AdwAlertDialog *dialog, const char *answer, EaselWindow *self)
{
  EaselImage *pic;
  int w = self->picture->width, h = self->picture->height;

  if (self->paste_prompt != ADW_DIALOG (dialog))
    return;                                       /* answered already (closing it after an answer answers again) */
  self->paste_prompt = NULL;
  pic = g_steal_pointer (&self->paste_pending);
  if (g_strcmp0 (answer, "resize") == 0)
    {
      /* The memory for the paste and the grown picture first: a refusal changes nothing, the tool included (REQ-0016-11). */
      if (!easel_canvas_can_paste (self->canvas, pic->width, pic->height, MAX (w, pic->width), MAX (h, pic->height)))
        {
          easel_image_free (pic);
          paste_error (self, NULL, EASEL_OPEN_NO_MEMORY);
        }
      else
        {
          if (!select_for_paste (self))
            easel_image_free (pic);
          else if (easel_canvas_paste_grown (self->canvas, pic, MAX (w, pic->width), MAX (h, pic->height), self->background))
            scroll_to_origin (self);
          else
            paste_error (self, NULL, EASEL_OPEN_NO_MEMORY);
        }
    }
  else if (g_strcmp0 (answer, "fit") == 0)
    {
      uint32_t *px = NULL;
      EaselImage *fitted = NULL;
      int nw, nh;

      easel_fit_size (pic->width, pic->height, w, h, &nw, &nh);
      /* The fitted picture and its float are each had within the selection's limit before anything changes (REQ-0016-11). */
      if (easel_canvas_can_paste (self->canvas, nw, nh, 0, 0)
          && easel_selection_scale (pic->px, NULL, pic->width, pic->height, nw, nh, &px, NULL,
                                    easel_canvas_get_selection_limit (self->canvas)) == EASEL_SELECTION_DONE)
        fitted = easel_image_new (nw, nh, 0);
      if (fitted != NULL)
        {
          memcpy (fitted->px, px, (size_t) nw * (size_t) nh * sizeof *px);
          if (!select_for_paste (self))
            easel_image_free (fitted);
          else if (easel_canvas_paste (self->canvas, fitted, 0, 0))
            scroll_to_origin (self);
          else
            paste_error (self, NULL, EASEL_OPEN_NO_MEMORY);
        }
      else
        paste_error (self, NULL, EASEL_OPEN_NO_MEMORY);
      free (px);
      easel_image_free (pic);
    }
  else
    easel_image_free (pic);
  sync_history_ui (self);
}

/* The prompt for a picture wider or taller than the picture (REQ-0016-13), before anything is put down or applied. */
static void
ask_paste_size (EaselWindow *self, EaselImage *pic)
{
  g_autofree char *body = g_strdup_printf (_n ("The picture is %d × %d pixel; the canvas is %d × %d.", "The picture is %d × %d pixels; the canvas is %d × %d.", (unsigned long) pic->height), pic->width, pic->height,
                                           self->picture->width, self->picture->height);
  AdwDialog *dialog = adw_alert_dialog_new (_ ("Pasted Picture Is Larger Than the Canvas"), body);

  /* Added as Cancel, Fit, Resize: side by side they read so, stacked (the long labels) the suggested one is on top. */
  adw_alert_dialog_add_responses (ADW_ALERT_DIALOG (dialog), "cancel", _ ("_Cancel"), "fit", _ ("_Fit Picture to Canvas"), "resize",
                                  _ ("_Resize Canvas to Picture"), NULL);
  adw_alert_dialog_set_response_appearance (ADW_ALERT_DIALOG (dialog), "resize", ADW_RESPONSE_SUGGESTED);
  adw_alert_dialog_set_default_response (ADW_ALERT_DIALOG (dialog), "resize");
  adw_alert_dialog_set_close_response (ADW_ALERT_DIALOG (dialog), "cancel");
  self->paste_pending = pic;
  self->paste_prompt = dialog;
  g_signal_connect_object (dialog, "response", G_CALLBACK (on_paste_prompt_answered), self, G_CONNECT_DEFAULT);
  adw_dialog_present (dialog, GTK_WIDGET (self));
}

/* A read picture pasted (REQ-0016-05): a bigger one asks first (REQ-0016-07); then what is pending applied as a tool change does,
   Select made the tool, and the picture floating at the top-left pixel in view. Takes pic. */
static void
paste_picture (EaselWindow *self, EaselImage *pic)
{
  int x, y;

  if (pic->width > self->picture->width || pic->height > self->picture->height)
    {
      ask_paste_size (self, pic);
      return;
    }
  if (!easel_canvas_can_paste (self->canvas, pic->width, pic->height, 0, 0))
    {
      easel_image_free (pic);
      paste_error (self, NULL, EASEL_OPEN_NO_MEMORY);          /* nothing changed, the tool included (REQ-0016-11) */
      return;
    }
  if (!select_for_paste (self))
    {
      easel_image_free (pic);
      return;
    }
  easel_canvas_top_left_in_view (self->canvas, &x, &y);
  if (!easel_canvas_paste (self->canvas, pic, x, y))
    paste_error (self, NULL, EASEL_OPEN_NO_MEMORY);
}

/* Paste (REQ-0016-05): the clipboard's picture read first, then pasted. */
/* The clipboard's text read as the picture is (REQ-0017-27: the busy cursor, input held, 5 seconds at most, a quit request after): its
   HTML for a rich paste when offered, else its plain text, else the copied files' paths, one a line. NULL, with "Could Not Paste" shown,
   if nothing arrived; NULL and the window closed if a quit came meanwhile. */
static GBytes *
read_clipboard_text (EaselWindow *self, gboolean rich, gboolean *is_html)
{
  static const char *html_types[] = { "text/html", NULL };
  static const char *text_types[] = { "text/plain;charset=utf-8", "text/plain", "UTF8_STRING", NULL };
  GdkContentFormats *formats = easel_clipboard_get_formats (self->clipboard);
  ClipboardWait wait = { self, g_cancellable_new (), FALSE, 0 };
  gboolean has_html = gdk_content_formats_contain_mime_type (formats, "text/html");
  gboolean has_text = gdk_content_formats_contain_gtype (formats, G_TYPE_STRING)
                      || gdk_content_formats_contain_mime_type (formats, "text/plain;charset=utf-8")
                      || gdk_content_formats_contain_mime_type (formats, "text/plain")
                      || gdk_content_formats_contain_mime_type (formats, "UTF8_STRING");
  Answer *answer;
  GAsyncResult *result = NULL;
  GBytes *bytes = NULL;

  *is_html = has_html && (rich || !has_text);
  clipboard_wait_begin (self, &wait);
  if (*is_html || has_text)
    {
      g_autoptr (GInputStream) stream = NULL;

      answer = g_new0 (Answer, 1);
      easel_clipboard_read_async (self->clipboard, *is_html ? html_types : text_types, wait.cancellable, on_answer, answer);
      result = wait_for_clipboard_result (&wait, answer);
      stream = result != NULL ? easel_clipboard_read_finish (self->clipboard, result, NULL, NULL) : NULL;
      g_clear_object (&result);
      if (stream != NULL)
        {
          /* Read whole, without blocking: an HTML of more than a pipe's 64 KiB too (engineering.md) */
          g_autoptr (GOutputStream) sink = g_memory_output_stream_new_resizable ();

          answer = g_new0 (Answer, 1);
          g_output_stream_splice_async (sink, stream, G_OUTPUT_STREAM_SPLICE_CLOSE_SOURCE | G_OUTPUT_STREAM_SPLICE_CLOSE_TARGET,
                                        G_PRIORITY_DEFAULT, wait.cancellable, on_answer, answer);
          result = wait_for_clipboard_result (&wait, answer);
          if (result != NULL && g_output_stream_splice_finish (sink, result, NULL) >= 0)
            bytes = g_memory_output_stream_steal_as_bytes (G_MEMORY_OUTPUT_STREAM (sink));
          g_clear_object (&result);
        }
    }
  else
    {
      GSList *files = NULL;
      GString *paths = g_string_new (NULL);

      answer = g_new0 (Answer, 1);
      easel_clipboard_read_files_async (self->clipboard, wait.cancellable, on_answer, answer);
      result = wait_for_clipboard_result (&wait, answer);
      if (result != NULL)
        files = easel_clipboard_read_files_finish (self->clipboard, result, NULL);
      g_clear_object (&result);
      for (GSList *f = files; f != NULL; f = f->next)
        {
          g_autofree char *path = g_file_get_path (f->data);

          if (path != NULL)
            g_string_append_printf (paths, "%s%s", paths->len > 0 ? "\n" : "", path);
        }
      g_slist_free_full (files, g_object_unref);
      bytes = files != NULL ? g_string_free_to_bytes (paths) : (g_string_free (paths, TRUE), NULL);
    }
  if (clipboard_wait_end (self, &wait))
    {
      g_clear_pointer (&bytes, g_bytes_unref);    /* text that arrives after a quit request is dropped, the quit gone on (REQ-0017-27) */
      return NULL;
    }
  if (bytes == NULL)
    show_save_error (self, _ ("Could Not Paste"), _ ("The clipboard's text cannot be read."));
  return bytes;
}

/* Paste and Paste as Plain Text with a box open (REQ-0017-20): rich text with each part's style, or the text alone in the style at the
   caret, as one edit at the caret. */
static void
paste_box_text (EaselWindow *self, gboolean rich)
{
  gboolean is_html = FALSE;
  g_autoptr (GBytes) bytes = read_clipboard_text (self, rich, &is_html);
  gsize n = 0;
  const char *data;

  if (bytes == NULL)
    return;
  data = g_bytes_get_data (bytes, &n);
  if (is_html)
    {
      EaselRichText rt;

      if (n > self->rich_limit || !easel_rich_text_from_html (data != NULL ? data : "", n, &rt))
        {
          show_save_error (self, _ ("Could Not Paste"), _ ("There is not enough memory to paste this text."));
          return;
        }
      easel_canvas_text_paste (self->canvas, rt.text, rt.len, rich ? rt.runs : NULL, rich ? rt.n : 0);
      easel_rich_text_free (&rt);
    }
  else
    easel_canvas_text_paste (self->canvas, data != NULL ? data : "", n, NULL, 0);
}

/* Whether a box is open (with Text): the clipboard commands then act on its text (REQ-0017-19) */
static gboolean
box_is_open (EaselWindow *self)
{
  int x, y, w, h;

  return self->tool == EASEL_TOOL_TEXT && easel_canvas_text_box (self->canvas, &x, &y, &w, &h);
}

/* Copy with a box open (REQ-0017-19): the selected text as plain text and as HTML with each part's style. FALSE with none selected. */
static gboolean
copy_box_text (EaselWindow *self)
{
  EaselTextBuffer *b = easel_canvas_text_buffer (self->canvas);
  size_t from, to, len, n, k = 0, html_len = 0;
  const char *text;
  const EaselTextRun *runs;
  g_autofree EaselTextRun *part = NULL;
  g_autofree char *html = NULL;
  g_autoptr (GBytes) html_bytes = NULL;
  g_autofree char *plain = NULL;
  GdkContentProvider *each[2];
  g_autoptr (GdkContentProvider) provider = NULL;

  if (b == NULL || !easel_text_buffer_selection (b, &from, &to))
    return FALSE;
  text = easel_text_buffer_text (b, &len);
  runs = easel_text_buffer_runs (b, &n);
  part = g_new (EaselTextRun, MAX (n, 1));
  for (size_t i = 0; i < n; i++)
    {
      size_t end = i + 1 < n ? runs[i + 1].start : len;

      if (end > from && runs[i].start < to)
        part[k++] = (EaselTextRun) { MAX (runs[i].start, from) - from, runs[i].style };
    }
  html = easel_rich_text_to_html (text + from, to - from, part, k, &html_len);
  if (html != NULL && html_len > self->rich_limit)
    g_clear_pointer (&html, free);                /* the tests' switch: as if its memory could not be had */
  plain = g_strndup (text + from, to - from);
  if (html == NULL || plain == NULL)
    {
      show_save_error (self, _ ("Could Not Copy the Text"), _ ("There is not enough memory to copy this text."));
      return FALSE;
    }
  html_bytes = g_bytes_new_take (g_steal_pointer (&html), html_len);
  each[0] = gdk_content_provider_new_for_bytes ("text/html", html_bytes);
  each[1] = gdk_content_provider_new_typed (G_TYPE_STRING, plain);
  provider = gdk_content_provider_new_union (each, 2);
  easel_clipboard_set_content (self->clipboard, provider);
  return TRUE;
}

static void
paste_plain_activated (GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
  EaselWindow *self = user_data;

  if (self->picture == NULL || dialog_is_open (self) || easel_canvas_is_stroking (self->canvas) || !box_is_open (self))
    return;
  paste_box_text (self, FALSE);
}

static void
paste_activated (GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
  EaselWindow *self = user_data;
  EaselImage *pic;

  if (card_field_action (self, "clipboard.paste"))
    return;
  if (self->picture == NULL || dialog_is_open (self) || easel_canvas_is_stroking (self->canvas)
      || easel_canvas_is_resizing (self->canvas))
    return;
  if (box_is_open (self))
    {
      paste_box_text (self, TRUE);                 /* a box open: its text (REQ-0017-20) */
      return;
    }
  pic = read_clipboard_picture (self);
  if (pic != NULL)
    paste_picture (self, pic);
}

/* A file chosen for Paste From… (REQ-0016-08): read as Open reads it and pasted; a refusal shows its dialog and changes nothing. */
void
easel_window_paste_from_file (EaselWindow *self, GFile *file)
{
  g_autofree char *path = g_file_get_path (file);
  g_autofree char *name = g_file_get_basename (file);
  EaselImage *pic = NULL;
  EaselOpenResult opened;
  int w, h;

  if (self->picture == NULL || dialog_is_open (self) || easel_canvas_is_stroking (self->canvas)
      || easel_canvas_is_resizing (self->canvas))
    return;
  opened = path != NULL ? easel_picture_open (path, &pic, &w, &h) : EASEL_OPEN_UNREADABLE;
  if (pic == NULL)
    paste_error (self, name, opened);
  else
    paste_picture (self, pic);
}

static void
on_paste_from_dialog_done (GtkFileDialog *dialog, GAsyncResult *result, EaselWindow *self)
{
  g_autoptr (GFile) file = gtk_file_dialog_open_finish (dialog, result, NULL);

  self->open_dialog_open = FALSE;
  if (file != NULL)                         /* Cancelled: nothing changes (REQ-0016-08) */
    easel_window_paste_from_file (self, file);
  g_object_unref (self);
}

/* Paste From… (REQ-0016-08): the file dialog; nothing is put down, removed or applied before a file is chosen. */
static void
paste_from_activated (GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
  EaselWindow *self = user_data;
  g_autoptr (GtkFileDialog) dialog = NULL;

  if (self->picture == NULL || dialog_is_open (self) || easel_canvas_is_stroking (self->canvas)
      || easel_canvas_is_resizing (self->canvas))
    return;
  dialog = easel_window_picture_dialog (self, _ ("Paste From"));
  self->open_dialog_open = TRUE;                 /* a quit request does nothing meanwhile, as for Open (REQ-0005-02) */
  gtk_file_dialog_open (dialog, GTK_WINDOW (self), NULL, (GAsyncReadyCallback) on_paste_from_dialog_done, g_object_ref (self));
}

static void
copy_activated (GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
  if (card_field_action (user_data, "clipboard.copy"))
    return;
  if (box_is_open (user_data))
    copy_box_text (user_data);                     /* a box open: its selected text (REQ-0017-19) */
  else
    copy_selection (user_data);
}

/* Cut (REQ-0016-04): the copy, then Clear Selection's own clear; if the copy fails nothing is cleared. */
static void
cut_activated (GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
  EaselWindow *self = user_data;

  if (card_field_action (self, "clipboard.cut"))
    return;
  if (box_is_open (self))
    {
      if (copy_box_text (self))
        easel_canvas_text_delete_selection (self->canvas, EASEL_TEXT_EDIT_CUT);   /* one edit (REQ-0017-19) */
      return;
    }
  if (copy_selection (self))
    easel_canvas_clear_selection (self->canvas);
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
  /* Another tool puts a pending curve or polygon down; the same tool keeps it (REQ-0011-12). */
  if (tool != self->tool && !put_down_for_action (self, FALSE))
    return;                                     /* a text box that could not be put down: Text stays the tool (REQ-0017-18) */
  if (tool == EASEL_TOOL_PICK && self->tool != EASEL_TOOL_PICK)
    self->pick_return_tool = self->tool;
  if (tool != self->tool)
    easel_window_announce (self, tool_title (tool));
  self->tool = tool;
  easel_canvas_set_tool (self->canvas, tool);
  easel_status_bar_set_hint (self->status_bar, tool_hint (tool, easel_tool_options_get_transparent (self->tool_options)));
  easel_tool_options_show_tool (self->tool_options, tool);
  sync_canvas_options (self);
  update_card (self);
  g_simple_action_set_state (action, value);
}

/* Numpad + and -: the next larger or smaller preset of the tool; like the View actions, not during a stroke or while a
   dialog is open (REQ-0006-14, REQ-0004-03). Only the Brush, the Airbrush and the Eraser have presets. */
static void
step_preset (EaselWindow *self, int direction)
{
  if (!view_actions_allowed (self))
    return;
  if (easel_tool_uses_line_width (self->tool))
    {
      /* The width of Line and the shapes, 1 to 5, stopping at the ends (REQ-0010-04); the fill style is not touched. */
      g_autoptr (GVariant) state = g_action_group_get_action_state (G_ACTION_GROUP (self), "line-width");
      int next = CLAMP (g_variant_get_int32 (state) + direction, 1, 5);

      g_action_group_change_action_state (G_ACTION_GROUP (self), "line-width", g_variant_new_int32 (next));
    }
  else if (self->tool == EASEL_TOOL_BRUSH || self->tool == EASEL_TOOL_AIRBRUSH || self->tool == EASEL_TOOL_ERASER)
    easel_tool_options_step (self->tool_options, self->tool, direction);
}

static void
preset_larger_activated (GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
  step_preset (user_data, 1);
}

static void
preset_smaller_activated (GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
  step_preset (user_data, -1);
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
  { .name = "preset-larger", .activate = preset_larger_activated },
  { .name = "preset-smaller", .activate = preset_smaller_activated },
  { .name = "zoom-in", .activate = zoom_in_activated },
  { .name = "zoom-out", .activate = zoom_out_activated },
  { .name = "zoom-normal", .activate = zoom_normal_activated },
  { .name = "zoom-large", .activate = zoom_large_activated },
  { .name = "zoom-fit", .activate = zoom_fit_activated },
  { .name = "zoom-to", .parameter_type = "i", .state = "0", .activate = zoom_to_activated },
  { .name = "magnifier-level", .parameter_type = "i", .state = "200", .change_state = magnifier_level_changed },
  { .name = "line-width", .parameter_type = "i", .state = "1", .change_state = line_width_changed },
  { .name = "fill-style", .parameter_type = "i", .state = "0", .change_state = fill_style_changed },
  { .name = "grid", .state = "false", .change_state = grid_state_changed },
  { .name = "thumbnail", .state = "false", .change_state = thumbnail_state_changed },
  { .name = "grid-dialog", .activate = grid_dialog_activated },
  { .name = "attributes", .activate = attributes_activated },
  { .name = "flip-horizontal", .activate = flip_horizontal_activated },
  { .name = "flip-vertical", .activate = flip_vertical_activated },
  { .name = "rotate-90", .activate = rotate_90_activated },
  { .name = "rotate-180", .activate = rotate_180_activated },
  { .name = "rotate-270", .activate = rotate_270_activated },
  { .name = "stretch-skew", .activate = stretch_skew_activated },
  { .name = "preferences", .activate = preferences_activated },
  { .name = "print", .activate = print_activated },
  { .name = "shortcuts", .activate = shortcuts_activated },
  { .name = "help", .activate = help_activated },
  { .name = "about", .activate = about_activated },
  { .name = "invert", .activate = invert_activated },
  { .name = "black-white", .activate = black_white_activated },
  { .name = "clear-image", .activate = clear_image_activated },
  { .name = "show-toolbox", .state = "true", .change_state = show_panel_state_changed },
  { .name = "show-colorbox", .state = "true", .change_state = show_panel_state_changed },
  { .name = "show-statusbar", .state = "true", .change_state = show_panel_state_changed },
  { .name = "close", .activate = close_activated },
  { .name = "clear-selection", .activate = clear_selection_activated },
  { .name = "select-all", .activate = select_all_activated },
  { .name = "cut", .activate = cut_activated },
  { .name = "copy", .activate = copy_activated },
  { .name = "paste", .activate = paste_activated },
  { .name = "paste-plain", .activate = paste_plain_activated },
  { .name = "copy-to", .activate = copy_to_activated },
  { .name = "paste-from", .activate = paste_from_activated },
};

gboolean
easel_window_get_text_opaque (EaselWindow *self)
{
  return !easel_tool_options_get_text_transparent (self->tool_options);
}

EaselToolOptions *
easel_window_get_tool_options (EaselWindow *self)
{
  return self->tool_options;
}

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

GtkWidget *
easel_window_get_text_access (EaselWindow *self)
{
  return self->text_access;
}

GtkWidget *
easel_window_get_thumbnail_card (EaselWindow *self)
{
  return GTK_WIDGET (self->thumbnail_card);
}

EaselFontCard *
easel_window_get_font_card (EaselWindow *self)
{
  return self->font_card;
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

  g_clear_handle_id (&self->card_idle, g_source_remove);
  g_clear_handle_id (&self->toolbox_follow, g_source_remove);
  g_clear_handle_id (&self->frame_time_timer, g_source_remove);
  g_clear_handle_id (&self->help_error_timer, g_source_remove);
  self->help_dropped = TRUE;
  g_clear_handle_id (&self->print_error_timer, g_source_remove);
  g_clear_handle_id (&self->card_focus_idle, g_source_remove);
  if (self->font_card != NULL)
    easel_font_card_set_canvas (self->font_card, NULL);   /* a field left with a typed value applies nothing while the window goes */
  if (self->selection_menu != NULL)
    {
      gtk_widget_unparent (self->selection_menu);
      self->selection_menu = NULL;
    }
  if (self->text_menu != NULL)
    {
      gtk_widget_unparent (self->text_menu);
      self->text_menu = NULL;
    }
  if (self->canvas_menu != NULL)
    {
      gtk_widget_unparent (self->canvas_menu);
      self->canvas_menu = NULL;
    }
  if (self->clipboard != NULL)
    g_signal_handlers_disconnect_by_data (self->clipboard, self);
  g_clear_object (&self->clipboard);
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
  self->tools_column = NULL;
  self->tool_options = NULL;
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
  easel_page_setup_clear (&self->page_setup);
  g_free (self->print_export);
  if (self->announced != NULL)
    g_string_free (self->announced, TRUE);
  g_clear_object (&self->print_settings);
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
  g_signal_add_emission_hook (g_signal_lookup ("close-request", GTK_TYPE_WINDOW), 0, close_request_hook, NULL, NULL);

  g_type_ensure (EASEL_TYPE_CANVAS);
  g_type_ensure (EASEL_TYPE_PALETTE_BAR);
  g_type_ensure (EASEL_TYPE_STATUS_BAR);
  g_type_ensure (EASEL_TYPE_FONT_CARD);
  g_type_ensure (EASEL_TYPE_THUMBNAIL_CARD);
  /* The template's marked strings in the language of this run, as text, so that no system locale is needed (REQ-0023-05). */
  {
    g_autoptr (GBytes) ui = g_resources_lookup_data (RESOURCE_PATH "/window.ui", G_RESOURCE_LOOKUP_FLAGS_NONE, NULL);
    gsize len = 0;
    const char *xml = g_bytes_get_data (ui, &len);
    char *translated = easel_i18n_translate_template (xml);
    g_autoptr (GBytes) bytes = translated != NULL ? g_bytes_new_with_free_func (translated, strlen (translated), free, translated) : g_bytes_ref (ui);

    gtk_widget_class_set_template (widget_class, bytes);
  }
  gtk_widget_class_bind_template_child (widget_class, EaselWindow, scroller);
  gtk_widget_class_bind_template_child (widget_class, EaselWindow, canvas_overlay);
  gtk_widget_class_bind_template_child (widget_class, EaselWindow, font_card);
  gtk_widget_class_bind_template_child (widget_class, EaselWindow, thumbnail_card);
  gtk_widget_class_bind_template_child (widget_class, EaselWindow, text_access);
  gtk_widget_class_bind_template_child (widget_class, EaselWindow, header_bar);
  gtk_widget_class_bind_template_child (widget_class, EaselWindow, toolbox);
  gtk_widget_class_bind_template_child (widget_class, EaselWindow, canvas);
  gtk_widget_class_bind_template_child (widget_class, EaselWindow, palette_bar);
  gtk_widget_class_bind_template_child (widget_class, EaselWindow, status_bar);
  gtk_widget_class_bind_template_child (widget_class, EaselWindow, toolbox_panel);
  gtk_widget_class_bind_template_child (widget_class, EaselWindow, tools_column);
  gtk_widget_class_bind_template_child (widget_class, EaselWindow, image_button);
  gtk_widget_class_bind_template_child (widget_class, EaselWindow, main_menu_button);
  gtk_widget_class_bind_template_child_full (widget_class, "selection_menu", FALSE,
                                            G_STRUCT_OFFSET (EaselWindow, selection_menu_model));
  gtk_widget_class_bind_template_child_full (widget_class, "canvas_menu", FALSE, G_STRUCT_OFFSET (EaselWindow, canvas_menu_model));
  gtk_widget_class_bind_template_child_full (widget_class, "text_menu", FALSE, G_STRUCT_OFFSET (EaselWindow, text_menu_model));
  gtk_widget_class_bind_template_child (widget_class, EaselWindow, view_button);
}

static void
easel_window_init (EaselWindow *self)
{
  g_autofree char *settings_path = easel_settings_file_path ();

  gtk_widget_init_template (GTK_WIDGET (self));
  {
    GtkWidget *bars[] = { self->header_bar, GTK_WIDGET (self->palette_bar), GTK_WIDGET (self->status_bar) };

    for (gsize i = 0; i < G_N_ELEMENTS (bars); i++)
      {
        GtkGesture *click = gtk_gesture_click_new ();

        gtk_event_controller_set_propagation_phase (GTK_EVENT_CONTROLLER (click), GTK_PHASE_CAPTURE);
        g_signal_connect (click, "pressed", G_CALLBACK (on_bar_pressed), self);
        gtk_widget_add_controller (bars[i], GTK_EVENT_CONTROLLER (click));
      }
  }

  self->clipboard_timeout_ms = 5000;
  self->rich_limit = G_MAXSIZE;
  self->transform_limit = G_MAXSIZE;
  easel_settings_file_load (settings_path, &self->settings);
  self->chosen = self->settings;
  self->tool = EASEL_TOOL_PAN;
  self->foreground = 0xFF000000u;
  self->background = self->settings.background;

  self->history = easel_history_new ((size_t) self->settings.undo_budget_mb << 20);
  if (self->history == NULL)
    g_error ("Out of memory creating the undo history");

  g_action_map_add_action_entries (G_ACTION_MAP (self), win_actions, G_N_ELEMENTS (win_actions), self);
  add_remove_background_action (self);
  g_simple_action_set_enabled (G_SIMPLE_ACTION (g_action_map_lookup_action (G_ACTION_MAP (self), "clear-selection")), FALSE);
  easel_window_set_clipboard (self, NULL);
  add_zoom_submenu (self);
  g_action_group_change_action_state (G_ACTION_GROUP (self), "line-width", g_variant_new_int32 (self->settings.line_width));
  g_action_group_change_action_state (G_ACTION_GROUP (self), "fill-style", g_variant_new_int32 ((int) self->settings.fill_style));
  g_action_group_change_action_state (G_ACTION_GROUP (self), "magnifier-level", g_variant_new_int32 (self->settings.magnifier * 100));
  gtk_popover_menu_add_child (GTK_POPOVER_MENU (gtk_menu_button_get_popover (self->view_button)), make_zoom_row (self),
                              "zoom");
  easel_font_card_set_canvas (self->font_card, self->canvas);
  easel_canvas_set_zoom_gate (self->canvas, wheel_zoom_gate, self);
  {
    /* Over the whole card, before its parts: Ctrl+wheel zooms, also over the size field whatever has the focus (REQ-0036-01) */
    GtkEventController *wheel = gtk_event_controller_scroll_new (GTK_EVENT_CONTROLLER_SCROLL_BOTH_AXES);

    gtk_event_controller_set_propagation_phase (wheel, GTK_PHASE_CAPTURE);
    g_signal_connect_object (wheel, "scroll", G_CALLBACK (on_card_scroll), self, G_CONNECT_DEFAULT);
    gtk_widget_add_controller (GTK_WIDGET (self->font_card), wheel);
  }
  {
    /* Before the size field's own wheel: without the focus in it, the wheel goes to the view (REQ-0017-25) */
    GtkEventController *wheel = gtk_event_controller_scroll_new (GTK_EVENT_CONTROLLER_SCROLL_BOTH_AXES);

    gtk_event_controller_set_propagation_phase (wheel, GTK_PHASE_CAPTURE);
    g_signal_connect_object (wheel, "scroll", G_CALLBACK (on_card_size_scroll), self, G_CONNECT_DEFAULT);
    gtk_widget_add_controller (GTK_WIDGET (easel_font_card_get_size (self->font_card)), wheel);
  }
  g_signal_connect (self, "notify::focus-widget", G_CALLBACK (on_focus_widget_changed), NULL);
  g_signal_connect_object (self->canvas_overlay, "get-child-position", G_CALLBACK (on_card_position), self, 0);
  g_signal_connect_object (gtk_scrolled_window_get_hadjustment (self->scroller), "value-changed", G_CALLBACK (update_card), self,
                           G_CONNECT_SWAPPED);
  g_signal_connect_object (gtk_scrolled_window_get_vadjustment (self->scroller), "value-changed", G_CALLBACK (update_card), self,
                           G_CONNECT_SWAPPED);
  g_signal_connect_object (gtk_scrolled_window_get_hadjustment (self->scroller), "changed", G_CALLBACK (update_card), self,
                           G_CONNECT_SWAPPED);
  g_signal_connect_object (gtk_scrolled_window_get_vadjustment (self->scroller), "changed", G_CALLBACK (update_card), self,
                           G_CONNECT_SWAPPED);
  g_signal_connect_object (self->canvas, "zoom-changed", G_CALLBACK (on_zoom_changed), self, G_CONNECT_SWAPPED);
  g_signal_connect_object (self->canvas, "pointer-moved", G_CALLBACK (on_pointer_moved), self, G_CONNECT_SWAPPED);
  g_signal_connect_object (self->canvas, "pointer-left", G_CALLBACK (on_pointer_left), self, G_CONNECT_SWAPPED);
  easel_status_bar_set_hint (self->status_bar, tool_hint (self->tool, FALSE));
  sync_zoom_ui (self);
  easel_canvas_set_history (self->canvas, self->history);
  easel_canvas_listen_on (self->canvas, GTK_WIDGET (self->scroller));
  gtk_widget_add_css_class (GTK_WIDGET (self->scroller), "workspace");
  apply_workspace_color (&self->settings);
  easel_canvas_set_tool (self->canvas, self->tool);
  {
    g_autofree char *palette_path = easel_palette_file_path ();

    easel_palette_file_load (palette_path, &self->palette);
  }
  easel_palette_bar_set_palette (self->palette_bar, &self->palette);
  easel_canvas_set_tint (self->canvas, self->settings.tint);
  easel_window_apply_style (self, self->settings.style);
  /* The grid and the panels start as the settings file says (REQ-0004-15, REQ-0004-27). */
  easel_canvas_set_grid (self->canvas, self->settings.grid, self->settings.grid_size, self->settings.grid_axonometric);
  apply_text_settings (self);
  g_simple_action_set_state (G_SIMPLE_ACTION (g_action_map_lookup_action (G_ACTION_MAP (self), "grid")),
                             g_variant_new_boolean (self->settings.grid));
  /* The options panel sits below the tool buttons and shows only for the tools that have options. */
  self->tool_options = easel_tool_options_new ();
  easel_tool_options_set_start (self->tool_options, self->settings.brush, self->settings.airbrush, self->settings.eraser, self->settings.eraser_transparent);
  {
    /* The panel asks for no width beyond its least: a long name wraps in the width the toolbox has instead of
       widening it (REQ-0030-10). Nothing scrolls. */
    GtkWidget *holder = gtk_scrolled_window_new ();

    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (holder), GTK_POLICY_NEVER, GTK_POLICY_NEVER);
    gtk_scrolled_window_set_propagate_natural_height (GTK_SCROLLED_WINDOW (holder), TRUE);
    gtk_scrolled_window_set_child (GTK_SCROLLED_WINDOW (holder), GTK_WIDGET (self->tool_options));
    gtk_box_append (GTK_BOX (self->tools_column), holder);
  }
  g_signal_connect_swapped (self->tool_options, "preset-changed", G_CALLBACK (sync_canvas_options), self);
  g_signal_connect_object (self->canvas, "stroking-changed", G_CALLBACK (on_stroking_changed), self, G_CONNECT_SWAPPED);
  /* The toolbox never shares the canvas area's space; its options keep their size, centered in a wider toolbox (REQ-0030-10). */
  gtk_widget_set_hexpand (self->toolbox_panel, FALSE);
  gtk_widget_set_halign (GTK_WIDGET (self->tool_options), GTK_ALIGN_CENTER);
  gtk_widget_set_hexpand (self->tools_column, TRUE);             /* a wider toolbox: the column fills it, its line at the right */
  make_toolbox_edge (self);
  self->toolbox_kept = self->settings.toolbox_width;
  apply_toolbox_width (self);
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
  g_signal_connect_swapped (self, "notify::visible-dialog", G_CALLBACK (on_visible_dialog_changed), self);
  self->help_launch = launch_in_browser;
  gtk_widget_set_focusable (GTK_WIDGET (self->canvas), TRUE);
  /* The view must not jump when the focus reaches the canvas (docs/design.md). */
  gtk_viewport_set_scroll_to_focus (GTK_VIEWPORT (gtk_widget_get_parent (GTK_WIDGET (self->canvas))), FALSE);
  name_by_tooltip (GTK_WIDGET (self));
  self->print_run = run_print;
  self->print_copy_limit = G_MAXSIZE;
  easel_page_setup_init (&self->page_setup);
  easel_thumbnail_card_set_view (self->thumbnail_card, self->canvas, self->scroller);
  thumbnail_connect_input (self);
  g_signal_connect_object (gtk_scrolled_window_get_hadjustment (self->scroller), "changed", G_CALLBACK (sync_thumbnail), self, G_CONNECT_SWAPPED);
  g_signal_connect_object (gtk_scrolled_window_get_vadjustment (self->scroller), "changed", G_CALLBACK (sync_thumbnail), self, G_CONNECT_SWAPPED);
  {
    GtkEventController *keys = gtk_event_controller_key_new ();

    gtk_event_controller_set_propagation_phase (keys, GTK_PHASE_CAPTURE);
    g_signal_connect_swapped (keys, "key-pressed", G_CALLBACK (on_key_pressed), self);
    g_signal_connect_swapped (keys, "key-released", G_CALLBACK (on_key_released), self);
    g_signal_connect_swapped (keys, "modifiers", G_CALLBACK (on_modifiers_changed), self);
    gtk_widget_add_controller (GTK_WIDGET (self), keys);
  }
  g_signal_connect_object (self->canvas, "color-picked", G_CALLBACK (on_color_picked), self, G_CONNECT_SWAPPED);
  g_signal_connect_object (self->canvas, "pick-ended", G_CALLBACK (on_pick_ended), self, G_CONNECT_SWAPPED);
  g_signal_connect_object (self->canvas, "fill-failed", G_CALLBACK (on_fill_failed), self, G_CONNECT_SWAPPED);
  g_signal_connect_object (self->canvas, "shape-failed", G_CALLBACK (on_shape_failed), self, G_CONNECT_SWAPPED);
  g_signal_connect_object (self->canvas, "gradient-failed", G_CALLBACK (on_gradient_failed), self, G_CONNECT_SWAPPED);
  g_signal_connect_object (self->canvas, "selection-changed", G_CALLBACK (on_selection_changed), self, G_CONNECT_SWAPPED);
  g_signal_connect_object (self->canvas, "polygon-closed", G_CALLBACK (on_polygon_closed), self, G_CONNECT_SWAPPED);
  g_signal_connect_object (self->canvas, "context-menu", G_CALLBACK (on_context_menu), self, G_CONNECT_SWAPPED);
  g_signal_connect_object (self->canvas, "selection-failed", G_CALLBACK (on_selection_failed), self, G_CONNECT_SWAPPED);
  g_signal_connect_object (self->canvas, "text-failed", G_CALLBACK (on_text_failed), self, G_CONNECT_SWAPPED);
  self->selection_menu = gtk_popover_menu_new_from_model (self->selection_menu_model);
  gtk_popover_set_has_arrow (GTK_POPOVER (self->selection_menu), FALSE);
  gtk_widget_set_halign (self->selection_menu, GTK_ALIGN_START);
  gtk_widget_set_parent (self->selection_menu, GTK_WIDGET (self->canvas));
  g_signal_connect_object (self->canvas, "canvas-menu", G_CALLBACK (on_canvas_menu), self, G_CONNECT_SWAPPED);
  g_signal_connect_object (self->canvas, "painted", G_CALLBACK (on_canvas_painted), self, G_CONNECT_SWAPPED);
  easel_window_apply_frame_time (self, self->settings.frame_time);
  g_signal_connect_object (self->canvas, "picture-replaced", G_CALLBACK (picture_size_changed), self, G_CONNECT_SWAPPED);
  self->canvas_menu = gtk_popover_menu_new_from_model (self->canvas_menu_model);
  gtk_popover_set_has_arrow (GTK_POPOVER (self->canvas_menu), FALSE);
  gtk_widget_set_halign (self->canvas_menu, GTK_ALIGN_START);
  gtk_widget_set_parent (self->canvas_menu, GTK_WIDGET (self->canvas));
  self->text_menu = gtk_popover_menu_new_from_model (self->text_menu_model);
  gtk_popover_set_has_arrow (GTK_POPOVER (self->text_menu), FALSE);
  gtk_widget_set_halign (self->text_menu, GTK_ALIGN_START);
  gtk_widget_set_parent (self->text_menu, GTK_WIDGET (self->canvas));
  g_signal_connect_object (self->canvas, "pending-changed", G_CALLBACK (sync_history_ui), self, G_CONNECT_SWAPPED);
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
