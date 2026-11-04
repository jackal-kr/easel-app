/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Languages (0023): reading a compiled catalog, plural rules, a template's text. */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "catalog.h"
#include "check.h"
#include "settings.h"

/* A compiled catalog of 32-bit little-endian tables, as msgfmt writes: its strings sorted by msgid. */
typedef struct
{
  const char *id;       /* with a plural: "one\0many" and id_len */
  size_t id_len;
  const char *str;
  size_t str_len;
} Entry;

static unsigned char *
build_mo (const Entry *e, int n, size_t *out_len)
{
  size_t head = 28 + 16 * (size_t) n, total = head;
  unsigned char *mo;
  uint32_t *w;
  size_t at = head;

  for (int i = 0; i < n; i++)
    total += e[i].id_len + 1 + e[i].str_len + 1;
  mo = calloc (1, total);
  w = (uint32_t *) mo;
  w[0] = 0x950412deu;
  w[1] = 0;
  w[2] = (uint32_t) n;
  w[3] = 28;
  w[4] = 28 + 8 * (uint32_t) n;
  for (int i = 0; i < n; i++)
    {
      w[7 + 2 * i] = (uint32_t) e[i].id_len;
      w[7 + 2 * i + 1] = (uint32_t) at;
      memcpy (mo + at, e[i].id, e[i].id_len);
      at += e[i].id_len + 1;
    }
  for (int i = 0; i < n; i++)
    {
      w[7 + 2 * n + 2 * i] = (uint32_t) e[i].str_len;
      w[7 + 2 * n + 2 * i + 1] = (uint32_t) at;
      memcpy (mo + at, e[i].str, e[i].str_len);
      at += e[i].str_len + 1;
    }
  *out_len = total;
  return mo;
}

#define E(a, b) { a, sizeof (a) - 1, b, sizeof (b) - 1 }

static const Entry entries[] = {
  E ("", "Content-Type: text/plain; charset=UTF-8\n"),
  E ("Open", "Открыть"),
  E ("Save &amp; go", "Сохр"),
  E ("Takes %d sheet\0Takes %d sheets", "Занимает %d лист\0Занимает %d листа\0Занимает %d листов"),
};

static EaselCatalog *
sample (unsigned char **mo)
{
  size_t len;

  *mo = build_mo (entries, 4, &len);
  return easel_catalog_new (*mo, len);
}

/* REQ-0023-02: what it has, and the source for what it lacks. */
static void
as3_lookup (void)
{
  unsigned char *mo;
  EaselCatalog *cat = sample (&mo);

  CHECK (cat != NULL);
  CHECK (strcmp (easel_catalog_get (cat, "Open"), "Открыть") == 0);
  CHECK (strcmp (easel_catalog_get (cat, "Nothing like it"), "Nothing like it") == 0);
  CHECK (strcmp (easel_catalog_get_plural (cat, "Takes %d sheet", "Takes %d sheets", 2, "ru"), "Занимает %d листа") == 0);
  CHECK (strcmp (easel_catalog_get_plural (cat, "Takes %d sheet", "Takes %d sheets", 5, "ru"), "Занимает %d листов") == 0);
  CHECK (strcmp (easel_catalog_get_plural (cat, "Takes %d sheet", "Takes %d sheets", 21, "ru"), "Занимает %d лист") == 0);
  /* Not there: the source, by English's own rule. */
  CHECK (strcmp (easel_catalog_get_plural (cat, "A cat", "%d cats", 1, "ru"), "A cat") == 0);
  CHECK (strcmp (easel_catalog_get_plural (cat, "A cat", "%d cats", 5, "ru"), "%d cats") == 0);
  /* A NULL catalog is English. */
  CHECK (strcmp (easel_catalog_get (NULL, "Open"), "Open") == 0);
  CHECK (strcmp (easel_catalog_get_plural (NULL, "A cat", "%d cats", 2, "en"), "%d cats") == 0);
  easel_catalog_free (cat);
  free (mo);
}

