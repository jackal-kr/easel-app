/* SPDX-License-Identifier: GPL-3.0-or-later */
/* The geometry of Rectangle, Rounded Rectangle and Ellipse, feature 0010 (REQ-0010-11, REQ-0010-12, REQ-0010-13). */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "check.h"
#include "figure.h"
#include "image.h"
#include "stroke.h"

/* Counts the pixels of a figure in a w x h picture: all, the outline's and the inside's. */
static void
count_figure (const EaselFigure *f, int *all, int *outline, int *inside)
{
  *all = *outline = *inside = 0;
  for (int y = -5; y < 80; y++)
    {
      EaselFigureRun runs[4];
      size_t n = easel_figure_row (f, y, runs, 4);

      for (size_t i = 0; i < n; i++)
        {
          int len = runs[i].x1 - runs[i].x0 + 1;

          *all += len;
          if (runs[i].inside)
            *inside += len;
          else
            *outline += len;
        }
    }
}

static EaselFigure
figure (EaselFigureKind kind, int x0, int y0, int x1, int y1, int width)
{
  return (EaselFigure) { kind, x0, y0, x1, y1, width, EASEL_FIGURE_OUTLINE };
}

/* The row y of the figure as a picture of characters: '#' outline, 'o' inside, '.' not the figure. */
static void
row_text (const EaselFigure *f, int y, int x_from, int x_to, char *out)
{
  EaselFigureRun runs[4];
  size_t n = easel_figure_row (f, y, runs, 4);
  int k = 0;

  for (int x = x_from; x <= x_to; x++)
    {
      char c = '.';

      for (size_t i = 0; i < n; i++)
        if (x >= runs[i].x0 && x <= runs[i].x1)
          c = runs[i].inside ? 'o' : '#';
      out[k++] = c;
    }
  out[k] = '\0';
}

/* AS-6 (REQ-0010-11, REQ-0010-12): the box runs from the outer corner of the start pixel to the outer corner of the end
   pixel, and the outline of a width w is what is not in the box made smaller by w on each side. */
static void
as6_rectangle_fill_styles (void)
{
  int all, outline, inside, x, y, w, h;
  EaselFigure f = figure (EASEL_FIGURE_RECTANGLE, 10, 10, 19, 14, 1);

  easel_figure_bounds (&f, &x, &y, &w, &h);
  CHECK (x == 10 && y == 10 && w == 10 && h == 5);                 /* both pixels included: 10 x 5 */
  count_figure (&f, &all, &outline, &inside);
  CHECK (all == 50 && outline == 26 && inside == 24);
  f.width = 2;
  count_figure (&f, &all, &outline, &inside);
  CHECK (all == 50 && outline == 44 && inside == 6);
  f.width = 3;                                                     /* 5 high: no inside left */
  count_figure (&f, &all, &outline, &inside);
  CHECK (all == 50 && outline == 50 && inside == 0);
  /* The end before the start: the same box. */
  f = figure (EASEL_FIGURE_RECTANGLE, 19, 14, 10, 10, 1);
  easel_figure_bounds (&f, &x, &y, &w, &h);
  CHECK (x == 10 && y == 10 && w == 10 && h == 5);
  /* A box of 2w or less in a direction has no inside: 2 x 5 at w = 1. */
  f = figure (EASEL_FIGURE_RECTANGLE, 0, 0, 1, 4, 1);
  count_figure (&f, &all, &outline, &inside);
  CHECK (all == 10 && outline == 10 && inside == 0);
  /* One pixel, and a line one pixel thick. */
  f = figure (EASEL_FIGURE_RECTANGLE, 5, 5, 5, 5, 1);
  count_figure (&f, &all, &outline, &inside);
  CHECK (all == 1 && outline == 1);
  f = figure (EASEL_FIGURE_ELLIPSE, 3, 3, 3, 22, 1);
  count_figure (&f, &all, &outline, &inside);
  CHECK (all == 20 && outline == 20);
}

