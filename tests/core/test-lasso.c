/* SPDX-License-Identifier: GPL-3.0-or-later */
/* The path of a free-form selection and its shape (feature 0014): REQ-0014-04, REQ-0014-06, REQ-0014-15. */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "check.h"
#include "image.h"
#include "lasso.h"
#include "lattice.h"

#define PW 21
#define PH 11

/* The shape of the path through the n pointer positions pts (x, y pairs) over a picture of 21 x 11, released at the last. */
static EaselSelectionResult
shape_of (const int *pts, int n, int *x, int *y, int *w, int *h, uint8_t **mask, size_t max_bytes)
{
  EaselPath p = { 0 };
  EaselSelectionResult r;

  for (int i = 0; i < n; i++)
    CHECK (easel_path_add (&p, pts[2 * i], pts[2 * i + 1], SIZE_MAX));
  r = easel_lasso_shape (&p, PW, PH, x, y, w, h, mask, max_bytes);
  easel_path_free (&p);
  return r;
}

static int
count_set (const uint8_t *mask, int w, int h)
{
  int n = 0;

  for (int i = 0; i < w * h; i++)
    n += mask[i] != 0;
  return n;
}

/* AS-2, the shape part: the path through the corners (1, 1), (7, 1), (1, 7) closes along the grid line x = 1 and encloses 15 pixels,
   i + j <= 4 in their rectangle (1, 1) to (5, 5); a path partly outside the picture is limited to it; one wholly outside, and a
   click, make none. */
static void
as2_lasso_shape (void)
{
  int x, y, w, h;
  uint8_t *mask = NULL;
  const int tri[] = { 1, 1, 7, 1, 1, 7 };
  const int clipped[] = { -2, -2, 3, -2, 3, 3 };
  const int outside[] = { -5, -5, -1, -1, -5, -1 };
  const int click[] = { 3, 3, 3, 3 };

  CHECK (shape_of (tri, 3, &x, &y, &w, &h, &mask, SIZE_MAX) == EASEL_SELECTION_DONE);
  CHECK (x == 1 && y == 1 && w == 5 && h == 5 && mask != NULL);
  CHECK (count_set (mask, w, h) == 15);
  for (int j = 0; j < 5; j++)
    for (int i = 0; i < 5; i++)
      CHECK ((mask[j * 5 + i] != 0) == (i + j <= 4));
  free (mask);
  CHECK (shape_of (clipped, 3, &x, &y, &w, &h, &mask, SIZE_MAX) == EASEL_SELECTION_DONE);
  CHECK (x == 0 && y == 0 && w == 3 && h == 3 && count_set (mask, w, h) == 6);
  free (mask);
  mask = NULL;
  CHECK (shape_of (outside, 3, &x, &y, &w, &h, &mask, SIZE_MAX) == EASEL_SELECTION_NOTHING && mask == NULL);
  CHECK (shape_of (click, 1, &x, &y, &w, &h, &mask, SIZE_MAX) == EASEL_SELECTION_NOTHING && mask == NULL);
  CHECK (shape_of (click, 2, &x, &y, &w, &h, &mask, SIZE_MAX) == EASEL_SELECTION_NOTHING && mask == NULL);
}

/* AS-3: a path that crosses itself holds the union of its loops (the bow tie, 8 pixels of its two lobes); a notch of the path that
   opens onto the edge of its extent is outside (the W and the M, 12 pixels each); a path that encloses no pixel, a straight drag
   and an out and back, makes none. */
