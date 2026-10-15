/* SPDX-License-Identifier: GPL-3.0-or-later */

#include <errno.h>
#include <glib/gstdio.h>
#include <stdlib.h>

#include "palette-file.h"

char *
easel_palette_file_path (void)
{
  return g_build_filename (g_get_user_config_dir (), "easel", "palette.ini", NULL);
}

void
easel_palette_file_load (const char *path, EaselPalette *p)
{
  g_autofree char *text = NULL;
  g_autoptr (GError) error = NULL;
  gsize len;

  if (!g_file_get_contents (path, &text, &len, &error))
    {
      /* Missing until the first edit (REQ-0003-15); any problem is silent (REQ-0003-17). */
      if (!g_error_matches (error, G_FILE_ERROR, G_FILE_ERROR_NOENT))
        g_debug ("Cannot read %s: %s", path, error->message);
      easel_palette_defaults (p);
      return;
    }
  easel_palette_parse (text, len, p);
}

gboolean
easel_palette_file_save (const char *path, const EaselPalette *p)
{
  g_autofree char *dir = g_path_get_dirname (path);
  g_autoptr (GError) error = NULL;
  char *text;
  gboolean ok;

  if (g_mkdir_with_parents (dir, 0700) != 0)
    {
      g_debug ("Cannot create %s: %s", dir, g_strerror (errno));
      return FALSE;
    }
  text = easel_palette_format (p);
  if (text == NULL)
    return FALSE;

  /* Temporary file and rename, as for pictures: the file is never half-written (REQ-0003-15). */
  ok = g_file_set_contents_full (path, text, -1, G_FILE_SET_CONTENTS_CONSISTENT, 0600, &error);
  if (!ok)
    g_debug ("Cannot write %s: %s", path, error->message);
  free (text);
  return ok;
}
