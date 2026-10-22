/* SPDX-License-Identifier: GPL-3.0-or-later */
/* The path of a free-form selection as a line along the grid lines (feature 0014, REQ-0014-04, REQ-0014-05): the walk between two
   corners, and the dashes counted along the path. */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "check.h"
#include "image.h"
#include "lattice.h"
#include "stroke.h"

/* The corners a walk from (x0, y0) to (x1, y1) visits, into out (x, y pairs): their number. */
static size_t
walk (int x0, int y0, int x1, int y1, int *out, size_t max)
{
  EaselLattice w;
  int x, y;
  size_t n = 0;

  easel_lattice_init (&w, x0, y0, x1, y1);
  while (easel_lattice_next (&w, &x, &y))
    {
      CHECK (n < max);
      out[2 * n] = x;
      out[2 * n + 1] = y;
      n++;
    }
  return n;
}

/* A walk is made of steps along grid lines: each step changes x or y by one, never both; it starts at the first corner and ends
   at the last; a diagonal step of the Pencil's line is a horizontal step and then a vertical one. By hand: (1, 1) to (4, 3). */
static void
lattice_follows_the_pencil (void)
{
  int c[64];
  size_t n;
  const int want[] = { 1, 1, 2, 1, 2, 2, 3, 2, 4, 2, 4, 3 };

  n = walk (1, 1, 4, 3, c, 32);
  CHECK (n == 6 && memcmp (c, want, sizeof want) == 0);
  CHECK (walk (3, 3, 3, 3, c, 32) == 1 && c[0] == 3 && c[1] == 3);                      /* a corner to itself: just the corner */
  n = walk (5, 2, 1, 2, c, 32);                                                         /* straight: one step a unit */
  CHECK (n == 5 && c[0] == 5 && c[8] == 1 && c[1] == 2 && c[9] == 2);
  /* Random pairs: the pixels of the Pencil's line between the two points, each diagonal step made of two, are the corners walked. */
  {
    uint32_t seed = 99;

    for (int round = 0; round < 300; round++)
      {
        int p[4];
        EaselImage *img = easel_image_new (40, 40, 0xFFFFFFFFu), *w = easel_image_new (40, 40, 0xFFFFFFFFu);
        EaselStroke *s;

        CHECK (img != NULL && w != NULL);
        for (int i = 0; i < 4; i++)
          {
            seed = seed * 1103515245u + 12345u;
            p[i] = (int) ((seed >> 16) % 20);
          }
        /* The Pencil's pixels of the line: the corners that the walk visits with no diagonal step are among them, and every
           Pencil pixel is a corner of the walk. */
        s = easel_stroke_begin (img, p[0], p[1], 0xFF000000u);
        CHECK (s != NULL);
        easel_stroke_to (s, p[2], p[3]);
        CHECK (easel_stroke_end (s, NULL) > 0);
        n = walk (p[0], p[1], p[2], p[3], c, 64);
        CHECK (n >= 1 && c[0] == p[0] && c[1] == p[1] && c[2 * n - 2] == p[2] && c[2 * n - 1] == p[3]);
        for (size_t i = 0; i < n; i++)
          w->px[c[2 * i + 1] * 40 + c[2 * i]] = 0xFF000000u;
        for (int i = 0; i < 40 * 40; i++)
          CHECK (img->px[i] != 0xFF000000u || w->px[i] == 0xFF000000u);
        for (size_t i = 1; i < n; i++)
          CHECK (abs (c[2 * i] - c[2 * i - 2]) + abs (c[2 * i + 1] - c[2 * i - 1]) == 1);
        easel_image_free (img);
        easel_image_free (w);
      }
  }
}

/* The black pieces along a path, counted from its first corner, black first, cut where it ends, a piece that turns a corner in
   two, pieces that continue one another joined. By hand: the open path (0, 0), (2, 0), (2, 1) is 3 steps long; with dashes of 1:
   the first step, and the last; with 1.5: from (0, 0) to (1.5, 0), one piece across the first two steps. The closed square
   (0, 0), (1, 0), (1, 1), (0, 1) is 4 long: the top and the bottom with dashes of 1. */
