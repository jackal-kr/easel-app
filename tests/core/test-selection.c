/* SPDX-License-Identifier: GPL-3.0-or-later */
/* The floating selection of feature 0013: lifting it (REQ-0013-07, -08), laying it over the picture and putting it down as
   one undo step (REQ-0013-11). */

#include <limits.h>
#include <malloc.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/resource.h>

#include "check.h"
#include "history.h"
#include "image.h"
#include "line.h"
#include "selection.h"
#include "transform.h"

#define WHITE 0xFFFFFFFFu
#define RED 0xFFFF0000u
#define YELLOW 0xFFFFFF00u
#define BLUE 0xFF0000FFu

/* The picture of the scenarios: 21 x 11 white, a red block at the columns 2 to 4 of the rows 2 and 3. */
static EaselImage *
scenario_picture (void)
{
  EaselImage *img = easel_image_new (21, 11, WHITE);

  CHECK (img != NULL);
  for (int y = 2; y <= 3; y++)
    for (int x = 2; x <= 4; x++)
      img->px[y * 21 + x] = RED;
  return img;
}

static EaselImage *
copy_of (const EaselImage *img)
{
  EaselImage *c = easel_image_new (img->width, img->height, 0);

  CHECK (c != NULL);
  memcpy (c->px, img->px, (size_t) img->width * img->height * sizeof img->px[0]);
  return c;
}

static int
differing (const EaselImage *a, const EaselImage *b)
{
  int n = 0;

  for (int i = 0; i < a->width * a->height; i++)
    n += a->px[i] != b->px[i];
  return n;
}

/* AS-3, the core part: (1, 1) to (5, 4) lifted with a yellow background and moved by 10 columns and 3 rows, then put down:
   26 pixels changed, the red block at the columns 12 to 14 of the rows 5 and 6, one step; Undo restores every pixel. Moved
   back onto its place, it changes nothing and adds no step. */
static void
as3_move_put_down (void)
{
  EaselImage *img = scenario_picture (), *before = copy_of (img);
  EaselHistory *h = easel_history_new (16 * 1024 * 1024);
  EaselFloat f = { 0 };

  CHECK (h != NULL);
  CHECK (easel_selection_lift (&f, img, 1, 1, 5, 4, NULL, false, YELLOW, SIZE_MAX) == EASEL_SELECTION_DONE);
  CHECK (f.w == 5 && f.h == 4 && f.x == 1 && f.y == 1 && f.hole);
  CHECK (differing (img, before) == 0);                       /* the picture is not touched while it floats */
  f.x += 10;
  f.y += 3;
  CHECK (easel_selection_put_down (&f, true, img, h, SIZE_MAX) == EASEL_SELECTION_DONE);
  CHECK (differing (img, before) == 26);
  for (int y = 0; y < 11; y++)
    for (int x = 0; x < 21; x++)
      {
        uint32_t want = before->px[y * 21 + x];

        if (x >= 1 && x <= 5 && y >= 1 && y <= 4)
          want = YELLOW;
        if (x >= 11 && x <= 15 && y >= 4 && y <= 7)
          want = (x >= 12 && x <= 14 && y >= 5 && y <= 6) ? RED : WHITE;
        CHECK (img->px[y * 21 + x] == want);
      }
  CHECK (easel_history_can_undo (h));
  CHECK (easel_history_undo (h, img));
  CHECK (differing (img, before) == 0);
  CHECK (!easel_history_can_undo (h));
  easel_float_free (&f);
  /* Moved and back: nothing differs, no step. */
  CHECK (easel_selection_lift (&f, img, 1, 1, 5, 4, NULL, false, YELLOW, SIZE_MAX) == EASEL_SELECTION_DONE);
  f.x += 4;
  f.x -= 4;
  CHECK (easel_selection_put_down (&f, true, img, h, SIZE_MAX) == EASEL_SELECTION_NOTHING);
  CHECK (differing (img, before) == 0);
  CHECK (!easel_history_can_undo (h));
  easel_float_free (&f);
  easel_history_free (h);
  easel_image_free (before);
  easel_image_free (img);
}

/* The lay of a rectangle partly outside the picture writes only the places of picture pixels. */
static void
lay_clips (void)
{
  enum { RW = 12, RH = 9 };
  EaselImage *img = scenario_picture ();
  EaselFloat f = { 0 };
  uint32_t buf[RW * RH];

  CHECK (easel_selection_lift (&f, img, 1, 1, 5, 4, NULL, false, YELLOW, SIZE_MAX) == EASEL_SELECTION_DONE);
  for (int i = 0; i < RW * RH; i++)
    buf[i] = 0x12345678u;
  easel_selection_lay (&f, true, img, -3, -2, RW, RH, buf, RW);
  for (int j = 0; j < RH; j++)
    for (int i = 0; i < RW; i++)
      {
        int x = i - 3, y = j - 2;

        if (x < 0 || y < 0)
          CHECK (buf[j * RW + i] == 0x12345678u);
        else
          CHECK (buf[j * RW + i] != 0x12345678u);
      }
  easel_float_free (&f);
  easel_image_free (img);
}

/* ---- REQ-0013-08 and -11 again, in the test's own words: the hole in the color of the lift, then the float's pixels where
   it is, each replacing what is under it, only inside the picture ---- */

/* The lifted pixels written at (px, py), each replacing what is under it (REQ-0013-11, -13: in both modes). */
static void
reference_place (const EaselImage *before, EaselImage *out, int sx, int sy, int w, int h, int px, int py)
{
  for (int j = 0; j < h; j++)
    for (int i = 0; i < w; i++)
      {
        int x = px + i, y = py + j;

        if (x >= 0 && y >= 0 && x < out->width && y < out->height)
          out->px[y * out->width + x] = before->px[(sy + j) * before->width + sx + i];
      }
}

static void
reference_put_down (const EaselImage *before, EaselImage *out, int sx, int sy, int w, int h, bool hole, uint32_t hole_color,
                    const EaselStamp *stamps, int n_stamps, int fx, int fy)
{
  memcpy (out->px, before->px, (size_t) before->width * before->height * sizeof before->px[0]);
  if (hole)
    for (int y = sy; y < sy + h; y++)
      for (int x = sx; x < sx + w; x++)
        out->px[y * out->width + x] = hole_color;
  for (int k = 0; k < n_stamps; k++)
    reference_place (before, out, sx, sy, w, h, stamps[k].x, stamps[k].y);
  reference_place (before, out, sx, sy, w, h, fx, fy);
}

static void
lay_matches_put_down (void)
{
  unsigned int seed = 13;

  for (int n = 0; n < 2000; n++)
    {
      int W, H, sx, sy, w, h, fx, fy, n_stamps;
      EaselImage *img, *before, *ref;
      EaselHistory *hist = easel_history_new (64 * 1024 * 1024);
      EaselFloat f = { 0 };
      EaselStamp stamps[3];
      uint32_t *laid, hole;
      bool copy;

#define RND(m) (seed = seed * 1103515245u + 12345u, (int) ((seed >> 8) % (unsigned) (m)))
      W = 1 + RND (17);
      H = 1 + RND (13);
      img = easel_image_new (W, H, 0);
      CHECK (img != NULL && hist != NULL);
      for (int i = 0; i < W * H; i++)
        img->px[i] = (uint32_t) RND (4) * 0x40000000u | (uint32_t) RND (3) * 0x00550000u | (uint32_t) RND (2) * 0xFFu;
      sx = RND (W);
      sy = RND (H);
      w = 1 + RND (W - sx);
      h = 1 + RND (H - sy);
      hole = n % 4 == 0 ? 0x00000000u : (uint32_t) RND (2) * 0x80000000u | 0x00123456u | 0xFF000000u * (n % 2);
      copy = RND (3) == 0;
      n_stamps = RND (4);
      before = copy_of (img);
      ref = copy_of (img);
      CHECK (easel_selection_lift (&f, img, sx, sy, w, h, NULL, copy, hole, SIZE_MAX) == EASEL_SELECTION_DONE);
      for (int k = 0; k < n_stamps; k++)
        {
          stamps[k] = (EaselStamp) { sx + RND (2 * W + 1) - W, sy + RND (2 * H + 1) - H };
          f.x = stamps[k].x;
          f.y = stamps[k].y;
          CHECK (easel_selection_stamp (&f, SIZE_MAX));
        }
      fx = sx + RND (2 * W + 1) - W;
      fy = sy + RND (2 * H + 1) - H;
      f.x = fx;
      f.y = fy;
      reference_put_down (before, ref, sx, sy, w, h, !copy, hole, stamps, n_stamps, fx, fy);
      laid = malloc ((size_t) W * H * sizeof *laid);
      CHECK (laid != NULL);
      easel_selection_lay (&f, true, img, 0, 0, W, H, laid, (size_t) W);
      for (int i = 0; i < W * H; i++)
        CHECK (laid[i] == ref->px[i]);                     /* the preview is the put-down */
      CHECK (easel_selection_put_down (&f, true, img, hist, SIZE_MAX)
             == (differing (ref, before) ? EASEL_SELECTION_DONE : EASEL_SELECTION_NOTHING));
      CHECK (differing (img, ref) == 0);
      if (easel_history_can_undo (hist))
        {
          CHECK (easel_history_undo (hist, img));
          CHECK (differing (img, before) == 0);
        }
#undef RND
      free (laid);
      easel_float_free (&f);
      easel_history_free (hist);
      easel_image_free (ref);
      easel_image_free (before);
      easel_image_free (img);
    }
}

static int
count (const EaselImage *img, uint32_t c)
{
  int n = 0;

  for (int i = 0; i < img->width * img->height; i++)
    n += img->px[i] == c;
  return n;
}

/* AS-8, the core part (REQ-0013-08, -13): the columns 11 to 15 of the rows 4 to 7 blue, (1, 1) to (5, 4) lifted and moved onto
   them: its white pixels keep their color and cover the blue, so no blue remains; the hole takes the color the lift was given:
   white in Opaque mode, fully transparent in Transparent mode; a half-transparent pixel replaces what is under it. */
static void
as8_hole_and_pixels (void)
{
  static const uint32_t holes[] = { WHITE, 0x00000000u };

  for (size_t c = 0; c < sizeof holes / sizeof holes[0]; c++)
    {
      EaselImage *img = scenario_picture ();
      EaselFloat f = { 0 };

      for (int y = 4; y <= 7; y++)
        for (int x = 11; x <= 15; x++)
          img->px[y * 21 + x] = BLUE;
      img->px[2 * 21 + 3] = 0x80FF0000u;
      CHECK (easel_selection_lift (&f, img, 1, 1, 5, 4, NULL, false, holes[c], SIZE_MAX) == EASEL_SELECTION_DONE);
      f.x += 10;
      f.y += 3;
      CHECK (easel_selection_put_down (&f, true, img, NULL, SIZE_MAX) == EASEL_SELECTION_DONE);
      CHECK (count (img, BLUE) == 0);
      CHECK (img->px[1 * 21 + 1] == holes[c]);                  /* the hole */
      CHECK (img->px[5 * 21 + 13] == 0x80FF0000u);             /* the half-transparent pixel, replacing the white under it */
      CHECK (img->px[4 * 21 + 11] == WHITE);                    /* a white pixel of the selection over the blue */
      easel_float_free (&f);
      easel_image_free (img);
    }
}

/* AS-4, the core part: a Ctrl lift leaves no hole; a stamp at (11, 4) then the float moved to (11, 7) and put down: the red
   block at the rows 2, 3 (the original), 5, 6 (the stamp) and 8, 9 (the float) of the columns 2 to 4 and 12 to 14, one step;
   Undo restores every pixel. */
static void
as4_stamps (void)
{
  EaselImage *img = scenario_picture (), *before = copy_of (img);
  EaselHistory *h = easel_history_new (16 * 1024 * 1024);
  EaselFloat f = { 0 };

  CHECK (h != NULL);
  CHECK (easel_selection_lift (&f, img, 1, 1, 5, 4, NULL, true, YELLOW, SIZE_MAX) == EASEL_SELECTION_DONE);
  CHECK (!f.hole);
  f.x = 11;
  f.y = 4;
  CHECK (easel_selection_stamp (&f, SIZE_MAX));
  f.y = 7;
  CHECK (easel_selection_put_down (&f, true, img, h, SIZE_MAX) == EASEL_SELECTION_DONE);
  for (int y = 0; y < 11; y++)
    for (int x = 0; x < 21; x++)
      {
        bool red = (x >= 2 && x <= 4 && (y == 2 || y == 3)) || (x >= 12 && x <= 14 && (y == 5 || y == 6 || y == 8 || y == 9));

        CHECK (img->px[y * 21 + x] == (red ? RED : WHITE));
      }
  CHECK (easel_history_undo (h, img));
  CHECK (differing (img, before) == 0);
  CHECK (!easel_history_can_undo (h));
  easel_float_free (&f);
  easel_history_free (h);
  easel_image_free (before);
  easel_image_free (img);
}

/* AS-9, the core part: Clear on a marked selection fills it with the background color now, one step (none if no pixel
   changes); on a float, the put-down without the float: its hole and its copies stay. */
static void
as9_clear (void)
{
  EaselImage *img = scenario_picture (), *before = copy_of (img);
  EaselHistory *h = easel_history_new (16 * 1024 * 1024);
  EaselFloat f = { 0 };

  CHECK (h != NULL);
  CHECK (easel_selection_clear (img, h, 1, 1, 5, 4, NULL, YELLOW, SIZE_MAX) == EASEL_SELECTION_DONE);
  CHECK (differing (img, before) == 20 && count (img, YELLOW) == 20);
  CHECK (easel_history_undo (h, img) && differing (img, before) == 0);
  CHECK (easel_selection_clear (img, h, 10, 6, 3, 3, NULL, WHITE, SIZE_MAX) == EASEL_SELECTION_NOTHING);
  CHECK (!easel_history_can_undo (h));
  /* A float lifted (a hole) and moved: cleared, the hole stays, nothing of it is put down. */
  CHECK (easel_selection_lift (&f, img, 1, 1, 5, 4, NULL, false, YELLOW, SIZE_MAX) == EASEL_SELECTION_DONE);
  f.x += 10;
  CHECK (easel_selection_put_down (&f, false, img, h, SIZE_MAX) == EASEL_SELECTION_DONE);
  CHECK (count (img, YELLOW) == 20 && count (img, RED) == 0 && differing (img, before) == 20);
  CHECK (easel_history_undo (h, img) && differing (img, before) == 0);
  easel_float_free (&f);
  /* A float lifted with Ctrl, a copy left at (11, 4), then cleared: the copy stays, no hole. */
  CHECK (easel_selection_lift (&f, img, 1, 1, 5, 4, NULL, true, YELLOW, SIZE_MAX) == EASEL_SELECTION_DONE);
  f.x = 11;
  f.y = 4;
  CHECK (easel_selection_stamp (&f, SIZE_MAX));
  f.y = 7;
  CHECK (easel_selection_put_down (&f, false, img, h, SIZE_MAX) == EASEL_SELECTION_DONE);
  CHECK (count (img, YELLOW) == 0 && count (img, RED) == 12);
  easel_float_free (&f);
  /* After a Ctrl lift and nothing else, Clear changes nothing: no step. */
  CHECK (easel_history_undo (h, img) && differing (img, before) == 0);
  CHECK (easel_selection_lift (&f, img, 1, 1, 5, 4, NULL, true, YELLOW, SIZE_MAX) == EASEL_SELECTION_DONE);
  CHECK (easel_selection_put_down (&f, false, img, h, SIZE_MAX) == EASEL_SELECTION_NOTHING);
  easel_float_free (&f);
  easel_history_free (h);
  easel_image_free (before);
  easel_image_free (img);
}

/* The size of the process's address space now, in bytes. */
static size_t
address_space (void)
{
  unsigned long pages = 0;
  FILE *fp = fopen ("/proc/self/statm", "r");

  CHECK (fp != NULL);
  CHECK (fscanf (fp, "%lu", &pages) == 1);
  fclose (fp);
  return (size_t) pages * 4096;
}

/* AS-13, the core part (REQ-0013-19): a lift, a stamp, a put-down and a clear that cannot get their memory refuse with nothing
   changed (a cap, and a real allocation failure); a step larger than the whole budget is kept as the only step. */
static void
as13_selection_memory (void)
{
  enum { N = 1800 };
  EaselImage *img = scenario_picture (), *before = copy_of (img), *big;
  EaselHistory *h = easel_history_new (16 * 1024 * 1024);
  EaselFloat f = { 0 };
  struct rlimit old, lim;

  CHECK (h != NULL);
  /* The lift over its cap: the float stays empty, the picture as it was. */
  CHECK (easel_selection_lift (&f, img, 1, 1, 5, 4, NULL, false, YELLOW, 79) == EASEL_SELECTION_NO_MEMORY);
  CHECK (f.px == NULL && f.w == 0 && !f.hole);
  CHECK (easel_selection_lift (&f, img, 1, 1, 5, 4, NULL, false, YELLOW, 80) == EASEL_SELECTION_DONE);
  /* A stamp over its cap: nothing added. */
  CHECK (!easel_selection_stamp (&f, 10));
  CHECK (f.n_stamps == 0);
  /* The put-down over its cap: the picture as before the lift, no step. */
  f.x += 10;
  CHECK (easel_selection_put_down (&f, true, img, h, 100) == EASEL_SELECTION_NO_MEMORY);
  CHECK (differing (img, before) == 0 && !easel_history_can_undo (h));
  easel_float_free (&f);
  /* The clear over its cap: nothing changes. */
  CHECK (easel_selection_clear (img, h, 1, 1, 5, 4, NULL, YELLOW, 100) == EASEL_SELECTION_NO_MEMORY);
  CHECK (differing (img, before) == 0 && !easel_history_can_undo (h));
  /* A real failure: the address space limited to a little above what is in use, a lift of 13 MB. */
  big = easel_image_new (N, N, WHITE);
  CHECK (big != NULL);
  CHECK (getrlimit (RLIMIT_AS, &old) == 0);
  lim = old;
  lim.rlim_cur = address_space () + 4 * 1024 * 1024;
  CHECK (setrlimit (RLIMIT_AS, &lim) == 0);
  CHECK (easel_selection_lift (&f, big, 0, 0, N, N, NULL, false, YELLOW, SIZE_MAX) == EASEL_SELECTION_NO_MEMORY);
  CHECK (setrlimit (RLIMIT_AS, &old) == 0);
  CHECK (f.px == NULL);
  /* With memory: the whole picture moved by one column, a step of 3.24 million pixels (38.9 MB) against 16 MB: the only one. */
  for (int y = 0; y < N; y++)
    for (int x = 0; x < N; x++)
      big->px[y * N + x] = (x & 1) ? RED : WHITE;
  CHECK (easel_selection_lift (&f, big, 0, 0, N, N, NULL, false, YELLOW, SIZE_MAX) == EASEL_SELECTION_DONE);
  f.x += 1;
  {
    /* A cap that lets the log hold 1024 changes and then fail: the pixels already written are put back, for a put-down and
       for a clear. */
    EaselImage *striped = copy_of (big);

    CHECK (easel_selection_put_down (&f, true, big, h, 1024 * 12) == EASEL_SELECTION_NO_MEMORY);
    CHECK (differing (big, striped) == 0 && !easel_history_can_undo (h));
    CHECK (easel_selection_clear (big, h, 0, 0, N, N, NULL, YELLOW, 1024 * 12) == EASEL_SELECTION_NO_MEMORY);
    CHECK (differing (big, striped) == 0 && !easel_history_can_undo (h));
    easel_image_free (striped);
  }
  CHECK (easel_selection_put_down (&f, true, big, h, SIZE_MAX) == EASEL_SELECTION_DONE);
  CHECK (easel_history_bytes (h) == (size_t) N * N * 12 + 64 && easel_history_bytes (h) > 16 * 1024 * 1024);
  CHECK (easel_history_undo (h, big));
  CHECK (!easel_history_can_undo (h));
  easel_float_free (&f);
  easel_history_free (h);
  easel_image_free (big);
  easel_image_free (before);
  easel_image_free (img);
}

/* The heap memory in use now, as glibc counts it: the small blocks and the mapped ones. */
static size_t
heap_in_use (void)
{
  struct mallinfo2 mi = mallinfo2 ();

  return mi.uordblks + mi.hblkhd;
}

/* REQ-0002-16, REQ-0013-11: the memory a put-down's step holds is the memory the budget counts. A selection of 513 x 512
   pixels moved by one column changes just more than a power of two of pixels, so the log's spare half must not stay. */
