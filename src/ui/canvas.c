/* SPDX-License-Identifier: GPL-3.0-or-later */

#include "config.h"
#include "canvas.h"
#include "stroke.h"
#include "view.h"

/* Straight-alpha 0xAARRGGBB in memory order (ADR-0002). */
#if G_BYTE_ORDER == G_LITTLE_ENDIAN
#define PICTURE_FORMAT GDK_MEMORY_B8G8R8A8
#else
#define PICTURE_FORMAT GDK_MEMORY_A8R8G8B8
#endif

#define CHECKER_SIZE 8

struct _EaselCanvas
{
  GtkWidget parent_instance;

  EaselImage *img;
  GdkMemoryTextureBuilder *builder;
  GdkTexture *texture;
  int zoom;
  GdkSurface *surface;    /* watched for scale changes while realized */

  EaselTool tool;
  uint32_t foreground;
  uint32_t background;
  EaselHistory *history;  /* not owned */
  EaselStroke *stroke;
  guint stroke_button;
  guint cancelled_button; /* button of a stroke cancelled by Undo, until it is released; 0 if none */
  gboolean input_blocked;
  GtkWidget *area;        /* ancestor that feeds input; not owned */

  gboolean panning;
  double pan_x, pan_y;    /* canvas point under the pointer when the pan began */
  double notch_acc;       /* touchpad scrolling not yet a whole notch */
  gboolean anchor_pending;
  double anchor_h, anchor_v;  /* scroll offsets to apply once the new zoom is allocated */
};

/* Touchpad scrolling arrives in surface pixels; this many count as one wheel notch. */
#define PIXELS_PER_NOTCH 30.0

enum
{
  SIGNAL_STROKE_FINISHED,
  N_SIGNALS
};

static guint signals[N_SIGNALS];

G_DEFINE_FINAL_TYPE (EaselCanvas, easel_canvas, GTK_TYPE_WIDGET)

static GBytes *
image_bytes (const EaselImage *img)
{
  return g_bytes_new (img->px, (gsize) img->width * (gsize) img->height * sizeof (uint32_t));
}

static void
rebuild_texture (EaselCanvas *self, const cairo_region_t *region)
{
  g_autoptr (GBytes) bytes = image_bytes (self->img);

  gdk_memory_texture_builder_set_bytes (self->builder, bytes);
  gdk_memory_texture_builder_set_update_texture (self->builder, region ? self->texture : NULL);
  gdk_memory_texture_builder_set_update_region (self->builder, (cairo_region_t *) region);
  g_set_object (&self->texture, NULL);
  self->texture = gdk_memory_texture_builder_build (self->builder);
}

void
easel_canvas_set_image (EaselCanvas *self, EaselImage *img)
{
  EaselRect ignored;

  self->img = img;
  g_clear_object (&self->texture);
  if (img != NULL)
    {
      /* The whole texture is new, so earlier dirty marks are already covered. */
      easel_image_take_dirty (img, &ignored);
      gdk_memory_texture_builder_set_width (self->builder, img->width);
      gdk_memory_texture_builder_set_height (self->builder, img->height);
      gdk_memory_texture_builder_set_stride (self->builder, (gsize) img->width * sizeof (uint32_t));
      gdk_memory_texture_builder_set_format (self->builder, PICTURE_FORMAT);
      rebuild_texture (self, NULL);
    }
  gtk_widget_queue_resize (GTK_WIDGET (self));
}

void
easel_canvas_update (EaselCanvas *self)
{
  EaselRect r;
  cairo_region_t *region;

  if (self->img == NULL || !easel_image_take_dirty (self->img, &r))
    return;
  region = cairo_region_create_rectangle (&(cairo_rectangle_int_t) { r.x0, r.y0, r.x1 - r.x0, r.y1 - r.y0 });
  rebuild_texture (self, region);
  cairo_region_destroy (region);
  gtk_widget_queue_draw (GTK_WIDGET (self));
}

int
easel_canvas_get_zoom (EaselCanvas *self)
{
  return self->zoom;
}

double
easel_canvas_get_scale (EaselCanvas *self)
{
  GtkNative *native = gtk_widget_get_native (GTK_WIDGET (self));
  GdkSurface *surface = native ? gtk_native_get_surface (native) : NULL;

  return surface ? gdk_surface_get_scale (surface) : 1.0;
}

