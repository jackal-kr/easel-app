/* SPDX-License-Identifier: GPL-3.0-or-later */
/* The bends of feature 0033 (REQ-0033-02, -04, -05): the grid, the grab, the places, the bend rule and the control points (one
   bend the peak), and the scenarios' pixels through the curve of REQ-0011-05. */

#include <stdint.h>
#include <string.h>

#include "check.h"
#include "curve-bend.h"
#include "figure.h"
#include "image.h"

#define WHITE 0xFFFFFFFFu
#define BLACK 0xFF000000u
#define ONE EASEL_CURVE_ONE

/* A curve as the user makes it: the line's ends and its bends. */
typedef struct
{
  int sx, sy, ex, ey, n;
  EaselCurveBend b[2];
} Curve;

static Curve
line (int sx, int sy, int ex, int ey)
{
  return (Curve) { sx, sy, ex, ey, 0, { { 0 } } };
}

/* The curve's four points as shown: before a bend the control points on the start (REQ-0011-05's Line). */
static void
points (const Curve *c, int pts[8])
{
  pts[0] = c->sx, pts[1] = c->sy, pts[6] = c->ex, pts[7] = c->ey;
  if (c->n == 0)
    {
      pts[2] = pts[4] = c->sx;
      pts[3] = pts[5] = c->sy;
      return;
    }
  easel_curve_controls (c->sx, c->sy, c->ex, c->ey, c->b, c->n, pts + 2);
}

/* A press on the pixel (px, py) dragged to (tx, ty) and released (REQ-0033-02, -04). Returns the grab's place. */
static int
grab (Curve *c, int px, int py, int tx, int ty)
{
  int pts[8], t, which;
  int64_t x, y, dx = (int64_t) (tx - px) * ONE, dy = (int64_t) (ty - py) * ONE;
  EaselCurveGrab kind;

  points (c, pts);
  t = easel_curve_grab (pts, c->n == 0, px, py);
  kind = easel_curve_bend_rule (c->sx, c->sy, c->ex, c->ey, c->b, c->n, t, &which);
  easel_curve_point (pts, c->n == 0, t, &x, &y);
  if (kind == EASEL_CURVE_MOVE)
    {
      c->b[which].x += dx;
      c->b[which].y += dy;
    }
  else if (kind == EASEL_CURVE_REPLACE)
    c->b[which] = (EaselCurveBend) { x + dx, y + dy };
  else
    c->b[c->n++] = (EaselCurveBend) { x + dx, y + dy };
  easel_curve_order (c->sx, c->sy, c->ex, c->ey, c->b, c->n);
  return t;
}

/* The curve's pixels on a white 300 x 200 picture, in black; set[y * 300 + x] is 1 where black. */
static int
draw (const Curve *c, unsigned char *set)
{
  int pts[8];
  EaselImage *img = easel_image_new (300, 200, WHITE);
  EaselFigure f;
  size_t changed = 0;

  CHECK (img != NULL);
  points (c, pts);
  f = (EaselFigure) { EASEL_FIGURE_CURVE, 0, 0, 0, 0, 1, EASEL_FIGURE_OUTLINE, NULL, pts, 4, false };
  easel_figure_apply (img, NULL, &f, BLACK, WHITE, &changed);
  for (int i = 0; i < 300 * 200; i++)
    set[i] = img->px[i] == BLACK;
  easel_image_free (img);
  return (int) changed;
}

static int
row_min (const unsigned char *set)
{
  for (int y = 0; y < 200; y++)
    for (int x = 0; x < 300; x++)
      if (set[y * 300 + x])
        return y;
  return -1;
}

static int
row_max (const unsigned char *set)
{
  for (int y = 199; y >= 0; y--)
    for (int x = 0; x < 300; x++)
      if (set[y * 300 + x])
        return y;
  return -1;
}

/* REQ-0033-02: the grid, the smallest power of two at least N and 64, at most 4096. */
static void
grid (void)
{
  static const int cases[][2] = { { 3, 64 }, { 40, 64 }, { 64, 64 }, { 65, 128 }, { 100, 128 }, { 1000, 1024 }, { 4096, 4096 },
                                  { 5000, 4096 } };

  for (size_t k = 0; k < sizeof cases / sizeof cases[0]; k++)
    {
      int pts[8] = { 10, 40, 10, 40, 10, 40, 10 + cases[k][0], 40 };

      CHECK (easel_curve_grid (pts, true) == cases[k][1]);
    }
  {
    int bent[8] = { 10, 40, 30, 13, 30, 13, 50, 40 };     /* the path S, C, C, E: about 33.6 + 0 + 33.6, N = 68 */

    CHECK (easel_curve_grid (bent, false) == 128);
  }
}

