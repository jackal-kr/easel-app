/* SPDX-License-Identifier: GPL-3.0-or-later */

#include <stdlib.h>

#include "stroke.h"

/* Pointer positions are far smaller; the clamp only keeps an absurd input from walking billions of steps. */
#define COORD_LIMIT 1000000

struct _EaselStroke
{
  EaselImage *img;
  uint32_t color;
  int x, y;
  EaselChange *log;       /* each changed pixel once, in change order; becomes the undo step */
  size_t n_log, log_cap;
};

static int
clamp_coord (int v)
{
  return v < -COORD_LIMIT ? -COORD_LIMIT : v > COORD_LIMIT ? COORD_LIMIT : v;
}

/* The stroke has one color, so a pixel changes at most once and the log holds its value from before the stroke. */
static void
plot (EaselStroke *s, int x, int y)
{
  EaselImage *img = s->img;
  size_t index;

  if (x < 0 || y < 0 || x >= img->width || y >= img->height)
    return;
  index = (size_t) y * (size_t) img->width + (size_t) x;
  if (img->px[index] == s->color)
    return;

  if (s->n_log == s->log_cap)
    {
      size_t cap = s->log_cap ? s->log_cap * 2 : 256;
      EaselChange *log = cap > SIZE_MAX / sizeof *log ? NULL : realloc (s->log, cap * sizeof *log);

      /* Without a log entry the pixel could not be restored on cancel, so it is not drawn. */
      if (log == NULL)
        return;
      s->log = log;
      s->log_cap = cap;
    }
  s->log[s->n_log++] = (EaselChange) { (uint32_t) index, img->px[index], s->color };
  easel_image_set (img, x, y, s->color);
}

/* Bresenham: 8-connected, one pixel per step along the major axis, both ends included. */
static void
line (EaselStroke *s, int x0, int y0, int x1, int y1)
{
  int dx = abs (x1 - x0), dy = -abs (y1 - y0);
  int sx = x0 < x1 ? 1 : -1, sy = y0 < y1 ? 1 : -1;
  long long err = (long long) dx + dy;

  for (;;)
    {
      long long e2 = 2 * err;

      plot (s, x0, y0);
      if (x0 == x1 && y0 == y1)
        break;
      if (e2 >= dy)
        {
          err += dy;
          x0 += sx;
        }
      if (e2 <= dx)
        {
          err += dx;
          y0 += sy;
        }
    }
}

EaselStroke *
easel_stroke_begin (EaselImage *img, int x, int y, uint32_t color)
{
  EaselStroke *s = calloc (1, sizeof *s);

  if (s == NULL)
    return NULL;
  /* A step stores 32-bit pixel indexes. */
  if ((size_t) img->width * (size_t) img->height > UINT32_MAX)
    {
      free (s);
      return NULL;
    }
  s->img = img;
  s->color = color;
  s->x = clamp_coord (x);
  s->y = clamp_coord (y);
  plot (s, s->x, s->y);
  return s;
}

void
easel_stroke_to (EaselStroke *s, int x, int y)
{
  x = clamp_coord (x);
  y = clamp_coord (y);
  line (s, s->x, s->y, x, y);
  s->x = x;
  s->y = y;
}

/* Puts every changed pixel back, newest first. */
static void
restore (EaselStroke *s)
{
  EaselImage *img = s->img;

  for (size_t i = s->n_log; i-- > 0;)
    {
      uint32_t index = s->log[i].index;

      easel_image_set (img, (int) (index % (uint32_t) img->width), (int) (index / (uint32_t) img->width),
                       s->log[i].old_value);
    }
}

size_t
easel_stroke_end (EaselStroke *s, EaselHistory *history)
{
  size_t changed = s->n_log;

  if (history != NULL && changed > 0)
    {
      /* The history takes the log as the step. If it cannot, the edit is refused: the picture goes back. */
      if (easel_history_push (history, s->log, changed))
        s->log = NULL;
      else
        {
          restore (s);
          changed = 0;
        }
    }
  free (s->log);
  free (s);
  return changed;
}

void
easel_stroke_cancel (EaselStroke *s)
{
  restore (s);
  free (s->log);
  free (s);
}