static void
as13_step_holds_what_it_counts (void)
{
  EaselImage *img = easel_image_new (600, 600, WHITE);
  EaselHistory *h = easel_history_new ((size_t) 512 << 20);
  EaselFloat f = { 0 };
  size_t before, held, counted;

  CHECK (img != NULL && h != NULL);
  for (int y = 0; y < 600; y++)
    for (int x = 0; x < 600; x++)
      img->px[y * 600 + x] = (x & 1) ? RED : WHITE;           /* stripes, so that a move by one column changes every pixel */
  CHECK (easel_selection_lift (&f, img, 0, 0, 513, 512, NULL, false, YELLOW, SIZE_MAX) == EASEL_SELECTION_DONE);
  f.x += 1;
  before = heap_in_use ();
  CHECK (easel_selection_put_down (&f, true, img, h, SIZE_MAX) == EASEL_SELECTION_DONE);
  held = heap_in_use () - before;
  counted = easel_history_bytes (h);
  printf ("  held %zu bytes, counted %zu\n", held, counted);
  CHECK (held <= counted + counted / 100 + 4096);
  easel_float_free (&f);
  easel_history_free (h);
  easel_image_free (img);
}

/* REQ-0002-16, REQ-0013-14: the same for Clear Selection's step: 513 x 512 stripes cleared change just more than a power of
   two of pixels. */
static void
as13_clear_step_holds_what_it_counts (void)
{
  EaselImage *img = easel_image_new (600, 600, WHITE);
  EaselHistory *h = easel_history_new ((size_t) 512 << 20);
  size_t before, held, counted;

  CHECK (img != NULL && h != NULL);
  for (int y = 0; y < 600; y++)
    for (int x = 0; x < 600; x++)
      img->px[y * 600 + x] = (x & 1) ? RED : WHITE;
  before = heap_in_use ();
  CHECK (easel_selection_clear (img, h, 0, 0, 513, 512, NULL, YELLOW, SIZE_MAX) == EASEL_SELECTION_DONE);
  held = heap_in_use () - before;
  counted = easel_history_bytes (h);
  printf ("  held %zu bytes, counted %zu\n", held, counted);
  CHECK (held <= counted + counted / 100 + 4096);
  easel_history_free (h);
  easel_image_free (img);
}

/* The triangle of the 0014 scenarios: (1, 1) to (7, 1), (1, 2) to (6, 2), ... (1, 7), in the rectangle (1, 1) to (7, 7). */
static uint8_t *
triangle_mask (void)
{
  uint8_t *m = calloc (49, 1);

  CHECK (m != NULL);
  for (int j = 0; j < 7; j++)
    for (int i = 0; i < 7; i++)
      m[j * 7 + i] = i + j <= 6;
  return m;
}

/* AS-4, the core part (REQ-0014-08): the triangle lifted with a yellow background and moved by (10, 3), then put down: 34
   pixels changed (the 28 of the hole, the 6 red ones), the rest of the bounding rectangle untouched; Transparent: the same
   34 with a hole of #00000000; one step. */
static void
as4_masked_move_put_down (void)
{
  uint32_t holes[2] = { YELLOW, 0 };

  for (int c = 0; c < 2; c++)
    {
      EaselImage *img = scenario_picture (), *before;
      EaselHistory *h = easel_history_new (16 * 1024 * 1024);
      EaselFloat f = { 0 };
      uint8_t *mask = triangle_mask ();

      img->px[7 * 21 + 7] = BLUE;                             /* inside the rectangle, outside the shape: it must stay put */
      before = copy_of (img);
      CHECK (h != NULL);
      CHECK (easel_selection_lift (&f, img, 1, 1, 7, 7, mask, false, holes[c], SIZE_MAX) == EASEL_SELECTION_DONE);
      memset (mask, 0, 49);                                   /* the float holds its own copy of the mask */
      free (mask);
      CHECK (differing (img, before) == 0);
      f.x += 10;
      f.y += 3;
      CHECK (easel_selection_put_down (&f, true, img, h, SIZE_MAX) == EASEL_SELECTION_DONE);
      CHECK (differing (img, before) == 34);
      for (int y = 0; y < 11; y++)
        for (int x = 0; x < 21; x++)
          {
            uint32_t want = before->px[y * 21 + x];
            int i = x - 1, j = y - 1;

            if (i >= 0 && j >= 0 && i < 7 && j < 7 && i + j <= 6)
              want = holes[c];
            if (x >= 12 && x <= 14 && y >= 5 && y <= 6)
              want = RED;
            CHECK (img->px[y * 21 + x] == want);
          }
      CHECK (img->px[7 * 21 + 7] == BLUE && img->px[2 * 21 + 7] == WHITE);         /* not lifted, not a hole */
      CHECK (img->px[10 * 21 + 17] == WHITE);                 /* where the float's blue corner would land: not laid */
      CHECK (easel_history_undo (h, img));
      CHECK (differing (img, before) == 0);
      CHECK (!easel_history_can_undo (h));
      easel_float_free (&f);
      easel_history_free (h);
      easel_image_free (before);
      easel_image_free (img);
    }
}

/* The mask's bytes count against the lift's memory limit with the pixels (REQ-0014-08, REQ-0013-19). */
static void
masked_lift_counts_the_mask (void)
{
  EaselImage *img = scenario_picture ();
  EaselFloat f = { 0 };
  uint8_t *mask = triangle_mask ();

  CHECK (easel_selection_lift (&f, img, 1, 1, 7, 7, mask, false, YELLOW, 49 * 4) == EASEL_SELECTION_NO_MEMORY);
  CHECK (f.px == NULL && f.mask == NULL);
  CHECK (easel_selection_lift (&f, img, 1, 1, 7, 7, NULL, false, YELLOW, 49 * 4) == EASEL_SELECTION_DONE && f.mask == NULL);
  easel_float_free (&f);
  CHECK (easel_selection_lift (&f, img, 1, 1, 7, 7, mask, false, YELLOW, 49 * 5) == EASEL_SELECTION_DONE && f.mask != NULL);
  easel_float_free (&f);
  free (mask);
  easel_image_free (img);
}

/* AS-6, the core part (REQ-0014-09): Clear on the marked triangle takes yellow on its 28 pixels only, one step. */
static void
as6_masked_clear (void)
{
  EaselImage *img = scenario_picture (), *before = copy_of (img);
  EaselHistory *h = easel_history_new (16 * 1024 * 1024);
  uint8_t *mask = triangle_mask ();

  CHECK (h != NULL);
  CHECK (easel_selection_clear (img, h, 1, 1, 7, 7, mask, YELLOW, SIZE_MAX) == EASEL_SELECTION_DONE);
  CHECK (differing (img, before) == 28);
  for (int j = 0; j < 7; j++)
    for (int i = 0; i < 7; i++)
      CHECK (img->px[(1 + j) * 21 + 1 + i] == (i + j <= 6 ? YELLOW : before->px[(1 + j) * 21 + 1 + i]));
  CHECK (easel_selection_clear (img, h, 1, 1, 7, 7, mask, YELLOW, SIZE_MAX) == EASEL_SELECTION_NOTHING);
  CHECK (easel_history_undo (h, img));
  CHECK (differing (img, before) == 0);
  CHECK (!easel_history_can_undo (h));
  free (mask);
  easel_history_free (h);
  easel_image_free (before);
  easel_image_free (img);
}

/* REQ-0002-16, REQ-0014-08: the memory the step of a masked put-down and of a masked clear hold is the memory counted. */
static void
as13_masked_step_holds_what_it_counts (void)
{
  uint8_t *mask = malloc ((size_t) 513 * 512);

  CHECK (mask != NULL);
  memset (mask, 1, (size_t) 513 * 512);
  for (int c = 0; c < 2; c++)
    {
      EaselImage *img = easel_image_new (600, 600, WHITE);
      EaselHistory *h = easel_history_new ((size_t) 512 << 20);
      EaselFloat f = { 0 };
      size_t before, held, counted;

      CHECK (img != NULL && h != NULL);
      for (int y = 0; y < 600; y++)
        for (int x = 0; x < 600; x++)
          img->px[y * 600 + x] = (x & 1) ? RED : WHITE;
      if (c == 0)
        {
          CHECK (easel_selection_lift (&f, img, 0, 0, 513, 512, mask, false, YELLOW, SIZE_MAX) == EASEL_SELECTION_DONE);
          f.x += 1;
        }
      before = heap_in_use ();
      if (c == 0)
        CHECK (easel_selection_put_down (&f, true, img, h, SIZE_MAX) == EASEL_SELECTION_DONE);
      else
        CHECK (easel_selection_clear (img, h, 0, 0, 513, 512, mask, YELLOW, SIZE_MAX) == EASEL_SELECTION_DONE);
      held = heap_in_use () - before;
      counted = easel_history_bytes (h);
      printf ("  held %zu bytes, counted %zu\n", held, counted);
      CHECK (held <= counted + counted / 100 + 4096);
      easel_float_free (&f);
      easel_history_free (h);
      easel_image_free (img);
    }
  free (mask);
}

/* A float of the triangle lifted as a copy (no hole) from (1, 1), for the smear. */
static void
lift_triangle_copy (EaselFloat *f, const EaselImage *img)
{
  uint8_t *mask = triangle_mask ();

  CHECK (easel_selection_lift (f, img, 1, 1, 7, 7, mask, true, YELLOW, SIZE_MAX) == EASEL_SELECTION_DONE);
  free (mask);
}

/* AS-8, the core part (REQ-0014-11): the triangle smeared from (1, 1) to (4, 1) leaves a copy at the places (1, 1), (2, 1), (3, 1)
   and floats at (4, 1); put down, the later copies cover the earlier: 12 pixels changed, the red block only at the columns 5 to 7
   of the rows 2 and 3. A jump of 10 pixels in one call leaves the copies of ten calls of one pixel; a diagonal jump leaves one at
   each pixel of the 8-connected line; no movement leaves none. */
static void
as8_smear_trail (void)
{
  EaselImage *img = scenario_picture (), *before = copy_of (img), *other = copy_of (img);
  EaselHistory *h = easel_history_new (16 * 1024 * 1024);
  EaselFloat f = { 0 }, g = { 0 };

  CHECK (h != NULL);
  lift_triangle_copy (&f, img);
  CHECK (easel_selection_smear (&f, 4, 1, SIZE_MAX));
  CHECK (f.x == 4 && f.y == 1 && f.n_stamps == 3);
  for (int i = 0; i < 3; i++)
    CHECK (f.stamps[i].x == 1 + i && f.stamps[i].y == 1);
  CHECK (easel_selection_put_down (&f, true, img, h, SIZE_MAX) == EASEL_SELECTION_DONE);
  CHECK (differing (img, before) == 12);
  for (int y = 2; y <= 3; y++)
    for (int x = 2; x <= 7; x++)
      CHECK (img->px[y * 21 + x] == (x >= 5 ? RED : WHITE));
  easel_float_free (&f);
  /* One jump equals the steps. */
  lift_triangle_copy (&f, other);
  lift_triangle_copy (&g, other);
  CHECK (easel_selection_smear (&f, 11, 1, SIZE_MAX));
  for (int x = 2; x <= 11; x++)
    CHECK (easel_selection_smear (&g, x, 1, SIZE_MAX));
  CHECK (f.n_stamps == 10 && g.n_stamps == 10 && f.x == g.x && f.y == g.y);
  CHECK (memcmp (f.stamps, g.stamps, 10 * sizeof f.stamps[0]) == 0);
  easel_float_free (&f);
  easel_float_free (&g);
  /* A diagonal jump from (1, 1) to (5, 4): the line's pixels but the last. */
  lift_triangle_copy (&f, other);
  CHECK (easel_selection_smear (&f, 5, 4, SIZE_MAX));
  {
    EaselLine l;
    int lx, ly;
    size_t k = 0;

    easel_line_init (&l, 1, 1, 5, 4);
    while (easel_line_next (&l, &lx, &ly))
      if (lx != 5 || ly != 4)
        {
          CHECK (k < f.n_stamps && f.stamps[k].x == lx && f.stamps[k].y == ly);
          k++;
        }
    CHECK (k == f.n_stamps && k == 4 && f.x == 5 && f.y == 4);
  }
  /* No movement: nothing left, nothing moved. */
  CHECK (easel_selection_smear (&f, 5, 4, SIZE_MAX) && f.n_stamps == 4 && f.x == 5 && f.y == 4);
  easel_float_free (&f);
  easel_history_free (h);
  easel_image_free (before);
  easel_image_free (other);
  easel_image_free (img);
}

/* AS-10, the core part (REQ-0014-13): a smear whose copies cannot be had leaves the float where it was with the copies it had: the
   stamps of the call are taken back. */
static void
as10_smear_memory (void)
{
  EaselImage *img = scenario_picture ();
  EaselFloat f = { 0 };

  lift_triangle_copy (&f, img);
  CHECK (easel_selection_stamp (&f, 64) && f.n_stamps == 1);               /* one copy, already there */
  CHECK (!easel_selection_smear (&f, 12, 1, 64));                          /* the 9th copy needs 128 bytes of stamps */
  CHECK (f.n_stamps == 1 && f.x == 1 && f.y == 1 && f.stamps[0].x == 1 && f.stamps[0].y == 1);
  CHECK (easel_selection_smear (&f, 8, 1, 64));                            /* 7 more: 8 in all fit in 64 bytes */
  CHECK (f.n_stamps == 8 && f.x == 8);
  easel_float_free (&f);
  easel_image_free (img);
}

/* The lay of the first design, a check of every pixel against the hole, each stamp and the float: the oracle for the faster one. */
static void
lay_oracle (const EaselFloat *f, bool with_float, const EaselImage *img, int x0, int y0, int w, int h, uint32_t *dst)
{
  for (int j = 0; j < h; j++)
    for (int i = 0; i < w; i++)
      {
        int x = x0 + i, y = y0 + j;
        uint32_t v;

        if (x < 0 || y < 0 || x >= img->width || y >= img->height)
          continue;
        v = img->px[(size_t) y * img->width + x];
        if (f->hole && x >= f->hx && y >= f->hy && x < f->hx + f->w && y < f->hy + f->h
            && (f->mask == NULL || f->mask[(size_t) (y - f->hy) * f->w + (x - f->hx)]))
          v = f->hole_color;
        for (size_t k = 0; k <= f->n_stamps; k++)
          {
            int px = k < f->n_stamps ? f->stamps[k].x : f->x, py = k < f->n_stamps ? f->stamps[k].y : f->y;

            if (k == f->n_stamps && !with_float)
              break;
            if (x >= px && y >= py && x < px + f->w && y < py + f->h
                && (f->mask == NULL || f->mask[(size_t) (y - py) * f->w + (x - px)]))
              v = f->px[(size_t) (y - py) * f->w + (x - px)];
          }
        dst[(size_t) j * w + i] = v;
      }
}

/* The lay equals the oracle for random floats (masks or none, a hole or none, stamps in and out of the picture, overlapping ones) over
   random windows, partly outside the picture, with and without the float. */
static void
lay_matches_the_oracle (void)
{
  srand (7);
  for (int round = 0; round < 300; round++)
    {
      EaselImage *img = easel_image_new (12, 9, 0xFF000000u);
      EaselFloat f = { 0 };
      uint8_t mask[36];
      int fw = 1 + rand () % 6, fh = 1 + rand () % 6, x0 = rand () % 7 - 3, y0 = rand () % 7 - 3, w = 1 + rand () % 16, h = 1 + rand () % 12;
      bool with_mask = rand () % 2;
      uint32_t got[16 * 12], want[16 * 12];

      CHECK (img != NULL);
      for (int i = 0; i < 12 * 9; i++)
        img->px[i] = 0xFF000000u | (uint32_t) rand ();
      for (int i = 0; i < fw * fh; i++)
        mask[i] = rand () % 3 != 0;
      CHECK (easel_selection_lift (&f, img, 2, 2, fw, fh, with_mask ? mask : NULL, rand () % 2, 0xFF123456u, SIZE_MAX) == EASEL_SELECTION_DONE);
      for (int k = rand () % 12; k > 0; k--)
        {
          f.x = rand () % 20 - 6;
          f.y = rand () % 16 - 5;
          CHECK (easel_selection_stamp (&f, SIZE_MAX));
        }
      f.x = rand () % 20 - 6;
      f.y = rand () % 16 - 5;
      for (int with_float = 0; with_float < 2; with_float++)
        {
          for (int i = 0; i < 16 * 12; i++)
            got[i] = want[i] = 0xDEADBEEFu;
          easel_selection_lay (&f, with_float, img, x0, y0, w, h, got, (size_t) w);
          lay_oracle (&f, with_float, img, x0, y0, w, h, want);
          CHECK (memcmp (got, want, sizeof (uint32_t) * (size_t) w * h) == 0);
        }
      easel_float_free (&f);
      easel_image_free (img);
    }
}

/* The rectangle the lay can differ from the picture in: the hole, the copies and the float, clipped to the picture; none if all lie outside. */
static void
bounds_cover_the_changes (void)
{
  EaselImage *img = scenario_picture ();
  EaselFloat f = { 0 };
  int x0, y0, x1, y1;

  CHECK (easel_selection_lift (&f, img, 1, 1, 5, 4, NULL, false, BLUE, SIZE_MAX) == EASEL_SELECTION_DONE);
  CHECK (easel_selection_bounds (&f, true, img, &x0, &y0, &x1, &y1));          /* the float still on the hole */
  CHECK (x0 == 1 && y0 == 1 && x1 == 5 && y1 == 4);
  f.x = 11;
  f.y = 4;
  CHECK (easel_selection_bounds (&f, true, img, &x0, &y0, &x1, &y1));          /* the hole and the float */
  CHECK (x0 == 1 && y0 == 1 && x1 == 15 && y1 == 7);
  CHECK (easel_selection_bounds (&f, false, img, &x0, &y0, &x1, &y1));         /* without the float: the hole only */
  CHECK (x0 == 1 && y0 == 1 && x1 == 5 && y1 == 4);
  f.x = 18;
  f.y = 9;
  CHECK (easel_selection_stamp (&f, SIZE_MAX));
  f.x = -30;
  f.y = -30;
  CHECK (easel_selection_bounds (&f, true, img, &x0, &y0, &x1, &y1));          /* a copy at (18, 9) is clipped to the picture */
  CHECK (x0 == 0 && y0 == 0 && x1 == 20 && y1 == 10);                         /* a union: the float far outside stretches it to the corner */
  easel_float_free (&f);
  /* A copy of a float with no hole, all outside the picture: nothing differs. */
  CHECK (easel_selection_lift (&f, img, 1, 1, 5, 4, NULL, true, BLUE, SIZE_MAX) == EASEL_SELECTION_DONE);
  f.x = -30;
  f.y = -30;
  CHECK (!easel_selection_bounds (&f, true, img, &x0, &y0, &x1, &y1));
  easel_float_free (&f);
  easel_image_free (img);
}

/* The trail of 0014 T023: the lay of the picture and the hole with no copy, then the copies from 0 to a, then from a to b, then from b to
   the end with the float, each over what is there, equals the whole lay, for random floats, splits and windows. */
static void
lay_range_adds_up (void)
{
  srand (11);
  for (int round = 0; round < 300; round++)
    {
      EaselImage *img = easel_image_new (12, 9, 0xFF000000u);
      EaselFloat f = { 0 };
      uint8_t mask[36];
      int fw = 1 + rand () % 6, fh = 1 + rand () % 6, x0 = rand () % 7 - 3, y0 = rand () % 7 - 3, w = 1 + rand () % 16, h = 1 + rand () % 12;
      bool with_mask = rand () % 2;
      uint32_t got[16 * 12], want[16 * 12];
      size_t a, b;

      CHECK (img != NULL);
      for (int i = 0; i < 12 * 9; i++)
        img->px[i] = (uint32_t) rand ();
      for (int i = 0; i < fw * fh; i++)
        mask[i] = rand () % 3 != 0;
      CHECK (easel_selection_lift (&f, img, 2, 2, fw, fh, with_mask ? mask : NULL, rand () % 2, 0x80123456u, SIZE_MAX) == EASEL_SELECTION_DONE);
      for (int k = rand () % 12; k > 0; k--)
        {
          f.x = rand () % 20 - 6;
          f.y = rand () % 16 - 5;
          CHECK (easel_selection_stamp (&f, SIZE_MAX));
        }
      f.x = rand () % 20 - 6;
      f.y = rand () % 16 - 5;
      a = f.n_stamps ? (size_t) rand () % (f.n_stamps + 1) : 0;
      b = a + (f.n_stamps - a ? (size_t) rand () % (f.n_stamps - a + 1) : 0);
      for (int with_float = 0; with_float < 2; with_float++)
        {
          for (int i = 0; i < 16 * 12; i++)
            got[i] = want[i] = 0xDEADBEEFu;
          easel_selection_lay (&f, with_float, img, x0, y0, w, h, want, (size_t) w);
          easel_selection_lay_range (&f, 0, 0, false, img, x0, y0, w, h, got, (size_t) w, true);
          easel_selection_lay_range (&f, 0, a, false, img, x0, y0, w, h, got, (size_t) w, false);
          easel_selection_lay_range (&f, a, b, false, img, x0, y0, w, h, got, (size_t) w, false);
          easel_selection_lay_range (&f, b, f.n_stamps, with_float, img, x0, y0, w, h, got, (size_t) w, false);
          CHECK (memcmp (got, want, sizeof (uint32_t) * (size_t) w * h) == 0);
        }
      easel_float_free (&f);
      easel_image_free (img);
    }
}

