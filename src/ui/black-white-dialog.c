/* SPDX-License-Identifier: GPL-3.0-or-later */

#include "black-white-dialog.h"

#include <adwaita.h>

#include "i18n.h"

typedef struct
{
  AdwAlertDialog *dialog;
  GtkCheckButton *choice[3];
  EaselBlackWhiteApplied applied;
  gpointer data;
} BlackWhiteDialog;

static EaselBw
chosen (const BlackWhiteDialog *d)
{
  for (int i = 0; i < 3; i++)
    if (gtk_check_button_get_active (d->choice[i]))
      return (EaselBw) i;
  return EASEL_BW_THRESHOLD;
}

static void
on_response (AdwAlertDialog *dialog, const char *response, BlackWhiteDialog *d)
{
  if (g_strcmp0 (response, "convert") == 0)
    d->applied (chosen (d), d->data);
}

static void
free_dialog (gpointer data, GObject *where_the_dialog_was)
{
  g_free (data);
}

/* A choice: its name, and its line under it in the dim style (REQ-0034-02). */
static GtkCheckButton *
add_choice (GtkWidget *box, const char *name, const char *title, const char *line, GtkCheckButton *group)
{
  GtkWidget *button = gtk_check_button_new (), *labels = gtk_box_new (GTK_ORIENTATION_VERTICAL, 2);
  GtkWidget *t = gtk_label_new (title), *l = gtk_label_new (line);

  gtk_widget_set_name (button, name);
  gtk_label_set_xalign (GTK_LABEL (t), 0);
  gtk_label_set_xalign (GTK_LABEL (l), 0);
  gtk_widget_add_css_class (l, "dim-label");
  gtk_widget_add_css_class (l, "caption");
  gtk_box_append (GTK_BOX (labels), t);
  gtk_box_append (GTK_BOX (labels), l);
  gtk_check_button_set_child (GTK_CHECK_BUTTON (button), labels);
  gtk_accessible_update_property (GTK_ACCESSIBLE (button), GTK_ACCESSIBLE_PROPERTY_LABEL, title,
                                  GTK_ACCESSIBLE_PROPERTY_DESCRIPTION, line, -1);
  if (group != NULL)
    gtk_check_button_set_group (GTK_CHECK_BUTTON (button), group);
  gtk_box_append (GTK_BOX (box), button);
  return GTK_CHECK_BUTTON (button);
}

AdwDialog *
easel_black_white_dialog_new (gboolean selection, EaselBlackWhiteApplied applied, gpointer data)
{
  BlackWhiteDialog *d = g_new0 (BlackWhiteDialog, 1);
  GtkWidget *box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);

  d->applied = applied;
  d->data = data;
  d->dialog = ADW_ALERT_DIALOG (adw_alert_dialog_new (_ ("Black and White"),
                                                      selection ? _ ("Turns the selection to black and white.")
                                                                : _ ("Turns the picture to black and white.")));
  d->choice[0] = add_choice (box, "bw-threshold", _ ("Threshold"), _ ("Hard edges, no dots"), NULL);
  d->choice[1] = add_choice (box, "bw-ordered", _ ("Ordered dither"), _ ("Grays as an even pattern"), d->choice[0]);
  d->choice[2] = add_choice (box, "bw-diffusion", _ ("Error diffusion"), _ ("Most like a photo"), d->choice[0]);
  gtk_check_button_set_active (d->choice[0], TRUE);          /* Threshold each time it opens (REQ-0034-02) */
  adw_alert_dialog_set_extra_child (d->dialog, box);
  adw_alert_dialog_add_responses (d->dialog, "cancel", _ ("_Cancel"), "convert", _ ("Con_vert"), NULL);
  adw_alert_dialog_set_response_appearance (d->dialog, "convert", ADW_RESPONSE_SUGGESTED);
  adw_alert_dialog_set_default_response (d->dialog, "convert");
  adw_alert_dialog_set_close_response (d->dialog, "cancel");
  g_signal_connect (d->dialog, "response", G_CALLBACK (on_response), d);
  g_object_weak_ref (G_OBJECT (d->dialog), free_dialog, d);
  adw_dialog_set_focus (ADW_DIALOG (d->dialog), GTK_WIDGET (d->choice[0]));   /* the focus on the chosen one */
  return ADW_DIALOG (d->dialog);
}
