/* SPDX-License-Identifier: GPL-3.0-or-later */

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <errno.h>

#include "png.h"

/* gdk-pixbuf's RGBA is straight alpha, like the picture (ADR-0002), so this is a byte reorder only. */
static GdkPixbuf *
to_pixbuf (const EaselImage *img)
{
  GdkPixbuf *pb = gdk_pixbuf_new (GDK_COLORSPACE_RGB, TRUE, 8, img->width, img->height);
  guint8 *pixels;
  int stride;

  if (pb == NULL)
    return NULL;
  pixels = gdk_pixbuf_get_pixels (pb);
  stride = gdk_pixbuf_get_rowstride (pb);
  for (int y = 0; y < img->height; y++)
    {
      guint8 *row = pixels + (gsize) y * (gsize) stride;
      const uint32_t *src = img->px + (gsize) y * (gsize) img->width;

      for (int x = 0; x < img->width; x++)
        {
          uint32_t c = src[x];

          row[4 * x + 0] = (guint8) (c >> 16);
          row[4 * x + 1] = (guint8) (c >> 8);
          row[4 * x + 2] = (guint8) c;
          row[4 * x + 3] = (guint8) (c >> 24);
        }
    }
  return pb;
}

gboolean
easel_png_save (const EaselImage *img, const char *path, GError **error)
{
  g_autoptr (GdkPixbuf) pb = to_pixbuf (img);
  g_autofree char *buffer = NULL;
  gsize size;

  if (pb == NULL)
    {
      g_set_error_literal (error, G_IO_ERROR, G_IO_ERROR_NO_SPACE, g_strerror (ENOMEM));
      return FALSE;
    }
  if (!gdk_pixbuf_save_to_buffer (pb, &buffer, &size, "png", error, NULL))
    return FALSE;

  /* Written to a temporary file and renamed over the target, so a failure never leaves it half-written.
     GIO's g_file_replace_contents() is not used: when it cannot create the temporary file it rewrites the
     target in place. */
  return g_file_set_contents_full (path, buffer, (gssize) size, G_FILE_SET_CONTENTS_CONSISTENT, 0666, error);
}
