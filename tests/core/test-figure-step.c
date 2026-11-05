/* SPDX-License-Identifier: GPL-3.0-or-later */
/* A curve or a polygon drawn into a picture, with its one undo step, feature 0011 (REQ-0011-14, REQ-0011-15). */

#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/resource.h>

#include "check.h"
#include "figure.h"
#include "history.h"
#include "image.h"

#define WHITE 0xFFFFFFFFu
#define BLACK 0xFF000000u
#define RED 0xFFFF0000u

static EaselFigure
with_points (EaselFigureKind kind, const int *pts, size_t n, bool closed, int width, EaselFigureStyle style)
{
  return (EaselFigure) { kind, 0, 0, 0, 0, width, style, NULL, pts, n, closed };
}

static int
count (const EaselImage *img, uint32_t color)
{
  int n = 0;

  for (int i = 0; i < img->width * img->height; i++)
    n += img->px[i] == color;
  return n;
}

static bool
same_pixels (const EaselImage *a, const EaselImage *b)
{
  return a->width == b->width && a->height == b->height
         && memcmp (a->px, b->px, (size_t) a->width * a->height * sizeof a->px[0]) == 0;
}

/* AS-12 (REQ-0011-14): one undo step, 12 bytes a changed pixel plus 64, undo and redo exact, no step when nothing changes,
   clipped, exactly the color. */
static void
as12_curve_polygon_step_and_nothing (void)
{
  static const int curve_pts[8] = { 10, 40, 30, 20, 50, 80, 50, 40 };
  static const int tri[6] = { 10, 10, 60, 10, 10, 60 };
  static const int half_out[6] = { -20, 50, 40, 20, 40, 80 };
  static const int far_out[6] = { -90, -90, -50, -90, -50, -50 };
  EaselFigure figures[2] = {
    with_points (EASEL_FIGURE_CURVE, curve_pts, 4, false, 3, EASEL_FIGURE_OUTLINE),
    with_points (EASEL_FIGURE_POLYGON, tri, 3, true, 1, EASEL_FIGURE_OUTLINE_AND_FILL),
  };

  for (int k = 0; k < 2; k++)
    {
      EaselImage *img = easel_image_new (300, 200, WHITE), *start = easel_image_new (300, 200, WHITE);
      EaselHistory *h = easel_history_new (1024 * 1024);
      size_t changed = 0, bytes;
      EaselImage *drawn;

      CHECK (img != NULL && start != NULL && h != NULL);
      CHECK (easel_figure_apply (img, h, &figures[k], 0xFF000080u, RED, &changed) == EASEL_FIGURE_DONE);
      CHECK (changed > 100);
      CHECK (easel_history_bytes (h) == changed * 12 + 64);
      CHECK (count (img, WHITE) == 300 * 200 - (int) changed);
      CHECK (count (img, 0xFF000080u) > 0);                       /* exactly the color, no blending */
      CHECK (easel_history_is_modified (h));
      drawn = easel_image_new (300, 200, WHITE);
      memcpy (drawn->px, img->px, (size_t) 300 * 200 * sizeof drawn->px[0]);
      CHECK (easel_history_undo (h, img) && same_pixels (img, start));
      CHECK (easel_history_redo (h, img) && same_pixels (img, drawn));
      /* The same again changes no pixel: no step. */
      bytes = easel_history_bytes (h);
      CHECK (easel_figure_apply (img, h, &figures[k], 0xFF000080u, RED, &changed) == EASEL_FIGURE_NOTHING);
      CHECK (changed == 0 && easel_history_bytes (h) == bytes);
      CHECK (easel_history_undo (h, img) && same_pixels (img, start));
      CHECK (!easel_history_can_undo (h));                            /* one step only */
      easel_image_free (drawn);
      easel_history_free (h);
      easel_image_free (start);
      easel_image_free (img);
    }
  /* Partly outside: only the pixels inside change; wholly outside: nothing, no step. */
  {
    EaselImage *img = easel_image_new (300, 200, WHITE);
    EaselHistory *h = easel_history_new (1024 * 1024);
    EaselFigure f = with_points (EASEL_FIGURE_POLYGON, half_out, 3, true, 1, EASEL_FIGURE_FILL_ONLY);
    size_t changed = 0;

    CHECK (easel_figure_apply (img, h, &f, BLACK, RED, &changed) == EASEL_FIGURE_DONE);
    CHECK (changed > 0 && count (img, BLACK) == (int) changed);
    CHECK (easel_image_get (img, 0, 50) == BLACK && easel_image_get (img, 0, 30) == WHITE);   /* the edge crosses x = 0 at y 40 to 60 */
    f = with_points (EASEL_FIGURE_POLYGON, far_out, 3, true, 1, EASEL_FIGURE_FILL_ONLY);
    CHECK (easel_figure_apply (img, h, &f, BLACK, RED, &changed) == EASEL_FIGURE_NOTHING && changed == 0);
    easel_history_free (h);
    easel_image_free (img);
  }
}

