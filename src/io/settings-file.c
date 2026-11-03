/* SPDX-License-Identifier: GPL-3.0-or-later */

#include <errno.h>
#include <glib/gstdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

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

gboolean
easel_settings_file_change (const char *path, const EaselSettingChange *changes, size_t n)
{
  g_autofree char *resolved = realpath (path, NULL);
  const char *target = resolved != NULL ? resolved : path;
  g_autofree char *dir = g_path_get_dirname (path);
  g_autoptr (GError) error = NULL;
  char *text = NULL;
  gsize len = 0;
  mode_t mode = 0600;
  struct stat st;
  gboolean ok;

  if (g_stat (target, &st) == 0)
    {
      /* The replace would succeed on a read-only file in a writable folder, and it must not (REQ-0020-10). */
      if (S_ISDIR (st.st_mode) || g_access (target, R_OK | W_OK) != 0 || !g_file_get_contents (target, &text, &len, &error))
        return FALSE;
      mode = st.st_mode & 0777;
    }
  else
    {
      if (errno != ENOENT || g_mkdir_with_parents (dir, 0700) != 0)
        return FALSE;
      text = g_strdup (easel_settings_default_text);
      len = strlen (text);
    }
  for (size_t i = 0; i < n; i++)
    {
      char *next = NULL;
      size_t next_len = 0;

      if (!easel_settings_edit (text, len, changes[i].section, changes[i].key, changes[i].value, &next, &next_len))
        {
          g_free (text);
          return FALSE;
        }
      g_free (text);
      text = g_memdup2 (next, next_len > 0 ? next_len : 1);
      len = next_len;
      free (next);
    }
  /* Written beside it and renamed over it, so that a failure never leaves it half written. */
  ok = g_file_set_contents_full (target, text, (gssize) len, G_FILE_SET_CONTENTS_CONSISTENT, (int) mode, &error);
  if (!ok)
    g_debug ("Cannot write %s: %s", target, error->message);
  g_free (text);
  return ok;
}
