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
#include "settings.h"
#include "grid.h"
#include "magnifier.h"
#include "stroke.h"
#include "text-buffer.h"
#include "text-put.h"
#include "text-render.h"
#include "view.h"
#include "i18n.h"

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
  GdkTexture *view_tex;   /* the view built by the edges (REQ-0025-02), kept for a repaint with nothing changed */
  int view_x, view_y, view_w, view_h, view_zoom;
  guint view_gen;
  gboolean view_kept;
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
  size_t view_memory_max;                  /* test hook, see easel_canvas_set_view_memory_limit () */
  int tint;                                /* the strength of the selection's tint in percent (REQ-0020-03) */
  double frame_ms;                         /* the CPU time of the last frame's build (REQ-0021-12) */
  guint frames_drawn, painted_idle;
  gboolean marker_on, marker_placed;
  gboolean key_pressing;   /* a keyboard press is being entered, or moved: it takes no handle (REQ-0022-07) */
  guint key_button;        /* the button a keyboard press holds */
  int marker_x, marker_y;
  int draw_delay_ms;                       /* test hook, see easel_canvas_set_draw_delay () */
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
  gboolean sel_for_text;                        /* the rectangle dragged out is a text box's (REQ-0017-03) */
  gboolean text_dragged;                        /* its pointer has gone beyond the drag threshold: a drag to the release, not a click */
  double text_press_x, text_press_y;            /* where that press was, in logical pixels */
  gboolean text_open;                           /* a text box is open (0017) */
  int tb_x, tb_y, tb_w, tb_set_h;               /* its left, top, width and set height (REQ-0017-26) */
  EaselTextBuffer *text_buf;
  char *text_family;                            /* the starting style (REQ-0017-12), set by the window */
  int text_size;
  guint8 text_flags;
  int hover_text;                               /* the pointer over the open box: 1 inside, 2 on its move band, else 0 */
  PangoLayout *text_layout;                     /* the box's text laid out at picture scale (the Plan) */
  EaselTextInk text_ink;                        /* its pixels: colors and mask, owned (REQ-0017-16) */
  int text_h;                                   /* the text's height, picture pixels */
  gboolean text_opaque;                         /* the Text tool's mode: Opaque (REQ-0017-02) */
  gboolean caret_on;                            /* the caret is shown now (blinking, REQ-0017-05) */
  guint caret_blink;                            /* its timer, or 0 */
  size_t text_show_max;                         /* the most the ink may use (a test hook, REQ-0017-07) */
  size_t text_step_max;                         /* the most the put-down's step may use (a test hook, REQ-0017-18) */
  guint error_bells;                            /* the error bells sounded (a test hook) */
  gboolean text_selecting;                      /* a drag inside the box selects (REQ-0017-06) */
  int text_drag;                                /* a drag of the box: 0 none, 1 a move by its band, 2 a resize by a handle (REQ-0017-09, -10) */
  int text_drag_handle;                         /* the handle of a resize (EaselSelHandle) */
  double text_drag_x0, text_drag_y0;            /* the press, in picture pixels */
  int text_drag_old[4];                         /* the box at the press: x, y, width, set height */
  int text_drag_shown_h;                        /* its height as shown at the press */
  size_t text_anchor;
  GtkIMContext *im;                             /* input methods, compose and dead keys (REQ-0017-06) */
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
  EaselOrient stretch_orient0;                  /* and its orientation (a drag past the opposite edge adds mirrors to it) */
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
  EaselImage *paste_base;                       /* a grown paste floats: the picture's pixels before it (REQ-0016-07) */
  uint32_t paste_fill;                          /* and the color of the area it added */
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

enum
{
  SIGNAL_STROKE_FINISHED,
  SIGNAL_STROKING_CHANGED,
  SIGNAL_FILL_FAILED,
  SIGNAL_SHAPE_FAILED,
  SIGNAL_GRADIENT_FAILED,
  SIGNAL_SELECTION_CHANGED,
  SIGNAL_POLYGON_CLOSED,
  SIGNAL_CONTEXT_MENU,
  SIGNAL_CANVAS_MENU,
  SIGNAL_PAINTED,
  SIGNAL_PICTURE_REPLACED,
  SIGNAL_SELECTION_FAILED,
  SIGNAL_TEXT_FAILED,
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
static gboolean view_built_by_edges (EaselCanvas *self);
static gboolean append_cells_by_edges (EaselCanvas *self, GtkSnapshot *snapshot, const uint32_t *px, int x0, int y0, int rw, int rh);

#define TEXT_BAND 4.0          /* the width of a text box's move band, logical pixels (REQ-0017-09) */
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
  g_clear_object (&self->view_tex);
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

  if (img != NULL)
    {
      g_autofree char *name = g_strdup_printf (_n ("Picture, %d by %d pixel", "Picture, %d by %d pixels", (unsigned long) img->height), img->width, img->height);

      gtk_accessible_update_property (GTK_ACCESSIBLE (self), GTK_ACCESSIBLE_PROPERTY_LABEL, name, -1);
      self->marker_x = MIN (self->marker_x, img->width - 1);
      self->marker_y = MIN (self->marker_y, img->height - 1);
    }
  free_tiles (self);
  self->img = img;
  self->resizing = EASEL_HANDLE_NONE;
  self->hover = EASEL_HANDLE_NONE;
  self->hover_sel_handle = -1;
  self->tiles_rebuilt = 0;
  self->text_show_max = SIZE_MAX;
  self->view_memory_max = SIZE_MAX;
  self->text_step_max = SIZE_MAX;
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
  if (self->key_pressing)
    return EASEL_HANDLE_NONE;                   /* a keyboard press acts on the pixel only (REQ-0022-07) */
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
  if (self->key_pressing)
    return FALSE;
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

  /* A keyboard press takes a gradient's marker on that marker's own pixel, and no other (REQ-0022-07). */
  if (self->key_pressing)
    {
      if (!self->pending || !self->pend_gradient)
        return -1;
      for (int end = 1; end >= 0; end--)
        if (self->pend_pts[2 * end] == self->marker_x && self->pend_pts[2 * end + 1] == self->marker_y)
          return end;
      return -1;
    }
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

  if (self->sel_for_text && !self->text_dragged)
    return FALSE;                                 /* a box's press within the drag threshold is a click: no rectangle (REQ-0017-03) */
  if (self->shift && !self->sel_for_text)
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
  if (self->text_open)
    return easel_canvas_text_box (self, &x, &y, w, h);           /* REQ-0017-01: the box's size as a selection's */
  return easel_canvas_get_selection (self, &x, &y, w, h, &floating);
}

/* ---- the text box (0017) ---- */

static void text_free_ink (EaselCanvas *self);
static void text_close (EaselCanvas *self);

gboolean
easel_canvas_text_box (EaselCanvas *self, int *x, int *y, int *w, int *h)
{
  if (!self->text_open)
    return FALSE;
  *x = self->tb_x;
  *y = self->tb_y;
  *w = self->tb_w;
  /* The larger of the set height and the text's, down to the picture's bottom edge (REQ-0017-26, REQ-0017-07) */
  *h = MIN (MAX (self->tb_set_h, self->text_h), self->img->height - self->tb_y);
  return TRUE;
}

EaselTextBuffer *
easel_canvas_text_buffer (EaselCanvas *self)
{
  return self->text_open ? self->text_buf : NULL;
}

gboolean
easel_canvas_text_ink (EaselCanvas *self, EaselTextInk *ink)
{
  if (!self->text_open || self->text_ink.mask == NULL)
    return FALSE;
  *ink = self->text_ink;
  return TRUE;
}

/* At the start of the text or of a line started by Enter the style is the next character's; at a wrapped line's start, as anywhere
   else, the one before (REQ-0017-26, JOD, 2026-10-26) */
static void
style_at_caret_of (const EaselTextBuffer *b, EaselTextStyle *out)
{
  size_t len, caret = easel_text_buffer_caret (b);
  const char *text = easel_text_buffer_text (b, &len);

  easel_text_buffer_style_at_caret (b, caret == 0 || text[caret - 1] == '\n', out);
}

void
easel_canvas_text_selection_style (EaselCanvas *self, EaselTextStyle *out, unsigned *mixed)
{
  size_t len, caret;
  const char *text;

  *mixed = 0;
  if (!self->text_open)
    return;
  text = easel_text_buffer_text (self->text_buf, &len);
  caret = easel_text_buffer_caret (self->text_buf);
  easel_text_buffer_selection_style (self->text_buf, caret == 0 || text[caret - 1] == '\n', out, mixed);
}

void
easel_canvas_text_style_at_caret (EaselCanvas *self, EaselTextStyle *out)
{
  if (self->text_open)
    style_at_caret_of (self->text_buf, out);
}

void
easel_canvas_set_view_memory_limit (EaselCanvas *self, size_t max_bytes)
{
  self->view_memory_max = max_bytes;
  self->view_kept = FALSE;
}

gboolean
easel_canvas_text_caret_on (EaselCanvas *self)
{
  return self->text_open && self->caret_on;
}

void
easel_canvas_set_text_show_limit (EaselCanvas *self, size_t max_bytes)
{
  self->text_show_max = max_bytes;
}

void
easel_canvas_set_text_step_limit (EaselCanvas *self, size_t max_bytes)
{
  self->text_step_max = max_bytes;
}

guint
easel_canvas_get_error_bells (EaselCanvas *self)
{
  return self->error_bells;
}

static void
error_bell (EaselCanvas *self)
{
  self->error_bells++;
  gtk_widget_error_bell (GTK_WIDGET (self));
}

void
easel_canvas_set_text_opaque (EaselCanvas *self, gboolean opaque)
{
  self->text_opaque = opaque;
  gtk_widget_queue_draw (GTK_WIDGET (self));
}

static gboolean
caret_tick (gpointer data)
{
  EaselCanvas *self = data;

  self->caret_on = !self->caret_on;
  gtk_widget_queue_draw (GTK_WIDGET (self));
  return G_SOURCE_CONTINUE;
}

/* The caret shown again, and blinking as the desktop says (REQ-0017-05): a change starts it over, as in other GNOME text fields. */
static void
caret_restart (EaselCanvas *self)
{
  gboolean blink = TRUE;
  int time = 1200;

  g_clear_handle_id (&self->caret_blink, g_source_remove);
  self->caret_on = TRUE;
  g_object_get (gtk_widget_get_settings (GTK_WIDGET (self)), "gtk-cursor-blink", &blink, "gtk-cursor-blink-time", &time, NULL);
  if (blink && self->text_open)
    self->caret_blink = g_timeout_add (MAX (time / 2, 50), caret_tick, self);
}

/* The box's layout and pixels made again from its text (the Plan): after every edit and change of its width. */
static void
text_relayout (EaselCanvas *self)
{
  EaselTextInk ink;

  g_clear_object (&self->text_layout);
  self->text_layout = easel_text_render_layout (self->text_buf, self->tb_w);
  self->text_h = easel_text_render_height (self->text_layout);
  if (easel_text_render_ink (self->text_layout, self->text_buf, self->tb_x, self->tb_y, self->text_show_max, &ink))
    {
      text_free_ink (self);
      self->text_ink = ink;
    }
}

void
easel_canvas_text_edited (EaselCanvas *self)
{
  if (!self->text_open)
    return;
  text_relayout (self);
  caret_restart (self);
  g_signal_emit (self, signals[SIGNAL_SELECTION_CHANGED], 0);
  gtk_widget_queue_draw (GTK_WIDGET (self));
}

gboolean
easel_canvas_text_step (EaselCanvas *self, gboolean undo)
{
  int rect[4];
  bool geometry = false;

  if (!self->text_open)
    return FALSE;
  if (undo && !easel_text_buffer_can_undo (self->text_buf))
    {
      text_close (self);                              /* nothing left: no step, the picture's redo steps kept */
      return TRUE;
    }
  if (!(undo ? easel_text_buffer_undo (self->text_buf, rect, &geometry) : easel_text_buffer_redo (self->text_buf, rect, &geometry)))
    {
      if (undo || easel_text_buffer_can_redo (self->text_buf))
        error_bell (self);                            /* no memory for it: the box as it was (REQ-0017-08) */
      return TRUE;
    }
  if (geometry)
    {
      self->tb_x = rect[0];
      self->tb_y = rect[1];
      self->tb_w = rect[2];
      self->tb_set_h = rect[3];
    }
  easel_canvas_text_edited (self);
  return TRUE;
}

gboolean
easel_canvas_text_select_all (EaselCanvas *self)
{
  size_t len;

  if (!self->text_open)
    return FALSE;
  easel_text_buffer_text (self->text_buf, &len);
  easel_text_buffer_select (self->text_buf, 0, len);
  easel_canvas_text_edited (self);
  return TRUE;
}

gboolean
easel_canvas_text_can_redo (EaselCanvas *self)
{
  return self->text_open && easel_text_buffer_can_redo (self->text_buf);
}

/* ---- editing the box (REQ-0017-06, REQ-0017-07) ---- */

typedef struct
{
  const char *text;            /* an insertion at the caret, replacing the selection */
  size_t len;
  size_t from, to;             /* or a deletion, when text is NULL */
  EaselTextEditKind kind;
  EaselTextField field;        /* or, when not 0, a change of this style field to `value` (REQ-0017-12) */
  const EaselTextStyle *value;
  const EaselTextRun *runs;    /* an insertion in these runs (a paste, REQ-0017-20), when not NULL */
  size_t n_runs;
  int step;                    /* or, when not 0, each part's size stepped by it (REQ-0017-12) */
} TextOp;

/* Whether the caret is at the start of the text or of a line started by Enter (REQ-0017-26). */
static gboolean
caret_at_line_start (const EaselTextBuffer *b)
{
  size_t len, caret = easel_text_buffer_caret (b);
  const char *text = easel_text_buffer_text (b, &len);

  return caret == 0 || text[caret - 1] == '\n';
}

static gboolean
apply_op (EaselTextBuffer *b, const TextOp *op)
{
  EaselTextStyle st;
  bool changed;

  if (op->step != 0)
    return easel_text_buffer_step_size (b, op->step, EASEL_TEXT_SIZE_MIN, EASEL_TEXT_SIZE_MAX, caret_at_line_start (b), &changed);
  if (op->field != 0)
    return easel_text_buffer_apply (b, op->field, op->value, caret_at_line_start (b));
  if (op->runs != NULL)
    return easel_text_buffer_insert_runs (b, op->text, op->len, op->runs, op->n_runs, op->kind);
  if (op->text == NULL)
    return easel_text_buffer_delete (b, op->from, op->to, op->kind);
  style_at_caret_of (b, &st);                         /* typed text takes the style at the caret (REQ-0017-12) */
  return easel_text_buffer_insert (b, op->text, op->len, &st, op->kind);
}

/* An edit tried on a copy first: refused with the error bell, nothing of it done, if the text would pass the picture's bottom edge (the
   first line always allowed) or its pixels cannot get their memory; a box whose first text cannot be shown closes, nothing put down,
   and the window says why once (REQ-0017-07). Otherwise made on the box, with the copy's layout and pixels. */