static void
as21_lattice_dashes (void)
{
  const int open[] = { 0, 0, 2, 0, 2, 1 };
  const int square[] = { 0, 0, 1, 0, 1, 1, 0, 1 };
  EaselDash d[8];

  CHECK (easel_lattice_dashes (open, 3, false, 1.0, 0.0, d, 8) == 2);
  CHECK (d[0].x0 == 0 && d[0].y0 == 0 && d[0].x1 == 1 && d[0].y1 == 0);
  CHECK (d[1].x0 == 2 && d[1].y0 == 0 && d[1].x1 == 2 && d[1].y1 == 1);
  CHECK (easel_lattice_dashes (open, 3, false, 1.5, 0.0, d, 8) == 1);
  CHECK (d[0].x0 == 0 && d[0].y0 == 0 && d[0].x1 == 1.5 && d[0].y1 == 0);
  CHECK (easel_lattice_dashes (open, 3, false, 2.0, 0.0, d, 8) == 1);                    /* [0, 2) along the top; [4, 6) is past the end */
  CHECK (d[0].x0 == 0 && d[0].y0 == 0 && d[0].x1 == 2 && d[0].y1 == 0);
  CHECK (easel_lattice_dashes (square, 4, true, 1.0, 0.0, d, 8) == 2);
  CHECK (d[0].x0 == 0 && d[0].y0 == 0 && d[0].x1 == 1 && d[0].y1 == 0);
  CHECK (d[1].x0 == 1 && d[1].y0 == 1 && d[1].x1 == 0 && d[1].y1 == 1);
  CHECK (easel_lattice_dashes (square, 4, false, 1.0, 0.0, NULL, 0) == 2);                /* open, 3 steps: the top and the bottom */
  /* The closing segment counts: with dashes of 1.5 the closed square is black on [0, 1.5) (the top and half the right side) and on
     [3, 4.5) (the left side, going up, all of it that there is), the open one has no left side. */
  CHECK (easel_lattice_dashes (square, 4, true, 1.5, 0.0, d, 8) == 3);
  CHECK (d[2].x0 == 0 && d[2].y0 == 1 && d[2].x1 == 0 && d[2].y1 == 0);
  CHECK (easel_lattice_dashes (square, 4, false, 1.5, 0.0, d, 8) == 2);
}

/* A path that moves keeps its dashes: the same corners moved by (10, 3) give the same pieces moved, also for a dash that is not a
   whole number of pixels and for a diagonal step. */
static void
as21_moved_path_same_dashes (void)
{
  const int a[] = { 1, 1, 7, 1, 1, 7 };
  int b[6];
  double dashes[3] = { 2.0, 1.7, 0.25 };

  for (int i = 0; i < 3; i++)
    {
      b[2 * i] = a[2 * i] + 10;
      b[2 * i + 1] = a[2 * i + 1] + 3;
    }
  for (int k = 0; k < 3; k++)
    {
      size_t na = easel_lattice_dashes (a, 3, true, dashes[k], 0.0, NULL, 0), nb = easel_lattice_dashes (b, 3, true, dashes[k], 0.0, NULL, 0);
      EaselDash *da = malloc ((na + 1) * sizeof *da), *db = malloc ((nb + 1) * sizeof *db);

      CHECK (da != NULL && db != NULL && na == nb && na > 0);
      CHECK (easel_lattice_dashes (a, 3, true, dashes[k], 0.0, da, na) == na && easel_lattice_dashes (b, 3, true, dashes[k], 0.0, db, nb) == nb);
      for (size_t i = 0; i < na; i++)
        CHECK (db[i].x0 == da[i].x0 + 10 && db[i].y0 == da[i].y0 + 3 && db[i].x1 == da[i].x1 + 10 && db[i].y1 == da[i].y1 + 3);
      free (da);
      free (db);
    }
  /* The triangle: 24 steps, the dashes of 2 black on [0, 2), [4, 6), ... [20, 22): 12 black in all. */
  {
    size_t n = easel_lattice_dashes (a, 3, true, 2.0, 0.0, NULL, 0);
    EaselDash *d = malloc ((n + 1) * sizeof *d);
    double sum = 0;

    CHECK (d != NULL && easel_lattice_dashes (a, 3, true, 2.0, 0.0, d, n) == n);
    for (size_t i = 0; i < n; i++)
      sum += abs ((int) (d[i].x1 - d[i].x0)) + abs ((int) (d[i].y1 - d[i].y0));
    CHECK (sum == 12.0);
    free (d);
  }
}

