/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Settings file scenarios of feature 0001. Runs with isolated XDG dirs, so the real config is never touched. */

#include <glib/gstdio.h>
#include <unistd.h>

#include "settings-file.h"

static void
as4_creates_default_file (void)
{
  g_autofree char *path = easel_settings_file_path ();
  g_autofree char *contents = NULL;
  EaselSettings s;

  g_assert_true (g_str_has_suffix (path, "/easel/settings.ini"));
  g_assert_false (g_file_test (path, G_FILE_TEST_EXISTS));

  easel_settings_file_load (path, &s);

  g_assert_cmpint (s.width, ==, EASEL_SIZE_AUTO);
  g_assert_cmpint (s.height, ==, EASEL_SIZE_AUTO);
  g_assert_cmphex (s.background, ==, 0xFFFFFFFFu);
  g_assert_true (g_file_get_contents (path, &contents, NULL, NULL));
  g_assert_nonnull (g_strstr_len (contents, -1, "[new-picture]\nwidth=auto\nheight=auto\ncanvas-color=#FFFFFF\n"));
}

/* REQ-0004-15: a created file also holds the [view] defaults. */
static void
as14_created_file_has_view (void)
{
  g_autofree char *dir = g_dir_make_tmp ("easel-XXXXXX", NULL);
  g_autofree char *path = g_build_filename (dir, "settings.ini", NULL);
  g_autofree char *contents = NULL;
  EaselSettings s;

  easel_settings_file_load (path, &s);
  g_assert_false (s.grid);
  g_assert_cmpint (s.grid_size, ==, 1);
  g_assert_true (s.show_toolbox && s.show_colorbox && s.show_statusbar);
  g_assert_true (g_file_get_contents (path, &contents, NULL, NULL));
  g_assert_nonnull (g_strstr_len (contents, -1, "[view]\ngrid=off\ngrid-size=1\ngrid-axonometric=off\ntoolbox=on\n"
                                                "colorbox=on\nstatusbar=on\n"));
}

/* REQ-0005-14: a created file also holds [save] with the JPEG quality. */
static void
as16_save_section_in_new_file (void)
{
  g_autofree char *dir = g_dir_make_tmp ("easel-XXXXXX", NULL);
  g_autofree char *path = g_build_filename (dir, "settings.ini", NULL);
  g_autofree char *contents = NULL;
  EaselSettings s;

  easel_settings_file_load (path, &s);
  g_assert_cmpint (s.jpeg_quality, ==, 90);
  g_assert_true (g_file_get_contents (path, &contents, NULL, NULL));
  g_assert_nonnull (g_strstr_len (contents, -1, "[save]\njpeg-quality=90\n"));
}

/* REQ-0004-15: an existing file without [view] is never rewritten, and the defaults apply. */
static void
as14_existing_file_untouched (void)
{
  static const char old[] = "[new-picture]\nwidth=300\nheight=200\n\n[history]\nmemory=64\n";
  g_autofree char *dir = g_dir_make_tmp ("easel-XXXXXX", NULL);
  g_autofree char *path = g_build_filename (dir, "settings.ini", NULL);
  g_autofree char *after = NULL;
  gsize after_len;
  EaselSettings s;

  g_assert_true (g_file_set_contents (path, old, -1, NULL));
  easel_settings_file_load (path, &s);
  g_assert_cmpint (s.width, ==, 300);
  g_assert_cmpint (s.undo_budget_mb, ==, 64);
  g_assert_false (s.grid);
  g_assert_cmpint (s.grid_size, ==, 1);
  g_assert_cmpint (s.jpeg_quality, ==, 90);
  g_assert_true (s.show_toolbox && s.show_colorbox && s.show_statusbar);
  g_assert_true (g_file_get_contents (path, &after, &after_len, NULL));
  g_assert_cmpmem (after, after_len, old, sizeof old - 1);
}

/* REQ-0002-19: a file the app creates also holds the undo budget. */
static void
as19_new_file_has_history (void)
{
  g_autofree char *dir = g_dir_make_tmp ("easel-XXXXXX", NULL);
  g_autofree char *path = g_build_filename (dir, "settings.ini", NULL);
  g_autofree char *contents = NULL;
  EaselSettings s;

  easel_settings_file_load (path, &s);
  g_assert_cmpint (s.undo_budget_mb, ==, 512);
  g_assert_true (g_file_get_contents (path, &contents, NULL, NULL));
  g_assert_nonnull (g_strstr_len (contents, -1, "[history]\nmemory=512\n"));
}

/* REQ-0002-19: an existing file without [history] is never rewritten. */
static void
as19_existing_file_untouched (void)
{
  static const char old[] = "[new-picture]\nwidth=300\nheight=200\nbackground=#FFFF00\n";
  g_autofree char *dir = g_dir_make_tmp ("easel-XXXXXX", NULL);
  g_autofree char *path = g_build_filename (dir, "settings.ini", NULL);
  g_autofree char *after = NULL;
  gsize after_len;
  EaselSettings s;

  g_assert_true (g_file_set_contents (path, old, -1, NULL));
  easel_settings_file_load (path, &s);
  g_assert_cmpint (s.width, ==, 300);
  g_assert_cmpint (s.undo_budget_mb, ==, 512);
  g_assert_true (g_file_get_contents (path, &after, &after_len, NULL));
  g_assert_cmpmem (after, after_len, old, sizeof old - 1);
}

