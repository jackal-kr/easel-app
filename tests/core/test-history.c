/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Undo history scenarios of feature 0002. */

#include <stdlib.h>

#include "check.h"
#include "history.h"

#define WHITE 0xFFFFFFFFu
#define BLACK 0xFF000000u
#define W 10

/* An edit: row y, x0..x1 set to color, recorded as one step. */
static void
draw_row (EaselImage *img, EaselHistory *h, int y, int x0, int x1, uint32_t color)
{
  EaselChange *changes = malloc ((size_t) (x1 - x0 + 1) * sizeof *changes);
  size_t n = 0;

  CHECK (changes != NULL);
  for (int x = x0; x <= x1; x++)
    {
      uint32_t old = easel_image_get (img, x, y);

      if (old == color)
        continue;
      changes[n++] = (EaselChange) { (uint32_t) (y * img->width + x), old, color };
      easel_image_set (img, x, y, color);
    }
  CHECK (easel_history_push (h, changes, n));
}

/* An edit: pixels start..start+count-1 (linear index) set to color, recorded as one step. */
static void
draw_run (EaselImage *img, EaselHistory *h, int start, int count, uint32_t color)
{
  EaselChange *changes = malloc ((size_t) count * sizeof *changes);
  size_t n = 0;

  CHECK (changes != NULL);
  for (int i = start; i < start + count; i++)
    {
      uint32_t old = easel_image_get (img, i % img->width, i / img->width);

      if (old == color)
        continue;
      changes[n++] = (EaselChange) { (uint32_t) i, old, color };
      easel_image_set (img, i % img->width, i / img->width, color);
    }
  CHECK (easel_history_push (h, changes, n));
}

static int
row_count (const EaselImage *img, int y, uint32_t color)
{
  int n = 0;

  for (int x = 0; x < img->width; x++)
    n += easel_image_get (img, x, y) == color;
  return n;
}

static void
as1_undo_restores (void)
{
  EaselImage *img = easel_image_new (W, W, WHITE);
  EaselHistory *h = easel_history_new (512u << 20);

  CHECK (!easel_history_can_undo (h));
  CHECK (!easel_history_undo (h, img));
  draw_row (img, h, 0, 0, 3, BLACK);
  draw_row (img, h, 1, 0, 3, BLACK);

  CHECK (easel_history_undo (h, img));
  CHECK (row_count (img, 0, BLACK) == 4 && row_count (img, 1, BLACK) == 0);
  CHECK (easel_history_undo (h, img));
  CHECK (row_count (img, 0, BLACK) == 0 && row_count (img, 1, BLACK) == 0);
  CHECK (!easel_history_can_undo (h));
  CHECK (!easel_history_undo (h, img));

  easel_history_free (h);
  easel_image_free (img);
}

static void
as2_redo_reapplies (void)
{
  EaselImage *img = easel_image_new (W, W, WHITE);
  EaselHistory *h = easel_history_new (512u << 20);

  draw_row (img, h, 0, 0, 3, BLACK);
  draw_row (img, h, 1, 0, 3, BLACK);
  CHECK (!easel_history_can_redo (h));
  CHECK (easel_history_undo (h, img) && easel_history_undo (h, img));
  CHECK (easel_history_can_redo (h));

  CHECK (easel_history_redo (h, img));
  CHECK (row_count (img, 0, BLACK) == 4 && row_count (img, 1, BLACK) == 0);
  CHECK (easel_history_redo (h, img));
  CHECK (row_count (img, 1, BLACK) == 4);
  CHECK (!easel_history_can_redo (h));
  CHECK (!easel_history_redo (h, img));

  easel_history_free (h);
  easel_image_free (img);
}

static void
as4_new_edit_discards_redo (void)
{
  EaselImage *img = easel_image_new (W, W, WHITE);
  EaselHistory *h = easel_history_new (512u << 20);

  draw_row (img, h, 0, 0, 3, BLACK);
  draw_row (img, h, 1, 0, 3, BLACK);
  CHECK (easel_history_undo (h, img));
  CHECK (easel_history_can_redo (h));

  draw_row (img, h, 2, 0, 3, BLACK);
  CHECK (!easel_history_can_redo (h));
  CHECK (row_count (img, 0, BLACK) == 4 && row_count (img, 1, BLACK) == 0 && row_count (img, 2, BLACK) == 4);

  /* No pixel changed: no step. */
  draw_row (img, h, 2, 0, 3, BLACK);
  CHECK (easel_history_undo (h, img));
  CHECK (row_count (img, 2, BLACK) == 0 && row_count (img, 0, BLACK) == 4);

  easel_history_free (h);
  easel_image_free (img);
}