static void
easel_canvas_measure (GtkWidget *widget, GtkOrientation orientation, int for_size,
                      int *minimum, int *natural, int *minimum_baseline, int *natural_baseline)
{
  EaselCanvas *self = EASEL_CANVAS (widget);
  int size = 0;

  if (self->img != NULL)
    size = easel_view_content_size (orientation == GTK_ORIENTATION_HORIZONTAL ? self->img->width : self->img->height,
                                    self->zoom, easel_canvas_get_scale (self));
  *minimum = *natural = size;
}

void
easel_snapshot_append_checkerboard (GtkSnapshot *snapshot, const graphene_rect_t *bounds, float cell)
{
  static const GdkRGBA light = { 0.8, 0.8, 0.8, 1.0 };
  static const GdkRGBA dark = { 0.6, 0.6, 0.6, 1.0 };
  const float c = cell;

  gtk_snapshot_push_repeat (snapshot, bounds, &GRAPHENE_RECT_INIT (0, 0, 2 * c, 2 * c));
  gtk_snapshot_append_color (snapshot, &light, &GRAPHENE_RECT_INIT (0, 0, 2 * c, 2 * c));
  gtk_snapshot_append_color (snapshot, &dark, &GRAPHENE_RECT_INIT (c, 0, c, c));
  gtk_snapshot_append_color (snapshot, &dark, &GRAPHENE_RECT_INIT (0, c, c, c));
  gtk_snapshot_pop (snapshot);
}

static void
easel_canvas_snapshot (GtkWidget *widget, GtkSnapshot *snapshot)
{
  EaselCanvas *self = EASEL_CANVAS (widget);
  double l;
  graphene_rect_t bounds;

  if (self->texture == NULL)
    return;
  l = self->zoom / (100.0 * easel_canvas_get_scale (self));
  bounds = GRAPHENE_RECT_INIT (0, 0, (float) (self->img->width * l), (float) (self->img->height * l));

  /* Transparent pixels show the checkerboard (REQ-0001-11); nearest filtering keeps pixels hard-edged
     at every zoom (REQ-0001-18). */
  easel_snapshot_append_checkerboard (snapshot, &bounds, CHECKER_SIZE);
  gtk_snapshot_append_scaled_texture (snapshot, self->texture, GSK_SCALING_FILTER_NEAREST, &bounds);
}

static GdkCursor *
pencil_cursor (void)
{
  /* Created once and kept for the life of the process, like named cursors. */
  static GdkCursor *pencil;

  if (pencil == NULL)
    {
      g_autoptr (GdkTexture) texture = gdk_texture_new_from_resource (RESOURCE_PATH "/cursors/pencil.svg");
      g_autoptr (GdkCursor) fallback = gdk_cursor_new_from_name ("crosshair", NULL);

      /* The pencil's tip is the hotspot. */
      pencil = gdk_cursor_new_from_texture (texture, 1, 31, fallback);
    }
  return pencil;
}

/* The cursor shows the tool over the whole canvas area, not only over the picture (REQ-0001-10). */
static void
update_cursor (EaselCanvas *self)
{
  g_autoptr (GdkCursor) cursor = NULL;

  if (self->tool == EASEL_TOOL_PENCIL)
    cursor = g_object_ref (pencil_cursor ());
  else
    cursor = gdk_cursor_new_from_name (self->panning ? "grabbing" : "grab", NULL);

  gtk_widget_set_cursor (GTK_WIDGET (self), cursor);
  if (self->area != NULL)
    gtk_widget_set_cursor (self->area, cursor);
}

void
easel_canvas_set_input_blocked (EaselCanvas *self, gboolean blocked)
{
  self->input_blocked = blocked;
}

void
easel_canvas_set_history (EaselCanvas *self, EaselHistory *history)
{
  self->history = history;
}

void
easel_canvas_set_tool (EaselCanvas *self, EaselTool tool)
{
  self->tool = tool;
  update_cursor (self);
}

void
easel_canvas_set_colors (EaselCanvas *self, uint32_t foreground, uint32_t background)
{
  self->foreground = foreground;
  self->background = background;
}

gboolean
easel_canvas_is_stroking (EaselCanvas *self)
{
  return self->stroke != NULL;
}

static GtkAdjustment *
adjustment (EaselCanvas *self, GtkOrientation orientation)
{
  GtkScrollable *scrollable = GTK_SCROLLABLE (gtk_widget_get_parent (GTK_WIDGET (self)));

  if (orientation == GTK_ORIENTATION_HORIZONTAL)
    return gtk_scrollable_get_hadjustment (scrollable);
  return gtk_scrollable_get_vadjustment (scrollable);
}