/* REQ-0033-02: the grabbed place and point; the limits 1/8 and 7/8; the smallest i on a tie. */
static void
grab_place (void)
{
  int pts[8] = { 10, 40, 10, 40, 10, 40, 50, 40 }, far[8] = { 10, 40, 10, 40, 10, 40, 90, 40 };
  int loop[8] = { 0, 0, 0, 64, 0, 64, 0, 0 };
  int64_t x, y;

  CHECK (easel_curve_grab (pts, true, 30, 40) == 2048);
  easel_curve_point (pts, true, 2048, &x, &y);
  CHECK (x == 30 * ONE && y == 40 * ONE);
  CHECK (easel_curve_grab (far, true, 11, 40) == 512);         /* near the start: 1/8 */
  easel_curve_point (far, true, 512, &x, &y);
  CHECK (x == 20 * ONE && y == 40 * ONE);
  CHECK (easel_curve_grab (far, true, 89, 40) == 3584);        /* near the end: 7/8 */
  CHECK (easel_curve_grab (far, true, 50, 70) == 2048);        /* off the line, the nearest point still */
  /* A loop from (0, 0) back to it: the press on its ends is as near i = 0 as i = M; the smallest wins, limited to 1/8. */
  CHECK (easel_curve_grab (loop, false, 0, 0) == 512);
  /* A point of a bent curve, exact: B (1/2) of S (10, 40), C (30, 13), C (30, 13), E (50, 40) is (30, 19.75). */
  {
    int bent[8] = { 10, 40, 30, 13, 30, 13, 50, 40 };

    easel_curve_point (bent, false, 2048, &x, &y);
    CHECK (x == 30 * ONE && y == 19 * ONE + 3 * ONE / 4);
  }
}

/* REQ-0033-04: a bend's place from where its point lies along the line; the spread of two close places; new, move or replace;
   the 1/8 boundary; the tie. On the line from (0, 0) to (4096, 0) a point at x has the place x. */
static void
bend_rule (void)
{
  EaselCurveBend one[1] = { { 2048 * ONE, 0 } }, two[2] = { { 1024 * ONE, 5 * ONE }, { 3072 * ONE, -7 * ONE } };
  EaselCurveBend close[2] = { { 2000 * ONE, 0 }, { 2300 * ONE, 0 } }, ends[2] = { { 100 * ONE, 0 }, { 300 * ONE, 0 } };
  EaselCurveBend swapped[2] = { { 3000 * ONE, 0 }, { 1000 * ONE, 9 * ONE } };
  int which = -1, t[2];

  CHECK (easel_curve_place (0, 0, 4096, 0, &one[0]) == 2048);
  CHECK (easel_curve_place (0, 0, 4096, 0, &ends[0]) == 512);                  /* limited to 1/8 */
  CHECK (easel_curve_place (5, 5, 5, 5, &one[0]) == 2048);                     /* S = E: 1/2 */
  CHECK (easel_curve_place (0, 0, 8192, 0, &(EaselCurveBend) { 2001 * ONE, 0 }) == 1001);   /* 1000.5 rounds up */
  CHECK (easel_curve_place (0, 0, 8192, 0, &(EaselCurveBend) { 1999 * ONE, 0 }) == 1000);   /* 999.5 rounds up */
  easel_curve_places (0, 0, 4096, 0, close, 2, t);
  CHECK (t[0] == 1894 && t[1] == 2406);                                          /* 300 apart: 512 round 2150 */
  easel_curve_places (0, 0, 4096, 0, ends, 2, t);
  CHECK (t[0] == 512 && t[1] == 1024);                                           /* the middle limited to 3/16 */
  easel_curve_order (0, 0, 4096, 0, swapped, 2);
  CHECK (swapped[0].x == 1000 * ONE && swapped[1].x == 3000 * ONE);
  CHECK (easel_curve_bend_rule (0, 0, 4096, 0, NULL, 0, 2048, &which) == EASEL_CURVE_NEW && which == 0);
  CHECK (easel_curve_bend_rule (0, 0, 4096, 0, one, 1, 2048 + 511, &which) == EASEL_CURVE_MOVE && which == 0);
  CHECK (easel_curve_bend_rule (0, 0, 4096, 0, one, 1, 2048 - 511, &which) == EASEL_CURVE_MOVE && which == 0);
  CHECK (easel_curve_bend_rule (0, 0, 4096, 0, one, 1, 2048 + 512, &which) == EASEL_CURVE_NEW);     /* exactly 1/8: new */
  CHECK (easel_curve_bend_rule (0, 0, 4096, 0, one, 1, 2048 - 512, &which) == EASEL_CURVE_NEW);
  CHECK (easel_curve_bend_rule (0, 0, 4096, 0, two, 2, 2048, &which) == EASEL_CURVE_REPLACE && which == 0);   /* the tie */
  CHECK (easel_curve_bend_rule (0, 0, 4096, 0, two, 2, 1100, &which) == EASEL_CURVE_MOVE && which == 0);
  CHECK (easel_curve_bend_rule (0, 0, 4096, 0, two, 2, 2600, &which) == EASEL_CURVE_MOVE && which == 1);
  CHECK (easel_curve_bend_rule (0, 0, 4096, 0, two, 2, 1600, &which) == EASEL_CURVE_REPLACE && which == 0);
  CHECK (easel_curve_bend_rule (0, 0, 4096, 0, two, 2, 2560, &which) == EASEL_CURVE_REPLACE && which == 1);
  CHECK (easel_curve_bend_rule (0, 0, 4096, 0, two, 2, 3583, &which) == EASEL_CURVE_MOVE && which == 1);
  CHECK (easel_curve_bend_rule (0, 0, 4096, 0, two, 2, 3584, &which) == EASEL_CURVE_REPLACE && which == 1);
}

