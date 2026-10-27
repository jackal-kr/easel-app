/* SPDX-License-Identifier: GPL-3.0-or-later */

#include "text-put.h"

#include "change-log.h"

/* The value the put-down gives the pixel (x, y): the ink where masked, else the background inside an Opaque box, else its own. */
static uint32_t
value_at (const EaselImage *img, int x, int y, int bx, int by, int bw, int bh, const EaselTextInk *ink, bool opaque,
          uint32_t background)
{
  if (ink->mask != NULL && x >= ink->x && x < ink->x + ink->w && y >= ink->y && y < ink->y + ink->h)
    {
      size_t i = (size_t) (y - ink->y) * (size_t) ink->w + (size_t) (x - ink->x);

      if (ink->mask[i])
        return ink->color[i];
    }
  if (opaque && x >= bx && x < bx + bw && y >= by && y < by + bh)
    return background;
  return easel_image_get (img, x, y);
}

EaselSelectionResult
easel_text_put_down (EaselImage *img, EaselHistory *history, int x, int y, int w, int h, const EaselTextInk *ink, bool opaque,
                     uint32_t background, size_t max_bytes)
{
  EaselChangeLog l;
  int x0, y0, x1, y1;
  bool ok = true;

  /* What can change: the box if Opaque, and the ink; cut to the picture. */
  x0 = opaque ? x : ink->x;
  y0 = opaque ? y : ink->y;
  x1 = opaque ? x + w : ink->x + ink->w;
  y1 = opaque ? y + h : ink->y + ink->h;
  if (opaque && ink->w > 0 && ink->h > 0)
    {
      x0 = x0 < ink->x ? x0 : ink->x;
      y0 = y0 < ink->y ? y0 : ink->y;
      x1 = x1 > ink->x + ink->w ? x1 : ink->x + ink->w;
      y1 = y1 > ink->y + ink->h ? y1 : ink->y + ink->h;
    }
  x0 = x0 > 0 ? x0 : 0;
  y0 = y0 > 0 ? y0 : 0;
  x1 = x1 < img->width ? x1 : img->width;
  y1 = y1 < img->height ? y1 : img->height;
  easel_change_log_init (&l, max_bytes);
  for (int py = y0; ok && py < y1; py++)
    for (int px = x0; ok && px < x1; px++)
      ok = easel_change_log_set (&l, img, px, py, value_at (img, px, py, x, y, w, h, ink, opaque, background));
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
