/* SPDX-License-Identifier: GPL-3.0-or-later */

#include "canvas.h"
#include "palette-bar.h"

/* Dimensions of REQ-0003-01 and REQ-0003-18, in logical pixels. */
#define SWATCH_SIZE 20
#define SWATCH_SPACING 2
#define INDICATOR_WIDTH 46
#define INDICATOR_HEIGHT 42
#define INDICATOR_SQUARE 28
#define FRONT_X 2
#define FRONT_Y 2
#define BACK_X 16
#define BACK_Y 12
#define BAR_MARGIN_X 8
#define BAR_MARGIN_Y 6
#define BAR_SPACING 8
#define SWATCH_CHECKER 4

static GdkRGBA
to_rgba (uint32_t c)
{
  return (GdkRGBA) { ((c >> 16) & 0xFF) / 255.0f, ((c >> 8) & 0xFF) / 255.0f, (c & 0xFF) / 255.0f, (c >> 24) / 255.0f };
}

/* A color square: a checkerboard behind any transparency (REQ-0003-05), then the color, then a thin outline. */
static void
append_color_square (GtkWidget *widget, GtkSnapshot *snapshot, const graphene_rect_t *r, uint32_t color)
{
  GdkRGBA rgba = to_rgba (color);
  GdkRGBA line = { 0, 0, 0, 0.35f };
  float widths[4] = { 1, 1, 1, 1 };
  GdkRGBA lines[4] = { line, line, line, line };

  if ((color >> 24) != 0xFF)
    easel_snapshot_append_checkerboard (snapshot, r, SWATCH_CHECKER);
  gtk_snapshot_append_color (snapshot, &rgba, r);
  gtk_snapshot_append_border (snapshot, &GSK_ROUNDED_RECT_INIT (r->origin.x, r->origin.y, r->size.width, r->size.height),
                              widths, lines);
}

/* A color as a name says it: #RRGGBB, with its alpha when not opaque (REQ-0022-14). */
static char *
easel_color_hex_name (uint32_t c)
{
  if ((c >> 24) == 0xFF)
    return g_strdup_printf ("#%06X", (unsigned) (c & 0xFFFFFF));
  return g_strdup_printf ("#%06X%02X", (unsigned) (c & 0xFFFFFF), (unsigned) (c >> 24));
}

/* ---- Swatch ---- */

struct _EaselSwatch
{
  GtkWidget parent_instance;
  uint32_t color;
  int number;             /* 1..28 */
};

G_DEFINE_FINAL_TYPE (EaselSwatch, easel_swatch, GTK_TYPE_WIDGET)

int
easel_swatch_get_number (EaselSwatch *self)
{
  return self->number;
}

uint32_t
easel_swatch_get_color (EaselSwatch *self)
{
  return self->color;
}

static void
easel_swatch_set_color (EaselSwatch *self, uint32_t color)
{
  g_autofree char *hex = easel_color_hex_name (color);
  g_autofree char *name = g_strdup_printf ("Color %d, %s", self->number, hex);

  gtk_accessible_update_property (GTK_ACCESSIBLE (self), GTK_ACCESSIBLE_PROPERTY_LABEL, name, -1);
  self->color = color;
  gtk_widget_queue_draw (GTK_WIDGET (self));
}

static void
easel_swatch_measure (GtkWidget *widget, GtkOrientation orientation, int for_size, int *minimum, int *natural,
                      int *minimum_baseline, int *natural_baseline)
{
  *minimum = *natural = SWATCH_SIZE;
}

static void
easel_swatch_snapshot (GtkWidget *widget, GtkSnapshot *snapshot)
{
  EaselSwatch *self = EASEL_SWATCH (widget);
  graphene_rect_t r = GRAPHENE_RECT_INIT (0, 0, SWATCH_SIZE, SWATCH_SIZE);

  append_color_square (widget, snapshot, &r, self->color);
  if (gtk_widget_has_visible_focus (widget))
    {
      GdkRGBA ring;
      float widths[4] = { 2, 2, 2, 2 };

      gtk_widget_get_color (widget, &ring);
      gtk_snapshot_append_border (snapshot, &GSK_ROUNDED_RECT_INIT (0, 0, SWATCH_SIZE, SWATCH_SIZE), widths,
                                  (GdkRGBA[4]) { ring, ring, ring, ring });
    }
}

