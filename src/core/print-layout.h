/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Print (0026): how a picture lies on paper. Sizes are in millimeters; a picture pixel at 100 % is 1/96 inch. */
#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct
{
  double percent;       /* of 1/96 inch to a pixel's side */
  int pixels_across;    /* the whole pixels a sheet holds */
  int pixels_down;
  int sheets_across;
  int sheets_down;
} EaselPrintLayout;

/* REQ-0026-06, -07: with fit, the largest percent not above 100 at which the picture fits one sheet; else percent. */
EaselPrintLayout easel_print_layout (int pic_w, int pic_h, double area_w_mm, double area_h_mm, bool fit, int percent);
int easel_print_sheets (const EaselPrintLayout *l);
/* The picture pixels of a sheet; sheets count left to right, then top to bottom (REQ-0026-07). */
void easel_print_sheet (const EaselPrintLayout *l, int pic_w, int pic_h, int index, int *x, int *y, int *w, int *h);
/* Where a sheet's pixels start inside the printable area (REQ-0026-08). */
void easel_print_place (const EaselPrintLayout *l, int pic_w, int pic_h, double area_w_mm, double area_h_mm, bool center_h, bool center_v,
                        double *x_mm, double *y_mm);
/* Whether the printable area has room: one pixel at Adjust to, anything at Fit to (REQ-0026-05). */
bool easel_print_area_ok (double area_w_mm, double area_h_mm, bool fit, int percent);
/* The orientation that follows the picture (REQ-0026-04). */
bool easel_print_wide_is_landscape (int w, int h);
/* Each of dst_w x dst_h dots the average of the opaque pixels of src it covers, by the area covered, a dot being pixels_per_dot pixels on a side
   (REQ-0026-09). src has stride pixels to a row. */
void easel_print_average (const uint32_t *src, int src_w, int src_h, size_t stride, double pixels_per_dot, uint32_t *dst, int dst_w, int dst_h);