static void
as3_crossing_and_thin_paths (void)
{
  int x, y, w, h;
  uint8_t *mask = NULL;
  const int bow[] = { 1, 1, 5, 5, 5, 1, 1, 5 };
  const int wshape[] = { 1, 1, 1, 5, 3, 3, 5, 5, 5, 1 };
  const int mshape[] = { 1, 5, 1, 1, 3, 3, 5, 1, 5, 5 };
  const int line[] = { 1, 1, 5, 1 };
  const int back[] = { 1, 1, 5, 3, 1, 1 };

  CHECK (shape_of (bow, 4, &x, &y, &w, &h, &mask, SIZE_MAX) == EASEL_SELECTION_DONE);
  CHECK (x == 1 && y == 1 && w == 4 && h == 3 && count_set (mask, w, h) == 8);
  free (mask);
  CHECK (shape_of (wshape, 5, &x, &y, &w, &h, &mask, SIZE_MAX) == EASEL_SELECTION_DONE);
  CHECK (x == 1 && y == 1 && w == 4 && h == 4 && count_set (mask, w, h) == 12);
  CHECK (!mask[3 * 4 + 1] && !mask[3 * 4 + 2] && !mask[3 * 4 + 3] && !mask[2 * 4 + 2] && mask[3 * 4] && mask[2 * 4 + 1]);
  free (mask);
  CHECK (shape_of (mshape, 5, &x, &y, &w, &h, &mask, SIZE_MAX) == EASEL_SELECTION_DONE);
  CHECK (x == 1 && y == 1 && w == 4 && h == 4 && count_set (mask, w, h) == 12);
  CHECK (!mask[1] && !mask[2] && !mask[3] && !mask[1 * 4 + 2] && mask[0] && mask[1 * 4 + 1]);
  free (mask);
  mask = NULL;
  CHECK (shape_of (line, 2, &x, &y, &w, &h, &mask, SIZE_MAX) == EASEL_SELECTION_NOTHING && mask == NULL);
  CHECK (shape_of (back, 3, &x, &y, &w, &h, &mask, SIZE_MAX) == EASEL_SELECTION_NOTHING && mask == NULL);
}

/* AS-20: the diamond through (1, 3), (3, 1), (5, 3), (3, 5) encloses 8 pixels, 4 x 4; a path round the whole picture all of it. */
static void
as20_diamond_and_whole_picture (void)
{
  int x, y, w, h;
  uint8_t *mask = NULL;
  const int diamond[] = { 1, 3, 3, 1, 5, 3, 3, 5 };
  const int round[] = { -2, -2, 23, -2, 23, 13, -2, 13 };
  const int want[16] = { 0, 0, 1, 0,  0, 1, 1, 1,  1, 1, 1, 0,  0, 1, 0, 0 };      /* (3, 1), (2, 2) to (4, 2), (1, 3) to (3, 3), (2, 4) */

  CHECK (shape_of (diamond, 4, &x, &y, &w, &h, &mask, SIZE_MAX) == EASEL_SELECTION_DONE);
  CHECK (x == 1 && y == 1 && w == 4 && h == 4 && count_set (mask, w, h) == 8);
  for (int i = 0; i < 16; i++)
    CHECK ((mask[i] != 0) == (want[i] != 0));
  free (mask);
  CHECK (shape_of (round, 4, &x, &y, &w, &h, &mask, SIZE_MAX) == EASEL_SELECTION_DONE);
  CHECK (x == 0 && y == 0 && w == PW && h == PH && count_set (mask, w, h) == PW * PH);
  free (mask);
}

/* AS-12, the core part (REQ-0014-15): a path whose memory is not had makes NO_MEMORY, with no mask and nothing changed; the
   path itself stops growing at its limit. */
static void
as12_shape_memory (void)
{
  int x, y, w, h;
  uint8_t *mask = NULL;
  const int tri[] = { 1, 1, 7, 1, 1, 7 };
  EaselPath p = { 0 };

  CHECK (shape_of (tri, 3, &x, &y, &w, &h, &mask, 100) == EASEL_SELECTION_NO_MEMORY && mask == NULL);
  CHECK (shape_of (tri, 3, &x, &y, &w, &h, &mask, 4000) == EASEL_SELECTION_DONE && mask != NULL);
  free (mask);
  /* The mask counts with the rest: a square of 999 x 999 pixels over a 2000 x 2000 picture needs a mask of about a million bytes. */
  CHECK (easel_path_add (&p, 0, 0, SIZE_MAX) && easel_path_add (&p, 999, 0, SIZE_MAX) && easel_path_add (&p, 999, 999, SIZE_MAX)
         && easel_path_add (&p, 0, 999, SIZE_MAX));
  CHECK (easel_lasso_shape (&p, 2000, 2000, &x, &y, &w, &h, &mask, 100000) == EASEL_SELECTION_NO_MEMORY && mask == NULL);
  CHECK (easel_lasso_shape (&p, 2000, 2000, &x, &y, &w, &h, &mask, SIZE_MAX) == EASEL_SELECTION_DONE);
  CHECK (w == 999 && h == 999 && count_set (mask, w, h) == 999 * 999);
  free (mask);
  easel_path_free (&p);
  CHECK (easel_path_add (&p, 1, 1, 16) && easel_path_add (&p, 2, 2, 16));
  CHECK (!easel_path_add (&p, 3, 3, 16) && p.n == 2);
  easel_path_free (&p);
}

