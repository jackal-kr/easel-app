/* SPDX-License-Identifier: GPL-3.0-or-later */

#include "status-bar.h"

struct _EaselStatusBar
{
  GtkBox parent_instance;
  GtkLabel *labels[EASEL_STATUS_N_FIELDS];
  char *text[EASEL_STATUS_N_FIELDS];      /* what each label shows now */
  guint writes[EASEL_STATUS_N_FIELDS];    /* test hook: how often each label was written */
};

G_DEFINE_FINAL_TYPE (EaselStatusBar, easel_status_bar, GTK_TYPE_BOX)

/* The one place a label is written: only if the text differs from what it shows (REQ-0004-10), so pointer events
   inside one picture pixel, or an unchanged size or zoom, cost nothing. */
static void
set_field (EaselStatusBar *self, EaselStatusField field, const char *text)
{
  if (g_strcmp0 (self->text[field], text) == 0)
    return;
  g_free (self->text[field]);
  self->text[field] = g_strdup (text);
  gtk_label_set_text (self->labels[field], text);
  self->writes[field]++;
}

void
easel_status_bar_set_position (EaselStatusBar *self, gboolean inside, int x, int y)
{
  if (!inside)
    {
      set_field (self, EASEL_STATUS_POSITION, "");
      return;
    }
  {
    g_autofree char *text = g_strdup_printf ("%d, %d", x, y);

    set_field (self, EASEL_STATUS_POSITION, text);
  }
}

void
easel_status_bar_set_size (EaselStatusBar *self, gboolean exists, int width, int height)
{
  g_autofree char *text = exists ? g_strdup_printf ("%d × %d px", width, height) : g_strdup ("");

  set_field (self, EASEL_STATUS_SIZE, text);
}

void
easel_status_bar_set_zoom (EaselStatusBar *self, gboolean exists, int percent)
{
  g_autofree char *text = exists ? g_strdup_printf ("%d%%", percent) : g_strdup ("");

  set_field (self, EASEL_STATUS_ZOOM, text);
}

void
easel_status_bar_set_selection (EaselStatusBar *self, gboolean exists, int width, int height)
{
  g_autofree char *text = exists ? g_strdup_printf ("%d × %d", width, height) : g_strdup ("");

  set_field (self, EASEL_STATUS_SELECTION, text);
  gtk_widget_set_visible (GTK_WIDGET (self->labels[EASEL_STATUS_SELECTION]), exists);
}

void
easel_status_bar_set_hint (EaselStatusBar *self, const char *hint)
{
  set_field (self, EASEL_STATUS_HINT, hint);
}

const char *
easel_status_bar_get_text (EaselStatusBar *self, EaselStatusField field)
{
  return self->text[field] ? self->text[field] : "";
}

guint
easel_status_bar_get_writes (EaselStatusBar *self, EaselStatusField field)
{
  return self->writes[field];
}

GtkLabel *
easel_status_bar_get_label (EaselStatusBar *self, EaselStatusField field)
{
  return self->labels[field];
}

static void
easel_status_bar_finalize (GObject *object)
{
  EaselStatusBar *self = EASEL_STATUS_BAR (object);

  for (int i = 0; i < EASEL_STATUS_N_FIELDS; i++)
    g_free (self->text[i]);
  G_OBJECT_CLASS (easel_status_bar_parent_class)->finalize (object);
}

static void
easel_status_bar_class_init (EaselStatusBarClass *klass)
{
  G_OBJECT_CLASS (klass)->finalize = easel_status_bar_finalize;
}