/* 0015 T001 (REQ-0015-05, -06): the scale of lifted pixels and shape by the nearest pixel, with the keep-one-pixel rule. */

/* The shape's pixels counted per row of a w x h mask. */
static int
mask_row (const uint8_t *m, int w, int j)
{
  int n = 0;

  for (int i = 0; i < w; i++)
    n += m[(size_t) j * w + i] != 0;
  return n;
}

/* AS-2: the lifted 5 x 4 block (1, 1) to (5, 4) of the scenario picture, red at the local columns 1 to 3 of the rows 1 and 2, made 10
   wide: the new pixel i is the lifted pixel i / 2, so the red is at the columns 2 to 7 of the rows 1 and 2. */
static void
as2_scale_pixels (void)
{
  uint32_t px[5 * 4], *out = NULL;

  for (int j = 0; j < 4; j++)
    for (int i = 0; i < 5; i++)
      px[j * 5 + i] = (j == 1 || j == 2) && i >= 1 && i <= 3 ? RED : WHITE;
  CHECK (easel_selection_scale (px, NULL, 5, 4, 10, 4, &out, NULL, SIZE_MAX) == EASEL_SELECTION_DONE);
  CHECK (out != NULL);
  for (int j = 0; j < 4; j++)
    for (int i = 0; i < 10; i++)
      CHECK (out[j * 10 + i] == ((j == 1 || j == 2) && i >= 2 && i <= 7 ? RED : WHITE));
  free (out);
  /* Shrunk to 3 wide: the new pixel i is the lifted pixel floor((i + 1/2) * 5 / 3), 0, 2 and 4. */
  CHECK (easel_selection_scale (px, NULL, 5, 4, 3, 4, &out, NULL, SIZE_MAX) == EASEL_SELECTION_DONE);
  CHECK (out[1 * 3 + 0] == WHITE && out[1 * 3 + 1] == RED && out[1 * 3 + 2] == WHITE);
  free (out);
}

/* AS-4: the scale is always from the pixels as lifted, so a selection made smaller and then as big as before has its pixels back
   exactly: the original scaled to its own size is the original, whatever sizes the stretches went through. */
static void
as4_scale_back_exactly (void)
{
  uint32_t px[7 * 5], *small = NULL, *again = NULL;

  srand (3);
  for (int i = 0; i < 7 * 5; i++)
    px[i] = (uint32_t) rand () | 0xFF000000u;
  CHECK (easel_selection_scale (px, NULL, 7, 5, 1, 5, &small, NULL, SIZE_MAX) == EASEL_SELECTION_DONE);
  CHECK (easel_selection_scale (px, NULL, 7, 5, 7, 5, &again, NULL, SIZE_MAX) == EASEL_SELECTION_DONE);
  CHECK (memcmp (px, again, sizeof px) == 0);
  free (small);
  free (again);
}

/* AS-7: the shape of the triangle (the 15 pixels i + j <= 4 of its 5 x 5 box) made 10 x 5 has 30 pixels, the rows holding 10, 8, 6,
   4 and 2 from the left; the L (the column 0 of the rows 0 to 3 and the row 3 of the columns 0 to 3 of a 4 x 4 box, each pixel its
   own color) made 1 x 1 keeps one pixel, with the color of its first pixel in reading order, and made 2 x 2 has its 2 lower pixels. */
static void
as7_scale_shape (void)
{
  uint8_t tri[25], el[16], *om = NULL;
  uint32_t tpx[25], epx[16], *op = NULL;
  int total = 0;
  static const int want[5] = { 10, 8, 6, 4, 2 };

  for (int j = 0; j < 5; j++)
    for (int i = 0; i < 5; i++)
      {
        tri[j * 5 + i] = i + j <= 4;
        tpx[j * 5 + i] = RED;
      }
  CHECK (easel_selection_scale (tpx, tri, 5, 5, 10, 5, &op, &om, SIZE_MAX) == EASEL_SELECTION_DONE);
  CHECK (om != NULL);
  for (int j = 0; j < 5; j++)
    {
      CHECK (mask_row (om, 10, j) == want[j]);
      total += mask_row (om, 10, j);
      for (int i = 0; i < want[j]; i++)
        CHECK (om[j * 10 + i] != 0);                           /* from the left */
    }
  CHECK (total == 30);
  free (op);
  free (om);
  for (int j = 0; j < 4; j++)
    for (int i = 0; i < 4; i++)
      {
        el[j * 4 + i] = i == 0 || j == 3;
        epx[j * 4 + i] = 0xFF000000u | (uint32_t) (j * 4 + i + 1);
      }
  CHECK (easel_selection_scale (epx, el, 4, 4, 1, 1, &op, &om, SIZE_MAX) == EASEL_SELECTION_DONE);
  CHECK (om[0] != 0 && op[0] == 0xFF000001u);                    /* the nearest sample, the local (2, 2), is not in the L: the first pixel stays */
  free (op);
  free (om);
  CHECK (easel_selection_scale (epx, el, 4, 4, 2, 2, &op, &om, SIZE_MAX) == EASEL_SELECTION_DONE);
  CHECK (om[0] == 0 && om[1] == 0 && om[2] != 0 && om[3] != 0);  /* its lower row */
  CHECK (op[2] == 0xFF000000u + 14 && op[3] == 0xFF000000u + 16);
  free (op);
  free (om);
  /* A shape that grows is never empty either: one pixel made 3 x 3 is the 9 pixels. */
  {
    uint8_t one = 1;
    uint32_t red = RED;

    CHECK (easel_selection_scale (&red, &one, 1, 1, 3, 3, &op, &om, SIZE_MAX) == EASEL_SELECTION_DONE);
    for (int i = 0; i < 9; i++)
      CHECK (om[i] != 0 && op[i] == RED);
    free (op);
    free (om);
  }
}

/* The memory: the pixels and the mask are counted against the limit exactly, a refusal makes nothing, and a size whose bytes overflow
   is refused (never a short buffer). */
static void
as10_scale_memory (void)
{
  uint8_t mask[4] = { 1, 1, 1, 1 };
  uint32_t px[4] = { RED, RED, RED, RED }, *op = (uint32_t *) 1;
  uint8_t *om = (uint8_t *) 1;

  CHECK (easel_selection_scale (px, mask, 2, 2, 6, 5, &op, &om, 6 * 5 * 4 + 6 * 5) == EASEL_SELECTION_DONE);
  free (op);
  free (om);
  op = (uint32_t *) 1;
  om = (uint8_t *) 1;
  CHECK (easel_selection_scale (px, mask, 2, 2, 6, 5, &op, &om, 6 * 5 * 4 + 6 * 5 - 1) == EASEL_SELECTION_NO_MEMORY);
  CHECK (op == NULL && om == NULL);                                /* nothing made */
  CHECK (easel_selection_scale (px, NULL, 2, 2, 6, 5, &op, NULL, 6 * 5 * 4) == EASEL_SELECTION_DONE);
  free (op);
  CHECK (easel_selection_scale (px, NULL, 2, 2, 6, 5, &op, NULL, 6 * 5 * 4 - 1) == EASEL_SELECTION_NO_MEMORY);
  CHECK (easel_selection_scale (px, NULL, 2, 2, INT_MAX, INT_MAX, &op, NULL, SIZE_MAX) == EASEL_SELECTION_NO_MEMORY);   /* the bytes overflow */
  CHECK (op == NULL);
}

/* 0015 T002 (REQ-0015-03, -07): the rectangle a handle drag gives. */

/* The rectangle of (x, y, w, h) dragged by the handle h through (dx, dy), with or without Shift. */
static void
stretched (EaselSelHandle h, int x, int y, int w, int hh, int dx, int dy, bool shift, int *nx, int *ny, int *nw, int *nh)
{
  easel_stretch_rect (h, x, y, w, hh, dx, dy, shift, nx, ny, nw, nh);
}

/* AS-5 (REQ-0015-07): Shift on a corner keeps the ratio; the side whose size differs more from its size at the press, as a factor,
   leads (the width on a tie); the other is its size times the factor, halves up, at least 1, and both fit 10000. */
static void
as5_stretch_rect_ratio (void)
{
  int x, y, w, h;

  /* Each handle moves only its own edges: marked (1, 1) 5 x 4 dragged through (2, 1). */
  static const struct { EaselSelHandle h; int x, y, w, hh; } edges[] = {
    { EASEL_SEL_HANDLE_TL, 3, 2, 3, 3 }, { EASEL_SEL_HANDLE_T, 1, 2, 5, 3 }, { EASEL_SEL_HANDLE_TR, 1, 2, 7, 3 },
    { EASEL_SEL_HANDLE_L, 3, 1, 3, 4 },  { EASEL_SEL_HANDLE_R, 1, 1, 7, 4 }, { EASEL_SEL_HANDLE_BL, 3, 1, 3, 5 },
    { EASEL_SEL_HANDLE_B, 1, 1, 5, 5 },  { EASEL_SEL_HANDLE_BR, 1, 1, 7, 5 } };

  for (size_t k = 0; k < sizeof edges / sizeof edges[0]; k++)
    {
      stretched (edges[k].h, 1, 1, 5, 4, 2, 1, false, &x, &y, &w, &h);
      CHECK (x == edges[k].x && y == edges[k].y && w == edges[k].w && h == edges[k].hh);
    }
  /* The 1-pixel stop beside the opposite edge, no mirroring: the right handle by (-10, 0), the left one by (10, 0), the left by (-1, 0). */
  stretched (EASEL_SEL_HANDLE_R, 1, 1, 5, 4, -10, 0, false, &x, &y, &w, &h);
  CHECK (x == 1 && y == 1 && w == 1 && h == 4);
  stretched (EASEL_SEL_HANDLE_L, 1, 1, 5, 4, 10, 0, false, &x, &y, &w, &h);
  CHECK (x == 5 && y == 1 && w == 1 && h == 4);
  stretched (EASEL_SEL_HANDLE_L, 1, 1, 5, 4, -1, 0, false, &x, &y, &w, &h);
  CHECK (x == 0 && y == 1 && w == 6 && h == 4);
  stretched (EASEL_SEL_HANDLE_TL, 1, 1, 5, 4, 10, 10, false, &x, &y, &w, &h);
  CHECK (x == 5 && y == 4 && w == 1 && h == 1);
  /* The limit 10000, the fixed edge staying: the bottom handle by (0, 20000); the left handle growing past it. */
  stretched (EASEL_SEL_HANDLE_B, 1, 1, 5, 4, 0, 20000, false, &x, &y, &w, &h);
  CHECK (x == 1 && y == 1 && w == 5 && h == 10000);
  stretched (EASEL_SEL_HANDLE_L, 1, 1, 5, 4, -20000, 0, false, &x, &y, &w, &h);
  CHECK (x == 6 - 10000 && w == 10000);
  /* AS-5: marked 5 x 4, the bottom-right handle with Shift. */
  stretched (EASEL_SEL_HANDLE_BR, 1, 1, 5, 4, 5, 1, true, &x, &y, &w, &h);
  CHECK (w == 10 && h == 8 && x == 1 && y == 1);
  stretched (EASEL_SEL_HANDLE_BR, 1, 1, 5, 4, 1, 5, true, &x, &y, &w, &h);
  CHECK (w == 11 && h == 9);                                       /* 5 x 2.25 = 11.25 */
  stretched (EASEL_SEL_HANDLE_BR, 1, 1, 5, 4, -3, -1, true, &x, &y, &w, &h);
  CHECK (w == 2 && h == 2);                                        /* 4 x 0.4 = 1.6 */
  stretched (EASEL_SEL_HANDLE_BR, 1, 1, 5, 4, 5, -3, true, &x, &y, &w, &h);
  CHECK (w == 10 && h == 8);                                       /* the width x2 differs more from 1 than the height x0.25 */
  /* An edge handle ignores Shift. */
  stretched (EASEL_SEL_HANDLE_R, 1, 1, 5, 4, 5, 0, true, &x, &y, &w, &h);
  CHECK (w == 10 && h == 4);
  /* A tie: the width leads. 4 x 2 by (-2, +1): the width x0.5 and the height x1.5 differ by the same; width leads: 2 x 1. */
  stretched (EASEL_SEL_HANDLE_BR, 0, 0, 4, 2, -2, 1, true, &x, &y, &w, &h);
  CHECK (w == 2 && h == 1);
  /* Halves round up: 2 x 3 by (+1, 0): the factor 1.5 makes the height 4.5, 5. */
  stretched (EASEL_SEL_HANDLE_BR, 0, 0, 2, 3, 1, 0, true, &x, &y, &w, &h);
  CHECK (w == 3 && h == 5);
  /* The corner opposite the fixed one: the top-left handle keeps the bottom-right corner. 5 x 4 at (1, 1) by (-5, -1): the width x2 leads: 10 x 8. */
  stretched (EASEL_SEL_HANDLE_TL, 1, 1, 5, 4, -5, -1, true, &x, &y, &w, &h);
  CHECK (w == 10 && h == 8 && x + w == 6 && y + h == 5);
  /* At the limit the ratio holds: 1 x 10 by (+3000, 0): the other side would be 30010, so the width is reduced: 1000 x 10000. */
  stretched (EASEL_SEL_HANDLE_BR, 0, 0, 1, 10, 3000, 0, true, &x, &y, &w, &h);
  CHECK (w == 1000 && h == 10000);
  /* The limit before the comparison: 5 x 4 by (20000, 0): the width stops at 10000 (x2000), the height 8000. */
  stretched (EASEL_SEL_HANDLE_BR, 0, 0, 5, 4, 20000, 0, true, &x, &y, &w, &h);
  CHECK (w == 10000 && h == 8000);
  /* No movement: nothing changes. */
  stretched (EASEL_SEL_HANDLE_BR, 1, 1, 5, 4, 0, 0, true, &x, &y, &w, &h);
  CHECK (x == 1 && y == 1 && w == 5 && h == 4);
}

/* 0015 T003 (REQ-0015-04, -05, -09): the float stretched from its original; copies keep the image they were made with. */

/* How many pixels of a laid w x h picture have a color. */
static int
count_color (const uint32_t *px, int n, uint32_t color)
{
  int k = 0;

  for (int i = 0; i < n; i++)
    k += px[i] == color;
  return k;
}

/* A 40 x 20 white picture with the red block of the scenarios (the columns 2 to 4 of the rows 2 and 3). */
static EaselImage *
wide_picture (void)
{
  EaselImage *img = easel_image_new (40, 20, WHITE);

  CHECK (img != NULL);
  for (int y = 2; y <= 3; y++)
    for (int x = 2; x <= 4; x++)
      easel_image_set (img, x, y, RED);
  return img;
}

/* REQ-0015-09, Check F2: the first stretch after copies exist makes the copies' image list (4 bytes per entry of capacity, so 12 bytes per
   copy with the 8 of its place). It counts against the limit as the image does: with 9 copies (capacity 16: 192 bytes) a 10 x 4 stretch
   (160 bytes of pixels) is refused under 192 with nothing changed, and done at 192; the memory held by the list is then within it. */
static void
as10_copies_list_counted_at_stretch (void)
{
  EaselImage *img = wide_picture ();
  EaselFloat f = { 0 };
  uint32_t *before;

  CHECK (easel_selection_lift (&f, img, 1, 1, 5, 4, NULL, false, YELLOW, SIZE_MAX) == EASEL_SELECTION_DONE);
  for (int i = 0; i < 9; i++)
    CHECK (easel_selection_stamp (&f, SIZE_MAX));
  CHECK (f.n_stamps == 9 && f.cap == 16 && f.stamp_img == NULL);
  before = f.px;
  CHECK (easel_selection_stretch (&f, 1, 1, 10, 4, 191) == EASEL_SELECTION_NO_MEMORY);
  CHECK (f.px == before && f.w == 5 && f.h == 4 && f.n_stamps == 9 && f.stamp_img == NULL && f.orig_px == NULL);
  CHECK (easel_selection_stretch (&f, 1, 1, 10, 4, 192) == EASEL_SELECTION_DONE);
  CHECK (f.w == 10 && f.stamp_img != NULL);
  CHECK (f.cap * (sizeof *f.stamps + sizeof *f.stamp_img) <= 192);          /* held, against the limit it was counted under */
  easel_float_free (&f);
  easel_image_free (img);
}

/* 0016 T001 (REQ-0016-07), AS-7: the size a picture to paste takes in Fit Picture to Canvas: the side that limits takes the picture's
   size, the other is rounded to the nearest pixel, halves up, at least 1. */
static void
as7_fit_size (void)
{
  static const int cases[][6] = { { 30, 20, 21, 11, 17, 11 },        /* 16.5 rounded up */
                                  { 25, 11, 21, 11, 21, 9 },          /* 9.24 */
                                  { 22, 5, 21, 11, 21, 5 },           /* 4.77 */
                                  { 10000, 1, 21, 11, 21, 1 },        /* 0.0021: at least 1 */
                                  { 1, 10000, 21, 11, 1, 11 },
                                  { 1920, 1080, 1024, 768, 1024, 576 },
                                  { 7353, 1024, 800, 600, 800, 111 }, /* 111.4 */
                                  { 21, 11, 21, 11, 21, 11 } };       /* the same size */
  int nw, nh;

  for (size_t i = 0; i < sizeof cases / sizeof cases[0]; i++)
    {
      easel_fit_size (cases[i][0], cases[i][1], cases[i][2], cases[i][3], &nw, &nh);
      CHECK (nw == cases[i][4] && nh == cases[i][5]);
    }
}

/* 0016 T001 (REQ-0016-05, -06), AS-4: a float made from a pasted picture: no hole; its fully transparent pixels are neither laid, put
   down nor stamped, also after a stretch; partly transparent ones replace all four channels; a picture of only fully transparent
   pixels puts nothing down; the memory is refused with the float left empty. */
