/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Stretch and Skew (0019): the dialog's math, the pixels of the whole picture and of a float. */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "check.h"
#include "history.h"
#include "image.h"
#include "lasso.h"
#include "selection.h"
#include "stretch-skew.h"
#include "transform.h"

/* AS-4 (REQ-0019-04): a percent of a size, halves up and at least 1, and back, each within the unit's limits. */
static void
as4_percent_and_conversion (void)
{
  CHECK (easel_ss_percent (400, 33) == 132);
  CHECK (easel_ss_percent (50, 1) == 1);
  CHECK (easel_ss_percent (3, 50) == 2);
  CHECK (easel_ss_percent (1, 1) == 1);
  CHECK (easel_ss_percent (400, 200) == 800);
  CHECK (easel_ss_percent (8000, 500) == 10000);            /* 40000, past the limit */
  CHECK (easel_ss_percent_unlimited (8000, 500) == 40000);  /* what New size asks for */
  CHECK (easel_ss_percent_unlimited (50, 1) == 1);
  CHECK (easel_ss_to_percent (132, 400) == 33);
  CHECK (easel_ss_to_percent (800, 400) == 200);
  CHECK (easel_ss_to_percent (1, 10000) == 1);              /* 0.01 % is below the limit */
  CHECK (easel_ss_percent (10000, 1) == 100);               /* a conversion may change the size */
  CHECK (easel_ss_to_percent (8000, 400) == 500);           /* 2000 % is past the limit */
  CHECK (easel_ss_to_percent (2, 3) == 67);
}

/* AS-5 (REQ-0019-05): the other side, and how far a side may go with the other within its limit. */
static void
as5_ratio (void)
{
  CHECK (easel_ss_ratio_other (800, 400, 300) == 600);
  CHECK (easel_ss_ratio_other (1, 400, 300) == 1);
  CHECK (easel_ss_ratio_other (3, 2, 1) == 2);              /* 1.5, a half goes up */
  CHECK (easel_ss_ratio_fit (2, 1, 10000) == 1);            /* 1 x 10000 cannot be made wider */
  CHECK (easel_ss_ratio_fit (10000, 300, 400) == 7500);
  CHECK (easel_ss_ratio_other (7500, 300, 400) == 10000);
  CHECK (easel_ss_ratio_fit (800, 400, 300) == 800);
  CHECK (easel_ss_ratio_fit (10000, 8000, 300) == 10000);
  CHECK (easel_ss_ratio_other (10000, 8000, 300) == 375);
}

/* AS-9 (REQ-0019-10): the shifts of the rows, for a positive and a negative amount. */
static void
as9_shifts (void)
{
  static const int two[] = { 0, 1, 1, 2, 2 }, minus_two[] = { 2, 1, 1, 0, 0 }, minus_three[] = { 3, 2, 1, 1, 0 };

  for (int y = 0; y < 5; y++)
    {
      CHECK (easel_ss_shift (y, 5, 2) == two[y]);
      CHECK (easel_ss_shift (y, 5, -2) == minus_two[y]);
      CHECK (easel_ss_shift (y, 5, -3) == minus_three[y]);
      CHECK (easel_ss_shift (y, 5, 0) == 0);
    }
  CHECK (easel_ss_shift (0, 1, 5) == 0);
  CHECK (easel_ss_shift (0, 1, -5) == 0);
  CHECK (easel_ss_shift (0, 4, 1) == 0);
  CHECK (easel_ss_shift (1, 4, 1) == 0);
  CHECK (easel_ss_shift (2, 4, 1) == 1);
  CHECK (easel_ss_shift (3, 4, 1) == 1);
}

/* AS-4, AS-9 (REQ-0019-04, REQ-0019-10): degrees to an amount and back, the examples of the spec. */
static void
as9_degrees (void)
{
  CHECK (easel_ss_skew_amount (100, 45) == 99);
  CHECK (easel_ss_skew_amount (300, 10) == 53);
  CHECK (easel_ss_skew_amount (300, -10) == -53);
  CHECK (easel_ss_skew_amount (5, 30) == 2);
  CHECK (easel_ss_skew_amount (100, 89) == 5672);
  CHECK (easel_ss_skew_amount (8000, 89) == 458262);
  CHECK (easel_ss_skew_amount (8000, 1) == 140);
  CHECK (easel_ss_skew_amount (2, 26) == 0);
  CHECK (easel_ss_skew_amount (2, 27) == 1);
  CHECK (easel_ss_skew_amount (1, 45) == 0);
  CHECK (easel_ss_skew_amount (100, 0) == 0);
  CHECK (easel_ss_skew_degrees (9, 2) == 84);
  CHECK (easel_ss_skew_amount (2, 84) == 10);
  CHECK (easel_ss_skew_degrees (1, 1000) == 0);
  CHECK (easel_ss_skew_degrees (5, 1) == 0);
  CHECK (easel_ss_skew_degrees (-9, 2) == -84);
  CHECK (easel_ss_skew_degrees (99, 100) == 45);
  CHECK (easel_ss_skew_degrees (9999, 2) == 89);            /* 89.99 stays within the limit */
  /* Each whole angle comes back from its own amount on a tall picture. */
  for (int d = -89; d <= 89; d++)
    CHECK (easel_ss_skew_degrees (easel_ss_skew_amount (10000, d), 10000) == d);
}

