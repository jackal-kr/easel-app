/* SPDX-License-Identifier: GPL-3.0-or-later */

#include <errno.h>
#include <fcntl.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <glib/gstdio.h>
#include <string.h>
#include <unistd.h>

#include "picture-file.h"

static char *hidden_format;

void
easel_picture_file_hide_format (const char *name)
{
  g_free (hidden_format);
  hidden_format = g_strdup (name);
}

/* ---- the header sniffer (Plan, "How formats are detected") ---- */

static guint
be16 (const guint8 *p)
{
  return (guint) p[0] << 8 | p[1];
}

static guint32
be32 (const guint8 *p)
{
  return (guint32) p[0] << 24 | (guint32) p[1] << 16 | (guint32) p[2] << 8 | p[3];
}

static guint
le16 (const guint8 *p)
{
  return (guint) p[1] << 8 | p[0];
}

static guint32
le32 (const guint8 *p)
{
  return (guint32) p[3] << 24 | (guint32) p[2] << 16 | (guint32) p[1] << 8 | p[0];
}

/* A size from a header, which can be up to 2^32; anything above int stays "very big" and positive. */
static int
size_of (guint32 v)
{
  return v > (guint32) G_MAXINT ? G_MAXINT : (int) v;
}

static gboolean
read_at (FILE *f, long offset, guint8 *buf, size_t n)
{
  return fseek (f, offset, SEEK_SET) == 0 && fread (buf, 1, n, f) == n;
}

static gboolean
sniff_png (const guint8 *b, int *w, int *h)
{
  static const guint8 signature[8] = { 0x89, 'P', 'N', 'G', '\r', '\n', 0x1A, '\n' };

  if (memcmp (b, signature, 8) != 0 || be32 (b + 8) != 13 || memcmp (b + 12, "IHDR", 4) != 0)
    return FALSE;
  *w = size_of (be32 (b + 16));
  *h = size_of (be32 (b + 20));
  return TRUE;
}

static gboolean
sniff_gif (const guint8 *b, int *w, int *h)
{
  if (memcmp (b, "GIF87a", 6) != 0 && memcmp (b, "GIF89a", 6) != 0)
    return FALSE;
  *w = (int) le16 (b + 6);
  *h = (int) le16 (b + 8);
  return TRUE;
}

/* The 12-byte OS/2 header has 16-bit sizes; the others (40 bytes and longer) have signed 32-bit ones, and a negative
   height only means the rows run top to bottom. */
static gboolean
sniff_bmp (const guint8 *b, int *w, int *h)
{
  guint32 dib;

  if (b[0] != 'B' || b[1] != 'M')
    return FALSE;
  dib = le32 (b + 14);
  if (dib == 12)
    {
      *w = (int) le16 (b + 18);
      *h = (int) le16 (b + 20);
      return TRUE;
    }
  if (dib < 40)
    return FALSE;
  {
    gint32 sw = (gint32) le32 (b + 18), sh = (gint32) le32 (b + 22);

    *w = sw < 0 ? G_MAXINT : sw;
    *h = sh == G_MININT32 ? G_MAXINT : (sh < 0 ? -sh : sh);
  }
  return TRUE;
}

/* The segments up to the first start-of-frame marker; the size is there. A start of scan or the end first means a file
   with no size, and so no picture. */
