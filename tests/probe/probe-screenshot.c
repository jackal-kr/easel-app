/* SPDX-License-Identifier: GPL-3.0-or-later */
/* The README's and the metadata's screenshot (data/screenshots/main.png), made again for a release: the window as Easel opens it on a 1920 x 1200 screen at 100 %, a picture
   drawn through the app's own tools (a gradient sky, smooth shapes, a smooth brush stroke, smooth text), then Shapes with a star left
   waiting (its dashed line and handles), rendered by GTK and saved as PNG. Run through tools/isolated.sh, so that the desktop is untouched and the
   theme is GNOME's default: tools/isolated.sh build/tests/probe-screenshot data/screenshots/main.png */

#include <glib/gstdio.h>
#include <string.h>

#include "application.h"
#include "canvas.h"
#include "window.h"

static GtkApplication *app;

static void
wait_frames (EaselWindow *win, int n)
{
  GdkFrameClock *clock = gtk_widget_get_frame_clock (GTK_WIDGET (win));
  gint64 deadline = g_get_monotonic_time () + 3 * G_USEC_PER_SEC;
  gint64 start = clock != NULL ? gdk_frame_clock_get_frame_counter (clock) : 0;

  while (g_get_monotonic_time () < deadline && (clock == NULL || gdk_frame_clock_get_frame_counter (clock) < start + n))
    {
      gtk_widget_queue_draw (GTK_WIDGET (win));
      g_main_context_iteration (NULL, FALSE);
      clock = gtk_widget_get_frame_clock (GTK_WIDGET (win));
    }
}

static void
drag (EaselCanvas *canvas, const double *xy, int n)
{
  easel_canvas_press (canvas, 1, xy[0] + 0.5, xy[1] + 0.5);
  for (int k = 1; k < n; k++)
    easel_canvas_motion (canvas, xy[2 * k] + 0.5, xy[2 * k + 1] + 0.5);
  easel_canvas_release (canvas, 1);
}

static void
key (EaselWindow *win, guint keyval)
{
  g_autoptr (GListModel) controllers = gtk_widget_observe_controllers (GTK_WIDGET (win));

  for (guint i = 0; i < g_list_model_get_n_items (controllers); i++)
    {
      g_autoptr (GtkEventController) c = g_list_model_get_item (controllers, i);
      gboolean handled = FALSE;

      if (GTK_IS_EVENT_CONTROLLER_KEY (c))
        g_signal_emit_by_name (c, "key-pressed", keyval, 0, 0, &handled);
    }
}

static void
tool (EaselWindow *win, const char *name)
{
  if (g_str_equal (name, "rectangle") || g_str_equal (name, "rounded-rectangle") || g_str_equal (name, "ellipse"))
    {
      g_action_group_change_action_state (G_ACTION_GROUP (win), "shape", g_variant_new_string (name));   /* 0043: shapes of Shapes */
      name = "shapes";
    }
  g_action_group_activate_action (G_ACTION_GROUP (win), "tool", g_variant_new_string (name));
}

