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

/* Adds a scroll delta (1.0 = one wheel notch down) to *acc and returns whole notches, positive = zoom in. */
int easel_view_take_notches (double *acc, double delta);
