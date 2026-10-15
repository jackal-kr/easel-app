/* SPDX-License-Identifier: GPL-3.0-or-later */

#include <stdlib.h>
#include <string.h>

#include "savename.h"

static int
lower (int c)
{
  return (c >= 'A' && c <= 'Z') ? c - 'A' + 'a' : c;
}

EaselSavenameResult
easel_savename_fix (const char *path, char **fixed)
{
  const char *slash = strrchr (path, '/');
  const char *name = slash ? slash + 1 : path;
  const char *dot = strrchr (name, '.');
  size_t len = strlen (path);
  const char *suffix = "";

  *fixed = NULL;

  /* A leading dot marks a hidden file, and a trailing dot leaves no extension. */
  if (dot == NULL || dot == name)
    suffix = ".png";
  else if (dot[1] == '\0')
    suffix = "png";
  else if (strlen (dot) != 4 || lower (dot[1]) != 'p' || lower (dot[2]) != 'n' || lower (dot[3]) != 'g')
    return EASEL_SAVENAME_NOT_PNG;

  *fixed = malloc (len + strlen (suffix) + 1);
  if (*fixed == NULL)
    return EASEL_SAVENAME_NO_MEMORY;
  memcpy (*fixed, path, len);
  strcpy (*fixed + len, suffix);
  return EASEL_SAVENAME_OK;
}