/* AS-7, AS-8, AS-9 (REQ-0019-07, REQ-0019-08, REQ-0019-10): the new size, stretch first, then horizontal, then vertical. */
static void
as8_order (void)
{
  int w, h, sh, sv;

  easel_ss_size (4, 4, 45, true, 0, true, &w, &h, &sh, &sv);        /* 2 x 2 stretched to 200 % */
  CHECK (w == 7 && h == 4 && sh == 3 && sv == 0);
  easel_ss_size (100, 100, 45, true, 45, true, &w, &h, &sh, &sv);
  CHECK (w == 199 && h == 298 && sh == 99 && sv == 198);
  easel_ss_size (100, 100, 45, true, 0, true, &w, &h, &sh, &sv);
  CHECK (w == 199 && h == 100);
  easel_ss_size (400, 300, 10, true, 0, true, &w, &h, &sh, &sv);
  CHECK (w == 453 && h == 300);
  easel_ss_size (400, 1, 45, true, 0, true, &w, &h, &sh, &sv);      /* one row: nothing shifts */
  CHECK (w == 400 && h == 1 && sh == 0);
  easel_ss_size (400, 1, 5, false, 0, true, &w, &h, &sh, &sv);
  CHECK (w == 400 && h == 1 && sh == 0);
  easel_ss_size (1, 400, 0, true, 5, false, &w, &h, &sh, &sv);      /* one column */
  CHECK (w == 1 && h == 400 && sv == 0);
  easel_ss_size (1, 1, 45, true, 45, true, &w, &h, &sh, &sv);
  CHECK (w == 1 && h == 1 && sh == 0 && sv == 0);
  easel_ss_size (3, 3, 45, true, 0, true, &w, &h, &sh, &sv);        /* 1 x 1 at 300 % */
  CHECK (w == 5 && h == 3);
  easel_ss_size (8000, 300, 2000, false, 0, true, &w, &h, &sh, &sv);
  CHECK (w == 10000 && !easel_ss_too_large (w, h));
  easel_ss_size (8000, 300, 2001, false, 0, true, &w, &h, &sh, &sv);
  CHECK (w == 10001 && easel_ss_too_large (w, h));
  easel_ss_size (8000, 300, -2001, false, 0, true, &w, &h, &sh, &sv);
  CHECK (w == 10001 && sh == -2001);
  easel_ss_size (300, 8000, 0, true, 0, true, &w, &h, &sh, &sv);
  CHECK (w == 300 && h == 8000 && !easel_ss_too_large (w, h));
  CHECK (easel_ss_too_large (10000, 10001));
}

/* Allocation failure: this test is linked with --wrap, so every allocation of the core comes here. fail_at = n: the n-th one from now
   returns NULL, as a real failure does. 0: none fails. */
static int fail_at, allocations;
void *__real_malloc (size_t n);
void *__real_calloc (size_t a, size_t b);
void *__real_realloc (void *p, size_t n);
void *__wrap_malloc (size_t n);
void *__wrap_calloc (size_t a, size_t b);
void *__wrap_realloc (void *p, size_t n);

static bool
fails_now (void)
{
  allocations++;
  return fail_at != 0 && allocations == fail_at;
}

void *
__wrap_malloc (size_t n)
{
  return fails_now () ? NULL : __real_malloc (n);
}

void *
__wrap_calloc (size_t a, size_t b)
{
  return fails_now () ? NULL : __real_calloc (a, b);
}

void *
__wrap_realloc (void *p, size_t n)
{
  return fails_now () ? NULL : __real_realloc (p, n);
}

#define A 0x11223344u
#define B 0x55667788u
#define C 0x99AABBCCu
#define D 0xDDEEFF00u
#define BG 0x12345678u

static EaselImage *
image_of (int w, int h, const uint32_t *px)
{
  EaselImage *img = easel_image_new (w, h, 0);

  for (int i = 0; i < w * h; i++)
    img->px[i] = px[i];
  return img;
}

static bool
image_is (const EaselImage *img, int w, int h, const uint32_t *px)
{
  return img->width == w && img->height == h && memcmp (img->px, px, (size_t) w * h * sizeof *px) == 0;
}