static gboolean
sniff_jpeg (FILE *f, int *w, int *h)
{
  guint8 b[8];
  long offset = 2;

  for (int segments = 0; segments < 4096; segments++)
    {
      int marker;

      if (!read_at (f, offset, b, 2) || b[0] != 0xFF)
        return FALSE;
      /* Any number of 0xFF may pad before the marker byte. */
      marker = b[1];
      offset += 2;
      while (marker == 0xFF)
        {
          if (!read_at (f, offset, b, 1))
            return FALSE;
          marker = b[0];
          offset++;
        }
      if (marker == 0x01 || (marker >= 0xD0 && marker <= 0xD8))
        continue;                       /* markers without a length */
      if (marker == 0xD9 || marker == 0xDA)
        return FALSE;
      if (!read_at (f, offset, b, 2))
        return FALSE;
      if (be16 (b) < 2)
        return FALSE;
      /* Start of frame: C0 to CF without the table (C4), JPG (C8) and arithmetic-table (CC) markers. */
      if (marker >= 0xC0 && marker <= 0xCF && marker != 0xC4 && marker != 0xC8 && marker != 0xCC)
        {
          if (!read_at (f, offset + 2, b, 5))
            return FALSE;
          *h = (int) be16 (b + 1);
          *w = (int) be16 (b + 3);
          return TRUE;
        }
      offset += (long) be16 (b);
    }
  return FALSE;
}

/* The first directory's width and height entries (tags 256 and 257, as SHORT or LONG). */
static gboolean
sniff_tiff (FILE *f, const guint8 *b, int *w, int *h)
{
  gboolean big = b[0] == 'M';
  guint (*u16) (const guint8 *) = big ? be16 : le16;
  guint32 (*u32) (const guint8 *) = big ? be32 : le32;
  guint8 head[2], entry[12];
  guint32 ifd;
  guint count;
  int found_w = 0, found_h = 0;

  if (!((b[0] == 'I' && b[1] == 'I') || (b[0] == 'M' && b[1] == 'M')) || u16 (b + 2) != 42)
    return FALSE;
  ifd = u32 (b + 4);
  if (ifd > (guint32) G_MAXLONG - 4096 * 12 || !read_at (f, (long) ifd, head, 2))
    return FALSE;
  count = u16 (head);
  if (count > 4096)
    return FALSE;
  for (guint i = 0; i < count; i++)
    {
      guint tag, type;

      if (fread (entry, 1, 12, f) != 12)
        return FALSE;
      tag = u16 (entry);
      type = u16 (entry + 2);
      if ((tag == 256 || tag == 257) && (type == 3 || type == 4))
        {
          int v = type == 3 ? (int) u16 (entry + 8) : size_of (u32 (entry + 8));

          if (tag == 256)
            found_w = v;
          else
            found_h = v;
        }
    }
  *w = found_w;
  *h = found_h;
  return TRUE;
}

/* RIFF, WEBP, then the first chunk: lossy (VP8), lossless (VP8L) or extended (VP8X). */
static gboolean
sniff_webp (const guint8 *b, int *w, int *h)
{
  if (memcmp (b, "RIFF", 4) != 0 || memcmp (b + 8, "WEBP", 4) != 0)
    return FALSE;
  if (memcmp (b + 12, "VP8 ", 4) == 0)
    {
      if (b[23] != 0x9D || b[24] != 0x01 || b[25] != 0x2A)
        return FALSE;
      *w = (int) (le16 (b + 26) & 0x3FFF);
      *h = (int) (le16 (b + 28) & 0x3FFF);
      return TRUE;
    }
  if (memcmp (b + 12, "VP8L", 4) == 0)
    {
      guint32 bits = le32 (b + 21);

      if (b[20] != 0x2F)
        return FALSE;
      *w = (int) (bits & 0x3FFF) + 1;
      *h = (int) ((bits >> 14) & 0x3FFF) + 1;
      return TRUE;
    }
  if (memcmp (b + 12, "VP8X", 4) == 0)
    {
      *w = (int) (b[24] | b[25] << 8 | b[26] << 16) + 1;
      *h = (int) (b[27] | b[28] << 8 | b[29] << 16) + 1;
      return TRUE;
    }
  return FALSE;
}

static gboolean sniff_stream (FILE *f, const char **format, int *width, int *height);

gboolean
easel_picture_sniff (const char *path, const char **format, int *width, int *height)
{
  FILE *f;

  if (!g_file_test (path, G_FILE_TEST_IS_REGULAR) || (f = g_fopen (path, "rb")) == NULL)
    return FALSE;
  return sniff_stream (f, format, width, height);
}

