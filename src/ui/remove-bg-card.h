/* SPDX-License-Identifier: GPL-3.0-or-later */
#pragma once

#include <adwaita.h>

G_BEGIN_DECLS

/* The number shown changed (its preview is due). */
typedef void (*EaselRemoveBgChanged) (int tolerance, gpointer data);
/* The card's answer: ok TRUE for OK (the number shown), FALSE for Cancel, Esc and a forced end (a quit). Called once. */
typedef void (*EaselRemoveBgClosed) (gboolean ok, int tolerance, gpointer data);

/* The card "Remove Background" (REQ-0035-04): a floating card for a corner of the canvas area, in the look of the font card, with
   the line "Works best on a plain background.", for a complex background a second line (REQ-0035-04), one row "Tolerance" from 1 to
   40 starting at automatic, and Cancel and OK. Enter in the field takes the typed number, then OK. The caller places it and removes it
   after the answer. */
GtkWidget *easel_remove_bg_card_new (int automatic, gboolean complex, EaselRemoveBgChanged changed, EaselRemoveBgClosed closed,
                                     gpointer data);

/* Answers as its buttons do (Esc: FALSE; a quit: FALSE); nothing once answered. */
void easel_remove_bg_card_answer (GtkWidget *card, gboolean ok);

/* The brush the card's toggles set (REQ-0038-01): none, Keep this or Remove this. */
typedef enum
{
  EASEL_RBG_BRUSH_NONE,
  EASEL_RBG_BRUSH_KEEP,
  EASEL_RBG_BRUSH_REMOVE
} EaselRbgBrush;

typedef void (*EaselRemoveBgBrushChanged) (EaselRbgBrush brush, gpointer data);

/* Tells changed each time the brush changes by a toggle (not when it is set by easel_remove_bg_card_set_brush). */
void easel_remove_bg_card_on_brush (GtkWidget *card, EaselRemoveBgBrushChanged changed, gpointer data);

/* The brush on. */
EaselRbgBrush easel_remove_bg_card_brush (GtkWidget *card);

/* While held (a stroke in progress), a click or a mnemonic on a toggle changes nothing (REQ-0038-01). */
void easel_remove_bg_card_hold_brushes (GtkWidget *card, gboolean held);

/* Takes a number typed in the field, as leaving the field does (REQ-0037-05). */
void easel_remove_bg_card_commit (GtkWidget *card);

/* Its OK button, the last of its controls (REQ-0037-03: Tab from it goes to the canvas). */
GtkWidget *easel_remove_bg_card_ok_button (GtkWidget *card);

/* The row, for the tests and the focus. */
AdwSpinRow *easel_remove_bg_card_row (GtkWidget *card);

G_END_DECLS