/* AS-8 (REQ-0019-09): the nearest pixel, every channel exact, the same rule as a selection's stretch. */
static void
as8_stretch_nearest (void)
{
  static const uint32_t abcd[] = { A, B, C, D }, half[] = { B, D }, twice[] = { A, A, B, B, C, C, D, D };
  static const uint32_t one[] = { A }, nine[] = { A, A, A, A, A, A, A, A, A };
  EaselHistory *h = easel_history_new ((size_t) 512 << 20);
  EaselImage *img = image_of (4, 1, abcd);

  CHECK (easel_image_stretch_skew (img, h, 2, 1, 0, 0, BG, SIZE_MAX) == EASEL_TRANSFORM_DONE);
  CHECK (image_is (img, 2, 1, half));
  CHECK (easel_history_undo (h, img));
  CHECK (image_is (img, 4, 1, abcd));
  CHECK (easel_image_stretch_skew (img, h, 8, 1, 0, 0, BG, SIZE_MAX) == EASEL_TRANSFORM_DONE);
  CHECK (image_is (img, 8, 1, twice));
  easel_image_free (img);
  img = image_of (1, 1, one);
  CHECK (easel_image_stretch_skew (img, h, 3, 3, 0, 0, BG, SIZE_MAX) == EASEL_TRANSFORM_DONE);
  CHECK (image_is (img, 3, 3, nine));
  easel_image_free (img);
  /* The rows by the same rule: 4 rows a b c d to 2 give b d. */
  img = image_of (1, 4, abcd);
  CHECK (easel_image_stretch_skew (img, h, 1, 2, 0, 0, BG, SIZE_MAX) == EASEL_TRANSFORM_DONE);
  CHECK (image_is (img, 1, 2, half));
  easel_image_free (img);
  easel_history_free (h);
}

/* AS-9, AS-10 (REQ-0019-10, REQ-0019-11): an asymmetric picture, both signs, both axes; what no row or column reaches is the background. */
static void
as9_skew_pixels_asymmetric (void)
{
  /*  a b      a b .       . a b
      c d      . c d       c d .  */
  static const uint32_t abcd[] = { A, B, C, D };
  static const uint32_t right[] = { A, B, BG, BG, C, D }, left[] = { BG, A, B, C, D, BG };
  static const uint32_t down[] = { A, BG, C, B, BG, D }, up[] = { BG, B, A, D, C, BG };
  EaselHistory *h = easel_history_new ((size_t) 512 << 20);
  EaselImage *img = image_of (2, 2, abcd);

  CHECK (easel_image_stretch_skew (img, h, 2, 2, 1, 0, BG, SIZE_MAX) == EASEL_TRANSFORM_DONE);
  CHECK (image_is (img, 3, 2, right));
  CHECK (easel_history_undo (h, img));
  CHECK (easel_image_stretch_skew (img, h, 2, 2, -1, 0, BG, SIZE_MAX) == EASEL_TRANSFORM_DONE);
  CHECK (image_is (img, 3, 2, left));
  CHECK (easel_history_undo (h, img));
  CHECK (easel_image_stretch_skew (img, h, 2, 2, 0, 1, BG, SIZE_MAX) == EASEL_TRANSFORM_DONE);
  CHECK (image_is (img, 2, 3, down));
  CHECK (easel_history_undo (h, img));
  CHECK (easel_image_stretch_skew (img, h, 2, 2, 0, -1, BG, SIZE_MAX) == EASEL_TRANSFORM_DONE);
  CHECK (image_is (img, 2, 3, up));
  CHECK (easel_history_undo (h, img));
  /* Both: the vertical skew acts on the width the horizontal one made, 3 columns: shifts 0, 1, 2 for S = 2. */
  {
    int w, hgt, sh, sv;

    easel_ss_size (2, 2, 1, false, 2, false, &w, &hgt, &sh, &sv);
    CHECK (w == 3 && hgt == 4);
    CHECK (easel_image_stretch_skew (img, h, 2, 2, 1, 2, BG, SIZE_MAX) == EASEL_TRANSFORM_DONE);
    /* After the horizontal skew: a b . over . c d. Column 0 stays, column 1 goes down 1, column 2 down 2. */
    CHECK (img->width == 3 && img->height == 4);
    CHECK (easel_image_get (img, 0, 0) == A);
    CHECK (easel_image_get (img, 1, 1) == B);
    CHECK (easel_image_get (img, 1, 2) == C);
    CHECK (easel_image_get (img, 2, 3) == D);
    CHECK (easel_image_get (img, 0, 1) == BG && easel_image_get (img, 2, 0) == BG && easel_image_get (img, 0, 3) == BG);
  }
  easel_image_free (img);
  easel_history_free (h);
}

/* AS-9, REQ-0019-13: a stretch to the same size and no amount is nothing, and adds no step. */
static void
nothing_changes_adds_no_step (void)
{
  static const uint32_t abcd[] = { A, B, C, D };
  EaselHistory *h = easel_history_new ((size_t) 512 << 20);
  EaselImage *img = image_of (2, 2, abcd);

  CHECK (easel_image_stretch_skew (img, h, 2, 2, 0, 0, BG, SIZE_MAX) == EASEL_TRANSFORM_NOTHING);
  CHECK (!easel_history_can_undo (h));
  CHECK (image_is (img, 2, 2, abcd));
  easel_image_free (img);
  easel_history_free (h);
}