/* The sniffer over an open stream, which it closes: a file's, or bytes' through fmemopen. */
static gboolean
sniff_stream (FILE *f, const char **format, int *width, int *height)
{
  guint8 b[32];
  size_t got;
  const char *name = NULL;
  int w = 0, h = 0;
  gboolean ok = FALSE;

  memset (b, 0, sizeof b);
  got = fread (b, 1, sizeof b, f);
  if (got >= 24 && sniff_png (b, &w, &h))
    name = "png";
  else if (got >= 10 && sniff_gif (b, &w, &h))
    name = "gif";
  else if (got >= 26 && sniff_bmp (b, &w, &h))
    name = "bmp";
  else if (got >= 30 && sniff_webp (b, &w, &h))
    name = "webp";
  else if (got >= 8 && sniff_tiff (f, b, &w, &h))
    name = "tiff";
  else if (got >= 4 && b[0] == 0xFF && b[1] == 0xD8 && sniff_jpeg (f, &w, &h))
    name = "jpeg";
  fclose (f);

  /* A picture has a size. */
  if (name != NULL && w > 0 && h > 0 && (hidden_format == NULL || strcmp (name, hidden_format) != 0))
    {
      *format = name;
      *width = w;
      *height = h;
      ok = TRUE;
    }
  return ok;
}

/* True if that many buffers of w x h RGBA pixels can be had at once. They are let go again at once: memory that is not
   touched costs nothing, and the point is to find out here, with an error to give, and not in an allocation that aborts
   the process. */
static gboolean
memory_for (int w, int h, int buffers)
{
  gsize bytes = (gsize) w * (gsize) h * 4;
  void *held[3] = { NULL, NULL, NULL };
  gboolean ok = TRUE;

  g_assert (buffers <= 3);
  for (int i = 0; i < buffers && ok; i++)
    {
      held[i] = g_try_malloc (bytes);
      ok = held[i] != NULL;
    }
  for (int i = 0; i < buffers; i++)
    g_free (held[i]);
  return ok;
}

/* A file the app can read from: a regular file it may open. */
static gboolean
is_readable_file (const char *path)
{
  int fd;

  if (!g_file_test (path, G_FILE_TEST_IS_REGULAR))
    return FALSE;
  fd = g_open (path, O_RDONLY, 0);
  if (fd < 0)
    return FALSE;
  close (fd);
  return TRUE;
}

static EaselOpenResult from_pixbuf (GdkPixbuf *pixbuf, EaselImage **img, int *width, int *height);

EaselOpenResult
easel_picture_open (const char *path, EaselImage **img, int *width, int *height)
{
  g_autoptr (GdkPixbuf) pixbuf = NULL;
  g_autoptr (GError) error = NULL;
  const char *format;
  int w = 0, h = 0;

  *img = NULL;
  *width = *height = 0;
  if (!is_readable_file (path))
    return EASEL_OPEN_UNREADABLE;

  /* The first bytes say what the file is and how big, so a wrong or oversized file is refused before any pixel is
     decoded (REQ-0005-06, REQ-0005-17). Turning a picture by 90 degrees swaps its sides, so the larger side decides
     either way. */
  if (!easel_picture_sniff (path, &format, &w, &h))
    return EASEL_OPEN_NOT_PICTURE;
  if (w > EASEL_IMAGE_MAX_SIZE || h > EASEL_IMAGE_MAX_SIZE)
    {
      *width = w;
      *height = h;
      return EASEL_OPEN_TOO_BIG;
    }

  /* gdk-pixbuf decodes with glycin, a Rust library that aborts the whole process when an allocation fails, so a lack of
     memory must be found here, before it is asked for: reserve what the decoder's pixels and the picture need at most
     (4 bytes per pixel each), and let both go again (untouched memory costs nothing). */
  if (!memory_for (w, h, 2))
    {
      *width = w;
      *height = h;
      return EASEL_OPEN_NO_MEMORY;
    }

  pixbuf = gdk_pixbuf_new_from_file (path, &error);
  if (pixbuf == NULL)
    {
      *width = w;
      *height = h;
      if (g_error_matches (error, GDK_PIXBUF_ERROR, GDK_PIXBUF_ERROR_INSUFFICIENT_MEMORY))
        return EASEL_OPEN_NO_MEMORY;
      *width = *height = 0;
      return EASEL_OPEN_NOT_PICTURE;
    }

  return from_pixbuf (g_steal_pointer (&pixbuf), img, width, height);
}

