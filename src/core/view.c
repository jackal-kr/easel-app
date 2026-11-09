/* SPDX-License-Identifier: GPL-3.0-or-later */

#include <math.h>
#include <stdint.h>

#include "view.h"

static const int zoom_levels[] = { 10, 25, 50, 100, 200, 300, 400, 600, 800, 1600 };   /* 1600: JOD, 2026-10-27 */
#define N_LEVELS ((int) (sizeof zoom_levels / sizeof zoom_levels[0]))

const int easel_view_presets[EASEL_VIEW_N_PRESETS] = { 100, 200, 400, 600, 800, 1600 };

int
easel_view_preset_index (int pct)
{
  for (int i = 0; i < EASEL_VIEW_N_PRESETS; i++)
    if (easel_view_presets[i] == pct)
      return i;
  return -1;
}

int
easel_view_zoom_step (int pct, int notches)
{
  int i = 0;

  /* i is the level at or below the zoom. A zoom between two levels (Fit to Window gives one) is not a level: stepping
     in goes to the level above it, and stepping out to the level below it, which is the first step out. */
  while (i + 1 < N_LEVELS && zoom_levels[i + 1] <= pct)
    i++;
  if (notches < 0 && zoom_levels[i] != pct)
    notches++;
  if (notches > N_LEVELS || notches < -N_LEVELS)
    notches = notches > 0 ? N_LEVELS : -N_LEVELS;
  i += notches;
  if (i < 0)
    i = 0;
  if (i >= N_LEVELS)
    i = N_LEVELS - 1;
  return zoom_levels[i];
}

static double
logical_per_pixel (int pct, double scale)
{
  return pct / (100.0 * scale);
}

int
easel_view_content_size (int picture_size, int pct, double scale)
{
  /* The epsilon keeps exact sizes such as 1920 / 1.0 from rounding up through floating-point noise. Below 100 %, the device pixels
     REQ-0031-01 gives the picture, in logical pixels. */
  int size = pct < 100 ? (int) ceil (easel_view_count_below (picture_size, pct) / scale - 1e-9)
                       : (int) ceil (picture_size * logical_per_pixel (pct, scale) - 1e-9);

  return size < 1 ? 1 : size;
}

int
easel_view_edge (int k, int pct)
{
  /* The product first: it is whole, so a tie is exactly a half and the division cannot miss it. */
  return (int) floor ((double) k * pct / 100.0 + 0.5);
}

int
easel_view_cell_at (int d, int pct)
{
  /* The largest k with edge (k) <= d: k < (d + 1/2) / size, in whole numbers, rounded up then less one. */
  int64_t a = ((int64_t) 2 * d + 1) * 50;

  return (int) ((a > 0 ? (a + pct - 1) / pct : -(-a / pct)) - 1);
}

int
easel_view_pixel_below (int d, int pct)
{
  /* floor ((d + 1/2) x 100 / pct) = floor ((2d + 1) x 50 / pct), in whole numbers, rounded toward smaller values for d < 0 too. */
  int64_t a = ((int64_t) 2 * d + 1) * 50;

  return (int) (a >= 0 ? a / pct : -((-a + pct - 1) / pct));
}

int
easel_view_edge_below (int k, int pct)
{
  /* The first d whose pixel is k or more: (2d + 1) x 50 >= k x pct, so d >= (k x pct - 50) / 100, rounded up, also left of or above the
     picture (a float outside it, REQ-0031-02). */
  int64_t a = (int64_t) k * pct - 50;

  return (int) (a >= 0 ? (a + 99) / 100 : -((-a) / 100));
}

int
easel_view_count_below (int size, int pct)
{
  int n = easel_view_edge_below (size, pct);

  return n < 1 ? 1 : n;
}

int
easel_view_first_cell (double scroll_logical, double margin, int pct, double scale)
{
  double d = (scroll_logical - margin) * scale;

  return d <= 0 ? 0 : easel_view_cell_at ((int) floor (d), pct);
}

bool
easel_view_thumbnail_fits (int w, int h)
{
  return w >= 172 + 2 * 12 && h >= 132 + 2 * 12;
}

bool
easel_view_draws_by_edges (int pct)
{
  return pct >= 100 && pct % 100 != 0;
}

bool
easel_view_built_by_edges (int pct, double scale)
{
  /* Below 100 % the view is built by REQ-0031-01's rule at a whole screen scale. */
  return scale == floor (scale) && (pct < 100 || easel_view_draws_by_edges (pct) || scale != 1);
}