static void
as4_paste_float_skips_clear (void)
{
  EaselImage *img = wide_picture (), *before = wide_picture ();
  EaselImage *pic = easel_image_new (3, 2, 0);
  EaselHistory *h = easel_history_new (16 * 1024 * 1024);
  EaselFloat f = { 0 };
  uint32_t out[40 * 20];

  CHECK (pic != NULL && h != NULL);
  easel_image_set (pic, 0, 0, RED);
  easel_image_set (pic, 1, 0, 0x00000000u);                          /* fully transparent: leaves the picture */
  easel_image_set (pic, 2, 0, 0x80FF0000u);                          /* partly transparent: replaces */
  easel_image_set (pic, 0, 1, 0x00FF0000u);                          /* alpha 0 with a color: leaves the picture too */
  easel_image_set (pic, 1, 1, BLUE);
  easel_image_set (pic, 2, 1, 0x00000000u);
  /* Memory: 24 bytes needed. */
  CHECK (easel_selection_paste (&f, pic, 2, 2, 23) == EASEL_SELECTION_NO_MEMORY);
  CHECK (f.px == NULL && f.w == 0);
  CHECK (easel_selection_paste (&f, pic, 2, 2, 24) == EASEL_SELECTION_DONE);
  CHECK (f.x == 2 && f.y == 2 && f.w == 3 && f.h == 2 && !f.hole && f.mask == NULL);
  /* Laid at (2, 2) over the red block: the transparent ones show the picture (red at (3, 2), red at (2, 3)), the others replace. */
  easel_selection_lay (&f, true, img, 0, 0, 40, 20, out, 40);
  CHECK (out[2 * 40 + 2] == RED && out[2 * 40 + 3] == RED && out[2 * 40 + 4] == 0x80FF0000u);
  CHECK (out[3 * 40 + 2] == RED && out[3 * 40 + 3] == BLUE && out[3 * 40 + 4] == RED);
  /* A copy left at (2, 2), the float moved to (10, 5): both skip the transparent ones; no hole at (2, 2). */
  CHECK (easel_selection_stamp (&f, SIZE_MAX));
  f.x = 10;
  f.y = 5;
  easel_selection_lay (&f, true, img, 0, 0, 40, 20, out, 40);
  CHECK (out[2 * 40 + 3] == RED && out[3 * 40 + 3] == BLUE);
  CHECK (out[5 * 40 + 10] == RED && out[5 * 40 + 11] == WHITE && out[5 * 40 + 12] == 0x80FF0000u);
  CHECK (out[6 * 40 + 10] == WHITE && out[6 * 40 + 11] == BLUE && out[6 * 40 + 12] == WHITE);
  /* Stretched to 6 x 4: each pixel doubled, the transparent ones still skipped. */
  CHECK (easel_selection_stretch (&f, 10, 5, 6, 4, SIZE_MAX) == EASEL_SELECTION_DONE);
  easel_selection_lay (&f, true, img, 0, 0, 40, 20, out, 40);
  CHECK (out[5 * 40 + 12] == WHITE && out[6 * 40 + 13] == WHITE && out[5 * 40 + 14] == 0x80FF0000u);
  CHECK (out[7 * 40 + 10] == WHITE && out[8 * 40 + 13] == BLUE && out[8 * 40 + 15] == WHITE);
  /* Put down: the copy changes (2, 2) red to red (no change), (4, 2) to 0x80FF0000, (3, 3) to blue; the float its 4 x 2 non-clear
     pixels on white: 2 + 8 changes; nothing else. */
  CHECK (easel_selection_put_down (&f, true, img, h, SIZE_MAX) == EASEL_SELECTION_DONE);
  CHECK (easel_image_get (img, 4, 2) == 0x80FF0000u && easel_image_get (img, 3, 3) == BLUE && easel_image_get (img, 3, 2) == RED);
  CHECK (easel_image_get (img, 12, 5) == WHITE && easel_image_get (img, 14, 5) == 0x80FF0000u);
  CHECK (easel_history_undo (h, img));
  for (int y = 0; y < 20; y++)
    for (int x = 0; x < 40; x++)
      CHECK (easel_image_get (img, x, y) == easel_image_get (before, x, y));
  easel_float_free (&f);
  /* Only fully transparent pixels: nothing is put down, no step. */
  for (int y = 0; y < 2; y++)
    for (int x = 0; x < 3; x++)
      easel_image_set (pic, x, y, (uint32_t) (x * 0x00102030u));
  CHECK (easel_selection_paste (&f, pic, 2, 2, SIZE_MAX) == EASEL_SELECTION_DONE);
  CHECK (easel_selection_put_down (&f, true, img, h, SIZE_MAX) == EASEL_SELECTION_NOTHING);
  CHECK (easel_history_can_redo (h) && !easel_history_can_undo (h));          /* the step undone above stays redoable */
  easel_float_free (&f);
  easel_history_free (h);
  easel_image_free (pic);
  easel_image_free (before);
  easel_image_free (img);
}

/* 0016 T003 (REQ-0016-03, -11), AS-2: the copy of a selection: its bounding rectangle, the pixels of its shape as they are, every other
   pixel #00000000; a marked one from the picture, a floating one from its own pixels as they float (stretched, also outside the
   picture); its memory refused with NULL. */
static void
as2_copy_image (void)
{
  EaselImage *img = wide_picture (), *c;
  EaselFloat f = { 0 };
  uint8_t tri[25];

  /* Marked (1, 1) to (5, 4): the picture's pixels, red at the copy's columns 1 to 3 of its rows 1 and 2. */
  c = easel_selection_copy_image (img, NULL, 1, 1, 5, 4, NULL, SIZE_MAX);
  CHECK (c != NULL && c->width == 5 && c->height == 4);
  for (int y = 0; y < 4; y++)
    for (int x = 0; x < 5; x++)
      CHECK (easel_image_get (c, x, y) == (x >= 1 && x <= 3 && y >= 1 && y <= 2 ? RED : WHITE));
  easel_image_free (c);
  /* The marked triangle (i + j <= 4 of a 5 x 5 box at (1, 1)): 15 pixels, the other 10 #00000000. */
  for (int j = 0; j < 5; j++)
    for (int i = 0; i < 5; i++)
      tri[j * 5 + i] = i + j <= 4;
  c = easel_selection_copy_image (img, NULL, 1, 1, 5, 5, tri, SIZE_MAX);
  CHECK (c != NULL && c->width == 5 && c->height == 5);
  for (int j = 0; j < 5; j++)
    for (int i = 0; i < 5; i++)
      CHECK (easel_image_get (c, i, j) == (i + j <= 4 ? easel_image_get (img, 1 + i, 1 + j) : 0x00000000u));
  easel_image_free (c);
  /* Memory: 100 bytes needed for 5 x 5. */
  CHECK (easel_selection_copy_image (img, NULL, 1, 1, 5, 5, tri, 99) == NULL);
  c = easel_selection_copy_image (img, NULL, 1, 1, 5, 5, tri, 100);
  CHECK (c != NULL);
  easel_image_free (c);
  /* Floating, stretched to 10 x 4 and moved to (-3, 1), partly outside the picture: its own pixels, all of them. */
  CHECK (easel_selection_lift (&f, img, 1, 1, 5, 4, NULL, false, YELLOW, SIZE_MAX) == EASEL_SELECTION_DONE);
  CHECK (easel_selection_stretch (&f, -3, 1, 10, 4, SIZE_MAX) == EASEL_SELECTION_DONE);
  c = easel_selection_copy_image (img, &f, f.x, f.y, f.w, f.h, NULL, SIZE_MAX);
  CHECK (c != NULL && c->width == 10 && c->height == 4);
  for (int y = 0; y < 4; y++)
    for (int x = 0; x < 10; x++)
      CHECK (easel_image_get (c, x, y) == f.px[y * 10 + x]);
  CHECK (easel_image_get (c, 3, 1) == RED && easel_image_get (c, 0, 0) == WHITE);
  easel_image_free (c);
  easel_float_free (&f);
  /* A floating triangle: outside its shape #00000000. */
  CHECK (easel_selection_lift (&f, img, 1, 1, 5, 5, tri, false, YELLOW, SIZE_MAX) == EASEL_SELECTION_DONE);
  c = easel_selection_copy_image (img, &f, f.x, f.y, f.w, f.h, NULL, SIZE_MAX);
  CHECK (c != NULL);
  CHECK (easel_image_get (c, 4, 4) == 0x00000000u && easel_image_get (c, 1, 1) == RED && easel_image_get (c, 4, 0) == WHITE);
  easel_image_free (c);
  easel_float_free (&f);
  easel_image_free (img);
}

/* 0016 (JOD, 2026-10-23, after perf-0016): the copy made by rows equals the copy made pixel by pixel (the oracle below, the first
   version of easel_selection_copy_image) for a marked rectangle, a free-form selection (transparent outside its shape), a floating
   selection partly outside the picture, and one pixel wide and one pixel high. */
static EaselImage *
copy_oracle (const EaselImage *img, const EaselFloat *f, int x, int y, int w, int h, const uint8_t *mask)
{
  EaselImage *c = easel_image_new (w, h, 0x00000000u);

  CHECK (c != NULL);
  if (f != NULL)
    mask = f->mask;
  for (int j = 0; j < h; j++)
    for (int i = 0; i < w; i++)
      {
        size_t at = (size_t) j * (size_t) w + (size_t) i;

        if (mask == NULL || mask[at] != 0)
          c->px[at] = f != NULL ? f->px[at] : img->px[(size_t) (y + j) * (size_t) img->width + (size_t) (x + i)];
      }
  return c;
}

static void
check_copy_equals_oracle (const EaselImage *img, const EaselFloat *f, int x, int y, int w, int h, const uint8_t *mask)
{
  EaselImage *a = easel_selection_copy_image (img, f, x, y, w, h, mask, SIZE_MAX), *b = copy_oracle (img, f, x, y, w, h, mask);

  CHECK (a != NULL && a->width == w && a->height == h);
  CHECK (memcmp (a->px, b->px, (size_t) w * (size_t) h * sizeof *a->px) == 0);
  easel_image_free (a);
  easel_image_free (b);
}

static void
copy_by_rows_matches_oracle (void)
{
  EaselImage *img = easel_image_new (40, 20, 0);
  EaselFloat f = { 0 };
  uint8_t mask[7 * 6];

  CHECK (img != NULL);
  for (int y = 0; y < 20; y++)
    for (int x = 0; x < 40; x++)
      easel_image_set (img, x, y, 0xFF000000u | (uint32_t) (x * 6 + 1) << 16 | (uint32_t) (y * 12 + 3) << 8 | (uint32_t) (x ^ y));
  for (int k = 0; k < 7 * 6; k++)
    mask[k] = (k * 7 + k / 7) % 3 != 0;
  check_copy_equals_oracle (img, NULL, 3, 2, 7, 6, NULL);              /* a marked rectangle */
  check_copy_equals_oracle (img, NULL, 33, 14, 7, 6, NULL);            /* at the picture's right and bottom edges */
  check_copy_equals_oracle (img, NULL, 0, 0, 40, 20, NULL);            /* the whole picture */
  check_copy_equals_oracle (img, NULL, 5, 4, 1, 9, NULL);              /* one pixel wide */
  check_copy_equals_oracle (img, NULL, 5, 4, 9, 1, NULL);              /* one pixel high */
  check_copy_equals_oracle (img, NULL, 3, 2, 7, 6, mask);              /* a free-form shape */
  /* Floating, partly outside the picture, with and without a shape. */
  CHECK (easel_selection_lift (&f, img, 3, 2, 7, 6, NULL, false, 0xFFFFFF00u, SIZE_MAX) == EASEL_SELECTION_DONE);
  f.x = -2;
  f.y = 17;
  check_copy_equals_oracle (img, &f, f.x, f.y, f.w, f.h, NULL);
  easel_float_free (&f);
  CHECK (easel_selection_lift (&f, img, 3, 2, 7, 6, mask, false, 0xFFFFFF00u, SIZE_MAX) == EASEL_SELECTION_DONE);
  f.x = 36;
  check_copy_equals_oracle (img, &f, f.x, f.y, f.w, f.h, NULL);
  easel_float_free (&f);
  easel_image_free (img);
}

/* 0016 T010 (REQ-0016-05, -07), AS-7 and AS-15: a paste that grew the picture is put down as one step with the growth: Undo crops back
   to the old size with every pixel; put down with no pixel changed (the float's pixels as the picture's), the growth is still a step;
   cleared (put down without the float), the copies it left are in the step with the growth. */
static void
as7_put_down_grown (void)
{
  EaselImage *img = easel_image_new (4, 3, WHITE), *pic = easel_image_new (6, 5, RED);
  EaselHistory *h = easel_history_new (1 << 20);
  EaselFloat f = { 0 };

  CHECK (img != NULL && pic != NULL && h != NULL);
  /* The picture grown to 6 x 5 (yellow), the paste at (0, 0). */
  CHECK (easel_image_resize (img, 6, 5, YELLOW) == EASEL_RESIZE_OK);
  CHECK (easel_selection_paste (&f, pic, 0, 0, SIZE_MAX) == EASEL_SELECTION_DONE);
  CHECK (easel_selection_put_down_grown (&f, true, img, h, 4, 3, YELLOW, SIZE_MAX) == EASEL_SELECTION_DONE);
  CHECK (easel_image_get (img, 5, 4) == RED && easel_image_get (img, 0, 0) == RED);
  CHECK (easel_history_undo (h, img));
  CHECK (img->width == 4 && img->height == 3);
  for (int y = 0; y < 3; y++)
    for (int x = 0; x < 4; x++)
      CHECK (easel_image_get (img, x, y) == WHITE);
  CHECK (easel_history_redo (h, img));
  CHECK (img->width == 6 && easel_image_get (img, 5, 4) == RED);
  easel_float_free (&f);
  /* Cleared: a copy left at (0, 0), the float moved to (3, 3) and discarded: the copy and the growth are the step. */
  CHECK (easel_history_undo (h, img));
  CHECK (easel_image_resize (img, 6, 5, YELLOW) == EASEL_RESIZE_OK);
  easel_image_free (pic);
  pic = easel_image_new (2, 2, BLUE);
  CHECK (pic != NULL);
  CHECK (easel_selection_paste (&f, pic, 0, 0, SIZE_MAX) == EASEL_SELECTION_DONE);
  CHECK (easel_selection_stamp (&f, SIZE_MAX));
  f.x = 3;
  f.y = 3;
  CHECK (easel_selection_put_down_grown (&f, false, img, h, 4, 3, YELLOW, SIZE_MAX) == EASEL_SELECTION_DONE);
  CHECK (easel_image_get (img, 0, 0) == BLUE && easel_image_get (img, 3, 3) == YELLOW && easel_image_get (img, 2, 1) == WHITE);
  CHECK (easel_history_undo (h, img));
  CHECK (img->width == 4 && easel_image_get (img, 0, 0) == WHITE);
  easel_float_free (&f);
  /* Nothing to lay at all (a cleared paste with no copies: no hole, no copy, no float): the growth alone is the step. */
  CHECK (easel_image_resize (img, 6, 5, YELLOW) == EASEL_RESIZE_OK);
  easel_image_free (pic);
  pic = easel_image_new (2, 2, BLUE);
  CHECK (pic != NULL);
  CHECK (easel_selection_paste (&f, pic, 0, 0, SIZE_MAX) == EASEL_SELECTION_DONE);
  CHECK (easel_selection_put_down_grown (&f, false, img, h, 4, 3, YELLOW, SIZE_MAX) == EASEL_SELECTION_DONE);
  CHECK (easel_history_undo (h, img));
  CHECK (img->width == 4 && img->height == 3 && easel_image_get (img, 0, 0) == WHITE);
  easel_float_free (&f);
  /* The growth alone: the paste's pixels equal to what is under them (a wholly transparent one), still a step. */
  CHECK (easel_image_resize (img, 6, 5, YELLOW) == EASEL_RESIZE_OK);
  easel_image_free (pic);
  pic = easel_image_new (6, 5, 0x00000000u);
  CHECK (pic != NULL);
  CHECK (easel_selection_paste (&f, pic, 0, 0, SIZE_MAX) == EASEL_SELECTION_DONE);
  CHECK (easel_selection_put_down_grown (&f, true, img, h, 4, 3, YELLOW, SIZE_MAX) == EASEL_SELECTION_DONE);
  CHECK (easel_history_can_undo (h));
  CHECK (easel_history_undo (h, img));
  CHECK (img->width == 4 && img->height == 3);
  easel_float_free (&f);
  easel_image_free (pic);
  easel_history_free (h);
  easel_image_free (img);
}

/* REQ-0002-16, REQ-0016-07, Check F3 of 0016: the step of a put-down that grew the picture holds the memory the budget counts. A pasted
   513 x 512 picture of stripes opposite to the picture's changes just more than a power of two of pixels, so the log's spare half
   must not stay with the step. */
static void
as7_grown_step_holds_what_it_counts (void)
{
  EaselImage *img = easel_image_new (600, 600, WHITE), *pic = easel_image_new (513, 512, WHITE);
  EaselHistory *h = easel_history_new ((size_t) 512 << 20);
  EaselFloat f = { 0 };
  size_t before, held, counted;

  CHECK (img != NULL && pic != NULL && h != NULL);
  for (int y = 0; y < 600; y++)
    for (int x = 0; x < 600; x++)
      img->px[y * 600 + x] = (x & 1) ? RED : WHITE;
  for (int y = 0; y < 512; y++)
    for (int x = 0; x < 513; x++)
      pic->px[y * 513 + x] = (x & 1) ? WHITE : RED;          /* every pixel differs from the one under it */
  CHECK (easel_selection_paste (&f, pic, 0, 0, SIZE_MAX) == EASEL_SELECTION_DONE);
  before = heap_in_use ();
  CHECK (easel_selection_put_down_grown (&f, true, img, h, 550, 550, YELLOW, SIZE_MAX) == EASEL_SELECTION_DONE);
  held = heap_in_use () - before;
  counted = easel_history_bytes (h);
  printf ("  held %zu bytes, counted %zu\n", held, counted);
  CHECK (held <= counted + counted / 100 + 4096);
  easel_float_free (&f);
  easel_history_free (h);
  easel_image_free (pic);
  easel_image_free (img);
}

/* AS-11: a copy left before a stretch keeps its size and pixels, a copy left after has the stretched ones; the float is made from the
   original each time (5 x 4, then 10 x 4, then 3 x 4: the middle pixel only of the red columns). The hole stays at the original's
   size and shape, 20 yellow pixels, not the stretched float's. */
static void
as11_copies_keep_their_image (void)
{
  EaselImage *img = wide_picture ();
  EaselFloat f = { 0 };
  uint32_t out[40 * 20];
  int x0, y0, x1, y1;

  CHECK (easel_selection_lift (&f, img, 1, 1, 5, 4, NULL, false, YELLOW, SIZE_MAX) == EASEL_SELECTION_DONE);
  f.x = 11;
  f.y = 4;
  CHECK (easel_selection_stamp (&f, SIZE_MAX));                    /* a 5 x 4 copy at (11, 4) */
  CHECK (easel_selection_stretch (&f, 21, 4, 10, 4, SIZE_MAX) == EASEL_SELECTION_DONE);
  CHECK (f.w == 10 && f.h == 4 && f.x == 21 && f.y == 4);
  CHECK (easel_selection_stamp (&f, SIZE_MAX));                    /* a 10 x 4 copy at (21, 4) */
  CHECK (easel_selection_stretch (&f, 21, 10, 3, 4, SIZE_MAX) == EASEL_SELECTION_DONE);
  CHECK (f.w == 3 && f.h == 4 && f.x == 21 && f.y == 10);          /* made from the original 5 x 4, not from the 10 x 4 */
  easel_selection_lay (&f, true, img, 0, 0, 40, 20, out, 40);
  /* The hole: the original's 5 x 4 at (1, 1), 20 yellow pixels (the original red block is covered by it). */
  CHECK (count_color (out, 40 * 20, YELLOW) == 20);
  for (int y = 1; y <= 4; y++)
    for (int x = 1; x <= 5; x++)
      CHECK (out[y * 40 + x] == YELLOW);
  /* The copy before the first stretch: 5 x 4, the red at the columns 12 to 14 of the rows 5 and 6. */
  for (int x = 11; x <= 15; x++)
    CHECK (out[5 * 40 + x] == (x >= 12 && x <= 14 ? RED : WHITE));
  /* The copy before the second: 10 x 4, the red at the columns 23 to 28 of the rows 5 and 6. */
  for (int x = 21; x <= 30; x++)
    CHECK (out[5 * 40 + x] == (x >= 23 && x <= 28 ? RED : WHITE));
  /* The float: 3 x 4 at (21, 10): the new pixel i is the original i * 5/3 + 5/6, the columns 0, 2 and 4: red at the middle one. */
  for (int x = 21; x <= 23; x++)
    CHECK (out[11 * 40 + x] == (x == 22 ? RED : WHITE));
  CHECK (count_color (out, 40 * 20, RED) == 6 + 12 + 2);
  /* The rectangle the lay can change: the hole, the copies and the float. */
  CHECK (easel_selection_bounds (&f, true, img, &x0, &y0, &x1, &y1));
  CHECK (x0 == 1 && y0 == 1 && x1 == 30 && y1 == 13);
  easel_float_free (&f);
  easel_image_free (img);
}

/* AS-4, REQ-0015-05, -06: the pixels are always made from those as lifted: a 5 x 4 float made 2 wide (the red columns 1 and 3 of the
   original 1 to 3 survive as its two pixels) and then 5 wide again has the original's pixels back, not the red columns 0 to 4 that a
   scale of the 2-wide image would give; and the hole keeps the original's size and place however big the float is made. */
static void
stretch_keeps_the_original_and_the_hole (void)
{
  EaselImage *img = wide_picture ();
  EaselFloat f = { 0 };
  uint32_t lifted[5 * 4];
  int x0, y0, x1, y1;

  CHECK (easel_selection_lift (&f, img, 1, 1, 5, 4, NULL, false, YELLOW, SIZE_MAX) == EASEL_SELECTION_DONE);
  memcpy (lifted, f.px, sizeof lifted);
  CHECK (easel_selection_stretch (&f, 1, 1, 2, 4, SIZE_MAX) == EASEL_SELECTION_DONE);
  CHECK (easel_selection_stretch (&f, 1, 1, 5, 4, SIZE_MAX) == EASEL_SELECTION_DONE);
  CHECK (f.w == 5 && f.h == 4 && memcmp (f.px, lifted, sizeof lifted) == 0);
  /* The hole at the original's 5 x 4 whatever the size: bounds without the float and no copy are (1, 1) to (5, 4). */
  CHECK (easel_selection_stretch (&f, 1, 1, 12, 6, SIZE_MAX) == EASEL_SELECTION_DONE);
  CHECK (easel_selection_bounds (&f, false, img, &x0, &y0, &x1, &y1));
  CHECK (x0 == 1 && y0 == 1 && x1 == 5 && y1 == 4);
  CHECK (easel_selection_bounds (&f, true, img, &x0, &y0, &x1, &y1));
  CHECK (x0 == 1 && y0 == 1 && x1 == 12 && y1 == 6);
  easel_float_free (&f);
  easel_image_free (img);
}