/* One pass into the picture, upright, as 8-bit RGBA; then the decoder's pixels are freed at once, before the caller does anything
   else (Plan, "Memory when opening"). Takes pixbuf. */
static EaselOpenResult
from_pixbuf (GdkPixbuf *pixbuf, EaselImage **img, int *width, int *height)
{
  const char *orientation = gdk_pixbuf_get_option (pixbuf, "orientation");

  *img = easel_image_from_bytes (gdk_pixbuf_read_pixels (pixbuf), gdk_pixbuf_get_width (pixbuf),
                                 gdk_pixbuf_get_height (pixbuf), gdk_pixbuf_get_rowstride (pixbuf),
                                 gdk_pixbuf_get_n_channels (pixbuf), orientation ? atoi (orientation) : 1);
  if (*img == NULL)
    {
      *width = gdk_pixbuf_get_width (pixbuf);
      *height = gdk_pixbuf_get_height (pixbuf);
      g_object_unref (pixbuf);
      return EASEL_OPEN_NO_MEMORY;
    }
  g_object_unref (pixbuf);
  *width = (*img)->width;
  *height = (*img)->height;
  return EASEL_OPEN_OK;
}

EaselOpenResult
easel_picture_open_bytes (GBytes *bytes, EaselImage **img, int *width, int *height)
{
  g_autoptr (GInputStream) stream = NULL;
  g_autoptr (GError) error = NULL;
  GdkPixbuf *pixbuf;
  const char *format;
  gsize len = 0;
  const void *data = g_bytes_get_data (bytes, &len);
  FILE *f;
  int w = 0, h = 0;

  *img = NULL;
  *width = *height = 0;
  /* The same checks as a file's, in the same order: what it is and how big from its first bytes, then the memory (REQ-0016-10). */
  if (len == 0 || (f = fmemopen ((void *) data, len, "rb")) == NULL || !sniff_stream (f, &format, &w, &h))
    return EASEL_OPEN_NOT_PICTURE;
  if (w > EASEL_IMAGE_MAX_SIZE || h > EASEL_IMAGE_MAX_SIZE || !memory_for (w, h, 2))
    {
      *width = w;
      *height = h;
      return w > EASEL_IMAGE_MAX_SIZE || h > EASEL_IMAGE_MAX_SIZE ? EASEL_OPEN_TOO_BIG : EASEL_OPEN_NO_MEMORY;
    }
  stream = g_memory_input_stream_new_from_bytes (bytes);
  pixbuf = gdk_pixbuf_new_from_stream (stream, NULL, &error);
  if (pixbuf == NULL)
    {
      if (g_error_matches (error, GDK_PIXBUF_ERROR, GDK_PIXBUF_ERROR_INSUFFICIENT_MEMORY))
        {
          *width = w;
          *height = h;
          return EASEL_OPEN_NO_MEMORY;
        }
      return EASEL_OPEN_NOT_PICTURE;
    }
  return from_pixbuf (pixbuf, img, width, height);
}

/* ---- saving ---- */

/* gdk-pixbuf's RGBA is straight alpha, like the picture (ADR-0002), so this is a byte reorder only; formats without
   transparency get RGB, flattened on white by the one formula of the core. */
