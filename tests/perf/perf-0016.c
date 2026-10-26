/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Measurements of feature 0016 (T009): a paste of a 2000 x 2000 and an 8000 x 8000 picture, from Easel's own copy and from another
   app's PNG, and a Copy and a Cut of a selection of those sizes, each timed from the command to the painted frame, with the peak
   memory; and the PNG encode another app's read starts (the worker thread's work, which the window does not wait for), timed apart.
   JOD's rule for 0016: one display frame, else the numbers go to him before any fix. This prints numbers and does not fail. Run in the
   real session, with no other Easel window open (it uses the desktop's clipboard, which only a focused window may set) and the
   pointer outside its window: a real pointer motion with no button down is a lost release (REQ-0007-18). */

#include "perf-common.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

#include "clipboard.h"
#include "picture-file.h"

/* The peak resident memory of the process so far, in MB (VmHWM, a high-water mark shared by the cases of one run). */
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

/* The command, then the painted frame: milliseconds. */
static double
timed (EaselWindow *win, const char *action)
{
  gint64 start = g_get_monotonic_time ();

  g_action_group_activate_action (G_ACTION_GROUP (win), action, NULL);
  wait_for_paint (win);
  return (g_get_monotonic_time () - start) / 1000.0;
}

/* A photo-like picture: smooth light and color with grain, which compresses as a photo does (a few bytes a pixel in PNG). */
static EaselImage *
photo_like (int size)
{
  EaselImage *img = easel_image_new (size, size, 0);
  guint32 seed = 12345;

  for (int y = 0; y < size; y++)
    for (int x = 0; x < size; x++)
      {
        int noise;
        guint32 r, g, b;

        seed = seed * 1103515245u + 12345u;
        noise = (int) ((seed >> 16) % 25) - 12;
        r = (guint32) CLAMP (128 + 100 * sin (x / (double) size * 6.0) + noise, 0, 255);
        g = (guint32) CLAMP (128 + 100 * cos (y / (double) size * 5.0) + noise, 0, 255);
        b = (guint32) CLAMP (128 + 80 * sin ((x + y) / (double) size * 4.0) + noise, 0, 255);
        img->px[(gsize) y * size + x] = 0xFF000000u | r << 16 | g << 8 | b;
      }
  return img;
}

/* Another app's photo-like PNG pasted at 100 %: from the command to the painted frame, with the decode alone timed apart. */
static void
measure_photo (const char *config_dir, int size)
{
  EaselWindow *win;
  EaselImage *pic = photo_like (size), *back = NULL;
  g_autoptr (GBytes) png = easel_picture_encode (pic, EASEL_FORMAT_PNG, 90, NULL);
  g_autoptr (GdkContentProvider) other = gdk_content_provider_new_for_bytes ("image/png", png);
  double paste_ms, decode_ms, before;
  gint64 start;
  int w, h;

  easel_image_free (pic);
  start = g_get_monotonic_time ();
  easel_picture_open_bytes (png, &back, &w, &h);
  decode_ms = (g_get_monotonic_time () - start) / 1000.0;
  easel_image_free (back);
  write_settings (config_dir, size, size);
  win = open_window ();
  g_action_group_activate_action (G_ACTION_GROUP (win), "zoom-normal", NULL);
  wait_for_paint (win);
  before = peak_mb ();
  gdk_clipboard_set_content (gtk_widget_get_clipboard (GTK_WIDGET (win)), other);
  wait_for_paint (win);
  paste_ms = timed (win, "paste");
  g_print ("photo %dx%d at 100 %%: paste of another app's PNG (%.1f MB) %.1f ms, of which the decode alone %.1f ms; peak memory %.0f MB "
           "(was %.0f MB)\n", size, size, g_bytes_get_size (png) / 1048576.0, paste_ms, decode_ms, peak_mb (), before);
  gtk_window_destroy (GTK_WINDOW (win));
}

/* The oversize prompt's answer (T011): an 8000 x 8000 picture of another app pasted onto a 2000 x 2000 picture, Fit and Resize, each timed
   from the answer to the painted frame (the read and decode happen at Paste, before the prompt, and are not in it). */
static void
measure_oversize (const char *config_dir)
{
  EaselWindow *win;
  EaselImage *pic = easel_image_new (8000, 8000, 0xFF336699u);
  g_autoptr (GBytes) png = easel_picture_encode (pic, EASEL_FORMAT_PNG, 90, NULL);
  g_autoptr (GdkContentProvider) other = gdk_content_provider_new_for_bytes ("image/png", png);
  double ms[2], before;
  static const char *answers[] = { "fit", "resize" };

  easel_image_free (pic);
  write_settings (config_dir, 2000, 2000);
  win = open_window ();
  g_action_group_activate_action (G_ACTION_GROUP (win), "zoom-normal", NULL);
  wait_for_paint (win);
  before = peak_mb ();
  for (int k = 0; k < 2; k++)
    {
      AdwDialog *dialog;
      gint64 start;

      gdk_clipboard_set_content (gtk_widget_get_clipboard (GTK_WIDGET (win)), other);
      wait_for_paint (win);
      g_action_group_activate_action (G_ACTION_GROUP (win), "paste", NULL);
      wait_for_paint (win);
      dialog = adw_application_window_get_visible_dialog (ADW_APPLICATION_WINDOW (win));
      if (dialog == NULL)
        {
          g_print ("oversize: no prompt\n");
          return;
        }
      start = g_get_monotonic_time ();
      g_signal_emit_by_name (dialog, "response", answers[k]);
      adw_dialog_close (dialog);
      wait_for_paint (win);
      ms[k] = (g_get_monotonic_time () - start) / 1000.0;
      g_action_group_activate_action (G_ACTION_GROUP (win), "undo", NULL);      /* the paste, floating: removed (and the growth) */
      wait_for_paint (win);
    }
  g_print ("oversize 8000x8000 onto 2000x2000 at 100 %%: Fit %.1f ms; Resize Canvas to Picture %.1f ms; peak memory %.0f MB (was %.0f MB)\n",
           ms[0], ms[1], peak_mb (), before);
  gtk_window_destroy (GTK_WINDOW (win));
}

static void
measure (const char *config_dir, int size, gboolean normal)
{
  EaselWindow *win;
  EaselImage *pic;
  GdkClipboard *clipboard;
  g_autoptr (GBytes) png = NULL;
  g_autoptr (GdkContentProvider) other = NULL;
  double copy_ms, cut_ms, own_ms, png_ms, encode_ms, before;
  gint64 start;

  write_settings (config_dir, size, size);
  win = open_window ();
  clipboard = gtk_widget_get_clipboard (GTK_WIDGET (win));
  g_action_group_activate_action (G_ACTION_GROUP (win), normal ? "zoom-normal" : "zoom-fit", NULL);
  wait_for_paint (win);
  before = peak_mb ();

  /* Copy, then Easel's own paste of it (put down by Esc-like Select All afterwards is not timed). */
  g_action_group_activate_action (G_ACTION_GROUP (win), "select-all", NULL);
  wait_for_paint (win);
  copy_ms = timed (win, "copy");
  own_ms = timed (win, "paste");
  g_action_group_activate_action (G_ACTION_GROUP (win), "undo", NULL);   /* the paste, floating: removed */
  wait_for_paint (win);

  /* The PNG another app's read makes: the encode alone, as the worker thread runs it. */
  pic = easel_image_new (size, size, 0xFF336699u);
  start = g_get_monotonic_time ();
  png = easel_picture_encode (pic, EASEL_FORMAT_PNG, 90, NULL);
  encode_ms = (g_get_monotonic_time () - start) / 1000.0;
  easel_image_free (pic);

  /* Another app's PNG on the clipboard, then a paste of it (read and decoded on the spot). */
  other = gdk_content_provider_new_for_bytes ("image/png", png);
  gdk_clipboard_set_content (clipboard, other);
  wait_for_paint (win);
  png_ms = timed (win, "paste");
  g_action_group_activate_action (G_ACTION_GROUP (win), "undo", NULL);
  wait_for_paint (win);

  /* Cut of the whole picture. */
  g_action_group_activate_action (G_ACTION_GROUP (win), "select-all", NULL);
  wait_for_paint (win);
  cut_ms = timed (win, "cut");

  g_print ("%dx%d at %s: copy %.1f ms; paste of its own copy %.1f ms; paste of another app's PNG %.1f ms; cut %.1f ms; "
           "PNG encode (worker thread) %.1f ms, %.1f MB; peak memory %.0f MB (was %.0f MB)\n", size, size,
           normal ? "100 %" : "the fit zoom", copy_ms, own_ms, png_ms, cut_ms, encode_ms, g_bytes_get_size (png) / 1048576.0, peak_mb (),
           before);
  gtk_window_destroy (GTK_WINDOW (win));
}

int
main (int argc, char **argv)
{
  g_autofree char *config_dir = g_dir_make_tmp ("easel-perf-XXXXXX", NULL);
  static const int sizes[] = { 2000, 8000 };

  g_setenv ("XDG_CONFIG_HOME", config_dir, TRUE);
  if (!gtk_init_check ())
    return 77;
  app = easel_application_new ();
  g_application_set_flags (G_APPLICATION (app), G_APPLICATION_NON_UNIQUE);
  g_application_register (G_APPLICATION (app), NULL, NULL);
  /* No argument: the fit zoom, both sizes (T009); "oversize": the prompt's Fit and Resize (T011); "zoom100": both sizes at 100 %; "photo": a photo-like PNG pasted at 100 %. */
  if (argc > 1 && strcmp (argv[1], "zoom100") == 0)
    for (guint i = 0; i < G_N_ELEMENTS (sizes); i++)
      measure (config_dir, sizes[i], TRUE);
  else if (argc > 1 && strcmp (argv[1], "oversize") == 0)
    measure_oversize (config_dir);
  else if (argc > 1 && strcmp (argv[1], "photo") == 0)
    for (guint i = 0; i < G_N_ELEMENTS (sizes); i++)
      measure_photo (config_dir, sizes[i]);
  else
    for (guint i = 0; i < G_N_ELEMENTS (sizes); i++)
      measure (config_dir, sizes[i], FALSE);
  g_object_unref (app);
  return 0;
}
