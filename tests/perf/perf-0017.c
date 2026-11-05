/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Measurements of feature 0017 (T010, the Plan's Measurements): a key typed at the end of a large text box, from the edit to the painted
   frame (the layout made again, the ink drawn, the box's texture composed and shown): a box over a 1920 x 1080 picture holding 60 lines of
   DejaVu Sans 12, and a box 8000 wide on an 8000 x 8000 picture holding 40 lines; the first fill, then the median and the worst of 15
   keys, with the peak memory; then the box put down, Transparent, and made again and put down Opaque (T013); the font card's family list
   opened (T016). The budget for a key is one frame on a 1920 x 1080 picture (engineering.md); this prints numbers and does not fail. Run
   with meson test --benchmark; needs a display, no other Easel window, and the pointer parked outside the window. The typing itself is
   T011's: here a key is the edit it makes (one character at the end) and the box laid out and drawn again. */

#include "perf-common.h"

#include <stdio.h>
#include <string.h>

#include "canvas.h"
#include "text-buffer.h"
#include "text-render.h"

/* The peak resident memory of the process so far, in MB (VmHWM). */
static double
peak_mb (void)
{
  FILE *f = fopen ("/proc/self/status", "r");
  char line[256];
  long kb = 0;

  while (f != NULL && fgets (line, sizeof line, f))
    if (sscanf (line, "VmHWM: %ld kB", &kb) == 1)
      break;
  if (f != NULL)
    fclose (f);
  return kb / 1024.0;
}

static void
write_text_settings (const char *dir, int w, int h)
{
  g_autofree char *path = g_build_filename (dir, "easel", "settings.ini", NULL);
  g_autofree char *parent = g_path_get_dirname (path);
  g_autofree char *text = g_strdup_printf ("[new-picture]\nwidth=%d\nheight=%d\ncanvas-color=#FFFFFF\n\n[text]\nfont=DejaVu Sans\nsize=12\n",
                                           w, h);

  g_mkdir_with_parents (parent, 0700);
  g_file_set_contents (path, text, -1, NULL);
}

/* A box from (0, 0) to (box_w - 1, box_h - 1), made by a drag at 100 %. */
static void
make_box (EaselWindow *win, int box_w, int box_h)
{
  EaselCanvas *canvas = easel_window_get_canvas (win);
  double l = easel_window_get_zoom (win) / 100.0 / easel_canvas_get_scale (canvas);

  easel_canvas_press (canvas, 1, 0.5 * l, 0.5 * l);
  easel_canvas_motion (canvas, (box_w - 0.5) * l, (box_h - 0.5) * l);
  easel_canvas_release (canvas, 1);
}

static void
measure_typing (const char *config_dir, const char *what, int pic_w, int pic_h, int box_w, int box_h, int lines, int chars)
{
  enum { N = 15 };
  EaselWindow *win;
  EaselCanvas *canvas;
  EaselTextBuffer *b;
  EaselTextStyle st;
  GString *text = g_string_new (NULL);
  double ms[N], fill, before, worst = 0;
  int worst_at = 0;
  gint64 start;
  size_t len;

  write_text_settings (config_dir, pic_w, pic_h);
  win = open_window ();
  canvas = easel_window_get_canvas (win);
  g_action_group_activate_action (G_ACTION_GROUP (win), "zoom-normal", NULL);
  g_action_group_activate_action (G_ACTION_GROUP (win), "tool", g_variant_new_string ("text"));
  wait_for_paint (win);
  make_box (win, box_w, box_h);
  wait_for_paint (win);
  b = easel_canvas_text_buffer (canvas);
  g_assert (b != NULL);
  for (int i = 0; i < lines; i++)
    {
      for (int k = 0; k < chars; k++)
        g_string_append_c (text, "abcdefghij klmnopqrst uvwxyz 0123456789 "[k % 40]);
      g_string_append_c (text, '\n');
    }
  before = peak_mb ();
  easel_canvas_text_style_at_caret (canvas, &st);
  start = g_get_monotonic_time ();
  g_assert (easel_text_buffer_insert (b, text->str, text->len, &st, EASEL_TEXT_EDIT_PASTE));
  easel_canvas_text_edited (canvas);
  wait_for_paint (win);
  fill = (g_get_monotonic_time () - start) / 1000.0;
  for (int i = 0; i < N; i++)
    {
      easel_text_buffer_text (b, &len);
      easel_text_buffer_select (b, len, len);
      easel_canvas_text_style_at_caret (canvas, &st);
      start = g_get_monotonic_time ();
      g_assert (easel_text_buffer_insert (b, "x", 1, &st, EASEL_TEXT_EDIT_TYPING));
      easel_canvas_text_edited (canvas);
      wait_for_paint (win);
      ms[i] = (g_get_monotonic_time () - start) / 1000.0;
      if (ms[i] > worst)
        {
          worst = ms[i];
          worst_at = i + 1;
        }
    }
  qsort (ms, N, sizeof ms[0], cmp_double);
  printf ("%s: fill %.1f ms; a key: median %.1f ms, worst %.1f ms (key %d of %d); peak %.0f MB (before the text %.0f MB)\n", what, fill,
          ms[N / 2], worst, worst_at, N, peak_mb (), before);
  {
    /* Where a key's time goes: the layout, the ink (the two made again by each edit), the rest being the box's texture and the frame */
    double lay[N], inked[N];
    int bx, by, bw, bh;

    g_assert (easel_canvas_text_box (canvas, &bx, &by, &bw, &bh));
    for (int i = 0; i < N; i++)
      {
        PangoLayout *layout;
        EaselTextInk ink;

        start = g_get_monotonic_time ();
        layout = easel_text_render_layout (b, bw);
        easel_text_render_height (layout);
        lay[i] = (g_get_monotonic_time () - start) / 1000.0;
        start = g_get_monotonic_time ();
        g_assert (easel_text_render_ink (layout, b, bx, by, SIZE_MAX, &ink));
        inked[i] = (g_get_monotonic_time () - start) / 1000.0;
        g_free ((uint32_t *) ink.color);
        g_free ((uint8_t *) ink.mask);
        g_object_unref (layout);
      }
    qsort (lay, N, sizeof lay[0], cmp_double);
    qsort (inked, N, sizeof inked[0], cmp_double);
    printf ("    of it: the layout %.1f ms, the ink %.1f ms (medians); the rest, %.1f ms, is the box's texture and the frame\n", lay[N / 2],
            inked[N / 2], ms[N / 2] - lay[N / 2] - inked[N / 2]);
  }
  {
    /* The put-down (REQ-0017-16), from the request to the painted frame: the box Transparent, then made again and filled, Opaque */
    double put[2];

    for (int opaque = 0; opaque < 2; opaque++)
      {
        if (opaque)
          {
            g_action_group_activate_action (G_ACTION_GROUP (win), "undo", NULL);
            wait_for_paint (win);
            make_box (win, box_w, box_h);
            wait_for_paint (win);
            b = easel_canvas_text_buffer (canvas);
            g_assert (b != NULL);
            easel_canvas_text_style_at_caret (canvas, &st);
            g_assert (easel_text_buffer_insert (b, text->str, text->len, &st, EASEL_TEXT_EDIT_PASTE));
            easel_canvas_text_edited (canvas);
            easel_canvas_set_text_opaque (canvas, TRUE);
            wait_for_paint (win);
          }
        start = g_get_monotonic_time ();
        g_assert (easel_canvas_put_down (canvas));
        wait_for_paint (win);
        put[opaque] = (g_get_monotonic_time () - start) / 1000.0;
        g_assert (easel_canvas_text_buffer (canvas) == NULL);
      }
    printf ("    the put-down: Transparent %.1f ms, Opaque %.1f ms; peak %.0f MB\n", put[0], put[1], peak_mb ());
  }
  g_string_free (text, TRUE);
  gtk_window_destroy (GTK_WINDOW (win));
  for (int i = 0; i < 20; i++)
    g_main_context_iteration (NULL, FALSE);
}

static gboolean list_painted;

static void
on_list_painted (GdkFrameClock *clock, gpointer data)
{
  list_painted = TRUE;
}

/* The first popover under `root`, or NULL. */
static GtkWidget *
find_popover (GtkWidget *root)
{
  for (GtkWidget *child = gtk_widget_get_first_child (root); child; child = gtk_widget_get_next_sibling (child))
    {
      GtkWidget *found = GTK_IS_POPOVER (child) ? child : find_popover (child);

      if (found != NULL)
        return found;
    }
  return NULL;
}

/* Opening the font card's family popover (R5: built at the first opening, each row's family loaded when the row is bound): from the
   request to the list's first painted frame, three times (the first opening builds the list, the others find the names known). */
static void
measure_family_list (const char *config_dir)
{
  EaselWindow *win;
  EaselFontCard *card;
  GtkWidget *button, *popover;
  PangoFontFamily **families = NULL;
  int n = 0;

  write_text_settings (config_dir, 1920, 1080);
  win = open_window ();
  g_action_group_activate_action (G_ACTION_GROUP (win), "zoom-normal", NULL);
  g_action_group_activate_action (G_ACTION_GROUP (win), "tool", g_variant_new_string ("text"));
  wait_for_paint (win);
  make_box (win, 400, 100);
  wait_for_paint (win);
  pango_font_map_list_families (pango_cairo_font_map_get_default (), &families, &n);
  g_free (families);
  card = easel_window_get_font_card (win);
  button = easel_font_card_get_family_button (card);
  for (int run = 0; run < 3; run++)
    {
      gint64 start = g_get_monotonic_time (), deadline = start + 10 * G_USEC_PER_SEC;
      gulong id = 0;

      easel_font_card_open_family (card);                         /* as a press on the "Font family" button */
      popover = find_popover (button);
      while ((popover == NULL || !gtk_widget_get_mapped (popover)) && g_get_monotonic_time () < deadline)
        {
          g_main_context_iteration (NULL, TRUE);
          popover = find_popover (button);
        }
      list_painted = FALSE;
      if (popover != NULL && gtk_widget_get_frame_clock (popover) != NULL)
        id = g_signal_connect (gtk_widget_get_frame_clock (popover), "after-paint", G_CALLBACK (on_list_painted), NULL);
      while (!list_painted && g_get_monotonic_time () < deadline)
        g_main_context_iteration (NULL, TRUE);
      printf ("the family list (%d families), opening %d: %.1f ms\n", n, run + 1, (g_get_monotonic_time () - start) / 1000.0);
      if (id != 0)
        g_signal_handler_disconnect (gtk_widget_get_frame_clock (popover), id);
      gtk_popover_popdown (GTK_POPOVER (popover));
      for (int i = 0; i < 20; i++)
        g_main_context_iteration (NULL, FALSE);
    }
  gtk_window_destroy (GTK_WINDOW (win));
  for (int i = 0; i < 20; i++)
    g_main_context_iteration (NULL, FALSE);
}

int
main (int argc, char **argv)
{
  g_autofree char *config_dir = g_dir_make_tmp ("easel-perf-XXXXXX", NULL);

  g_setenv ("XDG_CONFIG_HOME", config_dir, TRUE);
  if (!gtk_init_check ())
    return 77;
  g_object_set (gtk_settings_get_default (), "gtk-cursor-blink", FALSE, NULL);   /* a blink would add frames of its own */
  app = easel_application_new ();
  g_application_set_flags (G_APPLICATION (app), G_APPLICATION_NON_UNIQUE);
  g_application_register (G_APPLICATION (app), NULL, NULL);
  measure_typing (config_dir, "1920 x 1080, a box over it, 60 lines", 1920, 1080, 1920, 1080, 60, 150);
  measure_typing (config_dir, "8000 x 8000, a box 8000 wide, 40 lines", 8000, 8000, 8000, 800, 40, 600);
  measure_family_list (config_dir);
  g_object_unref (app);
  return 0;
}
