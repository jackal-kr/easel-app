/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "catalog.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define MO_MAGIC 0x950412deu
#define MO_HEADER 28

struct _EaselCatalog
{
  const char *data;
  uint32_t n;
  const uint32_t *ids;   /* a length and an offset for each */
  const uint32_t *strs;
};

static uint32_t
word (const char *data, size_t at)
{
  uint32_t w;

  memcpy (&w, data + at, sizeof w);
  return w;
}

static int
entry_fits (const char *data, size_t len, uint32_t entry_len, uint32_t offset)
{
  return (size_t) offset < len && (size_t) entry_len < len - offset && data[(size_t) offset + entry_len] == '\0';
}

EaselCatalog *
easel_catalog_new (const void *mo, size_t len)
{
  const char *data = mo;
  uint32_t n, ids_at, strs_at;
  EaselCatalog *cat;

  if (len < MO_HEADER || word (data, 0) != MO_MAGIC || (word (data, 4) >> 16) != 0)
    return NULL;
  n = word (data, 8);
  ids_at = word (data, 12);
  strs_at = word (data, 16);
  if ((ids_at & 3) != 0 || (strs_at & 3) != 0 || n > len / 8 || (size_t) ids_at > len - (size_t) n * 8 || (size_t) strs_at > len - (size_t) n * 8)
    return NULL;
  for (uint32_t i = 0; i < n; i++)
    if (!entry_fits (data, len, word (data, (size_t) ids_at + (size_t) i * 8), word (data, (size_t) ids_at + (size_t) i * 8 + 4))
        || !entry_fits (data, len, word (data, (size_t) strs_at + (size_t) i * 8), word (data, (size_t) strs_at + (size_t) i * 8 + 4)))
      return NULL;
  cat = malloc (sizeof *cat);
  if (cat == NULL)
    return NULL;
  cat->data = data;
  cat->n = n;
  cat->ids = (const uint32_t *) (data + ids_at);
  cat->strs = (const uint32_t *) (data + strs_at);
  return cat;
}

void
easel_catalog_free (EaselCatalog *cat)
{
  free (cat);
}

/* The msgids are sorted as C strings; a plural's second form comes after a NUL and does not count. */
static const char *
find (const EaselCatalog *cat, const char *msgid, uint32_t *str_len)
{
  uint32_t low = 0, high;

  if (cat == NULL || msgid[0] == '\0')
    return NULL;
  high = cat->n;
  while (low < high)
    {
      uint32_t mid = low + (high - low) / 2;
      int order = strcmp (msgid, cat->data + cat->ids[2 * mid + 1]);

      if (order == 0)
        {
          *str_len = cat->strs[2 * mid];
          return cat->data + cat->strs[2 * mid + 1];
        }
      if (order < 0)
        high = mid;
      else
        low = mid + 1;
    }
  return NULL;
}

const char *
easel_catalog_get (const EaselCatalog *cat, const char *msgid)
{
  uint32_t len;
  const char *found = find (cat, msgid, &len);

  return found != NULL && found[0] != '\0' ? found : msgid;
}

const char *
easel_catalog_get_plural (const EaselCatalog *cat, const char *msgid, const char *msgid_plural, unsigned long n, const char *lang)
{
  uint32_t len;
  const char *found = find (cat, msgid, &len);
  const char *form;
  int index;

  if (found == NULL || found[0] == '\0')
    return n == 1 ? msgid : msgid_plural;
  form = found;
  index = easel_plural_form (lang, n);
  for (int i = 0; i < index; i++)
    {
      size_t skip = strlen (form) + 1;

      if ((size_t) (form - found) + skip > len)
        return found;                           /* fewer forms than the rule: the first */
      form += skip;
    }
  return form;
}

int
easel_plural_form (const char *lang, unsigned long n)
{
  unsigned long last = n % 10, two = n % 100;
  int few = last >= 2 && last <= 4 && (two < 10 || two >= 20);

  if (strcmp (lang, "fr") == 0 || strcmp (lang, "pt") == 0)
    return n > 1;
  if (strcmp (lang, "cs") == 0)
    return n == 1 ? 0 : n >= 2 && n <= 4 ? 1 : 2;
  if (strcmp (lang, "pl") == 0)
    return n == 1 ? 0 : few ? 1 : 2;
  if (strcmp (lang, "ro") == 0)
    return n == 1 ? 0 : n == 0 || (two > 0 && two < 20) ? 1 : 2;
  if (strcmp (lang, "ru") == 0 || strcmp (lang, "uk") == 0)
    return last == 1 && two != 11 ? 0 : few ? 1 : 2;
  return n != 1;
}