/* AS-7 (REQ-0010-11, REQ-0010-12): the ellipse touches the box's outer edges, a pixel is in it when its center is. */
static void
as7_ellipse_pixels (void)
{
  static const char *const circle9[9] = {
    "..#####..", ".##ooo##.", "##ooooo##", "#ooooooo#", "#ooooooo#", "#ooooooo#", "##ooooo##", ".##ooo##.", "..#####..",
  };
  EaselFigure f = figure (EASEL_FIGURE_ELLIPSE, 0, 0, 8, 8, 1);
  int all, outline, inside;
  char text[16];

  count_figure (&f, &all, &outline, &inside);
  CHECK (all == 69 && outline == 32 && inside == 37);
  for (int y = 0; y < 9; y++)
    {
      row_text (&f, y, 0, 8, text);
      CHECK (strcmp (text, circle9[y]) == 0);
    }
  f = figure (EASEL_FIGURE_ELLIPSE, 0, 0, 19, 9, 1);
  count_figure (&f, &all, &outline, &inside);
  CHECK (all == 160 && outline == 48);
  f = figure (EASEL_FIGURE_ELLIPSE, 0, 0, 0, 0, 1);               /* 1 x 1 */
  count_figure (&f, &all, &outline, &inside);
  CHECK (all == 1);
  f = figure (EASEL_FIGURE_ELLIPSE, 0, 0, 1, 1, 1);               /* 2 x 2: all four pixels */
  count_figure (&f, &all, &outline, &inside);
  CHECK (all == 4 && outline == 4);
}

/* AS-8 (REQ-0010-11, REQ-0010-12): the corners are quarter circles of radius 8, or half the smaller side when less. */
static void
as8_rounded_rectangle_pixels (void)
{
  static const char *const small[4] = { ".########.", "##########", "##########", ".########." };
  EaselFigure f = figure (EASEL_FIGURE_ROUNDED, 0, 0, 29, 19, 1);
  int all, outline, inside;
  char text[16];

  count_figure (&f, &all, &outline, &inside);
  CHECK (all == 552 && outline == 88);                           /* 600 less four corners of 12 */
  f = figure (EASEL_FIGURE_ROUNDED, 0, 0, 9, 3, 1);              /* radius 2: half of 4 */
  count_figure (&f, &all, &outline, &inside);
  CHECK (all == 36);
  for (int y = 0; y < 4; y++)
    {
      row_text (&f, y, 0, 9, text);
      for (int i = 0; text[i]; i++)
        if (text[i] == 'o')
          text[i] = '#';                                          /* only the shape is checked here, not the inside */
      CHECK (strcmp (text, small[y]) == 0);
    }
  f = figure (EASEL_FIGURE_ROUNDED, 0, 0, 0, 0, 1);               /* 1 x 1 */
  count_figure (&f, &all, &outline, &inside);
  CHECK (all == 1);
  /* A box of 16 or more on its smaller side has the radius 8; at 15 it is 7 (half of 15, rounded down). */
  f = figure (EASEL_FIGURE_ROUNDED, 0, 0, 39, 15, 1);
  count_figure (&f, &all, &outline, &inside);
  CHECK (all == 592);                                             /* radius 8: 640 less four corners of 12 */
  f = figure (EASEL_FIGURE_ROUNDED, 0, 0, 39, 14, 1);             /* 40 x 15: radius 7 */
  count_figure (&f, &all, &outline, &inside);
  CHECK (all == 560);                                             /* radius 7: 600 less four corners of 10 */
}

/* Every pixel of random boxes classified directly, per pixel, and compared with the rows (the Plan's risk: the preview
   and the drawing come from one function, and it must equal the definition). */
static int
member_ellipse (int bx, int by, int bw, int bh, int x, int y)
{
  long long dx, dy;

  if (bw <= 0 || bh <= 0)
    return 0;
  dx = (2LL * x + 1) - (2LL * bx + bw);
  dy = (2LL * y + 1) - (2LL * by + bh);
  return dx * dx * bh * bh + dy * dy * bw * bw <= (long long) bw * bw * bh * bh;
}

