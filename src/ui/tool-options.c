/* SPDX-License-Identifier: GPL-3.0-or-later */

#include "magnifier.h"
#include "tool-options.h"
#include "i18n.h"

enum { SIGNAL_PRESET_CHANGED, N_SIGNALS };
static guint signals[N_SIGNALS];


typedef struct
{
  EaselFootprint footprint;
  int mode;                  /* -1 for a size preset, 0 for Opaque, 1 for Transparent (the Eraser's and the selection's alike) */
  gboolean spray;            /* the Airbrush's disc: only its pixels whose x + y is even (REQ-0030-03) */
} PresetData;



struct _EaselToolOptions
{
  GtkBox parent_instance;

  GtkWidget *line;           /* the separator above the presets */
  GtkWidget *stack;          /* one page per group, and an empty page: as wide as its widest page (REQ-0008-07) */
  GtkWidget *groups[8];      /* Brush, Airbrush, Eraser, Magnifier, Line, the shapes, Select, Text: a vertical box each */
  GtkToggleButton *brush[EASEL_BRUSH_N_PRESETS];
  GtkToggleButton *airbrush[EASEL_AIRBRUSH_N_PRESETS];
  GtkToggleButton *eraser[EASEL_ERASER_N_SIZES];
  GtkLabel *brush_label[EASEL_BRUSH_N_PRESETS];       /* the number, or the name in the named form (REQ-0030-02, -10) */
  GtkLabel *airbrush_label[EASEL_AIRBRUSH_N_PRESETS];
  GtkLabel *eraser_label[EASEL_ERASER_N_SIZES];
  GtkLabel *width_label[2][5];                        /* Line's page and the shapes' page */
  GtkWidget *brush_grid, *air_grid, *eraser_grid;
  GtkWidget *brush_line[3];                           /* the thin lines between the Brush's shapes */
  GtkToggleButton *modes[2];
  GtkToggleButton *select_modes[2];
  GtkToggleButton *text_modes[2];
  GtkToggleButton *magnifier[EASEL_MAGNIFIER_N_LEVELS];
  int brush_index, airbrush_index, eraser_index;
  gboolean transparent;
  gboolean select_transparent;   /* the selection's own mode (REQ-0013-02) */
  gboolean text_transparent;     /* the Text tool's own mode, Transparent at each start (REQ-0017-02) */
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
      /* The modes, the Eraser's and the selection's alike: an opaque square, and a dashed outline for transparent. */
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
  /* A size preset: the footprint in the theme's text color made opaque, one logical pixel per picture pixel, centered
     on whole pixels with no smoothing (REQ-0030-02, -03, -07). */
  {
    int size = p->footprint.size, x0 = (width - size) / 2, y0 = (height - size) / 2;

    cairo_set_antialias (cr, CAIRO_ANTIALIAS_NONE);
    cairo_set_source_rgb (cr, color.red, color.green, color.blue);
    for (int row = 0; row < size; row++)
      for (int col = 0; col < size; col++)
        if (easel_footprint_has (p->footprint, col, row) && (!p->spray || (col + row) % 2 == 0))
          cairo_rectangle (cr, x0 + col, y0 + row, 1, 1);
    cairo_fill (cr);
  }
}

static char *
preset_name (EaselFootprint f, EaselTool tool)
{
  /* A whole sentence for each: no translated word in a placeholder (REQ-0023-01). */
  unsigned long n = (unsigned long) f.size;

  if (tool == EASEL_TOOL_ERASER)
    return g_strdup_printf (_n ("Eraser, %d pixel", "Eraser, %d pixels", n), f.size);
  if (tool == EASEL_TOOL_AIRBRUSH)
    return g_strdup_printf (_n ("Spray, %d pixel", "Spray, %d pixels", n), f.size);
  if (f.shape == EASEL_SHAPE_CIRCLE)
    return g_strdup_printf (_n ("Circle, %d pixel", "Circle, %d pixels", n), f.size);
  if (f.shape == EASEL_SHAPE_SQUARE)
    return g_strdup_printf (_n ("Square, %d pixel", "Square, %d pixels", n), f.size);
  if (f.shape == EASEL_SHAPE_SLASH)
    return g_strdup_printf (_n ("Slash, %d pixel", "Slash, %d pixels", n), f.size);
  return g_strdup_printf (_n ("Backslash, %d pixel", "Backslash, %d pixels", n), f.size);
}