/* REQ-0001-01: values come from the file. */
static void
reads_existing_file (void)
{
  g_autofree char *dir = g_dir_make_tmp ("easel-XXXXXX", NULL);
  g_autofree char *path = g_build_filename (dir, "settings.ini", NULL);
  EaselSettings s;

  g_assert_true (g_file_set_contents (path, "[new-picture]\nwidth=300\nheight=200\nbackground=#FFFF00\n", -1, NULL));
  easel_settings_file_load (path, &s);
  g_assert_cmpint (s.width, ==, 300);
  g_assert_cmpint (s.height, ==, 200);
  g_assert_cmphex (s.background, ==, 0xFFFFFF00u);
}

static void
as6_garbage_file_untouched (void)
{
  static const char garbage[] = "\x89PNG\r\n\x1a\n\0\0\0\rIHDR\xff\xfe";
  g_autofree char *dir = g_dir_make_tmp ("easel-XXXXXX", NULL);
  g_autofree char *path = g_build_filename (dir, "settings.ini", NULL);
  g_autofree char *after = NULL;
  gsize after_len;
  EaselSettings s;

  g_assert_true (g_file_set_contents (path, garbage, sizeof garbage - 1, NULL));
  easel_settings_file_load (path, &s);

  g_assert_cmpint (s.width, ==, EASEL_SIZE_AUTO);
  g_assert_cmphex (s.background, ==, 0xFFFFFFFFu);
  g_assert_true (g_file_get_contents (path, &after, &after_len, NULL));
  g_assert_cmpmem (after, after_len, garbage, sizeof garbage - 1);
}

/* REQ-0001-05: a file that cannot be read gives the defaults and is left alone. */
static void
unreadable_file_is_defaults_and_untouched (void)
{
  g_autofree char *dir = g_dir_make_tmp ("easel-XXXXXX", NULL);
  g_autofree char *path = g_build_filename (dir, "settings.ini", NULL);
  EaselSettings s;
  GStatBuf st;

  if (geteuid () == 0)
    {
      g_test_skip ("root ignores file permissions");
      return;
    }
  g_assert_true (g_file_set_contents (path, "[new-picture]\nwidth=300\n", -1, NULL));
  g_assert_cmpint (g_chmod (path, 0), ==, 0);

  easel_settings_file_load (path, &s);

  g_assert_cmpint (s.width, ==, EASEL_SIZE_AUTO);
  g_assert_cmpint (g_stat (path, &st), ==, 0);
  g_assert_cmpint (st.st_mode & 0777, ==, 0);
  g_chmod (path, 0600);
}

/* REQ-0001-04, REQ-0001-05: if the file cannot be created, the defaults still apply, silently. */
static void
uncreatable_file_is_defaults (void)
{
  g_autofree char *dir = g_dir_make_tmp ("easel-XXXXXX", NULL);
  g_autofree char *path = g_build_filename (dir, "sub", "settings.ini", NULL);
  EaselSettings s;

  if (geteuid () == 0)
    {
      g_test_skip ("root ignores folder permissions");
      return;
    }
  g_assert_cmpint (g_chmod (dir, 0500), ==, 0);

  easel_settings_file_load (path, &s);

  g_assert_cmpint (s.width, ==, EASEL_SIZE_AUTO);
  g_assert_false (g_file_test (path, G_FILE_TEST_EXISTS));
  g_chmod (dir, 0700);
}

int
main (int argc, char **argv)
{
  g_test_init (&argc, &argv, G_TEST_OPTION_ISOLATE_DIRS, NULL);
  g_test_add_func ("/0001/as4_creates_default_file", as4_creates_default_file);
  g_test_add_func ("/0004/as14_created_file_has_view", as14_created_file_has_view);
  g_test_add_func ("/0004/as14_existing_file_untouched", as14_existing_file_untouched);
  g_test_add_func ("/0005/as16_save_section_in_new_file", as16_save_section_in_new_file);
  g_test_add_func ("/0002/as19_new_file_has_history", as19_new_file_has_history);
  g_test_add_func ("/0002/as19_existing_file_untouched", as19_existing_file_untouched);
  g_test_add_func ("/0001/reads_existing_file", reads_existing_file);
  g_test_add_func ("/0001/as6_garbage_file_untouched", as6_garbage_file_untouched);
  g_test_add_func ("/0001/unreadable_file_is_defaults_and_untouched", unreadable_file_is_defaults_and_untouched);
  g_test_add_func ("/0001/uncreatable_file_is_defaults", uncreatable_file_is_defaults);
  return g_test_run ();
}
