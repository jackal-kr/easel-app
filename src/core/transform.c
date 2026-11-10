/* SPDX-License-Identifier: GPL-3.0-or-later */

#include "transform.h"

#include <stdlib.h>
#include <string.h>

/* Where the pixel (x, y) of a w x h picture goes under o, and the size after it. */
static void
forward (EaselOrient o, int w, int h, int x, int y, int *nx, int *ny, int *nw, int *nh)
{
  bool t = (o & EASEL_ORIENT_TRANSPOSE) != 0;
  int ow = t ? h : w, oh = t ? w : h;
  int ax = t ? y : x, ay = t ? x : y;

  if (o & EASEL_ORIENT_FLIP_X)
    ax = ow - 1 - ax;
  if (o & EASEL_ORIENT_FLIP_Y)
    ay = oh - 1 - ay;
  *nx = ax;
  *ny = ay;
  *nw = ow;
  *nh = oh;
}

EaselOrient
easel_op_orient (EaselOp op)
{
  switch (op)
    {
    case EASEL_OP_FLIP_H:
      return EASEL_ORIENT_FLIP_X;
    case EASEL_OP_FLIP_V:
      return EASEL_ORIENT_FLIP_Y;
    case EASEL_OP_ROTATE_90:
      return EASEL_ORIENT_TRANSPOSE | EASEL_ORIENT_FLIP_X;
    case EASEL_OP_ROTATE_180:
      return EASEL_ORIENT_FLIP_X | EASEL_ORIENT_FLIP_Y;
    case EASEL_OP_ROTATE_270:
      return EASEL_ORIENT_TRANSPOSE | EASEL_ORIENT_FLIP_Y;
    default:
      return EASEL_ORIENT_IDENTITY;
    }
}

/* The orientation that moves four points of a 2 x 3 picture where `first` then `then` move them: the eight differ on them. */
EaselOrient
easel_orient_compose (EaselOrient first, EaselOrient then)
{
  static const int pts[4][2] = { { 0, 0 }, { 1, 0 }, { 0, 1 }, { 1, 2 } };

  for (EaselOrient c = 0; c < 8; c++)
    {
      bool same = true;

      for (int i = 0; i < 4 && same; i++)
        {
          int x, y, w, h, ex, ey, cx, cy, cw, ch;

          forward (first, 2, 3, pts[i][0], pts[i][1], &x, &y, &w, &h);
          forward (then, w, h, x, y, &ex, &ey, &w, &h);
          forward (c, 2, 3, pts[i][0], pts[i][1], &cx, &cy, &cw, &ch);
          same = ex == cx && ey == cy && w == cw && h == ch;
        }
      if (same)
        return c;
    }
  return EASEL_ORIENT_IDENTITY;
}

EaselOrient
easel_orient_inverse (EaselOrient o)
{
  for (EaselOrient c = 0; c < 8; c++)
    if (easel_orient_compose (o, c) == EASEL_ORIENT_IDENTITY)
      return c;
  return EASEL_ORIENT_IDENTITY;
}

bool
easel_orient_swaps (EaselOrient o)
{
  return (o & EASEL_ORIENT_TRANSPOSE) != 0;
}

void
easel_orient_size (EaselOrient o, int w, int h, int *ow, int *oh)
{
  *ow = easel_orient_swaps (o) ? h : w;
  *oh = easel_orient_swaps (o) ? w : h;
}

void
easel_orient_source (EaselOrient o, int w, int h, int dx, int dy, int *sx, int *sy)
{
  int ow, oh, ax = dx, ay = dy;

  easel_orient_size (o, w, h, &ow, &oh);
  if (o & EASEL_ORIENT_FLIP_X)
    ax = ow - 1 - dx;
  if (o & EASEL_ORIENT_FLIP_Y)
    ay = oh - 1 - dy;
  *sx = easel_orient_swaps (o) ? ay : ax;
  *sy = easel_orient_swaps (o) ? ax : ay;
}

void
easel_orient_target (EaselOrient o, int w, int h, int x, int y, int *tx, int *ty)
{
  int ow, oh;

  forward (o, w, h, x, y, tx, ty, &ow, &oh);
}

