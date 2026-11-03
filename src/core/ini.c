/* SPDX-License-Identifier: GPL-3.0-or-later */

#include <string.h>

#include "ini.h"

static bool
is_space (char c)
{
  return c == ' ' || c == '\t' || c == '\r';
}

EaselSpan
easel_span_trim (EaselSpan s)
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

static char
lower (char c)
{
  return (c >= 'A' && c <= 'Z') ? (char) (c - 'A' + 'a') : c;
}

bool
easel_span_is (EaselSpan s, const char *word)
{
  return s.n == strlen (word) && memcmp (s.p, word, s.n) == 0;
}

bool
easel_span_is_nocase (EaselSpan s, const char *lower_word)
{
  if (s.n != strlen (lower_word))
    return false;
  for (size_t i = 0; i < s.n; i++)
    if (lower (s.p[i]) != lower_word[i])
      return false;
  return true;
}

bool
easel_ini_parse (const char *text, size_t len, EaselIniFunc func, void *data)
{
  EaselSpan section = { "", 0 };
  size_t pos = 0;

  while (pos < len)
    {
      const char *nl = memchr (text + pos, '\n', len - pos);
      size_t end = nl ? (size_t) (nl - text) : len;
      EaselSpan line = easel_span_trim ((EaselSpan) { text + pos, end - pos });
      const char *eq;

      pos = end + 1;

      if (line.n == 0 || line.p[0] == '#')
        continue;

      if (line.p[0] == '[' && line.p[line.n - 1] == ']' && line.n >= 2)
        {
          section = (EaselSpan) { line.p + 1, line.n - 2 };
          continue;
        }

      eq = memchr (line.p, '=', line.n);
      if (eq == NULL || eq == line.p || memchr (line.p, '\0', line.n) != NULL)
        return false;

      func (section, easel_span_trim ((EaselSpan) { line.p, (size_t) (eq - line.p) }),
            easel_span_trim ((EaselSpan) { eq + 1, line.n - (size_t) (eq - line.p) - 1 }), data);
    }
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

bool
easel_ini_parse_color (EaselSpan v, uint32_t *out)
{
  uint32_t rgba = 0;

  if (easel_span_is_nocase (v, "transparent"))
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