static void swatch_edit_activated (GtkWidget *widget, const char *action_name, GVariant *parameter);

static void
easel_swatch_class_init (EaselSwatchClass *klass)
{
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  widget_class->measure = easel_swatch_measure;
  widget_class->snapshot = easel_swatch_snapshot;
  gtk_widget_class_set_accessible_role (widget_class, GTK_ACCESSIBLE_ROLE_BUTTON);

  /* F2 on the focused swatch edits it (REQ-0003-11). */
  gtk_widget_class_install_action (widget_class, "swatch.edit", NULL, swatch_edit_activated);
  gtk_widget_class_add_binding_action (widget_class, GDK_KEY_F2, 0, "swatch.edit", NULL);
}

static void
easel_swatch_init (EaselSwatch *self)
{
  gtk_widget_set_focusable (GTK_WIDGET (self), TRUE);
  gtk_widget_add_css_class (GTK_WIDGET (self), "palette-swatch");
}

/* ---- Indicator ---- */

struct _EaselColorIndicator
{
  GtkWidget parent_instance;
  uint32_t foreground;
  uint32_t background;
  GtkWidget *front_square, *back_square;     /* unseen, over each square: a keyboard stop and a name */
};

G_DEFINE_FINAL_TYPE (EaselColorIndicator, easel_color_indicator, GTK_TYPE_WIDGET)

void
easel_color_indicator_get_square (EaselColorIndicator *self, gboolean front, graphene_rect_t *out)
{
  if (front)
    *out = GRAPHENE_RECT_INIT (FRONT_X, FRONT_Y, INDICATOR_SQUARE, INDICATOR_SQUARE);
  else
    *out = GRAPHENE_RECT_INIT (BACK_X, BACK_Y, INDICATOR_SQUARE, INDICATOR_SQUARE);
}

static void
easel_color_indicator_measure (GtkWidget *widget, GtkOrientation orientation, int for_size, int *minimum,
                               int *natural, int *minimum_baseline, int *natural_baseline)
{
  *minimum = *natural = orientation == GTK_ORIENTATION_HORIZONTAL ? INDICATOR_WIDTH : INDICATOR_HEIGHT;
}

/* The background square behind at the bottom right, the foreground square in front at the top left (REQ-0003-02),
   each with a 1-pixel frame (REQ-0003-18). */
static void
easel_color_indicator_snapshot (GtkWidget *widget, GtkSnapshot *snapshot)
{
  EaselColorIndicator *self = EASEL_COLOR_INDICATOR (widget);
  graphene_rect_t back, front;

  easel_color_indicator_get_square (self, FALSE, &back);
  easel_color_indicator_get_square (self, TRUE, &front);
  append_color_square (widget, snapshot, &back, self->background);
  append_color_square (widget, snapshot, &front, self->foreground);
  gtk_widget_snapshot_child (widget, self->back_square, snapshot);
  gtk_widget_snapshot_child (widget, self->front_square, snapshot);
}

static void
allocate_square (GtkWidget *square, const graphene_rect_t *r)
{
  gtk_widget_size_allocate (square, &(GtkAllocation) { (int) r->origin.x, (int) r->origin.y, (int) r->size.width, (int) r->size.height }, -1);
}

static void
easel_color_indicator_size_allocate (GtkWidget *widget, int width, int height, int baseline)
{
  EaselColorIndicator *self = EASEL_COLOR_INDICATOR (widget);
  graphene_rect_t r;

  easel_color_indicator_get_square (self, TRUE, &r);
  allocate_square (self->front_square, &r);
  easel_color_indicator_get_square (self, FALSE, &r);
  allocate_square (self->back_square, &r);
}