/* The shape by brute force: the grid segments of the closed path in two bitmaps, and the pixels reachable from outside by steps that
   cross no segment, flooded over a window round the path; the rest, limited to the picture, is the shape. Returns the pixel count
   and fills `want` (PW x PH, the picture) with the shape. */
static int
shape_by_flood (const int *c, int n, int *want)
{
  enum { OFF = 20, S = 80 };                                     /* the lattice window: coordinates -20 to 59 */
  static unsigned char hseg[S][S], vseg[S][S], seen[S][S];
  int stack[S * S * 2], top = 0, count = 0;

  memset (hseg, 0, sizeof hseg);
  memset (vseg, 0, sizeof vseg);
  memset (seen, 0, sizeof seen);
  for (int i = 0; i < n; i++)
    {
      int j = (i + 1) % n, x, y, px = 0, py = 0;
      EaselLattice w;
      int first = 1;

      easel_lattice_init (&w, c[2 * i], c[2 * i + 1], c[2 * j], c[2 * j + 1]);
      while (easel_lattice_next (&w, &x, &y))
        {
          if (!first)
            {
              if (y == py)
                hseg[y + OFF][(x < px ? x : px) + OFF] = 1;          /* the segment (x, y)-(x + 1, y) */
              else
                vseg[(y < py ? y : py) + OFF][x + OFF] = 1;          /* the segment (x, y)-(x, y + 1) */
            }
          first = 0;
          px = x;
          py = y;
        }
    }
  /* The pixel (x, y) is the cell [x, x + 1) x [y, y + 1); moving right crosses vseg (x + 1, y), left vseg (x, y), down hseg (x, y + 1),
     up hseg (x, y). The flood starts at the corner of the window, outside everything. */
  stack[top++] = 0;
  stack[top++] = 0;
  seen[0][0] = 1;
  while (top > 0)
    {
      int y = stack[--top], x = stack[--top];
      static const int dx[4] = { 1, -1, 0, 0 }, dy[4] = { 0, 0, 1, -1 };

      for (int d = 0; d < 4; d++)
        {
          int nx = x + dx[d], ny = y + dy[d], blocked;

          if (nx < 0 || ny < 0 || nx >= S - 1 || ny >= S - 1)
            continue;
          blocked = d == 0 ? vseg[y][x + 1] : d == 1 ? vseg[y][x] : d == 2 ? hseg[y + 1][x] : hseg[y][x];
          if (blocked || seen[ny][nx])
            continue;
          seen[ny][nx] = 1;
          stack[top++] = nx;
          stack[top++] = ny;
        }
    }
  for (int y = 0; y < PH; y++)
    for (int x = 0; x < PW; x++)
      {
        want[y * PW + x] = !seen[y + OFF][x + OFF];
        count += want[y * PW + x];
      }
  return count;
}

/* The shape maker against the flood, on random closed paths of 3 to 8 corners in and round the picture (the window of the flood is
   -20 to 59): the same pixels, the same bounding rectangle, and none where the flood finds none. */
static void
lasso_matches_the_flood (void)
{
  uint32_t seed = 4242;

  for (int round = 0; round < 400; round++)
    {
      int c[16], n = 3 + (int) ((seed >> 7) % 6), want[PW * PH], count, x, y, w, h;
      uint8_t *mask = NULL;
      EaselSelectionResult r;

      for (int i = 0; i < 2 * n; i++)
        {
          seed = seed * 1103515245u + 12345u;
          c[i] = (int) ((seed >> 16) % 30) - 5;
        }
      count = shape_by_flood (c, n, want);
      r = shape_of (c, n, &x, &y, &w, &h, &mask, SIZE_MAX);
      if (count == 0)
        {
          CHECK (r == EASEL_SELECTION_NOTHING && mask == NULL);
          continue;
        }
      CHECK (r == EASEL_SELECTION_DONE && mask != NULL);
      {
        int min_x = PW, min_y = PH, max_x = -1, max_y = -1, got = 0;

        for (int j = 0; j < PH; j++)
          for (int i = 0; i < PW; i++)
            if (want[j * PW + i])
              {
                min_x = i < min_x ? i : min_x;
                max_x = i > max_x ? i : max_x;
                min_y = j < min_y ? j : min_y;
                max_y = j > max_y ? j : max_y;
              }
        CHECK (x == min_x && y == min_y && w == max_x - min_x + 1 && h == max_y - min_y + 1);
        for (int j = 0; j < h; j++)
          for (int i = 0; i < w; i++)
            {
              CHECK ((mask[j * w + i] != 0) == (want[(y + j) * PW + x + i] != 0));
              got += mask[j * w + i] != 0;
            }
        CHECK (got == count);
      }
      free (mask);
    }
}