static int
member_rounded (int bx, int by, int bw, int bh, int r, int x, int y)
{
  long long cx2 = 0, cy2 = 0, px = 2LL * x + 1, py = 2LL * y + 1;
  int in_x = 0, in_y = 0;

  if (bw <= 0 || bh <= 0 || x < bx || x >= bx + bw || y < by || y >= by + bh)
    return 0;
  if (px < 2LL * (bx + r))
    {
      cx2 = 2LL * (bx + r);
      in_x = 1;
    }
  else if (px > 2LL * (bx + bw - r))
    {
      cx2 = 2LL * (bx + bw - r);
      in_x = 1;
    }
  if (py < 2LL * (by + r))
    {
      cy2 = 2LL * (by + r);
      in_y = 1;
    }
  else if (py > 2LL * (by + bh - r))
    {
      cy2 = 2LL * (by + bh - r);
      in_y = 1;
    }
  if (in_x && in_y)
    return (px - cx2) * (px - cx2) + (py - cy2) * (py - cy2) <= 4LL * r * r;
  return 1;
}

static int
member_kind (EaselFigureKind kind, int bx, int by, int bw, int bh, int x, int y)
{
  int r = bw < bh ? bw / 2 : bh / 2;

  if (bw <= 0 || bh <= 0)
    return 0;
  if (kind == EASEL_FIGURE_RECTANGLE)
    return x >= bx && x < bx + bw && y >= by && y < by + bh;
  if (kind == EASEL_FIGURE_ELLIPSE)
    return member_ellipse (bx, by, bw, bh, x, y);
  return member_rounded (bx, by, bw, bh, r > 8 ? 8 : r, x, y);
}

static void
brute_force_kinds (void)
{
  static const EaselFigureKind kinds[] = { EASEL_FIGURE_RECTANGLE, EASEL_FIGURE_ELLIPSE, EASEL_FIGURE_ROUNDED };
  unsigned seed = 12345;

  for (int k = 0; k < 3; k++)
    for (int iter = 0; iter < 600; iter++)
      {
        int bw, bh, bx, by, width;
        EaselFigure f;

        seed = seed * 1103515245u + 12345u;
        bw = 1 + (int) (seed >> 16) % 40;
        seed = seed * 1103515245u + 12345u;
        bh = 1 + (int) (seed >> 16) % 40;
        seed = seed * 1103515245u + 12345u;
        bx = (int) (seed >> 16) % 30 - 10;
        seed = seed * 1103515245u + 12345u;
        by = (int) (seed >> 16) % 30 - 10;
        seed = seed * 1103515245u + 12345u;
        width = 1 + (int) (seed >> 16) % 5;
        f = figure (kinds[k], bx, by, bx + bw - 1, by + bh - 1, width);
        for (int y = by - 2; y < by + bh + 2; y++)
          {
            EaselFigureRun runs[4];
            size_t n = easel_figure_row (&f, y, runs, 4);

            for (int x = bx - 2; x < bx + bw + 2; x++)
              {
                int in_outer = member_kind (kinds[k], bx, by, bw, bh, x, y);
                int r_outer = bw < bh ? bw / 2 : bh / 2;
                int ri = (r_outer > 8 ? 8 : r_outer) - width;
                int in_inner = kinds[k] == EASEL_FIGURE_ROUNDED
                                   ? bw - 2 * width > 0 && bh - 2 * width > 0
                                         && member_rounded (bx + width, by + width, bw - 2 * width, bh - 2 * width,
                                                            ri < 0 ? 0 : ri, x, y)
                                   : member_kind (kinds[k], bx + width, by + width, bw - 2 * width, bh - 2 * width, x, y);
                int found = 0, found_inside = 0;

                for (size_t i = 0; i < n; i++)
                  if (x >= runs[i].x0 && x <= runs[i].x1)
                    {
                      found++;
                      found_inside = runs[i].inside;
                    }
                CHECK (found == in_outer);
                if (in_outer)
                  CHECK (found_inside == in_inner);
              }
          }
      }
}

static void
as6_runs_match_the_definition (void)
{
  brute_force_kinds ();
}

/* ---- Line (REQ-0010-10) ---- */

#define PW 120
#define PH 100
#define OFF 30                       /* the picture is the figure's plane shifted by this, so negative coordinates fit */

/* The Pencil's 1-pixel line from (x0, y0) to (x1, y1), as an image (coordinates shifted by OFF). */
static EaselImage *
pencil_line (int x0, int y0, int x1, int y1)
{
  EaselImage *img = easel_image_new (PW, PH, 0xFFFFFFFFu);
  EaselStroke *st = easel_stroke_begin (img, x0 + OFF, y0 + OFF, 0xFF000000u);

  CHECK (img != NULL && st != NULL);
  easel_stroke_to (st, x1 + OFF, y1 + OFF);
  easel_stroke_end (st, NULL);
  return img;
}

