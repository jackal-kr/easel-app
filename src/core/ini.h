/* SPDX-License-Identifier: GPL-3.0-or-later */
/* The syntax shared by the hand-editable files (REQ-0001-40): `[section]` headers, `key=value` lines, `#` comments,
   spaces around `=` and the value trimmed. Any other non-empty line makes the whole text unparseable. */
#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* A piece of the text: not NUL-terminated. */
typedef struct
{
  const char *p;
  size_t n;
} EaselSpan;

bool easel_span_is (EaselSpan s, const char *word);
/* The piece without the spaces, tabs and carriage returns at its two ends. */
EaselSpan easel_span_trim (EaselSpan s);
bool easel_span_is_nocase (EaselSpan s, const char *lower_word);

/* Called for each key line, in order. section is empty before the first header. */
typedef void (*EaselIniFunc) (EaselSpan section, EaselSpan key, EaselSpan value, void *data);

/* Walks text (len bytes, may contain NULs). Returns false if the text is unparseable; func may already have been called
   for earlier lines, so the caller discards what it collected. */
bool easel_ini_parse (const char *text, size_t len, EaselIniFunc func, void *data);

/* `#RRGGBB`, `#RRGGBBAA` or `transparent` (hex digits and the word in any case) into straight-alpha 0xAARRGGBB
   (REQ-0001-03). */
bool easel_ini_parse_color (EaselSpan v, uint32_t *out);
