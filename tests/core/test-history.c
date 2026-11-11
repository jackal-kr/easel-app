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

/* ---- 0041 T001: the crop step (REQ-0041-04) ---- */

/* The picture an old w x h one gives when cut to the rectangle (cx, cy, nw, nh), which may hang past its edges: a pixel inside the old
   picture is its own, one outside is the fill (P7.7: a fully transparent one as 0). Made pixel by pixel from the new place, not as the
   step makes it. */
static EaselImage *
oracle_cut (const EaselImage *old, int cx, int cy, int nw, int nh, uint32_t fill)
{
  EaselImage *cut = easel_image_new (nw, nh, 0);

  CHECK (cut != NULL);
  for (int ny = 0; ny < nh; ny++)
    for (int nx = 0; nx < nw; nx++)
      {
        int ox = cx + nx, oy = cy + ny;
        uint32_t v = ox >= 0 && oy >= 0 && ox < old->width && oy < old->height ? old->px[(size_t) oy * old->width + ox]
                                                                                 : ((fill >> 24) != 0 ? fill : 0);

        cut->px[(size_t) ny * nw + nx] = v;
      }
  return cut;
}

static EaselImage *
copy_of (const EaselImage *img)
{
  EaselImage *c = easel_image_new (img->width, img->height, 0);

  CHECK (c != NULL);
  for (size_t i = 0; i < (size_t) img->width * img->height; i++)
    c->px[i] = img->px[i];
  return c;
}

static void
same_picture (const EaselImage *a, const EaselImage *b)
{
  CHECK (a->width == b->width && a->height == b->height);
  for (size_t i = 0; i < (size_t) a->width * a->height; i++)
    CHECK (a->px[i] == b->px[i]);
}

/* Makes the crop the way a caller does (REQ-0041-03, -04): the picture becomes the cut with `n_float` of its pixels (every `every`-th, from
   the first) replaced by a float's, the old values being the cut's; the removed pixels are the old ones outside the rectangle, in the
   row-major order of the old picture; both malloc'd at their length and handed to the history. Returns the picture after the crop. */
static EaselImage *
make_crop (EaselImage *img, EaselHistory *h, int cx, int cy, int nw, int nh, uint32_t fill, int every, size_t *n_removed_out,
           size_t *n_changes_out)
{
  EaselImage *old = copy_of (img), *cut = oracle_cut (old, cx, cy, nw, nh, fill), *after;
  size_t n_removed = 0, n = 0, i = 0;
  uint32_t *removed;
  EaselChange *changes;

  for (int y = 0; y < old->height; y++)
    for (int x = 0; x < old->width; x++)
      n_removed += !(x >= cx && x < cx + nw && y >= cy && y < cy + nh);
  for (int k = 0; every > 0 && k < nw * nh; k += every)
    n++;
  removed = malloc ((n_removed > 0 ? n_removed : 1) * sizeof *removed);
  changes = malloc ((n > 0 ? n : 1) * sizeof *changes);
  CHECK (removed != NULL && changes != NULL);
  for (int y = 0; y < old->height; y++)
    for (int x = 0; x < old->width; x++)
      if (!(x >= cx && x < cx + nw && y >= cy && y < cy + nh))
        removed[i++] = old->px[(size_t) y * old->width + x];
  after = copy_of (cut);
  n = 0;
  for (int k = 0; every > 0 && k < nw * nh; k += every)
    {
      uint32_t nv = 0xFF000000u | (uint32_t) (k * 2654435761u >> 8);

      changes[n++] = (EaselChange) { (uint32_t) k, cut->px[k], nv };
      after->px[k] = nv;
    }
  /* the picture itself becomes the crop, as the caller's own step does before it records */
  free (img->px);
  img->px = after->px;
  img->width = nw;
  img->height = nh;
  after->px = NULL;
  easel_image_free (after);
  CHECK (easel_history_push_crop (h, old->width, old->height, cx, cy, nw, nh, fill, removed, n_removed, n > 0 ? changes : (free (changes), NULL), n));
  if (n_removed_out != NULL)
    *n_removed_out = n_removed;
  if (n_changes_out != NULL)
    *n_changes_out = n;
  easel_image_free (old);
  after = copy_of (img);
  easel_image_free (cut);
  return after;
}

/* One crop through the step: Undo gives the old size and every pixel back (the removed ones and the float's), Redo the cut with the
   step's own fill, whatever was done since; the bytes counted are 4 a removed pixel plus the charge of the change log. */
