/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "shapes.h"

#include <string.h>

static const char *const names[EASEL_SHAPE_KIND_COUNT] = {
  "rectangle", "rounded-rectangle", "ellipse", "triangle", "right-triangle", "diamond", "pentagon", "hexagon", "right-arrow",
  "left-arrow", "up-arrow", "down-arrow", "4-point-star", "5-point-star", "6-point-star", "rounded-rectangle-callout",
  "oval-callout", "heart", "lightning-bolt"
};

/* The corners of docs/reference/proto-0043/corners.md (normative, REQ-0043-04), as (u, v) pairs in its order; made from that file by a
   script, and checked against it by core-shapes. */
/* Triangle */
static const int16_t triangle[] = {
  500, 0, 1000, 1000, 0, 1000
};

/* Right Triangle */
static const int16_t right_triangle[] = {
  0, 0, 1000, 1000, 0, 1000
};

/* Diamond */
static const int16_t diamond[] = {
  500, 0, 1000, 500, 500, 1000, 0, 500
};

/* Pentagon */
static const int16_t pentagon[] = {
  500, 0, 1000, 382, 809, 1000, 191, 1000, 0, 382
};

/* Hexagon */
static const int16_t hexagon[] = {
  1000, 500, 750, 1000, 250, 1000, 0, 500, 250, 0, 750, 0
};

/* Right Arrow */
static const int16_t right_arrow[] = {
  0, 250, 600, 250, 600, 0, 1000, 500, 600, 1000, 600, 750, 0, 750
};

/* Left Arrow */
static const int16_t left_arrow[] = {
  1000, 250, 400, 250, 400, 0, 0, 500, 400, 1000, 400, 750, 1000, 750
};

/* Up Arrow */
static const int16_t up_arrow[] = {
  250, 1000, 250, 400, 0, 400, 500, 0, 1000, 400, 750, 400, 750, 1000
};

/* Down Arrow */
static const int16_t down_arrow[] = {
  250, 0, 250, 600, 0, 600, 500, 1000, 1000, 600, 750, 600, 750, 0
};

/* 4-Point Star */
static const int16_t star_4[] = {
  500, 0, 634, 366, 1000, 500, 634, 634, 500, 1000, 366, 634, 0, 500, 366, 366
};

/* 5-Point Star */
static const int16_t star_5[] = {
  500, 0, 617, 383, 1000, 382, 690, 618, 809, 1000, 500, 763, 191, 1000, 310, 618, 0, 382, 383, 383
};

/* 6-Point Star */
static const int16_t star_6[] = {
  500, 0, 644, 283, 1000, 250, 789, 500, 1000, 750, 644, 717, 500, 1000, 356, 717, 0, 750, 211, 500, 0, 250, 356, 283
};

/* Rounded Rectangle Callout */
static const int16_t rounded_callout[] = {
  880, 0, 896, 1, 911, 4, 926, 9, 940, 16, 953, 25, 965, 35, 975, 47, 984, 60, 991, 74, 996, 89, 999, 104,
  1000, 120, 1000, 630, 999, 646, 996, 661, 991, 676, 984, 690, 975, 703, 965, 715, 953, 725, 940, 734, 926, 741, 911, 746,
  896, 749, 880, 750, 400, 750, 150, 1000, 250, 750, 120, 750, 104, 749, 89, 746, 74, 741, 60, 734, 47, 725, 35, 715,
  25, 703, 16, 690, 9, 676, 4, 661, 1, 646, 0, 630, 0, 120, 1, 104, 4, 89, 9, 74, 16, 60, 25, 47,
  35, 35, 47, 25, 60, 16, 74, 9, 89, 4, 104, 1, 120, 0
};

