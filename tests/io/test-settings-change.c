/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Preferences (0020): a change to the settings file on disk, in one step, or none (REQ-0020-09, REQ-0020-10). */

#include <errno.h>
#include <glib.h>
#include <glib/gstdio.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "../core/check.h"
#include "settings-file.h"

static char *dir;

static char *
in_dir (const char *name)
{
  return g_build_filename (dir, name, NULL);
}

static void
write_file (const char *path, const char *text)
{
  CHECK (g_file_set_contents (path, text, -1, NULL));
}

static void
check_file (const char *path, const char *expected)
{
  g_autofree char *text = NULL;

  CHECK (g_file_get_contents (path, &text, NULL, NULL));
  if (strcmp (text, expected) != 0)
    fprintf (stderr, "got:\n%s\nexpected:\n%s\n", text, expected);
  CHECK (strcmp (text, expected) == 0);
}

/* AS-7, AS-11: only the key changes; both keys of one change, or neither. */
static void
as7_values_on_disk (void)
{
  g_autofree char *path = in_dir ("a.ini");
  const EaselSettingChange one[] = { { "text", "size", "14" } };
  const EaselSettingChange two[] = { { "new-picture", "width", "1024" }, { "new-picture", "height", "768" } };
  const EaselSettingChange remove[] = { { "view", "workspace-color", NULL } };

  write_file (path, "# mine\n[text]\nsize = 12\n[new-picture]\nwidth=auto\nheight=auto\n[view]\nworkspace-color=#111111\n");
  CHECK (easel_settings_file_change (path, one, 1));
  CHECK (easel_settings_file_change (path, two, 2));
  CHECK (easel_settings_file_change (path, remove, 1));
  check_file (path, "# mine\n[text]\nsize=14\n[new-picture]\nwidth=1024\nheight=768\n[view]\n");
  /* A hand edit made since is kept: the file is read as it is at each change. */
  write_file (path, "# edited by hand\n[text]\nsize=14\n");
  CHECK (easel_settings_file_change (path, two, 2));
  check_file (path, "# edited by hand\n[text]\nsize=14\n\n[new-picture]\nwidth=1024\nheight=768\n");
}

/* AS-12: no file and no folder: both are made, with the defaults and the change. */
static void
as12_missing_file_and_folder (void)
{
  g_autofree char *path = g_build_filename (dir, "new", "folder", "b.ini", NULL);
  const EaselSettingChange one[] = { { "view", "tint", "40" } };
  g_autofree char *text = NULL;
  EaselSettings s;

  CHECK (easel_settings_file_change (path, one, 1));
  CHECK (g_file_get_contents (path, &text, NULL, NULL));
  CHECK (strstr (text, "tint=40\n") != NULL);
  CHECK (strstr (text, "[drawing]\n") != NULL);
  easel_settings_parse (text, strlen (text), &s);
  CHECK (s.tint == 40 && s.line_width == 1);
}

/* AS-12: a file that may not be changed is left exactly as it is, and the change is refused. */
static void
as12_refused_leaves_it (void)
{
  const EaselSettingChange one[] = { { "view", "tint", "40" } };
  g_autofree char *garbage = in_dir ("garbage.ini"), *readonly = in_dir ("readonly.ini"), *folder = in_dir ("folder.ini");
  g_autofree char *sub = in_dir ("locked"), *inside = NULL;

  write_file (garbage, "this line has no equals sign\n[view]\ntint=12\n");
  CHECK (!easel_settings_file_change (garbage, one, 1));
  check_file (garbage, "this line has no equals sign\n[view]\ntint=12\n");

  /* R2: a read-only file in a writable folder, where a rename would succeed. */
  write_file (readonly, "[view]\ntint=12\n");
  CHECK (g_chmod (readonly, 0444) == 0);
  CHECK (!easel_settings_file_change (readonly, one, 1));
  check_file (readonly, "[view]\ntint=12\n");
  CHECK (g_chmod (readonly, 0644) == 0);

  CHECK (g_mkdir (folder, 0700) == 0);
  CHECK (!easel_settings_file_change (folder, one, 1));
  CHECK (g_file_test (folder, G_FILE_TEST_IS_DIR));

  /* A folder that cannot be written: the old file whole. */
  CHECK (g_mkdir (sub, 0700) == 0);
  inside = g_build_filename (sub, "c.ini", NULL);
  write_file (inside, "[view]\ntint=12\n");
  CHECK (g_chmod (sub, 0500) == 0);
  CHECK (!easel_settings_file_change (inside, one, 1));
  check_file (inside, "[view]\ntint=12\n");
  CHECK (g_chmod (sub, 0700) == 0);
}

/* R1, AS-11: a symbolic link stays and its target is written; the file's mode stays. */
static void
as12_symlink_and_mode (void)
{
  g_autofree char *target = in_dir ("target.ini"), *link_path = in_dir ("link.ini");
  const EaselSettingChange one[] = { { "view", "tint", "40" } };
  struct stat st;

  write_file (target, "[view]\ntint=12\n");
  CHECK (g_chmod (target, 0640) == 0);
  CHECK (symlink (target, link_path) == 0);
  CHECK (easel_settings_file_change (link_path, one, 1));
  CHECK (lstat (link_path, &st) == 0 && S_ISLNK (st.st_mode));
  check_file (target, "[view]\ntint=40\n");
  CHECK (stat (target, &st) == 0 && (st.st_mode & 0777) == 0640);
}

/* R7: what a burst costs, and that it ends with one line of the last value. */
static void
as8_quick_writes (void)
{
  g_autofree char *path = in_dir ("burst.ini");
  gint64 start;
  char value[8];

  write_file (path, "[view]\ntint=12\n");
  start = g_get_monotonic_time ();
  for (int i = 0; i < 100; i++)
    {
      EaselSettingChange c = { "view", "tint", value };

      g_snprintf (value, sizeof value, "%d", i % 51);
      CHECK (easel_settings_file_change (path, &c, 1));
    }
  printf ("100 changes: %.1f ms\n", (g_get_monotonic_time () - start) / 1000.0);
  check_file (path, "[view]\ntint=48\n");
}

int
main (void)
{
  dir = g_dir_make_tmp ("easel-change-XXXXXX", NULL);
  CHECK (dir != NULL);
  RUN (as7_values_on_disk);
  RUN (as12_missing_file_and_folder);
  RUN (as12_refused_leaves_it);
  RUN (as12_symlink_and_mode);
  RUN (as8_quick_writes);
  {
    g_autofree char *remove_it = g_strdup_printf ("rm -rf '%s'", dir);

    CHECK (g_spawn_command_line_sync (remove_it, NULL, NULL, NULL, NULL));
  }
  g_free (dir);
  return 0;
}
