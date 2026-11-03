/* SPDX-License-Identifier: GPL-3.0-or-later */
#pragma once

#include <adwaita.h>

G_BEGIN_DECLS

/* A number field with a minus and a plus button that shows its unit (REQ-0019-03, REQ-0020-06): whole numbers from lower to upper, by 1, by 10 with the
   Page keys. A typed value may have the unit after it, or none; a number past a limit becomes the limit; any other text goes back to the value
   the field had. A NULL unit shows the number alone; "°" takes no space before it. */
AdwSpinRow *easel_unit_row_new (const char *title, int lower, int upper, int value, const char *unit);
/* Another unit for the field, shown at once. The caller changes the range and the value as it needs. */
void easel_unit_row_set_unit (AdwSpinRow *row, const char *unit);
/* The wheel changes the value only while the field has the focus; otherwise it goes on to what holds the field (REQ-0020-06). */
void easel_unit_row_wheel_with_focus (AdwSpinRow *row);

G_END_DECLS