/* 0015 T010 (REQ-0015-06): the traced path scaled with its shape. */

/* The path through the n corners pts (x, y pairs). */
static EaselPath
path_of (const int *pts, int n)
{
  EaselPath p = { 0 };

  for (int i = 0; i < n; i++)
    CHECK (easel_path_add (&p, pts[2 * i], pts[2 * i + 1], SIZE_MAX));
  return p;
}

/* AS-7: the triangle (corners (1, 1), (7, 1), (1, 7), its shape box (1, 1, 5, 5), the path's box one grid line larger to the right and
   below) made 10 x 5 at (1, 1): the box of the path becomes the new shape box widened by the same one grid line, each corner keeps its
   place in it: (1, 1), (12, 1), (1, 7); and the line, the walk of the closed path, is AS-7's, run by run. */
static void
as7_scaled_path_line (void)
{
  static const int tri[] = { 1, 1, 7, 1, 1, 7 };
  static const int runs_want[][4] = { { 1, 1, 12, 1 }, { 12, 1, 11, 1 }, { 11, 1, 11, 2 }, { 11, 2, 9, 2 }, { 9, 2, 9, 3 },
                                      { 9, 3, 7, 3 }, { 7, 3, 7, 4 }, { 7, 4, 5, 4 }, { 5, 4, 5, 5 }, { 5, 5, 3, 5 },
                                      { 3, 5, 3, 6 }, { 3, 6, 1, 6 }, { 1, 6, 1, 7 }, { 1, 7, 1, 1 } };
  EaselPath orig = path_of (tri, 3), out = { 0 };
  EaselDash runs[40];
  size_t n;

  CHECK (easel_lasso_scale_path (&orig, 1, 1, 5, 5, 1, 1, 10, 5, &out, SIZE_MAX));
  CHECK (out.n == 3);
  CHECK (out.xy[0] == 1 && out.xy[1] == 1 && out.xy[2] == 12 && out.xy[3] == 1 && out.xy[4] == 1 && out.xy[5] == 7);
  n = easel_lattice_runs (out.xy, out.n, true, runs, 40);
  CHECK (n == 14);
  for (size_t i = 0; i < n; i++)
    CHECK ((int) runs[i].x0 == runs_want[i][0] && (int) runs[i].y0 == runs_want[i][1] && (int) runs[i].x1 == runs_want[i][2]
           && (int) runs[i].y1 == runs_want[i][3]);
  easel_path_free (&out);
  /* The same size at the same place gives the same path; moved by (3, 2) the corners move by it. */
  CHECK (easel_lasso_scale_path (&orig, 1, 1, 5, 5, 1, 1, 5, 5, &out, SIZE_MAX));
  CHECK (memcmp (out.xy, tri, sizeof tri) == 0);
  easel_path_free (&out);
  CHECK (easel_lasso_scale_path (&orig, 1, 1, 5, 5, 4, 3, 5, 5, &out, SIZE_MAX));
  CHECK (out.xy[0] == 4 && out.xy[1] == 3 && out.xy[2] == 10 && out.xy[3] == 3 && out.xy[4] == 4 && out.xy[5] == 9);
  easel_path_free (&out);
  easel_path_free (&orig);
}

/* A corner halfway between two grid lines goes to the larger one (halves toward larger x and y); the margins are kept, not scaled; a
   path whose corners lie in one column has no width to scale; a refusal makes nothing. */
