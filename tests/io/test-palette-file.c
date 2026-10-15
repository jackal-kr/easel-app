/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Palette file scenarios of feature 0003. Runs with isolated XDG dirs, so the real config is never touched. */

#include <glib/gstdio.h>
#include <string.h>
#include <unistd.h>

#include "palette-file.h"

static char *
temp_config_file (const char *name)
{
  g_autofree char *dir = g_dir_make_tmp ("easel-palette-XXXXXX", NULL);

  return g_build_filename (dir, name, NULL);
}

/* REQ-0003-15, REQ-0003-17: no file means the defaults, and reading never creates one. */
static void
missing_file_gives_defaults (void)
{
  g_autofree char *path = easel_palette_file_path ();
  EaselPalette p, def;

  g_assert_true (g_str_has_suffix (path, "/easel/palette.ini"));
  easel_palette_file_load (path, &p);
  easel_palette_defaults (&def);
  g_assert_cmpmem (&p, sizeof p, &def, sizeof def);
  /* Reading never creates the file. */
  g_assert_false (g_file_test (path, G_FILE_TEST_EXISTS));
}

static void
as14_writes_whole_palette (void)
{
  static const char settings[] = "[new-picture]\nwidth=300\n# mine\n";
  g_autofree char *path = temp_config_file ("easel/palette.ini");
  g_autofree char *dir = g_path_get_dirname (path);
  g_autofree char *settings_path = g_build_filename (dir, "settings.ini", NULL);
  g_autofree char *text = NULL, *after = NULL;
  gsize after_len;
  EaselPalette p, back;

  g_assert_cmpint (g_mkdir_with_parents (dir, 0700), ==, 0);
  g_assert_true (g_file_set_contents (settings_path, settings, -1, NULL));

  easel_palette_defaults (&p);
  p.colors[3 - 1] = 0xFF123456u;
  g_assert_true (easel_palette_file_save (path, &p));

  g_assert_true (g_file_get_contents (path, &text, NULL, NULL));
  g_assert_true (g_str_has_prefix (text, "[palette]\n"));
  g_assert_nonnull (strstr (text, "\ncolor3=#123456\n"));
  for (int i = 1; i <= EASEL_PALETTE_SIZE; i++)
    {
      g_autofree char *key = g_strdup_printf ("\ncolor%d=", i);

      g_assert_nonnull (strstr (text, key));
    }
  easel_palette_file_load (path, &back);
  g_assert_cmpmem (&p, sizeof p, &back, sizeof back);

  /* The hand-edited settings file is not touched. */
  g_assert_true (g_file_get_contents (settings_path, &after, &after_len, NULL));
  g_assert_cmpmem (after, after_len, settings, sizeof settings - 1);
}

static void
as15_write_failure_silent (void)
{
  g_autofree char *path = temp_config_file ("ro/palette.ini");
  g_autofree char *dir = g_path_get_dirname (path);
  EaselPalette p;

  if (geteuid () == 0)
    {
      g_test_skip ("root ignores folder permissions");
      return;
    }
  g_assert_cmpint (g_mkdir (dir, 0500), ==, 0);
  easel_palette_defaults (&p);
  p.colors[3 - 1] = 0xFF123456u;
  g_assert_false (easel_palette_file_save (path, &p));
  g_assert_false (g_file_test (path, G_FILE_TEST_EXISTS));
  g_chmod (dir, 0700);
}

/* REQ-0003-15: the palette file is never half-written. With the file already there and no new file allowed in its
   folder, an in-place rewrite would succeed; the atomic write must fail and leave the old palette whole. */
static void
as15_existing_file_kept_when_write_fails (void)
{
  static const char old[] = "[palette]\ncolor3=#111111\n";
  g_autofree char *path = temp_config_file ("ro/palette.ini");
  g_autofree char *dir = g_path_get_dirname (path);
  g_autofree char *after = NULL;
  gsize after_len;
  EaselPalette p;

  if (geteuid () == 0)
    {
      g_test_skip ("root ignores folder permissions");
      return;
    }
  g_assert_cmpint (g_mkdir (dir, 0700), ==, 0);
  g_assert_true (g_file_set_contents (path, old, -1, NULL));
  g_assert_cmpint (g_chmod (dir, 0500), ==, 0);

  easel_palette_defaults (&p);
  p.colors[3 - 1] = 0xFF123456u;
  g_assert_false (easel_palette_file_save (path, &p));
  g_assert_true (g_file_get_contents (path, &after, &after_len, NULL));
  g_assert_cmpmem (after, after_len, old, sizeof old - 1);
  g_chmod (dir, 0700);
}

static void
as16_garbage_file_defaults (void)
{
  static const char garbage[] = "\x89PNG\r\n\x1a\n\0\0\0\rIHDR\xff";
  g_autofree char *path = temp_config_file ("palette.ini");
  EaselPalette p, def;

  g_assert_true (g_file_set_contents (path, garbage, sizeof garbage - 1, NULL));
  easel_palette_file_load (path, &p);
  easel_palette_defaults (&def);
  g_assert_cmpmem (&p, sizeof p, &def, sizeof def);
}

/* One valid, one garbage and one missing entry: only the garbage and the missing swatch fall back. */
static void
as16_mixed_entries_fall_back (void)
{
  g_autofree char *path = temp_config_file ("palette.ini");
  g_autoptr (GString) text = g_string_new ("[palette]\n");
  EaselPalette p, def;

  for (int i = 1; i <= EASEL_PALETTE_SIZE; i++)
    {
      if (i == 3)
        g_string_append (text, "color3=#12345G\n");
      else if (i != 4)
        g_string_append_printf (text, "color%d=#010203\n", i);
    }
  g_assert_true (g_file_set_contents (path, text->str, -1, NULL));

  easel_palette_file_load (path, &p);
  easel_palette_defaults (&def);
  for (int i = 0; i < EASEL_PALETTE_SIZE; i++)
    {
      if (i == 3 - 1 || i == 4 - 1)
        g_assert_cmphex (p.colors[i], ==, def.colors[i]);
      else
        g_assert_cmphex (p.colors[i], ==, 0xFF010203u);
    }
}

int
main (int argc, char **argv)
{
  g_test_init (&argc, &argv, G_TEST_OPTION_ISOLATE_DIRS, NULL);
  g_test_add_func ("/0003/missing_file_gives_defaults", missing_file_gives_defaults);
  g_test_add_func ("/0003/as14_writes_whole_palette", as14_writes_whole_palette);
  g_test_add_func ("/0003/as15_write_failure_silent", as15_write_failure_silent);
  g_test_add_func ("/0003/as15_existing_file_kept_when_write_fails", as15_existing_file_kept_when_write_fails);
  g_test_add_func ("/0003/as16_garbage_file_defaults", as16_garbage_file_defaults);
  g_test_add_func ("/0003/as16_mixed_entries_fall_back", as16_mixed_entries_fall_back);
  return g_test_run ();
}
