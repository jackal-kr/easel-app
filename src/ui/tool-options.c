/* SPDX-License-Identifier: GPL-3.0-or-later */

#include "tool-options.h"

enum { SIGNAL_PRESET_CHANGED, N_SIGNALS };
static guint signals[N_SIGNALS];

/* A button shows its footprint in a square this big (the largest footprint is 24 pixels). */
#define BUTTON_SIZE 28

typedef struct
{
  EaselFootprint footprint;
  int mode;                  /* -1 for a size preset, 0 for Opaque, 1 for Transparent */
} PresetData;

struct _EaselToolOptions
{
  GtkBox parent_instance;

  GtkWidget *line;           /* the separator above the presets */
  GtkWidget *groups[3];      /* Brush, Airbrush, Eraser: a vertical box each */
  GtkToggleButton *brush[EASEL_BRUSH_N_PRESETS];
  GtkToggleButton *airbrush[EASEL_AIRBRUSH_N_PRESETS];
  GtkToggleButton *eraser[EASEL_ERASER_N_SIZES];
  GtkToggleButton *modes[2];
  int brush_index, airbrush_index, eraser_index;
  gboolean transparent;
  gboolean busy;
};

G_DEFINE_FINAL_TYPE (EaselToolOptions, easel_tool_options, GTK_TYPE_BOX)

static void
draw_preset (GtkDrawingArea *area, cairo_t *cr, int width, int height, gpointer data)
{
  const PresetData *p = data;
  GdkRGBA color;

  gtk_widget_get_color (GTK_WIDGET (area), &color);
  gdk_cairo_set_source_rgba (cr, &color);
  if (p->mode >= 0)
    {
      /* The Eraser's modes: an opaque square, and a dashed outline for transparent. */
      double side = 14, x = (width - side) / 2.0, y = (height - side) / 2.0;

      if (p->mode == 0)
        {
          cairo_rectangle (cr, x, y, side, side);
          cairo_fill (cr);
        }
      else
        {
          static const double dashes[] = { 2.0, 2.0 };

          cairo_set_line_width (cr, 1.5);
          cairo_set_dash (cr, dashes, 2, 0);
          cairo_rectangle (cr, x + 0.75, y + 0.75, side - 1.5, side - 1.5);
          cairo_stroke (cr);
        }
      return;
    }
  /* The footprint, one logical pixel per picture pixel, centered in the button. */
  {
    int size = p->footprint.size, x0 = (width - size) / 2, y0 = (height - size) / 2;

    for (int row = 0; row < size; row++)
      for (int col = 0; col < size; col++)
        if (easel_footprint_has (p->footprint, col, row))
          cairo_rectangle (cr, x0 + col, y0 + row, 1, 1);
    cairo_fill (cr);
  }
}

static char *
preset_name (EaselFootprint f, EaselTool tool)
{
  const char *what = tool == EASEL_TOOL_ERASER ? "Eraser" : tool == EASEL_TOOL_AIRBRUSH ? "Spray"
                     : f.shape == EASEL_SHAPE_CIRCLE ? "Circle" : f.shape == EASEL_SHAPE_SQUARE ? "Square"
                     : f.shape == EASEL_SHAPE_SLASH ? "Slash" : "Backslash";

  return g_strdup_printf (f.size == 1 ? "%s, %d pixel" : "%s, %d pixels", what, f.size);
}

static void
on_toggled (GtkToggleButton *button, EaselToolOptions *self)
{
  /* Only the one that became active counts; the one that became inactive is the other half of the same change. */
  if (!gtk_toggle_button_get_active (button))
    return;
  g_signal_emit (self, signals[SIGNAL_PRESET_CHANGED], 0);
}

