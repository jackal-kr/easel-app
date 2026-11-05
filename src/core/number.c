/* SPDX-License-Identifier: GPL-3.0-or-later */

#include <stddef.h>
#include <string.h>

#include "number.h"

bool
easel_parse_field (const char *text, int min, int max, int *out)
{
  size_t n;
  long value = 0;

  if (text == NULL)
    return false;
  while (*text == ' ' || *text == '\t' || *text == '\r' || *text == '\n')
    text++;
  n = strlen (text);
  while (n > 0 && (text[n - 1] == ' ' || text[n - 1] == '\t' || text[n - 1] == '\r' || text[n - 1] == '\n'))
    n--;
  if (n == 0)
    return false;
  for (size_t i = 0; i < n; i++)
    {
      if (text[i] < '0' || text[i] > '9')
        return false;
      value = value * 10 + (text[i] - '0');
      /* Stop before the value can overflow: anything this big is out of range anyway. */
      if (value > 1000000000L)
        return false;
    }
  if (value < min || value > max)
    return false;
  *out = (int) value;
  return true;
}
