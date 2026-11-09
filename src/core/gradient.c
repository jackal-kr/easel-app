/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "gradient.h"

#include "change-log.h"

#include <math.h>
#include <stdlib.h>

#define COORD_LIMIT 1000000

static long long
clamp_coord (int v)
{
  return v < -COORD_LIMIT ? -COORD_LIMIT : v > COORD_LIMIT ? COORD_LIMIT : v;
}

/* The weight of a span (the color's alpha) along the drag (REQ-0012-04, REQ-0032-06): round (span (L - d) / L), d the dot product of p - S and E - S
   (its absolute value for Reflected) limited to 0 to L, L the square of the drag's length. At the coordinate limits L is at
   most 8 x 10^12 and 2 span L about 4 x 10^15: within 64 bits. */
static unsigned
weight_along (long long length2, long long d, unsigned span, int mirrored)
{
  if (mirrored && d < 0)
    d = -d;
  if (d < 0)
    d = 0;
  else if (d > length2)
    d = length2;
  return (unsigned) ((2 * (long long) span * (length2 - d) + length2) / (2 * length2));
}

/* Whether w is at most span (R - r) / R + 1/2, in whole numbers (REQ-0032-05): 4 span^2 D <= L (2 span - 2w + 1)^2. With the
   coordinates limited, D and L are at most 8 x 10^12 and span at most 255, so both sides are at most 4 x 65025 x 8 x 10^12, about
   2.1 x 10^18, below 2^64 (1.8 x 10^19): 64 bits suffice. */
static int
radial_fits (uint64_t lhs, long long length2, unsigned span, unsigned w)
{
  uint64_t m = 2 * (uint64_t) span - 2 * (uint64_t) w + 1;

  return lhs <= (uint64_t) length2 * (m * m);
}

/* Radial (REQ-0032-05): the largest w from 0 to span that fits, 0 if none. The square root in doubles gives a first w one
   below its floor: the double's error is far below 1, so that w is never above the answer (and 0 always fits when r <= R, or
   is the answer when r > R); the exact test then raises it while the next one fits, so the result is exact whatever the
   estimate, with no step down. */
static unsigned
weight_radial (long long length2, long long dist2, double ratio, unsigned span)
{
  uint64_t lhs = 4 * (uint64_t) span * span * (uint64_t) dist2;
  double estimate = span * (1.0 - ratio) + 0.5 - 1;
  unsigned w = estimate < 1 ? 0 : estimate >= span ? span : (unsigned) estimate;

  while (w < span && radial_fits (lhs, length2, span, w + 1))
    w++;
  return w;
}

/* The drag, worked out once for many pixels: the clamped start, the drag's vector and L, the type and the color's alpha. */
typedef struct
{
  long long x0, y0, dx, dy, length2;
  EaselGradientType type;
  unsigned alpha;
} Frame;

static Frame
frame_of (const EaselGradient *g)
{
  Frame f;

  f.x0 = clamp_coord (g->x0);
  f.y0 = clamp_coord (g->y0);
  f.dx = clamp_coord (g->x1) - f.x0;
  f.dy = clamp_coord (g->y1) - f.y0;
  f.length2 = f.dx * f.dx + f.dy * f.dy;
  f.type = g->type;
  f.alpha = g->color >> 24;
  return f;
}

/* The opacity at (x, y): the weight of the color's alpha, by the type's rule. */
static unsigned
opacity_in (const Frame *f, int x, int y)
{
  long long px = clamp_coord (x) - f->x0, py = clamp_coord (y) - f->y0;

  if (f->length2 == 0 || f->alpha == 0)
    return 0;
  if (f->type == EASEL_GRADIENT_RADIAL)
    {
      long long dist2 = px * px + py * py;

      return weight_radial (f->length2, dist2, sqrt ((double) dist2 / (double) f->length2), f->alpha);
    }
  return weight_along (f->length2, px * f->dx + py * f->dy, f->alpha, f->type == EASEL_GRADIENT_REFLECTED);
}

unsigned
easel_gradient_opacity (const EaselGradient *g, int x, int y)
{
  Frame f = frame_of (g);

  return opacity_in (&f, x, y);
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
  Frame f = frame_of (g);

  for (int j = j0; j < j1; j++)
    {
      const uint32_t *src = img->px + (size_t) (y0 + j) * img->width + (x0 + i0);    /* the first pixel inside */
      uint32_t *out = dst + (size_t) j * dst_stride + i0;

      for (int i = i0; i < i1; i++)
        {
          unsigned a = opacity_in (&f, x0 + i, y0 + j);

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
