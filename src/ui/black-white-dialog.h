/* SPDX-License-Identifier: GPL-3.0-or-later */
#pragma once

#include <adwaita.h>

#include "black-white.h"

G_BEGIN_DECLS

/* Convert: the conversion chosen. */
typedef void (*EaselBlackWhiteApplied) (EaselBw how, gpointer data);

/* The modal dialog "Black and White" (REQ-0034-02), not yet presented, its line naming the selection or the picture: Threshold
   chosen and focused each time; Cancel, Esc, closing and a forced close (a quit, REQ-0034-10) change nothing; Convert calls applied. */
AdwDialog *easel_black_white_dialog_new (gboolean selection, EaselBlackWhiteApplied applied, gpointer data);

G_END_DECLS