/* AS-13 (REQ-0019-14): Undo and Redo give the size and every pixel back; the step costs 4 bytes per old pixel plus 64; the area is dirty. */
static void
as13_undo_and_cost (void)
{
  enum { W = 5, H = 3 };
  uint32_t before[W * H], *after;
  EaselHistory *h = easel_history_new ((size_t) 512 << 20);
  EaselImage *img;
  size_t base = easel_history_bytes (h);
  EaselRect r;

  for (int i = 0; i < W * H; i++)
    before[i] = 0xFF000000u | (uint32_t) (i * 7919);
  img = image_of (W, H, before);
  easel_image_take_dirty (img, &r);
  CHECK (easel_image_stretch_skew (img, h, 10, 6, 3, -2, BG, SIZE_MAX) == EASEL_TRANSFORM_DONE);
  CHECK (easel_history_bytes (h) == base + 4 * W * H + 64);
  CHECK (img->width == 13 && img->height == 8);
  CHECK (easel_image_take_dirty (img, &r));
  CHECK (r.x0 == 0 && r.y0 == 0 && r.x1 == 13 && r.y1 == 8);
  after = malloc (13 * 8 * sizeof *after);
  memcpy (after, img->px, 13 * 8 * sizeof *after);
  CHECK (easel_history_undo (h, img));
  CHECK (image_is (img, W, H, before));
  CHECK (easel_history_redo (h, img));
  CHECK (image_is (img, 13, 8, after));
  free (after);
  easel_image_free (img);
  easel_history_free (h);
}

/* AS-14 (REQ-0019-15): past the limit, and each allocation failing in turn: refused, with the picture and the history exactly as they were. */
static void
as14_refused (void)
{
  static const uint32_t abcd[] = { A, B, C, D };
  EaselHistory *h = easel_history_new ((size_t) 512 << 20);
  EaselImage *img = image_of (2, 2, abcd);
  size_t bytes = easel_history_bytes (h);
  uint32_t *held = img->px;
  int n = 0;

  CHECK (easel_image_stretch_skew (img, h, 100, 100, 7, 7, BG, 1000) == EASEL_TRANSFORM_NO_MEMORY);
  CHECK (img->px == held && image_is (img, 2, 2, abcd));
  CHECK (easel_history_bytes (h) == bytes && !easel_history_can_undo (h));
  for (n = 1; n < 100; n++)
    {
      EaselTransformResult result;

      allocations = 0;
      fail_at = n;
      result = easel_image_stretch_skew (img, h, 100, 100, 7, 7, BG, SIZE_MAX);
      fail_at = 0;
      if (allocations < n)
        {
          CHECK (result == EASEL_TRANSFORM_DONE);       /* every allocation has failed once and now none did */
          break;
        }
      CHECK (result == EASEL_TRANSFORM_NO_MEMORY);
      CHECK (img->px == held && image_is (img, 2, 2, abcd));
      CHECK (easel_history_bytes (h) == bytes && !easel_history_can_undo (h));
    }
  CHECK (n >= 3 && n < 100);                            /* the pixels, the tables and the step were all tried */
  easel_image_free (img);
  easel_history_free (h);
}

static void path_text (const EaselPath *p, char *out, size_t size);

#define R 0xFFFF0000u
#define HOLE 0xFF00FF00u

static EaselImage *
red_picture (int w, int h)
{
  return easel_image_new (w, h, R);
}

/* AS-11 (REQ-0019-12): the shown pixels and the shape are skewed; what no row reaches is not part of it; the hole stays as the lift made
   it and the picture stays under the exposed pixels. */