static size_t
address_space (void)
{
  unsigned long pages = 0;
  FILE *f = fopen ("/proc/self/statm", "r");

  CHECK (f != NULL);
  CHECK (fscanf (f, "%lu", &pages) == 1);
  fclose (f);
  return (size_t) pages * 4096;
}

/* AS-12 (REQ-0011-15): with no memory for its step nothing is drawn and nothing changes. */
static void
as12_curve_polygon_memory (void)
{
  enum { N = 1800 };
  static const int square[8] = { 0, 0, N - 1, 0, N - 1, N - 1, 0, N - 1 };
  static const int curve_pts[8] = { 0, 900, 600, 0, 1200, 1800, 1799, 900 };
  EaselImage *img = easel_image_new (N, N, WHITE);
  EaselHistory *h = easel_history_new (16 * 1024 * 1024);
  EaselFigure poly = with_points (EASEL_FIGURE_POLYGON, square, 4, true, 1, EASEL_FIGURE_FILL_ONLY);
  EaselFigure cur = with_points (EASEL_FIGURE_CURVE, curve_pts, 4, false, 5, EASEL_FIGURE_OUTLINE);
  size_t changed = 0;
  struct rlimit old, lim;

  CHECK (img != NULL && h != NULL);
  CHECK (getrlimit (RLIMIT_AS, &old) == 0);
  lim = old;
  lim.rlim_cur = address_space () + 4 * 1024 * 1024;
  CHECK (setrlimit (RLIMIT_AS, &lim) == 0);
  CHECK (easel_figure_apply (img, h, &poly, BLACK, WHITE, &changed) == EASEL_FIGURE_NO_MEMORY);
  CHECK (setrlimit (RLIMIT_AS, &old) == 0);
  CHECK (changed == 0 && count (img, WHITE) == N * N);
  CHECK (!easel_history_can_undo (h) && !easel_history_is_modified (h));
  /* The cap: the same for the curve, whose log is smaller than the polygon's. */
  CHECK (easel_figure_apply_capped (img, h, &cur, BLACK, WHITE, &changed, 1024) == EASEL_FIGURE_NO_MEMORY);
  CHECK (count (img, WHITE) == N * N && changed == 0 && !easel_history_can_undo (h));
  CHECK (easel_figure_apply_capped (img, h, &poly, BLACK, WHITE, &changed, 1024 * 1024) == EASEL_FIGURE_NO_MEMORY);
  CHECK (count (img, WHITE) == N * N && !easel_history_can_undo (h));
  /* With memory the polygon is drawn: a step larger than the budget, kept as the only step. */
  CHECK (easel_figure_apply (img, h, &poly, BLACK, WHITE, &changed) == EASEL_FIGURE_DONE);
  CHECK (changed == (size_t) N * N && easel_history_bytes (h) > 16 * 1024 * 1024);
  CHECK (easel_history_undo (h, img) && count (img, WHITE) == N * N);
  easel_history_free (h);
  easel_image_free (img);
}

static size_t
heap_in_use (void)
{
  struct mallinfo2 mi = mallinfo2 ();

  return mi.uordblks + mi.hblkhd;
}

/* The memory a polygon's step holds is the memory the budget counts (the retro's rule, as 0010 T012). */
static void
as12_step_holds_what_it_counts (void)
{
  static const int rect[8] = { 0, 0, 512, 0, 512, 511, 0, 511 };
  EaselImage *img = easel_image_new (600, 600, WHITE);
  EaselHistory *h = easel_history_new ((size_t) 512 << 20);
  EaselFigure f = with_points (EASEL_FIGURE_POLYGON, rect, 4, true, 1, EASEL_FIGURE_FILL_ONLY);
  size_t changed = 0, before, held, counted;

  before = heap_in_use ();
  CHECK (easel_figure_apply (img, h, &f, BLACK, RED, &changed) == EASEL_FIGURE_DONE);
  CHECK (changed == 513u * 512u);
  held = heap_in_use () - before;
  counted = easel_history_bytes (h);
  CHECK (held <= counted + counted / 100 + 4096);
  easel_history_free (h);
  easel_image_free (img);
}

int
main (void)
{
  RUN (as12_curve_polygon_step_and_nothing);
  RUN (as12_curve_polygon_memory);
  RUN (as12_step_holds_what_it_counts);
  return 0;
}
