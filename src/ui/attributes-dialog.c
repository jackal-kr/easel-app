/* SPDX-License-Identifier: GPL-3.0-or-later */

#include "attributes-dialog.h"
#include "image.h"
#include "i18n.h"

typedef struct
{
  AdwAlertDialog *dialog;
  AdwSpinRow *width;
  AdwSpinRow *height;
  int default_width, default_height;
  EaselAttributesApplied applied;
  gpointer data;
} AttributesDialog;

/* The fields hold a size from 1 to 10000 by themselves: a typed number is clamped and other text goes back to the old
   value. Typed text counts once it is committed, as when the field is left or Enter is pressed. */
static void
read_size (AttributesDialog *a, int *width, int *height)
{
  adw_spin_row_update (a->width);
  adw_spin_row_update (a->height);
  *width = (int) adw_spin_row_get_value (a->width);
  *height = (int) adw_spin_row_get_value (a->height);
}

static void
apply (AttributesDialog *a)
{
  int w, h;

  read_size (a, &w, &h);
  a->applied (w, h, a->data);
}

static void
on_response (AdwAlertDialog *dialog, const char *response, AttributesDialog *a)
{
  if (g_strcmp0 (response, "ok") == 0)
    apply (a);
}

/* Enter in a field is OK and closes the dialog. */
static void
on_field_activated (AttributesDialog *a)
{
  apply (a);
  adw_dialog_close (ADW_DIALOG (a->dialog));
}

static void
on_default_clicked (AttributesDialog *a)
{
  adw_spin_row_set_value (a->width, a->default_width);
  adw_spin_row_set_value (a->height, a->default_height);
}

static void
free_attributes_dialog (gpointer data, GObject *where_the_dialog_was)
{
  g_free (data);
}

/* A typed number is clamped to 1 to 10000; text that is not a whole number goes back to the old value, which
   (GTK would take an error as 0 and clamp it to 1, so the old value is handed back instead, REQ-0004-21). */
static int
on_input (AdwSpinRow *row, double *new_value, gpointer data)
{
  const char *text = gtk_editable_get_text (GTK_EDITABLE (row));
  g_autofree char *trimmed = g_strstrip (g_strdup (text));
  char *end = NULL;
  gint64 n;

  n = g_ascii_strtoll (trimmed, &end, 10);   /* saturates on overflow, so a huge number clamps */
  if (trimmed[0] == '\0' || *end != '\0')
    {
      *new_value = adw_spin_row_get_value (row);
      return TRUE;
    }
  *new_value = (double) CLAMP (n, 1, EASEL_IMAGE_MAX_SIZE);
  return TRUE;
}

static AdwSpinRow *
add_row (GtkWidget *list, const char *title, int value)
{
  GtkWidget *row = adw_spin_row_new_with_range (1, EASEL_IMAGE_MAX_SIZE, 1);

  adw_preferences_row_set_title (ADW_PREFERENCES_ROW (row), title);
  adw_spin_row_set_value (ADW_SPIN_ROW (row), value);
  g_signal_connect (row, "input", G_CALLBACK (on_input), NULL);
  gtk_list_box_append (GTK_LIST_BOX (list), row);
  return ADW_SPIN_ROW (row);
}

void
easel_attributes_dialog_present (GtkWidget *parent, int width, int height, int default_width, int default_height,
                                 EaselAttributesApplied applied, gpointer data)
{
  AttributesDialog *a = g_new0 (AttributesDialog, 1);
  GtkWidget *box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 12);
  GtkWidget *list = gtk_list_box_new ();
  GtkWidget *reset = gtk_button_new_with_label (_ ("Default"));

  a->applied = applied;
  a->data = data;
  a->default_width = default_width;
  a->default_height = default_height;
  a->dialog = ADW_ALERT_DIALOG (adw_alert_dialog_new (_ ("Attributes"),
                                                        _ ("New area is filled with the background color; a smaller size crops the picture.")));

  gtk_list_box_set_selection_mode (GTK_LIST_BOX (list), GTK_SELECTION_NONE);
  gtk_widget_add_css_class (list, "boxed-list");
  a->width = add_row (list, _ ("Width"), width);
  a->height = add_row (list, _ ("Height"), height);
  gtk_widget_set_halign (reset, GTK_ALIGN_START);
  gtk_box_append (GTK_BOX (box), list);
  gtk_box_append (GTK_BOX (box), reset);
  adw_alert_dialog_set_extra_child (a->dialog, box);
  adw_alert_dialog_add_responses (a->dialog, "cancel", _ ("_Cancel"), "ok", _ ("_OK"), NULL);
  adw_alert_dialog_set_response_appearance (a->dialog, "ok", ADW_RESPONSE_SUGGESTED);
  adw_alert_dialog_set_default_response (a->dialog, "ok");
  adw_alert_dialog_set_close_response (a->dialog, "cancel");

  /* Enter is the text's "activate"; the row's own "activated" is a click on the row. */
  g_signal_connect_swapped (gtk_editable_get_delegate (GTK_EDITABLE (a->width)), "activate",
                            G_CALLBACK (on_field_activated), a);
  g_signal_connect_swapped (gtk_editable_get_delegate (GTK_EDITABLE (a->height)), "activate",
                            G_CALLBACK (on_field_activated), a);
  g_signal_connect_swapped (reset, "clicked", G_CALLBACK (on_default_clicked), a);
  g_signal_connect (a->dialog, "response", G_CALLBACK (on_response), a);
  g_object_weak_ref (G_OBJECT (a->dialog), free_attributes_dialog, a);
  adw_dialog_present (ADW_DIALOG (a->dialog), parent);
}
