/* SPDX-License-Identifier: GPL-3.0-or-later */
#include <stdlib.h>

#include "change-log.h"

void
easel_change_log_init (EaselChangeLog *l, size_t max_bytes)
{
  *l = (EaselChangeLog) { NULL, 0, 0, max_bytes };
}

bool
easel_change_log_set (EaselChangeLog *l, EaselImage *img, int x, int y, uint32_t value)
{
  size_t index = (size_t) y * (size_t) img->width + (size_t) x;
  uint32_t old = img->px[index];

  if (old == value)
    return true;
  if (l->n == l->cap)
    {
      size_t cap = l->cap ? l->cap * 2 : 1024;
      EaselChange *p;

      if (cap > SIZE_MAX / sizeof *l->log || cap * sizeof *l->log > l->max_bytes)
        return false;
      p = realloc (l->log, cap * sizeof *l->log);
      if (p == NULL)
        return false;
      l->log = p;
      l->cap = cap;
    }
  l->log[l->n++] = (EaselChange) { (uint32_t) index, old, value };
  easel_image_set (img, x, y, value);
  return true;
}

void
easel_change_log_restore (const EaselChangeLog *l, EaselImage *img)
{
  for (size_t i = l->n; i > 0; i--)
    easel_image_set (img, (int) (l->log[i - 1].index % (uint32_t) img->width),
                     (int) (l->log[i - 1].index / (uint32_t) img->width), l->log[i - 1].old_value);
}

bool
easel_change_log_push (EaselChangeLog *l, EaselHistory *h)
{
  /* A shrink that fails keeps the bigger block, which is still valid. */
  if (l->cap > l->n && l->n > 0)
    {
      EaselChange *p = realloc (l->log, l->n * sizeof *l->log);

      if (p != NULL)
        {
          l->log = p;
          l->cap = l->n;
        }
    }
  if (!easel_history_push (h, l->log, l->n))
    return false;
  *l = (EaselChangeLog) { NULL, 0, 0, l->max_bytes };
  return true;
}

void
easel_change_log_free (EaselChangeLog *l)
{
  free (l->log);
  *l = (EaselChangeLog) { NULL, 0, 0, l->max_bytes };
}