static void
to_pixel (EaselCanvas *self, double x, double y, int *px, int *py)
{
  easel_view_to_pixel (x, y, self->zoom, easel_canvas_get_scale (self), px, py);
}

void
easel_canvas_cancel_stroke (EaselCanvas *self)
{
  if (self->stroke == NULL)
    return;
  easel_stroke_cancel (self->stroke);
  self->stroke = NULL;
  self->cancelled_button = self->stroke_button;
  easel_canvas_update (self);
}

void
easel_canvas_reset_pointer (EaselCanvas *self)
{
  easel_canvas_end_stroke (self);
  self->cancelled_button = 0;
  if (self->panning)
    {
      self->panning = FALSE;
      update_cursor (self);
    }
}

gboolean
easel_canvas_is_waiting_for_release (EaselCanvas *self)
{
  return self->cancelled_button != 0;
}

void
easel_canvas_end_stroke (EaselCanvas *self)
{
  size_t changed;

  if (self->stroke == NULL)
    return;
  changed = easel_stroke_end (self->stroke, self->history);
  self->stroke = NULL;
  g_signal_emit (self, signals[SIGNAL_STROKE_FINISHED], 0, (guint) changed);
}

void
easel_canvas_press (EaselCanvas *self, guint button, double x, double y)
{
  int px, py;

  /* Only the primary and secondary buttons draw; others are ignored (REQ-0001-24). */
  if (self->input_blocked || (button != GDK_BUTTON_PRIMARY && button != GDK_BUTTON_SECONDARY))
    return;

  if (self->stroke != NULL)
    {
      /* The other button cancels the stroke (REQ-0001-23). */
      if (button != self->stroke_button)
        {
          easel_stroke_cancel (self->stroke);
          self->stroke = NULL;
          easel_canvas_update (self);
        }
      return;
    }

  if (self->tool == EASEL_TOOL_PAN)
    {
      /* Only a primary drag pans (REQ-0001-14). */
      if (button == GDK_BUTTON_PRIMARY)
        {
          self->panning = TRUE;
          self->pan_x = x;
          self->pan_y = y;
          update_cursor (self);
        }
      return;
    }

  if (self->tool != EASEL_TOOL_PENCIL || self->img == NULL)
    return;
  to_pixel (self, x, y, &px, &py);
  self->stroke = easel_stroke_begin (self->img, px, py,
                                     button == GDK_BUTTON_PRIMARY ? self->foreground : self->background);
  if (self->stroke == NULL)
    {
      g_critical ("Out of memory starting a stroke");
      return;
    }
  self->stroke_button = button;
  easel_canvas_update (self);
}

void
easel_canvas_motion (EaselCanvas *self, double x, double y)
{
  int px, py;

  if (self->panning)
    {
      /* Keep the canvas point grabbed at the press under the pointer; the adjustments stop at the
         picture's edges. */
      GtkAdjustment *h = adjustment (self, GTK_ORIENTATION_HORIZONTAL);
      GtkAdjustment *v = adjustment (self, GTK_ORIENTATION_VERTICAL);

      gtk_adjustment_set_value (h, gtk_adjustment_get_value (h) + self->pan_x - x);
      gtk_adjustment_set_value (v, gtk_adjustment_get_value (v) + self->pan_y - y);
      return;
    }

  if (self->stroke == NULL)
    return;
  to_pixel (self, x, y, &px, &py);
  easel_stroke_to (self->stroke, px, py);
  easel_canvas_update (self);
}

void
easel_canvas_release (EaselCanvas *self, guint button)
{
  if (button == self->cancelled_button)
    self->cancelled_button = 0;
  if (self->panning && button == GDK_BUTTON_PRIMARY)
    {
      self->panning = FALSE;
      update_cursor (self);
    }
  if (self->stroke != NULL && button == self->stroke_button)
    easel_canvas_end_stroke (self);
}

