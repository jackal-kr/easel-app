/* SPDX-License-Identifier: GPL-3.0-or-later */

#include <stdbool.h>
#include <string.h>

#include "settings.h"

#define SECTION "new-picture"
#define SECTION_HISTORY "history"

typedef enum
{
  IN_OTHER,
  IN_NEW_PICTURE,
  IN_HISTORY,
} Section;

typedef struct
{
  const char *p;
  size_t n;
} Span;

static bool
is_space (char c)
{
  return c == ' ' || c == '\t' || c == '\r';
}

static Span
trim (Span s)
{
  while (s.n > 0 && is_space (s.p[0]))
    {
      s.p++;
      s.n--;
    }
  while (s.n > 0 && is_space (s.p[s.n - 1]))
    s.n--;
  return s;
}

static bool
span_is (Span s, const char *word)
{
  return s.n == strlen (word) && memcmp (s.p, word, s.n) == 0;
}

static char
lower (char c)
{
  return (c >= 'A' && c <= 'Z') ? (char) (c - 'A' + 'a') : c;
}

static bool
span_is_nocase (Span s, const char *word)
{
  if (s.n != strlen (word))
    return false;
  for (size_t i = 0; i < s.n; i++)
    if (lower (s.p[i]) != word[i])
      return false;
  return true;
}

/* Digits only, 1..EASEL_SIZE_MAX, or auto. */
static bool
parse_size (Span v, int *out)
{
  int n = 0;

  if (span_is_nocase (v, "auto"))
    {
      *out = EASEL_SIZE_AUTO;
      return true;
    }
  if (v.n == 0)
    return false;
  for (size_t i = 0; i < v.n; i++)
    {
      if (v.p[i] < '0' || v.p[i] > '9')
        return false;
      n = n * 10 + (v.p[i] - '0');
      if (n > EASEL_SIZE_MAX)
        return false;
    }
  if (n < 1)
    return false;
  *out = n;
  return true;
}

static int
hex_digit (char c)
{
  c = lower (c);
  if (c >= '0' && c <= '9')
    return c - '0';
  if (c >= 'a' && c <= 'f')
    return c - 'a' + 10;
  return -1;
}

/* Digits only, EASEL_UNDO_MB_MIN..EASEL_UNDO_MB_MAX. */
static bool
parse_undo_mb (Span v, int *out)
{
  int n = 0;

  if (v.n == 0)
    return false;
  for (size_t i = 0; i < v.n; i++)
    {
      if (v.p[i] < '0' || v.p[i] > '9')
        return false;
      n = n * 10 + (v.p[i] - '0');
      if (n > EASEL_UNDO_MB_MAX)
        return false;
    }
  if (n < EASEL_UNDO_MB_MIN)
    return false;
  *out = n;
  return true;
}

/* #RRGGBB, #RRGGBBAA or transparent, into 0xAARRGGBB. */
static bool
parse_color (Span v, uint32_t *out)
{
  uint32_t rgba = 0;

  if (span_is_nocase (v, "transparent"))
    {
      *out = 0x00000000u;
      return true;
    }
  if ((v.n != 7 && v.n != 9) || v.p[0] != '#')
    return false;
  for (size_t i = 1; i < v.n; i++)
    {
      int d = hex_digit (v.p[i]);

      if (d < 0)
        return false;
      rgba = (rgba << 4) | (uint32_t) d;
    }
  if (v.n == 7)
    *out = 0xFF000000u | rgba;
  else
    *out = (rgba >> 8) | (rgba << 24);
  return true;
}

const char easel_settings_default_text[] =
  "[" SECTION "]\n"
  "width=auto\n"
  "height=auto\n"
  "background=#FFFFFF\n"
  "\n"
  "[" SECTION_HISTORY "]\n"
  "memory=512\n";

void
easel_settings_defaults (EaselSettings *s)
{
  s->width = EASEL_SIZE_AUTO;
  s->height = EASEL_SIZE_AUTO;
  s->background = 0xFFFFFFFFu;
  s->undo_budget_mb = EASEL_UNDO_MB_DEFAULT;
}

void
easel_settings_parse (const char *text, size_t len, EaselSettings *s)
{
  EaselSettings parsed;
  Section section = IN_OTHER;
  size_t pos = 0;

  easel_settings_defaults (&parsed);

  while (pos < len)
    {
      const char *nl = memchr (text + pos, '\n', len - pos);
      size_t end = nl ? (size_t) (nl - text) : len;
      Span line = trim ((Span) { text + pos, end - pos });
      const char *eq;

      pos = end + 1;

      if (line.n == 0 || line.p[0] == '#')
        continue;

      if (line.p[0] == '[' && line.p[line.n - 1] == ']' && line.n >= 2)
        {
          Span name = { line.p + 1, line.n - 2 };

          section = span_is (name, SECTION) ? IN_NEW_PICTURE : span_is (name, SECTION_HISTORY) ? IN_HISTORY : IN_OTHER;
          continue;
        }

      eq = memchr (line.p, '=', line.n);
      if (eq == NULL || eq == line.p || memchr (line.p, '\0', line.n) != NULL)
        {
          /* Not a comment, section or key=value line: the whole file is unparseable (REQ-0001-40). */
          easel_settings_defaults (s);
          return;
        }

      if (section != IN_OTHER)
        {
          Span key = trim ((Span) { line.p, (size_t) (eq - line.p) });
          Span value = trim ((Span) { eq + 1, line.n - (size_t) (eq - line.p) - 1 });

          /* An invalid value resets the key to its default, so a later valid repeat still wins. */
          if (section == IN_NEW_PICTURE)
            {
              if (span_is (key, "width") && !parse_size (value, &parsed.width))
                parsed.width = EASEL_SIZE_AUTO;
              else if (span_is (key, "height") && !parse_size (value, &parsed.height))
                parsed.height = EASEL_SIZE_AUTO;
              else if (span_is (key, "background") && !parse_color (value, &parsed.background))
                parsed.background = 0xFFFFFFFFu;
            }
          else if (span_is (key, "memory") && !parse_undo_mb (value, &parsed.undo_budget_mb))
            parsed.undo_budget_mb = EASEL_UNDO_MB_DEFAULT;
        }
    }

  *s = parsed;
}

void
easel_settings_resolve_size (const EaselSettings *s, int area_width, int area_height,
                             int *width, int *height)
{
  /* Largest last. Preferences (0013) will make this list a setting. */
  static const int standard[][2] = { { 640, 480 }, { 800, 600 }, { 1024, 768 } };

  if (s->width != EASEL_SIZE_AUTO && s->height != EASEL_SIZE_AUTO)
    {
      *width = s->width;
      *height = s->height;
      return;
    }

  *width = standard[0][0];
  *height = standard[0][1];
  for (size_t i = 0; i < sizeof standard / sizeof standard[0]; i++)
    if (standard[i][0] < area_width && standard[i][1] < area_height)
      {
        *width = standard[i][0];
        *height = standard[i][1];
      }
}