/* Oval Callout */
static const int16_t oval_callout[] = {
  1000, 375, 999, 400, 996, 424, 990, 448, 983, 472, 973, 496, 962, 519, 948, 541, 933, 562, 916, 583, 897, 603, 876, 622,
  854, 640, 830, 657, 804, 673, 778, 687, 750, 700, 721, 711, 691, 721, 661, 730, 629, 737, 598, 743, 565, 747, 533, 749,
  500, 750, 467, 749, 435, 747, 402, 743, 371, 737, 150, 1000, 196, 673, 170, 657, 146, 640, 124, 622, 103, 603, 84, 583,
  67, 562, 52, 541, 38, 519, 27, 496, 17, 472, 10, 448, 4, 424, 1, 400, 0, 375, 1, 350, 4, 326, 10, 302,
  17, 278, 27, 254, 38, 231, 52, 209, 67, 188, 84, 167, 103, 147, 124, 128, 146, 110, 170, 93, 196, 77, 222, 63,
  250, 50, 279, 39, 309, 29, 339, 20, 371, 13, 402, 7, 435, 3, 467, 1, 500, 0, 533, 1, 565, 3, 598, 7,
  629, 13, 661, 20, 691, 29, 721, 39, 750, 50, 778, 63, 804, 77, 830, 93, 854, 110, 876, 128, 897, 147, 916, 167,
  933, 188, 948, 209, 962, 231, 973, 254, 983, 278, 990, 302, 996, 326, 999, 350
};

/* Heart */
static const int16_t heart[] = {
  500, 239, 500, 236, 501, 227, 504, 213, 509, 194, 517, 171, 528, 146, 543, 119, 562, 92, 586, 67, 613, 44, 643, 25,
  677, 11, 712, 2, 750, 0, 787, 4, 825, 15, 861, 31, 894, 54, 925, 82, 951, 115, 972, 151, 987, 190, 997, 231,
  1000, 274, 997, 317, 987, 360, 972, 403, 951, 445, 925, 486, 894, 526, 861, 565, 825, 602, 787, 639, 750, 675, 712, 710,
  677, 744, 643, 778, 613, 810, 586, 841, 562, 871, 543, 898, 528, 923, 517, 946, 509, 964, 504, 980, 501, 991, 500, 998,
  500, 1000, 500, 998, 499, 991, 496, 980, 491, 964, 483, 946, 472, 923, 457, 898, 438, 871, 414, 841, 387, 810, 357, 778,
  323, 744, 288, 710, 250, 675, 213, 639, 175, 602, 139, 565, 106, 526, 75, 486, 49, 445, 28, 403, 13, 360, 3, 317,
  0, 274, 3, 231, 13, 190, 28, 151, 49, 115, 75, 82, 106, 54, 139, 31, 175, 15, 213, 4, 250, 0, 288, 2,
  323, 11, 357, 25, 387, 44, 414, 67, 438, 92, 457, 119, 472, 146, 483, 171, 491, 194, 496, 213, 499, 227, 500, 236
};

/* Lightning Bolt */
static const int16_t lightning_bolt[] = {
  323, 0, 871, 0, 597, 360, 1000, 360, 113, 1000, 355, 520, 0, 520
};

static const struct
{
  const int16_t *uv;
  size_t n;
} corners[EASEL_SHAPE_KIND_COUNT] = {
  [EASEL_SHAPE_KIND_TRIANGLE] = { triangle, 3 },
  [EASEL_SHAPE_KIND_RIGHT_TRIANGLE] = { right_triangle, 3 },
  [EASEL_SHAPE_KIND_DIAMOND] = { diamond, 4 },
  [EASEL_SHAPE_KIND_PENTAGON] = { pentagon, 5 },
  [EASEL_SHAPE_KIND_HEXAGON] = { hexagon, 6 },
  [EASEL_SHAPE_KIND_RIGHT_ARROW] = { right_arrow, 7 },
  [EASEL_SHAPE_KIND_LEFT_ARROW] = { left_arrow, 7 },
  [EASEL_SHAPE_KIND_UP_ARROW] = { up_arrow, 7 },
  [EASEL_SHAPE_KIND_DOWN_ARROW] = { down_arrow, 7 },
  [EASEL_SHAPE_KIND_STAR_4] = { star_4, 8 },
  [EASEL_SHAPE_KIND_STAR_5] = { star_5, 10 },
  [EASEL_SHAPE_KIND_STAR_6] = { star_6, 12 },
  [EASEL_SHAPE_KIND_ROUNDED_CALLOUT] = { rounded_callout, 55 },
  [EASEL_SHAPE_KIND_OVAL_CALLOUT] = { oval_callout, 92 },
  [EASEL_SHAPE_KIND_HEART] = { heart, 96 },
  [EASEL_SHAPE_KIND_LIGHTNING_BOLT] = { lightning_bolt, 7 },
};