static gboolean
text_try (EaselCanvas *self, const TextOp *op)
{
  EaselTextBuffer *copy = easel_text_buffer_copy (self->text_buf);
  PangoLayout *layout = NULL;
  EaselTextInk ink = { 0 };
  int h;
  size_t len;
  gboolean ok = copy != NULL && apply_op (copy, op), no_memory = !ok;   /* an edit fails only for memory (REQ-0017-08) */

  if (ok)
    {
      layout = easel_text_render_layout (copy, self->tb_w);
      h = easel_text_render_height (layout);
      if (pango_layout_get_line_count (layout) > 1 && self->tb_y + MAX (h, self->tb_set_h) > self->img->height)
        ok = FALSE;                                   /* past the bottom edge */
      else if (!easel_text_render_ink (layout, copy, self->tb_x, self->tb_y, self->text_show_max, &ink))
        ok = FALSE, no_memory = TRUE;
    }
  if (ok && !apply_op (self->text_buf, op))
    ok = FALSE, no_memory = TRUE;
  easel_text_buffer_free (copy);
  if (!ok)
    {
      g_clear_object (&layout);
      g_free ((uint32_t *) ink.color);
      g_free ((uint8_t *) ink.mask);
      easel_text_buffer_text (self->text_buf, &len);
      if (no_memory && len == 0)
        {
          /* the first text cannot be had or shown: the box goes, nothing put down (REQ-0017-07, REQ-0017-08) */
          text_close (self);
          g_signal_emit (self, signals[SIGNAL_TEXT_FAILED], 0, 0);
          return FALSE;
        }
      error_bell (self);
      return FALSE;
    }
  g_clear_object (&self->text_layout);
  self->text_layout = layout;
  self->text_h = h;
  text_free_ink (self);
  self->text_ink = ink;
  caret_restart (self);
  g_signal_emit (self, signals[SIGNAL_SELECTION_CHANGED], 0);
  gtk_widget_queue_draw (GTK_WIDGET (self));
  return TRUE;
}

gboolean
easel_canvas_text_apply (EaselCanvas *self, EaselTextField field, const EaselTextStyle *value)
{
  EaselTextStyle v = *value;
  TextOp op = { NULL, 0, 0, 0, EASEL_TEXT_EDIT_FORMAT, field, &v };

  if (!self->text_open)
    return FALSE;
  /* The box's own copy of a family's name (the style's names are the buffer's) */
  if (field == EASEL_TEXT_FAMILY && (v.family = easel_text_buffer_intern (self->text_buf, value->family)) == NULL)
    {
      error_bell (self);
      return FALSE;
    }
  return text_try (self, &op);
}

gboolean
easel_canvas_text_step_size (EaselCanvas *self, int delta)
{
  TextOp op = { .kind = EASEL_TEXT_EDIT_FORMAT, .step = delta };

  return self->text_open && text_try (self, &op);
}

void
easel_canvas_text_toggle (EaselCanvas *self, guint8 flag)
{
  /* on unless all the selected text (or the caret's style) has it (REQ-0017-12) */
  EaselTextField field = flag == EASEL_TEXT_BOLD ? EASEL_TEXT_FLAG_BOLD : flag == EASEL_TEXT_ITALIC ? EASEL_TEXT_FLAG_ITALIC
                         : flag == EASEL_TEXT_UNDERLINE ? EASEL_TEXT_FLAG_UNDERLINE : EASEL_TEXT_FLAG_STRIKE;
  EaselTextStyle st = { 0 }, v = { 0 };
  unsigned mixed;

  if (!self->text_open)
    return;
  easel_canvas_text_selection_style (self, &st, &mixed);
  v.flags = (st.flags & flag) && !(mixed & field) ? 0 : flag;
  easel_canvas_text_apply (self, field, &v);
}

gboolean
easel_canvas_text_delete_selection (EaselCanvas *self, EaselTextEditKind kind)
{
  size_t from, to;
  TextOp op = { NULL, 0, 0, 0, kind, 0, NULL, NULL, 0 };

  if (!self->text_open || !easel_text_buffer_selection (self->text_buf, &from, &to))
    return FALSE;
  op.from = from;
  op.to = to;
  return text_try (self, &op);
}

gboolean
easel_canvas_text_paste (EaselCanvas *self, const char *text, size_t len, const EaselRichRun *rich, size_t n_rich)
{
  g_autofree char *out = NULL;
  g_autofree size_t *at = NULL;
  g_autofree EaselTextRun *runs = NULL;
  EaselTextStyle caret;
  size_t n = 0, k = 0;
  TextOp op = { NULL, 0, 0, 0, EASEL_TEXT_EDIT_PASTE, 0, NULL, NULL, 0 };

  if (!self->text_open)
    return FALSE;
  out = g_try_malloc (len + 1);
  at = g_try_new (size_t, len + 1);
  runs = g_try_new (EaselTextRun, MAX (n_rich, 1));
  if (out == NULL || at == NULL || runs == NULL)
    {
      error_bell (self);
      return FALSE;
    }
  /* Line breaks of any kind become new lines, a tab a space, other control characters go (REQ-0017-20); at[i] is where byte i went */
  for (size_t i = 0; i < len; i++)
    {
      unsigned char c = (unsigned char) text[i];

      at[i] = k;
      if (c == '\r')
        {
          out[k++] = '\n';
          if (i + 1 < len && text[i + 1] == '\n')
            at[++i] = k - 1;                          /* "\r\n": one new line */
        }
      else if (c == '\t')
        out[k++] = ' ';
      else if (c >= 0x20 || c == '\n')
        out[k++] = (char) c;
    }
  at[len] = k;
  out[k] = '\0';
  style_at_caret_of (self->text_buf, &caret);
  /* Each part's own style; what a part does not give, and a family not installed, is the caret's (REQ-0017-20) */
  for (size_t r = 0; r < n_rich || (r == 0 && n_rich == 0); r++)
    {
      EaselTextStyle st = caret;
      size_t start = rich != NULL && r < n_rich ? at[MIN (rich[r].start, len)] : 0;

      if (rich != NULL && r < n_rich)
        {
          const char *family = rich[r].family != NULL ? easel_text_render_find_family (rich[r].family) : NULL;

          if (family != NULL && (st.family = easel_text_buffer_intern (self->text_buf, family)) == NULL)
            st.family = caret.family;
          if (rich[r].size_pt > 0)
            st.size_pt = rich[r].size_pt;
          st.flags = rich[r].flags;
          if (rich[r].has_color)
            st.color = rich[r].color;
        }
      if (n > 0 && start <= runs[n - 1].start)
        runs[n - 1].style = st;                     /* an empty part: the later one's style */
      else
        runs[n++] = (EaselTextRun) { start, st };
      if (rich == NULL)
        break;
    }
  if (k == 0)
    return TRUE;                                    /* nothing to put in */
  runs[0].start = 0;
  op.text = out;
  op.len = k;
  op.runs = runs;
  op.n_runs = n;
  return text_try (self, &op);
}

void
easel_canvas_text_commit (EaselCanvas *self, const char *utf8)
{
  TextOp op = { utf8, strlen (utf8), 0, 0, EASEL_TEXT_EDIT_TYPING };

  if (self->text_open && utf8[0] != '\0')
    text_try (self, &op);
}

/* The caret moved to `caret`, the selection kept from its anchor with Shift (`extend`), else none. */
static void
text_move (EaselCanvas *self, size_t caret, gboolean extend)
{
  size_t from, to, anchor = caret;

  if (extend)
    {
      easel_text_buffer_selection (self->text_buf, &from, &to);
      anchor = easel_text_buffer_caret (self->text_buf) == to ? from : to;
    }
  easel_text_buffer_select (self->text_buf, anchor, caret);
  caret_restart (self);
  g_signal_emit (self, signals[SIGNAL_SELECTION_CHANGED], 0);
  gtk_widget_queue_draw (GTK_WIDGET (self));
}

/* The byte index of the character offset `chars` of the text. */
static size_t
index_of_offset (const char *text, size_t len, int chars)
{
  const char *p = text;

  for (int i = 0; i < chars && (size_t) (p - text) < len; i++)
    p = g_utf8_next_char (p);
  return (size_t) (p - text);
}

/* The start of the word before `caret` (dir -1), or the end of the word after it (+1), from Pango's word boundaries. */
static size_t
word_edge (EaselCanvas *self, size_t caret, int dir)
{
  size_t len;
  const char *text = easel_text_buffer_text (self->text_buf, &len);
  int n = 0, at = (int) g_utf8_pointer_to_offset (text, text + caret);
  const PangoLogAttr *attrs = pango_layout_get_log_attrs_readonly (self->text_layout, &n);

  if (dir < 0)
    {
      for (at--; at > 0 && !attrs[at].is_word_start; at--)
        ;
      at = MAX (at, 0);
    }
  else
    {
      for (at++; at < n - 1 && !attrs[at].is_word_end; at++)
        ;
      at = MIN (at, n - 1);
    }
  return index_of_offset (text, len, at);
}

gboolean
easel_canvas_text_key (EaselCanvas *self, guint keyval, GdkModifierType state)
{
  size_t len, caret, from, to;
  const char *text;
  gboolean shift = (state & GDK_SHIFT_MASK) != 0, ctrl = (state & GDK_CONTROL_MASK) != 0;
  guint32 c;

  if (!self->text_open || self->text_layout == NULL || easel_canvas_is_stroking (self))
    return FALSE;
  text = easel_text_buffer_text (self->text_buf, &len);
  caret = easel_text_buffer_caret (self->text_buf);
  if (ctrl && !shift && (keyval == GDK_KEY_b || keyval == GDK_KEY_i || keyval == GDK_KEY_u))
    {
      /* Bold, Italic and Underline toggled (REQ-0017-12) */
      easel_canvas_text_toggle (self, keyval == GDK_KEY_b ? EASEL_TEXT_BOLD : keyval == GDK_KEY_i ? EASEL_TEXT_ITALIC : EASEL_TEXT_UNDERLINE);
      return TRUE;
    }
  if (ctrl && !(state & GDK_ALT_MASK) && (keyval == GDK_KEY_bracketright || keyval == GDK_KEY_bracketleft))
    {
      /* the size 1 point larger or smaller; a press that changes no size does nothing (REQ-0017-12); a bracket typed with AltGr
         (reported as Ctrl+Alt on some setups) is not this key */
      easel_canvas_text_step_size (self, keyval == GDK_KEY_bracketright ? 1 : -1);
      return TRUE;
    }
  switch (keyval)
    {
    case GDK_KEY_Return:
    case GDK_KEY_KP_Enter:
      easel_canvas_text_commit (self, "\n");          /* a new line (REQ-0017-06) */
      return TRUE;
    case GDK_KEY_BackSpace:
    case GDK_KEY_Delete:
    case GDK_KEY_KP_Delete:
      {
        TextOp op = { NULL, 0, 0, 0, keyval == GDK_KEY_BackSpace ? EASEL_TEXT_EDIT_BACKSPACE : EASEL_TEXT_EDIT_DELETE };

        if (easel_text_buffer_selection (self->text_buf, &from, &to))
          op.from = from, op.to = to;
        else if (keyval == GDK_KEY_BackSpace && caret > 0)
          op.from = (size_t) (g_utf8_find_prev_char (text, text + caret) - text), op.to = caret;
        else if (keyval != GDK_KEY_BackSpace && caret < len)
          op.from = caret, op.to = (size_t) (g_utf8_next_char (text + caret) - text);
        else
          return TRUE;
        text_try (self, &op);
        return TRUE;
      }
    case GDK_KEY_Left:
    case GDK_KEY_Right:
    case GDK_KEY_KP_Left:
    case GDK_KEY_KP_Right:
      {
        int dir = keyval == GDK_KEY_Left || keyval == GDK_KEY_KP_Left ? -1 : 1, index, trailing;

        if (!shift && !ctrl && easel_text_buffer_selection (self->text_buf, &from, &to))
          {
            text_move (self, dir < 0 ? from : to, FALSE);
            return TRUE;
          }
        if (ctrl)
          {
            text_move (self, word_edge (self, caret, dir), shift);
            return TRUE;
          }
        /* visually, as Pango lays the line out (right-to-left text too) */
        pango_layout_move_cursor_visually (self->text_layout, TRUE, (int) caret, 0, dir, &index, &trailing);
        if (index < 0)
          index = 0;
        else if (index == G_MAXINT || (size_t) index > len)
          index = (int) len;
        else
          index = (int) index_of_offset (text + index, len - (size_t) index, trailing) + index;
        text_move (self, (size_t) index, shift);
        return TRUE;
      }
    case GDK_KEY_Up:
    case GDK_KEY_Down:
    case GDK_KEY_KP_Up:
    case GDK_KEY_KP_Down:
    case GDK_KEY_Home:
    case GDK_KEY_End:
    case GDK_KEY_KP_Home:
    case GDK_KEY_KP_End:
      {
        int line_no = 0, x = 0, index = 0, trailing = 0;
        gboolean up = keyval == GDK_KEY_Up || keyval == GDK_KEY_KP_Up, home = keyval == GDK_KEY_Home || keyval == GDK_KEY_KP_Home;
        gboolean end = keyval == GDK_KEY_End || keyval == GDK_KEY_KP_End;
        PangoLayoutLine *line;

        if (ctrl && (home || end))
          {
            text_move (self, home ? 0 : len, shift);
            return TRUE;
          }
        pango_layout_index_to_line_x (self->text_layout, (int) caret, FALSE, &line_no, &x);
        if (home || end)
          {
            line = pango_layout_get_line_readonly (self->text_layout, line_no);
            index = home ? line->start_index : line->start_index + line->length;
            if (end && index > line->start_index && (size_t) index <= len && text[index - 1] == '\n')
              index--;
            text_move (self, (size_t) index, shift);
            return TRUE;
          }
        line_no += up ? -1 : 1;
        if (line_no < 0 || line_no >= pango_layout_get_line_count (self->text_layout))
          {
            text_move (self, up ? 0 : len, shift);
            return TRUE;
          }
        line = pango_layout_get_line_readonly (self->text_layout, line_no);
        pango_layout_line_x_to_index (line, x, &index, &trailing);
        text_move (self, index_of_offset (text + index, len - (size_t) index, trailing) + (size_t) index, shift);
        return TRUE;
      }
    default:
      break;
    }
  if (ctrl || (state & GDK_ALT_MASK) != 0)
    return FALSE;
  c = gdk_keyval_to_unicode (keyval);
  if (c != 0 && c != '\t' && g_unichar_isprint (c))
    {
      char utf8[7] = { 0 };

      g_unichar_to_utf8 (c, utf8);
      easel_canvas_text_commit (self, utf8);        /* the key's character, where no input method took it (REQ-0017-06) */
      return TRUE;
    }
  return FALSE;
}

gboolean
easel_canvas_text_filter (EaselCanvas *self, GdkEvent *event)
{
  return self->text_open && self->im != NULL && event != NULL && gtk_im_context_filter_keypress (self->im, event);
}

static void
on_im_commit (GtkIMContext *im, const char *text, EaselCanvas *self)
{
  easel_canvas_text_commit (self, text);
}

/* The picture point (canvas coordinates) of the character at byte `index`: the middle of its caret's line, just after the caret. */
void
easel_canvas_text_point (EaselCanvas *self, size_t index, double *x, double *y)
{
  PangoRectangle strong;
  double l = logical_per_pixel (self);

  *x = *y = 0;
  if (!self->text_open || self->text_layout == NULL)
    return;
  pango_layout_get_cursor_pos (self->text_layout, (int) index, &strong, NULL);
  *x = (self->tb_x + (double) strong.x / PANGO_SCALE + 0.5) * l;
  *y = (self->tb_y + ((double) strong.y + strong.height / 2.0) / PANGO_SCALE) * l;
}

/* The byte index of the text under the canvas point (x, y). */
static size_t
text_index_at (EaselCanvas *self, double x, double y)
{
  double l = logical_per_pixel (self);
  int index = 0, trailing = 0;
  size_t len;
  const char *text = easel_text_buffer_text (self->text_buf, &len);

  int px = (int) ((x / l - self->tb_x) * PANGO_SCALE), line_no = 0;
  PangoLayoutLine *line;

  pango_layout_xy_to_index (self->text_layout, px, (int) ((y / l - self->tb_y) * PANGO_SCALE), &index, &trailing);
  pango_layout_index_to_line_x (self->text_layout, index, FALSE, &line_no, NULL);
  line = pango_layout_get_line_readonly (self->text_layout, line_no);
  if (trailing == 0 && line_no + 1 < pango_layout_get_line_count (self->text_layout) && line->length > 0
      && index == (int) (g_utf8_prev_char (text + line->start_index + line->length) - text))
    {
      /* Pango never puts the point after a wrapped line's last character (it is also the next line's start): past that character's
         middle, it is the line's end, so that the character can be selected (REQ-0017-06) */
      PangoRectangle pos;

      pango_layout_index_to_pos (self->text_layout, index, &pos);
      if (px >= pos.x + pos.width / 2)
        return (size_t) (line->start_index + line->length);
    }
  return index_of_offset (text + index, len - (size_t) index, trailing) + (size_t) index;
}