/* A file that is no catalog is refused, however it is short or broken. */
static void
as3_refuses_a_bad_file (void)
{
  unsigned char *mo;
  size_t len;
  uint32_t *w;

  mo = build_mo (entries, 4, &len);
  CHECK (easel_catalog_new (mo, 10) == NULL);
  CHECK (easel_catalog_new ("not a catalog of any kind, at all", 33) == NULL);
  w = (uint32_t *) mo;
  w[0] = 0x12345678u;
  CHECK (easel_catalog_new (mo, len) == NULL);
  w[0] = 0x950412deu;
  w[2] = 1000000;                               /* more strings than the file holds */
  CHECK (easel_catalog_new (mo, len) == NULL);
  w[2] = 4;
  w[7 + 2 * 1 + 1] = (uint32_t) len + 100;      /* a string past the end */
  CHECK (easel_catalog_new (mo, len) == NULL);
  free (mo);
}

/* AS-5 (REQ-0023-06): each rule for 0, 1, 2, 5, 11, 12, 21, 22, 100, 101, 111. */
static void
as5_plural_rules (void)
{
  static const unsigned long counts[] = { 0, 1, 2, 5, 11, 12, 21, 22, 100, 101, 111 };
  static const struct { const char *lang; int forms[11]; } table[] = {
    { "de", { 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1 } },
    { "en", { 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1 } },
    { "es", { 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1 } },
    { "hu", { 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1 } },
    { "it", { 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1 } },
    { "fr", { 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1 } },
    { "pt", { 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1 } },
    { "cs", { 2, 0, 1, 2, 2, 2, 2, 2, 2, 2, 2 } },
    { "pl", { 2, 0, 1, 2, 2, 2, 2, 1, 2, 2, 2 } },
    { "ro", { 1, 0, 1, 1, 1, 1, 2, 2, 2, 1, 1 } },
    { "ru", { 2, 0, 1, 2, 2, 2, 0, 1, 2, 0, 2 } },
    { "uk", { 2, 0, 1, 2, 2, 2, 0, 1, 2, 0, 2 } },
  };

  for (size_t t = 0; t < sizeof table / sizeof table[0]; t++)
    for (size_t c = 0; c < 11; c++)
      CHECK (easel_plural_form (table[t].lang, counts[c]) == table[t].forms[c]);
  /* A language with no rule of its own counts as English. */
  CHECK (easel_plural_form ("xx", 1) == 0);
  CHECK (easel_plural_form ("xx", 2) == 1);
}

/* AS-1, AS-2 (REQ-0023-02, -03): the setting, then the first preferred language the app has, by its language part. */
static void
as1_resolve (void)
{
  static const char *const de_de[] = { "de_DE.UTF-8", "de_DE", "de", "C", NULL };
  static const char *const de_at[] = { "de_AT", "de", "C", NULL };
  static const char *const pt_pt[] = { "pt_PT", "pt", "C", NULL };
  static const char *const uk_ua[] = { "uk_UA.UTF-8", "uk", "C", NULL };
  static const char *const ja_pl[] = { "ja", "pl", "C", NULL };
  static const char *const en_pl[] = { "en_US", "en", "pl", "C", NULL };
  static const char *const ja[] = { "ja_JP", "ja", "C", NULL };
  static const char *const c[] = { "C", NULL };
  static const char *const with_at[] = { "ru_RU.UTF-8@foo", "C", NULL };

  CHECK (strcmp (easel_language_resolve ("auto", de_de), "de") == 0);
  CHECK (strcmp (easel_language_resolve ("auto", de_at), "de") == 0);
  CHECK (strcmp (easel_language_resolve ("auto", pt_pt), "pt") == 0);
  CHECK (strcmp (easel_language_resolve ("auto", uk_ua), "uk") == 0);
  CHECK (strcmp (easel_language_resolve ("auto", ja_pl), "pl") == 0);
  CHECK (strcmp (easel_language_resolve ("auto", en_pl), "en") == 0);
  CHECK (strcmp (easel_language_resolve ("auto", with_at), "ru") == 0);
  /* Not one of the twelve, or C: English. */
  CHECK (strcmp (easel_language_resolve ("auto", ja), "en") == 0);
  CHECK (strcmp (easel_language_resolve ("auto", c), "en") == 0);
  /* The setting wins, whatever the system says. */
  CHECK (strcmp (easel_language_resolve ("en", de_de), "en") == 0);
  CHECK (strcmp (easel_language_resolve ("de", ja), "de") == 0);
  /* The twelve, by code. */
  CHECK (easel_language_count () == 12);
  CHECK (strcmp (easel_language_code (0), "cs") == 0);
  CHECK (strcmp (easel_language_code (11), "uk") == 0);
  CHECK (strcmp (easel_language_name (5), "Magyar") == 0);
  CHECK (strcmp (easel_language_name (6), "Italiano") == 0);
  CHECK (strcmp (easel_language_name (10), "Русский") == 0);
}

