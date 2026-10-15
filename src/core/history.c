/* SPDX-License-Identifier: GPL-3.0-or-later */

#include <stdlib.h>
#include <string.h>

#include "history.h"

typedef struct
{
  unsigned long id;       /* unique and increasing; 0 is the start state, which has no step */
  EaselChange *changes;
  size_t n;
} Step;

struct _EaselHistory
{
  Step *steps;
  size_t first, len, cap; /* the kept steps are steps[first..len); older ones were dropped */
  size_t applied;         /* steps[first..applied) are applied; the rest can be redone */
  unsigned long next_id;
  unsigned long saved_id;
  bool saved_reachable;   /* false once the saved state's step is gone */
  size_t budget;
  size_t bytes;           /* what the kept steps need, see easel_history_bytes () */
};

/* Per step: the pixels it changed and the step itself (REQ-0002-16). */
#define BYTES_PER_CHANGE sizeof (EaselChange)
#define BYTES_PER_STEP 64

EaselHistory *
easel_history_new (size_t budget_bytes)
{
  EaselHistory *h = calloc (1, sizeof *h);

  if (h == NULL)
    return NULL;
  h->next_id = 1;
  h->saved_reachable = true;
  h->budget = budget_bytes;
  return h;
}

static void
free_step (Step *s)
{
  free (s->changes);
  s->changes = NULL;
  s->n = 0;
}

void
easel_history_free (EaselHistory *h)
{
  if (h == NULL)
    return;
  for (size_t i = h->first; i < h->len; i++)
    free_step (&h->steps[i]);
  free (h->steps);
  free (h);
}

static unsigned long
current_id (const EaselHistory *h)
{
  return h->applied == h->first ? 0 : h->steps[h->applied - 1].id;
}

static size_t
step_bytes (const Step *s)
{
  return s->n * BYTES_PER_CHANGE + BYTES_PER_STEP;
}

/* Drops the oldest kept step. The saved state goes with it: the start state (id 0) can no longer be reached once any
   step is gone, and the state after a dropped step becomes the oldest one, which the spec counts as lost
   (REQ-0002-14). */
static void
drop_oldest (EaselHistory *h)
{
  Step *s = &h->steps[h->first];

  if (s->id == h->saved_id || h->saved_id == 0)
    h->saved_reachable = false;
  h->bytes -= step_bytes (s);
  free_step (s);
  h->first++;
}

/* Makes room at the end of the array: first by moving the kept steps to its start, then by growing it. */
static bool
make_room (EaselHistory *h)
{
  size_t kept = h->len - h->first;

  if (h->len < h->cap)
    return true;
  if (h->first > 0 && h->first >= h->cap / 2)
    {
      memmove (h->steps, h->steps + h->first, kept * sizeof *h->steps);
      h->applied -= h->first;
      h->len = kept;
      h->first = 0;
      return true;
    }
  {
    size_t cap = h->cap ? h->cap * 2 : 16;
    Step *steps = cap > SIZE_MAX / sizeof *steps ? NULL : realloc (h->steps, cap * sizeof *steps);

    if (steps == NULL)
      return false;
    h->steps = steps;
    h->cap = cap;
  }
  return true;
}

bool
easel_history_push (EaselHistory *h, EaselChange *changes, size_t n)
{
  if (n == 0)
    {
      free (changes);
      return true;
    }
  if (n > (SIZE_MAX - BYTES_PER_STEP) / BYTES_PER_CHANGE || !make_room (h))
    return false;

  /* A new edit discards the redoable steps; the saved state goes with its step (REQ-0002-05, REQ-0002-14). */
  for (size_t i = h->applied; i < h->len; i++)
    {
      if (h->steps[i].id == h->saved_id)
        h->saved_reachable = false;
      h->bytes -= step_bytes (&h->steps[i]);
      free_step (&h->steps[i]);
    }
  h->len = h->applied;

  h->steps[h->len++] = (Step) { h->next_id++, changes, n };
  h->applied = h->len;
  h->bytes += step_bytes (&h->steps[h->len - 1]);

  /* Over the budget: the oldest steps go first, but the newest step always stays, even if it alone is over
     (REQ-0002-16, REQ-0002-17). */
  while (h->bytes > h->budget && h->len - h->first > 1)
    drop_oldest (h);
  return true;
}

static void
write_step (const Step *s, EaselImage *img, bool undo)
{
  for (size_t i = 0; i < s->n; i++)
    {
      const EaselChange *c = &s->changes[i];

      easel_image_set (img, (int) (c->index % (uint32_t) img->width), (int) (c->index / (uint32_t) img->width),
                       undo ? c->old_value : c->new_value);
    }
}

bool
easel_history_undo (EaselHistory *h, EaselImage *img)
{
  if (h->applied == h->first)
    return false;
  write_step (&h->steps[--h->applied], img, true);
  return true;
}

bool
easel_history_redo (EaselHistory *h, EaselImage *img)
{
  if (h->applied == h->len)
    return false;
  write_step (&h->steps[h->applied++], img, false);
  return true;
}

bool
easel_history_can_undo (const EaselHistory *h)
{
  return h->applied > h->first;
}

bool
easel_history_can_redo (const EaselHistory *h)
{
  return h->applied < h->len;
}

void
easel_history_mark_saved (EaselHistory *h)
{
  h->saved_id = current_id (h);
  h->saved_reachable = true;
}

bool
easel_history_is_modified (const EaselHistory *h)
{
  return !h->saved_reachable || current_id (h) != h->saved_id;
}

size_t
easel_history_bytes (const EaselHistory *h)
{
  return h->bytes;
}