static void
as11_float_skew (void)
{
  EaselImage *img = red_picture (6, 4);
  EaselHistory *h = easel_history_new ((size_t) 512 << 20);
  EaselFloat f = { 0 };

  easel_image_set (img, 1, 0, A);
  easel_image_set (img, 2, 0, B);
  easel_image_set (img, 1, 1, C);
  easel_image_set (img, 2, 1, D);
  CHECK (easel_selection_lift (&f, img, 1, 0, 2, 2, NULL, false, HOLE, SIZE_MAX) == EASEL_SELECTION_DONE);
  CHECK (easel_selection_skew (&f, 1, 0, SIZE_MAX) == EASEL_SELECTION_DONE);
  CHECK (f.w == 3 && f.h == 2 && f.x == 1 && f.y == 0);
  CHECK (f.mask != NULL);
  {
    static const uint8_t shape[] = { 1, 1, 0, 0, 1, 1 };

    CHECK (memcmp (f.mask, shape, sizeof shape) == 0);
  }
  CHECK (f.px[0] == A && f.px[1] == B && f.px[4] == C && f.px[5] == D);
  {
    EaselFloat again = { 0 };
    EaselPath line = { 0 };
    EaselImage *pasted = image_of (2, 2, (const uint32_t[]) { A, B, C, D });
    char text[128];

    /* The line made before the skew, from the same shape: 2 x 2 at (1, 0) leaning by one. */
    CHECK (easel_selection_paste (&again, pasted, 1, 0, SIZE_MAX) == EASEL_SELECTION_DONE);
    CHECK (easel_lasso_skew_line (&again, 1, 0, &line, SIZE_MAX));
    path_text (&line, text, sizeof text);
    CHECK (strcmp (text, "1 0 3 0 3 1 4 1 4 2 2 2 2 1 1 1 ") == 0);
    easel_path_free (&line);
    easel_float_free (&again);
    easel_image_free (pasted);
  }
  /* Moved away and put down: its hole at (1, 0), 2 x 2, as lifted; the exposed pixels leave what is under them. */
  f.x = 3;
  f.y = 2;
  CHECK (easel_selection_put_down (&f, true, img, h, SIZE_MAX) == EASEL_SELECTION_DONE);
  CHECK (easel_image_get (img, 1, 0) == HOLE && easel_image_get (img, 2, 0) == HOLE);
  CHECK (easel_image_get (img, 1, 1) == HOLE && easel_image_get (img, 2, 1) == HOLE);
  CHECK (easel_image_get (img, 3, 0) == R);
  CHECK (easel_image_get (img, 3, 2) == A && easel_image_get (img, 4, 2) == B);
  CHECK (easel_image_get (img, 5, 2) == R);                 /* exposed: the picture stays */
  CHECK (easel_image_get (img, 3, 3) == R);                 /* exposed */
  CHECK (easel_image_get (img, 4, 3) == C && easel_image_get (img, 5, 3) == D);
  easel_float_free (&f);
  easel_image_free (img);
  easel_history_free (h);
}

/* AS-11: a later stretch starts from the skewed pixels, and the flips and turns start again from none. */
static void
as11_stretch_after_skew (void)
{
  EaselImage *img = red_picture (6, 4);
  EaselFloat f = { 0 };

  easel_image_set (img, 0, 0, A);
  easel_image_set (img, 1, 0, B);
  easel_image_set (img, 0, 1, C);
  easel_image_set (img, 1, 1, D);
  CHECK (easel_selection_lift (&f, img, 0, 0, 2, 2, NULL, false, HOLE, SIZE_MAX) == EASEL_SELECTION_DONE);
  CHECK (easel_selection_transform (&f, EASEL_OP_FLIP_H, SIZE_MAX) == EASEL_SELECTION_DONE);    /* b a over d c */
  CHECK (f.px[0] == B && f.px[1] == A);
  CHECK (easel_selection_skew (&f, 1, 0, SIZE_MAX) == EASEL_SELECTION_DONE);                    /* b a . over . d c */
  CHECK (f.orient == EASEL_ORIENT_IDENTITY && f.orig_px == NULL);
  CHECK (f.px[0] == B && f.px[1] == A && f.px[4] == D && f.px[5] == C);
  CHECK (easel_selection_stretch (&f, f.x, f.y, 6, 2, SIZE_MAX) == EASEL_SELECTION_DONE);
  easel_selection_complete (&f);
  {
    static const uint8_t shape[] = { 1, 1, 1, 1, 0, 0, 0, 0, 1, 1, 1, 1 };

    CHECK (memcmp (f.mask, shape, sizeof shape) == 0);
  }
  CHECK (f.px[0] == B && f.px[1] == B && f.px[2] == A && f.px[3] == A);
  CHECK (f.px[8] == D && f.px[10] == C);
  CHECK (easel_selection_stretch (&f, f.x, f.y, 3, 2, SIZE_MAX) == EASEL_SELECTION_DONE);       /* back: exactly the skewed pixels */
  easel_selection_complete (&f);
  CHECK (f.px[0] == B && f.px[1] == A && f.px[4] == D && f.px[5] == C);
  easel_float_free (&f);
  easel_image_free (img);
}

/* AS-11: a free-form selection's shape is skewed with its pixels, and its hole keeps the shape it was lifted with. */
static void
as11_free_form (void)
{
  static const uint8_t shape[] = { 1, 0, 1, 1 }, skewed[] = { 1, 0, 0, 0, 1, 1 };
  EaselImage *img = red_picture (4, 2);
  EaselHistory *h = easel_history_new ((size_t) 512 << 20);
  EaselFloat f = { 0 };

  easel_image_set (img, 0, 0, A);
  easel_image_set (img, 1, 0, B);
  easel_image_set (img, 0, 1, C);
  easel_image_set (img, 1, 1, D);
  CHECK (easel_selection_lift (&f, img, 0, 0, 2, 2, shape, false, HOLE, SIZE_MAX) == EASEL_SELECTION_DONE);
  CHECK (easel_selection_skew (&f, 1, 0, SIZE_MAX) == EASEL_SELECTION_DONE);
  CHECK (memcmp (f.mask, skewed, sizeof skewed) == 0);
  f.x = 1;
  CHECK (easel_selection_put_down (&f, false, img, h, SIZE_MAX) == EASEL_SELECTION_DONE);       /* the hole alone */
  CHECK (easel_image_get (img, 0, 0) == HOLE && easel_image_get (img, 0, 1) == HOLE && easel_image_get (img, 1, 1) == HOLE);
  CHECK (easel_image_get (img, 1, 0) == B);                 /* outside the shape as lifted */
  easel_float_free (&f);
  easel_image_free (img);
  easel_history_free (h);
}

