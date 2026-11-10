/* SPDX-License-Identifier: GPL-3.0-or-later */

#include <stdbool.h>
#include <stdlib.h>

#include "stroke.h"

/* Pointer positions are far smaller; the clamp only keeps an absurd input from walking billions of steps. */
#define COORD_LIMIT 1000000

struct _EaselStroke
{
  EaselImage *img;
  uint32_t color;
  EaselFootprint footprint;   /* what one position paints; a square of 1 is the Pencil's pixel */
  bool has_only;              /* the color eraser: only pixels equal to `only` are changed */
  uint32_t only;
  /* The spray: the offsets (column, row in the disc's box) of the disc's pixels, how many pixels a spray paints, and the
     state of the random generator. */
  uint16_t *disc;             /* NULL for a stamping stroke; 2 values (column, row) per pixel */
  size_t n_disc;
  int per_spray;
  uint64_t rng;
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
  if (img->px[index] == s->color || (s->has_only && img->px[index] != s->only))
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

/* One position of the stroke: the footprint with the pointer's pixel at its anchor (REQ-0006-05). A pixel that several
   stamps cover is changed, and logged, once, because the next stamp finds it already of the color. */
static void
stamp (EaselStroke *s, int x, int y)
{
  int anchor = easel_footprint_anchor (s->footprint.size);

  for (int row = 0; row < s->footprint.size; row++)
    for (int col = 0; col < s->footprint.size; col++)
      if (easel_footprint_has (s->footprint, col, row))
        plot (s, x - anchor + col, y - anchor + row);
}

/* xorshift64*: small, fast, and good enough to pick pixels; a seed of 0 would stay 0, so it is replaced. */
static uint64_t
next_random (EaselStroke *s)
{
  uint64_t x = s->rng;

  x ^= x >> 12;
  x ^= x << 25;
  x ^= x >> 27;
  s->rng = x;
  return x * UINT64_C (2685821657736338717);
}

/* A number from 0 to n - 1, each with the same chance (the few values that would make some more likely are thrown away). */
static size_t
random_below (EaselStroke *s, size_t n)
{
  uint64_t limit = UINT64_MAX - UINT64_MAX % n, v;

  do
    v = next_random (s);
  while (v >= limit);
  return (size_t) (v % n);
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

      stamp (s, x0, y0);
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
  return easel_stroke_begin_stamp (img, x, y, color, (EaselFootprint) { EASEL_SHAPE_SQUARE, 1 }, NULL);
}

/* A stroke that has painted nothing yet; the callers stamp or spray. */
static EaselStroke *
stroke_alloc (EaselImage *img, int x, int y, uint32_t color, EaselFootprint f, const uint32_t *only)
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
  s->color = easel_px_canon (color);           /* P7.7: a fully transparent color draws 0 */
  s->footprint = f;
  s->has_only = only != NULL;
  s->only = only ? *only : 0;
  s->x = clamp_coord (x);
  s->y = clamp_coord (y);
  return s;
}

EaselStroke *
easel_stroke_begin_stamp (EaselImage *img, int x, int y, uint32_t color, EaselFootprint f, const uint32_t *only)
{
  EaselStroke *s = stroke_alloc (img, x, y, color, f, only);

  if (s != NULL)
    stamp (s, s->x, s->y);
  return s;
}

int
easel_spray_due (int64_t held_us)
{
  return held_us < 0 ? 1 : (int) (1 + held_us / 5000);
}

int
easel_spray_pixels (int diameter)
{
  return diameter >= 24 ? 3 : 1;
}

EaselStroke *
easel_stroke_begin_spray (EaselImage *img, uint32_t color, int diameter, int per_spray, uint64_t seed)
{
  EaselFootprint disc = { EASEL_SHAPE_CIRCLE, diameter };
  EaselStroke *s;

  if (diameter < 1 || diameter > 64 || per_spray < 1)
    return NULL;
  s = stroke_alloc (img, 0, 0, color, disc, NULL);
  if (s == NULL)
    return NULL;
  s->disc = malloc ((size_t) diameter * (size_t) diameter * 2 * sizeof *s->disc);
  if (s->disc == NULL)
    {
      easel_stroke_cancel (s);
      return NULL;
    }
  for (int row = 0; row < diameter; row++)
    for (int col = 0; col < diameter; col++)
      if (easel_footprint_has (disc, col, row))
        {
          s->disc[2 * s->n_disc] = (uint16_t) col;
          s->disc[2 * s->n_disc + 1] = (uint16_t) row;
          s->n_disc++;
        }
  s->per_spray = per_spray;
  s->rng = seed ? seed : UINT64_C (0x9E3779B97F4A7C15);
  return s;
}

void
easel_stroke_spray (EaselStroke *s, int x, int y, int sprays)
{
  int anchor;

  if (s->disc == NULL || s->n_disc == 0)
    return;
  anchor = easel_footprint_anchor (s->footprint.size);
  x = clamp_coord (x);
  y = clamp_coord (y);
  for (int i = 0; i < sprays; i++)
    for (int k = 0; k < s->per_spray; k++)
      {
        size_t pick = random_below (s, s->n_disc);

        plot (s, x - anchor + s->disc[2 * pick], y - anchor + s->disc[2 * pick + 1]);
      }
}

void
easel_stroke_to (EaselStroke *s, int x, int y)
{
  if (s->disc != NULL)
    return;
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
  free (s->disc);
  free (s);
  return changed;
}

void
easel_stroke_cancel (EaselStroke *s)
{
  restore (s);
  free (s->log);
  free (s->disc);
  free (s);
}
