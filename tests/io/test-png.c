/* SPDX-License-Identifier: GPL-3.0-or-later */
/* PNG save scenarios of feature 0001. Headless: GLib and gdk-pixbuf only. */

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <glib/gstdio.h>
#include <unistd.h>

#include "picture-file.h"

/* Decodes path back into straight-alpha 0xAARRGGBB, as the saved file holds it. */
static guint32
decoded_pixel (GdkPixbuf *pb, int x, int y)
{
  const guint8 *p = gdk_pixbuf_read_pixels (pb) + y * gdk_pixbuf_get_rowstride (pb) + x * gdk_pixbuf_get_n_channels (pb);
  guint32 a = gdk_pixbuf_get_has_alpha (pb) ? p[3] : 0xFF;

  return (a << 24) | ((guint32) p[0] << 16) | ((guint32) p[1] << 8) | p[2];
}

static void
assert_file_equals_image (const char *path, const EaselImage *img)
{
  g_autoptr (GError) error = NULL;
  g_autoptr (GdkPixbuf) pb = gdk_pixbuf_new_from_file (path, &error);

  g_assert_no_error (error);
  g_assert_cmpint (gdk_pixbuf_get_width (pb), ==, img->width);
  g_assert_cmpint (gdk_pixbuf_get_height (pb), ==, img->height);
  for (int y = 0; y < img->height; y++)
    for (int x = 0; x < img->width; x++)
      g_assert_cmphex (decoded_pixel (pb, x, y), ==, easel_image_get (img, x, y));
}

static char *
temp_path (const char *name)
{
  g_autofree char *dir = g_dir_make_tmp ("easel-png-XXXXXX", NULL);

  return g_build_filename (dir, name, NULL);
}

static void
as24_png_round_trip (void)
{
  g_autofree char *path = temp_path ("a.png");
  g_autoptr (GError) error = NULL;
  EaselImage *img = easel_image_new (37, 21, 0xFFFFFFFFu);

  /* Every channel distinct, so a swapped byte order shows. */
  easel_image_set (img, 0, 0, 0xFF000000u);
  easel_image_set (img, 36, 20, 0xFF123456u);
  easel_image_set (img, 5, 7, 0x80FF8040u);
  easel_image_set (img, 6, 7, 0x01020304u);

  g_assert_true (easel_picture_save (img, path, EASEL_FORMAT_PNG, 90, &error));
  g_assert_no_error (error);
  assert_file_equals_image (path, img);
  easel_image_free (img);
}

static void
as27_transparent_round_trip (void)
{
  g_autofree char *path = temp_path ("t.png");
  g_autoptr (GError) error = NULL;
  EaselImage *img = easel_image_new (10, 10, 0x00000000u);

  easel_image_set (img, 4, 4, 0xFF000000u);
  g_assert_true (easel_picture_save (img, path, EASEL_FORMAT_PNG, 90, &error));
  assert_file_equals_image (path, img);
  easel_image_free (img);
}

static void
as29_failed_save_keeps_old_file (void)
{
  g_autofree char *path = temp_path ("a.png");
  g_autofree char *dir = g_path_get_dirname (path);
  g_autofree char *after = NULL;
  g_autoptr (GError) error = NULL;
  EaselImage *img;

  if (geteuid () == 0)
    {
      g_test_skip ("root ignores folder permissions");
      return;
    }
  g_assert_true (g_file_set_contents (path, "old content", -1, NULL));
  g_assert_cmpint (g_chmod (dir, 0500), ==, 0);

  img = easel_image_new (4, 4, 0xFF000000u);
  g_assert_false (easel_picture_save (img, path, EASEL_FORMAT_PNG, 90, &error));
  g_assert_nonnull (error);
  g_assert_nonnull (error->message);
  easel_image_free (img);

  g_assert_true (g_file_get_contents (path, &after, NULL, NULL));
  g_assert_cmpstr (after, ==, "old content");
  g_chmod (dir, 0700);
}

int
main (int argc, char **argv)
{
  g_test_init (&argc, &argv, NULL);
  g_test_add_func ("/0001/as24_png_round_trip", as24_png_round_trip);
  g_test_add_func ("/0001/as27_transparent_round_trip", as27_transparent_round_trip);
  g_test_add_func ("/0001/as29_failed_save_keeps_old_file", as29_failed_save_keeps_old_file);
  return g_test_run ();
}