/* AS-10: a stretch that cannot get its memory makes nothing: the float, its copies and its place are as they were, also after earlier
   stretches of the same drag succeeded. */
static void
as10_stretch_memory (void)
{
  EaselImage *img = wide_picture ();
  EaselFloat f = { 0 };
  uint32_t *before;

  CHECK (easel_selection_lift (&f, img, 1, 1, 5, 4, NULL, false, YELLOW, SIZE_MAX) == EASEL_SELECTION_DONE);
  CHECK (easel_selection_stamp (&f, SIZE_MAX));
  CHECK (easel_selection_stretch (&f, 1, 1, 6, 4, 6 * 4 * 4) == EASEL_SELECTION_DONE);           /* 96 bytes: exactly enough */
  before = f.px;
  CHECK (easel_selection_stretch (&f, 3, 2, 12, 4, 12 * 4 * 4 - 1) == EASEL_SELECTION_NO_MEMORY);
  CHECK (f.px == before && f.w == 6 && f.h == 4 && f.x == 1 && f.y == 1 && f.n_stamps == 1);
  CHECK (easel_selection_stretch (&f, 1, 1, 6, 4, 6 * 4 * 4 - 1) == EASEL_SELECTION_NO_MEMORY);
  CHECK (f.px == before && f.w == 6);
  /* A shape counts its mask too. */
  {
    uint8_t mask[20];
    EaselFloat g = { 0 };

    memset (mask, 1, sizeof mask);
    CHECK (easel_selection_lift (&g, img, 1, 1, 5, 4, mask, false, YELLOW, SIZE_MAX) == EASEL_SELECTION_DONE);
    CHECK (easel_selection_stretch (&g, 1, 1, 6, 4, 6 * 4 * 4 + 6 * 4 - 1) == EASEL_SELECTION_NO_MEMORY);
    CHECK (g.w == 5);
    CHECK (easel_selection_stretch (&g, 1, 1, 6, 4, 6 * 4 * 4 + 6 * 4) == EASEL_SELECTION_DONE);
    CHECK (g.w == 6 && g.mask != NULL);
    easel_float_free (&g);
  }
  easel_float_free (&f);
  easel_image_free (img);
}

/* 0015 T006 (REQ-0015-05, -06, -09; perf): the lazy stretch. */

/* A lifted float of a w x h block of random pixels (with the L-shaped mask if shape) in the picture of the scenarios' larger kin, and
   the pixels it was lifted with, for the reference. */
static void
lifted_block (EaselImage *img, EaselFloat *f, int w, int h, bool shape, uint32_t **ref, uint8_t **ref_mask)
{
  uint8_t *mask = NULL;

  for (int i = 0; i < img->width * img->height; i++)
    img->px[i] = 0xFF000000u | (uint32_t) rand ();
  if (shape)
    {
      mask = malloc ((size_t) w * h);
      CHECK (mask != NULL);
      for (int j = 0; j < h; j++)
        for (int i = 0; i < w; i++)
          mask[j * w + i] = i == 0 || j == h - 1;
    }
  CHECK (easel_selection_lift (f, img, 1, 1, w, h, mask, false, YELLOW, SIZE_MAX) == EASEL_SELECTION_DONE);
  *ref = malloc ((size_t) w * h * sizeof **ref);
  CHECK (*ref != NULL);
  memcpy (*ref, f->px, (size_t) w * h * sizeof **ref);
  *ref_mask = mask;
}

/* The lazy stretch makes only the 256 x 256 blocks asked for, with the same pixels as the full scale: the pixels of a filled rectangle
   equal the full result's, the others are not made (zero, and counted), the whole after easel_selection_complete; for a shape too, and
   a shape that the nearest pixel loses keeps its one pixel at the completion. A refusal is the same as the full stretch's. */
static void
as2_lazy_stretch_matches_full (void)
{
  srand (9);
  for (int round = 0; round < 40; round++)
    {
      EaselImage *img = easel_image_new (80, 60, 0);
      EaselFloat f = { 0 };
      uint32_t *ref, *full, *got;
      uint8_t *ref_mask, *full_mask = NULL;
      bool shape = round % 2;
      int w = 3 + rand () % 30, h = 3 + rand () % 20, W = 1 + rand () % 700, H = 1 + rand () % 600, x0, y0, x1, y1;

      if (round % 8 == 3)
        W = 257 + 256 * (round % 3);                 /* a last block of one column: the edge of a block */
      if (round % 8 == 5)
        H = 257 + 256 * (round % 2);

      CHECK (img != NULL);
      lifted_block (img, &f, w, h, shape, &ref, &ref_mask);
      CHECK (easel_selection_scale (ref, ref_mask, w, h, W, H, &full, shape ? &full_mask : NULL, SIZE_MAX) == EASEL_SELECTION_DONE);
      CHECK (easel_selection_stretch_lazy (&f, 4, 5, W, H, SIZE_MAX) == EASEL_SELECTION_DONE);
      CHECK (f.w == W && f.h == H && f.x == 4 && f.y == 5);
      CHECK (f.made_px == 0);
      x0 = rand () % W;
      y0 = rand () % H;
      x1 = x0 + 1 + rand () % 300;
      y1 = y0 + 1 + rand () % 300;
      easel_selection_fill (&f, x0, y0, x1, y1);
      got = f.px;
      /* Every pixel of the asked rectangle is the full result's; a pixel in a block that was not asked for is not made (zero). */
      for (int j = 0; j < H; j++)
        for (int i = 0; i < W; i++)
          {
            bool asked = i >= x0 && i < x1 && j >= y0 && j < y1;
            bool made_block = (i / 256 >= x0 / 256 && i / 256 <= (x1 - 1) / 256 && j / 256 >= y0 / 256 && j / 256 <= (y1 - 1) / 256);

            if (asked)
              CHECK (got[(size_t) j * W + i] == full[(size_t) j * W + i]);
            if (!made_block)
              CHECK (got[(size_t) j * W + i] == 0);
          }
      CHECK (f.made_px > 0 && f.made_px <= (size_t) W * H);
      easel_selection_complete (&f);
      CHECK (f.made_px == (size_t) W * H);
      CHECK (memcmp (f.px, full, (size_t) W * H * sizeof *full) == 0);
      if (shape)
        CHECK (memcmp (f.mask, full_mask, (size_t) W * H) == 0);
      easel_float_free (&f);
      free (ref);
      free (ref_mask);
      free (full);
      free (full_mask);
      easel_image_free (img);
    }
}

/* A shape that the nearest pixel loses is known only when the whole image is made: the L (the column 0 and the row 3 of 4 x 4) made 1 x 1
   lazily keeps its one pixel, the color of its first pixel in reading order, at the completion. */
static void
as7_lazy_shape_keeps_one_pixel (void)
{
  EaselImage *img = easel_image_new (20, 20, 0);
  EaselFloat f = { 0 };
  uint32_t *ref;
  uint8_t *ref_mask;

  CHECK (img != NULL);
  srand (4);
  lifted_block (img, &f, 4, 4, true, &ref, &ref_mask);
  CHECK (easel_selection_stretch_lazy (&f, 1, 1, 1, 1, SIZE_MAX) == EASEL_SELECTION_DONE);
  easel_selection_fill (&f, 0, 0, 1, 1);
  CHECK (f.mask[0] == 0);                                              /* not yet: the nearest sample, the local (2, 2), is not in the L */
  easel_selection_complete (&f);
  CHECK (f.mask[0] == 1 && f.px[0] == ref[0]);
  easel_float_free (&f);
  free (ref);
  free (ref_mask);
  easel_image_free (img);
}

/* The refusal of the lazy stretch is the full one's (the bytes of pixels and mask against the limit, the float as it was), and a lazy
   image with the copies' images kept works as the full one does. */
static void
as10_lazy_stretch_memory (void)
{
  EaselImage *img = wide_picture ();
  EaselFloat f = { 0 };
  uint32_t *before;

  CHECK (easel_selection_lift (&f, img, 1, 1, 5, 4, NULL, false, YELLOW, SIZE_MAX) == EASEL_SELECTION_DONE);
  CHECK (easel_selection_stamp (&f, SIZE_MAX));
  CHECK (easel_selection_stretch_lazy (&f, 1, 1, 6, 4, 6 * 4 * 4) == EASEL_SELECTION_DONE);
  before = f.px;
  CHECK (easel_selection_stretch_lazy (&f, 3, 2, 12, 4, 12 * 4 * 4 - 1) == EASEL_SELECTION_NO_MEMORY);
  CHECK (f.px == before && f.w == 6 && f.x == 1 && f.n_stamps == 1);
  CHECK (easel_selection_stretch_lazy (&f, 1, 1, INT_MAX, INT_MAX, SIZE_MAX) == EASEL_SELECTION_NO_MEMORY);
  /* The copy made before keeps the 5 x 4 image; the lazy image completed lays as the full one (the float 10 x 4 at (21, 4)). */
  CHECK (easel_selection_stretch_lazy (&f, 21, 4, 10, 4, SIZE_MAX) == EASEL_SELECTION_DONE);
  easel_selection_complete (&f);
  {
    uint32_t out[40 * 20];

    easel_selection_lay (&f, true, img, 0, 0, 40, 20, out, 40);
    CHECK (out[2 * 40 + 3] == RED && out[2 * 40 + 5] == WHITE);        /* the copy at (1, 1), 5 x 4: red at its local columns 1 to 3 */
    CHECK (out[5 * 40 + 23] == RED && out[5 * 40 + 28] == RED);         /* the float, 10 x 4 at (21, 4): red at its columns 2 to 7 */
    CHECK (out[5 * 40 + 22] == WHITE && out[5 * 40 + 29] == WHITE);
  }
  easel_float_free (&f);
  easel_image_free (img);
}

/* ---- 0018 T004 (REQ-0018-07): flip, turn and invert of a floating selection ---- */

/* The new place of the pixel (x, y) of a w x h picture under a flip or turn, by the spec's formulas (REQ-0018-04), and the new size. */
static void
naive_place (EaselOp op, int w, int h, int x, int y, int *nx, int *ny, int *nw, int *nh)
{
  *nx = x;
  *ny = y;
  *nw = w;
  *nh = h;
  switch (op)
    {
    case EASEL_OP_FLIP_H: *nx = w - 1 - x; break;
    case EASEL_OP_FLIP_V: *ny = h - 1 - y; break;
    case EASEL_OP_ROTATE_90: *nx = h - 1 - y; *ny = x; *nw = h; *nh = w; break;
    case EASEL_OP_ROTATE_180: *nx = w - 1 - x; *ny = h - 1 - y; break;
    case EASEL_OP_ROTATE_270: *nx = y; *ny = w - 1 - x; *nw = h; *nh = w; break;
    default: break;
    }
}

/* The w x h pixels (and mask, when given) after op, by those formulas: new buffers. */
static void
naive_apply (EaselOp op, const uint32_t *px, const uint8_t *mask, int w, int h, uint32_t **out_px, uint8_t **out_mask, int *nw, int *nh)
{
  int dummy_x, dummy_y;

  naive_place (op, w, h, 0, 0, &dummy_x, &dummy_y, nw, nh);
  *out_px = malloc ((size_t) w * h * sizeof **out_px);
  *out_mask = mask != NULL ? malloc ((size_t) w * h) : NULL;
  CHECK (*out_px != NULL && (mask == NULL || *out_mask != NULL));
  for (int y = 0; y < h; y++)
    for (int x = 0; x < w; x++)
      {
        int nx, ny, ww, hh;

        naive_place (op, w, h, x, y, &nx, &ny, &ww, &hh);
        (*out_px)[(size_t) ny * *nw + nx] = px[(size_t) y * w + x];
        if (mask != NULL)
          (*out_mask)[(size_t) ny * *nw + nx] = mask[(size_t) y * w + x];
      }
}

/* A flip or turn of a stretched float shows exactly the flip or turn of what was shown (REQ-0018-07): for every lifted size up to 4 x 4,
   every stretch to up to 6 x 6, every command, and a second one on top; the place is kept, the lifted pixels stay as lifted, and a
   stretch back to the lifted size (turned by the commands) shows the lifted pixels flipped and turned, exactly. With a shape the mask
   follows its pixels, also where the nearest pixel loses the shape (the one pixel it keeps). */
static void
check_transform_of_shown (bool shape)
{
  static const EaselOp ops[] = { EASEL_OP_FLIP_H, EASEL_OP_FLIP_V, EASEL_OP_ROTATE_90, EASEL_OP_ROTATE_180, EASEL_OP_ROTATE_270 };

  srand (3);
  for (int lw = 1; lw <= 4; lw++)
    for (int lh = 1; lh <= 4; lh++)
      for (int W = 1; W <= 6; W++)
        for (int H = 1; H <= 6; H++)
          for (size_t k = 0; k < sizeof ops / sizeof *ops; k++)
            {
              EaselImage *img = easel_image_new (20, 20, 0);
              EaselFloat f = { 0 };
              uint32_t *ref, *shown_px, *want_px, *lifted_want;
              uint8_t *ref_mask, *shown_mask = NULL, *want_mask, *lifted_mask_want;
              int w2, h2, w3, h3, lw2, lh2;
              EaselOp second = ops[(k + 2) % (sizeof ops / sizeof *ops)];

              CHECK (img != NULL);
              lifted_block (img, &f, lw, lh, shape, &ref, &ref_mask);
              CHECK (easel_selection_stretch (&f, 3, 4, W, H, SIZE_MAX) == EASEL_SELECTION_DONE);
              shown_px = malloc ((size_t) W * H * sizeof *shown_px);
              CHECK (shown_px != NULL);
              memcpy (shown_px, f.px, (size_t) W * H * sizeof *shown_px);
              if (shape)
                {
                  shown_mask = malloc ((size_t) W * H);
                  CHECK (shown_mask != NULL);
                  memcpy (shown_mask, f.mask, (size_t) W * H);
                }
              CHECK (easel_selection_transform (&f, ops[k], SIZE_MAX) == EASEL_SELECTION_DONE);
              naive_apply (ops[k], shown_px, shown_mask, W, H, &want_px, &want_mask, &w2, &h2);
              CHECK (f.w == w2 && f.h == h2 && f.x == 3 && f.y == 4);
              CHECK (memcmp (f.px, want_px, (size_t) w2 * h2 * sizeof *want_px) == 0);
              if (shape)
                CHECK (memcmp (f.mask, want_mask, (size_t) w2 * h2) == 0);
              CHECK (f.ow == lw && f.oh == lh && memcmp (f.orig_px, ref, (size_t) lw * lh * sizeof *ref) == 0);   /* as lifted */
              /* a second command on top of the first: the command of what is shown now */
              CHECK (easel_selection_transform (&f, second, SIZE_MAX) == EASEL_SELECTION_DONE);
              {
                uint32_t *want2;
                uint8_t *want2_mask;

                naive_apply (second, want_px, want_mask, w2, h2, &want2, &want2_mask, &w3, &h3);
                CHECK (f.w == w3 && f.h == h3);
                CHECK (memcmp (f.px, want2, (size_t) w3 * h3 * sizeof *want2) == 0);
                if (shape)
                  CHECK (memcmp (f.mask, want2_mask, (size_t) w3 * h3) == 0);
                free (want2);
                free (want2_mask);
              }
              /* stretched back to the lifted size, turned as the two commands turn it: the lifted pixels flipped and turned, exactly */
              {
                uint32_t *l1, *l2;
                uint8_t *m1, *m2;
                int a, b, c, d;

                naive_apply (ops[k], ref, ref_mask, lw, lh, &l1, &m1, &a, &b);
                naive_apply (second, l1, m1, a, b, &l2, &m2, &c, &d);
                lifted_want = l2;
                lifted_mask_want = m2;
                lw2 = c;
                lh2 = d;
                free (l1);
                free (m1);
              }
              CHECK (easel_selection_stretch (&f, 3, 4, lw2, lh2, SIZE_MAX) == EASEL_SELECTION_DONE);
              CHECK (memcmp (f.px, lifted_want, (size_t) lw2 * lh2 * sizeof *lifted_want) == 0);
              if (shape)
                CHECK (memcmp (f.mask, lifted_mask_want, (size_t) lw2 * lh2) == 0);
              easel_float_free (&f);
              free (lifted_want);
              free (lifted_mask_want);
              free (want_px);
              free (want_mask);
              free (shown_px);
              free (shown_mask);
              free (ref);
              free (ref_mask);
              easel_image_free (img);
            }
}

static void
as7_transform_stretched_float (void)
{
  check_transform_of_shown (false);
}

static void
shape_oriented_with_pixels (void)
{
  check_transform_of_shown (true);
}

/* A stretch made lazily (the part in view while a handle is held, REQ-0015-05) of an oriented float gives, once completed, the pixels and
   shape of the full stretch of the same float, also across the 256-pixel blocks and for a shape the nearest pixel loses. */
static void
oriented_lazy_equals_whole (void)
{
  static const EaselOp ops[] = { EASEL_OP_FLIP_H, EASEL_OP_FLIP_V, EASEL_OP_ROTATE_90, EASEL_OP_ROTATE_180, EASEL_OP_ROTATE_270 };

  srand (21);
  for (int round = 0; round < 30; round++)
    {
      EaselImage *img = easel_image_new (80, 60, 0), *img2 = easel_image_new (80, 60, 0);
      EaselFloat whole = { 0 }, lazy = { 0 };
      uint32_t *ref, *ref2;
      uint8_t *ref_mask, *ref_mask2;
      bool shape = round % 2;
      int w = 3 + rand () % 30, h = 3 + rand () % 20, W = 1 + rand () % 700, H = 1 + rand () % 600;
      EaselOp op = ops[round % (sizeof ops / sizeof *ops)];

      if (round % 8 == 3)
        W = 257 + 256 * (round % 3);
      if (round % 8 == 5)
        H = 257 + 256 * (round % 2);
      CHECK (img != NULL && img2 != NULL);
      srand (1000 + round);
      lifted_block (img, &whole, w, h, shape, &ref, &ref_mask);
      srand (1000 + round);
      lifted_block (img2, &lazy, w, h, shape, &ref2, &ref_mask2);
      CHECK (easel_selection_transform (&whole, op, SIZE_MAX) == EASEL_SELECTION_DONE);
      CHECK (easel_selection_transform (&lazy, op, SIZE_MAX) == EASEL_SELECTION_DONE);
      CHECK (easel_selection_stretch (&whole, 4, 5, W, H, SIZE_MAX) == EASEL_SELECTION_DONE);
      CHECK (easel_selection_stretch_lazy (&lazy, 4, 5, W, H, SIZE_MAX) == EASEL_SELECTION_DONE);
      easel_selection_fill (&lazy, 0, 0, W < 300 ? W : 300, H < 300 ? H : 300);
      easel_selection_complete (&lazy);
      CHECK (lazy.w == whole.w && lazy.h == whole.h);
      CHECK (memcmp (lazy.px, whole.px, (size_t) W * H * sizeof *lazy.px) == 0);
      if (shape)
        CHECK (memcmp (lazy.mask, whole.mask, (size_t) W * H) == 0);
      easel_float_free (&whole);
      easel_float_free (&lazy);
      free (ref);
      free (ref2);
      free (ref_mask);
      free (ref_mask2);
      easel_image_free (img);
      easel_image_free (img2);
    }
}

/* The small sizes where the nearest pixel loses a shape (an L of 3 to 6 square lifted, made 1 to 4 wide and high), lazily and whole, for
   every command: the pixel kept at the completion is put where the orientation puts it (REQ-0018-07). */