static void
easel_status_bar_init (EaselStatusBar *self)
{
  GtkWidget *row = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 16);
  /* The hint at the left, then at the right edge: position, zoom, size (REQ-0004-09). Wide enough for the usual
     value, so the fields do not jump about while the pointer moves. */
  static const EaselStatusField order[EASEL_STATUS_N_FIELDS] = {
    EASEL_STATUS_HINT, EASEL_STATUS_PRESET, EASEL_STATUS_FRAME_TIME, EASEL_STATUS_POSITION, EASEL_STATUS_ZOOM, EASEL_STATUS_SELECTION,
    EASEL_STATUS_SIZE
  };
  static const int chars[EASEL_STATUS_N_FIELDS] = { 11, 15, 5, 1, 1, 9, 1 };   /* indexed by field; the frame time's fixed, so that the hint does not shift */

  gtk_orientable_set_orientation (GTK_ORIENTABLE (self), GTK_ORIENTATION_VERTICAL);
  gtk_box_append (GTK_BOX (self), gtk_separator_new (GTK_ORIENTATION_HORIZONTAL));
  gtk_widget_set_margin_start (row, 8);
  gtk_widget_set_margin_end (row, 8);
  gtk_widget_set_margin_top (row, 3);
  gtk_widget_set_margin_bottom (row, 3);
  gtk_box_append (GTK_BOX (self), row);

  for (int k = 0; k < EASEL_STATUS_N_FIELDS; k++)
    {
      EaselStatusField field = order[k];
      GtkLabel *label = GTK_LABEL (gtk_label_new (""));

      self->labels[field] = label;
      gtk_label_set_xalign (label, field == EASEL_STATUS_HINT ? 0 : 1);
      gtk_label_set_width_chars (label, chars[field]);
      gtk_widget_add_css_class (GTK_WIDGET (label), "caption");
      if (field != EASEL_STATUS_HINT && field != EASEL_STATUS_PRESET)
        gtk_widget_add_css_class (GTK_WIDGET (label), "numeric");
      gtk_box_append (GTK_BOX (row), GTK_WIDGET (label));
    }
  gtk_widget_set_visible (GTK_WIDGET (self->labels[EASEL_STATUS_FRAME_TIME]), FALSE);
  /* The hint takes the rest and is shortened with an ellipsis, so the bar never forces the window wider
     (REQ-0004-11). */
  gtk_widget_set_hexpand (GTK_WIDGET (self->labels[EASEL_STATUS_HINT]), TRUE);
  /* The selection's size takes no room without a selection (REQ-0013-06), so the hint keeps its width. */
  gtk_widget_set_visible (GTK_WIDGET (self->labels[EASEL_STATUS_SELECTION]), FALSE);
  gtk_widget_set_visible (GTK_WIDGET (self->labels[EASEL_STATUS_PRESET]), FALSE);
  gtk_label_set_ellipsize (self->labels[EASEL_STATUS_HINT], PANGO_ELLIPSIZE_END);
}

void
easel_status_bar_show_frame_time (EaselStatusBar *self, gboolean shown)
{
  gtk_widget_set_visible (GTK_WIDGET (self->labels[EASEL_STATUS_FRAME_TIME]), shown);
  if (shown)
    set_field (self, EASEL_STATUS_FRAME_TIME, "– ms");
}

void
easel_status_bar_set_preset (EaselStatusBar *self, const char *text, const char *const *names)
{
  GtkWidget *label = GTK_WIDGET (self->labels[EASEL_STATUS_PRESET]);
  int widest = 0;

  gtk_widget_set_visible (label, text != NULL);
  if (text == NULL)
    {
      set_field (self, EASEL_STATUS_PRESET, "");
      return;
    }
  /* As wide as the tool's longest name, so that a change of preset moves nothing (REQ-0030-08). */
  for (int i = 0; names != NULL && names[i] != NULL; i++)
    {
      g_autoptr (PangoLayout) layout = gtk_widget_create_pango_layout (label, names[i]);
      int w;

      pango_layout_get_pixel_size (layout, &w, NULL);
      widest = MAX (widest, w);
    }
  gtk_widget_set_size_request (label, widest, -1);
  set_field (self, EASEL_STATUS_PRESET, text);
}

void
easel_status_bar_set_frame_time (EaselStatusBar *self, const char *text)
{
  set_field (self, EASEL_STATUS_FRAME_TIME, text);
}