/* A primary press inside the box: the caret there; a double-click selects the word; a drag selects (REQ-0017-06). */
static void
text_press (EaselCanvas *self, double x, double y)
{
  size_t at = text_index_at (self, x, y);

  if (self->click_count == 2)
    {
      size_t len;
      const char *text = easel_text_buffer_text (self->text_buf, &len);
      int n = 0, off = (int) g_utf8_pointer_to_offset (text, text + at), a = off, z = off;
      const PangoLogAttr *attrs = pango_layout_get_log_attrs_readonly (self->text_layout, &n);

      while (a > 0 && !attrs[a].is_word_start)
        a--;
      while (z < n - 1 && !attrs[z].is_word_end)
        z++;
      easel_text_buffer_select (self->text_buf, index_of_offset (text, len, a), index_of_offset (text, len, z));
      caret_restart (self);
      g_signal_emit (self, signals[SIGNAL_SELECTION_CHANGED], 0);   /* the card shows the word's style (REQ-0017-14) */
      gtk_widget_queue_draw (GTK_WIDGET (self));
      return;
    }
  self->text_selecting = TRUE;
  self->text_anchor = at;
  text_move (self, at, FALSE);
}

void
easel_canvas_set_text_start_style (EaselCanvas *self, const char *family, int size_pt, guint8 flags)
{
  g_free (self->text_family);
  self->text_family = g_strdup (family);
  self->text_size = size_pt;
  self->text_flags = flags;
}

static EaselTextStyle
text_start_style (EaselCanvas *self)
{
  return (EaselTextStyle) { self->text_family != NULL ? self->text_family : "Sans", self->text_size > 0 ? self->text_size : 12,
                            self->text_flags, self->foreground };
}

/* Closes the open box. With text, the put-down writes it first (T013); an empty box puts nothing down (REQ-0017-16). */
static void
text_free_ink (EaselCanvas *self)
{
  g_free ((uint32_t *) self->text_ink.color);
  g_free ((uint8_t *) self->text_ink.mask);
  self->text_ink = (EaselTextInk) { 0 };
}

static void
text_close (EaselCanvas *self)
{
  if (!self->text_open)
    return;
  self->text_open = FALSE;
  self->text_selecting = FALSE;
  if (self->im != NULL)
    gtk_im_context_focus_out (self->im);
  self->hover_text = 0;
  g_clear_handle_id (&self->caret_blink, g_source_remove);
  g_clear_object (&self->text_layout);
  text_free_ink (self);
  self->text_h = 0;
  {
    /* The next box starts in the style at this one's caret (REQ-0017-12), its color excepted (the foreground's at its making) */
    EaselTextStyle st;

    style_at_caret_of (self->text_buf, &st);
    easel_canvas_set_text_start_style (self, st.family, st.size_pt, st.flags);
  }
  g_clear_pointer (&self->text_buf, easel_text_buffer_free);
  g_signal_emit (self, signals[SIGNAL_SELECTION_CHANGED], 0);
  gtk_widget_queue_draw (GTK_WIDGET (self));
}

/* Puts the open box down (REQ-0017-16): its text written into the picture, on the background in Opaque mode, as one step; the box
   closes. Without the memory for the step the picture stays as it was, the box stays open with its text and the dialog is asked for
   (REQ-0017-18): FALSE, and the caller does not go on. TRUE with no box. */
static gboolean
text_put_down (EaselCanvas *self)
{
  EaselSelectionResult r;
  int x, y, w, h;
  size_t len;

  if (!easel_canvas_text_box (self, &x, &y, &w, &h))
    return TRUE;
  if (easel_text_buffer_text (self->text_buf, &len) != NULL && len == 0)
    {
      text_close (self);                              /* no characters: nothing put down in either mode, no step (REQ-0017-16) */
      return TRUE;
    }
  r = easel_text_put_down (self->img, self->history, x, y, w, h, &self->text_ink, self->text_opaque, self->background, self->text_step_max);
  if (r == EASEL_SELECTION_NO_MEMORY)
    {
      g_signal_emit (self, signals[SIGNAL_TEXT_FAILED], 0, 1);
      return FALSE;
    }
  text_close (self);                              /* the window syncs Undo, Redo and the title on its selection-changed */
  if (r == EASEL_SELECTION_DONE)
    easel_canvas_update (self);
  return TRUE;
}

/* Opens a box at the end of its drag (REQ-0017-03): the rectangle dragged out, or for a click twenty widths of "0" from the click; at
   least one line high, moved toward smaller y as far as the picture's bottom edge requires, never taller than the picture. */
static void
text_open_from_drag (EaselCanvas *self, gboolean click)
{
  EaselTextStyle start = text_start_style (self);
  int line_h, twenty, x, y, w, h;

  easel_text_render_metrics (&start, &line_h, &twenty);
  if (click)
    {
      x = self->sel_px0;
      y = self->sel_py0;
      w = MIN (twenty, self->img->width - x);
      h = line_h;
    }
  else if (!drag_rect (self, &x, &y, &w, &h))
    return;
  h = MAX (h, line_h);
  if (h >= self->img->height)
    {
      y = 0;
      h = self->img->height;
    }
  else if (y + h > self->img->height)
    y = self->img->height - h;
  self->text_buf = easel_text_buffer_new (&start);
  if (self->text_buf == NULL)
    return;
  self->text_open = TRUE;
  self->tb_x = x;
  self->tb_y = y;
  self->tb_w = MAX (w, 1);
  self->tb_set_h = h;
  text_relayout (self);
  caret_restart (self);
  if (self->im != NULL)
    gtk_im_context_focus_in (self->im);
}

/* Where (x, y) of the canvas is for the open box: 1 inside, 2 on its move band (4 logical pixels outside it, less the handles' hit
   areas, which come first), 3 on a handle; 0 elsewhere or with no box (REQ-0017-09). */
static int
text_hit (EaselCanvas *self, double x, double y)
{
  if (self->key_pressing)
    return 0;
  double l = logical_per_pixel (self), bx, by, bw, bh;

  if (!self->text_open || self->img == NULL)
    return 0;
  {
    int x0, y0, w0, h0;

    easel_canvas_text_box (self, &x0, &y0, &w0, &h0);  /* the box as shown: at least its text's height */
    bx = x0 * l;
    by = y0 * l;
    bw = w0 * l;
    bh = h0 * l;
  }
  if (x >= bx && x < bx + bw && y >= by && y < by + bh)
    return 1;
  if (easel_canvas_selection_handle_at (self, x, y) >= 0)
    return 3;
  if (x >= bx - TEXT_BAND && x < bx + bw + TEXT_BAND && y >= by - TEXT_BAND && y < by + bh + TEXT_BAND)
    return 2;
  return 0;
}

/* Whether the box's text laid out at width `w` from y `y` stays above the picture's bottom edge (its first line always does, REQ-0017-07). */
static gboolean
text_fits (EaselCanvas *self, int y, int w)
{
  PangoLayout *layout = easel_text_render_layout (self->text_buf, w);
  gboolean fits = pango_layout_get_line_count (layout) <= 1 || y + easel_text_render_height (layout) <= self->img->height;

  g_object_unref (layout);
  return fits;
}

/* The box set to (x, y, w, set_h) during a drag: a move shifts its pixels; a new width lays its text out again, taken only if the text
   stays above the picture's bottom edge and its pixels can be had (REQ-0017-07, REQ-0017-10). FALSE, the box as it was, if not taken. */
static gboolean
text_set_rect (EaselCanvas *self, int x, int y, int w, int set_h)
{
  PangoLayout *layout;
  EaselTextInk ink;

  if (w != self->tb_w)
    {
      if (!text_fits (self, y, w))
        return FALSE;
      layout = easel_text_render_layout (self->text_buf, w);
      if (!easel_text_render_ink (layout, self->text_buf, x, y, self->text_show_max, &ink))
        {
          g_object_unref (layout);
          return FALSE;
        }
      g_clear_object (&self->text_layout);
      self->text_layout = layout;
      self->text_h = easel_text_render_height (layout);
      text_free_ink (self);
      self->text_ink = ink;
    }
  else
    {
      self->text_ink.x += x - self->tb_x;               /* the same lines, elsewhere */
      self->text_ink.y += y - self->tb_y;
    }
  self->tb_x = x;
  self->tb_y = y;
  self->tb_w = w;
  self->tb_set_h = set_h;
  return TRUE;
}

/* A primary press on the box's band (handle < 0: a move) or on one of its handles (a resize) (REQ-0017-09, REQ-0017-10). */
static void
text_drag_begin (EaselCanvas *self, int handle, double x, double y)
{
  double l = logical_per_pixel (self);
  int bx, by, bw, bh;

  easel_canvas_text_box (self, &bx, &by, &bw, &bh);
  self->text_drag = handle < 0 ? 1 : 2;
  self->text_drag_handle = handle;
  self->sel_button = GDK_BUTTON_PRIMARY;
  self->text_drag_x0 = x / l;
  self->text_drag_y0 = y / l;
  self->text_drag_old[0] = self->tb_x;
  self->text_drag_old[1] = self->tb_y;
  self->text_drag_old[2] = self->tb_w;
  self->text_drag_old[3] = self->tb_set_h;
  self->text_drag_shown_h = bh;
  self->text_selecting = FALSE;
  g_signal_emit (self, signals[SIGNAL_STROKING_CHANGED], 0, TRUE);
}

/* The pointer at (x, y) during a drag of the box: a move keeps the grab point on the box and the box wholly inside the picture; a resize
   changes the sides its handle stands for as a selection's does (REQ-0015-03), inside the picture and at least 1 pixel, the text wrapped
   at the new width, and stops at the last width whose text stays above the bottom edge (REQ-0017-09, REQ-0017-10). */
static void
text_drag_to (EaselCanvas *self, double x, double y)
{
  double l = logical_per_pixel (self);
  const int *o = self->text_drag_old;
  int dx = (int) floor (x / l - self->text_drag_x0 + 0.5), dy = (int) floor (y / l - self->text_drag_y0 + 0.5);
  int pw = self->img->width, ph = self->img->height;

  if (self->text_drag == 1)
    text_set_rect (self, CLAMP (o[0] + dx, 0, MAX (pw - o[2], 0)), CLAMP (o[1] + dy, 0, MAX (ph - self->text_drag_shown_h, 0)), o[2],
                   o[3]);
  else
    {
      int nx, ny, nw, nh;
      EaselSelHandle handle = (EaselSelHandle) self->text_drag_handle;
      gboolean left = handle == EASEL_SEL_HANDLE_L || handle == EASEL_SEL_HANDLE_TL || handle == EASEL_SEL_HANDLE_BL;

      easel_stretch_rect (handle, o[0], o[1], o[2], self->text_drag_shown_h, dx, dy, false, &nx, &ny, &nw, &nh);
      if (nx < 0)
        nw += nx, nx = 0;
      if (ny < 0)
        nh += ny, ny = 0;
      nw = CLAMP (nw, 1, pw - nx);
      nh = CLAMP (nh, 1, ph - ny);
      if (!text_set_rect (self, nx, ny, nw, nh) && nw < self->tb_w && text_fits (self, ny, self->tb_w))
        {
          /* Too narrow for the text to stay above the bottom edge: the narrowest width between that fits */
          int lo = nw, hi = self->tb_w, right = nx + nw;

          while (hi - lo > 1)
            {
              int mid = lo + (hi - lo) / 2;

              if (text_fits (self, ny, mid))
                hi = mid;
              else
                lo = mid;
            }
          text_set_rect (self, left ? right - hi : nx, ny, hi, nh);
        }
    }
  g_signal_emit (self, signals[SIGNAL_SELECTION_CHANGED], 0);
  gtk_widget_queue_draw (GTK_WIDGET (self));
}

/* The end of a drag of the box: at the release a changed box is one edit (REQ-0017-11); cancelled (the other button, Esc, Undo), or
   without the memory for the edit, the box is back as at the press. */
static void
text_drag_end (EaselCanvas *self, gboolean cancel)
{
  const int *o = self->text_drag_old;
  int now[4] = { self->tb_x, self->tb_y, self->tb_w, self->tb_set_h };
  gboolean changed = memcmp (now, o, sizeof now) != 0;

  self->text_drag = 0;
  if (changed && (cancel || !easel_text_buffer_push_geometry (self->text_buf, o, now)))
    {
      if (!cancel)
        error_bell (self);                                 /* no memory for the edit: refused (REQ-0017-08) */
      self->tb_x = o[0];
      self->tb_y = o[1];
      self->tb_w = o[2];
      self->tb_set_h = o[3];
      text_relayout (self);
    }
  if (cancel)
    self->cancelled_button = self->sel_button;
  g_signal_emit (self, signals[SIGNAL_STROKING_CHANGED], 0, FALSE);
  g_signal_emit (self, signals[SIGNAL_SELECTION_CHANGED], 0);
  gtk_widget_queue_draw (GTK_WIDGET (self));
}

/* The part of the picture an open box's drawing covers: the box and its ink, limited to the picture and to the part in view, so that a
   frame costs the view's size, not the box's (T010: an 8000-wide box took two frames, most of them composing pixels out of view). FALSE
   when there is no box or none of it is in view. */
static gboolean
text_box_area (EaselCanvas *self, double l, int *bx, int *by, int *x0, int *y0, int *x1, int *y1)
{
  const EaselTextInk *ink = &self->text_ink;
  int bw, bh;
  graphene_rect_t seen;

  if (self->tool != EASEL_TOOL_TEXT || !easel_canvas_text_box (self, bx, by, &bw, &bh))
    return FALSE;
  *x0 = *bx;
  *y0 = *by;
  *x1 = *bx + bw;
  *y1 = *by + bh;
  if (ink->mask != NULL)
    {
      *x0 = MIN (*x0, ink->x);
      *y0 = MIN (*y0, ink->y);
      *x1 = MAX (*x1, ink->x + ink->w);
      *y1 = MAX (*y1, ink->y + ink->h);
    }
  *x0 = MAX (*x0, 0);
  *y0 = MAX (*y0, 0);
  *x1 = MIN (*x1, self->img->width);
  *y1 = MIN (*y1, self->img->height);
  if (gtk_widget_compute_bounds (gtk_widget_get_parent (GTK_WIDGET (self)), GTK_WIDGET (self), &seen))
    {
      *x0 = MAX (*x0, (int) floor (seen.origin.x / l) - 1);
      *y0 = MAX (*y0, (int) floor (seen.origin.y / l) - 1);
      *x1 = MIN (*x1, (int) ceil ((seen.origin.x + seen.size.width) / l) + 1);
      *y1 = MIN (*y1, (int) ceil ((seen.origin.y + seen.size.height) / l) + 1);
    }
  return *x0 < *x1 && *y0 < *y1;
}

/* The open box as the pixels its put-down will write (REQ-0017-05, REQ-0017-16): the picture's pixels under it, the background over the
   box in Opaque mode, the ink's masked pixels. One texture over the tiles, drawn with their filter, so that it is the picture's look at
   every zoom (the Plan); drawn before the grid, the tint and the selected text's highlight (REQ-0017-05's order). */
