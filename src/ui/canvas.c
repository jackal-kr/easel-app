/* SPDX-License-Identifier: GPL-3.0-or-later */

#include <math.h>

#include "config.h"
#include "canvas.h"
#include "cursor-shape.h"
#include "fill.h"
#include "figure.h"
#include "grid.h"
#include "magnifier.h"
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
  GdkTexture **tiles;     /* tile_cols x tile_rows textures, row by row; see EASEL_CANVAS_TILE */
  int tile_cols, tile_rows;
  gsize tiles_rebuilt;    /* test hook, see easel_canvas_get_tiles_rebuilt () */
  gsize bytes_copied;     /* test hook: always 0, the tiles read the picture's own memory */
  int zoom;
  GdkSurface *surface;    /* watched for scale changes while realized */

  EaselTool tool;
  uint32_t foreground;
  uint32_t background;
  EaselHistory *history;  /* not owned */
  EaselStroke *stroke;
  EaselFootprint footprint;   /* of the Brush, the Airbrush or the Eraser, whichever is active */
  guint spray_source;         /* the 5 ms timer of a spray stroke, 0 when none runs */
  gint64 spray_start_us;      /* when the button was pressed */
  int sprays_made;
  int spray_x, spray_y;       /* the pointer's pixel, where the next sprays land */
  gboolean eraser_transparent;
  GdkCursor *outline_cursor;  /* the footprint's outline for cursor_footprint at cursor_zoom, NULL until made */
  EaselFootprint cursor_footprint;
  int cursor_zoom;
  guint stroke_button;
  guint cancelled_button; /* button of a stroke cancelled by Undo, until it is released; 0 if none */
  gboolean picking;       /* a Pick Color press is held: a pending gesture, treated like a stroke (REQ-0007-08) */
  int pick_px, pick_py;   /* the pixel the pick showed last */
  int magnifier_level;    /* the Magnifier's selected level in percent (REQ-0007-02) */
  gboolean pointer_in;    /* the pointer is over the canvas area (REQ-0007-04) */
  double pointer_ax, pointer_ay;  /* where, relative to the area (so a scroll or a zoom moves the picture under it) */
  int box_px, box_py;     /* the pixel the preview box was last drawn for, to redraw only when it changes */
  size_t fill_max_bytes;  /* test hook: what a Fill may use for its work buffers */
  size_t shape_max_bytes; /* test hook: what a shape's undo log may use */
  gboolean shaping;       /* a shape drag is held: a pending gesture of Line, Rectangle, Rounded Rectangle or Ellipse (0010) */
  guint shape_button;
  int shape_x0, shape_y0, shape_x1, shape_y1;   /* the start pixel and the end pixel (the pointer's, or Shift's constraint of it) */
  int shape_px, shape_py;                       /* the pixel under the pointer now */
  gboolean shift;                               /* Shift is down (REQ-0010-14) */
  uint32_t shape_draw, shape_other;             /* the colors of the press, kept until the release (REQ-0010-06) */
  int shape_width;                              /* 1 to 5, and the fill style: set by the window from its two actions */
  EaselFigureStyle shape_style;
  /* A pending curve or polygon (0011): from its first press until it is put down or dropped. */
  gboolean pending;
  gboolean pend_held;                           /* a stage's button is held */
  int pend_step;                                /* a curve: 0 the line, 1 the first bend, 2 the second bend (held now, or done last) */
  guint pend_button;                            /* the button of the first press, used by every stage (REQ-0011-10) */
  int pend_pts[8];                              /* a curve: the start, the two control points and the end */
  int pend_px, pend_py;                         /* the pixel under the pointer now */
  uint32_t pend_draw, pend_other;               /* the colors of the first press */
  gboolean pend_polygon;                        /* the pending shape is a polygon, not a curve */
  guint click_button;                           /* the last press: its button, time (ms), place and the count of clicks in a row (REQ-0011-07) */
  guint32 click_time;
  double click_x, click_y;
  int click_count;
  gboolean dbl_closed;                          /* the last double-click closed a polygon: a third click is ignored */
  gboolean pend_dbl;                            /* the held side's press was the second click of a double-click */
  double pend_wx, pend_wy;                      /* the pointer's place in the widget, and where the held side was pressed */
  double press_wx, press_wy;
  int *poly;                                    /* a polygon's corners as x, y pairs; the last one follows the pointer while held */
  size_t poly_n, poly_cap;
  guint action_button;    /* button held after a press that acted at once (Fill): no other press acts until it is released */
  gboolean input_blocked;
  gboolean busy;          /* a picture is loading: the busy cursor */
  gboolean grid_on, grid_axonometric;
  int grid_cell;
  GtkWidget *area;        /* ancestor that feeds input; not owned */

  EaselHandle hover;      /* the handle under the pointer, for the cursor */
  EaselHandle resizing;   /* the handle being dragged, or NONE */
  guint resize_button;
  double resize_x, resize_y;  /* canvas point where the drag began */
  int resize_w, resize_h;     /* the picture's size at the press */
  int outline_w, outline_h;   /* the size the drag gives now */

  gboolean panning;
  double pan_x, pan_y;    /* canvas point under the pointer when the pan began */
  double notch_acc;       /* touchpad scrolling not yet a whole notch */
  double pinch_base;      /* the pinch's scale at the last level change, 1 when it begins */
  gboolean anchor_pending;
  double anchor_h, anchor_v;  /* scroll offsets to apply once the new zoom is allocated */
  gboolean anchor_focus;      /* instead: the picture point focus_x, focus_y goes to the center of the view as allocated */
  double focus_x, focus_y;
};

/* Touchpad scrolling arrives in surface pixels; this many count as one wheel notch. */
#define PIXELS_PER_NOTCH 30.0

enum
{
  SIGNAL_STROKE_FINISHED,
  SIGNAL_STROKING_CHANGED,
  SIGNAL_FILL_FAILED,
  SIGNAL_SHAPE_FAILED,
  SIGNAL_PENDING_CHANGED,
  SIGNAL_COLOR_PICKED,
  SIGNAL_PICK_ENDED,
  SIGNAL_ZOOM_CHANGED,
  SIGNAL_POINTER_MOVED,
  SIGNAL_POINTER_LEFT,
  SIGNAL_RESIZE_REQUESTED,
  N_SIGNALS
};

static guint signals[N_SIGNALS];

static GtkAdjustment *adjustment (EaselCanvas *self, GtkOrientation orientation);
static void to_pixel (EaselCanvas *self, double x, double y, int *px, int *py);
static void update_cursor (EaselCanvas *self);
static void end_shape (EaselCanvas *self);
static void stop_spray (EaselCanvas *self);
static void commit_zoom (EaselCanvas *self, int zoom, double scroll_h, double scroll_v);
static void pend_follow (EaselCanvas *self);
static gboolean within_double_click_distance (EaselCanvas *self, double x, double y, double ox, double oy);
static EaselFigure pending_figure (EaselCanvas *self);
static gboolean on_spray_tick (gpointer data);

/* Resize handles (REQ-0004-16): 5 x 5 logical pixels touching the picture, a hit area of 9 x 9 around the center. They
   reach 5 + 2 pixels past the picture, inside the margin of EASEL_VIEW_MARGIN around it (REQ-0009-01). */
#define HANDLE_SIZE 5.0
#define HANDLE_HIT 9.0

static gboolean
handles_shown (EaselCanvas *self)
{
  return self->img != NULL && self->tool != EASEL_TOOL_PAN;
}

static double
logical_per_pixel (EaselCanvas *self)
{
  return self->zoom / (100.0 * easel_canvas_get_scale (self));
}

G_DEFINE_FINAL_TYPE (EaselCanvas, easel_canvas, GTK_TYPE_WIDGET)

GdkTexture *
easel_canvas_get_tile_texture (EaselCanvas *self, int tx, int ty)
{
  if (self->tiles == NULL || tx < 0 || ty < 0 || tx >= self->tile_cols || ty >= self->tile_rows)
    return NULL;
  return self->tiles[ty * self->tile_cols + tx];
}

gsize
easel_canvas_get_tiles_rebuilt (EaselCanvas *self)
{
  return self->tiles_rebuilt;
}

gsize
easel_canvas_get_bytes_copied (EaselCanvas *self)
{
  return self->bytes_copied;
}

static void
free_tiles (EaselCanvas *self)
{
  for (int i = 0; self->tiles != NULL && i < self->tile_cols * self->tile_rows; i++)
    g_clear_object (&self->tiles[i]);
  g_clear_pointer (&self->tiles, g_free);
  self->tile_cols = self->tile_rows = 0;
}

/* One tile's texture: a view of the picture's own memory (no copy; its rows are the picture's rows, so the stride is
   the picture's), rebuilt whenever a pixel in it changes. The memory stays valid because the picture's buffer is only
   replaced by a resize, which points the canvas at the new buffer in the same call, and the canvas drops its tiles
   before the picture is freed. */
static void
build_tile (EaselCanvas *self, int tx, int ty)
{
  const EaselImage *img = self->img;
  int x0 = tx * EASEL_CANVAS_TILE, y0 = ty * EASEL_CANVAS_TILE;
  int w = MIN (EASEL_CANVAS_TILE, img->width - x0), h = MIN (EASEL_CANVAS_TILE, img->height - y0);
  gsize stride = (gsize) img->width * sizeof (uint32_t);
  g_autoptr (GBytes) bytes = g_bytes_new_static (img->px + (gsize) y0 * (gsize) img->width + (gsize) x0,
                                                 (gsize) (h - 1) * stride + (gsize) w * sizeof (uint32_t));

  gdk_memory_texture_builder_set_bytes (self->builder, bytes);
  gdk_memory_texture_builder_set_width (self->builder, w);
  gdk_memory_texture_builder_set_height (self->builder, h);
  gdk_memory_texture_builder_set_stride (self->builder, stride);
  gdk_memory_texture_builder_set_format (self->builder, PICTURE_FORMAT);
  g_clear_object (&self->tiles[ty * self->tile_cols + tx]);
  self->tiles[ty * self->tile_cols + tx] = gdk_memory_texture_builder_build (self->builder);
  self->tiles_rebuilt++;
}

