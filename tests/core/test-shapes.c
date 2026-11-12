/* SPDX-License-Identifier: GPL-3.0-or-later */
/* The shapes of 0043 (REQ-0043-04, -05, -14, -18; AS-2's corners, AS-3): the corner table against docs/reference/proto-0043/corners.md,
   read at run time; the corners placed by REQ-0043-04's formula, written again here from the spec; each of the 16 shapes the very
   polygon the Polygon tool draws through those corners; the mirror; the box; Rectangle, Rounded Rectangle and Ellipse as the old
   tools drew them. */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "check.h"
#include "figure.h"
#include "image.h"
#include "shapes.h"

#define WHITE 0xFFFFFFFFu
#define BLACK 0xFF000000u
#define YELLOW 0xFFFFFF00u

/* corners.md, read once: the corners of each of the 16 shapes in its order. */
static int doc_n[EASEL_SHAPE_KIND_COUNT];
static int doc_uv[EASEL_SHAPE_KIND_COUNT][2 * EASEL_SHAPE_MAX_CORNERS];

static void
read_corners_md (void)
{
  const char *root = getenv ("EASEL_SOURCE_ROOT");
  char path[4096], line[8192];
  FILE *f;
  int shape = EASEL_SHAPE_KIND_TRIANGLE - 1;

  CHECK (root != NULL);
  snprintf (path, sizeof path, "%s/docs/reference/proto-0043/corners.md", root);
  f = fopen (path, "r");
  CHECK (f != NULL);
  while (fgets (line, sizeof line, f) != NULL)
    {
      if (strncmp (line, "## ", 3) == 0)
        {
          shape++;
          CHECK (shape < EASEL_SHAPE_KIND_COUNT);
          continue;
        }
      if (line[0] != '(' || shape < EASEL_SHAPE_KIND_TRIANGLE)
        continue;
      for (const char *p = line; (p = strchr (p, '(')) != NULL; p++)
        {
          int u, v;

          CHECK (sscanf (p, "(%d, %d)", &u, &v) == 2);
          CHECK (doc_n[shape] < EASEL_SHAPE_MAX_CORNERS);
          doc_uv[shape][2 * doc_n[shape]] = u;
          doc_uv[shape][2 * doc_n[shape] + 1] = v;
          doc_n[shape]++;
        }
    }
  fclose (f);
  CHECK (shape == EASEL_SHAPE_KIND_LIGHTNING_BOLT);
}

/* REQ-0043-04's p (u, N), written again from the spec's words. */
static int
spec_place (int u, int n)
{
  if (u <= 500)
    return (u * (n - 1) + 500) / 1000;
  return (n - 1) - ((1000 - u) * (n - 1) + 500) / 1000;
}

/* Polygon P: the closed polygon figure through the corners REQ-0043-04 computes for the shape in the box, from corners.md. */
static EaselFigure
polygon_p (EaselShapeKind s, int x, int y, int w, int h, int width, EaselFigureStyle style, bool smooth, bool mx, bool my, int *pts)
{
  int d = (width - 1) / 2, iw = w - (width - 1), ih = h - (width - 1), ix = x + d, iy = y + d;
  EaselFigure f = { EASEL_FIGURE_POLYGON, 0, 0, 0, 0, width, style, NULL, pts, (size_t) doc_n[s], true };

  if (iw < 1)
    {
      iw = 1;
      ix = x + (w - 1) / 2;
    }
  if (ih < 1)
    {
      ih = 1;
      iy = y + (h - 1) / 2;
    }
  for (int i = 0; i < doc_n[s]; i++)
    {
      int u = doc_uv[s][2 * i], v = doc_uv[s][2 * i + 1];

      pts[2 * i] = ix + spec_place (mx ? 1000 - u : u, iw);
      pts[2 * i + 1] = iy + spec_place (my ? 1000 - v : v, ih);
    }
  f.smooth = smooth;
  return f;
}

/* The figure drawn on a white pw x ph picture, BLACK drawing, YELLOW the other color. */
static EaselImage *
render (const EaselFigure *f, int pw, int ph)
{
  EaselImage *img = easel_image_new (pw, ph, WHITE);

  CHECK (img != NULL);
  CHECK (easel_figure_apply (img, NULL, f, BLACK, YELLOW, NULL) != EASEL_FIGURE_NO_MEMORY);
  return img;
}