/* The figure's pixels of the Line, over all rows of the plane, as a count and a check against a per-pixel set. */
static int
line_pixels (EaselFigure *f, unsigned char *set)
{
  int n = 0;

  CHECK (easel_figure_prepare (f, -OFF, PH - OFF - 1));
  for (int y = -OFF; y < PH - OFF; y++)
    {
      EaselFigureRun runs[4];
      size_t k = easel_figure_row (f, y, runs, 4);

      CHECK (k <= 1);                                          /* one contiguous range per row */
      for (size_t i = 0; i < k; i++)
        for (int x = runs[i].x0; x <= runs[i].x1; x++)
          if (x + OFF >= 0 && x + OFF < PW)
            {
              set[(y + OFF) * PW + x + OFF] = 1;
              n++;
            }
    }
  easel_figure_release (f);
  return n;
}

/* AS-4 (REQ-0010-10): a 1-pixel Line is the Pencil's 8-connected line from the start to the end. */
static void
as4_line_pixels (void)
{
  static const int cases[][4] = { { 10, 10, 40, 20 }, { 40, 20, 10, 10 }, { 5, 5, 5, 40 }, { 5, 5, 60, 5 }, { 0, 0, 30, 30 },
                                  { 30, 0, 0, 25 }, { -10, 50, 50, -10 }, { 7, 7, 7, 7 }, { -20, -20, 40, 15 } };
  int total;

  for (size_t c = 0; c < sizeof cases / sizeof cases[0]; c++)
    {
      EaselImage *img = pencil_line (cases[c][0], cases[c][1], cases[c][2], cases[c][3]);
      EaselFigure f = figure (EASEL_FIGURE_LINE, cases[c][0], cases[c][1], cases[c][2], cases[c][3], 1);
      unsigned char set[PW * PH] = { 0 };
      int expected = 0;

      total = line_pixels (&f, set);
      for (int i = 0; i < PW * PH; i++)
        {
          CHECK ((img->px[i] == 0xFF000000u) == (set[i] == 1));
          expected += img->px[i] == 0xFF000000u;
        }
      CHECK (total == expected);
      easel_image_free (img);
    }
  {
    EaselFigure f = figure (EASEL_FIGURE_LINE, 10, 10, 40, 20, 1);
    unsigned char set[PW * PH] = { 0 };

    CHECK (line_pixels (&f, set) == 31);                      /* 31 pixels, one per column of the major axis */
  }
}

/* AS-5 (REQ-0010-10): with a width above 1 every pixel of the line is stamped with the w x w square, the line's pixel at
   column and row (w - 1) / 2 of it. */
static void
as5_square_pen (void)
{
  unsigned char set[PW * PH];
  EaselFigure f;
  int n;

  memset (set, 0, sizeof set);
  f = figure (EASEL_FIGURE_LINE, 10, 10, 10, 10, 3);                /* a click: the 3 x 3 square 9..11 */
  n = line_pixels (&f, set);
  CHECK (n == 9);
  for (int y = 9; y <= 11; y++)
    for (int x = 9; x <= 11; x++)
      CHECK (set[(y + OFF) * PW + x + OFF] == 1);
  memset (set, 0, sizeof set);
  f = figure (EASEL_FIGURE_LINE, 10, 10, 10, 10, 4);                /* the 4 x 4 square 9..12 */
  n = line_pixels (&f, set);
  CHECK (n == 16);
  for (int y = 9; y <= 12; y++)
    for (int x = 9; x <= 12; x++)
      CHECK (set[(y + OFF) * PW + x + OFF] == 1);
  memset (set, 0, sizeof set);
  f = figure (EASEL_FIGURE_LINE, 10, 10, 20, 10, 2);                /* rows 10 and 11, x from 10 to 21 */
  n = line_pixels (&f, set);
  CHECK (n == 24);
  for (int y = 10; y <= 11; y++)
    for (int x = 10; x <= 21; x++)
      CHECK (set[(y + OFF) * PW + x + OFF] == 1);
  memset (set, 0, sizeof set);
  f = figure (EASEL_FIGURE_LINE, 10, 10, 10, 10, 5);                /* 8 5 x 5 square 8..12 */
  CHECK (line_pixels (&f, set) == 25);
  CHECK (set[(8 + OFF) * PW + 8 + OFF] == 1 && set[(12 + OFF) * PW + 12 + OFF] == 1 && set[(7 + OFF) * PW + 7 + OFF] == 0);
}

