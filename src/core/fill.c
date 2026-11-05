/* SPDX-License-Identifier: GPL-3.0-or-later */
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "fill.h"

/* A seed waiting for its run to be filled. */
typedef struct
{
  int x, y;
} Seed;

typedef struct
{
  Seed *seeds;
  size_t n_seeds, cap_seeds;
  EaselSpan *spans;
  size_t n_spans, cap_spans;
  size_t max_bytes;       /* what each of the two buffers may hold on its own; SIZE_MAX unless a test lowers it */
} Work;

/* Doubles a buffer; false if the size cannot be had. */
static bool
grow (const Work *w, void **buf, size_t *cap, size_t n, size_t item)
{
  size_t new_cap;
  void *p;

  if (n < *cap)
    return true;
  new_cap = *cap ? *cap * 2 : 64;
  if (new_cap > SIZE_MAX / item || new_cap * item > w->max_bytes)
    return false;
  p = realloc (*buf, new_cap * item);
  if (p == NULL)
    return false;
  *buf = p;
  *cap = new_cap;
  return true;
}

static bool
push_seed (Work *w, int x, int y)
{
  if (!grow (w, (void **) &w->seeds, &w->cap_seeds, w->n_seeds, sizeof *w->seeds))
    return false;
  w->seeds[w->n_seeds++] = (Seed) { x, y };
  return true;
}

/* Seeds for the runs of the target color in row y between xl and xr: one for the start of each run. */
static bool
seed_row (Work *w, const EaselImage *img, int y, int xl, int xr, uint32_t target)
{
  bool in_run = false;

  if (y < 0 || y >= img->height)
    return true;
  for (int x = xl; x <= xr; x++)
    {
      bool is_target = img->px[(size_t) y * (size_t) img->width + (size_t) x] == target;

      if (is_target && !in_run && !push_seed (w, x, y))
        return false;
      in_run = is_target;
    }
  return true;
}

static void
undo_spans (EaselImage *img, const Work *w, uint32_t target)
{
  for (size_t i = 0; i < w->n_spans; i++)
    for (uint32_t x = w->spans[i].x0; x <= w->spans[i].x1; x++)
      easel_image_set (img, (int) x, (int) w->spans[i].y, target);
}

EaselFillResult
easel_fill (EaselImage *img, EaselHistory *history, int x, int y, uint32_t color, size_t *changed)
{
  return easel_fill_capped (img, history, x, y, color, changed, SIZE_MAX);
}

EaselFillResult
easel_fill_capped (EaselImage *img, EaselHistory *history, int x, int y, uint32_t color, size_t *changed, size_t max_bytes)
{
  Work w = { .max_bytes = max_bytes };
  uint32_t target;
  size_t pixels = 0;

  if (changed != NULL)
    *changed = 0;
  if (x < 0 || y < 0 || x >= img->width || y >= img->height)
    return EASEL_FILL_NOTHING;
  target = img->px[(size_t) y * (size_t) img->width + (size_t) x];
  if (target == color)
    return EASEL_FILL_NOTHING;

  /* A filled pixel has `color`, which is not the target, so it is never taken again: no visited bitmap is needed. */
  if (!push_seed (&w, x, y))
    goto no_memory;
  while (w.n_seeds > 0)
    {
      Seed s = w.seeds[--w.n_seeds];
      int xl = s.x, xr = s.x;
      const uint32_t *row = img->px + (size_t) s.y * (size_t) img->width;

      if (row[s.x] != target)
        continue;
      while (xl > 0 && row[xl - 1] == target)
        xl--;
      while (xr < img->width - 1 && row[xr + 1] == target)
        xr++;
      if (!grow (&w, (void **) &w.spans, &w.cap_spans, w.n_spans, sizeof *w.spans))
        goto no_memory;
      w.spans[w.n_spans++] = (EaselSpan) { (uint32_t) s.y, (uint32_t) xl, (uint32_t) xr };
      for (int i = xl; i <= xr; i++)
        easel_image_set (img, i, s.y, color);
      pixels += (size_t) (xr - xl + 1);
      if (!seed_row (&w, img, s.y - 1, xl, xr, target) || !seed_row (&w, img, s.y + 1, xl, xr, target))
        goto no_memory;
    }
  free (w.seeds);
  w.seeds = NULL;
  if (history != NULL)
    {
      /* The history takes the spans, or refuses for lack of memory: then the picture goes back. */
      if (!easel_history_push_fill (history, target, color, w.spans, w.n_spans))
        goto no_memory;
    }
  else
    free (w.spans);
  if (changed != NULL)
    *changed = pixels;
  return EASEL_FILL_DONE;

no_memory:
  undo_spans (img, &w, target);
  free (w.seeds);
  free (w.spans);
  return EASEL_FILL_NO_MEMORY;
}