/* REQ-0033-05: one bend, the peak; S = E; two bends exact (a curve whose control points make its places its own t gives them
   back); rounding; the limits. */
static void
controls (void)
{
  int out[4];

  {
    EaselCurveBend b[1] = { { 30 * ONE, 20 * ONE } }, q[1] = { { 20 * ONE, 20 * ONE } };

    easel_curve_controls (10, 40, 50, 40, b, 1, out);
    CHECK (out[0] == 10 && out[1] == 13 && out[2] == 50 && out[3] == 13);      /* both at 13.33, over S and over E */
    easel_curve_controls (10, 40, 50, 40, q, 1, out);
    CHECK (out[0] == 10 && out[1] == 13 && out[2] == 23 && out[3] == 13);      /* f = 1/4: v = 1/3 */
  }
  {
    EaselCurveBend b[1] = { { 3 * ONE, 6 * ONE } };

    easel_curve_controls (0, 0, 0, 0, b, 1, out);
    CHECK (out[0] == 4 && out[1] == 8 && out[2] == 4 && out[3] == 8);          /* S = E: (4P - S) / 3 */
  }
  /* Two bends: control points one and two thirds of the way along make B (t) lie at t of the way along, so a bend's place is
     its own t, and its points give the control points back, exactly, also at the limits. */
  {
    static const int cases[][8] = { { 0, 0, 1000, 700, 2000, -900, 3000, 0 },
                                    { -999000, -3, -333000, 1000000, 333000, -1000000, 999000, -3 } };
    static const int places[][2] = { { 1024, 3072 }, { 512, 1024 }, { 3072, 3584 }, { 1536, 2048 } };

    for (size_t k = 0; k < sizeof cases / sizeof cases[0]; k++)
      for (size_t p = 0; p < sizeof places / sizeof places[0]; p++)
        {
          EaselCurveBend b[2];

          for (int j = 0; j < 2; j++)
            easel_curve_point (cases[k], false, places[p][j], &b[j].x, &b[j].y);
          easel_curve_controls (cases[k][0], cases[k][1], cases[k][6], cases[k][7], b, 2, out);
          CHECK (out[0] == cases[k][2] && out[1] == cases[k][3] && out[2] == cases[k][4] && out[3] == cases[k][5]);
        }
  }
  /* A half rounds toward larger: S = E = (0, 0), C = 4P / 3; P = (0, 3/8) gives 1/2, rounded to 1; (0, -3/8) gives 0. */
  {
    EaselCurveBend up[1] = { { 0, 3 * ONE / 8 } }, down[1] = { { 0, -3 * ONE / 8 } };

    easel_curve_controls (0, 0, 0, 0, up, 1, out);
    CHECK (out[0] == 0 && out[1] == 1 && out[3] == 1);
    easel_curve_controls (0, 0, 0, 0, down, 1, out);
    CHECK (out[0] == 0 && out[1] == 0 && out[3] == 0);
  }
  /* Far bends: the control points are limited to +-1,000,000. */
  {
    EaselCurveBend one[1] = { { (int64_t) 900000 * ONE, (int64_t) -900000 * ONE } };
    EaselCurveBend two[2] = { { (int64_t) -900000 * ONE, (int64_t) 1000000 * ONE }, { (int64_t) 1000000 * ONE, (int64_t) -1000000 * ONE } };

    easel_curve_controls (0, 0, 10, 0, one, 1, out);
    CHECK (out[0] == 1000000 && out[1] == -1000000 && out[2] == 10 && out[3] == -1000000);   /* C2 over E, both far off the line */
    easel_curve_controls (-1000000, 0, 1000000, 0, two, 2, out);
    for (int k = 0; k < 4; k++)
      CHECK (out[k] >= -1000000 && out[k] <= 1000000);
  }
}

