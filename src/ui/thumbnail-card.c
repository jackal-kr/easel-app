/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "thumbnail-card.h"

#include "view.h"

#define AREA_WIDTH 160
#define AREA_HEIGHT 120
#define PADDING 6

struct _EaselThumbnailCard
{
  GtkWidget parent_instance;
  GtkWidget *area;                               /* the picture area: its background is the workspace color (style.css) */
  EaselCanvas *canvas;
  GtkScrolledWindow *scroller;
};

G_DEFINE_FINAL_TYPE (EaselThumbnailCard, easel_thumbnail_card, GTK_TYPE_WIDGET)

static void
redraw (EaselThumbnailCard *self)
{
  gtk_widget_queue_draw (GTK_WIDGET (self));
}

static void
easel_thumbnail_card_snapshot (GtkWidget *widget, GtkSnapshot *snapshot)
{
  EaselThumbnailCard *self = EASEL_THUMBNAIL_CARD (widget);
  graphene_rect_t r;
  double scale;
  int zoom, x0, y0, w, h;
  const EaselImage *img;
  g_autoptr (GdkTexture) texture = NULL;

  gtk_widget_snapshot_child (widget, self->area, snapshot);
  if (self->canvas == NULL || (img = easel_canvas_get_image (self->canvas)) == NULL
      || !gtk_widget_compute_bounds (self->area, widget, &r))
    return;
  scale = easel_canvas_get_scale (self->canvas);
  zoom = easel_canvas_get_zoom (self->canvas);
  x0 = easel_view_first_cell (gtk_adjustment_get_value (gtk_scrolled_window_get_hadjustment (self->scroller)), EASEL_VIEW_MARGIN, zoom, scale);
  y0 = easel_view_first_cell (gtk_adjustment_get_value (gtk_scrolled_window_get_vadjustment (self->scroller)), EASEL_VIEW_MARGIN, zoom, scale);
  w = MIN ((int) (AREA_WIDTH * scale), img->width - x0);
  h = MIN ((int) (AREA_HEIGHT * scale), img->height - y0);
  if (w <= 0 || h <= 0)
    return;
  /* With no memory for it, this frame shows the workspace color. */
  texture = easel_canvas_compose_view (self->canvas, x0, y0, w, h);
  if (texture == NULL)
    return;
  gtk_snapshot_save (snapshot);
  gtk_snapshot_translate (snapshot, &r.origin);
  {
    graphene_rect_t picture = GRAPHENE_RECT_INIT (0, 0, (float) (w / scale), (float) (h / scale));

    easel_canvas_append_checkerboard (snapshot, &picture);
    gtk_snapshot_append_texture (snapshot, texture, &picture);   /* device size: nothing to scale, nothing blended at scale 2 */
  }
  gtk_snapshot_restore (snapshot);
}

static void
easel_thumbnail_card_dispose (GObject *object)
{
  EaselThumbnailCard *self = EASEL_THUMBNAIL_CARD (object);

  g_clear_pointer (&self->area, gtk_widget_unparent);
  G_OBJECT_CLASS (easel_thumbnail_card_parent_class)->dispose (object);
}

static void
easel_thumbnail_card_class_init (EaselThumbnailCardClass *klass)
{
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  G_OBJECT_CLASS (klass)->dispose = easel_thumbnail_card_dispose;
  widget_class->snapshot = easel_thumbnail_card_snapshot;
  gtk_widget_class_set_layout_manager_type (widget_class, GTK_TYPE_BIN_LAYOUT);
}

static void
easel_thumbnail_card_init (EaselThumbnailCard *self)
{
  GtkWidget *widget = GTK_WIDGET (self);

  gtk_widget_add_css_class (widget, "card");
  gtk_widget_add_css_class (widget, "font-card");       /* the font card's look (REQ-0021-07) */
  gtk_widget_set_cursor_from_name (widget, "default");
  gtk_accessible_update_property (GTK_ACCESSIBLE (widget), GTK_ACCESSIBLE_PROPERTY_LABEL, "Thumbnail", -1);
  self->area = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  gtk_widget_add_css_class (self->area, "thumbnail-area");
  gtk_widget_set_size_request (self->area, AREA_WIDTH, AREA_HEIGHT);
  gtk_widget_set_margin_start (self->area, PADDING);
  gtk_widget_set_margin_end (self->area, PADDING);
  gtk_widget_set_margin_top (self->area, PADDING);
  gtk_widget_set_margin_bottom (self->area, PADDING);
  gtk_widget_set_parent (self->area, widget);
}

void
easel_thumbnail_card_set_view (EaselThumbnailCard *self, EaselCanvas *canvas, GtkScrolledWindow *scroller)
{
  self->canvas = canvas;
  self->scroller = scroller;
  g_signal_connect_object (canvas, "painted", G_CALLBACK (redraw), self, G_CONNECT_SWAPPED);
  g_signal_connect_object (gtk_scrolled_window_get_hadjustment (scroller), "value-changed", G_CALLBACK (redraw), self, G_CONNECT_SWAPPED);
  g_signal_connect_object (gtk_scrolled_window_get_vadjustment (scroller), "value-changed", G_CALLBACK (redraw), self, G_CONNECT_SWAPPED);
}

GtkWidget *
easel_thumbnail_card_get_area (EaselThumbnailCard *self)
{
  return self->area;
}