void
easel_canvas_zoom_at (EaselCanvas *self, int notches, double view_x, double view_y)
{
  GtkAdjustment *h = adjustment (self, GTK_ORIENTATION_HORIZONTAL);
  GtkAdjustment *v = adjustment (self, GTK_ORIENTATION_VERTICAL);
  double scale = easel_canvas_get_scale (self);
  int zoom;

  if (self->img == NULL || self->tool != EASEL_TOOL_PAN)
    return;
  zoom = easel_view_zoom_step (self->zoom, notches);
  if (zoom == self->zoom)
    return;

  /* The new offsets can only be set once the adjustments know the new size, so they wait for allocation. */
  self->anchor_h = easel_view_anchor (gtk_adjustment_get_value (h), view_x, self->zoom, zoom, self->img->width,
                                      scale, gtk_adjustment_get_page_size (h));
  self->anchor_v = easel_view_anchor (gtk_adjustment_get_value (v), view_y, self->zoom, zoom, self->img->height,
                                      scale, gtk_adjustment_get_page_size (v));
  self->anchor_pending = TRUE;
  self->zoom = zoom;
  gtk_widget_queue_resize (GTK_WIDGET (self));
}

static gboolean
on_scroll (GtkEventControllerScroll *controller, double dx, double dy, EaselCanvas *self)
{
  GdkModifierType state = gtk_event_controller_get_current_event_state (GTK_EVENT_CONTROLLER (controller));
  GdkEvent *event = gtk_event_controller_get_current_event (GTK_EVENT_CONTROLLER (controller));
  GtkWidget *area = gtk_event_controller_get_widget (GTK_EVENT_CONTROLLER (controller));
  GtkNative *native = gtk_widget_get_native (area);
  graphene_point_t p = GRAPHENE_POINT_INIT (0, 0);
  double x, y, nx, ny;
  int notches;

  /* Without Ctrl, or with Ctrl in another tool, the wheel scrolls as usual (REQ-0001-15, REQ-0001-17). */
  if (!(state & GDK_CONTROL_MASK) || self->tool != EASEL_TOOL_PAN)
    return GDK_EVENT_PROPAGATE;

  if (gtk_event_controller_scroll_get_unit (controller) == GDK_SCROLL_UNIT_SURFACE)
    dy /= PIXELS_PER_NOTCH;
  notches = easel_view_take_notches (&self->notch_acc, dy);

  if (event != NULL && gdk_event_get_position (event, &x, &y))
    {
      gtk_native_get_surface_transform (native, &nx, &ny);
      if (!gtk_widget_compute_point (GTK_WIDGET (native), area,
                                     &GRAPHENE_POINT_INIT ((float) (x - nx), (float) (y - ny)), &p))
        p = GRAPHENE_POINT_INIT (0, 0);
    }
  if (notches != 0)
    easel_canvas_zoom_at (self, notches, p.x, p.y);
  return GDK_EVENT_STOP;
}

static gboolean
on_event (GtkEventControllerLegacy *controller, GdkEvent *event, EaselCanvas *self)
{
  GdkEventType type = gdk_event_get_event_type (event);
  double x, y;

  GtkWidget *area = gtk_event_controller_get_widget (GTK_EVENT_CONTROLLER (controller));

  if (type != GDK_BUTTON_PRESS && type != GDK_BUTTON_RELEASE && type != GDK_MOTION_NOTIFY)
    return GDK_EVENT_PROPAGATE;

  /* Event coordinates are relative to the surface; the canvas needs its own. */
  if (gdk_event_get_position (event, &x, &y))
    {
      GtkNative *native = gtk_widget_get_native (GTK_WIDGET (self));
      graphene_point_t p;
      double nx, ny;

      gtk_native_get_surface_transform (native, &nx, &ny);
      x -= nx;
      y -= ny;

      /* Presses on the scrollbars belong to them. */
      if (type == GDK_BUTTON_PRESS && self->stroke == NULL)
        {
          GtkWidget *picked;

          if (!gtk_widget_compute_point (GTK_WIDGET (native), area, &GRAPHENE_POINT_INIT ((float) x, (float) y), &p))
            return GDK_EVENT_PROPAGATE;
          picked = gtk_widget_pick (area, p.x, p.y, GTK_PICK_DEFAULT);
          if (picked != NULL && gtk_widget_get_ancestor (picked, GTK_TYPE_SCROLLBAR) != NULL)
            return GDK_EVENT_PROPAGATE;
        }

      if (!gtk_widget_compute_point (GTK_WIDGET (native), GTK_WIDGET (self),
                                     &GRAPHENE_POINT_INIT ((float) x, (float) y), &p))
        return GDK_EVENT_PROPAGATE;
      x = p.x;
      y = p.y;
    }

  if (type == GDK_BUTTON_PRESS)
    easel_canvas_press (self, gdk_button_event_get_button (event), x, y);
  else if (type == GDK_BUTTON_RELEASE)
    easel_canvas_release (self, gdk_button_event_get_button (event));
  else
    easel_canvas_motion (self, x, y);
  return GDK_EVENT_STOP;
}

