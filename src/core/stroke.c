/* SPDX-License-Identifier: GPL-3.0-or-later */

#include <stdbool.h>
#include <stdlib.h>

#include <string.h>

#include "cover.h"
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
  /* Smooth (0039): for every pixel the stroke has touched, which of its 256 points the path so far covers, in tiles of 16 x 16
     pixels made when first touched, with the pixel's place in the log; NULL for the other strokes. */
  struct SmoothTile **tiles;
  int tiles_w, tiles_h;
  size_t n_tiles, max_tiles;
};

/* 16 x 16 pixels: each pixel's points, 16 rows of 16 bits, and its log entry (-1 before it changes): 9 KB. */
struct SmoothTile
{
  uint16_t points[256][16];
  int32_t slot[256];
};

static void smooth_segment (EaselStroke *s, int x0, int y0, int x1, int y1);

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
  if (s->tiles != NULL)
    smooth_segment (s, s->x, s->y, x, y);
  else
    line (s, s->x, s->y, x, y);
  s->x = x;
  s->y = y;
}

/* The tile of pixel (x, y), inside the picture; made when first needed; NULL if it cannot be had. */
static struct SmoothTile *
smooth_tile (EaselStroke *s, int x, int y)
{
  struct SmoothTile **t = &s->tiles[(size_t) (y / 16) * (size_t) s->tiles_w + (size_t) (x / 16)];

  if (*t == NULL && s->n_tiles < s->max_tiles)
    {
      *t = calloc (1, sizeof **t);
      if (*t != NULL)
        {
          memset ((*t)->slot, 0xFF, sizeof (*t)->slot);
          s->n_tiles++;
        }
    }
  return *t;
}

/* The pixel (x, y) takes value; its log entry is made the first time it changes, then its new value is updated (each pixel is
   logged once, with its value before the stroke). */
static void
smooth_set (EaselStroke *s, struct SmoothTile *t, int k, int x, int y, uint32_t value)
{
  size_t index = (size_t) y * (size_t) s->img->width + (size_t) x;

  if (t->slot[k] < 0)
    {
      if (s->img->px[index] == value)
        return;
      if (s->n_log == s->log_cap)
        {
          size_t cap = s->log_cap ? s->log_cap * 2 : 256;
          EaselChange *log = cap > SIZE_MAX / sizeof *log || cap > INT32_MAX ? NULL : realloc (s->log, cap * sizeof *log);

          if (log == NULL)
            return;                                        /* not drawn: it could not be restored */
          s->log = log;
          s->log_cap = cap;
        }
      t->slot[k] = (int32_t) s->n_log;
      s->log[s->n_log++] = (EaselChange) { (uint32_t) index, s->img->px[index], value };
    }
  else
    s->log[t->slot[k]].new_value = value;
  easel_image_set (s->img, x, y, value);
}

/* The footprint swept from (x0, y0) to (x1, y1) (REQ-0039-05 (e)): its points are added to each pixel's, and every pixel it
   reaches is mixed again from its value before the stroke by all the points of the path so far (REQ-0039-07). */
static void
smooth_segment (EaselStroke *s, int x0, int y0, int x1, int y1)
{
  EaselImage *img = s->img;
  int reach = s->footprint.size + 1;
  int top = (y0 < y1 ? y0 : y1) - reach, bottom = (y0 < y1 ? y1 : y0) + reach;

  if (top < 0)
    top = 0;
  if (bottom > img->height - 1)
    bottom = img->height - 1;
  for (int y = top; y <= bottom; y++)
    {
      int64_t lo = INT64_MAX, hi = INT64_MIN;

      for (int j = 0; j < 16; j++)
        {
          int64_t ca, cb;

          if (!easel_cover_sweep (s->footprint, x0, y0, x1, y1, 32 * (int64_t) y + 2 * j + 1, &ca, &cb))
            continue;
          if (ca < 0)
            ca = 0;
          if (cb > 16 * (int64_t) img->width - 1)
            cb = 16 * (int64_t) img->width - 1;
          for (int64_t x = ca / 16; ca <= cb && x <= cb / 16; x++)
            {
              struct SmoothTile *t = smooth_tile (s, (int) x, y);
              int from = ca > 16 * x ? (int) (ca - 16 * x) : 0, to = cb < 16 * x + 15 ? (int) (cb - 16 * x) : 15;

              if (t == NULL)
                continue;                                  /* left as it is (REQ-0039-08) */
              t->points[(y % 16) * 16 + x % 16][j] |= (uint16_t) (((1u << (to + 1)) - 1) & ~((1u << from) - 1));
              lo = x < lo ? x : lo;
              hi = x > hi ? x : hi;
            }
        }
      for (int64_t x = lo; x <= hi; x++)
        {
          struct SmoothTile *t = s->tiles[(size_t) (y / 16) * (size_t) s->tiles_w + (size_t) (x / 16)];
          int k = (y % 16) * 16 + (int) (x % 16);
          unsigned n = 0;
          uint32_t before;

          if (t == NULL)
            continue;
          for (int j = 0; j < 16; j++)
            n += (unsigned) __builtin_popcount (t->points[k][j]);
          if (n == 0)
            continue;
          before = t->slot[k] >= 0 ? s->log[t->slot[k]].old_value : easel_image_get (img, (int) x, y);
          smooth_set (s, t, k, (int) x, y, easel_cover_mix (before, s->color, n, 0, 0));
        }
    }
}

EaselStroke *
easel_stroke_begin_smooth (EaselImage *img, int x, int y, uint32_t color, EaselFootprint f)
{
  EaselStroke *s = stroke_alloc (img, x, y, color, f, NULL);

  if (s == NULL)
    return NULL;
  s->tiles_w = (img->width + 15) / 16;
  s->tiles_h = (img->height + 15) / 16;
  s->max_tiles = SIZE_MAX;
  s->tiles = calloc ((size_t) s->tiles_w * (size_t) s->tiles_h + 1, sizeof *s->tiles);
  if (s->tiles == NULL)
    {
      free (s);
      return NULL;
    }
  smooth_segment (s, s->x, s->y, s->x, s->y);
  return s;
}

void
easel_stroke_limit_tiles (EaselStroke *s, size_t max_tiles)
{
  s->max_tiles = max_tiles;
}

static void
free_tiles (EaselStroke *s)
{
  if (s->tiles == NULL)
    return;
  for (size_t i = 0; i < (size_t) s->tiles_w * (size_t) s->tiles_h; i++)
    free (s->tiles[i]);
  free (s->tiles);
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
  free_tiles (s);
  free (s);
  return changed;
}

void
easel_stroke_cancel (EaselStroke *s)
{
  restore (s);
  free (s->log);
  free (s->disc);
  free_tiles (s);
  free (s);
}
