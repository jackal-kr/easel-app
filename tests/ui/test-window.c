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
#include "palette-bar.h"
#include "picture-file.h"
#include "palette-file.h"
#include "status-bar.h"
#include "settings-file.h"
#include "settings.h"
#include "view.h"
#include "window.h"

/* One application per process: GApplication cannot be registered twice. Tests close their windows. */
static GtkApplication *app;

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
  g_assert_cmpint (img->width, <, area_w);
  g_assert_cmpint (img->height, <, area_h);
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
  easel_view_to_pixel (gtk_adjustment_get_value (h) + 40.5, gtk_adjustment_get_value (v) + 30.5, 200, scale, &px, &py);
  g_assert_cmpint (px, ==, (int) (40.5 * scale));
  g_assert_cmpint (py, ==, (int) (30.5 * scale));

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
  g_assert_cmpstr (s1, ==, "Normal Size|win.zoom-normal; Large Size|win.zoom-large; Fit to Window|win.zoom-fit");
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
  cx = (gtk_adjustment_get_value (h) + ph / 2) / easel_canvas_get_scale (canvas);
  cy = (gtk_adjustment_get_value (v) + pv / 2) / easel_canvas_get_scale (canvas);

  g_action_group_activate_action (G_ACTION_GROUP (win), "zoom-in", NULL);
  wait_for_layout (win);
  g_assert_cmpint (easel_window_get_zoom (win), ==, 200);
  g_assert_cmpfloat_with_epsilon ((gtk_adjustment_get_value (h) + ph / 2) / 2 / easel_canvas_get_scale (canvas), cx, 1.0);
  g_assert_cmpfloat_with_epsilon ((gtk_adjustment_get_value (v) + pv / 2) / 2 / easel_canvas_get_scale (canvas), cy, 1.0);

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
      want = easel_view_fit_zoom (aw, ah, sizes[i][0], sizes[i][1]);

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
      g_assert_cmpfloat_with_epsilon (r.origin.x, -gtk_adjustment_get_value (h), 1.0);   /* GTK places it at whole pixels */
      g_assert_cmpfloat_with_epsilon (r.origin.y, -gtk_adjustment_get_value (v), 1.0);
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
  g_assert_cmpfloat_with_epsilon (gtk_adjustment_get_value (h), MAX (0, MIN (1500, 1700 - page_w)), 1.0);
  g_assert_cmpfloat_with_epsilon (gtk_adjustment_get_value (v), MAX (0, MIN (1500, 1700 - page_h)), 1.0);
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

  /* The scrollable area includes them, only while they show. */
  gtk_widget_measure (GTK_WIDGET (canvas), GTK_ORIENTATION_HORIZONTAL, -1, &min_pencil, &ignored, NULL, NULL);
  g_assert_cmpint (min_pencil, ==, min_pan + 7);

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

int
main (int argc, char **argv)
{
  g_autofree char *config_dir = NULL;
  int ret;

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
