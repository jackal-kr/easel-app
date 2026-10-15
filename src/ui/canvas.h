/* SPDX-License-Identifier: GPL-3.0-or-later */
#pragma once

#include <gtk/gtk.h>

#include "history.h"
#include "image.h"
#include "tool.h"

G_BEGIN_DECLS

#define EASEL_TYPE_CANVAS (easel_canvas_get_type ())
G_DECLARE_FINAL_TYPE (EaselCanvas, easel_canvas, EASEL, CANVAS, GtkWidget)

/* Shows img (not owned; it must outlive the canvas or be replaced first). NULL shows nothing. */
void easel_canvas_set_image (EaselCanvas *self, EaselImage *img);

/* Re-uploads the pixels the image marked dirty and redraws. Call after every edit. */
void easel_canvas_update (EaselCanvas *self);

int easel_canvas_get_zoom (EaselCanvas *self);

/* The picture is shown as tiles of EASEL_CANVAS_TILE x EASEL_CANVAS_TILE picture pixels (fewer at the right and bottom
   edges), each a texture that reads the picture's own memory. An update rebuilds only the tiles it touches: at 8000x8000
   the Vulkan renderer uploads a whole texture each frame, which one big texture made cost 56 ms. 500 divides every zoom
   level into whole device pixels (500 * 10 % = 50), so tile edges never fall between device pixels. */
#define EASEL_CANVAS_TILE 500

/* Test hooks (0004 T001): the texture of tile (tx, ty), not owned (NULL outside the picture); how many tile textures
   have been built since the picture was set; how many bytes of the picture the canvas has copied (always 0: the tiles
   read the picture's own memory). */
GdkTexture *easel_canvas_get_tile_texture (EaselCanvas *self, int tx, int ty);
gsize easel_canvas_get_tiles_rebuilt (EaselCanvas *self);
gsize easel_canvas_get_bytes_copied (EaselCanvas *self);

/* Device pixels per logical pixel of the canvas's surface. */
double easel_canvas_get_scale (EaselCanvas *self);

/* While blocked (the color chooser is open, REQ-0003-14), presses on the canvas do nothing. */
void easel_canvas_set_input_blocked (EaselCanvas *self, gboolean blocked);

/* Where finished strokes are recorded as undo steps (not owned; NULL records nothing). */
void easel_canvas_set_history (EaselCanvas *self, EaselHistory *history);

void easel_canvas_set_tool (EaselCanvas *self, EaselTool tool);
void easel_canvas_set_colors (EaselCanvas *self, uint32_t foreground, uint32_t background);

/* A pencil stroke is in progress. */
gboolean easel_canvas_is_stroking (EaselCanvas *self);

/* Cancels a stroke in progress, restoring the picture, and ignores the rest of that mouse press (REQ-0002-09). */
void easel_canvas_cancel_stroke (EaselCanvas *self);

/* A mouse button of a cancelled stroke is still held: history changes wait until it is released. */
gboolean easel_canvas_is_waiting_for_release (EaselCanvas *self);

/* The window lost focus, so the pointer buttons can no longer be trusted: ends a stroke in progress (kept), forgets
   a cancelled stroke's button and a pan (REQ-0001-25, REQ-0002-09). */
void easel_canvas_reset_pointer (EaselCanvas *self);

/* Ends a stroke in progress as if its button were released, keeping what was drawn (REQ-0001-25, REQ-0001-33). */
void easel_canvas_end_stroke (EaselCanvas *self);

/* Input entry points, in canvas coordinates (logical pixels from the picture's top-left). The event
   controller feeds them; tests call them directly. Emit "stroke-finished" (guint changed_pixels). */
void easel_canvas_press (EaselCanvas *self, guint button, double x, double y);
void easel_canvas_motion (EaselCanvas *self, double x, double y);
void easel_canvas_release (EaselCanvas *self, guint button);

/* Zoom commands from the menus and keys, with any tool (REQ-0004-04..07). Both emit "zoom-changed" (int percent) when the
   zoom changes. set_zoom keeps the center of the canvas area still as far as the edges allow; fit_to_window picks the
   largest level at which the whole picture fits the canvas area and shows its top-left corner. */
void easel_canvas_set_zoom (EaselCanvas *self, int pct);
void easel_canvas_fit_to_window (EaselCanvas *self);

/* The pixel grid (REQ-0004-12..14): on or off, the cell size in picture pixels, and whether it is axonometric. It is
   drawn over the picture, never saved, and only while one cell is at least 4 device pixels wide on screen. */
void easel_canvas_set_grid (EaselCanvas *self, gboolean on, int cell, gboolean axonometric);
gboolean easel_canvas_get_grid_on (EaselCanvas *self);
int easel_canvas_get_grid_cell (EaselCanvas *self);
gboolean easel_canvas_get_grid_axonometric (EaselCanvas *self);
/* On, and big enough to be drawn at the current zoom. */
gboolean easel_canvas_grid_is_drawn (EaselCanvas *self);

/* The pointer over the canvas area (REQ-0004-09): "pointer-moved" (int x, int y) with the picture pixel under it,
   negative or beyond the size when it is outside the picture, and "pointer-left" when it leaves the canvas area. The
   event controller feeds these; tests call them. (x, y) are in canvas coordinates, logical pixels from the picture's
   top-left. */
void easel_canvas_pointer_at (EaselCanvas *self, double x, double y);
void easel_canvas_pointer_left (EaselCanvas *self);

/* The touchpad or touch pinch (REQ-0004-08). The gesture feeds these; tests call them. scale is the gesture's scale since
   it began; (view_x, view_y) is its center relative to the canvas area. Pan only. */
void easel_canvas_pinch_begin (EaselCanvas *self);
void easel_canvas_pinch (EaselCanvas *self, double scale, double view_x, double view_y);
void easel_canvas_pinch_end (EaselCanvas *self);

/* In Pan mode, moves notches zoom levels (positive = in), keeping the picture point at view (x, y) still
   (REQ-0001-16). view_x, view_y are relative to the visible area. */
void easel_canvas_zoom_at (EaselCanvas *self, int notches, double view_x, double view_y);

/* Takes pointer input from area (the canvas's scrolled area), so strokes and pans can start outside the
   picture (REQ-0001-14, REQ-0001-22). */
void easel_canvas_listen_on (EaselCanvas *self, GtkWidget *area);

/* The checkerboard drawn behind transparency, on screen only (REQ-0001-11, REQ-0003-05). */
void easel_snapshot_append_checkerboard (GtkSnapshot *snapshot, const graphene_rect_t *bounds, float cell);

G_END_DECLS
