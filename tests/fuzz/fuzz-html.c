/* SPDX-License-Identifier: GPL-3.0-or-later */
/* libFuzzer harness: pasted HTML (another program's clipboard), read as styled text and written back. */
#include <stdint.h>
#include <stdlib.h>
#include "rich-text.h"

int
LLVMFuzzerTestOneInput (const uint8_t *data, size_t size)
{
  EaselRichText rt;

  if (easel_rich_text_from_html ((const char *) data, size, &rt))
    easel_rich_text_free (&rt);
  return 0;
}
