/* SPDX-License-Identifier: GPL-3.0-or-later */
/* The gradient of features 0012 and 0032: the drawing color fading to transparent along a drag, Linear, Radial or Reflected,
   its opacity at a pixel (REQ-0012-04, REQ-0032-04 to -06), its blend over a picture pixel (REQ-0012-05) and its apply as one
   undo step (REQ-0012-09). All in whole numbers, so that
   the preview and the drawing are the same pixels. */
#pragma once

#include <stddef.h>
#include <stdint.h>

#include "history.h"
#include "image.h"

typedef enum
{
  EASEL_GRADIENT_LINEAR,
  EASEL_GRADIENT_RADIAL,
  EASEL_GRADIENT_REFLECTED
} EaselGradientType;

/* From the start (x0, y0) to the end (x1, y1), picture pixels taken at their centers; color is the drawing color
   (0xAARRGGBB, straight alpha). Coordinates are limited to +-1,000,000. Zero-initialized, the type is Linear. */
typedef struct
{
  int x0, y0, x1, y1;
  uint32_t color;
  EaselGradientType type;
} EaselGradient;

/* The gradient's opacity at the pixel (x, y), 0 to 255: the color's own alpha at the start, falling to 0 at the end's distance
   and 0 beyond; Linear along the drag and full behind the start (REQ-0012-04), Radial with the distance from the start
   (REQ-0032-05), Reflected along the drag both ways from the start (REQ-0032-06); 0 everywhere for a drag of no length. */
unsigned easel_gradient_opacity (const EaselGradient *g, int x, int y);

/* The picture pixel with the gradient of color at the opacity a laid over it (REQ-0012-05), all 0xAARRGGBB, straight alpha;
   a pixel with a = 0 is returned as it is. */
uint32_t easel_gradient_blend (uint32_t picture, uint32_t color, unsigned a);

/* The one function that lays the gradient over picture pixels, for the preview and for the apply (REQ-0012-07): the
   rectangle of w x h pixels at (x0, y0) is written to dst (stride: dst_stride pixels), each pixel the picture's pixel with the
   gradient laid over it (REQ-0012-05), a pixel of opacity 0 copied as it is; the places of pixels outside the picture are
   left as they were. */
void easel_gradient_lay (const EaselGradient *g, const EaselImage *img, int x0, int y0, int w, int h, uint32_t *dst,
                         size_t dst_stride);

typedef enum
{
  EASEL_GRADIENT_DONE,       /* applied and, with a history, one undo step */
  EASEL_GRADIENT_NOTHING,    /* no pixel's value changed: no step */
  EASEL_GRADIENT_NO_MEMORY   /* the picture is as it was and no step was added */
} EaselGradientResult;

/* Lays the gradient over every pixel of the picture (REQ-0012-05) and, with a history, makes it one undo step
   (REQ-0012-09). changed (may be NULL) gets the count of changed pixels, 0 unless DONE. */
EaselGradientResult easel_gradient_apply (EaselImage *img, EaselHistory *history, const EaselGradient *g, size_t *changed);

/* Test hook: the same, but it gives up with NO_MEMORY when the log of changed pixels would need more than max_bytes. */
EaselGradientResult easel_gradient_apply_capped (EaselImage *img, EaselHistory *history, const EaselGradient *g,
                                                 size_t *changed, size_t max_bytes);