/* AS-11 (REQ-0019-12): copies left before keep their pixels, with the image as lifted and with a stretched one. */
static void
as11_copies_keep_their_pixels (void)
{
  EaselImage *img = red_picture (10, 6);
  EaselHistory *h = easel_history_new ((size_t) 512 << 20);
  EaselFloat f = { 0 };

  easel_image_set (img, 0, 0, A);
  easel_image_set (img, 1, 0, B);
  CHECK (easel_selection_lift (&f, img, 0, 0, 2, 1, NULL, false, HOLE, SIZE_MAX) == EASEL_SELECTION_DONE);
  f.x = 0;
  f.y = 2;
  CHECK (easel_selection_stamp (&f, SIZE_MAX));                                                 /* as lifted */
  CHECK (easel_selection_stretch (&f, 0, 3, 4, 1, SIZE_MAX) == EASEL_SELECTION_DONE);
  easel_selection_complete (&f);
  CHECK (easel_selection_stamp (&f, SIZE_MAX));                                                 /* stretched */
  CHECK (easel_selection_stretch (&f, 0, 4, 2, 2, SIZE_MAX) == EASEL_SELECTION_DONE);
  easel_selection_complete (&f);
  CHECK (easel_selection_skew (&f, 1, 0, SIZE_MAX) == EASEL_SELECTION_DONE);
  CHECK (easel_selection_put_down (&f, true, img, h, SIZE_MAX) == EASEL_SELECTION_DONE);
  CHECK (easel_image_get (img, 0, 2) == A && easel_image_get (img, 1, 2) == B);
  CHECK (easel_image_get (img, 0, 3) == A && easel_image_get (img, 1, 3) == A);
  CHECK (easel_image_get (img, 2, 3) == B && easel_image_get (img, 3, 3) == B);
  CHECK (easel_image_get (img, 0, 4) == A && easel_image_get (img, 1, 4) == B);
  CHECK (easel_image_get (img, 2, 4) == R);                 /* exposed */
  CHECK (easel_image_get (img, 0, 5) == R);                 /* exposed */
  CHECK (easel_image_get (img, 1, 5) == A && easel_image_get (img, 2, 5) == B);
  easel_float_free (&f);
  easel_image_free (img);
  easel_history_free (h);
}

/* AS-11: a copy made with the image as lifted, and one made with the image shown at the skew, each keep their own pixels. */
static void
as11_copies_differ_from_the_skewed (void)
{
  EaselImage *img = red_picture (10, 6);
  EaselHistory *h = easel_history_new ((size_t) 512 << 20);
  EaselFloat f = { 0 };

  easel_image_set (img, 0, 0, A);
  easel_image_set (img, 1, 0, B);
  easel_image_set (img, 0, 1, C);
  easel_image_set (img, 1, 1, D);
  CHECK (easel_selection_lift (&f, img, 0, 0, 2, 2, NULL, false, HOLE, SIZE_MAX) == EASEL_SELECTION_DONE);
  f.x = 4;
  CHECK (easel_selection_stamp (&f, SIZE_MAX));                                                 /* with the image as lifted */
  CHECK (easel_selection_stretch (&f, 7, 0, 2, 1, SIZE_MAX) == EASEL_SELECTION_DONE);           /* one row: c d */
  easel_selection_complete (&f);
  CHECK (easel_selection_stretch (&f, 7, 3, 2, 2, SIZE_MAX) == EASEL_SELECTION_DONE);           /* back to the lifted size */
  easel_selection_complete (&f);
  f.x = 0;
  f.y = 4;
  CHECK (easel_selection_stamp (&f, SIZE_MAX));                                                 /* with the image shown at the skew */
  CHECK (easel_selection_skew (&f, 1, 0, SIZE_MAX) == EASEL_SELECTION_DONE);
  CHECK (easel_selection_put_down (&f, false, img, h, SIZE_MAX) == EASEL_SELECTION_DONE);
  CHECK (easel_image_get (img, 4, 0) == A && easel_image_get (img, 5, 0) == B);                 /* as lifted */
  CHECK (easel_image_get (img, 4, 1) == C && easel_image_get (img, 5, 1) == D);
  CHECK (easel_image_get (img, 0, 4) == A && easel_image_get (img, 1, 4) == B);                 /* not skewed */
  CHECK (easel_image_get (img, 0, 5) == C && easel_image_get (img, 1, 5) == D);
  CHECK (easel_image_get (img, 2, 5) == R);
  easel_float_free (&f);
  easel_image_free (img);
  easel_history_free (h);
}

