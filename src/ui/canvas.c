/* SPDX-License-Identifier: GPL-3.0-or-later */

#include <math.h>

#include "config.h"
#include <adwaita.h>

#include "canvas.h"
#include "cursor-shape.h"
#include "fill.h"
#include "figure.h"
#include "gradient.h"
#include "lasso.h"
#include "lattice.h"
#include "line.h"
#include "selection.h"
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
  guint tiles_gen;        /* counts every tile rebuilt or dropped: a cached preview of a selection is stale after it */
  /* The trail of a floating selection (0014 T023), per tile, NULL where not laid: the picture, the hole and the copies laid so far, and
     its texture; the float laid over a copy of it, for the tiles under the float; and what a frame draws in each tile (borrowed). */
  GBytes **trail_data;
  size_t *trail_n;        /* the copies laid into each trail tile */
  GdkTexture **trail_tex;
  GdkTexture **comp_tex;
  GdkTexture **sel_tiles;
  int comp_x, comp_y;     /* where the float was when comp_tex were laid */
  guint64 trail_key;      /* what the trail was laid from, see trail_key_of (); 0: none */
  guint sel_epoch;        /* counts every cut-back of the copies: the trail laid before it is stale */
  gboolean probe_no_line, probe_no_tint;   /* benchmark hook, see easel_canvas_set_overlay_probe () */
  gboolean no_sel_handles;                 /* test hook, see easel_canvas_set_selection_handles_shown () */
  gboolean line_unmerged;                  /* test hook, see easel_canvas_set_line_merge () */
  gsize line_strips;                       /* test hook: the white strips of the last line drawn */
  gboolean tint_rows;                      /* test hook, see easel_canvas_set_tint_bands () */
  gsize tint_rects;                        /* test hook: the rectangles of the last tint drawn */
  gsize trail_tiles_laid, trail_copies_laid, trail_floats_laid, trail_textures, trail_bytes;   /* test hooks, see easel_canvas_get_trail_counts () */
  gsize comp_bytes;       /* the float's tiles over the trail held now: they count against the preview limit with the trail's */
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
  size_t preview_max_bytes; /* test hook: what a frame's gradient preview may use */
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
  gboolean pend_gradient;                       /* the pending shape is a gradient (0012): its start in pend_pts[0, 1], its end in [2, 3] */
  gboolean pend_unshown;                        /* the last frame could not get the memory to show the gradient (REQ-0012-09) */
  guint grad_button;                            /* the button that started the gradient: its color is the current foreground or background (REQ-0012-04) */
  gboolean guides_hidden;                       /* test hook: the markers and the guide line not drawn */
  /* The selection (0013): none, marked or floating, its rectangle in picture pixels; a rectangle being dragged out. */
  int sel_state;                                /* 0 none, 1 marked, 2 floating */
  int sel_x, sel_y, sel_w, sel_h;
  gboolean selecting;                           /* a rectangle is being dragged out (REQ-0013-04) */
  guint sel_button;
  int sel_px0, sel_py0, sel_px1, sel_py1;       /* the pixel under the press and under the pointer */
  EaselFloat sel_float;                         /* while floating: the lifted pixels at (sel_x, sel_y) (REQ-0013-07) */
  gboolean sel_moving;                          /* a move drag of the selection is held */
  gboolean sel_copy;                            /* Ctrl was held at the move's press: it moves a copy (REQ-0013-09) */
  gboolean sel_moved;                           /* the held move has moved (its copy, if any, is left) */
  gboolean sel_lifted_here;                     /* the held move lifted the selection (REQ-0013-18: a cancel undoes the lift) */
  gboolean sel_stamped_here;                    /* the held move left a copy at its press (a cancel takes it away) */
  gboolean sel_smear;                           /* Shift without Ctrl at the press: the held move leaves a trail of copies (REQ-0014-11) */
  size_t sel_stamps0;                           /* the copies the float had at the press (a cancel takes the trail away) */
  gboolean ctrl;                                /* Ctrl is held now (the window's key controller) */
  int sel_move_x0, sel_move_y0;                 /* the selection's place at the move's press */
  gboolean sel_transparent;                     /* Transparent mode (REQ-0013-13) */
  gboolean hover_sel;                           /* the pointer is on the selection with Select: the move cursor */
  int hover_sel_handle;                         /* the selection's handle under the pointer (an EaselSelHandle), or -1 */
  size_t stretch_stamps0;                       /* the copies the float had at the press of a stretch */
  gboolean stretching;                          /* a handle of the selection is dragged (REQ-0015-03) */
  int stretch_handle;                           /* the handle held (an EaselSelHandle) */
  double stretch_x0, stretch_y0;                /* where the pointer was at the press, in picture pixels: the zoom may change while it is held */
  double stretch_lx, stretch_ly;                /* where the pointer is now, in picture pixels */
  int stretch_sx, stretch_sy, stretch_sw, stretch_sh;   /* the selection's rectangle at the press */
  gboolean stretch_copied;                      /* the copy of a Ctrl stretch of a floating selection was left (REQ-0015-04) */
  guint ants_tick;                              /* the timer of the moving dashes, 12 a second; 0 when there is none */
  gsize ants_redraws;                           /* test hook: the redraws the moving dashes asked for */
  gsize ants_calls;                             /* test hook: the times their callback ran */
  gint64 ants_step;                             /* the step of the dashes last drawn (see ants_now_step) */
  gint64 ants_clock;                            /* test hook: a fixed time for the dashes, in microseconds; 0: the real clock */
  gboolean ants_continuous;                     /* test hook: the dashes advanced continuously, as before the steps */
  /* The free-form selection (0014): a path being traced, and the shape of a marked or floating selection. */
  gboolean lassoing;                            /* a path is being traced (REQ-0014-04) */
  EaselPath lasso_path;                         /* the corners of the grid the pointer reached */
  int lasso_x, lasso_y;                         /* the last pointer pixel */
  int lasso_x0, lasso_y0, lasso_x1, lasso_y1;   /* the box of the corners of the path so far, in corners (the size field) */
  gboolean lasso_failed;                        /* the memory for the path ran out: 0014 T014 */
  uint8_t *sel_mask;                            /* the shape, sel_w x sel_h, NULL for a rectangle */
  EaselPath sel_path;                           /* the corners of the path it was made by: its line, closed (REQ-0014-05) */
  int sel_path_x, sel_path_y;                   /* the place of the selection when the path was made: the line moves with it */
  EaselPath sel_path0;                          /* once stretched: the path as made, kept to scale afresh from (REQ-0015-06) */
  int sel_p0_x, sel_p0_y, sel_p0_w, sel_p0_h;   /* the shape box that path0 traced */
  size_t sel_lift_max, sel_step_max;            /* test hooks: what a lift and a put-down's step may use */
  int moving;                                   /* an end being moved by its marker: 0 the start, 1 the end, -1 none (REQ-0012-13) */
  int move_old[4];                              /* the line before that move, put back if it is cancelled */
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
  gboolean hover_marker;  /* a marker of a waiting gradient is under the pointer (REQ-0012-13): the move cursor */
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
  SIGNAL_GRADIENT_FAILED,
  SIGNAL_SELECTION_CHANGED,
  SIGNAL_CONTEXT_MENU,
  SIGNAL_SELECTION_FAILED,
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
static void update_cursor (EaselCanvas *self);
static void to_pixel (EaselCanvas *self, double x, double y, int *px, int *py);
static gboolean within_double_click_distance (EaselCanvas *self, double x, double y, double ox, double oy);
static EaselFigure pending_figure (EaselCanvas *self);
static gboolean on_spray_tick (gpointer data);

/* Resize handles (REQ-0004-16): 5 x 5 logical pixels touching the picture, a hit area of 9 x 9 around the center. They
   reach 5 + 2 pixels past the picture, inside the margin of EASEL_VIEW_MARGIN around it (REQ-0009-01). */
#define HANDLE_SIZE 5.0
#define HANDLE_HIT 9.0
#define MARKER_SIZE 5.0          /* a gradient's marker: the size of a handle (REQ-0012-13) ... */
#define MARKER_HIT 9.0           /* ... and its hit area */

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

static void free_trail (EaselCanvas *self);
static void drop_float (EaselCanvas *self);
static void cut_copies (EaselCanvas *self, size_t n);

static void
free_tiles (EaselCanvas *self)
{
  free_trail (self);
  self->tiles_gen++;
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
  self->tiles_gen++;
}

