/* SPDX-License-Identifier: GPL-3.0-or-later */
/* The 28 palette colors (REQ-0003-01, REQ-0003-04) and the text of the palette file (REQ-0003-15, REQ-0003-17). */
#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define EASEL_PALETTE_SIZE 28

/* colors[0] is swatch 1: row 1 is swatches 1..14, row 2 is 15..28. Straight-alpha 0xAARRGGBB. */
typedef struct
{
  uint32_t colors[EASEL_PALETTE_SIZE];
} EaselPalette;

void easel_palette_defaults (EaselPalette *p);

/* Section [palette], keys color1..color28. A missing or invalid entry keeps its default; an unparseable text gives
   every default; unknown keys and sections are ignored. */
void easel_palette_parse (const char *text, size_t len, EaselPalette *p);

/* The whole file text, every entry written: #RRGGBB when opaque, #RRGGBBAA otherwise. Free with free(); NULL if
   memory runs out. */
char *easel_palette_format (const EaselPalette *p);

/* The same text written into `text`, a buffer of `cap` bytes. False when it does not fit: nothing is written past the `cap` bytes, and
   when `cap` is above 0 the text in them is ended by a NUL. */
bool easel_palette_format_into (const EaselPalette *p, char *text, size_t cap);