/* AS-11: a paste's fully transparent pixels stay not put down after a skew. */
static void
as11_paste (void)
{
  static const uint32_t pasted[] = { A, 0x00000000u, C, D };
  EaselImage *pic = image_of (2, 2, pasted), *img = red_picture (4, 3);
  EaselHistory *h = easel_history_new ((size_t) 512 << 20);
  EaselFloat f = { 0 };

  CHECK (easel_selection_paste (&f, pic, 0, 0, SIZE_MAX) == EASEL_SELECTION_DONE);
  CHECK (easel_selection_skew (&f, 1, 0, SIZE_MAX) == EASEL_SELECTION_DONE);
  CHECK (easel_selection_put_down (&f, true, img, h, SIZE_MAX) == EASEL_SELECTION_DONE);
  CHECK (easel_image_get (img, 0, 0) == A);
  CHECK (easel_image_get (img, 1, 0) == R);                 /* transparent: not put down */
  CHECK (easel_image_get (img, 0, 1) == R);                 /* exposed */
  CHECK (easel_image_get (img, 1, 1) == C && easel_image_get (img, 2, 1) == D);
  easel_float_free (&f);
  easel_image_free (pic);
  easel_image_free (img);
  easel_history_free (h);
}

/* AS-14 (REQ-0019-15): a skew past the limit, and each allocation failing in turn: the float as it was. No amount: nothing, no change. */
static void
as14_float_refused (void)
{
  static const uint8_t shape[] = { 1, 0, 1, 1 };
  EaselImage *img = red_picture (6, 6);
  EaselFloat f = { 0 };
  uint32_t *held;
  int n;

  CHECK (easel_selection_lift (&f, img, 0, 0, 2, 2, shape, false, HOLE, SIZE_MAX) == EASEL_SELECTION_DONE);
  f.x = 2;
  CHECK (easel_selection_stamp (&f, SIZE_MAX));
  CHECK (easel_selection_stretch (&f, 2, 2, 4, 4, SIZE_MAX) == EASEL_SELECTION_DONE);
  easel_selection_complete (&f);
  held = f.px;
  CHECK (easel_selection_skew (&f, 0, 0, SIZE_MAX) == EASEL_SELECTION_NOTHING);
  CHECK (f.px == held);
  CHECK (easel_selection_skew (&f, 1, 0, 8) == EASEL_SELECTION_NO_MEMORY);
  CHECK (f.px == held && f.w == 4 && f.h == 4 && f.orig_px != NULL);
  for (n = 1; n < 100; n++)
    {
      EaselSelectionResult result;

      allocations = 0;
      fail_at = n;
      result = easel_selection_skew (&f, 1, 0, SIZE_MAX);
      fail_at = 0;
      if (allocations < n)
        {
          CHECK (result == EASEL_SELECTION_DONE);
          break;
        }
      CHECK (result == EASEL_SELECTION_NO_MEMORY);
      CHECK (f.px == held && f.w == 4 && f.h == 4 && f.orig_px != NULL && f.hole_fixed == false);
    }
  CHECK (n >= 3 && n < 100);
  easel_float_free (&f);
  easel_image_free (img);
}

/* The path as "x y x y | x y ..." with | at each break, to compare with what is expected. */
static void
path_text (const EaselPath *p, char *out, size_t size)
{
  size_t used = 0;

  out[0] = '\0';
  for (size_t i = 0; i < p->n; i++)
    used += (size_t) (p->xy[2 * i] == EASEL_PATH_BREAK ? snprintf (out + used, size - used, "| ")
                                                       : snprintf (out + used, size - used, "%d %d ", p->xy[2 * i], p->xy[2 * i + 1]));
}

static void
check_outline (const uint8_t *mask, int w, int h, int x, int y, const char *expected)
{
  EaselPath p = { 0 };
  char text[512];

  CHECK (easel_lasso_outline (mask, w, h, x, y, &p, SIZE_MAX));
  path_text (&p, text, sizeof text);
  if (strcmp (text, expected) != 0)
    fprintf (stderr, "outline: %s\nexpected: %s\n", text, expected);
  CHECK (strcmp (text, expected) == 0);
  easel_path_free (&p);
}

