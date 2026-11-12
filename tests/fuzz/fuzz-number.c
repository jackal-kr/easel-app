/* SPDX-License-Identifier: GPL-3.0-or-later */
/* libFuzzer harness: a number typed into a field. */
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "number.h"

int
LLVMFuzzerTestOneInput (const uint8_t *data, size_t size)
{
  char *text = malloc (size + 1);
  int v;

  if (text == NULL)
    return 0;
  memcpy (text, data, size);
  text[size] = '\0';
  (void) easel_parse_field (text, 1, 10000, &v);
  (void) easel_parse_field (text, -2147483647 - 1, 2147483647, &v);
  free (text);
  return 0;
}