static void
modified_follows_the_saved_state (void)
{
  EaselImage *img = easel_image_new (W, W, WHITE);
  EaselHistory *h = easel_history_new (512u << 20);

  CHECK (!easel_history_is_modified (h));
  draw_row (img, h, 0, 0, 3, BLACK);
  CHECK (easel_history_is_modified (h));
  CHECK (easel_history_undo (h, img));
  CHECK (!easel_history_is_modified (h));
  CHECK (easel_history_redo (h, img));
  CHECK (easel_history_is_modified (h));

  easel_history_mark_saved (h);
  CHECK (!easel_history_is_modified (h));
  CHECK (easel_history_undo (h, img));
  CHECK (easel_history_is_modified (h));
  CHECK (easel_history_redo (h, img));
  CHECK (!easel_history_is_modified (h));

  easel_history_free (h);
  easel_image_free (img);
}

static void
as12_rediscarded_saved_state (void)
{
  EaselImage *img = easel_image_new (W, W, WHITE);
  EaselHistory *h = easel_history_new (512u << 20);

  draw_row (img, h, 0, 0, 3, BLACK);
  easel_history_mark_saved (h);
  CHECK (easel_history_undo (h, img));

  /* The identical stroke again is a new step: the saved state is gone, so the picture stays modified. */
  draw_row (img, h, 0, 0, 3, BLACK);
  CHECK (easel_history_is_modified (h));
  CHECK (easel_history_undo (h, img));
  CHECK (easel_history_is_modified (h));
  CHECK (easel_history_redo (h, img));
  CHECK (easel_history_is_modified (h));
  easel_history_mark_saved (h);
  CHECK (!easel_history_is_modified (h));

  /* The start state is never discarded by a new edit: undoing back to it is still the saved state. */
  easel_history_free (h);
  h = easel_history_new (512u << 20);
  draw_row (img, h, 5, 0, 3, BLACK);
  CHECK (easel_history_undo (h, img));
  CHECK (!easel_history_is_modified (h));
  draw_row (img, h, 5, 0, 3, BLACK);
  CHECK (easel_history_is_modified (h));
  CHECK (easel_history_undo (h, img));
  CHECK (!easel_history_is_modified (h));

  easel_history_free (h);
  easel_image_free (img);
}

/* One step costs 12 bytes per changed pixel plus 64 (REQ-0002-16). */
static size_t
step_bytes (size_t pixels)
{
  return pixels * 12 + 64;
}

static void
as16_budget_drops_oldest (void)
{
  enum { BUDGET = 16 << 20, PIXELS = 50000, STROKES = 200 };
  EaselImage *img = easel_image_new (1024, 768, WHITE);
  EaselHistory *h = easel_history_new (BUDGET);
  int undone = 0;

  draw_run (img, h, 0, PIXELS, 0xFF000001u);
  CHECK (easel_history_bytes (h) == step_bytes (PIXELS));
  /* The bound of REQ-0002-16: at most 16 bytes per changed pixel plus 64 per step, even for one pixel. */
  CHECK (easel_history_bytes (h) <= (size_t) PIXELS * 16 + 64);
  CHECK (step_bytes (1) <= 16 + 64);

  for (int k = 1; k < STROKES; k++)
    {
      draw_run (img, h, (k * PIXELS) % (1024 * 768 - PIXELS), PIXELS, 0xFF000001u + (uint32_t) k);
      CHECK (easel_history_bytes (h) <= BUDGET);
    }

  while (easel_history_undo (h, img))
    undone++;
  /* The 20 most recent strokes at least (each needs under 0.8 MB), but not all 200. */
  CHECK (undone >= 20);
  CHECK (undone == BUDGET / (int) step_bytes (PIXELS));
  CHECK (!easel_history_can_undo (h));
  /* The oldest stroke could not be undone: its first pixel still has its color. */
  CHECK (easel_image_get (img, 0, 0) == 0xFF000001u);

  easel_history_free (h);
  easel_image_free (img);
}

