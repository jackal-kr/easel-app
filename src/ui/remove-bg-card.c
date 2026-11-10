/* SPDX-License-Identifier: GPL-3.0-or-later */

#include "remove-bg-card.h"

#include "i18n.h"
#include "remove-bg.h"
#include "unit-row.h"

#define DATA_KEY "easel-remove-bg-card"

typedef struct
{
  GtkWidget *card;
  AdwSpinRow *row;
  gboolean done;
  EaselRemoveBgChanged changed;
  EaselRemoveBgClosed closed;
  gpointer data;
} RemoveBgCard;

static void
on_value (RemoveBgCard *c)
{
  if (!c->done)
    c->changed ((int) adw_spin_row_get_value (c->row), c->data);
}

/* Every answer ends here, once: OK takes a typed number first. */
static void
finish (RemoveBgCard *c, gboolean ok)
{
  if (c->done)
    return;
  if (ok)
    adw_spin_row_update (c->row);
  c->done = TRUE;
  c->closed (ok, (int) adw_spin_row_get_value (c->row), c->data);
}

static void
on_ok (RemoveBgCard *c)
{
  finish (c, TRUE);
}

static void
on_cancel (RemoveBgCard *c)
{
  finish (c, FALSE);
}

static void
free_card (gpointer data, GObject *where_the_card_was)
{
  g_free (data);
}

GtkWidget *
easel_remove_bg_card_new (int automatic, gboolean complex, EaselRemoveBgChanged changed, EaselRemoveBgClosed closed, gpointer data)
{
  RemoveBgCard *c = g_new0 (RemoveBgCard, 1);
  GtkWidget *box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 8), *list = gtk_list_box_new ();
  GtkWidget *heading = gtk_label_new (_ ("Remove Background")), *line = gtk_label_new (_ ("Works best on a plain background."));
  GtkWidget *buttons = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 8);
  GtkWidget *cancel = gtk_button_new_with_mnemonic (_ ("_Cancel")), *ok = gtk_button_new_with_mnemonic (_ ("_OK"));

  c->changed = changed;
  c->closed = closed;
  c->data = data;
  c->card = box;
  gtk_widget_set_name (box, "rbg-card");
  gtk_widget_add_css_class (box, "card");
  gtk_widget_add_css_class (box, "font-card");               /* the font card's and the thumbnail's look (REQ-0035-04) */
  gtk_widget_add_css_class (box, "rbg-card");
  gtk_widget_set_size_request (box, 280, -1);
  gtk_widget_add_css_class (heading, "heading");
  gtk_label_set_xalign (GTK_LABEL (heading), 0);
  gtk_label_set_xalign (GTK_LABEL (line), 0);
  gtk_label_set_wrap (GTK_LABEL (line), TRUE);
  gtk_widget_add_css_class (line, "dim-label");
  gtk_list_box_set_selection_mode (GTK_LIST_BOX (list), GTK_SELECTION_NONE);
  gtk_widget_add_css_class (list, "boxed-list");
  c->row = easel_unit_row_new (_ ("Tolerance"), EASEL_RBG_TOLERANCE_MIN, EASEL_RBG_TOLERANCE_MAX, automatic, NULL);
  gtk_widget_set_name (GTK_WIDGET (c->row), "rbg-tolerance");
  gtk_list_box_append (GTK_LIST_BOX (list), GTK_WIDGET (c->row));
  gtk_widget_set_halign (buttons, GTK_ALIGN_END);
  gtk_widget_add_css_class (ok, "suggested-action");
  gtk_box_append (GTK_BOX (buttons), cancel);
  gtk_box_append (GTK_BOX (buttons), ok);
  gtk_box_append (GTK_BOX (box), heading);
  gtk_box_append (GTK_BOX (box), line);
  if (complex)
    {
      /* REQ-0035-04, for a background the method explains poorly; "Tolerance" is the row's name, "pieces" the parts a click flips */
      GtkWidget *hint = gtk_label_new (_ ("This background is complex. Lower the Tolerance, then click pieces to fix them."));

      gtk_widget_set_name (hint, "rbg-complex");
      gtk_label_set_xalign (GTK_LABEL (hint), 0);
      gtk_label_set_wrap (GTK_LABEL (hint), TRUE);
      gtk_widget_add_css_class (hint, "dim-label");
      gtk_box_append (GTK_BOX (box), hint);
    }
  gtk_box_append (GTK_BOX (box), list);
  gtk_box_append (GTK_BOX (box), buttons);
  g_signal_connect_swapped (c->row, "notify::value", G_CALLBACK (on_value), c);
  g_signal_connect_swapped (gtk_editable_get_delegate (GTK_EDITABLE (c->row)), "activate", G_CALLBACK (on_ok), c);   /* Enter */
  g_signal_connect_swapped (ok, "clicked", G_CALLBACK (on_ok), c);
  g_signal_connect_swapped (cancel, "clicked", G_CALLBACK (on_cancel), c);
  g_object_set_data (G_OBJECT (box), DATA_KEY, c);
  g_object_weak_ref (G_OBJECT (box), free_card, c);
  return box;
}

void
easel_remove_bg_card_answer (GtkWidget *card, gboolean ok)
{
  RemoveBgCard *c = g_object_get_data (G_OBJECT (card), DATA_KEY);

  if (c != NULL)
    finish (c, ok);
}

AdwSpinRow *
easel_remove_bg_card_row (GtkWidget *card)
{
  RemoveBgCard *c = g_object_get_data (G_OBJECT (card), DATA_KEY);

  return c != NULL ? c->row : NULL;
}