static void
as7_scaled_path_rounding_and_margins (void)
{
  static const int row[] = { 0, 0, 1, 0, 2, 0, 2, 2, 0, 2 };         /* the shape box (0, 0, 2, 2), the corner box [0, 2] x [0, 2] */
  static const int margin[] = { 2, 2, 4, 2, 4, 5, 2, 5 };            /* the shape box (3, 3, 1, 2): a margin of 1 left and above */
  static const int line[] = { 5, 1, 5, 4 };                          /* one column: no width */
  EaselPath orig = path_of (row, 5), out = { 0 };

  CHECK (easel_lasso_scale_path (&orig, 0, 0, 2, 2, 0, 0, 3, 2, &out, SIZE_MAX));
  CHECK (out.xy[0] == 0 && out.xy[2] == 2 && out.xy[4] == 3);          /* 0, 1.5 -> 2, 3 */
  easel_path_free (&out);
  easel_path_free (&orig);
  orig = path_of (margin, 4);
  CHECK (easel_lasso_scale_path (&orig, 3, 3, 1, 2, 3, 3, 5, 2, &out, SIZE_MAX));
  /* the corner at x = 2 lies one left of the shape box and stays one left of it; the corner at 4 is its far edge and goes to the new one */
  CHECK (out.xy[0] == 2 && out.xy[2] == 8 && out.xy[4] == 8 && out.xy[6] == 2);
  CHECK (out.xy[1] == 2 && out.xy[3] == 2 && out.xy[5] == 5 && out.xy[7] == 5);
  easel_path_free (&out);
  easel_path_free (&orig);
  orig = path_of (line, 2);
  CHECK (easel_lasso_scale_path (&orig, 5, 1, 1, 3, 5, 1, 4, 6, &out, SIZE_MAX));
  CHECK (out.n == 2 && out.xy[0] == out.xy[2]);
  easel_path_free (&out);
  /* Memory: the copy of 3 corners is 24 bytes; one byte less is refused, with nothing made. */
  easel_path_free (&orig);
  orig = path_of (row, 5);
  CHECK (!easel_lasso_scale_path (&orig, 0, 0, 2, 2, 0, 0, 3, 2, &out, 5 * 2 * sizeof (int) - 1));
  CHECK (out.n == 0 && out.xy == NULL);
  CHECK (easel_lasso_scale_path (&orig, 0, 0, 2, 2, 0, 0, 3, 2, &out, 16 * 2 * sizeof (int)));
  easel_path_free (&out);
  easel_path_free (&orig);
}

/* ---- 0018 T005 (REQ-0018-07): the line of a free-form selection after a flip or a turn ---- */

/* An L traced in the box of 4 x 3 pixels at (3, 2), so that the grid lines run from x 3 to 7 and from y 2 to 5: (3,2) (5,2) (5,4) (7,4)
   (7,5) (3,5), then three corners outside the box, as a path traced partly outside the picture has them: (9,7) (1,0) (-2,-1). The box
   is not square and its left and top are not 0, and nothing in the path is symmetric. */
static const int L_PATH[] = { 3, 2, 5, 2, 5, 4, 7, 4, 7, 5, 3, 5, 9, 7, 1, 0, -2, -1 };
#define L_N 9

/* The corners of that path after each command, worked out by hand from the spec (L = 3, T = 2, R = 7, B = 5; the box of the result
   keeps the left and the top):
   Flip Horizontal (x, y) -> (L + R - x, y) = (10 - x, y)          Flip Vertical (x, y) -> (x, T + B - y) = (x, 7 - y)
   Rotate 180      (x, y) -> (10 - x, 7 - y)
   Rotate 90       (x, y) -> (L + (B - y), T + (x - L)) = (8 - y, x - 1)
   Rotate 270      (x, y) -> (L + (y - T), T + (R - x)) = (y + 1, 9 - x) */
static const int WANT_FLIP_H[] = { 7, 2, 5, 2, 5, 4, 3, 4, 3, 5, 7, 5, 1, 7, 9, 0, 12, -1 };
static const int WANT_FLIP_V[] = { 3, 5, 5, 5, 5, 3, 7, 3, 7, 2, 3, 2, 9, 0, 1, 7, -2, 8 };
static const int WANT_ROT_180[] = { 7, 5, 5, 5, 5, 3, 3, 3, 3, 2, 7, 2, 1, 0, 9, 7, 12, 8 };
static const int WANT_ROT_90[] = { 6, 2, 6, 4, 4, 4, 4, 6, 3, 6, 3, 2, 1, 8, 8, 0, 9, -3 };
static const int WANT_ROT_270[] = { 3, 6, 3, 4, 5, 4, 5, 2, 6, 2, 6, 6, 8, 0, 1, 8, 0, 11 };

