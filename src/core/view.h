/* SPDX-License-Identifier: GPL-3.0-or-later */
/* View math (REQ-0001-16, REQ-0001-19). Lengths are in logical pixels; scale is device pixels per logical pixel;
   zoom is in percent of device pixels, so at 100 % one picture pixel is one device pixel. */
#pragma once

#include <stdbool.h>

/* Initial window size: 75 % of the monitor in each direction (REQ-0001-41). */
void easel_view_window_size (int monitor_width, int monitor_height, int *width, int *height);

/* The zoom levels in percent: 10, 25, 50, 100, 200, 300, 400, 600, 800, 1600 (1600: JOD, 2026-10-27). */
int easel_view_zoom_step (int pct, int notches);

/* The zoom presets (REQ-0008-01): the levels the Magnifier's options and the View > Zoom submenu offer, in this order.
   Every preset is a zoom level (REQ-0008-06); 10, 25, 50 and 300 % are levels and not presets. */
enum { EASEL_VIEW_N_PRESETS = 6 };

/* The largest zoom Fit to Window gives (REQ-0004-07); the levels go on to 1600 % (REQ-0001-16). */
enum { EASEL_VIEW_FIT_MAX = 800 };
extern const int easel_view_presets[EASEL_VIEW_N_PRESETS];

/* The index of pct in easel_view_presets, or -1 when it is not a preset (10, 25, 50, 300, a Fit to Window zoom such as 42). */
int easel_view_preset_index (int pct);

/* The margin around the picture in the canvas area, in logical pixels, on each side (REQ-0009-01). The scroll offsets
   the functions below take and give are those of the scrolled area, margin included. */
#define EASEL_VIEW_MARGIN 12

/* Width or height in logical pixels that shows picture_size pixels, rounded up. */
int easel_view_content_size (int picture_size, int pct, double scale);

/* REQ-0025-02: where the square of picture pixel k starts, in device pixels from the picture's origin, on any screen; the same function for
   the picture, the grid and the pointer. */
int easel_view_edge (int k, int pct);
/* The inverse: the picture pixel whose square covers the device pixel d (negative left of or above the picture). */
int easel_view_cell_at (int d, int pct);
/* Whether the view places its squares by those edges: from 100 % up, when a square is not a whole number of device pixels. */
bool easel_view_draws_by_edges (int pct);
/* Whether the view is built device pixel by device pixel on those edges, on a whole `scale`: where the renderer's scaling is not exact, a
   square that is not a whole number of device pixels and any zoom on a scale above 1 (0025's Plan; REQ-0001-18). */
bool easel_view_built_by_edges (int pct, double scale);

/* Picture pixel under a point given relative to the picture's top-left (negative left of or above it); on a border, the square that starts there. */
void easel_view_to_pixel (double x, double y, int pct, double scale, int *px, int *py);

/* New scroll offset along one axis after a zoom, keeping the picture point under the pointer still
   as far as the view's limits allow. pointer is relative to the view. */
double easel_view_anchor (double scroll, double pointer, int old_pct, int new_pct, int picture_size, double scale,
                          double view_size);

/* Fit to Window (REQ-0004-07): the largest whole percent at which a pic_w x pic_h picture fits in an area_w x area_h
   canvas area, all in device pixels with no scrollbars counted, limited to 10 to EASEL_VIEW_FIT_MAX, 800 (10 if even that is too big). It
   is usually not one of the zoom levels; easel_view_zoom_step steps from it to the level above or below. */
int easel_view_fit_zoom (int area_w, int area_h, int pic_w, int pic_h, double scale);

/* The pinch (REQ-0004-08). *base is the gesture's scale at the last level change (1.0 when a gesture begins); scale
   is its scale now; pct is the current zoom. Returns +1 or -1 when the scale has changed by 1.5x since *base and the
   zoom can move that way, else 0; on a 1.5x change *base becomes scale, also when the zoom cannot move (at 1600 % or
   10 %). */
int easel_view_pinch (double *base, double scale, int pct);

/* Adds a scroll delta (1.0 = one wheel notch down) to *acc and returns whole notches, positive = zoom in. */
int easel_view_take_notches (double *acc, double delta);

/* The resize handles (REQ-0004-16). */
typedef enum
{
  EASEL_HANDLE_NONE,
  EASEL_HANDLE_RIGHT,
  EASEL_HANDLE_BOTTOM,
  EASEL_HANDLE_CORNER
} EaselHandle;

/* The size after dragging a handle by dx, dy picture pixels (REQ-0004-18). *w and *h are the size at the press and
   become the new size: the right handle moves the right edge by dx, the bottom handle the bottom edge by dy, the corner
   both; each edge goes to the nearest pixel boundary (.5 rounds up) and is limited to 1 to 10000. */
void easel_view_resize_by (EaselHandle handle, double dx, double dy, int *w, int *h);

/* The scroll offset along one axis that puts the picture coordinate `focus` (in picture pixels, a pixel's center is at
   its index + 0.5) at the center of a view view_size logical pixels long, at zoom pct and the given scale, kept inside the
   content (0 when the content is not longer than the view; REQ-0007-03). */
double easel_view_focus_scroll (double focus, int pct, int picture_size, double scale, double view_size);
