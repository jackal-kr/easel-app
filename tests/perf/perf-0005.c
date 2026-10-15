/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Opening a picture (feature 0005, the Plan's budgets): from the request to the first painted frame, split into decoding
   and converting, with the peak memory next to each time. The files are made here, photo-like (a gradient with noise so
   that PNG cannot squeeze them to nothing). Run with meson test --benchmark; needs a display, and the window must stay
   visible while it runs. Prints numbers; it does not fail on a missed budget, because the budgets are judged by JOD.
   The window's own Open action (T009) is not built yet: this measures what it will do, the same calls in the same order,
   on the window's canvas. */

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <glib/gstdio.h>
#include <stdio.h>
#include <string.h>

#include "perf-common.h"
#include "picture-file.h"

/* The process's peak resident memory in MB (VmHWM), and a way to start measuring again from what is resident now. */
static double
peak_mb (void)
{
  g_autofree char *status = NULL;
  const char *p;

  if (!g_file_get_contents ("/proc/self/status", &status, NULL, NULL) || (p = strstr (status, "VmHWM:")) == NULL)
    return -1;
  return atof (p + 6) / 1024.0;
}

static void
reset_peak (void)
{
  FILE *f = fopen ("/proc/self/clear_refs", "w");

  if (f != NULL)
    {
      fputs ("5", f);
      fclose (f);
    }
}

/* A picture of a smooth gradient with noise, saved as PNG; returns the path. */
static char *
make_file (const char *dir, int w, int h)
{
  g_autoptr (GError) error = NULL;
  EaselImage *img = easel_image_new (w, h, 0xFF000000u);
  char *path = g_strdup_printf ("%s/photo-%dx%d.png", dir, w, h);
  guint32 seed = 12345;

  for (int y = 0; y < h; y++)
    for (int x = 0; x < w; x++)
      {
        seed = seed * 1664525u + 1013904223u;
        img->px[(gsize) y * (gsize) w + (gsize) x] = 0xFF000000u
          | (uint32_t) (((x * 255 / w) + ((seed >> 24) & 15)) & 255) << 16
          | (uint32_t) (((y * 255 / h) + ((seed >> 20) & 15)) & 255) << 8
          | (uint32_t) ((((x + y) * 255 / (w + h)) + ((seed >> 16) & 15)) & 255);
      }
  if (!easel_picture_save (img, path, EASEL_FORMAT_PNG, 90, &error))
    g_error ("cannot write %s: %s", path, error->message);
  easel_image_free (img);
  return path;
}

static double
median (double *v, int n)
{
  qsort (v, (size_t) n, sizeof v[0], cmp_double);
  return v[n / 2];
}

/* One size: the decode and the conversion alone, then the whole path on the canvas, alone and on top of a picture of the
   same size (the old one is freed once the canvas shows the new one). */
static void
measure (EaselWindow *win, const char *dir, int w, int h)
{
  enum { RUNS = 3 };
  g_autofree char *path = make_file (dir, w, h);
  EaselCanvas *canvas = easel_window_get_canvas (win);
  double decode_ms[RUNS], convert_ms[RUNS], total_ms[RUNS], frame_ms[RUNS], peak_alone[RUNS], peak_top[RUNS];
  double px_mb = (double) w * h * 4 / (1024.0 * 1024.0);
  g_autofree char *size = g_strdup_printf ("%dx%d", w, h);
  g_autoptr (GError) error = NULL;

  for (int i = 0; i < RUNS; i++)
    {
      /* The two steps by themselves. */
      gint64 t = g_get_monotonic_time ();
      GdkPixbuf *pb = gdk_pixbuf_new_from_file (path, &error);
      EaselImage *img;

      g_assert_nonnull (pb);
      decode_ms[i] = (g_get_monotonic_time () - t) / 1000.0;
      t = g_get_monotonic_time ();
      img = easel_image_from_bytes (gdk_pixbuf_read_pixels (pb), gdk_pixbuf_get_width (pb), gdk_pixbuf_get_height (pb),
                                    gdk_pixbuf_get_rowstride (pb), gdk_pixbuf_get_n_channels (pb), 1);
      convert_ms[i] = (g_get_monotonic_time () - t) / 1000.0;
      g_object_unref (pb);
      easel_image_free (img);

      /* Alone: the request to the first painted frame, with nothing else big in memory. */
      easel_canvas_set_image (canvas, NULL);
      reset_peak ();
      t = g_get_monotonic_time ();
      {
        EaselImage *opened = NULL;
        int ow, oh;

        g_assert_cmpint (easel_picture_open (path, &opened, &ow, &oh), ==, EASEL_OPEN_OK);
        total_ms[i] = (g_get_monotonic_time () - t) / 1000.0;
        easel_canvas_set_image (canvas, opened);
        wait_for_paint (win);
        frame_ms[i] = (g_get_monotonic_time () - t) / 1000.0;
        peak_alone[i] = peak_mb ();
        easel_canvas_set_image (canvas, NULL);
        easel_image_free (opened);
      }

      /* On top: an old picture of the same size is alive until the canvas shows the new one. */
      {
        EaselImage *old = easel_image_new (w, h, 0xFF336699u), *opened = NULL;
        int ow, oh;

        easel_canvas_set_image (canvas, old);
        wait_for_paint (win);
        reset_peak ();
        g_assert_cmpint (easel_picture_open (path, &opened, &ow, &oh), ==, EASEL_OPEN_OK);
        easel_canvas_set_image (canvas, opened);
        easel_image_free (old);
        wait_for_paint (win);
        peak_top[i] = peak_mb ();
        easel_canvas_set_image (canvas, NULL);
        easel_image_free (opened);
      }
    }
  g_print ("open %s PNG (%.0f MB as pixels):\n", size, px_mb);
  g_print ("  decode %.0f ms, convert %.0f ms (medians of %d)\n", median (decode_ms, RUNS), median (convert_ms, RUNS), RUNS);
  g_print ("  request to the picture in memory %.0f ms, to the first painted frame %.0f ms\n", median (total_ms, RUNS),
           median (frame_ms, RUNS));
  g_print ("  peak memory alone %.0f MB (expected about %.0f: decoder + picture, 2 copies), on top of a same-size "
           "picture %.0f MB (expected about %.0f: 3 copies)\n",
           median (peak_alone, RUNS), 2 * px_mb, median (peak_top, RUNS), 3 * px_mb);
  g_unlink (path);
}

int
main (int argc, char **argv)
{
  g_autofree char *config_dir = g_dir_make_tmp ("easel-perf-XXXXXX", NULL);
  EaselWindow *win;

  g_setenv ("XDG_CONFIG_HOME", config_dir, TRUE);
  if (!gtk_init_check ())
    return 77;
  app = easel_application_new ();
  g_application_set_flags (G_APPLICATION (app), G_APPLICATION_NON_UNIQUE);
  g_application_register (G_APPLICATION (app), NULL, NULL);

  write_settings (config_dir, 640, 480);
  win = open_window ();
  g_print ("renderer %s\n", G_OBJECT_TYPE_NAME (gtk_native_get_renderer (GTK_NATIVE (win))));
  measure (win, config_dir, 1920, 1080);
  measure (win, config_dir, 8000, 8000);
  gtk_window_destroy (GTK_WINDOW (win));
  g_object_unref (app);
  return 0;
}
