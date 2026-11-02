/* SPDX-License-Identifier: GPL-3.0-or-later */

#include "stretch-skew-dialog.h"

#include <adwaita.h>
#include <string.h>

#include "stretch-skew.h"

typedef struct
{
  AdwAlertDialog *dialog;
  AdwSpinRow *width, *height, *horizontal, *vertical;
  GtkToggleButton *stretch_percent, *skew_degrees;
  GtkCheckButton *keep;
  GtkLabel *size;
  int w, h;
  gboolean updating;                    /* a change made here: it does not start another */
  EaselStretchSkewApplied applied;
  gpointer data;
} StretchSkewDialog;

static int on_output (AdwSpinRow *row, StretchSkewDialog *d);

static int
value_of (AdwSpinRow *row)
{
  return (int) adw_spin_row_get_value (row);
}

/* The stretch's size, not limited: a percent can ask for more than a picture may have (REQ-0019-07). */
static void
stretched (StretchSkewDialog *d, int *W, int *H)
{
  gboolean percent = gtk_toggle_button_get_active (d->stretch_percent);

  *W = percent ? easel_ss_percent_unlimited (d->w, value_of (d->width)) : value_of (d->width);
  *H = percent ? easel_ss_percent_unlimited (d->h, value_of (d->height)) : value_of (d->height);
}

static void
result_of (StretchSkewDialog *d, int *W, int *H, int *out_w, int *out_h, int *amount_h, int *amount_v)
{
  gboolean degrees = gtk_toggle_button_get_active (d->skew_degrees);

  stretched (d, W, H);
  easel_ss_size (*W, *H, value_of (d->horizontal), degrees, value_of (d->vertical), degrees, out_w, out_h, amount_h, amount_v);
}

/* The New size line, and OK, which is off past the limit (REQ-0019-07). */
static void
refresh (StretchSkewDialog *d)
{
  int W, H, out_w, out_h, amount_h, amount_v;
  g_autofree char *text = NULL;
  gboolean too_large;

  result_of (d, &W, &H, &out_w, &out_h, &amount_h, &amount_v);
  too_large = easel_ss_too_large (out_w, out_h);
  text = too_large ? g_strdup_printf ("Too large: %d × %d; the most is 10000 × 10000", out_w, out_h)
                   : g_strdup_printf ("New size: %d × %d", out_w, out_h);
  gtk_label_set_text (d->size, text);
  adw_alert_dialog_set_response_enabled (d->dialog, "ok", !too_large);
}

/* With the ratio kept, the other side follows `changed`; in pixels `changed` stops where the other stays within its limit (REQ-0019-05). */
static void
follow (StretchSkewDialog *d, AdwSpinRow *changed, AdwSpinRow *other, int from, int other_from)
{
  d->updating = TRUE;
  if (gtk_toggle_button_get_active (d->stretch_percent))
    adw_spin_row_set_value (other, value_of (changed));
  else
    {
      int fit = easel_ss_ratio_fit (value_of (changed), from, other_from);

      adw_spin_row_set_value (changed, fit);
      adw_spin_row_set_value (other, easel_ss_ratio_other (fit, from, other_from));
    }
  d->updating = FALSE;
}

static void
on_stretch_changed (AdwSpinRow *row, GParamSpec *pspec, StretchSkewDialog *d)
{
  if (d->updating)
    return;
  if (gtk_check_button_get_active (d->keep))
    {
      if (row == d->width)
        follow (d, d->width, d->height, d->w, d->h);
      else
        follow (d, d->height, d->width, d->h, d->w);
    }
  refresh (d);
}

static void
on_skew_changed (StretchSkewDialog *d)
{
  if (!d->updating)
    refresh (d);
}

static void
on_keep_toggled (StretchSkewDialog *d)
{
  if (gtk_check_button_get_active (d->keep))
    follow (d, d->width, d->height, d->w, d->h);
  refresh (d);
}

static void
set_range (AdwSpinRow *row, int lower, int upper)
{
  GtkAdjustment *adj = adw_spin_row_get_adjustment (row);

  gtk_adjustment_set_lower (adj, lower);
  gtk_adjustment_set_upper (adj, upper);
}