/* The phase: how far the pattern has advanced along the line, so that the dashes move on as it grows. By hand, the open path (0, 0),
   (2, 0), (2, 1) with dashes of 1: a phase of 1 gives the black [1, 2), the piece (1, 0) to (2, 0) (the same as a phase of 3); a
   phase of 0.5 gives [0.5, 1.5) and [2.5, 3): the piece (0.5, 0) to (1.5, 0) and the last half of the side, (2, 0.5) to (2, 1); a
   phase of 2 is a phase of 0. */
static void
as25_dash_phase (void)
{
  const int open[] = { 0, 0, 2, 0, 2, 1 };
  EaselDash d[8];

  CHECK (easel_lattice_dashes (open, 3, false, 1.0, 1.0, d, 8) == 1);
  CHECK (d[0].x0 == 1 && d[0].y0 == 0 && d[0].x1 == 2 && d[0].y1 == 0);
  CHECK (easel_lattice_dashes (open, 3, false, 1.0, 3.0, d, 8) == 1);
  CHECK (d[0].x0 == 1 && d[0].y0 == 0 && d[0].x1 == 2 && d[0].y1 == 0);
  CHECK (easel_lattice_dashes (open, 3, false, 1.0, 0.5, d, 8) == 2);
  CHECK (d[0].x0 == 0.5 && d[0].y0 == 0 && d[0].x1 == 1.5 && d[0].y1 == 0);
  CHECK (d[1].x0 == 2 && d[1].y0 == 0.5 && d[1].x1 == 2 && d[1].y1 == 1);
  CHECK (easel_lattice_dashes (open, 3, false, 1.0, 2.0, d, 8) == 2);
  CHECK (d[0].x0 == 0 && d[0].x1 == 1 && d[1].x0 == 2 && d[1].y1 == 1);
  /* The pattern keeps its length whatever the phase: the black on the triangle is 12 of its 24 steps for any phase. */
  {
    const int tri[] = { 1, 1, 7, 1, 1, 7 };

    for (int k = 0; k < 8; k++)
      {
        size_t n = easel_lattice_dashes (tri, 3, true, 2.0, 0.37 * k, NULL, 0);
        EaselDash *dd = malloc ((n + 1) * sizeof *dd);
        double sum = 0;

        CHECK (dd != NULL && easel_lattice_dashes (tri, 3, true, 2.0, 0.37 * k, dd, n) == n);
        for (size_t i = 0; i < n; i++)
          sum += (dd[i].x1 > dd[i].x0 ? dd[i].x1 - dd[i].x0 : dd[i].x0 - dd[i].x1) + (dd[i].y1 > dd[i].y0 ? dd[i].y1 - dd[i].y0 : dd[i].y0 - dd[i].y1);
        CHECK (sum > 11.99 && sum < 12.01 + 2.0);        /* a dash cut at the end may lose up to a dash: 12 less at most 2 */
        free (dd);
      }
  }
}

/* The unit grid steps the path walks, in order, into out (x0, y0, x1, y1 each): their number. */
static size_t
path_steps (const int *xy, size_t n, bool closed, int *out, size_t max)
{
  size_t segments = closed && n >= 2 ? n : (n >= 1 ? n - 1 : 0), k = 0;

  for (size_t i = 0; i < segments; i++)
    {
      size_t j = (i + 1) % n;
      EaselLattice w;
      int x, y, px = 0, py = 0;
      bool first = true;

      easel_lattice_init (&w, xy[2 * i], xy[2 * i + 1], xy[2 * j], xy[2 * j + 1]);
      while (easel_lattice_next (&w, &x, &y))
        {
          if (!first && k < max)
            {
              out[4 * k] = px;
              out[4 * k + 1] = py;
              out[4 * k + 2] = x;
              out[4 * k + 3] = y;
              k++;
            }
          first = false;
          px = x;
          py = y;
        }
    }
  return k;
}