void
easel_view_to_pixel (double x, double y, int pct, double scale, int *px, int *py)
{
  double l = logical_per_pixel (pct, scale);

  /* Below 100 %: the pixel the point's device pixel shows (REQ-0031-03). */
  if (pct < 100)
    {
      *px = easel_view_pixel_below ((int) floor (x * scale), pct);
      *py = easel_view_pixel_below ((int) floor (y * scale), pct);
      return;
    }
  /* The pixel whose square is drawn under the point (REQ-0025-06): the view's edges are the only rule, drawn and pointed. A fractional screen
     scale stays with the division, untested (0025's Plan). */
  if (easel_view_built_by_edges (pct, scale))
    {
      *px = easel_view_cell_at ((int) floor (x * scale), pct);
      *py = easel_view_cell_at ((int) floor (y * scale), pct);
      return;
    }
  *px = (int) floor (x / l);
  *py = (int) floor (y / l);
}

double
easel_view_anchor (double scroll, double pointer, int old_pct, int new_pct, int picture_size, double scale,
                   double view_size)
{
  /* The scroll offsets count the margin before the picture, and the scrollable size the margins on both sides
     (REQ-0009-05). */
  double picture_point = (scroll + pointer - EASEL_VIEW_MARGIN) / logical_per_pixel (old_pct, scale);
  double next = picture_point * logical_per_pixel (new_pct, scale) + EASEL_VIEW_MARGIN - pointer;
  double max = easel_view_content_size (picture_size, new_pct, scale) + 2 * EASEL_VIEW_MARGIN - view_size;

  if (next > max)
    next = max;
  if (next < 0)
    next = 0;
  return next;
}

int
easel_view_take_notches (double *acc, double delta)
{
  int notches;

  /* A change of direction starts afresh, so one full notch the other way always counts. */
  if ((*acc > 0 && delta < 0) || (*acc < 0 && delta > 0))
    *acc = 0;
  *acc += delta;
  notches = (int) trunc (*acc);
  *acc -= notches;
  return -notches;
}

void
easel_view_window_size (int monitor_width, int monitor_height, int *width, int *height)
{
  *width = monitor_width * 3 / 4;
  *height = monitor_height * 3 / 4;
}

int
easel_view_fit_zoom (int area_w, int area_h, int pic_w, int pic_h, double scale)
{
  long zoom;

  /* The picture must fit with its two margins (REQ-0009-05), which are logical pixels and so scale times as many device
     pixels. */
  area_w -= (int) ceil (2 * EASEL_VIEW_MARGIN * scale);
  area_h -= (int) ceil (2 * EASEL_VIEW_MARGIN * scale);
  if (pic_w < 1 || pic_h < 1 || area_w < 1 || area_h < 1)
    return zoom_levels[0];
  /* Integer division rounds down, which is what "fits" needs: one percent more would not. */
  zoom = (long) area_w * 100 / pic_w;
  if ((long) area_h * 100 / pic_h < zoom)
    zoom = (long) area_h * 100 / pic_h;
  if (zoom < zoom_levels[0])
    return zoom_levels[0];
  return zoom > EASEL_VIEW_FIT_MAX ? EASEL_VIEW_FIT_MAX : (int) zoom;          /* Fit stops at 800 % (REQ-0004-07) */
}

int
easel_view_pinch (double *base, double scale, int pct)
{
  const double step = 1.5, eps = 1e-9;
  double ratio = scale / *base;

  if (ratio >= step - eps)
    {
      *base = scale;
      return easel_view_zoom_step (pct, 1) != pct ? 1 : 0;
    }
  if (ratio * step <= 1.0 + eps)
    {
      *base = scale;
      return easel_view_zoom_step (pct, -1) != pct ? -1 : 0;
    }
  return 0;
}

/* One edge: start plus the movement, rounded and limited. The limits apply before the conversion to int. */
static int
moved_edge (int start, double delta)
{
  double edge = floor (start + delta + 0.5);

  if (!(edge >= 1))       /* also catches NaN */
    return 1;
  return edge > 10000 ? 10000 : (int) edge;
}

void
easel_view_resize_by (EaselHandle handle, double dx, double dy, int *w, int *h)
{
  if (handle == EASEL_HANDLE_RIGHT || handle == EASEL_HANDLE_CORNER)
    *w = moved_edge (*w, dx);
  if (handle == EASEL_HANDLE_BOTTOM || handle == EASEL_HANDLE_CORNER)
    *h = moved_edge (*h, dy);
}

double
easel_view_focus_scroll (double focus, int pct, int picture_size, double scale, double view_size)
{
  double content = easel_view_content_size (picture_size, pct, scale);
  double offset = EASEL_VIEW_MARGIN + focus * pct / 100.0 / scale - view_size / 2;     /* the margin is before the picture */
  double most = content + 2 * EASEL_VIEW_MARGIN - view_size;

  if (offset > most)
    offset = most;
  if (offset < 0)
    offset = 0;
  return offset;
}