static void
crop_case (int ow, int oh, int cx, int cy, int nw, int nh, uint32_t fill, int every)
{
  EaselImage *img = easel_image_new (ow, oh, WHITE), *old, *after;
  EaselHistory *h = easel_history_new (512u * 1024 * 1024);
  size_t before, n_removed, n_changes;
  uint32_t first;

  CHECK (img != NULL && h != NULL);
  for (int i = 0; i < ow * oh; i++)                           /* distinct, asymmetric pixels */
    img->px[i] = 0xFF000000u | (uint32_t) ((i + 1) * 40503u);
  first = img->px[0];
  draw_row (img, h, 0, 0, 0, BLACK);                           /* an earlier step stays undoable through the crop */
  old = copy_of (img);
  before = easel_history_bytes (h);
  after = make_crop (img, h, cx, cy, nw, nh, fill, every, &n_removed, &n_changes);
  CHECK (easel_history_bytes (h) == before + n_removed * 4 + easel_history_change_log_bytes (n_changes));
  CHECK (easel_history_can_undo (h) && !easel_history_can_redo (h));
  CHECK (easel_history_undo (h, img));
  same_picture (img, old);
  CHECK (easel_history_can_redo (h));
  CHECK (easel_history_redo (h, img));
  same_picture (img, after);
  CHECK (easel_history_undo (h, img));
  same_picture (img, old);
  /* the step before it stays undoable through it */
  CHECK (easel_history_undo (h, img));
  CHECK (img->width == ow && img->px[0] == first);
  CHECK (easel_history_redo (h, img) && easel_history_redo (h, img));
  same_picture (img, after);
  easel_history_free (h);
  easel_image_free (img);
  easel_image_free (old);
  easel_image_free (after);
}

static void
history_crop_step (void)
{
  crop_case (17, 11, 3, 2, 6, 5, 0xFF00FF00u, 0);              /* inside, a marked crop: no change log */
  crop_case (17, 11, 10, 5, 12, 9, 0xFFFF0000u, 3);            /* hangs past the right and bottom edges */
  crop_case (17, 11, -4, -2, 9, 7, 0xFF0000FFu, 2);            /* hangs past the left and top edges */
  crop_case (17, 11, -6, -3, 30, 20, 0xFF00FFFFu, 5);          /* larger than the old picture, all around it */
  crop_case (17, 11, 40, 30, 4, 4, 0xFFFF00FFu, 1);            /* wholly outside: no old pixel kept */
  crop_case (17, 11, -3, 4, 8, 3, 0x00123456u, 4);             /* a fully transparent fill is #00000000 (P7.7) */
  crop_case (1, 1, 0, 0, 1, 1, WHITE, 1);                      /* the smallest */
}

/* REQ-0041-04: the change log is charged by the one function every change log is charged by, and a log's step by what it says. */
static void
history_crop_charge (void)
{
  EaselImage *img = easel_image_new (6, 5, WHITE);
  EaselHistory *h = easel_history_new (512u * 1024 * 1024);
  size_t b0, b1, b2, b3;
  EaselChange *a, *c;
  uint32_t *removed;

  CHECK (img != NULL && h != NULL);
  CHECK (easel_history_change_log_bytes (0) == 64);
  CHECK (easel_history_change_log_bytes (1) == 64 + sizeof (EaselChange));
  CHECK (easel_history_change_log_bytes (1000) == 64 + 1000 * sizeof (EaselChange));
  /* an ordinary step of 3 changes is charged by it */
  b0 = easel_history_bytes (h);
  draw_run (img, h, 0, 3, BLACK);
  b1 = easel_history_bytes (h);
  CHECK (b1 - b0 == easel_history_change_log_bytes (3));
  /* a growth with 2 changes too */
  CHECK (easel_image_resize (img, 8, 5, WHITE) == EASEL_RESIZE_OK);
  a = malloc (2 * sizeof *a);
  CHECK (a != NULL);
  a[0] = (EaselChange) { 0, BLACK, WHITE };
  a[1] = (EaselChange) { 7, WHITE, BLACK };
  CHECK (easel_history_push_grow (h, 6, 5, 8, 5, WHITE, a, 2));
  b2 = easel_history_bytes (h);
  CHECK (b2 - b1 == easel_history_change_log_bytes (2));
  /* a crop to 2 x 2 of the 8 x 5 picture: 36 removed pixels and 1 change */
  removed = malloc (36 * sizeof *removed);
  c = malloc (sizeof *c);
  CHECK (removed != NULL && c != NULL);
  for (size_t i = 0; i < 36; i++)
    removed[i] = WHITE;
  c[0] = (EaselChange) { 0, WHITE, BLACK };
  CHECK (easel_image_resize (img, 2, 2, WHITE) == EASEL_RESIZE_OK);
  CHECK (easel_history_push_crop (h, 8, 5, 0, 0, 2, 2, WHITE, removed, 36, c, 1));
  b3 = easel_history_bytes (h);
  CHECK (b3 - b2 == 36 * 4 + easel_history_change_log_bytes (1));
  /* the step above the budget stays alone (REQ-0002-17), and a smaller budget drops older steps first */
  easel_history_free (h);
  h = easel_history_new (100);
  CHECK (h != NULL);
  removed = malloc (36 * sizeof *removed);
  CHECK (removed != NULL);
  for (size_t i = 0; i < 36; i++)
    removed[i] = WHITE;
  draw_run (img, h, 0, 1, BLACK);
  CHECK (easel_image_resize (img, 8, 5, WHITE) == EASEL_RESIZE_OK);
  CHECK (easel_image_resize (img, 2, 2, WHITE) == EASEL_RESIZE_OK);
  CHECK (easel_history_push_crop (h, 8, 5, 0, 0, 2, 2, WHITE, removed, 36, NULL, 0));
  CHECK (easel_history_bytes (h) == 36 * 4 + easel_history_change_log_bytes (0));
  CHECK (easel_history_can_undo (h));
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
  RUN (history_crop_step);
  RUN (history_crop_charge);
  return 0;
}