static void
as17_huge_step_kept_alone (void)
{
  enum { BUDGET = 16 << 20, PIXELS = 1500000 };
  EaselImage *img = easel_image_new (2000, 1000, WHITE);
  EaselHistory *h = easel_history_new (BUDGET);

  draw_run (img, h, 0, 100, BLACK);
  draw_run (img, h, 5000, 100, BLACK);
  draw_run (img, h, 0, PIXELS, 0xFF123456u);
  CHECK (step_bytes (PIXELS) > BUDGET);
  CHECK (easel_history_bytes (h) == step_bytes (PIXELS));

  CHECK (easel_history_undo (h, img));
  CHECK (easel_image_get (img, 7, 0) == BLACK);
  CHECK (!easel_history_can_undo (h));
  CHECK (easel_history_redo (h, img));
  CHECK (easel_image_get (img, 7, 0) == 0xFF123456u);

  /* The next small edit drops the huge one again: the budget is exceeded only while it is alone. */
  draw_run (img, h, 1999 * 1000, 10, BLACK);
  CHECK (easel_history_bytes (h) == step_bytes (10));
  CHECK (easel_history_undo (h, img));
  CHECK (!easel_history_can_undo (h));

  easel_history_free (h);
  easel_image_free (img);
}

static void
as13_dropped_saved_state (void)
{
  EaselImage *img = easel_image_new (W, W, WHITE);
  EaselHistory *h = easel_history_new (3 * step_bytes (W));

  for (int y = 0; y < 5; y++)
    draw_row (img, h, y, 0, W - 1, BLACK);
  easel_history_mark_saved (h);
  CHECK (!easel_history_is_modified (h));

  /* Three more strokes: steps 1 to 5 are dropped, the saved state with them. */
  for (int y = 5; y < 8; y++)
    draw_row (img, h, y, 0, W - 1, BLACK);
  CHECK (easel_history_bytes (h) == 3 * step_bytes (W));
  while (easel_history_undo (h, img))
    ;
  CHECK (row_count (img, 0, BLACK) == W && row_count (img, 4, BLACK) == W);
  CHECK (row_count (img, 5, BLACK) == 0 && row_count (img, 7, BLACK) == 0);
  CHECK (easel_history_is_modified (h));
  easel_history_mark_saved (h);
  CHECK (!easel_history_is_modified (h));

  easel_history_free (h);
  easel_image_free (img);
}

/* A never-saved picture: its saved state is the start. Once old steps are dropped the start cannot be reached, so
   undoing as far as possible still leaves the picture modified (REQ-0002-14). */
static void
as13_dropped_start_state (void)
{
  EaselImage *img = easel_image_new (W, W, WHITE);
  EaselHistory *h = easel_history_new (3 * step_bytes (W));

  for (int y = 0; y < 8; y++)
    draw_row (img, h, y, 0, W - 1, BLACK);
  CHECK (easel_history_bytes (h) == 3 * step_bytes (W));
  while (easel_history_undo (h, img))
    ;
  CHECK (row_count (img, 4, BLACK) == W && row_count (img, 5, BLACK) == 0);
  CHECK (easel_history_is_modified (h));

  /* Within the budget nothing is dropped, the start stays reachable, and undoing back to it is unmodified. */
  easel_history_free (h);
  easel_image_free (img);
  img = easel_image_new (W, W, WHITE);
  h = easel_history_new (8 * step_bytes (W));
  for (int y = 0; y < 6; y++)
    draw_row (img, h, y, 0, W - 1, BLACK);
  CHECK (easel_history_bytes (h) == 6 * step_bytes (W));
  while (easel_history_undo (h, img))
    ;
  CHECK (row_count (img, 0, BLACK) == 0);
  CHECK (!easel_history_is_modified (h));

  easel_history_free (h);
  easel_image_free (img);
}

/* A fill as one step of spans (REQ-0007-05, REQ-0007-07): the pixels are written by the caller, the step stores the two
   colors and the spans. */
static EaselSpan *
make_spans (const EaselSpan *list, size_t n)
{
  EaselSpan *spans = malloc (n * sizeof *spans);

  CHECK (spans != NULL);
  for (size_t i = 0; i < n; i++)
    spans[i] = list[i];
  return spans;
}

