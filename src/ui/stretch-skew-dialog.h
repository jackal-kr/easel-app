/* SPDX-License-Identifier: GPL-3.0-or-later */
#pragma once

#include <gtk/gtk.h>

G_BEGIN_DECLS

/* OK: a stretch to W x H, then the skew's amounts (easel_ss_size). */
typedef void (*EaselStretchSkewApplied) (int W, int H, int amount_h, int amount_v, gpointer data);

/* The modal dialog "Stretch and Skew" (REQ-0019-02 to REQ-0019-05, REQ-0019-07) over parent, of what is w x h: every time at 100 %, 100 %,
   Keep aspect ratio on, 0 degrees, 0 degrees. Esc and Cancel change nothing. OK closes it first and then calls applied. */
void easel_stretch_skew_dialog_present (GtkWidget *parent, int w, int h, EaselStretchSkewApplied applied, gpointer data);

G_END_DECLS
