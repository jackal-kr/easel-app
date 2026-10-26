/* SPDX-License-Identifier: GPL-3.0-or-later */

#include <stdio.h>
#include <stdlib.h>

#include "ini.h"
#include "palette.h"

#define SECTION "palette"

/* The classic XP Paint colors, swatch 1 first (REQ-0003-04). */
static const uint32_t default_colors[EASEL_PALETTE_SIZE] = {
  0xFF000000,  0xFF808080,  0xFF800000,  0xFF808000,  0xFF008000,  0xFF008080, 0xFF000080,
  0xFF800080,  0xFF808040,  0xFF004040,  0xFF0080FF, 0xFF004080, 0xFF8000FF, 0xFF804000,
  0xFFFFFFFF, 0xFFC0C0C0, 0xFFFF0000, 0xFFFFFF00, 0xFF00FF00, 0xFF00FFFF, 0xFF0000FF,
  0xFFFF00FF, 0xFFFFFF80, 0xFF00FF80, 0xFF80FFFF, 0xFF8080FF, 0xFFFF0080, 0xFFFF8040,
};

void
easel_palette_defaults (EaselPalette *p)
{
  for (int i = 0; i < EASEL_PALETTE_SIZE; i++)
    p->colors[i] = default_colors[i];
}

/* "color1" .. "color28", exactly: no leading zero, no other spelling. Returns the index, or -1. */
static int
key_index (EaselSpan key)
{
  int n = 0;

  if (key.n < 6 || key.n > 7 || !easel_span_is ((EaselSpan) { key.p, 5 }, "color") || key.p[5] == '0')
    return -1;
  for (size_t i = 5; i < key.n; i++)
    {
      if (key.p[i] < '0' || key.p[i] > '9')
        return -1;
      n = n * 10 + (key.p[i] - '0');
    }
  return n >= 1 && n <= EASEL_PALETTE_SIZE ? n - 1 : -1;
}

/* An invalid value resets the entry to its default, so a later valid repeat still wins. */
static void
apply (EaselSpan section, EaselSpan key, EaselSpan value, void *data)
{
  EaselPalette *p = data;
  int i;

  if (!easel_span_is (section, SECTION) || (i = key_index (key)) < 0)
    return;
  if (!easel_ini_parse_color (value, &p->colors[i]))
    p->colors[i] = default_colors[i];
}

void
easel_palette_parse (const char *text, size_t len, EaselPalette *p)
{
  EaselPalette parsed;

  easel_palette_defaults (&parsed);
  if (!easel_ini_parse (text, len, apply, &parsed))
    {
      easel_palette_defaults (p);
      return;
    }
  *p = parsed;
}

char *
easel_palette_format (const EaselPalette *p)
{
  /* "[palette]\n" plus at most "color28=#RRGGBBAA\n" (18 bytes) per entry. */
  size_t cap = 10 + EASEL_PALETTE_SIZE * 18 + 1;
  char *text = malloc (cap);
  size_t len;

  if (text == NULL)
    return NULL;
  len = (size_t) snprintf (text, cap, "[" SECTION "]\n");
  for (int i = 0; i < EASEL_PALETTE_SIZE; i++)
    {
      uint32_t c = p->colors[i];

      if ((c >> 24) == 0xFF)
        len += (size_t) snprintf (text + len, cap - len, "color%d=#%06X\n", i + 1, (unsigned) (c & 0xFFFFFF));
      else
        len += (size_t) snprintf (text + len, cap - len, "color%d=#%06X%02X\n", i + 1, (unsigned) (c & 0xFFFFFF),
                                  (unsigned) (c >> 24));
    }
  return text;
}