/* A growing text. */
typedef struct
{
  char *s;
  size_t len, cap;
} Text;

static int
text_add (Text *t, const char *s, size_t n)
{
  if (t->len + n + 1 > t->cap)
    {
      size_t cap = (t->len + n + 1) * 2;
      char *grown = realloc (t->s, cap);

      if (grown == NULL)
        return 0;
      t->s = grown;
      t->cap = cap;
    }
  memcpy (t->s + t->len, s, n);
  t->len += n;
  t->s[t->len] = '\0';
  return 1;
}

static const struct { const char *entity; char c; } entities[] = {
  { "&amp;", '&' }, { "&lt;", '<' }, { "&gt;", '>' }, { "&quot;", '"' }, { "&apos;", '\'' },
};

/* The n bytes at s with their entities read, to a new string. */
static char *
unescape (const char *s, size_t n)
{
  char *out = malloc (n + 1), *to = out;

  if (out == NULL)
    return NULL;
  for (size_t i = 0; i < n; )
    {
      size_t e, count = sizeof entities / sizeof entities[0];

      for (e = 0; e < count; e++)
        if (strncmp (s + i, entities[e].entity, strlen (entities[e].entity)) == 0)
          break;
      if (e < count)
        {
          *to++ = entities[e].c;
          i += strlen (entities[e].entity);
        }
      else
        *to++ = s[i++];
    }
  *to = '\0';
  return out;
}

static int
add_escaped (Text *t, const char *s)
{
  for (; *s != '\0'; s++)
    {
      size_t e, count = sizeof entities / sizeof entities[0];

      for (e = 0; e < count; e++)
        if (*s == entities[e].c)
          break;
      if (!(e < count ? text_add (t, entities[e].entity, strlen (entities[e].entity)) : text_add (t, s, 1)))
        return 0;
    }
  return 1;
}

char *
easel_catalog_translate_ui (const EaselCatalog *cat, const char *xml)
{
  static const char mark[] = " translatable=\"yes\"";
  Text t = { NULL, 0, 0 };
  const char *at = xml, *hit;

  if (!text_add (&t, "", 0))
    return NULL;
  while ((hit = strstr (at, mark)) != NULL)
    {
      const char *open = strchr (hit, '>'), *close = open != NULL ? strchr (open, '<') : NULL;
      char *msgid;
      int ok;

      if (close == NULL)
        break;
      msgid = unescape (open + 1, (size_t) (close - open - 1));
      ok = msgid != NULL && text_add (&t, at, (size_t) (hit - at)) && text_add (&t, hit + sizeof mark - 1, (size_t) (open + 1 - (hit + sizeof mark - 1)))
           && add_escaped (&t, easel_catalog_get (cat, msgid));
      free (msgid);
      if (!ok)
        {
          free (t.s);
          return NULL;
        }
      at = close;
    }
  if (!text_add (&t, at, strlen (at)))
    {
      free (t.s);
      return NULL;
    }
  return t.s;
}

static const struct { const char *code, *name; } languages[] = {
  { "cs", "Čeština" }, { "de", "Deutsch" }, { "en", "English" }, { "es", "Español" }, { "fr", "Français" }, { "hu", "Magyar" },
  { "it", "Italiano" }, { "pl", "Polski" }, { "pt", "Português" }, { "ro", "Română" }, { "ru", "Русский" }, { "uk", "Українська" },
};

int
easel_language_count (void)
{
  return (int) (sizeof languages / sizeof languages[0]);
}

const char *
easel_language_code (int i)
{
  return languages[i].code;
}

const char *
easel_language_name (int i)
{
  return languages[i].name;
}

int
easel_language_known (const char *code)
{
  for (int i = 0; i < easel_language_count (); i++)
    if (strcmp (languages[i].code, code) == 0)
      return 1;
  return 0;
}

const char *
easel_language_resolve (const char *setting, const char *const *names)
{
  if (strcmp (setting, "auto") != 0)
    return setting;
  for (; *names != NULL; names++)
    {
      size_t part = strcspn (*names, "_.@");

      for (int i = 0; i < easel_language_count (); i++)
        if (strlen (languages[i].code) == part && strncmp (*names, languages[i].code, part) == 0)
          return languages[i].code;
    }
  return "en";
}