/* The block side of a transposing copy: both the rows read and the rows written stay in cache. */
#define BLOCK 64

void
easel_orient_pixels (const uint32_t *src, int w, int h, EaselOrient o, uint32_t *dst)
{
  int ow, oh;

  easel_orient_size (o, w, h, &ow, &oh);
  if (!easel_orient_swaps (o))
    {
      /* whole rows: copied, or copied backwards for a mirrored x */
      for (int dy = 0; dy < oh; dy++)
        {
          const uint32_t *row = src + (size_t) ((o & EASEL_ORIENT_FLIP_Y) ? oh - 1 - dy : dy) * (size_t) w;
          uint32_t *out = dst + (size_t) dy * (size_t) ow;

          if (o & EASEL_ORIENT_FLIP_X)
            for (int dx = 0; dx < ow; dx++)
              out[dx] = row[ow - 1 - dx];
          else
            memcpy (out, row, (size_t) ow * sizeof *out);
        }
      return;
    }
  /* A transpose: the pixel at (dx, dy) comes from column ay, row ax of the source (easel_orient_source, with the mirrors applied to
     the oriented coordinates first), so a row of the result is a column of the source. */
  for (int by = 0; by < oh; by += BLOCK)
    for (int bx = 0; bx < ow; bx += BLOCK)
      for (int dy = by; dy < by + BLOCK && dy < oh; dy++)
        {
          size_t ay = (o & EASEL_ORIENT_FLIP_Y) ? (size_t) (oh - 1 - dy) : (size_t) dy;
          uint32_t *out = dst + (size_t) dy * (size_t) ow;

          for (int dx = bx; dx < bx + BLOCK && dx < ow; dx++)
            {
              size_t ax = (o & EASEL_ORIENT_FLIP_X) ? (size_t) (ow - 1 - dx) : (size_t) dx;

              out[dx] = src[ax * (size_t) w + ay];
            }
        }
}

/* What the command makes of the pixel at (x, y) of a picture that keeps its size. */
static uint32_t
new_value (const EaselImage *img, EaselOp op, EaselOrient o, uint32_t background, int x, int y)
{
  int sx, sy;

  switch (op)
    {
    case EASEL_OP_INVERT:
      {
        uint32_t p = img->px[(size_t) y * (size_t) img->width + (size_t) x];

        return (p >> 24) != 0 ? p ^ 0x00FFFFFFu : p;   /* fully transparent: untouched (P7.7) */
      }
    case EASEL_OP_CLEAR:
      return easel_px_canon (background);
    default:
      easel_orient_source (o, img->width, img->height, x, y, &sx, &sy);
      return img->px[(size_t) sy * (size_t) img->width + (size_t) sx];
    }
}

