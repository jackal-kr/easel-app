/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "gradient.h"

#include "change-log.h"

#include <stdlib.h>

#define COORD_LIMIT 1000000

static long long
clamp_coord (int v)
{
  return v < -COORD_LIMIT ? -COORD_LIMIT : v > COORD_LIMIT ? COORD_LIMIT : v;
}

/* round (A (L - d) / L) with d, the dot product of p - S and E - S, limited to 0 to L, and L the square of the drag's length.
   At the coordinate limits L is at most 8 x 10^12 and 2 A L about 4 x 10^15: within 64 bits. */
unsigned
easel_gradient_opacity (const EaselGradient *g, int x, int y)
{
  long long x0 = clamp_coord (g->x0), y0 = clamp_coord (g->y0);
  long long dx = clamp_coord (g->x1) - x0, dy = clamp_coord (g->y1) - y0;
  long long length2 = dx * dx + dy * dy, d, alpha = g->color >> 24;

  if (length2 == 0)
    return 0;
  d = (clamp_coord (x) - x0) * dx + (clamp_coord (y) - y0) * dy;
  if (d < 0)
    d = 0;
  else if (d > length2)
    d = length2;
  return (unsigned) ((2 * alpha * (length2 - d) + length2) / (2 * length2));
}

/* round (p / q) for p >= 0, q > 0 */
static uint32_t
round_div (uint32_t p, uint32_t q)
{
  return (2 * p + q) / (2 * q);
}

/* M = 255 a + da (255 - a), the alpha round (M / 255), each channel round (N / M) with N = s a 255 + d da (255 - a); over an
   opaque pixel M is 255 x 255 and a channel is round ((s a + d (255 - a)) / 255). N is at most 255^3, 2 N + M within 32 bits. */
uint32_t
easel_gradient_blend (uint32_t picture, uint32_t color, unsigned a)
{
  uint32_t da = picture >> 24, m, out;

  if (a == 0)
    return picture;
  if (a > 255)
    a = 255;
  if (da == 255)
    {
      out = 0xFF000000u;
      for (int shift = 0; shift <= 16; shift += 8)
        out |= round_div ((color >> shift & 0xFF) * a + (picture >> shift & 0xFF) * (255 - a), 255) << shift;
      return out;
    }
  m = 255 * a + da * (255 - a);
  out = round_div (m, 255) << 24;
  for (int shift = 0; shift <= 16; shift += 8)
    out |= round_div ((color >> shift & 0xFF) * a * 255 + (picture >> shift & 0xFF) * da * (255 - a), m) << shift;
  return out;
}

void
easel_gradient_lay (const EaselGradient *g, const EaselImage *img, int x0, int y0, int w, int h, uint32_t *dst,
                    size_t dst_stride)
{
  /* Only the part of the rectangle inside the picture is read and written. */
  int i0 = x0 < 0 ? -x0 : 0, j0 = y0 < 0 ? -y0 : 0;
  int i1 = w < img->width - x0 ? w : img->width - x0, j1 = h < img->height - y0 ? h : img->height - y0;

  for (int j = j0; j < j1; j++)
    {
      const uint32_t *src = img->px + (size_t) (y0 + j) * img->width + (x0 + i0);    /* the first pixel inside */
      uint32_t *out = dst + (size_t) j * dst_stride + i0;

      for (int i = i0; i < i1; i++)
        {
          unsigned a = easel_gradient_opacity (g, x0 + i, y0 + j);

          out[i - i0] = a == 0 ? src[i - i0] : easel_gradient_blend (src[i - i0], g->color, a);
        }
    }
}

EaselGradientResult
easel_gradient_apply_capped (EaselImage *img, EaselHistory *history, const EaselGradient *g, size_t *changed,
                             size_t max_bytes)
{
  EaselChangeLog l;
  bool ok = true;
  uint32_t *row;

  if (changed != NULL)
    *changed = 0;
  row = malloc ((size_t) img->width * sizeof *row);
  if (row == NULL)
    return EASEL_GRADIENT_NO_MEMORY;
  easel_change_log_init (&l, max_bytes);
  /* Each row laid as the preview lays it (easel_gradient_lay), then the pixels that changed set through the log. */
  for (int y = 0; ok && y < img->height; y++)
    {
      easel_gradient_lay (g, img, 0, y, img->width, 1, row, (size_t) img->width);
      for (int x = 0; ok && x < img->width; x++)
        if (row[x] != img->px[(size_t) y * img->width + x])
          ok = easel_change_log_set (&l, img, x, y, row[x]);
    }
  free (row);
  if (!ok)
    {
      easel_change_log_restore (&l, img);
      easel_change_log_free (&l);
      return EASEL_GRADIENT_NO_MEMORY;
    }
  if (l.n == 0)
    {
      easel_change_log_free (&l);
      return EASEL_GRADIENT_NOTHING;
    }
  if (changed != NULL)
    *changed = l.n;
  if (history == NULL)
    {
      easel_change_log_free (&l);
      return EASEL_GRADIENT_DONE;
    }
  if (!easel_change_log_push (&l, history))
    {
      easel_change_log_restore (&l, img);
      easel_change_log_free (&l);
      if (changed != NULL)
        *changed = 0;
      return EASEL_GRADIENT_NO_MEMORY;
    }
  return EASEL_GRADIENT_DONE;            /* the history took the log */
}

EaselGradientResult
easel_gradient_apply (EaselImage *img, EaselHistory *history, const EaselGradient *g, size_t *changed)
{
  return easel_gradient_apply_capped (img, history, g, changed, SIZE_MAX);
}