static void
oriented_lazy_shape_small (void)
{
  static const EaselOp ops[] = { EASEL_OP_FLIP_H, EASEL_OP_FLIP_V, EASEL_OP_ROTATE_90, EASEL_OP_ROTATE_180, EASEL_OP_ROTATE_270 };

  for (int lw = 3; lw <= 6; lw++)
    for (int lh = 3; lh <= 6; lh++)
      for (int W = 1; W <= 4; W++)
        for (int H = 1; H <= 4; H++)
          for (size_t k = 0; k < sizeof ops / sizeof *ops; k++)
            {
              EaselImage *img = easel_image_new (20, 20, 0);
              EaselFloat whole = { 0 }, lazy = { 0 };
              uint32_t *ref, *ref2;
              uint8_t *ref_mask, *ref_mask2;

              srand (7);
              lifted_block (img, &whole, lw, lh, true, &ref, &ref_mask);
              srand (7);
              lifted_block (img, &lazy, lw, lh, true, &ref2, &ref_mask2);
              CHECK (easel_selection_transform (&whole, ops[k], SIZE_MAX) == EASEL_SELECTION_DONE);
              CHECK (easel_selection_transform (&lazy, ops[k], SIZE_MAX) == EASEL_SELECTION_DONE);
              CHECK (easel_selection_stretch (&whole, 2, 2, W, H, SIZE_MAX) == EASEL_SELECTION_DONE);
              CHECK (easel_selection_stretch_lazy (&lazy, 2, 2, W, H, SIZE_MAX) == EASEL_SELECTION_DONE);
              easel_selection_complete (&lazy);
              CHECK (memcmp (lazy.px, whole.px, (size_t) W * H * sizeof *lazy.px) == 0);
              CHECK (memcmp (lazy.mask, whole.mask, (size_t) W * H) == 0);
              easel_float_free (&whole);
              easel_float_free (&lazy);
              free (ref);
              free (ref2);
              free (ref_mask);
              free (ref_mask2);
              easel_image_free (img);
            }
}

/* Invert Colors of a floating selection (REQ-0018-05, -07): the lifted pixels and the shown ones are inverted (alpha kept), the shape
   not; the copies left before keep their pixels, whichever image they were made with (the unstretched one, the one before a stretch,
   the stretched one); twice gives the float back; a refused command (the memory) changes nothing. */
static void
selection_invert (void)
{
  EaselImage *img = easel_image_new (60, 40, 0);
  EaselFloat f = { 0 };
  uint32_t *ref, before_copies[60 * 40], after[60 * 40], *shown, *orig;
  uint8_t *ref_mask;
  size_t shown_n, orig_n;

  CHECK (img != NULL);
  srand (11);
  lifted_block (img, &f, 4, 3, true, &ref, &ref_mask);
  CHECK (easel_selection_stamp (&f, SIZE_MAX));                                   /* a copy of the unstretched image */
  CHECK (easel_selection_stretch (&f, 20, 10, 8, 6, SIZE_MAX) == EASEL_SELECTION_DONE);
  CHECK (easel_selection_stamp (&f, SIZE_MAX));                                   /* a copy of the stretched image */
  f.x = 30;
  f.y = 20;
  easel_selection_lay (&f, false, img, 0, 0, 60, 40, before_copies, 60);
  shown_n = (size_t) f.w * f.h;
  orig_n = (size_t) f.ow * f.oh;
  shown = malloc (shown_n * sizeof *shown);
  orig = malloc (orig_n * sizeof *orig);
  CHECK (shown != NULL && orig != NULL);
  memcpy (shown, f.px, shown_n * sizeof *shown);
  memcpy (orig, f.orig_px, orig_n * sizeof *orig);
  /* refused while the copies use the images: nothing changes */
  {
    uint32_t *before_px = f.px, *before_orig = f.orig_px;

    CHECK (easel_selection_transform (&f, EASEL_OP_INVERT, 3) == EASEL_SELECTION_NO_MEMORY);
    CHECK (f.px == before_px && f.orig_px == before_orig && memcmp (f.px, shown, shown_n * sizeof *shown) == 0);
    /* the shown image (8 x 6, 192 bytes) is the one over the limit; the lifted one (4 x 3 and its mask, 60 bytes) is not */
    CHECK (easel_selection_transform (&f, EASEL_OP_INVERT, 100) == EASEL_SELECTION_NO_MEMORY);
    CHECK (f.px == before_px && f.orig_px == before_orig && memcmp (f.px, shown, shown_n * sizeof *shown) == 0);
  }
  CHECK (easel_selection_transform (&f, EASEL_OP_INVERT, SIZE_MAX) == EASEL_SELECTION_DONE);
  for (size_t i = 0; i < shown_n; i++)
    CHECK (f.px[i] == (shown[i] ^ 0x00FFFFFFu));
  for (size_t i = 0; i < orig_n; i++)
    CHECK (f.orig_px[i] == (orig[i] ^ 0x00FFFFFFu));
  CHECK (f.mask != NULL && f.orig_mask != NULL && f.w == 8 && f.h == 6 && f.x == 30 && f.y == 20);
  easel_selection_lay (&f, false, img, 0, 0, 60, 40, after, 60);
  CHECK (memcmp (after, before_copies, sizeof after) == 0);                        /* the copies did not change */
  CHECK (easel_selection_transform (&f, EASEL_OP_INVERT, SIZE_MAX) == EASEL_SELECTION_DONE);
  CHECK (memcmp (f.px, shown, shown_n * sizeof *shown) == 0 && memcmp (f.orig_px, orig, orig_n * sizeof *orig) == 0);
  easel_selection_lay (&f, false, img, 0, 0, 60, 40, after, 60);
  CHECK (memcmp (after, before_copies, sizeof after) == 0);
  easel_float_free (&f);
  free (shown);
  free (orig);
  free (ref);
  free (ref_mask);
  /* made smaller than it was lifted, the lifted image is the one over the limit */
  CHECK (easel_selection_lift (&f, img, 2, 2, 8, 6, NULL, false, YELLOW, SIZE_MAX) == EASEL_SELECTION_DONE);
  CHECK (easel_selection_stamp (&f, SIZE_MAX));                                   /* a copy of the lifted image */
  CHECK (easel_selection_stretch (&f, 2, 2, 2, 2, SIZE_MAX) == EASEL_SELECTION_DONE);
  {
    uint32_t *before_px = f.px, *before_orig = f.orig_px;

    CHECK (easel_selection_transform (&f, EASEL_OP_INVERT, 100) == EASEL_SELECTION_NO_MEMORY);   /* 16 bytes shown, 192 lifted */
    CHECK (f.px == before_px && f.orig_px == before_orig);
  }
  easel_float_free (&f);
  /* with no copy the images are inverted where they are: no allocation, so no limit refuses it, and the images are the same buffers */
  CHECK (easel_selection_lift (&f, img, 2, 2, 8, 6, NULL, false, YELLOW, SIZE_MAX) == EASEL_SELECTION_DONE);
  CHECK (easel_selection_stretch (&f, 2, 2, 2, 2, SIZE_MAX) == EASEL_SELECTION_DONE);
  {
    uint32_t *before_px = f.px, *before_orig = f.orig_px, first = f.px[0], first_orig = f.orig_px[0];

    CHECK (easel_selection_transform (&f, EASEL_OP_INVERT, 3) == EASEL_SELECTION_DONE);
    CHECK (f.px == before_px && f.orig_px == before_orig);
    CHECK (f.px[0] == (first ^ 0x00FFFFFFu) && f.orig_px[0] == (first_orig ^ 0x00FFFFFFu));
  }
  easel_float_free (&f);
  /* an unstretched float with a copy: the same */
  CHECK (easel_selection_lift (&f, img, 2, 2, 5, 4, NULL, false, YELLOW, SIZE_MAX) == EASEL_SELECTION_DONE);
  CHECK (easel_selection_stamp (&f, SIZE_MAX));
  f.x = 20;
  f.y = 20;
  easel_selection_lay (&f, false, img, 0, 0, 60, 40, before_copies, 60);
  {
    uint32_t first = f.px[0];

    CHECK (easel_selection_transform (&f, EASEL_OP_INVERT, SIZE_MAX) == EASEL_SELECTION_DONE);
    CHECK (f.px[0] == (first ^ 0x00FFFFFFu) && f.orig_px == NULL);
  }
  easel_selection_lay (&f, false, img, 0, 0, 60, 40, after, 60);
  CHECK (memcmp (after, before_copies, sizeof after) == 0);
  easel_float_free (&f);
  easel_image_free (img);
}

/* ---- 0018 T006 (REQ-0018-11): a handle dragged past the opposite edge mirrors ---- */

/* The rectangles a handle gives by the spec (REQ-0018-11), worked out by hand: the extent runs from the opposite edge, which stays, to
   the dragged edge, at least 1 pixel; with the dragged edge on the opposite edge it is 1 pixel and not mirrored (on the side it was);
   past it, the extent is the distance and the axis is mirrored; each side limited to 10000; with Shift a corner keeps the ratio of the
   sizes at the press, the side that does not lead running from its opposite edge toward the pointer's side, mirrored when the pointer
   is past it. */
static void
as13_stretch_rect_mirror (void)
{
  static const struct
  {
    EaselSelHandle handle;
    int x, y, w, h, dx, dy;
    bool shift;
    int nx, ny, nw, nh;
    bool mx, my;
  } cases[] = {
    /* the right handle of 4 x 2 at (10, 10), edge 14, opposite edge 10 */
    { EASEL_SEL_HANDLE_R, 10, 10, 4, 2, 2, 0, false, 10, 10, 6, 2, false, false },
    { EASEL_SEL_HANDLE_R, 10, 10, 4, 2, -3, 0, false, 10, 10, 1, 2, false, false },          /* edge at 11: 1 pixel */
    { EASEL_SEL_HANDLE_R, 10, 10, 4, 2, -4, 0, false, 10, 10, 1, 2, false, false },          /* exactly on the opposite edge */
    { EASEL_SEL_HANDLE_R, 10, 10, 4, 2, -5, 0, false, 9, 10, 1, 2, true, false },            /* edge at 9: 1 pixel, mirrored */
    { EASEL_SEL_HANDLE_R, 10, 10, 4, 2, -6, 0, false, 8, 10, 2, 2, true, false },            /* edge at 8 */
    /* the left handle: edge 10, opposite edge 14 */
    { EASEL_SEL_HANDLE_L, 10, 10, 4, 2, -2, 0, false, 8, 10, 6, 2, false, false },
    { EASEL_SEL_HANDLE_L, 10, 10, 4, 2, 4, 0, false, 13, 10, 1, 2, false, false },           /* on the opposite edge: 1 pixel, its side */
    { EASEL_SEL_HANDLE_L, 10, 10, 4, 2, 5, 0, false, 14, 10, 1, 2, true, false },
    { EASEL_SEL_HANDLE_L, 10, 10, 4, 2, 7, 0, false, 14, 10, 3, 2, true, false },
    /* the bottom handle: edge 12, opposite edge 10; the top: edge 10, opposite edge 12 */
    { EASEL_SEL_HANDLE_B, 10, 10, 4, 2, 0, -3, false, 10, 9, 4, 1, false, true },
    { EASEL_SEL_HANDLE_B, 10, 10, 4, 2, 0, -2, false, 10, 10, 4, 1, false, false },
    { EASEL_SEL_HANDLE_T, 10, 10, 4, 2, 0, 5, false, 10, 12, 4, 3, false, true },
    { EASEL_SEL_HANDLE_T, 10, 10, 4, 2, 0, 2, false, 10, 11, 4, 1, false, false },
    /* corners: each axis on its own */
    { EASEL_SEL_HANDLE_BR, 10, 10, 4, 2, -6, -3, false, 8, 9, 2, 1, true, true },
    { EASEL_SEL_HANDLE_BR, 10, 10, 4, 2, -6, 2, false, 8, 10, 2, 4, true, false },
    { EASEL_SEL_HANDLE_TL, 10, 10, 4, 2, 6, 4, false, 14, 12, 2, 2, true, true },
    { EASEL_SEL_HANDLE_TL, 10, 10, 4, 2, -2, -1, false, 8, 9, 6, 3, false, false },
    /* the limit of 10000, also mirrored */
    { EASEL_SEL_HANDLE_R, 10, 10, 4, 2, 20000, 0, false, 10, 10, 10000, 2, false, false },
    { EASEL_SEL_HANDLE_R, 10, 10, 4, 2, -20000, 0, false, -9990, 10, 10000, 2, true, false },
    { EASEL_SEL_HANDLE_L, 10, 10, 4, 2, 20000, 0, false, 14, 10, 10000, 2, true, false },
    /* Shift on a corner of 5 x 4 at (1, 1) (0015's selection): the sizes keep their ratio */
    { EASEL_SEL_HANDLE_BR, 1, 1, 5, 4, -15, -6, true, -9, -7, 10, 8, true, true },           /* AS-14: the width leads, twice its size */
    { EASEL_SEL_HANDLE_BR, 1, 1, 5, 4, 5, 0, true, 1, 1, 10, 8, false, false },
    { EASEL_SEL_HANDLE_BR, 1, 1, 5, 4, -10, 0, true, -4, 1, 5, 4, true, false },             /* a tie: the width leads; the height keeps its size */
    { EASEL_SEL_HANDLE_BR, 1, 1, 5, 4, 0, -10, true, 1, -5, 8, 6, false, true },             /* the height leads (1.5); the width 7.5 -> 8 */
    { EASEL_SEL_HANDLE_TL, 1, 1, 5, 4, 10, 8, true, 6, 5, 5, 4, true, true },
    { EASEL_SEL_HANDLE_BR, 1, 1, 5, 4, 5, -6, true, 1, -7, 10, 8, false, true },             /* the follower is mirrored, the leader not */
    { EASEL_SEL_HANDLE_BR, 1, 1, 5, 4, 20000, 0, true, 1, 1, 10000, 8000, false, false },
    /* Shift changes nothing for an edge handle */
    { EASEL_SEL_HANDLE_R, 1, 1, 5, 4, -10, 0, true, -4, 1, 5, 4, true, false },
  };

  for (size_t k = 0; k < sizeof cases / sizeof *cases; k++)
    {
      int nx, ny, nw, nh;
      bool mx, my;

      easel_stretch_rect_mirror (cases[k].handle, cases[k].x, cases[k].y, cases[k].w, cases[k].h, cases[k].dx, cases[k].dy,
                                 cases[k].shift, &nx, &ny, &nw, &nh, &mx, &my);
      if (nx != cases[k].nx || ny != cases[k].ny || nw != cases[k].nw || nh != cases[k].nh || mx != cases[k].mx || my != cases[k].my)
        {
          fprintf (stderr, "case %zu: got (%d, %d, %d, %d) mirrored %d %d, want (%d, %d, %d, %d) mirrored %d %d\n", k, nx, ny, nw, nh,
                   mx, my, cases[k].nx, cases[k].ny, cases[k].nw, cases[k].nh, cases[k].mx, cases[k].my);
          exit (1);
        }
    }
}

/* A float stretched with an orientation of its own (the base orientation with the drag's mirrors): the lifted 4 x 1 pixels a b c d made 2
   wide show b d, with a flip of x d b; whole and lazily made the same; the float keeps the orientation it was given, so that the next
   stretch starts from it. */
static void
stretch_with_orientation (void)
{
  for (int lazy = 0; lazy < 2; lazy++)
    {
      EaselImage *img = easel_image_new (20, 20, 0);
      EaselFloat f = { 0 };
      uint32_t *ref;
      uint8_t *ref_mask;

      srand (5);
      lifted_block (img, &f, 4, 1, false, &ref, &ref_mask);
      if (lazy)
        CHECK (easel_selection_stretch_lazy_oriented (&f, 3, 4, 2, 1, EASEL_ORIENT_FLIP_X, SIZE_MAX) == EASEL_SELECTION_DONE);
      else
        CHECK (easel_selection_stretch_oriented (&f, 3, 4, 2, 1, EASEL_ORIENT_FLIP_X, SIZE_MAX) == EASEL_SELECTION_DONE);
      easel_selection_complete (&f);
      CHECK (f.orient == EASEL_ORIENT_FLIP_X && f.w == 2 && f.h == 1 && f.x == 3 && f.y == 4);
      CHECK (f.px[0] == ref[3] && f.px[1] == ref[1]);                                   /* d b */
      CHECK (easel_selection_stretch (&f, 3, 4, 4, 1, SIZE_MAX) == EASEL_SELECTION_DONE);   /* the orientation is kept: d c b a */
      CHECK (f.px[0] == ref[3] && f.px[1] == ref[2] && f.px[2] == ref[1] && f.px[3] == ref[0]);
      CHECK (easel_selection_stretch_oriented (&f, 3, 4, 2, 1, EASEL_ORIENT_IDENTITY, SIZE_MAX) == EASEL_SELECTION_DONE);
      CHECK (f.orient == EASEL_ORIENT_IDENTITY && f.px[0] == ref[1] && f.px[1] == ref[3]);   /* b d again */
      easel_float_free (&f);
      free (ref);
      free (ref_mask);
      easel_image_free (img);
    }
}

/* ---- 0041 T002: the marked crop (REQ-0041-02, -05, -06, -07) ---- */

/* A w x h picture whose pixels are all different and none symmetric to another, so a crop from the wrong place or in the wrong order shows. */
static EaselImage *
distinct_picture (int w, int h)
{
  EaselImage *img = easel_image_new (w, h, WHITE);

  CHECK (img != NULL);
  for (size_t i = 0; i < (size_t) w * h; i++)
    img->px[i] = 0xFF000000u | (uint32_t) ((i + 1) * 2654435761u >> 8);
  return img;
}

/* The crop is the old picture's rectangle, exactly: pixel (i, j) of it is the old pixel (x + i, y + j). */
static void
check_is_cut (const EaselImage *img, const EaselImage *old, int x, int y, int w, int h)
{
  CHECK (img->width == w && img->height == h);
  for (int j = 0; j < h; j++)
    for (int i = 0; i < w; i++)
      CHECK (img->px[(size_t) j * w + i] == old->px[(size_t) (y + j) * old->width + x + i]);
}

static void
check_same (const EaselImage *a, const EaselImage *b)
{
  CHECK (a->width == b->width && a->height == b->height);
  CHECK (memcmp (a->px, b->px, (size_t) a->width * a->height * sizeof a->px[0]) == 0);
}

/* AS-2 (core part): a 100 x 60 piece of a 300 x 200 picture at (40, 30): every pixel the old one shifted, one step that Undo and Redo
   give back exactly, counted as 4 bytes a removed pixel plus the step's 64, and held as counted (REQ-0041-04, REQ-0002-16). */
static void
crop_marked_offset (void)
{
  EaselImage *img = distinct_picture (300, 200), *old = copy_of (img), *cut;
  EaselHistory *h = easel_history_new ((size_t) 512 << 20);
  size_t before, held, counted, removed = (size_t) 300 * 200 - 100 * 60;

  CHECK (h != NULL);
  before = heap_in_use ();
  CHECK (easel_selection_crop (NULL, img, h, 40, 30, 100, 60, 0xFF00FF00u, SIZE_MAX) == EASEL_SELECTION_DONE);
  held = heap_in_use () - before + (size_t) 300 * 200 * 4 - (size_t) 100 * 60 * 4;      /* less the buffer swapped for the old one */
  check_is_cut (img, old, 40, 30, 100, 60);
  counted = easel_history_bytes (h);
  CHECK (counted == removed * 4 + easel_history_change_log_bytes (0));
  printf ("  held %zu bytes, counted %zu\n", held, counted);
  CHECK (held <= counted + counted / 100 + 4096);
  cut = copy_of (img);
  CHECK (easel_history_undo (h, img));
  check_same (img, old);
  CHECK (!easel_history_can_undo (h) && easel_history_can_redo (h));
  CHECK (easel_history_redo (h, img));
  check_same (img, cut);
  easel_image_free (cut);
  easel_image_free (old);
  easel_history_free (h);
  easel_image_free (img);
}

/* AS-3 (core part): the rectangle of a Free-Form selection is cropped whole, the pixels outside its shape kept: a picture of two
   colors in a checker, a crop of the box (10, 10) to (59, 49) keeps both. */
static void
crop_free_form_box (void)
{
  EaselImage *img = easel_image_new (80, 60, WHITE), *old;
  EaselHistory *h = easel_history_new ((size_t) 512 << 20);
  int reds = 0;

  CHECK (img != NULL && h != NULL);
  for (int y = 0; y < 60; y++)
    for (int x = 0; x < 80; x++)
      img->px[y * 80 + x] = (x + y) % 3 == 0 ? RED : (x > y ? BLUE : WHITE);       /* a triangle's worth of one color, the rest of others */
  old = copy_of (img);
  CHECK (easel_selection_crop (NULL, img, h, 10, 10, 50, 40, YELLOW, SIZE_MAX) == EASEL_SELECTION_DONE);
  check_is_cut (img, old, 10, 10, 50, 40);
  for (int i = 0; i < 50 * 40; i++)
    reds += img->px[i] == RED;
  CHECK (reds > 100);
  easel_image_free (old);
  easel_history_free (h);
  easel_image_free (img);
}

/* AS-5 (core part): the rectangle of the whole picture changes nothing and adds no step (REQ-0041-05, as REQ-0004-24); a 1 x 1 piece
   is a 1 x 1 picture. */