/* 0014 T015 (REQ-0014-16): the white line's runs. A rectangle traced through many corners is 4 runs, a straight path 1; for random paths,
   the runs walked one grid step at a time give the walker's steps in the same order, and two runs that follow one another never go
   the same way along the same axis. */
static void
lattice_runs_merge (void)
{
  static int rect[2 * 30], line[2 * 101], xy[2 * 12], steps[4 * 2000], again[4 * 2000];
  EaselDash runs[2000];
  size_t n = 0, k;

  for (int x = 0; x <= 10; x++, n++)
    rect[2 * n] = x, rect[2 * n + 1] = 0;
  for (int y = 1; y <= 5; y++, n++)
    rect[2 * n] = 10, rect[2 * n + 1] = y;
  for (int x = 9; x >= 0; x--, n++)
    rect[2 * n] = x, rect[2 * n + 1] = 5;
  for (int y = 4; y >= 1; y--, n++)
    rect[2 * n] = 0, rect[2 * n + 1] = y;
  k = easel_lattice_runs (rect, n, true, runs, 2000);
  CHECK (k == 4);
  CHECK (runs[0].x0 == 0 && runs[0].y0 == 0 && runs[0].x1 == 10 && runs[0].y1 == 0);
  CHECK (runs[1].x0 == 10 && runs[1].y0 == 0 && runs[1].x1 == 10 && runs[1].y1 == 5);
  CHECK (runs[2].x0 == 10 && runs[2].y0 == 5 && runs[2].x1 == 0 && runs[2].y1 == 5);
  CHECK (runs[3].x0 == 0 && runs[3].y0 == 5 && runs[3].x1 == 0 && runs[3].y1 == 0);
  for (int x = 0; x <= 100; x++)
    line[2 * x] = x, line[2 * x + 1] = 3;
  CHECK (easel_lattice_runs (line, 101, false, runs, 2000) == 1);
  CHECK (easel_lattice_runs (line, 101, false, NULL, 0) == 1);
  srand (5);
  for (int round = 0; round < 300; round++)
    {
      size_t m = 2 + (size_t) (rand () % 10), ns, nr, na = 0;
      bool closed = rand () % 2;

      for (size_t i = 0; i < m; i++)
        {
          xy[2 * i] = rand () % 15;
          xy[2 * i + 1] = rand () % 15;
        }
      ns = path_steps (xy, m, closed, steps, 2000);
      nr = easel_lattice_runs (xy, m, closed, runs, 2000);
      CHECK (nr <= ns);
      for (size_t r = 0; r < nr; r++)
        {
          int x = (int) runs[r].x0, y = (int) runs[r].y0, ex = (int) runs[r].x1, ey = (int) runs[r].y1;

          CHECK (x == ex || y == ey);
          CHECK (x != ex || y != ey);
          if (r > 0)
            {
              bool along_x = y == ey, prev_x = runs[r - 1].y0 == runs[r - 1].y1;
              int dir = along_x ? (ex > x) - (ex < x) : (ey > y) - (ey < y);
              int prev = prev_x ? (runs[r - 1].x1 > runs[r - 1].x0) - (runs[r - 1].x1 < runs[r - 1].x0)
                                : (runs[r - 1].y1 > runs[r - 1].y0) - (runs[r - 1].y1 < runs[r - 1].y0);

              CHECK (along_x != prev_x || dir != prev);
              CHECK (runs[r - 1].x1 == runs[r].x0 && runs[r - 1].y1 == runs[r].y0);
            }
          while (x != ex || y != ey)
            {
              int nx = x + (ex > x) - (ex < x), ny = y + (ey > y) - (ey < y);

              again[4 * na] = x;
              again[4 * na + 1] = y;
              again[4 * na + 2] = nx;
              again[4 * na + 3] = ny;
              na++;
              x = nx;
              y = ny;
            }
        }
      CHECK (na == ns);
      CHECK (memcmp (again, steps, sizeof (int) * 4 * ns) == 0);
    }
}

int
main (void)
{
  RUN (lattice_follows_the_pencil);
  RUN (as21_lattice_dashes);
  RUN (as21_moved_path_same_dashes);
  RUN (as25_dash_phase);
  RUN (lattice_runs_merge);
  return 0;
}