/* AS-4 (REQ-0023-04): the key. */
static void
as4_key (void)
{
  EaselSettings s;
  const char *file;

  easel_settings_defaults (&s);
  CHECK (strcmp (s.language, "auto") == 0);
  CHECK (strstr (easel_settings_default_text, "[interface]\n") != NULL);
  CHECK (strstr (easel_settings_default_text, "language=auto\n") != NULL);
  file = "[interface]\nlanguage=pl\n";
  easel_settings_parse (file, strlen (file), &s);
  CHECK (strcmp (s.language, "pl") == 0);
  file = "[interface]\nlanguage=PL\n";
  easel_settings_parse (file, strlen (file), &s);
  CHECK (strcmp (s.language, "pl") == 0);
  file = "[interface]\nlanguage=AUTO\n";
  easel_settings_parse (file, strlen (file), &s);
  CHECK (strcmp (s.language, "auto") == 0);
  file = "[interface]\nlanguage=pt_BR\n";
  easel_settings_parse (file, strlen (file), &s);
  CHECK (strcmp (s.language, "auto") == 0);
  file = "[interface]\nlanguage=xx\n";
  easel_settings_parse (file, strlen (file), &s);
  CHECK (strcmp (s.language, "auto") == 0);
  file = "[interface]\nlanguage=\n";
  easel_settings_parse (file, strlen (file), &s);
  CHECK (strcmp (s.language, "auto") == 0);
}

/* R1: the text of a template: marked elements take their translation, escaped; the mark goes; the rest stays. */
static void
as12_template_text (void)
{
  unsigned char *mo;
  EaselCatalog *cat = sample (&mo);
  char *out;

  out = easel_catalog_translate_ui (cat, "<a><attribute name=\"label\" translatable=\"yes\">Open</attribute><b>Open</b></a>");
  CHECK (strcmp (out, "<a><attribute name=\"label\">Открыть</attribute><b>Open</b></a>") == 0);
  free (out);
  /* What it lacks stays; the msgid is compared unescaped, and the result escaped again. */
  out = easel_catalog_translate_ui (cat, "<p translatable=\"yes\">Other</p>");
  CHECK (strcmp (out, "<p>Other</p>") == 0);
  free (out);
  out = easel_catalog_translate_ui (NULL, "<p translatable=\"yes\">A &amp; B</p>");
  CHECK (strcmp (out, "<p>A &amp; B</p>") == 0);
  free (out);
  easel_catalog_free (cat);
  free (mo);
}

int
main (void)
{
  RUN (as3_lookup);
  RUN (as3_refuses_a_bad_file);
  RUN (as5_plural_rules);
  RUN (as12_template_text);
  RUN (as1_resolve);
  RUN (as4_key);
  return 0;
}