static void
easel_color_indicator_dispose (GObject *object)
{
  EaselColorIndicator *self = EASEL_COLOR_INDICATOR (object);

  g_clear_pointer (&self->front_square, gtk_widget_unparent);
  g_clear_pointer (&self->back_square, gtk_widget_unparent);
  G_OBJECT_CLASS (easel_color_indicator_parent_class)->dispose (object);
}

static GtkWidget *
new_square (GtkWidget *parent, const char *css_name)
{
  GtkWidget *square = g_object_new (GTK_TYPE_BOX, "accessible-role", GTK_ACCESSIBLE_ROLE_BUTTON, "focusable", TRUE, "can-target", FALSE, NULL);

  gtk_widget_add_css_class (square, "color-square");
  gtk_widget_set_name (square, css_name);
  gtk_widget_set_parent (square, parent);
  return square;
}

static void
easel_color_indicator_class_init (EaselColorIndicatorClass *klass)
{
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  widget_class->measure = easel_color_indicator_measure;
  widget_class->snapshot = easel_color_indicator_snapshot;
  widget_class->size_allocate = easel_color_indicator_size_allocate;
  G_OBJECT_CLASS (klass)->dispose = easel_color_indicator_dispose;
}

static void
easel_color_indicator_init (EaselColorIndicator *self)
{
  self->front_square = new_square (GTK_WIDGET (self), "foreground-square");
  self->back_square = new_square (GTK_WIDGET (self), "background-square");
}

/* Which square a point hits: the front square is on top where they overlap. 1 front, 0 back, -1 neither. */
static int
indicator_hit (EaselColorIndicator *self, double x, double y)
{
  graphene_rect_t r;
  graphene_point_t p = GRAPHENE_POINT_INIT ((float) x, (float) y);

  easel_color_indicator_get_square (self, TRUE, &r);
  if (graphene_rect_contains_point (&r, &p))
    return 1;
  easel_color_indicator_get_square (self, FALSE, &r);
  if (graphene_rect_contains_point (&r, &p))
    return 0;
  return -1;
}

/* ---- Bar ---- */

struct _EaselPaletteBar
{
  GtkBox parent_instance;
  EaselColorIndicator *indicator;
  GtkButton *swap;
  EaselSwatch *swatches[EASEL_PALETTE_SIZE];
};

G_DEFINE_FINAL_TYPE (EaselPaletteBar, easel_palette_bar, GTK_TYPE_BOX)

enum
{
  SIGNAL_SWATCH_PRESSED,
  SIGNAL_SWATCH_EDIT,
  SIGNAL_INDICATOR_PRESSED,
  N_SIGNALS
};

static guint signals[N_SIGNALS];

void
easel_palette_bar_press (EaselPaletteBar *self, int n, guint button, int n_press)
{
  if (n < 1 || n > EASEL_PALETTE_SIZE || (button != GDK_BUTTON_PRIMARY && button != GDK_BUTTON_SECONDARY))
    return;
  gtk_widget_grab_focus (GTK_WIDGET (self->swatches[n - 1]));
  g_signal_emit (self, signals[SIGNAL_SWATCH_PRESSED], 0, n, button, n_press);
}

void
easel_palette_bar_press_indicator (EaselPaletteBar *self, gboolean front)
{
  g_signal_emit (self, signals[SIGNAL_INDICATOR_PRESSED], 0, front);
}

void
easel_palette_bar_set_swatch (EaselPaletteBar *self, int n, uint32_t color)
{
  g_return_if_fail (n >= 1 && n <= EASEL_PALETTE_SIZE);
  easel_swatch_set_color (self->swatches[n - 1], color);
}

static void
swatch_edit_activated (GtkWidget *widget, const char *action_name, GVariant *parameter)
{
  GtkWidget *bar = gtk_widget_get_ancestor (widget, EASEL_TYPE_PALETTE_BAR);

  if (bar != NULL)
    g_signal_emit (bar, signals[SIGNAL_SWATCH_EDIT], 0, EASEL_SWATCH (widget)->number);
}

