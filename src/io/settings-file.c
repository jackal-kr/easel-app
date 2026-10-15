/* SPDX-License-Identifier: GPL-3.0-or-later */

#include <errno.h>
#include <glib/gstdio.h>

#include "settings-file.h"

char *
easel_settings_file_path (void)
{
  return g_build_filename (g_get_user_config_dir (), "easel", "settings.ini", NULL);
}

static void
create_default_file (const char *path)
{
  g_autofree char *dir = g_path_get_dirname (path);
  g_autoptr (GError) error = NULL;

  if (g_mkdir_with_parents (dir, 0700) != 0)
    {
      g_debug ("Cannot create %s: %s", dir, g_strerror (errno));
      return;
    }
  if (!g_file_set_contents (path, easel_settings_default_text, -1, &error))
    g_debug ("Cannot create %s: %s", path, error->message);
}

void
easel_settings_file_load (const char *path, EaselSettings *s)
{
  g_autofree char *text = NULL;
  g_autoptr (GError) error = NULL;
  gsize len;

  if (g_file_get_contents (path, &text, &len, &error))
    {
      easel_settings_parse (text, len, s);
      return;
    }

  /* Problems stay silent for the user (REQ-0001-05); G_MESSAGES_DEBUG shows them. */
  easel_settings_defaults (s);
  if (g_error_matches (error, G_FILE_ERROR, G_FILE_ERROR_NOENT))
    create_default_file (path);
  else
    g_debug ("Cannot read %s: %s", path, error->message);
}
