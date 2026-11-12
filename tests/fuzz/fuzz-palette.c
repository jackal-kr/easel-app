/* SPDX-License-Identifier: GPL-3.0-or-later */
/* libFuzzer harness: a palette file's text, and a color value as the ini reader takes it. */
#include <stdint.h>
#include "ini.h"
#include "palette.h"

int
LLVMFuzzerTestOneInput (const uint8_t *data, size_t size)
{
  EaselPalette p;
  uint32_t c;

  easel_palette_parse ((const char *) data, size, &p);
  (void) easel_ini_parse_color ((EaselSpan) { (const char *) data, size }, &c);
  return 0;
}