static void
on_indicator_pressed (GtkGestureClick *gesture, int n_press, double x, double y, EaselPaletteBar *self)
{
  int hit = indicator_hit (self->indicator, x, y);

  if (hit >= 0)
    easel_palette_bar_press_indicator (self, hit == 1);
}

static void
on_swatch_pressed (GtkGestureClick *gesture, int n_press, double x, double y, EaselPaletteBar *self)
{
  EaselSwatch *swatch = EASEL_SWATCH (gtk_event_controller_get_widget (GTK_EVENT_CONTROLLER (gesture)));

  easel_palette_bar_press (self, swatch->number, gtk_gesture_single_get_current_button (GTK_GESTURE_SINGLE (gesture)),
                           n_press);
}

void
easel_palette_bar_set_palette (EaselPaletteBar *self, const EaselPalette *palette)
{
  for (int i = 0; i < EASEL_PALETTE_SIZE; i++)
    easel_swatch_set_color (self->swatches[i], palette->colors[i]);
}

void
easel_palette_bar_set_colors (EaselPaletteBar *self, uint32_t foreground, uint32_t background)
{
  g_autofree char *front = easel_color_hex_name (foreground), *back = easel_color_hex_name (background);
  g_autofree char *front_name = g_strdup_printf ("Foreground color, %s", front), *back_name = g_strdup_printf ("Background color, %s", back);

  self->indicator->foreground = foreground;
  self->indicator->background = background;
  gtk_accessible_update_property (GTK_ACCESSIBLE (self->indicator->front_square), GTK_ACCESSIBLE_PROPERTY_LABEL, front_name, -1);
  gtk_accessible_update_property (GTK_ACCESSIBLE (self->indicator->back_square), GTK_ACCESSIBLE_PROPERTY_LABEL, back_name, -1);
  gtk_widget_queue_draw (GTK_WIDGET (self->indicator));
}

uint32_t
easel_palette_bar_get_foreground (EaselPaletteBar *self)
{
  return self->indicator->foreground;
}

uint32_t
easel_palette_bar_get_background (EaselPaletteBar *self)
{
  return self->indicator->background;
}

GtkWidget *
easel_palette_bar_get_swap (EaselPaletteBar *self)
{
  return GTK_WIDGET (self->swap);
}

GtkWidget *
easel_palette_bar_get_square (EaselPaletteBar *self, gboolean front)
{
  return front ? self->indicator->front_square : self->indicator->back_square;
}

EaselSwatch *
easel_palette_bar_get_swatch (EaselPaletteBar *self, int n)
{
  g_return_val_if_fail (n >= 1 && n <= EASEL_PALETTE_SIZE, NULL);
  return self->swatches[n - 1];
}

EaselColorIndicator *
easel_palette_bar_get_indicator (EaselPaletteBar *self)
{
  return self->indicator;
}

GtkButton *
easel_palette_bar_get_swap_button (EaselPaletteBar *self)
{
  return self->swap;
}

static void
easel_palette_bar_class_init (EaselPaletteBarClass *klass)
{
  signals[SIGNAL_SWATCH_PRESSED] = g_signal_new ("swatch-pressed", G_TYPE_FROM_CLASS (klass), G_SIGNAL_RUN_LAST, 0,
                                                 NULL, NULL, NULL, G_TYPE_NONE, 3, G_TYPE_INT, G_TYPE_UINT,
                                                 G_TYPE_INT);
  signals[SIGNAL_SWATCH_EDIT] = g_signal_new ("swatch-edit", G_TYPE_FROM_CLASS (klass), G_SIGNAL_RUN_LAST, 0, NULL,
                                              NULL, NULL, G_TYPE_NONE, 1, G_TYPE_INT);
  signals[SIGNAL_INDICATOR_PRESSED] = g_signal_new ("indicator-pressed", G_TYPE_FROM_CLASS (klass),
                                                    G_SIGNAL_RUN_LAST, 0, NULL, NULL, NULL, G_TYPE_NONE, 1,
                                                    G_TYPE_BOOLEAN);
}

