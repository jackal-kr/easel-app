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
  return 0;
}