static EaselPath
l_path (void)
{
  EaselPath p = { 0 };

  for (int i = 0; i < L_N; i++)
    CHECK (easel_path_add (&p, L_PATH[2 * i], L_PATH[2 * i + 1], SIZE_MAX));
  return p;
}

static bool
path_is (const EaselPath *p, const int *want)
{
  return p->n == L_N && memcmp (p->xy, want, L_N * 2 * sizeof *want) == 0;
}

/* The line of a flipped or turned shape is the traced path turned within its box on the grid lines, also its corners outside the box
   (REQ-0018-07): each command gives the corners listed by hand; four turns of 90 and two flips on one axis give the original line
   exactly; a turn and its opposite do. A refusal for the memory leaves the output untouched. */
static void
line_oriented (void)
{
  static const struct { EaselOp op; const int *want; } cases[] = {
    { EASEL_OP_FLIP_H, WANT_FLIP_H }, { EASEL_OP_FLIP_V, WANT_FLIP_V }, { EASEL_OP_ROTATE_180, WANT_ROT_180 },
    { EASEL_OP_ROTATE_90, WANT_ROT_90 }, { EASEL_OP_ROTATE_270, WANT_ROT_270 },
  };
  EaselPath src = l_path(), out = { 0 }, a, b;
  int w = 4, h = 3;

  for (size_t k = 0; k < sizeof cases / sizeof *cases; k++)
    {
      CHECK (easel_lasso_orient_path (&src, 3, 2, w, h, easel_op_orient (cases[k].op), &out, SIZE_MAX));
      CHECK (path_is (&out, cases[k].want));
      easel_path_free (&out);
    }
  CHECK (path_is (&src, L_PATH));                                                   /* the source is not changed */
  /* four turns of 90, the box turning with the path (the same left and top, the width and the height swapped) */
  a = l_path ();
  for (int i = 0; i < 4; i++)
    {
      b = (EaselPath) { 0 };
      CHECK (easel_lasso_orient_path (&a, 3, 2, w, h, easel_op_orient (EASEL_OP_ROTATE_90), &b, SIZE_MAX));
      easel_path_free (&a);
      a = b;
      w = w ^ h;
      h = w ^ h;
      w = w ^ h;
    }
  CHECK (path_is (&a, L_PATH) && w == 4 && h == 3);
  easel_path_free (&a);
  /* two flips on one axis */
  for (int k = 0; k < 2; k++)
    {
      EaselOp op = k == 0 ? EASEL_OP_FLIP_H : EASEL_OP_FLIP_V;

      CHECK (easel_lasso_orient_path (&src, 3, 2, 4, 3, easel_op_orient (op), &a, SIZE_MAX));
      CHECK (easel_lasso_orient_path (&a, 3, 2, 4, 3, easel_op_orient (op), &b, SIZE_MAX));
      CHECK (path_is (&b, L_PATH));
      easel_path_free (&a);
      easel_path_free (&b);
    }
  /* a turn of 90 and then one of 270 (in the turned box, 3 x 4) */
  CHECK (easel_lasso_orient_path (&src, 3, 2, 4, 3, easel_op_orient (EASEL_OP_ROTATE_90), &a, SIZE_MAX));
  CHECK (easel_lasso_orient_path (&a, 3, 2, 3, 4, easel_op_orient (EASEL_OP_ROTATE_270), &b, SIZE_MAX));
  CHECK (path_is (&b, L_PATH));
  easel_path_free (&a);
  easel_path_free (&b);
  /* the memory: 9 corners are 72 bytes; less is refused and the output is untouched */
  out = (EaselPath) { 0 };
  CHECK (!easel_lasso_orient_path (&src, 3, 2, 4, 3, easel_op_orient (EASEL_OP_ROTATE_90), &out, 71));
  CHECK (out.n == 0 && out.xy == NULL);
  CHECK (easel_lasso_orient_path (&src, 3, 2, 4, 3, easel_op_orient (EASEL_OP_ROTATE_90), &out, 72));
  easel_path_free (&out);
  easel_path_free (&src);
}

int
main (void)
{
  RUN (as2_lasso_shape);
  RUN (as3_crossing_and_thin_paths);
  RUN (as20_diamond_and_whole_picture);
  RUN (lasso_matches_the_flood);
  RUN (as12_shape_memory);
  RUN (as7_scaled_path_line);
  RUN (as7_scaled_path_rounding_and_margins);
  RUN (line_oriented);
  return 0;
}