/* AS-1: one bend at (30, 40) dragged to (30, 20): the peak there, 59 pixels, the smallest y 20 from 24 to 35; at (20, 40) to
   (20, 20): 54 pixels, 17 to 24. */
static void
as1_one_bend (void)
{
  Curve c = line (10, 40, 50, 40), q = line (10, 40, 50, 40);
  unsigned char set[300 * 200];

  CHECK (grab (&c, 30, 40, 30, 20) == 2048);
  CHECK (draw (&c, set) == 59);
  CHECK (row_min (set) == 20 && row_max (set) == 40);
  for (int x = 0; x < 300; x++)
    CHECK (set[20 * 300 + x] == (x >= 24 && x <= 35));
  CHECK (set[40 * 300 + 10] && set[40 * 300 + 50]);
  CHECK (grab (&q, 20, 40, 20, 20) == 1024);
  CHECK (draw (&q, set) == 54 && row_min (set) == 20);
  for (int x = 0; x < 300; x++)
    CHECK (set[20 * 300 + x] == (x >= 17 && x <= 24));
}

/* AS-2 and AS-3: two bends, a move and a replace. */
static void
as2_as3_two_bends (void)
{
  Curve c = line (10, 40, 70, 40), d;
  unsigned char set[300 * 200];

  grab (&c, 25, 40, 25, 20);
  CHECK (draw (&c, set) == 72 && row_min (set) == 20);
  CHECK (grab (&c, 44, 25, 48, 48) == 3072);
  CHECK (c.n == 2);
  d = c;
  CHECK (draw (&c, set) == 82);
  CHECK (set[20 * 300 + 25] && set[48 * 300 + 48]);
  CHECK (row_min (set) == 20 && row_max (set) == 56);
  /* AS-3: the first bend moved by (0, -5). */
  grab (&c, 25, 20, 25, 15);
  CHECK (c.n == 2 && c.b[0].x == 25 * ONE && c.b[0].y == 15 * ONE);
  CHECK (draw (&c, set) == 94);
  CHECK (set[15 * 300 + 25] && set[48 * 300 + 48]);
  /* AS-3: a press at (40, 37), a little nearer the second bend: replaced. */
  grab (&d, 40, 37, 40, 30);
  CHECK (d.n == 2 && d.b[0].x == 25 * ONE && d.b[0].y == 20 * ONE);
  CHECK (draw (&d, set) == 69);
  CHECK (set[30 * 300 + 40] && set[20 * 300 + 25]);
}

/* AS-4: a press near either end grabs 1/8 or 7/8 and moves that point by the pointer's movement: the peak. */
static void
as4_near_the_ends (void)
{
  Curve c = line (10, 40, 90, 40), e = line (10, 40, 90, 40);
  unsigned char set[300 * 200];

  CHECK (grab (&c, 11, 40, 11, 30) == 512);
  CHECK (c.b[0].x == 20 * ONE && c.b[0].y == 30 * ONE);
  CHECK (draw (&c, set) == 87 && set[30 * 300 + 20] && row_min (set) == 30);
  CHECK (grab (&e, 89, 40, 89, 30) == 3584);
  CHECK (draw (&e, set) == 87 && set[30 * 300 + 80] && row_min (set) == 30);
}

int
main (void)
{
  RUN (grid);
  RUN (grab_place);
  RUN (bend_rule);
  RUN (controls);
  RUN (as1_one_bend);
  RUN (as2_as3_two_bends);
  RUN (as4_near_the_ends);
  return 0;
}