EaselTransformResult
easel_image_transform (EaselImage *img, EaselHistory *h, EaselOp op, uint32_t background, size_t max_bytes)
{
  EaselOrient o = easel_op_orient (op);
  int w = img->width, hgt = img->height;
  size_t total = (size_t) w * (size_t) hgt, changed = 0;
  bool quarter = easel_orient_swaps (o);

  /* How many pixels it changes, counted before anything is allocated or touched, and only as far as the answer needs: a quarter turn
     is a buffer step whatever it changes, so one changed pixel is enough; for the rest, more than a third of the pixels (12 bytes a
     change against 4 a pixel) already means a buffer step. A quarter turn of a picture that is not square changes its size. */
  if (quarter && w != hgt)
    changed = total;
  else
    {
      size_t cap = quarter ? 0 : total / 3;

      for (int y = 0; y < hgt && changed <= cap; y++)
        for (int x = 0; x < w; x++)
          changed += new_value (img, op, o, background, x, y) != img->px[(size_t) y * (size_t) w + (size_t) x];
    }
  if (changed == 0)
    return EASEL_TRANSFORM_NOTHING;

  if (!quarter && changed <= total / 3)
    {
      /* A log of the changed pixels, done in place; their new values are read from the picture before any is written. */
      EaselChange *log;
      size_t n = 0;

      if (changed * sizeof *log > max_bytes || (log = malloc (changed * sizeof *log)) == NULL)
        return EASEL_TRANSFORM_NO_MEMORY;
      for (int y = 0; y < hgt; y++)
        for (int x = 0; x < w; x++)
          {
            uint32_t now = img->px[(size_t) y * (size_t) w + (size_t) x], next = new_value (img, op, o, background, x, y);

            if (next != now && n < changed)
              log[n++] = (EaselChange) { (uint32_t) ((size_t) y * (size_t) w + (size_t) x), now, next };
          }
      for (size_t i = 0; i < n; i++)
        easel_image_set (img, (int) (log[i].index % (uint32_t) w), (int) (log[i].index / (uint32_t) w), log[i].new_value);
      if (!easel_history_push (h, log, n))
        {
          for (size_t i = 0; i < n; i++)
            easel_image_set (img, (int) (log[i].index % (uint32_t) w), (int) (log[i].index / (uint32_t) w), log[i].old_value);
          free (log);
          return EASEL_TRANSFORM_NO_MEMORY;
        }
      return EASEL_TRANSFORM_DONE;
    }

  {
    /* The picture's old buffer goes whole into the step; the new pixels are made in a buffer of their own first. */
    int nw, nh;
    uint32_t *px;

    easel_orient_size (o, w, hgt, &nw, &nh);
    if (total * sizeof *px > max_bytes || (px = malloc (total * sizeof *px)) == NULL)
      return EASEL_TRANSFORM_NO_MEMORY;
    if (op == EASEL_OP_INVERT)
      for (size_t i = 0; i < total; i++)
        px[i] = (img->px[i] >> 24) != 0 ? img->px[i] ^ 0x00FFFFFFu : img->px[i];
    else if (op == EASEL_OP_CLEAR)
      for (size_t i = 0; i < total; i++)
        px[i] = easel_px_canon (background);
    else
      easel_orient_pixels (img->px, w, hgt, o, px);
    if (!easel_history_push_buffer (h, w, hgt, img->px))
      {
        free (px);
        return EASEL_TRANSFORM_NO_MEMORY;
      }
    img->px = px;
    img->width = nw;
    img->height = nh;
    img->dirty = (EaselRect) { 0, 0, nw, nh };
  }
  return EASEL_TRANSFORM_DONE;
}

EaselTransformResult
easel_image_replace_pixels (EaselImage *img, EaselHistory *h, uint32_t *px, size_t max_bytes)
{
  size_t total = (size_t) img->width * (size_t) img->height, changed = 0, n = 0;
  EaselChange *log;

  for (size_t i = 0; i < total; i++)
    changed += px[i] != img->px[i];
  if (changed == 0)
    {
      free (px);
      return EASEL_TRANSFORM_NOTHING;
    }
  if (changed > total / 3)
    {
      /* The old buffer goes whole into the step (REQ-0018-09), the new one becomes the picture. */
      if (!easel_history_push_buffer (h, img->width, img->height, img->px))
        {
          free (px);
          return EASEL_TRANSFORM_NO_MEMORY;
        }
      img->px = px;
      img->dirty = (EaselRect) { 0, 0, img->width, img->height };
      return EASEL_TRANSFORM_DONE;
    }
  if (changed * sizeof *log > max_bytes || (log = malloc (changed * sizeof *log)) == NULL)
    {
      free (px);
      return EASEL_TRANSFORM_NO_MEMORY;
    }
  for (size_t i = 0; i < total; i++)
    if (px[i] != img->px[i])
      log[n++] = (EaselChange) { (uint32_t) i, img->px[i], px[i] };
  free (px);
  for (size_t i = 0; i < n; i++)
    easel_image_set (img, (int) (log[i].index % (uint32_t) img->width), (int) (log[i].index / (uint32_t) img->width),
                     log[i].new_value);
  if (!easel_history_push (h, log, n))
    {
      for (size_t i = 0; i < n; i++)
        easel_image_set (img, (int) (log[i].index % (uint32_t) img->width), (int) (log[i].index / (uint32_t) img->width),
                         log[i].old_value);
      free (log);
      return EASEL_TRANSFORM_NO_MEMORY;
    }
  return EASEL_TRANSFORM_DONE;
}