static bool
same (const EaselImage *a, const EaselImage *b)
{
  return a->width == b->width && a->height == b->height
         && memcmp (a->px, b->px, (size_t) a->width * a->height * sizeof a->px[0]) == 0;
}

static const EaselFigureStyle styles[3] = { EASEL_FIGURE_OUTLINE, EASEL_FIGURE_OUTLINE_AND_FILL, EASEL_FIGURE_FILL_ONLY };

static void
shapes_names (void)
{
  static const char *const names[EASEL_SHAPE_KIND_COUNT] = {
    "rectangle", "rounded-rectangle", "ellipse", "triangle", "right-triangle", "diamond", "pentagon", "hexagon", "right-arrow",
    "left-arrow", "up-arrow", "down-arrow", "4-point-star", "5-point-star", "6-point-star", "rounded-rectangle-callout",
    "oval-callout", "heart", "lightning-bolt"
  };
  EaselShapeKind s;

  for (int i = 0; i < EASEL_SHAPE_KIND_COUNT; i++)
    {
      CHECK (easel_shape_name ((EaselShapeKind) i) != NULL && strcmp (easel_shape_name ((EaselShapeKind) i), names[i]) == 0);
      CHECK (easel_shape_from_name (names[i], &s) && s == (EaselShapeKind) i);
    }
  CHECK (easel_shape_name (EASEL_SHAPE_KIND_COUNT) == NULL);
  CHECK (!easel_shape_from_name ("square", &s));
  CHECK (!easel_shape_from_name ("", &s));
}

/* The table is corners.md's, corner for corner; the three old shapes have none. */
static void
shapes_table_is_corners_md (void)
{
  const int16_t *uv;

  for (int s = 0; s < EASEL_SHAPE_KIND_TRIANGLE; s++)
    CHECK (easel_shape_corners ((EaselShapeKind) s, &uv) == 0 && uv == NULL);
  for (int s = EASEL_SHAPE_KIND_TRIANGLE; s < EASEL_SHAPE_KIND_COUNT; s++)
    {
      size_t n = easel_shape_corners ((EaselShapeKind) s, &uv);

      CHECK (n == (size_t) doc_n[s] && n >= 3);
      for (size_t i = 0; i < 2 * n; i++)
        CHECK (uv[i] == doc_uv[s][i]);
    }
}

/* p (u, N) against the spec's formula for every u at sides 1, 2, 3, 10, 11, 9999 and 10000, its range, and the mirror rule. */
static void
shapes_place (void)
{
  static const int sides[] = { 1, 2, 3, 10, 11, 999, 9999, 10000 };

  for (size_t k = 0; k < sizeof sides / sizeof sides[0]; k++)
    for (int u = 0; u <= 1000; u++)
      {
        int n = sides[k], p = easel_shape_place (u, n);

        CHECK (p == spec_place (u, n));
        CHECK (p >= 0 && p <= n - 1);
        if (u != 500 || n % 2 == 1)
          CHECK (easel_shape_place (1000 - u, n) == n - 1 - p);
      }
  CHECK (easel_shape_place (0, 11) == 0 && easel_shape_place (1000, 11) == 10 && easel_shape_place (500, 11) == 5);
  CHECK (easel_shape_place (382, 11) == 4);        /* 3.82 + 0.5 */
  CHECK (easel_shape_place (617, 11) == 6);        /* 10 - floor (3.83 + 0.5) */
}

/* AS-2: Triangle in (10, 10) to (20, 18): (15, 10), (20, 18), (10, 18); width 5: (15, 12), (18, 16), (12, 16). */
static void
shapes_triangle_corners (void)
{
  int pts[2 * EASEL_SHAPE_MAX_CORNERS];
  EaselFigure f = easel_shape_figure (EASEL_SHAPE_KIND_TRIANGLE, 10, 10, 11, 9, 1, EASEL_FIGURE_OUTLINE, false, false, false, pts);
  static const int one[6] = { 15, 10, 20, 18, 10, 18 }, five[6] = { 15, 12, 18, 16, 12, 16 };

  CHECK (f.kind == EASEL_FIGURE_POLYGON && f.closed && f.n_pts == 3 && f.pts == pts && f.width == 1);
  CHECK (memcmp (pts, one, sizeof one) == 0);
  f = easel_shape_figure (EASEL_SHAPE_KIND_TRIANGLE, 10, 10, 11, 9, 5, EASEL_FIGURE_OUTLINE, true, false, false, pts);
  CHECK (f.n_pts == 3 && f.width == 5 && f.smooth);
  CHECK (memcmp (pts, five, sizeof five) == 0);
  /* Even widths: the inset is (w - 1) / 2 rounded down, 0 for 2 and 1 for 4; the inner box 10 x 8 and 8 x 6. */
  static const int two[6] = { 15, 10, 19, 17, 10, 17 }, four[6] = { 15, 11, 18, 16, 11, 16 };
  easel_shape_figure (EASEL_SHAPE_KIND_TRIANGLE, 10, 10, 11, 9, 2, EASEL_FIGURE_OUTLINE, false, false, false, pts);
  CHECK (memcmp (pts, two, sizeof two) == 0);
  easel_shape_figure (EASEL_SHAPE_KIND_TRIANGLE, 10, 10, 11, 9, 4, EASEL_FIGURE_OUTLINE, false, false, false, pts);
  CHECK (memcmp (pts, four, sizeof four) == 0);
}