static void
easel_palette_bar_init (EaselPaletteBar *self)
{
  GtkWidget *row = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, BAR_SPACING);
  GtkWidget *grid = gtk_grid_new ();
  EaselPalette defaults;

  /* A separator line, then one row: indicator, swap button, swatch grid (REQ-0003-01, REQ-0003-18). The bar spans
     the full width; the margins belong to the row. */
  gtk_orientable_set_orientation (GTK_ORIENTABLE (self), GTK_ORIENTATION_VERTICAL);
  gtk_box_append (GTK_BOX (self), gtk_separator_new (GTK_ORIENTATION_HORIZONTAL));
  gtk_widget_set_margin_start (row, BAR_MARGIN_X);
  gtk_widget_set_margin_end (row, BAR_MARGIN_X);
  gtk_widget_set_margin_top (row, BAR_MARGIN_Y);
  gtk_widget_set_margin_bottom (row, BAR_MARGIN_Y);
  gtk_box_append (GTK_BOX (self), row);

  self->indicator = g_object_new (EASEL_TYPE_COLOR_INDICATOR, NULL);
  gtk_widget_set_valign (GTK_WIDGET (self->indicator), GTK_ALIGN_CENTER);
  {
    GtkGesture *click = gtk_gesture_click_new ();

    g_signal_connect_object (click, "pressed", G_CALLBACK (on_indicator_pressed), self, G_CONNECT_DEFAULT);
    gtk_widget_add_controller (GTK_WIDGET (self->indicator), GTK_EVENT_CONTROLLER (click));
  }
  gtk_box_append (GTK_BOX (row), GTK_WIDGET (self->indicator));

  /* The swap button runs the same action as Ctrl+Shift+X, with the same rules (REQ-0003-19). */
  self->swap = GTK_BUTTON (gtk_button_new_with_label ("⇄"));
  gtk_widget_add_css_class (GTK_WIDGET (self->swap), "flat");
  gtk_widget_set_valign (GTK_WIDGET (self->swap), GTK_ALIGN_CENTER);
  gtk_widget_set_tooltip_text (GTK_WIDGET (self->swap), "Swap foreground and background (Ctrl+Shift+X)");
  gtk_actionable_set_action_name (GTK_ACTIONABLE (self->swap), "win.swap-colors");
  gtk_box_append (GTK_BOX (row), GTK_WIDGET (self->swap));

  /* Two rows of 14: swatch n at row 1, column n for n <= 14, else row 2, column n - 14 (REQ-0003-01). */
  gtk_grid_set_row_spacing (GTK_GRID (grid), SWATCH_SPACING);
  gtk_grid_set_column_spacing (GTK_GRID (grid), SWATCH_SPACING);
  gtk_widget_set_valign (grid, GTK_ALIGN_CENTER);
  for (int i = 0; i < EASEL_PALETTE_SIZE; i++)
    {
      GtkGesture *click = gtk_gesture_click_new ();

      self->swatches[i] = g_object_new (EASEL_TYPE_SWATCH, NULL);
      self->swatches[i]->number = i + 1;
      gtk_gesture_single_set_button (GTK_GESTURE_SINGLE (click), 0);
      g_signal_connect_object (click, "pressed", G_CALLBACK (on_swatch_pressed), self, G_CONNECT_DEFAULT);
      gtk_widget_add_controller (GTK_WIDGET (self->swatches[i]), GTK_EVENT_CONTROLLER (click));
      gtk_grid_attach (GTK_GRID (grid), GTK_WIDGET (self->swatches[i]), i % 14, i / 14, 1, 1);
    }
  gtk_box_append (GTK_BOX (row), grid);

  easel_palette_defaults (&defaults);
  easel_palette_bar_set_palette (self, &defaults);
}
