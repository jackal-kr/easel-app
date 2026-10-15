/* SPDX-License-Identifier: GPL-3.0-or-later */
/* View math (REQ-0001-16, REQ-0001-19). Lengths are in logical pixels; scale is device pixels per logical pixel;
   zoom is in percent of device pixels, so at 100 % one picture pixel is one device pixel. */
#pragma once

/* Initial window size: 75 % of the monitor in each direction (REQ-0001-41). */
void easel_view_window_size (int monitor_width, int monitor_height, int *width, int *height);

/* The zoom levels in percent: 10, 25, 50, 100, 200, 300, 400, 600, 800. */
int easel_view_zoom_step (int pct, int notches);

/* Width or height in logical pixels that shows picture_size pixels, rounded up. */
int easel_view_content_size (int picture_size, int pct, double scale);

/* Picture pixel under a point given relative to the picture's top-left (negative left of or above it). */
void easel_view_to_pixel (double x, double y, int pct, double scale, int *px, int *py);

/* New scroll offset along one axis after a zoom, keeping the picture point under the pointer still
   as far as the view's limits allow. pointer is relative to the view. */
double easel_view_anchor (double scroll, double pointer, int old_pct, int new_pct, int picture_size, double scale,
                          double view_size);

/* Fit to Window (REQ-0004-07): the largest zoom level at which a pic_w x pic_h picture fits in an area_w x area_h canvas
   area, all in device pixels with no scrollbars counted; 10 if none fits. */
int easel_view_fit_level (int area_w, int area_h, int pic_w, int pic_h);

/* The pinch (REQ-0004-08). *base is the gesture's scale at the last level change (1.0 when a gesture begins); scale
   is its scale now; pct is the current zoom. Returns +1 or -1 when the scale has changed by 1.5x since *base and the
   zoom can move that way, else 0; on a 1.5x change *base becomes scale, also when the zoom cannot move (at 800 % or
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