/* AS-11 (REQ-0019-12): the outline along the grid lines, one closed line for each part. */
static void
as11_outline_parts (void)
{
  static const uint8_t one[] = { 1 }, square[] = { 1, 1, 1, 1 }, ell[] = { 1, 0, 1, 1 };
  static const uint8_t diagonal[] = { 1, 0, 0, 1 };
  /* 3 x 2 skewed by 5: the rows at x 0 to 2 and 5 to 7. */
  static const uint8_t split[] = { 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1 };
  static const uint8_t ring[] = { 1, 1, 1, 1, 0, 1, 1, 1, 1 };
  /* A skewed 2 x 2: the rows overlap in one pixel, so it is one part. */
  static const uint8_t leaning[] = { 1, 1, 0, 0, 1, 1 };

  check_outline (one, 1, 1, 0, 0, "0 0 1 0 1 1 0 1 ");
  check_outline (square, 2, 2, 3, 4, "3 4 5 4 5 6 3 6 ");
  check_outline (ell, 2, 2, 0, 0, "0 0 1 0 1 1 2 1 2 2 0 2 ");
  check_outline (split, 8, 2, 0, 0, "0 0 3 0 3 1 0 1 | 5 1 8 1 8 2 5 2 ");
  check_outline (diagonal, 2, 2, 0, 0, "0 0 1 0 1 1 0 1 | 1 1 2 1 2 2 1 2 ");       /* touching at a corner: two */
  check_outline (ring, 3, 3, 0, 0, "0 0 3 0 3 3 0 3 | 1 2 2 2 2 1 1 1 ");           /* the hole is a line of its own */
  check_outline (leaning, 3, 2, -1, -2, "-1 -2 1 -2 1 -1 2 -1 2 0 0 0 0 -1 -1 -1 ");
}

/* R1: a line of several parts through the readers of a path: each corner moves, the break stays, and a part can be found. */
static void
as11_outline_readers (void)
{
  static const uint8_t split[] = { 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1 };
  EaselPath p = { 0 }, q = { 0 };
  char text[512];
  size_t at, n;

  CHECK (easel_lasso_outline (split, 8, 2, 0, 0, &p, SIZE_MAX));
  CHECK (easel_lasso_scale_path (&p, 0, 0, 8, 2, 0, 0, 16, 4, &q, SIZE_MAX));
  path_text (&q, text, sizeof text);
  CHECK (strcmp (text, "0 0 6 0 6 2 0 2 | 10 2 16 2 16 4 10 4 ") == 0);
  easel_path_free (&q);
  CHECK (easel_lasso_orient_path (&p, 0, 0, 8, 2, EASEL_ORIENT_FLIP_X, &q, SIZE_MAX));
  path_text (&q, text, sizeof text);
  CHECK (strcmp (text, "8 0 5 0 5 1 8 1 | 3 1 0 1 0 2 3 2 ") == 0);
  easel_path_free (&q);
  at = 0;
  CHECK (easel_path_next_part (&p, &at, &n) == p.xy && n == 4);
  CHECK (easel_path_next_part (&p, &at, &n) == p.xy + 2 * 5 && n == 4);
  CHECK (easel_path_next_part (&p, &at, &n) == NULL);
  easel_path_free (&p);
  /* A traced path, with no break, is one part. */
  CHECK (easel_path_add (&q, 0, 0, SIZE_MAX) && easel_path_add (&q, 1, 0, SIZE_MAX));
  at = 0;
  CHECK (easel_path_next_part (&q, &at, &n) == q.xy && n == 2);
  CHECK (easel_path_next_part (&q, &at, &n) == NULL);
  easel_path_free (&q);
}

/* AS-14: past the limit, or an allocation failing, the outline leaves out untouched. */
static void
as14_outline_refused (void)
{
  static const uint8_t square[] = { 1, 1, 1, 1 };
  EaselPath p = { 0 };
  int n;

  CHECK (!easel_lasso_outline (square, 2, 2, 0, 0, &p, 1));
  CHECK (p.xy == NULL && p.n == 0);
  for (n = 1; n < 20; n++)
    {
      bool done;

      allocations = 0;
      fail_at = n;
      done = easel_lasso_outline (square, 2, 2, 0, 0, &p, SIZE_MAX);
      fail_at = 0;
      if (allocations < n)
        {
          CHECK (done);
          break;
        }
      CHECK (!done && p.xy == NULL && p.n == 0);
    }
  CHECK (n >= 2 && n < 20);
  easel_path_free (&p);
}

int
main (void)
{
  RUN (as4_percent_and_conversion);
  RUN (as5_ratio);
  RUN (as9_shifts);
  RUN (as9_degrees);
  RUN (as8_order);
  RUN (as8_stretch_nearest);
  RUN (as9_skew_pixels_asymmetric);
  RUN (nothing_changes_adds_no_step);
  RUN (as13_undo_and_cost);
  RUN (as14_refused);
  RUN (as11_float_skew);
  RUN (as11_stretch_after_skew);
  RUN (as11_free_form);
  RUN (as11_copies_keep_their_pixels);
  RUN (as11_copies_differ_from_the_skewed);
  RUN (as11_paste);
  RUN (as14_float_refused);
  RUN (as11_outline_parts);
  RUN (as11_outline_readers);
  RUN (as14_outline_refused);
  return 0;
}
