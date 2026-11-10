/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Fill, feature 0007 (REQ-0007-05, REQ-0007-06, REQ-0007-07). */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/resource.h>

#include "check.h"
#include "fill.h"

#define WHITE 0xFFFFFFFFu
#define RED 0xFFFF0000u
#define BLUE 0xFF0000FFu
#define GREEN 0xFF00FF00u

static int
count (const EaselImage *img, uint32_t color)
{
  int n = 0;

  for (int i = 0; i < img->width * img->height; i++)
    n += img->px[i] == color;
  return n;
}

/* AS-6: a closed ring keeps the fill inside, one that touches only by corners too, and a gap of one pixel lets it out. */
static void
as6_fill_ring (void)
{
  EaselImage *img = easel_image_new (20, 20, WHITE);
  EaselHistory *h = easel_history_new (1024 * 1024);
  size_t changed = 0;

  CHECK (img != NULL && h != NULL);
  for (int i = 5; i <= 14; i++)
    {
      easel_image_set (img, i, 5, RED);
      easel_image_set (img, i, 14, RED);
      easel_image_set (img, 5, i, RED);
      easel_image_set (img, 14, i, RED);
    }
  CHECK (easel_fill (img, h, 10, 10, BLUE, &changed) == EASEL_FILL_DONE);
  CHECK (changed == 8 * 8 && count (img, BLUE) == 64 && count (img, RED) == 36);
  CHECK (easel_image_get (img, 6, 6) == BLUE && easel_image_get (img, 13, 13) == BLUE && easel_image_get (img, 5, 5) == RED);
  CHECK (easel_image_get (img, 0, 0) == WHITE && easel_image_get (img, 19, 19) == WHITE);
  CHECK (easel_history_can_undo (h));
  CHECK (easel_history_undo (h, img));
  CHECK (count (img, BLUE) == 0 && count (img, RED) == 36 && count (img, WHITE) == 400 - 36);
  CHECK (!easel_history_can_undo (h));                                     /* one step, no more */
  CHECK (easel_history_redo (h, img) && count (img, BLUE) == 64);

  /* The outside of the ring, with the secondary color: everything else white. */
  CHECK (easel_fill (img, h, 0, 0, GREEN, &changed) == EASEL_FILL_DONE);
  CHECK (changed == 400 - 36 - 64 && count (img, WHITE) == 0 && count (img, GREEN) == 400 - 36 - 64);
  easel_history_free (h);
  easel_image_free (img);

  /* A ring of diagonal steps only (a diamond): its pixels touch by corners, which does not let the fill through. */
  img = easel_image_new (21, 21, WHITE);
  CHECK (img != NULL);
  for (int i = 0; i <= 8; i++)
    {
      easel_image_set (img, 10 + i, 2 + i, RED);
      easel_image_set (img, 10 - i, 2 + i, RED);
      easel_image_set (img, 10 + i, 18 - i, RED);
      easel_image_set (img, 10 - i, 18 - i, RED);
    }
  CHECK (easel_fill (img, NULL, 10, 10, BLUE, &changed) == EASEL_FILL_DONE);
  CHECK (easel_image_get (img, 10, 10) == BLUE && easel_image_get (img, 0, 0) == WHITE && easel_image_get (img, 10, 0) == WHITE);
  CHECK (changed == (size_t) count (img, BLUE));
  CHECK (easel_image_get (img, 3, 10) == BLUE && easel_image_get (img, 1, 10) == WHITE);   /* inside to the diamond's edge */
  easel_image_free (img);

  /* A gap of one pixel in a ring of edge-connected pixels lets it out: inside and outside are one area. */
  img = easel_image_new (20, 20, WHITE);
  CHECK (img != NULL);
  for (int i = 5; i <= 14; i++)
    {
      easel_image_set (img, i, 5, RED);
      easel_image_set (img, i, 14, RED);
      easel_image_set (img, 5, i, RED);
      easel_image_set (img, 14, i, RED);
    }
  easel_image_set (img, 9, 5, WHITE);
  CHECK (easel_fill (img, NULL, 10, 10, BLUE, &changed) == EASEL_FILL_DONE);
  CHECK (count (img, WHITE) == 0 && easel_image_get (img, 0, 0) == BLUE);
  easel_image_free (img);
}

/* AS-7: the match is exact in all four channels, except that every fully transparent pixel is one color whatever red, green and
   blue it holds (2026-11-10, ADR-0008); the new value is exactly the color, a fully transparent color written as 0x00000000. */
