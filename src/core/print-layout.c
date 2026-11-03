/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "print-layout.h"

#include <math.h>

#define MM_PER_PIXEL (25.4 / 96)
/* What a division that should be whole may lose in rounding: a picture that exactly fits stays on its sheet. */
#define WHOLE_SLACK 1e-9

static int
pixels_on_sheet (double area_mm, double percent)
{
  int n = (int) floor (area_mm / (MM_PER_PIXEL * percent / 100) + WHOLE_SLACK);

  return n < 1 ? 1 : n;
}

static int
ceil_div (int a, int b)
{
  return (a + b - 1) / b;
}

EaselPrintLayout
easel_print_layout (int pic_w, int pic_h, double area_w_mm, double area_h_mm, bool fit, int percent)
{
  EaselPrintLayout l;

  l.percent = percent;
  if (fit)
    {
      double across = 100 * area_w_mm / (pic_w * MM_PER_PIXEL);
      double down = 100 * area_h_mm / (pic_h * MM_PER_PIXEL);

      l.percent = fmin (100, fmin (across, down));
    }
  l.pixels_across = pixels_on_sheet (area_w_mm, l.percent);
  l.pixels_down = pixels_on_sheet (area_h_mm, l.percent);
  l.sheets_across = ceil_div (pic_w, l.pixels_across);
  l.sheets_down = ceil_div (pic_h, l.pixels_down);
  return l;
}

int
easel_print_sheets (const EaselPrintLayout *l)
{
  return l->sheets_across * l->sheets_down;
}

void
easel_print_sheet (const EaselPrintLayout *l, int pic_w, int pic_h, int index, int *x, int *y, int *w, int *h)
{
  *x = (index % l->sheets_across) * l->pixels_across;
  *y = (index / l->sheets_across) * l->pixels_down;
  *w = pic_w - *x < l->pixels_across ? pic_w - *x : l->pixels_across;
  *h = pic_h - *y < l->pixels_down ? pic_h - *y : l->pixels_down;
}

void
easel_print_place (const EaselPrintLayout *l, int pic_w, int pic_h, double area_w_mm, double area_h_mm, bool center_h, bool center_v,
                   double *x_mm, double *y_mm)
{
  double pixel = MM_PER_PIXEL * l->percent / 100;

  *x_mm = center_h && l->sheets_across == 1 ? (area_w_mm - pic_w * pixel) / 2 : 0;
  *y_mm = center_v && l->sheets_down == 1 ? (area_h_mm - pic_h * pixel) / 2 : 0;
}

bool
easel_print_area_ok (double area_w_mm, double area_h_mm, bool fit, int percent)
{
  double least = fit ? 0 : MM_PER_PIXEL * percent / 100;

  if (fit)
    return area_w_mm > least && area_h_mm > least;
  return area_w_mm >= least && area_h_mm >= least;
}

bool
easel_print_wide_is_landscape (int w, int h)
{
  return w > h;
}

void
easel_print_average (const uint32_t *src, int src_w, int src_h, size_t stride, double pixels_per_dot, uint32_t *dst, int dst_w, int dst_h)
{
  for (int dy = 0; dy < dst_h; dy++)
    {
      double top = dy * pixels_per_dot, bottom = fmin ((dy + 1) * pixels_per_dot, src_h);

      for (int dx = 0; dx < dst_w; dx++)
        {
          double left = dx * pixels_per_dot, right = fmin ((dx + 1) * pixels_per_dot, src_w);
          double sum[3] = { 0, 0, 0 }, area = 0;

          for (int y = (int) floor (top); y < src_h && y < bottom; y++)
            {
              double cover_y = fmin (y + 1, bottom) - fmax (y, top);

              for (int x = (int) floor (left); x < src_w && x < right; x++)
                {
                  double cover = cover_y * (fmin (x + 1, right) - fmax (x, left));
                  uint32_t c = src[(size_t) y * stride + (size_t) x];

                  sum[0] += cover * (double) ((c >> 16) & 0xFF);
                  sum[1] += cover * (double) ((c >> 8) & 0xFF);
                  sum[2] += cover * (double) (c & 0xFF);
                  area += cover;
                }
            }
          dst[(size_t) dy * (size_t) dst_w + (size_t) dx] = area <= 0 ? 0xFFFFFFFFu
            : 0xFF000000u | ((uint32_t) floor (sum[0] / area + 0.5) << 16) | ((uint32_t) floor (sum[1] / area + 0.5) << 8)
              | (uint32_t) floor (sum[2] / area + 0.5);
        }
    }
}