static void
on_toggled (GtkToggleButton *button, EaselToolOptions *self)
{
  /* Only the one that became active counts; the one that became inactive is the other half of the same change. */
  if (!gtk_toggle_button_get_active (button))
    return;
  g_signal_emit (self, signals[SIGNAL_PRESET_CHANGED], 0);
}

/* An option button is a cell of a well: no outline of its own, the chosen one tinted with the accent (REQ-0030-01, -17). */
static void
make_cell (GtkWidget *button)
{
  gtk_widget_add_css_class (button, "flat");
  gtk_widget_add_css_class (button, "cell");
}

static GtkToggleButton *
add_button (EaselToolOptions *self, GtkGrid *grid, int index, int columns, const char *name, PresetData *data,
            GtkToggleButton *group_first)
{
  GtkWidget *button = gtk_toggle_button_new (), *area = gtk_drawing_area_new ();

  make_cell (button);
  gtk_widget_set_tooltip_text (button, name);
  gtk_accessible_update_property (GTK_ACCESSIBLE (button), GTK_ACCESSIBLE_PROPERTY_LABEL, name, -1);
  gtk_drawing_area_set_content_width (GTK_DRAWING_AREA (area), 22);
  gtk_drawing_area_set_content_height (GTK_DRAWING_AREA (area), 22);
  gtk_drawing_area_set_draw_func (GTK_DRAWING_AREA (area), draw_preset, data, g_free);
  gtk_button_set_child (GTK_BUTTON (button), area);
  if (group_first != NULL)
    gtk_toggle_button_set_group (GTK_TOGGLE_BUTTON (button), group_first);
  gtk_grid_attach (grid, button, index % columns, index / columns, 1, 1);
  g_signal_connect (button, "toggled", G_CALLBACK (on_toggled), self);
  return GTK_TOGGLE_BUTTON (button);
}

/* A size preset's cell: its footprint only (REQ-0030-02 to -04); the name is its tooltip and accessible name. */
static GtkToggleButton *
add_size_button (EaselToolOptions *self, GtkGrid *grid, int column, int row, const char *name, PresetData *data, int side,
                 int height, GtkLabel **label, GtkToggleButton *group_first)
{
  GtkWidget *button = gtk_toggle_button_new (), *area = gtk_drawing_area_new ();
  GtkWidget *box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 4);

  make_cell (button);
  gtk_widget_set_tooltip_text (button, name);
  gtk_accessible_update_property (GTK_ACCESSIBLE (button), GTK_ACCESSIBLE_PROPERTY_LABEL, name, -1);
  gtk_drawing_area_set_content_width (GTK_DRAWING_AREA (area), side);
  gtk_drawing_area_set_content_height (GTK_DRAWING_AREA (area), height);
  gtk_widget_set_halign (area, GTK_ALIGN_CENTER);
  gtk_widget_set_valign (area, GTK_ALIGN_CENTER);
  gtk_drawing_area_set_draw_func (GTK_DRAWING_AREA (area), draw_preset, data, g_free);
  *label = NULL;                                     /* no number in the well: the size is in the tooltip (REQ-0030-02) */
  gtk_box_append (GTK_BOX (box), area);
  gtk_widget_set_halign (box, GTK_ALIGN_CENTER);
  gtk_button_set_child (GTK_BUTTON (button), box);
  if (group_first != NULL)
    gtk_toggle_button_set_group (GTK_TOGGLE_BUTTON (button), group_first);
  gtk_grid_attach (grid, button, column, row, 1, 1);
  g_signal_connect (button, "toggled", G_CALLBACK (on_toggled), self);
  return GTK_TOGGLE_BUTTON (button);
}

/* The two-column place of a Brush preset (REQ-0030-01): circles then squares on the left, "/" then "\" on the right. */
static void
brush_place (int i, int *column, int *row)
{
  *column = i % 3;                                    /* a row per shape, the sizes across, a line between (REQ-0030-01) */
  *row = (i / 3) * 2;
}

/* A text button of the Magnifier's levels: "2x", with the zoom it gives as its tooltip. It selects the level through the
   window's action win.magnifier-level, whose state is the selected level, so the buttons show that state and never zoom
   (REQ-0007-02, REQ-0008-02). It is laid out as a tool button is, its label at the normal text size (REQ-0008-07). */
