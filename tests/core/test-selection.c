/* SPDX-License-Identifier: GPL-3.0-or-later */
/* The floating selection of feature 0013: lifting it (REQ-0013-07, -08), laying it over the picture and putting it down as
   one undo step (REQ-0013-11). */

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
  RUN (lay_range_adds_up);
  return 0;
}
