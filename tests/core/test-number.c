/* SPDX-License-Identifier: GPL-3.0-or-later */
/* The whole-number fields of the Grid and Attributes dialogs (REQ-0004-13, REQ-0004-21). */

#include "check.h"
#include "number.h"

static void
field_accepts_whole_numbers (void)
{
  int v = -1;

  CHECK (easel_parse_field ("7", 1, 1000, &v) && v == 7);
  CHECK (easel_parse_field ("007", 1, 1000, &v) && v == 7);
  CHECK (easel_parse_field (" 16 ", 1, 1000, &v) && v == 16);
  CHECK (easel_parse_field ("\t16\r", 1, 1000, &v) && v == 16);
  CHECK (easel_parse_field ("1", 1, 1000, &v) && v == 1);
  CHECK (easel_parse_field ("1000", 1, 1000, &v) && v == 1000);
  CHECK (easel_parse_field ("10000", 1, 10000, &v) && v == 10000);
  CHECK (easel_parse_field ("0", 0, 10, &v) && v == 0);
}

static void
field_refuses_the_rest (void)
{
  int v = 42;

  CHECK (!easel_parse_field ("", 1, 1000, &v));
  CHECK (!easel_parse_field ("   ", 1, 1000, &v));
  CHECK (!easel_parse_field ("0", 1, 1000, &v));
  CHECK (!easel_parse_field ("1001", 1, 1000, &v));
  CHECK (!easel_parse_field ("10001", 1, 10000, &v));
  CHECK (!easel_parse_field ("-3", 1, 1000, &v));
  CHECK (!easel_parse_field ("+3", 1, 1000, &v));
  CHECK (!easel_parse_field ("abc", 1, 1000, &v));
  CHECK (!easel_parse_field ("1 2", 1, 1000, &v));
  CHECK (!easel_parse_field ("12abc", 1, 1000, &v));
  CHECK (!easel_parse_field ("1.5", 1, 1000, &v));
  CHECK (!easel_parse_field ("99999999999999999999", 1, 1000, &v));
  CHECK (!easel_parse_field (NULL, 1, 1000, &v));
  /* A refused text leaves the output alone. */
  CHECK (v == 42);
}

int
main (void)
{
  RUN (field_accepts_whole_numbers);
  RUN (field_refuses_the_rest);
  return 0;
}