static GtkToggleButton *
add_button (EaselToolOptions *self, GtkGrid *grid, int index, int columns, const char *name, PresetData *data,
            GtkToggleButton *group_first)
{
  GtkWidget *button = gtk_toggle_button_new (), *area = gtk_drawing_area_new ();

  gtk_widget_add_css_class (button, "flat");
  gtk_widget_set_tooltip_text (button, name);
  gtk_accessible_update_property (GTK_ACCESSIBLE (button), GTK_ACCESSIBLE_PROPERTY_LABEL, name, -1);
  gtk_drawing_area_set_content_width (GTK_DRAWING_AREA (area), BUTTON_SIZE);
  gtk_drawing_area_set_content_height (GTK_DRAWING_AREA (area), BUTTON_SIZE);
  gtk_drawing_area_set_draw_func (GTK_DRAWING_AREA (area), draw_preset, data, g_free);
  gtk_button_set_child (GTK_BUTTON (button), area);
  if (group_first != NULL)
    gtk_toggle_button_set_group (GTK_TOGGLE_BUTTON (button), group_first);
  gtk_grid_attach (grid, button, index % columns, index / columns, 1, 1);
  g_signal_connect (button, "toggled", G_CALLBACK (on_toggled), self);
  return GTK_TOGGLE_BUTTON (button);
}

static GtkWidget *
new_grid (void)
{
  GtkWidget *grid = gtk_grid_new ();

  gtk_grid_set_row_spacing (GTK_GRID (grid), 4);
  gtk_grid_set_column_spacing (GTK_GRID (grid), 4);
  gtk_widget_set_margin_start (grid, 6);
  gtk_widget_set_margin_end (grid, 6);
  gtk_widget_set_margin_top (grid, 6);
  gtk_widget_set_margin_bottom (grid, 6);
  return grid;
}

static PresetData *
preset_data (EaselFootprint f, int mode)
{
  PresetData *p = g_new (PresetData, 1);

  p->footprint = f;
  p->mode = mode;
  return p;
}

static void
build_presets (EaselToolOptions *self)
{
  GtkWidget *brush_grid = new_grid (), *air_grid = new_grid (), *eraser_box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  GtkWidget *size_grid = new_grid (), *mode_grid = new_grid ();

  for (int i = 0; i < EASEL_BRUSH_N_PRESETS; i++)
    {
      g_autofree char *name = preset_name (easel_brush_presets[i], EASEL_TOOL_BRUSH);

      self->brush[i] = add_button (self, GTK_GRID (brush_grid), i, 2, name, preset_data (easel_brush_presets[i], -1),
                                   i ? self->brush[0] : NULL);
    }
  for (int i = 0; i < EASEL_AIRBRUSH_N_PRESETS; i++)
    {
      g_autofree char *name = preset_name (easel_airbrush_presets[i], EASEL_TOOL_AIRBRUSH);

      self->airbrush[i] = add_button (self, GTK_GRID (air_grid), i, 2, name, preset_data (easel_airbrush_presets[i], -1),
                                      i ? self->airbrush[0] : NULL);
    }
  for (int i = 0; i < EASEL_ERASER_N_SIZES; i++)
    {
      g_autofree char *name = preset_name (easel_eraser_presets[i], EASEL_TOOL_ERASER);

      self->eraser[i] = add_button (self, GTK_GRID (size_grid), i, 2, name, preset_data (easel_eraser_presets[i], -1),
                                    i ? self->eraser[0] : NULL);
    }
  /* The modes are a group of their own, one under the other, after a line. */
  self->modes[0] = add_button (self, GTK_GRID (mode_grid), 0, 1, "Opaque: erase to the background color",
                               preset_data ((EaselFootprint) { EASEL_SHAPE_SQUARE, 1 }, 0), NULL);
  self->modes[1] = add_button (self, GTK_GRID (mode_grid), 1, 1, "Transparent: erase to transparency",
                               preset_data ((EaselFootprint) { EASEL_SHAPE_SQUARE, 1 }, 1), self->modes[0]);
  gtk_box_append (GTK_BOX (eraser_box), size_grid);
  gtk_box_append (GTK_BOX (eraser_box), gtk_separator_new (GTK_ORIENTATION_HORIZONTAL));
  gtk_box_append (GTK_BOX (eraser_box), mode_grid);

  self->groups[0] = brush_grid;
  self->groups[1] = air_grid;
  self->groups[2] = eraser_box;
  for (int i = 0; i < 3; i++)
    {
      gtk_widget_set_visible (self->groups[i], FALSE);
      gtk_box_append (GTK_BOX (self), self->groups[i]);
    }
}

static void
sync_selection (EaselToolOptions *self)
{
  gtk_toggle_button_set_active (self->brush[self->brush_index], TRUE);
  gtk_toggle_button_set_active (self->airbrush[self->airbrush_index], TRUE);
  gtk_toggle_button_set_active (self->eraser[self->eraser_index], TRUE);
  gtk_toggle_button_set_active (self->modes[self->transparent ? 1 : 0], TRUE);
}

