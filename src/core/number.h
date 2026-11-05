/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Whole-number fields: the Grid and Attributes dialogs (REQ-0004-13, REQ-0004-21) and the `grid-size` setting. */
#pragma once

#include <stdbool.h>

/* A text of digits only, with spaces and tabs around it trimmed ("007" is 7), from min to max. Anything else (empty,
   a sign, a decimal point, letters, a value outside the range or too big for an int) is refused, and *out is left
   alone. */
bool easel_parse_field (const char *text, int min, int max, int *out);