/* The Plan's risk: a square pen along an 8-connected line covers one contiguous range per row. Random lines and widths,
   the figure against the stamps of every pixel of the Pencil's line, per pixel. */
static void
line_matches_the_stamps (void)
{
  unsigned seed = 4242;

  for (int iter = 0; iter < 400; iter++)
    {
      int c[4], w;
      EaselImage *img;
      EaselFigure f;
      unsigned char set[PW * PH] = { 0 }, stamped[PW * PH] = { 0 };
      int a;

      for (int i = 0; i < 4; i++)
        {
          seed = seed * 1103515245u + 12345u;
          c[i] = (int) (seed >> 16) % 60 - 15;
        }
      seed = seed * 1103515245u + 12345u;
      w = 1 + (int) (seed >> 16) % 5;
      img = pencil_line (c[0], c[1], c[2], c[3]);
      a = (w - 1) / 2;
      for (int y = 0; y < PH; y++)
        for (int x = 0; x < PW; x++)
          if (img->px[y * PW + x] == 0xFF000000u)
            for (int dy = 0; dy < w; dy++)
              for (int dx = 0; dx < w; dx++)
                {
                  int sx = x - a + dx, sy = y - a + dy;

                  if (sx >= 0 && sx < PW && sy >= 0 && sy < PH)
                    stamped[sy * PW + sx] = 1;
                }
      f = figure (EASEL_FIGURE_LINE, c[0], c[1], c[2], c[3], w);
      line_pixels (&f, set);
      for (int i = 0; i < PW * PH; i++)
        CHECK (set[i] == stamped[i]);
      easel_image_free (img);
    }
}

/* AS-9 (REQ-0010-14): Shift. A Line goes in the nearest of the eight directions by angle, with the pointer's distance
   along it; the other tools draw a square or a circle of the smaller side, from the start toward the end. */