const char *
easel_shape_name (EaselShapeKind shape)
{
  return (unsigned) shape < EASEL_SHAPE_KIND_COUNT ? names[shape] : NULL;
}

bool
easel_shape_from_name (const char *name, EaselShapeKind *shape)
{
  for (int i = 0; i < EASEL_SHAPE_KIND_COUNT; i++)
    if (strcmp (name, names[i]) == 0)
      {
        *shape = (EaselShapeKind) i;
        return true;
      }
  return false;
}

size_t
easel_shape_corners (EaselShapeKind shape, const int16_t **uv)
{
  *uv = (unsigned) shape < EASEL_SHAPE_KIND_COUNT ? corners[shape].uv : NULL;
  return *uv != NULL ? corners[shape].n : 0;
}

/* The end (*e) brought within EASEL_SHAPE_MAX_SIDE pixels of the start s, both included. */
static int
limit_end (int s, int e)
{
  if (e - s >= EASEL_SHAPE_MAX_SIDE)
    return s + EASEL_SHAPE_MAX_SIDE - 1;
  if (s - e >= EASEL_SHAPE_MAX_SIDE)
    return s - (EASEL_SHAPE_MAX_SIDE - 1);
  return e;
}

void
easel_shape_box (int x0, int y0, int x1, int y1, bool shift, int *x, int *y, int *w, int *h)
{
  x1 = limit_end (x0, x1);
  y1 = limit_end (y0, y1);
  if (shift)
    easel_figure_constrain (EASEL_FIGURE_RECTANGLE, x0, y0, &x1, &y1);   /* a square from the start toward the end (REQ-0010-14) */
  *x = x0 < x1 ? x0 : x1;
  *y = y0 < y1 ? y0 : y1;
  *w = (x0 < x1 ? x1 - x0 : x0 - x1) + 1;
  *h = (y0 < y1 ? y1 - y0 : y0 - y1) + 1;
}

int
easel_shape_place (int u, int n)
{
  /* Whole numbers only: u <= 1000 and n <= 10000 keep the product below 10 000 000 (REQ-0043-14 (4)). A corner and its mirror
     (1000 - u) are rounded alike from their own side, so the mirror is exact. */
  if (u <= 500)
    return (u * (n - 1) + 500) / 1000;
  return (n - 1) - ((1000 - u) * (n - 1) + 500) / 1000;
}

/* The inner box on one axis (REQ-0043-04): the side less the width, from half the width in; a side that would be less than one
   pixel is the box's middle pixel. */
static void
inner (int x, int w, int width, int *ix, int *iw)
{
  *iw = w - (width - 1);
  *ix = x + (width - 1) / 2;
  if (*iw < 1)
    {
      *iw = 1;
      *ix = x + (w - 1) / 2;
    }
}

EaselFigure
easel_shape_figure (EaselShapeKind shape, int x, int y, int w, int h, int width, EaselFigureStyle style, bool smooth, bool mirror_x,
                    bool mirror_y, int *pts)
{
  static const EaselFigureKind own[3] = { EASEL_FIGURE_RECTANGLE, EASEL_FIGURE_ROUNDED, EASEL_FIGURE_ELLIPSE };
  EaselFigure f = { EASEL_FIGURE_POLYGON, 0, 0, 0, 0, width, style, NULL, pts, 0, true };
  const int16_t *uv;
  int ix, iw, iy, ih;

  if (shape < EASEL_SHAPE_KIND_TRIANGLE)
    {
      /* Their own figure in the box, as the old tools drew it (REQ-0043-18): no inset, no mirror. */
      f = (EaselFigure) { own[shape], x, y, x + w - 1, y + h - 1, width, style, NULL };
      f.smooth = smooth;
      return f;
    }
  f.n_pts = easel_shape_corners (shape, &uv);
  inner (x, w, width, &ix, &iw);
  inner (y, h, width, &iy, &ih);
  for (size_t i = 0; i < f.n_pts; i++)
    {
      pts[2 * i] = ix + easel_shape_place (mirror_x ? 1000 - uv[2 * i] : uv[2 * i], iw);
      pts[2 * i + 1] = iy + easel_shape_place (mirror_y ? 1000 - uv[2 * i + 1] : uv[2 * i + 1], ih);
    }
  f.smooth = smooth;
  return f;
}