/* The box of a drag (REQ-0043-04). */
static void
shapes_box (void)
{
  int x, y, w, h;

  easel_shape_box (10, 10, 20, 18, false, &x, &y, &w, &h);
  CHECK (x == 10 && y == 10 && w == 11 && h == 9);
  easel_shape_box (20, 18, 10, 10, false, &x, &y, &w, &h);
  CHECK (x == 10 && y == 10 && w == 11 && h == 9);
  easel_shape_box (20, 10, 10, 18, false, &x, &y, &w, &h);
  CHECK (x == 10 && y == 10 && w == 11 && h == 9);
  easel_shape_box (10, 10, 20, 18, true, &x, &y, &w, &h);          /* Shift: the smaller side, from the start toward the end */
  CHECK (x == 10 && y == 10 && w == 9 && h == 9);
  easel_shape_box (20, 18, 10, 10, true, &x, &y, &w, &h);
  CHECK (x == 12 && y == 10 && w == 9 && h == 9);
  easel_shape_box (20, 10, 10, 30, true, &x, &y, &w, &h);
  CHECK (x == 10 && y == 10 && w == 11 && h == 11);
  easel_shape_box (5, 5, 5, 5, false, &x, &y, &w, &h);              /* a 1 x 1 box */
  CHECK (x == 5 && y == 5 && w == 1 && h == 1);
  easel_shape_box (0, 0, 20000, 5, false, &x, &y, &w, &h);          /* the end stops at 10000 */
  CHECK (x == 0 && y == 0 && w == 10000 && h == 6);
  easel_shape_box (20000, 0, 0, 5, false, &x, &y, &w, &h);
  CHECK (x == 10001 && y == 0 && w == 10000 && h == 6);
  easel_shape_box (-3, 7, -3, -30000, true, &x, &y, &w, &h);
  CHECK (w == 1 && h == 1 && x == -3 && y == 7);
  easel_shape_box (0, 0, 9999, 9999, false, &x, &y, &w, &h);
  CHECK (w == 10000 && h == 10000);
  easel_shape_box (0, 0, 30000, 20000, true, &x, &y, &w, &h);
  CHECK (x == 0 && y == 0 && w == 10000 && h == 10000);
}

/* AS-3: each of the 16 shapes equals Polygon P, in boxes 1 x 1, 2 x 2, 11 x 9 and 101 x 61, mirrored on each axis or not, Sharp and
   Smooth, widths 1 and 5, every fill style; with Sharp and a box of at least w x w no pixel lies outside the box. */
