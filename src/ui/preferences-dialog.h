/* SPDX-License-Identifier: GPL-3.0-or-later */
#pragma once

#include "window.h"

G_BEGIN_DECLS

/* The dialog "Preferences" over win (0020, 0027): a change shows at once where it can, and OK or Apply keeps it. */
void easel_preferences_dialog_present (EaselWindow *win);

G_END_DECLS