static int
index_of (GtkToggleButton *const *buttons, int n)
{
  for (int i = 0; i < n; i++)
    if (gtk_toggle_button_get_active (buttons[i]))
      return i;
  return 0;
}

/* The user's choice, read from the buttons after a toggle. */
static void
read_selection (EaselToolOptions *self)
{
  self->brush_index = index_of (self->brush, EASEL_BRUSH_N_PRESETS);
  self->airbrush_index = index_of (self->airbrush, EASEL_AIRBRUSH_N_PRESETS);
  self->eraser_index = index_of (self->eraser, EASEL_ERASER_N_SIZES);
  self->transparent = gtk_toggle_button_get_active (self->modes[1]);
}

static void
easel_tool_options_class_init (EaselToolOptionsClass *klass)
{
  signals[SIGNAL_PRESET_CHANGED] = g_signal_new ("preset-changed", G_TYPE_FROM_CLASS (klass), G_SIGNAL_RUN_LAST, 0, NULL, NULL,
                                                 NULL, G_TYPE_NONE, 0);
}

static void
easel_tool_options_init (EaselToolOptions *self)
{
  gtk_orientable_set_orientation (GTK_ORIENTABLE (self), GTK_ORIENTATION_VERTICAL);
  self->brush_index = EASEL_BRUSH_DEFAULT;
  self->airbrush_index = EASEL_AIRBRUSH_DEFAULT;
  self->eraser_index = EASEL_ERASER_DEFAULT;
  self->line = gtk_separator_new (GTK_ORIENTATION_HORIZONTAL);
  gtk_box_append (GTK_BOX (self), self->line);
  build_presets (self);
  sync_selection (self);
  gtk_widget_set_visible (GTK_WIDGET (self), FALSE);
}

EaselToolOptions *
easel_tool_options_new (void)
{
  return g_object_new (EASEL_TYPE_TOOL_OPTIONS, NULL);
}

void
easel_tool_options_show_tool (EaselToolOptions *self, EaselTool tool)
{
  int group = tool == EASEL_TOOL_BRUSH ? 0 : tool == EASEL_TOOL_AIRBRUSH ? 1 : tool == EASEL_TOOL_ERASER ? 2 : -1;

  for (int i = 0; i < 3; i++)
    gtk_widget_set_visible (self->groups[i], i == group);
  gtk_widget_set_visible (GTK_WIDGET (self), group >= 0);
}

EaselFootprint
easel_tool_options_get_footprint (EaselToolOptions *self, EaselTool tool)
{
  read_selection (self);
  if (tool == EASEL_TOOL_AIRBRUSH)
    return easel_airbrush_presets[self->airbrush_index];
  if (tool == EASEL_TOOL_ERASER)
    return easel_eraser_presets[self->eraser_index];
  return easel_brush_presets[self->brush_index];
}

gboolean
easel_tool_options_get_transparent (EaselToolOptions *self)
{
  read_selection (self);
  return self->transparent;
}

gboolean
easel_tool_options_step (EaselToolOptions *self, EaselTool tool, int direction)
{
  const EaselFootprint *table;
  int n, *index, next;

  if (self->busy)
    return FALSE;
  read_selection (self);
  if (tool == EASEL_TOOL_BRUSH)
    {
      table = easel_brush_presets;
      n = EASEL_BRUSH_N_PRESETS;
      index = &self->brush_index;
    }
  else if (tool == EASEL_TOOL_AIRBRUSH)
    {
      table = easel_airbrush_presets;
      n = EASEL_AIRBRUSH_N_PRESETS;
      index = &self->airbrush_index;
    }
  else if (tool == EASEL_TOOL_ERASER)
    {
      table = easel_eraser_presets;
      n = EASEL_ERASER_N_SIZES;
      index = &self->eraser_index;
    }
  else
    return FALSE;
  next = easel_preset_step (table, n, *index, direction);
  if (next == *index)
    return FALSE;
  *index = next;
  sync_selection (self);
  return TRUE;
}

void
easel_tool_options_set_busy (EaselToolOptions *self, gboolean busy)
{
  self->busy = busy;
  gtk_widget_set_sensitive (GTK_WIDGET (self), !busy);
}
