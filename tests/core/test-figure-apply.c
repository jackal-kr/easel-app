/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Drawing a figure into a picture, with its one undo step, feature 0010 (REQ-0010-06, REQ-0010-07, REQ-0010-13,
   REQ-0010-20). */

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
figure (EaselFigureKind kind, int x0, int y0, int x1, int y1, int width, EaselFigureStyle style)
{
  return (EaselFigure) { kind, x0, y0, x1, y1, width, style, NULL };
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

/* AS-10 (REQ-0010-05, REQ-0010-06): only the pixels inside the picture change, and they are exactly the color, also with
   transparency (no blending). */
static void
as10_clipped_and_exact_color (void)
{
  EaselImage *img = easel_image_new (300, 200, WHITE);
  EaselHistory *h = easel_history_new (1024 * 1024);
  EaselFigure f = figure (EASEL_FIGURE_LINE, -20, 50, 50, 50, 1, EASEL_FIGURE_OUTLINE);
  size_t changed = 0;

  CHECK (img != NULL && h != NULL);
  CHECK (easel_figure_apply (img, h, &f, 0xFF000080u, WHITE, &changed) == EASEL_FIGURE_DONE);
  CHECK (changed == 51);                                           /* x 0 to 50 of row 50 */
  CHECK (count (img, 0xFF000080u) == 51);
  for (int x = 0; x < 300; x++)
    CHECK ((easel_image_get (img, x, 50) == 0xFF000080u) == (x <= 50));
  CHECK (count (img, WHITE) == 300 * 200 - 51);                    /* nothing else changed */
  /* Wholly outside: nothing, no step. */
  f = figure (EASEL_FIGURE_RECTANGLE, -50, -50, -10, -10, 1, EASEL_FIGURE_FILL_ONLY);
  CHECK (easel_figure_apply (img, h, &f, BLACK, WHITE, &changed) == EASEL_FIGURE_NOTHING);
  CHECK (changed == 0);
  /* The fill styles: a 10 x 5 rectangle, black drawing color, red the other. */
  easel_history_free (h);
  easel_image_free (img);
  img = easel_image_new (300, 200, WHITE);
  h = easel_history_new (1024 * 1024);
  f = figure (EASEL_FIGURE_RECTANGLE, 10, 10, 19, 14, 1, EASEL_FIGURE_OUTLINE);
  CHECK (easel_figure_apply (img, h, &f, BLACK, RED, &changed) == EASEL_FIGURE_DONE);
  CHECK (changed == 26 && count (img, BLACK) == 26 && count (img, RED) == 0);       /* outline only: the inside kept */
  f.style = EASEL_FIGURE_OUTLINE_AND_FILL;
  CHECK (easel_figure_apply (img, h, &f, BLACK, RED, &changed) == EASEL_FIGURE_DONE);
  CHECK (changed == 24 && count (img, BLACK) == 26 && count (img, RED) == 24);      /* the inside in the other color */
  f.style = EASEL_FIGURE_FILL_ONLY;
  CHECK (easel_figure_apply (img, h, &f, BLACK, RED, &changed) == EASEL_FIGURE_DONE);
  CHECK (changed == 24 && count (img, BLACK) == 50 && count (img, RED) == 0);       /* all in the drawing color */
  easel_history_free (h);
  easel_image_free (img);
}

/* AS-4, AS-13 (REQ-0010-07): one step of 12 bytes a changed pixel plus 64; undo and redo restore exactly; a figure that
   changes nothing adds no step. */
static void
as13_step_undo_redo_and_nothing (void)
{
  EaselImage *img = easel_image_new (100, 80, WHITE), *before = easel_image_new (100, 80, WHITE);
  EaselHistory *h = easel_history_new (16 * 1024 * 1024);
  EaselFigure f = figure (EASEL_FIGURE_ELLIPSE, 10, 10, 60, 40, 3, EASEL_FIGURE_OUTLINE_AND_FILL);
  EaselImage *after;
  size_t changed = 0;

  CHECK (img != NULL && before != NULL && h != NULL);
  CHECK (easel_figure_apply (img, h, &f, BLACK, RED, &changed) == EASEL_FIGURE_DONE);
  CHECK (changed > 100);
  CHECK (easel_history_bytes (h) == changed * 12 + 64);
  CHECK (easel_history_is_modified (h));
  after = easel_image_new (100, 80, WHITE);
  memcpy (after->px, img->px, (size_t) 100 * 80 * sizeof after->px[0]);
  CHECK (easel_history_undo (h, img));
  CHECK (same_pixels (img, before));
  CHECK (!easel_history_can_undo (h));
  CHECK (easel_history_redo (h, img));
  CHECK (same_pixels (img, after));
  /* The same figure again changes nothing: no step, no change of the history. */
  {
    size_t bytes = easel_history_bytes (h);

    CHECK (easel_figure_apply (img, h, &f, BLACK, RED, &changed) == EASEL_FIGURE_NOTHING);
    CHECK (changed == 0 && easel_history_bytes (h) == bytes && !easel_history_can_redo (h));
  }
  /* White on white: nothing changes (REQ-0010-07). */
  {
    EaselImage *white = easel_image_new (50, 50, WHITE);
    EaselHistory *h2 = easel_history_new (1024 * 1024);
    EaselFigure g = figure (EASEL_FIGURE_RECTANGLE, 5, 5, 30, 30, 1, EASEL_FIGURE_OUTLINE);

    CHECK (easel_figure_apply (white, h2, &g, WHITE, WHITE, &changed) == EASEL_FIGURE_NOTHING);
    CHECK (!easel_history_can_undo (h2) && !easel_history_is_modified (h2));
    easel_history_free (h2);
    easel_image_free (white);
  }
  /* Without a history the pixels are just drawn. */
  {
    EaselImage *plain = easel_image_new (50, 50, WHITE);
    EaselFigure g = figure (EASEL_FIGURE_RECTANGLE, 5, 5, 10, 10, 1, EASEL_FIGURE_FILL_ONLY);

    CHECK (easel_figure_apply (plain, NULL, &g, BLACK, WHITE, &changed) == EASEL_FIGURE_DONE);
    CHECK (changed == 36 && count (plain, BLACK) == 36);
    easel_image_free (plain);
  }
  easel_image_free (after);
  easel_image_free (before);
  easel_history_free (h);
  easel_image_free (img);
}

/* The size of the address space of this process in bytes (Linux), to set a limit just above it. */
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

/* AS-16 (REQ-0010-20): with no memory for its step the figure is not drawn and nothing changes; a step larger than the
   whole budget is kept as the only step and one Undo restores the picture. */
static void
as16_shape_memory_and_budget (void)
{
  enum { N = 1800 };
  EaselImage *img = easel_image_new (N, N, WHITE);
  EaselHistory *h = easel_history_new (16 * 1024 * 1024);
  EaselFigure f = figure (EASEL_FIGURE_RECTANGLE, 0, 0, N - 1, N - 1, 1, EASEL_FIGURE_FILL_ONLY);
  size_t changed = 0;
  struct rlimit old, lim;

  CHECK (img != NULL && h != NULL);
  /* No memory: the address space is limited to a little above what is in use. */
  CHECK (getrlimit (RLIMIT_AS, &old) == 0);
  lim = old;
  lim.rlim_cur = address_space () + 4 * 1024 * 1024;
  CHECK (setrlimit (RLIMIT_AS, &lim) == 0);
  CHECK (easel_figure_apply (img, h, &f, BLACK, WHITE, &changed) == EASEL_FIGURE_NO_MEMORY);
  CHECK (setrlimit (RLIMIT_AS, &old) == 0);
  CHECK (changed == 0 && count (img, WHITE) == N * N);                       /* every pixel back */
  CHECK (!easel_history_can_undo (h) && !easel_history_is_modified (h));
  /* The test hook with a cap does the same without limiting the process. */
  CHECK (easel_figure_apply_capped (img, h, &f, BLACK, WHITE, &changed, 1024 * 1024) == EASEL_FIGURE_NO_MEMORY);
  CHECK (count (img, WHITE) == N * N && !easel_history_can_undo (h));
  /* With memory: a step of 3.24 million pixels, 38.9 MB against a budget of 16 MB: the only step. */
  CHECK (easel_figure_apply (img, h, &f, BLACK, WHITE, &changed) == EASEL_FIGURE_DONE);
  CHECK (changed == (size_t) N * N && count (img, BLACK) == N * N);
  CHECK (easel_history_bytes (h) == (size_t) N * N * 12 + 64 && easel_history_bytes (h) > 16 * 1024 * 1024);
  CHECK (easel_history_can_undo (h));
  CHECK (easel_history_undo (h, img));
  CHECK (count (img, WHITE) == N * N);
  CHECK (!easel_history_can_undo (h));
  easel_history_free (h);
  easel_image_free (img);
}

/* The heap memory in use now, as glibc counts it: the small blocks and the mapped ones (a big log is a mapped block). */
static size_t
heap_in_use (void)
{
  struct mallinfo2 mi = mallinfo2 ();

  return mi.uordblks + mi.hblkhd;
}

/* F1 of the 0010 check (REQ-0010-07, REQ-0002-16): the memory a step holds is the memory the budget counts. A figure that
   changes just more pixels than a power of two of log entries must not leave the log's spare half in the history. */
static void
as4_step_holds_what_it_counts (void)
{
  EaselImage *img = easel_image_new (600, 600, WHITE);
  EaselHistory *h = easel_history_new ((size_t) 512 << 20);
  EaselFigure f = figure (EASEL_FIGURE_RECTANGLE, 0, 0, 512, 511, 1, EASEL_FIGURE_FILL_ONLY);   /* 513 x 512 pixels */
  size_t changed = 0, before, held, counted;

  before = heap_in_use ();
  CHECK (easel_figure_apply (img, h, &f, BLACK, RED, &changed) == EASEL_FIGURE_DONE);
  CHECK (changed == 513u * 512u);
  held = heap_in_use () - before;
  counted = easel_history_bytes (h);
  printf ("  changed %zu pixels: held %zu bytes, counted %zu\n", changed, held, counted);
  CHECK (held <= counted + counted / 100 + 4096);
  easel_history_free (h);
  easel_image_free (img);
}

/* AS-5 (REQ-0025-01): each shape writes the given color exactly, alpha included, over white, and no other value. */
static void
as5_shapes_write_exact_colors (void)
{
  static const uint32_t colors[] = { 0x00000000u, 0x80FF0000u, 0xFF336699u };
  static const int pts[] = { 20, 30, 150, 100, 30, 140 };
  static const int curve[] = { 20, 30, 150, 100, 60, 120, 110, 10 };

  for (size_t c = 0; c < sizeof colors / sizeof colors[0]; c++)
    for (int shape = 0; shape < 5; shape++)
      {
        EaselImage *img = easel_image_new (200, 150, WHITE);
        EaselFigure f = figure (EASEL_FIGURE_LINE, 20, 30, 150, 100, 3, EASEL_FIGURE_OUTLINE);
        size_t changed = 0;
        int written = 0;

        CHECK (img != NULL);
        if (shape == 1)
          f = figure (EASEL_FIGURE_RECTANGLE, 20, 30, 150, 100, 2, EASEL_FIGURE_FILL_ONLY);
        else if (shape == 2)
          f = figure (EASEL_FIGURE_ELLIPSE, 20, 30, 150, 100, 4, EASEL_FIGURE_OUTLINE);
        else if (shape == 3)
          {
            f.kind = EASEL_FIGURE_CURVE;
            f.width = 1;
            f.pts = curve;
            f.n_pts = 4;
          }
        else if (shape == 4)
          {
            f.kind = EASEL_FIGURE_POLYGON;
            f.width = 1;
            f.pts = pts;
            f.n_pts = 3;
            f.closed = true;
          }
        CHECK (easel_figure_apply (img, NULL, &f, colors[c], WHITE, &changed) == EASEL_FIGURE_DONE);
        for (int i = 0; i < img->width * img->height; i++)
          {
            CHECK (img->px[i] == WHITE || img->px[i] == colors[c]);
            written += img->px[i] == colors[c];
          }
        CHECK (written > 0);
        CHECK ((size_t) written == changed);
        easel_image_free (img);
      }
}

int
main (void)
{
  RUN (as10_clipped_and_exact_color);
  RUN (as5_shapes_write_exact_colors);
  RUN (as13_step_undo_redo_and_nothing);
  RUN (as16_shape_memory_and_budget);
  RUN (as4_step_holds_what_it_counts);
  return 0;
}