static GtkToggleButton *
add_level_button (EaselToolOptions *self, GtkGrid *grid, int index, int pct)
{
  g_autofree char *label = g_strdup_printf ("%dx", pct / 100);
  g_autofree char *tip = g_strdup_printf ("%d %%", pct);
  GtkWidget *button = gtk_toggle_button_new_with_label (label);

  make_cell (button);
  gtk_widget_add_css_class (button, "level");
  gtk_widget_set_tooltip_text (button, tip);
  gtk_actionable_set_action_name (GTK_ACTIONABLE (button), "win.magnifier-level");
  gtk_actionable_set_action_target_value (GTK_ACTIONABLE (button), g_variant_new_int32 (pct));
  gtk_grid_attach (grid, button, index % 3, index / 3, 1, 1);       /* two rows of three (REQ-0030-16) */
  g_signal_connect (button, "toggled", G_CALLBACK (on_toggled), self);
  return GTK_TOGGLE_BUTTON (button);
}

/* The width buttons and the fill-style buttons of the four shape tools (REQ-0010-02): drawn buttons bound to the window's
   actions win.line-width and win.fill-style, so each choice is one state shared by every page that shows it. */
static void
draw_width (GtkDrawingArea *area, cairo_t *cr, int width, int height, gpointer data)
{
  int w = GPOINTER_TO_INT (data);
  GdkRGBA color;

  gtk_widget_get_color (GTK_WIDGET (area), &color);
  gdk_cairo_set_source_rgba (cr, &color);
  cairo_rectangle (cr, 4, (height - w) / 2, width - 8, w);       /* a horizontal line w logical pixels thick */
  cairo_fill (cr);
}

static void
draw_fill_style (GtkDrawingArea *area, cairo_t *cr, int width, int height, gpointer data)
{
  int style = GPOINTER_TO_INT (data);                           /* 0 outline, 1 outline and fill, 2 fill */
  GdkRGBA color;
  double x = 6, y = 4, w = width - 12, h = height - 8;

  gtk_widget_get_color (GTK_WIDGET (area), &color);
  gdk_cairo_set_source_rgba (cr, &color);
  if (style == 2)
    {
      cairo_rectangle (cr, x, y, w, h);
      cairo_fill (cr);
      return;
    }
  if (style == 1)
    {
      GdkRGBA inner = color;

      inner.alpha *= 0.35f;
      gdk_cairo_set_source_rgba (cr, &inner);
      cairo_rectangle (cr, x + 2, y + 2, w - 4, h - 4);
      cairo_fill (cr);
      gdk_cairo_set_source_rgba (cr, &color);
    }
  cairo_set_line_width (cr, 2);
  cairo_rectangle (cr, x + 1, y + 1, w - 2, h - 2);
  cairo_stroke (cr);
}

static GtkWidget *
add_shape_button (GtkBox *box, const char *tip, const char *action, int target, GtkDrawingAreaDrawFunc draw, int content_w,
                  int content_h, const char *number)
{
  GtkWidget *button = gtk_toggle_button_new (), *area = gtk_drawing_area_new ();

  make_cell (button);
  gtk_widget_set_tooltip_text (button, tip);
  gtk_accessible_update_property (GTK_ACCESSIBLE (button), GTK_ACCESSIBLE_PROPERTY_LABEL, tip, -1);
  gtk_drawing_area_set_content_width (GTK_DRAWING_AREA (area), content_w);
  gtk_drawing_area_set_content_height (GTK_DRAWING_AREA (area), content_h);
  gtk_drawing_area_set_draw_func (GTK_DRAWING_AREA (area), draw, GINT_TO_POINTER (target), NULL);
  if (number != NULL)
    {
      /* A line width: the line and "3 px" to its larger x, dimmed (REQ-0030-05). */
      GtkWidget *row = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 4), *label = gtk_label_new (number);

      gtk_label_set_xalign (GTK_LABEL (label), 0);
      gtk_widget_add_css_class (label, "dim-label");
      gtk_box_append (GTK_BOX (row), area);
      gtk_box_append (GTK_BOX (row), label);
      gtk_button_set_child (GTK_BUTTON (button), row);
    }
  else
    gtk_button_set_child (GTK_BUTTON (button), area);
  gtk_actionable_set_action_name (GTK_ACTIONABLE (button), action);
  gtk_actionable_set_action_target_value (GTK_ACTIONABLE (button), g_variant_new_int32 (target));
  gtk_box_append (box, button);
  return button;
}