void
easel_canvas_set_image (EaselCanvas *self, EaselImage *img)
{
  EaselRect ignored;

  free_tiles (self);
  self->img = img;
  self->resizing = EASEL_HANDLE_NONE;
  self->hover = EASEL_HANDLE_NONE;
  self->tiles_rebuilt = 0;
  if (img != NULL)
    {
      /* Every tile is new, so earlier dirty marks are already covered. */
      easel_image_take_dirty (img, &ignored);
      self->tile_cols = (img->width + EASEL_CANVAS_TILE - 1) / EASEL_CANVAS_TILE;
      self->tile_rows = (img->height + EASEL_CANVAS_TILE - 1) / EASEL_CANVAS_TILE;
      self->tiles = g_new0 (GdkTexture *, (gsize) self->tile_cols * (gsize) self->tile_rows);
      for (int ty = 0; ty < self->tile_rows; ty++)
        for (int tx = 0; tx < self->tile_cols; tx++)
          build_tile (self, tx, ty);
      self->tiles_rebuilt = 0;
    }
  gtk_widget_queue_resize (GTK_WIDGET (self));
}

void
easel_canvas_update (EaselCanvas *self)
{
  EaselRect r;

  if (self->img == NULL || !easel_image_take_dirty (self->img, &r))
    return;
  /* Only the tiles that hold a changed pixel: r is half-open. */
  for (int ty = r.y0 / EASEL_CANVAS_TILE; ty <= (r.y1 - 1) / EASEL_CANVAS_TILE; ty++)
    for (int tx = r.x0 / EASEL_CANVAS_TILE; tx <= (r.x1 - 1) / EASEL_CANVAS_TILE; tx++)
      build_tile (self, tx, ty);
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

gboolean
easel_canvas_get_handle_rect (EaselCanvas *self, EaselHandle handle, graphene_rect_t *rect)
{
  double l, pw, ph, x, y;

  if (!handles_shown (self) || handle == EASEL_HANDLE_NONE)
    return FALSE;
  l = logical_per_pixel (self);
  pw = self->img->width * l;
  ph = self->img->height * l;
  x = handle == EASEL_HANDLE_BOTTOM ? pw / 2 - HANDLE_SIZE / 2 : pw;
  y = handle == EASEL_HANDLE_RIGHT ? ph / 2 - HANDLE_SIZE / 2 : ph;
  *rect = GRAPHENE_RECT_INIT ((float) x, (float) y, HANDLE_SIZE, HANDLE_SIZE);
  return TRUE;
}

EaselHandle
easel_canvas_handle_at (EaselCanvas *self, double x, double y)
{
  /* The corner first: where hit areas overlap it wins. */
  static const EaselHandle order[] = { EASEL_HANDLE_CORNER, EASEL_HANDLE_RIGHT, EASEL_HANDLE_BOTTOM };

  for (guint i = 0; i < G_N_ELEMENTS (order); i++)
    {
      graphene_rect_t r;
      double cx, cy;

      if (!easel_canvas_get_handle_rect (self, order[i], &r))
        return EASEL_HANDLE_NONE;
      cx = r.origin.x + r.size.width / 2;
      cy = r.origin.y + r.size.height / 2;
      if (fabs (x - cx) <= HANDLE_HIT / 2 && fabs (y - cy) <= HANDLE_HIT / 2)
        return order[i];
    }
  return EASEL_HANDLE_NONE;
}

gboolean
easel_canvas_is_resizing (EaselCanvas *self)
{
  return self->resizing != EASEL_HANDLE_NONE;
}

gboolean
easel_canvas_get_resize_outline (EaselCanvas *self, int *width, int *height)
{
  if (self->resizing == EASEL_HANDLE_NONE)
    return FALSE;
  *width = self->outline_w;
  *height = self->outline_h;
  return TRUE;
}

/* A drag that ends or is cancelled: forget it and show the tool's cursor again. */
static void
stop_resizing (EaselCanvas *self)
{
  self->resizing = EASEL_HANDLE_NONE;
  update_cursor (self);
  gtk_widget_queue_draw (GTK_WIDGET (self));
}

void
easel_canvas_cancel_resize (EaselCanvas *self)
{
  if (self->resizing == EASEL_HANDLE_NONE)
    return;
  /* The press is over for good, but its button may still be held (REQ-0004-19). */
  self->cancelled_button = self->resize_button;
  stop_resizing (self);
}

/* The size a drag to the canvas point (x, y) gives: the movement since the press, in picture pixels (REQ-0004-18). */
static void
resize_to (EaselCanvas *self, double x, double y)
{
  double l = logical_per_pixel (self);

  self->outline_w = self->resize_w;
  self->outline_h = self->resize_h;
  easel_view_resize_by (self->resizing, (x - self->resize_x) / l, (y - self->resize_y) / l, &self->outline_w,
                        &self->outline_h);
  gtk_widget_queue_draw (GTK_WIDGET (self));
}

static void
easel_canvas_measure (GtkWidget *widget, GtkOrientation orientation, int for_size,
                      int *minimum, int *natural, int *minimum_baseline, int *natural_baseline)
{
  EaselCanvas *self = EASEL_CANVAS (widget);
  int size = 0;

  /* The picture only: the handles lie in the margin around it, drawn past the canvas's edge (REQ-0009-01,
     REQ-0004-16). */
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

typedef struct
{
  cairo_t *cr;
  double l;         /* logical pixels per picture pixel */
  double scale;     /* device pixels per logical pixel */
} GridDraw;

/* Picture coordinates to canvas coordinates, putting the line on a whole device pixel: a vertical or horizontal line
   covers the device pixel that starts at the boundary, and any other line the pixels it passes through. */
static void
grid_segment (double x0, double y0, double x1, double y1, void *data)
{
  GridDraw *d = data;
  double ax = x0 * d->l, ay = y0 * d->l, bx = x1 * d->l, by = y1 * d->l;
  double half = 0.5 / d->scale;

  if (x0 == x1)
    ax = bx = floor (ax * d->scale) / d->scale + half;
  else if (y0 == y1)
    ay = by = floor (ay * d->scale) / d->scale + half;
  cairo_move_to (d->cr, ax, ay);
  cairo_line_to (d->cr, bx, by);
}

/* The grid over the visible part of the picture, as one Cairo node of that size with antialiasing off, so the lines
   are hard-edged: 1 device pixel wide, mid grey at 50 % (REQ-0004-12). */
static void
append_grid (EaselCanvas *self, GtkSnapshot *snapshot)
{
  double scale = easel_canvas_get_scale (self);
  double l = self->zoom / (100.0 * scale);
  graphene_rect_t seen;
  double vx0, vy0, vx1, vy1;
  GridDraw d = { NULL, l, scale };

  /* The part of the picture the view shows, in the canvas's own coordinates: the viewport's bounds seen from the canvas,
     so that the margin around the picture (REQ-0009-01) needs no arithmetic here. */
  if (!gtk_widget_compute_bounds (gtk_widget_get_parent (GTK_WIDGET (self)), GTK_WIDGET (self), &seen))
    return;
  vx0 = fmax (0, seen.origin.x);
  vy0 = fmax (0, seen.origin.y);
  vx1 = fmin (self->img->width * l, seen.origin.x + seen.size.width);
  vy1 = fmin (self->img->height * l, seen.origin.y + seen.size.height);

  if (vx1 <= vx0 || vy1 <= vy0)
    return;
  d.cr = gtk_snapshot_append_cairo (snapshot, &GRAPHENE_RECT_INIT ((float) vx0, (float) vy0, (float) (vx1 - vx0),
                                                                    (float) (vy1 - vy0)));
  /* The context draws in the snapshot's own coordinates, which are the canvas's: no translation, whatever the scroll
     (an offset here moved the lines out of the bounds once the view was scrolled by more than a page). */
  cairo_set_antialias (d.cr, CAIRO_ANTIALIAS_NONE);
  cairo_set_source_rgba (d.cr, 0.5, 0.5, 0.5, 0.5);
  cairo_set_line_width (d.cr, 1.0 / scale);
  /* The visible area in picture pixels, with a pixel to spare so a line on its edge is not lost. */
  easel_grid_lines (self->img->width, self->img->height, self->grid_cell, self->grid_axonometric,
                    vx0 / l - 1, vy0 / l - 1, vx1 / l + 1, vy1 / l + 1, grid_segment, &d);
  cairo_stroke (d.cr);
  cairo_destroy (d.cr);
}

/* The handles as small dark blue squares and, during a drag, the new picture's outline as a thin line (REQ-0004-16,
   REQ-0004-18). */
static void
append_handles (EaselCanvas *self, GtkSnapshot *snapshot)
{
  static const EaselHandle all[] = { EASEL_HANDLE_RIGHT, EASEL_HANDLE_BOTTOM, EASEL_HANDLE_CORNER };
  static const GdkRGBA navy = { 0.0f, 0.0f, 0.5f, 1.0f };
  graphene_rect_t r;

  for (guint i = 0; i < G_N_ELEMENTS (all); i++)
    if (easel_canvas_get_handle_rect (self, all[i], &r))
      gtk_snapshot_append_color (snapshot, &navy, &r);
  if (self->resizing != EASEL_HANDLE_NONE)
    {
      double l = logical_per_pixel (self), t = 1.0 / easel_canvas_get_scale (self);
      double w = self->outline_w * l, h = self->outline_h * l;
      static const GdkRGBA black = { 0, 0, 0, 1 };

      gtk_snapshot_append_color (snapshot, &black, &GRAPHENE_RECT_INIT (0, 0, (float) w, (float) t));
      gtk_snapshot_append_color (snapshot, &black, &GRAPHENE_RECT_INIT (0, (float) (h - t), (float) w, (float) t));
      gtk_snapshot_append_color (snapshot, &black, &GRAPHENE_RECT_INIT (0, 0, (float) t, (float) h));
      gtk_snapshot_append_color (snapshot, &black, &GRAPHENE_RECT_INIT ((float) (w - t), 0, (float) t, (float) h));
    }
}

/* The preview box (REQ-0007-04) in picture pixels, if it is shown now: the Magnifier at 100 % with a level that zooms in
   and the pointer over a picture pixel. */
static gboolean
magnifier_box (EaselCanvas *self, int *bx, int *by, int *bw, int *bh)
{
  GtkAdjustment *h, *v;
  graphene_point_t p;
  double scale;
  int px, py;

  if (self->img == NULL || self->tool != EASEL_TOOL_MAGNIFIER || self->zoom != 100 || self->magnifier_level <= 100
      || !self->pointer_in)
    return FALSE;
  if (!gtk_widget_compute_point (self->area != NULL ? self->area : GTK_WIDGET (self), GTK_WIDGET (self),
                                 &GRAPHENE_POINT_INIT ((float) self->pointer_ax, (float) self->pointer_ay), &p))
    return FALSE;
  to_pixel (self, p.x, p.y, &px, &py);
  if (px < 0 || py < 0 || px >= self->img->width || py >= self->img->height)
    return FALSE;
  h = adjustment (self, GTK_ORIENTATION_HORIZONTAL);
  v = adjustment (self, GTK_ORIENTATION_VERTICAL);
  scale = easel_canvas_get_scale (self);
  easel_magnifier_box ((int) (gtk_adjustment_get_page_size (h) * scale), (int) (gtk_adjustment_get_page_size (v) * scale),
                       self->magnifier_level, self->img->width, self->img->height, px, py, bx, by, bw, bh);
  self->box_px = px;
  self->box_py = py;
  return TRUE;
}

gboolean
easel_canvas_get_magnifier_box (EaselCanvas *self, int *x, int *y, int *w, int *h)
{
  return magnifier_box (self, x, y, w, h);
}

/* The preview box as a black line with a white line outside it, so that it shows on any color (REQ-0007-04). */
static void
append_magnifier_box (EaselCanvas *self, GtkSnapshot *snapshot)
{
  static const GdkRGBA black = { 0, 0, 0, 1 }, white = { 1, 1, 1, 1 };
  int bx, by, bw, bh;
  double l, t, x0, y0, w, h;

  if (!magnifier_box (self, &bx, &by, &bw, &bh))
    return;
  l = logical_per_pixel (self);
  t = 1.0 / easel_canvas_get_scale (self);
  x0 = bx * l;
  y0 = by * l;
  w = bw * l;
  h = bh * l;
  /* The black line is inside the box's edge, the white one just outside it. */
  for (int i = 0; i < 2; i++)
    {
      const GdkRGBA *c = i == 0 ? &white : &black;
      double o = i == 0 ? t : 0;     /* white: one line further out */
      double rx = x0 - o, ry = y0 - o, rw = w + 2 * o, rh = h + 2 * o;

      gtk_snapshot_append_color (snapshot, c, &GRAPHENE_RECT_INIT ((float) rx, (float) ry, (float) rw, (float) t));
      gtk_snapshot_append_color (snapshot, c, &GRAPHENE_RECT_INIT ((float) rx, (float) (ry + rh - t), (float) rw, (float) t));
      gtk_snapshot_append_color (snapshot, c, &GRAPHENE_RECT_INIT ((float) rx, (float) ry, (float) t, (float) rh));
      gtk_snapshot_append_color (snapshot, c, &GRAPHENE_RECT_INIT ((float) (rx + rw - t), (float) ry, (float) t, (float) rh));
    }
}

/* The end pixel follows the pointer, or the pointer's constraint while Shift is down (REQ-0010-14). */
static void
update_shape_end (EaselCanvas *self)
{
  EaselFigureKind kind = self->tool == EASEL_TOOL_LINE ? EASEL_FIGURE_LINE
                         : self->tool == EASEL_TOOL_RECTANGLE ? EASEL_FIGURE_RECTANGLE
                         : self->tool == EASEL_TOOL_ROUNDED_RECTANGLE ? EASEL_FIGURE_ROUNDED : EASEL_FIGURE_ELLIPSE;
  int x = self->shape_px, y = self->shape_py;

  if (self->shift)
    easel_figure_constrain (kind, self->shape_x0, self->shape_y0, &x, &y);
  if (x != self->shape_x1 || y != self->shape_y1)
    {
      self->shape_x1 = x;
      self->shape_y1 = y;
      gtk_widget_queue_draw (GTK_WIDGET (self));
    }
}

void
easel_canvas_set_shift (EaselCanvas *self, gboolean down)
{
  self->shift = down;
  if (self->shaping)
    update_shape_end (self);
  if (self->pend_held)
    pend_follow (self);
}

/* The shape being dragged, as the core describes it (REQ-0010-05). */
static EaselFigure
current_figure (EaselCanvas *self)
{
  EaselFigureKind kind = self->tool == EASEL_TOOL_LINE ? EASEL_FIGURE_LINE
                         : self->tool == EASEL_TOOL_RECTANGLE ? EASEL_FIGURE_RECTANGLE
                         : self->tool == EASEL_TOOL_ROUNDED_RECTANGLE ? EASEL_FIGURE_ROUNDED : EASEL_FIGURE_ELLIPSE;

  return (EaselFigure) { kind, self->shape_x0, self->shape_y0, self->shape_x1, self->shape_y1, self->shape_width,
                         self->shape_style, NULL };
}

static GdkRGBA
rgba_of (uint32_t c)
{
  return (GdkRGBA) { ((c >> 16) & 0xFF) / 255.0f, ((c >> 8) & 0xFF) / 255.0f, (c & 0xFF) / 255.0f, (c >> 24) / 255.0f };
}

/* One run of the shape's pixels as a colored rectangle. A color with transparency replaces the pixel, so the checkerboard
   is drawn under it, clipped to the run, as the picture's own transparent pixels show it (REQ-0001-11). */
static void
append_shape_run (GtkSnapshot *snapshot, const graphene_rect_t *r, const graphene_rect_t *picture, uint32_t color)
{
  GdkRGBA rgba = rgba_of (color);

  if ((color >> 24) != 0xFF)
    {
      gtk_snapshot_push_clip (snapshot, r);
      easel_snapshot_append_checkerboard (snapshot, picture, CHECKER_SIZE);
      gtk_snapshot_pop (snapshot);
    }
  if ((color >> 24) != 0)
    gtk_snapshot_append_color (snapshot, &rgba, r);
}

/* The shape being dragged, drawn over the picture as it will be drawn: the same runs the release applies, only for the rows
   the view shows (REQ-0010-05). At a zoom where a device pixel holds many rows, one row in step is drawn, so a frame costs
   what the view costs and not what the shape does. */
static void
append_shape_preview (EaselCanvas *self, GtkSnapshot *snapshot)
{
  EaselFigure f;
  EaselFigureRun *runs;
  graphene_rect_t seen, picture;
  double l = self->zoom / (100.0 * easel_canvas_get_scale (self));
  int row0, row1, step;

  uint32_t draw, other;

  if (self->pending)
    {
      f = pending_figure (self);
      draw = self->pend_draw;
      other = self->pend_other;
    }
  else if (self->shaping)
    {
      f = current_figure (self);
      draw = self->shape_draw;
      other = self->shape_other;
    }
  else
    return;
  if (!gtk_widget_compute_bounds (gtk_widget_get_parent (GTK_WIDGET (self)), GTK_WIDGET (self), &seen))
    return;
  picture = GRAPHENE_RECT_INIT (0, 0, (float) (self->img->width * l), (float) (self->img->height * l));
  row0 = (int) floor (fmax (0, seen.origin.y) / l);
  row1 = (int) ceil (fmin (self->img->height * l, seen.origin.y + seen.size.height) / l) - 1;
  if (row1 > self->img->height - 1)
    row1 = self->img->height - 1;
  if (row0 > row1)
    return;
  if (!easel_figure_prepare (&f, row0, row1))
    return;
  runs = malloc (easel_figure_max_runs (&f) * sizeof *runs);
  if (runs == NULL)
    {
      easel_figure_release (&f);
      return;
    }
  step = (row1 - row0 + 1) / 3000 + 1;
  for (int y = row0; y <= row1; y += step)
    {
      size_t n = easel_figure_row (&f, y, runs, easel_figure_max_runs (&f));

      for (size_t i = 0; i < n; i++)
        {
          uint32_t color;
          int x0 = runs[i].x0 < 0 ? 0 : runs[i].x0, x1 = runs[i].x1 > self->img->width - 1 ? self->img->width - 1 : runs[i].x1;
          graphene_rect_t r;

          if (x0 > x1)
            continue;
          if (!runs[i].inside)
            color = draw;
          else if (f.style == EASEL_FIGURE_OUTLINE_AND_FILL)
            color = other;
          else if (f.style == EASEL_FIGURE_FILL_ONLY)
            color = draw;
          else
            continue;
          r = GRAPHENE_RECT_INIT ((float) (x0 * l), (float) (y * l), (float) ((x1 - x0 + 1) * l), (float) (step * l));
          append_shape_run (snapshot, &r, &picture, color);
        }
    }
  easel_figure_release (&f);
  free (runs);
}

static void
easel_canvas_snapshot (GtkWidget *widget, GtkSnapshot *snapshot)
{
  EaselCanvas *self = EASEL_CANVAS (widget);
  double l;
  graphene_rect_t bounds;

  if (self->tiles == NULL)
    return;
  l = self->zoom / (100.0 * easel_canvas_get_scale (self));
  bounds = GRAPHENE_RECT_INIT (0, 0, (float) (self->img->width * l), (float) (self->img->height * l));

  /* Transparent pixels show the checkerboard (REQ-0001-11); nearest filtering keeps pixels hard-edged
     at every zoom (REQ-0001-18). Each tile is drawn where its pixels are; edges fall on whole device pixels. */
  easel_snapshot_append_checkerboard (snapshot, &bounds, CHECKER_SIZE);
  for (int ty = 0; ty < self->tile_rows; ty++)
    for (int tx = 0; tx < self->tile_cols; tx++)
      {
        GdkTexture *tile = self->tiles[ty * self->tile_cols + tx];
        graphene_rect_t r = GRAPHENE_RECT_INIT ((float) (tx * EASEL_CANVAS_TILE * l), (float) (ty * EASEL_CANVAS_TILE * l),
                                                (float) (gdk_texture_get_width (tile) * l),
                                                (float) (gdk_texture_get_height (tile) * l));

        gtk_snapshot_append_scaled_texture (snapshot, tile, GSK_SCALING_FILTER_NEAREST, &r);
      }
  append_shape_preview (self, snapshot);
  if (easel_canvas_grid_is_drawn (self))
    append_grid (self, snapshot);
  append_handles (self, snapshot);
  append_magnifier_box (self, snapshot);
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

static GdkCursor *
airbrush_cursor (void)
{
  static GdkCursor *spray_can;

  if (spray_can == NULL)
    {
      g_autoptr (GdkTexture) texture = gdk_texture_new_from_resource (RESOURCE_PATH "/cursors/airbrush.svg");
      g_autoptr (GdkCursor) fallback = gdk_cursor_new_from_name ("crosshair", NULL);

      spray_can = gdk_cursor_new_from_texture (texture, 7, 26, fallback);
    }
  return spray_can;
}

/* The cursors of Fill, Pick Color and the Magnifier (REQ-0007-12): the hotspot is the spilling tip of the bucket, the tip of
   the dropper and the center of the lens. */
static GdkCursor *
texture_cursor (GdkCursor **cache, const char *name, int hot_x, int hot_y)
{
  if (*cache == NULL)
    {
      g_autofree char *path = g_strconcat (RESOURCE_PATH "/cursors/", name, NULL);
      g_autoptr (GdkTexture) texture = gdk_texture_new_from_resource (path);
      g_autoptr (GdkCursor) fallback = gdk_cursor_new_from_name ("crosshair", NULL);

      *cache = gdk_cursor_new_from_texture (texture, hot_x, hot_y, fallback);
    }
  return *cache;
}

/* The outline of the Brush's or the Eraser's footprint at the current zoom (REQ-0006-16); made again only when the
   footprint or the zoom changed. The scale is 1: how GDK maps a cursor texture on a scaled screen is not known here. */
static GdkCursor *
outline_cursor (EaselCanvas *self)
{
  if (self->outline_cursor == NULL || self->cursor_zoom != self->zoom
      || self->cursor_footprint.shape != self->footprint.shape || self->cursor_footprint.size != self->footprint.size)
    {
      EaselCursorShape shape = easel_cursor_shape (self->footprint, self->zoom, 1);
      g_autofree uint32_t *argb = g_new (uint32_t, (gsize) shape.width * shape.height);
      g_autoptr (GBytes) bytes = NULL;
      g_autoptr (GdkTexture) texture = NULL;
      g_autoptr (GdkCursor) fallback = gdk_cursor_new_from_name ("crosshair", NULL);

      easel_cursor_render (self->footprint, self->zoom, 1, argb);
      /* 0xAARRGGBB in a native 32 bit word is what GDK calls B8G8R8A8 premultiplied on little endian; the pixels are
         only opaque black, opaque white or transparent, which premultiplied and straight alpha share. */
      bytes = g_bytes_new (argb, (gsize) shape.width * shape.height * sizeof *argb);
      texture = gdk_memory_texture_new (shape.width, shape.height, GDK_MEMORY_B8G8R8A8_PREMULTIPLIED, bytes,
                                        (gsize) shape.width * sizeof *argb);
      g_clear_object (&self->outline_cursor);
      self->outline_cursor = gdk_cursor_new_from_texture (texture, shape.hot_x, shape.hot_y, fallback);
      self->cursor_zoom = self->zoom;
      self->cursor_footprint = self->footprint;
    }
  return self->outline_cursor;
}

/* The cursor shows the tool over the whole canvas area, not only over the picture (REQ-0001-10). */
static void
update_cursor (EaselCanvas *self)
{
  g_autoptr (GdkCursor) cursor = NULL;

  EaselHandle handle = self->resizing != EASEL_HANDLE_NONE ? self->resizing : self->hover;

  if (self->busy)
    cursor = gdk_cursor_new_from_name ("wait", NULL);
  else if (handle != EASEL_HANDLE_NONE)
    cursor = gdk_cursor_new_from_name (handle == EASEL_HANDLE_RIGHT ? "ew-resize"
                                       : handle == EASEL_HANDLE_BOTTOM ? "ns-resize" : "nwse-resize", NULL);
  else if (self->tool == EASEL_TOOL_PENCIL)
    cursor = g_object_ref (pencil_cursor ());
  else if (self->tool == EASEL_TOOL_BRUSH || self->tool == EASEL_TOOL_ERASER)
    cursor = g_object_ref (outline_cursor (self));
  else if (self->tool == EASEL_TOOL_AIRBRUSH)
    cursor = g_object_ref (airbrush_cursor ());
  else if (self->tool == EASEL_TOOL_FILL)
    {
      static GdkCursor *bucket;

      cursor = g_object_ref (texture_cursor (&bucket, "fill.svg", 4, 29));
    }
  else if (self->tool == EASEL_TOOL_PICK)
    {
      static GdkCursor *dropper;

      cursor = g_object_ref (texture_cursor (&dropper, "pick.svg", 2, 30));
    }
  else if (easel_tool_draws_shape (self->tool) || easel_tool_draws_pending (self->tool))
    {
      /* The crosshair: its center is the pixel a shape would start from (REQ-0010-15, REQ-0011-03). */
      cursor = gdk_cursor_new_from_name ("crosshair", NULL);
    }
  else if (self->tool == EASEL_TOOL_MAGNIFIER)
    {
      static GdkCursor *glass;

      cursor = g_object_ref (texture_cursor (&glass, "magnifier.svg", 12, 12));
    }
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
easel_canvas_set_busy (EaselCanvas *self, gboolean busy)
{
  self->busy = busy;
  update_cursor (self);
}

void
easel_canvas_set_tool (EaselCanvas *self, EaselTool tool)
{
  self->tool = tool;
  self->hover = EASEL_HANDLE_NONE;
  update_cursor (self);
  gtk_widget_queue_draw (GTK_WIDGET (self));     /* the preview box comes and goes with the Magnifier */
  gtk_widget_queue_resize (GTK_WIDGET (self));   /* the handles come and go with the room they need */
}

void
easel_canvas_set_footprint (EaselCanvas *self, EaselFootprint footprint)
{
  self->footprint = footprint;
  update_cursor (self);
}

void
easel_canvas_set_magnifier_level (EaselCanvas *self, int pct)
{
  self->magnifier_level = pct;
  gtk_widget_queue_draw (GTK_WIDGET (self));
}

void
easel_canvas_set_shape_options (EaselCanvas *self, int width, int style)
{
  self->shape_width = width;
  self->shape_style = (EaselFigureStyle) style;
  gtk_widget_queue_draw (GTK_WIDGET (self));     /* a pending curve or polygon follows the choice at once (REQ-0011-12) */
}

void
easel_canvas_set_shape_memory_limit (EaselCanvas *self, size_t max_bytes)
{
  self->shape_max_bytes = max_bytes;
}

void
easel_canvas_set_fill_memory_limit (EaselCanvas *self, size_t max_bytes)
{
  self->fill_max_bytes = max_bytes;
}

void
easel_canvas_set_eraser_transparent (EaselCanvas *self, gboolean transparent)
{
  self->eraser_transparent = transparent;
}

void
easel_canvas_set_colors (EaselCanvas *self, uint32_t foreground, uint32_t background)
{
  self->foreground = foreground;
  self->background = background;
}

gboolean
easel_canvas_is_pending (EaselCanvas *self)
{
  return self->pending;
}

gboolean
easel_canvas_is_stroking (EaselCanvas *self)
{
  return self->stroke != NULL || self->picking || self->shaping || self->pend_held;
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

/* The pick ends: kept (the button was released, the focus lost) or cancelled (the other button, Undo). */
static void
finish_pick (EaselCanvas *self, gboolean cancelled)
{
  self->picking = FALSE;
  g_signal_emit (self, signals[SIGNAL_STROKING_CHANGED], 0, FALSE);
  g_signal_emit (self, signals[SIGNAL_PICK_ENDED], 0, cancelled);
}

/* The held stage of a curve follows the pointer (REQ-0011-04): the line from its start, Shift constraining it as for Line; a
   bend moves the control points to the pointer, Shift having no effect on it. */
static void
pend_follow (EaselCanvas *self)
{
  int x = self->pend_px, y = self->pend_py;

  if (self->pend_polygon)
    {
      /* The last corner is the end of the side being dragged: the pointer's pixel, Shift constraining it as for Line from the
         corner before (REQ-0011-06). */
      if (self->shift && self->poly_n >= 2)
        easel_figure_constrain (EASEL_FIGURE_LINE, self->poly[2 * (self->poly_n - 2)], self->poly[2 * (self->poly_n - 2) + 1], &x, &y);
      self->poly[2 * (self->poly_n - 1)] = x;
      self->poly[2 * (self->poly_n - 1) + 1] = y;
    }
  else if (self->pend_step == 0)
    {
      if (self->shift)
        easel_figure_constrain (EASEL_FIGURE_LINE, self->pend_pts[0], self->pend_pts[1], &x, &y);
      self->pend_pts[6] = x;
      self->pend_pts[7] = y;
      /* Both control points on the start: on the closed segment, so the curve is the Line (REQ-0011-05). */
      self->pend_pts[2] = self->pend_pts[4] = self->pend_pts[0];
      self->pend_pts[3] = self->pend_pts[5] = self->pend_pts[1];
    }
  else if (self->pend_step == 1)
    {
      self->pend_pts[2] = self->pend_pts[4] = x;
      self->pend_pts[3] = self->pend_pts[5] = y;
    }
  else
    {
      self->pend_pts[4] = x;
      self->pend_pts[5] = y;
    }
  gtk_widget_queue_draw (GTK_WIDGET (self));
}

static void
set_pending (EaselCanvas *self, gboolean pending)
{
  if (self->pending == pending)
    return;
  self->pending = pending;
  g_signal_emit (self, signals[SIGNAL_PENDING_CHANGED], 0);
}

/* The pending figure as the core describes it. */
static EaselFigure
pending_figure (EaselCanvas *self)
{
  if (self->pend_polygon)
    return (EaselFigure) { EASEL_FIGURE_POLYGON, 0, 0, 0, 0, self->shape_width, self->shape_style, NULL, self->poly, self->poly_n,
                           FALSE };
  return (EaselFigure) { EASEL_FIGURE_CURVE, 0, 0, 0, 0, self->shape_width, self->shape_style, NULL, self->pend_pts, 4, FALSE };
}

/* A corner is added to the polygon: false if the memory cannot be had. */
static gboolean
poly_add (EaselCanvas *self, int x, int y)
{
  if (self->poly_n == self->poly_cap)
    {
      size_t cap = self->poly_cap ? self->poly_cap * 2 : 16;
      int *p;

      if (cap > SIZE_MAX / (2 * sizeof *p))
        return FALSE;
      p = realloc (self->poly, cap * 2 * sizeof *p);
      if (p == NULL)
        return FALSE;
      self->poly = p;
      self->poly_cap = cap;
    }
  self->poly[2 * self->poly_n] = x;
  self->poly[2 * self->poly_n + 1] = y;
  self->poly_n++;
  return TRUE;
}

/* The pending curve is drawn into the picture as one undo step, or not at all if it changes no pixel (REQ-0011-14); a
   failed allocation shows the dialog and changes nothing (REQ-0011-15). */
static void
put_down_pending (EaselCanvas *self)
{
  EaselFigure f = pending_figure (self);
  EaselFigureResult r;
  gboolean was_held = self->pend_held;

  f.closed = self->pend_polygon;                 /* a polygon is closed when put down (REQ-0011-07) */

  self->pend_held = FALSE;
  r = easel_figure_apply_capped (self->img, self->history, &f, self->pend_draw, self->pend_other, NULL,
                                 self->shape_max_bytes);
  if (was_held)
    g_signal_emit (self, signals[SIGNAL_STROKING_CHANGED], 0, FALSE);
  /* No "stroke-finished" here: "pending-changed", emitted just after the apply, makes the window sync the history, the title
     and Undo and Redo, which is all "stroke-finished" does there. */
  set_pending (self, FALSE);
  if (r == EASEL_FIGURE_DONE)
    easel_canvas_update (self);
  else if (r == EASEL_FIGURE_NO_MEMORY)
    g_signal_emit (self, signals[SIGNAL_SHAPE_FAILED], 0);
  gtk_widget_queue_draw (GTK_WIDGET (self));
}

/* A stage's button goes up, or the stage is ended as last shown (a lost release, a lost focus, Save, Quit): the curve waits
   for its next stage, and the second bend puts it down (REQ-0011-04, -13). */
static void
end_pending_stage (EaselCanvas *self)
{
  self->pend_held = FALSE;
  g_signal_emit (self, signals[SIGNAL_STROKING_CHANGED], 0, FALSE);
  if (self->pend_polygon && self->poly_n >= 3)
    {
      /* From the second side on, a polygon closes when the side ends near the first corner, or when its press was the second
         click of a double-click and it ends near that press; the side that closed it adds no corner (REQ-0011-07). */
      double l = self->zoom / (100.0 * easel_canvas_get_scale (self));
      gboolean near_start = hypot (self->pend_wx - (self->poly[0] + 0.5) * l, self->pend_wy - (self->poly[1] + 0.5) * l) <= 4.0;
      gboolean dbl = self->pend_dbl && within_double_click_distance (self, self->pend_wx, self->pend_wy, self->press_wx, self->press_wy);

      if (near_start || dbl)
        {
          self->poly_n--;
          self->dbl_closed = dbl;
          put_down_pending (self);
          return;
        }
    }
  if (!self->pend_polygon && self->pend_step == 2)
    put_down_pending (self);
  else
    gtk_widget_queue_draw (GTK_WIDGET (self));
}

void
easel_canvas_put_down (EaselCanvas *self)
{
  if (self->pending)
    put_down_pending (self);
}

gboolean
easel_canvas_enter (EaselCanvas *self)
{
  if (!self->pending)
    return FALSE;
  if (!self->pend_held)
    put_down_pending (self);
  return TRUE;
}

void
easel_canvas_drop (EaselCanvas *self)
{
  gboolean was_held = self->pend_held;

  if (!self->pending)
    return;
  /* Nothing is drawn and no step is added; a held button is waited for until its release (REQ-0011-11). */
  self->pend_held = FALSE;
  if (was_held)
    {
      self->cancelled_button = self->pend_button;
      g_signal_emit (self, signals[SIGNAL_STROKING_CHANGED], 0, FALSE);
    }
  set_pending (self, FALSE);
  gtk_widget_queue_draw (GTK_WIDGET (self));
}

void
easel_canvas_cancel_stroke (EaselCanvas *self)
{
  if (self->picking)
    {
      self->cancelled_button = self->stroke_button;
      finish_pick (self, TRUE);
      return;
    }
  if (self->pend_held)
    {
      easel_canvas_drop (self);
      return;
    }
  if (self->shaping)
    {
      /* A shape drag is cancelled: nothing was drawn, nothing is added; its button is held until the release
         (REQ-0010-08). */
      self->shaping = FALSE;
      self->cancelled_button = self->shape_button;
      g_signal_emit (self, signals[SIGNAL_STROKING_CHANGED], 0, FALSE);
      gtk_widget_queue_draw (GTK_WIDGET (self));
      return;
    }
  if (self->stroke == NULL)
    return;
  easel_stroke_cancel (self->stroke);
  self->stroke = NULL;
  stop_spray (self);
  g_signal_emit (self, signals[SIGNAL_STROKING_CHANGED], 0, FALSE);
  self->cancelled_button = self->stroke_button;
  easel_canvas_update (self);
}

void
easel_canvas_buttons_released (EaselCanvas *self)
{
  /* What a release would have done, for each press the canvas still holds (REQ-0007-18). */
  self->action_button = 0;
  if (self->stroke != NULL || self->picking || self->shaping || self->pend_held)
    easel_canvas_end_stroke (self);
  if (self->resizing != EASEL_HANDLE_NONE)
    easel_canvas_release (self, self->resize_button);
  self->cancelled_button = 0;
}

void
easel_canvas_reset_pointer (EaselCanvas *self)
{
  easel_canvas_end_stroke (self);
  self->cancelled_button = 0;
  self->action_button = 0;
  if (self->resizing != EASEL_HANDLE_NONE)
    stop_resizing (self);
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

static void
stop_spray (EaselCanvas *self)
{
  if (self->spray_source != 0)
    {
      g_source_remove (self->spray_source);
      self->spray_source = 0;
    }
}

int
easel_canvas_get_spray_count (EaselCanvas *self)
{
  return self->sprays_made;
}

/* Every 5 ms: the sprays that are due for the time the button has been held and are not made yet, at the pointer's pixel
   now (REQ-0006-08). A late timer catches up; it never runs ahead of the clock. */
static gboolean
on_spray_tick (gpointer data)
{
  EaselCanvas *self = data;
  int due, n;

  if (self->stroke == NULL)
    {
      self->spray_source = 0;
      return G_SOURCE_REMOVE;
    }
  due = easel_spray_due (g_get_monotonic_time () - self->spray_start_us);
  n = due - self->sprays_made;
  if (n > 0)
    {
      easel_stroke_spray (self->stroke, self->spray_x, self->spray_y, n);
      self->sprays_made = due;
      easel_canvas_update (self);
    }
  return G_SOURCE_CONTINUE;
}

void
easel_canvas_end_stroke (EaselCanvas *self)
{
  size_t changed;

  if (self->picking)
    {
      finish_pick (self, FALSE);
      return;
    }
  if (self->pend_held)
    {
      end_pending_stage (self);
      return;
    }
  if (self->shaping)
    {
      /* Save, Quit, a lost focus and a lost release draw the shape as last shown (REQ-0010-09, -18, -19). */
      end_shape (self);
      return;
    }
  if (self->stroke == NULL)
    return;
  changed = easel_stroke_end (self->stroke, self->history);
  self->stroke = NULL;
  stop_spray (self);
  g_signal_emit (self, signals[SIGNAL_STROKING_CHANGED], 0, FALSE);
  g_signal_emit (self, signals[SIGNAL_STROKE_FINISHED], 0, (guint) changed);
}

/* The Magnifier (REQ-0007-03): a press on the picture sets the zoom to the selected level, or to 100 % if it already is
   that level. The focus, the point that goes to the center of the view, is the center of the preview box at 100 % (the box
   is the part the zoom will show) and the pixel under the pointer at other zooms. */
static void
magnifier_at (EaselCanvas *self, guint button, int px, int py)
{
  GtkAdjustment *h = adjustment (self, GTK_ORIENTATION_HORIZONTAL);
  GtkAdjustment *v = adjustment (self, GTK_ORIENTATION_VERTICAL);
  double scale = easel_canvas_get_scale (self), focus_x = px + 0.5, focus_y = py + 0.5;
  int zoom = easel_magnifier_zoom (self->zoom, self->magnifier_level);

  if (px < 0 || py < 0 || px >= self->img->width || py >= self->img->height)
    return;
  self->action_button = button;
  if (zoom == self->zoom)
    return;
  if (self->zoom == 100 && zoom > 100)
    {
      int bx, by, bw, bh;

      easel_magnifier_box ((int) (gtk_adjustment_get_page_size (h) * scale), (int) (gtk_adjustment_get_page_size (v) * scale),
                           zoom, self->img->width, self->img->height, px, py, &bx, &by, &bw, &bh);
      /* A box at an edge of the picture puts the view at that edge: the focus is the edge itself, which the scroll limits
         then turn into offset 0 or the largest one, however wide a scrollbar makes the page. */
      focus_x = bx == 0 ? 0 : bx + bw >= self->img->width ? self->img->width : bx + bw / 2.0;
      focus_y = by == 0 ? 0 : by + bh >= self->img->height ? self->img->height : by + bh / 2.0;
    }
  commit_zoom (self, zoom, 0, 0);
  self->anchor_focus = TRUE;                 /* the scroll is worked out at the allocation, with the page size then */
  self->focus_x = focus_x;
  self->focus_y = focus_y;
}

/* Fill (REQ-0007-05..07): the press fills at once, one undo step; the button stays "held" until its release, and no other
   press acts meanwhile, since there is no stroke to cancel. */
static void
fill_at (EaselCanvas *self, guint button, int px, int py)
{
  size_t changed = 0;
  EaselFillResult r;

  if (px < 0 || py < 0 || px >= self->img->width || py >= self->img->height)
    return;
  self->action_button = button;
  r = easel_fill_capped (self->img, self->history, px, py, button == GDK_BUTTON_PRIMARY ? self->foreground : self->background,
                         &changed, self->fill_max_bytes);
  if (r == EASEL_FILL_NO_MEMORY)
    g_signal_emit (self, signals[SIGNAL_FILL_FAILED], 0);
  else if (r == EASEL_FILL_DONE)
    {
      easel_canvas_update (self);
      g_signal_emit (self, signals[SIGNAL_STROKE_FINISHED], 0, (guint) changed);
    }
}

/* Counts the clicks in a row: a press of the same button within the desktop's double-click time and distance of the one
   before is the next click of the same multi-click (REQ-0011-07). */
static void
count_click (EaselCanvas *self, guint button, double x, double y, guint32 time_ms)
{
  int time = 400, distance = 5;
  GtkSettings *settings = gtk_widget_get_settings (GTK_WIDGET (self));

  g_object_get (settings, "gtk-double-click-time", &time, "gtk-double-click-distance", &distance, NULL);
  if (button == self->click_button && self->click_count > 0 && (guint32) (time_ms - self->click_time) <= (guint32) time
      && hypot (x - self->click_x, y - self->click_y) <= distance)
    self->click_count++;
  else
    {
      self->click_count = 1;
      self->dbl_closed = FALSE;
    }
  self->click_button = button;
  self->click_time = time_ms;
  self->click_x = x;
  self->click_y = y;
}

/* Is (x, y), a place in the widget, within the desktop's double-click distance of (ox, oy)? */
static gboolean
within_double_click_distance (EaselCanvas *self, double x, double y, double ox, double oy)
{
  int distance = 5;

  g_object_get (gtk_widget_get_settings (GTK_WIDGET (self)), "gtk-double-click-distance", &distance, NULL);
  return hypot (x - ox, y - oy) <= distance;
}

void
easel_canvas_press (EaselCanvas *self, guint button, double x, double y)
{
  easel_canvas_press_timed (self, button, x, y, (guint32) (g_get_monotonic_time () / 1000));
}

void
easel_canvas_press_timed (EaselCanvas *self, guint button, double x, double y, guint32 time_ms)
{
  int px, py;

  /* Only the primary and secondary buttons draw; others are ignored (REQ-0001-24). */
  if (self->input_blocked || (button != GDK_BUTTON_PRIMARY && button != GDK_BUTTON_SECONDARY))
    return;
  if (self->action_button != 0)
    return;
  count_click (self, button, x, y, time_ms);
  if (self->dbl_closed && self->click_count >= 3 && self->tool == EASEL_TOOL_POLYGON && !self->pending)
    return;                                 /* the third click of a multi-click that closed a polygon starts nothing (REQ-0011-07) */

  if (self->picking)
    {
      /* The other button cancels the pick, as it cancels a stroke (REQ-0007-10). */
      if (button != self->stroke_button)
        finish_pick (self, TRUE);
      return;
    }
  if (self->shaping)
    {
      /* The other button cancels the shape drag, as it cancels a stroke (REQ-0010-08). */
      if (button != self->shape_button)
        easel_canvas_cancel_stroke (self);
      return;
    }
  if (self->pending && !self->pend_held && handles_shown (self) && easel_canvas_handle_at (self, x, y) != EASEL_HANDLE_NONE)
    {
      /* A press on a resize handle: a secondary one does nothing and keeps the shape (REQ-0004-16); a primary one puts it down
         and goes on as a press on the handle (REQ-0011-12). */
      if (button != GDK_BUTTON_PRIMARY)
        return;
      put_down_pending (self);
    }
  if (self->pending)
    {
      /* A curve waits or has a stage held: the other button drops it, the same button goes on to the next stage
         (REQ-0011-04, -11). */
      if (button != self->pend_button)
        easel_canvas_drop (self);
      else if (!self->pend_held)
        {
          to_pixel (self, x, y, &px, &py);
          if (self->pend_polygon && !poly_add (self, px, py))
            {
              easel_canvas_drop (self);
              g_signal_emit (self, signals[SIGNAL_SHAPE_FAILED], 0);
              return;
            }
          self->pend_held = TRUE;
          self->pend_step++;
          self->pend_px = px;
          self->pend_py = py;
          self->pend_dbl = self->pend_polygon && self->click_count == 2;
          self->pend_wx = self->press_wx = x;
          self->pend_wy = self->press_wy = y;
          g_signal_emit (self, signals[SIGNAL_STROKING_CHANGED], 0, TRUE);
          pend_follow (self);
        }
      return;
    }
  if (self->stroke != NULL)
    {
      /* The other button cancels the stroke (REQ-0001-23). */
      if (button != self->stroke_button)
        {
          easel_stroke_cancel (self->stroke);
          self->stroke = NULL;
          stop_spray (self);
          g_signal_emit (self, signals[SIGNAL_STROKING_CHANGED], 0, FALSE);
          easel_canvas_update (self);
        }
      return;
    }

  /* A drag of a handle: the other button cancels it (REQ-0004-19). */
  if (self->resizing != EASEL_HANDLE_NONE)
    {
      if (button != self->resize_button)
        easel_canvas_cancel_resize (self);
      return;
    }
  if (handles_shown (self))
    {
      EaselHandle handle = easel_canvas_handle_at (self, x, y);

      if (handle != EASEL_HANDLE_NONE)
        {
          /* Only the primary button starts a resize; the other does nothing, and does not draw (REQ-0004-16). */
          if (button == GDK_BUTTON_PRIMARY)
            {
              self->resizing = handle;
              self->resize_button = button;
              self->resize_x = x;
              self->resize_y = y;
              self->resize_w = self->outline_w = self->img->width;
              self->resize_h = self->outline_h = self->img->height;
              update_cursor (self);
              gtk_widget_queue_draw (GTK_WIDGET (self));
            }
          return;
        }
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

  if (self->img == NULL || self->tool == EASEL_TOOL_PAN)
    return;
  to_pixel (self, x, y, &px, &py);
  if (self->tool == EASEL_TOOL_MAGNIFIER)
    {
      magnifier_at (self, button, px, py);
      return;
    }
  if (self->tool == EASEL_TOOL_PICK)
    {
      /* A press on the picture starts a pick: the color of the pixel under the pointer, then of each pixel it moves to
         (REQ-0007-08). A press outside the picture picks nothing. */
      if (px < 0 || py < 0 || px >= self->img->width || py >= self->img->height)
        return;
      self->picking = TRUE;
      self->stroke_button = button;
      self->pick_px = px;
      self->pick_py = py;
      g_signal_emit (self, signals[SIGNAL_STROKING_CHANGED], 0, TRUE);
      g_signal_emit (self, signals[SIGNAL_COLOR_PICKED], 0, button, (guint) easel_image_get (self->img, px, py));
      return;
    }
  if (self->tool == EASEL_TOOL_FILL)
    {
      fill_at (self, button, px, py);
      return;
    }
  if (self->tool == EASEL_TOOL_CURVE)
    {
      /* The first stage of a curve: the line from the pixel under the press, the colors those of the press (REQ-0011-04,
         -10). */
      set_pending (self, TRUE);
      self->pend_polygon = FALSE;
      self->pend_held = TRUE;
      self->pend_step = 0;
      self->pend_button = button;
      self->pend_pts[0] = self->pend_pts[6] = self->pend_px = px;
      self->pend_pts[1] = self->pend_pts[7] = self->pend_py = py;
      self->pend_pts[2] = self->pend_pts[4] = px;
      self->pend_pts[3] = self->pend_pts[5] = py;
      self->pend_draw = button == GDK_BUTTON_PRIMARY ? self->foreground : self->background;
      self->pend_other = button == GDK_BUTTON_PRIMARY ? self->background : self->foreground;
      g_signal_emit (self, signals[SIGNAL_STROKING_CHANGED], 0, TRUE);
      gtk_widget_queue_draw (GTK_WIDGET (self));
      return;
    }
  if (self->tool == EASEL_TOOL_POLYGON)
    {
      /* The first side of a polygon: from the pixel under the press, which is its first corner, to the pointer (REQ-0011-06). */
      self->poly_n = 0;
      if (!poly_add (self, px, py) || !poly_add (self, px, py))
        {
          g_signal_emit (self, signals[SIGNAL_SHAPE_FAILED], 0);
          return;
        }
      set_pending (self, TRUE);
      self->pend_polygon = TRUE;
      self->pend_held = TRUE;
      self->pend_step = 0;
      self->pend_button = button;
      self->pend_px = px;
      self->pend_py = py;
      self->pend_dbl = FALSE;
      self->pend_wx = self->press_wx = x;
      self->pend_wy = self->press_wy = y;
      self->pend_draw = button == GDK_BUTTON_PRIMARY ? self->foreground : self->background;
      self->pend_other = button == GDK_BUTTON_PRIMARY ? self->background : self->foreground;
      g_signal_emit (self, signals[SIGNAL_STROKING_CHANGED], 0, TRUE);
      gtk_widget_queue_draw (GTK_WIDGET (self));
      return;
    }
  if (easel_tool_draws_shape (self->tool))
    {
      /* A shape drag: the start is the pixel under the press, also outside the picture (REQ-0010-05); the colors are those
         of the press (REQ-0010-06). */
      self->shaping = TRUE;
      self->shape_button = button;
      self->shape_x0 = self->shape_x1 = self->shape_px = px;
      self->shape_y0 = self->shape_y1 = self->shape_py = py;
      self->shape_draw = button == GDK_BUTTON_PRIMARY ? self->foreground : self->background;
      self->shape_other = button == GDK_BUTTON_PRIMARY ? self->background : self->foreground;
      g_signal_emit (self, signals[SIGNAL_STROKING_CHANGED], 0, TRUE);
      gtk_widget_queue_draw (GTK_WIDGET (self));
      return;
    }
  if (self->tool == EASEL_TOOL_AIRBRUSH)
    {
      /* The random sequence starts differently each time (REQ-0006-09). */
      uint64_t seed = (uint64_t) g_get_monotonic_time () * UINT64_C (0x9E3779B97F4A7C15) ^ (uint64_t) g_get_real_time ();

      self->stroke = easel_stroke_begin_spray (self->img, button == GDK_BUTTON_PRIMARY ? self->foreground : self->background,
                                               self->footprint.size, easel_spray_pixels (self->footprint.size), seed);
      if (self->stroke != NULL)
        {
          self->spray_x = px;
          self->spray_y = py;
          self->spray_start_us = g_get_monotonic_time ();
          self->sprays_made = 0;
          self->stroke_button = button;
          g_signal_emit (self, signals[SIGNAL_STROKING_CHANGED], 0, TRUE);
          on_spray_tick (self);                                 /* the spray of the press */
          self->spray_source = g_timeout_add (5, on_spray_tick, self);
          return;
        }
    }
  else if (self->tool == EASEL_TOOL_PENCIL)
    self->stroke = easel_stroke_begin (self->img, px, py, button == GDK_BUTTON_PRIMARY ? self->foreground : self->background);
  else if (self->tool == EASEL_TOOL_BRUSH)
    self->stroke = easel_stroke_begin_stamp (self->img, px, py,
                                             button == GDK_BUTTON_PRIMARY ? self->foreground : self->background,
                                             self->footprint, NULL);
  else
    {
      /* The Eraser paints the background color, or transparency; with the secondary button (XP's color eraser) only
         where the foreground color is (REQ-0006-11, REQ-0006-12). */
      uint32_t color = self->eraser_transparent ? 0x00000000u : self->background;
      uint32_t only = self->foreground;

      self->stroke = easel_stroke_begin_stamp (self->img, px, py, color, self->footprint,
                                               button == GDK_BUTTON_PRIMARY ? NULL : &only);
    }
  if (self->stroke == NULL)
    {
      g_critical ("Out of memory starting a stroke");
      return;
    }
  self->stroke_button = button;
  g_signal_emit (self, signals[SIGNAL_STROKING_CHANGED], 0, TRUE);
  easel_canvas_update (self);
}

void
easel_canvas_motion (EaselCanvas *self, double x, double y)
{
  int px, py;

  if (self->resizing != EASEL_HANDLE_NONE)
    {
      resize_to (self, x, y);
      return;
    }
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

  if (self->picking)
    {
      to_pixel (self, x, y, &px, &py);
      if (px >= 0 && py >= 0 && px < self->img->width && py < self->img->height
          && (px != self->pick_px || py != self->pick_py))
        {
          self->pick_px = px;
          self->pick_py = py;
          g_signal_emit (self, signals[SIGNAL_COLOR_PICKED], 0, self->stroke_button, (guint) easel_image_get (self->img, px, py));
        }
      return;
    }
  if (self->pend_held)
    {
      self->pend_wx = x;
      self->pend_wy = y;
      to_pixel (self, x, y, &px, &py);
      self->pend_px = px;
      self->pend_py = py;
      pend_follow (self);
      return;
    }
  if (self->shaping)
    {
      to_pixel (self, x, y, &px, &py);
      self->shape_px = px;
      self->shape_py = py;
      update_shape_end (self);
      return;
    }
  if (self->stroke == NULL)
    return;
  to_pixel (self, x, y, &px, &py);
  self->spray_x = px;                  /* where the next sprays land (nothing else uses these two) */
  self->spray_y = py;
  easel_stroke_to (self->stroke, px, py);
  easel_canvas_update (self);
}

/* The shape drag ends: the shape is drawn into the picture as one undo step, or not at all if it changes no pixel
   (REQ-0010-05, REQ-0010-07). */
static void
end_shape (EaselCanvas *self)
{
  EaselFigure f = current_figure (self);
  size_t changed = 0;
  EaselFigureResult r;

  self->shaping = FALSE;
  r = easel_figure_apply_capped (self->img, self->history, &f, self->shape_draw, self->shape_other, &changed,
                                 self->shape_max_bytes);
  g_signal_emit (self, signals[SIGNAL_STROKING_CHANGED], 0, FALSE);
  if (r == EASEL_FIGURE_DONE)
    {
      easel_canvas_update (self);
      g_signal_emit (self, signals[SIGNAL_STROKE_FINISHED], 0, (guint) changed);
    }
  else if (r == EASEL_FIGURE_NO_MEMORY)
    g_signal_emit (self, signals[SIGNAL_SHAPE_FAILED], 0);
  gtk_widget_queue_draw (GTK_WIDGET (self));
}

void
easel_canvas_release (EaselCanvas *self, guint button)
{
  if (button == self->cancelled_button)
    self->cancelled_button = 0;
  if (button == self->action_button)
    self->action_button = 0;
  if (self->resizing != EASEL_HANDLE_NONE && button == self->resize_button)
    {
      int w = self->outline_w, h = self->outline_h;
      gboolean changed = w != self->resize_w || h != self->resize_h;

      stop_resizing (self);
      if (changed)
        g_signal_emit (self, signals[SIGNAL_RESIZE_REQUESTED], 0, w, h);
      return;
    }
  if (self->panning && button == GDK_BUTTON_PRIMARY)
    {
      self->panning = FALSE;
      update_cursor (self);
    }
  if (self->pend_held && button == self->pend_button)
    end_pending_stage (self);
  if (self->shaping && button == self->shape_button)
    end_shape (self);
  if ((self->stroke != NULL || self->picking) && button == self->stroke_button)
    easel_canvas_end_stroke (self);
}

/* Sets the zoom; the new scroll offsets can only be applied once the adjustments know the new size, so they wait for
   the next allocation. */
static void
commit_zoom (EaselCanvas *self, int zoom, double scroll_h, double scroll_v)
{
  self->anchor_h = scroll_h;
  self->anchor_v = scroll_v;
  self->anchor_pending = TRUE;
  self->anchor_focus = FALSE;
  self->zoom = zoom;
  gtk_widget_queue_resize (GTK_WIDGET (self));
  gtk_widget_queue_draw (GTK_WIDGET (self));
  update_cursor (self);
  g_signal_emit (self, signals[SIGNAL_ZOOM_CHANGED], 0, zoom);
}

/* The zoom that keeps the picture point under (view_x, view_y), relative to the canvas area, still. */
static void
zoom_around (EaselCanvas *self, int zoom, double view_x, double view_y)
{
  GtkAdjustment *h = adjustment (self, GTK_ORIENTATION_HORIZONTAL);
  GtkAdjustment *v = adjustment (self, GTK_ORIENTATION_VERTICAL);
  double scale = easel_canvas_get_scale (self);

  commit_zoom (self, zoom,
               easel_view_anchor (gtk_adjustment_get_value (h), view_x, self->zoom, zoom, self->img->width, scale,
                                  gtk_adjustment_get_page_size (h)),
               easel_view_anchor (gtk_adjustment_get_value (v), view_y, self->zoom, zoom, self->img->height, scale,
                                  gtk_adjustment_get_page_size (v)));
}

void
easel_canvas_set_zoom (EaselCanvas *self, int pct)
{
  GtkAdjustment *h = adjustment (self, GTK_ORIENTATION_HORIZONTAL);
  GtkAdjustment *v = adjustment (self, GTK_ORIENTATION_VERTICAL);

  if (self->img == NULL || pct == self->zoom)
    return;
  zoom_around (self, pct, gtk_adjustment_get_page_size (h) / 2, gtk_adjustment_get_page_size (v) / 2);
}

void
easel_canvas_reset_view (EaselCanvas *self)
{
  if (self->img == NULL)
    return;
  /* Also at 100 % already: the new picture's size is not known to the adjustments yet, so the scroll waits for the next
     allocation like any zoom does. */
  commit_zoom (self, 100, 0, 0);
}

void
easel_canvas_fit_to_window (EaselCanvas *self)
{
  GtkAdjustment *h = adjustment (self, GTK_ORIENTATION_HORIZONTAL);
  GtkAdjustment *v = adjustment (self, GTK_ORIENTATION_VERTICAL);
  double scale = easel_canvas_get_scale (self);
  int zoom;

  if (self->img == NULL)
    return;
  /* The canvas area in device pixels, with no scrollbars counted (REQ-0004-07). */
  zoom = easel_view_fit_zoom ((int) (gtk_adjustment_get_page_size (h) * scale),
                               (int) (gtk_adjustment_get_page_size (v) * scale), self->img->width, self->img->height,
                               scale);
  if (zoom == self->zoom)
    {
      /* Same zoom: still show the top-left corner. */
      gtk_adjustment_set_value (h, 0);
      gtk_adjustment_set_value (v, 0);
      return;
    }
  commit_zoom (self, zoom, 0, 0);
}

void
easel_canvas_zoom_at (EaselCanvas *self, int notches, double view_x, double view_y)
{
  int zoom;

  if (self->img == NULL || self->tool != EASEL_TOOL_PAN)
    return;
  zoom = easel_view_zoom_step (self->zoom, notches);
  if (zoom == self->zoom)
    return;
  zoom_around (self, zoom, view_x, view_y);
}

void
easel_canvas_set_grid (EaselCanvas *self, gboolean on, int cell, gboolean axonometric)
{
  self->grid_on = on;
  self->grid_cell = CLAMP (cell, 1, 1000);
  self->grid_axonometric = axonometric;
  gtk_widget_queue_draw (GTK_WIDGET (self));
}

gboolean
easel_canvas_get_grid_on (EaselCanvas *self)
{
  return self->grid_on;
}

int
easel_canvas_get_grid_cell (EaselCanvas *self)
{
  return self->grid_cell;
}

gboolean
easel_canvas_get_grid_axonometric (EaselCanvas *self)
{
  return self->grid_axonometric;
}

gboolean
easel_canvas_grid_is_drawn (EaselCanvas *self)
{
  return self->grid_on && easel_grid_visible (self->grid_cell, self->zoom);
}

void
easel_canvas_pointer_at (EaselCanvas *self, double x, double y)
{
  int px, py;

  EaselHandle hover;

  if (self->img == NULL)
    return;
  hover = easel_canvas_handle_at (self, x, y);
  if (hover != self->hover)
    {
      self->hover = hover;
      update_cursor (self);
    }
  to_pixel (self, x, y, &px, &py);
  {
    graphene_point_t p;
    GtkWidget *ref = self->area != NULL ? self->area : GTK_WIDGET (self);

    if (gtk_widget_compute_point (GTK_WIDGET (self), ref, &GRAPHENE_POINT_INIT ((float) x, (float) y), &p))
      {
        gboolean was_in = self->pointer_in;

        self->pointer_ax = p.x;
        self->pointer_ay = p.y;
        self->pointer_in = TRUE;
        if (self->tool == EASEL_TOOL_MAGNIFIER && (!was_in || px != self->box_px || py != self->box_py))
          gtk_widget_queue_draw (GTK_WIDGET (self));
      }
  }
  g_signal_emit (self, signals[SIGNAL_POINTER_MOVED], 0, px, py);
}

void
easel_canvas_pointer_left (EaselCanvas *self)
{
  self->pointer_in = FALSE;
  gtk_widget_queue_draw (GTK_WIDGET (self));
  g_signal_emit (self, signals[SIGNAL_POINTER_LEFT], 0);
}

void
easel_canvas_pinch_begin (EaselCanvas *self)
{
  self->pinch_base = 1.0;
}

void
easel_canvas_pinch_end (EaselCanvas *self)
{
  self->pinch_base = 1.0;
}

void
easel_canvas_pinch (EaselCanvas *self, double scale, double view_x, double view_y)
{
  int step;

  /* Like Ctrl+wheel, the pinch zooms only in Pan mode (REQ-0001-17, REQ-0004-08). */
  if (self->img == NULL || self->tool != EASEL_TOOL_PAN || self->pinch_base <= 0)
    return;
  step = easel_view_pinch (&self->pinch_base, scale, self->zoom);
  if (step != 0)
    easel_canvas_zoom_at (self, step, view_x, view_y);
}

static void
on_pinch_begin (GtkGesture *gesture, GdkEventSequence *sequence, EaselCanvas *self)
{
  easel_canvas_pinch_begin (self);
}

static void
on_pinch_end (GtkGesture *gesture, GdkEventSequence *sequence, EaselCanvas *self)
{
  easel_canvas_pinch_end (self);
}

static void
on_pinch_scale_changed (GtkGestureZoom *gesture, double scale, EaselCanvas *self)
{
  double x = 0, y = 0;

  gtk_gesture_get_bounding_box_center (GTK_GESTURE (gesture), &x, &y);
  easel_canvas_pinch (self, scale, x, y);
}

gboolean
easel_canvas_wheel (EaselCanvas *self, GdkModifierType state, double notches_dy, double view_x, double view_y)
{
  int notches;

  /* Without Ctrl, or with Ctrl in another tool, the wheel scrolls as usual (REQ-0001-15, REQ-0001-17), also during a shape
     drag, whose end then moves at the next pointer motion (REQ-0010-21). */
  if (!(state & GDK_CONTROL_MASK) || self->tool != EASEL_TOOL_PAN)
    return GDK_EVENT_PROPAGATE;
  notches = easel_view_take_notches (&self->notch_acc, notches_dy);
  if (notches != 0)
    easel_canvas_zoom_at (self, notches, view_x, view_y);
  return GDK_EVENT_STOP;
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

  if (gtk_event_controller_scroll_get_unit (controller) == GDK_SCROLL_UNIT_SURFACE)
    dy /= PIXELS_PER_NOTCH;
  if ((state & GDK_CONTROL_MASK) && event != NULL && gdk_event_get_position (event, &x, &y))
    {
      gtk_native_get_surface_transform (native, &nx, &ny);
      if (!gtk_widget_compute_point (GTK_WIDGET (native), area,
                                     &GRAPHENE_POINT_INIT ((float) (x - nx), (float) (y - ny)), &p))
        p = GRAPHENE_POINT_INIT (0, 0);
    }
  return easel_canvas_wheel (self, state, dy, p.x, p.y);
}

static gboolean
on_event (GtkEventControllerLegacy *controller, GdkEvent *event, EaselCanvas *self)
{
  GdkEventType type = gdk_event_get_event_type (event);
  double x, y;

  GtkWidget *area = gtk_event_controller_get_widget (GTK_EVENT_CONTROLLER (controller));

  if (type == GDK_LEAVE_NOTIFY)
    {
      easel_canvas_pointer_left (self);
      return GDK_EVENT_PROPAGATE;
    }
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

  /* A pointer move or a first press while the window sees no button down: a release got lost (a menu or a popover took
     it), so what is still held ends now (REQ-0007-18). */
  if ((type == GDK_MOTION_NOTIFY || type == GDK_BUTTON_PRESS)
      && !(gdk_event_get_modifier_state (event) & (GDK_BUTTON1_MASK | GDK_BUTTON2_MASK | GDK_BUTTON3_MASK)))
    easel_canvas_buttons_released (self);
  if (type == GDK_MOTION_NOTIFY || type == GDK_BUTTON_PRESS)
    self->shift = (gdk_event_get_modifier_state (event) & GDK_SHIFT_MASK) != 0;
  if (type == GDK_MOTION_NOTIFY)
    easel_canvas_pointer_at (self, x, y);
  if (type == GDK_BUTTON_PRESS)
    easel_canvas_press_timed (self, gdk_button_event_get_button (event), x, y, gdk_event_get_time (event));
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

  /* The pinch on a touchpad or touch screen. */
  {
    GtkGesture *pinch = gtk_gesture_zoom_new ();

    g_signal_connect_object (pinch, "begin", G_CALLBACK (on_pinch_begin), self, G_CONNECT_DEFAULT);
    g_signal_connect_object (pinch, "end", G_CALLBACK (on_pinch_end), self, G_CONNECT_DEFAULT);
    g_signal_connect_object (pinch, "scale-changed", G_CALLBACK (on_pinch_scale_changed), self, G_CONNECT_DEFAULT);
    gtk_widget_add_controller (area, GTK_EVENT_CONTROLLER (pinch));
  }
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

/* One frame later: the viewport placed the canvas before the scroll values changed inside this allocation, and it does
   not lay out again for a change made while it is allocating. Asking from a tick callback (the frame's update phase)
   makes it place the canvas at the new scroll position in that frame's layout. */
static gboolean
replace_in_viewport (GtkWidget *widget, GdkFrameClock *clock, gpointer data)
{
  GtkWidget *viewport = gtk_widget_get_parent (widget);

  if (viewport != NULL)
    gtk_widget_queue_allocate (viewport);
  return G_SOURCE_REMOVE;
}

static void
easel_canvas_size_allocate (GtkWidget *widget, int width, int height, int baseline)
{
  EaselCanvas *self = EASEL_CANVAS (widget);

  /* The viewport has configured the adjustments for the new size before allocating the canvas. */
  if (self->anchor_pending)
    {
      GtkAdjustment *h = adjustment (self, GTK_ORIENTATION_HORIZONTAL), *v = adjustment (self, GTK_ORIENTATION_VERTICAL);

      self->anchor_pending = FALSE;
      if (self->anchor_focus)
        {
          /* The page size is known only now: scrollbars may have come or gone with the new zoom. */
          double scale = easel_canvas_get_scale (self);

          self->anchor_h = easel_view_focus_scroll (self->focus_x, self->zoom, self->img->width, scale,
                                                    gtk_adjustment_get_page_size (h));
          self->anchor_v = easel_view_focus_scroll (self->focus_y, self->zoom, self->img->height, scale,
                                                    gtk_adjustment_get_page_size (v));
          self->anchor_focus = FALSE;
        }
      gtk_adjustment_set_value (adjustment (self, GTK_ORIENTATION_HORIZONTAL), self->anchor_h);
      gtk_adjustment_set_value (adjustment (self, GTK_ORIENTATION_VERTICAL), self->anchor_v);
      gtk_widget_add_tick_callback (widget, replace_in_viewport, NULL, NULL);
    }
}

static void
easel_canvas_dispose (GObject *object)
{
  EaselCanvas *self = EASEL_CANVAS (object);

  stop_spray (self);
  if (self->stroke != NULL)
    {
      easel_stroke_cancel (self->stroke);
      self->stroke = NULL;
    }
  self->img = NULL;
  self->history = NULL;
  self->area = NULL;
  free_tiles (self);
  g_clear_object (&self->outline_cursor);
  g_clear_object (&self->builder);
  g_clear_pointer (&self->poly, free);
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

  signals[SIGNAL_POINTER_MOVED] = g_signal_new ("pointer-moved", G_TYPE_FROM_CLASS (klass), G_SIGNAL_RUN_LAST, 0, NULL,
                                                NULL, NULL, G_TYPE_NONE, 2, G_TYPE_INT, G_TYPE_INT);
  signals[SIGNAL_POINTER_LEFT] = g_signal_new ("pointer-left", G_TYPE_FROM_CLASS (klass), G_SIGNAL_RUN_LAST, 0, NULL,
                                               NULL, NULL, G_TYPE_NONE, 0);
  signals[SIGNAL_ZOOM_CHANGED] = g_signal_new ("zoom-changed", G_TYPE_FROM_CLASS (klass), G_SIGNAL_RUN_LAST, 0, NULL, NULL,
                                               NULL, G_TYPE_NONE, 1, G_TYPE_INT);
  signals[SIGNAL_RESIZE_REQUESTED] = g_signal_new ("resize-requested", G_TYPE_FROM_CLASS (klass), G_SIGNAL_RUN_LAST, 0,
                                                   NULL, NULL, NULL, G_TYPE_NONE, 2, G_TYPE_INT, G_TYPE_INT);
  signals[SIGNAL_COLOR_PICKED] = g_signal_new ("color-picked", G_TYPE_FROM_CLASS (klass), G_SIGNAL_RUN_LAST, 0, NULL, NULL, NULL,
                                               G_TYPE_NONE, 2, G_TYPE_UINT, G_TYPE_UINT);
  signals[SIGNAL_PICK_ENDED] = g_signal_new ("pick-ended", G_TYPE_FROM_CLASS (klass), G_SIGNAL_RUN_LAST, 0, NULL, NULL, NULL,
                                             G_TYPE_NONE, 1, G_TYPE_BOOLEAN);
  signals[SIGNAL_FILL_FAILED] = g_signal_new ("fill-failed", G_TYPE_FROM_CLASS (klass), G_SIGNAL_RUN_LAST, 0, NULL, NULL, NULL,
                                              G_TYPE_NONE, 0);
  signals[SIGNAL_SHAPE_FAILED] = g_signal_new ("shape-failed", G_TYPE_FROM_CLASS (klass), G_SIGNAL_RUN_LAST, 0, NULL, NULL, NULL,
                                              G_TYPE_NONE, 0);
  signals[SIGNAL_PENDING_CHANGED] = g_signal_new ("pending-changed", G_TYPE_FROM_CLASS (klass), G_SIGNAL_RUN_LAST, 0, NULL, NULL, NULL,
                                              G_TYPE_NONE, 0);
  signals[SIGNAL_STROKING_CHANGED] = g_signal_new ("stroking-changed", G_TYPE_FROM_CLASS (klass), G_SIGNAL_RUN_LAST, 0, NULL,
                                                   NULL, NULL, G_TYPE_NONE, 1, G_TYPE_BOOLEAN);
  signals[SIGNAL_STROKE_FINISHED] = g_signal_new ("stroke-finished", G_TYPE_FROM_CLASS (klass), G_SIGNAL_RUN_LAST,
                                                  0, NULL, NULL, NULL, G_TYPE_NONE, 1, G_TYPE_UINT);
}

static void
easel_canvas_init (EaselCanvas *self)
{
  self->zoom = 100;
  self->fill_max_bytes = SIZE_MAX;
  self->shape_max_bytes = SIZE_MAX;
  self->shape_width = 1;
  self->magnifier_level = easel_magnifier_levels[EASEL_MAGNIFIER_DEFAULT];
  self->grid_cell = 1;
  self->pinch_base = 1.0;
  self->builder = gdk_memory_texture_builder_new ();
  update_cursor (self);
  gtk_widget_set_halign (GTK_WIDGET (self), GTK_ALIGN_START);
  gtk_widget_set_valign (GTK_WIDGET (self), GTK_ALIGN_START);
  /* The picture lies EASEL_VIEW_MARGIN logical pixels in from every edge of the scrollable area (REQ-0009-01); the
     canvas's own coordinates stay those of the picture. */
  gtk_widget_set_margin_start (GTK_WIDGET (self), EASEL_VIEW_MARGIN);
  gtk_widget_set_margin_end (GTK_WIDGET (self), EASEL_VIEW_MARGIN);
  gtk_widget_set_margin_top (GTK_WIDGET (self), EASEL_VIEW_MARGIN);
  gtk_widget_set_margin_bottom (GTK_WIDGET (self), EASEL_VIEW_MARGIN);
  /* The style sheet draws the soft shadow around the canvas's box, which is the picture (REQ-0009-02). */
  gtk_widget_add_css_class (GTK_WIDGET (self), "easel-picture");
}
