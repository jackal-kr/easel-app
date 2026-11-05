/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Rich text on the clipboard (feature 0017, REQ-0017-19, REQ-0017-20): a box's text and runs written as an HTML fragment, and the
   HTML other apps offer (LibreOffice Writer, browsers) read back into text and runs. A subset: what a box can hold (family, size, color,
   bold, italic, underline, strikethrough, line breaks); links, pictures, tables, highlights and alignment are dropped, their text kept.
   Pure C. */
#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "text-buffer.h"

/* A run of read text: the characters from `start` to the next run's start. A field the HTML does not give is left unset (family NULL,
   size_pt 0, has_color false), for the box to take from the style at the caret; a style flag not given is off, as in HTML. */
typedef struct
{
  size_t start;
  const char *family;     /* owned by the EaselRichText; NULL: not given */
  int size_pt;            /* whole points, 1..500; 0: not given */
  bool has_color;
  uint32_t color;         /* 0xAARRGGBB */
  uint8_t flags;          /* EASEL_TEXT_BOLD, ... */
} EaselRichRun;

typedef struct
{
  char *text;             /* UTF-8, NUL-terminated; '\n' between lines */
  size_t len;
  EaselRichRun *runs;
  size_t n;
  char **families;        /* the family names the runs point to */
  size_t n_families;
} EaselRichText;

/* Reads `len` bytes of HTML. False, with *out empty, without memory; HTML it cannot make sense of gives what text it finds. */
bool easel_rich_text_from_html (const char *html, size_t len, EaselRichText *out);
void easel_rich_text_free (EaselRichText *rt);

/* The HTML fragment Copy and Cut offer for `len` bytes of `text` with `runs` (as a text buffer keeps them): one span per run with
   inline font-family, font-size in points, color, font-weight, font-style and text-decoration; '\n' as <br>. NULL without memory;
   *out_len gets its length. */
char *easel_rich_text_to_html (const char *text, size_t len, const EaselTextRun *runs, size_t n, size_t *out_len);