static void
as9_shift_constraint (void)
{
  int x, y;

  /* The three Lines of the scenario. */
  x = 40; y = 22;
  easel_figure_constrain (EASEL_FIGURE_LINE, 10, 10, &x, &y);
  CHECK (x == 40 && y == 10);                                    /* horizontal */
  x = 40; y = 35;
  easel_figure_constrain (EASEL_FIGURE_LINE, 10, 10, &x, &y);
  CHECK (x == 35 && y == 35);                                    /* 45 degrees, the smaller distance */
  x = 12; y = 40;
  easel_figure_constrain (EASEL_FIGURE_LINE, 10, 10, &x, &y);
  CHECK (x == 10 && y == 40);                                    /* vertical */
  /* All eight directions, from a start in the middle. */
  {
    static const int d[8][4] = { { 30, 3, 30, 0 }, { 30, -3, 30, 0 }, { -30, 3, -30, 0 }, { 3, 30, 0, 30 }, { -3, -30, 0, -30 },
                                 { 20, 22, 20, 20 }, { -20, 22, -20, 20 }, { 25, -22, 22, -22 } };

    for (int i = 0; i < 8; i++)
      {
        x = 50 + d[i][0];
        y = 50 + d[i][1];
        easel_figure_constrain (EASEL_FIGURE_LINE, 50, 50, &x, &y);
        CHECK (x == 50 + d[i][2] && y == 50 + d[i][3]);
      }
  }
  /* The boundary at 22.5 degrees: dx = 29, dy = 12 is nearer to horizontal (29 / 12 = 2.4167 > 1 + sqrt 2), dx = 28, dy = 12
     is nearer to the diagonal. */
  x = 39; y = 22;
  easel_figure_constrain (EASEL_FIGURE_LINE, 10, 10, &x, &y);
  CHECK (x == 39 && y == 10);
  x = 38; y = 22;
  easel_figure_constrain (EASEL_FIGURE_LINE, 10, 10, &x, &y);
  CHECK (x == 22 && y == 22);
  /* No movement stays put. */
  x = 10; y = 10;
  easel_figure_constrain (EASEL_FIGURE_LINE, 10, 10, &x, &y);
  CHECK (x == 10 && y == 10);
  /* Squares and circles: of the smaller side, toward the end. */
  for (int kind = EASEL_FIGURE_RECTANGLE; kind <= EASEL_FIGURE_ELLIPSE; kind++)
    {
      x = 49; y = 34;                                            /* a 40 x 25 box */
      easel_figure_constrain ((EaselFigureKind) kind, 10, 10, &x, &y);
      CHECK (x == 34 && y == 34);
      x = 30; y = 20;                                            /* from (50, 50): 21 x 31 */
      easel_figure_constrain ((EaselFigureKind) kind, 50, 50, &x, &y);
      CHECK (x == 30 && y == 30);
      x = 14; y = 60;                                            /* from (10, 10) to the lower right: 5 x 51 */
      easel_figure_constrain ((EaselFigureKind) kind, 10, 10, &x, &y);
      CHECK (x == 14 && y == 14);
      x = 70; y = 12;                                            /* up-right is not a case here: 61 x 3 */
      easel_figure_constrain ((EaselFigureKind) kind, 10, 10, &x, &y);
      CHECK (x == 12 && y == 12);
      x = 20; y = 0;                                             /* toward the upper right: 11 x 11 */
      easel_figure_constrain ((EaselFigureKind) kind, 10, 10, &x, &y);
      CHECK (x == 20 && y == 0);
      x = 5; y = 40;                                             /* toward the lower left: 6 x 31 */
      easel_figure_constrain ((EaselFigureKind) kind, 10, 10, &x, &y);
      CHECK (x == 5 && y == 15);
    }
}

/* ---- Many runs (0011, REQ-0011-08, -09) ---- */

/* The buffer a caller sizes with easel_figure_max_runs holds the runs of every row of every figure: the bound is no smaller
   than the most runs a row has, and a row asked with exactly that many gives them all. */
static void
as5_max_runs_bounds_the_rows (void)
{
  static const EaselFigureKind kinds[] = { EASEL_FIGURE_LINE, EASEL_FIGURE_RECTANGLE, EASEL_FIGURE_ROUNDED,
                                           EASEL_FIGURE_ELLIPSE };
  unsigned int seed = 7;
  size_t most = 0;

  for (int n = 0; n < 400; n++)
    {
      EaselFigure f = { kinds[n % 4], 0, 0, 0, 0, 1 + n % 5, (EaselFigureStyle) (n % 3), NULL };
      size_t max;
      EaselFigureRun *runs;

      seed = seed * 1103515245u + 12345u;
      f.x0 = (int) ((seed >> 8) % 60) - 10;
      f.y0 = (int) ((seed >> 16) % 60) - 10;
      seed = seed * 1103515245u + 12345u;
      f.x1 = (int) ((seed >> 8) % 60) - 10;
      f.y1 = (int) ((seed >> 16) % 60) - 10;
      max = easel_figure_max_runs (&f);
      CHECK (max >= 3);
      runs = malloc (max * sizeof *runs);
      CHECK (runs != NULL);
      CHECK (easel_figure_prepare (&f, -30, 60));
      for (int y = -30; y <= 60; y++)
        {
          size_t k = easel_figure_row (&f, y, runs, max);

          CHECK (k <= max);
          if (k > most)
            most = k;
        }
      easel_figure_release (&f);
      free (runs);
    }
  CHECK (most == 3);                 /* a row of an outline-and-fill shape: left edge, inside, right edge */
}

int
main (void)
{
  RUN (as6_rectangle_fill_styles);
  RUN (as7_ellipse_pixels);
  RUN (as8_rounded_rectangle_pixels);
  RUN (as6_runs_match_the_definition);
  RUN (as4_line_pixels);
  RUN (as5_square_pen);
  RUN (line_matches_the_stamps);
  RUN (as9_shift_constraint);
  RUN (as5_max_runs_bounds_the_rows);
  return 0;
}