/* What an open box makes of the picture's color c at (x, y), the box at (bx, by), bw x bh: its background when opaque, then its letters. */
static uint32_t
text_box_pixel (EaselCanvas *self, int x, int y, uint32_t c, int bx, int by, int bw, int bh)
{
  const EaselTextInk *ink = &self->text_ink;

  if (self->text_opaque && x >= bx && x < bx + bw && y >= by && y < by + bh)
    c = self->background;
  if (ink->mask != NULL && x >= ink->x && x < ink->x + ink->w && y >= ink->y && y < ink->y + ink->h
      && ink->mask[(size_t) (y - ink->y) * ink->w + (x - ink->x)])
    c = ink->color[(size_t) (y - ink->y) * ink->w + (x - ink->x)];
  return c;
}

static void
append_text_box (EaselCanvas *self, GtkSnapshot *snapshot, double l)
{
  int bx, by, bw, bh, x0, y0, x1, y1, rw, rh;
  uint32_t *px;
  g_autoptr (GBytes) data = NULL;
  g_autoptr (GdkTexture) texture = NULL;

  if (!text_box_area (self, l, &bx, &by, &x0, &y0, &x1, &y1) || !easel_canvas_text_box (self, &bx, &by, &bw, &bh))
    return;
  rw = x1 - x0;
  rh = y1 - y0;
  px = g_try_malloc ((gsize) rw * rh * sizeof *px);
  if (px == NULL)
    return;
  for (int j = 0; j < rh; j++)
    for (int i = 0; i < rw; i++)
      {
        px[(gsize) j * rw + i] = text_box_pixel (self, x0 + i, y0 + j, easel_image_get (self->img, x0 + i, y0 + j), bx, by, bw, bh);
      }
  /* On the squares drawn under it (REQ-0025-03); with no memory for that, by the renderer's scaling below. */
  if (view_built_by_edges (self) && append_cells_by_edges (self, snapshot, px, x0, y0, rw, rh))
    {
      g_free (px);
      return;
    }
  data = g_bytes_new_take (px, (gsize) rw * rh * sizeof *px);
  texture = gdk_memory_texture_new (rw, rh, PICTURE_FORMAT, data, (gsize) rw * sizeof *px);
  gtk_snapshot_append_scaled_texture (snapshot, texture, GSK_SCALING_FILTER_NEAREST,
                                      &GRAPHENE_RECT_INIT ((float) (x0 * l), (float) (y0 * l), (float) (rw * l), (float) (rh * l)));
}

/* The selected text's highlight (REQ-0017-05): the accent at 35 % over the selected text's line area, except the pixels set in the
   text, which keep the letter's own color; drawn on top of the tint, one rectangle per run of a picture row. */
static void
append_text_highlight (EaselCanvas *self, GtkSnapshot *snapshot, double l)
{
  const EaselTextInk *ink = &self->text_ink;
  int bx, by, x0, y0, x1, y1, rw, rh;
  size_t from = 0, to = 0;
  g_autoptr (GdkRGBA) accent = NULL;
  g_autofree uint8_t *selected = NULL;
  PangoLayoutIter *it;
  GdkRGBA color;

  if (self->text_layout == NULL || !easel_text_buffer_selection (self->text_buf, &from, &to)
      || !text_box_area (self, l, &bx, &by, &x0, &y0, &x1, &y1))
    return;
  rw = x1 - x0;
  rh = y1 - y0;
  selected = g_try_malloc0 ((gsize) rw * rh);
  if (selected == NULL)
    return;
  /* The selected text's area, from the layout's lines */
  it = pango_layout_get_iter (self->text_layout);
  do
    {
      PangoLayoutLine *line = pango_layout_iter_get_line_readonly (it);
      PangoRectangle lr;
      int *ranges = NULL, n = 0;
      int a = MAX ((int) from, line->start_index), z = MIN ((int) to, line->start_index + line->length);

      if (a >= z)
        continue;
      pango_layout_iter_get_line_extents (it, NULL, &lr);
      pango_layout_line_get_x_ranges (line, a, z, &ranges, &n);
      for (int k = 0; k < n; k++)
        for (int yy = by + (int) floor ((double) lr.y / PANGO_SCALE); yy < by + (int) ceil ((double) (lr.y + lr.height) / PANGO_SCALE); yy++)
          for (int xx = bx + (int) floor ((double) ranges[2 * k] / PANGO_SCALE);
               xx < bx + (int) ceil ((double) ranges[2 * k + 1] / PANGO_SCALE); xx++)
            if (xx >= x0 && xx < x1 && yy >= y0 && yy < y1
                && !(ink->mask != NULL && xx >= ink->x && xx < ink->x + ink->w && yy >= ink->y && yy < ink->y + ink->h
                     && ink->mask[(size_t) (yy - ink->y) * ink->w + (xx - ink->x)]))
              selected[(gsize) (yy - y0) * rw + (xx - x0)] = 1;
      g_free (ranges);
    }
  while (pango_layout_iter_next_line (it));
  pango_layout_iter_free (it);
  accent = adw_style_manager_get_accent_color_rgba (adw_style_manager_get_default ());
  color = (GdkRGBA) { accent->red, accent->green, accent->blue, 0.35f };
  for (int j = 0; j < rh; j++)
    for (int i = 0; i < rw;)
      {
        int i0 = i;

        if (!selected[(gsize) j * rw + i])
          {
            i++;
            continue;
          }
        while (i < rw && selected[(gsize) j * rw + i])
          i++;
        gtk_snapshot_append_color (snapshot, &color,
                                   &GRAPHENE_RECT_INIT ((float) ((x0 + i0) * l), (float) ((y0 + j) * l), (float) ((i - i0) * l), (float) l));
      }
}

/* The caret at the place of the next character, in the color of the style at the caret, as tall as its line's text, 1 logical pixel
   wide below 400 % and 2 from 400 % (REQ-0017-05). */
static void
append_text_caret (EaselCanvas *self, GtkSnapshot *snapshot, double l)
{
  PangoRectangle strong;
  EaselTextStyle st;
  GdkRGBA c;
  double w;

  if (self->tool != EASEL_TOOL_TEXT || !self->text_open || self->text_layout == NULL || !self->caret_on || self->guides_hidden)
    return;
  pango_layout_get_cursor_pos (self->text_layout, (int) easel_text_buffer_caret (self->text_buf), &strong, NULL);
  easel_canvas_text_style_at_caret (self, &st);
  c = (GdkRGBA) { ((st.color >> 16) & 255) / 255.0f, ((st.color >> 8) & 255) / 255.0f, (st.color & 255) / 255.0f, 1.0f };
  w = self->zoom >= 400 ? 2 : 1;
  gtk_snapshot_append_color (snapshot, &c,
                             &GRAPHENE_RECT_INIT ((float) ((self->tb_x + (double) strong.x / PANGO_SCALE) * l),
                                                  (float) ((self->tb_y + (double) strong.y / PANGO_SCALE) * l), (float) w,
                                                  (float) ((double) strong.height / PANGO_SCALE * l)));
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
  gboolean by_edges;
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
  by_edges = view_built_by_edges (self);
  pw = (float) (by_edges ? easel_view_edge (self->img->width, self->zoom) / easel_canvas_get_scale (self) : self->img->width * l);
  ph = (float) (by_edges ? easel_view_edge (self->img->height, self->zoom) / easel_canvas_get_scale (self) : self->img->height * l);
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
        if (!by_edges || !append_cells_by_edges (self, snapshot, g_bytes_get_data (data, NULL), x0, y0, rw, rh))
          gtk_snapshot_append_scaled_texture (snapshot, texture, GSK_SCALING_FILTER_NEAREST,
                                              &GRAPHENE_RECT_INIT ((float) (x0 * l), (float) (y0 * l), (float) (rw * l),
                                                                   (float) (rh * l)));
        gtk_snapshot_pop (snapshot);
      }
  }
}

/* Do the dashes of a selection's line move now (REQ-0014-17 c)? A marked or floating selection, or an open text box (REQ-0017-05), with
   the button up, the guides shown and animations on in the user's settings. */
static gboolean
ants_active (EaselCanvas *self)
{
  gboolean on = TRUE;
  int x, y, w, h;

  if (self->guides_hidden)
    return FALSE;
  if (self->tool == EASEL_TOOL_TEXT)
    {
      if (!easel_canvas_text_box (self, &x, &y, &w, &h) || self->text_drag != 0 || self->text_selecting || self->selecting)
        return FALSE;                           /* no box, or a button held on it (a move, a resize, a selection of its text) */
    }
  else if (self->sel_state == 0 || self->sel_moving)
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

/* The tint over the pixels of a selection (REQ-0014-17 b): the accent color of the system's settings at 12 %, over the picture and the grid and under the line, for
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
  GdkRGBA tint = { accent->red, accent->green, accent->blue, self->tint / 100.0f };      /* the system's accent at 12 % (JOD, 2026-10-28) */
  double l = logical_per_pixel (self);
  const uint8_t *mask = selection_mask (self);
  int x, y, w, h;
  gboolean floating;

  if (self->guides_hidden)
    return;                                       /* (a path being traced is no selection yet: nothing is tinted) */
  if (self->tool == EASEL_TOOL_TEXT)
    {
      /* An open box is tinted as a selection is, over its whole rectangle (REQ-0017-05) */
      if (easel_canvas_text_box (self, &x, &y, &w, &h))
        gtk_snapshot_append_color (snapshot, &tint, &GRAPHENE_RECT_INIT ((float) (x * l), (float) (y * l), (float) (w * l), (float) (h * l)));
      return;
    }
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
      /* A line for each part: only a skew makes more than one (REQ-0019-12). */
      size_t at = 0, n;
      const int *part;

      while ((part = easel_path_next_part (&self->sel_path, &at, &n)) != NULL)
        append_path_line (self, snapshot, part, n, self->sel_x - self->sel_path_x, self->sel_y - self->sel_path_y, TRUE,
                          ants_phase (self, 8 * line_thickness (self)));
      return;
    }
  if (self->selecting ? !drag_rect (self, &x, &y, &w, &h)
      : self->tool == EASEL_TOOL_TEXT ? !easel_canvas_text_box (self, &x, &y, &w, &h)
                                      : !easel_canvas_get_selection (self, &x, &y, &w, &h, &floating))
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
      /* A text box's dashes move as a selection's (REQ-0017-05) */
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

/* The line of a free-form selection whose rectangle is (nx, ny, nw, nh) and whose float has the orientation `orient` (REQ-0015-06,
   REQ-0018-07): the path as traced, scaled to the rectangle before the orientation (a quarter turn swaps its width and height), then
   turned and flipped within it. The path as traced is path0, or, before the first stretch or command, the selection's own line. FALSE,
   with out untouched, if the copy would need more than the limit. */
static gboolean
oriented_line (EaselCanvas *self, EaselOrient orient, int nx, int ny, int nw, int nh, EaselPath *out)
{
  gboolean has0 = self->sel_path0.n > 0;
  const EaselPath *src = has0 ? &self->sel_path0 : &self->sel_path;
  int sx = has0 ? self->sel_p0_x : self->sel_path_x, sy = has0 ? self->sel_p0_y : self->sel_path_y;
  int sw = has0 ? self->sel_p0_w : self->sel_w, sh = has0 ? self->sel_p0_h : self->sel_h;
  int pw = easel_orient_swaps (orient) ? nh : nw, ph = easel_orient_swaps (orient) ? nw : nh;
  EaselPath scaled = { 0 };
  gboolean ok;

  if (!easel_lasso_scale_path (src, sx, sy, sw, sh, nx, ny, pw, ph, &scaled, self->sel_lift_max))
    return FALSE;
  ok = easel_lasso_orient_path (&scaled, nx, ny, pw, ph, orient, out, self->sel_lift_max);
  easel_path_free (&scaled);
  return ok;
}

/* A command on the selection could not get its memory: marked again if this command lifted it (the picture was never touched), and the
   dialog of the command (REQ-0018-10). */
static void
transform_refused (EaselCanvas *self, gboolean lifted_here, int kind)
{
  if (lifted_here)
    {
      drop_float (self);
      self->sel_state = 1;
      g_signal_emit (self, signals[SIGNAL_PENDING_CHANGED], 0);
    }
  g_signal_emit (self, signals[SIGNAL_SELECTION_FAILED], 0, kind);
}

gboolean
easel_canvas_stretch_skew_selection (EaselCanvas *self, int W, int H, int amount_h, int amount_v)
{
  gboolean lifted_here = FALSE, stretched, skewed = amount_h != 0 || amount_v != 0;
  EaselFloat *f = &self->sel_float;
  EaselPath line = { 0 };
  int old_w, old_h;
  EaselOrient old_orient;

  if (self->sel_state == 0 || self->img == NULL)
    return FALSE;
  if (W == self->sel_w && H == self->sel_h && !skewed)
    return TRUE;
  if (self->sel_state == 1)
    {
      if (easel_selection_lift (f, self->img, self->sel_x, self->sel_y, self->sel_w, self->sel_h, self->sel_mask, FALSE, hole_color (self),
                                self->sel_lift_max) != EASEL_SELECTION_DONE)
        {
          g_signal_emit (self, signals[SIGNAL_SELECTION_FAILED], 0, 0);      /* the lift's own dialog, REQ-0013-19 */
          return TRUE;
        }
      self->sel_state = 2;
      lifted_here = TRUE;
      g_signal_emit (self, signals[SIGNAL_PENDING_CHANGED], 0);
    }
  old_w = f->w;
  old_h = f->h;
  old_orient = f->orient;
  stretched = W != old_w || H != old_h;
  /* The stretch alone: a free-form line follows, made first, so that a refusal leaves everything as it was. */
  if (!skewed && (self->sel_path.n > 0 || self->sel_path0.n > 0)
      && !oriented_line (self, f->orient, self->sel_x, self->sel_y, W, H, &line))
    {
      transform_refused (self, lifted_here, 7);
      return TRUE;
    }
  if (stretched && easel_selection_stretch_oriented (f, self->sel_x, self->sel_y, W, H, f->orient, self->sel_lift_max) != EASEL_SELECTION_DONE)
    {
      easel_path_free (&line);
      transform_refused (self, lifted_here, 7);
      return TRUE;
    }
  if (skewed && (!easel_lasso_skew_line (f, amount_h, amount_v, &line, self->sel_lift_max)
                 || easel_selection_skew (f, amount_h, amount_v, self->sel_lift_max) != EASEL_SELECTION_DONE))
    {
      easel_path_free (&line);
      /* As before OK: the shown image is made afresh from the same lifted pixels, at most the size it was. */
      if (stretched)
        easel_selection_stretch_oriented (f, self->sel_x, self->sel_y, old_w, old_h, old_orient, SIZE_MAX);
      transform_refused (self, lifted_here, 7);
      return TRUE;
    }
  if (line.n > 0)
    {
      if (skewed)
        easel_path_free (&self->sel_path0);          /* the skewed shape is the new one to scale from */
      else if (self->sel_path0.n == 0)
        {
          self->sel_path0 = self->sel_path;          /* the path as traced, kept to scale afresh from (REQ-0015-06) */
          self->sel_path = (EaselPath) { 0 };
          self->sel_p0_x = self->sel_path_x;
          self->sel_p0_y = self->sel_path_y;
          self->sel_p0_w = self->sel_w;
          self->sel_p0_h = self->sel_h;
        }
      easel_path_free (&self->sel_path);
      self->sel_path = line;
      self->sel_path_x = self->sel_x;
      self->sel_path_y = self->sel_y;
    }
  self->sel_w = f->w;
  self->sel_h = f->h;
  self->sel_epoch++;
  g_signal_emit (self, signals[SIGNAL_SELECTION_CHANGED], 0);
  gtk_widget_queue_draw (GTK_WIDGET (self));
  return TRUE;
}

