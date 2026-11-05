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

/* The format a file name's extension names, without changing the name: EASEL_FORMAT_UNKNOWN for no extension (as for a
   name ending in a dot, or only a leading dot) and for an extension that cannot be saved. Used for Save, which writes an
   existing file in the format it has (REQ-0005-13). */
EaselFormat easel_format_of_name (const char *path);

/* The name a format chosen in the save dialog gives (REQ-0024-12): no extension gets the format's first one; an extension of another
   format that can be saved is replaced; one of this format's stays; any other last part stays and the extension is added. On true
   *named is the name (free with free ()); false: no memory. */
bool easel_format_name_for (const char *path, EaselFormat format, char **named);

/* JPEG and BMP cannot keep transparency: they are saved flattened on white (REQ-0005-11). */
bool easel_format_is_flat (EaselFormat format);
