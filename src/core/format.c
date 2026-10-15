/* SPDX-License-Identifier: GPL-3.0-or-later */

#include <stdlib.h>
#include <string.h>

#include "format.h"

static int
lower (int c)
{
  return (c >= 'A' && c <= 'Z') ? c - 'A' + 'a' : c;
}

/* ext is what follows the dot. */
static bool
ext_is (const char *ext, const char *lowercase)
{
  size_t n = strlen (lowercase);

  if (strlen (ext) != n)
    return false;
  for (size_t i = 0; i < n; i++)
    if (lower (ext[i]) != lowercase[i])
      return false;
  return true;
}

static EaselFormat
format_of_extension (const char *ext)
{
  static const struct { const char *ext; EaselFormat format; } known[] = {
    { "png", EASEL_FORMAT_PNG }, { "jpg", EASEL_FORMAT_JPEG }, { "jpeg", EASEL_FORMAT_JPEG },
    { "bmp", EASEL_FORMAT_BMP }, { "tif", EASEL_FORMAT_TIFF }, { "tiff", EASEL_FORMAT_TIFF },
    { "webp", EASEL_FORMAT_WEBP },
  };

  for (size_t i = 0; i < sizeof known / sizeof known[0]; i++)
    if (ext_is (ext, known[i].ext))
      return known[i].format;
  return EASEL_FORMAT_UNKNOWN;
}

EaselFormatResult
easel_format_fix_name (const char *path, char **fixed, EaselFormat *format)
{
  const char *slash = strrchr (path, '/');
  const char *name = slash ? slash + 1 : path;
  const char *dot = strrchr (name, '.');
  size_t len = strlen (path);
  const char *suffix = "";

  *fixed = NULL;
  *format = EASEL_FORMAT_UNKNOWN;

  /* A leading dot marks a hidden file, and a trailing dot leaves no extension. */
  if (dot == NULL || dot == name)
    {
      suffix = ".png";
      *format = EASEL_FORMAT_PNG;
    }
  else if (dot[1] == '\0')
    {
      suffix = "png";
      *format = EASEL_FORMAT_PNG;
    }
  else
    {
      *format = format_of_extension (dot + 1);
      if (*format == EASEL_FORMAT_UNKNOWN)
        return EASEL_FORMAT_UNKNOWN_EXTENSION;
    }

  *fixed = malloc (len + strlen (suffix) + 1);
  if (*fixed == NULL)
    {
      *format = EASEL_FORMAT_UNKNOWN;
      return EASEL_FORMAT_NO_MEMORY;
    }
  memcpy (*fixed, path, len);
  strcpy (*fixed + len, suffix);
  return EASEL_FORMAT_OK;
}

bool
easel_format_is_flat (EaselFormat format)
{
  return format == EASEL_FORMAT_JPEG || format == EASEL_FORMAT_BMP;
}
