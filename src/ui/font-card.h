/* SPDX-License-Identifier: GPL-3.0-or-later */
/* The font card of an open text box (feature 0017, REQ-0017-14, REQ-0017-25): one row over the canvas area with the font family, the
   size and Bold, Italic, Underline and Strikethrough. It shows the style of the box's selected text (or at its caret) and sets a field
   of it through the canvas; pressing a toggle leaves the focus in the box, and a field gives it back when it is applied or cancelled.
   The window places it. */
#pragma once

#include <gtk/gtk.h>

#include "canvas.h"

G_BEGIN_DECLS

#define EASEL_TYPE_FONT_CARD (easel_font_card_get_type ())
G_DECLARE_FINAL_TYPE (EaselFontCard, easel_font_card, EASEL, FONT_CARD, GtkBox)

/* The canvas whose box the card edits. */
void easel_font_card_set_canvas (EaselFontCard *self, EaselCanvas *canvas);

/* Shows `style`: a family or size in `mixed` (EaselTextField bits) as an empty field, a flag in it as off. Applies nothing. */
void easel_font_card_show_style (EaselFontCard *self, const EaselTextStyle *style, unsigned mixed);

/* Gives up what is typed in a field and in the family's search: the field shows the box's style again and the focus goes back to the
   box (Esc). */
void easel_font_card_cancel_field (EaselFontCard *self);

/* A key in the family's popover, what its search field's key handler calls: Up and Down move the highlight, Enter picks the highlighted
   family, Esc closes only the popover; FALSE for any other key (it goes to the search). */
gboolean easel_font_card_family_key (EaselFontCard *self, guint keyval);

/* Test hooks (the family's list is a popover, R5): opening it as its button does, and whether it is open; whether it has a search
   field; the search typed; the families listed (the list's own model); the highlighted family, NULL for none; picking one as a press
   on its row does (FALSE if it is not installed); Esc in it; the family button; the family a row's name is written in, or NULL for the
   interface font (a font that cannot show its own name); the button's text ("" when mixed); the size field; a toggle by its flag
   (EaselTextFlags). */
void easel_font_card_open_family (EaselFontCard *self);
gboolean easel_font_card_family_is_open (EaselFontCard *self);
gboolean easel_font_card_has_search (EaselFontCard *self);
void easel_font_card_set_search (EaselFontCard *self, const char *text);
GListModel *easel_font_card_get_listed (EaselFontCard *self);
const char *easel_font_card_get_highlighted (EaselFontCard *self);
gboolean easel_font_card_choose_family (EaselFontCard *self, const char *family);
void easel_font_card_cancel_family (EaselFontCard *self);
GtkWidget *easel_font_card_get_family_button (EaselFontCard *self);
const char *easel_font_card_row_font (EaselFontCard *self, const char *family);
const char *easel_font_card_get_family_text (EaselFontCard *self);
GtkSpinButton *easel_font_card_get_size (EaselFontCard *self);
GtkToggleButton *easel_font_card_get_toggle (EaselFontCard *self, guint8 flag);

G_END_DECLS