static void
fill_spans (EaselImage *img, const EaselSpan *spans, size_t n, uint32_t color)
{
  for (size_t i = 0; i < n; i++)
    for (uint32_t x = spans[i].x0; x <= spans[i].x1; x++)
      easel_image_set (img, (int) x, (int) spans[i].y, color);
}

static void
as6_fill_step_undo_redo (void)
{
  EaselImage *img = easel_image_new (W, 6, WHITE);
  EaselHistory *h = easel_history_new (1024 * 1024);
  const EaselSpan list[] = { { 2, 1, 4 }, { 3, 1, 1 }, { 3, 6, 9 } };
  EaselSpan *spans = make_spans (list, 3);
  EaselSpan *none = make_spans (list, 1);

  CHECK (img != NULL && h != NULL);
  fill_spans (img, list, 3, BLACK);
  CHECK (easel_history_push_fill (h, WHITE, BLACK, spans, 3));
  CHECK (easel_history_can_undo (h));
  CHECK (easel_history_is_modified (h));
  CHECK (easel_history_bytes (h) == 3 * 12 + 64);                       /* 12 bytes a span, not a pixel */

  CHECK (easel_history_undo (h, img));
  for (int y = 0; y < 6; y++)
    CHECK (row_count (img, y, WHITE) == W);                              /* exactly as before the fill */
  CHECK (!easel_history_is_modified (h));
  CHECK (easel_history_redo (h, img));
  CHECK (row_count (img, 2, BLACK) == 4 && row_count (img, 3, BLACK) == 5);
  CHECK (easel_image_get (img, 0, 2) == WHITE && easel_image_get (img, 5, 3) == WHITE);   /* neighbors untouched */
  CHECK (easel_history_bytes (h) == 3 * 12 + 64);

  /* No span: no step, and the spans are freed. */
  CHECK (easel_history_push_fill (h, WHITE, BLACK, none, 0));
  CHECK (easel_history_bytes (h) == 3 * 12 + 64);
  /* A count that cannot be stored is refused and the caller keeps the spans. */
  CHECK (!easel_history_push_fill (h, WHITE, BLACK, spans, SIZE_MAX / 8));
  CHECK (easel_history_bytes (h) == 3 * 12 + 64);

  /* A new edit after an undo discards the redo, as any step (REQ-0002-05). */
  CHECK (easel_history_undo (h, img));
  CHECK (easel_history_can_redo (h));
  draw_row (img, h, 5, 0, 2, BLACK);
  CHECK (!easel_history_can_redo (h));
  easel_history_free (h);
  easel_image_free (img);
}

/* The budget counts a fill by its spans, and a lone step over it stays (REQ-0002-16, REQ-0002-17). */
static void
as6_fill_step_budget (void)
{
  EaselImage *img = easel_image_new (W, 120, WHITE);
  EaselHistory *h = easel_history_new (1000);
  EaselSpan small[10], big[100];

  CHECK (img != NULL && h != NULL);
  for (uint32_t i = 0; i < 10; i++)
    small[i] = (EaselSpan) { i, 0, 0 };
  for (uint32_t i = 0; i < 100; i++)
    big[i] = (EaselSpan) { i, 2, 3 };
  fill_spans (img, small, 10, BLACK);
  CHECK (easel_history_push_fill (h, WHITE, BLACK, make_spans (small, 10), 10));
  CHECK (easel_history_bytes (h) == 10 * 12 + 64);
  fill_spans (img, big, 100, 0xFFFF0000u);
  CHECK (easel_history_push_fill (h, WHITE, 0xFFFF0000u, make_spans (big, 100), 100));    /* 1264 bytes > 1000 */
  CHECK (easel_history_bytes (h) == 100 * 12 + 64);                      /* the older step was dropped, this one stays */
  CHECK (easel_history_undo (h, img));
  CHECK (!easel_history_can_undo (h));
  CHECK (easel_image_get (img, 2, 50) == WHITE && easel_image_get (img, 0, 3) == BLACK);
  easel_history_free (h);
  easel_image_free (img);
}

/* 0016 T002 (REQ-0016-05, -07), AS-7 and AS-15: a growth of the picture joined to the pixels a paste put down, one step: undo writes the
   old values and crops back to the old size, every pixel as before; redo grows with the same fill and writes the new values; it counts
   64 bytes plus 12 per change (a growth removes no pixel); a step of the growth alone (no pixel put down, after Clear) is still a step;
   it discards the redo steps, and the steps before it stay undoable through it. */
