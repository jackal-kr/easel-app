/* SPDX-License-Identifier: GPL-3.0-or-later */
#pragma once

#include "window.h"

G_BEGIN_DECLS

/* The dialog "Preferences" over win (REQ-0020-02 to REQ-0020-07): a change applies and is written at once. */
void easel_preferences_dialog_present (EaselWindow *win);

G_END_DECLS