static void
shapes_every_shape_is_polygon_p (void)
{
  static const int boxes[4][2] = { { 1, 1 }, { 2, 2 }, { 11, 9 }, { 101, 61 } };
  const int pw = 120, ph = 80, bx = 9, by = 7;
  int pts[2 * EASEL_SHAPE_MAX_CORNERS], want_pts[2 * EASEL_SHAPE_MAX_CORNERS];
  int cases = 0;

  for (int s = EASEL_SHAPE_KIND_TRIANGLE; s < EASEL_SHAPE_KIND_COUNT; s++)
    for (int b = 0; b < 4; b++)
      for (int m = 0; m < 4; m++)
        for (int sm = 0; sm < 2; sm++)
          for (int width = 1; width <= 5; width += 4)
            for (int st = 0; st < 3; st++)
              {
                int w = boxes[b][0], h = boxes[b][1];
                EaselFigure got = easel_shape_figure ((EaselShapeKind) s, bx, by, w, h, width, styles[st], sm, m & 1, m & 2, pts);
                EaselFigure want = polygon_p ((EaselShapeKind) s, bx, by, w, h, width, styles[st], sm, m & 1, m & 2, want_pts);
                EaselImage *a, *c;

                CHECK (got.kind == EASEL_FIGURE_POLYGON && got.closed && got.n_pts == want.n_pts && got.pts == pts);
                CHECK (got.width == width && got.style == styles[st] && got.smooth == (bool) sm);
                CHECK (memcmp (pts, want_pts, 2 * want.n_pts * sizeof pts[0]) == 0);
                a = render (&got, pw, ph);
                c = render (&want, pw, ph);
                CHECK (same (a, c));
                if (!sm && w >= width && h >= width)
                  for (int y = 0; y < ph; y++)
                    for (int x = 0; x < pw; x++)
                      if (a->px[y * pw + x] != WHITE)
                        CHECK (x >= bx && x < bx + w && y >= by && y < by + h);
                easel_image_free (a);
                easel_image_free (c);
                cases++;
              }
  CHECK (cases == 16 * 4 * 4 * 2 * 2 * 3);
}

/* AS-3: at odd sizes a shape mirrored left to right is the pixel mirror of the unmirrored one, and so top to bottom. */
static void
shapes_mirror_is_pixel_mirror (void)
{
  static const int boxes[2][2] = { { 11, 9 }, { 101, 61 } };
  const int pw = 120, ph = 80, bx = 9, by = 7;
  int pts[2 * EASEL_SHAPE_MAX_CORNERS];

  for (int s = EASEL_SHAPE_KIND_TRIANGLE; s < EASEL_SHAPE_KIND_COUNT; s++)
    for (int b = 0; b < 2; b++)
      for (int sm = 0; sm < 2; sm++)
        for (int width = 1; width <= 5; width += 4)
          for (int st = 0; st < 3; st++)
            {
              int w = boxes[b][0], h = boxes[b][1];
              EaselFigure f = easel_shape_figure ((EaselShapeKind) s, bx, by, w, h, width, styles[st], sm, false, false, pts);
              EaselImage *plain = render (&f, pw, ph), *mx, *my;

              f = easel_shape_figure ((EaselShapeKind) s, bx, by, w, h, width, styles[st], sm, true, false, pts);
              mx = render (&f, pw, ph);
              f = easel_shape_figure ((EaselShapeKind) s, bx, by, w, h, width, styles[st], sm, false, true, pts);
              my = render (&f, pw, ph);
              for (int y = 0; y < ph; y++)
                for (int x = 0; x < pw; x++)
                  {
                    int x2 = 2 * bx + w - 1 - x, y2 = 2 * by + h - 1 - y;
                    uint32_t c = plain->px[y * pw + x];

                    if (x2 >= 0 && x2 < pw)
                      CHECK (mx->px[y * pw + x2] == c);
                    if (y2 >= 0 && y2 < ph)
                      CHECK (my->px[y2 * pw + x] == c);
                  }
              easel_image_free (plain);
              easel_image_free (mx);
              easel_image_free (my);
            }
}

/* A 1 x 1 box: every corner is the one pixel; a box narrower than the width: the inner box is the middle pixel. */
static void
shapes_small_boxes (void)
{
  int pts[2 * EASEL_SHAPE_MAX_CORNERS];
  EaselFigure f;

  for (int s = EASEL_SHAPE_KIND_TRIANGLE; s < EASEL_SHAPE_KIND_COUNT; s++)
    {
      f = easel_shape_figure ((EaselShapeKind) s, 4, 6, 1, 1, 5, EASEL_FIGURE_OUTLINE, false, true, true, pts);
      for (size_t i = 0; i < f.n_pts; i++)
        CHECK (pts[2 * i] == 4 && pts[2 * i + 1] == 6);
      f = easel_shape_figure ((EaselShapeKind) s, 4, 6, 3, 4, 5, EASEL_FIGURE_OUTLINE, false, false, false, pts);
      for (size_t i = 0; i < f.n_pts; i++)
        CHECK (pts[2 * i] == 5 && pts[2 * i + 1] == 7);   /* 4 + (3 - 1) / 2, 6 + (4 - 1) / 2 */
    }
}