static const char *const width_tips[5] = { N_ ("1 pixel"), N_ ("2 pixels"), N_ ("3 pixels"), N_ ("4 pixels"), N_ ("5 pixels") };

static void
add_width_buttons (GtkBox *box, GtkLabel **labels)
{
  const char *const *tips = width_tips;

  for (int w = 1; w <= 5; w++)
    {
      g_autofree char *number = g_strdup_printf ("%d px", w);
      GtkWidget *b = add_shape_button (box, _ (tips[w - 1]), "win.line-width", w, draw_width, 40, 14, number);

      labels[w - 1] = GTK_LABEL (gtk_widget_get_last_child (gtk_button_get_child (GTK_BUTTON (b))));
    }
}

/* A well: a sunken box for one group of options (REQ-0030-01, -05, -16); its look is in style.css. */
static GtkWidget *
new_well_box (void)
{
  GtkWidget *box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);

  gtk_widget_add_css_class (box, "options-well");
  return box;
}

static GtkWidget *
new_shape_box (gboolean with_fill_styles, GtkLabel **width_labels)
{
  GtkWidget *box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6), *widths = new_well_box ();

  gtk_widget_set_margin_start (box, 4);
  gtk_widget_set_margin_end (box, 4);
  gtk_widget_set_margin_top (box, 6);
  gtk_widget_set_margin_bottom (box, 6);
  if (with_fill_styles)
    {
      static const char *const tips[3] = { N_ ("Outline"), N_ ("Outline and fill"), N_ ("Fill") };
      GtkWidget *styles = new_well_box ();

      for (int i = 0; i < 3; i++)
        add_shape_button (GTK_BOX (styles), _ (tips[i]), "win.fill-style", i, draw_fill_style, 52, 22, NULL);
      gtk_box_append (GTK_BOX (box), styles);
    }
  add_width_buttons (GTK_BOX (widths), width_labels);
  gtk_box_append (GTK_BOX (box), widths);
  return box;
}

static GtkWidget *
new_grid (void)
{
  GtkWidget *grid = gtk_grid_new ();

  /* A well of cells (REQ-0030-01): no gaps, equal columns, within the tool grid's width (REQ-0008-07). */
  gtk_widget_add_css_class (grid, "options-well");
  gtk_grid_set_column_homogeneous (GTK_GRID (grid), TRUE);
  gtk_widget_set_margin_start (grid, 4);
  gtk_widget_set_margin_end (grid, 4);
  gtk_widget_set_margin_top (grid, 6);
  gtk_widget_set_margin_bottom (grid, 6);
  return grid;
}

static PresetData *
preset_data (EaselFootprint f, int mode)
{
  PresetData *p = g_new0 (PresetData, 1);

  p->footprint = f;
  p->mode = mode;
  return p;
}

