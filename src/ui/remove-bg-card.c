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
  GtkWidget *ok;
  GtkWidget *keep, *remove;
  gboolean setting, held;
  EaselRemoveBgBrushChanged brush_changed;
  gpointer brush_data;
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

/* A toggle clicked (REQ-0038-01): the one turned on turns the other off; at most one is on. While a stroke is held the click is
   undone. */
static void
on_toggled (GtkToggleButton *button, RemoveBgCard *c)
{
  GtkWidget *other = GTK_WIDGET (button) == c->keep ? c->remove : c->keep;

  if (c->setting)
    return;
  c->setting = TRUE;
  if (c->held)
    gtk_toggle_button_set_active (button, !gtk_toggle_button_get_active (button));
  else if (gtk_toggle_button_get_active (button))
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (other), FALSE);
  c->setting = FALSE;
  if (!c->held && c->brush_changed != NULL)
    c->brush_changed (easel_remove_bg_card_brush (c->card), c->brush_data);
}

/* The color dot on a toggle (JOD, 2026-11-10): a disc in the widget's color, which its style class sets (success, error), as the
   bands' (REQ-0038-05). Drawn, so a screen reader does not read it. */
static void
draw_dot (GtkDrawingArea *area, cairo_t *cr, int width, int height, gpointer data)
{
  GdkRGBA c;

  gtk_widget_get_color (GTK_WIDGET (area), &c);
  gdk_cairo_set_source_rgba (cr, &c);
  cairo_arc (cr, width / 2.0, height / 2.0, MIN (width, height) / 2.0, 0, 2 * G_PI);
  cairo_fill (cr);
}

/* A toggle with its color dot and its label (a mnemonic). */
static GtkWidget *
brush_toggle (const char *label, const char *dot_class)
{
  GtkWidget *button = gtk_toggle_button_new (), *box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
  GtkWidget *dot = gtk_drawing_area_new (), *text = gtk_label_new_with_mnemonic (label);

  gtk_widget_set_name (dot, "rbg-dot");
  gtk_drawing_area_set_content_width (GTK_DRAWING_AREA (dot), 10);
  gtk_drawing_area_set_content_height (GTK_DRAWING_AREA (dot), 10);
  gtk_drawing_area_set_draw_func (GTK_DRAWING_AREA (dot), draw_dot, NULL, NULL);
  gtk_widget_set_valign (dot, GTK_ALIGN_CENTER);
  gtk_widget_add_css_class (dot, dot_class);
  gtk_label_set_mnemonic_widget (GTK_LABEL (text), button);
  gtk_widget_set_halign (box, GTK_ALIGN_CENTER);
  gtk_box_append (GTK_BOX (box), dot);
  gtk_box_append (GTK_BOX (box), text);
  gtk_button_set_child (GTK_BUTTON (button), box);
  return button;
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
  gtk_label_set_max_width_chars (GTK_LABEL (line), 40);   /* the card about 380 wide at most, a long line wraps (JOD, 2026-11-10) */
  gtk_widget_add_css_class (line, "dim-label");
  gtk_list_box_set_selection_mode (GTK_LIST_BOX (list), GTK_SELECTION_NONE);
  gtk_widget_add_css_class (list, "boxed-list");
  c->row = easel_unit_row_new (_ ("Tolerance"), EASEL_RBG_TOLERANCE_MIN, EASEL_RBG_TOLERANCE_MAX, automatic, NULL);
  gtk_widget_set_name (GTK_WIDGET (c->row), "rbg-tolerance");
  gtk_list_box_append (GTK_LIST_BOX (list), GTK_WIDGET (c->row));
  gtk_widget_set_halign (buttons, GTK_ALIGN_END);
  gtk_widget_add_css_class (ok, "suggested-action");
  c->ok = ok;
  gtk_box_append (GTK_BOX (buttons), cancel);
  gtk_box_append (GTK_BOX (buttons), ok);
  gtk_box_append (GTK_BOX (box), heading);
  gtk_box_append (GTK_BOX (box), line);
  if (complex)
    {
      /* REQ-0035-04, REQ-0038-10, for a background the method explains poorly; "Tolerance" is the row's name, "Keep this" and "Remove this" the two toggles' */
      GtkWidget *hint = gtk_label_new (_ ("This background is complex. Lower the Tolerance, then use Keep this and Remove this."));

      gtk_widget_set_name (hint, "rbg-complex");
      gtk_label_set_xalign (GTK_LABEL (hint), 0);
      gtk_label_set_wrap (GTK_LABEL (hint), TRUE);
      gtk_label_set_max_width_chars (GTK_LABEL (hint), 40);
      gtk_widget_add_css_class (hint, "dim-label");
      gtk_box_append (GTK_BOX (box), hint);
    }
  gtk_box_append (GTK_BOX (box), list);
  {
    /* Keep this and Remove this, side by side between the row and the buttons (REQ-0038-01), not grouped: both may be off */
    GtkWidget *brushes = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 8);

    c->keep = brush_toggle (_ ("_Keep this"), "success");
    c->remove = brush_toggle (_ ("_Remove this"), "error");
    gtk_widget_set_name (c->keep, "rbg-keep");
    gtk_widget_set_name (c->remove, "rbg-remove");
    gtk_widget_set_tooltip_text (c->keep, _ ("Paint over parts to keep"));
    gtk_widget_set_tooltip_text (c->remove, _ ("Paint over parts to remove"));
    gtk_box_set_homogeneous (GTK_BOX (brushes), TRUE);
    gtk_box_append (GTK_BOX (brushes), c->keep);
    gtk_box_append (GTK_BOX (brushes), c->remove);
    g_signal_connect (c->keep, "toggled", G_CALLBACK (on_toggled), c);
    g_signal_connect (c->remove, "toggled", G_CALLBACK (on_toggled), c);
    gtk_box_append (GTK_BOX (box), brushes);
  }
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

void
easel_remove_bg_card_on_brush (GtkWidget *card, EaselRemoveBgBrushChanged changed, gpointer data)
{
  RemoveBgCard *c = g_object_get_data (G_OBJECT (card), DATA_KEY);

  if (c != NULL)
    c->brush_changed = changed, c->brush_data = data;
}

EaselRbgBrush
easel_remove_bg_card_brush (GtkWidget *card)
{
  RemoveBgCard *c = g_object_get_data (G_OBJECT (card), DATA_KEY);

  if (c == NULL)
    return EASEL_RBG_BRUSH_NONE;
  if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (c->keep)))
    return EASEL_RBG_BRUSH_KEEP;
  return gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (c->remove)) ? EASEL_RBG_BRUSH_REMOVE : EASEL_RBG_BRUSH_NONE;
}

void
easel_remove_bg_card_hold_brushes (GtkWidget *card, gboolean held)
{
  RemoveBgCard *c = g_object_get_data (G_OBJECT (card), DATA_KEY);

  if (c != NULL)
    c->held = held;
}

void
easel_remove_bg_card_commit (GtkWidget *card)
{
  RemoveBgCard *c = g_object_get_data (G_OBJECT (card), DATA_KEY);

  if (c != NULL && !c->done)
    adw_spin_row_update (c->row);
}

GtkWidget *
easel_remove_bg_card_ok_button (GtkWidget *card)
{
  RemoveBgCard *c = g_object_get_data (G_OBJECT (card), DATA_KEY);

  return c != NULL ? c->ok : NULL;
}

AdwSpinRow *
easel_remove_bg_card_row (GtkWidget *card)
{
  RemoveBgCard *c = g_object_get_data (G_OBJECT (card), DATA_KEY);

  return c != NULL ? c->row : NULL;
}
