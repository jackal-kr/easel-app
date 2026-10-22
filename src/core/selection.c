/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "selection.h"

#include <stdlib.h>
#include <string.h>

#include "change-log.h"
#include "line.h"

static bool
inside (int x, int y, int rx, int ry, int rw, int rh)
{
  return x >= rx && y >= ry && x - rx < rw && y - ry < rh;
}

EaselSelectionResult
easel_selection_lift (EaselFloat *f, const EaselImage *img, int x, int y, int w, int h, const uint8_t *mask, bool copy,
                      uint32_t hole_color, size_t max_bytes)
{
  size_t n = (size_t) w * (size_t) h;
  size_t bytes;

  *f = (EaselFloat) { 0 };
  if (w <= 0 || h <= 0 || x < 0 || y < 0 || x > img->width - w || y > img->height - h)
    return EASEL_SELECTION_NO_MEMORY;
  if (n > SIZE_MAX / (sizeof *f->px + 1))
    return EASEL_SELECTION_NO_MEMORY;
  bytes = n * sizeof *f->px + (mask != NULL ? n : 0);
  if (bytes > max_bytes)
    return EASEL_SELECTION_NO_MEMORY;
  f->px = malloc (n * sizeof *f->px);
  if (f->px == NULL)
    return EASEL_SELECTION_NO_MEMORY;
  if (mask != NULL)
    {
      f->mask = malloc (n);
      if (f->mask == NULL)
        {
          free (f->px);
          *f = (EaselFloat) { 0 };
          return EASEL_SELECTION_NO_MEMORY;
        }
      memcpy (f->mask, mask, n);
    }
  for (int j = 0; j < h; j++)
    memcpy (f->px + (size_t) j * w, img->px + (size_t) (y + j) * img->width + x, (size_t) w * sizeof *f->px);
  f->x = f->hx = x;
  f->y = f->hy = y;
  f->w = w;
  f->h = h;
  f->hole = !copy;
  f->hole_color = hole_color;
  return EASEL_SELECTION_DONE;
}

bool
easel_selection_stamp (EaselFloat *f, size_t max_bytes)
{
  if (f->n_stamps == f->cap)
    {
      size_t cap = f->cap ? f->cap * 2 : 8;
      EaselStamp *p;

      if (cap > SIZE_MAX / sizeof *p || cap * sizeof *p > max_bytes)
        return false;
      p = realloc (f->stamps, cap * sizeof *p);
      if (p == NULL)
        return false;
      f->stamps = p;
      f->cap = cap;
    }
  f->stamps[f->n_stamps++] = (EaselStamp) { f->x, f->y };
  return true;
}

bool
easel_selection_smear (EaselFloat *f, int x, int y, size_t max_bytes)
{
  EaselLine l;
  int lx, ly, x0 = f->x, y0 = f->y;
  size_t n = f->n_stamps;

  easel_line_init (&l, x0, y0, x, y);
  while (easel_line_next (&l, &lx, &ly))
    {
      if (lx == x && ly == y)
        break;                                  /* the place it moves to is not a copy */
      f->x = lx;
      f->y = ly;
      if (!easel_selection_stamp (f, max_bytes))
        {
          f->n_stamps = n;
          f->x = x0;
          f->y = y0;
          return false;
        }
    }
  f->x = x;
  f->y = y;
  return true;
}

/* Is (x, y) a pixel of the float's shape when it is at (px, py)? */
static bool
in_shape (const EaselFloat *f, int px, int py, int x, int y)
{
  return inside (x, y, px, py, f->w, f->h) && (f->mask == NULL || f->mask[(size_t) (y - py) * f->w + (x - px)] != 0);
}

/* The float's pixels at (px, py) written over the part of the window (x0, y0, w, h) that is inside the picture: the cost is the float's
   own area, not the window's. */
static void
lay_float_at (const EaselFloat *f, int px, int py, const EaselImage *img, int x0, int y0, int w, int h, uint32_t *dst,
              size_t dst_stride)
{
  int ax0 = px > x0 ? px : x0, ay0 = py > y0 ? py : y0;
  int ax1 = px + f->w < x0 + w ? px + f->w : x0 + w, ay1 = py + f->h < y0 + h ? py + f->h : y0 + h;

  ax0 = ax0 > 0 ? ax0 : 0;
  ay0 = ay0 > 0 ? ay0 : 0;
  ax1 = ax1 < img->width ? ax1 : img->width;
  ay1 = ay1 < img->height ? ay1 : img->height;
  for (int y = ay0; y < ay1; y++)
    for (int x = ax0; x < ax1; x++)
      {
        size_t at = (size_t) (y - py) * f->w + (size_t) (x - px);

        if (f->mask == NULL || f->mask[at] != 0)
          dst[(size_t) (y - y0) * dst_stride + (size_t) (x - x0)] = f->px[at];
      }
}

void
easel_selection_lay (const EaselFloat *f, bool with_float, const EaselImage *img, int x0, int y0, int w, int h,
                     uint32_t *dst, size_t dst_stride)
{
  easel_selection_lay_range (f, 0, f->n_stamps, with_float, img, x0, y0, w, h, dst, dst_stride, true);
}

