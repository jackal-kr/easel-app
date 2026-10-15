/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Palette scenarios of feature 0003. */

#include <stdlib.h>
#include <string.h>

#include "check.h"
#include "palette.h"

static EaselPalette
parse (const char *text)
{
  EaselPalette p;

  easel_palette_parse (text, strlen (text), &p);
  return p;
}

static void
as1_default_palette (void)
{
  /* REQ-0003-04, by swatch number; all opaque. */
  static const uint32_t expected[EASEL_PALETTE_SIZE] = {
    0xFF000000, 0xFF808080, 0xFF800000, 0xFF808000, 0xFF008000, 0xFF008080, 0xFF000080,
    0xFF800080, 0xFF808040, 0xFF004040, 0xFF0080FF, 0xFF004080, 0xFF8000FF, 0xFF804000,
    0xFFFFFFFF, 0xFFC0C0C0, 0xFFFF0000, 0xFFFFFF00, 0xFF00FF00, 0xFF00FFFF, 0xFF0000FF,
    0xFFFF00FF, 0xFFFFFF80, 0xFF00FF80, 0xFF80FFFF, 0xFF8080FF, 0xFFFF0080, 0xFFFF8040,
  };
  EaselPalette p;

  easel_palette_defaults (&p);
  for (int i = 0; i < EASEL_PALETTE_SIZE; i++)
    {
      if (p.colors[i] != expected[i])
        {
          fprintf (stderr, "swatch %d: %08X, want %08X\n", i + 1, (unsigned) p.colors[i], (unsigned) expected[i]);
          exit (1);
        }
    }
  /* Swatch 17 is row 2, column 3: red. */
  CHECK (p.colors[17 - 1] == 0xFFFF0000u);
}

static void
as16_palette_syntax (void)
{
  EaselPalette def, p;

  easel_palette_defaults (&def);
  p = parse ("# note\n"
             "[palette]\n"
             "color3 = #123456\n"
             "color5=abc\n"
             "color40=#FFFFFF\n"
             "color0=#FFFFFF\n"
             "color03=#FFFFFF\n"
             "Color4=#FFFFFF\n"
             "color17=#00ff0080\n"
             "color18=TRANSPARENT\n"
             "color19=#010101\n"
             "color19=#020202\n"
             "[other]\n"
             "color1=#010101\n");
  CHECK (p.colors[3 - 1] == 0xFF123456u);
  CHECK (p.colors[5 - 1] == def.colors[5 - 1]);
  CHECK (p.colors[4 - 1] == def.colors[4 - 1]);
  CHECK (p.colors[17 - 1] == 0x8000FF00u);
  CHECK (p.colors[18 - 1] == 0x00000000u);
  CHECK (p.colors[19 - 1] == 0xFF020202u);
  CHECK (p.colors[1 - 1] == def.colors[1 - 1]);
  for (int i = 0; i < EASEL_PALETTE_SIZE; i++)
    if (i != 3 - 1 && i != 17 - 1 && i != 18 - 1 && i != 19 - 1)
      CHECK (p.colors[i] == def.colors[i]);

  /* A key outside [palette], an invalid repeat (last wins, so the default), and an unparseable file. */
  CHECK (parse ("color3=#123456\n").colors[3 - 1] == def.colors[3 - 1]);
  CHECK (parse ("[palette]\ncolor3=#123456\ncolor3=bad\n").colors[3 - 1] == def.colors[3 - 1]);
  p = parse ("[palette]\ncolor3=#123456\nhello\n");
  CHECK (memcmp (&p, &def, sizeof p) == 0);
  {
    static const char garbage[] = "\x89PNG\r\n\x1a\n\0\0\0\rIHDR";

    easel_palette_parse (garbage, sizeof garbage - 1, &p);
    CHECK (memcmp (&p, &def, sizeof p) == 0);
  }
}

static void
format_round_trip (void)
{
  EaselPalette p, back;
  char *text;

  easel_palette_defaults (&p);
  p.colors[3 - 1] = 0xFF123456u;
  p.colors[17 - 1] = 0x8000FF00u;
  p.colors[18 - 1] = 0x00000000u;
  text = easel_palette_format (&p);
  CHECK (text != NULL);
  CHECK (strncmp (text, "[palette]\ncolor1=#000000\n", 25) == 0);
  CHECK (strstr (text, "\ncolor3=#123456\n") != NULL);
  CHECK (strstr (text, "\ncolor17=#00FF0080\n") != NULL);
  CHECK (strstr (text, "\ncolor18=#00000000\n") != NULL);
  CHECK (strstr (text, "\ncolor28=#FF8040\n") != NULL);

  easel_palette_parse (text, strlen (text), &back);
  CHECK (memcmp (&p, &back, sizeof p) == 0);
  free (text);
}

int
main (void)
{
  RUN (as1_default_palette);
  RUN (as16_palette_syntax);
  RUN (format_round_trip);
  return 0;
}