static void
crop_whole_is_nothing (void)
{
  EaselImage *img = distinct_picture (30, 20), *old = copy_of (img);
  EaselHistory *h = easel_history_new ((size_t) 512 << 20);

  CHECK (h != NULL);
  CHECK (easel_selection_crop (NULL, img, h, 0, 0, 30, 20, WHITE, SIZE_MAX) == EASEL_SELECTION_NOTHING);
  check_same (img, old);
  CHECK (!easel_history_can_undo (h) && easel_history_bytes (h) == 0);
  CHECK (easel_selection_crop (NULL, img, h, 7, 5, 1, 1, WHITE, SIZE_MAX) == EASEL_SELECTION_DONE);
  check_is_cut (img, old, 7, 5, 1, 1);
  CHECK (easel_history_undo (h, img));
  check_same (img, old);
  easel_image_free (old);
  easel_history_free (h);
  easel_image_free (img);
}

/* AS-6 (core part), REQ-0041-06: an allocation above max_bytes, or one that cannot be had, leaves the picture and the history as they were.
   Each allocation is counted by itself (the removed pixels, the new picture), as the put-down's and a transform's are. */
static void
crop_no_memory (void)
{
  EaselImage *img = distinct_picture (20, 20), *old = copy_of (img);
  EaselHistory *h = easel_history_new ((size_t) 512 << 20);
  size_t kept, removed = (size_t) (400 - 19 * 19) * 4, buffer = (size_t) 19 * 19 * 4;    /* 156 and 1444 bytes */

  CHECK (h != NULL && removed < buffer);
  /* the removed pixels do not fit */
  CHECK (easel_selection_crop (NULL, img, h, 0, 0, 19, 19, WHITE, removed - 1) == EASEL_SELECTION_NO_MEMORY);
  check_same (img, old);
  CHECK (!easel_history_can_undo (h) && easel_history_bytes (h) == 0);
  /* they do, the new picture does not */
  CHECK (easel_selection_crop (NULL, img, h, 0, 0, 19, 19, WHITE, buffer - 1) == EASEL_SELECTION_NO_MEMORY);
  check_same (img, old);
  CHECK (!easel_history_can_undo (h) && easel_history_bytes (h) == 0);
  /* each fits alone: the crop is made */
  CHECK (easel_selection_crop (NULL, img, h, 0, 0, 19, 19, WHITE, buffer) == EASEL_SELECTION_DONE);
  check_is_cut (img, old, 0, 0, 19, 19);
  CHECK (easel_history_undo (h, img));
  check_same (img, old);
  /* a picture of no side, or of more than EASEL_IMAGE_MAX_SIZE, cannot be made: nothing changes */
  CHECK (easel_selection_crop (NULL, img, h, 0, 0, 0, 5, WHITE, SIZE_MAX) == EASEL_SELECTION_NO_MEMORY);
  CHECK (easel_selection_crop (NULL, img, h, 0, 0, 5, 0, WHITE, SIZE_MAX) == EASEL_SELECTION_NO_MEMORY);
  CHECK (easel_selection_crop (NULL, img, h, 0, 0, EASEL_IMAGE_MAX_SIZE + 1, 5, WHITE, SIZE_MAX) == EASEL_SELECTION_NO_MEMORY);
  CHECK (easel_selection_crop (NULL, img, h, 0, 0, 5, EASEL_IMAGE_MAX_SIZE + 1, WHITE, SIZE_MAX) == EASEL_SELECTION_NO_MEMORY);
  check_same (img, old);
  /* with no history to refuse it, the size rule is the function's own */
  CHECK (easel_selection_crop (NULL, img, NULL, 0, 0, 0, 5, WHITE, SIZE_MAX) == EASEL_SELECTION_NO_MEMORY);
  CHECK (easel_selection_crop (NULL, img, NULL, 0, 0, EASEL_IMAGE_MAX_SIZE + 1, 5, WHITE, SIZE_MAX) == EASEL_SELECTION_NO_MEMORY);
  check_same (img, old);
  /* the other order: the removed pixels (1536 bytes) are more than the new picture (64 bytes), so each check stands alone */
  kept = easel_history_bytes (h);                          /* the undone crop above stays redoable */
  CHECK (easel_selection_crop (NULL, img, h, 8, 8, 4, 4, WHITE, (size_t) (400 - 16) * 4 - 1) == EASEL_SELECTION_NO_MEMORY);
  check_same (img, old);
  CHECK (!easel_history_can_undo (h) && easel_history_can_redo (h) && easel_history_bytes (h) == kept);
  CHECK (easel_selection_crop (NULL, img, h, 8, 8, 4, 4, WHITE, (size_t) (400 - 16) * 4) == EASEL_SELECTION_DONE);
  check_is_cut (img, old, 8, 8, 4, 4);
  CHECK (easel_history_undo (h, img));
  check_same (img, old);
  /* a real shortage: the address space limited to a little above what is in use, a picture of 13 MB to build */
  {
    enum { N = 1800 };
    struct rlimit lim, was;
    EaselImage *big = distinct_picture (N, N), *big_old = copy_of (big);
    EaselHistory *h2 = easel_history_new ((size_t) 512 << 20);

    CHECK (h2 != NULL && getrlimit (RLIMIT_AS, &was) == 0);
    lim = was;
    lim.rlim_cur = address_space () + 4 * 1024 * 1024;
    CHECK (setrlimit (RLIMIT_AS, &lim) == 0);
    CHECK (easel_selection_crop (NULL, big, h2, 0, 0, N - 1, N - 1, WHITE, SIZE_MAX) == EASEL_SELECTION_NO_MEMORY);
    CHECK (setrlimit (RLIMIT_AS, &was) == 0);
    check_same (big, big_old);
    CHECK (!easel_history_can_undo (h2) && easel_history_bytes (h2) == 0);
    easel_history_free (h2);
    easel_image_free (big_old);
    easel_image_free (big);
  }
  easel_image_free (old);
  easel_history_free (h);
  easel_image_free (img);
}

/* The builder for a rectangle that hangs past the edges (a floating crop's, 0041 T003): the part inside the picture is the old pixels, the
   rest fill, a fully transparent fill #00000000 (P7.7); the removed pixels are those outside the rectangle; Undo and Redo give both back. */
static void
crop_hangs_fill (void)
{
  static const struct { int x, y, w, h; uint32_t fill; } cases[] = {
    { 10, 5, 12, 9, 0xFF00FF00u }, { -4, -2, 9, 7, 0xFF0000FFu }, { -6, -3, 30, 20, 0xFF00FFFFu }, { 40, 30, 4, 4, 0xFFFF00FFu },
    { -3, 4, 8, 3, 0x00123456u }, { 15, -5, 6, 30, 0xFFFFFF00u },
  };

  for (size_t c = 0; c < sizeof cases / sizeof cases[0]; c++)
    {
      EaselImage *img = distinct_picture (17, 11), *old = copy_of (img), *cut;
      EaselHistory *h = easel_history_new ((size_t) 512 << 20);
      int x = cases[c].x, y = cases[c].y, w = cases[c].w, hh = cases[c].h;
      uint32_t fill = (cases[c].fill >> 24) != 0 ? cases[c].fill : 0;

      CHECK (h != NULL);
      CHECK (easel_selection_crop (NULL, img, h, x, y, w, hh, cases[c].fill, SIZE_MAX) == EASEL_SELECTION_DONE);
      CHECK (img->width == w && img->height == hh);
      for (int j = 0; j < hh; j++)
        for (int i = 0; i < w; i++)
          {
            int ox = x + i, oy = y + j;
            uint32_t want = ox >= 0 && oy >= 0 && ox < 17 && oy < 11 ? old->px[oy * 17 + ox] : fill;

            CHECK (img->px[j * w + i] == want);
          }
      cut = copy_of (img);
      CHECK (easel_history_undo (h, img));
      check_same (img, old);
      CHECK (easel_history_redo (h, img));
      check_same (img, cut);
      easel_image_free (cut);
      easel_image_free (old);
      easel_history_free (h);
      easel_image_free (img);
    }
}

/* ---- 0041 T003: the floating crop (REQ-0041-03, -04, -07) ---- */

#define GREEN 0xFF00FF00u
#define BLACK 0xFF000000u

/* The pixels where a new picture differs from the plain cut of the old one (its pixels inside, fill outside), counted from the picture
   the crop made and the old one: what the change log must hold (REQ-0041-04). */
static size_t
differing_from_cut (const EaselImage *old, const EaselImage *now, int x, int y, uint32_t fill)
{
  size_t n = 0;

  for (int j = 0; j < now->height; j++)
    for (int i = 0; i < now->width; i++)
      {
        int ox = x + i, oy = y + j;
        uint32_t cut = ox >= 0 && oy >= 0 && ox < old->width && oy < old->height ? old->px[(size_t) oy * old->width + ox]
                                                                                    : ((fill >> 24) != 0 ? fill : 0);

        n += now->px[(size_t) j * now->width + i] != cut;
      }
  return n;
}

/* AS-4: a 20 x 20 floating selection of black pixels moved to (90, 90) of a 100 x 100 white picture, hanging 10 pixels past the right and
   bottom edges: the crop is 20 x 20 and all black, the pixels outside the picture too; Undo gives the picture as before the lift (it was
   never touched while the selection floated); the step is counted as 4 bytes a removed pixel plus the charge of its log of the 400
   pixels that differ from the plain cut (REQ-0041-03, -04). */
static void
crop_float_hanging (void)
{
  EaselImage *img = easel_image_new (100, 100, WHITE), *old, *cut;
  EaselHistory *h = easel_history_new ((size_t) 512 << 20);
  EaselFloat f = { 0 };

  CHECK (img != NULL && h != NULL);
  for (int y = 10; y < 30; y++)
    for (int x = 10; x < 30; x++)
      img->px[y * 100 + x] = BLACK;
  old = copy_of (img);
  CHECK (easel_selection_lift (&f, img, 10, 10, 20, 20, NULL, false, YELLOW, SIZE_MAX) == EASEL_SELECTION_DONE);
  f.x = f.y = 90;
  CHECK (easel_selection_crop (&f, img, h, 90, 90, 20, 20, 0xFFFF0000u, SIZE_MAX) == EASEL_SELECTION_DONE);
  CHECK (img->width == 20 && img->height == 20);
  for (int i = 0; i < 400; i++)
    CHECK (img->px[i] == BLACK);
  CHECK (easel_history_bytes (h) == (size_t) (10000 - 100) * 4 + easel_history_change_log_bytes (400));
  CHECK (differing_from_cut (old, img, 90, 90, 0xFFFF0000u) == 400);
  cut = copy_of (img);
  CHECK (easel_history_undo (h, img));
  check_same (img, old);
  CHECK (easel_history_redo (h, img));
  check_same (img, cut);
  easel_image_free (cut);
  easel_image_free (old);
  easel_float_free (&f);
  easel_history_free (h);
  easel_image_free (img);
}

/* AS-8: a 100 x 100 green picture, red as the fill, a Free-Form triangle of blue pixels floating so that its box (80, 20) to (119, 59) hangs 20
   pixels past the right edge: the triangle's pixels blue, those outside it and inside the old picture green, those outside both red. */
static void
crop_float_shape_fill (void)
{
  EaselImage *img = easel_image_new (100, 100, GREEN), *old;
  EaselHistory *h = easel_history_new ((size_t) 512 << 20);
  EaselFloat f = { 0 };
  uint8_t mask[40 * 40];
  const uint32_t RED_FILL = 0xFFFF0000u;

  CHECK (img != NULL && h != NULL);
  for (int y = 10; y < 50; y++)
    for (int x = 10; x < 50; x++)
      img->px[y * 100 + x] = BLUE;
  for (int j = 0; j < 40; j++)
    for (int i = 0; i < 40; i++)
      mask[j * 40 + i] = i <= j;                           /* the pixels at and under the diagonal */
  old = copy_of (img);
  CHECK (easel_selection_lift (&f, img, 10, 10, 40, 40, mask, false, YELLOW, SIZE_MAX) == EASEL_SELECTION_DONE);
  f.x = 80;
  f.y = 20;
  CHECK (easel_selection_crop (&f, img, h, 80, 20, 40, 40, RED_FILL, SIZE_MAX) == EASEL_SELECTION_DONE);
  CHECK (img->width == 40 && img->height == 40);
  for (int j = 0; j < 40; j++)
    for (int i = 0; i < 40; i++)
      {
        int ox = 80 + i;
        uint32_t want = i <= j ? BLUE : (ox < 100 ? GREEN : RED_FILL);

        CHECK (img->px[j * 40 + i] == want);
      }
  CHECK (easel_history_bytes (h) == (size_t) (10000 - 20 * 40) * 4 + easel_history_change_log_bytes (differing_from_cut (old, img, 80, 20, RED_FILL)));
  CHECK (easel_history_undo (h, img));
  check_same (img, old);
  easel_image_free (old);
  easel_float_free (&f);
  easel_history_free (h);
  easel_image_free (img);
}

/* Inside the old picture the crop is what the float's lay gives (the picture, the hole, the copies, then the float: what the view shows and
   the put-down writes, REQ-0041-07); outside it the float's own pixels over the fill, the copies there left out. */
static void
crop_float_equals_lay (void)
{
  EaselImage *img = distinct_picture (60, 40), *old = copy_of (img), *lay;
  EaselHistory *h = easel_history_new ((size_t) 512 << 20);
  EaselFloat f = { 0 };
  const uint32_t FILL = 0xFF123456u;
  int x = 40, y = 25, w = 30, hh = 25;                      /* the float's box, hanging past the right and bottom edges */

  CHECK (h != NULL);
  CHECK (easel_selection_lift (&f, img, 5, 5, w, hh, NULL, false, YELLOW, SIZE_MAX) == EASEL_SELECTION_DONE);
  f.x = 20;
  f.y = 10;
  CHECK (easel_selection_stamp (&f, SIZE_MAX));             /* a copy inside the picture ... */
  f.x = 56;
  f.y = 30;
  CHECK (easel_selection_stamp (&f, SIZE_MAX));             /* ... and one hanging past the edges, which the crop does not have */
  f.x = x;
  f.y = y;
  lay = copy_of (img);
  easel_selection_lay (&f, true, old, 0, 0, 60, 40, lay->px, 60);
  CHECK (easel_selection_crop (&f, img, h, x, y, w, hh, FILL, SIZE_MAX) == EASEL_SELECTION_DONE);
  CHECK (img->width == w && img->height == hh);
  for (int j = 0; j < hh; j++)
    for (int i = 0; i < w; i++)
      {
        int ox = x + i, oy = y + j;
        uint32_t want = ox < 60 && oy < 40 ? lay->px[oy * 60 + ox] : f.px[j * w + i];     /* outside: the float's own, whole here */

        CHECK (img->px[j * w + i] == want);
      }
  CHECK (easel_history_undo (h, img));
  check_same (img, old);
  easel_image_free (lay);
  easel_image_free (old);
  easel_float_free (&f);
  easel_history_free (h);
  easel_image_free (img);
}

/* REQ-0041-04: Redo gives the same pixels, the fill outside the old picture the step's own. */
static void
crop_redo_keeps_fill (void)
{
  EaselImage *img = easel_image_new (30, 20, WHITE), *first;
  EaselHistory *h = easel_history_new ((size_t) 512 << 20);
  EaselFloat f = { 0 };
  uint8_t mask[10 * 10];

  CHECK (img != NULL && h != NULL);
  for (int k = 0; k < 100; k++)
    mask[k] = (k % 10) + (k / 10) < 12;                      /* a corner cut off */
  CHECK (easel_selection_lift (&f, img, 0, 0, 10, 10, mask, true, YELLOW, SIZE_MAX) == EASEL_SELECTION_DONE);
  f.x = 25;
  f.y = 15;
  CHECK (easel_selection_crop (&f, img, h, 25, 15, 10, 10, 0xFF800000u, SIZE_MAX) == EASEL_SELECTION_DONE);
  first = copy_of (img);
  CHECK (img->px[0] == WHITE && img->px[9 * 10 + 9] == 0xFF800000u);     /* in the picture the float's, outside the shape and the picture the fill */
  CHECK (easel_history_undo (h, img) && img->width == 30);
  CHECK (easel_history_redo (h, img));
  check_same (img, first);
  CHECK (easel_history_undo (h, img) && easel_history_redo (h, img));
  check_same (img, first);
  easel_image_free (first);
  easel_float_free (&f);
  easel_history_free (h);
  easel_image_free (img);
}

/* REQ-0002-16: the memory a floating crop's step holds is the memory the budget counts (the removed pixels and the log, each at its length). */
static void
crop_float_step_holds_what_it_counts (void)
{
  EaselImage *img = easel_image_new (600, 600, WHITE);
  EaselHistory *h = easel_history_new ((size_t) 512 << 20);
  EaselFloat f = { 0 };
  size_t before, held, counted;

  CHECK (img != NULL && h != NULL);
  for (int y = 0; y < 600; y++)
    for (int x = 0; x < 600; x++)
      img->px[y * 600 + x] = (x & 1) ? RED : WHITE;
  CHECK (easel_selection_lift (&f, img, 0, 0, 513, 512, NULL, false, YELLOW, SIZE_MAX) == EASEL_SELECTION_DONE);
  f.x = f.y = 300;
  before = heap_in_use ();
  CHECK (easel_selection_crop (&f, img, h, 300, 300, 513, 512, BLUE, SIZE_MAX) == EASEL_SELECTION_DONE);
  held = heap_in_use () - before + (size_t) 600 * 600 * 4 - (size_t) 513 * 512 * 4;
  counted = easel_history_bytes (h);
  printf ("  held %zu bytes, counted %zu\n", held, counted);
  CHECK (held <= counted + counted / 100 + 4096);
  easel_float_free (&f);
  easel_history_free (h);
  easel_image_free (img);
}

/* REQ-0041-06: the log is the third allocation, counted by itself: a rectangle that holds the whole picture removes nothing, the new picture is
   1936 bytes and the log of its 400 changed pixels (the float's) 4800; one byte less and the crop is refused with the picture, the history and the float as
   they were. */
static void
crop_float_no_memory (void)
{
  EaselImage *img = easel_image_new (20, 20, WHITE), *old;
  EaselHistory *h = easel_history_new ((size_t) 512 << 20);
  EaselFloat f = { 0 };

  CHECK (img != NULL && h != NULL);
  CHECK (easel_selection_lift (&f, img, 0, 0, 20, 20, NULL, true, YELLOW, SIZE_MAX) == EASEL_SELECTION_DONE);
  for (int i = 0; i < 400; i++)
    f.px[i] = BLACK;
  f.x = f.y = -1;
  old = copy_of (img);
  CHECK (easel_selection_crop (&f, img, h, -1, -1, 22, 22, WHITE, 400 * 12 - 1) == EASEL_SELECTION_NO_MEMORY);
  check_same (img, old);
  CHECK (!easel_history_can_undo (h) && easel_history_bytes (h) == 0);
  CHECK (f.x == -1 && f.y == -1 && f.px[0] == BLACK);
  CHECK (easel_selection_crop (&f, img, h, -1, -1, 22, 22, WHITE, 400 * 12) == EASEL_SELECTION_DONE);
  CHECK (img->width == 22 && img->height == 22 && img->px[0] == BLACK && img->px[20] == WHITE && img->px[21 * 22 + 21] == WHITE);
  CHECK (easel_history_bytes (h) == easel_history_change_log_bytes (400));
  CHECK (easel_history_undo (h, img));
  check_same (img, old);
  easel_image_free (old);
  easel_float_free (&f);
  easel_history_free (h);
  easel_image_free (img);
}

/* A pasted float's fully transparent pixels leave the picture as it is (REQ-0016-05): in the crop too, inside the old picture (the picture
   shows) and outside it (the fill shows, since nothing is laid there). */
static void
crop_float_paste_skips_clear (void)
{
  EaselImage *img = easel_image_new (10, 10, WHITE), *pic = easel_image_new (4, 4, RED);
  EaselHistory *h = easel_history_new ((size_t) 512 << 20);
  EaselFloat f = { 0 };

  CHECK (img != NULL && pic != NULL && h != NULL);
  pic->px[0] = 0;                                           /* (8, 8): inside the picture */
  pic->px[3] = 0;                                           /* (11, 8): outside it */
  pic->px[3 * 4 + 3] = 0;                                   /* (11, 11): outside it */
  CHECK (easel_selection_paste (&f, pic, 8, 8, SIZE_MAX) == EASEL_SELECTION_DONE);
  CHECK (easel_selection_crop (&f, img, h, 8, 8, 4, 4, GREEN, SIZE_MAX) == EASEL_SELECTION_DONE);
  CHECK (img->px[0] == WHITE);                              /* the picture's own pixel */
  CHECK (img->px[1] == RED && img->px[4] == RED);
  CHECK (img->px[3] == GREEN && img->px[15] == GREEN);      /* the fill where nothing is laid */
  CHECK (img->px[2 * 4 + 2] == RED);
  easel_float_free (&f);
  easel_image_free (pic);
  easel_history_free (h);
  easel_image_free (img);
}