void
easel_selection_lay_range (const EaselFloat *f, size_t first, size_t last, bool with_float, const EaselImage *img, int x0, int y0,
                           int w, int h, uint32_t *dst, size_t dst_stride, bool with_base)
{
  for (int j = 0; with_base && j < h; j++)
    {
      int y = y0 + j;

      if (y < 0 || y >= img->height)
        continue;
      for (int i = 0; i < w; i++)
        {
          int x = x0 + i;
          uint32_t v;

          if (x < 0 || x >= img->width)
            continue;
          v = img->px[(size_t) y * img->width + x];
          if (f->hole && in_shape (f, f->hx, f->hy, x, y))
            v = f->hole_color;
          dst[(size_t) j * dst_stride + i] = v;
        }
    }
  for (size_t k = first; k < last; k++)
    lay_float_at (f, f->stamps[k].x, f->stamps[k].y, img, x0, y0, w, h, dst, dst_stride);
  if (with_float)
    lay_float_at (f, f->x, f->y, img, x0, y0, w, h, dst, dst_stride);
}

/* The rectangle of the picture the result can differ in: the hole and the float, limited to the picture. FALSE if none. */
bool
easel_selection_bounds (const EaselFloat *f, bool with_float, const EaselImage *img, int *x0, int *y0, int *x1, int *y1)
{
  long long ax0 = img->width, ay0 = img->height, ax1 = -1, ay1 = -1;

  if (f->hole)
    {
      ax0 = f->hx;
      ay0 = f->hy;
      ax1 = (long long) f->hx + f->w - 1;
      ay1 = (long long) f->hy + f->h - 1;
    }
  for (size_t k = 0; k <= f->n_stamps; k++)
    {
      /* each stamp, then the float itself */
      int px = k < f->n_stamps ? f->stamps[k].x : f->x, py = k < f->n_stamps ? f->stamps[k].y : f->y;

      if (k == f->n_stamps && !with_float)
        break;
      ax0 = px < ax0 ? px : ax0;
      ay0 = py < ay0 ? py : ay0;
      ax1 = (long long) px + f->w - 1 > ax1 ? (long long) px + f->w - 1 : ax1;
      ay1 = (long long) py + f->h - 1 > ay1 ? (long long) py + f->h - 1 : ay1;
    }
  ax0 = ax0 < 0 ? 0 : ax0;
  ay0 = ay0 < 0 ? 0 : ay0;
  ax1 = ax1 > img->width - 1 ? img->width - 1 : ax1;
  ay1 = ay1 > img->height - 1 ? img->height - 1 : ay1;
  if (ax0 > ax1 || ay0 > ay1)
    return false;
  *x0 = (int) ax0;
  *y0 = (int) ay0;
  *x1 = (int) ax1;
  *y1 = (int) ay1;
  return true;
}

EaselSelectionResult
easel_selection_put_down (const EaselFloat *f, bool with_float, EaselImage *img, EaselHistory *history, size_t max_bytes)
{
  EaselChangeLog l;
  int x0, y0, x1, y1;
  uint32_t *row;
  bool ok = true;

  if (!easel_selection_bounds (f, with_float, img, &x0, &y0, &x1, &y1))
    return EASEL_SELECTION_NOTHING;
  row = malloc ((size_t) (x1 - x0 + 1) * sizeof *row);
  if (row == NULL)
    return EASEL_SELECTION_NO_MEMORY;
  easel_change_log_init (&l, max_bytes);
  /* Each row laid as the preview lays it, then the pixels that differ set through the log (it logs only changes). */
  for (int y = y0; ok && y <= y1; y++)
    {
      easel_selection_lay (f, with_float, img, x0, y, x1 - x0 + 1, 1, row, (size_t) (x1 - x0 + 1));
      for (int x = x0; ok && x <= x1; x++)
        ok = easel_change_log_set (&l, img, x, y, row[x - x0]);
    }
  free (row);
  if (!ok)
    {
      easel_change_log_restore (&l, img);
      easel_change_log_free (&l);
      return EASEL_SELECTION_NO_MEMORY;
    }
  if (l.n == 0)
    {
      easel_change_log_free (&l);
      return EASEL_SELECTION_NOTHING;
    }
  if (history == NULL)
    {
      easel_change_log_free (&l);
      return EASEL_SELECTION_DONE;
    }
  if (!easel_change_log_push (&l, history))
    {
      easel_change_log_restore (&l, img);
      easel_change_log_free (&l);
      return EASEL_SELECTION_NO_MEMORY;
    }
  return EASEL_SELECTION_DONE;
}

EaselSelectionResult
easel_selection_clear (EaselImage *img, EaselHistory *history, int x, int y, int w, int h, const uint8_t *mask, uint32_t color,
                       size_t max_bytes)
{
  EaselChangeLog l;
  bool ok = true;

  if (w <= 0 || h <= 0 || x < 0 || y < 0 || x > img->width - w || y > img->height - h)
    return EASEL_SELECTION_NOTHING;
  easel_change_log_init (&l, max_bytes);
  for (int j = y; ok && j < y + h; j++)
    for (int i = x; ok && i < x + w; i++)
      if (mask == NULL || mask[(size_t) (j - y) * w + (i - x)] != 0)
        ok = easel_change_log_set (&l, img, i, j, color);
  if (!ok)
    {
      easel_change_log_restore (&l, img);
      easel_change_log_free (&l);
      return EASEL_SELECTION_NO_MEMORY;
    }
  if (l.n == 0)
    {
      easel_change_log_free (&l);
      return EASEL_SELECTION_NOTHING;
    }
  if (history != NULL && !easel_change_log_push (&l, history))
    {
      easel_change_log_restore (&l, img);
      easel_change_log_free (&l);
      return EASEL_SELECTION_NO_MEMORY;
    }
  easel_change_log_free (&l);
  return EASEL_SELECTION_DONE;
}

void
easel_float_free (EaselFloat *f)
{
  free (f->px);
  free (f->mask);
  free (f->stamps);
  *f = (EaselFloat) { 0 };
}