int
main (int argc, char **argv)
{
  g_autofree char *config = g_dir_make_tmp ("easel-shot-XXXXXX", NULL);
  g_autofree char *dir = g_build_filename (config, "easel", NULL);
  g_autofree char *settings = g_build_filename (dir, "settings.ini", NULL);
  EaselWindow *win;
  EaselCanvas *canvas;
  gint64 deadline;

  if (argc != 2)
    {
      g_printerr ("usage: probe-screenshot OUT.png\n");
      return 2;
    }
  g_setenv ("XDG_CONFIG_HOME", config, TRUE);
  g_mkdir_with_parents (dir, 0700);
  g_file_set_contents (settings,
                       "[new-picture]\nwidth=640\nheight=420\nbackground=#FFFFFF\n\n[drawing]\nsmooth=true\nline-width=3\nfill-style=outline-and-fill\n\n"
                       "[text]\nfont=DejaVu Sans\nsize=40\nbold=true\nsmooth=true\n",
                       -1, NULL);
  if (!gtk_init_check ())
    return 77;
  app = easel_application_new ();
  g_application_set_flags (G_APPLICATION (app), G_APPLICATION_NON_UNIQUE);
  g_application_register (G_APPLICATION (app), NULL, NULL);
  g_application_activate (G_APPLICATION (app));
  win = EASEL_WINDOW (gtk_application_get_active_window (app));
  /* the window at the size Easel opens at on the 1920 x 1200 screen at scale 1, 100 % in Settings (JOD, 2026-11-13) */
  deadline = g_get_monotonic_time () + 5 * G_USEC_PER_SEC;
  while (easel_window_get_picture (win) == NULL && g_get_monotonic_time () < deadline)
    g_main_context_iteration (NULL, FALSE);
  wait_frames (win, 3);
  canvas = easel_window_get_canvas (win);

  /* a sky: a radial gradient from a soft blue, put down by Enter */
  tool (win, "gradient");
  easel_canvas_set_colors (canvas, 0xFF7FB6F0u, 0xFFFFFFFFu);
  drag (canvas, (const double[]) { 520, 60, 160, 400 }, 2);
  key (win, GDK_KEY_Return);
  /* a smooth sun, a hill and a house's walls and roof; each shape of Shapes waits after its release (0043), so Enter puts it down
     before the colors change (a color change would recolor a waiting shape, REQ-0043-09) */
  tool (win, "ellipse");
  easel_canvas_set_colors (canvas, 0xFFE0A000u, 0xFFFFD84Au);
  drag (canvas, (const double[]) { 470, 40, 560, 130 }, 2);
  key (win, GDK_KEY_Return);
  easel_canvas_set_colors (canvas, 0xFF2F7D32u, 0xFF4CAF50u);
  drag (canvas, (const double[]) { -80, 300, 720, 560 }, 2);
  key (win, GDK_KEY_Return);
  tool (win, "rounded-rectangle");
  easel_canvas_set_colors (canvas, 0xFF7A3E1Du, 0xFFF4E3C1u);
  drag (canvas, (const double[]) { 90, 230, 220, 330 }, 2);
  key (win, GDK_KEY_Return);
  tool (win, "polygon");
  easel_canvas_set_colors (canvas, 0xFF7A3E1Du, 0xFFC0392Bu);
  drag (canvas, (const double[]) { 80, 232, 155, 175 }, 2);
  drag (canvas, (const double[]) { 230, 232, 230, 232 }, 2);
  key (win, GDK_KEY_Return);
  /* a smooth brush stroke: a bird's wings */
  tool (win, "brush");
  easel_canvas_set_colors (canvas, 0xFF333344u, 0xFFFFFFFFu);
  drag (canvas, (const double[]) { 300, 120, 315, 110, 330, 118, 345, 110, 360, 120 }, 5);
  /* smooth text */
  tool (win, "text");
  easel_canvas_set_colors (canvas, 0xFF1E3A5Fu, 0xFFFFFFFFu);
  drag (canvas, (const double[]) { 250, 175, 635, 240 }, 2);
  {
    EaselTextBuffer *b = easel_canvas_text_buffer (canvas);
    EaselTextStyle st;

    if (b != NULL)
      {
        easel_canvas_text_style_at_caret (canvas, &st);
        easel_text_buffer_insert (b, "Hello, Easel", strlen ("Hello, Easel"), &st, EASEL_TEXT_EDIT_TYPING);
        easel_canvas_text_edited (canvas);
      }
  }
  /* Shapes chosen, a star left waiting in the sky: its dashed line and handles show (0043), and the options show the Shape button, the
     fill styles, the widths and Sharp | Smooth; a tool change puts the text down; the focus on the canvas */
  g_action_group_change_action_state (G_ACTION_GROUP (win), "shape", g_variant_new_string ("5-point-star"));
  tool (win, "shapes");
  easel_canvas_set_colors (canvas, 0xFFB8860Bu, 0xFFFFE066u);
  drag (canvas, (const double[]) { 60, 40, 150, 125 }, 2);
  gtk_widget_grab_focus (GTK_WIDGET (canvas));
  gtk_window_set_focus_visible (GTK_WINDOW (win), FALSE);              /* no focus mark on a button in the picture */
  {
    /* the styles' fades (a tool button's background when it stops being chosen) take a few hundred milliseconds */
    gint64 settle = g_get_monotonic_time () + 600 * 1000;

    while (g_get_monotonic_time () < settle)
      wait_frames (win, 1);
  }
  {
    g_autoptr (GdkPaintable) paintable = gtk_widget_paintable_new (GTK_WIDGET (win));
    int w = gtk_widget_get_width (GTK_WIDGET (win)), h = gtk_widget_get_height (GTK_WIDGET (win));
    GtkSnapshot *snapshot = gtk_snapshot_new ();
    g_autoptr (GskRenderNode) node = NULL;
    g_autoptr (GdkTexture) texture = NULL;
    GskRenderer *renderer = gtk_native_get_renderer (GTK_NATIVE (win));

    gdk_paintable_snapshot (paintable, snapshot, w, h);
    node = gtk_snapshot_free_to_node (snapshot);
    if (node == NULL || renderer == NULL)
      return 1;
    texture = gsk_renderer_render_texture (renderer, node, &GRAPHENE_RECT_INIT (0, 0, (float) w, (float) h));
    if (!gdk_texture_save_to_png (texture, argv[1]))
      return 1;
    g_print ("wrote %s (%d x %d)\n", argv[1], w, h);
  }
  gtk_window_destroy (GTK_WINDOW (win));
  g_object_unref (app);
  g_remove (settings);
  g_rmdir (dir);
  g_rmdir (config);
  return 0;
}
