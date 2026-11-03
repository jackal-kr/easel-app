/* SPDX-License-Identifier: GPL-3.0-or-later */

#include "unit-row.h"

#include <string.h>

#define UNIT_KEY "easel-unit"

static int
on_output (AdwSpinRow *row, gpointer data)
{
  const char *unit = g_object_get_data (G_OBJECT (row), UNIT_KEY);
  int value = (int) adw_spin_row_get_value (row);
  g_autofree char *text = unit == NULL ? g_strdup_printf ("%d", value)
                          : g_str_equal (unit, "°") ? g_strdup_printf ("%d%s", value, unit) : g_strdup_printf ("%d %s", value, unit);

  gtk_editable_set_text (GTK_EDITABLE (row), text);
  return TRUE;
}

static int
on_input (AdwSpinRow *row, double *new_value, gpointer data)
{
  GtkAdjustment *adj = adw_spin_row_get_adjustment (row);
  const char *unit = g_object_get_data (G_OBJECT (row), UNIT_KEY);
  g_autofree char *trimmed = g_strstrip (g_strdup (gtk_editable_get_text (GTK_EDITABLE (row))));
  char *end = NULL;
  gint64 n = g_ascii_strtoll (trimmed, &end, 10);   /* saturates on overflow, so a huge number stops at the limit */
  gboolean is_number = end != trimmed;

  while (*end == ' ')
    end++;
  if (is_number && unit != NULL && g_str_equal (end, unit))
    end += strlen (unit);
  if (!is_number || *end != '\0')
    *new_value = adw_spin_row_get_value (row);
  else
    *new_value = CLAMP ((double) n, gtk_adjustment_get_lower (adj), gtk_adjustment_get_upper (adj));
  return TRUE;
}

AdwSpinRow *
easel_unit_row_new (const char *title, int lower, int upper, int value, const char *unit)
{
  AdwSpinRow *row = ADW_SPIN_ROW (adw_spin_row_new_with_range (lower, upper, 1));

  adw_preferences_row_set_title (ADW_PREFERENCES_ROW (row), title);
  adw_spin_row_set_numeric (row, FALSE);       /* a numeric field refuses the unit's text; on_input judges what is typed */
  gtk_adjustment_set_page_increment (adw_spin_row_get_adjustment (row), 10);
  g_object_set_data_full (G_OBJECT (row), UNIT_KEY, g_strdup (unit), g_free);
  g_signal_connect (row, "input", G_CALLBACK (on_input), NULL);
  g_signal_connect (row, "output", G_CALLBACK (on_output), NULL);
  adw_spin_row_set_value (row, value);
  on_output (row, NULL);                       /* a value that did not change gives no output by itself */
  return row;
}

void
easel_unit_row_set_unit (AdwSpinRow *row, const char *unit)
{
  g_object_set_data_full (G_OBJECT (row), UNIT_KEY, g_strdup (unit), g_free);
  on_output (row, NULL);
}

static void
set_wheel_phase (GtkEventController *focus, GtkPropagationPhase phase)
{
  GtkWidget *row = gtk_event_controller_get_widget (focus);
  GListModel *controllers = gtk_widget_observe_controllers (GTK_WIDGET (gtk_editable_get_delegate (GTK_EDITABLE (row))));

  for (guint i = 0; i < g_list_model_get_n_items (controllers); i++)
    {
      g_autoptr (GObject) controller = g_list_model_get_item (controllers, i);

      if (GTK_IS_EVENT_CONTROLLER_SCROLL (controller))
        gtk_event_controller_set_propagation_phase (GTK_EVENT_CONTROLLER (controller), phase);
    }
  g_object_unref (controllers);
}

static void
on_focus_enter (GtkEventController *focus)
{
  set_wheel_phase (focus, GTK_PHASE_BUBBLE);
}

static void
on_focus_leave (GtkEventController *focus)
{
  set_wheel_phase (focus, GTK_PHASE_NONE);
}

void
easel_unit_row_wheel_with_focus (AdwSpinRow *row)
{
  GtkEventController *focus = gtk_event_controller_focus_new ();

  g_signal_connect (focus, "enter", G_CALLBACK (on_focus_enter), NULL);
  g_signal_connect (focus, "leave", G_CALLBACK (on_focus_leave), NULL);
  gtk_widget_add_controller (GTK_WIDGET (row), focus);
}