static void
as7_fill_exact (void)
{
  EaselImage *img = easel_image_new (6, 1, 0x00000000u);
  size_t changed = 0;

  CHECK (img != NULL);
  easel_image_set (img, 2, 0, 0x00FF0000u);                                /* transparent with a hidden color: the same */
  easel_image_set (img, 3, 0, 0x00ABCDEFu);
  easel_image_set (img, 4, 0, 0x80000000u);                                /* same rgb as 0, other alpha: not the same */
  CHECK (easel_fill (img, NULL, 0, 0, 0xFF000080u, &changed) == EASEL_FILL_DONE);
  CHECK (changed == 4);
  for (int x = 0; x < 4; x++)
    CHECK (easel_image_get (img, x, 0) == 0xFF000080u);
  CHECK (easel_image_get (img, 4, 0) == 0x80000000u && easel_image_get (img, 5, 0) == 0x00000000u);
  CHECK (easel_fill (img, NULL, 4, 0, BLUE, &changed) == EASEL_FILL_DONE && changed == 1);
  CHECK (easel_image_get (img, 4, 0) == BLUE);
  /* A fully transparent fill color writes 0x00000000, whatever red, green and blue it holds. */
  CHECK (easel_fill (img, NULL, 0, 0, 0x00FF00FFu, &changed) == EASEL_FILL_DONE && changed == 4);
  for (int x = 0; x < 4; x++)
    CHECK (easel_image_get (img, x, 0) == 0x00000000u);
  /* ... and on a fully transparent area it is the color already there: nothing changes. */
  easel_image_set (img, 1, 0, 0x00123456u);
  CHECK (easel_fill (img, NULL, 0, 0, 0x00ABCDEFu, &changed) == EASEL_FILL_NOTHING && changed == 0);
  CHECK (easel_image_get (img, 1, 0) == 0x00123456u);
  easel_image_free (img);
  {
    /* Undo of a fill over fully transparent pixels of several hidden colors gives each its own back. */
    EaselImage *t = easel_image_new (3, 1, 0);
    EaselHistory *h = easel_history_new (1u << 20);

    CHECK (t != NULL && h != NULL);
    easel_image_set (t, 0, 0, 0x00112233u);                               /* hidden colors, as an older picture may hold */
    easel_image_set (t, 1, 0, 0x00445566u);
    easel_image_set (t, 2, 0, 0x00000000u);
    CHECK (easel_fill (t, h, 0, 0, BLUE, &changed) == EASEL_FILL_DONE && changed == 3);
    CHECK (easel_history_undo (h, t));
    CHECK (easel_image_get (t, 0, 0) == 0x00112233u && easel_image_get (t, 1, 0) == 0x00445566u && easel_image_get (t, 2, 0) == 0);
    CHECK (easel_history_redo (h, t) && easel_image_get (t, 1, 0) == BLUE);
    easel_history_free (h);
    easel_image_free (t);
  }
}

/* AS-8: the color already there, or a point outside, changes nothing and adds no step. */
static void
as8_fill_nothing (void)
{
  EaselImage *img = easel_image_new (10, 10, WHITE);
  EaselHistory *h = easel_history_new (1024 * 1024);
  size_t changed = 99;

  CHECK (img != NULL && h != NULL);
  CHECK (easel_fill (img, h, 3, 3, WHITE, &changed) == EASEL_FILL_NOTHING && changed == 0);
  CHECK (!easel_history_can_undo (h) && !easel_history_is_modified (h));
  CHECK (easel_fill (img, h, -1, 3, BLUE, &changed) == EASEL_FILL_NOTHING);
  CHECK (easel_fill (img, h, 3, 10, BLUE, &changed) == EASEL_FILL_NOTHING);
  CHECK (easel_fill (img, h, 10, 0, BLUE, NULL) == EASEL_FILL_NOTHING);
  CHECK (!easel_history_can_undo (h) && count (img, WHITE) == 100);
  CHECK (!img->dirty.x1 || img->dirty.x1 <= img->dirty.x0);                /* nothing was marked changed */
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

/* The worst case for a fill: the area is every other column joined by its top row, so almost every span is one pixel. */
static EaselImage *
comb (int n)
{
  EaselImage *img = easel_image_new (n, n, WHITE);

  CHECK (img != NULL);
  for (int y = 0; y < n; y++)
    for (int x = 0; x < n; x++)
      if (y == 0 || x % 2 == 0)
        easel_image_set (img, x, y, RED);
  return img;
}

/* AS-9: a step larger than the budget is kept alone, and a fill that cannot get its memory changes nothing. */
static void
as9_fill_memory_and_budget (void)
{
  enum { N = 1800 };
  const size_t spans = (size_t) (N - 1) * (N / 2) + 1;          /* the top row, and a span for each row of each column */
  EaselImage *img = comb (N);
  EaselHistory *h = easel_history_new (16 * 1024 * 1024);
  size_t changed = 0;
  struct rlimit old, lim;

  CHECK (img != NULL && h != NULL);

  /* No memory: the address space is limited to a little above what is in use, the fill refuses, nothing changed. */
  CHECK (getrlimit (RLIMIT_AS, &old) == 0);
  lim = old;
  lim.rlim_cur = address_space () + 4 * 1024 * 1024;
  CHECK (setrlimit (RLIMIT_AS, &lim) == 0);
  CHECK (easel_fill (img, h, 0, 0, BLUE, &changed) == EASEL_FILL_NO_MEMORY);
  CHECK (setrlimit (RLIMIT_AS, &old) == 0);
  CHECK (changed == 0 && !easel_history_can_undo (h) && !easel_history_is_modified (h));
  CHECK (count (img, BLUE) == 0 && count (img, RED) == N + (N - 1) * (N / 2));   /* every red pixel is red again */

  /* With memory: one step of 1.6 million spans, 19.4 MB against a budget of 16 MB: the only step, and one Undo undoes it. */
  CHECK (easel_fill (img, h, 0, 0, BLUE, &changed) == EASEL_FILL_DONE);
  CHECK (changed == (size_t) N + (size_t) (N - 1) * (N / 2) && count (img, RED) == 0);
  CHECK (easel_history_bytes (h) == spans * 12 + 64 && easel_history_bytes (h) > 16 * 1024 * 1024);
  CHECK (easel_history_can_undo (h));
  CHECK (easel_history_undo (h, img));
  CHECK (count (img, BLUE) == 0 && count (img, RED) == N + (N - 1) * (N / 2));
  CHECK (!easel_history_can_undo (h));
  easel_history_free (h);
  easel_image_free (img);
}

int
main (void)
{
  RUN (as6_fill_ring);
  RUN (as7_fill_exact);
  RUN (as8_fill_nothing);
  RUN (as9_fill_memory_and_budget);
  return 0;
}
