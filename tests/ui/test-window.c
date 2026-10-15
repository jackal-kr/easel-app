/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Widget-level scenarios of feature 0001. Needs a display; exits 77 (skip) without one. */

#include <adwaita.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <glib/gstdio.h>
#include <string.h>
#include <unistd.h>

#include "application.h"
#include "canvas.h"
#include "palette-bar.h"
#include "palette-file.h"
#include "settings-file.h"
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
  g_application_set_flags (G_APPLICATION (app), G_APPLICATION_NON_UNIQUE);
  g_assert_true (g_application_register (G_APPLICATION (app), NULL, NULL));

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