static PresetData *
spray_data (EaselFootprint f)
{
  PresetData *p = preset_data (f, -1);

  p->spray = TRUE;
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

      int column, row;

      brush_place (i, &column, &row);
      self->brush[i] = add_size_button (self, GTK_GRID (brush_grid), column, row, name, preset_data (easel_brush_presets[i], -1),
                                        12, 16, &self->brush_label[i], i ? self->brush[0] : NULL);
    }
  for (int k = 0; k < 3; k++)
    {
      /* the thin line between the shapes */
      self->brush_line[k] = gtk_separator_new (GTK_ORIENTATION_HORIZONTAL);
      gtk_grid_attach (GTK_GRID (brush_grid), self->brush_line[k], 0, 2 * k + 1, 3, 1);
    }
  for (int i = 0; i < EASEL_AIRBRUSH_N_PRESETS; i++)
    {
      g_autofree char *name = preset_name (easel_airbrush_presets[i], EASEL_TOOL_AIRBRUSH);

      self->airbrush[i] = add_size_button (self, GTK_GRID (air_grid), i, 0, name, spray_data (easel_airbrush_presets[i]),
                                           24, 26, &self->airbrush_label[i], i ? self->airbrush[0] : NULL);
    }
  for (int i = 0; i < EASEL_ERASER_N_SIZES; i++)
    {
      g_autofree char *name = preset_name (easel_eraser_presets[i], EASEL_TOOL_ERASER);

      self->eraser[i] = add_size_button (self, GTK_GRID (size_grid), i, 0, name, preset_data (easel_eraser_presets[i], -1),
                                         12, 18, &self->eraser_label[i], i ? self->eraser[0] : NULL);
    }
  /* The modes are a well of their own below, side by side (REQ-0030-04). */
  self->modes[0] = add_button (self, GTK_GRID (mode_grid), 0, 2, _ ("Opaque: erase to the background color"),
                               preset_data ((EaselFootprint) { EASEL_SHAPE_SQUARE, 1 }, 0), NULL);
  self->modes[1] = add_button (self, GTK_GRID (mode_grid), 1, 2, _ ("Transparent: erase to transparency"),
                               preset_data ((EaselFootprint) { EASEL_SHAPE_SQUARE, 1 }, 1), self->modes[0]);
  gtk_widget_set_margin_top (mode_grid, 0);
  gtk_box_append (GTK_BOX (eraser_box), size_grid);
  gtk_box_append (GTK_BOX (eraser_box), mode_grid);
  {
    /* The selection's modes, side by side in a well, as the Eraser's (REQ-0013-02, REQ-0030-16). */
    GtkWidget *select_grid = new_grid ();

    self->select_modes[0] = add_button (self, GTK_GRID (select_grid), 0, 2, _ ("Opaque: keep background color"),
                                        preset_data ((EaselFootprint) { EASEL_SHAPE_SQUARE, 1 }, 0), NULL);
    self->select_modes[1] = add_button (self, GTK_GRID (select_grid), 1, 2, _ ("Transparent: moving or clearing leaves a transparent hole"),
                                        preset_data ((EaselFootprint) { EASEL_SHAPE_SQUARE, 1 }, 1), self->select_modes[0]);
    self->groups[6] = select_grid;
  }
  {
    /* The Text tool's own modes, drawn as Select's, and nothing else (REQ-0017-02). */
    GtkWidget *text_grid = new_grid ();

    self->text_modes[0] = add_button (self, GTK_GRID (text_grid), 0, 2, _ ("Opaque: fill the text box with the background color"),
                                      preset_data ((EaselFootprint) { EASEL_SHAPE_SQUARE, 1 }, 0), NULL);
    self->text_modes[1] = add_button (self, GTK_GRID (text_grid), 1, 2, _ ("Transparent: only the text"),
                                      preset_data ((EaselFootprint) { EASEL_SHAPE_SQUARE, 1 }, 1), self->text_modes[0]);
    self->groups[7] = text_grid;
  }

  {
    /* The levels in a well, two rows of three (REQ-0030-16). */
    GtkWidget *magnifier_grid = new_grid ();

    for (int i = 0; i < EASEL_MAGNIFIER_N_LEVELS; i++)
      self->magnifier[i] = add_level_button (self, GTK_GRID (magnifier_grid), i, easel_magnifier_levels[i]);
    self->groups[3] = magnifier_grid;
  }
  self->groups[0] = brush_grid;
  self->groups[1] = air_grid;
  self->groups[2] = eraser_box;
  self->groups[4] = new_shape_box (FALSE, self->width_label[0]);          /* Line: the widths */
  self->groups[5] = new_shape_box (TRUE, self->width_label[1]);           /* the shapes: the fill styles, a line, the widths */
  self->brush_grid = brush_grid;
  self->air_grid = air_grid;
  self->eraser_grid = size_grid;
  /* The groups are the pages of a stack as wide as its widest page and as high as the page shown, so that the toolbox has
     one width for every tool at any text size; a tool without options shows the empty page (REQ-0008-07, REQ-0006-02). */
  self->stack = gtk_stack_new ();
  gtk_stack_set_hhomogeneous (GTK_STACK (self->stack), TRUE);
  gtk_stack_set_vhomogeneous (GTK_STACK (self->stack), FALSE);
  gtk_stack_set_transition_type (GTK_STACK (self->stack), GTK_STACK_TRANSITION_TYPE_NONE);
  for (int i = 0; i < 8; i++)
    gtk_stack_add_named (GTK_STACK (self->stack), self->groups[i], NULL);
  gtk_stack_add_named (GTK_STACK (self->stack), gtk_box_new (GTK_ORIENTATION_VERTICAL, 0), "none");
  gtk_box_append (GTK_BOX (self), self->stack);
}

