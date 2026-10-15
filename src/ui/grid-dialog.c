/* SPDX-License-Identifier: GPL-3.0-or-later */

#include "grid-dialog.h"
#include "number.h"
#include "settings.h"

typedef struct
{
  AdwAlertDialog *dialog;
  GtkEditable *entry;
  GtkCheckButton *axonometric;
  EaselGridDialogApplied applied;
  gpointer data;
} GridDialog;

static gboolean
read_cell (GridDialog *g, int *cell)
{
  return easel_parse_field (gtk_editable_get_text (g->entry), EASEL_GRID_SIZE_MIN, EASEL_GRID_SIZE_MAX, cell);
}

/* Apply only while the cell size is a whole number in range. */
static void
on_entry_changed (GridDialog *g)
{
  int cell;

  adw_alert_dialog_set_response_enabled (g->dialog, "apply", read_cell (g, &cell));
}

static void
apply (GridDialog *g)
{
  int cell;

  if (read_cell (g, &cell))
    g->applied (cell, gtk_check_button_get_active (g->axonometric), g->data);
}

static void
on_response (AdwAlertDialog *dialog, const char *response, GridDialog *g)
{
  if (g_strcmp0 (response, "apply") == 0)
    apply (g);
}

/* Enter in the field applies a valid value and closes the dialog; an invalid one does nothing. */
static void
on_entry_activated (GridDialog *g)
{
  int cell;

  if (!read_cell (g, &cell))
    return;
  apply (g);
  adw_dialog_close (ADW_DIALOG (g->dialog));
}

/* The dialog owns the little state it needs; it is freed with the dialog. */
static void
free_grid_dialog (gpointer data, GObject *where_the_dialog_was)
{
  g_free (data);
}

void
easel_grid_dialog_present (GtkWidget *parent, int cell, gboolean axonometric, EaselGridDialogApplied applied,
                           gpointer data)
{
  GridDialog *g = g_new0 (GridDialog, 1);
  GtkWidget *box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 12);
  GtkWidget *list = gtk_list_box_new ();
  g_autofree char *text = g_strdup_printf ("%d", cell);

  g->applied = applied;
  g->data = data;
  g->dialog = ADW_ALERT_DIALOG (adw_alert_dialog_new ("Grid", NULL));

  g->entry = GTK_EDITABLE (adw_entry_row_new ());
  adw_preferences_row_set_title (ADW_PREFERENCES_ROW (g->entry), "Cell size in picture pixels (1 to 1000)");
  gtk_editable_set_text (g->entry, text);
  gtk_list_box_set_selection_mode (GTK_LIST_BOX (list), GTK_SELECTION_NONE);
  gtk_widget_add_css_class (list, "boxed-list");
  gtk_list_box_append (GTK_LIST_BOX (list), GTK_WIDGET (g->entry));

  g->axonometric = GTK_CHECK_BUTTON (gtk_check_button_new_with_label ("Axonometric"));
  gtk_check_button_set_active (g->axonometric, axonometric);

  gtk_box_append (GTK_BOX (box), list);
  gtk_box_append (GTK_BOX (box), GTK_WIDGET (g->axonometric));
  adw_alert_dialog_set_extra_child (g->dialog, box);
  adw_alert_dialog_add_responses (g->dialog, "cancel", "_Cancel", "apply", "_Apply", NULL);
  adw_alert_dialog_set_response_appearance (g->dialog, "apply", ADW_RESPONSE_SUGGESTED);
  adw_alert_dialog_set_default_response (g->dialog, "apply");
  adw_alert_dialog_set_close_response (g->dialog, "cancel");

  g_signal_connect_swapped (g->entry, "changed", G_CALLBACK (on_entry_changed), g);
  g_signal_connect_swapped (g->entry, "entry-activated", G_CALLBACK (on_entry_activated), g);
  g_signal_connect (g->dialog, "response", G_CALLBACK (on_response), g);
  g_object_weak_ref (G_OBJECT (g->dialog), free_grid_dialog, g);
  adw_dialog_present (ADW_DIALOG (g->dialog), parent);
}
