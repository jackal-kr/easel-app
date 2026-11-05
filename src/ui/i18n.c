/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "i18n.h"

#include "catalog.h"
#include "config.h"
#include "settings-file.h"
#include "settings.h"

static EaselCatalog *catalog;
static const char *language = "en";
static char *catalog_bytes;

const char *
easel_i18n_choose (const char *setting)
{
  return easel_language_resolve (setting, (const char *const *) g_get_language_names ());
}

char *
easel_i18n_catalog_path (const char *code)
{
  const char *set = g_getenv ("EASEL_LOCALE_DIR");
  g_autofree char *program = g_file_read_link ("/proc/self/exe", NULL);
  g_autofree char *folder = program != NULL ? g_path_get_dirname (program) : g_strdup (".");
  g_autofree char *in_build = g_build_filename (folder, "..", "po", code, "LC_MESSAGES", "easel.mo", NULL);

  if (set != NULL)
    return g_build_filename (set, code, "LC_MESSAGES", "easel.mo", NULL);
  /* An uninstalled build: its own copy, beside the program's folder, however it was started. */
  if (g_file_test (in_build, G_FILE_TEST_IS_REGULAR))
    return g_steal_pointer (&in_build);
  return g_build_filename (LOCALE_DIR, code, "LC_MESSAGES", "easel.mo", NULL);
}

void
easel_i18n_init (void)
{
  const char *test = g_getenv ("EASEL_TEST_LANGUAGE");
  g_autofree char *path = NULL;
  gsize len = 0;

  if (catalog != NULL)
    return;
  if (test != NULL)
    language = g_intern_string (test);
  else
    {
      g_autofree char *settings_path = easel_settings_file_path ();
      EaselSettings settings;

      easel_settings_defaults (&settings);
      easel_settings_file_load (settings_path, &settings);
      /* The choice can be the settings' own text: keep a copy that lasts. */
      language = g_intern_string (easel_i18n_choose (settings.language));
    }
  /* GTK's own strings follow what the system has, in this language; Portuguese is as in Brazil (REQ-0023-01). */
  g_setenv ("LANGUAGE", strcmp (language, "pt") == 0 ? "pt_BR:pt" : language, TRUE);
  if (strcmp (language, "en") == 0)
    return;
  path = easel_i18n_catalog_path (language);
  /* A language with no catalog is English, string by string (REQ-0023-02). */
  if (g_file_get_contents (path, &catalog_bytes, &len, NULL))
    catalog = easel_catalog_new (catalog_bytes, len);
}

const char *
easel_i18n_language (void)
{
  return language;
}

char *
easel_i18n_translate_template (const char *xml)
{
  return easel_catalog_translate_ui (catalog, xml);
}

const char *
_ (const char *msgid)
{
  return easel_catalog_get (catalog, msgid);
}

const char *
_n (const char *msgid, const char *msgid_plural, unsigned long n)
{
  return easel_catalog_get_plural (catalog, msgid, msgid_plural, n, language);
}