/* Stretch's other unit: each field converted, within the new limits; with the ratio kept, Height from Width (REQ-0019-04). */
static void
on_stretch_unit (GtkToggleButton *percent, StretchSkewDialog *d)
{
  int width = value_of (d->width), height = value_of (d->height);

  d->updating = TRUE;
  if (gtk_toggle_button_get_active (percent))
    {
      width = easel_ss_to_percent (width, d->w);
      height = easel_ss_to_percent (height, d->h);
      set_range (d->width, 1, 500);
      set_range (d->height, 1, 500);
    }
  else
    {
      width = easel_ss_percent (d->w, width);
      height = easel_ss_percent (d->h, height);
      set_range (d->width, 1, EASEL_SS_MAX_SIZE);
      set_range (d->height, 1, EASEL_SS_MAX_SIZE);
    }
  adw_spin_row_set_value (d->width, width);
  adw_spin_row_set_value (d->height, height);
  d->updating = FALSE;
  if (gtk_check_button_get_active (d->keep))
    follow (d, d->width, d->height, d->w, d->h);
  on_output (d->width, d);                  /* the same number in the other unit changes no value, and so no text by itself */
  on_output (d->height, d);
  refresh (d);
}

/* Skew's other unit: Horizontal against the height at that step, Vertical against the width after the horizontal growth (REQ-0019-04). */
static void
on_skew_unit (GtkToggleButton *degrees, StretchSkewDialog *d)
{
  gboolean to_degrees = gtk_toggle_button_get_active (degrees);
  int W, H, horizontal = value_of (d->horizontal), vertical = value_of (d->vertical), amount, step_w;

  stretched (d, &W, &H);
  W = MIN (W, EASEL_SS_MAX_SIZE);
  H = MIN (H, EASEL_SS_MAX_SIZE);
  /* The horizontal amount as it stands, in the old unit, gives the width the vertical skew sees. */
  amount = H <= 1 ? 0 : to_degrees ? horizontal : easel_ss_skew_amount (H, horizontal);
  step_w = W + ABS (amount);
  d->updating = TRUE;
  if (to_degrees)
    {
      set_range (d->horizontal, -89, 89);
      set_range (d->vertical, -89, 89);
      adw_spin_row_set_value (d->horizontal, easel_ss_skew_degrees (horizontal, H));
      adw_spin_row_set_value (d->vertical, easel_ss_skew_degrees (vertical, step_w));
    }
  else
    {
      set_range (d->horizontal, -9999, 9999);
      set_range (d->vertical, -9999, 9999);
      adw_spin_row_set_value (d->horizontal, CLAMP (easel_ss_skew_amount (H, horizontal), -9999, 9999));
      adw_spin_row_set_value (d->vertical, CLAMP (easel_ss_skew_amount (step_w, vertical), -9999, 9999));
    }
  d->updating = FALSE;
  on_output (d->horizontal, d);
  on_output (d->vertical, d);
  refresh (d);
}

/* The unit a field shows now (REQ-0019-03). */
static const char *
unit_of (AdwSpinRow *row, StretchSkewDialog *d)
{
  if (row == d->width || row == d->height)
    return gtk_toggle_button_get_active (d->stretch_percent) ? "%" : "px";
  return gtk_toggle_button_get_active (d->skew_degrees) ? "°" : "px";
}

/* The value with its unit: "100 %", "800 px", "45°". */
static int
on_output (AdwSpinRow *row, StretchSkewDialog *d)
{
  const char *unit = unit_of (row, d);
  g_autofree char *text = g_strdup_printf (g_str_equal (unit, "°") ? "%d%s" : "%d %s", value_of (row), unit);

  gtk_editable_set_text (GTK_EDITABLE (row), text);
  return TRUE;
}

/* A typed number stops at the field's limits; text that is not a whole number, with or without the field's own unit after it, goes back to
   the old value (REQ-0019-03, as Attributes). */