static GdkPixbuf *
to_pixbuf (const EaselImage *img, gboolean flat)
{
  GdkPixbuf *pb = gdk_pixbuf_new (GDK_COLORSPACE_RGB, !flat, 8, img->width, img->height);
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

      if (flat)
        {
          easel_flatten_on_white (src, row, img->width);
          continue;
        }
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

static const char *
format_name (EaselFormat format)
{
  switch (format)
    {
    case EASEL_FORMAT_PNG: return "png";
    case EASEL_FORMAT_JPEG: return "jpeg";
    case EASEL_FORMAT_BMP: return "bmp";
    case EASEL_FORMAT_TIFF: return "tiff";
    case EASEL_FORMAT_WEBP: return "webp";
    case EASEL_FORMAT_UNKNOWN: break;
    }
  return "";
}

gboolean
easel_picture_save (const EaselImage *img, const char *path, EaselFormat format, int jpeg_quality, GError **error)
{
  g_autoptr (GBytes) bytes = easel_picture_encode (img, format, jpeg_quality, error);
  gsize size = 0;
  const char *buffer;

  if (bytes == NULL)
    return FALSE;
  buffer = g_bytes_get_data (bytes, &size);
  /* Written to a temporary file and renamed over the target, so a failure never leaves it half-written, in every
     format. GIO's g_file_replace_contents() is not used: when it cannot create the temporary file it rewrites the
     target in place. */
  return g_file_set_contents_full (path, buffer, (gssize) size, G_FILE_SET_CONTENTS_CONSISTENT, 0666, error);
}

/* The encoding itself; png_fast asks the PNG encoder for its fastest level (1). */
static GBytes *
encode_with (const EaselImage *img, EaselFormat format, int jpeg_quality, gboolean png_fast, GError **error)
{
  g_autoptr (GdkPixbuf) pb = NULL;
  g_autofree char *buffer = NULL;
  g_autofree char *quality = g_strdup_printf ("%d", jpeg_quality);
  const char *name = format_name (format);
  gboolean encoded;
  gsize size;

  /* The encoders abort the process when an allocation fails, so the memory they need is looked for first (REQ-0005-18):
     the pixbuf, the encoder's own copy and its output, 4 bytes per pixel each at most. Best effort. */
  if (memory_for (img->width, img->height, 3))
    pb = to_pixbuf (img, easel_format_is_flat (format));
  if (pb == NULL)
    {
      g_set_error_literal (error, G_IO_ERROR, G_IO_ERROR_NO_SPACE, g_strerror (ENOMEM));
      return NULL;
    }
  /* A format taken out by the test hook is handed to gdk-pixbuf under a name it does not know, so the failure and its
     message are gdk-pixbuf's own (Plan, "A missing loader or saver"). */
  if (hidden_format != NULL && strcmp (name, hidden_format) == 0)
    name = "not-installed";

  if (format == EASEL_FORMAT_JPEG)
    encoded = gdk_pixbuf_save_to_buffer (pb, &buffer, &size, name, error, "quality", quality, NULL);
  else if (format == EASEL_FORMAT_WEBP)
    encoded = gdk_pixbuf_save_to_buffer (pb, &buffer, &size, name, error, "lossless", "TRUE", NULL);
  else if (format == EASEL_FORMAT_PNG && png_fast)
    encoded = gdk_pixbuf_save_to_buffer (pb, &buffer, &size, name, error, "compression", "1", NULL);
  else
    encoded = gdk_pixbuf_save_to_buffer (pb, &buffer, &size, name, error, NULL);
  if (!encoded)
    return NULL;
  return g_bytes_new_take (g_steal_pointer (&buffer), size);
}

GBytes *
easel_picture_encode (const EaselImage *img, EaselFormat format, int jpeg_quality, GError **error)
{
  return encode_with (img, format, jpeg_quality, FALSE, error);
}

GBytes *
easel_picture_encode_fast_png (const EaselImage *img, GError **error)
{
  return encode_with (img, EASEL_FORMAT_PNG, 90, TRUE, error);
}
