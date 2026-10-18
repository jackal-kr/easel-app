/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Widget-level scenarios of feature 0001. Needs a display; exits 77 (skip) without one. */

#include <adwaita.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <glib/gstdio.h>
#include <stdio.h>
#include <string.h>
#include <sys/resource.h>
#include <unistd.h>

#include "application.h"
#include "canvas.h"
#include "magnifier.h"
#include "palette-bar.h"
#include "picture-file.h"
#include "palette-file.h"
#include "status-bar.h"
#include "stroke.h"
#include "settings-file.h"
#include "settings.h"
#include "view.h"
#include "window.h"

/* One application per process: GApplication cannot be registered twice. Tests close their windows. */
static GtkApplication *app;

/* The canvas coordinate (logical pixels from the picture's top-left) of a point of the view, view_point pixels from the
   left (or top) of the canvas area: the scroll offset counts the margin before the picture (REQ-0009-01). */
static double
view_to_canvas (GtkAdjustment *adj, double view_point)
{
  return gtk_adjustment_get_value (adj) + view_point - EASEL_VIEW_MARGIN;
}

static void
close_windows (void)
{
  GList *windows;

  while ((windows = gtk_application_get_windows (app)) != NULL)
    gtk_window_destroy (windows->data);
}

static void
as8_one_window (void)
{
  g_application_activate (G_APPLICATION (app));
  g_application_activate (G_APPLICATION (app));

  g_assert_cmpuint (g_list_length (gtk_application_get_windows (app)), ==, 1);
  close_windows ();
}

static void
write_settings (const char *text)
{
  g_autofree char *path = easel_settings_file_path ();
  g_autofree char *dir = g_path_get_dirname (path);

  g_assert_cmpint (g_mkdir_with_parents (dir, 0700), ==, 0);
  g_assert_true (g_file_set_contents (path, text, -1, NULL));
}

/* Tests that edit swatches remove the palette file they wrote, so later tests see the default palette. */
static void
remove_palette_file (void)
{
  g_autofree char *path = easel_palette_file_path ();

  g_remove (path);
}

/* Activates the app and waits (at most 5 s) until the window has its start-up picture. */
static EaselWindow *
start_window (void)
{
  EaselWindow *win;
  gint64 deadline = g_get_monotonic_time () + 5 * G_USEC_PER_SEC;

  g_application_activate (G_APPLICATION (app));
  win = EASEL_WINDOW (gtk_application_get_active_window (app));
  while (easel_window_get_picture (win) == NULL && g_get_monotonic_time () < deadline)
    g_main_context_iteration (NULL, FALSE);
  g_assert_nonnull (easel_window_get_picture (win));
  return win;
}

static void
as1_starts_with_settings (void)
{
  const EaselImage *img;
  EaselWindow *win;

  write_settings ("[new-picture]\nwidth=300\nheight=200\nbackground=#FFFF00\n");
  win = start_window ();
  img = easel_window_get_picture (win);

  g_assert_cmpint (img->width, ==, 300);
  g_assert_cmpint (img->height, ==, 200);
  g_assert_cmphex (easel_image_get (img, 0, 0), ==, 0xFFFFFF00u);
  g_assert_cmphex (easel_image_get (img, 299, 199), ==, 0xFFFFFF00u);
  g_assert_cmpint (easel_window_get_zoom (win), ==, 100);
  g_assert_cmpint (easel_window_get_tool (win), ==, EASEL_TOOL_PAN);
  g_assert_cmphex (easel_window_get_foreground (win), ==, 0xFF000000u);
  g_assert_cmphex (easel_window_get_background (win), ==, 0xFFFFFF00u);
  close_windows ();
}

static void
as2_auto_fits_view (void)
{
  EaselWindow *win;
  const EaselImage *img;
  GtkScrolledWindow *scroller;
  GdkMonitor *monitor;
  GdkRectangle geometry;
  double scale;
  int area_w, area_h, w, h, dw, dh;

  write_settings ("[new-picture]\nwidth=auto\nheight=auto\n");
  win = start_window ();
  img = easel_window_get_picture (win);

  /* Window: 75 % of the first monitor (REQ-0001-41). */
  monitor = g_list_model_get_item (gdk_display_get_monitors (gdk_display_get_default ()), 0);
  gdk_monitor_get_geometry (monitor, &geometry);
  g_object_unref (monitor);
  easel_view_window_size (geometry.width, geometry.height, &w, &h);
  gtk_window_get_default_size (GTK_WINDOW (win), &dw, &dh);
  g_assert_cmpint (dw, ==, w);
  g_assert_cmpint (dh, ==, h);

  /* Picture: the largest standard size smaller than the visible area, so no scrollbars (REQ-0001-02). */
  scroller = GTK_SCROLLED_WINDOW (gtk_widget_get_ancestor (GTK_WIDGET (easel_window_get_canvas (win)),
                                                           GTK_TYPE_SCROLLED_WINDOW));
  scale = easel_canvas_get_scale (easel_window_get_canvas (win));
  area_w = (int) (gtk_adjustment_get_page_size (gtk_scrolled_window_get_hadjustment (scroller)) * scale);
  area_h = (int) (gtk_adjustment_get_page_size (gtk_scrolled_window_get_vadjustment (scroller)) * scale);
  /* Smaller than the area less its two margins (REQ-0001-02 as amended by 0009, REQ-0009-01). */
  g_assert_cmpint (img->width, <, area_w - (int) (2 * EASEL_VIEW_MARGIN * scale));
  g_assert_cmpint (img->height, <, area_h - (int) (2 * EASEL_VIEW_MARGIN * scale));
  g_assert_true ((img->width == 640 && img->height == 480) || (img->width == 800 && img->height == 600)
                 || (img->width == 1024 && img->height == 768));
  close_windows ();
  write_settings ("[new-picture]\nwidth=300\nheight=200\nbackground=#FFFF00\n");
}

/* Runs the main loop until the window is shown and its size has not changed for 200 ms (at most 3 s), so a
   late resize from the compositor cannot land between a test's action and its check. */
static void
wait_for_layout (EaselWindow *win)
{
  gint64 deadline = g_get_monotonic_time () + 3 * G_USEC_PER_SEC;
  gint64 stable_since = g_get_monotonic_time ();
  int last_w = -1, last_h = -1;

  while (g_get_monotonic_time () < deadline)
    {
      int w = gtk_widget_get_width (GTK_WIDGET (win));
      int h = gtk_widget_get_height (GTK_WIDGET (win));

      g_main_context_iteration (NULL, FALSE);
      if (w != last_w || h != last_h || !gtk_widget_get_mapped (GTK_WIDGET (win)))
        {
          last_w = w;
          last_h = h;
          stable_since = g_get_monotonic_time ();
        }
      else if (g_get_monotonic_time () - stable_since > G_USEC_PER_SEC / 5)
        return;
    }
}

static void
as13_zoom_keeps_point_under_pointer (void)
{
  EaselWindow *win;
  EaselCanvas *canvas;
  GtkScrolledWindow *scroller;
  GtkAdjustment *h, *v;
  double scale;
  int px, py;

  write_settings ("[new-picture]\nwidth=2000\nheight=2000\n");
  win = start_window ();
  canvas = easel_window_get_canvas (win);
  scroller = GTK_SCROLLED_WINDOW (gtk_widget_get_ancestor (GTK_WIDGET (canvas), GTK_TYPE_SCROLLED_WINDOW));
  h = gtk_scrolled_window_get_hadjustment (scroller);
  v = gtk_scrolled_window_get_vadjustment (scroller);
  wait_for_layout (win);
  scale = easel_canvas_get_scale (canvas);

  easel_canvas_zoom_at (canvas, 1, 40.5, 30.5);
  wait_for_layout (win);
  g_assert_cmpint (easel_window_get_zoom (win), ==, 200);
  easel_view_to_pixel (view_to_canvas (h, 40.5), view_to_canvas (v, 30.5), 200, scale, &px, &py);
  g_assert_cmpint (px, ==, (int) ((40.5 - EASEL_VIEW_MARGIN) * scale));    /* the pixel the point was over at 100 % */
  g_assert_cmpint (py, ==, (int) ((30.5 - EASEL_VIEW_MARGIN) * scale));

  easel_canvas_zoom_at (canvas, 20, 0, 0);
  g_assert_cmpint (easel_window_get_zoom (win), ==, 800);
  easel_canvas_zoom_at (canvas, -20, 0, 0);
  g_assert_cmpint (easel_window_get_zoom (win), ==, 10);

  /* Only Pan zooms (REQ-0001-17). */
  g_action_group_activate_action (G_ACTION_GROUP (win), "tool", g_variant_new_string ("pencil"));
  easel_canvas_zoom_at (canvas, 1, 0, 0);
  g_assert_cmpint (easel_window_get_zoom (win), ==, 10);
  close_windows ();
  write_settings ("[new-picture]\nwidth=300\nheight=200\nbackground=#FFFF00\n");
}

static GtkToggleButton *
find_tool_button (GtkWidget *root, const char *tool)
{
  for (GtkWidget *child = gtk_widget_get_first_child (root); child; child = gtk_widget_get_next_sibling (child))
    {
      GtkToggleButton *found;

      if (GTK_IS_TOGGLE_BUTTON (child))
        {
          GVariant *target = gtk_actionable_get_action_target_value (GTK_ACTIONABLE (child));

          if (target != NULL && g_strcmp0 (g_variant_get_string (target, NULL), tool) == 0)
            return GTK_TOGGLE_BUTTON (child);
        }
      found = find_tool_button (child, tool);
      if (found != NULL)
        return found;
    }
  return NULL;
}

static void
as9_tool_selected_state (void)
{
  EaselWindow *win = start_window ();
  GtkToggleButton *pan = find_tool_button (GTK_WIDGET (win), "pan");
  GtkToggleButton *pencil = find_tool_button (GTK_WIDGET (win), "pencil");
  EaselCanvas *canvas = easel_window_get_canvas (win);

  g_assert_nonnull (pan);
  g_assert_nonnull (pencil);
  g_assert_true (gtk_toggle_button_get_active (pan));
  g_assert_false (gtk_toggle_button_get_active (pencil));

  g_signal_emit_by_name (pencil, "clicked");
  g_assert_cmpint (easel_window_get_tool (win), ==, EASEL_TOOL_PENCIL);
  g_assert_true (gtk_toggle_button_get_active (pencil));
  g_assert_false (gtk_toggle_button_get_active (pan));

  /* A tool cannot change during a stroke (REQ-0001-09). */
  easel_canvas_press (canvas, 1, 5.5, 5.5);
  g_signal_emit_by_name (pan, "clicked");
  g_assert_cmpint (easel_window_get_tool (win), ==, EASEL_TOOL_PENCIL);
  g_assert_true (gtk_toggle_button_get_active (pencil));
  easel_canvas_release (canvas, 1);

  g_signal_emit_by_name (pan, "clicked");
  g_assert_cmpint (easel_window_get_tool (win), ==, EASEL_TOOL_PAN);
  close_windows ();
}

static void
as21_pencil_on_canvas (void)
{
  EaselWindow *win = start_window ();
  EaselCanvas *canvas = easel_window_get_canvas (win);
  const EaselImage *img = easel_window_get_picture (win);

  /* Pan does not draw. */
  easel_canvas_press (canvas, 1, 10.5, 10.5);
  easel_canvas_release (canvas, 1);
  g_assert_cmphex (easel_image_get (img, 10, 10), ==, 0xFFFFFF00u);
  g_assert_false (easel_window_get_modified (win));

  g_action_group_activate_action (G_ACTION_GROUP (win), "tool", g_variant_new_string ("pencil"));

  /* The middle button is ignored and does not end the stroke (REQ-0001-24). */
  easel_canvas_press (canvas, 1, 10.5, 10.5);
  easel_canvas_motion (canvas, 15.5, 10.5);
  easel_canvas_press (canvas, 2, 15.5, 10.5);
  easel_canvas_release (canvas, 2);
  easel_canvas_motion (canvas, 20.5, 10.5);
  easel_canvas_release (canvas, 1);
  for (int x = 10; x <= 20; x++)
    g_assert_cmphex (easel_image_get (img, x, 10), ==, 0xFF000000u);
  g_assert_cmphex (easel_image_get (img, 21, 10), ==, 0xFFFFFF00u);
  g_assert_true (easel_window_get_modified (win));

  /* Secondary draws the background color (yellow here). */
  easel_canvas_press (canvas, 3, 12.5, 10.5);
  easel_canvas_release (canvas, 3);
  g_assert_cmphex (easel_image_get (img, 12, 10), ==, 0xFFFFFF00u);

  /* The other button cancels the stroke (REQ-0001-23); motion after that draws nothing. */
  easel_canvas_press (canvas, 1, 0.5, 50.5);
  easel_canvas_motion (canvas, 30.5, 50.5);
  easel_canvas_press (canvas, 3, 30.5, 50.5);
  easel_canvas_motion (canvas, 40.5, 50.5);
  easel_canvas_release (canvas, 3);
  easel_canvas_release (canvas, 1);
  for (int x = 0; x <= 40; x++)
    g_assert_cmphex (easel_image_get (img, x, 50), ==, 0xFFFFFF00u);
  close_windows ();
}

static void
as10_title_tracks_state (void)
{
  EaselWindow *win = start_window ();
  EaselCanvas *canvas = easel_window_get_canvas (win);
  g_autofree char *dir = g_dir_make_tmp ("easel-ui-XXXXXX", NULL);
  g_autofree char *path = g_build_filename (dir, "a.png", NULL);

  g_assert_cmpstr (gtk_window_get_title (GTK_WINDOW (win)), ==, "Untitled – Easel");

  g_action_group_activate_action (G_ACTION_GROUP (win), "tool", g_variant_new_string ("pencil"));
  easel_canvas_press (canvas, 1, 3.5, 3.5);
  easel_canvas_release (canvas, 1);
  g_assert_cmpstr (gtk_window_get_title (GTK_WINDOW (win)), ==, "• Untitled – Easel");

  g_assert_true (easel_window_save_to (win, path));
  g_assert_cmpstr (gtk_window_get_title (GTK_WINDOW (win)), ==, "a.png – Easel");
  g_assert_false (easel_window_get_modified (win));
  close_windows ();
}

static void
as26_save_existing_no_dialog (void)
{
  EaselWindow *win = start_window ();
  EaselCanvas *canvas = easel_window_get_canvas (win);
  g_autofree char *dir = g_dir_make_tmp ("easel-ui-XXXXXX", NULL);
  g_autofree char *path = g_build_filename (dir, "a.png", NULL);
  g_autofree char *before = NULL, *after = NULL;
  gsize before_len, after_len;

  g_assert_true (easel_window_save_to (win, path));
  g_assert_true (g_file_get_contents (path, &before, &before_len, NULL));

  g_action_group_activate_action (G_ACTION_GROUP (win), "tool", g_variant_new_string ("pencil"));
  easel_canvas_press (canvas, 1, 3.5, 3.5);
  easel_canvas_release (canvas, 1);
  g_assert_true (easel_window_get_modified (win));

  /* Save on a picture with a file writes it at once, with no dialog in between. */
  g_action_group_activate_action (G_ACTION_GROUP (win), "save", NULL);
  g_assert_false (easel_window_get_modified (win));
  g_assert_true (g_file_get_contents (path, &after, &after_len, NULL));
  g_assert_false (before_len == after_len && memcmp (before, after, before_len) == 0);
  close_windows ();
}

static int
count_alert_dialogs (GtkWidget *root, AdwDialog **last)
{
  int n = 0;

  for (GtkWidget *child = gtk_widget_get_first_child (root); child; child = gtk_widget_get_next_sibling (child))
    {
      if (ADW_IS_ALERT_DIALOG (child))
        {
          *last = ADW_DIALOG (child);
          n++;
        }
      n += count_alert_dialogs (child, last);
    }
  return n;
}

/* Waits (at most 3 s) until win shows exactly n alert dialogs; closing ones linger for their animation. */
static void
wait_for_dialogs (EaselWindow *win, int n)
{
  gint64 deadline = g_get_monotonic_time () + 3 * G_USEC_PER_SEC;
  AdwDialog *ignored = NULL;

  while (count_alert_dialogs (GTK_WIDGET (win), &ignored) != n && g_get_monotonic_time () < deadline)
    g_main_context_iteration (NULL, FALSE);
}

static void
as33_one_quit_prompt (void)
{
  EaselWindow *win = start_window ();
  EaselCanvas *canvas = easel_window_get_canvas (win);
  AdwDialog *prompt;

  /* Unmodified: closes at once (REQ-0001-34). */
  wait_for_layout (win);
  gtk_window_close (GTK_WINDOW (win));
  g_assert_null (gtk_application_get_windows (app));

  win = start_window ();
  canvas = easel_window_get_canvas (win);
  /* gtk_window_close () ignores a window that is not realized yet. */
  wait_for_layout (win);
  g_action_group_activate_action (G_ACTION_GROUP (win), "tool", g_variant_new_string ("pencil"));
  easel_canvas_press (canvas, 1, 3.5, 3.5);
  easel_canvas_release (canvas, 1);

  /* Modified: the prompt (REQ-0001-35). */
  gtk_window_close (GTK_WINDOW (win));
  wait_for_layout (win);
  g_assert_cmpuint (g_list_length (gtk_application_get_windows (app)), ==, 1);
  wait_for_dialogs (win, 1);
  g_assert_cmpint (count_alert_dialogs (GTK_WIDGET (win), &prompt), ==, 1);
  g_assert_cmpstr (adw_alert_dialog_get_heading (ADW_ALERT_DIALOG (prompt)), ==, "Save Changes?");
  g_assert_cmpstr (adw_alert_dialog_get_default_response (ADW_ALERT_DIALOG (prompt)), ==, "save");
  g_assert_cmpstr (adw_alert_dialog_get_close_response (ADW_ALERT_DIALOG (prompt)), ==, "cancel");

  /* Esc or Cancel (the close response) keeps the window and the picture (REQ-0001-37). */
  adw_dialog_close (prompt);
  wait_for_layout (win);
  g_assert_cmpuint (g_list_length (gtk_application_get_windows (app)), ==, 1);
  g_assert_true (easel_window_get_modified (win));

  /* Quit again while the prompt is open: no second prompt (REQ-0001-38). libadwaita closes the open
     dialog as its close response (Cancel), so the window stays, still modified. */
  gtk_window_close (GTK_WINDOW (win));
  wait_for_layout (win);
  gtk_window_close (GTK_WINDOW (win));
  wait_for_layout (win);
  wait_for_dialogs (win, 0);
  g_assert_cmpint (count_alert_dialogs (GTK_WIDGET (win), &prompt), ==, 0);
  g_assert_cmpuint (g_list_length (gtk_application_get_windows (app)), ==, 1);
  g_assert_true (easel_window_get_modified (win));
  wait_for_layout (win);

  /* Discard closes without saving. */
  gtk_window_close (GTK_WINDOW (win));
  wait_for_layout (win);
  wait_for_dialogs (win, 1);
  g_assert_cmpint (count_alert_dialogs (GTK_WIDGET (win), &prompt), ==, 1);
  g_signal_emit_by_name (prompt, "response", "discard");
  g_assert_null (gtk_application_get_windows (app));
}

static GtkButton *
find_button (GtkWidget *root, const char *label)
{
  for (GtkWidget *child = gtk_widget_get_first_child (root); child; child = gtk_widget_get_next_sibling (child))
    {
      GtkButton *found;

      if (GTK_IS_BUTTON (child) && g_strcmp0 (gtk_button_get_label (GTK_BUTTON (child)), label) == 0)
        return GTK_BUTTON (child);
      found = find_button (child, label);
      if (found != NULL)
        return found;
    }
  return NULL;
}

/* Every toplevel except win: the in-process file chooser, when one is open. */
static GtkWindow *
other_toplevel (EaselWindow *win)
{
  GListModel *tops = gtk_window_get_toplevels ();

  for (guint i = 0; i < g_list_model_get_n_items (tops); i++)
    {
      g_autoptr (GtkWindow) w = g_list_model_get_item (tops, i);

      if (w != GTK_WINDOW (win) && gtk_widget_get_visible (GTK_WIDGET (w)))
        return w;
    }
  return NULL;
}

static void
as38_quit_save_while_dialog_open (void)
{
  EaselWindow *win = start_window ();
  EaselCanvas *canvas = easel_window_get_canvas (win);
  AdwDialog *prompt = NULL;
  GtkWindow *chooser;

  wait_for_layout (win);
  g_action_group_activate_action (G_ACTION_GROUP (win), "tool", g_variant_new_string ("pencil"));
  easel_canvas_press (canvas, 1, 3.5, 3.5);
  easel_canvas_release (canvas, 1);

  /* The save dialog is open... */
  g_action_group_activate_action (G_ACTION_GROUP (win), "save", NULL);
  wait_for_layout (win);
  chooser = other_toplevel (win);
  g_assert_nonnull (chooser);

  /* ...and the user closes the window and clicks Save in the prompt. */
  gtk_window_close (GTK_WINDOW (win));
  wait_for_layout (win);
  wait_for_dialogs (win, 1);
  g_assert_cmpint (count_alert_dialogs (GTK_WIDGET (win), &prompt), ==, 1);
  g_signal_emit_by_name (find_button (GTK_WIDGET (prompt), "_Save"), "clicked");
  wait_for_layout (win);

  /* Treated as Cancel: still open and modified, and a later quit asks again (REQ-0001-43). */
  g_assert_cmpuint (g_list_length (gtk_application_get_windows (app)), >=, 1);
  g_assert_true (easel_window_get_modified (win));
  wait_for_dialogs (win, 0);
  g_assert_cmpint (count_alert_dialogs (GTK_WIDGET (win), &prompt), ==, 0);
  gtk_window_close (GTK_WINDOW (win));
  wait_for_layout (win);
  wait_for_dialogs (win, 1);
  g_assert_cmpint (count_alert_dialogs (GTK_WIDGET (win), &prompt), ==, 1);

  adw_dialog_close (prompt);
  wait_for_layout (win);
  gtk_window_destroy (chooser);
  wait_for_layout (win);
  gtk_window_destroy (GTK_WINDOW (win));
}

/* ---- 0002: undo and redo ---- */

static GtkButton *
find_action_button (GtkWidget *root, const char *action)
{
  for (GtkWidget *child = gtk_widget_get_first_child (root); child; child = gtk_widget_get_next_sibling (child))
    {
      GtkButton *found;

      if (GTK_IS_BUTTON (child) && g_strcmp0 (gtk_actionable_get_action_name (GTK_ACTIONABLE (child)), action) == 0)
        return GTK_BUTTON (child);
      found = find_action_button (child, action);
      if (found != NULL)
        return found;
    }
  return NULL;
}

static gboolean
action_enabled (EaselWindow *win, const char *name)
{
  return g_action_group_get_action_enabled (G_ACTION_GROUP (win), name);
}

static void
use_pencil (EaselWindow *win)
{
  g_action_group_activate_action (G_ACTION_GROUP (win), "tool", g_variant_new_string ("pencil"));
}

/* One pencil stroke, as the pointer would make it. */
static void
stroke (EaselWindow *win, double x0, double y0, double x1, double y1)
{
  EaselCanvas *canvas = easel_window_get_canvas (win);

  easel_canvas_press (canvas, 1, x0, y0);
  easel_canvas_motion (canvas, x1, y1);
  easel_canvas_release (canvas, 1);
}

static gboolean
has_accel (const char *action, guint keyval, GdkModifierType mods)
{
  g_auto (GStrv) accels = gtk_application_get_accels_for_action (app, action);

  for (int i = 0; accels != NULL && accels[i] != NULL; i++)
    {
      guint key;
      GdkModifierType m;

      if (gtk_accelerator_parse (accels[i], &key, &m) && key == keyval && m == mods)
        return TRUE;
    }
  return FALSE;
}

#define A_STROKE(win) stroke ((win), 3.5, 3.5, 9.5, 3.5)
#define B_STROKE(win) stroke ((win), 3.5, 8.5, 9.5, 8.5)

static void
as6_initial_sensitivity (void)
{
  EaselWindow *win = start_window ();
  GtkButton *undo = find_action_button (GTK_WIDGET (win), "win.undo");
  GtkButton *redo = find_action_button (GTK_WIDGET (win), "win.redo");

  g_assert_nonnull (undo);
  g_assert_nonnull (redo);
  g_assert_false (action_enabled (win, "undo"));
  g_assert_false (action_enabled (win, "redo"));
  g_assert_false (gtk_widget_is_sensitive (GTK_WIDGET (undo)));
  g_assert_false (gtk_widget_is_sensitive (GTK_WIDGET (redo)));

  use_pencil (win);
  A_STROKE (win);
  g_assert_true (action_enabled (win, "undo"));
  g_assert_false (action_enabled (win, "redo"));
  g_assert_true (gtk_widget_is_sensitive (GTK_WIDGET (undo)));
  g_assert_false (gtk_widget_is_sensitive (GTK_WIDGET (redo)));

  /* Nothing to undo: the action does nothing (REQ-0002-06). */
  g_action_group_activate_action (G_ACTION_GROUP (win), "undo", NULL);
  g_action_group_activate_action (G_ACTION_GROUP (win), "undo", NULL);
  g_action_group_activate_action (G_ACTION_GROUP (win), "undo", NULL);
  g_assert_false (action_enabled (win, "undo"));
  g_assert_true (action_enabled (win, "redo"));
  close_windows ();
}

static void
as1_undo_shortcuts (void)
{
  EaselWindow *win = start_window ();
  const EaselImage *img = easel_window_get_picture (win);
  uint32_t bg = easel_image_get (img, 0, 0);

  g_assert_true (has_accel ("win.undo", GDK_KEY_z, GDK_CONTROL_MASK));
  g_assert_true (has_accel ("win.redo", GDK_KEY_y, GDK_CONTROL_MASK));
  g_assert_true (has_accel ("win.redo", GDK_KEY_z, GDK_CONTROL_MASK | GDK_SHIFT_MASK));

  use_pencil (win);
  A_STROKE (win);
  B_STROKE (win);

  g_action_group_activate_action (G_ACTION_GROUP (win), "undo", NULL);
  g_assert_cmphex (easel_image_get (img, 5, 3), ==, 0xFF000000u);
  g_assert_cmphex (easel_image_get (img, 5, 8), ==, bg);
  g_action_group_activate_action (G_ACTION_GROUP (win), "undo", NULL);
  g_assert_cmphex (easel_image_get (img, 5, 3), ==, bg);

  g_action_group_activate_action (G_ACTION_GROUP (win), "redo", NULL);
  g_assert_cmphex (easel_image_get (img, 5, 3), ==, 0xFF000000u);
  g_assert_cmphex (easel_image_get (img, 5, 8), ==, bg);
  g_action_group_activate_action (G_ACTION_GROUP (win), "redo", NULL);
  g_assert_cmphex (easel_image_get (img, 5, 8), ==, 0xFF000000u);
  close_windows ();
}

static void
as3_header_buttons (void)
{
  EaselWindow *win = start_window ();
  GtkButton *undo = find_action_button (GTK_WIDGET (win), "win.undo");
  GtkButton *redo = find_action_button (GTK_WIDGET (win), "win.redo");

  use_pencil (win);
  A_STROKE (win);
  g_assert_cmphex (easel_image_get (easel_window_get_picture (win), 5, 3), ==, 0xFF000000u);

  g_signal_emit_by_name (undo, "clicked");
  g_assert_cmphex (easel_image_get (easel_window_get_picture (win), 5, 3), !=, 0xFF000000u);
  g_signal_emit_by_name (redo, "clicked");
  g_assert_cmphex (easel_image_get (easel_window_get_picture (win), 5, 3), ==, 0xFF000000u);
  close_windows ();
}

static void
as4_redo_insensitive_after_edit (void)
{
  EaselWindow *win = start_window ();
  const EaselImage *img = easel_window_get_picture (win);
  uint32_t bg = easel_image_get (img, 0, 0);

  use_pencil (win);
  A_STROKE (win);
  B_STROKE (win);
  g_action_group_activate_action (G_ACTION_GROUP (win), "undo", NULL);
  g_assert_true (action_enabled (win, "redo"));

  stroke (win, 3.5, 13.5, 9.5, 13.5);
  g_assert_false (action_enabled (win, "redo"));
  g_assert_cmphex (easel_image_get (img, 5, 3), ==, 0xFF000000u);
  g_assert_cmphex (easel_image_get (img, 5, 8), ==, bg);
  g_assert_cmphex (easel_image_get (img, 5, 13), ==, 0xFF000000u);
  close_windows ();
}

static void
as5_non_edits_add_no_step (void)
{
  EaselWindow *win = start_window ();
  EaselCanvas *canvas = easel_window_get_canvas (win);
  GtkScrolledWindow *scroller = GTK_SCROLLED_WINDOW (gtk_widget_get_ancestor (GTK_WIDGET (canvas),
                                                                              GTK_TYPE_SCROLLED_WINDOW));
  g_autofree char *dir = g_dir_make_tmp ("easel-ui-XXXXXX", NULL);
  g_autofree char *path = g_build_filename (dir, "a.png", NULL);

  use_pencil (win);
  A_STROKE (win);

  /* Pan, scroll, zoom and back, switch tools. */
  g_action_group_activate_action (G_ACTION_GROUP (win), "tool", g_variant_new_string ("pan"));
  easel_canvas_press (canvas, 1, 20.5, 20.5);
  easel_canvas_motion (canvas, 40.5, 30.5);
  easel_canvas_release (canvas, 1);
  gtk_adjustment_set_value (gtk_scrolled_window_get_vadjustment (scroller), 10);
  easel_canvas_zoom_at (canvas, 1, 0, 0);
  easel_canvas_zoom_at (canvas, -1, 0, 0);
  g_assert_cmpint (easel_window_get_zoom (win), ==, 100);
  use_pencil (win);

  /* Black on black, a cancelled stroke, a save. */
  easel_canvas_press (canvas, 1, 5.5, 3.5);
  easel_canvas_release (canvas, 1);
  easel_canvas_press (canvas, 1, 50.5, 50.5);
  easel_canvas_motion (canvas, 60.5, 50.5);
  easel_canvas_press (canvas, 3, 60.5, 50.5);
  easel_canvas_release (canvas, 3);
  easel_canvas_release (canvas, 1);
  g_assert_true (easel_window_save_to (win, path));

  g_action_group_activate_action (G_ACTION_GROUP (win), "undo", NULL);
  g_assert_cmphex (easel_image_get (easel_window_get_picture (win), 5, 3), !=, 0xFF000000u);
  g_assert_false (action_enabled (win, "undo"));
  close_windows ();
}

static void
as10_modified_follows_history (void)
{
  EaselWindow *win = start_window ();
  g_autofree char *dir = g_dir_make_tmp ("easel-ui-XXXXXX", NULL);
  g_autofree char *path = g_build_filename (dir, "a.png", NULL);

  wait_for_layout (win);
  g_assert_true (easel_window_save_to (win, path));
  g_assert_cmpstr (gtk_window_get_title (GTK_WINDOW (win)), ==, "a.png – Easel");

  use_pencil (win);
  A_STROKE (win);
  g_assert_cmpstr (gtk_window_get_title (GTK_WINDOW (win)), ==, "• a.png – Easel");

  g_action_group_activate_action (G_ACTION_GROUP (win), "undo", NULL);
  g_assert_cmpstr (gtk_window_get_title (GTK_WINDOW (win)), ==, "a.png – Easel");
  g_assert_false (easel_window_get_modified (win));

  g_action_group_activate_action (G_ACTION_GROUP (win), "redo", NULL);
  g_assert_cmpstr (gtk_window_get_title (GTK_WINDOW (win)), ==, "• a.png – Easel");
  g_assert_true (easel_window_get_modified (win));

  /* Back at the saved state, closing needs no prompt (REQ-0002-13). */
  g_action_group_activate_action (G_ACTION_GROUP (win), "undo", NULL);
  gtk_window_close (GTK_WINDOW (win));
  {
    /* The window is gone after closing, so wait on the main loop alone. */
    gint64 until = g_get_monotonic_time () + G_USEC_PER_SEC / 4;

    while (g_get_monotonic_time () < until)
      g_main_context_iteration (NULL, FALSE);
  }
  g_assert_null (gtk_application_get_windows (app));
}

static void
as11_undo_to_start_unmodified (void)
{
  EaselWindow *win = start_window ();

  use_pencil (win);
  A_STROKE (win);
  g_assert_cmpstr (gtk_window_get_title (GTK_WINDOW (win)), ==, "• Untitled – Easel");
  g_action_group_activate_action (G_ACTION_GROUP (win), "undo", NULL);
  g_assert_cmpstr (gtk_window_get_title (GTK_WINDOW (win)), ==, "Untitled – Easel");
  g_assert_false (easel_window_get_modified (win));
  close_windows ();
}

static void
as8_undo_during_stroke (void)
{
  EaselWindow *win = start_window ();
  EaselCanvas *canvas = easel_window_get_canvas (win);
  const EaselImage *img = easel_window_get_picture (win);
  uint32_t bg = easel_image_get (img, 0, 0);

  use_pencil (win);
  A_STROKE (win);

  /* Undo while the button is held cancels the stroke: nothing is added, there is nothing to redo (REQ-0002-09). */
  easel_canvas_press (canvas, 1, 3.5, 8.5);
  easel_canvas_motion (canvas, 9.5, 8.5);
  g_assert_cmphex (easel_image_get (img, 5, 8), ==, 0xFF000000u);
  g_action_group_activate_action (G_ACTION_GROUP (win), "undo", NULL);
  g_assert_cmphex (easel_image_get (img, 5, 8), ==, bg);
  g_assert_cmphex (easel_image_get (img, 5, 3), ==, 0xFF000000u);
  g_assert_false (action_enabled (win, "redo"));
  g_assert_false (easel_canvas_is_stroking (canvas));

  /* Further undo and redo are ignored until the button is released. */
  g_action_group_activate_action (G_ACTION_GROUP (win), "undo", NULL);
  g_action_group_activate_action (G_ACTION_GROUP (win), "redo", NULL);
  g_assert_cmphex (easel_image_get (img, 5, 3), ==, 0xFF000000u);
  easel_canvas_motion (canvas, 9.5, 12.5);
  g_assert_cmphex (easel_image_get (img, 9, 12), ==, bg);

  easel_canvas_release (canvas, 1);
  g_assert_cmphex (easel_image_get (img, 5, 8), ==, bg);
  g_action_group_activate_action (G_ACTION_GROUP (win), "undo", NULL);
  g_assert_cmphex (easel_image_get (img, 5, 3), ==, bg);
  close_windows ();
}

/* REQ-0002-09: if the release of a cancelled stroke's button never arrives, losing focus ends the wait. */
static void
as8_focus_loss_unblocks_undo (void)
{
  EaselWindow *win = start_window ();
  EaselCanvas *canvas = easel_window_get_canvas (win);
  const EaselImage *img = easel_window_get_picture (win);
  uint32_t bg = easel_image_get (img, 0, 0);

  use_pencil (win);
  A_STROKE (win);
  easel_canvas_press (canvas, 1, 3.5, 8.5);
  easel_canvas_motion (canvas, 9.5, 8.5);
  g_action_group_activate_action (G_ACTION_GROUP (win), "undo", NULL);
  g_action_group_activate_action (G_ACTION_GROUP (win), "undo", NULL);
  g_assert_cmphex (easel_image_get (img, 5, 3), ==, 0xFF000000u);

  easel_canvas_reset_pointer (canvas);
  g_action_group_activate_action (G_ACTION_GROUP (win), "undo", NULL);
  g_assert_cmphex (easel_image_get (img, 5, 3), ==, bg);

  /* A late release of that button changes nothing. */
  easel_canvas_release (canvas, 1);
  g_assert_false (action_enabled (win, "undo"));
  close_windows ();
}

static void
as9_shortcuts_focus_and_dialogs (void)
{
  EaselWindow *win = start_window ();
  const EaselImage *img = easel_window_get_picture (win);
  GtkToggleButton *pencil = find_tool_button (GTK_WIDGET (win), "pencil");
  AdwDialog *prompt = NULL;
  uint32_t bg = easel_image_get (img, 0, 0);

  wait_for_layout (win);
  use_pencil (win);
  A_STROKE (win);

  /* Keyboard focus on a toolbox button: the shortcut still reaches the window's action (REQ-0002-11). */
  gtk_widget_grab_focus (GTK_WIDGET (pencil));
  g_assert_true (gtk_widget_activate_action (GTK_WIDGET (pencil), "win.undo", NULL));
  g_assert_cmphex (easel_image_get (img, 5, 3), ==, bg);
  g_assert_true (gtk_widget_activate_action (GTK_WIDGET (pencil), "win.redo", NULL));
  g_assert_cmphex (easel_image_get (img, 5, 3), ==, 0xFF000000u);

  /* With the quit prompt open, undo and redo do nothing. */
  gtk_window_close (GTK_WINDOW (win));
  wait_for_dialogs (win, 1);
  g_assert_cmpint (count_alert_dialogs (GTK_WIDGET (win), &prompt), ==, 1);
  gtk_widget_activate_action (GTK_WIDGET (pencil), "win.undo", NULL);
  g_assert_cmphex (easel_image_get (img, 5, 3), ==, 0xFF000000u);
  g_assert_true (action_enabled (win, "undo"));

  adw_dialog_close (prompt);
  wait_for_dialogs (win, 0);
  gtk_widget_activate_action (GTK_WIDGET (pencil), "win.undo", NULL);
  g_assert_cmphex (easel_image_get (img, 5, 3), ==, bg);
  close_windows ();
}

/* A stroke of exactly 50,000 changed pixels on a 10000x100 picture: five rows of 10,000, zigzagging. Alternating the
   primary and secondary button toggles the same pixels between two colors, so every stroke changes all of them. */
static void
stroke_50000_pixels (EaselWindow *win, guint button)
{
  EaselCanvas *canvas = easel_window_get_canvas (win);

  easel_canvas_press (canvas, button, 0.5, 0.5);
  easel_canvas_motion (canvas, 9999.5, 0.5);
  for (int r = 1; r < 5; r++)
    {
      easel_canvas_motion (canvas, r % 2 ? 9999.5 : 0.5, r + 0.5);
      easel_canvas_motion (canvas, r % 2 ? 0.5 : 9999.5, r + 0.5);
    }
  easel_canvas_release (canvas, button);
}

/* REQ-0002-18: the budget from the settings file is the one the window's history uses. At 16 MB and 600,064 bytes per
   step, exactly 27 strokes stay undoable; a budget ignored or misread would keep 1 or all 40. */
static void
as18_budget_reaches_history (void)
{
  EaselWindow *win;
  int undone = 0;

  write_settings ("[new-picture]\nwidth=10000\nheight=100\n\n[history]\nmemory=16\n");
  win = start_window ();
  use_pencil (win);
  for (int k = 0; k < 40; k++)
    stroke_50000_pixels (win, k % 2 == 0 ? 1 : 3);

  while (action_enabled (win, "undo"))
    {
      g_action_group_activate_action (G_ACTION_GROUP (win), "undo", NULL);
      undone++;
    }
  g_assert_cmpint (undone, ==, 27);
  close_windows ();
  write_settings ("[new-picture]\nwidth=300\nheight=200\nbackground=#FFFF00\n");
}

static void
as14_failed_save_keeps_saved_state (void)
{
  g_autofree char *dir = g_dir_make_tmp ("easel-ui-XXXXXX", NULL);
  g_autofree char *path = g_build_filename (dir, "a.png", NULL);
  g_autofree char *ro_dir = g_build_filename (dir, "ro", NULL);
  g_autofree char *ro_path = g_build_filename (ro_dir, "b.png", NULL);
  EaselWindow *win = start_window ();
  AdwDialog *error = NULL;

  if (geteuid () == 0)
    {
      g_test_skip ("root ignores folder permissions");
      close_windows ();
      return;
    }
  g_assert_cmpint (g_mkdir (ro_dir, 0500), ==, 0);
  wait_for_layout (win);

  /* Saved state S, then two strokes, then a save that fails. */
  g_assert_true (easel_window_save_to (win, path));
  use_pencil (win);
  A_STROKE (win);
  B_STROKE (win);
  g_assert_cmpstr (gtk_window_get_title (GTK_WINDOW (win)), ==, "• a.png – Easel");
  g_assert_false (easel_window_save_to (win, ro_path));
  wait_for_dialogs (win, 1);
  g_assert_cmpint (count_alert_dialogs (GTK_WIDGET (win), &error), ==, 1);
  adw_dialog_close (error);
  wait_for_dialogs (win, 0);
  g_assert_cmpstr (gtk_window_get_title (GTK_WINDOW (win)), ==, "• a.png – Easel");

  /* The saved state did not move to the failed save: S is still two undos back. */
  g_action_group_activate_action (G_ACTION_GROUP (win), "undo", NULL);
  g_assert_cmpstr (gtk_window_get_title (GTK_WINDOW (win)), ==, "• a.png – Easel");
  g_action_group_activate_action (G_ACTION_GROUP (win), "undo", NULL);
  g_assert_cmpstr (gtk_window_get_title (GTK_WINDOW (win)), ==, "a.png – Easel");
  g_action_group_activate_action (G_ACTION_GROUP (win), "redo", NULL);
  g_action_group_activate_action (G_ACTION_GROUP (win), "redo", NULL);
  g_assert_cmpstr (gtk_window_get_title (GTK_WINDOW (win)), ==, "• a.png – Easel");
  g_assert_cmpint (g_chmod (ro_dir, 0700), ==, 0);
  close_windows ();
}

static void
as15_save_during_stroke (void)
{
  g_autofree char *dir = g_dir_make_tmp ("easel-ui-XXXXXX", NULL);
  g_autofree char *path = g_build_filename (dir, "a.png", NULL);
  EaselWindow *win = start_window ();
  EaselCanvas *canvas = easel_window_get_canvas (win);
  g_autoptr (GError) error = NULL;
  g_autoptr (GdkPixbuf) saved = NULL;

  wait_for_layout (win);
  g_assert_true (easel_window_save_to (win, path));
  use_pencil (win);

  easel_canvas_press (canvas, 1, 3.5, 3.5);
  easel_canvas_motion (canvas, 9.5, 3.5);
  g_action_group_activate_action (G_ACTION_GROUP (win), "save", NULL);

  /* The stroke ended first, so it is in the file and the saved state is after it. */
  g_assert_false (easel_canvas_is_stroking (canvas));
  g_assert_cmpstr (gtk_window_get_title (GTK_WINDOW (win)), ==, "a.png – Easel");
  saved = gdk_pixbuf_new_from_file (path, &error);
  g_assert_no_error (error);
  g_assert_cmpint (gdk_pixbuf_get_pixels (saved)[3 * gdk_pixbuf_get_rowstride (saved) + 5 * 4], ==, 0);
  easel_canvas_release (canvas, 1);

  g_action_group_activate_action (G_ACTION_GROUP (win), "undo", NULL);
  g_assert_cmpstr (gtk_window_get_title (GTK_WINDOW (win)), ==, "• a.png – Easel");
  close_windows ();
}

/* ---- 0003: colors and palette ---- */

static graphene_rect_t
bounds_in (GtkWidget *w, GtkWidget *target)
{
  graphene_rect_t r;

  g_assert_true (gtk_widget_compute_bounds (w, target, &r));
  return r;
}

static void
as1_palette_bar (void)
{
  EaselWindow *win;
  EaselPaletteBar *bar;
  GtkWidget *scroller;
  graphene_rect_t bar_r, canvas_r;
  EaselPalette def;

  remove_palette_file ();
  win = start_window ();
  bar = easel_window_get_palette_bar (win);
  g_assert_nonnull (bar);
  wait_for_layout (win);
  scroller = gtk_widget_get_ancestor (GTK_WIDGET (easel_window_get_canvas (win)), GTK_TYPE_SCROLLED_WINDOW);
  bar_r = bounds_in (GTK_WIDGET (bar), GTK_WIDGET (win));
  canvas_r = bounds_in (scroller, GTK_WIDGET (win));

  /* Below the canvas, across the window's full width (REQ-0003-01). */
  g_assert_cmpfloat (bar_r.origin.y, >=, canvas_r.origin.y + canvas_r.size.height - 0.5);
  g_assert_cmpfloat (bar_r.origin.x, <=, 0.5);
  g_assert_cmpfloat (bar_r.size.width, >=, gtk_widget_get_width (GTK_WIDGET (win)) - 0.5);

  /* The default colors (REQ-0003-04), and black in front of the picture's background (REQ-0003-02). */
  easel_palette_defaults (&def);
  for (int n = 1; n <= EASEL_PALETTE_SIZE; n++)
    g_assert_cmphex (easel_swatch_get_color (easel_palette_bar_get_swatch (bar, n)), ==, def.colors[n - 1]);
  g_assert_cmphex (easel_palette_bar_get_foreground (bar), ==, 0xFF000000u);
  g_assert_cmphex (easel_palette_bar_get_background (bar), ==, easel_window_get_background (win));
  close_windows ();
}

/* Swatches are 20x20 logical pixels, 2 apart, in two rows of 14, numbered row by row (REQ-0003-01). */
static void
as1_swatch_layout (void)
{
  EaselWindow *win = start_window ();
  EaselPaletteBar *bar = easel_window_get_palette_bar (win);
  graphene_rect_t first, r[EASEL_PALETTE_SIZE + 1];

  wait_for_layout (win);
  for (int n = 1; n <= EASEL_PALETTE_SIZE; n++)
    {
      r[n] = bounds_in (GTK_WIDGET (easel_palette_bar_get_swatch (bar, n)), GTK_WIDGET (bar));
      g_assert_cmpfloat (r[n].size.width, ==, 20);
      g_assert_cmpfloat (r[n].size.height, ==, 20);
    }
  first = r[1];
  for (int n = 1; n <= EASEL_PALETTE_SIZE; n++)
    {
      int row = n <= 14 ? 1 : 2, col = n <= 14 ? n : n - 14;

      /* Same row, same top; next column, further right; second row below the first, columns aligned. */
      g_assert_cmpfloat (r[n].origin.y, ==, row == 1 ? first.origin.y : r[15].origin.y);
      if (col > 1)
        g_assert_cmpfloat (r[n].origin.x, ==, r[n - 1].origin.x + 22);
      if (row == 2)
        {
          g_assert_cmpfloat (r[n].origin.x, ==, r[n - 14].origin.x);
          g_assert_cmpfloat (r[n].origin.y, ==, r[n - 14].origin.y + 22);
        }
    }
  /* The indicator is left of swatch 1. */
  g_assert_cmpfloat (bounds_in (GTK_WIDGET (easel_palette_bar_get_indicator (bar)), GTK_WIDGET (bar)).origin.x
                     + 1, <, first.origin.x);
  close_windows ();
}

/* REQ-0003-18, REQ-0003-19: the bar's exact dimensions, and the swap button. */
static void
as18_bar_geometry_and_swap_button (void)
{
  EaselWindow *win = start_window ();
  EaselPaletteBar *bar = easel_window_get_palette_bar (win);
  GtkWidget *indicator = GTK_WIDGET (easel_palette_bar_get_indicator (bar));
  GtkWidget *swap = GTK_WIDGET (easel_palette_bar_get_swap_button (bar));
  GtkWidget *grid = gtk_widget_get_parent (GTK_WIDGET (easel_palette_bar_get_swatch (bar, 1)));
  GtkWidget *separator = gtk_widget_get_first_child (GTK_WIDGET (bar));
  graphene_rect_t ind, btn, grd, sep, front, back;

  wait_for_layout (win);
  /* A separator line above the bar's contents. */
  g_assert_true (GTK_IS_SEPARATOR (separator));
  sep = bounds_in (separator, GTK_WIDGET (bar));

  ind = bounds_in (indicator, GTK_WIDGET (bar));
  btn = bounds_in (swap, GTK_WIDGET (bar));
  grd = bounds_in (grid, GTK_WIDGET (bar));
  g_assert_cmpfloat (ind.size.width, ==, 46);
  g_assert_cmpfloat (ind.size.height, ==, 42);
  /* From the left: 8 margin, indicator, 8, swap button, 8, grid; at least 6 below the separator. */
  g_assert_cmpfloat (ind.origin.x, ==, 8);
  g_assert_cmpfloat (btn.origin.x, ==, ind.origin.x + ind.size.width + 8);
  g_assert_cmpfloat (grd.origin.x, ==, btn.origin.x + btn.size.width + 8);
  g_assert_cmpfloat (MIN (ind.origin.y, grd.origin.y), >=, sep.origin.y + sep.size.height + 6);
  g_assert_cmpfloat (gtk_widget_get_height (GTK_WIDGET (bar)),
                     >=, MAX (ind.origin.y + ind.size.height, grd.origin.y + grd.size.height) + 6);

  /* The two squares, 28x28 at (2, 2) in front and (16, 12) behind. */
  easel_color_indicator_get_square (EASEL_COLOR_INDICATOR (indicator), TRUE, &front);
  easel_color_indicator_get_square (EASEL_COLOR_INDICATOR (indicator), FALSE, &back);
  g_assert_true (graphene_rect_equal (&front, &GRAPHENE_RECT_INIT (2, 2, 28, 28)));
  g_assert_true (graphene_rect_equal (&back, &GRAPHENE_RECT_INIT (16, 12, 28, 28)));

  /* The swap button: its symbol and tooltip, and it swaps. */
  g_assert_cmpstr (gtk_button_get_label (GTK_BUTTON (swap)), ==, "⇄");
  g_assert_cmpstr (gtk_widget_get_tooltip_text (swap), ==, "Swap foreground and background (Ctrl+Shift+X)");
  easel_palette_bar_press (bar, 17, 1, 1);
  easel_palette_bar_press (bar, 21, 3, 1);
  g_signal_emit_by_name (swap, "clicked");
  g_assert_cmphex (easel_window_get_foreground (win), ==, 0xFF0000FFu);
  g_assert_cmphex (easel_window_get_background (win), ==, 0xFFFF0000u);

  /* Not while the chooser is open. */
  easel_palette_bar_press_indicator (bar, TRUE);
  g_assert_true (easel_window_is_choosing_color (win));
  g_signal_emit_by_name (swap, "clicked");
  g_assert_cmphex (easel_window_get_foreground (win), ==, 0xFF0000FFu);
  easel_window_finish_color_choice (win, NULL);
  close_windows ();
}

static void
as3_click_picks_colors (void)
{
  EaselWindow *win = start_window ();
  EaselPaletteBar *bar = easel_window_get_palette_bar (win);
  uint32_t bg;

  wait_for_layout (win);
  easel_palette_bar_press (bar, 17, 1, 1);
  g_assert_cmphex (easel_window_get_foreground (win), ==, 0xFFFF0000u);
  g_assert_cmphex (easel_palette_bar_get_foreground (bar), ==, 0xFFFF0000u);
  g_assert_true (gtk_widget_is_focus (GTK_WIDGET (easel_palette_bar_get_swatch (bar, 17))));

  easel_palette_bar_press (bar, 21, 3, 1);
  g_assert_cmphex (easel_window_get_background (win), ==, 0xFF0000FFu);
  g_assert_cmphex (easel_palette_bar_get_background (bar), ==, 0xFF0000FFu);
  g_assert_true (gtk_widget_is_focus (GTK_WIDGET (easel_palette_bar_get_swatch (bar, 21))));

  /* The middle button does nothing. */
  bg = easel_window_get_background (win);
  easel_palette_bar_press (bar, 1, 2, 1);
  g_assert_cmphex (easel_window_get_foreground (win), ==, 0xFFFF0000u);
  g_assert_cmphex (easel_window_get_background (win), ==, bg);
  close_windows ();
}

static void
as4_swap_colors (void)
{
  EaselWindow *win = start_window ();
  EaselPaletteBar *bar = easel_window_get_palette_bar (win);
  AdwDialog *prompt = NULL;

  wait_for_layout (win);
  g_assert_true (has_accel ("win.swap-colors", GDK_KEY_x, GDK_CONTROL_MASK | GDK_SHIFT_MASK));
  easel_palette_bar_press (bar, 17, 1, 1);
  easel_palette_bar_press (bar, 21, 3, 1);
  g_action_group_activate_action (G_ACTION_GROUP (win), "swap-colors", NULL);
  g_assert_cmphex (easel_window_get_foreground (win), ==, 0xFF0000FFu);
  g_assert_cmphex (easel_window_get_background (win), ==, 0xFFFF0000u);
  g_assert_cmphex (easel_palette_bar_get_foreground (bar), ==, 0xFF0000FFu);

  /* Not while the quit prompt is open (REQ-0003-07). */
  use_pencil (win);
  A_STROKE (win);
  gtk_window_close (GTK_WINDOW (win));
  wait_for_dialogs (win, 1);
  g_action_group_activate_action (G_ACTION_GROUP (win), "swap-colors", NULL);
  g_assert_cmphex (easel_window_get_foreground (win), ==, 0xFF0000FFu);
  g_assert_cmpint (count_alert_dialogs (GTK_WIDGET (win), &prompt), ==, 1);
  adw_dialog_close (prompt);
  wait_for_dialogs (win, 0);
  close_windows ();
}

static void
as5_strokes_use_current_colors (void)
{
  EaselWindow *win = start_window ();
  EaselPaletteBar *bar = easel_window_get_palette_bar (win);
  EaselCanvas *canvas = easel_window_get_canvas (win);
  const EaselImage *img = easel_window_get_picture (win);

  easel_palette_bar_press (bar, 17, 1, 1);
  easel_palette_bar_press (bar, 21, 3, 1);
  use_pencil (win);

  stroke (win, 3.5, 3.5, 9.5, 3.5);
  easel_canvas_press (canvas, 3, 3.5, 8.5);
  easel_canvas_motion (canvas, 9.5, 8.5);
  easel_canvas_release (canvas, 3);
  g_assert_cmphex (easel_image_get (img, 5, 3), ==, 0xFFFF0000u);
  g_assert_cmphex (easel_image_get (img, 5, 8), ==, 0xFF0000FFu);

  /* A swap during a stroke changes the next stroke only (REQ-0003-08). */
  easel_canvas_press (canvas, 1, 3.5, 13.5);
  easel_canvas_motion (canvas, 6.5, 13.5);
  g_action_group_activate_action (G_ACTION_GROUP (win), "swap-colors", NULL);
  easel_canvas_motion (canvas, 9.5, 13.5);
  easel_canvas_release (canvas, 1);
  for (int x = 3; x <= 9; x++)
    g_assert_cmphex (easel_image_get (img, x, 13), ==, 0xFFFF0000u);
  stroke (win, 3.5, 18.5, 9.5, 18.5);
  g_assert_cmphex (easel_image_get (img, 5, 18), ==, 0xFF0000FFu);
  close_windows ();
}

static void
as6_transparent_background_swap (void)
{
  EaselWindow *win;
  const EaselImage *img;

  write_settings ("[new-picture]\nwidth=300\nheight=200\nbackground=transparent\n");
  win = start_window ();
  img = easel_window_get_picture (win);
  g_assert_cmphex (easel_palette_bar_get_background (easel_window_get_palette_bar (win)), ==, 0x00000000u);

  /* Black first, so the transparent stroke has something to replace. */
  use_pencil (win);
  stroke (win, 3.5, 3.5, 9.5, 3.5);
  g_assert_cmphex (easel_image_get (img, 5, 3), ==, 0xFF000000u);
  g_action_group_activate_action (G_ACTION_GROUP (win), "swap-colors", NULL);
  g_assert_cmphex (easel_window_get_foreground (win), ==, 0x00000000u);
  stroke (win, 3.5, 3.5, 9.5, 3.5);
  for (int x = 3; x <= 9; x++)
    g_assert_cmphex (easel_image_get (img, x, 3), ==, 0x00000000u);
  close_windows ();
  write_settings ("[new-picture]\nwidth=300\nheight=200\nbackground=#FFFF00\n");
}

static void
as7_colors_are_not_edits (void)
{
  EaselWindow *win = start_window ();
  EaselPaletteBar *bar = easel_window_get_palette_bar (win);

  easel_palette_bar_press (bar, 17, 1, 1);
  easel_palette_bar_press (bar, 21, 3, 1);
  g_action_group_activate_action (G_ACTION_GROUP (win), "swap-colors", NULL);
  g_assert_cmpstr (gtk_window_get_title (GTK_WINDOW (win)), ==, "Untitled – Easel");
  g_assert_false (action_enabled (win, "undo"));
  close_windows ();
}

static GdkRGBA
rgba_of (uint32_t c)
{
  return (GdkRGBA) { ((c >> 16) & 0xFF) / 255.0f, ((c >> 8) & 0xFF) / 255.0f, (c & 0xFF) / 255.0f, (c >> 24) / 255.0f };
}

/* The user confirms color c in the open chooser (NULL: cancels). */
static void
choose (EaselWindow *win, const uint32_t *c)
{
  GdkRGBA rgba;

  g_assert_true (easel_window_is_choosing_color (win));
  if (c != NULL)
    rgba = rgba_of (*c);
  easel_window_finish_color_choice (win, c ? &rgba : NULL);
  g_assert_false (easel_window_is_choosing_color (win));
}

static void
as9_double_click_edits_swatch (void)
{
  EaselWindow *win = start_window ();
  EaselPaletteBar *bar = easel_window_get_palette_bar (win);
  const uint32_t c = 0xFF123456u;

  wait_for_layout (win);
  easel_palette_bar_press (bar, 17, 1, 1);
  easel_palette_bar_press (bar, 17, 1, 2);
  choose (win, &c);
  g_assert_cmphex (easel_swatch_get_color (easel_palette_bar_get_swatch (bar, 17)), ==, c);
  g_assert_cmphex (easel_window_get_foreground (win), ==, c);

  /* Cancel: the swatch keeps its color; the foreground is what the first click picked. */
  easel_palette_bar_press (bar, 1, 1, 1);
  easel_palette_bar_press (bar, 1, 1, 2);
  choose (win, NULL);
  g_assert_cmphex (easel_swatch_get_color (easel_palette_bar_get_swatch (bar, 1)), ==, 0xFF000000u);
  g_assert_cmphex (easel_window_get_foreground (win), ==, 0xFF000000u);

  /* Editing a swatch is not an edit of the picture (REQ-0003-09). */
  g_assert_cmpstr (gtk_window_get_title (GTK_WINDOW (win)), ==, "Untitled – Easel");
  g_assert_false (action_enabled (win, "undo"));
  /* Not even hidden in the history: after one stroke, exactly one undo is possible. */
  use_pencil (win);
  A_STROKE (win);
  g_action_group_activate_action (G_ACTION_GROUP (win), "undo", NULL);
  g_assert_false (action_enabled (win, "undo"));
  close_windows ();
  remove_palette_file ();
}

static void
as10_f2_edits_focused_swatch (void)
{
  EaselWindow *win = start_window ();
  EaselPaletteBar *bar = easel_window_get_palette_bar (win);
  EaselSwatch *swatch = easel_palette_bar_get_swatch (bar, 17);
  const uint32_t c = 0x8000FF00u;     /* #00FF0080 */

  wait_for_layout (win);
  easel_palette_bar_press (bar, 17, 1, 1);
  g_assert_true (gtk_widget_is_focus (GTK_WIDGET (swatch)));
  /* F2 is bound to this action on every swatch. */
  g_assert_true (gtk_widget_activate_action (GTK_WIDGET (swatch), "swatch.edit", NULL));
  choose (win, &c);
  g_assert_cmphex (easel_swatch_get_color (swatch), ==, c);
  close_windows ();
  remove_palette_file ();
}

static void
as11_click_sequences (void)
{
  EaselWindow *win = start_window ();
  EaselPaletteBar *bar = easel_window_get_palette_bar (win);

  /* Two quick clicks on two different swatches are two single clicks: each swatch has its own count. */
  easel_palette_bar_press (bar, 17, 1, 1);
  easel_palette_bar_press (bar, 18, 1, 1);
  g_assert_false (easel_window_is_choosing_color (win));
  g_assert_cmphex (easel_window_get_foreground (win), ==, 0xFFFFFF00u);

  /* A secondary double-click only sets the background. */
  easel_palette_bar_press (bar, 21, 3, 1);
  easel_palette_bar_press (bar, 21, 3, 2);
  g_assert_false (easel_window_is_choosing_color (win));
  g_assert_cmphex (easel_window_get_background (win), ==, 0xFF0000FFu);

  /* A third quick click is a single click again. */
  easel_palette_bar_press (bar, 19, 1, 1);
  easel_palette_bar_press (bar, 19, 1, 2);
  choose (win, NULL);
  easel_palette_bar_press (bar, 19, 1, 3);
  g_assert_false (easel_window_is_choosing_color (win));
  g_assert_cmphex (easel_window_get_foreground (win), ==, 0xFF00FF00u);
  close_windows ();
}

static void
as12_indicator_edits_color (void)
{
  EaselWindow *win;
  EaselPaletteBar *bar;
  const uint32_t fg = 0xFF123456u, bg = 0xFF00FF00u;
  EaselPalette def;

  remove_palette_file ();
  win = start_window ();
  bar = easel_window_get_palette_bar (win);
  easel_palette_bar_press_indicator (bar, TRUE);
  choose (win, &fg);
  g_assert_cmphex (easel_window_get_foreground (win), ==, fg);
  easel_palette_bar_press_indicator (bar, FALSE);
  choose (win, &bg);
  g_assert_cmphex (easel_window_get_background (win), ==, bg);
  g_assert_cmphex (easel_window_get_foreground (win), ==, fg);

  /* No swatch changed. */
  easel_palette_defaults (&def);
  for (int n = 1; n <= EASEL_PALETTE_SIZE; n++)
    g_assert_cmphex (easel_swatch_get_color (easel_palette_bar_get_swatch (bar, n)), ==, def.colors[n - 1]);

  /* Cancel changes nothing. */
  easel_palette_bar_press_indicator (bar, TRUE);
  choose (win, NULL);
  g_assert_cmphex (easel_window_get_foreground (win), ==, fg);
  close_windows ();
}

/* REQ-0003-14: while the chooser is open, undo, redo, swap, swatches and drawing do nothing. */
static void
as17_undo_ignored_while_chooser_open (void)
{
  EaselWindow *win = start_window ();
  EaselPaletteBar *bar = easel_window_get_palette_bar (win);
  EaselCanvas *canvas = easel_window_get_canvas (win);
  const EaselImage *img = easel_window_get_picture (win);
  uint32_t bg = easel_image_get (img, 0, 0);

  use_pencil (win);
  A_STROKE (win);
  easel_palette_bar_press (bar, 17, 1, 1);
  easel_palette_bar_press (bar, 17, 1, 2);
  g_assert_true (easel_window_is_choosing_color (win));

  g_action_group_activate_action (G_ACTION_GROUP (win), "undo", NULL);
  g_action_group_activate_action (G_ACTION_GROUP (win), "redo", NULL);
  g_assert_cmphex (easel_image_get (img, 5, 3), ==, 0xFF000000u);
  g_assert_true (action_enabled (win, "undo"));
  g_assert_false (action_enabled (win, "redo"));

  g_action_group_activate_action (G_ACTION_GROUP (win), "swap-colors", NULL);
  easel_palette_bar_press (bar, 21, 3, 1);
  g_assert_cmphex (easel_window_get_foreground (win), ==, 0xFFFF0000u);
  g_assert_cmphex (easel_window_get_background (win), !=, 0xFF0000FFu);
  easel_canvas_press (canvas, 1, 3.5, 8.5);
  easel_canvas_motion (canvas, 9.5, 8.5);
  easel_canvas_release (canvas, 1);
  g_assert_cmphex (easel_image_get (img, 5, 8), ==, bg);

  choose (win, NULL);
  g_action_group_activate_action (G_ACTION_GROUP (win), "undo", NULL);
  g_assert_cmphex (easel_image_get (img, 5, 3), ==, bg);
  close_windows ();
}

/* REQ-0003-15, REQ-0003-17: a confirmed edit is written at once, and the next window shows it. */
static void
as14_edit_is_remembered (void)
{
  g_autofree char *path = easel_palette_file_path ();
  g_autofree char *text = NULL;
  const uint32_t c = 0xFF123456u;
  EaselWindow *win;

  remove_palette_file ();
  win = start_window ();
  easel_palette_bar_press (easel_window_get_palette_bar (win), 3, 1, 1);
  easel_palette_bar_press (easel_window_get_palette_bar (win), 3, 1, 2);
  choose (win, &c);
  g_assert_true (g_file_get_contents (path, &text, NULL, NULL));
  g_assert_nonnull (strstr (text, "\ncolor3=#123456\n"));
  close_windows ();

  win = start_window ();
  g_assert_cmphex (easel_swatch_get_color (easel_palette_bar_get_swatch (easel_window_get_palette_bar (win), 3)), ==, c);
  close_windows ();
  remove_palette_file ();
}

/* REQ-0003-16: if the palette cannot be written, the swatch still changes for this run, silently. */
static void
as15_swatch_changes_when_write_fails (void)
{
  g_autofree char *path = easel_palette_file_path ();
  g_autofree char *dir = g_path_get_dirname (path);
  const uint32_t c = 0xFF123456u;
  AdwDialog *dialog = NULL;
  EaselWindow *win;

  if (geteuid () == 0)
    {
      g_test_skip ("root ignores folder permissions");
      return;
    }
  remove_palette_file ();
  win = start_window ();
  g_assert_cmpint (g_chmod (dir, 0500), ==, 0);
  easel_palette_bar_press (easel_window_get_palette_bar (win), 3, 1, 1);
  easel_palette_bar_press (easel_window_get_palette_bar (win), 3, 1, 2);
  choose (win, &c);
  g_assert_cmpint (g_chmod (dir, 0700), ==, 0);

  g_assert_cmphex (easel_swatch_get_color (easel_palette_bar_get_swatch (easel_window_get_palette_bar (win), 3)), ==, c);
  g_assert_false (g_file_test (path, G_FILE_TEST_EXISTS));
  wait_for_layout (win);
  g_assert_cmpint (count_alert_dialogs (GTK_WIDGET (win), &dialog), ==, 0);
  close_windows ();
}

/* The pixel (x, y) of the picture as its tile's texture holds it (opaque colors only: the download is premultiplied). */
static uint32_t
tile_pixel (EaselCanvas *canvas, int x, int y)
{
  GdkTexture *texture = easel_canvas_get_tile_texture (canvas, x / EASEL_CANVAS_TILE, y / EASEL_CANVAS_TILE);
  int w, h;
  g_autofree uint32_t *data = NULL;

  g_assert_nonnull (texture);
  w = gdk_texture_get_width (texture);
  h = gdk_texture_get_height (texture);
  data = g_malloc ((gsize) w * (gsize) h * 4);
  gdk_texture_download (texture, (guchar *) data, (gsize) w * 4);
  return data[(gsize) (y % EASEL_CANVAS_TILE) * (gsize) w + (gsize) (x % EASEL_CANVAS_TILE)];
}

/* 0004 T001: the canvas shows the picture's own memory in tiles, copies nothing, rebuilds only the tiles an update
   touches, and follows the pixels after strokes and after it is pointed at another picture. */
static void
texture_is_not_copied (void)
{
  EaselWindow *win;
  EaselCanvas *canvas;
  EaselImage *other = easel_image_new (20, 10, 0xFF112233u);
  const EaselImage *picture;
  gsize before;

  write_settings ("[new-picture]\nwidth=1200\nheight=1100\nbackground=#FFFF00\n");
  win = start_window ();
  canvas = easel_window_get_canvas (win);
  picture = easel_window_get_picture (win);
  use_pencil (win);

  /* 1200 x 1100 is 3 x 3 tiles; the right and bottom ones are smaller. */
  g_assert_nonnull (easel_canvas_get_tile_texture (canvas, 2, 2));
  g_assert_null (easel_canvas_get_tile_texture (canvas, 3, 0));
  g_assert_null (easel_canvas_get_tile_texture (canvas, 0, 3));
  g_assert_cmpint (gdk_texture_get_width (easel_canvas_get_tile_texture (canvas, 2, 2)), ==, 200);
  g_assert_cmpint (gdk_texture_get_height (easel_canvas_get_tile_texture (canvas, 2, 2)), ==, 100);

  /* A stroke inside one tile rebuilds that tile; a stroke across three tiles rebuilds those three; nothing, none. */
  before = easel_canvas_get_tiles_rebuilt (canvas);
  stroke (win, 10.5, 10.5, 20.5, 10.5);
  g_assert_cmpuint (easel_canvas_get_tiles_rebuilt (canvas) - before, ==, 2);   /* press and motion: 1 + 1 */
  before = easel_canvas_get_tiles_rebuilt (canvas);
  g_assert_cmpuint (easel_canvas_get_tiles_rebuilt (canvas) - before, ==, 0);
  stroke (win, 10.5, 600.5, 1100.5, 600.5);
  g_assert_cmpuint (easel_canvas_get_tiles_rebuilt (canvas) - before, ==, 4);   /* press: 1 tile, the motion: 3 tiles */
  g_assert_cmpuint (easel_canvas_get_bytes_copied (canvas), ==, 0);

  /* The tiles hold the pixels: inside a tile, on a tile edge, across the tiles, and untouched ones. */
  g_assert_cmphex (tile_pixel (canvas, 15, 10), ==, 0xFF000000u);
  g_assert_cmphex (tile_pixel (canvas, 499, 600), ==, 0xFF000000u);
  g_assert_cmphex (tile_pixel (canvas, 500, 600), ==, 0xFF000000u);
  g_assert_cmphex (tile_pixel (canvas, 1000, 600), ==, 0xFF000000u);
  g_assert_cmphex (tile_pixel (canvas, 1100, 600), ==, 0xFF000000u);
  g_assert_cmphex (tile_pixel (canvas, 1150, 700), ==, easel_image_get (picture, 1150, 700));

  /* Another picture, then back; the other can be freed once the canvas shows the first again. */
  easel_canvas_set_image (canvas, other);
  g_assert_cmphex (tile_pixel (canvas, 5, 5), ==, 0xFF112233u);
  g_assert_null (easel_canvas_get_tile_texture (canvas, 1, 0));
  easel_canvas_set_image (canvas, (EaselImage *) picture);
  easel_image_free (other);
  g_assert_cmphex (tile_pixel (canvas, 1100, 600), ==, 0xFF000000u);
  stroke (win, 1150.5, 1050.5, 1190.5, 1050.5);
  g_assert_cmphex (tile_pixel (canvas, 1170, 1050), ==, 0xFF000000u);
  g_assert_cmpuint (easel_canvas_get_bytes_copied (canvas), ==, 0);
  close_windows ();
  write_settings ("[new-picture]\nwidth=300\nheight=200\nbackground=#FFFF00\n");
}

/* ---- 0004: menus and zoom ---- */

/* The labels and actions of one section of a menu model, as "label|action" (a custom item as "custom:<name>"). */
static char *
describe_section (GMenuModel *model, int section)
{
  g_autoptr (GMenuModel) items = g_menu_model_get_item_link (model, section, G_MENU_LINK_SECTION);
  GString *out = g_string_new (NULL);

  for (int i = 0; i < g_menu_model_get_n_items (items); i++)
    {
      g_autofree char *label = NULL, *action = NULL, *custom = NULL;

      g_menu_model_get_item_attribute (items, i, "label", "s", &label);
      g_menu_model_get_item_attribute (items, i, "action", "s", &action);
      g_menu_model_get_item_attribute (items, i, "custom", "s", &custom);
      if (i > 0)
        g_string_append (out, "; ");
      if (custom != NULL)
        g_string_append_printf (out, "custom:%s", custom);
      else
        g_string_append_printf (out, "%s|%s", label, action);
    }
  return g_string_free (out, FALSE);
}

static GMenuModel *
menu_of (GtkMenuButton *button)
{
  return gtk_menu_button_get_menu_model (button);
}

/* REQ-0004-01, REQ-0004-02: the Image and View buttons and their menus. */
static void
as1_menus (void)
{
  EaselWindow *win = start_window ();
  GtkMenuButton *image = easel_window_get_image_button (win);
  GtkMenuButton *view = easel_window_get_view_button (win);
  GMenuModel *vm = menu_of (view), *im = menu_of (image);
  GtkWidget *popover = GTK_WIDGET (gtk_menu_button_get_popover (view));
  g_autofree char *s0 = NULL, *s1 = NULL, *s2 = NULL, *s3 = NULL, *i0 = NULL;
  GtkButton *zoom_in, *zoom_out;

  wait_for_layout (win);
  /* Image first, then View, each with an icon and its label, both before the primary menu in the header bar. */
  g_assert_true (ADW_IS_BUTTON_CONTENT (gtk_menu_button_get_child (image)));
  g_assert_cmpstr (adw_button_content_get_label (ADW_BUTTON_CONTENT (gtk_menu_button_get_child (image))), ==, "Image");
  g_assert_cmpstr (adw_button_content_get_label (ADW_BUTTON_CONTENT (gtk_menu_button_get_child (view))), ==, "View");
  g_assert_cmpstr (adw_button_content_get_icon_name (ADW_BUTTON_CONTENT (gtk_menu_button_get_child (image))), !=, NULL);
  g_assert_cmpstr (adw_button_content_get_icon_name (ADW_BUTTON_CONTENT (gtk_menu_button_get_child (view))), !=, NULL);
  {
    graphene_rect_t ri = bounds_in (GTK_WIDGET (image), GTK_WIDGET (win)), rv = bounds_in (GTK_WIDGET (view), GTK_WIDGET (win));

    g_assert_cmpfloat (ri.origin.x + ri.size.width, <=, rv.origin.x);
  }

  g_assert_cmpint (g_menu_model_get_n_items (vm), ==, 4);
  s0 = describe_section (vm, 0);
  s1 = describe_section (vm, 1);
  s2 = describe_section (vm, 2);
  s3 = describe_section (vm, 3);
  g_assert_cmpstr (s0, ==, "custom:zoom");
  /* 0008 (REQ-0008-03) puts the Zoom submenu first in this group; its items are checked by /0008/as1. */
  g_assert_cmpstr (s1, ==, "Zoom|(null); Normal Size|win.zoom-normal; Large Size|win.zoom-large; Fit to Window|win.zoom-fit");
  g_assert_cmpstr (s2, ==, "Show Grid|win.grid; Grid…|win.grid-dialog");
  g_assert_cmpstr (s3, ==, "Tool Box|win.show-toolbox; Color Box|win.show-colorbox; Status Bar|win.show-statusbar");
  g_assert_cmpint (g_menu_model_get_n_items (im), ==, 1);
  i0 = describe_section (im, 0);
  g_assert_cmpstr (i0, ==, "Attributes…|win.attributes");

  /* No Frame Time and no Show Thumbnail. */
  g_assert_null (g_strstr_len (s3, -1, "Frame"));
  g_assert_null (g_strstr_len (s2, -1, "Thumbnail"));

  /* The shortcuts, shown beside the items by the menus (REQ-0004-01). */
  g_assert_true (has_accel ("win.zoom-in", GDK_KEY_equal, GDK_CONTROL_MASK));
  g_assert_true (has_accel ("win.zoom-in", GDK_KEY_plus, GDK_CONTROL_MASK));
  g_assert_true (has_accel ("win.zoom-in", GDK_KEY_KP_Add, GDK_CONTROL_MASK));
  g_assert_true (has_accel ("win.zoom-out", GDK_KEY_minus, GDK_CONTROL_MASK));
  g_assert_true (has_accel ("win.zoom-out", GDK_KEY_KP_Subtract, GDK_CONTROL_MASK));
  g_assert_true (has_accel ("win.zoom-normal", GDK_KEY_Page_Up, GDK_CONTROL_MASK));
  g_assert_true (has_accel ("win.zoom-large", GDK_KEY_Page_Down, GDK_CONTROL_MASK));
  g_assert_true (has_accel ("win.zoom-fit", GDK_KEY_0, GDK_CONTROL_MASK));
  g_assert_true (has_accel ("win.attributes", GDK_KEY_e, GDK_CONTROL_MASK));

  /* The zoom row: Zoom Out, the zoom as N%, Zoom In, with their tooltips. */
  zoom_in = find_action_button (popover, "win.zoom-in");
  zoom_out = find_action_button (popover, "win.zoom-out");
  g_assert_nonnull (zoom_in);
  g_assert_nonnull (zoom_out);
  g_assert_cmpstr (gtk_widget_get_tooltip_text (GTK_WIDGET (zoom_in)), ==, "Zoom In (Ctrl+=)");
  g_assert_cmpstr (gtk_widget_get_tooltip_text (GTK_WIDGET (zoom_out)), ==, "Zoom Out (Ctrl+-)");
  g_assert_cmpstr (easel_window_get_zoom_text (win), ==, "100%");

  /* At 100 %: Normal Size has nothing to do; the rest can act. */
  g_assert_false (action_enabled (win, "zoom-normal"));
  g_assert_true (action_enabled (win, "zoom-in"));
  g_assert_true (action_enabled (win, "zoom-out"));
  g_assert_true (action_enabled (win, "zoom-large"));
  g_assert_true (action_enabled (win, "zoom-fit"));
  close_windows ();
}

static AdwAlertDialog *open_alert (EaselWindow *win);

/* REQ-0004-03: the actions do nothing in these states. */
static void
as2_actions_ignored (void)
{
  EaselWindow *win = start_window ();
  EaselCanvas *canvas = easel_window_get_canvas (win);
  AdwDialog *prompt = NULL;

  wait_for_layout (win);
  use_pencil (win);
  A_STROKE (win);

  /* A stroke in progress (the button held). */
  easel_canvas_press (canvas, 1, 3.5, 30.5);
  easel_canvas_motion (canvas, 9.5, 30.5);
  g_action_group_activate_action (G_ACTION_GROUP (win), "zoom-in", NULL);
  g_action_group_activate_action (G_ACTION_GROUP (win), "zoom-large", NULL);
  g_action_group_activate_action (G_ACTION_GROUP (win), "zoom-fit", NULL);
  g_action_group_activate_action (G_ACTION_GROUP (win), "grid", NULL);
  g_action_group_activate_action (G_ACTION_GROUP (win), "grid-dialog", NULL);
  g_assert_cmpint (easel_window_get_zoom (win), ==, 100);
  g_assert_false (easel_canvas_get_grid_on (canvas));
  g_assert_null (open_alert (win));
  easel_canvas_release (canvas, 1);

  /* The quit prompt open. */
  gtk_window_close (GTK_WINDOW (win));
  wait_for_dialogs (win, 1);
  g_action_group_activate_action (G_ACTION_GROUP (win), "zoom-in", NULL);
  g_action_group_activate_action (G_ACTION_GROUP (win), "zoom-large", NULL);
  g_action_group_activate_action (G_ACTION_GROUP (win), "grid", NULL);
  g_action_group_activate_action (G_ACTION_GROUP (win), "grid-dialog", NULL);
  g_assert_cmpint (easel_window_get_zoom (win), ==, 100);
  g_assert_false (easel_canvas_get_grid_on (canvas));
  g_assert_cmpint (count_alert_dialogs (GTK_WIDGET (win), &prompt), ==, 1);        /* only the quit prompt */
  adw_dialog_close (prompt);
  wait_for_dialogs (win, 0);

  /* The color chooser open. */
  easel_palette_bar_press_indicator (easel_window_get_palette_bar (win), TRUE);
  g_assert_true (easel_window_is_choosing_color (win));
  g_action_group_activate_action (G_ACTION_GROUP (win), "zoom-in", NULL);
  g_action_group_activate_action (G_ACTION_GROUP (win), "zoom-fit", NULL);
  g_action_group_activate_action (G_ACTION_GROUP (win), "grid", NULL);
  g_action_group_activate_action (G_ACTION_GROUP (win), "grid-dialog", NULL);
  g_assert_cmpint (easel_window_get_zoom (win), ==, 100);
  g_assert_false (easel_canvas_get_grid_on (canvas));
  g_assert_null (open_alert (win));
  easel_window_finish_color_choice (win, NULL);

  /* Free again: it works. */
  g_action_group_activate_action (G_ACTION_GROUP (win), "zoom-in", NULL);
  g_assert_cmpint (easel_window_get_zoom (win), ==, 200);
  close_windows ();
}

/* A window whose picture does not exist yet ignores the zoom actions too (REQ-0004-03, REQ-0001-39). */
static void
as2_actions_ignored_before_picture (void)
{
  EaselWindow *win;

  write_settings ("[new-picture]\nwidth=auto\nheight=auto\n");
  win = easel_window_new (app);
  g_assert_null (easel_window_get_picture (win));
  g_action_group_activate_action (G_ACTION_GROUP (win), "zoom-in", NULL);
  g_action_group_activate_action (G_ACTION_GROUP (win), "zoom-large", NULL);
  g_action_group_activate_action (G_ACTION_GROUP (win), "zoom-fit", NULL);
  g_action_group_activate_action (G_ACTION_GROUP (win), "grid", NULL);
  g_action_group_activate_action (G_ACTION_GROUP (win), "grid-dialog", NULL);
  g_assert_cmpint (easel_window_get_zoom (win), ==, 100);
  g_assert_false (easel_canvas_get_grid_on (easel_window_get_canvas (win)));
  gtk_window_destroy (GTK_WINDOW (win));
  write_settings ("[new-picture]\nwidth=300\nheight=200\nbackground=#FFFF00\n");
}

/* REQ-0004-04, REQ-0004-02: the zoom row's buttons, with any tool. */
static void
as3_zoom_buttons (void)
{
  EaselWindow *win = start_window ();
  GtkWidget *popover;
  GtkButton *zoom_in, *zoom_out;

  wait_for_layout (win);
  use_pencil (win);
  popover = GTK_WIDGET (gtk_menu_button_get_popover (easel_window_get_view_button (win)));
  zoom_in = find_action_button (popover, "win.zoom-in");
  zoom_out = find_action_button (popover, "win.zoom-out");

  g_signal_emit_by_name (zoom_in, "clicked");
  g_assert_cmpint (easel_window_get_zoom (win), ==, 200);
  g_assert_cmpstr (easel_window_get_zoom_text (win), ==, "200%");

  /* Up to 800 %: Zoom In becomes insensitive and does nothing. */
  for (int i = 0; i < 5; i++)
    g_action_group_activate_action (G_ACTION_GROUP (win), "zoom-in", NULL);
  g_assert_cmpint (easel_window_get_zoom (win), ==, 800);
  g_assert_cmpstr (easel_window_get_zoom_text (win), ==, "800%");
  g_assert_false (action_enabled (win, "zoom-in"));
  g_assert_false (gtk_widget_is_sensitive (GTK_WIDGET (zoom_in)));
  g_assert_true (gtk_widget_is_sensitive (GTK_WIDGET (zoom_out)));
  g_action_group_activate_action (G_ACTION_GROUP (win), "zoom-in", NULL);
  g_assert_cmpint (easel_window_get_zoom (win), ==, 800);

  /* And down to 10 %. */
  g_signal_emit_by_name (zoom_out, "clicked");
  g_assert_cmpint (easel_window_get_zoom (win), ==, 600);
  for (int i = 0; i < 10; i++)
    g_action_group_activate_action (G_ACTION_GROUP (win), "zoom-out", NULL);
  g_assert_cmpint (easel_window_get_zoom (win), ==, 10);
  g_assert_false (action_enabled (win, "zoom-out"));
  g_assert_false (gtk_widget_is_sensitive (GTK_WIDGET (zoom_out)));
  g_assert_true (action_enabled (win, "zoom-in"));
  close_windows ();
}

/* REQ-0004-05, REQ-0004-02. */
static void
as4_normal_and_large (void)
{
  EaselWindow *win = start_window ();

  wait_for_layout (win);
  g_action_group_activate_action (G_ACTION_GROUP (win), "zoom-large", NULL);
  g_assert_cmpint (easel_window_get_zoom (win), ==, 400);
  g_assert_false (action_enabled (win, "zoom-large"));
  g_assert_true (action_enabled (win, "zoom-normal"));
  g_assert_cmpstr (easel_window_get_zoom_text (win), ==, "400%");

  g_action_group_activate_action (G_ACTION_GROUP (win), "zoom-normal", NULL);
  g_assert_cmpint (easel_window_get_zoom (win), ==, 100);
  g_assert_false (action_enabled (win, "zoom-normal"));
  g_assert_true (action_enabled (win, "zoom-large"));
  close_windows ();
}

/* REQ-0004-06: a zoom command keeps the center of the canvas area still. */
static void
as5_zoom_keeps_center (void)
{
  EaselWindow *win;
  EaselCanvas *canvas;
  GtkScrolledWindow *scroller;
  GtkAdjustment *h, *v;
  double cx, cy, ph, pv;

  write_settings ("[new-picture]\nwidth=2000\nheight=2000\n");
  win = start_window ();
  canvas = easel_window_get_canvas (win);
  scroller = GTK_SCROLLED_WINDOW (gtk_widget_get_ancestor (GTK_WIDGET (canvas), GTK_TYPE_SCROLLED_WINDOW));
  h = gtk_scrolled_window_get_hadjustment (scroller);
  v = gtk_scrolled_window_get_vadjustment (scroller);
  wait_for_layout (win);
  ph = gtk_adjustment_get_page_size (h);
  pv = gtk_adjustment_get_page_size (v);

  gtk_adjustment_set_value (h, 500);
  gtk_adjustment_set_value (v, 400);
  wait_for_layout (win);
  /* The picture point at the center of the canvas area, in picture pixels (zoom 100 %, scale 1). */
  cx = view_to_canvas (h, ph / 2) / easel_canvas_get_scale (canvas);
  cy = view_to_canvas (v, pv / 2) / easel_canvas_get_scale (canvas);

  g_action_group_activate_action (G_ACTION_GROUP (win), "zoom-in", NULL);
  wait_for_layout (win);
  g_assert_cmpint (easel_window_get_zoom (win), ==, 200);
  g_assert_cmpfloat_with_epsilon (view_to_canvas (h, ph / 2) / 2 / easel_canvas_get_scale (canvas), cx, 1.0);
  g_assert_cmpfloat_with_epsilon (view_to_canvas (v, pv / 2) / 2 / easel_canvas_get_scale (canvas), cy, 1.0);

  /* A picture smaller than the canvas area stays at the top-left. */
  close_windows ();
  write_settings ("[new-picture]\nwidth=300\nheight=200\nbackground=#FFFF00\n");
  win = start_window ();
  canvas = easel_window_get_canvas (win);
  scroller = GTK_SCROLLED_WINDOW (gtk_widget_get_ancestor (GTK_WIDGET (canvas), GTK_TYPE_SCROLLED_WINDOW));
  wait_for_layout (win);
  g_action_group_activate_action (G_ACTION_GROUP (win), "zoom-in", NULL);
  wait_for_layout (win);
  g_assert_cmpfloat (gtk_adjustment_get_value (gtk_scrolled_window_get_hadjustment (scroller)), ==, 0);
  g_assert_cmpfloat (gtk_adjustment_get_value (gtk_scrolled_window_get_vadjustment (scroller)), ==, 0);
  close_windows ();
}

/* The canvas area in device pixels, as Fit to Window sees it. */
static void
canvas_area (EaselWindow *win, int *w, int *h)
{
  EaselCanvas *canvas = easel_window_get_canvas (win);
  GtkScrolledWindow *scroller = GTK_SCROLLED_WINDOW (gtk_widget_get_ancestor (GTK_WIDGET (canvas), GTK_TYPE_SCROLLED_WINDOW));
  double scale = easel_canvas_get_scale (canvas);

  *w = (int) (gtk_adjustment_get_page_size (gtk_scrolled_window_get_hadjustment (scroller)) * scale);
  *h = (int) (gtk_adjustment_get_page_size (gtk_scrolled_window_get_vadjustment (scroller)) * scale);
}

/* REQ-0004-07: Fit to Window picks the largest level that fits, shows the top-left corner, and the zoom then stays. */
static void
as6_fit_to_window (void)
{
  static const int sizes[][2] = { { 3000, 2000 }, { 800, 600 }, { 100, 100 }, { 10000, 100 }, { 5840, 5184 } };

  for (guint i = 0; i < G_N_ELEMENTS (sizes); i++)
    {
      g_autofree char *settings = g_strdup_printf ("[new-picture]\nwidth=%d\nheight=%d\n", sizes[i][0], sizes[i][1]);
      EaselWindow *win;
      EaselCanvas *canvas;
      GtkScrolledWindow *scroller;
      int aw, ah, want;

      write_settings (settings);
      win = start_window ();
      canvas = easel_window_get_canvas (win);
      scroller = GTK_SCROLLED_WINDOW (gtk_widget_get_ancestor (GTK_WIDGET (canvas), GTK_TYPE_SCROLLED_WINDOW));
      wait_for_layout (win);
      canvas_area (win, &aw, &ah);
      want = easel_view_fit_zoom (aw, ah, sizes[i][0], sizes[i][1], easel_canvas_get_scale (canvas));

      gtk_adjustment_set_value (gtk_scrolled_window_get_hadjustment (scroller), 300);
      gtk_adjustment_set_value (gtk_scrolled_window_get_vadjustment (scroller), 200);
      use_pencil (win);                                     /* with any tool */
      g_action_group_activate_action (G_ACTION_GROUP (win), "zoom-fit", NULL);
      wait_for_layout (win);
      g_assert_cmpint (easel_window_get_zoom (win), ==, want);
      g_assert_cmpfloat (gtk_adjustment_get_value (gtk_scrolled_window_get_hadjustment (scroller)), ==, 0);
      g_assert_cmpfloat (gtk_adjustment_get_value (gtk_scrolled_window_get_vadjustment (scroller)), ==, 0);
      g_assert_cmpstr (easel_window_get_zoom_text (win), ==, g_strdup_printf ("%d%%", want));

      /* The whole picture is visible at that zoom unless even 10 % is too big. */
      if (want > 10)
        {
          g_assert_cmpint (sizes[i][0] * want, <=, aw * 100);
          g_assert_cmpint (sizes[i][1] * want, <=, ah * 100);
        }
      /* No fit mode: the zoom stays when the layout changes. */
      gtk_widget_queue_resize (GTK_WIDGET (win));
      wait_for_layout (win);
      g_assert_cmpint (easel_window_get_zoom (win), ==, want);

      /* Zoom In and Zoom Out go to the level above and below (REQ-0004-04). */
      if (want < 800)
        {
          g_action_group_activate_action (G_ACTION_GROUP (win), "zoom-in", NULL);
          wait_for_layout (win);
          g_assert_cmpint (easel_window_get_zoom (win), ==, easel_view_zoom_step (want, 1));
          g_action_group_activate_action (G_ACTION_GROUP (win), "zoom-fit", NULL);
          wait_for_layout (win);
        }
      if (want > 10)
        {
          g_action_group_activate_action (G_ACTION_GROUP (win), "zoom-out", NULL);
          wait_for_layout (win);
          g_assert_cmpint (easel_window_get_zoom (win), ==, easel_view_zoom_step (want, -1));
        }
      close_windows ();
    }
  write_settings ("[new-picture]\nwidth=300\nheight=200\nbackground=#FFFF00\n");
}

/* REQ-0004-08: the pinch moves a level per 1.5x of the gesture, in Pan mode only. */
static void
as7_pinch_in_window (void)
{
  EaselWindow *win = start_window ();
  EaselCanvas *canvas = easel_window_get_canvas (win);

  wait_for_layout (win);
  g_action_group_activate_action (G_ACTION_GROUP (win), "tool", g_variant_new_string ("pan"));
  easel_canvas_pinch_begin (canvas);
  easel_canvas_pinch (canvas, 1.2, 100, 100);
  g_assert_cmpint (easel_window_get_zoom (win), ==, 100);
  easel_canvas_pinch (canvas, 1.5, 100, 100);
  g_assert_cmpint (easel_window_get_zoom (win), ==, 200);
  easel_canvas_pinch (canvas, 2.25, 100, 100);
  g_assert_cmpint (easel_window_get_zoom (win), ==, 300);
  easel_canvas_pinch (canvas, 1.5, 100, 100);
  g_assert_cmpint (easel_window_get_zoom (win), ==, 200);
  easel_canvas_pinch_end (canvas);

  /* A new pinch starts again at scale 1, and any other tool ignores it. */
  easel_canvas_pinch_begin (canvas);
  easel_canvas_pinch (canvas, 0.6, 100, 100);
  g_assert_cmpint (easel_window_get_zoom (win), ==, 100);
  easel_canvas_pinch_end (canvas);
  use_pencil (win);
  easel_canvas_pinch_begin (canvas);
  easel_canvas_pinch (canvas, 3.0, 100, 100);
  g_assert_cmpint (easel_window_get_zoom (win), ==, 100);
  easel_canvas_pinch_end (canvas);
  close_windows ();
}

/* ---- 0004: the status bar ---- */

#define PAN_HINT "Drag to pan. Ctrl+wheel zooms."
#define PENCIL_HINT "Draw with the foreground color. The secondary button uses the background color."

/* REQ-0004-09, REQ-0004-10: the four fields. */
static void
as8_status_fields (void)
{
  EaselWindow *win = start_window ();
  EaselCanvas *canvas = easel_window_get_canvas (win);
  EaselStatusBar *bar = easel_window_get_status_bar (win);

  wait_for_layout (win);
  /* Below the palette bar, across the window's full width. */
  {
    graphene_rect_t sb = bounds_in (GTK_WIDGET (bar), GTK_WIDGET (win));
    graphene_rect_t pb = bounds_in (GTK_WIDGET (easel_window_get_palette_bar (win)), GTK_WIDGET (win));

    g_assert_cmpfloat (sb.origin.y, >=, pb.origin.y + pb.size.height - 0.5);
    g_assert_cmpfloat (sb.origin.x, <=, 0.5);
    g_assert_cmpfloat (sb.size.width, >=, gtk_widget_get_width (GTK_WIDGET (win)) - 0.5);
  }

  /* The hint at the left; at the right edge, in order: position, zoom, size (REQ-0004-09). */
  {
    graphene_rect_t hint = bounds_in (GTK_WIDGET (easel_status_bar_get_label (bar, EASEL_STATUS_HINT)), GTK_WIDGET (bar));
    graphene_rect_t pos = bounds_in (GTK_WIDGET (easel_status_bar_get_label (bar, EASEL_STATUS_POSITION)), GTK_WIDGET (bar));
    graphene_rect_t zoom = bounds_in (GTK_WIDGET (easel_status_bar_get_label (bar, EASEL_STATUS_ZOOM)), GTK_WIDGET (bar));
    graphene_rect_t size = bounds_in (GTK_WIDGET (easel_status_bar_get_label (bar, EASEL_STATUS_SIZE)), GTK_WIDGET (bar));

    g_assert_cmpfloat (hint.origin.x, <, pos.origin.x);
    g_assert_cmpfloat (pos.origin.x + pos.size.width, <=, zoom.origin.x);
    g_assert_cmpfloat (zoom.origin.x + zoom.size.width, <=, size.origin.x);
    /* The group sits at the right edge of the bar. */
    g_assert_cmpfloat (size.origin.x + size.size.width, >=, gtk_widget_get_width (GTK_WIDGET (bar)) - 12);
  }

  /* A 300x200 picture at 200 % with Pan active. */
  g_action_group_activate_action (G_ACTION_GROUP (win), "tool", g_variant_new_string ("pan"));
  g_action_group_activate_action (G_ACTION_GROUP (win), "zoom-in", NULL);
  wait_for_layout (win);
  g_assert_cmpint (easel_window_get_zoom (win), ==, 200);
  g_assert_cmpstr (easel_status_bar_get_text (bar, EASEL_STATUS_SIZE), ==, "300 × 200 px");
  g_assert_cmpstr (easel_status_bar_get_text (bar, EASEL_STATUS_ZOOM), ==, "200%");
  g_assert_cmpstr (easel_status_bar_get_text (bar, EASEL_STATUS_HINT), ==, PAN_HINT);
  /* A real pointer over the test window may have reported a position; start from the pointer being elsewhere. */
  easel_canvas_pointer_left (canvas);
  g_assert_cmpstr (easel_status_bar_get_text (bar, EASEL_STATUS_POSITION), ==, "");

  /* Over picture pixel (12, 34); at 200 % a pixel is 2 x 2 logical pixels. */
  easel_canvas_pointer_at (canvas, 25.0, 69.0);
  g_assert_cmpstr (easel_status_bar_get_text (bar, EASEL_STATUS_POSITION), ==, "12, 34");
  /* Over the canvas area but outside the picture: negative or beyond the size. */
  easel_canvas_pointer_at (canvas, -5.0, 900.0);
  g_assert_cmpstr (easel_status_bar_get_text (bar, EASEL_STATUS_POSITION), ==, "-3, 450");
  easel_canvas_pointer_at (canvas, 700.0, 10.0);
  g_assert_cmpstr (easel_status_bar_get_text (bar, EASEL_STATUS_POSITION), ==, "350, 5");
  /* Elsewhere in the window (the header bar, the palette bar): empty. */
  easel_canvas_pointer_left (canvas);
  g_assert_cmpstr (easel_status_bar_get_text (bar, EASEL_STATUS_POSITION), ==, "");

  /* The hint follows the tool. */
  use_pencil (win);
  g_assert_cmpstr (easel_status_bar_get_text (bar, EASEL_STATUS_HINT), ==, PENCIL_HINT);
  g_action_group_activate_action (G_ACTION_GROUP (win), "tool", g_variant_new_string ("pan"));
  g_assert_cmpstr (easel_status_bar_get_text (bar, EASEL_STATUS_HINT), ==, PAN_HINT);
  close_windows ();
}

/* REQ-0004-10: before the picture exists the position, size and zoom fields are empty; the hint is shown. */
static void
as8_status_before_picture (void)
{
  EaselWindow *win;
  EaselStatusBar *bar;

  write_settings ("[new-picture]\nwidth=auto\nheight=auto\n");
  win = easel_window_new (app);
  bar = easel_window_get_status_bar (win);
  g_assert_null (easel_window_get_picture (win));
  g_assert_cmpstr (easel_status_bar_get_text (bar, EASEL_STATUS_POSITION), ==, "");
  g_assert_cmpstr (easel_status_bar_get_text (bar, EASEL_STATUS_SIZE), ==, "");
  g_assert_cmpstr (easel_status_bar_get_text (bar, EASEL_STATUS_ZOOM), ==, "");
  g_assert_cmpstr (easel_status_bar_get_text (bar, EASEL_STATUS_HINT), ==, PAN_HINT);
  gtk_window_destroy (GTK_WINDOW (win));
  write_settings ("[new-picture]\nwidth=300\nheight=200\nbackground=#FFFF00\n");
}

/* REQ-0004-10 (Plan): a field is written only when its value changes. */
static void
as8_status_writes_only_on_change (void)
{
  EaselWindow *win = start_window ();
  EaselCanvas *canvas = easel_window_get_canvas (win);
  EaselStatusBar *bar = easel_window_get_status_bar (win);
  guint pos, size, zoom, hint;

  wait_for_layout (win);
  g_action_group_activate_action (G_ACTION_GROUP (win), "tool", g_variant_new_string ("pan"));
  g_action_group_activate_action (G_ACTION_GROUP (win), "zoom-large", NULL);       /* 400 %: a pixel is 4 x 4 */
  wait_for_layout (win);
  easel_canvas_pointer_at (canvas, 41.0, 41.0);                                    /* pixel (10, 10) */
  pos = easel_status_bar_get_writes (bar, EASEL_STATUS_POSITION);
  size = easel_status_bar_get_writes (bar, EASEL_STATUS_SIZE);
  zoom = easel_status_bar_get_writes (bar, EASEL_STATUS_ZOOM);
  hint = easel_status_bar_get_writes (bar, EASEL_STATUS_HINT);

  /* 50 pointer events inside that one picture pixel write nothing. */
  for (int i = 0; i < 50; i++)
    easel_canvas_pointer_at (canvas, 40.0 + 0.07 * i, 40.0 + 0.07 * (49 - i));
  g_assert_cmpuint (easel_status_bar_get_writes (bar, EASEL_STATUS_POSITION), ==, pos);

  /* The same tool, the same zoom and the same size again write nothing either. */
  g_action_group_activate_action (G_ACTION_GROUP (win), "tool", g_variant_new_string ("pan"));
  easel_status_bar_set_size (bar, TRUE, 300, 200);
  easel_status_bar_set_zoom (bar, TRUE, 400);
  easel_status_bar_set_hint (bar, PAN_HINT);
  easel_canvas_pointer_left (canvas);
  easel_canvas_pointer_left (canvas);                                              /* empty twice: one write */
  g_assert_cmpuint (easel_status_bar_get_writes (bar, EASEL_STATUS_SIZE), ==, size);
  g_assert_cmpuint (easel_status_bar_get_writes (bar, EASEL_STATUS_ZOOM), ==, zoom);
  g_assert_cmpuint (easel_status_bar_get_writes (bar, EASEL_STATUS_HINT), ==, hint);
  g_assert_cmpuint (easel_status_bar_get_writes (bar, EASEL_STATUS_POSITION), ==, pos + 1);

  /* A different pixel writes once. */
  easel_canvas_pointer_at (canvas, 45.0, 41.0);
  g_assert_cmpstr (easel_status_bar_get_text (bar, EASEL_STATUS_POSITION), ==, "11, 10");
  g_assert_cmpuint (easel_status_bar_get_writes (bar, EASEL_STATUS_POSITION), ==, pos + 2);
  close_windows ();
}

/* REQ-0004-11: the status bar never makes the window wider; the hint is shortened with an ellipsis. */
static void
as9_status_bar_does_not_force_width (void)
{
  EaselWindow *win = start_window ();
  EaselStatusBar *bar = easel_window_get_status_bar (win);
  int min = 0;

  gtk_widget_measure (GTK_WIDGET (bar), GTK_ORIENTATION_HORIZONTAL, -1, &min, NULL, NULL, NULL);
  g_assert_cmpint (min, <=, 300);
  g_assert_cmpint (gtk_label_get_ellipsize (easel_status_bar_get_label (bar, EASEL_STATUS_HINT)), ==, PANGO_ELLIPSIZE_END);
  close_windows ();
}

/* ---- 0004: panel toggles ---- */

static gboolean
panel_checked (EaselWindow *win, const char *action)
{
  g_autoptr (GVariant) state = g_action_group_get_action_state (G_ACTION_GROUP (win), action);

  return g_variant_get_boolean (state);
}

/* The width the bottom area (the color box over the status bar) needs. The window's own minimum is set by the header
   bar, so the color box's part is measured where it counts. */
static int
bottom_min_width (EaselWindow *win)
{
  int min = 0;

  gtk_widget_measure (gtk_widget_get_parent (GTK_WIDGET (easel_window_get_palette_bar (win))), GTK_ORIENTATION_HORIZONTAL,
                      -1, &min, NULL, NULL, NULL);
  return min;
}

/* REQ-0004-27: Tool Box, Color Box and Status Bar hide and show their panels. */
static void
as28_panel_toggles (void)
{
  EaselWindow *win = start_window ();
  EaselPaletteBar *colors = easel_window_get_palette_bar (win);
  EaselStatusBar *status = easel_window_get_status_bar (win);
  GtkToggleButton *pan = find_tool_button (GTK_WIDGET (win), "pan");
  GtkWidget *scroller = gtk_widget_get_ancestor (GTK_WIDGET (easel_window_get_canvas (win)), GTK_TYPE_SCROLLED_WINDOW);
  int height_with, min_with;

  wait_for_layout (win);
  g_assert_true (panel_checked (win, "show-toolbox") && panel_checked (win, "show-colorbox") && panel_checked (win, "show-statusbar"));
  g_assert_true (gtk_widget_is_visible (GTK_WIDGET (colors)) && gtk_widget_is_visible (GTK_WIDGET (status)));
  g_assert_true (gtk_widget_is_visible (GTK_WIDGET (pan)));

  /* Color Box off: the bar is gone and the canvas area is taller by its height. */
  height_with = gtk_widget_get_height (scroller);
  min_with = bottom_min_width (win);
  g_assert_cmpint (min_with, >, 300);
  g_action_group_activate_action (G_ACTION_GROUP (win), "show-colorbox", NULL);
  wait_for_layout (win);
  g_assert_false (panel_checked (win, "show-colorbox"));
  g_assert_false (gtk_widget_is_visible (GTK_WIDGET (colors)));
  g_assert_cmpint (gtk_widget_get_height (scroller), >=, height_with + 40);
  g_assert_cmpint (bottom_min_width (win), <=, 300);        /* REQ-0003-03 applies only while the bar is shown */

  /* The colors still swap and strokes still use them. */
  easel_palette_bar_press (colors, 17, 1, 1);
  easel_palette_bar_press (colors, 21, 3, 1);
  g_action_group_activate_action (G_ACTION_GROUP (win), "swap-colors", NULL);
  g_assert_cmphex (easel_window_get_foreground (win), ==, 0xFF0000FFu);
  use_pencil (win);
  stroke (win, 3.5, 3.5, 9.5, 3.5);
  g_assert_cmphex (easel_image_get (easel_window_get_picture (win), 5, 3), ==, 0xFF0000FFu);

  /* Back on: the bar shows the current colors. */
  g_action_group_activate_action (G_ACTION_GROUP (win), "show-colorbox", NULL);
  g_assert_true (panel_checked (win, "show-colorbox"));
  g_assert_true (gtk_widget_is_visible (GTK_WIDGET (colors)));
  g_assert_cmphex (easel_palette_bar_get_foreground (colors), ==, 0xFF0000FFu);

  /* Tool Box off: the buttons go, the active tool stays. */
  g_action_group_activate_action (G_ACTION_GROUP (win), "show-toolbox", NULL);
  wait_for_layout (win);
  g_assert_false (panel_checked (win, "show-toolbox"));
  g_assert_false (gtk_widget_is_visible (GTK_WIDGET (pan)));
  g_assert_cmpint (easel_window_get_tool (win), ==, EASEL_TOOL_PENCIL);
  g_action_group_activate_action (G_ACTION_GROUP (win), "show-toolbox", NULL);
  g_assert_true (gtk_widget_is_visible (GTK_WIDGET (pan)));

  /* Status Bar off: its fields go. */
  g_action_group_activate_action (G_ACTION_GROUP (win), "show-statusbar", NULL);
  g_assert_false (panel_checked (win, "show-statusbar"));
  g_assert_false (gtk_widget_is_visible (GTK_WIDGET (status)));
  g_action_group_activate_action (G_ACTION_GROUP (win), "show-statusbar", NULL);
  g_assert_true (gtk_widget_is_visible (GTK_WIDGET (status)));

  /* Not edits: one stroke was drawn, so exactly one undo step exists, and nothing else marks the picture. */
  g_action_group_activate_action (G_ACTION_GROUP (win), "undo", NULL);
  g_assert_false (action_enabled (win, "undo"));
  close_windows ();
}

/* REQ-0004-27, REQ-0004-15: a panel starts as the settings say; the toggles ignore the cases of REQ-0004-03. */
static void
as28_panels_start_as_settings_say (void)
{
  EaselWindow *win;
  AdwDialog *prompt = NULL;

  write_settings ("[new-picture]\nwidth=300\nheight=200\n\n[view]\ntoolbox=off\ncolorbox=off\nstatusbar=off\n");
  win = start_window ();
  wait_for_layout (win);
  g_assert_false (panel_checked (win, "show-toolbox") || panel_checked (win, "show-colorbox") || panel_checked (win, "show-statusbar"));
  g_assert_false (gtk_widget_is_visible (GTK_WIDGET (easel_window_get_palette_bar (win))));
  g_assert_false (gtk_widget_is_visible (GTK_WIDGET (easel_window_get_status_bar (win))));
  g_assert_false (gtk_widget_is_visible (GTK_WIDGET (find_tool_button (GTK_WIDGET (win), "pan"))));

  /* While the quit prompt is open the toggles do nothing. */
  use_pencil (win);
  A_STROKE (win);
  gtk_window_close (GTK_WINDOW (win));
  wait_for_dialogs (win, 1);
  g_action_group_activate_action (G_ACTION_GROUP (win), "show-colorbox", NULL);
  g_assert_false (panel_checked (win, "show-colorbox"));
  g_assert_false (gtk_widget_is_visible (GTK_WIDGET (easel_window_get_palette_bar (win))));
  g_assert_cmpint (count_alert_dialogs (GTK_WIDGET (win), &prompt), ==, 1);
  adw_dialog_close (prompt);
  wait_for_dialogs (win, 0);
  close_windows ();
  write_settings ("[new-picture]\nwidth=300\nheight=200\nbackground=#FFFF00\n");
}

/* REQ-0001-16 and REQ-0004-06: after a zoom the picture is shown where the scroll position says, not only the adjustment
   values (found while testing the grid: the view stayed unscrolled until the next layout). */
static void
as13_offset_is_displayed (void)
{
  EaselWindow *win;
  EaselCanvas *canvas;
  GtkWidget *scroller;
  GtkAdjustment *h, *v;
  graphene_rect_t r;
  const char *actions[] = { "zoom-in", "zoom-in", "zoom-out", "zoom-large", "zoom-normal", "zoom-fit", "zoom-large" };

  write_settings ("[new-picture]\nwidth=2000\nheight=2000\n");
  win = start_window ();
  canvas = easel_window_get_canvas (win);
  scroller = gtk_widget_get_ancestor (GTK_WIDGET (canvas), GTK_TYPE_SCROLLED_WINDOW);
  h = gtk_scrolled_window_get_hadjustment (GTK_SCROLLED_WINDOW (scroller));
  v = gtk_scrolled_window_get_vadjustment (GTK_SCROLLED_WINDOW (scroller));
  wait_for_layout (win);
  gtk_adjustment_set_value (h, 500);
  gtk_adjustment_set_value (v, 400);
  wait_for_layout (win);

  for (guint i = 0; i < G_N_ELEMENTS (actions); i++)
    {
      g_action_group_activate_action (G_ACTION_GROUP (win), actions[i], NULL);
      wait_for_layout (win);
      g_assert_true (gtk_widget_compute_bounds (GTK_WIDGET (canvas), scroller, &r));
      /* A failure here has not been reproduced on demand: say what the view looked like (docs/learning-log.md). */
      if (fabs (r.origin.x - (EASEL_VIEW_MARGIN - gtk_adjustment_get_value (h))) > 1.0
          || fabs (r.origin.y - (EASEL_VIEW_MARGIN - gtk_adjustment_get_value (v))) > 1.0)
        {
          GdkFrameClock *clock = gtk_widget_get_frame_clock (GTK_WIDGET (win));

          g_printerr ("as13 diagnostic: step %u (%s) zoom %d, origin %.1f,%.1f, scroll %.1f,%.1f, window %dx%d, frame %" G_GINT64_FORMAT "\n",
                      i, actions[i], easel_canvas_get_zoom (canvas), r.origin.x, r.origin.y,
                      gtk_adjustment_get_value (h), gtk_adjustment_get_value (v),
                      gtk_widget_get_width (GTK_WIDGET (win)), gtk_widget_get_height (GTK_WIDGET (win)),
                      clock ? gdk_frame_clock_get_frame_counter (clock) : (gint64) -1);
        }
      /* The canvas is the margin in from the start of the scroll (REQ-0009-01); GTK places it at whole pixels. */
      g_assert_cmpfloat_with_epsilon (r.origin.x, EASEL_VIEW_MARGIN - gtk_adjustment_get_value (h), 1.0);
      g_assert_cmpfloat_with_epsilon (r.origin.y, EASEL_VIEW_MARGIN - gtk_adjustment_get_value (v), 1.0);
    }
  close_windows ();
  write_settings ("[new-picture]\nwidth=300\nheight=200\nbackground=#FFFF00\n");
}

/* ---- 0004: the pixel grid ---- */

/* The window's pixels as drawn now (premultiplied 0xAARRGGBB), and the position of the canvas in them. */
static uint32_t *
render_pixels (EaselWindow *win, int *w, int *h, int *canvas_x, int *canvas_y)
{
  GtkWidget *widget = GTK_WIDGET (win);
  EaselCanvas *canvas = easel_window_get_canvas (win);
  g_autoptr (GdkPaintable) paintable = gtk_widget_paintable_new (widget);
  GtkSnapshot *snapshot = gtk_snapshot_new ();
  GskRenderNode *node;
  g_autoptr (GdkTexture) texture = NULL;
  graphene_rect_t r;
  double scale = easel_canvas_get_scale (canvas);
  uint32_t *px;

  *w = gtk_widget_get_width (widget);
  *h = gtk_widget_get_height (widget);
  gdk_paintable_snapshot (paintable, snapshot, *w, *h);
  node = gtk_snapshot_free_to_node (snapshot);
  texture = gsk_renderer_render_texture (gtk_native_get_renderer (GTK_NATIVE (win)), node,
                                         &GRAPHENE_RECT_INIT (0, 0, *w, *h));
  gsk_render_node_unref (node);
  *w = gdk_texture_get_width (texture);
  *h = gdk_texture_get_height (texture);
  px = g_malloc ((gsize) *w * (gsize) *h * 4);
  gdk_texture_download (texture, (guchar *) px, (gsize) *w * 4);
  g_assert_true (gtk_widget_compute_bounds (GTK_WIDGET (canvas), widget, &r));
  *canvas_x = (int) (r.origin.x * scale);
  *canvas_y = (int) (r.origin.y * scale);
  return px;
}

/* REQ-0004-12: Show Grid and Ctrl+G toggle the grid; the lines show only while a cell is 4 device pixels wide; the grid
   is not part of the picture, not an edit. */
static void
as10_grid_toggle (void)
{
  g_autofree char *dir = g_dir_make_tmp ("easel-ui-XXXXXX", NULL);
  g_autofree char *path = g_build_filename (dir, "a.png", NULL);
  EaselWindow *win = start_window ();
  EaselCanvas *canvas = easel_window_get_canvas (win);
  g_autoptr (GdkPixbuf) saved = NULL;

  wait_for_layout (win);
  g_assert_true (has_accel ("win.grid", GDK_KEY_g, GDK_CONTROL_MASK));
  g_assert_false (panel_checked (win, "grid"));
  g_assert_false (easel_canvas_get_grid_on (canvas));

  /* On, but 1-pixel cells at 100 % are 1 device pixel: hidden, and the check item stays checked. */
  g_action_group_activate_action (G_ACTION_GROUP (win), "grid", NULL);
  g_assert_true (panel_checked (win, "grid"));
  g_assert_true (easel_canvas_get_grid_on (canvas));
  g_assert_false (easel_canvas_grid_is_drawn (canvas));

  g_action_group_activate_action (G_ACTION_GROUP (win), "zoom-large", NULL);       /* 400 % */
  g_assert_true (easel_canvas_grid_is_drawn (canvas));
  g_action_group_activate_action (G_ACTION_GROUP (win), "zoom-out", NULL);         /* 300 % */
  g_assert_false (easel_canvas_grid_is_drawn (canvas));
  g_assert_true (panel_checked (win, "grid"));
  g_action_group_activate_action (G_ACTION_GROUP (win), "zoom-large", NULL);
  g_assert_true (easel_canvas_grid_is_drawn (canvas));

  /* Not an edit: no modified mark, nothing to undo; and not in the saved picture. */
  g_assert_cmpstr (gtk_window_get_title (GTK_WINDOW (win)), ==, "Untitled – Easel");
  g_assert_false (action_enabled (win, "undo"));
  g_assert_true (easel_window_save_to (win, path));
  saved = gdk_pixbuf_new_from_file (path, NULL);
  g_assert_nonnull (saved);
  g_assert_cmpint (gdk_pixbuf_get_width (saved), ==, 300);
  g_assert_cmpint (gdk_pixbuf_get_pixels (saved)[0], ==, 0xFF);            /* the yellow background's red channel */
  g_assert_cmpint (gdk_pixbuf_get_pixels (saved)[4 * 4 + 2], ==, 0);       /* blue 0: no grey line at x = 4 */
  g_assert_cmpstr (gtk_window_get_title (GTK_WINDOW (win)), ==, "a.png – Easel");

  /* Ctrl+G's action turns it off again. */
  g_action_group_activate_action (G_ACTION_GROUP (win), "grid", NULL);
  g_assert_false (panel_checked (win, "grid"));
  g_assert_false (easel_canvas_get_grid_on (canvas));
  close_windows ();
}

/* The grid as it is drawn: 1 device pixel wide lines in grey at 50 %, every picture pixel at 400 %, none at 100 %. The
   yellow background #FFFF00 under 50 % grey gives #BFBF40. The scrolled window shades the first few pixels along its
   edges, so the samples are taken well inside the picture. (That the picture's own edges get no line is checked on the
   geometry in core: easel_grid_lines.) */
static void
as10_grid_pixels (void)
{
  const uint32_t yellow = 0xFFFFFF00u, line = 0xFFBFBF40u;
  EaselWindow *win;
  GtkWidget *scroller;
  int w, h, cx, cy, sv, sh, R, C;
  g_autofree uint32_t *px = NULL;

  write_settings ("[new-picture]\nwidth=300\nheight=200\nbackground=#FFFF00\n");
  win = start_window ();
  scroller = gtk_widget_get_ancestor (GTK_WIDGET (easel_window_get_canvas (win)), GTK_TYPE_SCROLLED_WINDOW);
  wait_for_layout (win);
  g_action_group_activate_action (G_ACTION_GROUP (win), "grid", NULL);
  g_action_group_activate_action (G_ACTION_GROUP (win), "zoom-large", NULL);
  wait_for_layout (win);
  px = render_pixels (win, &w, &h, &cx, &cy);
  /* The canvas can be scrolled (a 200-pixel picture at 400 % is 800 high): rows and columns are canvas device pixels,
     taken at least 30 inside the visible part so the scrolled window's edge shading does not reach them. A line is at
     every multiple of 4 (a cell of 1 picture pixel); R and C are such multiples. */
  sv = (int) gtk_adjustment_get_value (gtk_scrolled_window_get_vadjustment (GTK_SCROLLED_WINDOW (scroller)));
  sh = (int) gtk_adjustment_get_value (gtk_scrolled_window_get_hadjustment (GTK_SCROLLED_WINDOW (scroller)));
  R = ((sv + 30) / 4 + 1) * 4;
  C = ((sh + 30) / 4 + 1) * 4;
#define PX(row, col) px[(gsize) (cy + (row)) * w + cx + (col)]

  g_assert_cmphex (PX (R + 2, C), ==, line);              /* a vertical line, between two horizontal ones */
  g_assert_cmphex (PX (R, C + 2), ==, line);              /* a horizontal line */
  g_assert_cmphex (PX (R, C), ==, line);                  /* where they cross: one line color, not blended twice */
  g_assert_cmphex (PX (R + 2, C + 8), ==, line);          /* two picture pixels further */
  g_assert_cmphex (PX (R + 12, C + 20), ==, line);        /* a horizontal line three pixels further down */
  /* Inside a cell: the picture. */
  g_assert_cmphex (PX (R + 2, C + 1), ==, yellow);
  g_assert_cmphex (PX (R + 2, C + 2), ==, yellow);
  g_assert_cmphex (PX (R + 2, C + 3), ==, yellow);
  g_assert_cmphex (PX (R + 1, C + 2), ==, yellow);
  g_assert_cmphex (PX (R + 3, C + 3), ==, yellow);

  /* At 100 % the lines are hidden. */
  g_free (px);
  g_action_group_activate_action (G_ACTION_GROUP (win), "zoom-normal", NULL);
  wait_for_layout (win);
  px = render_pixels (win, &w, &h, &cx, &cy);
  g_assert_cmphex (px[(gsize) (cy + 40) * w + cx + 40], ==, yellow);
  g_assert_cmphex (px[(gsize) (cy + 42) * w + cx + 40], ==, yellow);
  g_assert_cmphex (px[(gsize) (cy + 40) * w + cx + 42], ==, yellow);
#undef PX
  close_windows ();
}

/* REQ-0004-15: the grid starts as the settings file says; changes in the window never rewrite the file. */
static void
as14_grid_starts_as_settings_say (void)
{
  g_autofree char *path = easel_settings_file_path ();
  g_autofree char *before = NULL, *after = NULL;
  EaselWindow *win;
  EaselCanvas *canvas;

  write_settings ("[new-picture]\nwidth=300\nheight=200\n\n[view]\ngrid=on\ngrid-size=8\ngrid-axonometric=on\n");
  g_assert_true (g_file_get_contents (path, &before, NULL, NULL));
  win = start_window ();
  canvas = easel_window_get_canvas (win);
  g_assert_true (easel_canvas_get_grid_on (canvas));
  g_assert_cmpint (easel_canvas_get_grid_cell (canvas), ==, 8);
  g_assert_true (easel_canvas_get_grid_axonometric (canvas));
  g_assert_true (panel_checked (win, "grid"));

  /* A change in the window lasts until quitting and leaves the file alone. */
  g_action_group_activate_action (G_ACTION_GROUP (win), "grid", NULL);
  g_assert_false (easel_canvas_get_grid_on (canvas));
  g_assert_cmpint (easel_canvas_get_grid_cell (canvas), ==, 8);        /* the toggle keeps the cell size and the style */
  g_assert_true (easel_canvas_get_grid_axonometric (canvas));
  g_assert_true (g_file_get_contents (path, &after, NULL, NULL));
  g_assert_cmpstr (before, ==, after);
  close_windows ();

  /* Invalid values mean the defaults. */
  write_settings ("[new-picture]\nwidth=300\nheight=200\n\n[view]\ngrid=maybe\ngrid-size=0\n");
  win = start_window ();
  canvas = easel_window_get_canvas (win);
  g_assert_false (easel_canvas_get_grid_on (canvas));
  g_assert_cmpint (easel_canvas_get_grid_cell (canvas), ==, 1);
  g_assert_false (easel_canvas_get_grid_axonometric (canvas));
  close_windows ();
  write_settings ("[new-picture]\nwidth=300\nheight=200\nbackground=#FFFF00\n");
}

/* The first descendant of root that is of the given type. */
static GtkWidget *
find_of_type (GtkWidget *root, GType type)
{
  for (GtkWidget *child = gtk_widget_get_first_child (root); child; child = gtk_widget_get_next_sibling (child))
    {
      GtkWidget *found;

      if (G_TYPE_CHECK_INSTANCE_TYPE (child, type))
        return child;
      found = find_of_type (child, type);
      if (found != NULL)
        return found;
    }
  return NULL;
}

/* The one open alert dialog of the window, or NULL. */
static AdwAlertDialog *
open_alert (EaselWindow *win)
{
  AdwDialog *dialog = NULL;

  return count_alert_dialogs (GTK_WIDGET (win), &dialog) == 1 ? ADW_ALERT_DIALOG (dialog) : NULL;
}

/* REQ-0004-13: the Grid dialog. */
static void
as12_grid_dialog (void)
{
  static const char *bad[] = { "", "0", "abc", "1001", "-3", "1.5", "1 2", "99999999999999999999" };
  static const char *good[] = { "1000", " 16 ", "007", "1" };
  EaselWindow *win = start_window ();
  EaselCanvas *canvas = easel_window_get_canvas (win);
  AdwAlertDialog *dialog;
  GtkEditable *entry;
  GtkCheckButton *axon;

  wait_for_layout (win);
  g_action_group_activate_action (G_ACTION_GROUP (win), "grid-dialog", NULL);
  wait_for_dialogs (win, 1);
  dialog = open_alert (win);
  g_assert_nonnull (dialog);
  g_assert_cmpstr (adw_alert_dialog_get_heading (dialog), ==, "Grid");
  entry = GTK_EDITABLE (find_of_type (GTK_WIDGET (dialog), ADW_TYPE_ENTRY_ROW));
  axon = GTK_CHECK_BUTTON (find_of_type (GTK_WIDGET (dialog), GTK_TYPE_CHECK_BUTTON));
  g_assert_nonnull (entry);
  g_assert_nonnull (axon);
  g_assert_cmpstr (gtk_check_button_get_label (axon), ==, "Axonometric");
  g_assert_cmpstr (gtk_editable_get_text (entry), ==, "1");
  g_assert_false (gtk_check_button_get_active (axon));
  g_assert_cmpstr (adw_alert_dialog_get_default_response (dialog), ==, "apply");
  g_assert_cmpstr (adw_alert_dialog_get_close_response (dialog), ==, "cancel");
  g_assert_true (adw_alert_dialog_get_response_enabled (dialog, "apply"));

  /* The same action again, or the other dialog, does not open a second one. */
  g_action_group_activate_action (G_ACTION_GROUP (win), "grid-dialog", NULL);
  wait_for_layout (win);
  g_assert_nonnull (open_alert (win));

  /* Apply is insensitive while the cell size is not a whole number from 1 to 1000. */
  for (guint i = 0; i < G_N_ELEMENTS (bad); i++)
    {
      gtk_editable_set_text (entry, bad[i]);
      g_assert_false (adw_alert_dialog_get_response_enabled (dialog, "apply"));
    }
  for (guint i = 0; i < G_N_ELEMENTS (good); i++)
    {
      gtk_editable_set_text (entry, good[i]);
      g_assert_true (adw_alert_dialog_get_response_enabled (dialog, "apply"));
    }

  /* Esc (the close response) changes nothing. */
  gtk_editable_set_text (entry, "20");
  gtk_check_button_set_active (axon, TRUE);
  adw_dialog_close (ADW_DIALOG (dialog));
  wait_for_dialogs (win, 0);
  g_assert_cmpint (easel_canvas_get_grid_cell (canvas), ==, 1);
  g_assert_false (easel_canvas_get_grid_axonometric (canvas));

  /* Apply sets the cell size and the style, and never switches the grid on. */
  g_assert_false (panel_checked (win, "grid"));
  g_action_group_activate_action (G_ACTION_GROUP (win), "grid-dialog", NULL);
  wait_for_dialogs (win, 1);
  dialog = open_alert (win);
  entry = GTK_EDITABLE (find_of_type (GTK_WIDGET (dialog), ADW_TYPE_ENTRY_ROW));
  axon = GTK_CHECK_BUTTON (find_of_type (GTK_WIDGET (dialog), GTK_TYPE_CHECK_BUTTON));
  gtk_editable_set_text (entry, " 8 ");
  gtk_check_button_set_active (axon, TRUE);
  g_signal_emit_by_name (dialog, "response", "apply");
  adw_dialog_close (ADW_DIALOG (dialog));
  wait_for_dialogs (win, 0);
  g_assert_cmpint (easel_canvas_get_grid_cell (canvas), ==, 8);
  g_assert_true (easel_canvas_get_grid_axonometric (canvas));
  g_assert_false (easel_canvas_get_grid_on (canvas));
  g_assert_false (panel_checked (win, "grid"));

  /* The dialog starts with what is set; Enter in the field applies a valid value and ignores an invalid one. */
  g_action_group_activate_action (G_ACTION_GROUP (win), "grid-dialog", NULL);
  wait_for_dialogs (win, 1);
  dialog = open_alert (win);
  entry = GTK_EDITABLE (find_of_type (GTK_WIDGET (dialog), ADW_TYPE_ENTRY_ROW));
  axon = GTK_CHECK_BUTTON (find_of_type (GTK_WIDGET (dialog), GTK_TYPE_CHECK_BUTTON));
  g_assert_cmpstr (gtk_editable_get_text (entry), ==, "8");
  g_assert_true (gtk_check_button_get_active (axon));
  gtk_editable_set_text (entry, "abc");
  g_signal_emit_by_name (entry, "entry-activated");
  wait_for_layout (win);
  g_assert_nonnull (open_alert (win));                       /* still open, nothing applied */
  g_assert_cmpint (easel_canvas_get_grid_cell (canvas), ==, 8);
  gtk_editable_set_text (entry, "12");
  g_signal_emit_by_name (entry, "entry-activated");
  wait_for_dialogs (win, 0);
  g_assert_cmpint (easel_canvas_get_grid_cell (canvas), ==, 12);

  /* Applying with the grid on keeps it on. */
  g_action_group_activate_action (G_ACTION_GROUP (win), "grid", NULL);
  g_action_group_activate_action (G_ACTION_GROUP (win), "grid-dialog", NULL);
  wait_for_dialogs (win, 1);
  dialog = open_alert (win);
  entry = GTK_EDITABLE (find_of_type (GTK_WIDGET (dialog), ADW_TYPE_ENTRY_ROW));
  gtk_editable_set_text (entry, "4");
  g_signal_emit_by_name (dialog, "response", "apply");
  adw_dialog_close (ADW_DIALOG (dialog));
  wait_for_dialogs (win, 0);
  g_assert_true (easel_canvas_get_grid_on (canvas));
  g_assert_cmpint (easel_canvas_get_grid_cell (canvas), ==, 4);
  close_windows ();
}

static void
as35_input_ignored_before_picture (void)
{
  EaselWindow *win;
  EaselCanvas *canvas;
  AdwDialog *dialog = NULL;

  /* An auto picture waits for the canvas area, which a window that was never shown does not have. */
  write_settings ("[new-picture]\nwidth=auto\nheight=auto\n");
  win = easel_window_new (app);
  canvas = easel_window_get_canvas (win);
  g_assert_null (easel_window_get_picture (win));

  g_action_group_activate_action (G_ACTION_GROUP (win), "tool", g_variant_new_string ("pencil"));
  easel_canvas_press (canvas, 1, 3.5, 3.5);
  easel_canvas_motion (canvas, 9.5, 3.5);
  easel_canvas_release (canvas, 1);
  g_assert_false (easel_window_get_modified (win));
  g_assert_false (easel_canvas_is_stroking (canvas));

  g_action_group_activate_action (G_ACTION_GROUP (win), "save", NULL);
  g_assert_false (easel_window_save_to (win, "/nonexistent/x.png"));

  g_action_group_activate_action (G_ACTION_GROUP (win), "tool", g_variant_new_string ("pan"));
  easel_canvas_zoom_at (canvas, 1, 0, 0);
  g_assert_cmpint (easel_window_get_zoom (win), ==, 100);

  g_assert_cmpint (count_alert_dialogs (GTK_WIDGET (win), &dialog), ==, 0);
  gtk_window_destroy (GTK_WINDOW (win));
  write_settings ("[new-picture]\nwidth=300\nheight=200\nbackground=#FFFF00\n");
}

/* ---- 0004: changing the picture's size ---- */

static GtkEditable *
find_row (GtkWidget *root, const char *title)
{
  for (GtkWidget *child = gtk_widget_get_first_child (root); child; child = gtk_widget_get_next_sibling (child))
    {
      GtkEditable *found;

      if (ADW_IS_SPIN_ROW (child)
          && g_strcmp0 (adw_preferences_row_get_title (ADW_PREFERENCES_ROW (child)), title) == 0)
        return GTK_EDITABLE (child);
      found = find_row (child, title);
      if (found != NULL)
        return found;
    }
  return NULL;
}

static void
open_attributes (EaselWindow *win, AdwAlertDialog **dialog, GtkEditable **width, GtkEditable **height)
{
  g_action_group_activate_action (G_ACTION_GROUP (win), "attributes", NULL);
  wait_for_dialogs (win, 1);
  *dialog = open_alert (win);
  g_assert_nonnull (*dialog);
  *width = find_row (GTK_WIDGET (*dialog), "Width");
  *height = find_row (GTK_WIDGET (*dialog), "Height");
  g_assert_nonnull (*width);
  g_assert_nonnull (*height);
}

/* The picture, the canvas's tiles and the status bar all have the size. */
static void
assert_size (EaselWindow *win, int w, int h)
{
  const EaselImage *img = easel_window_get_picture (win);
  GdkTexture *tile = easel_canvas_get_tile_texture (easel_window_get_canvas (win), 0, 0);
  g_autofree char *text = g_strdup_printf ("%d × %d", w, h);

  g_assert_cmpint (img->width, ==, w);
  g_assert_cmpint (img->height, ==, h);
  g_assert_cmpint (gdk_texture_get_width (tile), ==, MIN (w, EASEL_CANVAS_TILE));
  g_assert_cmpint (gdk_texture_get_height (tile), ==, MIN (h, EASEL_CANVAS_TILE));
  g_assert_nonnull (strstr (easel_status_bar_get_text (easel_window_get_status_bar (win), EASEL_STATUS_SIZE), text));
}

/* Limits the address space to what the process uses now plus extra_mb, to make a big allocation fail. */
static gboolean
limit_memory (struct rlimit *old, long extra_mb)
{
  struct rlimit tight;
  FILE *f = fopen ("/proc/self/statm", "r");
  long pages = 0;

  if (f == NULL)
    return FALSE;
  if (fscanf (f, "%ld", &pages) != 1)
    pages = 0;
  fclose (f);
  if (pages <= 0 || getrlimit (RLIMIT_AS, old) != 0)
    return FALSE;
  tight = *old;
  tight.rlim_cur = (rlim_t) pages * (rlim_t) sysconf (_SC_PAGESIZE) + (rlim_t) extra_mb * 1024 * 1024;
  if (old->rlim_max != RLIM_INFINITY && tight.rlim_cur > old->rlim_max)
    tight.rlim_cur = old->rlim_max;
  return setrlimit (RLIMIT_AS, &tight) == 0;
}

static int
field_value (GtkEditable *row)
{
  return (int) adw_spin_row_get_value (ADW_SPIN_ROW (row));
}

/* Types text into a spin row and commits it, as leaving the field or pressing Enter does. */
static void
type_into (GtkEditable *row, const char *text)
{
  gtk_editable_set_text (row, text);
  adw_spin_row_update (ADW_SPIN_ROW (row));
}

/* REQ-0004-21, REQ-0004-20: the Attributes dialog. */
static void
as20_attributes_dialog (void)
{
  static const struct { const char *typed; int expected; } typed[] = {
    { "0", 1 }, { "-3", 1 }, { "10001", 10000 }, { "99999999999999999999", 10000 }, { "10000", 10000 },
    { "1", 1 }, { "640", 640 }, { "abc", 640 }, { "1.5", 640 }, { "", 640 }, { "007", 7 },
  };
  EaselWindow *win = start_window ();
  AdwAlertDialog *dialog;
  GtkEditable *width, *height;
  int area_w, area_h, dw, dh;

  wait_for_layout (win);
  open_attributes (win, &dialog, &width, &height);
  g_assert_cmpstr (adw_alert_dialog_get_heading (dialog), ==, "Attributes");
  g_assert_cmpstr (adw_alert_dialog_get_body (dialog), ==,
                   "New area is filled with the background color; a smaller size crops the picture.");
  g_assert_cmpint (field_value (width), ==, 300);
  g_assert_cmpint (field_value (height), ==, 200);
  g_assert_cmpstr (adw_alert_dialog_get_default_response (dialog), ==, "ok");
  g_assert_cmpstr (adw_alert_dialog_get_close_response (dialog), ==, "cancel");
  g_assert_true (adw_alert_dialog_get_response_enabled (dialog, "ok"));
  g_assert_nonnull (find_button (GTK_WIDGET (dialog), "Default"));

  /* Plus and minus step by 1; the fields run from 1 to 10000. */
  {
    GtkAdjustment *adj = adw_spin_row_get_adjustment (ADW_SPIN_ROW (width));

    g_assert_cmpfloat (gtk_adjustment_get_step_increment (adj), ==, 1);
    g_assert_cmpfloat (gtk_adjustment_get_lower (adj), ==, 1);
    g_assert_cmpfloat (gtk_adjustment_get_upper (adj), ==, 10000);
    gtk_adjustment_set_value (adj, gtk_adjustment_get_value (adj) + gtk_adjustment_get_step_increment (adj));
    gtk_adjustment_set_value (adw_spin_row_get_adjustment (ADW_SPIN_ROW (height)),
                              200 - gtk_adjustment_get_step_increment (adj));
    g_assert_cmpint (field_value (width), ==, 301);
    g_assert_cmpint (field_value (height), ==, 199);
  }

  /* Typed numbers are clamped, other text goes back; OK is always usable. */
  for (guint i = 0; i < G_N_ELEMENTS (typed); i++)
    {
      type_into (width, typed[i].typed);
      g_assert_cmpint (field_value (width), ==, typed[i].expected);
      type_into (height, typed[i].typed);
      g_assert_cmpint (field_value (height), ==, typed[i].expected);
      g_assert_true (adw_alert_dialog_get_response_enabled (dialog, "ok"));
    }

  /* Cancel and Esc change nothing. */
  type_into (width, "640");
  type_into (height, "480");
  adw_dialog_close (ADW_DIALOG (dialog));
  wait_for_dialogs (win, 0);
  assert_size (win, 300, 200);
  g_assert_false (easel_window_get_modified (win));

  /* Enter is OK; the added area has the current background color (REQ-0004-20), here the swapped one. */
  g_action_group_activate_action (G_ACTION_GROUP (win), "swap-colors", NULL);
  open_attributes (win, &dialog, &width, &height);
  type_into (width, "640");
  type_into (height, "480");
  g_signal_emit_by_name (gtk_editable_get_delegate (height), "activate");
  wait_for_dialogs (win, 0);
  assert_size (win, 640, 480);
  g_assert_cmpuint (easel_image_get (easel_window_get_picture (win), 0, 0), ==, 0xFFFFFF00u);
  g_assert_cmpuint (easel_image_get (easel_window_get_picture (win), 639, 479), ==, 0xFF000000u);
  g_assert_true (easel_window_get_modified (win));

  /* The OK response applies what the fields hold. */
  open_attributes (win, &dialog, &width, &height);
  g_assert_cmpint (field_value (width), ==, 640);
  type_into (width, "20");
  g_signal_emit_by_name (dialog, "response", "ok");
  adw_dialog_close (ADW_DIALOG (dialog));
  wait_for_dialogs (win, 0);
  assert_size (win, 20, 480);

  /* Default: the size a new picture would get now; here the settings say 300 x 200. */
  open_attributes (win, &dialog, &width, &height);
  g_signal_emit_by_name (find_button (GTK_WIDGET (dialog), "Default"), "clicked");
  g_assert_cmpint (field_value (width), ==, 300);
  g_assert_cmpint (field_value (height), ==, 200);
  adw_dialog_close (ADW_DIALOG (dialog));
  wait_for_dialogs (win, 0);
  assert_size (win, 20, 480);
  close_windows ();

  /* With auto the answer depends on the canvas area at that moment. */
  write_settings ("[new-picture]\nwidth=auto\nheight=auto\n");
  win = start_window ();
  wait_for_layout (win);
  canvas_area (win, &area_w, &area_h);
  /* The auto size is chosen in the area less its two margins (REQ-0001-02 as amended by 0009, REQ-0009-01). */
  area_w -= (int) (2 * EASEL_VIEW_MARGIN * easel_canvas_get_scale (easel_window_get_canvas (win)));
  area_h -= (int) (2 * EASEL_VIEW_MARGIN * easel_canvas_get_scale (easel_window_get_canvas (win)));
  easel_settings_resolve_size (&(EaselSettings) { .width = EASEL_SIZE_AUTO, .height = EASEL_SIZE_AUTO }, area_w, area_h,
                               &dw, &dh);
  open_attributes (win, &dialog, &width, &height);
  type_into (width, "5");
  type_into (height, "5");
  g_signal_emit_by_name (find_button (GTK_WIDGET (dialog), "Default"), "clicked");
  g_assert_cmpint (field_value (width), ==, dw);
  g_assert_cmpint (field_value (height), ==, dh);
  adw_dialog_close (ADW_DIALOG (dialog));
  wait_for_dialogs (win, 0);
  close_windows ();
  write_settings ("[new-picture]\nwidth=300\nheight=200\nbackground=#FFFF00\n");
}

/* REQ-0004-22: a resize is one step between strokes, and undo and redo go through it with the right size and pixels. */
static void
as22_undo_through_resize (void)
{
  EaselWindow *win = start_window ();
  const EaselImage *img = easel_window_get_picture (win);

  use_pencil (win);
  stroke (win, 10.5, 10.5, 10.5, 10.5);                        /* A: black */
  g_assert_cmpuint (easel_image_get (img, 10, 10), ==, 0xFF000000u);
  g_assert_cmpint (easel_window_resize_picture (win, 8, 8), ==, EASEL_RESIZE_OK);     /* the stroke is cropped away */
  assert_size (win, 8, 8);
  stroke (win, 5.5, 5.5, 5.5, 5.5);                            /* B */
  g_assert_cmpuint (easel_image_get (img, 5, 5), ==, 0xFF000000u);

  g_action_group_activate_action (G_ACTION_GROUP (win), "undo", NULL);   /* B */
  assert_size (win, 8, 8);
  g_assert_cmpuint (easel_image_get (img, 5, 5), ==, 0xFFFFFF00u);
  g_action_group_activate_action (G_ACTION_GROUP (win), "undo", NULL);   /* the resize */
  assert_size (win, 300, 200);
  g_assert_cmpuint (easel_image_get (img, 10, 10), ==, 0xFF000000u);
  g_action_group_activate_action (G_ACTION_GROUP (win), "undo", NULL);   /* A */
  g_assert_cmpuint (easel_image_get (img, 10, 10), ==, 0xFFFFFF00u);
  g_assert_false (action_enabled (win, "undo"));
  for (int i = 0; i < 3; i++)
    g_action_group_activate_action (G_ACTION_GROUP (win), "redo", NULL);
  assert_size (win, 8, 8);
  g_assert_cmpuint (easel_image_get (img, 5, 5), ==, 0xFF000000u);
  g_assert_false (action_enabled (win, "redo"));
  close_windows ();
}

/* REQ-0004-22: the modified mark follows the history across a resize. */
static void
as23_modified_across_resize (void)
{
  EaselWindow *win = start_window ();
  g_autofree char *dir = g_dir_make_tmp ("easel-ui-XXXXXX", NULL);
  g_autofree char *path = g_build_filename (dir, "a.png", NULL);

  g_assert_true (easel_window_save_to (win, path));
  g_assert_cmpstr (gtk_window_get_title (GTK_WINDOW (win)), ==, "a.png – Easel");
  g_assert_cmpint (easel_window_resize_picture (win, 120, 90), ==, EASEL_RESIZE_OK);
  g_assert_cmpstr (gtk_window_get_title (GTK_WINDOW (win)), ==, "• a.png – Easel");
  g_action_group_activate_action (G_ACTION_GROUP (win), "undo", NULL);
  g_assert_cmpstr (gtk_window_get_title (GTK_WINDOW (win)), ==, "a.png – Easel");
  g_assert_false (easel_window_get_modified (win));
  g_action_group_activate_action (G_ACTION_GROUP (win), "redo", NULL);
  g_assert_true (easel_window_get_modified (win));
  close_windows ();
}

/* REQ-0004-24: the same size adds no step. */
static void
as25_same_size_no_step (void)
{
  EaselWindow *win = start_window ();
  AdwAlertDialog *dialog;
  GtkEditable *width, *height;

  wait_for_layout (win);
  open_attributes (win, &dialog, &width, &height);
  g_signal_emit_by_name (dialog, "response", "ok");
  adw_dialog_close (ADW_DIALOG (dialog));
  wait_for_dialogs (win, 0);
  g_assert_false (action_enabled (win, "undo"));
  g_assert_false (easel_window_get_modified (win));
  g_assert_cmpint (easel_window_resize_picture (win, 300, 200), ==, EASEL_RESIZE_UNCHANGED);
  g_assert_false (action_enabled (win, "undo"));
  close_windows ();
}

/* REQ-0004-25: the scroll position is kept as far as the new size allows; the zoom and the window size stay. */
static void
as26_scroll_clamp_after_resize (void)
{
  EaselWindow *win;
  EaselCanvas *canvas;
  GtkScrolledWindow *scroller;
  GtkAdjustment *h, *v;
  double page_w, page_h;
  int win_w, win_h;

  write_settings ("[new-picture]\nwidth=2000\nheight=2000\n");
  win = start_window ();
  canvas = easel_window_get_canvas (win);
  scroller = GTK_SCROLLED_WINDOW (gtk_widget_get_ancestor (GTK_WIDGET (canvas), GTK_TYPE_SCROLLED_WINDOW));
  h = gtk_scrolled_window_get_hadjustment (scroller);
  v = gtk_scrolled_window_get_vadjustment (scroller);
  g_action_group_activate_action (G_ACTION_GROUP (win), "zoom-normal", NULL);
  wait_for_layout (win);
  page_w = gtk_adjustment_get_page_size (h);
  page_h = gtk_adjustment_get_page_size (v);
  gtk_adjustment_set_value (h, 1500);
  gtk_adjustment_set_value (v, 1500);
  wait_for_layout (win);
  win_w = gtk_widget_get_width (GTK_WIDGET (win));
  win_h = gtk_widget_get_height (GTK_WIDGET (win));

  g_assert_cmpint (easel_window_resize_picture (win, 1700, 1700), ==, EASEL_RESIZE_OK);
  wait_for_layout (win);
  g_assert_cmpint (easel_canvas_get_zoom (canvas), ==, 100);
  g_assert_cmpfloat_with_epsilon (gtk_adjustment_get_value (h), MAX (0, MIN (1500, 1700 + 2 * EASEL_VIEW_MARGIN - page_w)), 1.0);
  g_assert_cmpfloat_with_epsilon (gtk_adjustment_get_value (v), MAX (0, MIN (1500, 1700 + 2 * EASEL_VIEW_MARGIN - page_h)), 1.0);
  g_assert_cmpint (gtk_widget_get_width (GTK_WIDGET (win)), ==, win_w);
  g_assert_cmpint (gtk_widget_get_height (GTK_WIDGET (win)), ==, win_h);

  g_assert_cmpint (easel_window_resize_picture (win, 100, 100), ==, EASEL_RESIZE_OK);
  wait_for_layout (win);
  g_assert_cmpfloat (gtk_adjustment_get_value (h), ==, 0);
  g_assert_cmpfloat (gtk_adjustment_get_value (v), ==, 0);

  g_action_group_activate_action (G_ACTION_GROUP (win), "undo", NULL);
  g_action_group_activate_action (G_ACTION_GROUP (win), "undo", NULL);
  wait_for_layout (win);
  assert_size (win, 2000, 2000);
  g_assert_cmpint (easel_canvas_get_zoom (canvas), ==, 100);
  g_assert_cmpfloat (gtk_adjustment_get_value (h), ==, 0);
  close_windows ();
  write_settings ("[new-picture]\nwidth=300\nheight=200\nbackground=#FFFF00\n");
}

/* REQ-0004-26: a resize that cannot be allocated shows the dialog and changes nothing. */
static void
as27_error_dialog (void)
{
  EaselWindow *win = start_window ();
  AdwAlertDialog *dialog;
  struct rlimit old;
  EaselResizeResult r;

  g_assert_true (limit_memory (&old, 200));
  r = easel_window_resize_picture (win, 10000, 10000);
  g_assert_cmpint (setrlimit (RLIMIT_AS, &old), ==, 0);
  g_assert_cmpint (r, ==, EASEL_RESIZE_NO_MEMORY);
  wait_for_dialogs (win, 1);
  dialog = open_alert (win);
  g_assert_cmpstr (adw_alert_dialog_get_heading (dialog), ==, "Could Not Resize the Picture");
  g_assert_cmpstr (adw_alert_dialog_get_body (dialog), ==, "There is not enough memory for a picture of 10000 × 10000 pixels.");
  assert_size (win, 300, 200);
  g_assert_false (action_enabled (win, "undo"));
  g_assert_false (easel_window_get_modified (win));
  adw_dialog_close (ADW_DIALOG (dialog));
  wait_for_dialogs (win, 0);
  close_windows ();
}

/* REQ-0004-26 through the Attributes dialog: OK on a size that cannot be had leaves one dialog, the error. */
static void
as27_error_through_attributes (void)
{
  EaselWindow *win = start_window ();
  AdwAlertDialog *dialog;
  GtkEditable *width, *height;
  struct rlimit old;

  wait_for_layout (win);
  open_attributes (win, &dialog, &width, &height);
  type_into (width, "10000");
  type_into (height, "10000");
  g_assert_true (limit_memory (&old, 200));
  g_signal_emit_by_name (dialog, "response", "ok");
  g_assert_cmpint (setrlimit (RLIMIT_AS, &old), ==, 0);
  adw_dialog_close (ADW_DIALOG (dialog));
  wait_for_dialogs (win, 1);
  dialog = open_alert (win);
  g_assert_nonnull (dialog);
  g_assert_cmpstr (adw_alert_dialog_get_heading (dialog), ==, "Could Not Resize the Picture");
  assert_size (win, 300, 200);
  g_assert_false (action_enabled (win, "undo"));
  adw_dialog_close (ADW_DIALOG (dialog));
  wait_for_dialogs (win, 0);
  close_windows ();
}

/* AS-30: undo that cannot be allocated shows the same dialog and leaves picture and history position as they are. */
static void
as30_undo_error_dialog (void)
{
  EaselWindow *win = start_window ();
  AdwAlertDialog *dialog;
  struct rlimit old;

  g_assert_cmpint (easel_window_resize_picture (win, 10000, 5000), ==, EASEL_RESIZE_OK);
  g_assert_cmpint (easel_window_resize_picture (win, 1, 1), ==, EASEL_RESIZE_OK);
  g_assert_true (limit_memory (&old, 64));
  g_action_group_activate_action (G_ACTION_GROUP (win), "undo", NULL);
  g_assert_cmpint (setrlimit (RLIMIT_AS, &old), ==, 0);
  wait_for_dialogs (win, 1);
  dialog = open_alert (win);
  g_assert_nonnull (dialog);                       /* the error is shown; without it the heading below would crash */
  g_assert_cmpstr (adw_alert_dialog_get_heading (dialog), ==, "Could Not Resize the Picture");
  g_assert_cmpstr (adw_alert_dialog_get_body (dialog), ==, "There is not enough memory for a picture of 10000 × 5000 pixels.");
  assert_size (win, 1, 1);
  g_assert_true (action_enabled (win, "undo"));
  g_assert_false (action_enabled (win, "redo"));
  adw_dialog_close (ADW_DIALOG (dialog));
  wait_for_dialogs (win, 0);

  /* Memory is back: the same undo works. */
  g_action_group_activate_action (G_ACTION_GROUP (win), "undo", NULL);
  assert_size (win, 10000, 5000);
  g_assert_true (action_enabled (win, "redo"));
  close_windows ();
}

/* ---- 0004: the resize handles ---- */

/* Logical pixels per picture pixel at the canvas's zoom. */
static double
logical_per_pixel (EaselCanvas *canvas)
{
  return easel_canvas_get_zoom (canvas) / (100.0 * easel_canvas_get_scale (canvas));
}

static void
handle_center (EaselCanvas *canvas, EaselHandle handle, double *x, double *y)
{
  graphene_rect_t r;

  g_assert_true (easel_canvas_get_handle_rect (canvas, handle, &r));
  *x = r.origin.x + r.size.width / 2;
  *y = r.origin.y + r.size.height / 2;
}

/* One drag of a handle by dx, dy picture pixels, as the pointer would make it. */
static void
drag_handle (EaselWindow *win, EaselHandle handle, double dx, double dy)
{
  EaselCanvas *canvas = easel_window_get_canvas (win);
  double x, y, l = logical_per_pixel (canvas);

  handle_center (canvas, handle, &x, &y);
  easel_canvas_press (canvas, 1, x, y);
  easel_canvas_motion (canvas, x + dx * l, y + dy * l);
  easel_canvas_release (canvas, 1);
}

/* REQ-0004-16: where the handles are, and when. */
static void
as15_handles (void)
{
  EaselWindow *win = start_window ();
  EaselCanvas *canvas = easel_window_get_canvas (win);
  graphene_rect_t r;
  int min_pan, min_pencil, ignored;

  wait_for_layout (win);
  /* Pan, the start-up tool: no handles. */
  g_assert_false (easel_canvas_get_handle_rect (canvas, EASEL_HANDLE_RIGHT, &r));
  g_assert_cmpint (easel_canvas_handle_at (canvas, 302.5, 100), ==, EASEL_HANDLE_NONE);
  gtk_widget_measure (GTK_WIDGET (canvas), GTK_ORIENTATION_HORIZONTAL, -1, &min_pan, &ignored, NULL, NULL);

  use_pencil (win);
  wait_for_layout (win);
  /* 300x200 at 100 %: just outside the right middle, bottom middle and corner, 5x5 each, touching the picture. */
  g_assert_true (easel_canvas_get_handle_rect (canvas, EASEL_HANDLE_RIGHT, &r));
  g_assert_cmpfloat_with_epsilon (r.origin.x * easel_canvas_get_scale (canvas), 300, 0.01);
  g_assert_cmpfloat_with_epsilon ((r.origin.y + 2.5) * easel_canvas_get_scale (canvas), 100, 0.01);
  g_assert_cmpfloat_with_epsilon (r.size.width, 5, 0.001);
  g_assert_cmpfloat_with_epsilon (r.size.height, 5, 0.001);
  g_assert_true (easel_canvas_get_handle_rect (canvas, EASEL_HANDLE_BOTTOM, &r));
  g_assert_cmpfloat_with_epsilon ((r.origin.x + 2.5) * easel_canvas_get_scale (canvas), 150, 0.01);
  g_assert_cmpfloat_with_epsilon (r.origin.y * easel_canvas_get_scale (canvas), 200, 0.01);
  g_assert_true (easel_canvas_get_handle_rect (canvas, EASEL_HANDLE_CORNER, &r));
  g_assert_cmpfloat_with_epsilon (r.origin.x * easel_canvas_get_scale (canvas), 300, 0.01);
  g_assert_cmpfloat_with_epsilon (r.origin.y * easel_canvas_get_scale (canvas), 200, 0.01);
  g_assert_cmpfloat_with_epsilon (r.size.width, 5, 0.001);

  /* The hit area is 9x9 around the center. */
  {
    double rx, ry, bx, by, cx, cy;

    handle_center (canvas, EASEL_HANDLE_RIGHT, &rx, &ry);
    handle_center (canvas, EASEL_HANDLE_BOTTOM, &bx, &by);
    handle_center (canvas, EASEL_HANDLE_CORNER, &cx, &cy);
    g_assert_cmpint (easel_canvas_handle_at (canvas, rx, ry), ==, EASEL_HANDLE_RIGHT);
    g_assert_cmpint (easel_canvas_handle_at (canvas, rx + 4.4, ry - 4.4), ==, EASEL_HANDLE_RIGHT);
    g_assert_cmpint (easel_canvas_handle_at (canvas, rx + 4.6, ry), ==, EASEL_HANDLE_NONE);
    g_assert_cmpint (easel_canvas_handle_at (canvas, rx, ry + 4.6), ==, EASEL_HANDLE_NONE);
    g_assert_cmpint (easel_canvas_handle_at (canvas, 10, 10), ==, EASEL_HANDLE_NONE);   /* inside the picture */
    g_assert_cmpint (easel_canvas_handle_at (canvas, bx, by), ==, EASEL_HANDLE_BOTTOM);
    g_assert_cmpint (easel_canvas_handle_at (canvas, cx, cy), ==, EASEL_HANDLE_CORNER);
  }

  /* The scrollable area includes them: they lie in the margin around the picture (REQ-0004-16 as amended by 0009,
     REQ-0009-01), which is there whether they show or not, and reaches past them (5 + 2 pixels). */
  gtk_widget_measure (GTK_WIDGET (canvas), GTK_ORIENTATION_HORIZONTAL, -1, &min_pencil, &ignored, NULL, NULL);
  g_assert_cmpint (min_pencil, ==, min_pan);
  g_assert_cmpint (min_pencil, ==, 300 + 2 * EASEL_VIEW_MARGIN);
  g_assert_cmpint (5 + 2, <=, EASEL_VIEW_MARGIN);

  /* Where the hit areas overlap, the corner wins: an 8x8 picture. */
  g_assert_cmpint (easel_window_resize_picture (win, 8, 8), ==, EASEL_RESIZE_OK);
  wait_for_layout (win);
  {
    double rx, ry, cx, cy;

    handle_center (canvas, EASEL_HANDLE_RIGHT, &rx, &ry);
    handle_center (canvas, EASEL_HANDLE_CORNER, &cx, &cy);
    g_assert_cmpint (easel_canvas_handle_at (canvas, rx, (ry + cy) / 2), ==, EASEL_HANDLE_CORNER);
    g_assert_cmpint (easel_canvas_handle_at (canvas, rx, ry - 3.9), ==, EASEL_HANDLE_RIGHT);
  }

  /* Back to Pan: hidden again. */
  g_action_group_activate_action (G_ACTION_GROUP (win), "tool", g_variant_new_string ("pan"));
  g_assert_false (easel_canvas_get_handle_rect (canvas, EASEL_HANDLE_CORNER, &r));
  close_windows ();
}

/* REQ-0004-18, REQ-0004-24: dragging a handle resizes by the movement. */
static void
as17_handle_drag (void)
{
  EaselWindow *win = start_window ();
  EaselCanvas *canvas = easel_window_get_canvas (win);
  double x, y;
  int ow = 0, oh = 0;

  use_pencil (win);
  wait_for_layout (win);

  /* While dragging the outline shows the new size and the picture waits. */
  handle_center (canvas, EASEL_HANDLE_RIGHT, &x, &y);
  easel_canvas_press (canvas, 1, x, y);
  g_assert_true (easel_canvas_is_resizing (canvas));
  g_assert_true (easel_canvas_get_resize_outline (canvas, &ow, &oh));
  g_assert_cmpint (ow, ==, 300);
  easel_canvas_motion (canvas, x + 100.4 / easel_canvas_get_scale (canvas), y + 30);
  g_assert_true (easel_canvas_get_resize_outline (canvas, &ow, &oh));
  g_assert_cmpint (ow, ==, 400);
  g_assert_cmpint (oh, ==, 200);
  assert_size (win, 300, 200);
  easel_canvas_release (canvas, 1);
  g_assert_false (easel_canvas_is_resizing (canvas));
  g_assert_false (easel_canvas_get_resize_outline (canvas, &ow, &oh));
  assert_size (win, 400, 200);
  wait_for_layout (win);

  drag_handle (win, EASEL_HANDLE_BOTTOM, 20, -50);
  assert_size (win, 400, 150);
  wait_for_layout (win);
  drag_handle (win, EASEL_HANDLE_CORNER, 100, 150);
  assert_size (win, 500, 300);
  wait_for_layout (win);
  drag_handle (win, EASEL_HANDLE_CORNER, -100000, -100000);
  assert_size (win, 1, 1);
  wait_for_layout (win);
  drag_handle (win, EASEL_HANDLE_CORNER, 100000, 100000);
  assert_size (win, 10000, 10000);
  /* One step each: five resizes, then nothing more to undo. */
  for (int i = 0; i < 5; i++)
    g_action_group_activate_action (G_ACTION_GROUP (win), "undo", NULL);
  assert_size (win, 300, 200);
  g_assert_false (action_enabled (win, "undo"));
  wait_for_layout (win);

  /* A click, or a drag that ends where it started, adds no step. */
  handle_center (canvas, EASEL_HANDLE_CORNER, &x, &y);
  easel_canvas_press (canvas, 1, x, y);
  easel_canvas_release (canvas, 1);
  drag_handle (win, EASEL_HANDLE_RIGHT, 0, 0);
  {
    double l = logical_per_pixel (canvas);

    easel_canvas_press (canvas, 1, x, y);
    easel_canvas_motion (canvas, x + 40 * l, y + 40 * l);
    easel_canvas_motion (canvas, x, y);
    easel_canvas_release (canvas, 1);
  }
  assert_size (win, 300, 200);
  g_assert_false (action_enabled (win, "undo"));
  g_assert_false (easel_window_get_modified (win));

  /* The same at another zoom: the movement is in picture pixels. */
  g_action_group_activate_action (G_ACTION_GROUP (win), "zoom-large", NULL);
  wait_for_layout (win);
  drag_handle (win, EASEL_HANDLE_RIGHT, 40.4, 0);
  assert_size (win, 340, 200);
  /* Drawing is not hindered: a press inside the picture is a stroke, one on a handle is not. */
  g_assert_cmpint (easel_canvas_handle_at (canvas, 10, 10), ==, EASEL_HANDLE_NONE);
  close_windows ();
}

static void
press_escape (EaselWindow *win)
{
  g_autoptr (GListModel) controllers = gtk_widget_observe_controllers (GTK_WIDGET (win));

  for (guint i = 0; i < g_list_model_get_n_items (controllers); i++)
    {
      g_autoptr (GtkEventController) c = g_list_model_get_item (controllers, i);
      gboolean handled = FALSE;

      if (GTK_IS_EVENT_CONTROLLER_KEY (c))
        g_signal_emit_by_name (c, "key-pressed", GDK_KEY_Escape, 0, 0, &handled);
    }
}

/* REQ-0004-19: what cancels a handle drag, and what is ignored until the button is released. */
static void
as18_drag_cancel_rules (void)
{
  EaselWindow *win = start_window ();
  EaselCanvas *canvas = easel_window_get_canvas (win);
  g_autofree char *dir = g_dir_make_tmp ("easel-ui-XXXXXX", NULL);
  g_autofree char *path = g_build_filename (dir, "a.png", NULL);
  double x, y;
  gboolean grid_before;

  use_pencil (win);
  stroke (win, 5.5, 5.5, 20.5, 5.5);              /* something to undo */
  wait_for_layout (win);
  grid_before = panel_checked (win, "grid");
  handle_center (canvas, EASEL_HANDLE_CORNER, &x, &y);

  /* Esc cancels; the release afterwards changes nothing. */
  easel_canvas_press (canvas, 1, x, y);
  easel_canvas_motion (canvas, x + 50, y + 50);
  press_escape (win);
  g_assert_false (easel_canvas_is_resizing (canvas));
  g_assert_false (easel_canvas_get_resize_outline (canvas, &(int) { 0 }, &(int) { 0 }));
  /* Until the release: undo, zoom and the grid shortcut do nothing. */
  g_action_group_activate_action (G_ACTION_GROUP (win), "undo", NULL);
  g_action_group_activate_action (G_ACTION_GROUP (win), "zoom-large", NULL);
  g_action_group_activate_action (G_ACTION_GROUP (win), "grid", NULL);
  g_assert_cmpint (easel_canvas_get_zoom (canvas), ==, 100);
  g_assert_cmpuint (easel_image_get (easel_window_get_picture (win), 10, 5), ==, 0xFF000000u);
  g_assert_cmpint (panel_checked (win, "grid"), ==, grid_before);
  easel_canvas_motion (canvas, x + 80, y + 80);
  easel_canvas_release (canvas, 1);
  assert_size (win, 300, 200);

  /* The other button cancels, also while the pointer is far away. */
  easel_canvas_press (canvas, 1, x, y);
  easel_canvas_motion (canvas, x - 5000, y + 5000);
  easel_canvas_press (canvas, 3, x - 5000, y + 5000);
  g_assert_false (easel_canvas_is_resizing (canvas));
  easel_canvas_release (canvas, 3);
  easel_canvas_release (canvas, 1);
  assert_size (win, 300, 200);

  /* A press of the other button on a handle starts nothing. */
  easel_canvas_press (canvas, 3, x, y);
  g_assert_false (easel_canvas_is_resizing (canvas));
  easel_canvas_release (canvas, 3);

  /* Losing the focus cancels. */
  easel_canvas_press (canvas, 1, x, y);
  easel_canvas_motion (canvas, x + 30, y + 30);
  easel_canvas_reset_pointer (canvas);
  g_assert_false (easel_canvas_is_resizing (canvas));
  easel_canvas_release (canvas, 1);
  assert_size (win, 300, 200);

  /* Save cancels: the picture is saved at its old size. */
  g_assert_true (easel_window_save_to (win, path));
  easel_canvas_press (canvas, 1, x, y);
  easel_canvas_motion (canvas, x + 30, y + 30);
  g_action_group_activate_action (G_ACTION_GROUP (win), "save", NULL);
  g_assert_false (easel_canvas_is_resizing (canvas));
  easel_canvas_release (canvas, 1);
  assert_size (win, 300, 200);
  g_assert_false (easel_window_get_modified (win));

  /* While the drag is in progress, undo and the view actions do nothing, and the tool cannot change. */
  easel_canvas_press (canvas, 1, x, y);
  easel_canvas_motion (canvas, x + 30, y + 30);
  g_action_group_activate_action (G_ACTION_GROUP (win), "undo", NULL);
  g_action_group_activate_action (G_ACTION_GROUP (win), "zoom-large", NULL);
  g_action_group_activate_action (G_ACTION_GROUP (win), "tool", g_variant_new_string ("pan"));
  g_assert_true (easel_canvas_is_resizing (canvas));
  g_assert_cmpint (easel_canvas_get_zoom (canvas), ==, 100);
  g_assert_cmpint (easel_window_get_tool (win), ==, EASEL_TOOL_PENCIL);
  g_assert_cmpuint (easel_image_get (easel_window_get_picture (win), 10, 5), ==, 0xFF000000u);   /* undo did nothing */
  easel_canvas_release (canvas, 1);
  assert_size (win, 330, 230);
  close_windows ();
}

/* ---- 0006: the freehand tools ---- */

/* REQ-0006-01: the toolbox is two columns of toggle buttons, Pan and Eraser, Pencil and Brush, Airbrush. */
static void
as1_toolbox_and_options (void)
{
  static const struct { const char *name, *tip; int col, row; EaselTool tool; } tools[] = {
    { "pan", "Pan", 0, 0, EASEL_TOOL_PAN }, { "eraser", "Eraser", 0, 1, EASEL_TOOL_ERASER },
    { "pencil", "Pencil", 1, 2, EASEL_TOOL_PENCIL }, { "brush", "Brush", 0, 3, EASEL_TOOL_BRUSH },
    { "airbrush", "Airbrush", 1, 3, EASEL_TOOL_AIRBRUSH },
  };
  EaselWindow *win = start_window ();
  GtkToggleButton *buttons[5];
  graphene_rect_t first, r;

  wait_for_layout (win);
  for (guint i = 0; i < G_N_ELEMENTS (tools); i++)
    {
      buttons[i] = find_tool_button (GTK_WIDGET (win), tools[i].name);
      g_assert_nonnull (buttons[i]);
      g_assert_cmpstr (gtk_widget_get_tooltip_text (GTK_WIDGET (buttons[i])), ==, tools[i].tip);
      g_assert_cmpstr (gtk_button_get_icon_name (GTK_BUTTON (buttons[i])), !=, NULL);
    }
  /* The grid: the second column is to the right of the first, rows are below each other, in this order. */
  first = bounds_in (GTK_WIDGET (buttons[0]), GTK_WIDGET (win));
  for (guint i = 1; i < G_N_ELEMENTS (tools); i++)
    {
      r = bounds_in (GTK_WIDGET (buttons[i]), GTK_WIDGET (win));
      if (tools[i].col == 0)
        g_assert_cmpfloat_with_epsilon (r.origin.x, first.origin.x, 1.0);
      else
        g_assert_cmpfloat (r.origin.x, >, first.origin.x + first.size.width - 1);
      if (tools[i].row > tools[i - 1].row)
        g_assert_cmpfloat (r.origin.y, >, bounds_in (GTK_WIDGET (buttons[i - 1]), GTK_WIDGET (win)).origin.y + 1);
      else
        g_assert_cmpfloat_with_epsilon (r.origin.y, bounds_in (GTK_WIDGET (buttons[i - 1]), GTK_WIDGET (win)).origin.y, 1.0);
    }
  /* Each tool can be chosen, shows selected, and the others do not. */
  for (guint i = 0; i < G_N_ELEMENTS (tools); i++)
    {
      g_signal_emit_by_name (buttons[i], "clicked");
      g_assert_cmpint (easel_window_get_tool (win), ==, tools[i].tool);
      for (guint j = 0; j < G_N_ELEMENTS (tools); j++)
        g_assert_true (gtk_toggle_button_get_active (buttons[j]) == (i == j));
    }
  close_windows ();
}

/* The widget under root whose tooltip is text, or NULL. */
static GtkWidget *
find_by_tooltip (GtkWidget *root, const char *text)
{
  for (GtkWidget *child = gtk_widget_get_first_child (root); child; child = gtk_widget_get_next_sibling (child))
    {
      GtkWidget *found;

      if (g_strcmp0 (gtk_widget_get_tooltip_text (child), text) == 0)
        return child;
      found = find_by_tooltip (child, text);
      if (found != NULL)
        return found;
    }
  return NULL;
}

/* Is there a widget with this tooltip that is on screen (mapped)? */
static gboolean
on_screen (EaselWindow *win, const char *tip)
{
  GtkWidget *w = find_by_tooltip (GTK_WIDGET (win), tip);

  return w != NULL && gtk_widget_get_mapped (w);
}

/* REQ-0006-02 (and REQ-0007-02 for Pick Color and Fill): the active tool has no options: no option button of any tool
   is on screen, and the options, their line included, take no height. */
static void
assert_no_options_shown (EaselWindow *win)
{
  static const char *const any[] = { "Circle, 4 pixels", "Spray, 16 pixels", "Eraser, 8 pixels",
                                     "Opaque: erase to the background color", "200 %" };

  for (guint i = 0; i < G_N_ELEMENTS (any); i++)
    g_assert_false (on_screen (win, any[i]));
  g_assert_cmpint (gtk_widget_get_height (GTK_WIDGET (easel_window_get_tool_options (win))), ==, 0);
}

static void
use_tool (EaselWindow *win, const char *tool)
{
  g_action_group_activate_action (G_ACTION_GROUP (win), "tool", g_variant_new_string (tool));
}

/* The preset buttons of a tool, by their tooltips, in the order of the spec; the one that is on is returned (or -1). */
static int
check_presets (EaselWindow *win, const char *const *tips, int n, int columns)
{
  int on = -1;
  graphene_rect_t prev = { { 0, 0 }, { 0, 0 } };

  for (int i = 0; i < n; i++)
    {
      GtkWidget *b = find_by_tooltip (GTK_WIDGET (win), tips[i]);
      graphene_rect_t r;

      g_assert_nonnull (b);
      g_assert_true (GTK_IS_TOGGLE_BUTTON (b));
      g_assert_true (gtk_widget_get_mapped (b));
      r = bounds_in (b, GTK_WIDGET (win));
      if (i > 0)
        {
          /* Row by row: the next one is to the right in the same row, or the first of the next row. */
          if (i % columns != 0)
            {
              g_assert_cmpfloat_with_epsilon (r.origin.y, prev.origin.y, 1.0);
              g_assert_cmpfloat (r.origin.x, >, prev.origin.x + 1);
            }
          else
            g_assert_cmpfloat (r.origin.y, >, prev.origin.y + 1);
        }
      prev = r;
      if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (b)))
        {
          g_assert_cmpint (on, ==, -1);                      /* only one is on */
          on = i;
        }
    }
  return on;
}

static const char *const brush_tips[12] = {
  "Circle, 7 pixels", "Circle, 4 pixels", "Circle, 1 pixel", "Square, 8 pixels", "Square, 5 pixels", "Square, 2 pixels",
  "Slash, 8 pixels", "Slash, 5 pixels", "Slash, 2 pixels", "Backslash, 8 pixels", "Backslash, 5 pixels", "Backslash, 2 pixels",
};
static const char *const airbrush_tips[3] = { "Spray, 9 pixels", "Spray, 16 pixels", "Spray, 24 pixels" };
static const char *const eraser_tips[4] = { "Eraser, 4 pixels", "Eraser, 6 pixels", "Eraser, 8 pixels", "Eraser, 10 pixels" };
static const char *const eraser_mode_tips[2] = { "Opaque: erase to the background color", "Transparent: erase to transparency" };

/* REQ-0006-02: the options panel of each tool, its defaults, and when it shows. */
static void
as1_options_panel (void)
{
  EaselWindow *win = start_window ();
  GtkWidget *brush_default;

  wait_for_layout (win);
  /* Pan and Pencil have no options: no preset button is on screen. */
  g_assert_false (on_screen (win, "Circle, 4 pixels"));
  assert_no_options_shown (win);                                 /* nor its line */
  use_tool (win, "pencil");
  wait_for_layout (win);
  g_assert_false (on_screen (win, "Circle, 4 pixels"));
  assert_no_options_shown (win);

  /* The Brush: twelve presets in two columns, the 4-pixel circle on. */
  use_tool (win, "brush");
  wait_for_layout (win);
  g_assert_cmpint (check_presets (win, brush_tips, 12, 2), ==, 1);
  brush_default = find_by_tooltip (GTK_WIDGET (win), "Circle, 4 pixels");
  g_assert_true (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (brush_default)));

  /* The Airbrush: three, the 16 on. */
  use_tool (win, "airbrush");
  wait_for_layout (win);
  g_assert_cmpint (check_presets (win, airbrush_tips, 3, 2), ==, 1);
  g_assert_false (on_screen (win, "Circle, 4 pixels"));

  /* The Eraser: four sizes (8 on), a line, and the two modes (Opaque on). */
  use_tool (win, "eraser");
  wait_for_layout (win);
  g_assert_cmpint (check_presets (win, eraser_tips, 4, 2), ==, 2);
  g_assert_cmpint (check_presets (win, eraser_mode_tips, 2, 1), ==, 0);
  {
    GtkWidget *last_size = find_by_tooltip (GTK_WIDGET (win), "Eraser, 10 pixels");
    GtkWidget *opaque = find_by_tooltip (GTK_WIDGET (win), eraser_mode_tips[0]);
    graphene_rect_t rs = bounds_in (last_size, GTK_WIDGET (win)), ro = bounds_in (opaque, GTK_WIDGET (win));

    g_assert_cmpfloat (ro.origin.y, >, rs.origin.y + rs.size.height);
    /* A separator, drawn on screen, lies between the last size and the modes. */
    {
      gboolean line = FALSE;

      for (GtkWidget *w = gtk_widget_get_first_child (gtk_widget_get_ancestor (opaque, GTK_TYPE_BOX)); w;
           w = gtk_widget_get_next_sibling (w))
        if (GTK_IS_SEPARATOR (w) && gtk_widget_get_mapped (w))
          {
            graphene_rect_t rl = bounds_in (w, GTK_WIDGET (win));

            line = line || (rl.origin.y > rs.origin.y + rs.size.height - 1 && rl.origin.y < ro.origin.y + 1);
          }
      g_assert_true (line);
    }
  }

  /* Hiding the Tool Box hides the options with it, showing it brings them back. */
  g_action_group_change_action_state (G_ACTION_GROUP (win), "show-toolbox", g_variant_new_boolean (FALSE));
  wait_for_layout (win);
  g_assert_false (on_screen (win, "Eraser, 8 pixels"));
  g_action_group_change_action_state (G_ACTION_GROUP (win), "show-toolbox", g_variant_new_boolean (TRUE));
  wait_for_layout (win);
  g_assert_cmpint (check_presets (win, eraser_tips, 4, 2), ==, 2);

  /* Back to Pan: gone again. */
  use_tool (win, "pan");
  wait_for_layout (win);
  g_assert_false (on_screen (win, "Eraser, 8 pixels"));
  assert_no_options_shown (win);
  close_windows ();
}

/* REQ-0006-03: each tool keeps its own selected preset while the session lasts, and the defaults come back at a restart. */
static void
as2_presets_kept (void)
{
  EaselWindow *win = start_window ();

  wait_for_layout (win);
  use_tool (win, "brush");
  wait_for_layout (win);
  g_signal_emit_by_name (find_by_tooltip (GTK_WIDGET (win), "Slash, 5 pixels"), "clicked");
  g_assert_cmpint (check_presets (win, brush_tips, 12, 2), ==, 7);
  use_tool (win, "eraser");
  wait_for_layout (win);
  g_signal_emit_by_name (find_by_tooltip (GTK_WIDGET (win), "Eraser, 4 pixels"), "clicked");
  g_signal_emit_by_name (find_by_tooltip (GTK_WIDGET (win), eraser_mode_tips[1]), "clicked");
  g_assert_cmpint (check_presets (win, eraser_tips, 4, 2), ==, 0);
  g_assert_cmpint (check_presets (win, eraser_mode_tips, 2, 1), ==, 1);

  /* What the canvas will use: the footprint and the mode of each tool, kept apart. */
  {
    EaselToolOptions *o = easel_window_get_tool_options (win);

    g_assert_true (easel_tool_options_get_footprint (o, EASEL_TOOL_BRUSH).shape == EASEL_SHAPE_SLASH
                   && easel_tool_options_get_footprint (o, EASEL_TOOL_BRUSH).size == 5);
    g_assert_true (easel_tool_options_get_footprint (o, EASEL_TOOL_ERASER).shape == EASEL_SHAPE_SQUARE
                   && easel_tool_options_get_footprint (o, EASEL_TOOL_ERASER).size == 4);
    g_assert_cmpint (easel_tool_options_get_footprint (o, EASEL_TOOL_AIRBRUSH).size, ==, 16);
    g_assert_true (easel_tool_options_get_transparent (o));
  }

  /* Away and back: the same presets, the size and the mode kept apart. */
  use_tool (win, "pencil");
  use_tool (win, "airbrush");
  wait_for_layout (win);
  g_signal_emit_by_name (find_by_tooltip (GTK_WIDGET (win), "Spray, 24 pixels"), "clicked");
  use_tool (win, "brush");
  wait_for_layout (win);
  g_assert_cmpint (check_presets (win, brush_tips, 12, 2), ==, 7);
  use_tool (win, "eraser");
  wait_for_layout (win);
  g_assert_cmpint (check_presets (win, eraser_tips, 4, 2), ==, 0);
  g_assert_cmpint (check_presets (win, eraser_mode_tips, 2, 1), ==, 1);
  use_tool (win, "airbrush");
  wait_for_layout (win);
  g_assert_cmpint (check_presets (win, airbrush_tips, 3, 2), ==, 2);

  /* A new window (a restart) has the defaults again. */
  close_windows ();
  win = start_window ();
  wait_for_layout (win);
  use_tool (win, "brush");
  wait_for_layout (win);
  g_assert_cmpint (check_presets (win, brush_tips, 12, 2), ==, 1);
  use_tool (win, "eraser");
  wait_for_layout (win);
  g_assert_cmpint (check_presets (win, eraser_tips, 4, 2), ==, 2);
  g_assert_cmpint (check_presets (win, eraser_mode_tips, 2, 1), ==, 0);
  close_windows ();
}

/* ---- 0006: drawing with the Brush and the Eraser ---- */

static int
count_color (const EaselImage *img, uint32_t color)
{
  int n = 0;

  for (int y = 0; y < img->height; y++)
    for (int x = 0; x < img->width; x++)
      n += easel_image_get (img, x, y) == color;
  return n;
}

/* One stroke with the given button, as the pointer would make it, in picture pixels at 100 % (the center of each pixel). */
static void
stroke_with (EaselWindow *win, guint button, double x0, double y0, double x1, double y1)
{
  EaselCanvas *canvas = easel_window_get_canvas (win);

  easel_canvas_press (canvas, button, x0 + 0.5, y0 + 0.5);
  easel_canvas_motion (canvas, x1 + 0.5, y1 + 0.5);
  easel_canvas_release (canvas, button);
}

static void
choose_preset (EaselWindow *win, const char *tip)
{
  GtkWidget *b = find_by_tooltip (GTK_WIDGET (win), tip);

  g_assert_nonnull (b);
  g_signal_emit_by_name (b, "clicked");
}

/* AS-4: the Brush paints its footprint on a click and along a drag, with either button's color, one undo step each. */
static void
as4_brush_on_canvas (void)
{
  EaselWindow *win = start_window ();
  const EaselImage *img = easel_window_get_picture (win);
  uint32_t bg = easel_window_get_background (win), fg = easel_window_get_foreground (win);

  wait_for_layout (win);
  use_tool (win, "brush");
  wait_for_layout (win);
  choose_preset (win, "Circle, 7 pixels");

  /* A click: the 37 pixels of the circle, the pointer's pixel at column 3, row 3. */
  stroke_with (win, 1, 20, 20, 20, 20);
  g_assert_cmpint (count_color (img, fg), ==, 37);
  g_assert_cmphex (easel_image_get (img, 20, 17), ==, fg);
  g_assert_cmphex (easel_image_get (img, 23, 20), ==, fg);
  g_assert_cmphex (easel_image_get (img, 17, 17), ==, bg);
  g_assert_true (easel_window_get_modified (win));
  g_action_group_activate_action (G_ACTION_GROUP (win), "undo", NULL);       /* one step */
  g_assert_cmpint (count_color (img, fg), ==, 0);
  g_assert_false (action_enabled (win, "undo"));

  /* A drag, 7 pixels thick; the secondary button paints with the background color (here swapped to see it). */
  stroke_with (win, 1, 10, 60, 40, 60);
  g_assert_cmphex (easel_image_get (img, 25, 57), ==, fg);
  g_assert_cmphex (easel_image_get (img, 25, 63), ==, fg);
  g_assert_cmphex (easel_image_get (img, 25, 56), ==, bg);
  g_assert_cmphex (easel_image_get (img, 25, 64), ==, bg);
  g_assert_cmphex (easel_image_get (img, 7, 60), ==, fg);                  /* the footprint reaches 3 left of the start */
  g_assert_cmphex (easel_image_get (img, 6, 60), ==, bg);
  /* A stroke that starts outside the picture paints only the inside. */
  stroke_with (win, 1, -5, 100, 5, 100);
  g_assert_cmphex (easel_image_get (img, 2, 100), ==, fg);
  g_assert_cmphex (easel_image_get (img, 8, 100), ==, fg);                 /* 5 plus the 3 the footprint reaches */
  g_assert_cmphex (easel_image_get (img, 9, 100), ==, bg);

  /* The secondary button paints with the background color: after a swap that is the former foreground (black). */
  g_action_group_activate_action (G_ACTION_GROUP (win), "swap-colors", NULL);
  g_assert_cmphex (easel_window_get_background (win), ==, fg);
  stroke_with (win, 3, 10, 80, 20, 80);
  g_assert_cmphex (easel_image_get (img, 15, 80), ==, fg);
  g_action_group_activate_action (G_ACTION_GROUP (win), "undo", NULL);
  g_assert_cmphex (easel_image_get (img, 15, 80), ==, bg);                 /* one step took all of it */
  g_assert_cmphex (easel_image_get (img, 25, 57), ==, fg);                 /* the first drag is still there */
  close_windows ();
}

/* AS-7: the Eraser paints its square with the background color, or with transparency; a click is one square. */
static void
as7_eraser_on_canvas (void)
{
  EaselWindow *win = start_window ();
  const EaselImage *img = easel_window_get_picture (win);
  uint32_t bg = easel_window_get_background (win), fg = easel_window_get_foreground (win);

  wait_for_layout (win);
  /* Black everything with a big brush first, so that erasing can be seen. */
  use_tool (win, "brush");
  wait_for_layout (win);
  choose_preset (win, "Square, 8 pixels");
  for (int y = 0; y <= 100; y += 8)
    stroke_with (win, 1, 0, y, 150, y);
  use_tool (win, "eraser");
  wait_for_layout (win);

  /* Opaque, 8: a click is an 8 x 8 square with the pointer pixel at column 3, row 3. */
  stroke_with (win, 1, 30, 30, 30, 30);
  g_assert_cmphex (easel_image_get (img, 27, 27), ==, bg);
  g_assert_cmphex (easel_image_get (img, 34, 34), ==, bg);
  g_assert_cmphex (easel_image_get (img, 26, 30), ==, fg);
  g_assert_cmphex (easel_image_get (img, 35, 30), ==, fg);
  g_action_group_activate_action (G_ACTION_GROUP (win), "undo", NULL);
  g_assert_cmphex (easel_image_get (img, 30, 30), ==, fg);

  /* Transparent: the pixels become #00000000. */
  choose_preset (win, "Transparent: erase to transparency");
  stroke_with (win, 1, 60, 50, 80, 50);
  g_assert_cmphex (easel_image_get (img, 70, 50), ==, 0x00000000u);
  g_assert_cmphex (easel_image_get (img, 70, 47), ==, 0x00000000u);       /* rows 47 to 54: the pointer's row is 50 */
  g_assert_cmphex (easel_image_get (img, 70, 46), ==, fg);
  g_action_group_activate_action (G_ACTION_GROUP (win), "undo", NULL);
  g_assert_cmphex (easel_image_get (img, 70, 50), ==, fg);
  close_windows ();
}

/* AS-8: the color eraser (the secondary button) changes only the foreground color, to the background color or to
   transparency. */
static void
as8_color_eraser_on_canvas (void)
{
  EaselWindow *win = start_window ();
  EaselPaletteBar *bar = easel_window_get_palette_bar (win);
  const EaselImage *img = easel_window_get_picture (win);
  uint32_t red, other, bg = easel_window_get_background (win);

  wait_for_layout (win);
  use_tool (win, "brush");
  wait_for_layout (win);
  choose_preset (win, "Square, 8 pixels");
  easel_palette_bar_press (bar, 17, 1, 1);
  red = easel_window_get_foreground (win);
  stroke_with (win, 1, 20, 20, 20, 20);                                    /* a red square, columns 17..24 */
  easel_palette_bar_press (bar, 21, 1, 1);
  other = easel_window_get_foreground (win);
  g_assert_cmphex (other, !=, red);
  stroke_with (win, 1, 28, 20, 28, 20);                                    /* another color, columns 25..32 */
  g_assert_cmpint (count_color (img, red), ==, 64);
  g_assert_cmpint (count_color (img, other), ==, 64);

  /* The foreground is red again; the color eraser passes over both squares. */
  easel_palette_bar_press (bar, 17, 1, 1);
  use_tool (win, "eraser");
  wait_for_layout (win);
  stroke_with (win, 3, 14, 20, 36, 20);
  g_assert_cmpint (count_color (img, red), ==, 0);
  g_assert_cmpint (count_color (img, other), ==, 64);                      /* untouched */
  g_assert_cmphex (easel_image_get (img, 20, 20), ==, bg);
  g_action_group_activate_action (G_ACTION_GROUP (win), "undo", NULL);
  g_assert_cmpint (count_color (img, red), ==, 64);

  /* Transparent mode: the red pixels become transparent instead. */
  choose_preset (win, "Transparent: erase to transparency");
  stroke_with (win, 3, 14, 20, 36, 20);
  g_assert_cmpint (count_color (img, red), ==, 0);
  g_assert_cmphex (easel_image_get (img, 20, 20), ==, 0x00000000u);
  g_assert_cmpint (count_color (img, other), ==, 64);
  close_windows ();
}

/* AS-9: a stroke is one undo step; the other button cancels it; Brush and Eraser. */
static void
as9_brush_and_eraser_steps (void)
{
  EaselWindow *win = start_window ();
  EaselCanvas *canvas = easel_window_get_canvas (win);
  const EaselImage *img = easel_window_get_picture (win);
  uint32_t fg = easel_window_get_foreground (win);

  wait_for_layout (win);
  use_tool (win, "brush");
  wait_for_layout (win);
  stroke_with (win, 1, 10, 10, 60, 40);
  g_assert_true (action_enabled (win, "undo"));
  g_action_group_activate_action (G_ACTION_GROUP (win), "undo", NULL);
  g_assert_false (action_enabled (win, "undo"));                           /* one step took all of it */
  g_assert_cmpint (count_color (img, fg), ==, 0);
  g_assert_false (easel_window_get_modified (win));

  /* The other button during a stroke cancels it. */
  easel_canvas_press (canvas, 1, 10.5, 10.5);
  easel_canvas_motion (canvas, 40.5, 10.5);
  g_assert_cmpint (count_color (img, fg), >, 0);
  easel_canvas_press (canvas, 3, 40.5, 10.5);
  g_assert_cmpint (count_color (img, fg), ==, 0);
  easel_canvas_release (canvas, 3);
  easel_canvas_release (canvas, 1);
  g_assert_false (action_enabled (win, "undo"));

  /* The Eraser too. */
  use_tool (win, "eraser");
  wait_for_layout (win);
  easel_canvas_press (canvas, 1, 10.5, 10.5);
  easel_canvas_motion (canvas, 40.5, 10.5);
  easel_canvas_press (canvas, 3, 40.5, 10.5);
  easel_canvas_release (canvas, 3);
  easel_canvas_release (canvas, 1);
  g_assert_false (easel_window_get_modified (win));
  close_windows ();
}

/* AS-11: choosing a preset is not an edit, and is not possible during a stroke. */
static void
as11_presets_are_not_edits_in_a_stroke (void)
{
  EaselWindow *win = start_window ();
  EaselCanvas *canvas = easel_window_get_canvas (win);
  GtkWidget *other;

  wait_for_layout (win);
  use_tool (win, "brush");
  wait_for_layout (win);
  choose_preset (win, "Square, 5 pixels");
  choose_preset (win, "Circle, 1 pixel");
  g_assert_false (easel_window_get_modified (win));
  g_assert_false (action_enabled (win, "undo"));

  other = find_by_tooltip (GTK_WIDGET (win), "Circle, 7 pixels");
  g_assert_true (gtk_widget_is_sensitive (other));
  easel_canvas_press (canvas, 1, 10.5, 10.5);
  g_assert_false (gtk_widget_is_sensitive (other));                         /* not while the button is held */
  easel_canvas_release (canvas, 1);
  g_assert_true (gtk_widget_is_sensitive (other));
  /* Also after a cancelled stroke. */
  easel_canvas_press (canvas, 1, 10.5, 10.5);
  easel_canvas_press (canvas, 3, 10.5, 10.5);
  g_assert_true (gtk_widget_is_sensitive (other));
  easel_canvas_release (canvas, 3);
  easel_canvas_release (canvas, 1);
  close_windows ();
}

/* ---- 0006: the Airbrush on the canvas, with the real timer ---- */

/* Runs the main loop for ms milliseconds (of the monotonic clock). */
static void
run_for (int ms)
{
  gint64 end = g_get_monotonic_time () + ms * 1000;

  while (g_get_monotonic_time () < end)
    g_main_context_iteration (NULL, FALSE);
}

/* AS-6: held for a second, the spray makes at most one spray per 5 ms of the time held (never ahead of the clock) and at
   least 100; a second spray at the same place differs; the middle button does nothing; the release stops it. The exact
   count (201 in 1000 ms) is the core's test, with a simulated clock. */
static void
as6_airbrush_sprays (void)
{
  EaselWindow *win = start_window ();
  EaselCanvas *canvas = easel_window_get_canvas (win);
  const EaselImage *img = easel_window_get_picture (win);
  uint32_t fg = easel_window_get_foreground (win);
  gint64 start, held_us;
  int made, first_count;

  wait_for_layout (win);
  use_tool (win, "airbrush");
  wait_for_layout (win);
  choose_preset (win, "Spray, 16 pixels");

  start = g_get_monotonic_time ();
  easel_canvas_press (canvas, 1, 150.5, 100.5);
  run_for (1000);
  held_us = g_get_monotonic_time () - start;
  made = easel_canvas_get_spray_count (canvas);
  easel_canvas_release (canvas, 1);
  g_assert_cmpint (made, >=, 100);
  g_assert_cmpint (made, <=, easel_spray_due (held_us));                  /* never more than the time held allows */
  first_count = count_color (img, fg);
  g_assert_cmpint (first_count, >, 60);                                    /* about 125 different pixels of 208 */
  for (int y = 0; y < img->height; y++)
    for (int x = 0; x < img->width; x++)
      if (easel_image_get (img, x, y) == fg)
        {
          /* In the disc around (150, 100): the box 16 wide with the pointer pixel at column 7. */
          g_assert_cmpint (x, >=, 150 - 7);
          g_assert_cmpint (x, <=, 150 + 8);
          g_assert_cmpint (y, >=, 100 - 7);
          g_assert_cmpint (y, <=, 100 + 8);
        }

  /* Released: no more sprays, however long we wait. */
  run_for (100);
  g_assert_cmpint (count_color (img, fg), ==, first_count);
  g_action_group_activate_action (G_ACTION_GROUP (win), "undo", NULL);
  g_assert_cmpint (count_color (img, fg), ==, 0);                          /* one step */
  g_assert_false (action_enabled (win, "undo"));

  /* The middle button does nothing. */
  easel_canvas_press (canvas, 2, 150.5, 100.5);
  run_for (50);
  easel_canvas_release (canvas, 2);
  g_assert_cmpint (count_color (img, fg), ==, 0);
  g_assert_false (easel_window_get_modified (win));
  close_windows ();
}

/* REQ-0006-09: the random choice starts differently each time: ten clicks at the same pixel (one spray each) do not
   all paint the same pixel (by chance, with odds of failing of one in 208 to the ninth power). */
static void
as6_airbrush_clicks_differ (void)
{
  EaselWindow *win = start_window ();
  EaselCanvas *canvas = easel_window_get_canvas (win);
  const EaselImage *img = easel_window_get_picture (win);
  uint32_t fg = easel_window_get_foreground (win);
  int first_x = -1, first_y = -1;
  gboolean differ = FALSE;

  wait_for_layout (win);
  use_tool (win, "airbrush");
  wait_for_layout (win);
  choose_preset (win, "Spray, 16 pixels");
  for (int i = 0; i < 10; i++)
    {
      int n = 0, px = -1, py = -1;

      easel_canvas_press (canvas, 1, 100.5, 100.5);
      easel_canvas_release (canvas, 1);                                   /* nothing can fire in between: one spray */
      for (int y = 0; y < img->height; y++)
        for (int x = 0; x < img->width; x++)
          if (easel_image_get (img, x, y) == fg)
            {
              n++;
              px = x;
              py = y;
            }
      g_assert_cmpint (n, ==, 1);
      if (i == 0)
        {
          first_x = px;
          first_y = py;
        }
      else
        differ = differ || px != first_x || py != first_y;
      g_action_group_activate_action (G_ACTION_GROUP (win), "undo", NULL);
    }
  g_assert_true (differ);
  close_windows ();
}

/* The spray lands where the pointer is, also when it moved, and nothing is painted while it is held off the picture. */
static void
as6_airbrush_follows_the_pointer (void)
{
  EaselWindow *win = start_window ();
  EaselCanvas *canvas = easel_window_get_canvas (win);
  const EaselImage *img = easel_window_get_picture (win);
  uint32_t fg = easel_window_get_foreground (win);

  wait_for_layout (win);
  use_tool (win, "airbrush");
  wait_for_layout (win);
  choose_preset (win, "Spray, 9 pixels");

  easel_canvas_press (canvas, 1, 40.5, 40.5);
  easel_canvas_motion (canvas, 200.5, 120.5);
  run_for (300);
  easel_canvas_release (canvas, 1);
  for (int y = 0; y < img->height; y++)
    for (int x = 0; x < img->width; x++)
      if (easel_image_get (img, x, y) == fg)
        {
          gboolean near_start = abs (x - 40) <= 5 && abs (y - 40) <= 5;
          gboolean near_end = abs (x - 200) <= 5 && abs (y - 120) <= 5;

          g_assert_true (near_start || near_end);
        }
  g_assert_cmpint (count_color (img, fg), >, 20);                          /* the held pointer at the end painted much */
  {
    int at_end = 0;

    for (int y = 115; y <= 125; y++)
      for (int x = 195; x <= 205; x++)
        at_end += easel_image_get (img, x, y) == fg;
    g_assert_cmpint (at_end, >, 10);                                       /* it landed where the pointer went */
  }
  g_action_group_activate_action (G_ACTION_GROUP (win), "undo", NULL);

  /* Held outside the picture: the spray runs, paints nothing and adds no step. */
  easel_canvas_press (canvas, 1, -60.5, 50.5);
  run_for (100);
  easel_canvas_release (canvas, 1);
  g_assert_cmpint (count_color (img, fg), ==, 0);
  g_assert_false (action_enabled (win, "undo"));
  close_windows ();
}

/* AS-9, the spray: held and released is one step; the other button cancels it and the spray stops. */
static void
as9_spray_one_step_and_cancel (void)
{
  EaselWindow *win = start_window ();
  EaselCanvas *canvas = easel_window_get_canvas (win);
  const EaselImage *img = easel_window_get_picture (win);
  uint32_t fg = easel_window_get_foreground (win);
  int after_cancel;

  wait_for_layout (win);
  use_tool (win, "airbrush");
  wait_for_layout (win);
  easel_canvas_press (canvas, 1, 60.5, 60.5);
  run_for (200);
  g_assert_cmpint (count_color (img, fg), >, 0);
  easel_canvas_press (canvas, 3, 60.5, 60.5);                              /* the other button cancels */
  after_cancel = count_color (img, fg);
  g_assert_cmpint (after_cancel, ==, 0);
  run_for (100);
  g_assert_cmpint (count_color (img, fg), ==, 0);                          /* the spray stopped with the stroke */
  easel_canvas_release (canvas, 3);
  easel_canvas_release (canvas, 1);
  g_assert_false (action_enabled (win, "undo"));

  /* Losing the focus ends the stroke and keeps what was sprayed. */
  easel_canvas_press (canvas, 1, 60.5, 60.5);
  run_for (100);
  easel_canvas_reset_pointer (canvas);
  after_cancel = count_color (img, fg);
  g_assert_cmpint (after_cancel, >, 0);
  run_for (100);
  g_assert_cmpint (count_color (img, fg), ==, after_cancel);
  g_assert_true (action_enabled (win, "undo"));
  close_windows ();
}

/* ---- 0005: file I/O ---- */

/* REQ-0005-01: the Open button before Save, and the main menu with its shortcuts. */
static void
as1_file_menu (void)
{
  EaselWindow *win = start_window ();
  GtkMenuButton *main = easel_window_get_main_menu_button (win);
  GMenuModel *menu = menu_of (main);
  GtkButton *open = find_action_button (GTK_WIDGET (win), "win.open");
  GtkButton *save = find_action_button (GTK_WIDGET (win), "win.save");
  GtkButton *undo = find_action_button (GTK_WIDGET (win), "win.undo");
  g_autofree char *s0 = NULL, *s1 = NULL;

  wait_for_layout (win);
  /* Open, Save, Undo, Redo, left to right (docs/design.md). */
  g_assert_nonnull (open);
  g_assert_nonnull (save);
  g_assert_cmpstr (gtk_widget_get_tooltip_text (GTK_WIDGET (open)), ==, "Open");
  {
    graphene_rect_t ro = bounds_in (GTK_WIDGET (open), GTK_WIDGET (win)), rs = bounds_in (GTK_WIDGET (save), GTK_WIDGET (win)),
                    ru = bounds_in (GTK_WIDGET (undo), GTK_WIDGET (win));

    g_assert_cmpfloat (ro.origin.x + ro.size.width, <=, rs.origin.x);
    g_assert_cmpfloat (rs.origin.x + rs.size.width, <=, ru.origin.x);
  }

  /* New, Open…, Save and Save As… in one section, Quit apart from them in the next. */
  g_assert_true (gtk_menu_button_get_primary (main));
  g_assert_cmpint (g_menu_model_get_n_items (menu), ==, 2);
  s0 = describe_section (menu, 0);
  s1 = describe_section (menu, 1);
  g_assert_cmpstr (s0, ==, "_New|win.new; _Open…|win.open; _Save|win.save; Save _As…|win.save-as");
  g_assert_cmpstr (s1, ==, "_Quit|app.quit");

  g_assert_true (has_accel ("win.new", GDK_KEY_n, GDK_CONTROL_MASK));
  g_assert_true (has_accel ("win.open", GDK_KEY_o, GDK_CONTROL_MASK));
  g_assert_true (has_accel ("win.save", GDK_KEY_s, GDK_CONTROL_MASK));
  g_assert_true (has_accel ("win.save-as", GDK_KEY_s, GDK_CONTROL_MASK | GDK_SHIFT_MASK));
  g_assert_true (has_accel ("app.quit", GDK_KEY_q, GDK_CONTROL_MASK));
  close_windows ();
}

static void
respond (EaselWindow *win, const char *response)
{
  AdwAlertDialog *dialog = open_alert (win);

  g_assert_nonnull (dialog);
  g_signal_emit_by_name (dialog, "response", response);
  adw_dialog_close (ADW_DIALOG (dialog));
  wait_for_dialogs (win, 0);
}

static void
assert_unsaved_prompt (EaselWindow *win, const char *name)
{
  AdwAlertDialog *dialog = open_alert (win);
  g_autofree char *body = g_strdup_printf ("“%s” has unsaved changes.", name);

  g_assert_nonnull (dialog);
  g_assert_cmpstr (adw_alert_dialog_get_heading (dialog), ==, "Save Changes?");
  g_assert_cmpstr (adw_alert_dialog_get_body (dialog), ==, body);
  g_assert_cmpstr (adw_alert_dialog_get_default_response (dialog), ==, "save");
  g_assert_cmpstr (adw_alert_dialog_get_close_response (dialog), ==, "cancel");
}

/* REQ-0005-03: New on an unmodified picture replaces it at once with a picture as at start-up. */
static void
as2_new_picture (void)
{
  EaselWindow *win = start_window ();
  EaselCanvas *canvas = easel_window_get_canvas (win);
  g_autofree char *dir = g_dir_make_tmp ("easel-ui-XXXXXX", NULL);
  g_autofree char *path = g_build_filename (dir, "a.png", NULL);
  uint32_t fg, bg;

  wait_for_layout (win);
  use_pencil (win);
  g_action_group_activate_action (G_ACTION_GROUP (win), "swap-colors", NULL);
  fg = easel_window_get_foreground (win);
  bg = easel_window_get_background (win);
  g_assert_cmpint (easel_window_resize_picture (win, 50, 40), ==, EASEL_RESIZE_OK);
  g_action_group_activate_action (G_ACTION_GROUP (win), "grid", NULL);
  g_action_group_activate_action (G_ACTION_GROUP (win), "zoom-large", NULL);
  wait_for_layout (win);
  g_assert_true (easel_window_save_to (win, path));                 /* now unmodified, with a file */
  g_assert_cmpstr (gtk_window_get_title (GTK_WINDOW (win)), ==, "a.png – Easel");

  g_action_group_activate_action (G_ACTION_GROUP (win), "new", NULL);
  g_assert_cmpint (count_alert_dialogs (GTK_WIDGET (win), &(AdwDialog *) { NULL }), ==, 0);
  assert_size (win, 300, 200);
  g_assert_cmphex (easel_image_get (easel_window_get_picture (win), 299, 199), ==, 0xFFFFFF00u);   /* the settings' background */
  g_assert_cmpstr (gtk_window_get_title (GTK_WINDOW (win)), ==, "Untitled – Easel");
  g_assert_false (easel_window_get_modified (win));
  g_assert_false (action_enabled (win, "undo"));
  g_assert_false (action_enabled (win, "redo"));
  wait_for_layout (win);
  g_assert_cmpint (easel_canvas_get_zoom (canvas), ==, 100);
  g_assert_true (gtk_widget_get_ancestor (GTK_WIDGET (canvas), GTK_TYPE_SCROLLED_WINDOW) != NULL);
  g_assert_cmpint (easel_window_get_tool (win), ==, EASEL_TOOL_PENCIL);
  g_assert_true (easel_canvas_get_grid_on (canvas));
  g_assert_cmphex (easel_window_get_foreground (win), ==, fg);
  g_assert_cmphex (easel_window_get_background (win), ==, bg);
  close_windows ();
}

/* REQ-0005-03: the zoom and the scroll start again. */
static void
as2_new_resets_the_view (void)
{
  EaselWindow *win;
  EaselCanvas *canvas;
  GtkScrolledWindow *scroller;

  write_settings ("[new-picture]\nwidth=2000\nheight=2000\n");
  win = start_window ();
  canvas = easel_window_get_canvas (win);
  scroller = GTK_SCROLLED_WINDOW (gtk_widget_get_ancestor (GTK_WIDGET (canvas), GTK_TYPE_SCROLLED_WINDOW));
  g_action_group_activate_action (G_ACTION_GROUP (win), "zoom-large", NULL);
  wait_for_layout (win);
  gtk_adjustment_set_value (gtk_scrolled_window_get_hadjustment (scroller), 500);
  gtk_adjustment_set_value (gtk_scrolled_window_get_vadjustment (scroller), 400);
  wait_for_layout (win);
  g_action_group_activate_action (G_ACTION_GROUP (win), "new", NULL);
  wait_for_layout (win);
  g_assert_cmpint (easel_canvas_get_zoom (canvas), ==, 100);
  g_assert_cmpstr (easel_window_get_zoom_text (win), ==, "100%");
  g_assert_cmpfloat (gtk_adjustment_get_value (gtk_scrolled_window_get_hadjustment (scroller)), ==, 0);
  g_assert_cmpfloat (gtk_adjustment_get_value (gtk_scrolled_window_get_vadjustment (scroller)), ==, 0);
  g_assert_cmpint (easel_window_get_picture (win)->width, ==, 2000);
  close_windows ();
  write_settings ("[new-picture]\nwidth=300\nheight=200\nbackground=#FFFF00\n");
}

/* REQ-0005-04: New on a modified picture asks first, with the question of the quit prompt. */
static void
as3_new_and_open_prompt (void)
{
  EaselWindow *win = start_window ();
  g_autofree char *dir = g_dir_make_tmp ("easel-ui-XXXXXX", NULL);
  g_autofree char *path = g_build_filename (dir, "a.png", NULL);
  g_autoptr (GdkPixbuf) saved = NULL;

  wait_for_layout (win);
  use_pencil (win);
  A_STROKE (win);
  g_assert_true (easel_window_get_modified (win));

  /* The prompt; Cancel, and Esc, change nothing. */
  g_action_group_activate_action (G_ACTION_GROUP (win), "new", NULL);
  wait_for_dialogs (win, 1);
  assert_unsaved_prompt (win, "Untitled");
  respond (win, "cancel");
  g_assert_cmphex (easel_image_get (easel_window_get_picture (win), 5, 3), ==, 0xFF000000u);
  g_assert_true (easel_window_get_modified (win));
  g_action_group_activate_action (G_ACTION_GROUP (win), "new", NULL);
  wait_for_dialogs (win, 1);
  adw_dialog_close (ADW_DIALOG (open_alert (win)));               /* Esc is the close response */
  wait_for_dialogs (win, 0);
  g_assert_true (easel_window_get_modified (win));
  g_assert_cmphex (easel_image_get (easel_window_get_picture (win), 5, 3), ==, 0xFF000000u);

  /* While the prompt is open, the actions do nothing: a second New adds no second prompt. */
  g_action_group_activate_action (G_ACTION_GROUP (win), "new", NULL);
  wait_for_dialogs (win, 1);
  g_action_group_activate_action (G_ACTION_GROUP (win), "new", NULL);
  g_action_group_activate_action (G_ACTION_GROUP (win), "undo", NULL);
  wait_for_layout (win);
  g_assert_nonnull (open_alert (win));
  g_assert_cmphex (easel_image_get (easel_window_get_picture (win), 5, 3), ==, 0xFF000000u);

  /* Discard replaces the picture. */
  respond (win, "discard");
  g_assert_cmphex (easel_image_get (easel_window_get_picture (win), 5, 3), ==, 0xFFFFFF00u);
  g_assert_false (easel_window_get_modified (win));
  g_assert_cmpstr (gtk_window_get_title (GTK_WINDOW (win)), ==, "Untitled – Easel");

  /* Save, with a file: the file is written and then the picture replaced. */
  g_assert_true (easel_window_save_to (win, path));
  A_STROKE (win);
  g_assert_cmpstr (gtk_window_get_title (GTK_WINDOW (win)), ==, "• a.png – Easel");
  g_action_group_activate_action (G_ACTION_GROUP (win), "new", NULL);
  wait_for_dialogs (win, 1);
  assert_unsaved_prompt (win, "a.png");
  respond (win, "save");
  g_assert_cmpstr (gtk_window_get_title (GTK_WINDOW (win)), ==, "Untitled – Easel");
  g_assert_cmphex (easel_image_get (easel_window_get_picture (win), 5, 3), ==, 0xFFFFFF00u);
  {
    g_autoptr (GError) error = NULL;

    saved = gdk_pixbuf_new_from_file (path, &error);
    g_assert_no_error (error);
    g_assert_cmpint (gdk_pixbuf_get_pixels (saved)[3 * gdk_pixbuf_get_rowstride (saved) + 5 * 4], ==, 0);   /* the stroke */
  }
  close_windows ();
}

/* REQ-0005-04: a failing save in the prompt keeps the picture. */
static void
as3_new_save_fails (void)
{
  EaselWindow *win = start_window ();
  g_autofree char *dir = g_dir_make_tmp ("easel-ui-XXXXXX", NULL);
  g_autofree char *path = g_build_filename (dir, "a.png", NULL);

  if (geteuid () == 0)
    {
      g_test_skip ("root ignores folder permissions");
      close_windows ();
      return;
    }
  wait_for_layout (win);
  g_assert_true (easel_window_save_to (win, path));
  use_pencil (win);
  A_STROKE (win);
  g_assert_cmpint (g_chmod (dir, 0500), ==, 0);
  g_action_group_activate_action (G_ACTION_GROUP (win), "new", NULL);
  wait_for_dialogs (win, 1);
  {
    AdwAlertDialog *prompt = open_alert (win);

    g_signal_emit_by_name (prompt, "response", "save");
    adw_dialog_close (ADW_DIALOG (prompt));
  }
  /* The save error dialog shows; the picture is the old one, still modified. */
  wait_for_dialogs (win, 1);
  g_assert_cmphex (easel_image_get (easel_window_get_picture (win), 5, 3), ==, 0xFF000000u);
  g_assert_true (easel_window_get_modified (win));
  g_assert_cmpstr (gtk_window_get_title (GTK_WINDOW (win)), ==, "• a.png – Easel");
  g_chmod (dir, 0700);
  close_windows ();
}

/* REQ-0005-16: New starts a new history whose first state is the saved one. */
static void
as14_new_starts_a_history (void)
{
  EaselWindow *win = start_window ();

  wait_for_layout (win);
  use_pencil (win);
  A_STROKE (win);
  g_action_group_activate_action (G_ACTION_GROUP (win), "new", NULL);
  wait_for_dialogs (win, 1);
  respond (win, "discard");
  g_assert_false (action_enabled (win, "undo"));
  g_assert_false (easel_window_get_modified (win));
  A_STROKE (win);
  g_assert_true (easel_window_get_modified (win));
  g_assert_cmpstr (gtk_window_get_title (GTK_WINDOW (win)), ==, "• Untitled – Easel");
  g_action_group_activate_action (G_ACTION_GROUP (win), "undo", NULL);
  g_assert_false (easel_window_get_modified (win));
  g_assert_cmpstr (gtk_window_get_title (GTK_WINDOW (win)), ==, "Untitled – Easel");
  close_windows ();
}

/* REQ-0005-02: New does nothing before the start-up picture exists. */
static void
as15_new_before_picture (void)
{
  EaselWindow *win;

  write_settings ("[new-picture]\nwidth=auto\nheight=auto\n");
  win = easel_window_new (app);
  g_assert_null (easel_window_get_picture (win));
  g_action_group_activate_action (G_ACTION_GROUP (win), "new", NULL);
  g_assert_null (easel_window_get_picture (win));
  gtk_window_destroy (GTK_WINDOW (win));
  write_settings ("[new-picture]\nwidth=300\nheight=200\nbackground=#FFFF00\n");
}

/* REQ-0005-03: a new picture whose memory cannot be had shows the error and keeps the current one. */
static void
as2_new_memory_error (void)
{
  EaselWindow *win;
  AdwAlertDialog *dialog;
  struct rlimit old;

  write_settings ("[new-picture]\nwidth=10000\nheight=10000\n");
  win = start_window ();
  wait_for_layout (win);
  g_assert_true (limit_memory (&old, 200));
  g_action_group_activate_action (G_ACTION_GROUP (win), "new", NULL);
  g_assert_cmpint (setrlimit (RLIMIT_AS, &old), ==, 0);
  wait_for_dialogs (win, 1);
  dialog = open_alert (win);
  g_assert_cmpstr (adw_alert_dialog_get_heading (dialog), ==, "Could Not Create a New Picture");
  g_assert_cmpstr (adw_alert_dialog_get_body (dialog), ==, "There is not enough memory for a picture of 10000 × 10000 pixels.");
  g_assert_cmpint (easel_window_get_picture (win)->width, ==, 10000);
  g_assert_false (easel_window_get_modified (win));
  adw_dialog_close (ADW_DIALOG (dialog));
  wait_for_dialogs (win, 0);
  close_windows ();
  write_settings ("[new-picture]\nwidth=300\nheight=200\nbackground=#FFFF00\n");
}

/* ---- 0005: Open ---- */

static char *
fixture_path (const char *name)
{
  const char *dir = g_getenv ("EASEL_TEST_DATA");

  return g_build_filename (dir ? dir : "tests/data", name, NULL);
}

/* A copy of a fixture in a fresh folder, so that Save may write it. */
static char *
fixture_copy (const char *name)
{
  g_autofree char *source = fixture_path (name);
  g_autofree char *dir = g_dir_make_tmp ("easel-ui-XXXXXX", NULL);
  char *path = g_build_filename (dir, name, NULL);
  g_autofree char *contents = NULL;
  gsize len;

  g_assert_true (g_file_get_contents (source, &contents, &len, NULL));
  g_assert_true (g_file_set_contents (path, contents, (gssize) len, NULL));
  return path;
}

/* Asks the window to open the file and waits until the picture is shown or the error is. */
static void
open_and_wait (EaselWindow *win, const char *path)
{
  g_autoptr (GFile) file = g_file_new_for_path (path);
  gint64 deadline = g_get_monotonic_time () + 20 * G_USEC_PER_SEC;

  easel_window_open_file (win, file);
  while (easel_window_is_loading (win) && g_get_monotonic_time () < deadline)
    g_main_context_iteration (NULL, FALSE);
  g_assert_false (easel_window_is_loading (win));
}

/* The pattern of the fixtures (make-fixtures.py) at one pixel, as the picture holds it. */
static uint32_t
fixture_pixel (int x, int y)
{
  uint32_t a = (x + y) % 4 == 0 ? 0 : (x % 3 == 0 ? 128 : 255);
  uint32_t r = (uint32_t) (30 * x + 20), g = (uint32_t) (40 * y + 10), b = (uint32_t) (100 + x + y);

  return a == 0 ? 0 : (a << 24) | (r << 16) | (g << 8) | b;
}

static void
assert_fixture_picture (EaselWindow *win)
{
  assert_size (win, 7, 5);
  for (int y = 0; y < 5; y++)
    for (int x = 0; x < 7; x++)
      g_assert_cmphex (easel_image_get (easel_window_get_picture (win), x, y), ==, fixture_pixel (x, y));
}

/* The Plan's promise: the busy cursor is shown, and has been sent to the screen, before decoding starts. */
static void
busy_cursor_before_decoding (void)
{
  EaselWindow *win = start_window ();
  const EaselImage *before = easel_window_get_picture (win);
  g_autofree char *path = fixture_copy ("rgba.png");
  g_autoptr (GFile) file = g_file_new_for_path (path);
  GdkCursor *cursor;

  wait_for_layout (win);
  g_assert_null (easel_window_get_cursor_at_decode (win));
  easel_window_open_file (win, file);

  /* Right after the request: nothing decoded yet, the window says it is loading, the cursor is the busy one. */
  g_assert_true (easel_window_is_loading (win));
  g_assert_true (easel_window_get_picture (win) == before);
  cursor = gtk_widget_get_cursor (GTK_WIDGET (win));
  g_assert_nonnull (cursor);
  g_assert_cmpstr (gdk_cursor_get_name (cursor), ==, "wait");
  g_assert_cmpint (easel_window_get_frames_before_decode (win), ==, -1);       /* not started */
  /* Also over the canvas and the area around it, which have cursors of their own. */
  g_assert_cmpstr (gdk_cursor_get_name (gtk_widget_get_cursor (GTK_WIDGET (easel_window_get_canvas (win)))), ==, "wait");
  g_assert_cmpstr (gdk_cursor_get_name (gtk_widget_get_cursor (
                     gtk_widget_get_ancestor (GTK_WIDGET (easel_window_get_canvas (win)), GTK_TYPE_SCROLLED_WINDOW))),
                   ==, "wait");
  /* Input is ignored meanwhile. */
  g_action_group_activate_action (G_ACTION_GROUP (win), "new", NULL);
  g_assert_true (easel_window_get_picture (win) == before);

  /* The decode starts after a frame has been painted, and the cursor was the busy one when it did. */
  {
    gint64 deadline = g_get_monotonic_time () + 20 * G_USEC_PER_SEC;

    while (easel_window_is_loading (win) && g_get_monotonic_time () < deadline)
      g_main_context_iteration (NULL, FALSE);
  }
  g_assert_cmpstr (easel_window_get_cursor_at_decode (win), ==, "wait");
  g_assert_cmpint (easel_window_get_frames_before_decode (win), >=, 1);
  assert_fixture_picture (win);
  g_assert_null (gtk_widget_get_cursor (GTK_WIDGET (win)));
  close_windows ();
}

/* REQ-0005-07: the opened picture replaces the current one, and the view starts at its top-left at 100 %. */
static void
as6_open_replaces_picture (void)
{
  EaselWindow *win;
  EaselCanvas *canvas;
  g_autofree char *path = fixture_copy ("rgba.png");
  uint32_t *px;
  int w, h, cx, cy;

  write_settings ("[new-picture]\nwidth=2000\nheight=2000\n");
  win = start_window ();
  canvas = easel_window_get_canvas (win);
  g_action_group_activate_action (G_ACTION_GROUP (win), "zoom-large", NULL);
  wait_for_layout (win);
  gtk_adjustment_set_value (gtk_scrolled_window_get_hadjustment (GTK_SCROLLED_WINDOW (
                              gtk_widget_get_ancestor (GTK_WIDGET (canvas), GTK_TYPE_SCROLLED_WINDOW))), 300);

  open_and_wait (win, path);
  wait_for_layout (win);
  assert_fixture_picture (win);
  g_assert_cmpstr (gtk_window_get_title (GTK_WINDOW (win)), ==, "rgba.png – Easel");
  g_assert_false (easel_window_get_modified (win));
  g_assert_false (action_enabled (win, "undo"));
  g_assert_cmpint (easel_canvas_get_zoom (canvas), ==, 100);
  g_assert_cmpstr (easel_window_get_zoom_text (win), ==, "100%");
  g_assert_cmpstr (easel_status_bar_get_text (easel_window_get_status_bar (win), EASEL_STATUS_SIZE), ==, "7 × 5 px");

  /* What is drawn: the picture's top-left pixel (1, 0), opaque, is at the canvas origin plus one. */
  px = render_pixels (win, &w, &h, &cx, &cy);
  g_assert_cmphex (px[(gsize) cy * (gsize) w + (gsize) (cx + 1)], ==, 0xFF320A65u);
  g_free (px);

  /* Save writes the same file again without asking. */
  g_action_group_activate_action (G_ACTION_GROUP (win), "save", NULL);
  g_assert_null (other_toplevel (win));
  g_assert_cmpint (count_alert_dialogs (GTK_WIDGET (win), &(AdwDialog *) { NULL }), ==, 0);
  g_assert_false (easel_window_get_modified (win));
  close_windows ();
  write_settings ("[new-picture]\nwidth=300\nheight=200\nbackground=#FFFF00\n");
}

/* REQ-0005-08: a modified picture asks first, before the file dialog. */
static void
as7_prompt_before_open_dialog (void)
{
  EaselWindow *win = start_window ();
  const EaselImage *before = easel_window_get_picture (win);

  wait_for_layout (win);
  use_pencil (win);
  A_STROKE (win);
  g_action_group_activate_action (G_ACTION_GROUP (win), "open", NULL);
  wait_for_dialogs (win, 1);
  wait_for_layout (win);
  assert_unsaved_prompt (win, "Untitled");
  g_assert_null (other_toplevel (win));                /* no file dialog yet */

  /* Discard goes on to the file dialog; cancelling that leaves the picture as it was, still modified (REQ-0005-04). */
  respond (win, "discard");
  wait_for_layout (win);
  {
    GtkWindow *chooser = other_toplevel (win);

    g_assert_nonnull (chooser);
    gtk_window_close (chooser);   /* as Esc does: the dialog ends as cancelled */
  }
  wait_for_layout (win);
  g_assert_true (easel_window_get_picture (win) == before);
  g_assert_cmphex (easel_image_get (easel_window_get_picture (win), 5, 3), ==, 0xFF000000u);
  g_assert_true (easel_window_get_modified (win));
  g_assert_true (action_enabled (win, "undo"));

  /* An unmodified picture shows the dialog at once. */
  g_action_group_activate_action (G_ACTION_GROUP (win), "undo", NULL);
  g_assert_false (easel_window_get_modified (win));
  g_action_group_activate_action (G_ACTION_GROUP (win), "open", NULL);
  wait_for_layout (win);
  g_assert_cmpint (count_alert_dialogs (GTK_WIDGET (win), &(AdwDialog *) { NULL }), ==, 0);
  g_assert_nonnull (other_toplevel (win));
  gtk_window_close (other_toplevel (win));
  wait_for_layout (win);
  close_windows ();
}

/* REQ-0005-04 for Open, without the file dialog: the prompt, Cancel, Discard, and a bad file after Discard. */
static void
as3_open_and_prompt (void)
{
  EaselWindow *win = start_window ();
  g_autofree char *good = fixture_copy ("rgba.png");
  g_autofree char *bad = fixture_copy ("text.png");
  g_autoptr (GFile) good_file = g_file_new_for_path (good);
  g_autoptr (GFile) bad_file = g_file_new_for_path (bad);
  const EaselImage *before = easel_window_get_picture (win);

  wait_for_layout (win);
  use_pencil (win);
  A_STROKE (win);

  easel_window_open_file (win, good_file);
  wait_for_dialogs (win, 1);
  assert_unsaved_prompt (win, "Untitled");
  g_assert_false (easel_window_is_loading (win));
  respond (win, "cancel");
  g_assert_false (easel_window_is_loading (win));
  g_assert_true (easel_window_get_picture (win) == before);
  g_assert_true (easel_window_get_modified (win));

  /* Discard, then a file that cannot be opened: the old picture is still there, and still modified. */
  easel_window_open_file (win, bad_file);
  wait_for_dialogs (win, 1);
  respond (win, "discard");
  {
    gint64 deadline = g_get_monotonic_time () + 20 * G_USEC_PER_SEC;

    while (easel_window_is_loading (win) && g_get_monotonic_time () < deadline)
      g_main_context_iteration (NULL, FALSE);
  }
  wait_for_dialogs (win, 1);
  g_assert_cmpstr (adw_alert_dialog_get_heading (open_alert (win)), ==, "Could Not Open “text.png”");
  g_assert_true (easel_window_get_picture (win) == before);
  g_assert_cmphex (easel_image_get (easel_window_get_picture (win), 5, 3), ==, 0xFF000000u);
  g_assert_true (easel_window_get_modified (win));
  adw_dialog_close (ADW_DIALOG (open_alert (win)));
  wait_for_dialogs (win, 0);

  /* Discard, then a good file: the picture is replaced. */
  easel_window_open_file (win, good_file);
  wait_for_dialogs (win, 1);
  respond (win, "discard");
  {
    gint64 deadline = g_get_monotonic_time () + 20 * G_USEC_PER_SEC;

    while (easel_window_is_loading (win) && g_get_monotonic_time () < deadline)
      g_main_context_iteration (NULL, FALSE);
  }
  assert_fixture_picture (win);
  g_assert_false (easel_window_get_modified (win));
  close_windows ();
}

/* REQ-0005-09: each reason has its body, and nothing changes. */
static void
as8_open_error_dialog (void)
{
  static const struct { const char *name, *body; } cases[] = {
    { "text.png", "This is not a picture the app can open." },
    { "empty.png", "This is not a picture the app can open." },
    { "cut-header.png", "This is not a picture the app can open." },
    { "vector.svg", "This is not a picture the app can open." },
    { "w10001.png", "The picture is bigger than 10000 pixels in width or height." },
    { "nonexistent.png", "The file cannot be read." },
  };
  EaselWindow *win = start_window ();
  const EaselImage *before = easel_window_get_picture (win);
  g_autofree char *title = g_strdup (gtk_window_get_title (GTK_WINDOW (win)));
  struct rlimit old;
  AdwAlertDialog *dialog;
  g_autofree char *path = NULL;

  wait_for_layout (win);
  for (gsize i = 0; i < G_N_ELEMENTS (cases); i++)
    {
      g_autofree char *file = fixture_path (cases[i].name);
      g_autofree char *heading = g_strdup_printf ("Could Not Open “%s”", cases[i].name);

      open_and_wait (win, file);
      wait_for_dialogs (win, 1);
      dialog = open_alert (win);
      g_assert_nonnull (dialog);
      g_assert_cmpstr (adw_alert_dialog_get_heading (dialog), ==, heading);
      g_assert_cmpstr (adw_alert_dialog_get_body (dialog), ==, cases[i].body);
      adw_dialog_close (ADW_DIALOG (dialog));
      wait_for_dialogs (win, 0);
      g_assert_true (easel_window_get_picture (win) == before);
      g_assert_cmpstr (gtk_window_get_title (GTK_WINDOW (win)), ==, title);
      g_assert_false (easel_window_get_modified (win));
      g_assert_false (action_enabled (win, "undo"));
      g_assert_null (gtk_widget_get_cursor (GTK_WIDGET (win)));
    }

  /* A folder is not readable as a file. */
  path = g_dir_make_tmp ("easel-ui-XXXXXX", NULL);
  open_and_wait (win, path);
  wait_for_dialogs (win, 1);
  g_assert_cmpstr (adw_alert_dialog_get_body (open_alert (win)), ==, "The file cannot be read.");
  adw_dialog_close (ADW_DIALOG (open_alert (win)));
  wait_for_dialogs (win, 0);

  /* A picture that cannot get its memory (simulated: the address space is limited). */
  g_assert_true (limit_memory (&old, 100));
  {
    g_autofree char *big = fixture_path ("flat6000.png");

    open_and_wait (win, big);
  }
  g_assert_cmpint (setrlimit (RLIMIT_AS, &old), ==, 0);
  wait_for_dialogs (win, 1);
  dialog = open_alert (win);
  g_assert_cmpstr (adw_alert_dialog_get_heading (dialog), ==, "Could Not Open “flat6000.png”");
  g_assert_cmpstr (adw_alert_dialog_get_body (dialog), ==, "There is not enough memory for a picture of 6000 × 6000 pixels.");
  adw_dialog_close (ADW_DIALOG (dialog));
  wait_for_dialogs (win, 0);
  g_assert_true (easel_window_get_picture (win) == before);
  close_windows ();
}

/* REQ-0005-17: 10000 opens; over it is refused, also after the orientation turns it. */
static void
as17_too_big_dialog (void)
{
  static const char *refused[] = { "w10001.png", "h10001.png", "big10001.png", "wide-rot6.jpg" };
  EaselWindow *win = start_window ();
  const EaselImage *before = easel_window_get_picture (win);

  wait_for_layout (win);
  for (gsize i = 0; i < G_N_ELEMENTS (refused); i++)
    {
      g_autofree char *file = fixture_path (refused[i]);
      AdwAlertDialog *dialog;

      open_and_wait (win, file);
      wait_for_dialogs (win, 1);
      dialog = open_alert (win);
      g_assert_cmpstr (adw_alert_dialog_get_body (dialog), ==, "The picture is bigger than 10000 pixels in width or height.");
      adw_dialog_close (ADW_DIALOG (dialog));
      wait_for_dialogs (win, 0);
      g_assert_true (easel_window_get_picture (win) == before);
      g_assert_false (easel_window_get_modified (win));
    }
  {
    g_autofree char *file = fixture_path ("w10000.png");

    open_and_wait (win, file);
    assert_size (win, 10000, 1);
  }
  close_windows ();
}

/* REQ-0005-16: a freshly opened picture starts a history whose first state is the saved one. */
static void
as14_saved_state_across_open_and_save_as (void)
{
  EaselWindow *win = start_window ();
  g_autofree char *path = fixture_copy ("rgba.png");

  wait_for_layout (win);
  use_pencil (win);
  open_and_wait (win, path);
  g_assert_false (action_enabled (win, "undo"));
  g_assert_false (easel_window_get_modified (win));
  /* Drawn away from the handles: on a picture this small their hit areas reach into it (REQ-0004-16). */
  stroke (win, 1.5, 1.5, 3.5, 1.5);
  g_assert_true (easel_window_get_modified (win));
  g_assert_cmpstr (gtk_window_get_title (GTK_WINDOW (win)), ==, "• rgba.png – Easel");
  g_action_group_activate_action (G_ACTION_GROUP (win), "undo", NULL);
  g_assert_false (easel_window_get_modified (win));
  g_assert_cmpstr (gtk_window_get_title (GTK_WINDOW (win)), ==, "rgba.png – Easel");
  close_windows ();
}

/* REQ-0005-02: quit does nothing while the Open file dialog is open, and waits for a load that is under way. */
static void
as15_quit_around_open (void)
{
  EaselWindow *win = start_window ();
  g_autofree char *path = fixture_copy ("rgba.png");
  g_autoptr (GFile) file = g_file_new_for_path (path);
  EaselWindow *alive = win;

  g_object_add_weak_pointer (G_OBJECT (win), (gpointer *) &alive);
  wait_for_layout (win);
  g_action_group_activate_action (G_ACTION_GROUP (win), "open", NULL);
  wait_for_layout (win);
  g_assert_nonnull (other_toplevel (win));
  gtk_window_close (GTK_WINDOW (win));
  wait_for_layout (win);
  g_assert_cmpint (count_alert_dialogs (GTK_WIDGET (win), &(AdwDialog *) { NULL }), ==, 0);
  g_assert_true (gtk_widget_get_visible (GTK_WIDGET (win)));
  gtk_window_close (other_toplevel (win));
  wait_for_layout (win);

  /* Quit while the file is loading: the window stays until the picture is shown, then closes (the picture is not
     modified, so no prompt). */
  easel_window_open_file (win, file);
  g_assert_true (easel_window_is_loading (win));
  gtk_window_close (GTK_WINDOW (win));
  g_assert_nonnull (alive);                          /* still there: the quit waits, it does not close the window now */
  g_assert_true (easel_window_is_loading (win));
  g_assert_true (gtk_widget_get_visible (GTK_WIDGET (win)));
  {
    gint64 deadline = g_get_monotonic_time () + 20 * G_USEC_PER_SEC;

    while (g_list_length (gtk_application_get_windows (app)) > 0 && g_get_monotonic_time () < deadline)
      g_main_context_iteration (NULL, FALSE);
  }
  g_assert_cmpuint (g_list_length (gtk_application_get_windows (app)), ==, 0);
}

/* ---- 0005: a file on the command line ---- */

/* What the system does with `easel a.png b.png`: the application's open signal with the files. */
static EaselWindow *
launch_with (GFile **files, int n)
{
  gint64 deadline = g_get_monotonic_time () + 10 * G_USEC_PER_SEC;
  EaselWindow *win = NULL;

  g_application_open (G_APPLICATION (app), files, n, "");
  while (g_get_monotonic_time () < deadline)
    {
      win = EASEL_WINDOW (gtk_application_get_active_window (app));
      if (win != NULL && easel_window_get_picture (win) != NULL)
        break;
      g_main_context_iteration (NULL, FALSE);
    }
  g_assert_nonnull (win);
  g_assert_nonnull (easel_window_get_picture (win));
  wait_for_layout (win);
  return win;
}

/* REQ-0005-15: the first file is the start-up picture; a path and a file:// address both work; the others are ignored. */
static void
as13_command_line (void)
{
  g_autofree char *first = fixture_copy ("rgba.png");
  g_autofree char *second = fixture_copy ("flat.jpg");
  g_autofree char *uri = g_filename_to_uri (first, NULL, NULL);
  g_autoptr (GFile) by_path = g_file_new_for_path (first);
  g_autoptr (GFile) by_uri = g_file_new_for_uri (uri);
  g_autoptr (GFile) other = g_file_new_for_path (second);
  GFile *two[] = { by_path, other };
  EaselWindow *win;
  const EaselImage *picture;

  /* One file, by path. */
  win = launch_with (two, 1);
  assert_fixture_picture (win);
  g_assert_cmpstr (gtk_window_get_title (GTK_WINDOW (win)), ==, "rgba.png – Easel");
  g_assert_false (easel_window_get_modified (win));
  g_assert_false (action_enabled (win, "undo"));
  g_assert_cmpint (easel_window_get_zoom (win), ==, 100);
  g_assert_cmpstr (easel_status_bar_get_text (easel_window_get_status_bar (win), EASEL_STATUS_SIZE), ==, "7 × 5 px");
  g_assert_cmpint (count_alert_dialogs (GTK_WIDGET (win), &(AdwDialog *) { NULL }), ==, 0);
  picture = easel_window_get_picture (win);

  /* A second launch while the window exists only presents it. */
  {
    GFile *again[] = { other };

    g_assert_true (launch_with (again, 1) == win);
    g_assert_true (easel_window_get_picture (win) == picture);
    g_assert_cmpstr (gtk_window_get_title (GTK_WINDOW (win)), ==, "rgba.png – Easel");
  }
  close_windows ();

  /* By file:// address. */
  win = launch_with (&by_uri, 1);
  assert_fixture_picture (win);
  close_windows ();

  /* Two files: only the first opens. */
  win = launch_with (two, 2);
  assert_fixture_picture (win);
  g_assert_cmpstr (gtk_window_get_title (GTK_WINDOW (win)), ==, "rgba.png – Easel");
  close_windows ();
}

/* REQ-0005-15: a file that cannot be opened leaves the blank picture and tells why. */
static void
as13_command_line_error (void)
{
  g_autoptr (GFile) missing = g_file_new_for_path ("/nonexistent/missing.png");
  g_autoptr (GFile) text = g_file_new_for_path (g_build_filename (g_getenv ("EASEL_TEST_DATA"), "text.png", NULL));
  EaselWindow *win = launch_with (&missing, 1);
  AdwAlertDialog *dialog;

  wait_for_dialogs (win, 1);
  dialog = open_alert (win);
  g_assert_nonnull (dialog);                       /* the error is shown; without it the heading below would crash */
  g_assert_cmpstr (adw_alert_dialog_get_heading (dialog), ==, "Could Not Open “missing.png”");
  g_assert_cmpstr (adw_alert_dialog_get_body (dialog), ==, "The file cannot be read.");
  assert_size (win, 300, 200);                                         /* the blank picture of the settings */
  g_assert_cmphex (easel_image_get (easel_window_get_picture (win), 10, 10), ==, 0xFFFFFF00u);
  g_assert_cmpstr (gtk_window_get_title (GTK_WINDOW (win)), ==, "Untitled – Easel");
  g_assert_false (easel_window_get_modified (win));
  adw_dialog_close (ADW_DIALOG (dialog));
  wait_for_dialogs (win, 0);
  close_windows ();

  /* The same with the size of the settings set to auto, which waits for the canvas area first. */
  write_settings ("[new-picture]\nwidth=auto\nheight=auto\n");
  win = launch_with (&text, 1);
  wait_for_dialogs (win, 1);
  g_assert_cmpstr (adw_alert_dialog_get_body (open_alert (win)), ==, "This is not a picture the app can open.");
  g_assert_cmpstr (gtk_window_get_title (GTK_WINDOW (win)), ==, "Untitled – Easel");
  adw_dialog_close (ADW_DIALOG (open_alert (win)));
  wait_for_dialogs (win, 0);
  close_windows ();
  write_settings ("[new-picture]\nwidth=300\nheight=200\nbackground=#FFFF00\n");
}

/* ---- 0005: Save and Save As ---- */

static gboolean
file_starts_with (const char *path, const guint8 *magic, gsize n)
{
  g_autofree char *contents = NULL;
  gsize len;

  return g_file_get_contents (path, &contents, &len, NULL) && len >= n && memcmp (contents, magic, n) == 0;
}

static const guint8 jpeg_magic[] = { 0xFF, 0xD8, 0xFF };
static const guint8 png_magic[] = { 0x89, 'P', 'N', 'G' };

/* REQ-0005-10: the extension decides; the names that cannot be saved give one error and write nothing. */
static void
as9_unknown_extension_error (void)
{
  static const char *bad[] = { "c.gif", "d.xyz", "report 2024.05.10", "e.jpg.bak" };
  EaselWindow *win = start_window ();
  g_autofree char *dir = g_dir_make_tmp ("easel-ui-XXXXXX", NULL);
  g_autofree char *title = NULL;
  g_autofree char *base = NULL;

  wait_for_layout (win);
  use_pencil (win);
  A_STROKE (win);
  title = g_strdup (gtk_window_get_title (GTK_WINDOW (win)));
  for (gsize i = 0; i < G_N_ELEMENTS (bad); i++)
    {
      g_autofree char *path = g_build_filename (dir, bad[i], NULL);
      AdwAlertDialog *dialog;

      easel_window_save_chosen (win, path);
      wait_for_dialogs (win, 1);
      dialog = open_alert (win);
      g_assert_cmpstr (adw_alert_dialog_get_heading (dialog), ==, "Cannot Save This Kind of File");
      g_assert_cmpstr (adw_alert_dialog_get_body (dialog), ==, "Pictures can be saved as PNG, JPEG, BMP, TIFF or WebP.");
      adw_dialog_close (ADW_DIALOG (dialog));
      wait_for_dialogs (win, 0);
      g_assert_false (g_file_test (path, G_FILE_TEST_EXISTS));
      g_assert_true (easel_window_get_modified (win));
      g_assert_cmpstr (gtk_window_get_title (GTK_WINDOW (win)), ==, title);
    }

  /* Names without an extension get .png; the others are written in their format. */
  base = g_build_filename (dir, "b", NULL);
  easel_window_save_chosen (win, base);
  {
    g_autofree char *png = g_build_filename (dir, "b.png", NULL);

    g_assert_true (file_starts_with (png, png_magic, sizeof png_magic));
    g_assert_cmpstr (gtk_window_get_title (GTK_WINDOW (win)), ==, "b.png – Easel");
  }
  {
    g_autofree char *upper = g_build_filename (dir, "E.JPG", NULL);

    easel_window_save_chosen (win, upper);
    g_assert_true (file_starts_with (upper, jpeg_magic, sizeof jpeg_magic));
  }
  close_windows ();
}

/* REQ-0005-11: a save in any format clears the modified mark; the picture in the window keeps its transparency. */
static void
as10_save_clears_modified (void)
{
  static const char *names[] = { "a.png", "a.jpg", "a.bmp", "a.tif", "a.webp" };
  EaselWindow *win = start_window ();
  g_autofree char *dir = g_dir_make_tmp ("easel-ui-XXXXXX", NULL);

  wait_for_layout (win);
  use_pencil (win);
  g_action_group_activate_action (G_ACTION_GROUP (win), "swap-colors", NULL);    /* the stroke is yellow on yellow...  */
  g_action_group_activate_action (G_ACTION_GROUP (win), "swap-colors", NULL);
  for (gsize i = 0; i < G_N_ELEMENTS (names); i++)
    {
      g_autofree char *path = g_build_filename (dir, names[i], NULL);
      g_autofree char *title = g_strdup_printf ("%s – Easel", names[i]);
      g_autofree char *dirty = g_strdup_printf ("• %s – Easel", names[i]);

      stroke (win, 3.5, 3.5 + (double) i, 9.5, 3.5 + (double) i);
      g_assert_true (easel_window_get_modified (win));
      easel_window_save_chosen (win, path);
      g_assert_true (g_file_test (path, G_FILE_TEST_EXISTS));
      g_assert_false (easel_window_get_modified (win));
      g_assert_cmpstr (gtk_window_get_title (GTK_WINDOW (win)), ==, title);
      /* Undo goes to before the saved state: modified again, and the mark is the bullet. */
      g_action_group_activate_action (G_ACTION_GROUP (win), "undo", NULL);
      g_assert_cmpstr (gtk_window_get_title (GTK_WINDOW (win)), ==, dirty);
      g_action_group_activate_action (G_ACTION_GROUP (win), "redo", NULL);
      g_assert_cmpstr (gtk_window_get_title (GTK_WINDOW (win)), ==, title);
    }
  close_windows ();
}

/* REQ-0005-12: Save As, with its dialog, its replace question, and a failure that changes nothing. */
static void
as11_save_as (void)
{
  EaselWindow *win = start_window ();
  g_autofree char *start = fixture_copy ("rgba.png");
  g_autofree char *dir = g_path_get_dirname (start);
  g_autofree char *jpg = g_build_filename (dir, "b.jpg", NULL);
  g_autofree char *png = g_build_filename (dir, "b.png", NULL);
  g_autofree char *plain = g_build_filename (dir, "b", NULL);

  wait_for_layout (win);
  open_and_wait (win, start);
  use_pencil (win);
  stroke (win, 1.5, 1.5, 3.5, 1.5);

  /* The dialog is titled "Save Picture As" and suggests the file's name. */
  g_action_group_activate_action (G_ACTION_GROUP (win), "save-as", NULL);
  wait_for_layout (win);
  {
    GtkWindow *chooser = other_toplevel (win);

    g_assert_nonnull (chooser);
    g_assert_cmpstr (gtk_window_get_title (chooser), ==, "Save Picture As");
    gtk_window_close (chooser);
  }
  wait_for_layout (win);
  g_assert_cmpstr (gtk_window_get_title (GTK_WINDOW (win)), ==, "• rgba.png – Easel");        /* cancelled: nothing changed */
  g_assert_true (easel_window_get_modified (win));

  /* A typed b.jpg: written as JPEG, and from now on it is the picture's file. */
  easel_window_save_chosen (win, jpg);
  g_assert_true (file_starts_with (jpg, jpeg_magic, sizeof jpeg_magic));
  g_assert_cmpstr (gtk_window_get_title (GTK_WINDOW (win)), ==, "b.jpg – Easel");
  g_assert_false (easel_window_get_modified (win));
  stroke (win, 1.5, 2.5, 3.5, 2.5);
  g_action_group_activate_action (G_ACTION_GROUP (win), "save", NULL);                       /* no dialog */
  g_assert_null (other_toplevel (win));
  g_assert_false (easel_window_get_modified (win));
  g_assert_cmpstr (gtk_window_get_title (GTK_WINDOW (win)), ==, "b.jpg – Easel");

  /* A typed name without an extension: b.png, and an existing one asks (REQ-0001-42). */
  g_assert_true (g_file_set_contents (png, "old", -1, NULL));
  easel_window_save_chosen (win, plain);
  wait_for_dialogs (win, 1);
  g_assert_nonnull (open_alert (win));             /* the question is asked; without it the heading below would crash */
  g_assert_cmpstr (adw_alert_dialog_get_heading (open_alert (win)), ==, "Replace “b.png”?");
  respond (win, "cancel");
  wait_for_layout (win);
  {
    g_autofree char *contents = NULL;

    g_assert_true (g_file_get_contents (png, &contents, NULL, NULL));
    g_assert_cmpstr (contents, ==, "old");
  }
  g_assert_cmpstr (gtk_window_get_title (GTK_WINDOW (win)), ==, "b.jpg – Easel");
  easel_window_save_chosen (win, plain);
  wait_for_dialogs (win, 1);
  respond (win, "replace");
  wait_for_layout (win);                                   /* the answer reaches the save on the next iteration */
  g_assert_true (file_starts_with (png, png_magic, sizeof png_magic));
  g_assert_cmpstr (gtk_window_get_title (GTK_WINDOW (win)), ==, "b.png – Easel");

  /* A typed b.jpg that exists: the app asks nothing more (the system dialog has asked already). */
  stroke (win, 1.5, 3.5, 3.5, 3.5);
  easel_window_save_chosen (win, jpg);
  g_assert_cmpint (count_alert_dialogs (GTK_WIDGET (win), &(AdwDialog *) { NULL }), ==, 0);
  g_assert_cmpstr (gtk_window_get_title (GTK_WINDOW (win)), ==, "b.jpg – Easel");
  close_windows ();
}

/* REQ-0005-02: the file actions do nothing while any dialog or prompt is open, and end a stroke or cancel a resize drag
   first. */
static void
as15_actions_ignored_and_stroke_ended (void)
{
  static const char *actions[] = { "new", "open", "save-as" };
  EaselWindow *win = start_window ();
  EaselCanvas *canvas = easel_window_get_canvas (win);
  EaselPaletteBar *bar = easel_window_get_palette_bar (win);
  g_autofree char *dir = g_dir_make_tmp ("easel-ui-XXXXXX", NULL);
  g_autofree char *path = g_build_filename (dir, "a.png", NULL);
  const EaselImage *before = easel_window_get_picture (win);
  double x, y;

  wait_for_layout (win);
  use_pencil (win);
  A_STROKE (win);

  /* The color chooser is open: nothing happens (REQ-0003-14). */
  easel_palette_bar_press (bar, 17, 1, 1);
  easel_palette_bar_press (bar, 17, 1, 2);
  g_assert_true (easel_window_is_choosing_color (win));
  for (gsize i = 0; i < G_N_ELEMENTS (actions); i++)
    g_action_group_activate_action (G_ACTION_GROUP (win), actions[i], NULL);
  g_action_group_activate_action (G_ACTION_GROUP (win), "save", NULL);
  wait_for_layout (win);
  g_assert_cmpint (count_alert_dialogs (GTK_WIDGET (win), &(AdwDialog *) { NULL }), ==, 0);
  g_assert_false (other_toplevel (win) != NULL && GTK_IS_FILE_CHOOSER (other_toplevel (win)));   /* no file dialog */
  g_assert_true (easel_window_get_picture (win) == before);
  g_assert_true (easel_window_get_modified (win));
  choose (win, NULL);

  /* The New prompt is open: the other actions do nothing, and no second prompt comes. */
  g_action_group_activate_action (G_ACTION_GROUP (win), "new", NULL);
  wait_for_dialogs (win, 1);
  for (gsize i = 0; i < G_N_ELEMENTS (actions); i++)
    g_action_group_activate_action (G_ACTION_GROUP (win), actions[i], NULL);
  g_action_group_activate_action (G_ACTION_GROUP (win), "save", NULL);
  wait_for_layout (win);
  g_assert_cmpint (count_alert_dialogs (GTK_WIDGET (win), &(AdwDialog *) { NULL }), ==, 1);
  g_assert_null (other_toplevel (win));
  /* Quit opens no second prompt: libadwaita closes the open one as Cancel, and the window and the picture stay. */
  gtk_window_close (GTK_WINDOW (win));
  wait_for_layout (win);
  wait_for_dialogs (win, 0);
  g_assert_cmpint (count_alert_dialogs (GTK_WIDGET (win), &(AdwDialog *) { NULL }), ==, 0);
  g_assert_true (gtk_widget_get_visible (GTK_WIDGET (win)));
  g_assert_true (easel_window_get_picture (win) == before);
  g_assert_true (easel_window_get_modified (win));

  /* A stroke in progress: Save ends it and saves it (REQ-0001-33); New ends it, and asks. */
  easel_window_save_chosen (win, path);
  easel_canvas_press (canvas, 1, 3.5, 9.5);
  easel_canvas_motion (canvas, 9.5, 9.5);
  g_assert_true (easel_canvas_is_stroking (canvas));
  g_action_group_activate_action (G_ACTION_GROUP (win), "save", NULL);
  g_assert_false (easel_canvas_is_stroking (canvas));
  g_assert_false (easel_window_get_modified (win));                        /* the stroke is in the saved file */
  easel_canvas_release (canvas, 1);
  easel_canvas_press (canvas, 1, 3.5, 12.5);
  easel_canvas_motion (canvas, 9.5, 12.5);
  g_action_group_activate_action (G_ACTION_GROUP (win), "new", NULL);
  g_assert_false (easel_canvas_is_stroking (canvas));
  wait_for_dialogs (win, 1);
  respond (win, "cancel");
  easel_canvas_release (canvas, 1);
  g_assert_true (easel_window_get_picture (win) == before);
  g_assert_cmphex (easel_image_get (before, 5, 12), ==, 0xFFFF0000u);         /* the ended stroke is kept (red: the swatch picked above) */

  /* A resize drag: Open cancels it first, and the picture keeps its size. */
  handle_center (canvas, EASEL_HANDLE_CORNER, &x, &y);
  easel_canvas_press (canvas, 1, x, y);
  easel_canvas_motion (canvas, x + 40, y + 40);
  g_assert_true (easel_canvas_is_resizing (canvas));
  g_action_group_activate_action (G_ACTION_GROUP (win), "open", NULL);
  g_assert_false (easel_canvas_is_resizing (canvas));
  wait_for_layout (win);
  /* The prompt comes first, since the stroke made the picture modified again; answer and close the dialog's remains. */
  wait_for_dialogs (win, 1);
  respond (win, "cancel");
  easel_canvas_release (canvas, 1);
  assert_size (win, 300, 200);
  close_windows ();
}

/* REQ-0005-03: strokes after New go into the new history: Undo undoes only them, and Redo brings them back. */
static void
as2_new_history_records_strokes (void)
{
  EaselWindow *win = start_window ();
  const EaselImage *picture;

  wait_for_layout (win);
  use_pencil (win);
  A_STROKE (win);                                              /* a stroke of the old picture, to be left behind */
  g_action_group_activate_action (G_ACTION_GROUP (win), "new", NULL);
  wait_for_dialogs (win, 1);
  respond (win, "discard");
  picture = easel_window_get_picture (win);
  g_assert_false (action_enabled (win, "undo"));

  stroke (win, 3.5, 20.5, 9.5, 20.5);                          /* the first new stroke */
  stroke (win, 3.5, 30.5, 9.5, 30.5);                          /* the second */
  g_assert_true (easel_window_get_modified (win));
  g_assert_true (action_enabled (win, "undo"));
  g_assert_cmphex (easel_image_get (picture, 5, 30), ==, 0xFF000000u);
  g_action_group_activate_action (G_ACTION_GROUP (win), "undo", NULL);
  g_assert_cmphex (easel_image_get (picture, 5, 30), ==, 0xFFFFFF00u);
  g_assert_cmphex (easel_image_get (picture, 5, 20), ==, 0xFF000000u);
  g_assert_true (action_enabled (win, "undo"));
  g_action_group_activate_action (G_ACTION_GROUP (win), "undo", NULL);
  g_assert_cmphex (easel_image_get (picture, 5, 20), ==, 0xFFFFFF00u);
  g_assert_false (action_enabled (win, "undo"));               /* the stroke before New is not there to undo */
  g_assert_false (easel_window_get_modified (win));
  g_action_group_activate_action (G_ACTION_GROUP (win), "redo", NULL);
  g_action_group_activate_action (G_ACTION_GROUP (win), "redo", NULL);
  g_assert_cmphex (easel_image_get (picture, 5, 30), ==, 0xFF000000u);
  g_assert_false (action_enabled (win, "redo"));
  close_windows ();
}

/* REQ-0005-04: Cancel in the unsaved-changes prompt leaves the picture, its history and its unsaved state as they were. */
static void
as3_cancel_leaves_everything (void)
{
  EaselWindow *win = start_window ();
  const EaselImage *picture;

  wait_for_layout (win);
  use_pencil (win);
  picture = easel_window_get_picture (win);
  stroke (win, 3.5, 3.5, 9.5, 3.5);                            /* A */
  stroke (win, 3.5, 8.5, 9.5, 8.5);                            /* B, then undone: a redo step is waiting */
  g_action_group_activate_action (G_ACTION_GROUP (win), "undo", NULL);
  g_assert_true (action_enabled (win, "undo"));
  g_assert_true (action_enabled (win, "redo"));
  g_assert_true (easel_window_get_modified (win));

  g_action_group_activate_action (G_ACTION_GROUP (win), "new", NULL);
  wait_for_dialogs (win, 1);
  respond (win, "cancel");

  g_assert_true (easel_window_get_picture (win) == picture);                       /* the same picture */
  g_assert_cmphex (easel_image_get (picture, 5, 3), ==, 0xFF000000u);               /* A is there */
  g_assert_cmphex (easel_image_get (picture, 5, 8), ==, 0xFFFFFF00u);               /* B is undone */
  g_assert_true (easel_window_get_modified (win));
  g_assert_cmpstr (gtk_window_get_title (GTK_WINDOW (win)), ==, "• Untitled – Easel");
  /* The history is whole: Redo brings B back, and two Undos go back to the start. */
  g_assert_true (action_enabled (win, "undo"));
  g_assert_true (action_enabled (win, "redo"));
  g_action_group_activate_action (G_ACTION_GROUP (win), "redo", NULL);
  g_assert_cmphex (easel_image_get (picture, 5, 8), ==, 0xFF000000u);
  g_action_group_activate_action (G_ACTION_GROUP (win), "undo", NULL);
  g_action_group_activate_action (G_ACTION_GROUP (win), "undo", NULL);
  g_assert_cmphex (easel_image_get (picture, 5, 3), ==, 0xFFFFFF00u);
  g_assert_false (easel_window_get_modified (win));
  close_windows ();
}

/* REQ-0005-18: a Save that cannot get the memory shows the save error and changes nothing. */
static void
as19_save_memory_error_dialog (void)
{
  EaselWindow *win;
  g_autofree char *dir = g_dir_make_tmp ("easel-ui-XXXXXX", NULL);
  g_autofree char *path = g_build_filename (dir, "big.png", NULL);
  AdwAlertDialog *dialog;
  struct rlimit old;

  write_settings ("[new-picture]\nwidth=6000\nheight=6000\n");
  win = start_window ();
  wait_for_layout (win);
  use_pencil (win);
  stroke (win, 3.5, 3.5, 9.5, 3.5);
  g_assert_true (limit_memory (&old, 100));
  easel_window_save_chosen (win, path);
  g_assert_cmpint (setrlimit (RLIMIT_AS, &old), ==, 0);
  wait_for_dialogs (win, 1);
  dialog = open_alert (win);
  g_assert_cmpstr (adw_alert_dialog_get_heading (dialog), ==, "Could Not Save “big.png”");
  g_assert_nonnull (g_strstr_len (adw_alert_dialog_get_body (dialog), -1, "Cannot allocate memory"));
  g_assert_false (g_file_test (path, G_FILE_TEST_EXISTS));
  g_assert_true (easel_window_get_modified (win));
  g_assert_cmpstr (gtk_window_get_title (GTK_WINDOW (win)), ==, "• Untitled – Easel");
  adw_dialog_close (ADW_DIALOG (dialog));
  wait_for_dialogs (win, 0);
  close_windows ();
  write_settings ("[new-picture]\nwidth=300\nheight=200\nbackground=#FFFF00\n");
}

/* REQ-0005-12: a Save As that fails keeps the file, the title and the modified state. */
static void
as11_save_as_fails (void)
{
  EaselWindow *win = start_window ();
  g_autofree char *start = fixture_copy ("rgba.png");
  g_autofree char *dir = g_path_get_dirname (start);
  g_autofree char *target = g_build_filename (dir, "x.jpg", NULL);

  if (geteuid () == 0)
    {
      g_test_skip ("root ignores folder permissions");
      close_windows ();
      return;
    }
  wait_for_layout (win);
  open_and_wait (win, start);
  use_pencil (win);
  stroke (win, 1.5, 1.5, 3.5, 1.5);
  g_assert_cmpint (g_chmod (dir, 0500), ==, 0);
  easel_window_save_chosen (win, target);
  wait_for_dialogs (win, 1);
  g_assert_cmpstr (adw_alert_dialog_get_heading (open_alert (win)), ==, "Could Not Save “x.jpg”");
  g_assert_cmpstr (gtk_window_get_title (GTK_WINDOW (win)), ==, "• rgba.png – Easel");
  g_assert_true (easel_window_get_modified (win));
  g_chmod (dir, 0700);
  adw_dialog_close (ADW_DIALOG (open_alert (win)));
  wait_for_dialogs (win, 0);
  close_windows ();
}

/* REQ-0005-13: Save writes the file in its own format without asking; a file with no saveable extension asks. */
static void
as12_save_keeps_format (void)
{
  EaselWindow *win = start_window ();
  g_autofree char *jpg = fixture_copy ("flat.jpg");
  g_autofree char *gif = fixture_copy ("transparent.gif");
  g_autofree char *dat = fixture_copy ("named.dat");

  wait_for_layout (win);
  use_pencil (win);
  open_and_wait (win, jpg);
  stroke (win, 2.5, 2.5, 6.5, 2.5);
  g_assert_true (easel_window_get_modified (win));
  g_action_group_activate_action (G_ACTION_GROUP (win), "save", NULL);
  g_assert_null (other_toplevel (win));                                  /* no dialog */
  g_assert_false (easel_window_get_modified (win));
  g_assert_true (file_starts_with (jpg, jpeg_magic, sizeof jpeg_magic));
  {
    g_autofree char *contents = NULL;
    gsize len;
    g_autoptr (GError) error = NULL;
    g_autoptr (GdkPixbuf) pb = gdk_pixbuf_new_from_file (jpg, &error);

    g_assert_no_error (error);
    g_assert_cmpint (gdk_pixbuf_get_width (pb), ==, 16);
    g_assert_true (g_file_get_contents (jpg, &contents, &len, NULL));
  }

  /* A GIF, and a PNG named .dat, cannot be saved in place: Save is a Save As with a .png suggestion. */
  open_and_wait (win, gif);
  g_action_group_activate_action (G_ACTION_GROUP (win), "save", NULL);
  wait_for_layout (win);
  {
    GtkWindow *chooser = other_toplevel (win);
    g_autofree char *name = NULL;

    g_assert_nonnull (chooser);
    g_assert_true (GTK_IS_FILE_CHOOSER (chooser));
    G_GNUC_BEGIN_IGNORE_DEPRECATIONS   /* the in-process chooser is the only way to read its suggested name */
    name = gtk_file_chooser_get_current_name (GTK_FILE_CHOOSER (chooser));
    G_GNUC_END_IGNORE_DEPRECATIONS
    g_assert_cmpstr (name, ==, "transparent.png");
    g_assert_cmpstr (gtk_window_get_title (chooser), ==, "Save Picture As");
    gtk_window_close (chooser);
  }
  wait_for_layout (win);
  open_and_wait (win, dat);
  g_action_group_activate_action (G_ACTION_GROUP (win), "save", NULL);
  wait_for_layout (win);
  {
    GtkWindow *chooser = other_toplevel (win);
    g_autofree char *name = NULL;

    g_assert_nonnull (chooser);
    G_GNUC_BEGIN_IGNORE_DEPRECATIONS   /* the in-process chooser is the only way to read its suggested name */
    name = gtk_file_chooser_get_current_name (GTK_FILE_CHOOSER (chooser));
    G_GNUC_END_IGNORE_DEPRECATIONS
    g_assert_cmpstr (name, ==, "named.png");
    gtk_window_close (chooser);
  }
  wait_for_layout (win);
  close_windows ();
}

/* REQ-0005-14: the quality of the settings file reaches the JPEG that Save writes. */
static void
as16_quality_setting_reaches_save (void)
{
  static const int qualities[] = { 5, 100 };
  g_autofree char *dir = g_dir_make_tmp ("easel-ui-XXXXXX", NULL);
  g_autofree char *noise = g_build_filename (dir, "noise.png", NULL);
  gsize sizes[2];
  EaselImage *img = easel_image_new (64, 64, 0xFF000000u);
  g_autoptr (GError) error = NULL;

  for (int y = 0; y < 64; y++)
    for (int x = 0; x < 64; x++)
      img->px[y * 64 + x] = 0xFF000000u | (uint32_t) ((x * 37 + y * 11) % 256) << 16 | (uint32_t) ((x * 5 + y * 53) % 256) << 8
                            | (uint32_t) ((x * y) % 256);
  g_assert_true (easel_picture_save (img, noise, EASEL_FORMAT_PNG, 90, &error));
  easel_image_free (img);

  for (gsize i = 0; i < 2; i++)
    {
      g_autofree char *settings = g_strdup_printf ("[new-picture]\nwidth=300\nheight=200\n[save]\njpeg-quality=%d\n", qualities[i]);
      g_autofree char *jpg = g_build_filename (dir, i == 0 ? "low.jpg" : "high.jpg", NULL);
      g_autofree char *contents = NULL;
      EaselWindow *win;

      write_settings (settings);
      win = start_window ();
      wait_for_layout (win);
      open_and_wait (win, noise);
      easel_window_save_chosen (win, jpg);
      g_assert_true (g_file_get_contents (jpg, &contents, &sizes[i], NULL));
      close_windows ();
    }
  g_assert_cmpuint (sizes[0], <, sizes[1]);
  write_settings ("[new-picture]\nwidth=300\nheight=200\nbackground=#FFFF00\n");
}

/* A format that is not installed: the save fails with gdk-pixbuf's reason, the old file keeps its bytes and the picture
   keeps its file, title and modified state (REQ-0001-31, Plan "A missing loader or saver"). */
static void
missing_saver_error_dialog (void)
{
  EaselWindow *win = start_window ();
  g_autofree char *dir = g_dir_make_tmp ("easel-ui-XXXXXX", NULL);
  g_autofree char *path = g_build_filename (dir, "a.webp", NULL);
  g_autofree char *contents = NULL;
  AdwAlertDialog *dialog;

  wait_for_layout (win);
  use_pencil (win);
  A_STROKE (win);
  g_assert_true (g_file_set_contents (path, "old webp", -1, NULL));
  easel_picture_file_hide_format ("webp");
  easel_window_save_chosen (win, path);
  easel_picture_file_hide_format (NULL);
  wait_for_dialogs (win, 1);
  dialog = open_alert (win);
  g_assert_cmpstr (adw_alert_dialog_get_heading (dialog), ==, "Could Not Save “a.webp”");
  g_assert_nonnull (g_strstr_len (adw_alert_dialog_get_body (dialog), -1, "is not supported"));
  g_assert_true (g_file_get_contents (path, &contents, NULL, NULL));
  g_assert_cmpstr (contents, ==, "old webp");
  g_assert_true (easel_window_get_modified (win));
  g_assert_cmpstr (gtk_window_get_title (GTK_WINDOW (win)), ==, "• Untitled – Easel");
  adw_dialog_close (ADW_DIALOG (dialog));
  wait_for_dialogs (win, 0);
  close_windows ();
}

/* REQ-0005-16: after Save As the saved state is the new file's; undoing to before it is modified again. */
static void
as14_save_as_moves_the_saved_state (void)
{
  EaselWindow *win = start_window ();
  g_autofree char *dir = g_dir_make_tmp ("easel-ui-XXXXXX", NULL);
  g_autofree char *a = g_build_filename (dir, "a.png", NULL);
  g_autofree char *b = g_build_filename (dir, "b.bmp", NULL);

  wait_for_layout (win);
  use_pencil (win);
  A_STROKE (win);
  easel_window_save_chosen (win, a);
  stroke (win, 3.5, 6.5, 9.5, 6.5);
  easel_window_save_chosen (win, b);                      /* the saved state is now after the second stroke */
  g_assert_false (easel_window_get_modified (win));
  g_action_group_activate_action (G_ACTION_GROUP (win), "undo", NULL);
  g_assert_true (easel_window_get_modified (win));
  g_action_group_activate_action (G_ACTION_GROUP (win), "redo", NULL);
  g_assert_false (easel_window_get_modified (win));
  g_action_group_activate_action (G_ACTION_GROUP (win), "undo", NULL);
  g_action_group_activate_action (G_ACTION_GROUP (win), "undo", NULL);
  g_assert_true (easel_window_get_modified (win));
  close_windows ();
}

/* ---- 0006: the cursors ---- */

/* The size of the canvas's cursor texture, or 0 when it is a named cursor. */
static int
cursor_width (EaselWindow *win)
{
  GdkCursor *cursor = gtk_widget_get_cursor (GTK_WIDGET (easel_window_get_canvas (win)));
  GdkTexture *texture = cursor != NULL ? gdk_cursor_get_texture (cursor) : NULL;

  return texture != NULL ? gdk_texture_get_width (texture) : 0;
}

/* AS-12 (REQ-0006-16), the wiring: the cursor over the canvas follows the tool, the preset and the zoom; its geometry is
   checked in the core (as12_cursor_geometry), how it looks is the manual check M3. */
static void
as12_cursor_follows_tool_preset_and_zoom (void)
{
  EaselWindow *win = start_window ();

  wait_for_layout (win);
  g_assert_cmpint (cursor_width (win), ==, 0);                              /* Pan: the hand */
  use_tool (win, "pencil");
  g_assert_cmpint (cursor_width (win), ==, 32);                             /* the pencil's picture */
  use_tool (win, "eraser");
  choose_preset (win, "Eraser, 10 pixels");
  g_assert_cmpint (cursor_width (win), ==, 12);                             /* 10 at 100 %, and the white line */
  choose_preset (win, "Eraser, 6 pixels");
  g_assert_cmpint (cursor_width (win), ==, 8);
  g_action_group_activate_action (G_ACTION_GROUP (win), "zoom-large", NULL);
  g_assert_cmpint (cursor_width (win), ==, 26);                             /* 6 at 400 % */
  use_tool (win, "brush");
  choose_preset (win, "Circle, 7 pixels");
  g_assert_cmpint (cursor_width (win), ==, 30);                             /* 7 at 400 % */
  g_action_group_activate_action (G_ACTION_GROUP (win), "zoom-normal", NULL);
  g_assert_cmpint (cursor_width (win), ==, 9);
  use_tool (win, "airbrush");
  g_assert_cmpint (cursor_width (win), ==, 32);                             /* the spray can's picture */
  close_windows ();
}

/* AS-1 (REQ-0007-12): each of the three tools has its own picture as the cursor, with the hotspot at the tip; a resize
   handle under the pointer wins. How they look is the manual check M4. */
static void
as1_cursors_follow_tool (void)
{
  EaselWindow *win = start_window ();
  EaselCanvas *canvas = easel_window_get_canvas (win);
  static const struct { const char *tool; int hot_x, hot_y; } tools[] = {
    { "fill", 4, 29 }, { "pick", 2, 30 }, { "magnifier", 12, 12 },
  };
  graphene_rect_t r;

  wait_for_layout (win);
  for (guint i = 0; i < G_N_ELEMENTS (tools); i++)
    {
      GdkCursor *cursor;

      use_tool (win, tools[i].tool);
      g_assert_cmpint (cursor_width (win), ==, 32);
      cursor = gtk_widget_get_cursor (GTK_WIDGET (canvas));
      g_assert_cmpint (gdk_cursor_get_hotspot_x (cursor), ==, tools[i].hot_x);
      g_assert_cmpint (gdk_cursor_get_hotspot_y (cursor), ==, tools[i].hot_y);

      /* Over the corner handle: the resize cursor; away from it: the tool's again. */
      g_assert_true (easel_canvas_get_handle_rect (canvas, EASEL_HANDLE_CORNER, &r));
      easel_canvas_pointer_at (canvas, r.origin.x + r.size.width / 2, r.origin.y + r.size.height / 2);
      g_assert_cmpstr (gdk_cursor_get_name (gtk_widget_get_cursor (GTK_WIDGET (canvas))), ==, "nwse-resize");
      easel_canvas_pointer_at (canvas, 10, 10);
      g_assert_cmpint (cursor_width (win), ==, 32);
    }
  use_tool (win, "pan");
  g_assert_cmpint (cursor_width (win), ==, 0);
  close_windows ();
}

/* ---- 0006: the hints ---- */

/* AS-13 (REQ-0006-17): the status bar hint of each new tool, and of the Eraser's two modes. */
static void
as13_hints (void)
{
  EaselWindow *win = start_window ();
  EaselStatusBar *bar = easel_window_get_status_bar (win);

  wait_for_layout (win);
  use_tool (win, "brush");
  g_assert_cmpstr (easel_status_bar_get_text (bar, EASEL_STATUS_HINT), ==,
                   "Paint with the foreground color. The secondary button uses the background color.");
  use_tool (win, "airbrush");
  g_assert_cmpstr (easel_status_bar_get_text (bar, EASEL_STATUS_HINT), ==,
                   "Spray with the foreground color. The secondary button uses the background color.");
  use_tool (win, "eraser");
  g_assert_cmpstr (easel_status_bar_get_text (bar, EASEL_STATUS_HINT), ==,
                   "Erase to the background color. The secondary button changes only the foreground color.");
  /* The mode changes the hint at once, and back. */
  choose_preset (win, eraser_mode_tips[1]);
  g_assert_cmpstr (easel_status_bar_get_text (bar, EASEL_STATUS_HINT), ==,
                   "Erase to transparency. The secondary button erases only the foreground color.");
  /* Leaving and coming back keeps the mode, so the hint. */
  use_tool (win, "brush");
  use_tool (win, "eraser");
  g_assert_cmpstr (easel_status_bar_get_text (bar, EASEL_STATUS_HINT), ==,
                   "Erase to transparency. The secondary button erases only the foreground color.");
  choose_preset (win, eraser_mode_tips[0]);
  g_assert_cmpstr (easel_status_bar_get_text (bar, EASEL_STATUS_HINT), ==,
                   "Erase to the background color. The secondary button changes only the foreground color.");
  /* A change of the size leaves it. */
  choose_preset (win, "Eraser, 4 pixels");
  g_assert_cmpstr (easel_status_bar_get_text (bar, EASEL_STATUS_HINT), ==,
                   "Erase to the background color. The secondary button changes only the foreground color.");
  /* The other tools still have theirs. */
  use_tool (win, "pencil");
  g_assert_cmpstr (easel_status_bar_get_text (bar, EASEL_STATUS_HINT), ==, PENCIL_HINT);
  use_tool (win, "pan");
  g_assert_cmpstr (easel_status_bar_get_text (bar, EASEL_STATUS_HINT), ==, PAN_HINT);
  close_windows ();
}

/* ---- 0007: Fill, Pick Color and the Magnifier ---- */

/* AS-1 (REQ-0007-01, REQ-0007-13): the toolbox order, the tooltips, the hints, and no options for Pick Color and Fill. */
static void
as1_new_tools_toolbox_hints (void)
{
  static const struct { const char *name, *tip; int col, row; EaselTool tool; } tools[] = {
    { "pan", "Pan", 0, 0, EASEL_TOOL_PAN }, { "magnifier", "Magnifier", 1, 0, EASEL_TOOL_MAGNIFIER },
    { "eraser", "Eraser", 0, 1, EASEL_TOOL_ERASER }, { "pick", "Pick Color", 1, 1, EASEL_TOOL_PICK },
    { "fill", "Fill", 0, 2, EASEL_TOOL_FILL }, { "pencil", "Pencil", 1, 2, EASEL_TOOL_PENCIL },
    { "brush", "Brush", 0, 3, EASEL_TOOL_BRUSH }, { "airbrush", "Airbrush", 1, 3, EASEL_TOOL_AIRBRUSH },
  };
  EaselWindow *win = start_window ();
  EaselStatusBar *bar = easel_window_get_status_bar (win);
  GtkToggleButton *buttons[8];
  graphene_rect_t first, r;

  wait_for_layout (win);
  for (guint i = 0; i < G_N_ELEMENTS (tools); i++)
    {
      buttons[i] = find_tool_button (GTK_WIDGET (win), tools[i].name);
      g_assert_nonnull (buttons[i]);
      g_assert_cmpstr (gtk_widget_get_tooltip_text (GTK_WIDGET (buttons[i])), ==, tools[i].tip);
      g_assert_cmpstr (gtk_button_get_icon_name (GTK_BUTTON (buttons[i])), !=, NULL);
    }
  /* Two columns, row by row: Pan, Magnifier / Eraser, Pick Color / Fill, Pencil / Brush, Airbrush. */
  first = bounds_in (GTK_WIDGET (buttons[0]), GTK_WIDGET (win));
  for (guint i = 1; i < G_N_ELEMENTS (tools); i++)
    {
      graphene_rect_t before = bounds_in (GTK_WIDGET (buttons[i - 1]), GTK_WIDGET (win));

      r = bounds_in (GTK_WIDGET (buttons[i]), GTK_WIDGET (win));
      if (tools[i].col == 0)
        g_assert_cmpfloat_with_epsilon (r.origin.x, first.origin.x, 1.0);
      else
        g_assert_cmpfloat (r.origin.x, >, first.origin.x + first.size.width - 1);
      if (tools[i].row > tools[i - 1].row)
        g_assert_cmpfloat (r.origin.y, >, before.origin.y + 1);
      else
        g_assert_cmpfloat_with_epsilon (r.origin.y, before.origin.y, 1.0);
    }
  /* Each can be chosen, shows selected, and the others do not. */
  for (guint i = 0; i < G_N_ELEMENTS (tools); i++)
    {
      g_signal_emit_by_name (buttons[i], "clicked");
      g_assert_cmpint (easel_window_get_tool (win), ==, tools[i].tool);
      for (guint j = 0; j < G_N_ELEMENTS (tools); j++)
        g_assert_true (gtk_toggle_button_get_active (buttons[j]) == (i == j));
    }
  /* The hints of REQ-0007-13, and no options panel for Pick Color and Fill. */
  use_tool (win, "fill");
  wait_for_layout (win);
  g_assert_cmpstr (easel_status_bar_get_text (bar, EASEL_STATUS_HINT), ==, "Fills an area with the current drawing color.");
  assert_no_options_shown (win);
  use_tool (win, "pick");
  wait_for_layout (win);
  g_assert_cmpstr (easel_status_bar_get_text (bar, EASEL_STATUS_HINT), ==, "Picks up a color from the picture for drawing.");
  assert_no_options_shown (win);
  use_tool (win, "magnifier");
  g_assert_cmpstr (easel_status_bar_get_text (bar, EASEL_STATUS_HINT), ==, "Changes the magnification.");
  close_windows ();
}

/* The Magnifier's selected level: the state of win.magnifier-level, which the level buttons set (REQ-0007-02). */
static int
magnifier_level (EaselWindow *win)
{
  g_autoptr (GVariant) state = g_action_group_get_action_state (G_ACTION_GROUP (win), "magnifier-level");

  return g_variant_get_int32 (state);
}

static const char *const magnifier_tips[5] = { "100 %", "200 %", "400 %", "600 %", "800 %" };

/* AS-2 (REQ-0007-02, amended by REQ-0008-02): the Magnifier's five levels, 2x selected, choosing one zooms nothing, and the choice is kept. */
static void
as2_magnifier_level_kept (void)
{
  static const char *const labels[5] = { "1x", "2x", "4x", "6x", "8x" };
  EaselWindow *win = start_window ();
  EaselToolOptions *options = easel_window_get_tool_options (win);

  wait_for_layout (win);
  g_assert_cmpint (magnifier_level (win), ==, 200);
  use_tool (win, "magnifier");
  wait_for_layout (win);
  g_assert_true (gtk_widget_get_mapped (GTK_WIDGET (options)));
  g_assert_cmpint (check_presets (win, magnifier_tips, 5, 2), ==, 1);        /* 1x 2x / 4x 6x / 8x; 2x is on */
  for (int i = 0; i < 5; i++)
    g_assert_cmpstr (gtk_button_get_label (GTK_BUTTON (find_by_tooltip (GTK_WIDGET (win), magnifier_tips[i]))), ==, labels[i]);
  g_assert_false (on_screen (win, "Circle, 4 pixels"));                      /* only its own group */

  choose_preset (win, "800 %");
  g_assert_cmpint (easel_window_get_zoom (win), ==, 100);                    /* choosing zooms nothing */
  g_assert_cmpint (check_presets (win, magnifier_tips, 5, 2), ==, 4);
  g_assert_cmpint (magnifier_level (win), ==, 800);
  choose_preset (win, "100 %");
  g_assert_cmpint (easel_window_get_zoom (win), ==, 100);
  g_assert_cmpint (magnifier_level (win), ==, 100);
  choose_preset (win, "400 %");
  g_assert_cmpint (easel_window_get_zoom (win), ==, 100);
  g_assert_cmpint (magnifier_level (win), ==, 400);
  choose_preset (win, "800 %");
  g_assert_false (easel_window_get_modified (win));
  g_assert_false (action_enabled (win, "undo"));

  use_tool (win, "pencil");
  wait_for_layout (win);
  g_assert_false (on_screen (win, "800 %"));
  use_tool (win, "brush");
  wait_for_layout (win);
  g_assert_false (on_screen (win, "800 %"));
  use_tool (win, "magnifier");
  wait_for_layout (win);
  g_assert_cmpint (check_presets (win, magnifier_tips, 5, 2), ==, 4);        /* kept */
  g_assert_cmpint (magnifier_level (win), ==, 800);
  close_windows ();
}

/* A black ring (a rectangle outline, x 50..150, y 50..120) with the Pencil, on the yellow start-up picture; leaves Fill
   active. */
static void
draw_ring (EaselWindow *win)
{
  use_tool (win, "pencil");
  stroke_with (win, 1, 50, 50, 150, 50);
  stroke_with (win, 1, 150, 50, 150, 120);
  stroke_with (win, 1, 150, 120, 50, 120);
  stroke_with (win, 1, 50, 120, 50, 50);
  use_tool (win, "fill");
}

/* AS-6 (REQ-0007-05): a press fills the area inside the ring, one undo step; the secondary button uses the background. */
static void
as6_fill_on_canvas (void)
{
  EaselWindow *win = start_window ();
  EaselCanvas *canvas = easel_window_get_canvas (win);
  const EaselImage *img = easel_window_get_picture (win);
  const uint32_t black = 0xFF000000u, yellow = 0xFFFFFF00u;

  wait_for_layout (win);
  draw_ring (win);
  g_assert_cmpint (count_color (img, black), ==, 340);
  easel_canvas_press (canvas, 1, 100.5, 80.5);
  g_assert_cmpint (count_color (img, black), ==, 340 + 99 * 69);                 /* the inside took the foreground */
  g_assert_cmpint (count_color (img, yellow), ==, 300 * 200 - 340 - 99 * 69);
  g_assert_cmphex (easel_image_get (img, 0, 0), ==, yellow);                     /* outside the ring: untouched */
  easel_canvas_release (canvas, 1);
  g_assert_true (easel_window_get_modified (win));

  /* The secondary button fills the whole black area (ring and inside are one area now) with the background. */
  easel_canvas_press (canvas, 3, 100.5, 80.5);
  easel_canvas_release (canvas, 3);
  g_assert_cmpint (count_color (img, black), ==, 0);
  g_assert_cmpint (count_color (img, yellow), ==, 300 * 200);

  /* Each fill is one step: two Undos give back the ring alone, a third takes a stroke of the ring away. */
  g_action_group_activate_action (G_ACTION_GROUP (win), "undo", NULL);
  g_assert_cmpint (count_color (img, black), ==, 340 + 99 * 69);
  g_action_group_activate_action (G_ACTION_GROUP (win), "undo", NULL);
  g_assert_cmpint (count_color (img, black), ==, 340);
  g_action_group_activate_action (G_ACTION_GROUP (win), "redo", NULL);
  g_assert_cmpint (count_color (img, black), ==, 340 + 99 * 69);
  close_windows ();
}

/* AS-8 (REQ-0007-06): the same color, a point outside, the middle button, and a second button held: no step, no mark. */
static void
as8_fill_nothing_on_canvas (void)
{
  EaselWindow *win = start_window ();
  EaselCanvas *canvas = easel_window_get_canvas (win);
  const EaselImage *img = easel_window_get_picture (win);

  wait_for_layout (win);
  use_tool (win, "fill");
  easel_canvas_press (canvas, 3, 100.5, 80.5);                                   /* yellow on yellow */
  easel_canvas_release (canvas, 3);
  easel_canvas_press (canvas, 1, -10.5, 80.5);                                   /* outside the picture */
  easel_canvas_release (canvas, 1);
  easel_canvas_press (canvas, 1, 400.5, 80.5);
  easel_canvas_release (canvas, 1);
  easel_canvas_press (canvas, 2, 100.5, 80.5);                                   /* the middle button */
  easel_canvas_release (canvas, 2);
  g_assert_false (action_enabled (win, "undo"));
  g_assert_false (easel_window_get_modified (win));
  g_assert_cmpint (count_color (img, 0xFFFFFF00u), ==, 300 * 200);

  /* A press fills at once; the other button pressed while it is held does nothing more (there is no stroke to cancel). */
  easel_canvas_press (canvas, 1, 100.5, 80.5);
  g_assert_cmpint (count_color (img, 0xFF000000u), ==, 300 * 200);
  easel_canvas_press (canvas, 3, 100.5, 80.5);
  g_assert_cmpint (count_color (img, 0xFF000000u), ==, 300 * 200);
  easel_canvas_release (canvas, 3);
  easel_canvas_release (canvas, 1);
  g_action_group_activate_action (G_ACTION_GROUP (win), "undo", NULL);
  g_assert_cmpint (count_color (img, 0xFFFFFF00u), ==, 300 * 200);               /* one step */
  g_assert_false (action_enabled (win, "undo"));
  close_windows ();
}

/* AS-9 (REQ-0007-07): a fill that cannot get its memory shows the dialog and changes nothing. The failure is made with the
   fill's test hook (the core test also does it with a real address-space limit). */
static void
as9_fill_memory_dialog (void)
{
  EaselWindow *win = start_window ();
  EaselCanvas *canvas = easel_window_get_canvas (win);
  const EaselImage *img = easel_window_get_picture (win);
  AdwAlertDialog *dialog;

  wait_for_layout (win);
  draw_ring (win);

  /* 99 rows inside the ring need 99 spans, more than the 768 bytes the work buffer may have. */
  easel_canvas_set_fill_memory_limit (canvas, 1000);
  easel_canvas_press (canvas, 1, 100.5, 80.5);
  easel_canvas_release (canvas, 1);
  easel_canvas_set_fill_memory_limit (canvas, SIZE_MAX);
  wait_for_dialogs (win, 1);
  dialog = open_alert (win);
  g_assert_nonnull (dialog);
  g_assert_cmpstr (adw_alert_dialog_get_heading (dialog), ==, "Could Not Fill");
  g_assert_cmpstr (adw_alert_dialog_get_body (dialog), ==, "There is not enough memory to fill this area.");
  g_assert_cmpint (count_color (img, 0xFF000000u), ==, 340);                       /* nothing changed: the ring alone */
  adw_dialog_close (ADW_DIALOG (dialog));
  wait_for_dialogs (win, 0);

  /* No step was added: one Undo takes a stroke of the ring away, not a fill; and the same press works with memory. */
  easel_canvas_press (canvas, 1, 100.5, 80.5);
  easel_canvas_release (canvas, 1);
  g_assert_cmpint (count_color (img, 0xFF000000u), ==, 340 + 99 * 69);
  close_windows ();
}

/* ---- 0007: Pick Color ---- */

/* Makes swatch n the foreground color, and returns it. */
static uint32_t
choose_swatch (EaselWindow *win, int n)
{
  easel_palette_bar_press (easel_window_get_palette_bar (win), n, 1, 1);
  return easel_window_get_foreground (win);
}

/* A dot of the foreground color at picture pixel (x, y), drawn with the Pencil. */
static void
dot (EaselWindow *win, int x, int y)
{
  use_tool (win, "pencil");
  stroke_with (win, 1, x, y, x, y);
}

/* AS-10 (REQ-0007-08, REQ-0007-09, REQ-0007-11): the pick follows the pointer, the previous tool comes back, no edit. */
static void
as10_pick_follows_and_returns (void)
{
  EaselWindow *win = start_window ();
  EaselCanvas *canvas = easel_window_get_canvas (win);
  const EaselImage *img = easel_window_get_picture (win);
  uint32_t red, green;

  wait_for_layout (win);
  red = choose_swatch (win, 3);
  dot (win, 10, 10);
  green = choose_swatch (win, 5);
  dot (win, 20, 10);
  g_assert_cmphex (red, !=, green);
  choose_swatch (win, 1);                                                     /* black again */
  g_assert_cmphex (easel_window_get_foreground (win), ==, 0xFF000000u);
  use_tool (win, "pick");

  easel_canvas_press (canvas, 1, 10.5, 10.5);
  g_assert_cmphex (easel_window_get_foreground (win), ==, red);               /* at once */
  g_assert_true (easel_canvas_is_stroking (canvas));
  easel_canvas_motion (canvas, 20.5, 10.5);
  g_assert_cmphex (easel_window_get_foreground (win), ==, green);             /* follows the pointer */
  easel_canvas_motion (canvas, -30.5, 10.5);                                   /* off the picture: the last stays */
  g_assert_cmphex (easel_window_get_foreground (win), ==, green);
  g_assert_cmpint (easel_window_get_tool (win), ==, EASEL_TOOL_PICK);          /* still Pick Color while held */
  easel_canvas_release (canvas, 1);
  g_assert_cmpint (easel_window_get_tool (win), ==, EASEL_TOOL_PENCIL);        /* the previous tool is back */
  g_assert_cmphex (easel_window_get_foreground (win), ==, green);
  g_assert_false (easel_canvas_is_stroking (canvas));

  /* Not an edit: one Undo takes the last dot away, so the pick added no step; the picture was not touched. */
  g_assert_cmphex (easel_image_get (img, 10, 10), ==, red);
  g_action_group_activate_action (G_ACTION_GROUP (win), "undo", NULL);
  g_assert_cmphex (easel_image_get (img, 20, 10), ==, 0xFFFFFF00u);
  g_assert_cmphex (easel_image_get (img, 10, 10), ==, red);

  /* The secondary button sets the background color. */
  use_tool (win, "pick");
  easel_canvas_press (canvas, 3, 10.5, 10.5);
  g_assert_cmphex (easel_window_get_background (win), ==, red);
  g_assert_cmphex (easel_window_get_foreground (win), ==, green);
  easel_canvas_release (canvas, 3);
  g_assert_cmpint (easel_window_get_tool (win), ==, EASEL_TOOL_PENCIL);
  close_windows ();
}

/* AS-11 (REQ-0007-08): the color is exact in all four channels, a transparent pixel gives a transparent color. */
static void
as11_pick_exact_colors (void)
{
  EaselWindow *win;
  EaselCanvas *canvas;

  write_settings ("[new-picture]\nwidth=300\nheight=200\nbackground=transparent\n");
  win = start_window ();
  canvas = easel_window_get_canvas (win);
  wait_for_layout (win);
  {
    uint32_t half = 0x80FF8000u;

    easel_palette_bar_press_indicator (easel_window_get_palette_bar (win), TRUE);   /* the chooser for the foreground */
    choose (win, &half);
  }
  g_assert_cmphex (easel_window_get_foreground (win), ==, 0x80FF8000u);
  dot (win, 10, 10);
  g_assert_cmphex (easel_image_get (easel_window_get_picture (win), 10, 10), ==, 0x80FF8000u);
  choose_swatch (win, 1);
  use_tool (win, "pick");
  easel_canvas_press (canvas, 1, 10.5, 10.5);
  g_assert_cmphex (easel_window_get_foreground (win), ==, 0x80FF8000u);
  easel_canvas_motion (canvas, 11.5, 10.5);                                     /* a transparent pixel next to it */
  g_assert_cmphex (easel_window_get_foreground (win), ==, 0x00000000u);
  easel_canvas_release (canvas, 1);
  close_windows ();
  write_settings ("[new-picture]\nwidth=300\nheight=200\nbackground=#FFFF00\n");
}

/* AS-12 (REQ-0007-08, REQ-0007-09): a press outside the picture picks nothing and the tool stays; from start-up it is Pan. */
static void
as12_pick_outside_picture (void)
{
  EaselWindow *win = start_window ();
  EaselCanvas *canvas = easel_window_get_canvas (win);

  wait_for_layout (win);
  g_assert_cmpint (easel_window_get_tool (win), ==, EASEL_TOOL_PAN);
  use_tool (win, "pick");
  easel_canvas_press (canvas, 1, -10.5, -10.5);
  g_assert_false (easel_canvas_is_stroking (canvas));
  easel_canvas_release (canvas, 1);
  g_assert_cmpint (easel_window_get_tool (win), ==, EASEL_TOOL_PICK);
  g_assert_cmphex (easel_window_get_foreground (win), ==, 0xFF000000u);
  easel_canvas_press (canvas, 2, 5.5, 5.5);                                    /* the middle button does nothing */
  easel_canvas_release (canvas, 2);
  g_assert_cmpint (easel_window_get_tool (win), ==, EASEL_TOOL_PICK);
  easel_canvas_press (canvas, 3, 5.5, 5.5);
  easel_canvas_release (canvas, 3);
  g_assert_cmpint (easel_window_get_tool (win), ==, EASEL_TOOL_PAN);            /* picked: back to Pan */
  g_assert_cmphex (easel_window_get_background (win), ==, 0xFFFFFF00u);
  close_windows ();
}

/* AS-13 (REQ-0007-10): the other button and Undo cancel; losing the focus ends the pick and keeps it. */
static void
as13_pick_cancel_and_focus (void)
{
  EaselWindow *win = start_window ();
  EaselCanvas *canvas = easel_window_get_canvas (win);
  uint32_t red;

  wait_for_layout (win);
  red = choose_swatch (win, 3);
  dot (win, 10, 10);
  choose_swatch (win, 1);

  /* The other button: the colors go back, Pick Color stays, the releases change nothing. */
  use_tool (win, "pick");
  easel_canvas_press (canvas, 1, 10.5, 10.5);
  g_assert_cmphex (easel_window_get_foreground (win), ==, red);
  easel_canvas_press (canvas, 3, 10.5, 10.5);
  g_assert_cmphex (easel_window_get_foreground (win), ==, 0xFF000000u);
  g_assert_false (easel_canvas_is_stroking (canvas));
  g_assert_cmpint (easel_window_get_tool (win), ==, EASEL_TOOL_PICK);
  easel_canvas_release (canvas, 3);
  easel_canvas_release (canvas, 1);
  g_assert_cmphex (easel_window_get_foreground (win), ==, 0xFF000000u);
  g_assert_cmpint (easel_window_get_tool (win), ==, EASEL_TOOL_PICK);

  /* Undo: the colors go back; a second Undo before the release does nothing (the dot stays). */
  easel_canvas_press (canvas, 1, 10.5, 10.5);
  g_assert_cmphex (easel_window_get_foreground (win), ==, red);
  g_action_group_activate_action (G_ACTION_GROUP (win), "undo", NULL);
  g_assert_cmphex (easel_window_get_foreground (win), ==, 0xFF000000u);
  g_assert_cmpint (easel_window_get_tool (win), ==, EASEL_TOOL_PICK);
  g_action_group_activate_action (G_ACTION_GROUP (win), "undo", NULL);
  g_assert_cmphex (easel_image_get (easel_window_get_picture (win), 10, 10), ==, red);
  easel_canvas_release (canvas, 1);
  g_assert_cmphex (easel_window_get_foreground (win), ==, 0xFF000000u);
  g_assert_cmpint (easel_window_get_tool (win), ==, EASEL_TOOL_PICK);

  /* The window loses the focus (or the button is released outside it): the pick ends and keeps its color. */
  easel_canvas_press (canvas, 1, 10.5, 10.5);
  easel_canvas_reset_pointer (canvas);
  g_assert_cmphex (easel_window_get_foreground (win), ==, red);
  g_assert_cmpint (easel_window_get_tool (win), ==, EASEL_TOOL_PENCIL);
  easel_canvas_release (canvas, 1);
  g_assert_cmphex (easel_window_get_foreground (win), ==, red);
  close_windows ();
}

/* AS-16 (REQ-0007-16): a tool button pressed during a pick is ignored. */
static void
as16_tool_change_ignored_during_pick (void)
{
  EaselWindow *win = start_window ();
  EaselCanvas *canvas = easel_window_get_canvas (win);

  wait_for_layout (win);
  use_tool (win, "pick");
  easel_canvas_press (canvas, 1, 10.5, 10.5);
  use_tool (win, "brush");
  g_assert_cmpint (easel_window_get_tool (win), ==, EASEL_TOOL_PICK);
  g_signal_emit_by_name (find_tool_button (GTK_WIDGET (win), "eraser"), "clicked");
  g_assert_cmpint (easel_window_get_tool (win), ==, EASEL_TOOL_PICK);
  easel_canvas_release (canvas, 1);
  g_assert_cmpint (easel_window_get_tool (win), ==, EASEL_TOOL_PAN);
  use_tool (win, "brush");                                                      /* after the pick it works again */
  g_assert_cmpint (easel_window_get_tool (win), ==, EASEL_TOOL_BRUSH);
  close_windows ();
}

/* ---- 0007: the Magnifier's click ---- */

typedef struct
{
  GtkWidget *scroller;
  GtkAdjustment *h, *v;
} View;

static View
view_of (EaselWindow *win)
{
  View view;

  view.scroller = gtk_widget_get_ancestor (GTK_WIDGET (easel_window_get_canvas (win)), GTK_TYPE_SCROLLED_WINDOW);
  view.h = gtk_scrolled_window_get_hadjustment (GTK_SCROLLED_WINDOW (view.scroller));
  view.v = gtk_scrolled_window_get_vadjustment (GTK_SCROLLED_WINDOW (view.scroller));
  return view;
}

/* The canvas point (logical pixels) of the center of picture pixel (px, py) at the current zoom. */
static void
canvas_point (EaselWindow *win, double px, double py, double *x, double *y)
{
  double scale = easel_canvas_get_scale (easel_window_get_canvas (win));

  *x = px * easel_window_get_zoom (win) / 100.0 / scale;
  *y = py * easel_window_get_zoom (win) / 100.0 / scale;
}

/* The picture point at the center of the view now, in picture pixels. */
static void
view_center (EaselWindow *win, double *cx, double *cy)
{
  View view = view_of (win);
  double scale = easel_canvas_get_scale (easel_window_get_canvas (win)), zoom = easel_window_get_zoom (win);

  *cx = view_to_canvas (view.h, gtk_adjustment_get_page_size (view.h) / 2) * scale * 100.0 / zoom;
  *cy = view_to_canvas (view.v, gtk_adjustment_get_page_size (view.v) / 2) * scale * 100.0 / zoom;
}

/* A press and release of the Magnifier (or any tool) at picture pixel (px, py) at the current zoom. */
static void
click_pixel (EaselWindow *win, guint button, double px, double py)
{
  EaselCanvas *canvas = easel_window_get_canvas (win);
  double x, y;

  canvas_point (win, px + 0.5, py + 0.5, &x, &y);
  easel_canvas_press (canvas, button, x, y);
  easel_canvas_release (canvas, button);
  wait_for_layout (win);
}

/* AS-3, AS-4 (REQ-0007-03, REQ-0007-11): the zoom a press gives, the focus at the center of the view, and nothing edited. */
static void
as3_magnifier_click (void)
{
  EaselWindow *win;
  EaselCanvas *canvas;
  View view;
  graphene_rect_t r;
  double cx, cy;

  write_settings ("[new-picture]\nwidth=3000\nheight=2000\nbackground=#FFFF00\n");
  win = start_window ();
  canvas = easel_window_get_canvas (win);
  view = view_of (win);
  wait_for_layout (win);
  use_tool (win, "magnifier");

  /* 2x at (1000, 700): the zoom is 200 % and the box's center, which is the focus at 100 %, is at the center of the view. */
  {
    int bx, by, bw, bh;
    int area_w = (int) gtk_adjustment_get_page_size (view.h), area_h = (int) gtk_adjustment_get_page_size (view.v);

    easel_magnifier_box (area_w, area_h, 200, 3000, 2000, 1000, 700, &bx, &by, &bw, &bh);
    click_pixel (win, 1, 1000, 700);
    g_assert_cmpint (easel_window_get_zoom (win), ==, 200);
    view_center (win, &cx, &cy);
    g_assert_cmpfloat_with_epsilon (cx, bx + bw / 2.0, 1.0);
    g_assert_cmpfloat_with_epsilon (cy, by + bh / 2.0, 1.0);
    /* The picture is drawn where the scroll says (rendering, as the offset test does). */
    g_assert_true (gtk_widget_compute_bounds (GTK_WIDGET (canvas), view.scroller, &r));
    g_assert_cmpfloat_with_epsilon (r.origin.x, EASEL_VIEW_MARGIN - gtk_adjustment_get_value (view.h), 1.0);
    g_assert_cmpfloat_with_epsilon (r.origin.y, EASEL_VIEW_MARGIN - gtk_adjustment_get_value (view.v), 1.0);
  }
  g_assert_cmpint (easel_window_get_tool (win), ==, EASEL_TOOL_MAGNIFIER);        /* stays the Magnifier */
  g_assert_false (easel_window_get_modified (win));
  g_assert_false (action_enabled (win, "undo"));

  /* The same level again goes back to 100 %, anywhere on the picture. */
  click_pixel (win, 1, 1100, 750);
  g_assert_cmpint (easel_window_get_zoom (win), ==, 100);

  /* 4x: 100 % to 400 %, and back; from 200 % the selected level; the secondary button does the same. */
  choose_preset (win, "400 %");
  click_pixel (win, 3, 500, 500);
  g_assert_cmpint (easel_window_get_zoom (win), ==, 400);
  click_pixel (win, 1, 520, 510);
  g_assert_cmpint (easel_window_get_zoom (win), ==, 100);
  g_action_group_activate_action (G_ACTION_GROUP (win), "zoom-in", NULL);              /* 200 % from the menu */
  g_assert_cmpint (easel_window_get_zoom (win), ==, 200);
  click_pixel (win, 1, 500, 500);
  g_assert_cmpint (easel_window_get_zoom (win), ==, 400);
  g_action_group_activate_action (G_ACTION_GROUP (win), "zoom-fit", NULL);             /* the fit zoom, not a level */
  wait_for_layout (win);
  g_assert_cmpint (easel_window_get_zoom (win), !=, 400);
  click_pixel (win, 1, 500, 500);
  g_assert_cmpint (easel_window_get_zoom (win), ==, 400);

  /* 1x: back to 100 % from any zoom, and nothing at 100 %. */
  choose_preset (win, "100 %");
  click_pixel (win, 1, 500, 500);
  g_assert_cmpint (easel_window_get_zoom (win), ==, 100);
  click_pixel (win, 1, 500, 500);
  g_assert_cmpint (easel_window_get_zoom (win), ==, 100);

  /* Below 100 % (two Zoom Out steps give 25 %): 4x gives 400 %, and 1x gives 100 % (amended by REQ-0008-02: no
     reduction levels; there is no 1/10x or 1/2x to choose any more). */
  g_assert_null (find_by_tooltip (GTK_WIDGET (win), "10 %"));
  g_assert_null (find_by_tooltip (GTK_WIDGET (win), "50 %"));
  g_action_group_activate_action (G_ACTION_GROUP (win), "zoom-out", NULL);
  g_action_group_activate_action (G_ACTION_GROUP (win), "zoom-out", NULL);
  wait_for_layout (win);
  g_assert_cmpint (easel_window_get_zoom (win), ==, 25);
  choose_preset (win, "400 %");
  click_pixel (win, 1, 500, 500);
  g_assert_cmpint (easel_window_get_zoom (win), ==, 400);
  g_action_group_activate_action (G_ACTION_GROUP (win), "zoom-normal", NULL);
  g_action_group_activate_action (G_ACTION_GROUP (win), "zoom-out", NULL);
  g_action_group_activate_action (G_ACTION_GROUP (win), "zoom-out", NULL);
  wait_for_layout (win);
  choose_preset (win, "100 %");
  click_pixel (win, 3, 500, 500);
  g_assert_cmpint (easel_window_get_zoom (win), ==, 100);

  /* The middle button and a press outside the picture do nothing; a second button held does nothing more. */
  choose_preset (win, "200 %");
  click_pixel (win, 2, 500, 500);
  g_assert_cmpint (easel_window_get_zoom (win), ==, 100);
  easel_canvas_press (canvas, 1, -20.5, -20.5);
  easel_canvas_release (canvas, 1);
  g_assert_cmpint (easel_window_get_zoom (win), ==, 100);
  easel_canvas_press (canvas, 1, 500.5, 500.5);
  g_assert_cmpint (easel_window_get_zoom (win), ==, 200);
  easel_canvas_press (canvas, 3, 1001, 1001);
  g_assert_cmpint (easel_window_get_zoom (win), ==, 200);
  easel_canvas_release (canvas, 3);
  easel_canvas_release (canvas, 1);
  g_assert_cmpint (easel_window_get_zoom (win), ==, 200);
  g_assert_false (easel_window_get_modified (win));
  close_windows ();
  write_settings ("[new-picture]\nwidth=300\nheight=200\nbackground=#FFFF00\n");
}

/* AS-4 (REQ-0007-03): near the corners the edges limit the view; a picture that fits stays at its top-left. */
static void
as4_magnifier_edges (void)
{
  EaselWindow *win;
  View view;

  write_settings ("[new-picture]\nwidth=3000\nheight=2000\nbackground=#FFFF00\n");
  win = start_window ();
  view = view_of (win);
  wait_for_layout (win);
  use_tool (win, "magnifier");
  /* A press where the box just touches the picture's left and top edges (half a box from them) shows the corner exactly,
     although the page gets narrower when the scrollbar comes with the zoom. */
  click_pixel (win, 1, (int) gtk_adjustment_get_page_size (view.h) / 4, (int) gtk_adjustment_get_page_size (view.v) / 4);
  g_assert_cmpint (easel_window_get_zoom (win), ==, 200);
  g_assert_cmpfloat (gtk_adjustment_get_value (view.h), ==, 0);
  g_assert_cmpfloat (gtk_adjustment_get_value (view.v), ==, 0);
  click_pixel (win, 1, 100, 50);
  g_assert_cmpint (easel_window_get_zoom (win), ==, 100);
  click_pixel (win, 1, 100, 50);                                                   /* near the top-left corner */
  g_assert_cmpint (easel_window_get_zoom (win), ==, 200);
  g_assert_cmpfloat (gtk_adjustment_get_value (view.h), ==, 0);
  g_assert_cmpfloat (gtk_adjustment_get_value (view.v), ==, 0);
  click_pixel (win, 1, 100, 50);
  g_assert_cmpint (easel_window_get_zoom (win), ==, 100);
  click_pixel (win, 1, 2985, 1985);                                                /* near the bottom-right corner, off the resize handle */
  g_assert_cmpint (easel_window_get_zoom (win), ==, 200);
  /* The picture's right and bottom edges (6000 and 4000 logical pixels at 200 %) are at the view's edges. */
  g_assert_cmpfloat_with_epsilon (gtk_adjustment_get_value (view.h) + gtk_adjustment_get_page_size (view.h),
                                  6000 + 2 * EASEL_VIEW_MARGIN, 1.0);
  g_assert_cmpfloat_with_epsilon (gtk_adjustment_get_value (view.v) + gtk_adjustment_get_page_size (view.v),
                                  4000 + 2 * EASEL_VIEW_MARGIN, 1.0);
  close_windows ();

  /* A small picture: 200 % of 300 x 200 fits the view, so it stays at the top-left (REQ-0001-11). */
  write_settings ("[new-picture]\nwidth=300\nheight=200\nbackground=#FFFF00\n");
  win = start_window ();
  view = view_of (win);
  wait_for_layout (win);
  use_tool (win, "magnifier");
  click_pixel (win, 1, 150, 100);
  g_assert_cmpint (easel_window_get_zoom (win), ==, 200);
  g_assert_cmpfloat (gtk_adjustment_get_value (view.h), ==, 0);
  g_assert_cmpfloat (gtk_adjustment_get_value (view.v), ==, 0);
  close_windows ();
}

/* The pointer over picture pixel (px, py) at the current zoom, as the window's motion handler reports it. */
static void
point_at_pixel (EaselWindow *win, double px, double py)
{
  double x, y;

  canvas_point (win, px + 0.5, py + 0.5, &x, &y);
  easel_canvas_pointer_at (easel_window_get_canvas (win), x, y);
}

/* AS-5 (REQ-0007-04): the preview box follows the pointer at 100 % with a level that zooms in, and only then. */
static void
as5_box_follows_pointer (void)
{
  EaselWindow *win;
  EaselCanvas *canvas;
  View view;
  int x, y, w, h, ex, ey, ew, eh;

  write_settings ("[new-picture]\nwidth=3000\nheight=2000\nbackground=#FFFF00\n");
  win = start_window ();
  canvas = easel_window_get_canvas (win);
  view = view_of (win);
  wait_for_layout (win);

  /* Not the Magnifier: no box. */
  point_at_pixel (win, 1000, 700);
  g_assert_false (easel_canvas_get_magnifier_box (canvas, &x, &y, &w, &h));

  use_tool (win, "magnifier");
  g_assert_true (easel_canvas_get_magnifier_box (canvas, &x, &y, &w, &h));
  easel_magnifier_box ((int) gtk_adjustment_get_page_size (view.h), (int) gtk_adjustment_get_page_size (view.v), 200, 3000, 2000,
                       1000, 700, &ex, &ey, &ew, &eh);
  g_assert_cmpint (x, ==, ex);
  g_assert_cmpint (y, ==, ey);
  g_assert_cmpint (w, ==, ew);
  g_assert_cmpint (h, ==, eh);

  /* It follows the pointer and keeps inside the picture at a corner. */
  point_at_pixel (win, 0, 0);
  g_assert_true (easel_canvas_get_magnifier_box (canvas, &x, &y, &w, &h));
  g_assert_cmpint (x, ==, 0);
  g_assert_cmpint (y, ==, 0);

  /* 8x makes it smaller. */
  choose_preset (win, "800 %");
  point_at_pixel (win, 1000, 700);
  g_assert_true (easel_canvas_get_magnifier_box (canvas, &x, &y, &w, &h));
  g_assert_cmpint (w, <, ew);

  /* Outside the picture and outside the area: none. */
  easel_canvas_pointer_at (canvas, -50, -50);
  g_assert_false (easel_canvas_get_magnifier_box (canvas, &x, &y, &w, &h));
  point_at_pixel (win, 1000, 700);
  g_assert_true (easel_canvas_get_magnifier_box (canvas, &x, &y, &w, &h));
  easel_canvas_pointer_left (canvas);
  g_assert_false (easel_canvas_get_magnifier_box (canvas, &x, &y, &w, &h));

  /* 1x does not zoom in: none (there is no reduction level, REQ-0008-02). */
  choose_preset (win, "100 %");
  point_at_pixel (win, 1000, 700);
  g_assert_false (easel_canvas_get_magnifier_box (canvas, &x, &y, &w, &h));

  /* At another zoom none; back at 100 % with the pointer still there, it is shown at once. */
  choose_preset (win, "200 %");
  g_assert_true (easel_canvas_get_magnifier_box (canvas, &x, &y, &w, &h));
  click_pixel (win, 1, 1000, 700);
  g_assert_cmpint (easel_window_get_zoom (win), ==, 200);
  point_at_pixel (win, 1000, 700);
  g_assert_false (easel_canvas_get_magnifier_box (canvas, &x, &y, &w, &h));
  g_action_group_activate_action (G_ACTION_GROUP (win), "zoom-normal", NULL);
  wait_for_layout (win);
  g_assert_true (easel_canvas_get_magnifier_box (canvas, &x, &y, &w, &h));

  /* Another tool: gone. */
  use_tool (win, "pencil");
  g_assert_false (easel_canvas_get_magnifier_box (canvas, &x, &y, &w, &h));
  close_windows ();

  /* A picture smaller than the box: the whole picture. */
  write_settings ("[new-picture]\nwidth=500\nheight=300\nbackground=#FFFF00\n");
  win = start_window ();
  canvas = easel_window_get_canvas (win);
  wait_for_layout (win);
  use_tool (win, "magnifier");
  point_at_pixel (win, 250, 150);
  g_assert_true (easel_canvas_get_magnifier_box (canvas, &x, &y, &w, &h));
  g_assert_cmpint (x, ==, 0);
  g_assert_cmpint (y, ==, 0);
  g_assert_cmpint (w, ==, 500);
  g_assert_cmpint (h, ==, 300);
  close_windows ();
}

static const char *const new_tools[] = { "fill", "pick", "magnifier" };

/* AS-14 (REQ-0007-14): the keypad + and - do nothing with Fill, Pick Color and the Magnifier; Ctrl+keypad still zooms. */
static void
as14_numpad_ignored (void)
{
  EaselWindow *win = start_window ();

  wait_for_layout (win);
  for (guint i = 0; i < G_N_ELEMENTS (new_tools); i++)
    {
      int level = magnifier_level (win);

      use_tool (win, new_tools[i]);
      g_action_group_activate_action (G_ACTION_GROUP (win), "preset-larger", NULL);
      g_action_group_activate_action (G_ACTION_GROUP (win), "preset-smaller", NULL);
      g_assert_cmpint (magnifier_level (win), ==, level);
      g_assert_cmpint (easel_window_get_zoom (win), ==, 100);
      g_assert_false (easel_window_get_modified (win));
    }
  g_assert_true (has_accel ("win.zoom-in", GDK_KEY_KP_Add, GDK_CONTROL_MASK));
  g_action_group_activate_action (G_ACTION_GROUP (win), "zoom-in", NULL);
  g_assert_cmpint (easel_window_get_zoom (win), >, 100);
  close_windows ();
}

/* One press and release with each of the three tools; true if anything changed (the picture, the colors, the zoom). */
static gboolean
three_tools_change_something (EaselWindow *win, double x, double y)
{
  EaselCanvas *canvas = easel_window_get_canvas (win);
  uint32_t fg = easel_window_get_foreground (win), bg = easel_window_get_background (win);
  int zoom = easel_window_get_zoom (win);
  gboolean changed = FALSE;

  for (guint i = 0; i < G_N_ELEMENTS (new_tools); i++)
    {
      use_tool (win, new_tools[i]);
      easel_canvas_press (canvas, 1, x, y);
      easel_canvas_release (canvas, 1);
      easel_canvas_press (canvas, 3, x, y);
      easel_canvas_release (canvas, 3);
      changed |= easel_window_get_modified (win) || easel_window_get_foreground (win) != fg
                 || easel_window_get_background (win) != bg || easel_window_get_zoom (win) != zoom
                 || easel_canvas_is_stroking (canvas);
    }
  return changed;
}

/* AS-15 (REQ-0007-15): before the picture exists, with the color chooser open and during a resize drag, a press with Fill,
   Pick Color or the Magnifier does nothing. */
static void
as15_new_tools_ignored_when_blocked (void)
{
  EaselWindow *win;
  EaselCanvas *canvas;
  EaselPaletteBar *bar;
  double x, y;

  /* Before the picture exists. */
  write_settings ("[new-picture]\nwidth=auto\nheight=auto\n");
  win = easel_window_new (app);
  g_assert_null (easel_window_get_picture (win));
  g_assert_false (three_tools_change_something (win, 3.5, 3.5));
  gtk_window_destroy (GTK_WINDOW (win));
  write_settings ("[new-picture]\nwidth=300\nheight=200\nbackground=#FFFF00\n");

  /* The color chooser open. */
  win = start_window ();
  bar = easel_window_get_palette_bar (win);
  wait_for_layout (win);
  easel_palette_bar_press (bar, 17, 1, 1);
  easel_palette_bar_press (bar, 17, 1, 2);
  g_assert_true (easel_window_is_choosing_color (win));
  g_assert_false (three_tools_change_something (win, 50.5, 50.5));
  choose (win, NULL);
  close_windows ();

  /* A resize drag in progress (the other button would cancel it, so only the dragging button presses again). */
  win = start_window ();
  canvas = easel_window_get_canvas (win);
  for (guint i = 0; i < G_N_ELEMENTS (new_tools); i++)
    {
      uint32_t fg = easel_window_get_foreground (win);

      use_tool (win, new_tools[i]);
      wait_for_layout (win);
      handle_center (canvas, EASEL_HANDLE_CORNER, &x, &y);
      easel_canvas_press (canvas, 1, x, y);
      g_assert_true (easel_canvas_is_resizing (canvas));
      easel_canvas_press (canvas, 1, 50.5, 50.5);
      easel_canvas_release (canvas, 1);
      g_assert_false (easel_window_get_modified (win));
      g_assert_cmphex (easel_window_get_foreground (win), ==, fg);
      g_assert_cmpint (easel_window_get_zoom (win), ==, 100);
      g_assert_false (easel_canvas_is_resizing (canvas));
    }
  close_windows ();
}

/* The next frame the window paints, so that render_pixels shows the state after what was just done (the paintable replays
   the last frame: a scroll or a zoom that only queued its redraw is not in it yet). */
static void
wait_for_next_frame (EaselWindow *win)
{
  GdkFrameClock *clock = gtk_widget_get_frame_clock (GTK_WIDGET (win));
  gint64 before = gdk_frame_clock_get_frame_counter (clock);
  gint64 deadline = g_get_monotonic_time () + 3 * G_USEC_PER_SEC;

  gtk_widget_queue_draw (GTK_WIDGET (easel_window_get_canvas (win)));
  while (gdk_frame_clock_get_frame_counter (clock) <= before + 1 && g_get_monotonic_time () < deadline)
    g_main_context_iteration (NULL, FALSE);
}

/* The pixel a grid line gives over a base color: #808080 at 50 % (REQ-0004-12), as the window shows it (opaque 0xFFRRGGBB). */
static uint32_t
grid_line_over (uint32_t base)
{
  uint32_t c = 0xFF000000u;

  for (int shift = 0; shift <= 16; shift += 8)
    c |= (uint32_t) ((128 + ((base >> shift) & 0xFF) + 1) / 2) << shift;
  return c;
}

/* How many rendered pixels are within 2 of the color in every channel. */
static int
count_near (const uint32_t *px, int n, uint32_t color)
{
  int found = 0;

  for (int i = 0; i < n; i++)
    {
      int d = 0;

      for (int shift = 0; shift <= 16; shift += 8)
        d = MAX (d, abs ((int) ((px[i] >> shift) & 0xFF) - (int) ((color >> shift) & 0xFF)));
      found += d <= 2;
    }
  return found;
}

/* True if the pixel (x, y) of the rendered window is within 2 of the color in every channel. */
static gboolean
pixel_is (const uint32_t *px, int w, int x, int y, uint32_t color)
{
  return count_near (&px[(gsize) y * w + x], 1, color) == 1;
}

static void assert_grid_lines_near (const uint32_t *px, int w, int cx, int cy, int zoom, int cell, uint32_t base, int wx,
                                    int wy);

/* The lines nearest the window point (500, 300). */
static void
assert_grid_lines_at_scroll (const uint32_t *px, int w, int cx, int cy, int zoom, int cell, uint32_t base)
{
  assert_grid_lines_near (px, w, cx, cy, zoom, cell, base, 500, 300);
}

/* The grid lines at the place the view's own scroll puts them: the canvas starts at (cx, cy) in the rendered window
   (its offset is minus the scroll), so the line of picture column k is at x = cx + k * cell * zoom / 100 and the line of
   picture row k at y = cy + k * cell * zoom / 100 (a whole device pixel each, REQ-0004-12), and everything between two
   lines is the picture. Looks at the lines nearest the window point (wx, wy). */
static void
assert_grid_lines_near (const uint32_t *px, int w, int cx, int cy, int zoom, int cell, uint32_t base, int wx, int wy)
{
  int step = cell * zoom / 100;
  int col = (wx - cx) / step, row = (wy - cy) / step;
  int x = cx + col * step, y = cy + row * step;
  uint32_t line = grid_line_over (base);

  g_assert_cmpint (step, >=, 4);
  /* On a row between two horizontal lines: a line at x and at x + step, the picture between. */
  g_assert_true (pixel_is (px, w, x, y + step / 2, line));
  g_assert_true (pixel_is (px, w, x + step, y + step / 2, line));
  for (int i = 1; i < step; i++)
    g_assert_true (pixel_is (px, w, x + i, y + step / 2, base));
  /* On a column between two vertical lines: a line at y and at y + step, the picture between. */
  g_assert_true (pixel_is (px, w, x + step / 2, y, line));
  g_assert_true (pixel_is (px, w, x + step / 2, y + step, line));
  for (int i = 1; i < step; i++)
    g_assert_true (pixel_is (px, w, x + step / 2, y + i, base));
}

/* REQ-0004-12, from the screencast of 2026-10-18: at 4x, 6x and 8x, reached with the Magnifier, the grid is on and its
   lines are drawn (one every picture pixel), on a white picture and on an orange one, where a grey line at 50 % has little
   contrast, and on a picture bigger than the view. */
static void
as10_grid_at_magnifier_levels (void)
{
  static const struct { const char *settings; int w, h; uint32_t base; } pictures[] = {
    { "[new-picture]\nwidth=3000\nheight=2000\nbackground=#FFFFFF\n", 3000, 2000, 0xFFFFFFFFu },
    { "[new-picture]\nwidth=3000\nheight=2000\nbackground=#FB9411\n", 3000, 2000, 0xFFFB9411u },
    { "[new-picture]\nwidth=8000\nheight=6000\nbackground=#FB9411\n", 8000, 6000, 0xFFFB9411u },
  };
  static const char *const levels[] = { "400 %", "600 %", "800 %" };

  for (guint k = 0; k < G_N_ELEMENTS (pictures); k++)
    {
      EaselWindow *win;
      EaselCanvas *canvas;

      write_settings (pictures[k].settings);
      win = start_window ();
      canvas = easel_window_get_canvas (win);
      wait_for_layout (win);
      g_action_group_activate_action (G_ACTION_GROUP (win), "grid", NULL);
      g_assert_true (panel_checked (win, "grid"));
      g_assert_true (easel_canvas_get_grid_on (canvas));
      use_tool (win, "magnifier");
      for (guint i = 0; i < G_N_ELEMENTS (levels); i++)
        {
          int w, h, cx, cy, zoom = atoi (levels[i]);
          g_autofree uint32_t *px = NULL;

          choose_preset (win, levels[i]);
          click_pixel (win, 1, pictures[k].w / 2, pictures[k].h / 2);
          g_assert_cmpint (easel_window_get_zoom (win), ==, zoom);
          g_assert_true (easel_canvas_grid_is_drawn (canvas));
          wait_for_next_frame (win);
          px = render_pixels (win, &w, &h, &cx, &cy);
          /* The view is scrolled by more than one page in each direction (the zoom keeps the center). */
          {
            GtkWidget *scr = gtk_widget_get_ancestor (GTK_WIDGET (canvas), GTK_TYPE_SCROLLED_WINDOW);
            GtkAdjustment *ha = gtk_scrolled_window_get_hadjustment (GTK_SCROLLED_WINDOW (scr));
            GtkAdjustment *va = gtk_scrolled_window_get_vadjustment (GTK_SCROLLED_WINDOW (scr));

            g_assert_cmpfloat (gtk_adjustment_get_value (ha), >, gtk_adjustment_get_page_size (ha));
            g_assert_cmpfloat (gtk_adjustment_get_value (va), >, gtk_adjustment_get_page_size (va));
          }
          /* Where the lines are, from the scroll and the cell size. */
          assert_grid_lines_at_scroll (px, w, cx, cy, zoom, easel_canvas_get_grid_cell (canvas), pictures[k].base);
          /* A line every picture pixel: over a view about 1350 x 775 that is thousands of pixels. */
          g_test_message ("picture %d x %d, %s: %d line pixels", pictures[k].w, pictures[k].h, levels[i],
                          count_near (px, w * h, grid_line_over (pictures[k].base)));
          g_assert_cmpint (count_near (px, w * h, grid_line_over (pictures[k].base)), >, 20000);
          click_pixel (win, 1, pictures[k].w / 2, pictures[k].h / 2);       /* the same level again: back to 100 % */
          g_assert_cmpint (easel_window_get_zoom (win), ==, 100);
        }
      close_windows ();
    }
  write_settings ("[new-picture]\nwidth=300\nheight=200\nbackground=#FFFF00\n");
}

/* AS-17 (REQ-0007-17): after the zoom the Pencil and Fill act on the pixel under the pointer at the new zoom. */
static void
as17_pencil_after_magnifier (void)
{
  EaselWindow *win;
  EaselCanvas *canvas;
  const EaselImage *img;
  View view;
  double cx, cy;

  write_settings ("[new-picture]\nwidth=3000\nheight=2000\nbackground=#FFFF00\n");
  win = start_window ();
  canvas = easel_window_get_canvas (win);
  img = easel_window_get_picture (win);
  view = view_of (win);
  wait_for_layout (win);
  use_tool (win, "magnifier");
  choose_preset (win, "400 %");
  click_pixel (win, 1, 1000, 700);
  g_assert_cmpint (easel_window_get_zoom (win), ==, 400);

  /* The center of the view, as the pointer there would give it. */
  view_center (win, &cx, &cy);
  use_tool (win, "pencil");
  easel_canvas_press (canvas, 1, view_to_canvas (view.h, gtk_adjustment_get_page_size (view.h) / 2),
                      view_to_canvas (view.v, gtk_adjustment_get_page_size (view.v) / 2));
  easel_canvas_release (canvas, 1);
  g_assert_cmpint (count_color (img, 0xFF000000u), ==, 1);
  g_assert_cmphex (easel_image_get (img, (int) cx, (int) cy), ==, 0xFF000000u);        /* the pixel at the center */
  g_assert_cmpfloat_with_epsilon (cx, 1000, 175.0);                                    /* and near the one that was clicked */

  /* Fill at the same point fills the area of that pixel: the black pixel alone, with the background color. */
  use_tool (win, "fill");
  easel_canvas_press (canvas, 3, view_to_canvas (view.h, gtk_adjustment_get_page_size (view.h) / 2),
                      view_to_canvas (view.v, gtk_adjustment_get_page_size (view.v) / 2));
  easel_canvas_release (canvas, 3);
  g_assert_cmpint (count_color (img, 0xFF000000u), ==, 0);
  close_windows ();
  write_settings ("[new-picture]\nwidth=300\nheight=200\nbackground=#FFFF00\n");
}

/* AS-18 (REQ-0007-18): a release the window never saw: the next sign that no button is down ends the held press. */
static void
as18_lost_release_ends_the_press (void)
{
  EaselWindow *win = start_window ();
  EaselCanvas *canvas = easel_window_get_canvas (win);
  const EaselImage *img = easel_window_get_picture (win);
  uint32_t red;

  wait_for_layout (win);

  /* Fill: the press fills; without the release a second press does nothing, after the move it fills again. */
  use_tool (win, "fill");
  easel_canvas_press (canvas, 1, 100.5, 80.5);
  g_assert_cmpint (count_color (img, 0xFF000000u), ==, 300 * 200);
  easel_canvas_press (canvas, 3, 100.5, 80.5);
  g_assert_cmpint (count_color (img, 0xFF000000u), ==, 300 * 200);               /* still held in the app */
  easel_canvas_buttons_released (canvas);
  easel_canvas_press (canvas, 3, 100.5, 80.5);
  g_assert_cmpint (count_color (img, 0xFFFFFF00u), ==, 300 * 200);
  easel_canvas_release (canvas, 3);

  /* A stroke: the move ends it, what it drew is kept, and the options panel is usable. */
  use_tool (win, "brush");
  easel_canvas_press (canvas, 1, 20.5, 20.5);
  g_assert_false (gtk_widget_is_sensitive (GTK_WIDGET (easel_window_get_tool_options (win))));
  easel_canvas_buttons_released (canvas);
  g_assert_true (gtk_widget_is_sensitive (GTK_WIDGET (easel_window_get_tool_options (win))));
  g_assert_false (easel_canvas_is_stroking (canvas));
  g_assert_cmphex (easel_image_get (img, 20, 20), ==, 0xFF000000u);
  g_action_group_activate_action (G_ACTION_GROUP (win), "undo", NULL);             /* the stroke is one step */
  g_assert_cmphex (easel_image_get (img, 20, 20), ==, 0xFFFFFF00u);

  /* A pick: ends and keeps its color, the previous tool comes back. */
  red = choose_swatch (win, 3);
  dot (win, 10, 10);
  choose_swatch (win, 1);
  use_tool (win, "pick");
  easel_canvas_press (canvas, 1, 10.5, 10.5);
  g_assert_cmphex (easel_window_get_foreground (win), ==, red);
  easel_canvas_buttons_released (canvas);
  g_assert_false (easel_canvas_is_stroking (canvas));
  g_assert_cmpint (easel_window_get_tool (win), ==, EASEL_TOOL_PENCIL);
  g_assert_cmphex (easel_window_get_foreground (win), ==, red);
  close_windows ();
}

/* ---- 0006: Numpad + and - ---- */

/* AS-10 (REQ-0006-14, REQ-0006-15): the keypad keys step the presets of the active tool. */
static void
as10_numpad_steps (void)
{
  EaselWindow *win = start_window ();
  EaselCanvas *canvas = easel_window_get_canvas (win);

  /* The keys, bare: the keypad ones, not the main keyboard's + and -; Ctrl+keypad still zooms. */
  g_assert_true (has_accel ("win.preset-larger", GDK_KEY_KP_Add, 0));
  g_assert_true (has_accel ("win.preset-smaller", GDK_KEY_KP_Subtract, 0));
  g_assert_false (has_accel ("win.preset-larger", GDK_KEY_plus, 0));
  g_assert_false (has_accel ("win.preset-larger", GDK_KEY_equal, 0));
  g_assert_false (has_accel ("win.preset-smaller", GDK_KEY_minus, 0));
  g_assert_true (has_accel ("win.zoom-in", GDK_KEY_KP_Add, GDK_CONTROL_MASK));
  g_assert_true (has_accel ("win.zoom-out", GDK_KEY_KP_Subtract, GDK_CONTROL_MASK));

  wait_for_layout (win);
  /* The Brush: 4 -> 7, the end stays, 7 -> 4 -> 1, the end stays. */
  use_tool (win, "brush");
  wait_for_layout (win);
  g_assert_cmpint (check_presets (win, brush_tips, 12, 2), ==, 1);
  g_action_group_activate_action (G_ACTION_GROUP (win), "preset-larger", NULL);
  g_assert_cmpint (check_presets (win, brush_tips, 12, 2), ==, 0);
  g_action_group_activate_action (G_ACTION_GROUP (win), "preset-larger", NULL);
  g_assert_cmpint (check_presets (win, brush_tips, 12, 2), ==, 0);
  g_action_group_activate_action (G_ACTION_GROUP (win), "preset-smaller", NULL);
  g_action_group_activate_action (G_ACTION_GROUP (win), "preset-smaller", NULL);
  g_assert_cmpint (check_presets (win, brush_tips, 12, 2), ==, 2);
  g_action_group_activate_action (G_ACTION_GROUP (win), "preset-smaller", NULL);
  g_assert_cmpint (check_presets (win, brush_tips, 12, 2), ==, 2);
  /* Within the shape: the slashes go 8, 5, 2. */
  choose_preset (win, "Slash, 5 pixels");
  g_action_group_activate_action (G_ACTION_GROUP (win), "preset-larger", NULL);
  g_assert_cmpint (check_presets (win, brush_tips, 12, 2), ==, 6);

  /* The Airbrush: 16 -> 24, the end stays, then 16 -> 9. */
  use_tool (win, "airbrush");
  wait_for_layout (win);
  g_action_group_activate_action (G_ACTION_GROUP (win), "preset-larger", NULL);
  g_assert_cmpint (check_presets (win, airbrush_tips, 3, 2), ==, 2);
  g_action_group_activate_action (G_ACTION_GROUP (win), "preset-larger", NULL);
  g_assert_cmpint (check_presets (win, airbrush_tips, 3, 2), ==, 2);
  g_action_group_activate_action (G_ACTION_GROUP (win), "preset-smaller", NULL);
  g_action_group_activate_action (G_ACTION_GROUP (win), "preset-smaller", NULL);
  g_assert_cmpint (check_presets (win, airbrush_tips, 3, 2), ==, 0);

  /* The Eraser keeps its mode: 8 Transparent -> 10 Transparent. */
  use_tool (win, "eraser");
  wait_for_layout (win);
  choose_preset (win, eraser_mode_tips[1]);
  g_action_group_activate_action (G_ACTION_GROUP (win), "preset-larger", NULL);
  g_assert_cmpint (check_presets (win, eraser_tips, 4, 2), ==, 3);
  g_assert_cmpint (check_presets (win, eraser_mode_tips, 2, 1), ==, 1);
  g_assert_true (easel_tool_options_get_transparent (easel_window_get_tool_options (win)));

  /* Not an edit (REQ-0006-15). */
  g_assert_false (easel_window_get_modified (win));
  g_assert_false (action_enabled (win, "undo"));

  /* During a stroke nothing happens. */
  easel_canvas_press (canvas, 1, 10.5, 10.5);
  g_action_group_activate_action (G_ACTION_GROUP (win), "preset-smaller", NULL);
  easel_canvas_release (canvas, 1);
  g_assert_cmpint (check_presets (win, eraser_tips, 4, 2), ==, 3);
  g_action_group_activate_action (G_ACTION_GROUP (win), "preset-smaller", NULL);
  g_assert_cmpint (check_presets (win, eraser_tips, 4, 2), ==, 2);

  /* Pencil and Pan do nothing, and the presets of the others are kept. */
  use_tool (win, "pencil");
  g_action_group_activate_action (G_ACTION_GROUP (win), "preset-larger", NULL);
  use_tool (win, "pan");
  g_action_group_activate_action (G_ACTION_GROUP (win), "preset-larger", NULL);
  use_tool (win, "eraser");
  wait_for_layout (win);
  g_assert_cmpint (check_presets (win, eraser_tips, 4, 2), ==, 2);

  /* The color chooser open: nothing happens. */
  easel_palette_bar_press_indicator (easel_window_get_palette_bar (win), TRUE);
  g_assert_true (easel_window_is_choosing_color (win));
  g_action_group_activate_action (G_ACTION_GROUP (win), "preset-larger", NULL);
  g_assert_cmpint (check_presets (win, eraser_tips, 4, 2), ==, 2);
  easel_window_finish_color_choice (win, NULL);
  close_windows ();
}

/* The pid of another Easel or test-window process (not this one), or 0. A second window on the display changes the layout
   of the test windows, so the ui tests fail with it open for reasons that are not in the code (docs/engineering.md, Testing). */
static int
other_easel_process (void)
{
  g_autoptr (GDir) proc = g_dir_open ("/proc", 0, NULL);
  const char *entry;

  while (proc != NULL && (entry = g_dir_read_name (proc)) != NULL)
    {
      g_autofree char *path = g_strdup_printf ("/proc/%s/comm", entry);
      g_autofree char *comm = NULL;
      int pid = atoi (entry);

      if (pid > 0 && pid != getpid () && g_file_get_contents (path, &comm, NULL, NULL)
          && (g_str_has_prefix (comm, "easel\n") || g_str_has_prefix (comm, "test-window\n")))
        return pid;
    }
  return 0;
}

/* ---- 0008: the zoom presets ---- */

/* The View menu's "Zoom" submenu: its model, found as the item labelled "Zoom" in any section of the View menu. */
static GMenuModel *
zoom_submenu (EaselWindow *win, int *section_index, int *item_index)
{
  GMenuModel *menu = gtk_menu_button_get_menu_model (easel_window_get_view_button (win));

  for (int s = 0; s < g_menu_model_get_n_items (menu); s++)
    {
      GMenuModel *section = g_menu_model_get_item_link (menu, s, G_MENU_LINK_SECTION);

      for (int i = 0; section != NULL && i < g_menu_model_get_n_items (section); i++)
        {
          g_autofree char *label = NULL;

          if (g_menu_model_get_item_attribute (section, i, G_MENU_ATTRIBUTE_LABEL, "s", &label)
              && g_str_equal (label, "Zoom"))
            {
              GMenuModel *sub = g_menu_model_get_item_link (section, i, G_MENU_LINK_SUBMENU);

              if (section_index != NULL)
                *section_index = s;
              if (item_index != NULL)
                *item_index = i;
              g_object_unref (section);
              return sub;
            }
        }
      g_clear_object (&section);
    }
  return NULL;
}

/* AS-1 (REQ-0008-01, REQ-0008-02, REQ-0008-03): View > Zoom holds the five presets in order, labelled "100 %" to
   "800 %", each choosing win.zoom-to with its percent; it is the first item of the section that holds Normal Size; the
   Magnifier's options show the same eight levels. */
static void
as1_zoom_submenu_and_lens_levels (void)
{
  static const int presets[] = { 100, 200, 400, 600, 800 };
  EaselWindow *win = start_window ();
  GMenuModel *menu = gtk_menu_button_get_menu_model (easel_window_get_view_button (win));
  g_autoptr (GMenuModel) sub = NULL;
  int s = -1, i = -1;

  wait_for_layout (win);
  sub = zoom_submenu (win, &s, &i);
  g_assert_nonnull (sub);
  g_assert_cmpint (g_menu_model_get_n_items (sub), ==, 5);
  for (int k = 0; k < 5; k++)
    {
      g_autofree char *label = NULL, *action = NULL, *expected = g_strdup_printf ("%d %%", presets[k]);
      g_autoptr (GVariant) target = NULL;

      g_assert_true (g_menu_model_get_item_attribute (sub, k, G_MENU_ATTRIBUTE_LABEL, "s", &label));
      g_assert_cmpstr (label, ==, expected);
      g_assert_true (g_menu_model_get_item_attribute (sub, k, G_MENU_ATTRIBUTE_ACTION, "s", &action));
      g_assert_cmpstr (action, ==, "win.zoom-to");
      target = g_menu_model_get_item_attribute_value (sub, k, G_MENU_ATTRIBUTE_TARGET, G_VARIANT_TYPE_INT32);
      g_assert_nonnull (target);
      g_assert_cmpint (g_variant_get_int32 (target), ==, presets[k]);
    }
  /* Placed first in the section of Normal Size, Large Size and Fit to Window, which stay. */
  {
    g_autoptr (GMenuModel) section = g_menu_model_get_item_link (menu, s, G_MENU_LINK_SECTION);
    g_autofree char *next = NULL;

    g_assert_cmpint (i, ==, 0);
    g_assert_true (g_menu_model_get_item_attribute (section, 1, G_MENU_ATTRIBUTE_ACTION, "s", &next));
    g_assert_cmpstr (next, ==, "win.zoom-normal");
    g_assert_cmpint (g_menu_model_get_n_items (section), ==, 4);
  }
  /* The Magnifier's options: the same five levels, in the same order, labelled 1x to 8x (REQ-0008-02); no others. */
  {
    static const char *const labels[] = { "1x", "2x", "4x", "6x", "8x" };

    use_tool (win, "magnifier");
    wait_for_layout (win);
    g_assert_null (find_by_tooltip (GTK_WIDGET (win), "10 %"));
    g_assert_null (find_by_tooltip (GTK_WIDGET (win), "25 %"));
    g_assert_null (find_by_tooltip (GTK_WIDGET (win), "50 %"));
    for (int k = 0; k < 5; k++)
      {
        g_autofree char *tip = g_strdup_printf ("%d %%", presets[k]);
        GtkWidget *b = find_by_tooltip (GTK_WIDGET (win), tip);

        g_assert_nonnull (b);
        g_assert_cmpstr (gtk_button_get_label (GTK_BUTTON (b)), ==, labels[k]);
        g_assert_true (on_screen (win, tip));
      }
  }
  close_windows ();
}

/* AS-2 (REQ-0008-04): a preset from the menu sets the zoom, keeps the center of the canvas area still, leaves the
   Magnifier's selected level alone, and edits nothing. */
static void
as2_zoom_to_preset (void)
{
  EaselWindow *win;
  double cx0, cy0, cx, cy;

  write_settings ("[new-picture]\nwidth=3000\nheight=2000\nbackground=#FFFF00\n");
  win = start_window ();
  wait_for_layout (win);
  use_pencil (win);
  view_center (win, &cx0, &cy0);
  g_action_group_activate_action (G_ACTION_GROUP (win), "zoom-to", g_variant_new_int32 (800));
  wait_for_layout (win);
  g_assert_cmpint (easel_window_get_zoom (win), ==, 800);
  view_center (win, &cx, &cy);
  g_assert_cmpfloat_with_epsilon (cx, cx0, 1.0);
  g_assert_cmpfloat_with_epsilon (cy, cy0, 1.0);
  g_action_group_activate_action (G_ACTION_GROUP (win), "zoom-to", g_variant_new_int32 (100));
  wait_for_layout (win);
  g_assert_cmpint (easel_window_get_zoom (win), ==, 100);
  g_assert_cmpint (magnifier_level (win), ==, 200);
  g_assert_cmpint (easel_window_get_tool (win), ==, EASEL_TOOL_PENCIL);
  g_assert_false (easel_window_get_modified (win));
  g_assert_false (action_enabled (win, "undo"));
  close_windows ();
  write_settings ("[new-picture]\nwidth=300\nheight=200\nbackground=#FFFF00\n");
}

/* The preset the Zoom submenu marks: the state of win.zoom-to, 0 when no item is checked. */
static int
zoom_checked (EaselWindow *win)
{
  g_autoptr (GVariant) state = g_action_group_get_action_state (G_ACTION_GROUP (win), "zoom-to");

  return g_variant_get_int32 (state);
}

/* AS-3 (REQ-0008-05): the check mark follows every zoom change, and no item is checked at a zoom that is not a preset. */
static void
as3_zoom_check_follows_zoom (void)
{
  EaselWindow *win;
  EaselCanvas *canvas;

  write_settings ("[new-picture]\nwidth=3000\nheight=2000\nbackground=#FFFF00\n");
  win = start_window ();
  canvas = easel_window_get_canvas (win);
  wait_for_layout (win);
  g_assert_cmpint (zoom_checked (win), ==, 100);
  g_action_group_activate_action (G_ACTION_GROUP (win), "zoom-large", NULL);           /* Ctrl+PageDown */
  g_assert_cmpint (zoom_checked (win), ==, 400);
  g_action_group_activate_action (G_ACTION_GROUP (win), "zoom-in", NULL);
  g_assert_cmpint (zoom_checked (win), ==, 600);
  g_action_group_activate_action (G_ACTION_GROUP (win), "zoom-large", NULL);
  g_action_group_activate_action (G_ACTION_GROUP (win), "zoom-out", NULL);             /* 300 %: a step, not a preset */
  g_assert_cmpint (easel_window_get_zoom (win), ==, 300);
  g_assert_cmpint (zoom_checked (win), ==, 0);
  /* 50 % and 25 % are steps, not presets (REQ-0008-01). */
  g_action_group_activate_action (G_ACTION_GROUP (win), "zoom-normal", NULL);
  g_action_group_activate_action (G_ACTION_GROUP (win), "zoom-out", NULL);
  g_assert_cmpint (easel_window_get_zoom (win), ==, 50);
  g_assert_cmpint (zoom_checked (win), ==, 0);
  g_action_group_activate_action (G_ACTION_GROUP (win), "zoom-out", NULL);
  g_assert_cmpint (easel_window_get_zoom (win), ==, 25);
  g_assert_cmpint (zoom_checked (win), ==, 0);
  g_action_group_activate_action (G_ACTION_GROUP (win), "zoom-fit", NULL);
  wait_for_layout (win);
  g_assert_cmpint (easel_view_preset_index (easel_window_get_zoom (win)), ==, -1);    /* 42 % or so */
  g_assert_cmpint (zoom_checked (win), ==, 0);
  /* The Magnifier with 4x chosen. */
  use_tool (win, "magnifier");
  choose_preset (win, "400 %");
  easel_canvas_press (canvas, 1, 100.5, 100.5);
  easel_canvas_release (canvas, 1);
  wait_for_layout (win);
  g_assert_cmpint (easel_window_get_zoom (win), ==, 400);
  g_assert_cmpint (zoom_checked (win), ==, 400);
  /* The menu's own choice. */
  g_action_group_activate_action (G_ACTION_GROUP (win), "zoom-to", g_variant_new_int32 (200));
  g_assert_cmpint (zoom_checked (win), ==, 200);
  g_action_group_activate_action (G_ACTION_GROUP (win), "zoom-to", g_variant_new_int32 (25));      /* not a preset: ignored */
  g_assert_cmpint (easel_window_get_zoom (win), ==, 200);
  close_windows ();
  write_settings ("[new-picture]\nwidth=300\nheight=200\nbackground=#FFFF00\n");
}

/* The toolbox panel: the box that holds the tool grid and the options, found from the Pan button. */
static GtkWidget *
toolbox_panel (EaselWindow *win)
{
  GtkWidget *grid = gtk_widget_get_parent (find_by_tooltip (GTK_WIDGET (win), "Pan"));

  return gtk_widget_get_parent (gtk_widget_get_parent (grid));
}

/* Every button of the shown options lies inside the toolbox, and a text button's label gets at least its natural width
   (no label is cut). */
static void
assert_options_fit (EaselWindow *win, const char *const *tips, int n)
{
  GtkWidget *panel = toolbox_panel (win);

  for (int i = 0; i < n; i++)
    {
      GtkWidget *b = find_by_tooltip (GTK_WIDGET (win), tips[i]);
      graphene_rect_t r = bounds_in (b, panel);
      GtkWidget *child = gtk_button_get_child (GTK_BUTTON (b));

      g_assert_true (gtk_widget_get_mapped (b));
      g_assert_cmpfloat (r.origin.x, >=, 0);
      g_assert_cmpfloat (r.origin.x + r.size.width, <=, gtk_widget_get_width (panel));
      if (GTK_IS_LABEL (child))
        {
          int min, nat;

          gtk_widget_measure (child, GTK_ORIENTATION_HORIZONTAL, -1, &min, &nat, NULL, NULL);
          g_assert_cmpint (gtk_widget_get_width (child), >=, nat);
        }
    }
}

/* The Magnifier's level buttons are laid out as the tool buttons are (REQ-0008-07): 1x, 4x and 8x under Pan, 2x and 6x
   under the Magnifier tool button, as wide as a tool button, their labels at the window's own text size. */
static void
assert_lens_like_tool_buttons (EaselWindow *win)
{
  static const char *const left[] = { "100 %", "400 %", "800 %" }, *const right[] = { "200 %", "600 %" };
  GtkWidget *panel = toolbox_panel (win);
  graphene_rect_t pan = bounds_in (find_by_tooltip (GTK_WIDGET (win), "Pan"), panel);
  graphene_rect_t mag = bounds_in (find_by_tooltip (GTK_WIDGET (win), "Magnifier"), panel);
  int window_size = pango_font_description_get_size (pango_context_get_font_description (
                      gtk_widget_get_pango_context (GTK_WIDGET (win))));

  use_tool (win, "magnifier");
  wait_for_layout (win);
  for (guint i = 0; i < G_N_ELEMENTS (left) + G_N_ELEMENTS (right); i++)
    {
      gboolean on_left = i < G_N_ELEMENTS (left);
      GtkWidget *b = find_by_tooltip (GTK_WIDGET (win), on_left ? left[i] : right[i - G_N_ELEMENTS (left)]);
      graphene_rect_t r = bounds_in (b, panel);
      GtkWidget *label = gtk_button_get_child (GTK_BUTTON (b));

      g_assert_cmpfloat_with_epsilon (r.origin.x, on_left ? pan.origin.x : mag.origin.x, 1.0);
      g_assert_cmpfloat_with_epsilon (r.size.width, pan.size.width, 1.0);
      g_assert_cmpint (pango_font_description_get_size (pango_context_get_font_description (gtk_widget_get_pango_context (label))),
                       ==, window_size);
    }
}

/* AS-6 (REQ-0008-07): the toolbox has the same width for every tool, 87 px at the default text size, and at 150 % and
   200 % text the same for every tool at each size; every preset and level button is fully shown, no label cut. */
static void
as6_toolbox_width_is_the_same_for_every_tool (void)
{
  static const char *const tools[] = { "pan", "pencil", "brush", "airbrush", "eraser", "magnifier", "pick", "fill" };
  static const char *const lens[] = { "100 %", "200 %", "400 %", "600 %", "800 %" };
  GtkSettings *st = gtk_settings_get_default ();
  EaselWindow *win = start_window ();
  int dpi;

  wait_for_layout (win);
  g_object_get (st, "gtk-xft-dpi", &dpi, NULL);
  for (int pct = 100; pct <= 200; pct += 50)
    {
      int width = -1;

      /* -1 means "not set" (no settings daemon, as under meson): the default is 96 dpi, in 1024ths. */
      g_object_set (st, "gtk-xft-dpi", (dpi > 0 ? dpi : 96 * 1024) * pct / 100, NULL);
      wait_for_layout (win);
      for (guint i = 0; i < G_N_ELEMENTS (tools); i++)
        {
          use_tool (win, tools[i]);
          wait_for_layout (win);
          g_test_message ("text %d %%, %s: toolbox %d px", pct, tools[i], gtk_widget_get_width (toolbox_panel (win)));
          if (width < 0)
            width = gtk_widget_get_width (toolbox_panel (win));
          g_assert_cmpint (gtk_widget_get_width (toolbox_panel (win)), ==, width);
          if (g_str_equal (tools[i], "brush"))
            assert_options_fit (win, brush_tips, 12);
          else if (g_str_equal (tools[i], "airbrush"))
            assert_options_fit (win, airbrush_tips, 3);
          else if (g_str_equal (tools[i], "eraser"))
            {
              assert_options_fit (win, eraser_tips, 4);
              assert_options_fit (win, eraser_mode_tips, 2);
            }
          else if (g_str_equal (tools[i], "magnifier"))
            assert_options_fit (win, lens, 5);
        }
      if (pct == 100)
        {
          g_assert_cmpint (width, ==, 87);
          assert_lens_like_tool_buttons (win);
        }
    }
  g_object_set (st, "gtk-xft-dpi", dpi, NULL);
  close_windows ();
}

/* AS-4 (REQ-0008-04): before the picture exists, with the color chooser open and during a stroke, a preset does nothing. */
static void
as4_zoom_to_ignored_when_blocked (void)
{
  EaselWindow *win;
  EaselCanvas *canvas;
  EaselPaletteBar *bar;

  write_settings ("[new-picture]\nwidth=auto\nheight=auto\n");
  win = easel_window_new (app);
  g_assert_null (easel_window_get_picture (win));
  g_action_group_activate_action (G_ACTION_GROUP (win), "zoom-to", g_variant_new_int32 (400));
  g_assert_cmpint (easel_canvas_get_zoom (easel_window_get_canvas (win)), ==, 100);
  gtk_window_destroy (GTK_WINDOW (win));
  write_settings ("[new-picture]\nwidth=300\nheight=200\nbackground=#FFFF00\n");

  win = start_window ();
  canvas = easel_window_get_canvas (win);
  bar = easel_window_get_palette_bar (win);
  wait_for_layout (win);
  /* The action exists, so that a menu item can use it. */
  g_assert_true (g_action_group_has_action (G_ACTION_GROUP (win), "zoom-to"));
  easel_palette_bar_press (bar, 17, 1, 1);
  easel_palette_bar_press (bar, 17, 1, 2);
  g_assert_true (easel_window_is_choosing_color (win));
  g_action_group_activate_action (G_ACTION_GROUP (win), "zoom-to", g_variant_new_int32 (400));
  g_assert_cmpint (easel_window_get_zoom (win), ==, 100);
  choose (win, NULL);

  use_pencil (win);
  easel_canvas_press (canvas, 1, 10.5, 10.5);
  g_action_group_activate_action (G_ACTION_GROUP (win), "zoom-to", g_variant_new_int32 (400));
  g_assert_cmpint (easel_window_get_zoom (win), ==, 100);
  easel_canvas_release (canvas, 1);
  g_action_group_activate_action (G_ACTION_GROUP (win), "zoom-to", g_variant_new_int32 (400));
  g_assert_cmpint (easel_window_get_zoom (win), ==, 400);           /* after the release it works */
  close_windows ();
}


/* ---- 0009: the margin around the picture ---- */

/* AS-1, the margin part (REQ-0009-01): a small picture lies 12 pixels in from the canvas area's left and top; a big one,
   scrolled to its end, ends 12 pixels before the area's right and bottom edges. */
static void
as1_margin_and_workspace (void)
{
  EaselWindow *win;
  EaselCanvas *canvas;
  View view;
  graphene_rect_t r;
  int w, h, cx, cy;
  g_autofree uint32_t *px = NULL;

  adw_style_manager_set_color_scheme (adw_style_manager_get_default (), ADW_COLOR_SCHEME_FORCE_LIGHT);   /* #E7E7E7 is the light theme's */
  win = start_window ();
  canvas = easel_window_get_canvas (win);
  view = view_of (win);

  wait_for_layout (win);
  wait_for_next_frame (win);
  r = bounds_in (GTK_WIDGET (canvas), view.scroller);
  g_assert_cmpfloat_with_epsilon (r.origin.x, EASEL_VIEW_MARGIN, 0.5);
  g_assert_cmpfloat_with_epsilon (r.origin.y, EASEL_VIEW_MARGIN, 0.5);
  /* Rendered: the picture's first pixel 12 in, the pixel before it not the picture. */
  px = render_pixels (win, &w, &h, &cx, &cy);
  g_assert_cmphex (px[(gsize) (cy + 10) * w + cx], ==, 0xFFFFFF00u);
  g_assert_cmphex (px[(gsize) (cy + 10) * w + cx - 1], !=, 0xFFFFFF00u);
  g_assert_cmphex (px[(gsize) (cy - 1) * w + cx + 10], !=, 0xFFFFFF00u);
  /* The color part (REQ-0009-03): the workspace around the picture, past the shadow, is #E7E7E7. */
  g_assert_cmphex (px[(gsize) (cy + 100) * w + cx + 300 + 40], ==, 0xFFE7E7E7u);
  g_assert_cmphex (px[(gsize) (cy + 200 + 40) * w + cx + 100], ==, 0xFFE7E7E7u);
  close_windows ();

  write_settings ("[new-picture]\nwidth=3000\nheight=2000\nbackground=#FFFF00\n");
  win = start_window ();
  canvas = easel_window_get_canvas (win);
  view = view_of (win);
  wait_for_layout (win);
  gtk_adjustment_set_value (view.h, gtk_adjustment_get_upper (view.h) - gtk_adjustment_get_page_size (view.h));
  gtk_adjustment_set_value (view.v, gtk_adjustment_get_upper (view.v) - gtk_adjustment_get_page_size (view.v));
  wait_for_layout (win);
  wait_for_next_frame (win);
  r = bounds_in (GTK_WIDGET (canvas), view.scroller);
  g_assert_cmpfloat_with_epsilon (r.origin.x + r.size.width, gtk_adjustment_get_page_size (view.h) - EASEL_VIEW_MARGIN, 1.0);
  g_assert_cmpfloat_with_epsilon (r.origin.y + r.size.height, gtk_adjustment_get_page_size (view.v) - EASEL_VIEW_MARGIN, 1.0);
  close_windows ();
  write_settings ("[new-picture]\nwidth=300\nheight=200\nbackground=#FFFF00\n");
  adw_style_manager_set_color_scheme (adw_style_manager_get_default (), ADW_COLOR_SCHEME_DEFAULT);
}

/* AS-4 (REQ-0009-05): a press at a point of the view paints the pixel drawn there: 12 pixels in is pixel (0, 0), 11 is the
   margin; at 400 % scrolled by more than a page, the pixel under the view's center. */
static void
as4_pointer_maps_with_margin (void)
{
  EaselWindow *win = start_window ();
  EaselCanvas *canvas = easel_window_get_canvas (win);
  const EaselImage *img = easel_window_get_picture (win);
  View view = view_of (win);
  double x, y;

  wait_for_layout (win);
  use_pencil (win);
  easel_canvas_press (canvas, 1, view_to_canvas (view.h, 12.5), view_to_canvas (view.v, 12.5));
  easel_canvas_release (canvas, 1);
  g_assert_cmphex (easel_image_get (img, 0, 0), ==, 0xFF000000u);
  g_assert_cmpint (count_color (img, 0xFF000000u), ==, 1);
  /* And it is drawn there: 12 pixels in from the canvas area's corner, as the window lays it out. */
  {
    graphene_rect_t area = bounds_in (view.scroller, GTK_WIDGET (win));
    double scale = easel_canvas_get_scale (canvas);
    int w, h, cx, cy;
    g_autofree uint32_t *px = NULL;

    wait_for_next_frame (win);
    px = render_pixels (win, &w, &h, &cx, &cy);
    g_assert_cmphex (px[(gsize) ((area.origin.y + 12.5) * scale) * w + (gsize) ((area.origin.x + 12.5) * scale)], ==, 0xFF000000u);
  }
  easel_canvas_press (canvas, 1, view_to_canvas (view.h, 11.5), view_to_canvas (view.v, 30.5));     /* the margin */
  easel_canvas_release (canvas, 1);
  g_assert_cmpint (count_color (img, 0xFF000000u), ==, 1);
  close_windows ();

  write_settings ("[new-picture]\nwidth=3000\nheight=2000\nbackground=#FFFF00\n");
  win = start_window ();
  canvas = easel_window_get_canvas (win);
  img = easel_window_get_picture (win);
  view = view_of (win);
  wait_for_layout (win);
  g_action_group_activate_action (G_ACTION_GROUP (win), "zoom-large", NULL);
  wait_for_layout (win);
  g_assert_cmpfloat (gtk_adjustment_get_value (view.h), >, gtk_adjustment_get_page_size (view.h));
  use_pencil (win);
  x = view_to_canvas (view.h, gtk_adjustment_get_page_size (view.h) / 2);
  y = view_to_canvas (view.v, gtk_adjustment_get_page_size (view.v) / 2);
  easel_canvas_press (canvas, 1, x, y);
  easel_canvas_release (canvas, 1);
  g_assert_cmphex (easel_image_get (img, (int) (x / 4 * easel_canvas_get_scale (canvas)),
                                    (int) (y / 4 * easel_canvas_get_scale (canvas))), ==, 0xFF000000u);
  close_windows ();
  write_settings ("[new-picture]\nwidth=300\nheight=200\nbackground=#FFFF00\n");
}

/* AS-7 (REQ-0009-05, REQ-0004-16): the handles touch the picture and lie in the margin; a corner drag of 40 adds 40; a
   press on the shadow beside a handle's hit area does nothing. */
static void
as7_handles_in_margin (void)
{
  EaselWindow *win = start_window ();
  EaselCanvas *canvas = easel_window_get_canvas (win);
  graphene_rect_t r;
  double x, y;

  wait_for_layout (win);
  use_pencil (win);
  wait_for_layout (win);
  /* The margin is there, in the layout of the window. */
  g_assert_cmpfloat_with_epsilon (bounds_in (GTK_WIDGET (canvas), gtk_widget_get_parent (GTK_WIDGET (canvas))).origin.x,
                                  EASEL_VIEW_MARGIN, 0.5);
  g_assert_true (easel_canvas_get_handle_rect (canvas, EASEL_HANDLE_RIGHT, &r));
  g_assert_cmpfloat_with_epsilon (r.origin.x, 300, 0.5);                         /* touching the right edge */
  g_assert_cmpfloat (r.origin.x + r.size.width, <=, 300 + EASEL_VIEW_MARGIN);   /* inside the margin */
  g_assert_true (easel_canvas_get_handle_rect (canvas, EASEL_HANDLE_BOTTOM, &r));
  g_assert_cmpfloat_with_epsilon (r.origin.y, 200, 0.5);
  g_assert_cmpfloat (r.origin.y + r.size.height, <=, 200 + EASEL_VIEW_MARGIN);
  /* A press on the shadow beside the right handle's hit area, 10 pixels right of the picture: nothing. */
  easel_canvas_press (canvas, 1, 310.5, 50.5);
  g_assert_false (easel_canvas_is_resizing (canvas));
  easel_canvas_release (canvas, 1);
  g_assert_false (easel_window_get_modified (win));
  /* The corner handle dragged by 40 at 100 %. */
  handle_center (canvas, EASEL_HANDLE_CORNER, &x, &y);
  easel_canvas_press (canvas, 1, x, y);
  easel_canvas_motion (canvas, x + 40, y + 40);
  easel_canvas_release (canvas, 1);
  wait_for_layout (win);
  assert_size (win, 340, 240);
  close_windows ();
}


/* The picture point (in picture pixels) under a point of the canvas area, from where the window lays the canvas out (its
   bounds in the scrolled area), not from the margin constant. */
static void
picture_point_at (EaselWindow *win, double view_x, double view_y, double *px, double *py)
{
  EaselCanvas *canvas = easel_window_get_canvas (win);
  graphene_rect_t r = bounds_in (GTK_WIDGET (canvas), gtk_widget_get_ancestor (GTK_WIDGET (canvas), GTK_TYPE_SCROLLED_WINDOW));
  double l = easel_window_get_zoom (win) / (100.0 * easel_canvas_get_scale (canvas));

  *px = (view_x - r.origin.x) / l;
  *py = (view_y - r.origin.y) / l;
}

/* AS-5 (REQ-0009-05, REQ-0004-07): Fit to Window fits the picture with its margins: the whole percent at which the
   picture and 2 x 12 fit the canvas area, and one more would not; the view shows the picture's top-left with its margin. */
static void
as5_fit_to_window_with_margin (void)
{
  EaselWindow *win;
  View view;
  double scale, pw, ph;
  int zoom;
  graphene_rect_t r;

  write_settings ("[new-picture]\nwidth=3000\nheight=2000\nbackground=#FFFF00\n");
  win = start_window ();
  view = view_of (win);
  wait_for_layout (win);
  g_action_group_activate_action (G_ACTION_GROUP (win), "zoom-fit", NULL);
  wait_for_layout (win);
  scale = easel_canvas_get_scale (easel_window_get_canvas (win));
  zoom = easel_window_get_zoom (win);
  pw = gtk_adjustment_get_page_size (view.h) * scale;                 /* device pixels, no scrollbars */
  ph = gtk_adjustment_get_page_size (view.v) * scale;
  g_test_message ("area %.0f x %.0f, fit %d %%", pw, ph, zoom);
  /* It fits with its margins (the canvas area's own size, read from the window), and one percent more would not. */
  g_assert_cmpfloat (3000.0 * zoom / 100 + 2 * EASEL_VIEW_MARGIN * scale, <=, pw);
  g_assert_cmpfloat (2000.0 * zoom / 100 + 2 * EASEL_VIEW_MARGIN * scale, <=, ph);
  g_assert_true (3000.0 * (zoom + 1) / 100 + 2 * EASEL_VIEW_MARGIN * scale > pw
                 || 2000.0 * (zoom + 1) / 100 + 2 * EASEL_VIEW_MARGIN * scale > ph);
  /* No scrollbars needed, and the picture's top-left is 12 in. */
  g_assert_cmpfloat (gtk_adjustment_get_upper (view.h), <=, gtk_adjustment_get_page_size (view.h));
  g_assert_cmpfloat (gtk_adjustment_get_upper (view.v), <=, gtk_adjustment_get_page_size (view.v));
  r = bounds_in (GTK_WIDGET (easel_window_get_canvas (win)), view.scroller);
  g_assert_cmpfloat_with_epsilon (r.origin.x, EASEL_VIEW_MARGIN, 0.5);
  g_assert_cmpfloat_with_epsilon (r.origin.y, EASEL_VIEW_MARGIN, 0.5);
  close_windows ();
  write_settings ("[new-picture]\nwidth=300\nheight=200\nbackground=#FFFF00\n");
}

/* AS-6 (REQ-0009-05): a zoom command keeps the picture point at the center of the canvas area, the wheel the one under the
   pointer, and the Magnifier puts the pixel under the pointer at the center; at 100 %, at 400 % scrolled and at the fit
   zoom; each point read from the window's layout before and after, within one device pixel. */
static void
as6_zoom_anchors_with_margin (void)
{
  EaselWindow *win;
  EaselCanvas *canvas;
  View view;
  double x0, y0, x1, y1, w, h;

  write_settings ("[new-picture]\nwidth=3000\nheight=2000\nbackground=#FFFF00\n");
  win = start_window ();
  canvas = easel_window_get_canvas (win);
  view = view_of (win);
  wait_for_layout (win);
  w = gtk_adjustment_get_page_size (view.h);
  h = gtk_adjustment_get_page_size (view.v);

  /* Zoom In from 100 % at the start of the scroll: the center stays. */
  picture_point_at (win, w / 2, h / 2, &x0, &y0);
  g_action_group_activate_action (G_ACTION_GROUP (win), "zoom-in", NULL);
  wait_for_layout (win);
  picture_point_at (win, w / 2, h / 2, &x1, &y1);
  g_assert_cmpfloat_with_epsilon (x1, x0, 1.0);
  g_assert_cmpfloat_with_epsilon (y1, y0, 1.0);
  /* Zoom In again from 200 % to 300 % and 400 %, scrolled by now: the center stays. */
  g_action_group_activate_action (G_ACTION_GROUP (win), "zoom-in", NULL);
  wait_for_layout (win);
  g_action_group_activate_action (G_ACTION_GROUP (win), "zoom-in", NULL);
  wait_for_layout (win);
  g_assert_cmpint (easel_window_get_zoom (win), ==, 400);
  g_assert_cmpfloat (gtk_adjustment_get_value (view.h), >, w);
  picture_point_at (win, w / 2, h / 2, &x1, &y1);
  g_assert_cmpfloat_with_epsilon (x1, x0, 1.0);
  g_assert_cmpfloat_with_epsilon (y1, y0, 1.0);

  /* The wheel (Pan) at a point near the top-left of the view, at 400 % scrolled: that point stays. */
  picture_point_at (win, 40.5, 30.5, &x0, &y0);
  easel_canvas_zoom_at (canvas, -1, 40.5, 30.5);
  wait_for_layout (win);
  g_assert_cmpint (easel_window_get_zoom (win), ==, 300);
  picture_point_at (win, 40.5, 30.5, &x1, &y1);
  g_assert_cmpfloat_with_epsilon (x1, x0, 1.0);
  g_assert_cmpfloat_with_epsilon (y1, y0, 1.0);

  /* From the fit zoom: the wheel at the view's top-left corner, in the margin, keeps that point too. */
  g_action_group_activate_action (G_ACTION_GROUP (win), "zoom-fit", NULL);
  wait_for_layout (win);
  picture_point_at (win, 100.5, 80.5, &x0, &y0);
  easel_canvas_zoom_at (canvas, 1, 100.5, 80.5);
  wait_for_layout (win);
  picture_point_at (win, 100.5, 80.5, &x1, &y1);
  g_assert_cmpfloat_with_epsilon (x1, x0, 1.0);
  g_assert_cmpfloat_with_epsilon (y1, y0, 1.0);

  /* The Magnifier at 100 % with 2x: the box's center, here the pixel under the pointer (1000, 700), comes to the center. */
  g_action_group_activate_action (G_ACTION_GROUP (win), "zoom-normal", NULL);
  wait_for_layout (win);
  use_tool (win, "magnifier");
  {
    int bx, by, bw, bh;

    easel_magnifier_box ((int) (w * easel_canvas_get_scale (canvas)), (int) (h * easel_canvas_get_scale (canvas)), 200,
                         3000, 2000, 1000, 700, &bx, &by, &bw, &bh);
    click_pixel (win, 1, 1000, 700);
    g_assert_cmpint (easel_window_get_zoom (win), ==, 200);
    picture_point_at (win, w / 2, h / 2, &x1, &y1);
    g_assert_cmpfloat_with_epsilon (x1, bx + bw / 2.0, 1.0);
    g_assert_cmpfloat_with_epsilon (y1, by + bh / 2.0, 1.0);
  }
  close_windows ();
  write_settings ("[new-picture]\nwidth=300\nheight=200\nbackground=#FFFF00\n");
}


/* AS-8 (REQ-0009-05, REQ-0004-12): with the grid on at 400 %, scrolled by more than a page, the lines lie on the picture's
   pixel edges everywhere in the view, also in the first 12 pixels of it from the left and the top, where the margin
   would shift a grid read from the scroll offsets alone. */
static void
as8_grid_with_margin (void)
{
  EaselWindow *win;
  View view;
  graphene_rect_t area;
  int w, h, cx, cy;
  g_autofree uint32_t *px = NULL;

  write_settings ("[new-picture]\nwidth=3000\nheight=2000\nbackground=#FFFFFF\n");
  win = start_window ();
  view = view_of (win);
  wait_for_layout (win);
  g_action_group_activate_action (G_ACTION_GROUP (win), "grid", NULL);
  g_action_group_activate_action (G_ACTION_GROUP (win), "zoom-large", NULL);
  wait_for_layout (win);
  g_assert_cmpfloat (gtk_adjustment_get_value (view.h), >, gtk_adjustment_get_page_size (view.h));
  wait_for_next_frame (win);
  px = render_pixels (win, &w, &h, &cx, &cy);
  area = bounds_in (view.scroller, GTK_WIDGET (win));
  /* In the middle of the view, and 8 pixels in from its top-left corner: inside the 12 pixels a grid read from the scroll
     offsets would leave bare, and past the 2 pixels the scrolled area shades along its edge. */
  assert_grid_lines_near (px, w, cx, cy, 400, 1, 0xFFFFFFFFu, (int) (area.origin.x + area.size.width / 2),
                          (int) (area.origin.y + area.size.height / 2));
  assert_grid_lines_near (px, w, cx, cy, 400, 1, 0xFFFFFFFFu, (int) area.origin.x + 8, (int) area.origin.y + 8);
  close_windows ();
  write_settings ("[new-picture]\nwidth=300\nheight=200\nbackground=#FFFF00\n");
}


/* AS-10 (REQ-0009-07): with Pan, a primary drag that starts on the shadow (6 pixels before the picture's top-left) or
   in the margin 11 pixels from the canvas area's left edge pans the view as a drag on the picture would; with Pencil the
   same press does nothing. The press points come from the window's layout. */
static void
as10_pan_from_the_margin (void)
{
  EaselWindow *win;
  EaselCanvas *canvas;
  const EaselImage *img;
  View view;
  graphene_rect_t r;
  double x, y, before;

  write_settings ("[new-picture]\nwidth=3000\nheight=2000\nbackground=#FFFF00\n");
  win = start_window ();
  canvas = easel_window_get_canvas (win);
  img = easel_window_get_picture (win);
  view = view_of (win);
  wait_for_layout (win);
  r = bounds_in (GTK_WIDGET (canvas), view.scroller);           /* where the picture's top-left is in the canvas area */

  /* On the shadow, 6 pixels up and left of the picture's corner: drag 50 to the left and up. */
  use_tool (win, "pan");
  x = (r.origin.x - 6) - r.origin.x;
  y = (r.origin.y - 6) - r.origin.y;
  before = gtk_adjustment_get_value (view.h);
  easel_canvas_press (canvas, 1, x, y);
  easel_canvas_motion (canvas, x - 50, y - 50);
  easel_canvas_release (canvas, 1);
  wait_for_layout (win);
  g_assert_cmpfloat_with_epsilon (gtk_adjustment_get_value (view.h), before + 50, 0.5);
  g_assert_cmpfloat_with_epsilon (gtk_adjustment_get_value (view.v), 50, 0.5);

  /* In the margin 11 pixels from the canvas area's left edge, back at the start of the scroll: it pans too. */
  gtk_adjustment_set_value (view.h, 0);
  wait_for_layout (win);
  r = bounds_in (GTK_WIDGET (canvas), view.scroller);
  x = 11 - r.origin.x;
  y = 100 - r.origin.y;
  easel_canvas_press (canvas, 1, x, y);
  easel_canvas_motion (canvas, x - 30, y);
  easel_canvas_release (canvas, 1);
  wait_for_layout (win);
  g_assert_cmpfloat_with_epsilon (gtk_adjustment_get_value (view.h), 30, 0.5);
  g_assert_false (easel_window_get_modified (win));
  g_assert_cmpint (count_color (img, 0xFFFFFF00u), ==, 3000 * 2000);

  /* With Pencil the same press on the shadow paints nothing and moves nothing. */
  use_pencil (win);
  gtk_adjustment_set_value (view.h, 0);
  gtk_adjustment_set_value (view.v, 0);
  wait_for_layout (win);
  easel_canvas_press (canvas, 1, -6, -6);
  easel_canvas_motion (canvas, -56, -6);
  easel_canvas_release (canvas, 1);
  wait_for_layout (win);
  g_assert_cmpfloat (gtk_adjustment_get_value (view.h), ==, 0);
  g_assert_cmpint (count_color (img, 0xFFFFFF00u), ==, 3000 * 2000);
  g_assert_false (easel_window_get_modified (win));
  close_windows ();
  write_settings ("[new-picture]\nwidth=300\nheight=200\nbackground=#FFFF00\n");
}


/* The red, green and blue of a rendered pixel, summed: lower is darker. */
static int
brightness (uint32_t c)
{
  return (int) ((c >> 16) & 0xFF) + (int) ((c >> 8) & 0xFF) + (int) (c & 0xFF);
}

/* AS-2 (REQ-0009-02): a soft shadow on the four sides: next to the picture darker than the workspace, fading to it within
   about 6 pixels (one device pixel of tolerance for the rounded canvas size), and never over the picture; the saved file
   is the picture alone. */
static void
as2_shadow_on_four_sides (void)
{
  EaselWindow *win;
  EaselCanvas *canvas;
  g_autofree char *dir = g_dir_make_tmp ("easel-ui-XXXXXX", NULL);
  g_autofree char *path = g_build_filename (dir, "a.png", NULL);
  g_autoptr (GdkPixbuf) saved = NULL;
  int w, h, cx, cy, pw, ph;
  g_autofree uint32_t *px = NULL;
  double scale;

  adw_style_manager_set_color_scheme (adw_style_manager_get_default (), ADW_COLOR_SCHEME_FORCE_LIGHT);   /* on #E7E7E7, as AS-1 */
  win = start_window ();
  canvas = easel_window_get_canvas (win);
  wait_for_layout (win);
  wait_for_next_frame (win);
  px = render_pixels (win, &w, &h, &cx, &cy);
  scale = easel_canvas_get_scale (canvas);
  pw = (int) (300 * scale);
  ph = (int) (200 * scale);
#define PX(x, y) px[(gsize) (cy + (y)) * w + cx + (x)]
  {
    /* For each side: the pixel 1 out, the pixel 9 out (the workspace), and the picture's own pixel at that edge. */
    struct { int x1, y1, x9, y9, xin, yin; } side[] = {
      { -2, 100, -10, 100, 0, 100 },                          /* left: 2 out, as the canvas may round by 1 */
      { pw + 1, 100, pw + 9, 100, pw - 1, 100 },              /* right */
      { 150, -2, 150, -10, 150, 0 },                          /* top */
      { 150, ph + 1, 150, ph + 9, 150, ph - 1 },              /* bottom */
    };

    for (guint i = 0; i < G_N_ELEMENTS (side); i++)
      {
        uint32_t near = PX (side[i].x1, side[i].y1), far = PX (side[i].x9, side[i].y9), in = PX (side[i].xin, side[i].yin);

        g_test_message ("side %u: near %08x far %08x picture %08x", i, near, far, in);
        g_assert_cmpint (brightness (near), <, brightness (far) - 20);       /* a visible shadow next to the picture */
        g_assert_cmphex (in, ==, 0xFFFFFF00u);                               /* not over the picture */
      }
    /* Fading: from 1 out to 6 out it gets lighter, and 9 out it is the workspace. */
    g_assert_cmpint (brightness (PX (-2, 100)), <, brightness (PX (-4, 100)));
    g_assert_cmpint (brightness (PX (-4, 100)), <=, brightness (PX (-7, 100)));
  }
#undef PX
  /* The saved file is the picture alone: no margin, no shadow. */
  g_assert_true (easel_window_save_to (win, path));
  saved = gdk_pixbuf_new_from_file (path, NULL);
  g_assert_nonnull (saved);
  g_assert_cmpint (gdk_pixbuf_get_width (saved), ==, 300);
  g_assert_cmpint (gdk_pixbuf_get_height (saved), ==, 200);
  {
    const guchar *p = gdk_pixbuf_read_pixels (saved);

    g_assert_cmpint (p[0], ==, 0xFF);
    g_assert_cmpint (p[1], ==, 0xFF);
    g_assert_cmpint (p[2], ==, 0x00);
  }
  g_unlink (path);
  g_rmdir (dir);
  close_windows ();
  adw_style_manager_set_color_scheme (adw_style_manager_get_default (), ADW_COLOR_SCHEME_DEFAULT);
}


/* The workspace as drawn: a pixel 40 pixels right of a 300 x 200 picture, past its shadow. */
static uint32_t
workspace_pixel (EaselWindow *win)
{
  int w, h, cx, cy;
  g_autofree uint32_t *px = NULL;

  wait_for_layout (win);
  wait_for_next_frame (win);
  px = render_pixels (win, &w, &h, &cx, &cy);
  return px[(gsize) (cy + 100) * w + cx + (int) (340 * easel_canvas_get_scale (easel_window_get_canvas (win)))];
}

/* AS-3 (REQ-0009-04): `[view] workspace-color` as `#rrggbb`, either case, is the workspace; any other value, or none, is
   #E7E7E7; the settings file is not rewritten. */
static void
as3_workspace_color_from_settings (void)
{
  static const struct { const char *value; uint32_t expected; } cases[] = {
    { "#3465A4", 0xFF3465A4u }, { "#3465a4", 0xFF3465A4u }, { "3465A4", 0xFFE7E7E7u }, { "#346", 0xFFE7E7E7u },
    { "#3465A4FF", 0xFFE7E7E7u }, { "blue", 0xFFE7E7E7u }, { "", 0xFFE7E7E7u },
  };
  g_autofree char *settings_path = easel_settings_file_path ();

  adw_style_manager_set_color_scheme (adw_style_manager_get_default (), ADW_COLOR_SCHEME_FORCE_LIGHT);   /* the light theme's colors */
  for (guint i = 0; i < G_N_ELEMENTS (cases); i++)
    {
      g_autofree char *text = g_strdup_printf ("[new-picture]\nwidth=300\nheight=200\nbackground=#FFFF00\n[view]\n"
                                               "workspace-color=%s\n", cases[i].value);
      g_autofree char *after = NULL;
      EaselWindow *win;

      write_settings (text);
      win = start_window ();
      g_test_message ("workspace-color=%s", cases[i].value);
      g_assert_cmphex (workspace_pixel (win), ==, cases[i].expected);
      close_windows ();
      g_assert_true (g_file_get_contents (settings_path, &after, NULL, NULL));
      g_assert_cmpstr (after, ==, text);                                      /* not rewritten */
    }
  write_settings ("[new-picture]\nwidth=300\nheight=200\nbackground=#FFFF00\n");                  /* no key: the default */
  {
    EaselWindow *win = start_window ();

    g_assert_cmphex (workspace_pixel (win), ==, 0xFFE7E7E7u);
    close_windows ();
  }
  adw_style_manager_set_color_scheme (adw_style_manager_get_default (), ADW_COLOR_SCHEME_DEFAULT);
}

/* AS-9 (REQ-0009-06, REQ-0009-04, amended before M2): the workspace per theme: #E7E7E7 light and #2C2C2C dark by
   default; `workspace-color` alone for both; `workspace-color-dark` for the dark theme, falling back to `workspace-color`
   when it is invalid; and a change of theme while the app runs changes the workspace at once. */
static void
as9_workspace_dark_theme (void)
{
  static const struct { const char *keys; uint32_t light, dark; } cases[] = {
    { "", 0xFFE7E7E7u, 0xFF2C2C2Cu },
    { "workspace-color=#3465A4\n", 0xFF3465A4u, 0xFF3465A4u },
    { "workspace-color=#3465A4\nworkspace-color-dark=#102030\n", 0xFF3465A4u, 0xFF102030u },
    { "workspace-color=#3465A4\nworkspace-color-dark=#123\n", 0xFF3465A4u, 0xFF3465A4u },
    { "workspace-color-dark=#102030\n", 0xFFE7E7E7u, 0xFF102030u },
  };
  AdwStyleManager *manager = adw_style_manager_get_default ();

  for (guint i = 0; i < G_N_ELEMENTS (cases); i++)
    {
      g_autofree char *text = g_strdup_printf ("[new-picture]\nwidth=300\nheight=200\nbackground=#FFFF00\n[view]\n%s",
                                               cases[i].keys);
      EaselWindow *win;

      g_test_message ("keys: %s", cases[i].keys);
      write_settings (text);
      adw_style_manager_set_color_scheme (manager, ADW_COLOR_SCHEME_FORCE_LIGHT);
      win = start_window ();
      g_assert_cmphex (workspace_pixel (win), ==, cases[i].light);
      adw_style_manager_set_color_scheme (manager, ADW_COLOR_SCHEME_FORCE_DARK);    /* the theme changes while it runs */
      g_assert_cmphex (workspace_pixel (win), ==, cases[i].dark);
      adw_style_manager_set_color_scheme (manager, ADW_COLOR_SCHEME_FORCE_LIGHT);   /* and back */
      g_assert_cmphex (workspace_pixel (win), ==, cases[i].light);
      close_windows ();
    }
  adw_style_manager_set_color_scheme (manager, ADW_COLOR_SCHEME_DEFAULT);
  write_settings ("[new-picture]\nwidth=300\nheight=200\nbackground=#FFFF00\n");
}


int
main (int argc, char **argv)
{
  g_autofree char *config_dir = NULL;
  gboolean is_subprocess = FALSE;
  int ret, other;

  /* A GTest subprocess (g_test_trap_subprocess) runs next to its parent, which is a test-window too. */
  for (int i = 1; i < argc; i++)
    is_subprocess |= g_str_equal (argv[i], "--GTestSubprocess");
  if (!is_subprocess && (other = other_easel_process ()) != 0)
    {
      g_printerr ("Close every Easel window first: process %d is an Easel or a ui test, and a second window changes "
                  "the layout of the test windows.\n", other);
      return 1;
    }

  /* Keep the real settings file out of reach. G_TEST_OPTION_ISOLATE_DIRS would also hide the GSettings
     schemas libadwaita needs, so only the config dir is redirected, before anything reads it. */
  config_dir = g_dir_make_tmp ("easel-ui-XXXXXX", NULL);
  g_setenv ("XDG_CONFIG_HOME", config_dir, TRUE);

  if (!gtk_init_check ())
    return 77;

  g_test_init (&argc, &argv, NULL);

  app = easel_application_new ();
  /* Do not talk to a running Easel on the session bus. */
  g_application_set_flags (G_APPLICATION (app), G_APPLICATION_NON_UNIQUE | G_APPLICATION_HANDLES_OPEN);
  g_assert_true (g_application_register (G_APPLICATION (app), NULL, NULL));

  /* Every case starts from the same settings, whatever ran before it (the cases of one feature may be run alone, or in
     another order): the size and background most of them expect. A case that needs others writes them and restores these. */
  write_settings ("[new-picture]\nwidth=300\nheight=200\nbackground=#FFFF00\n");

  g_test_add_func ("/0001/as1_starts_with_settings", as1_starts_with_settings);
  g_test_add_func ("/0001/as2_auto_fits_view", as2_auto_fits_view);
  g_test_add_func ("/0001/as8_one_window", as8_one_window);
  g_test_add_func ("/0001/as13_zoom_keeps_point_under_pointer", as13_zoom_keeps_point_under_pointer);
  g_test_add_func ("/0001/as9_tool_selected_state", as9_tool_selected_state);
  g_test_add_func ("/0001/as21_pencil_on_canvas", as21_pencil_on_canvas);
  g_test_add_func ("/0001/as10_title_tracks_state", as10_title_tracks_state);
  g_test_add_func ("/0001/as26_save_existing_no_dialog", as26_save_existing_no_dialog);
  g_test_add_func ("/0001/as33_one_quit_prompt", as33_one_quit_prompt);
  g_test_add_func ("/0001/as35_input_ignored_before_picture", as35_input_ignored_before_picture);
  g_test_add_func ("/0001/as38_quit_save_while_dialog_open", as38_quit_save_while_dialog_open);
  g_test_add_func ("/0002/as6_initial_sensitivity", as6_initial_sensitivity);
  g_test_add_func ("/0002/as1_undo_shortcuts", as1_undo_shortcuts);
  g_test_add_func ("/0002/as3_header_buttons", as3_header_buttons);
  g_test_add_func ("/0002/as4_redo_insensitive_after_edit", as4_redo_insensitive_after_edit);
  g_test_add_func ("/0002/as5_non_edits_add_no_step", as5_non_edits_add_no_step);
  g_test_add_func ("/0002/as10_modified_follows_history", as10_modified_follows_history);
  g_test_add_func ("/0002/as11_undo_to_start_unmodified", as11_undo_to_start_unmodified);
  g_test_add_func ("/0002/as8_undo_during_stroke", as8_undo_during_stroke);
  g_test_add_func ("/0002/as9_shortcuts_focus_and_dialogs", as9_shortcuts_focus_and_dialogs);
  g_test_add_func ("/0002/as18_budget_reaches_history", as18_budget_reaches_history);
  g_test_add_func ("/0002/as8_focus_loss_unblocks_undo", as8_focus_loss_unblocks_undo);
  g_test_add_func ("/0002/as14_failed_save_keeps_saved_state", as14_failed_save_keeps_saved_state);
  g_test_add_func ("/0002/as15_save_during_stroke", as15_save_during_stroke);
  g_test_add_func ("/0004/texture_is_not_copied", texture_is_not_copied);
  g_test_add_func ("/0004/as1_menus", as1_menus);
  g_test_add_func ("/0004/as2_actions_ignored", as2_actions_ignored);
  g_test_add_func ("/0004/as2_actions_ignored_before_picture", as2_actions_ignored_before_picture);
  g_test_add_func ("/0004/as3_zoom_buttons", as3_zoom_buttons);
  g_test_add_func ("/0004/as4_normal_and_large", as4_normal_and_large);
  g_test_add_func ("/0004/as5_zoom_keeps_center", as5_zoom_keeps_center);
  g_test_add_func ("/0004/as6_fit_to_window", as6_fit_to_window);
  g_test_add_func ("/0004/as8_status_fields", as8_status_fields);
  g_test_add_func ("/0004/as8_status_before_picture", as8_status_before_picture);
  g_test_add_func ("/0004/as8_status_writes_only_on_change", as8_status_writes_only_on_change);
  g_test_add_func ("/0004/as9_status_bar_does_not_force_width", as9_status_bar_does_not_force_width);
  g_test_add_func ("/0001/as13_offset_is_displayed", as13_offset_is_displayed);
  g_test_add_func ("/0004/as10_grid_toggle", as10_grid_toggle);
  g_test_add_func ("/0004/as10_grid_pixels", as10_grid_pixels);
  g_test_add_func ("/0004/as12_grid_dialog", as12_grid_dialog);
  g_test_add_func ("/0004/as14_grid_starts_as_settings_say", as14_grid_starts_as_settings_say);
  g_test_add_func ("/0005/as1_file_menu", as1_file_menu);
  /* The cases that end the work of the user if they fail come first, so that a broken rule is found by an assertion
     before anything else can crash. */
  g_test_add_func ("/0005/as2_new_picture", as2_new_picture);
  g_test_add_func ("/0005/as2_new_history_records_strokes", as2_new_history_records_strokes);
  g_test_add_func ("/0005/as3_cancel_leaves_everything", as3_cancel_leaves_everything);
  g_test_add_func ("/0005/as3_new_save_fails", as3_new_save_fails);
  g_test_add_func ("/0005/as11_save_as_fails", as11_save_as_fails);
  g_test_add_func ("/0005/busy_cursor_before_decoding", busy_cursor_before_decoding);
  g_test_add_func ("/0005/as6_open_replaces_picture", as6_open_replaces_picture);
  g_test_add_func ("/0005/as7_prompt_before_open_dialog", as7_prompt_before_open_dialog);
  g_test_add_func ("/0005/as3_open_and_prompt", as3_open_and_prompt);
  g_test_add_func ("/0005/as8_open_error_dialog", as8_open_error_dialog);
  g_test_add_func ("/0005/as17_too_big_dialog", as17_too_big_dialog);
  g_test_add_func ("/0005/as14_saved_state_across_open_and_save_as", as14_saved_state_across_open_and_save_as);
  g_test_add_func ("/0005/as15_quit_around_open", as15_quit_around_open);
  g_test_add_func ("/0005/as15_actions_ignored_and_stroke_ended", as15_actions_ignored_and_stroke_ended);
  g_test_add_func ("/0005/as9_unknown_extension_error", as9_unknown_extension_error);
  g_test_add_func ("/0005/as10_save_clears_modified", as10_save_clears_modified);
  g_test_add_func ("/0005/as11_save_as", as11_save_as);
  g_test_add_func ("/0005/as12_save_keeps_format", as12_save_keeps_format);
  g_test_add_func ("/0005/as16_quality_setting_reaches_save", as16_quality_setting_reaches_save);
  g_test_add_func ("/0005/as19_save_memory_error_dialog", as19_save_memory_error_dialog);
  g_test_add_func ("/0005/missing_saver_error_dialog", missing_saver_error_dialog);
  g_test_add_func ("/0005/as14_save_as_moves_the_saved_state", as14_save_as_moves_the_saved_state);
  g_test_add_func ("/0005/as13_command_line", as13_command_line);
  g_test_add_func ("/0005/as13_command_line_error", as13_command_line_error);
  g_test_add_func ("/0005/as2_new_resets_the_view", as2_new_resets_the_view);
  g_test_add_func ("/0005/as2_new_memory_error", as2_new_memory_error);
  g_test_add_func ("/0005/as3_new_and_open_prompt", as3_new_and_open_prompt);
  g_test_add_func ("/0005/as14_new_starts_a_history", as14_new_starts_a_history);
  g_test_add_func ("/0005/as15_new_before_picture", as15_new_before_picture);
  g_test_add_func ("/0006/as1_toolbox_and_options", as1_toolbox_and_options);
  g_test_add_func ("/0006/as1_options_panel", as1_options_panel);
  g_test_add_func ("/0006/as2_presets_kept", as2_presets_kept);
  g_test_add_func ("/0006/as4_brush_on_canvas", as4_brush_on_canvas);
  g_test_add_func ("/0006/as7_eraser_on_canvas", as7_eraser_on_canvas);
  g_test_add_func ("/0006/as8_color_eraser_on_canvas", as8_color_eraser_on_canvas);
  g_test_add_func ("/0006/as9_brush_and_eraser_steps", as9_brush_and_eraser_steps);
  g_test_add_func ("/0006/as11_presets_are_not_edits_in_a_stroke", as11_presets_are_not_edits_in_a_stroke);
  g_test_add_func ("/0006/as6_airbrush_sprays", as6_airbrush_sprays);
  g_test_add_func ("/0006/as6_airbrush_clicks_differ", as6_airbrush_clicks_differ);
  g_test_add_func ("/0006/as6_airbrush_follows_the_pointer", as6_airbrush_follows_the_pointer);
  g_test_add_func ("/0006/as9_spray_one_step_and_cancel", as9_spray_one_step_and_cancel);
  g_test_add_func ("/0006/as10_numpad_steps", as10_numpad_steps);
  g_test_add_func ("/0006/as13_hints", as13_hints);
  g_test_add_func ("/0007/as1_new_tools_toolbox_hints", as1_new_tools_toolbox_hints);
  g_test_add_func ("/0007/as2_magnifier_level_kept", as2_magnifier_level_kept);
  g_test_add_func ("/0007/as6_fill_on_canvas", as6_fill_on_canvas);
  g_test_add_func ("/0007/as8_fill_nothing_on_canvas", as8_fill_nothing_on_canvas);
  g_test_add_func ("/0007/as9_fill_memory_dialog", as9_fill_memory_dialog);
  g_test_add_func ("/0007/as10_pick_follows_and_returns", as10_pick_follows_and_returns);
  g_test_add_func ("/0007/as11_pick_exact_colors", as11_pick_exact_colors);
  g_test_add_func ("/0007/as12_pick_outside_picture", as12_pick_outside_picture);
  g_test_add_func ("/0007/as13_pick_cancel_and_focus", as13_pick_cancel_and_focus);
  g_test_add_func ("/0007/as16_tool_change_ignored_during_pick", as16_tool_change_ignored_during_pick);
  g_test_add_func ("/0007/as3_magnifier_click", as3_magnifier_click);
  g_test_add_func ("/0007/as4_magnifier_edges", as4_magnifier_edges);
  g_test_add_func ("/0007/as1_cursors_follow_tool", as1_cursors_follow_tool);
  g_test_add_func ("/0007/as5_box_follows_pointer", as5_box_follows_pointer);
  g_test_add_func ("/0007/as14_numpad_ignored", as14_numpad_ignored);
  g_test_add_func ("/0007/as15_new_tools_ignored_when_blocked", as15_new_tools_ignored_when_blocked);
  g_test_add_func ("/0004/as10_grid_at_magnifier_levels", as10_grid_at_magnifier_levels);
  g_test_add_func ("/0007/as17_pencil_after_magnifier", as17_pencil_after_magnifier);
  g_test_add_func ("/0007/as18_lost_release_ends_the_press", as18_lost_release_ends_the_press);
  g_test_add_func ("/0008/as1_zoom_submenu_and_lens_levels", as1_zoom_submenu_and_lens_levels);
  g_test_add_func ("/0008/as2_zoom_to_preset", as2_zoom_to_preset);
  g_test_add_func ("/0008/as3_zoom_check_follows_zoom", as3_zoom_check_follows_zoom);
  g_test_add_func ("/0008/as6_toolbox_width_is_the_same_for_every_tool", as6_toolbox_width_is_the_same_for_every_tool);
  g_test_add_func ("/0008/as4_zoom_to_ignored_when_blocked", as4_zoom_to_ignored_when_blocked);
  g_test_add_func ("/0009/as1_margin_and_workspace", as1_margin_and_workspace);
  g_test_add_func ("/0009/as4_pointer_maps_with_margin", as4_pointer_maps_with_margin);
  g_test_add_func ("/0009/as7_handles_in_margin", as7_handles_in_margin);
  g_test_add_func ("/0009/as5_fit_to_window_with_margin", as5_fit_to_window_with_margin);
  g_test_add_func ("/0009/as6_zoom_anchors_with_margin", as6_zoom_anchors_with_margin);
  g_test_add_func ("/0009/as8_grid_with_margin", as8_grid_with_margin);
  g_test_add_func ("/0009/as10_pan_from_the_margin", as10_pan_from_the_margin);
  g_test_add_func ("/0009/as2_shadow_on_four_sides", as2_shadow_on_four_sides);
  g_test_add_func ("/0009/as3_workspace_color_from_settings", as3_workspace_color_from_settings);
  g_test_add_func ("/0009/as9_workspace_dark_theme", as9_workspace_dark_theme);
  g_test_add_func ("/0006/as12_cursor_follows_tool_preset_and_zoom", as12_cursor_follows_tool_preset_and_zoom);
  g_test_add_func ("/0004/as20_attributes_dialog", as20_attributes_dialog);
  g_test_add_func ("/0004/as22_undo_through_resize", as22_undo_through_resize);
  g_test_add_func ("/0004/as23_modified_across_resize", as23_modified_across_resize);
  g_test_add_func ("/0004/as25_same_size_no_step", as25_same_size_no_step);
  g_test_add_func ("/0004/as26_scroll_clamp_after_resize", as26_scroll_clamp_after_resize);
  g_test_add_func ("/0004/as27_error_dialog", as27_error_dialog);
  g_test_add_func ("/0004/as27_error_through_attributes", as27_error_through_attributes);
  g_test_add_func ("/0004/as30_undo_error_dialog", as30_undo_error_dialog);
  g_test_add_func ("/0004/as15_handles", as15_handles);
  g_test_add_func ("/0004/as17_handle_drag", as17_handle_drag);
  g_test_add_func ("/0004/as18_drag_cancel_rules", as18_drag_cancel_rules);
  g_test_add_func ("/0004/as28_panel_toggles", as28_panel_toggles);
  g_test_add_func ("/0004/as28_panels_start_as_settings_say", as28_panels_start_as_settings_say);
  g_test_add_func ("/0004/as7_pinch_in_window", as7_pinch_in_window);
  g_test_add_func ("/0003/as1_palette_bar", as1_palette_bar);
  g_test_add_func ("/0003/as1_swatch_layout", as1_swatch_layout);
  g_test_add_func ("/0003/as18_bar_geometry_and_swap_button", as18_bar_geometry_and_swap_button);
  g_test_add_func ("/0003/as3_click_picks_colors", as3_click_picks_colors);
  g_test_add_func ("/0003/as4_swap_colors", as4_swap_colors);
  g_test_add_func ("/0003/as5_strokes_use_current_colors", as5_strokes_use_current_colors);
  g_test_add_func ("/0003/as6_transparent_background_swap", as6_transparent_background_swap);
  g_test_add_func ("/0003/as7_colors_are_not_edits", as7_colors_are_not_edits);
  g_test_add_func ("/0003/as9_double_click_edits_swatch", as9_double_click_edits_swatch);
  g_test_add_func ("/0003/as10_f2_edits_focused_swatch", as10_f2_edits_focused_swatch);
  g_test_add_func ("/0003/as11_click_sequences", as11_click_sequences);
  g_test_add_func ("/0003/as12_indicator_edits_color", as12_indicator_edits_color);
  g_test_add_func ("/0003/as17_undo_ignored_while_chooser_open", as17_undo_ignored_while_chooser_open);
  g_test_add_func ("/0003/as14_edit_is_remembered", as14_edit_is_remembered);
  g_test_add_func ("/0003/as15_swatch_changes_when_write_fails", as15_swatch_changes_when_write_fails);
  ret = g_test_run ();
  g_object_unref (app);
  return ret;
}