static int
on_input (AdwSpinRow *row, double *new_value, StretchSkewDialog *d)
{
  GtkAdjustment *adj = adw_spin_row_get_adjustment (row);
  g_autofree char *trimmed = g_strstrip (g_strdup (gtk_editable_get_text (GTK_EDITABLE (row))));
  const char *unit = unit_of (row, d);
  char *end = NULL;
  gint64 n = g_ascii_strtoll (trimmed, &end, 10);   /* saturates on overflow, so a huge number stops at the limit */

  while (*end == ' ')
    end++;
  if (end != trimmed && g_str_equal (end, unit))
    end += strlen (unit);
  if (trimmed[0] == '\0' || end == trimmed || *end != '\0')
    *new_value = adw_spin_row_get_value (row);
  else
    *new_value = CLAMP ((double) n, gtk_adjustment_get_lower (adj), gtk_adjustment_get_upper (adj));
  return TRUE;
}

/* Typed text counts once it is committed, as when the field is left or Enter is pressed (REQ-0019-02). */
static void
commit_fields (StretchSkewDialog *d)
{
  adw_spin_row_update (d->width);
  adw_spin_row_update (d->height);
  adw_spin_row_update (d->horizontal);
  adw_spin_row_update (d->vertical);
}

static void
apply (StretchSkewDialog *d)
{
  int W, H, out_w, out_h, amount_h, amount_v;

  result_of (d, &W, &H, &out_w, &out_h, &amount_h, &amount_v);
  if (!easel_ss_too_large (out_w, out_h))
    d->applied (W, H, amount_h, amount_v, d->data);
}

static void
on_response (AdwAlertDialog *dialog, const char *response, StretchSkewDialog *d)
{
  if (g_strcmp0 (response, "ok") != 0)
    return;
  commit_fields (d);
  apply (d);
}

/* Enter: the typed value first, then OK only if OK is on; otherwise the dialog stays open (REQ-0019-02). */
static void
on_field_activated (StretchSkewDialog *d)
{
  EaselStretchSkewApplied applied = d->applied;
  gpointer data = d->data;
  int W, H, out_w, out_h, amount_h, amount_v;

  commit_fields (d);
  if (!adw_alert_dialog_get_response_enabled (d->dialog, "ok"))
    return;
  result_of (d, &W, &H, &out_w, &out_h, &amount_h, &amount_v);
  adw_dialog_close (ADW_DIALOG (d->dialog));          /* it may be gone after this */
  applied (W, H, amount_h, amount_v, data);
}

static void
free_dialog (gpointer data, GObject *where_the_dialog_was)
{
  g_free (data);
}

static AdwSpinRow *
add_row (GtkWidget *list, const char *title, int lower, int upper, int value, StretchSkewDialog *d)
{
  GtkWidget *row = adw_spin_row_new_with_range (lower, upper, 1);

  adw_preferences_row_set_title (ADW_PREFERENCES_ROW (row), title);
  adw_spin_row_set_numeric (ADW_SPIN_ROW (row), FALSE);       /* a numeric field refuses the unit's text; on_input judges what is typed */
  gtk_adjustment_set_page_increment (adw_spin_row_get_adjustment (ADW_SPIN_ROW (row)), 10);
  adw_spin_row_set_value (ADW_SPIN_ROW (row), value);
  g_signal_connect (row, "input", G_CALLBACK (on_input), d);
  g_signal_connect (row, "output", G_CALLBACK (on_output), d);
  g_signal_connect_swapped (gtk_editable_get_delegate (GTK_EDITABLE (row)), "activate", G_CALLBACK (on_field_activated), d);
  gtk_list_box_append (GTK_LIST_BOX (list), row);
  return ADW_SPIN_ROW (row);
}

/* A section's heading and its unit's two buttons, the first one on. */
static GtkToggleButton *
add_units (GtkWidget *box, const char *heading, const char *first, const char *first_name, const char *second, const char *second_name)
{
  GtkWidget *title = gtk_label_new (heading);
  GtkWidget *row = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
  GtkWidget *a = gtk_toggle_button_new_with_label (first), *b = gtk_toggle_button_new_with_label (second);

  gtk_widget_set_halign (title, GTK_ALIGN_START);
  gtk_widget_add_css_class (title, "heading");
  gtk_widget_add_css_class (row, "linked");
  gtk_widget_set_halign (row, GTK_ALIGN_START);
  gtk_widget_set_name (a, first_name);
  gtk_widget_set_name (b, second_name);
  gtk_toggle_button_set_group (GTK_TOGGLE_BUTTON (b), GTK_TOGGLE_BUTTON (a));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (a), TRUE);
  gtk_box_append (GTK_BOX (row), a);
  gtk_box_append (GTK_BOX (row), b);
  gtk_box_append (GTK_BOX (box), title);
  gtk_box_append (GTK_BOX (box), row);
  return GTK_TOGGLE_BUTTON (a);
}

