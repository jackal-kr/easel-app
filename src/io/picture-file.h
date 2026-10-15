/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Opening and saving pictures in the formats of feature 0005, through gdk-pixbuf. Never needs a display. */
#pragma once

#include <glib.h>

#include "format.h"
#include "image.h"

G_BEGIN_DECLS

typedef enum
{
  EASEL_OPEN_OK,
  EASEL_OPEN_UNREADABLE,     /* missing, no permission, a folder, vanished meanwhile */
  EASEL_OPEN_NOT_PICTURE,    /* other content, empty, truncated, or a format the app does not open */
  EASEL_OPEN_TOO_BIG,        /* more than EASEL_IMAGE_MAX_SIZE in width or height (REQ-0005-17) */
  EASEL_OPEN_NO_MEMORY
} EaselOpenResult;

/* Opens a picture by its content, not its name (REQ-0005-06): PNG, JPEG, BMP, TIFF, WebP or GIF, as 8-bit RGBA, upright
   as its orientation data says, an animation as its first frame. On OK, *img is the new picture (free it with
   easel_image_free) and *width and *height its size. On TOO_BIG and NO_MEMORY, *width and *height are the size the file
   states (as stored, before its orientation is applied), for the message of REQ-0005-09; on the other failures they
   are 0. The size is checked from the file's
   header, before any pixel is decoded. */
EaselOpenResult easel_picture_open (const char *path, EaselImage **img, int *width, int *height);

/* Reads only the first bytes of the file and answers its format ("png", "jpeg", "bmp", "tiff", "webp" or "gif", the
   gdk-pixbuf names) and the size it states, as stored, without decoding anything (Plan, "How formats are detected").
   FALSE if the file is empty or cut off before its size, has no size marker, is of another kind, or cannot be read; the
   outputs are then untouched. Only for honest files: a header that lies is the decoder's to find out. *format is a
   static string. */
gboolean easel_picture_sniff (const char *path, const char **format, int *width, int *height);

/* Writes img to path in the given format (REQ-0005-10, REQ-0005-11): PNG, TIFF and WebP (lossless) keep transparency,
   JPEG (at jpeg_quality, 1 to 100) and BMP are flattened on white. The file is replaced atomically: on failure an
   existing file keeps its old content, and error holds the reason (the system's, or gdk-pixbuf's when the format is not
   installed). */
gboolean easel_picture_save (const EaselImage *img, const char *path, EaselFormat format, int jpeg_quality,
                             GError **error);

/* Test hook (REQ-0005-06, REQ-0005-10): treats the gdk-pixbuf format of that name ("webp", "tiff", ...) as not installed,
   for opening and for saving, until it is called again with NULL. */
void easel_picture_file_hide_format (const char *name);

G_END_DECLS