static void
as7_grow_step (void)
{
  EaselImage *img = easel_image_new (21, 11, WHITE);
  EaselHistory *h = easel_history_new (1 << 20);
  EaselChange *changes;
  size_t n = 0, before;

  CHECK (img != NULL && h != NULL);
  draw_row (img, h, 2, 0, 4, BLACK);                           /* an earlier step */
  CHECK (easel_history_undo (h, img));                         /* now redoable: the grow step discards it */
  draw_row (img, h, 3, 0, 1, BLACK);
  before = easel_history_bytes (h);
  /* The paste grew the picture to 30 x 20 (yellow) and is put down: 3 pixels changed. */
  CHECK (easel_image_resize (img, 30, 20, 0xFFFFFF00u) == EASEL_RESIZE_OK);
  changes = malloc (3 * sizeof *changes);
  CHECK (changes != NULL);
  changes[n++] = (EaselChange) { 0 * 30 + 0, WHITE, BLACK };
  changes[n++] = (EaselChange) { 15 * 30 + 25, 0xFFFFFF00u, BLACK };          /* in the grown part */
  changes[n++] = (EaselChange) { 10 * 30 + 20, WHITE, BLACK };                /* the old corner */
  for (size_t i = 0; i < n; i++)
    easel_image_set (img, (int) (changes[i].index % 30), (int) (changes[i].index / 30), changes[i].new_value);
  CHECK (easel_history_push_grow (h, 21, 11, 30, 20, 0xFFFFFF00u, changes, n));
  CHECK (easel_history_bytes (h) == before + 64 + 3 * 12);
  CHECK (!easel_history_can_redo (h));
  /* Undo: 21 x 11, every pixel as before (white, with row 3's two black pixels). */
  CHECK (easel_history_undo (h, img));
  CHECK (img->width == 21 && img->height == 11);
  for (int y = 0; y < 11; y++)
    for (int x = 0; x < 21; x++)
      CHECK (easel_image_get (img, x, y) == (y == 3 && x <= 1 ? BLACK : WHITE));
  /* Redo: 30 x 20, the grown part yellow, the three pixels black. */
  CHECK (easel_history_redo (h, img));
  CHECK (img->width == 30 && img->height == 20);
  CHECK (easel_image_get (img, 0, 0) == BLACK && easel_image_get (img, 25, 15) == BLACK && easel_image_get (img, 20, 10) == BLACK);
  CHECK (easel_image_get (img, 29, 19) == 0xFFFFFF00u && easel_image_get (img, 21, 0) == 0xFFFFFF00u && easel_image_get (img, 5, 5) == WHITE);
  /* The step before stays undoable through it. */
  CHECK (easel_history_undo (h, img) && easel_history_undo (h, img));
  CHECK (img->width == 21 && easel_image_get (img, 0, 3) == WHITE);
  CHECK (easel_history_redo (h, img) && easel_history_redo (h, img));
  CHECK (img->width == 30 && easel_image_get (img, 0, 3) == BLACK);
  /* The growth alone (a Clear of a grown paste): no change, still one step, 64 bytes. */
  before = easel_history_bytes (h);
  CHECK (easel_image_resize (img, 35, 20, 0xFF00FF00u) == EASEL_RESIZE_OK);
  CHECK (easel_history_push_grow (h, 30, 20, 35, 20, 0xFF00FF00u, NULL, 0));
  CHECK (easel_history_bytes (h) == before + 64);
  CHECK (easel_history_undo (h, img) && img->width == 30);
  CHECK (easel_history_redo (h, img) && img->width == 35 && easel_image_get (img, 34, 0) == 0xFF00FF00u);
  easel_history_free (h);
  easel_image_free (img);
}

int
main (void)
{
  RUN (as1_undo_restores);
  RUN (as2_redo_reapplies);
  RUN (as4_new_edit_discards_redo);
  RUN (modified_follows_the_saved_state);
  RUN (as12_rediscarded_saved_state);
  RUN (as13_dropped_saved_state);
  RUN (as13_dropped_start_state);
  RUN (as16_budget_drops_oldest);
  RUN (as17_huge_step_kept_alone);
  RUN (as6_fill_step_undo_redo);
  RUN (as6_fill_step_budget);
  RUN (as7_grow_step);
  return 0;
}
