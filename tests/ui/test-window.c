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
#include "status-bar.h"
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
  static const int sizes[][2] = { { 3000, 2000 }, { 800, 600 }, { 100, 100 }, { 10000, 100 } };

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
      want = easel_view_fit_level (aw, ah, sizes[i][0], sizes[i][1]);

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