/* REQ-0043-18, the Plan's Risk 1: Rectangle, Rounded Rectangle and Ellipse from their box give the pixels the old tools gave from the
   drag, in each of the four directions, sizes 1 to 20 on each side, widths 1 to 5, the three styles, Sharp and Smooth; the mirror
   flags change nothing. */
static void
shapes_old_three_identical (void)
{
  static const EaselFigureKind kinds[3] = { EASEL_FIGURE_RECTANGLE, EASEL_FIGURE_ROUNDED, EASEL_FIGURE_ELLIPSE };
  const int pw = 30, ph = 30, bx = 4, by = 5;
  int pts[2 * EASEL_SHAPE_MAX_CORNERS];

  for (int k = 0; k < 3; k++)
    for (int w = 1; w <= 20; w++)
      for (int h = 1; h <= 20; h++)
        for (int width = 1; width <= 5; width++)
          for (int st = 0; st < 3; st++)
            for (int sm = 0; sm < 2; sm++)
              {
                EaselFigure got = easel_shape_figure ((EaselShapeKind) k, bx, by, w, h, width, styles[st], sm, (w + h) & 1, (w + h) & 2, pts);
                EaselImage *a = render (&got, pw, ph);

                CHECK (got.kind == kinds[k]);
                for (int d = 0; d < 4; d++)
                  {
                    int sx = d & 1 ? bx + w - 1 : bx, ex = d & 1 ? bx : bx + w - 1;
                    int sy = d & 2 ? by + h - 1 : by, ey = d & 2 ? by : by + h - 1;
                    EaselFigure old = { kinds[k], sx, sy, ex, ey, width, styles[st], NULL };
                    EaselImage *b;

                    old.smooth = sm;
                    b = render (&old, pw, ph);
                    if (!same (a, b))
                      {
                        fprintf (stderr, "REQ-0043-18: kind %d, %d x %d, width %d, style %d, smooth %d, direction %d differs\n", k, w, h,
                                 width, st, sm, d);
                        CHECK (0);
                      }
                    easel_image_free (b);
                  }
                easel_image_free (a);
              }
}

/* REQ-0043-14 (4): the same input gives the same pixels. */
static void
shapes_deterministic (void)
{
  int pts[2 * EASEL_SHAPE_MAX_CORNERS];

  for (int s = 0; s < EASEL_SHAPE_KIND_COUNT; s++)
    {
      EaselFigure f = easel_shape_figure ((EaselShapeKind) s, 3, 2, 37, 23, 3, EASEL_FIGURE_OUTLINE_AND_FILL, s & 1, true, false, pts);
      EaselImage *a = render (&f, 50, 40), *b;

      f = easel_shape_figure ((EaselShapeKind) s, 3, 2, 37, 23, 3, EASEL_FIGURE_OUTLINE_AND_FILL, s & 1, true, false, pts);
      b = render (&f, 50, 40);
      CHECK (same (a, b));
      easel_image_free (a);
      easel_image_free (b);
    }
}

/* The largest box: every corner within it, the products in range (REQ-0043-14 (4)). */
static void
shapes_largest_box (void)
{
  int pts[2 * EASEL_SHAPE_MAX_CORNERS];

  for (int s = EASEL_SHAPE_KIND_TRIANGLE; s < EASEL_SHAPE_KIND_COUNT; s++)
    {
      EaselFigure f = easel_shape_figure ((EaselShapeKind) s, -5000, 3, 10000, 10000, 1, EASEL_FIGURE_OUTLINE, false, true, true, pts);
      bool left = false, right = false;

      for (size_t i = 0; i < f.n_pts; i++)
        {
          CHECK (pts[2 * i] >= -5000 && pts[2 * i] <= 4999 && pts[2 * i + 1] >= 3 && pts[2 * i + 1] <= 10002);
          left |= pts[2 * i] == -5000;
          right |= pts[2 * i] == 4999;
        }
      CHECK (left && right);
    }
}

int
main (void)
{
  read_corners_md ();
  RUN (shapes_names);
  RUN (shapes_table_is_corners_md);
  RUN (shapes_place);
  RUN (shapes_triangle_corners);
  RUN (shapes_box);
  RUN (shapes_old_three_identical);
  RUN (shapes_every_shape_is_polygon_p);
  RUN (shapes_mirror_is_pixel_mirror);
  RUN (shapes_small_boxes);
  RUN (shapes_deterministic);
  RUN (shapes_largest_box);
  return 0;
}