void
easel_canvas_listen_on (EaselCanvas *self, GtkWidget *area)
{
  GtkEventController *legacy = gtk_event_controller_legacy_new ();

  self->area = area;
  update_cursor (self);
  GtkEventController *scroll = gtk_event_controller_scroll_new (GTK_EVENT_CONTROLLER_SCROLL_VERTICAL);

  g_signal_connect_object (legacy, "event", G_CALLBACK (on_event), self, G_CONNECT_DEFAULT);
  gtk_widget_add_controller (area, legacy);

  /* Capture phase: Ctrl+wheel must reach us before the scrolled window scrolls. */
  gtk_event_controller_set_propagation_phase (scroll, GTK_PHASE_CAPTURE);
  g_signal_connect_object (scroll, "scroll", G_CALLBACK (on_scroll), self, G_CONNECT_DEFAULT);
  gtk_widget_add_controller (area, scroll);
}

static void
on_scale_changed (EaselCanvas *self)
{
  gtk_widget_queue_resize (GTK_WIDGET (self));
}

static void
easel_canvas_realize (GtkWidget *widget)
{
  EaselCanvas *self = EASEL_CANVAS (widget);

  GTK_WIDGET_CLASS (easel_canvas_parent_class)->realize (widget);
  self->surface = gtk_native_get_surface (gtk_widget_get_native (widget));
  g_signal_connect_object (self->surface, "notify::scale", G_CALLBACK (on_scale_changed), self, G_CONNECT_SWAPPED);
  gtk_widget_queue_resize (widget);
}

static void
easel_canvas_unrealize (GtkWidget *widget)
{
  EaselCanvas *self = EASEL_CANVAS (widget);

  g_signal_handlers_disconnect_by_func (self->surface, on_scale_changed, self);
  self->surface = NULL;
  GTK_WIDGET_CLASS (easel_canvas_parent_class)->unrealize (widget);
}

static void
easel_canvas_size_allocate (GtkWidget *widget, int width, int height, int baseline)
{
  EaselCanvas *self = EASEL_CANVAS (widget);

  /* The viewport has configured the adjustments for the new size before allocating the canvas. */
  if (self->anchor_pending)
    {
      self->anchor_pending = FALSE;
      gtk_adjustment_set_value (adjustment (self, GTK_ORIENTATION_HORIZONTAL), self->anchor_h);
      gtk_adjustment_set_value (adjustment (self, GTK_ORIENTATION_VERTICAL), self->anchor_v);
    }
}

static void
easel_canvas_dispose (GObject *object)
{
  EaselCanvas *self = EASEL_CANVAS (object);

  if (self->stroke != NULL)
    {
      easel_stroke_cancel (self->stroke);
      self->stroke = NULL;
    }
  self->img = NULL;
  self->history = NULL;
  self->area = NULL;
  g_clear_object (&self->texture);
  g_clear_object (&self->builder);
  G_OBJECT_CLASS (easel_canvas_parent_class)->dispose (object);
}

static void
easel_canvas_class_init (EaselCanvasClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->dispose = easel_canvas_dispose;
  widget_class->measure = easel_canvas_measure;
  widget_class->snapshot = easel_canvas_snapshot;
  widget_class->size_allocate = easel_canvas_size_allocate;
  widget_class->realize = easel_canvas_realize;
  widget_class->unrealize = easel_canvas_unrealize;

  signals[SIGNAL_STROKE_FINISHED] = g_signal_new ("stroke-finished", G_TYPE_FROM_CLASS (klass), G_SIGNAL_RUN_LAST,
                                                  0, NULL, NULL, NULL, G_TYPE_NONE, 1, G_TYPE_UINT);
}

static void
easel_canvas_init (EaselCanvas *self)
{
  self->zoom = 100;
  self->builder = gdk_memory_texture_builder_new ();
  update_cursor (self);
  gtk_widget_set_halign (GTK_WIDGET (self), GTK_ALIGN_START);
  gtk_widget_set_valign (GTK_WIDGET (self), GTK_ALIGN_START);
}