void
easel_canvas_set_image (EaselCanvas *self, EaselImage *img)
{
  EaselRect ignored;

  free_tiles (self);
  self->img = img;
  self->resizing = EASEL_HANDLE_NONE;
  self->hover = EASEL_HANDLE_NONE;
  self->hover_sel_handle = -1;
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

/* The markers of a waiting gradient (REQ-0012-13): 5 x 5 logical pixels, centered on the end's pixel, placed on whole device
   pixels so that their edges are sharp. FALSE when no marker is shown. */
gboolean
easel_canvas_get_marker_rect (EaselCanvas *self, int end, graphene_rect_t *rect)
{
  double l, scale, cx, cy;

  if (!self->pending || !self->pend_gradient || (self->pend_held && self->moving < 0) || end < 0 || end > 1)
    return FALSE;
  l = logical_per_pixel (self);
  scale = easel_canvas_get_scale (self);
  cx = (self->pend_pts[2 * end] + 0.5) * l;
  cy = (self->pend_pts[2 * end + 1] + 0.5) * l;
  *rect = GRAPHENE_RECT_INIT ((float) (round ((cx - MARKER_SIZE / 2.0) * scale) / scale),
                              (float) (round ((cy - MARKER_SIZE / 2.0) * scale) / scale), MARKER_SIZE, MARKER_SIZE);
  return TRUE;
}

void
easel_canvas_set_guides_hidden (EaselCanvas *self, gboolean hidden)
{
  self->guides_hidden = hidden;
}

gboolean
easel_canvas_get_guides_hidden (EaselCanvas *self)
{
  return self->guides_hidden;
}

/* Is (x), a canvas point, within the hit area of a marker? */
static gboolean
marker_hit (EaselCanvas *self, double x, double y)
{
  for (int end = 0; end <= 1; end++)
    {
      graphene_rect_t r;

      if (easel_canvas_get_marker_rect (self, end, &r)
          && fabs (x - (r.origin.x + MARKER_SIZE / 2.0)) <= MARKER_HIT / 2.0
          && fabs (y - (r.origin.y + MARKER_SIZE / 2.0)) <= MARKER_HIT / 2.0)
        return TRUE;
    }
  return FALSE;
}

/* The marker under (x, y), a canvas point: 0 the start's, 1 the end's, -1 none. Within both hit areas, the one whose center is
   nearest, the end's on a tie (REQ-0012-13). */
static int
marker_at (EaselCanvas *self, double x, double y)
{
  int best = -1;
  double best_d = 0;

  for (int end = 1; end >= 0; end--)
    {
      graphene_rect_t r;
      double dx, dy, d;

      if (!easel_canvas_get_marker_rect (self, end, &r))
        return -1;
      dx = x - (r.origin.x + MARKER_SIZE / 2.0);
      dy = y - (r.origin.y + MARKER_SIZE / 2.0);
      d = dx * dx + dy * dy;
      if (fabs (dx) <= MARKER_HIT / 2.0 && fabs (dy) <= MARKER_HIT / 2.0 && (best < 0 || d < best_d))
        {
          best = end;
          best_d = d;
        }
    }
  return best;
}

/* The guides of a waiting gradient (REQ-0012-13): the line between the markers' centers, 1 logical pixel wide, white under
   black dashes of 4 logical pixels, then the two markers over it, white with a black border one logical pixel wide. */
static void
append_gradient_markers (EaselCanvas *self, GtkSnapshot *snapshot)
{
  static const GdkRGBA black = { 0, 0, 0, 1 }, white = { 1, 1, 1, 1 };
  static const float dash[] = { 4, 4 };
  graphene_rect_t m[2];
  g_autoptr (GskPathBuilder) builder = NULL;
  g_autoptr (GskPath) path = NULL;
  g_autoptr (GskStroke) stroke = NULL;

  if (self->guides_hidden || !easel_canvas_get_marker_rect (self, 0, &m[0]) || !easel_canvas_get_marker_rect (self, 1, &m[1]))
    return;
  builder = gsk_path_builder_new ();
  gsk_path_builder_move_to (builder, m[0].origin.x + MARKER_SIZE / 2, m[0].origin.y + MARKER_SIZE / 2);
  gsk_path_builder_line_to (builder, m[1].origin.x + MARKER_SIZE / 2, m[1].origin.y + MARKER_SIZE / 2);
  path = gsk_path_builder_to_path (builder);
  stroke = gsk_stroke_new (1.0f);
  gtk_snapshot_append_stroke (snapshot, path, stroke, &white);
  gsk_stroke_set_dash (stroke, dash, G_N_ELEMENTS (dash));
  gtk_snapshot_append_stroke (snapshot, path, stroke, &black);
  for (int end = 0; end <= 1; end++)
    {
      gtk_snapshot_append_color (snapshot, &black, &m[end]);
      gtk_snapshot_append_color (snapshot, &white, &GRAPHENE_RECT_INIT (m[end].origin.x + 1, m[end].origin.y + 1,
                                                                        m[end].size.width - 2, m[end].size.height - 2));
    }
}

/* The rectangle being dragged out (REQ-0013-04): from the pixel under the press to the pixel under the pointer, both
   included; with Shift a square of the smaller side from the press toward the pointer, made first; then limited to the
   picture. FALSE if no pixel of it is inside the picture. */
static gboolean
drag_rect (EaselCanvas *self, int *x, int *y, int *w, int *h)
{
  int x0 = self->sel_px0, y0 = self->sel_py0, x1 = self->sel_px1, y1 = self->sel_py1, lx, ly, hx, hy;

  if (self->shift)
    easel_figure_constrain (EASEL_FIGURE_RECTANGLE, x0, y0, &x1, &y1);
  lx = MAX (MIN (x0, x1), 0);
  ly = MAX (MIN (y0, y1), 0);
  hx = MIN (MAX (x0, x1), self->img->width - 1);
  hy = MIN (MAX (y0, y1), self->img->height - 1);
  if (lx > hx || ly > hy)
    return FALSE;
  *x = lx;
  *y = ly;
  *w = hx - lx + 1;
  *h = hy - ly + 1;
  return TRUE;
}

void
easel_canvas_set_selection_lift_limit (EaselCanvas *self, size_t max_bytes)
{
  self->sel_lift_max = max_bytes;
}

void
easel_canvas_set_selection_step_limit (EaselCanvas *self, size_t max_bytes)
{
  self->sel_step_max = max_bytes;
}

void
easel_canvas_set_selection_transparent (EaselCanvas *self, gboolean transparent)
{
  self->sel_transparent = transparent;            /* what the next lift and Clear leave behind (REQ-0013-08, -14) */
}

gboolean
easel_canvas_get_selection (EaselCanvas *self, int *x, int *y, int *w, int *h, gboolean *floating)
{
  *x = self->sel_x;
  *y = self->sel_y;
  *w = self->sel_w;
  *h = self->sel_h;
  *floating = self->sel_state == 2;
  return self->sel_state != 0;
}

gboolean
easel_canvas_get_selection_size (EaselCanvas *self, int *w, int *h)
{
  int x, y;
  gboolean floating;

  if (self->lassoing)
    {
      /* The box of the corners so far, limited to the picture. */
      int x0 = MAX (self->lasso_x0, 0), y0 = MAX (self->lasso_y0, 0);
      int x1 = MIN (self->lasso_x1, self->img->width), y1 = MIN (self->lasso_y1, self->img->height);

      if (x1 <= x0 || y1 <= y0)
        return FALSE;
      *w = x1 - x0;
      *h = y1 - y0;
      return TRUE;
    }
  if (self->selecting)
    return drag_rect (self, &x, &y, w, h);
  return easel_canvas_get_selection (self, &x, &y, w, h, &floating);
}

/* The part of a floating selection outside the picture, drawn over the canvas area so that the user sees what they move
   (REQ-0013-07): only the part in view, its see-through pixels left out in Transparent mode, clipped to the four bands around
   the picture (the picture's part is the tiles' preview). */
static void
append_float_outside (EaselCanvas *self, GtkSnapshot *snapshot, double l)
{
  const EaselFloat *f = &self->sel_float;
  graphene_rect_t seen;
  int vx0, vy0, vx1, vy1, x0, y0, x1, y1, rw, rh;
  float pw, ph;
  uint32_t *px;
  g_autoptr (GBytes) data = NULL;
  g_autoptr (GdkTexture) texture = NULL;

  if (self->sel_state != 2 || f->px == NULL)
    return;
  if (!gtk_widget_compute_bounds (gtk_widget_get_parent (GTK_WIDGET (self)), GTK_WIDGET (self), &seen))
    return;
  vx0 = (int) floor (seen.origin.x / l) - 1;
  vy0 = (int) floor (seen.origin.y / l) - 1;
  vx1 = (int) ceil ((seen.origin.x + seen.size.width) / l) + 1;
  vy1 = (int) ceil ((seen.origin.y + seen.size.height) / l) + 1;
  x0 = MAX (f->x, vx0);
  y0 = MAX (f->y, vy0);
  x1 = MIN (f->x + f->w, vx1);
  y1 = MIN (f->y + f->h, vy1);
  if (x0 >= x1 || y0 >= y1)
    return;
  if (x0 >= 0 && y0 >= 0 && x1 <= self->img->width && y1 <= self->img->height)
    return;                                     /* wholly over the picture: the tiles show it */
  rw = x1 - x0;
  rh = y1 - y0;
  easel_selection_fill (&self->sel_float, x0 - f->x, y0 - f->y, x1 - f->x, y1 - f->y);     /* a lazily stretched image: the part read */
  px = g_try_malloc ((gsize) rw * rh * sizeof *px);
  if (px == NULL)
    return;
  for (int j = 0; j < rh; j++)
    for (int i = 0; i < rw; i++)
      {
        size_t at = (size_t) (y0 + j - f->y) * f->w + (x0 + i - f->x);

        px[(size_t) j * rw + i] = f->mask == NULL || f->mask[at] ? f->px[at] : 0;      /* the pixels of a shape only */
      }
  data = g_bytes_new_take (px, (gsize) rw * rh * sizeof *px);
  texture = gdk_memory_texture_new (rw, rh, PICTURE_FORMAT, data, (gsize) rw * sizeof *px);
  pw = (float) (self->img->width * l);
  ph = (float) (self->img->height * l);
  {
    const float big = 1e5f;                                  /* past any picture, and within the range Cairo can clip */
    const graphene_rect_t bands[] = {
      GRAPHENE_RECT_INIT (-big, -big, big, 2 * big),            /* left of the picture */
      GRAPHENE_RECT_INIT (pw, -big, big, 2 * big),              /* right of it */
      GRAPHENE_RECT_INIT (0, -big, pw, big),                    /* above it */
      GRAPHENE_RECT_INIT (0, ph, pw, big),                      /* below it */
    };

    for (guint b = 0; b < G_N_ELEMENTS (bands); b++)
      {
        gtk_snapshot_push_clip (snapshot, &bands[b]);
        gtk_snapshot_append_scaled_texture (snapshot, texture, GSK_SCALING_FILTER_NEAREST,
                                            &GRAPHENE_RECT_INIT ((float) (x0 * l), (float) (y0 * l), (float) (rw * l),
                                                                 (float) (rh * l)));
        gtk_snapshot_pop (snapshot);
      }
  }
}

/* Do the dashes of a selection's line move now (REQ-0014-17 c)? A marked or floating selection with the button up, the guides shown
   and animations on in the user's settings. */
static gboolean
ants_active (EaselCanvas *self)
{
  gboolean on = TRUE;

  if (self->sel_state == 0 || self->sel_moving || self->guides_hidden)
    return FALSE;                               /* (a path traced or a rectangle dragged out is no selection yet) */
  g_object_get (gtk_widget_get_settings (GTK_WIDGET (self)), "gtk-enable-animations", &on, NULL);
  return on;
}

/* A step at a time (12 a second), while they move: the canvas is drawn again; when nothing moves any more the timer ends itself. */
static gboolean
ants_tick_cb (gpointer data)
{
  EaselCanvas *self = data;
  GtkWidget *widget = GTK_WIDGET (self);
  gint64 step;

  self->ants_calls++;
  if (!ants_active (self))
    {
      self->ants_tick = 0;
      return G_SOURCE_REMOVE;
    }
  /* A redraw only when the dashes reach their next step, 12 a second (REQ-0014-16, -17 c): between two steps nothing moves. */
  step = (gint64) floor ((self->ants_clock != 0 ? self->ants_clock : g_get_monotonic_time ()) / 1e6 * 12);
  if (step == self->ants_step && !self->ants_continuous)
    return G_SOURCE_CONTINUE;
  self->ants_step = step;
  self->ants_redraws++;
  gtk_widget_queue_draw (widget);
  return G_SOURCE_CONTINUE;
}

/* How far the pattern of dashes has advanced along the line of a selection, in logical pixels, modulo the length of a dash and a
   gap (12 a second), and the callback that keeps it moving started if it is not; 0 when nothing moves. */
static double
ants_phase (EaselCanvas *self, double period)
{
  if (!ants_active (self))
    return 0;
  if (self->ants_tick == 0)
    self->ants_tick = g_timeout_add (1000 / 12, ants_tick_cb, self);   /* once a step: nothing runs between two (REQ-0014-16) */
  {
    double t = (self->ants_clock != 0 ? self->ants_clock : g_get_monotonic_time ()) / 1e6 * 12;    /* logical pixels so far */

    /* In whole logical pixels: one step at a time (the continuous movement is a test hook). */
    return fmod (self->ants_continuous ? t : floor (t), period);
  }
}

gsize
easel_canvas_get_ants_calls (EaselCanvas *self)
{
  return self->ants_calls;
}

void
easel_canvas_get_ants_counts (EaselCanvas *self, gsize *redraws, gint64 *step)
{
  *redraws = self->ants_redraws;
  *step = (gint64) floor ((self->ants_clock != 0 ? self->ants_clock : g_get_monotonic_time ()) / 1e6 * 12);
}

void
easel_canvas_set_ants_clock (EaselCanvas *self, gint64 usec, gboolean continuous)
{
  self->ants_clock = usec;
  self->ants_continuous = continuous;
  gtk_widget_queue_draw (GTK_WIDGET (self));
}

gboolean
easel_canvas_get_ants_running (EaselCanvas *self)
{
  return self->ants_tick != 0;
}

/* The thickness of the outline of a selection in logical pixels (REQ-0014-17 a): 1 while a picture pixel is less than 6 device pixels
   wide, 2 from 6. */
static double
line_thickness (EaselCanvas *self)
{
  double cell = logical_per_pixel (self) * easel_canvas_get_scale (self);

  return cell < 6 ? 1 : 2;
}

/* One strip of the line of a free-form selection, th logical pixels thick, along a..b (logical pixels) on the device pixel that
   starts at the grid line c, as the grid is drawn (REQ-0004-12). */
static void
append_strip (EaselCanvas *self, GtkSnapshot *snapshot, const GdkRGBA *color, gboolean vertical, double a, double b, double c, double th)
{
  double scale = easel_canvas_get_scale (self), t = floor (c * scale) / scale;

  if (b <= a)
    return;
  if (vertical)
    gtk_snapshot_append_color (snapshot, color, &GRAPHENE_RECT_INIT ((float) t, (float) a, (float) th, (float) (b - a)));
  else
    gtk_snapshot_append_color (snapshot, color, &GRAPHENE_RECT_INIT ((float) a, (float) t, (float) (b - a), (float) th));
}

/* The line of a free-form selection (REQ-0014-05): along the grid lines of the path through the n corners (x, y pairs, moved by
   (dx, dy) picture pixels), closed back to the first corner once the path is made, a white strip with black dashes of 4 logical
   pixels counted along it from the first corner. Every strip lies on a grid line, so none crosses the inside of a pixel at any zoom. */
static void
append_path_line (EaselCanvas *self, GtkSnapshot *snapshot, const int *xy, size_t n, int dx, int dy, gboolean closed, double phase)
{
  static const GdkRGBA black = { 0, 0, 0, 1 }, white = { 1, 1, 1, 1 };
  double l = logical_per_pixel (self), th = line_thickness (self);
  size_t segments = closed && n >= 2 ? n : (n >= 1 ? n - 1 : 0), n_dash;
  EaselDash *dash;

  self->line_strips = 0;
  if (!self->line_unmerged)
    {
      /* The white line as one strip per straight run: a path along straight grid lines costs a few strips, however many corners it
         was traced through (REQ-0014-16). The same pixels as a strip per grid step, which the test hook draws. */
      size_t n_run = easel_lattice_runs (xy, n, closed, NULL, 0);
      EaselDash *run = n_run ? g_try_new (EaselDash, n_run) : NULL;

      if (run != NULL)
        {
          n_run = easel_lattice_runs (xy, n, closed, run, n_run);
          for (size_t i = 0; i < n_run; i++)
            {
              if (run[i].y0 == run[i].y1)
                append_strip (self, snapshot, &white, FALSE, (MIN (run[i].x0, run[i].x1) + dx) * l, (MAX (run[i].x0, run[i].x1) + dx) * l,
                              (run[i].y0 + dy) * l, th);
              else
                append_strip (self, snapshot, &white, TRUE, (MIN (run[i].y0, run[i].y1) + dy) * l, (MAX (run[i].y0, run[i].y1) + dy) * l,
                              (run[i].x0 + dx) * l, th);
            }
          self->line_strips = n_run;
          g_free (run);
        }
      segments = 0;                                  /* no strip per grid step below */
    }
  for (size_t i = 0; i < segments; i++)
    {
      size_t j = (i + 1) % n;
      EaselLattice w;
      int x, y, px = 0, py = 0;
      gboolean first = TRUE;

      easel_lattice_init (&w, xy[2 * i], xy[2 * i + 1], xy[2 * j], xy[2 * j + 1]);
      while (easel_lattice_next (&w, &x, &y))
        {
          if (!first)
            {
              if (y == py)
                append_strip (self, snapshot, &white, FALSE, (MIN (x, px) + dx) * l, (MAX (x, px) + dx) * l, (y + dy) * l, th);
              else
                append_strip (self, snapshot, &white, TRUE, (MIN (y, py) + dy) * l, (MAX (y, py) + dy) * l, (x + dx) * l, th);
              self->line_strips++;
            }
          first = FALSE;
          px = x;
          py = y;
        }
    }
  n_dash = easel_lattice_dashes (xy, n, closed, 4 * th / l, phase / l, NULL, 0);
  dash = n_dash ? g_try_new (EaselDash, n_dash) : NULL;
  if (dash == NULL)
    return;
  n_dash = easel_lattice_dashes (xy, n, closed, 4 * th / l, phase / l, dash, n_dash);
  for (size_t i = 0; i < n_dash; i++)
    {
      if (dash[i].y0 == dash[i].y1)
        append_strip (self, snapshot, &black, FALSE, (MIN (dash[i].x0, dash[i].x1) + dx) * l, (MAX (dash[i].x0, dash[i].x1) + dx) * l,
                      (dash[i].y0 + dy) * l, th);
      else
        append_strip (self, snapshot, &black, TRUE, (MIN (dash[i].y0, dash[i].y1) + dy) * l, (MAX (dash[i].y0, dash[i].y1) + dy) * l,
                      (dash[i].x0 + dx) * l, th);
    }
  g_free (dash);
}

/* A strip of a rectangle's ring: its rectangle in logical pixels, whether it runs along x, the way its length is counted (+1 along
   the axis, -1 against it), where the count of the line begins on it and how long it is. */
typedef struct
{
  double x, y, w, h;
  gboolean horizontal;
  int dir;
  double a, len;
} RingStrip;

/* The black parts of the pattern of dashes d long and gaps d long, begun `phase` before the line's start, that fall in [a, a + len),
   each as [from, to) of the line. */
static void
ring_dashes (const RingStrip *s, double d, double phase, GtkSnapshot *snapshot)
{
  static const GdkRGBA black = { 0, 0, 0, 1 };
  double period = 2 * d;

  for (double k = floor ((s->a - phase) / period) - 1; k * period + phase < s->a + s->len; k++)
    {
      double from = MAX (s->a, k * period + phase), to = MIN (s->a + s->len, k * period + phase + d), f, e;

      if (to <= from)
        continue;
      f = from - s->a;
      e = to - s->a;
      if (s->dir < 0)
        {
          double t = s->len - e;

          e = s->len - f;
          f = t;
        }
      if (s->horizontal)
        gtk_snapshot_append_color (snapshot, &black, &GRAPHENE_RECT_INIT ((float) (s->x + f), (float) s->y, (float) (e - f), (float) s->h));
      else
        gtk_snapshot_append_color (snapshot, &black, &GRAPHENE_RECT_INIT ((float) s->x, (float) (s->y + f), (float) s->w, (float) (e - f)));
    }
}

/* The tint over the pixels of a selection (REQ-0014-17 b): the accent color of the system's settings at 28 %, over the picture and the grid and under the line, for
   the rectangle (marked, floating or being dragged out) and, for a free-form selection, the pixels of its shape; not for a path that
   is being traced; where the guides are hidden, not at all. */
/* The shape of the selection now: the float's (it changes by a stretch) while it floats, else the canvas's own copy of what was marked;
   NULL for a rectangle (REQ-0015-06). */
static const uint8_t *
selection_mask (EaselCanvas *self)
{
  return self->sel_state == 2 ? self->sel_float.mask : self->sel_mask;
}

static void
append_selection_tint (EaselCanvas *self, GtkSnapshot *snapshot)
{
  g_autoptr (GdkRGBA) accent = adw_style_manager_get_accent_color_rgba (adw_style_manager_get_default ());
  GdkRGBA tint = { accent->red, accent->green, accent->blue, 0.28f };      /* the system's accent at 28 % */
  double l = logical_per_pixel (self);
  const uint8_t *mask = selection_mask (self);
  int x, y, w, h;
  gboolean floating;

  if (self->guides_hidden)
    return;                                       /* (a path being traced is no selection yet: nothing is tinted) */
  if (self->selecting ? !drag_rect (self, &x, &y, &w, &h) : !easel_canvas_get_selection (self, &x, &y, &w, &h, &floating))
    return;
  self->tint_rects = 0;
  if (self->selecting || mask == NULL)
    {
      gtk_snapshot_append_color (snapshot, &tint, &GRAPHENE_RECT_INIT ((float) (x * l), (float) (y * l), (float) (w * l), (float) (h * l)));
      self->tint_rects = 1;
      return;
    }
  if (!self->tint_rows && l * easel_canvas_get_scale (self) < 1)
    {
      /* Below 100 % several picture rows share a device row: one band per device row, the union of their runs, so the tint costs the
         rows the view shows, not the picture's (REQ-0014-16); at 100 % and above, one rectangle per run of a picture row, below. */
      double s = easel_canvas_get_scale (self), d = l * s;              /* device pixels per picture pixel */
      g_autofree uint8_t *band = g_try_malloc ((size_t) w);
      int k0 = (int) floor (y * d), k1 = (int) ceil ((y + h) * d);

      if (band == NULL)
        return;
      for (int k = k0; k < k1; k++)
        {
          int r0 = MAX (y, (int) ceil (k / d)), r1 = MIN (y + h, (int) ceil ((k + 1) / d));    /* the rows whose top is in device row k */
          double top = MAX (k / s, y * l), bottom = MIN ((k + 1) / s, (y + h) * l);

          if (r0 >= r1)
            continue;
          memset (band, 0, (size_t) w);
          for (int r = r0; r < r1; r++)
            for (int i = 0; i < w; i++)
              band[i] |= mask[(size_t) (r - y) * w + i];
          for (int i = 0; i < w;)
            {
              int i0 = i;

              if (!band[i])
                {
                  i++;
                  continue;
                }
              while (i < w && band[i])
                i++;
              gtk_snapshot_append_color (snapshot, &tint, &GRAPHENE_RECT_INIT ((float) ((x + i0) * l), (float) top, (float) ((i - i0) * l),
                                                                               (float) (bottom - top)));
              self->tint_rects++;
            }
        }
      return;
    }
  for (int j = 0; j < h; j++)
    for (int i = 0; i < w;)
      {
        int i0 = i;

        if (!mask[(size_t) j * w + i])
          {
            i++;
            continue;
          }
        while (i < w && mask[(size_t) j * w + i])
          i++;
        gtk_snapshot_append_color (snapshot, &tint,
                                   &GRAPHENE_RECT_INIT ((float) ((x + i0) * l), (float) ((y + j) * l), (float) ((i - i0) * l), (float) l));
        self->tint_rects++;
      }
}

/* The selection's outline (REQ-0013-05, REQ-0014-17): a ring just outside its pixels, white, with black dashes counted along it as
   one line clockwise from its top-left corner; drawn as rectangles, so that its edges fall on whole pixels with no blended corner. */
static void
append_selection_outline (EaselCanvas *self, GtkSnapshot *snapshot)
{
  static const GdkRGBA white = { 1, 1, 1, 1 };
  double l = logical_per_pixel (self);
  int x, y, w, h;
  gboolean floating;

  if (self->guides_hidden)
    return;
  if (self->lassoing)
    {
      append_path_line (self, snapshot, self->lasso_path.xy, self->lasso_path.n, 0, 0, FALSE, 0);
      return;
    }
  if (self->sel_state != 0 && selection_mask (self) != NULL)
    {
      append_path_line (self, snapshot, self->sel_path.xy, self->sel_path.n, self->sel_x - self->sel_path_x,
                        self->sel_y - self->sel_path_y, TRUE, ants_phase (self, 8 * line_thickness (self)));
      return;
    }
  if (self->selecting ? !drag_rect (self, &x, &y, &w, &h) : !easel_canvas_get_selection (self, &x, &y, &w, &h, &floating))
    return;
  {
    double th = line_thickness (self), d = 4 * th, X0 = x * l, Y0 = y * l, X1 = (x + w) * l, Y1 = (y + h) * l;
    double wl = (X1 - X0) + 2 * th, hl = Y1 - Y0;
    RingStrip ring[4] = {
      { X0 - th, Y0 - th, wl, th, TRUE, 1, 0, wl },                    /* the top, left to right */
      { X1, Y0, th, hl, FALSE, 1, wl, hl },                            /* the right side, down */
      { X0 - th, Y1, wl, th, TRUE, -1, wl + hl, wl },                  /* the bottom, right to left */
      { X0 - th, Y0, th, hl, FALSE, -1, 2 * wl + hl, hl },             /* the left side, up */
    };

    for (int i = 0; i < 4; i++)
      gtk_snapshot_append_color (snapshot, &white, &GRAPHENE_RECT_INIT ((float) ring[i].x, (float) ring[i].y, (float) ring[i].w, (float) ring[i].h));
    {
      double phase = ants_phase (self, 2 * d);

      for (int i = 0; i < 4; i++)
        ring_dashes (&ring[i], d, phase, snapshot);
    }
  }
}

/* What a lift or a Clear leaves behind (REQ-0013-08, -14): fully transparent in Transparent mode, the background color in
   Opaque mode. */
static uint32_t
hole_color (EaselCanvas *self)
{
  return self->sel_transparent ? 0x00000000u : self->background;
}

/* The shape of the selection and what is made from it are gone: a rectangle, or no selection, follows. */
static void
shape_free (EaselCanvas *self)
{
  g_clear_pointer (&self->sel_mask, free);
  easel_path_free (&self->sel_path);
  easel_path_free (&self->sel_path0);
}

/* The path being traced is gone. */
static void
lasso_free (EaselCanvas *self)
{
  easel_path_free (&self->lasso_path);
  self->lasso_failed = FALSE;
}

/* The corner of the grid nearest to the canvas point (x, y) (REQ-0014-04): ties go to the larger corner. */
static void
to_corner (EaselCanvas *self, double x, double y, int *cx, int *cy)
{
  double l = logical_per_pixel (self);

  *cx = (int) floor (x / l + 0.5);
  *cy = (int) floor (y / l + 0.5);
}

/* The pointer is now nearest to the corner (cx, cy): added to the path if it is another corner than the last (REQ-0014-04). */
static void
lasso_extend (EaselCanvas *self, int cx, int cy)
{
  if (self->lasso_failed || (cx == self->lasso_x && cy == self->lasso_y))
    return;
  if (!easel_path_add (&self->lasso_path, cx, cy, self->sel_lift_max))
    {
      self->lasso_failed = TRUE;
      return;
    }
  self->lasso_x = cx;
  self->lasso_y = cy;
  self->lasso_x0 = MIN (self->lasso_x0, cx);
  self->lasso_x1 = MAX (self->lasso_x1, cx);
  self->lasso_y0 = MIN (self->lasso_y0, cy);
  self->lasso_y1 = MAX (self->lasso_y1, cy);
  g_signal_emit (self, signals[SIGNAL_SELECTION_CHANGED], 0);
  gtk_widget_queue_draw (GTK_WIDGET (self));
}

/* A press with Free-Form Select that starts a path at the corner (cx, cy). */
static void
lasso_begin (EaselCanvas *self, guint button, int cx, int cy)
{
  lasso_free (self);
  self->lassoing = TRUE;
  self->sel_button = button;
  self->lasso_x = self->lasso_x0 = self->lasso_x1 = cx;
  self->lasso_y = self->lasso_y0 = self->lasso_y1 = cy;
  if (!easel_path_add (&self->lasso_path, cx, cy, self->sel_lift_max))
    self->lasso_failed = TRUE;
  g_signal_emit (self, signals[SIGNAL_STROKING_CHANGED], 0, TRUE);
  g_signal_emit (self, signals[SIGNAL_SELECTION_CHANGED], 0);
  gtk_widget_queue_draw (GTK_WIDGET (self));
}

/* The release of the path (or a lost one): the path is closed and the pixels it encloses marked, limited to the picture; a click, a
   path that encloses nothing, a shape outside the picture, and (0014 T014) a failure make no selection. */
static void
end_lasso (EaselCanvas *self)
{
  int x, y, w, h;
  uint8_t *mask = NULL;
  EaselPath path = self->lasso_path;
  EaselSelectionResult r = self->lasso_failed ? EASEL_SELECTION_NO_MEMORY
                           : easel_lasso_shape (&self->lasso_path, self->img->width, self->img->height, &x, &y, &w, &h, &mask,
                                                self->sel_lift_max);

  if (r == EASEL_SELECTION_NO_MEMORY)
    g_signal_emit (self, signals[SIGNAL_SELECTION_FAILED], 0, 2);  /* no selection; the dialog (REQ-0014-15) */
  self->lassoing = FALSE;
  self->lasso_path = (EaselPath) { 0 };         /* the path is the selection's now, or freed below */
  lasso_free (self);
  shape_free (self);
  self->sel_state = 0;
  if (r != EASEL_SELECTION_DONE)
    easel_path_free (&path);
  else
    {
      self->sel_state = 1;
      self->sel_x = self->sel_path_x = x;
      self->sel_y = self->sel_path_y = y;
      self->sel_w = w;
      self->sel_h = h;
      self->sel_mask = mask;
      self->sel_path = path;                   /* the selection keeps the path: it is its line */
    }
  g_signal_emit (self, signals[SIGNAL_STROKING_CHANGED], 0, FALSE);
  g_signal_emit (self, signals[SIGNAL_SELECTION_CHANGED], 0);
  gtk_widget_queue_draw (GTK_WIDGET (self));
}

const int *
easel_canvas_get_selection_path (EaselCanvas *self, size_t *n, int *dx, int *dy)
{
  if (self->sel_state == 0 || self->sel_path.n == 0)
    return NULL;
  *n = self->sel_path.n;
  *dx = self->sel_x - self->sel_path_x;
  *dy = self->sel_y - self->sel_path_y;
  return self->sel_path.xy;
}

const uint8_t *
easel_canvas_get_selection_mask (EaselCanvas *self, int *w, int *h)
{
  if (self->sel_state == 0 || selection_mask (self) == NULL)
    return NULL;
  *w = self->sel_w;
  *h = self->sel_h;
  return selection_mask (self);
}

/* Is (x, y), a canvas point, on the selection's pixels, with Select or Free-Form Select active (REQ-0013-03, -07, REQ-0014-07):
   a pixel of its rectangle, and for a free-form selection a pixel of its shape. */
static gboolean
on_selection (EaselCanvas *self, double x, double y)
{
  int px, py;

  if (!easel_tool_selects (self->tool) || self->sel_state == 0 || self->img == NULL)
    return FALSE;
  to_pixel (self, x, y, &px, &py);
  if (px < self->sel_x || py < self->sel_y || px - self->sel_x >= self->sel_w || py - self->sel_y >= self->sel_h)
    return FALSE;
  return selection_mask (self) == NULL || selection_mask (self)[(size_t) (py - self->sel_y) * self->sel_w + (px - self->sel_x)] != 0;
}

/* The one function that puts a floating selection down and removes the selection (REQ-0013-11, -12); keep_marked keeps a
   marked one (Save, Save As). FALSE if the put-down could not get its memory: the picture is as before the lift and the
   selection is gone (REQ-0013-19). */
static gboolean
finish_selection (EaselCanvas *self, gboolean keep_marked)
{
  gboolean ok = TRUE;

  if (self->sel_state == 2)
    {
      EaselSelectionResult r;

      easel_selection_complete (&self->sel_float);   /* a stretch still held: the whole image is made before it is laid */
      r = easel_selection_put_down (&self->sel_float, TRUE, self->img, self->history, self->sel_step_max);

      ok = r != EASEL_SELECTION_NO_MEMORY;
      if (!ok)
        g_signal_emit (self, signals[SIGNAL_SELECTION_FAILED], 0, 0);     /* the dialog comes after the action goes on */
      drop_float (self);
      shape_free (self);
      self->sel_state = 0;
      if (r == EASEL_SELECTION_DONE)
        easel_canvas_update (self);
      g_signal_emit (self, signals[SIGNAL_PENDING_CHANGED], 0);    /* the window syncs Undo, Redo and the title */
    }
  else if (self->sel_state == 1 && !keep_marked)
    {
      shape_free (self);
      self->sel_state = 0;
    }
  if (self->hover_sel && self->sel_state == 0)
    {
      self->hover_sel = FALSE;
      self->hover_sel_handle = -1;
      update_cursor (self);
    }
  g_signal_emit (self, signals[SIGNAL_SELECTION_CHANGED], 0);
  gtk_widget_queue_draw (GTK_WIDGET (self));
  return ok;
}

/* A lift or a Ctrl copy of a move could not get its memory: the dialog, nothing changed, the selection where it was; the
   drag ends there, its motion and release do nothing (REQ-0013-19). */
static void
end_failed_move (EaselCanvas *self)
{
  g_signal_emit (self, signals[SIGNAL_SELECTION_FAILED], 0, 0);
  self->sel_moving = FALSE;
  self->cancelled_button = self->sel_button;
  g_signal_emit (self, signals[SIGNAL_STROKING_CHANGED], 0, FALSE);
}

/* A copy of a smear could not get its memory: the dialog, and the selection as it was before the drag: marked again if the drag
   lifted it, else floating where it was with the copies of this drag gone; the drag ends and its release does nothing (REQ-0014-13). */
static void
end_failed_smear (EaselCanvas *self)
{
  if (self->sel_lifted_here)
    {
      drop_float (self);
      self->sel_state = 1;
      self->sel_lifted_here = FALSE;
      g_signal_emit (self, signals[SIGNAL_PENDING_CHANGED], 0);
    }
  else
    cut_copies (self, self->sel_stamps0);
  self->sel_x = self->sel_move_x0;
  self->sel_y = self->sel_move_y0;
  if (self->sel_state == 2)
    {
      self->sel_float.x = self->sel_x;
      self->sel_float.y = self->sel_y;
    }
  end_failed_move (self);
  g_signal_emit (self, signals[SIGNAL_SELECTION_CHANGED], 0);
  gtk_widget_queue_draw (GTK_WIDGET (self));
}

/* The line of a free-form selection as it was at the press of a stretch that is cancelled: the traced path scaled to that rectangle. */
static void
stretch_restore_path (EaselCanvas *self)
{
  EaselPath scaled = { 0 };

  if (self->sel_path0.n == 0 || !easel_lasso_scale_path (&self->sel_path0, self->sel_p0_x, self->sel_p0_y, self->sel_p0_w, self->sel_p0_h,
                                                       self->stretch_sx, self->stretch_sy, self->stretch_sw, self->stretch_sh, &scaled,
                                                       self->sel_lift_max))
    return;
  easel_path_free (&self->sel_path);
  self->sel_path = scaled;
  self->sel_path_x = self->stretch_sx;
  self->sel_path_y = self->stretch_sy;
}

/* A stretch drag that is cancelled or fails ends with the selection as at its press (REQ-0015-08, REQ-0015-09): marked again if the drag
   lifted it, else floating with the copy of this drag gone and its pixels made afresh at the size of the press; the rectangle and the
   free-form line are those of the press; the held button is waited for. */
static void
restore_stretch (EaselCanvas *self)
{
  if (self->sel_state == 2 && self->sel_lifted_here)
    {
      drop_float (self);
      self->sel_state = 1;                        /* marked again at the size it had; the picture was never touched */
      g_signal_emit (self, signals[SIGNAL_PENDING_CHANGED], 0);
    }
  else if (self->sel_state == 2)
    {
      cut_copies (self, self->stretch_stamps0);
      /* Pixels already at the size of the press (the copy failed before any change) are as they were: nothing to make. */
      if ((self->sel_float.w != self->stretch_sw || self->sel_float.h != self->stretch_sh)
          && easel_selection_stretch (&self->sel_float, self->stretch_sx, self->stretch_sy, self->stretch_sw, self->stretch_sh,
                                      self->sel_lift_max) != EASEL_SELECTION_DONE)
        g_signal_emit (self, signals[SIGNAL_SELECTION_FAILED], 0, 3);
    }
  self->sel_x = self->stretch_sx;
  self->sel_y = self->stretch_sy;
  self->sel_w = self->stretch_sw;
  self->sel_h = self->stretch_sh;
  stretch_restore_path (self);
  self->stretching = FALSE;
  self->cancelled_button = self->sel_button;
}

/* The other button, Esc and Undo during a drag of Select cancel that drag only (REQ-0013-18): a rectangle leaves no
   selection; a move puts the selection back where it was before the drag, undoing a lift the drag made and taking away a
   copy it left. The held button is waited for until its release. FALSE if no such drag is held. */
static gboolean
cancel_selection_drag (EaselCanvas *self)
{
  if (self->selecting)
    {
      self->selecting = FALSE;
      self->sel_state = 0;
      self->cancelled_button = self->sel_button;
    }
  else if (self->lassoing)
    {
      self->lassoing = FALSE;                     /* no selection is made; a put-down made by the press stays (REQ-0014-14) */
      lasso_free (self);
      self->cancelled_button = self->sel_button;
    }
  else if (self->stretching)
    restore_stretch (self);
  else if (self->sel_moving)
    {
      if (self->sel_lifted_here)
        {
          drop_float (self);
          self->sel_state = 1;                    /* marked again; the picture was never touched */
          g_signal_emit (self, signals[SIGNAL_PENDING_CHANGED], 0);
        }
      else if (self->sel_smear)
        cut_copies (self, self->sel_stamps0);
      else if (self->sel_stamped_here && self->sel_float.n_stamps > 0)
        cut_copies (self, self->sel_float.n_stamps - 1);
      self->sel_x = self->sel_move_x0;
      self->sel_y = self->sel_move_y0;
      if (self->sel_state == 2)
        {
          self->sel_float.x = self->sel_x;
          self->sel_float.y = self->sel_y;
        }
      self->sel_moving = FALSE;
      self->cancelled_button = self->sel_button;
    }
  else
    return FALSE;
  g_signal_emit (self, signals[SIGNAL_STROKING_CHANGED], 0, FALSE);
  g_signal_emit (self, signals[SIGNAL_SELECTION_CHANGED], 0);
  gtk_widget_queue_draw (GTK_WIDGET (self));
  return TRUE;
}

/* The one function that forgets a floating selection (Undo while it floats, REQ-0013-12): the picture was never touched, so
   every pixel is as before the lift; the selection is removed and nothing is added. */
static void
cancel_float (EaselCanvas *self)
{
  if (self->sel_state != 2)
    return;
  drop_float (self);
  shape_free (self);
  self->sel_state = 0;
  self->hover_sel = FALSE;
  self->hover_sel_handle = -1;
  update_cursor (self);
  g_signal_emit (self, signals[SIGNAL_PENDING_CHANGED], 0);
  g_signal_emit (self, signals[SIGNAL_SELECTION_CHANGED], 0);
  gtk_widget_queue_draw (GTK_WIDGET (self));
}

void
easel_canvas_select_all (EaselCanvas *self)
{
  if (self->img == NULL)
    return;
  finish_selection (self, FALSE);
  shape_free (self);
  self->sel_state = 1;
  self->sel_x = self->sel_y = 0;
  self->sel_w = self->img->width;
  self->sel_h = self->img->height;
  g_signal_emit (self, signals[SIGNAL_SELECTION_CHANGED], 0);
  gtk_widget_queue_draw (GTK_WIDGET (self));
}

gboolean
easel_canvas_clear_selection (EaselCanvas *self)
{
  EaselSelectionResult r;

  if (self->sel_state == 0 || easel_canvas_is_stroking (self))
    return FALSE;
  /* A marked selection's area takes the background color now; a floating one is put down without its pixels, so its hole and
     its copies stay (REQ-0013-14). The memory dialog: 0013 T017. */
  if (self->sel_state == 1)
    r = easel_selection_clear (self->img, self->history, self->sel_x, self->sel_y, self->sel_w, self->sel_h, self->sel_mask,
                               hole_color (self), self->sel_step_max);
  else
    r = easel_selection_put_down (&self->sel_float, FALSE, self->img, self->history, self->sel_step_max);
  if (r == EASEL_SELECTION_NO_MEMORY)
    {
      g_signal_emit (self, signals[SIGNAL_SELECTION_FAILED], 0, 1);          /* nothing changed, the selection stays */
      return TRUE;
    }
  drop_float (self);
  shape_free (self);
  self->sel_state = 0;
  self->hover_sel = FALSE;
  self->hover_sel_handle = -1;
  update_cursor (self);
  if (r == EASEL_SELECTION_DONE)
    easel_canvas_update (self);
  g_signal_emit (self, signals[SIGNAL_PENDING_CHANGED], 0);
  g_signal_emit (self, signals[SIGNAL_SELECTION_CHANGED], 0);
  gtk_widget_queue_draw (GTK_WIDGET (self));
  return TRUE;
}

gboolean
easel_canvas_finish_selection (EaselCanvas *self)
{
  if (self->sel_state == 0 || easel_canvas_is_stroking (self))
    return FALSE;
  finish_selection (self, FALSE);
  return TRUE;
}

void
easel_canvas_deselect (EaselCanvas *self)
{
  if (self->sel_state == 1)
    finish_selection (self, FALSE);
}

/* The rectangle dragged out ends (a release, a lost release, a lost focus): it is marked, or none for a click or a rectangle
   with nothing inside the picture (REQ-0013-04). */
/* A press on the selection: a move, lifted at the first movement to another pixel (REQ-0013-07, REQ-0014-08). */
static void
begin_move (EaselCanvas *self, guint button, int px, int py)
{
  self->sel_moving = TRUE;
  self->sel_button = button;
  self->sel_px0 = self->sel_px1 = px;
  self->sel_py0 = self->sel_py1 = py;
  self->sel_move_x0 = self->sel_x;
  self->sel_move_y0 = self->sel_y;
  self->sel_copy = self->ctrl;
  self->sel_moved = FALSE;
  self->sel_lifted_here = FALSE;
  self->sel_stamped_here = FALSE;
  self->sel_smear = self->shift && !self->ctrl;
  self->sel_stamps0 = self->sel_state == 2 ? self->sel_float.n_stamps : 0;
  g_signal_emit (self, signals[SIGNAL_STROKING_CHANGED], 0, TRUE);
}

/* A primary press on a handle of the selection: a stretch drag (REQ-0015-03); nothing changes until the pointer reaches another size. */
static void
begin_stretch (EaselCanvas *self, int handle, double x, double y)
{
  self->stretching = TRUE;
  self->sel_button = GDK_BUTTON_PRIMARY;
  self->stretch_handle = handle;
  self->stretch_x0 = self->stretch_lx = x / logical_per_pixel (self);
  self->stretch_y0 = self->stretch_ly = y / logical_per_pixel (self);
  self->stretch_sx = self->sel_x;
  self->stretch_sy = self->sel_y;
  self->stretch_sw = self->sel_w;
  self->stretch_sh = self->sel_h;
  self->sel_copy = self->ctrl;
  self->sel_lifted_here = FALSE;
  self->stretch_copied = FALSE;
  self->stretch_stamps0 = self->sel_state == 2 ? self->sel_float.n_stamps : 0;
  g_signal_emit (self, signals[SIGNAL_STROKING_CHANGED], 0, TRUE);
}

/* The stretch drag ends because the memory failed (REQ-0015-09): the dialog, the selection as at the press, the release does nothing. */
static void
end_failed_stretch (EaselCanvas *self, int kind)
{
  g_signal_emit (self, signals[SIGNAL_SELECTION_FAILED], 0, kind);
  restore_stretch (self);
  g_signal_emit (self, signals[SIGNAL_STROKING_CHANGED], 0, FALSE);
  g_signal_emit (self, signals[SIGNAL_SELECTION_CHANGED], 0);
  gtk_widget_queue_draw (GTK_WIDGET (self));
}

/* The selection as the pointer now has it dragged by the handle held (REQ-0015-03): the rectangle from the pointer's movement since the
   press, rounded to whole picture pixels, a marked selection lifted at the first movement to another size (REQ-0015-04), the float made
   afresh from the pixels as lifted (REQ-0015-05). */
static void
stretch_apply (EaselCanvas *self)
{
  int dx = (int) lround (self->stretch_lx - self->stretch_x0), dy = (int) lround (self->stretch_ly - self->stretch_y0);
  int nx, ny, nw, nh;

  easel_stretch_rect ((EaselSelHandle) self->stretch_handle, self->stretch_sx, self->stretch_sy, self->stretch_sw, self->stretch_sh, dx, dy,
                      self->shift, &nx, &ny, &nw, &nh);
  if (nx == self->sel_x && ny == self->sel_y && nw == self->sel_w && nh == self->sel_h)
    return;                                       /* no movement to another size */
  if (self->sel_state == 1)
    {
      if (easel_selection_lift (&self->sel_float, self->img, self->sel_x, self->sel_y, self->sel_w, self->sel_h, self->sel_mask,
                                self->sel_copy, hole_color (self), self->sel_lift_max) != EASEL_SELECTION_DONE)
        {
          end_failed_stretch (self, 0);
          return;
        }
      self->sel_state = 2;
      self->sel_lifted_here = TRUE;
      g_signal_emit (self, signals[SIGNAL_PENDING_CHANGED], 0);
    }
  else if (self->sel_copy && !self->stretch_copied)
    {
      /* Ctrl at the press of a floating selection: a copy stays where it is, with the size and pixels it has (REQ-0015-04). */
      if (!easel_selection_stamp (&self->sel_float, self->sel_lift_max))
        {
          end_failed_stretch (self, 3);
          return;
        }
      self->stretch_copied = TRUE;
    }
  /* At 100 % and above only the part in view is made while the button is held (the memory of the whole image is had now, so a failure
     shows at the first movement, REQ-0015-09); the whole is made at the release. Below 100 % the whole view is the picture: all is made. */
  if ((self->zoom >= 100 ? easel_selection_stretch_lazy (&self->sel_float, nx, ny, nw, nh, self->sel_lift_max)
                         : easel_selection_stretch (&self->sel_float, nx, ny, nw, nh, self->sel_lift_max)) != EASEL_SELECTION_DONE)
    {
      end_failed_stretch (self, 3);
      return;
    }
  if (self->sel_path.n > 0 || self->sel_path0.n > 0)
    {
      /* A free-form selection: its line is the original path scaled with the shape, the corners keeping their place (REQ-0015-06). */
      EaselPath scaled = { 0 };

      if (self->sel_path0.n == 0)
        {
          self->sel_path0 = self->sel_path;
          self->sel_path = (EaselPath) { 0 };
          self->sel_p0_x = self->sel_path_x;
          self->sel_p0_y = self->sel_path_y;
          self->sel_p0_w = self->sel_w;
          self->sel_p0_h = self->sel_h;
        }
      if (!easel_lasso_scale_path (&self->sel_path0, self->sel_p0_x, self->sel_p0_y, self->sel_p0_w, self->sel_p0_h, nx, ny, nw, nh,
                                   &scaled, self->sel_lift_max))
        {
          end_failed_stretch (self, 3);
          return;
        }
      easel_path_free (&self->sel_path);
      self->sel_path = scaled;
      self->sel_path_x = nx;
      self->sel_path_y = ny;
    }
  self->sel_x = nx;
  self->sel_y = ny;
  self->sel_w = nw;
  self->sel_h = nh;
  g_signal_emit (self, signals[SIGNAL_SELECTION_CHANGED], 0);
  gtk_widget_queue_draw (GTK_WIDGET (self));
}

static void
end_select (EaselCanvas *self)
{
  int x, y, w, h;
  gboolean click = self->sel_px1 == self->sel_px0 && self->sel_py1 == self->sel_py0;

  self->selecting = FALSE;
  shape_free (self);
  if (!click && drag_rect (self, &x, &y, &w, &h))
    {
      self->sel_state = 1;
      self->sel_x = x;
      self->sel_y = y;
      self->sel_w = w;
      self->sel_h = h;
    }
  else
    self->sel_state = 0;
  g_signal_emit (self, signals[SIGNAL_STROKING_CHANGED], 0, FALSE);
  g_signal_emit (self, signals[SIGNAL_SELECTION_CHANGED], 0);
  gtk_widget_queue_draw (GTK_WIDGET (self));
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
/* The handle h of the selection's bounding rectangle in canvas logical pixels (REQ-0015-01): a corner handle has its inner corner at
   the rectangle's corner, outside it; an edge handle touches the middle of its edge from outside, centered, its position rounded
   down to a whole logical pixel. FALSE if there is no selection. Not limited by the guides: a press hits them hidden or not. */
gboolean
easel_canvas_get_selection_handle_rect (EaselCanvas *self, EaselSelHandle handle, graphene_rect_t *rect)
{
  double l = logical_per_pixel (self), rx, ry, rw, rh, x, y;
  int sx, sy, sw, sh;
  gboolean floating;

  if (self->img == NULL || !easel_canvas_get_selection (self, &sx, &sy, &sw, &sh, &floating))
    return FALSE;
  rx = sx * l;
  ry = sy * l;
  rw = sw * l;
  rh = sh * l;
  switch (handle)
    {
    case EASEL_SEL_HANDLE_TL: case EASEL_SEL_HANDLE_L: case EASEL_SEL_HANDLE_BL:
      x = rx - HANDLE_SIZE;
      break;
    case EASEL_SEL_HANDLE_TR: case EASEL_SEL_HANDLE_R: case EASEL_SEL_HANDLE_BR:
      x = rx + rw;
      break;
    default:
      x = floor (rx + rw / 2 - HANDLE_SIZE / 2);
    }
  switch (handle)
    {
    case EASEL_SEL_HANDLE_TL: case EASEL_SEL_HANDLE_T: case EASEL_SEL_HANDLE_TR:
      y = ry - HANDLE_SIZE;
      break;
    case EASEL_SEL_HANDLE_BL: case EASEL_SEL_HANDLE_B: case EASEL_SEL_HANDLE_BR:
      y = ry + rh;
      break;
    default:
      y = floor (ry + rh / 2 - HANDLE_SIZE / 2);
    }
  *rect = GRAPHENE_RECT_INIT ((float) x, (float) y, HANDLE_SIZE, HANDLE_SIZE);
  return TRUE;
}

/* The handle of the selection under the canvas point (x, y), as an EaselSelHandle, or -1 (REQ-0015-02): first the handle whose own 5 x 5
   square holds the point, then the handle whose 9 x 9 area round its center does, the area less the selection's bounding rectangle;
   in each, a corner handle before an edge handle. */
int
easel_canvas_selection_handle_at (EaselCanvas *self, double x, double y)
{
  static const EaselSelHandle order[] = { EASEL_SEL_HANDLE_TL, EASEL_SEL_HANDLE_TR, EASEL_SEL_HANDLE_BL, EASEL_SEL_HANDLE_BR,
                                          EASEL_SEL_HANDLE_T, EASEL_SEL_HANDLE_L, EASEL_SEL_HANDLE_R, EASEL_SEL_HANDLE_B };
  graphene_rect_t r;
  double l = logical_per_pixel (self);
  int sx, sy, sw, sh;
  gboolean floating;

  if (self->img == NULL || !easel_tool_selects (self->tool) || !easel_canvas_get_selection (self, &sx, &sy, &sw, &sh, &floating))
    return -1;
  for (guint i = 0; i < G_N_ELEMENTS (order); i++)
    if (easel_canvas_get_selection_handle_rect (self, order[i], &r) && x >= r.origin.x && x < r.origin.x + r.size.width
        && y >= r.origin.y && y < r.origin.y + r.size.height)
      return order[i];
  /* A selection handle that lies on a picture's handle (the selection reaches the picture's edge there) owns that handle's whole hit
     area, also the part inside the picture and the sliver its whole-pixel place leaves out: the picture is resized once there is no
     selection (REQ-0015-02). */
  for (guint k = 0; k < 3; k++)
    {
      static const EaselHandle pic[] = { EASEL_HANDLE_CORNER, EASEL_HANDLE_RIGHT, EASEL_HANDLE_BOTTOM };
      static const EaselSelHandle sel[] = { EASEL_SEL_HANDLE_BR, EASEL_SEL_HANDLE_R, EASEL_SEL_HANDLE_B };
      graphene_rect_t pr;

      if (easel_canvas_get_handle_rect (self, pic[k], &pr) && easel_canvas_get_selection_handle_rect (self, sel[k], &r)
          && fabs (x - (pr.origin.x + pr.size.width / 2)) <= HANDLE_HIT / 2 && fabs (y - (pr.origin.y + pr.size.height / 2)) <= HANDLE_HIT / 2
          && fabs (r.origin.x - pr.origin.x) <= 1 && fabs (r.origin.y - pr.origin.y) <= 1)
        return sel[k];
    }
  if (x >= sx * l && x < (sx + sw) * l && y >= sy * l && y < (sy + sh) * l)
    return -1;                                    /* inside the rectangle a press is the selection's or the tool's */
  for (guint i = 0; i < G_N_ELEMENTS (order); i++)
    if (easel_canvas_get_selection_handle_rect (self, order[i], &r)
        && fabs (x - (r.origin.x + r.size.width / 2)) <= HANDLE_HIT / 2 && fabs (y - (r.origin.y + r.size.height / 2)) <= HANDLE_HIT / 2)
      return order[i];
  return -1;
}

/* The cursor of a selection handle's direction. */
static const char *
selection_handle_cursor (int handle)
{
  switch (handle)
    {
    case EASEL_SEL_HANDLE_L: case EASEL_SEL_HANDLE_R:
      return "ew-resize";
    case EASEL_SEL_HANDLE_T: case EASEL_SEL_HANDLE_B:
      return "ns-resize";
    case EASEL_SEL_HANDLE_TR: case EASEL_SEL_HANDLE_BL:
      return "nesw-resize";
    default:
      return "nwse-resize";
    }
}

/* The eight handles of the selection (REQ-0015-01), over its line, the edge ones first so that the corner ones lie over them; none
   while the guides are hidden. */
static void
append_selection_handles (EaselCanvas *self, GtkSnapshot *snapshot)
{
  static const EaselSelHandle order[] = { EASEL_SEL_HANDLE_T, EASEL_SEL_HANDLE_L, EASEL_SEL_HANDLE_R, EASEL_SEL_HANDLE_B,
                                          EASEL_SEL_HANDLE_TL, EASEL_SEL_HANDLE_TR, EASEL_SEL_HANDLE_BL, EASEL_SEL_HANDLE_BR };
  static const GdkRGBA navy = { 0.0f, 0.0f, 0.5f, 1.0f };
  graphene_rect_t r;

  if (self->guides_hidden || self->no_sel_handles || !easel_tool_selects (self->tool))
    return;
  for (guint i = 0; i < G_N_ELEMENTS (order); i++)
    if (easel_canvas_get_selection_handle_rect (self, order[i], &r))
      gtk_snapshot_append_color (snapshot, &navy, &r);
}

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

gboolean
easel_canvas_move_selection (EaselCanvas *self, int dx, int dy)
{
  if (self->sel_state == 0 || easel_canvas_is_stroking (self))
    return FALSE;
  if (self->sel_state == 1)
    {
      /* The first key lifts it as a move does (REQ-0013-10, REQ-0013-08). The memory dialog: 0013 T017. */
      if (easel_selection_lift (&self->sel_float, self->img, self->sel_x, self->sel_y, self->sel_w, self->sel_h, self->sel_mask,
                                FALSE, hole_color (self), self->sel_lift_max) != EASEL_SELECTION_DONE)
        {
          g_signal_emit (self, signals[SIGNAL_SELECTION_FAILED], 0, 0);
          return TRUE;
        }
      self->sel_state = 2;
      g_signal_emit (self, signals[SIGNAL_PENDING_CHANGED], 0);
    }
  self->sel_x = self->sel_float.x = self->sel_x + dx;
  self->sel_y = self->sel_float.y = self->sel_y + dy;
  g_signal_emit (self, signals[SIGNAL_SELECTION_CHANGED], 0);
  gtk_widget_queue_draw (GTK_WIDGET (self));
  return TRUE;
}

void
easel_canvas_set_ctrl (EaselCanvas *self, gboolean down)
{
  self->ctrl = down;
}

void
easel_canvas_set_shift (EaselCanvas *self, gboolean down)
{
  self->shift = down;
  if (self->stretching)
    stretch_apply (self);                       /* the ratio follows Shift at once (REQ-0015-07) */
  if (self->selecting)
    {
      g_signal_emit (self, signals[SIGNAL_SELECTION_CHANGED], 0);
      gtk_widget_queue_draw (GTK_WIDGET (self));
    }
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

  if (self->pending && self->pend_gradient)
    return;                                     /* the gradient replaces the tiles instead (gradient_preview) */
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

/* The pending gradient as it is shown and would be applied now, in the color of its button now (REQ-0012-04). */
static EaselGradient
shown_gradient (EaselCanvas *self)
{
  const int *p = self->pend_pts;

  return (EaselGradient) { p[0], p[1], p[2], p[3],
                           self->grad_button == GDK_BUTTON_PRIMARY ? self->foreground : self->background };
}

/* A pending gradient as it will be applied (REQ-0012-07): each tile the view shows, laid over by the core function the apply
   uses, as a texture of the tile's size and format, drawn in its place with the same rectangle and filter; so the same picture
   pixel lands on each device pixel before and after Enter, at every zoom. Holds, until the next frame, 4 bytes per pixel of
   the tiles in view. NULL if not a gradient, or if the memory cannot be had: then pend_unshown is set and the picture is drawn
   as it is. */
/* The float's tile i over the trail dropped, and its memory no longer counted. */
static void
clear_comp (EaselCanvas *self, int i)
{
  if (self->comp_tex[i] == NULL)
    return;
  self->comp_bytes -= (gsize) gdk_texture_get_width (self->comp_tex[i]) * (gsize) gdk_texture_get_height (self->comp_tex[i]) * 4;
  g_clear_object (&self->comp_tex[i]);
}

static void
free_trail (EaselCanvas *self)
{
  for (int i = 0; self->trail_data != NULL && i < self->tile_cols * self->tile_rows; i++)
    {
      clear_comp (self, i);
      g_clear_object (&self->trail_tex[i]);
      g_clear_pointer (&self->trail_data[i], g_bytes_unref);
    }
  g_clear_pointer (&self->trail_data, g_free);
  g_clear_pointer (&self->trail_n, g_free);
  g_clear_pointer (&self->trail_tex, g_free);
  g_clear_pointer (&self->comp_tex, g_free);
  g_clear_pointer (&self->sel_tiles, g_free);
  self->trail_key = 0;
  self->trail_bytes = 0;
  self->comp_bytes = 0;
}

/* The floating selection is freed (put down, cancelled, removed, or lifted afresh): its trail with it. */
static void
drop_float (EaselCanvas *self)
{
  free_trail (self);
  easel_float_free (&self->sel_float);
}

/* The copies are cut back to n (a cancelled or failed smear, a cancelled Ctrl copy): the trail laid with the later ones is stale, even
   if as many copies come again before the next frame. */
static void
cut_copies (EaselCanvas *self, size_t n)
{
  self->sel_float.n_stamps = n;
  self->sel_epoch++;
}

/* What the trail is laid from, apart from the copies, which only grow between two cut-backs: the picture's tiles, the hole, the float's
   pixels and shape, and the epoch. */
static guint64
trail_key_of (EaselCanvas *self)
{
  const EaselFloat *f = &self->sel_float;
  guint64 h = G_GUINT64_CONSTANT (1469598103934665603);
  guint64 v[] = { self->tiles_gen, (guint64) (guintptr) self->img, f->hole, (guint64) (gint64) f->hx, (guint64) (gint64) f->hy,
                  f->hole_color, (guint64) (guintptr) f->px, (guint64) (guintptr) f->mask, (guint64) (gint64) f->w,
                  (guint64) (gint64) f->h, self->sel_epoch };

  for (gsize i = 0; i < G_N_ELEMENTS (v); i++)
    h = (h ^ v[i]) * G_GUINT64_CONSTANT (1099511628211);
  return h | 1;                                   /* never 0, which means "none" */
}

/* Does the float's rectangle at (px, py) meet the tile's? */
static gboolean
meets (const EaselFloat *f, int px, int py, int x0, int y0, int w, int h)
{
  return px < x0 + w && py < y0 + h && px + f->w > x0 && py + f->h > y0;
}

static GdkTexture *
texture_of (EaselCanvas *self, GBytes *data, int w, int h)
{
  gdk_memory_texture_builder_set_bytes (self->builder, data);
  gdk_memory_texture_builder_set_width (self->builder, w);
  gdk_memory_texture_builder_set_height (self->builder, h);
  gdk_memory_texture_builder_set_stride (self->builder, (gsize) w * sizeof (uint32_t));
  gdk_memory_texture_builder_set_format (self->builder, PICTURE_FORMAT);
  return gdk_memory_texture_builder_build (self->builder);
}

/* The floating selection as it will be put down, for the tiles in view it can change (the rest are drawn as they are), in two layers kept
   between frames (0014 T023): the trail, the picture with the hole and the copies, laid once per tile and then one new copy at a time
   into its own rectangle; and, for the tiles under the float, the float laid over a copy of the trail tile, laid again only when the
   float moves or the trail under it grows. So a frame costs the new copies and the tiles under the float, however long the trail is,
   and a redraw with nothing changed costs nothing. The result is the core lay's (easel_selection_lay_range in parts adds up to it).
   The array is the canvas's own. NULL if nothing differs or the memory cannot be had (then the picture is drawn without it). */
static GdkTexture **
selection_preview (EaselCanvas *self)
{
  const EaselFloat *f = &self->sel_float;
  int n = self->tile_cols * self->tile_rows, bx0, by0, bx1, by1;
  size_t copies = f->n_stamps;
  guint64 key;
  graphene_rect_t seen;
  double l = self->zoom / (100.0 * easel_canvas_get_scale (self));

  if (self->sel_state != 2 || !easel_selection_bounds (f, TRUE, self->img, &bx0, &by0, &bx1, &by1)
      || !gtk_widget_compute_bounds (gtk_widget_get_parent (GTK_WIDGET (self)), GTK_WIDGET (self), &seen))
    {
      free_trail (self);
      return NULL;
    }
  key = trail_key_of (self);
  if (key != self->trail_key)
    free_trail (self);
  if (self->trail_data == NULL)
    {
      self->trail_data = g_try_new0 (GBytes *, n);
      self->trail_n = g_try_new0 (size_t, n);
      self->trail_tex = g_try_new0 (GdkTexture *, n);
      self->comp_tex = g_try_new0 (GdkTexture *, n);
      self->sel_tiles = g_try_new0 (GdkTexture *, n);
      if (self->trail_data == NULL || self->trail_n == NULL || self->trail_tex == NULL || self->comp_tex == NULL
          || self->sel_tiles == NULL)
        {
          free_trail (self);
          return NULL;
        }
      self->trail_key = key;
      self->comp_x = f->x;
      self->comp_y = f->y;
    }
  if (self->comp_x != f->x || self->comp_y != f->y)
    {
      for (int i = 0; i < n; i++)
        clear_comp (self, i);
      self->comp_x = f->x;
      self->comp_y = f->y;
    }
  for (int i = 0; i < n; i++)
    {
      int tx = i % self->tile_cols, ty = i / self->tile_cols;
      int x0 = tx * EASEL_CANVAS_TILE, y0 = ty * EASEL_CANVAS_TILE;
      int w = MIN (EASEL_CANVAS_TILE, self->img->width - x0), h = MIN (EASEL_CANVAS_TILE, self->img->height - y0);
      graphene_rect_t r = GRAPHENE_RECT_INIT ((float) (x0 * l), (float) (y0 * l), (float) (w * l), (float) (h * l));
      size_t bytes = (size_t) w * (size_t) h * sizeof (uint32_t);
      gboolean laid = FALSE;

      self->sel_tiles[i] = NULL;
      if (x0 > bx1 || y0 > by1 || x0 + w - 1 < bx0 || y0 + h - 1 < by0 || !graphene_rect_intersection (&r, &seen, NULL))
        continue;
      if (self->trail_data[i] != NULL && self->trail_n[i] > copies)
        {
          clear_comp (self, i);                 /* fewer copies than laid, with no cut-back seen: lay it afresh */
          g_clear_object (&self->trail_tex[i]);
          g_clear_pointer (&self->trail_data[i], g_bytes_unref);
          self->trail_bytes -= bytes;
        }
      if (self->trail_data[i] == NULL)
        {
          uint32_t *px = self->trail_bytes + self->comp_bytes + bytes <= self->preview_max_bytes ? g_try_malloc (bytes) : NULL;

          if (px == NULL)
            {
              free_trail (self);
              return NULL;
            }
          easel_selection_lay_range (f, 0, copies, FALSE, self->img, x0, y0, w, h, px, (size_t) w, TRUE);
          self->trail_data[i] = g_bytes_new_take (px, bytes);
          self->trail_n[i] = copies;
          self->trail_bytes += bytes;
          self->trail_tiles_laid++;
          laid = TRUE;
        }
      else if (self->trail_n[i] < copies)
        {
          uint32_t *px = (uint32_t *) g_bytes_get_data (self->trail_data[i], NULL);

          for (size_t k = self->trail_n[i]; k < copies; k++)
            if (meets (f, f->stamps[k].x, f->stamps[k].y, x0, y0, w, h))
              {
                easel_selection_lay_range (f, k, k + 1, FALSE, self->img, x0, y0, w, h, px, (size_t) w, FALSE);
                self->trail_copies_laid++;
                laid = TRUE;
              }
          self->trail_n[i] = copies;
        }
      if (laid || self->trail_tex[i] == NULL)
        {
          g_clear_object (&self->trail_tex[i]);
          clear_comp (self, i);
          self->trail_tex[i] = texture_of (self, self->trail_data[i], w, h);
          self->trail_textures++;
        }
      if (!meets (f, f->x, f->y, x0, y0, w, h))
        {
          clear_comp (self, i);
          self->sel_tiles[i] = self->trail_tex[i];
          continue;
        }
      if (self->comp_tex[i] == NULL)
        {
          uint32_t *px = self->trail_bytes + self->comp_bytes + bytes <= self->preview_max_bytes ? g_try_malloc (bytes) : NULL;
          g_autoptr (GBytes) data = NULL;

          if (px == NULL)
            {
              free_trail (self);
              return NULL;
            }
          memcpy (px, g_bytes_get_data (self->trail_data[i], NULL), bytes);
          easel_selection_fill (&self->sel_float, x0 - f->x, y0 - f->y, x0 + w - f->x, y0 + h - f->y);     /* only the tiles in view are made */
          easel_selection_lay_range (f, copies, copies, TRUE, self->img, x0, y0, w, h, px, (size_t) w, FALSE);
          data = g_bytes_new_take (px, bytes);
          self->comp_tex[i] = texture_of (self, data, w, h);
          self->comp_bytes += bytes;
          self->trail_floats_laid++;
        }
      self->sel_tiles[i] = self->comp_tex[i];
    }
  return self->sel_tiles;
}

void
easel_canvas_get_trail_counts (EaselCanvas *self, gsize *tiles, gsize *copies, gsize *floats, gsize *textures)
{
  *textures = self->trail_textures;
  *tiles = self->trail_tiles_laid;
  *copies = self->trail_copies_laid;
  *floats = self->trail_floats_laid;
}

gsize
easel_canvas_get_trail_bytes (EaselCanvas *self)
{
  return self->trail_bytes;
}

gsize
easel_canvas_get_preview_bytes_held (EaselCanvas *self)
{
  gsize held = 0;

  for (int i = 0; self->trail_data != NULL && i < self->tile_cols * self->tile_rows; i++)
    {
      if (self->trail_data[i] != NULL)
        held += g_bytes_get_size (self->trail_data[i]);
      if (self->comp_tex[i] != NULL)
        held += (gsize) gdk_texture_get_width (self->comp_tex[i]) * (gsize) gdk_texture_get_height (self->comp_tex[i]) * 4;
    }
  return held;
}

void
easel_canvas_set_overlay_probe (EaselCanvas *self, gboolean line, gboolean tint)
{
  self->probe_no_line = !line;
  self->probe_no_tint = !tint;
  gtk_widget_queue_draw (GTK_WIDGET (self));
}

void
easel_canvas_set_tint_bands (EaselCanvas *self, gboolean bands)
{
  self->tint_rows = !bands;
  gtk_widget_queue_draw (GTK_WIDGET (self));
}

gsize
easel_canvas_get_tint_rects (EaselCanvas *self)
{
  return self->tint_rects;
}

void
easel_canvas_set_selection_handles_shown (EaselCanvas *self, gboolean shown)
{
  self->no_sel_handles = !shown;
  gtk_widget_queue_draw (GTK_WIDGET (self));
}

void
easel_canvas_set_line_merge (EaselCanvas *self, gboolean merge)
{
  self->line_unmerged = !merge;
  gtk_widget_queue_draw (GTK_WIDGET (self));
}

gsize
easel_canvas_get_line_strips (EaselCanvas *self)
{
  return self->line_strips;
}

void
easel_canvas_drop_preview (EaselCanvas *self)
{
  free_trail (self);
  gtk_widget_queue_draw (GTK_WIDGET (self));
}

void
easel_canvas_get_stretch_made (EaselCanvas *self, gsize *made, gsize *total)
{
  *made = self->sel_state == 2 ? self->sel_float.made_px : 0;
  *total = self->sel_state == 2 ? (gsize) self->sel_float.w * (gsize) self->sel_float.h : 0;
}

gsize
easel_canvas_get_selection_copies (EaselCanvas *self)
{
  return self->sel_state == 2 ? self->sel_float.n_stamps : 0;
}

static GdkTexture **
gradient_preview (EaselCanvas *self, double l)
{
  EaselGradient g = shown_gradient (self);
  gboolean gradient = self->pending && self->pend_gradient;
  int n = self->tile_cols * self->tile_rows;
  GdkTexture **textures;
  graphene_rect_t seen;
  size_t total = 0;

  if (!gradient)
    return NULL;
  if (!gtk_widget_compute_bounds (gtk_widget_get_parent (GTK_WIDGET (self)), GTK_WIDGET (self), &seen))
    return NULL;
  textures = g_try_new0 (GdkTexture *, n);
  for (int i = 0; textures != NULL && i < n; i++)
    {
      int tx = i % self->tile_cols, ty = i / self->tile_cols;
      int x0 = tx * EASEL_CANVAS_TILE, y0 = ty * EASEL_CANVAS_TILE;
      int w = MIN (EASEL_CANVAS_TILE, self->img->width - x0), h = MIN (EASEL_CANVAS_TILE, self->img->height - y0);
      graphene_rect_t r = GRAPHENE_RECT_INIT ((float) (x0 * l), (float) (y0 * l), (float) (w * l), (float) (h * l));
      size_t bytes = (size_t) w * (size_t) h * sizeof (uint32_t);
      uint32_t *px = NULL;
      g_autoptr (GBytes) data = NULL;

      if (!graphene_rect_intersection (&r, &seen, NULL))
        continue;
      total += bytes;
      if (total <= self->preview_max_bytes)
        px = g_try_malloc (bytes);
      if (px == NULL)
        {
          for (int k = 0; k < n; k++)
            g_clear_object (&textures[k]);
          g_clear_pointer (&textures, g_free);
          break;
        }
      easel_gradient_lay (&g, self->img, x0, y0, w, h, px, (size_t) w);
      data = g_bytes_new_take (px, bytes);
      gdk_memory_texture_builder_set_bytes (self->builder, data);
      gdk_memory_texture_builder_set_width (self->builder, w);
      gdk_memory_texture_builder_set_height (self->builder, h);
      gdk_memory_texture_builder_set_stride (self->builder, (gsize) w * sizeof (uint32_t));
      gdk_memory_texture_builder_set_format (self->builder, PICTURE_FORMAT);
      textures[i] = gdk_memory_texture_builder_build (self->builder);
    }
  self->pend_unshown = textures == NULL;
  return textures;
}

static void
easel_canvas_snapshot (GtkWidget *widget, GtkSnapshot *snapshot)
{
  EaselCanvas *self = EASEL_CANVAS (widget);
  double l;
  graphene_rect_t bounds;
  GdkTexture **preview;

  if (self->tiles == NULL)
    return;
  l = self->zoom / (100.0 * easel_canvas_get_scale (self));
  preview = self->sel_state == 2 ? selection_preview (self) : gradient_preview (self, l);
  if (self->sel_state != 2)
    free_trail (self);
  bounds = GRAPHENE_RECT_INIT (0, 0, (float) (self->img->width * l), (float) (self->img->height * l));

  /* Transparent pixels show the checkerboard (REQ-0001-11); nearest filtering keeps pixels hard-edged
     at every zoom (REQ-0001-18). Each tile is drawn where its pixels are; edges fall on whole device pixels. */
  easel_snapshot_append_checkerboard (snapshot, &bounds, CHECKER_SIZE);
  for (int ty = 0; ty < self->tile_rows; ty++)
    for (int tx = 0; tx < self->tile_cols; tx++)
      {
        GdkTexture *tile = self->tiles[ty * self->tile_cols + tx];
        graphene_rect_t r;

        if (preview != NULL && preview[ty * self->tile_cols + tx] != NULL)
          tile = preview[ty * self->tile_cols + tx];
        r = GRAPHENE_RECT_INIT ((float) (tx * EASEL_CANVAS_TILE * l), (float) (ty * EASEL_CANVAS_TILE * l),
                                                (float) (gdk_texture_get_width (tile) * l),
                                                (float) (gdk_texture_get_height (tile) * l));

        gtk_snapshot_append_scaled_texture (snapshot, tile, GSK_SCALING_FILTER_NEAREST, &r);
      }
  if (preview != NULL && preview != self->sel_tiles)
    {
      for (int i = 0; i < self->tile_cols * self->tile_rows; i++)
        g_clear_object (&preview[i]);                /* the render node keeps its own references */
      g_free (preview);
    }
  append_shape_preview (self, snapshot);
  if (easel_canvas_grid_is_drawn (self))
    append_grid (self, snapshot);
  append_gradient_markers (self, snapshot);
  append_handles (self, snapshot);                /* the picture's handles lie under the selection's pixels and tint (REQ-0015-01) */
  append_float_outside (self, snapshot, l);
  if (!self->probe_no_tint)
    append_selection_tint (self, snapshot);
  if (!self->probe_no_line)
    append_selection_outline (self, snapshot);
  append_selection_handles (self, snapshot);
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
  else if (self->hover_sel_handle >= 0)
    cursor = gdk_cursor_new_from_name (selection_handle_cursor (self->hover_sel_handle), NULL);    /* REQ-0015-02: before the picture's */
  else if (handle != EASEL_HANDLE_NONE)
    cursor = gdk_cursor_new_from_name (handle == EASEL_HANDLE_RIGHT ? "ew-resize"
                                       : handle == EASEL_HANDLE_BOTTOM ? "ns-resize" : "nwse-resize", NULL);
  else if (self->hover_marker || self->hover_sel)
    cursor = gdk_cursor_new_from_name ("move", NULL);                 /* over a gradient's marker (REQ-0012-13) */
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
  else if (easel_tool_draws_shape (self->tool) || easel_tool_draws_pending (self->tool) || easel_tool_selects (self->tool))
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
  self->hover_sel_handle = -1;
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
easel_canvas_set_preview_memory_limit (EaselCanvas *self, size_t max_bytes)
{
  self->preview_max_bytes = max_bytes;
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
  if (self->pending && self->pend_gradient)
    gtk_widget_queue_draw (GTK_WIDGET (self));         /* a gradient follows the colors at once (REQ-0012-04) */
}

gboolean
easel_canvas_is_pending (EaselCanvas *self)
{
  /* A waiting curve, polygon or gradient, or a floating selection (REQ-0013-12): not put down yet. */
  return self->pending || self->sel_state == 2;
}

gboolean
easel_canvas_is_stroking (EaselCanvas *self)
{
  return self->stroke != NULL || self->picking || self->shaping || self->pend_held || self->selecting || self->sel_moving
         || self->lassoing || self->stretching;
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

  if (self->pend_gradient)
    {
      /* The end is the pointer's pixel, Shift constraining it as for Line from the start (REQ-0012-03); an end moved by its
         marker likewise, from the other end (REQ-0012-13). */
      int k = self->moving >= 0 ? self->moving : 1, o = 1 - k;

      if (self->shift)
        easel_figure_constrain (EASEL_FIGURE_LINE, self->pend_pts[2 * o], self->pend_pts[2 * o + 1], &x, &y);
      self->pend_pts[2 * k] = x;
      self->pend_pts[2 * k + 1] = y;
    }
  else if (self->pend_polygon)
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
  if (self->hover_marker)
    {
      self->hover_marker = FALSE;                     /* the markers go with the gradient */
      update_cursor (self);
    }
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
static EaselFigureResult
put_down_pending (EaselCanvas *self)
{
  EaselFigure f = pending_figure (self);
  EaselFigureResult r;
  gboolean was_held = self->pend_held;

  f.closed = self->pend_polygon;                 /* a polygon is closed when put down (REQ-0011-07) */

  gboolean gradient = self->pend_gradient;

  self->pend_held = FALSE;
  if (gradient)
    {
      /* The gradient is laid over the picture as one undo step, or not at all if it changes no pixel; without the memory,
         its own dialog (REQ-0012-09). */
      EaselGradient g = shown_gradient (self);
      EaselGradientResult gr = easel_gradient_apply_capped (self->img, self->history, &g, NULL, self->shape_max_bytes);

      r = gr == EASEL_GRADIENT_DONE ? EASEL_FIGURE_DONE
          : gr == EASEL_GRADIENT_NOTHING ? EASEL_FIGURE_NOTHING : EASEL_FIGURE_NO_MEMORY;
    }
  else
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
    g_signal_emit (self, signals[gradient ? SIGNAL_GRADIENT_FAILED : SIGNAL_SHAPE_FAILED], 0);
  gtk_widget_queue_draw (GTK_WIDGET (self));
  return r;
}

/* A stage's button goes up, or the stage is ended as last shown (a lost release, a lost focus, Save, Quit): the curve waits
   for its next stage, and the second bend puts it down (REQ-0011-04, -13). */
static void
end_pending_stage (EaselCanvas *self)
{
  self->pend_held = FALSE;
  g_signal_emit (self, signals[SIGNAL_STROKING_CHANGED], 0, FALSE);
  if (self->pend_gradient && self->moving >= 0)
    {
      /* A moved end ends as last shown; on the other end's pixel the line is put back (REQ-0012-13). */
      if (self->pend_pts[0] == self->pend_pts[2] && self->pend_pts[1] == self->pend_pts[3])
        memcpy (self->pend_pts, self->move_old, sizeof self->move_old);
      self->moving = -1;
      gtk_widget_queue_draw (GTK_WIDGET (self));
      return;
    }
  if (self->pend_gradient)
    {
      /* A drag that ends on its start pixel makes no gradient (REQ-0012-06); any other waits (REQ-0012-07), unless it could
         not be shown: then it is dropped with the dialog (REQ-0012-09). */
      if (self->pend_unshown)
        {
          set_pending (self, FALSE);
          g_signal_emit (self, signals[SIGNAL_GRADIENT_FAILED], 0);
        }
      else if (self->pend_pts[2] == self->pend_pts[0] && self->pend_pts[3] == self->pend_pts[1])
        set_pending (self, FALSE);
      gtk_widget_queue_draw (GTK_WIDGET (self));
      return;
    }
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

gboolean
easel_canvas_put_down_keeping_marked (EaselCanvas *self)
{
  gboolean ok = finish_selection (self, TRUE);

  if (self->pending)
    put_down_pending (self);
  return ok;
}

gboolean
easel_canvas_put_down (EaselCanvas *self)
{
  gboolean ok = finish_selection (self, FALSE);

  if (self->pending)
    put_down_pending (self);
  return ok;
}

gboolean
easel_canvas_enter (EaselCanvas *self)
{
  if (easel_canvas_finish_selection (self))
    return TRUE;                               /* Enter puts a selection down, or removes a marked one (REQ-0013-11) */
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

  if (cancel_selection_drag (self))
    return;                                    /* Esc and Undo during a drag of Select cancel the drag only */
  cancel_float (self);
  if (!self->pending)
    return;
  if (was_held && self->pend_gradient && self->moving >= 0)
    {
      /* Esc, Undo and the other button cancel the move of an end: the line is put back and the gradient keeps waiting
         (REQ-0012-13). */
      memcpy (self->pend_pts, self->move_old, sizeof self->move_old);
      self->moving = -1;
      self->pend_held = FALSE;
      self->cancelled_button = self->pend_button;
      g_signal_emit (self, signals[SIGNAL_STROKING_CHANGED], 0, FALSE);
      gtk_widget_queue_draw (GTK_WIDGET (self));
      return;
    }
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
  if (cancel_selection_drag (self))
    return;
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
  if (self->stroke != NULL || self->picking || self->shaping || self->pend_held || self->selecting || self->sel_moving
      || self->lassoing || self->stretching)
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
  if (self->selecting)
    {
      end_select (self);                       /* a lost release or focus: the rectangle as last shown (REQ-0013-18) */
      return;
    }
  if (self->lassoing)
    {
      end_lasso (self);                        /* the path closed and made as last shown (REQ-0014-14) */
      return;
    }
  if (self->stretching)
    {
      self->stretching = FALSE;                /* ... and the stretch as last shown (REQ-0015-08) */
      easel_selection_complete (&self->sel_float);
      g_signal_emit (self, signals[SIGNAL_STROKING_CHANGED], 0, FALSE);
      return;
    }
  if (self->sel_moving)
    {
      self->sel_moving = FALSE;                /* ... and the move as last shown */
      g_signal_emit (self, signals[SIGNAL_STROKING_CHANGED], 0, FALSE);
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

  if (self->selecting || self->sel_moving || self->lassoing || self->stretching)
    {
      /* The other button cancels the drag of Select, and only it (REQ-0013-18). */
      if (button != self->sel_button)
        cancel_selection_drag (self);
      return;
    }
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
  {
    int handle = easel_canvas_selection_handle_at (self, x, y);

    if (handle >= 0)
      {
        /* A press on a handle of the selection (REQ-0015-02, -03): it wins over the picture's handles and neither puts the selection
           down nor removes it; the primary button starts a stretch, the secondary one does nothing. */
        if (button == GDK_BUTTON_PRIMARY)
          begin_stretch (self, handle, x, y);
        return;
      }
  }
  if (self->pending && !self->pend_held && handles_shown (self) && easel_canvas_handle_at (self, x, y) != EASEL_HANDLE_NONE)
    {
      /* A press on a resize handle: a secondary one does nothing and keeps the shape (REQ-0004-16); a primary one puts it down
         and goes on as a press on the handle (REQ-0011-12). */
      if (button != GDK_BUTTON_PRIMARY)
        return;
      put_down_pending (self);
    }
  if (self->pending && self->pend_gradient && !self->pend_held && marker_at (self, x, y) >= 0)
    {
      /* A press on a marker moves that end; it applies nothing (REQ-0012-13; a handle was dealt with above, it wins). */
      self->moving = marker_at (self, x, y);
      memcpy (self->move_old, self->pend_pts, sizeof self->move_old);
      self->pend_held = TRUE;
      self->pend_button = button;
      to_pixel (self, x, y, &self->pend_px, &self->pend_py);
      g_signal_emit (self, signals[SIGNAL_STROKING_CHANGED], 0, TRUE);
      gtk_widget_queue_draw (GTK_WIDGET (self));
      return;
    }
  if (self->pending && self->pend_gradient && !self->pend_held)
    {
      /* A press of either button on a waiting gradient applies it as shown, then goes on as a first press of Gradient, a new
         drag with this button; if the apply fails (the dialog) the press starts nothing (REQ-0012-08). */
      if (put_down_pending (self) == EASEL_FIGURE_NO_MEMORY)
        return;
    }
  if (self->pending)
    {
      /* A curve waits or has a stage held: the other button drops it, the same button goes on to the next stage
         (REQ-0011-04, -11). A gradient's drag held: the other button drops it (REQ-0012-08). */
      if (button != self->pend_button)
        easel_canvas_drop (self);
      else if (self->pend_gradient)
        return;
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
          /* Only the primary button starts a resize; the other does nothing, and does not draw (REQ-0004-16). A primary
             press puts a floating selection down first (REQ-0013-11). */
          if (button == GDK_BUTTON_PRIMARY)
            {
              finish_selection (self, FALSE);
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
      self->pend_gradient = FALSE;
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
  if (self->tool == EASEL_TOOL_FREE_SELECT)
    {
      /* The secondary button on a pixel of the shape asks for the context menu and changes nothing; elsewhere it does nothing
         (REQ-0014-07, REQ-0013-17). */
      if (button != GDK_BUTTON_PRIMARY)
        {
          if (on_selection (self, x, y))
            g_signal_emit (self, signals[SIGNAL_CONTEXT_MENU], 0, x, y);
          return;
        }
      /* A press on the shape is a move (REQ-0014-08); any other press puts the selection down or removes it, and a path is traced
         from the pixel under it, also outside the picture (REQ-0014-04, REQ-0014-07). */
      if (on_selection (self, x, y))
        {
          begin_move (self, button, px, py);
          return;
        }
      if (!finish_selection (self, FALSE))
        return;                               /* the put-down failed: the press starts no path */
      {
        int cx, cy;

        to_corner (self, x, y, &cx, &cy);
        lasso_begin (self, button, cx, cy);
      }
      return;
    }
  if (self->tool == EASEL_TOOL_SELECT)
    {
      /* A press not on the selection: any selection is removed, and a rectangle is dragged out from the pixel under the
         press, also outside the picture (REQ-0013-04). The secondary button on the selection asks for its context menu and
         changes nothing; elsewhere it does nothing (REQ-0013-17). */
      if (button != GDK_BUTTON_PRIMARY)
        {
          if (on_selection (self, x, y))
            g_signal_emit (self, signals[SIGNAL_CONTEXT_MENU], 0, x, y);
          return;
        }
      if (on_selection (self, x, y))
        {
          begin_move (self, button, px, py);
          return;
        }
      if (!finish_selection (self, FALSE))
        return;                                   /* the put-down failed: the press starts no rectangle (REQ-0013-04) */
      self->selecting = TRUE;
      self->sel_button = button;
      self->sel_px0 = self->sel_px1 = px;
      self->sel_py0 = self->sel_py1 = py;
      g_signal_emit (self, signals[SIGNAL_STROKING_CHANGED], 0, TRUE);
      g_signal_emit (self, signals[SIGNAL_SELECTION_CHANGED], 0);
      gtk_widget_queue_draw (GTK_WIDGET (self));
      return;
    }
  if (self->tool == EASEL_TOOL_GRADIENT)
    {
      /* The drag of a gradient: the start is the pixel under the press, also outside the picture; the color is that of the
         press (REQ-0012-03, -04). */
      set_pending (self, TRUE);
      self->pend_polygon = FALSE;
      self->pend_gradient = TRUE;
      self->pend_unshown = FALSE;
      self->moving = -1;
      self->pend_held = TRUE;
      self->pend_step = 0;
      self->pend_button = self->grad_button = button;
      self->pend_pts[0] = self->pend_pts[2] = self->pend_px = px;
      self->pend_pts[1] = self->pend_pts[3] = self->pend_py = py;
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
      self->pend_gradient = FALSE;
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
  if (self->stretching)
    {
      self->stretch_lx = x / logical_per_pixel (self);
      self->stretch_ly = y / logical_per_pixel (self);
      stretch_apply (self);
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
  if (self->sel_moving)
    {
      to_pixel (self, x, y, &px, &py);
      if (px == self->sel_px1 && py == self->sel_py1)
        return;
      self->sel_px1 = px;
      self->sel_py1 = py;
      if (self->sel_state == 1)
        {
          /* The first movement lifts it: the hole in the background color now (REQ-0013-08), none with Ctrl (REQ-0013-09).
             The memory dialog: 0013 T017. */
          if (easel_selection_lift (&self->sel_float, self->img, self->sel_x, self->sel_y, self->sel_w, self->sel_h,
                                    self->sel_mask, self->sel_copy || self->sel_smear, hole_color (self),
                                    self->sel_lift_max) != EASEL_SELECTION_DONE)
            {
              end_failed_move (self);
              return;
            }
          self->sel_state = 2;
          self->sel_moved = TRUE;
          self->sel_lifted_here = TRUE;
          g_signal_emit (self, signals[SIGNAL_PENDING_CHANGED], 0);  /* floating is pending (easel_canvas_is_pending) */
        }
      if (!self->sel_moved)
        {
          /* A floating selection moved with Ctrl first leaves a copy where it is (REQ-0013-09). The memory dialog: T017. */
          self->sel_moved = TRUE;
          if (self->sel_copy && !easel_selection_stamp (&self->sel_float, self->sel_lift_max))
            {
              end_failed_move (self);
              return;
            }
          self->sel_stamped_here = self->sel_copy;
        }
      if (self->sel_smear)
        {
          /* A trail of copies from the place it was at to the place it moves to (REQ-0014-11). The memory dialog: T012. */
          if (!easel_selection_smear (&self->sel_float, self->sel_move_x0 + (px - self->sel_px0),
                                      self->sel_move_y0 + (py - self->sel_py0), self->sel_lift_max))
            {
              end_failed_smear (self);
              return;
            }
          self->sel_x = self->sel_float.x;
          self->sel_y = self->sel_float.y;
        }
      else
        {
          self->sel_x = self->sel_float.x = self->sel_move_x0 + (px - self->sel_px0);
          self->sel_y = self->sel_float.y = self->sel_move_y0 + (py - self->sel_py0);
        }
      g_signal_emit (self, signals[SIGNAL_SELECTION_CHANGED], 0);
      gtk_widget_queue_draw (GTK_WIDGET (self));
      return;
    }
  if (self->lassoing)
    {
      to_corner (self, x, y, &px, &py);
      lasso_extend (self, px, py);
      return;
    }
  if (self->selecting)
    {
      to_pixel (self, x, y, &px, &py);
      if (px != self->sel_px1 || py != self->sel_py1)
        {
          self->sel_px1 = px;
          self->sel_py1 = py;
          g_signal_emit (self, signals[SIGNAL_SELECTION_CHANGED], 0);
          gtk_widget_queue_draw (GTK_WIDGET (self));
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
  if (self->selecting && button == self->sel_button)
    end_select (self);
  if (self->lassoing && button == self->sel_button)
    end_lasso (self);
  if (self->stretching && button == self->sel_button)
    {
      self->stretching = FALSE;                     /* the selection floats at the size it has (REQ-0015-08) */
      easel_selection_complete (&self->sel_float);  /* the whole image is made at the release */
      g_signal_emit (self, signals[SIGNAL_STROKING_CHANGED], 0, FALSE);
      gtk_widget_queue_draw (GTK_WIDGET (self));
    }
  if (self->sel_moving && button == self->sel_button)
    {
      self->sel_moving = FALSE;
      /* A Shift-click on a floating selection, no other pixel reached: a copy stays where it is (REQ-0014-12). The memory dialog: T012. */
      if (self->sel_smear && self->sel_state == 2 && !self->sel_moved
          && !easel_selection_stamp (&self->sel_float, self->sel_lift_max))
        g_signal_emit (self, signals[SIGNAL_SELECTION_FAILED], 0, 0);
      g_signal_emit (self, signals[SIGNAL_STROKING_CHANGED], 0, FALSE);
      gtk_widget_queue_draw (GTK_WIDGET (self));          /* the dashes move on again */
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
  if (hover != self->hover || marker_hit (self, x, y) != self->hover_marker || on_selection (self, x, y) != self->hover_sel
      || easel_canvas_selection_handle_at (self, x, y) != self->hover_sel_handle)
    {
      self->hover = hover;
      self->hover_marker = marker_hit (self, x, y);
      self->hover_sel = on_selection (self, x, y);
      self->hover_sel_handle = easel_canvas_selection_handle_at (self, x, y);
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
    {
      self->shift = (gdk_event_get_modifier_state (event) & GDK_SHIFT_MASK) != 0;
      self->ctrl = (gdk_event_get_modifier_state (event) & GDK_CONTROL_MASK) != 0;     /* the press decides a Ctrl copy (REQ-0013-09) */
    }
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
  /* The canvas is drawn again whenever the view scrolls: what it draws over the picture (the grid, the previews) is limited to the
     part in view, which a scroll changes without the canvas being asked to draw (REQ-0004-12). */
  if (GTK_IS_SCROLLABLE (gtk_widget_get_parent (GTK_WIDGET (self))))
    {
      g_signal_connect_object (adjustment (self, GTK_ORIENTATION_HORIZONTAL), "value-changed", G_CALLBACK (gtk_widget_queue_draw),
                               self, G_CONNECT_SWAPPED);
      g_signal_connect_object (adjustment (self, GTK_ORIENTATION_VERTICAL), "value-changed", G_CALLBACK (gtk_widget_queue_draw),
                               self, G_CONNECT_SWAPPED);
    }
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
   makes it place the canvas at the new scroll position in that frame's layout. What the canvas draws over the picture is limited
   to the part in view, found from where it is placed, so it is drawn again too: a draw before this placement saw the view a
   scroll away (the grid covered only a rectangle of the earlier view, REQ-0004-12). */
static gboolean
replace_in_viewport (GtkWidget *widget, GdkFrameClock *clock, gpointer data)
{
  GtkWidget *viewport = gtk_widget_get_parent (widget);

  if (viewport != NULL)
    gtk_widget_queue_allocate (viewport);
  gtk_widget_queue_draw (widget);
  return G_SOURCE_REMOVE;
}

static void
easel_canvas_size_allocate (GtkWidget *widget, int width, int height, int baseline)
{
  EaselCanvas *self = EASEL_CANVAS (widget);

  /* A popover parented to the canvas (the selection's context menu) is placed by its parent. */
  for (GtkWidget *child = gtk_widget_get_first_child (widget); child != NULL; child = gtk_widget_get_next_sibling (child))
    if (GTK_IS_POPOVER (child))
      gtk_popover_present (GTK_POPOVER (child));

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

  g_clear_handle_id (&self->ants_tick, g_source_remove);   /* a timer, unlike a frame callback, outlives its widget */
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
  drop_float (self);
  shape_free (self);
  lasso_free (self);
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
  signals[SIGNAL_GRADIENT_FAILED] = g_signal_new ("gradient-failed", G_TYPE_FROM_CLASS (klass), G_SIGNAL_RUN_LAST, 0, NULL, NULL,
                                                 NULL, G_TYPE_NONE, 0);
  signals[SIGNAL_SELECTION_CHANGED] = g_signal_new ("selection-changed", G_TYPE_FROM_CLASS (klass), G_SIGNAL_RUN_LAST, 0, NULL,
                                                   NULL, NULL, G_TYPE_NONE, 0);
  signals[SIGNAL_SELECTION_FAILED] = g_signal_new ("selection-failed", G_TYPE_FROM_CLASS (klass), G_SIGNAL_RUN_LAST, 0, NULL, NULL,
                                                  NULL, G_TYPE_NONE, 1, G_TYPE_INT);
  signals[SIGNAL_CONTEXT_MENU] = g_signal_new ("context-menu", G_TYPE_FROM_CLASS (klass), G_SIGNAL_RUN_LAST, 0, NULL, NULL,
                                               NULL, G_TYPE_NONE, 2, G_TYPE_DOUBLE, G_TYPE_DOUBLE);
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
  /* The tint of a selection is the system's accent: drawn again when the user changes it (REQ-0014-17 b). */
  g_signal_connect_object (adw_style_manager_get_default (), "notify::accent-color", G_CALLBACK (gtk_widget_queue_draw), self,
                           G_CONNECT_SWAPPED);
  self->fill_max_bytes = SIZE_MAX;
  self->shape_max_bytes = SIZE_MAX;
  self->preview_max_bytes = SIZE_MAX;
  self->hover_sel_handle = -1;
  self->sel_lift_max = SIZE_MAX;
  self->sel_step_max = SIZE_MAX;
  self->moving = -1;
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
