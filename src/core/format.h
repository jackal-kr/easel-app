/* SPDX-License-Identifier: GPL-3.0-or-later */
/* The picture formats the app saves, and the file name rule for Save (REQ-0001-28, REQ-0005-10). */
#pragma once

#include <stdbool.h>

typedef enum
{
  EASEL_FORMAT_UNKNOWN,
  EASEL_FORMAT_PNG,
  EASEL_FORMAT_JPEG,
  EASEL_FORMAT_BMP,
  EASEL_FORMAT_TIFF,
  EASEL_FORMAT_WEBP,
} EaselFormat;

typedef enum
{
  EASEL_FORMAT_OK,
  EASEL_FORMAT_UNKNOWN_EXTENSION,   /* the name has an extension that is not saveable (GIF can only be opened) */
  EASEL_FORMAT_NO_MEMORY,
} EaselFormatResult;

/* The extension is what follows the last dot of the file name, unless that dot starts the name; it decides the format,
   in any case: png, jpg, jpeg, bmp, tif, tiff, webp. A name without an extension, or ending in a dot, gets ".png".
   On OK, *fixed is the name to save to (free with free ()) and *format its format. Otherwise *fixed is NULL and *format
   is EASEL_FORMAT_UNKNOWN. */
EaselFormatResult easel_format_fix_name (const char *path, char **fixed, EaselFormat *format);

/* JPEG and BMP cannot keep transparency: they are saved flattened on white (REQ-0005-11). */
bool easel_format_is_flat (EaselFormat format);