static void
sync_selection (EaselToolOptions *self)
{
  gtk_toggle_button_set_active (self->brush[self->brush_index], TRUE);
  gtk_toggle_button_set_active (self->airbrush[self->airbrush_index], TRUE);
  gtk_toggle_button_set_active (self->eraser[self->eraser_index], TRUE);
  gtk_toggle_button_set_active (self->modes[self->transparent ? 1 : 0], TRUE);
  gtk_toggle_button_set_active (self->select_modes[self->select_transparent ? 1 : 0], TRUE);
  gtk_toggle_button_set_active (self->text_modes[self->text_transparent ? 1 : 0], TRUE);
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
  self->select_transparent = gtk_toggle_button_get_active (self->select_modes[1]);
  self->text_transparent = gtk_toggle_button_get_active (self->text_modes[1]);
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
  self->text_transparent = TRUE;
  self->line = gtk_separator_new (GTK_ORIENTATION_HORIZONTAL);
  gtk_box_append (GTK_BOX (self), self->line);
  build_presets (self);
  sync_selection (self);
  easel_tool_options_show_tool (self, EASEL_TOOL_PAN);
}

EaselToolOptions *
easel_tool_options_new (void)
{
  return g_object_new (EASEL_TYPE_TOOL_OPTIONS, NULL);
}

void
easel_tool_options_show_tool (EaselToolOptions *self, EaselTool tool)
{
  int group = tool == EASEL_TOOL_BRUSH ? 0 : tool == EASEL_TOOL_AIRBRUSH ? 1 : tool == EASEL_TOOL_ERASER ? 2
              : tool == EASEL_TOOL_MAGNIFIER ? 3 : tool == EASEL_TOOL_LINE || tool == EASEL_TOOL_CURVE ? 4
              : easel_tool_draws_shape (tool) || tool == EASEL_TOOL_POLYGON ? 5 : easel_tool_selects (tool) ? 6
              : tool == EASEL_TOOL_TEXT ? 7 : -1;

  /* Nothing of the options shows for a tool without any: the empty page, and no line above it (REQ-0006-02). */
  if (group >= 0)
    gtk_stack_set_visible_child (GTK_STACK (self->stack), self->groups[group]);
  else
    gtk_stack_set_visible_child_name (GTK_STACK (self->stack), "none");
  gtk_widget_set_visible (self->line, group >= 0);
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
easel_tool_options_get_select_transparent (EaselToolOptions *self)
{
  read_selection (self);
  return self->select_transparent;
}

gboolean
easel_tool_options_get_text_transparent (EaselToolOptions *self)
{
  read_selection (self);
  return self->text_transparent;
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

GStrv
easel_tool_options_size_names (EaselTool tool)
{
  g_autoptr (GStrvBuilder) b = g_strv_builder_new ();
  const EaselFootprint *table = NULL;
  int n = 0;

  if (easel_tool_uses_line_width (tool))
    {
      for (int w = 1; w <= 5; w++)
        g_strv_builder_add (b, _ (width_tips[w - 1]));
      return g_strv_builder_end (b);
    }
  if (tool == EASEL_TOOL_BRUSH)
    table = easel_brush_presets, n = EASEL_BRUSH_N_PRESETS;
  else if (tool == EASEL_TOOL_AIRBRUSH)
    table = easel_airbrush_presets, n = EASEL_AIRBRUSH_N_PRESETS;
  else if (tool == EASEL_TOOL_ERASER)
    table = easel_eraser_presets, n = EASEL_ERASER_N_SIZES;
  else
    return NULL;
  for (int i = 0; i < n; i++)
    g_strv_builder_take (b, preset_name (table[i], tool));
  return g_strv_builder_end (b);
}

int
easel_tool_options_size_index (EaselToolOptions *self, EaselTool tool, int line_width)
{
  read_selection (self);
  if (easel_tool_uses_line_width (tool))
    return CLAMP (line_width, 1, 5) - 1;
  return tool == EASEL_TOOL_AIRBRUSH ? self->airbrush_index : tool == EASEL_TOOL_ERASER ? self->eraser_index : self->brush_index;
}

void
easel_tool_options_set_busy (EaselToolOptions *self, gboolean busy)
{
  self->busy = busy;
  gtk_widget_set_sensitive (GTK_WIDGET (self), !busy);
}

void
easel_tool_options_set_start (EaselToolOptions *self, int brush, int airbrush, int eraser, gboolean transparent)
{
  self->brush_index = brush;
  self->airbrush_index = airbrush;
  self->eraser_index = eraser;
  self->transparent = transparent;
  sync_selection (self);
}