void
easel_stretch_skew_dialog_present (GtkWidget *parent, int w, int h, EaselStretchSkewApplied applied, gpointer data)
{
  StretchSkewDialog *d = g_new0 (StretchSkewDialog, 1);
  GtkWidget *box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 8);
  GtkWidget *stretch_list = gtk_list_box_new (), *skew_list = gtk_list_box_new ();
  GtkWidget *keep = gtk_check_button_new_with_label ("Keep aspect ratio");
  GtkWidget *size = gtk_label_new (NULL);

  d->w = w;
  d->h = h;
  d->applied = applied;
  d->data = data;
  d->dialog = ADW_ALERT_DIALOG (adw_alert_dialog_new ("Stretch and Skew", NULL));
  gtk_list_box_set_selection_mode (GTK_LIST_BOX (stretch_list), GTK_SELECTION_NONE);
  gtk_list_box_set_selection_mode (GTK_LIST_BOX (skew_list), GTK_SELECTION_NONE);
  gtk_widget_add_css_class (stretch_list, "boxed-list");
  gtk_widget_add_css_class (skew_list, "boxed-list");

  d->stretch_percent = add_units (box, "Stretch", "Percent", "stretch-percent", "Pixels", "stretch-pixels");
  d->width = add_row (stretch_list, "Width", 1, 500, 100, d);
  d->height = add_row (stretch_list, "Height", 1, 500, 100, d);
  gtk_box_append (GTK_BOX (box), stretch_list);
  gtk_check_button_set_active (GTK_CHECK_BUTTON (keep), TRUE);
  d->keep = GTK_CHECK_BUTTON (keep);
  gtk_box_append (GTK_BOX (box), keep);
  d->skew_degrees = add_units (box, "Skew", "Degrees", "skew-degrees", "Pixels", "skew-pixels");
  d->horizontal = add_row (skew_list, "Horizontal", -89, 89, 0, d);
  d->vertical = add_row (skew_list, "Vertical", -89, 89, 0, d);
  gtk_box_append (GTK_BOX (box), skew_list);
  gtk_widget_set_name (size, "new-size");
  gtk_widget_set_halign (size, GTK_ALIGN_START);
  d->size = GTK_LABEL (size);
  gtk_box_append (GTK_BOX (box), size);

  adw_alert_dialog_set_extra_child (d->dialog, box);
  adw_alert_dialog_add_responses (d->dialog, "cancel", "_Cancel", "ok", "_OK", NULL);
  adw_alert_dialog_set_response_appearance (d->dialog, "ok", ADW_RESPONSE_SUGGESTED);
  adw_alert_dialog_set_default_response (d->dialog, "ok");
  adw_alert_dialog_set_close_response (d->dialog, "cancel");

  g_signal_connect (d->width, "notify::value", G_CALLBACK (on_stretch_changed), d);
  g_signal_connect (d->height, "notify::value", G_CALLBACK (on_stretch_changed), d);
  g_signal_connect_swapped (d->horizontal, "notify::value", G_CALLBACK (on_skew_changed), d);
  g_signal_connect_swapped (d->vertical, "notify::value", G_CALLBACK (on_skew_changed), d);
  g_signal_connect_swapped (keep, "toggled", G_CALLBACK (on_keep_toggled), d);
  g_signal_connect (d->stretch_percent, "toggled", G_CALLBACK (on_stretch_unit), d);
  g_signal_connect (d->skew_degrees, "toggled", G_CALLBACK (on_skew_unit), d);
  g_signal_connect (d->dialog, "response", G_CALLBACK (on_response), d);
  g_object_weak_ref (G_OBJECT (d->dialog), free_dialog, d);
  on_output (d->width, d);
  on_output (d->height, d);
  on_output (d->horizontal, d);
  on_output (d->vertical, d);
  refresh (d);
  adw_dialog_present (ADW_DIALOG (d->dialog), parent);
}