/* A floating selection exactly over the whole picture is not "nothing" to the crop (the caller puts such a one down, REQ-0041-05, but the
   function never answers NOTHING for a float): one step, the size unchanged, Undo exact. */
static void
crop_float_whole (void)
{
  EaselImage *img = distinct_picture (12, 9), *old = copy_of (img);
  EaselHistory *h = easel_history_new ((size_t) 512 << 20);
  EaselFloat f = { 0 };

  CHECK (h != NULL);
  CHECK (easel_selection_lift (&f, img, 0, 0, 12, 9, NULL, true, YELLOW, SIZE_MAX) == EASEL_SELECTION_DONE);
  f.px[5] = BLACK;
  CHECK (easel_selection_crop (&f, img, h, 0, 0, 12, 9, WHITE, SIZE_MAX) == EASEL_SELECTION_DONE);
  CHECK (img->width == 12 && img->height == 9 && img->px[5] == BLACK);
  CHECK (easel_history_bytes (h) == easel_history_change_log_bytes (1));
  CHECK (easel_history_undo (h, img));
  check_same (img, old);
  easel_image_free (old);
  easel_float_free (&f);
  easel_history_free (h);
  easel_image_free (img);
}

/* Check G3 (0041): a Free-Form float moved a little over its own hole: the pixels of its box outside its shape but inside the hole show the hole's
   color (the background, or #00000000 in Transparent mode), the others the picture; the shape's pixels the float's: what the lay gives. */
static void
crop_float_over_own_hole (void)
{
  for (int k = 0; k < 2; k++)
    {
      uint32_t hole = k == 0 ? YELLOW : 0;
      EaselImage *img = distinct_picture (30, 20), *old = copy_of (img), *lay = copy_of (img);
      EaselHistory *h = easel_history_new ((size_t) 512 << 20);
      EaselFloat f = { 0 };
      uint8_t mask[10 * 10];

      CHECK (h != NULL);
      for (int j = 0; j < 10; j++)
        for (int i = 0; i < 10; i++)
          mask[j * 10 + i] = i <= j;                         /* the lower-left triangle */
      CHECK (easel_selection_lift (&f, img, 5, 5, 10, 10, mask, false, hole, SIZE_MAX) == EASEL_SELECTION_DONE);
      f.x = 8;
      f.y = 8;                                               /* box (8, 8) to (17, 17): over the hole (5, 5) to (14, 14) */
      easel_selection_lay (&f, true, old, 0, 0, 30, 20, lay->px, 30);
      CHECK (easel_selection_crop (&f, img, h, 8, 8, 10, 10, 0xFF123456u, SIZE_MAX) == EASEL_SELECTION_DONE);
      CHECK (img->width == 10 && img->height == 10);
      for (int j = 0; j < 10; j++)
        for (int i = 0; i < 10; i++)
          {
            uint32_t want = lay->px[(8 + j) * 30 + 8 + i];

            int hx = 8 + i - 5, hy = 8 + j - 5;
            bool in_hole = hx >= 0 && hx < 10 && hy >= 0 && hy < 10 && mask[hy * 10 + hx];     /* the hole is the shape's old place */

            CHECK (img->px[j * 10 + i] == want);
            if (!mask[j * 10 + i] && in_hole)
              CHECK (img->px[j * 10 + i] == hole);          /* outside the float's shape, in the hole: its color */
            if (!mask[j * 10 + i] && !in_hole)
              CHECK (img->px[j * 10 + i] == old->px[(8 + j) * 30 + 8 + i]);   /* outside both: the picture */
          }
      CHECK (easel_history_undo (h, img));
      check_same (img, old);
      easel_image_free (lay);
      easel_image_free (old);
      easel_float_free (&f);
      easel_history_free (h);
      easel_image_free (img);
    }
}

/* ---- 0042 T001: the shape of the inverse (REQ-0042-02, -03, -05) ---- */

/* Is the picture's pixel (x, y) selected: inside the box (sx, sy, sw, sh) and, with a mask, a pixel of it. */
static bool
oracle_selected (const uint8_t *mask, int sx, int sy, int sw, int sh, int x, int y)
{
  if (x < sx || y < sy || x - sx >= sw || y - sy >= sh)
    return false;
  return mask == NULL || mask[(size_t) (y - sy) * sw + (x - sx)] != 0;
}

/* The inverse of the selection over a W x H picture, from its pixels one by one: the result of easel_selection_invert must be it, the bounding
   rectangle of the pixels not selected and their mask, or NOTHING when there are none. Returns the number of pixels of the new shape. */
static size_t
check_invert (const uint8_t *mask, int sx, int sy, int sw, int sh, int W, int H, int *bx, int *by, int *bw, int *bh, uint8_t **keep)
{
  int x0 = W, y0 = H, x1 = -1, y1 = -1;
  size_t count = 0;
  uint8_t *out = NULL;
  int x, y, w, h;
  EaselSelectionResult r;

  for (int j = 0; j < H; j++)
    for (int i = 0; i < W; i++)
      if (!oracle_selected (mask, sx, sy, sw, sh, i, j))
        {
          x0 = i < x0 ? i : x0;
          y0 = j < y0 ? j : y0;
          x1 = i > x1 ? i : x1;
          y1 = j > y1 ? j : y1;
          count++;
        }
  r = easel_selection_invert (mask, sx, sy, sw, sh, W, H, &x, &y, &w, &h, &out, SIZE_MAX);
  if (count == 0)
    {
      CHECK (r == EASEL_SELECTION_NOTHING && out == NULL);
      return 0;
    }
  CHECK (r == EASEL_SELECTION_DONE && out != NULL);
  CHECK (x == x0 && y == y0 && w == x1 - x0 + 1 && h == y1 - y0 + 1);
  for (int j = 0; j < h; j++)
    for (int i = 0; i < w; i++)
      CHECK ((out[j * w + i] != 0) == !oracle_selected (mask, sx, sy, sw, sh, x + i, y + j));
  if (bx != NULL)
    {
      *bx = x;
      *by = y;
      *bw = w;
      *bh = h;
    }
  if (keep != NULL)
    *keep = out;
  else
    free (out);
  return count;
}

/* AS-2 (core part): a 300 x 200 picture less (100, 50) to (199, 149): 50 000 pixels, the bounding rectangle the picture. */
static void
invert_rect_hole (void)
{
  int x, y, w, h;

  CHECK (check_invert (NULL, 100, 50, 100, 100, 300, 200, &x, &y, &w, &h, NULL) == 50000);
  CHECK (x == 0 && y == 0 && w == 300 && h == 200);
}

/* A triangle's mask in its box at (5, 5) of a 40 x 30 picture: every pixel of the box outside the triangle, and every pixel outside the box. */
static void
invert_mask_triangle (void)
{
  uint8_t mask[10 * 10];

  for (int j = 0; j < 10; j++)
    for (int i = 0; i < 10; i++)
      mask[j * 10 + i] = i <= j;
  CHECK (check_invert (mask, 5, 5, 10, 10, 40, 30, NULL, NULL, NULL, NULL, NULL) == (size_t) (40 * 30 - 55));
}

/* The inverse of the inverse is the first shape: a rectangle comes back as the full mask of its box, a triangle as its own mask. */
static void
invert_twice (void)
{
  uint8_t tri[10 * 10], *once = NULL, *twice = NULL;
  int x, y, w, h, x2, y2, w2, h2;

  CHECK (check_invert (NULL, 12, 7, 9, 6, 40, 30, &x, &y, &w, &h, &once) == (size_t) (40 * 30 - 54));
  CHECK (easel_selection_invert (once, x, y, w, h, 40, 30, &x2, &y2, &w2, &h2, &twice, SIZE_MAX) == EASEL_SELECTION_DONE);
  CHECK (x2 == 12 && y2 == 7 && w2 == 9 && h2 == 6);
  for (int k = 0; k < 9 * 6; k++)
    CHECK (twice[k] == 1);
  free (once);
  free (twice);
  once = twice = NULL;
  for (int j = 0; j < 10; j++)
    for (int i = 0; i < 10; i++)
      tri[j * 10 + i] = i <= j;
  CHECK (check_invert (tri, 5, 5, 10, 10, 40, 30, &x, &y, &w, &h, &once) > 0);
  CHECK (easel_selection_invert (once, x, y, w, h, 40, 30, &x2, &y2, &w2, &h2, &twice, SIZE_MAX) == EASEL_SELECTION_DONE);
  CHECK (x2 == 5 && y2 == 5 && w2 == 10 && h2 == 10);
  CHECK (memcmp (twice, tri, sizeof tri) == 0);
  free (once);
  free (twice);
}

/* REQ-0042-05: every pixel selected: nothing is left, and no mask is made. */
static void
invert_whole_is_nothing (void)
{
  uint8_t full[12 * 9];

  memset (full, 1, sizeof full);
  CHECK (check_invert (NULL, 0, 0, 12, 9, 12, 9, NULL, NULL, NULL, NULL, NULL) == 0);       /* the rectangle of the whole picture */
  CHECK (check_invert (full, 0, 0, 12, 9, 12, 9, NULL, NULL, NULL, NULL, NULL) == 0);       /* a Free-Form shape of every pixel */
  CHECK (check_invert (NULL, -5, -5, 22, 19, 12, 9, NULL, NULL, NULL, NULL, NULL) == 0);    /* a box that holds the picture, a float over it */
}

static void
invert_one_pixel (void)
{
  int x, y, w, h;

  CHECK (check_invert (NULL, 2, 1, 1, 1, 5, 4, &x, &y, &w, &h, NULL) == 19);
  CHECK (x == 0 && y == 0 && w == 5 && h == 4);
  CHECK (check_invert (NULL, 0, 0, 1, 1, 1, 2, &x, &y, &w, &h, NULL) == 1);              /* a 1 x 2 picture less its first pixel */
  CHECK (x == 0 && y == 1 && w == 1 && h == 1);
}

/* A box past the edges (a floating selection may hang past them): only the part inside the picture counts. */
static void
invert_hangs_past_the_edge (void)
{
  uint8_t mask[10 * 10];

  CHECK (check_invert (NULL, 15, 5, 10, 10, 20, 10, NULL, NULL, NULL, NULL, NULL) == (size_t) (20 * 10 - 25));      /* right and bottom */
  CHECK (check_invert (NULL, -3, -3, 6, 6, 20, 10, NULL, NULL, NULL, NULL, NULL) == (size_t) (20 * 10 - 9));         /* left and top */
  CHECK (check_invert (NULL, 40, 30, 6, 6, 20, 10, NULL, NULL, NULL, NULL, NULL) == 200);                            /* wholly outside: all of it */
  for (int k = 0; k < 100; k++)
    mask[k] = (k % 3) == 0;
  CHECK (check_invert (mask, 15, 5, 10, 10, 20, 10, NULL, NULL, NULL, NULL, NULL) > 0);                               /* a shape, past the edge */
}

/* The bounding rectangle of the inverse is smaller than the picture when the selection fills whole rows or columns at its edges. */
static void
invert_bounding_is_smaller (void)
{
  const int W = 30, H = 20;
  uint8_t l_shape[30 * 20];
  int x, y, w, h;

  CHECK (check_invert (NULL, 0, 0, W, 3, W, H, &x, &y, &w, &h, NULL) == (size_t) (W * (H - 3)));         /* a band across the top */
  CHECK (x == 0 && y == 3 && w == W && h == H - 3);
  CHECK (check_invert (NULL, 0, 0, 4, H, W, H, &x, &y, &w, &h, NULL) == (size_t) ((W - 4) * H));         /* a strip down the left side */
  CHECK (x == 4 && y == 0 && w == W - 4 && h == H);
  CHECK (check_invert (NULL, 0, H - 2, W, 2, W, H, &x, &y, &w, &h, NULL) == (size_t) (W * (H - 2)));     /* a band across the bottom */
  CHECK (x == 0 && y == 0 && w == W && h == H - 2);
  CHECK (check_invert (NULL, W - 5, 0, 5, H, W, H, &x, &y, &w, &h, NULL) == (size_t) ((W - 5) * H));     /* a strip down the right side */
  CHECK (x == 0 && y == 0 && w == W - 5 && h == H);
  for (int j = 0; j < H; j++)
    for (int i = 0; i < W; i++)
      l_shape[j * W + i] = j < 3 || i < 4;                                                              /* both: an L */
  CHECK (check_invert (l_shape, 0, 0, W, H, W, H, &x, &y, &w, &h, NULL) == (size_t) ((W - 4) * (H - 3)));
  CHECK (x == 4 && y == 3 && w == W - 4 && h == H - 3);
}

/* ---- 0042 T002: the memory of the shape (REQ-0042-07) ---- */

/* An allocation above max_bytes, or one that cannot be had, makes nothing: NO_MEMORY and no mask. Each is counted by itself, the counts of the
   picture's rows and columns (4 bytes each) and the mask: a 30 x 20 picture has 200 bytes of counts, and the inverse of a small box a mask
   of 600, of a nearly full one a mask of 3. */
static void
invert_no_memory (void)
{
  uint8_t full[30 * 20], *out = (uint8_t *) 1;
  int x, y, w, h;

  /* the mask (600 bytes) above the counts (200): the limit one below each */
  CHECK (easel_selection_invert (NULL, 10, 5, 4, 4, 30, 20, &x, &y, &w, &h, &out, 199) == EASEL_SELECTION_NO_MEMORY && out == NULL);
  CHECK (easel_selection_invert (NULL, 10, 5, 4, 4, 30, 20, &x, &y, &w, &h, &out, 599) == EASEL_SELECTION_NO_MEMORY && out == NULL);
  CHECK (easel_selection_invert (NULL, 10, 5, 4, 4, 30, 20, &x, &y, &w, &h, &out, 600) == EASEL_SELECTION_DONE && out != NULL && w == 30 && h == 20);
  free (out);
  /* the mask (3 bytes) below the counts: the counts are what the limit meets, and with them in the mask needs 3 */
  memset (full, 1, sizeof full);
  full[5 * 30 + 5] = full[5 * 30 + 6] = full[5 * 30 + 7] = 0;
  CHECK (easel_selection_invert (full, 0, 0, 30, 20, 30, 20, &x, &y, &w, &h, &out, 199) == EASEL_SELECTION_NO_MEMORY && out == NULL);
  CHECK (easel_selection_invert (full, 0, 0, 30, 20, 30, 20, &x, &y, &w, &h, &out, 200) == EASEL_SELECTION_DONE && out != NULL);
  CHECK (x == 5 && y == 5 && w == 3 && h == 1 && out[0] == 1 && out[1] == 1 && out[2] == 1);
  free (out);
  /* a real shortage: the address space limited to a little above what is in use, a mask of 16 MB to make */
  {
    struct rlimit lim, was;
    int ok;

    CHECK (getrlimit (RLIMIT_AS, &was) == 0);
    lim = was;
    lim.rlim_cur = address_space () + 4 * 1024 * 1024;
    CHECK (setrlimit (RLIMIT_AS, &lim) == 0);
    ok = easel_selection_invert (NULL, 100, 100, 10, 10, 4000, 4000, &x, &y, &w, &h, &out, SIZE_MAX) == EASEL_SELECTION_NO_MEMORY && out == NULL;
    CHECK (setrlimit (RLIMIT_AS, &was) == 0);
    CHECK (ok);
  }
  /* a size outside 1 to EASEL_IMAGE_MAX_SIZE */
  CHECK (easel_selection_invert (NULL, 0, 0, 1, 1, 0, 5, &x, &y, &w, &h, &out, SIZE_MAX) == EASEL_SELECTION_NO_MEMORY && out == NULL);
  CHECK (easel_selection_invert (NULL, 0, 0, 1, 1, 5, 0, &x, &y, &w, &h, &out, SIZE_MAX) == EASEL_SELECTION_NO_MEMORY && out == NULL);
  CHECK (easel_selection_invert (NULL, 0, 0, 1, 1, EASEL_IMAGE_MAX_SIZE + 1, 5, &x, &y, &w, &h, &out, SIZE_MAX) == EASEL_SELECTION_NO_MEMORY && out == NULL);
  CHECK (easel_selection_invert (NULL, 0, 0, 1, 1, 5, EASEL_IMAGE_MAX_SIZE + 1, &x, &y, &w, &h, &out, SIZE_MAX) == EASEL_SELECTION_NO_MEMORY && out == NULL);
}

/* REQ-0002-16 in spirit, as 0041's: what a call holds when it returns is the mask and nothing spare: the counts (16 000 bytes for 2000 x 2000) are freed.
   The inverse of a 1000 x 1000 box in the middle of a 2000 x 2000 picture is a mask of 4 000 000 bytes. */
static void
invert_mask_held_as_counted (void)
{
  uint8_t *out;
  int x, y, w, h;
  size_t before = heap_in_use (), held;

  CHECK (easel_selection_invert (NULL, 500, 500, 1000, 1000, 2000, 2000, &x, &y, &w, &h, &out, SIZE_MAX) == EASEL_SELECTION_DONE);
  held = heap_in_use () - before;
  printf ("  held %zu bytes, the mask %zu\n", held, (size_t) w * h);
  CHECK (w == 2000 && h == 2000);
  CHECK (held >= (size_t) w * h && held <= (size_t) w * h + 4096);
  free (out);
  /* NOTHING and NO_MEMORY hold nothing either */
  before = heap_in_use ();
  CHECK (easel_selection_invert (NULL, 0, 0, 2000, 2000, 2000, 2000, &x, &y, &w, &h, &out, SIZE_MAX) == EASEL_SELECTION_NOTHING);
  CHECK (easel_selection_invert (NULL, 500, 500, 1000, 1000, 2000, 2000, &x, &y, &w, &h, &out, 16000) == EASEL_SELECTION_NO_MEMORY);
  CHECK (heap_in_use () <= before + 4096);
}

int
main (void)
{
  RUN (as3_move_put_down);
  RUN (lay_clips);
  RUN (lay_matches_put_down);
  RUN (as8_hole_and_pixels);
  RUN (as4_stamps);
  RUN (as9_clear);
  RUN (as13_selection_memory);
  RUN (as13_step_holds_what_it_counts);
  RUN (as13_clear_step_holds_what_it_counts);
  RUN (as4_masked_move_put_down);
  RUN (masked_lift_counts_the_mask);
  RUN (as6_masked_clear);
  RUN (as13_masked_step_holds_what_it_counts);
  RUN (as8_smear_trail);
  RUN (as10_smear_memory);
  RUN (lay_matches_the_oracle);
  RUN (bounds_cover_the_changes);
  RUN (as2_scale_pixels);
  RUN (as4_scale_back_exactly);
  RUN (as7_scale_shape);
  RUN (as10_scale_memory);
  RUN (as5_stretch_rect_ratio);
  RUN (as11_copies_keep_their_image);
  RUN (stretch_keeps_the_original_and_the_hole);
  RUN (as2_lazy_stretch_matches_full);
  RUN (as7_lazy_shape_keeps_one_pixel);
  RUN (as7_transform_stretched_float);
  RUN (shape_oriented_with_pixels);
  RUN (oriented_lazy_equals_whole);
  RUN (oriented_lazy_shape_small);
  RUN (selection_invert);
  RUN (as13_stretch_rect_mirror);
  RUN (stretch_with_orientation);
  RUN (as10_lazy_stretch_memory);
  RUN (as10_stretch_memory);
  RUN (as10_copies_list_counted_at_stretch);
  RUN (lay_range_adds_up);
  RUN (as7_fit_size);
  RUN (as4_paste_float_skips_clear);
  RUN (as2_copy_image);
  RUN (copy_by_rows_matches_oracle);
  RUN (as7_put_down_grown);
  RUN (as7_grown_step_holds_what_it_counts);
  RUN (crop_marked_offset);
  RUN (crop_free_form_box);
  RUN (crop_whole_is_nothing);
  RUN (crop_no_memory);
  RUN (crop_hangs_fill);
  RUN (crop_float_hanging);
  RUN (crop_float_shape_fill);
  RUN (crop_float_equals_lay);
  RUN (crop_redo_keeps_fill);
  RUN (crop_float_step_holds_what_it_counts);
  RUN (crop_float_no_memory);
  RUN (crop_float_paste_skips_clear);
  RUN (crop_float_whole);
  RUN (crop_float_over_own_hole);
  RUN (invert_rect_hole);
  RUN (invert_mask_triangle);
  RUN (invert_twice);
  RUN (invert_whole_is_nothing);
  RUN (invert_one_pixel);
  RUN (invert_hangs_past_the_edge);
  RUN (invert_bounding_is_smaller);
  RUN (invert_no_memory);
  RUN (invert_mask_held_as_counted);
  return 0;
}