gboolean
easel_canvas_transform_selection (EaselCanvas *self, EaselOp op)
{
  gboolean lifted_here = FALSE;
  EaselPath line = { 0 };
  int kind = op == EASEL_OP_INVERT ? 6 : (op == EASEL_OP_FLIP_H || op == EASEL_OP_FLIP_V) ? 4 : 5;

  if (self->sel_state == 0 || self->img == NULL || op > EASEL_OP_INVERT)
    return FALSE;
  if (self->sel_state == 1)
    {
      if (easel_selection_lift (&self->sel_float, self->img, self->sel_x, self->sel_y, self->sel_w, self->sel_h, self->sel_mask,
                                FALSE, hole_color (self), self->sel_lift_max) != EASEL_SELECTION_DONE)
        {
          g_signal_emit (self, signals[SIGNAL_SELECTION_FAILED], 0, 0);      /* REQ-0013-19: the selection stays marked */
          return TRUE;
        }
      self->sel_state = 2;
      lifted_here = TRUE;
      g_signal_emit (self, signals[SIGNAL_PENDING_CHANGED], 0);
    }
  if (op != EASEL_OP_INVERT && (self->sel_path.n > 0 || self->sel_path0.n > 0))
    {
      /* A free-form selection: its line follows (REQ-0018-07), made first, so that a refusal leaves everything as it was. */
      EaselOrient step = easel_op_orient (op), o2 = easel_orient_compose (self->sel_float.orient, step);
      gboolean quarter = easel_orient_swaps (step);

      if (!oriented_line (self, o2, self->sel_x, self->sel_y, quarter ? self->sel_h : self->sel_w, quarter ? self->sel_w : self->sel_h,
                          &line))
        {
          transform_refused (self, lifted_here, kind);
          return TRUE;
        }
    }
  if (easel_selection_transform (&self->sel_float, op, self->sel_lift_max) != EASEL_SELECTION_DONE)
    {
      easel_path_free (&line);
      transform_refused (self, lifted_here, kind);
      return TRUE;
    }
  if (line.n > 0)
    {
      if (self->sel_path0.n == 0)
        {
          self->sel_path0 = self->sel_path;          /* the path as traced, kept to scale afresh from (REQ-0015-06) */
          self->sel_path = (EaselPath) { 0 };
          self->sel_p0_x = self->sel_path_x;
          self->sel_p0_y = self->sel_path_y;
          self->sel_p0_w = self->sel_w;
          self->sel_p0_h = self->sel_h;
        }
      easel_path_free (&self->sel_path);
      self->sel_path = line;
      self->sel_path_x = self->sel_x;
      self->sel_path_y = self->sel_y;
    }
  self->sel_w = self->sel_float.w;
  self->sel_h = self->sel_float.h;
  self->sel_epoch++;
  g_signal_emit (self, signals[SIGNAL_SELECTION_CHANGED], 0);
  gtk_widget_queue_draw (GTK_WIDGET (self));
  return TRUE;
}

/* The one function that puts a floating selection down and removes the selection (REQ-0013-11, -12); keep_marked keeps a
   marked one (Save, Save As). FALSE if the put-down could not get its memory: the picture is as before the lift and the
   selection is gone (REQ-0013-19). */
/* The picture as it was before a grown paste, given back (Undo while the paste floats, a put-down that fails: REQ-0016-07). */
static void
restore_paste_base (EaselCanvas *self)
{
  uint32_t *px;

  if (self->paste_base == NULL)
    return;
  px = self->img->px;
  self->img->px = self->paste_base->px;
  self->img->width = self->paste_base->width;
  self->img->height = self->paste_base->height;
  self->paste_base->px = px;
  easel_image_free (self->paste_base);
  self->paste_base = NULL;
  g_signal_emit (self, signals[SIGNAL_PICTURE_REPLACED], 0);
}

/* The picture as it was before a grown paste is no longer needed (the paste was put down or cleared). */
static void
drop_paste_base (EaselCanvas *self)
{
  g_clear_pointer (&self->paste_base, easel_image_free);
}

/* The float put down (with its pixels or, for a clear, only its copies), as one step; a grown paste's step holds the growth. */
static EaselSelectionResult
put_down_float (EaselCanvas *self, gboolean with_float)
{
  if (self->paste_base != NULL)
    return easel_selection_put_down_grown (&self->sel_float, with_float, self->img, self->history, self->paste_base->width,
                                           self->paste_base->height, self->paste_fill, self->sel_step_max);
  return easel_selection_put_down (&self->sel_float, with_float, self->img, self->history, self->sel_step_max);
}

