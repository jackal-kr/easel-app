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

/* In Pan mode, moves notches zoom levels (positive = in), keeping the picture point at view (x, y) still
   (REQ-0001-16). view_x, view_y are relative to the visible area. */
void easel_canvas_zoom_at (EaselCanvas *self, int notches, double view_x, double view_y);

/* Takes pointer input from area (the canvas's scrolled area), so strokes and pans can start outside the
   picture (REQ-0001-14, REQ-0001-22). */
void easel_canvas_listen_on (EaselCanvas *self, GtkWidget *area);

/* The checkerboard drawn behind transparency, on screen only (REQ-0001-11, REQ-0003-05). */
void easel_snapshot_append_checkerboard (GtkSnapshot *snapshot, const graphene_rect_t *bounds, float cell);

G_END_DECLS
