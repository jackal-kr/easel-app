/* SPDX-License-Identifier: GPL-3.0-or-later */

#include <stdlib.h>
#include <string.h>

#include "history.h"

typedef enum
{
  STEP_CHANGES,           /* pixels that changed: changes, n */
  STEP_RESIZE,            /* a new size: the fields below */
  STEP_FILL,              /* a fill: old_color becomes new_color in every pixel of the spans */
  STEP_GROW,              /* a growth from old_w x old_h to new_w x new_h with fill, then the changes (0016: a paste's put-down) */
  STEP_BUFFER,            /* a whole picture buffer, swapped with the picture's by undo and redo (0018: a turn, a flip, an invert) */
} StepKind;

typedef struct
{
  unsigned long id;       /* unique and increasing; 0 is the start state, which has no step */
  StepKind kind;
  EaselChange *changes;
  size_t n;
  /* A resize: the sizes before and after, the color of the added area, and the removed pixels, 4 bytes each, in the
     row-major order of the old picture, skipping the part that stayed (REQ-0004-22, REQ-0004-23). */
  int old_w, old_h, new_w, new_h;
  uint32_t fill;
  uint32_t *removed;
  size_t n_removed;
  EaselSpan *spans;
  size_t n_spans;
  uint32_t old_color, new_color;
  /* A buffer step: the other state's pixels and size (the picture's own buffer is the one that is not here; undo and redo swap them,
     so the pixel count, and the bytes the step counts, never change). */
  uint32_t *buf;
  int buf_w, buf_h;
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
  bool refused;           /* the last undo or redo could not allocate; refused_w x refused_h was the size */
  int refused_w, refused_h;
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
  free (s->removed);
  free (s->spans);
  free (s->buf);
  s->buf = NULL;
  s->changes = NULL;
  s->removed = NULL;
  s->spans = NULL;
  s->n = 0;
  s->n_removed = 0;
  s->n_spans = 0;
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
  if (s->kind == STEP_RESIZE)
    return s->n_removed * sizeof (uint32_t) + BYTES_PER_STEP;
  if (s->kind == STEP_FILL)
    return s->n_spans * sizeof (EaselSpan) + BYTES_PER_STEP;
  if (s->kind == STEP_BUFFER)
    return (size_t) s->buf_w * (size_t) s->buf_h * sizeof (uint32_t) + BYTES_PER_STEP;
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

/* Adds a finished step: room in the array was made before, so this cannot fail. A new edit discards the redoable
   steps, the saved state going with its step (REQ-0002-05, REQ-0002-14); over the budget, the oldest steps go first,
   but the newest always stays, even if it alone is over (REQ-0002-16, REQ-0002-17). */
static void
commit_step (EaselHistory *h, Step step)
{
  h->refused = false;
  for (size_t i = h->applied; i < h->len; i++)
    {
      if (h->steps[i].id == h->saved_id)
        h->saved_reachable = false;
      h->bytes -= step_bytes (&h->steps[i]);
      free_step (&h->steps[i]);
    }
  h->len = h->applied;

  step.id = h->next_id++;
  h->steps[h->len++] = step;
  h->applied = h->len;
  h->bytes += step_bytes (&h->steps[h->len - 1]);

  while (h->bytes > h->budget && h->len - h->first > 1)
    drop_oldest (h);
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
  commit_step (h, (Step) { .kind = STEP_CHANGES, .changes = changes, .n = n });
  return true;
}

bool
easel_history_push_grow (EaselHistory *h, int old_w, int old_h, int new_w, int new_h, uint32_t fill, EaselChange *changes, size_t n)
{
  if (n > (SIZE_MAX - BYTES_PER_STEP) / BYTES_PER_CHANGE || !make_room (h))
    return false;
  commit_step (h, (Step) { .kind = STEP_GROW, .old_w = old_w, .old_h = old_h, .new_w = new_w, .new_h = new_h, .fill = fill,
                           .changes = changes, .n = n });
  return true;
}

bool
easel_history_push_fill (EaselHistory *h, uint32_t old_value, uint32_t new_value, EaselSpan *spans, size_t n)
{
  if (n == 0)
    {
      free (spans);
      return true;
    }
  if (n > (SIZE_MAX - BYTES_PER_STEP) / sizeof (EaselSpan) || !make_room (h))
    return false;
  commit_step (h, (Step) { .kind = STEP_FILL, .spans = spans, .n_spans = n, .old_color = old_value, .new_color = new_value });
  return true;
}

bool
easel_history_push_buffer (EaselHistory *h, int w, int hgt, uint32_t *px)
{
  if (w < 1 || hgt < 1 || (size_t) w * (size_t) hgt > (SIZE_MAX - BYTES_PER_STEP) / sizeof (uint32_t) || !make_room (h))
    return false;
  commit_step (h, (Step) { .kind = STEP_BUFFER, .buf = px, .buf_w = w, .buf_h = hgt });
  return true;
}

EaselResizeResult
easel_resize_edit (EaselImage *img, EaselHistory *h, int width, int height, uint32_t fill)
{
  int old_w = img->width, old_h = img->height;
  int kept_w, kept_h;
  size_t n_removed;
  uint32_t *removed = NULL;
  EaselResizeResult r;

  if (width < 1 || height < 1 || width > EASEL_IMAGE_MAX_SIZE || height > EASEL_IMAGE_MAX_SIZE)
    return EASEL_RESIZE_BAD_SIZE;
  if (width == old_w && height == old_h)
    return EASEL_RESIZE_UNCHANGED;

  /* Everything that can fail comes before anything changes: the removed pixels, room for the step, the new buffer. */
  kept_w = width < old_w ? width : old_w;
  kept_h = height < old_h ? height : old_h;
  n_removed = (size_t) old_w * (size_t) old_h - (size_t) kept_w * (size_t) kept_h;
  if (n_removed > 0)
    {
      size_t i = 0;

      removed = malloc (n_removed * sizeof (uint32_t));
      if (removed == NULL)
        return EASEL_RESIZE_NO_MEMORY;
      for (int y = 0; y < old_h; y++)
        for (int x = 0; x < old_w; x++)
          if (x >= kept_w || y >= kept_h)
            removed[i++] = img->px[(size_t) y * (size_t) old_w + (size_t) x];
    }
  if (!make_room (h))
    {
      free (removed);
      return EASEL_RESIZE_NO_MEMORY;
    }
  r = easel_image_resize (img, width, height, fill);
  if (r != EASEL_RESIZE_OK)
    {
      free (removed);
      return r;
    }
  commit_step (h, (Step) { .kind = STEP_RESIZE, .old_w = old_w, .old_h = old_h, .new_w = width, .new_h = height,
                           .fill = fill, .removed = removed, .n_removed = n_removed });
  return EASEL_RESIZE_OK;
}

/* Applies a step (redo) or reverts it (undo). A step is only ever applied or reverted while the picture has the size it
   had when the step was recorded, so the indexes of a change step are valid (Plan, "A size change and the undo
   history"). False only if a resize cannot allocate the new buffer; then nothing changed. */
static bool
apply_step (Step *s, EaselImage *img, bool undo)
{
  if (s->kind == STEP_BUFFER)
    {
      /* The picture's buffer and size and the step's change places: nothing is allocated, so this cannot fail; the whole picture shows
         again. */
      uint32_t *px = img->px;
      int w = img->width, hgt = img->height;

      img->px = s->buf;
      img->width = s->buf_w;
      img->height = s->buf_h;
      s->buf = px;
      s->buf_w = w;
      s->buf_h = hgt;
      img->dirty = (EaselRect) { 0, 0, img->width, img->height };
      return true;
    }

  if (s->kind == STEP_RESIZE)
    {
      if (!undo)
        return easel_image_resize (img, s->new_w, s->new_h, s->fill) == EASEL_RESIZE_OK;

      /* Back to the old size; the part that stayed is already in place, the removed pixels fill the rest. */
      {
        int kept_w = s->new_w < s->old_w ? s->new_w : s->old_w, kept_h = s->new_h < s->old_h ? s->new_h : s->old_h;
        size_t i = 0;

        if (easel_image_resize (img, s->old_w, s->old_h, 0) != EASEL_RESIZE_OK)
          return false;
        for (int y = 0; y < s->old_h; y++)
          for (int x = 0; x < s->old_w; x++)
            if (x >= kept_w || y >= kept_h)
              img->px[(size_t) y * (size_t) s->old_w + (size_t) x] = s->removed[i++];
      }
      return true;
    }

  if (s->kind == STEP_GROW)
    {
      /* The size first, so that a buffer that cannot be had changes nothing; the changes' indexes are of the grown size. Undo crops
         back (the cropped pixels were the fill's, or the paste's over it) and writes the old values of the changes that stay. */
      if (easel_image_resize (img, undo ? s->old_w : s->new_w, undo ? s->old_h : s->new_h, s->fill) != EASEL_RESIZE_OK)
        return false;
      for (size_t i = 0; i < s->n; i++)
        {
          int x = (int) (s->changes[i].index % (uint32_t) s->new_w), y = (int) (s->changes[i].index / (uint32_t) s->new_w);

          if (!undo)
            easel_image_set (img, x, y, s->changes[i].new_value);
          else if (x < s->old_w && y < s->old_h)
            easel_image_set (img, x, y, s->changes[i].old_value);
        }
      return true;
    }

  if (s->kind == STEP_FILL)
    {
      uint32_t color = undo ? s->old_color : s->new_color;

      for (size_t i = 0; i < s->n_spans; i++)
        for (uint64_t x = s->spans[i].x0; x <= s->spans[i].x1; x++)      /* 64 bits: x1 may be the largest 32-bit value */
          easel_image_set (img, (int) x, (int) s->spans[i].y, color);
      return true;
    }

  for (size_t i = 0; i < s->n; i++)
    {
      const EaselChange *c = &s->changes[i];

      easel_image_set (img, (int) (c->index % (uint32_t) img->width), (int) (c->index / (uint32_t) img->width),
                       undo ? c->old_value : c->new_value);
    }
  return true;
}

/* Applies steps[i] one way; on failure remembers the size that could not be had. */
static bool
move (EaselHistory *h, EaselImage *img, size_t i, bool undo)
{
  Step *s = &h->steps[i];

  h->refused = false;
  if (apply_step (s, img, undo))
    return true;
  h->refused = true;
  h->refused_w = undo ? s->old_w : s->new_w;
  h->refused_h = undo ? s->old_h : s->new_h;
  return false;
}

bool
easel_history_undo (EaselHistory *h, EaselImage *img)
{
  h->refused = false;
  if (h->applied == h->first || !move (h, img, h->applied - 1, true))
    return false;
  h->applied--;
  return true;
}

bool
easel_history_redo (EaselHistory *h, EaselImage *img)
{
  h->refused = false;
  if (h->applied == h->len || !move (h, img, h->applied, false))
    return false;
  h->applied++;
  return true;
}

bool
easel_history_refused (const EaselHistory *h, int *w, int *h_out)
{
  if (h->refused)
    {
      *w = h->refused_w;
      *h_out = h->refused_h;
    }
  return h->refused;
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