static gboolean
finish_selection (EaselCanvas *self, gboolean keep_marked)
{
  gboolean ok = TRUE;

  if (self->sel_state == 2)
    {
      EaselSelectionResult r;

      easel_selection_complete (&self->sel_float);   /* a stretch still held: the whole image is made before it is laid */
      r = put_down_float (self, TRUE);

      ok = r != EASEL_SELECTION_NO_MEMORY;
      if (!ok)
        {
          restore_paste_base (self);                 /* a grown paste: the picture as before it, its size too (REQ-0016-07) */
          g_signal_emit (self, signals[SIGNAL_SELECTION_FAILED], 0, 0);     /* the dialog comes after the action goes on */
        }
      drop_paste_base (self);
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

  if (self->sel_path0.n == 0
      || !oriented_line (self, self->stretch_orient0, self->stretch_sx, self->stretch_sy, self->stretch_sw, self->stretch_sh, &scaled))
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
      if ((self->sel_float.w != self->stretch_sw || self->sel_float.h != self->stretch_sh
           || self->sel_float.orient != self->stretch_orient0)
          && easel_selection_stretch_oriented (&self->sel_float, self->stretch_sx, self->stretch_sy, self->stretch_sw, self->stretch_sh,
                                               self->stretch_orient0, self->sel_lift_max) != EASEL_SELECTION_DONE)
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
      self->sel_for_text = FALSE;
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
  restore_paste_base (self);                     /* Undo while a grown paste floats: the growth goes too (REQ-0016-07) */
  drop_float (self);
  g_clear_pointer (&self->paste_base, easel_image_free);
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
    r = put_down_float (self, FALSE);              /* a grown paste: the growth stays, with its copies (REQ-0016-05) */
  if (r == EASEL_SELECTION_NO_MEMORY)
    {
      g_signal_emit (self, signals[SIGNAL_SELECTION_FAILED], 0, 1);          /* nothing changed, the selection stays */
      return TRUE;
    }
  drop_paste_base (self);
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

void
easel_canvas_top_left_in_view (EaselCanvas *self, int *x, int *y)
{
  double l = logical_per_pixel (self);
  graphene_rect_t seen;

  *x = *y = 0;
  if (self->img == NULL || !gtk_widget_compute_bounds (gtk_widget_get_parent (GTK_WIDGET (self)), GTK_WIDGET (self), &seen))
    return;
  /* The pixel that holds the top-left point of the part of the picture in view; the margin before the picture counts as (0, 0). */
  *x = (int) CLAMP (floor (seen.origin.x / l), 0, self->img->width - 1);
  *y = (int) CLAMP (floor (seen.origin.y / l), 0, self->img->height - 1);
}

/* Whether `bytes` can be had: within the selection's limit and from the system (taken and let go at once). */
static gboolean
can_have (EaselCanvas *self, size_t bytes)
{
  void *probe;

  if (bytes > self->sel_lift_max)
    return FALSE;
  probe = g_try_malloc (bytes);
  g_free (probe);
  return probe != NULL;
}

gboolean
easel_canvas_can_paste (EaselCanvas *self, int pic_w, int pic_h, int grown_w, int grown_h)
{
  if (pic_w < 1 || pic_h < 1 || (size_t) pic_w > SIZE_MAX / 4 / (size_t) pic_h)
    return FALSE;
  if (!can_have (self, (size_t) pic_w * (size_t) pic_h * 4))
    return FALSE;
  if (grown_w > 0 && ((size_t) grown_w > SIZE_MAX / 4 / (size_t) grown_h || !can_have (self, (size_t) grown_w * (size_t) grown_h * 4)))
    return FALSE;
  return TRUE;
}

size_t
easel_canvas_get_selection_limit (EaselCanvas *self)
{
  return self->sel_lift_max;
}

gboolean
easel_canvas_paste_grown (EaselCanvas *self, EaselImage *pic, int width, int height, uint32_t fill)
{
  EaselFloat f = { 0 };
  EaselImage *grown;
  int old_w = self->img->width, old_h = self->img->height;

  /* Everything had first, so a refusal changes nothing (REQ-0016-11): the paste's pixels and the grown picture. */
  if (easel_selection_paste (&f, pic, 0, 0, self->sel_lift_max) != EASEL_SELECTION_DONE)
    {
      easel_image_free (pic);
      return FALSE;
    }
  easel_image_free (pic);
  grown = can_have (self, (size_t) width * (size_t) height * 4) ? easel_image_new (width, height, fill) : NULL;
  if (grown == NULL)
    {
      easel_float_free (&f);
      return FALSE;
    }
  finish_selection (self, FALSE);              /* on the picture as it was */
  for (int y = 0; y < old_h; y++)
    memcpy (grown->px + (size_t) y * (size_t) width, self->img->px + (size_t) y * (size_t) old_w, (size_t) old_w * sizeof *grown->px);
  /* The picture takes the grown pixels; the old ones are kept as they were, for Undo while the paste floats and for a put-down that
     fails, which give them back without allocating (Plan, "Where it is put down"). */
  {
    uint32_t *px = self->img->px;

    self->img->px = grown->px;
    self->img->width = width;
    self->img->height = height;
    grown->px = px;
    grown->width = old_w;
    grown->height = old_h;
  }
  self->paste_base = grown;
  self->paste_fill = fill;
  g_signal_emit (self, signals[SIGNAL_PICTURE_REPLACED], 0);
  shape_free (self);
  self->sel_float = f;
  self->sel_state = 2;
  self->sel_x = 0;
  self->sel_y = 0;
  self->sel_w = f.w;
  self->sel_h = f.h;
  self->sel_lifted_here = FALSE;
  g_signal_emit (self, signals[SIGNAL_PENDING_CHANGED], 0);
  g_signal_emit (self, signals[SIGNAL_SELECTION_CHANGED], 0);
  gtk_widget_queue_draw (GTK_WIDGET (self));
  return TRUE;
}

gboolean
easel_canvas_paste (EaselCanvas *self, EaselImage *pic, int x, int y)
{
  EaselFloat f = { 0 };

  /* The paste's pixels first: if they cannot be had nothing is put down (REQ-0016-11). */
  if (easel_selection_paste (&f, pic, x, y, self->sel_lift_max) != EASEL_SELECTION_DONE)
    {
      easel_image_free (pic);
      return FALSE;
    }
  easel_image_free (pic);
  finish_selection (self, FALSE);              /* a floating selection put down, a marked one removed (REQ-0016-05) */
  shape_free (self);
  self->sel_float = f;
  self->sel_state = 2;
  self->sel_x = f.x;
  self->sel_y = f.y;
  self->sel_w = f.w;
  self->sel_h = f.h;
  self->sel_lifted_here = FALSE;
  g_signal_emit (self, signals[SIGNAL_PENDING_CHANGED], 0);
  g_signal_emit (self, signals[SIGNAL_SELECTION_CHANGED], 0);
  gtk_widget_queue_draw (GTK_WIDGET (self));
  return TRUE;
}

EaselImage *
easel_canvas_copy_selection (EaselCanvas *self)
{
  if (self->sel_state == 1)
    return easel_selection_copy_image (self->img, NULL, self->sel_x, self->sel_y, self->sel_w, self->sel_h, self->sel_mask,
                                       self->sel_lift_max);
  if (self->sel_state == 2)
    {
      easel_selection_complete (&self->sel_float);   /* a stretch still held: the copy has the whole image */
      return easel_selection_copy_image (self->img, &self->sel_float, self->sel_float.x, self->sel_float.y, self->sel_float.w,
                                         self->sel_float.h, NULL, self->sel_lift_max);
    }
  return NULL;
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
  self->stretch_orient0 = self->sel_state == 2 ? self->sel_float.orient : EASEL_ORIENT_IDENTITY;
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
  bool mirror_x, mirror_y;
  EaselOrient orient = self->stretch_orient0;

  easel_stretch_rect_mirror ((EaselSelHandle) self->stretch_handle, self->stretch_sx, self->stretch_sy, self->stretch_sw, self->stretch_sh,
                             dx, dy, self->shift, &nx, &ny, &nw, &nh, &mirror_x, &mirror_y);
  if (mirror_x)
    orient = easel_orient_compose (orient, EASEL_ORIENT_FLIP_X);
  if (mirror_y)
    orient = easel_orient_compose (orient, EASEL_ORIENT_FLIP_Y);
  if (nx == self->sel_x && ny == self->sel_y && nw == self->sel_w && nh == self->sel_h)
    return;                                       /* no movement to another size (a mirrored rectangle is never the same one) */
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
  if ((self->zoom >= 100 ? easel_selection_stretch_lazy_oriented (&self->sel_float, nx, ny, nw, nh, orient, self->sel_lift_max)
                         : easel_selection_stretch_oriented (&self->sel_float, nx, ny, nw, nh, orient, self->sel_lift_max))
      != EASEL_SELECTION_DONE)
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
      if (!oriented_line (self, orient, nx, ny, nw, nh, &scaled))
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
  gboolean click = self->sel_for_text ? !self->text_dragged : self->sel_px1 == self->sel_px0 && self->sel_py1 == self->sel_py0;

  self->selecting = FALSE;
  shape_free (self);
  if (self->sel_for_text)
    {
      self->sel_state = 0;
      text_open_from_drag (self, click);           /* while sel_for_text holds: no Shift for a box (drag_rect) */
      self->sel_for_text = FALSE;
      g_signal_emit (self, signals[SIGNAL_STROKING_CHANGED], 0, FALSE);
      g_signal_emit (self, signals[SIGNAL_SELECTION_CHANGED], 0);
      gtk_widget_queue_draw (GTK_WIDGET (self));
      return;
    }
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
  int edge_zoom;    /* not 0: the view is built by the edges at this zoom, and an axis line lies on a cell's edge (REQ-0025-02) */
} GridDraw;

/* Whether the view is built here, cell by cell, and not left to the renderer's scaling (REQ-0025-02). At a fractional zoom the renderers put
   an edge halfway between two device pixels either way, by the row and the scroll (0025's Plan, the probes). On a screen of scale 2 a scaled
   texture with NEAREST mixes neighbouring pixels at any zoom (a bare GTK program does it, under Cairo and Vulkan), and a plain one does not.
   Below 100 % the view stays as it is, and so does a fractional screen scale, untested (the Plan). */
static gboolean
view_built_by_edges (EaselCanvas *self)
{
  return easel_view_built_by_edges (self->zoom, easel_canvas_get_scale (self));
}

/* An allocation of a view built on the edges (REQ-0025-03; the edge case of 0025): NULL without memory, or past the test hook's limit. */
static gpointer
view_try_malloc (EaselCanvas *self, gsize count, gsize size)
{
  if (count > self->view_memory_max / size)
    return NULL;
  return g_try_malloc_n (count, size);
}

/* Pixels of the picture's grid (the rectangle x0, y0, rw, rh of px, which has rw pixels to a row) as device pixels on the drawn edges, only what
   the view shows (REQ-0025-03): what lies over the picture falls on its squares. FALSE without memory, with nothing drawn: the caller
   draws them by the renderer's scaling, for this frame. */
static gboolean
append_cells_by_edges (EaselCanvas *self, GtkSnapshot *snapshot, const uint32_t *px, int x0, int y0, int rw, int rh)
{
  double scale = easel_canvas_get_scale (self);
  int zoom = self->zoom;
  graphene_rect_t seen;
  int dx0, dy0, dx1, dy1, vw, vh, prev_y = INT_MIN;
  uint32_t *dst;
  int *xk;
  g_autoptr (GBytes) bytes = NULL;
  g_autoptr (GdkTexture) texture = NULL;

  if (!gtk_widget_compute_bounds (gtk_widget_get_parent (GTK_WIDGET (self)), GTK_WIDGET (self), &seen))
    return TRUE;
  dx0 = MAX (easel_view_edge (x0, zoom), (int) floor (seen.origin.x * scale));
  dy0 = MAX (easel_view_edge (y0, zoom), (int) floor (seen.origin.y * scale));
  dx1 = MIN (easel_view_edge (x0 + rw, zoom), (int) ceil ((seen.origin.x + seen.size.width) * scale));
  dy1 = MIN (easel_view_edge (y0 + rh, zoom), (int) ceil ((seen.origin.y + seen.size.height) * scale));
  vw = dx1 - dx0;
  vh = dy1 - dy0;
  if (vw <= 0 || vh <= 0)
    return TRUE;
  dst = view_try_malloc (self, (gsize) vw * (gsize) vh, sizeof (uint32_t));
  xk = view_try_malloc (self, vw, sizeof (int));
  if (dst == NULL || xk == NULL)
    {
      g_free (dst);
      g_free (xk);
      return FALSE;
    }
  for (int i = 0; i < vw; i++)
    xk[i] = easel_view_cell_at (dx0 + i, zoom) - x0;
  for (int r = 0; r < vh; r++)
    {
      int ky = easel_view_cell_at (dy0 + r, zoom);
      uint32_t *row = dst + (gsize) r * vw;

      if (ky == prev_y)
        memcpy (row, row - vw, (gsize) vw * sizeof (uint32_t));
      else
        {
          const uint32_t *src = px + (gsize) (ky - y0) * rw;

          for (int i = 0; i < vw; i++)
            row[i] = src[xk[i]];
        }
      prev_y = ky;
    }
  g_free (xk);
  bytes = g_bytes_new_take (dst, (gsize) vw * (gsize) vh * sizeof (uint32_t));
  texture = gdk_memory_texture_new (vw, vh, PICTURE_FORMAT, bytes, (gsize) vw * sizeof (uint32_t));
  gtk_snapshot_append_texture (snapshot, texture, &GRAPHENE_RECT_INIT ((float) (dx0 / scale), (float) (dy0 / scale), (float) (vw / scale), (float) (vh / scale)));
  return TRUE;
}

/* The part of the picture in view at device size, each device pixel from the cell the edge rule puts there, through a source index per device
   column (0025's Plan, T001). While a preview replaces some tiles their pixels are read from those; a repaint with nothing changed reuses the
   last texture. FALSE without memory, with nothing drawn: the caller draws the tiles by the renderer's scaling, for this frame. */
static gboolean
append_view_by_edges (EaselCanvas *self, GtkSnapshot *snapshot, GdkTexture **preview)
{
  const EaselImage *img = self->img;
  double scale = easel_canvas_get_scale (self);
  int zoom = self->zoom;
  graphene_rect_t seen;
  int x0, y0, x1, y1, vw, vh;

  if (!gtk_widget_compute_bounds (gtk_widget_get_parent (GTK_WIDGET (self)), GTK_WIDGET (self), &seen))
    return TRUE;
  x0 = MAX (0, (int) floor (seen.origin.x * scale));
  y0 = MAX (0, (int) floor (seen.origin.y * scale));
  x1 = MIN (easel_view_edge (img->width, zoom), (int) ceil ((seen.origin.x + seen.size.width) * scale));
  y1 = MIN (easel_view_edge (img->height, zoom), (int) ceil ((seen.origin.y + seen.size.height) * scale));
  vw = x1 - x0;
  vh = y1 - y0;
  if (vw <= 0 || vh <= 0)
    return TRUE;
  if (preview != NULL || !self->view_kept || self->view_tex == NULL || self->view_gen != self->tiles_gen || self->view_zoom != zoom
      || self->view_x != x0 || self->view_y != y0 || self->view_w != vw || self->view_h != vh)
    {
      uint32_t *dst = view_try_malloc (self, (gsize) vw * (gsize) vh, sizeof (uint32_t));
      int *xk = view_try_malloc (self, vw, sizeof (int));
      int prev_y = -1;
      int first_col = 0, n_cols = 0;
      const uint32_t **from = NULL;
      GBytes **held = NULL;
      gsize *strides = NULL;
      g_autoptr (GBytes) bytes = NULL;
      gboolean have_memory = dst != NULL && xk != NULL;

      if (have_memory)
        for (int i = 0; i < vw; i++)
          xk[i] = easel_view_cell_at (x0 + i, zoom);
      if (have_memory && preview != NULL)
        {
          first_col = xk[0] / EASEL_CANVAS_TILE;
          n_cols = xk[vw - 1] / EASEL_CANVAS_TILE - first_col + 1;
          from = g_try_new (const uint32_t *, n_cols);
          held = g_try_new0 (GBytes *, (gsize) self->tile_cols * (gsize) self->tile_rows);
          strides = g_try_new (gsize, (gsize) self->tile_cols * (gsize) self->tile_rows);
          have_memory = from != NULL && held != NULL && strides != NULL;
        }
      if (!have_memory)
        {
          g_free (dst);
          g_free (xk);
          g_free (from);
          g_free (held);
          g_free (strides);
          return FALSE;
        }
      for (int r = 0; r < vh; r++)
        {
          int ky = easel_view_cell_at (y0 + r, zoom);
          uint32_t *row = dst + (gsize) r * vw;

          if (ky == prev_y)
            memcpy (row, row - vw, (gsize) vw * sizeof (uint32_t));
          else if (preview == NULL)
            {
              const uint32_t *src = img->px + (gsize) ky * img->width;

              for (int i = 0; i < vw; i++)
                row[i] = src[xk[i]];
            }
          else
            {
              int ty = ky / EASEL_CANVAS_TILE;

              for (int c = 0; c < n_cols; c++)
                {
                  int id = ty * self->tile_cols + first_col + c;

                  if (preview[id] == NULL)
                    from[c] = img->px + (gsize) ky * img->width + (gsize) (first_col + c) * EASEL_CANVAS_TILE;
                  else
                    {
                      if (held[id] == NULL)
                        {
                          g_autoptr (GdkTextureDownloader) dl = gdk_texture_downloader_new (preview[id]);

                          gdk_texture_downloader_set_format (dl, PICTURE_FORMAT);
                          held[id] = gdk_texture_downloader_download_bytes (dl, &strides[id]);
                        }
                      from[c] = (const uint32_t *) ((const guchar *) g_bytes_get_data (held[id], NULL)
                                                    + (gsize) (ky % EASEL_CANVAS_TILE) * strides[id]);
                    }
                }
              for (int i = 0; i < vw; i++)
                row[i] = from[xk[i] / EASEL_CANVAS_TILE - first_col][xk[i] % EASEL_CANVAS_TILE];
            }
          prev_y = ky;
        }
      if (preview != NULL)
        {
          for (int i = 0; i < self->tile_cols * self->tile_rows; i++)
            g_clear_pointer (&held[i], g_bytes_unref);
          g_free (held);
          g_free (strides);
          g_free (from);
        }
      g_free (xk);
      bytes = g_bytes_new_take (dst, (gsize) vw * (gsize) vh * sizeof (uint32_t));
      g_clear_object (&self->view_tex);
      self->view_tex = gdk_memory_texture_new (vw, vh, PICTURE_FORMAT, bytes, (gsize) vw * sizeof (uint32_t));
      self->view_kept = preview == NULL;
      self->view_gen = self->tiles_gen;
      self->view_zoom = zoom;
      self->view_x = x0;
      self->view_y = y0;
      self->view_w = vw;
      self->view_h = vh;
    }
  /* A plain texture at whole device pixels: exact, where a scaled one with NEAREST is not on every screen. */
  gtk_snapshot_append_texture (snapshot, self->view_tex,
                               &GRAPHENE_RECT_INIT ((float) (x0 / scale), (float) (y0 / scale), (float) (vw / scale), (float) (vh / scale)));
  return TRUE;
}

/* Picture coordinates to canvas coordinates, putting the line on a whole device pixel: a vertical or horizontal line
   covers the device pixel that starts at the boundary, and any other line the pixels it passes through. */
static void
grid_segment (double x0, double y0, double x1, double y1, void *data)
{
  GridDraw *d = data;
  double ax = x0 * d->l, ay = y0 * d->l, bx = x1 * d->l, by = y1 * d->l;
  double half = 0.5 / d->scale;

  if (x0 == x1)
    ax = bx = (d->edge_zoom != 0 ? easel_view_edge ((int) lround (x0), d->edge_zoom) : floor (ax * d->scale)) / d->scale + half;
  else if (y0 == y1)
    ay = by = (d->edge_zoom != 0 ? easel_view_edge ((int) lround (y0), d->edge_zoom) : floor (ay * d->scale)) / d->scale + half;
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
  GridDraw d = { NULL, l, scale, view_built_by_edges (self) ? self->zoom : 0 };

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

  /* The handles of an open text box with Text, else the selection's (REQ-0017-05, REQ-0015-01). */
  if (self->img == NULL)
    return FALSE;
  if (self->tool == EASEL_TOOL_TEXT ? !easel_canvas_text_box (self, &sx, &sy, &sw, &sh)
                                    : !easel_canvas_get_selection (self, &sx, &sy, &sw, &sh, &floating))
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
  if (self->key_pressing)
    return -1;                                  /* a keyboard press acts on the pixel only (REQ-0022-07) */
  static const EaselSelHandle order[] = { EASEL_SEL_HANDLE_TL, EASEL_SEL_HANDLE_TR, EASEL_SEL_HANDLE_BL, EASEL_SEL_HANDLE_BR,
                                          EASEL_SEL_HANDLE_T, EASEL_SEL_HANDLE_L, EASEL_SEL_HANDLE_R, EASEL_SEL_HANDLE_B };
  graphene_rect_t r;
  double l = logical_per_pixel (self);
  int sx, sy, sw, sh;
  gboolean floating;

  if (self->img == NULL)
    return -1;
  if (self->tool == EASEL_TOOL_TEXT)
    {
      /* A box's handles: inside the box a press is always the text's, also where a picture handle's area reaches in (REQ-0017-09). */
      if (!easel_canvas_text_box (self, &sx, &sy, &sw, &sh)
          || (x >= sx * l && x < (sx + sw) * l && y >= sy * l && y < (sy + sh) * l))
        return -1;
    }
  else if (!easel_tool_selects (self->tool) || !easel_canvas_get_selection (self, &sx, &sy, &sw, &sh, &floating))
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

  if (self->guides_hidden || self->no_sel_handles || !(easel_tool_selects (self->tool) || self->tool == EASEL_TOOL_TEXT))
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

gboolean
easel_canvas_smear_selection (EaselCanvas *self, int dx, int dy)
{
  gboolean lifted = FALSE;

  if (self->sel_state == 0 || easel_canvas_is_stroking (self))
    return FALSE;
  if (self->sel_state == 1)
    {
      /* The first key lifts it with no hole, as a Shift drag does (REQ-0014-11). */
      if (easel_selection_lift (&self->sel_float, self->img, self->sel_x, self->sel_y, self->sel_w, self->sel_h, self->sel_mask,
                                TRUE, hole_color (self), self->sel_lift_max) != EASEL_SELECTION_DONE)
        {
          g_signal_emit (self, signals[SIGNAL_SELECTION_FAILED], 0, 0);
          return TRUE;
        }
      lifted = TRUE;
      self->sel_state = 2;
    }
  if (!easel_selection_smear (&self->sel_float, self->sel_x + dx, self->sel_y + dy, self->sel_lift_max))
    {
      /* As it was before this key: marked if this key lifted it, the copies of earlier keys kept (REQ-0014-13). */
      if (lifted)
        {
          drop_float (self);
          self->sel_state = 1;
        }
      g_signal_emit (self, signals[SIGNAL_SELECTION_FAILED], 0, 0);
      gtk_widget_queue_draw (GTK_WIDGET (self));
      return TRUE;
    }
  self->sel_x = self->sel_float.x;
  self->sel_y = self->sel_float.y;
  if (lifted)
    g_signal_emit (self, signals[SIGNAL_PENDING_CHANGED], 0);
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
/* The figure shown over the picture now and its two colors: the waiting one or the one being dragged; none while a gradient waits, which replaces the
   tiles instead (gradient_preview). */
static gboolean
shown_figure (EaselCanvas *self, EaselFigure *f, uint32_t *draw, uint32_t *other)
{
  if (self->pending && self->pend_gradient)
    return FALSE;
  if (self->pending)
    {
      *f = pending_figure (self);
      *draw = self->pend_draw;
      *other = self->pend_other;
      return TRUE;
    }
  if (self->shaping)
    {
      *f = current_figure (self);
      *draw = self->shape_draw;
      *other = self->shape_other;
      return TRUE;
    }
  return FALSE;
}

/* The color of one run of a figure, FALSE for a run that draws nothing. */
static gboolean
figure_run_color (const EaselFigure *f, const EaselFigureRun *run, uint32_t draw, uint32_t other, uint32_t *color)
{
  if (!run->inside)
    *color = draw;
  else if (f->style == EASEL_FIGURE_OUTLINE_AND_FILL)
    *color = other;
  else if (f->style == EASEL_FIGURE_FILL_ONLY)
    *color = draw;
  else
    return FALSE;
  return TRUE;
}

static void
append_shape_preview (EaselCanvas *self, GtkSnapshot *snapshot)
{
  EaselFigure f;
  EaselFigureRun *runs;
  graphene_rect_t seen, picture;
  gboolean by_edges;
  double scale;
  double l = self->zoom / (100.0 * easel_canvas_get_scale (self));
  int row0, row1, step;

  uint32_t draw, other;

  if (!shown_figure (self, &f, &draw, &other))
    return;
  if (!gtk_widget_compute_bounds (gtk_widget_get_parent (GTK_WIDGET (self)), GTK_WIDGET (self), &seen))
    return;
  by_edges = view_built_by_edges (self);
  scale = easel_canvas_get_scale (self);
  picture = GRAPHENE_RECT_INIT (0, 0, (float) (self->img->width * l), (float) (self->img->height * l));
  if (by_edges)
    picture = GRAPHENE_RECT_INIT (0, 0, (float) (easel_view_edge (self->img->width, self->zoom) / scale),
                                  (float) (easel_view_edge (self->img->height, self->zoom) / scale));
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

          if (x0 > x1 || !figure_run_color (&f, &runs[i], draw, other, &color))
            continue;
          r = GRAPHENE_RECT_INIT ((float) (x0 * l), (float) (y * l), (float) ((x1 - x0 + 1) * l), (float) (step * l));
          if (by_edges)
            {
              /* On the squares drawn under it (REQ-0025-03). */
              int ax = easel_view_edge (x0, self->zoom), ay = easel_view_edge (y, self->zoom);

              r = GRAPHENE_RECT_INIT ((float) (ax / scale), (float) (ay / scale), (float) ((easel_view_edge (x1 + 1, self->zoom) - ax) / scale),
                                      (float) ((easel_view_edge (y + step, self->zoom) - ay) / scale));
            }
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

double
easel_canvas_get_frame_ms (EaselCanvas *self)
{
  return self->frame_ms;
}

void
easel_canvas_set_draw_delay (EaselCanvas *self, int ms)
{
  self->draw_delay_ms = ms;
}

guint
easel_canvas_get_frames_drawn (EaselCanvas *self)
{
  return self->frames_drawn;
}

void
easel_canvas_set_tint (EaselCanvas *self, int percent)
{
  self->tint = percent;
  gtk_widget_queue_draw (GTK_WIDGET (self));
}

int
easel_canvas_get_tint (EaselCanvas *self)
{
  return self->tint;
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

const EaselImage *
easel_canvas_get_image (EaselCanvas *self)
{
  return self->img;
}

void
easel_canvas_append_checkerboard (GtkSnapshot *snapshot, const graphene_rect_t *bounds)
{
  easel_snapshot_append_checkerboard (snapshot, bounds, CHECKER_SIZE);
}

/* The w x h picture pixels at (x0, y0) as the put-down would write them, to px (REQ-0021-07, REQ-0022-13). */
static void
compose_pixels (EaselCanvas *self, int x0, int y0, int w, int h, uint32_t *px)
{
  EaselFigure f;
  uint32_t draw, other;
  int bx, by, bw, bh;

  if (self->sel_state == 2)
    easel_selection_lay (&self->sel_float, TRUE, self->img, x0, y0, w, h, px, (size_t) w);
  else if (self->pending && self->pend_gradient)
    {
      EaselGradient g = shown_gradient (self);

      easel_gradient_lay (&g, self->img, x0, y0, w, h, px, (size_t) w);
    }
  else
    for (int j = 0; j < h; j++)
      for (int i = 0; i < w; i++)
        px[(gsize) j * w + i] = easel_image_get (self->img, x0 + i, y0 + j);
  if (self->tool == EASEL_TOOL_TEXT && easel_canvas_text_box (self, &bx, &by, &bw, &bh))
    for (int j = 0; j < h; j++)
      for (int i = 0; i < w; i++)
        px[(gsize) j * w + i] = text_box_pixel (self, x0 + i, y0 + j, px[(gsize) j * w + i], bx, by, bw, bh);
  if (shown_figure (self, &f, &draw, &other) && easel_figure_prepare (&f, y0, y0 + h - 1))
    {
      EaselFigureRun *runs = malloc (easel_figure_max_runs (&f) * sizeof *runs);

      if (runs != NULL)
        for (int y = y0; y < y0 + h; y++)
          {
            size_t n = easel_figure_row (&f, y, runs, easel_figure_max_runs (&f));

            for (size_t r = 0; r < n; r++)
              {
                uint32_t color;

                if (figure_run_color (&f, &runs[r], draw, other, &color))
                  for (int x = MAX (runs[r].x0, x0); x <= MIN (runs[r].x1, x0 + w - 1); x++)
                    px[(gsize) (y - y0) * w + (x - x0)] = color;
              }
          }
      free (runs);
      easel_figure_release (&f);
    }
}

GdkTexture *
easel_canvas_compose_view (EaselCanvas *self, int x0, int y0, int w, int h)
{
  size_t bytes = (size_t) w * (size_t) h * sizeof (uint32_t);
  g_autofree uint32_t *px = g_try_malloc (bytes);
  g_autoptr (GBytes) data = NULL;

  if (px == NULL)
    return NULL;
  compose_pixels (self, x0, y0, w, h, px);
  data = g_bytes_new_take (g_steal_pointer (&px), bytes);
  return gdk_memory_texture_new (w, h, PICTURE_FORMAT, data, (gsize) w * sizeof (uint32_t));
}

uint32_t
easel_canvas_marker_color (EaselCanvas *self)
{
  uint32_t c = 0;

  if (self->img != NULL)
    compose_pixels (self, self->marker_x, self->marker_y, 1, 1, &c);
  return c;
}

static gboolean
emit_painted (gpointer data)
{
  EaselCanvas *self = data;

  self->painted_idle = 0;
  g_signal_emit (self, signals[SIGNAL_PAINTED], 0);
  return G_SOURCE_REMOVE;
}

static void
canvas_snapshot_frame (EaselCanvas *self, GtkSnapshot *snapshot);

/* ---- The keyboard's marker (0022) ---- */

static void mouse_takes_marker (EaselCanvas *self, double x, double y);
void easel_canvas_marker_bounds (EaselCanvas *self, double *x0, double *y0, double *x1, double *y1);

/* While the marker shows, the hover of the position and the Magnifier's box is its (REQ-0022-11). */
static void
marker_hover (EaselCanvas *self)
{
  double x0, y0, x1, y1;
  gboolean pressing = self->key_pressing;

  if (!self->marker_on || self->img == NULL)
    return;
  easel_canvas_marker_bounds (self, &x0, &y0, &x1, &y1);
  self->key_pressing = TRUE;
  easel_canvas_pointer_at (self, (x0 + x1) / 2, (y0 + y1) / 2);
  self->key_pressing = pressing;
}

void
easel_canvas_marker_show (EaselCanvas *self, gboolean shown)
{
  if (shown == self->marker_on)
    return;
  self->marker_on = shown;
  marker_hover (self);
  gtk_widget_queue_draw (GTK_WIDGET (self));
}

gboolean
easel_canvas_marker_shown (EaselCanvas *self)
{
  return self->marker_on;
}

void
easel_canvas_marker_get (EaselCanvas *self, int *x, int *y)
{
  *x = self->marker_x;
  *y = self->marker_y;
}

void
easel_canvas_marker_set (EaselCanvas *self, int x, int y)
{
  self->marker_x = x;
  self->marker_y = y;
  self->marker_placed = TRUE;
  marker_hover (self);
  gtk_widget_queue_draw (GTK_WIDGET (self));
}

gboolean
easel_canvas_marker_is_set (EaselCanvas *self)
{
  return self->marker_placed;
}

void
easel_canvas_marker_reset (EaselCanvas *self)
{
  self->marker_placed = FALSE;
  self->marker_x = 0;
  self->marker_y = 0;
}

void
easel_canvas_marker_bounds (EaselCanvas *self, double *x0, double *y0, double *x1, double *y1)
{
  double scale = easel_canvas_get_scale (self);

  *x0 = easel_view_edge (self->marker_x, self->zoom) / scale;
  *y0 = easel_view_edge (self->marker_y, self->zoom) / scale;
  *x1 = MAX (easel_view_edge (self->marker_x, self->zoom) + 1, easel_view_edge (self->marker_x + 1, self->zoom)) / scale;
  *y1 = MAX (easel_view_edge (self->marker_y, self->zoom) + 1, easel_view_edge (self->marker_y + 1, self->zoom)) / scale;
}

gboolean
easel_canvas_is_key_held (EaselCanvas *self)
{
  /* A cancel, a lost focus, Save or Quit has ended it: nothing is held, and the guard against the release goes too (REQ-0022-10). */
  if (self->key_button != 0 && !easel_canvas_is_stroking (self))
    {
      guint button = self->key_button;

      self->key_button = 0;
      if (easel_canvas_is_waiting_for_release (self))
        easel_canvas_release (self, button);
    }
  return self->key_button != 0;
}

void
easel_canvas_key_press (EaselCanvas *self, guint button, gboolean ctrl, gboolean shift)
{
  double x0, y0, x1, y1;

  /* In Pan the arrows already move the view: a primary press there is nothing. */
  if (self->img == NULL || easel_canvas_is_key_held (self) || (self->tool == EASEL_TOOL_PAN && button == GDK_BUTTON_PRIMARY))
    return;
  easel_canvas_marker_bounds (self, &x0, &y0, &x1, &y1);
  self->key_pressing = TRUE;
  self->click_count = 0;
  easel_canvas_set_ctrl (self, ctrl);
  easel_canvas_set_shift (self, shift);
  easel_canvas_press (self, button, (x0 + x1) / 2, (y0 + y1) / 2);
  easel_canvas_set_shift (self, FALSE);
  self->key_pressing = FALSE;
  self->key_button = button;
  /* Fill and the Magnifier act at the press and a press that opened a menu holds nothing (REQ-0022-07). */
  if (self->tool == EASEL_TOOL_FILL || self->tool == EASEL_TOOL_MAGNIFIER)
    {
      easel_canvas_release (self, button);
      self->key_button = 0;
    }
  else
    easel_canvas_is_key_held (self);
}

void
easel_canvas_key_release (EaselCanvas *self)
{
  guint button = self->key_button;

  if (button == 0)
    return;
  self->key_pressing = TRUE;
  easel_canvas_release (self, button);
  self->key_pressing = FALSE;
  self->key_button = 0;
}

void
easel_canvas_key_move (EaselCanvas *self, int dx, int dy)
{
  double x0, y0, x1, y1;

  if (self->img == NULL)
    return;
  easel_canvas_marker_set (self, CLAMP (self->marker_x + dx, 0, self->img->width - 1), CLAMP (self->marker_y + dy, 0, self->img->height - 1));
  if (!easel_canvas_is_key_held (self))
    return;
  easel_canvas_marker_bounds (self, &x0, &y0, &x1, &y1);
  self->key_pressing = TRUE;
  easel_canvas_motion (self, (x0 + x1) / 2, (y0 + y1) / 2);
  self->key_pressing = FALSE;
}

gboolean
easel_canvas_key_hold_event (EaselCanvas *self, GdkEventType type, guint button)
{
  guint other;

  if (!easel_canvas_is_key_held (self))
    return FALSE;
  if (type != GDK_BUTTON_PRESS || (button != GDK_BUTTON_PRIMARY && button != GDK_BUTTON_SECONDARY))
    return TRUE;
  other = self->key_button == GDK_BUTTON_PRIMARY ? GDK_BUTTON_SECONDARY : GDK_BUTTON_PRIMARY;
  {
    double x0, y0, x1, y1;

    easel_canvas_marker_bounds (self, &x0, &y0, &x1, &y1);
    self->key_pressing = TRUE;
    easel_canvas_press (self, other, (x0 + x1) / 2, (y0 + y1) / 2);
    easel_canvas_release (self, other);
    self->key_pressing = FALSE;
  }
  easel_canvas_is_key_held (self);
  return TRUE;
}

void
easel_canvas_key_menu (EaselCanvas *self)
{
  double x0, y0, x1, y1, x, y;

  if (self->img == NULL || easel_canvas_is_key_held (self) || easel_canvas_is_stroking (self))
    return;
  if (self->text_open && self->text_layout != NULL)
    {
      PangoRectangle strong;
      double l = logical_per_pixel (self);

      pango_layout_get_cursor_pos (self->text_layout, (int) easel_text_buffer_caret (self->text_buf), &strong, NULL);
      g_signal_emit (self, signals[SIGNAL_CONTEXT_MENU], 0, (self->tb_x + (double) strong.x / PANGO_SCALE) * l,
                     (self->tb_y + (double) strong.y / PANGO_SCALE) * l);
      return;
    }
  easel_canvas_marker_bounds (self, &x0, &y0, &x1, &y1);
  x = (x0 + x1) / 2;
  y = (y0 + y1) / 2;
  if (self->tool == EASEL_TOOL_PAN || easel_tool_selects (self->tool))
    g_signal_emit (self, signals[on_selection (self, x, y) ? SIGNAL_CONTEXT_MENU : SIGNAL_CANVAS_MENU], 0, x, y);
}

void
easel_canvas_text_nudge (EaselCanvas *self, gboolean corner, int dx, int dy)
{
  double l = logical_per_pixel (self);

  if (!self->text_open)
    return;
  text_drag_begin (self, corner ? (int) EASEL_SEL_HANDLE_BR : -1, 0, 0);
  text_drag_to (self, dx * l, dy * l);
  text_drag_end (self, FALSE);
  g_signal_emit (self, signals[SIGNAL_SELECTION_CHANGED], 0);
}

static void
append_ring (GtkSnapshot *snapshot, const GdkRGBA *color, int x0, int y0, int x1, int y1, double scale)
{
  const struct { int x, y, w, h; } sides[] = { { x0, y0, x1 - x0, 1 }, { x0, y1 - 1, x1 - x0, 1 }, { x0, y0 + 1, 1, y1 - y0 - 2 }, { x1 - 1, y0 + 1, 1, y1 - y0 - 2 } };

  for (guint i = 0; i < G_N_ELEMENTS (sides); i++)
    gtk_snapshot_append_color (snapshot, color, &GRAPHENE_RECT_INIT ((float) (sides[i].x / scale), (float) (sides[i].y / scale),
                                                                     (float) (sides[i].w / scale), (float) (sides[i].h / scale)));
}

/* Black just outside the pixel's drawn square, white just outside the black, one device pixel each; below 100 %, around the device pixel that shows it. */
static void
append_keyboard_marker (EaselCanvas *self, GtkSnapshot *snapshot)
{
  static const GdkRGBA black = { 0, 0, 0, 1 }, white = { 1, 1, 1, 1 };
  double scale = easel_canvas_get_scale (self);
  int x0, y0, x1, y1;

  if (!self->marker_on || self->img == NULL)
    return;
  x0 = easel_view_edge (self->marker_x, self->zoom);
  y0 = easel_view_edge (self->marker_y, self->zoom);
  x1 = MAX (x0 + 1, easel_view_edge (self->marker_x + 1, self->zoom));
  y1 = MAX (y0 + 1, easel_view_edge (self->marker_y + 1, self->zoom));
  append_ring (snapshot, &black, x0 - 1, y0 - 1, x1 + 1, y1 + 1, scale);
  append_ring (snapshot, &white, x0 - 2, y0 - 2, x1 + 2, y1 + 2, scale);
  /* The footprint's outline, which a mouse has as its cursor (REQ-0022-11). */
  if (self->tool == EASEL_TOOL_BRUSH || self->tool == EASEL_TOOL_ERASER)
    {
      EaselCursorShape shape = easel_cursor_shape (self->footprint, self->zoom, 1);
      uint32_t *argb = g_new (uint32_t, (gsize) shape.width * shape.height);
      g_autoptr (GBytes) bytes = NULL;
      g_autoptr (GdkTexture) texture = NULL;
      double cx = (x0 + x1) / 2.0 / scale, cy = (y0 + y1) / 2.0 / scale;

      easel_cursor_render (self->footprint, self->zoom, 1, argb);
      bytes = g_bytes_new_take (argb, (gsize) shape.width * shape.height * sizeof *argb);
      texture = gdk_memory_texture_new (shape.width, shape.height, GDK_MEMORY_B8G8R8A8_PREMULTIPLIED, bytes, (gsize) shape.width * sizeof *argb);
      gtk_snapshot_append_texture (snapshot, texture, &GRAPHENE_RECT_INIT ((float) (cx - shape.hot_x), (float) (cy - shape.hot_y), (float) shape.width, (float) shape.height));
    }
}

/* The frame, and the CPU it took: this thread's, so that nothing of the GPU or the compositor counts (REQ-0021-12). */
static void
easel_canvas_snapshot (GtkWidget *widget, GtkSnapshot *snapshot)
{
  EaselCanvas *self = EASEL_CANVAS (widget);
  struct timespec start, end;

  if (self->tiles == NULL)
    return;
  clock_gettime (CLOCK_THREAD_CPUTIME_ID, &start);
  canvas_snapshot_frame (self, snapshot);
  if (self->draw_delay_ms > 0)
    {
      struct timespec now;

      do
        clock_gettime (CLOCK_THREAD_CPUTIME_ID, &now);
      while ((now.tv_sec - start.tv_sec) * 1000.0 + (now.tv_nsec - start.tv_nsec) / 1e6 < self->draw_delay_ms);
    }
  clock_gettime (CLOCK_THREAD_CPUTIME_ID, &end);
  self->frame_ms = (end.tv_sec - start.tv_sec) * 1000.0 + (end.tv_nsec - start.tv_nsec) / 1e6;
  self->frames_drawn++;
  if (self->painted_idle == 0)
    self->painted_idle = g_idle_add (emit_painted, self);
}

static void
canvas_snapshot_frame (EaselCanvas *self, GtkSnapshot *snapshot)
{
  double l;
  graphene_rect_t bounds;
  GdkTexture **preview;
  gboolean by_edges;

  l = self->zoom / (100.0 * easel_canvas_get_scale (self));
  preview = self->sel_state == 2 ? selection_preview (self) : gradient_preview (self, l);
  if (self->sel_state != 2)
    free_trail (self);
  by_edges = view_built_by_edges (self);
  bounds = GRAPHENE_RECT_INIT (0, 0, (float) (self->img->width * l), (float) (self->img->height * l));
  if (by_edges)
    {
      double scale = easel_canvas_get_scale (self);

      bounds.size.width = (float) (easel_view_edge (self->img->width, self->zoom) / scale);
      bounds.size.height = (float) (easel_view_edge (self->img->height, self->zoom) / scale);
    }

  /* Transparent pixels show the checkerboard (REQ-0001-11); nearest filtering keeps pixels hard-edged
     at every zoom (REQ-0001-18). Each tile is drawn where its pixels are; edges fall on whole device pixels. */
  easel_snapshot_append_checkerboard (snapshot, &bounds, CHECKER_SIZE);
  if (!by_edges || !append_view_by_edges (self, snapshot, preview))
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
  append_text_box (self, snapshot, l);           /* the box's pixels, under the grid and the tint (REQ-0017-05) */
  append_shape_preview (self, snapshot);
  if (easel_canvas_grid_is_drawn (self))
    append_grid (self, snapshot);
  append_gradient_markers (self, snapshot);
  append_handles (self, snapshot);                /* the picture's handles lie under the selection's pixels and tint (REQ-0015-01) */
  append_float_outside (self, snapshot, l);
  if (!self->probe_no_tint)
    append_selection_tint (self, snapshot);
  append_text_highlight (self, snapshot, l);      /* on top of the tint (REQ-0017-05) */
  if (!self->probe_no_line)
    append_selection_outline (self, snapshot);
  append_selection_handles (self, snapshot);
  append_text_caret (self, snapshot, l);
  append_magnifier_box (self, snapshot);
  append_keyboard_marker (self, snapshot);
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
  else if (self->tool == EASEL_TOOL_TEXT && self->hover_text != 0)
    cursor = gdk_cursor_new_from_name (self->hover_text == 1 ? "text" : "move", NULL);   /* REQ-0017-01, before the picture's handles */
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
  else if (easel_tool_draws_shape (self->tool) || easel_tool_draws_pending (self->tool) || easel_tool_selects (self->tool)
           || self->tool == EASEL_TOOL_TEXT)
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
         || self->lassoing || self->stretching || self->text_drag != 0;
}

gboolean
easel_canvas_is_panning (EaselCanvas *self)
{
  return self->panning;
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
  if (r == EASEL_FIGURE_DONE && f.closed && !gradient)
    g_signal_emit (self, signals[SIGNAL_POLYGON_CLOSED], 0);
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
  gboolean ok;

  if (!text_put_down (self))
    return FALSE;                               /* the box first; the box stays open (REQ-0017-18) */
  ok = finish_selection (self, TRUE);

  if (self->pending)
    put_down_pending (self);
  return ok;
}

gboolean
easel_canvas_put_down (EaselCanvas *self)
{
  gboolean ok;

  if (!text_put_down (self))
    return FALSE;                               /* the box first; the box stays open (REQ-0017-18) */
  ok = finish_selection (self, FALSE);

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
  if (self->text_drag != 0)
    {
      text_drag_end (self, TRUE);                 /* the other button, Esc or Undo: the box as at the press (REQ-0017-11) */
      return;
    }
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
  self->text_selecting = FALSE;
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
  /* A press on the canvas takes the keyboard there: the arrows then move a selection, not the toolbox's focus (REQ-0022-02). A field of the font card keeps its own rule below. */
  if (gtk_widget_get_root (GTK_WIDGET (self)) != NULL && !GTK_IS_TEXT (gtk_root_get_focus (gtk_widget_get_root (GTK_WIDGET (self)))))
    gtk_widget_grab_focus (GTK_WIDGET (self));
  if (self->img != NULL && !self->key_pressing)
    mouse_takes_marker (self, x, y);
  /* A keyboard press is never part of a double click (REQ-0022-07). */
  if (!self->key_pressing)
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
  if (self->text_drag != 0)
    {
      /* The other button cancels a move or a resize of the box (REQ-0017-11). */
      if (button != self->sel_button)
        text_drag_end (self, TRUE);
      return;
    }
  if (self->tool == EASEL_TOOL_TEXT && self->text_open)
    {
      /* Inside the box a press goes to the text, also where a picture handle's hit area reaches in; outside it, to a box handle, then
         to the band, before the picture's handles (REQ-0017-09); only the primary button moves or resizes it (REQ-0017-10). */
      int hit;

      /* A text field that has the focus (the font card's) loses it first, and applies its value before the press acts (REQ-0017-14) */
      if (gtk_widget_get_root (GTK_WIDGET (self)) != NULL && GTK_IS_TEXT (gtk_root_get_focus (gtk_widget_get_root (GTK_WIDGET (self)))))
        gtk_root_set_focus (gtk_widget_get_root (GTK_WIDGET (self)), NULL);
      hit = text_hit (self, x, y);
      if (hit == 1 && button == GDK_BUTTON_PRIMARY)
        text_press (self, x, y);
      else if ((hit == 1 || hit == 2) && button == GDK_BUTTON_SECONDARY)
        {
          /* The box's menu (REQ-0017-22): on text that is not selected the caret goes there first; on the selection it stays */
          size_t at = hit == 1 ? text_index_at (self, x, y) : 0, from, to;

          if (hit == 1 && !(easel_text_buffer_selection (self->text_buf, &from, &to) && at >= from && at <= to))
            text_move (self, at, FALSE);
          g_signal_emit (self, signals[SIGNAL_CONTEXT_MENU], 0, x, y);
        }
      else if (hit != 1 && hit != 0 && button == GDK_BUTTON_PRIMARY)
        text_drag_begin (self, hit == 3 ? easel_canvas_selection_handle_at (self, x, y) : -1, x, y);
      if (hit != 0)
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
              if (!text_put_down (self))
                return;                               /* a box that could not be put down: no resize (REQ-0017-18) */
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
      /* Only a primary drag pans (REQ-0001-14); the secondary button opens Pan's menu, with no button held (REQ-0021-13). */
      if (button == GDK_BUTTON_PRIMARY)
        {
          self->panning = TRUE;
          self->pan_x = x;
          self->pan_y = y;
          update_cursor (self);
        }
      else if (button == GDK_BUTTON_SECONDARY && !self->panning && self->img != NULL)
        g_signal_emit (self, signals[SIGNAL_CANVAS_MENU], 0, x, y);
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
      /* The secondary button on a pixel of the shape asks for the selection's context menu, elsewhere for the canvas menu; neither
         changes anything (REQ-0014-07, REQ-0013-17, REQ-0016-14). */
      if (button != GDK_BUTTON_PRIMARY)
        {
          g_signal_emit (self, signals[on_selection (self, x, y) ? SIGNAL_CONTEXT_MENU : SIGNAL_CANVAS_MENU], 0, x, y);
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
  if (self->tool == EASEL_TOOL_TEXT)
    {
      if (self->text_open)
        {
          /* Inside the box, on its band or a handle: the text, a move, a resize or the menu (T011, T014, T017). Elsewhere the
             primary and the secondary button put the box down and make no new box (REQ-0017-17). */
          if (button == GDK_BUTTON_PRIMARY || button == GDK_BUTTON_SECONDARY)
            text_put_down (self);
          return;
        }
      if (button != GDK_BUTTON_PRIMARY)
        return;                                   /* no box: the other buttons do nothing (REQ-0017-04, REQ-0017-22) */
      /* A rectangle dragged out from the pixel under the press, inside the picture (REQ-0017-03). */
      self->selecting = TRUE;
      self->sel_for_text = TRUE;
      self->text_dragged = FALSE;
      self->text_press_x = x;
      self->text_press_y = y;
      self->sel_button = button;
      self->sel_px0 = self->sel_px1 = CLAMP (px, 0, self->img->width - 1);
      self->sel_py0 = self->sel_py1 = CLAMP (py, 0, self->img->height - 1);
      g_signal_emit (self, signals[SIGNAL_STROKING_CHANGED], 0, TRUE);
      g_signal_emit (self, signals[SIGNAL_SELECTION_CHANGED], 0);
      gtk_widget_queue_draw (GTK_WIDGET (self));
      return;
    }
  if (self->tool == EASEL_TOOL_SELECT)
    {
      /* A press not on the selection: any selection is removed, and a rectangle is dragged out from the pixel under the
         press, also outside the picture (REQ-0013-04). The secondary button on the selection asks for its context menu, elsewhere
         for the canvas menu; neither changes anything (REQ-0013-17, REQ-0016-14). */
      if (button != GDK_BUTTON_PRIMARY)
        {
          g_signal_emit (self, signals[on_selection (self, x, y) ? SIGNAL_CONTEXT_MENU : SIGNAL_CANVAS_MENU], 0, x, y);
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
  if (self->text_drag != 0)
    {
      text_drag_to (self, x, y);
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
  if (self->text_selecting && self->text_open)
    {
      easel_text_buffer_select (self->text_buf, self->text_anchor, text_index_at (self, x, y));
      caret_restart (self);
      g_signal_emit (self, signals[SIGNAL_SELECTION_CHANGED], 0);   /* the card shows the selection's style (REQ-0017-14) */
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
      if (self->sel_for_text)
        {
          /* A drag starts when the pointer goes beyond the desktop's drag threshold (in screen pixels) and lasts to the release; its
             rectangle stays inside the picture, a pointer outside counting as the edge's nearest pixel (drag_rect) (REQ-0017-03). */
          if (!self->text_dragged
              && gtk_drag_check_threshold (GTK_WIDGET (self), (int) floor (self->text_press_x + 0.5), (int) floor (self->text_press_y + 0.5),
                                           (int) floor (x + 0.5), (int) floor (y + 0.5)))
            {
              self->text_dragged = TRUE;
              g_signal_emit (self, signals[SIGNAL_SELECTION_CHANGED], 0);
              gtk_widget_queue_draw (GTK_WIDGET (self));
            }
        }
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
  if (button == GDK_BUTTON_PRIMARY)
    {
      if (self->text_selecting)
        gtk_widget_queue_draw (GTK_WIDGET (self));  /* the frame that starts the box's dashes again (REQ-0017-05) */
      self->text_selecting = FALSE;
    }
  if (self->text_drag != 0 && button == self->sel_button)
    text_drag_end (self, FALSE);
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

/* The mouse hides the marker, and it takes the pixel under the mouse (REQ-0022-05). */
static void
mouse_takes_marker (EaselCanvas *self, double x, double y)
{
  int px, py;

  easel_view_to_pixel (x, y, self->zoom, easel_canvas_get_scale (self), &px, &py);
  easel_canvas_marker_show (self, FALSE);
  if (px >= 0 && py >= 0 && px < self->img->width && py < self->img->height)
    easel_canvas_marker_set (self, px, py);
}

void
easel_canvas_pointer_at (EaselCanvas *self, double x, double y)
{
  int px, py;

  EaselHandle hover;

  if (self->img == NULL)
    return;
  if (!self->key_pressing)
    mouse_takes_marker (self, x, y);
  hover = easel_canvas_handle_at (self, x, y);
  if (hover != self->hover || marker_hit (self, x, y) != self->hover_marker || on_selection (self, x, y) != self->hover_sel
      || easel_canvas_selection_handle_at (self, x, y) != self->hover_sel_handle || text_hit (self, x, y) % 3 != self->hover_text)
    {
      self->hover_text = text_hit (self, x, y) % 3;     /* 1 inside, 2 the band; a handle is hover_sel_handle's */
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
    dy /= EASEL_CANVAS_PIXELS_PER_NOTCH;
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
  if (easel_canvas_key_hold_event (self, type, type == GDK_MOTION_NOTIFY ? 0 : gdk_button_event_get_button (event)))
    return GDK_EVENT_STOP;

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
  gtk_im_context_set_client_widget (self->im, widget);
  gtk_widget_queue_resize (widget);
}

static void
easel_canvas_unrealize (GtkWidget *widget)
{
  EaselCanvas *self = EASEL_CANVAS (widget);

  g_signal_handlers_disconnect_by_func (self->surface, on_scale_changed, self);
  self->surface = NULL;
  /* The input method holds its client widget: only while the canvas is in a window, else it would keep a canvas whose window is gone
     (and the canvas it) and place its cursor relative to no window */
  gtk_im_context_focus_out (self->im);
  gtk_im_context_set_client_widget (self->im, NULL);
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

  g_clear_handle_id (&self->painted_idle, g_source_remove);
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
  g_clear_handle_id (&self->caret_blink, g_source_remove);
  g_clear_object (&self->text_layout);
  text_free_ink (self);
  g_clear_pointer (&self->text_buf, easel_text_buffer_free);
  g_clear_pointer (&self->text_family, g_free);
  if (self->im != NULL)
    gtk_im_context_set_client_widget (self->im, NULL);
  g_clear_object (&self->im);
  G_OBJECT_CLASS (easel_canvas_parent_class)->dispose (object);
}

static void
easel_canvas_class_init (EaselCanvasClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->dispose = easel_canvas_dispose;
  gtk_widget_class_set_accessible_role (widget_class, GTK_ACCESSIBLE_ROLE_IMG);
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
  signals[SIGNAL_POLYGON_CLOSED] = g_signal_new ("polygon-closed", G_TYPE_FROM_CLASS (klass), G_SIGNAL_RUN_LAST, 0, NULL, NULL, NULL, G_TYPE_NONE, 0);
  signals[SIGNAL_SELECTION_CHANGED] = g_signal_new ("selection-changed", G_TYPE_FROM_CLASS (klass), G_SIGNAL_RUN_LAST, 0, NULL,
                                                   NULL, NULL, G_TYPE_NONE, 0);
  signals[SIGNAL_SELECTION_FAILED] = g_signal_new ("selection-failed", G_TYPE_FROM_CLASS (klass), G_SIGNAL_RUN_LAST, 0, NULL, NULL,
                                                  NULL, G_TYPE_NONE, 1, G_TYPE_INT);
  /* A box's first text could not be shown: the box closed (REQ-0017-07); the window shows the dialog. */
  signals[SIGNAL_TEXT_FAILED] = g_signal_new ("text-failed", G_TYPE_FROM_CLASS (klass), G_SIGNAL_RUN_LAST, 0, NULL, NULL, NULL,
                                             G_TYPE_NONE, 1, G_TYPE_INT);
  signals[SIGNAL_CONTEXT_MENU] = g_signal_new ("context-menu", G_TYPE_FROM_CLASS (klass), G_SIGNAL_RUN_LAST, 0, NULL, NULL,
                                               NULL, G_TYPE_NONE, 2, G_TYPE_DOUBLE, G_TYPE_DOUBLE);
  /* The picture's pixels were replaced by the canvas (a paste grew it, or its growth was undone): its size may differ (REQ-0016-07). */
  signals[SIGNAL_PICTURE_REPLACED] = g_signal_new ("picture-replaced", G_TYPE_FROM_CLASS (klass), G_SIGNAL_RUN_LAST, 0, NULL, NULL, NULL,
                                                   G_TYPE_NONE, 0);
  /* A secondary press with Select or Free-Form Select not on the selection nor a handle: the canvas menu at (x, y) (REQ-0016-14). */
  signals[SIGNAL_PAINTED] = g_signal_new ("painted", G_TYPE_FROM_CLASS (klass), G_SIGNAL_RUN_LAST, 0, NULL, NULL, NULL, G_TYPE_NONE, 0);
  signals[SIGNAL_CANVAS_MENU] = g_signal_new ("canvas-menu", G_TYPE_FROM_CLASS (klass), G_SIGNAL_RUN_LAST, 0, NULL, NULL, NULL,
                                              G_TYPE_NONE, 2, G_TYPE_DOUBLE, G_TYPE_DOUBLE);
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
  self->tint = EASEL_TINT_DEFAULT;
  self->zoom = 100;
  /* Input methods, compose and dead keys for the text box (REQ-0017-06): their text arrives by "commit" */
  self->im = gtk_im_multicontext_new ();                   /* its client widget is set while the canvas is realized */
  g_signal_connect (self->im, "commit", G_CALLBACK (on_im_commit), self);
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
