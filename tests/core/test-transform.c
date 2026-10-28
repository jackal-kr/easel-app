/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Image Transforms, scenarios of feature 0018: the eight orientations, Invert Colors, Clear Image, and their undo steps. */

#include <stdlib.h>
#include <string.h>

#include "check.h"
#include "history.h"
#include "transform.h"

#define BIG ((size_t) 1 << 40)

/* A w x h picture whose pixel i is 0xFF000000 + i + 1: every pixel different, so a wrong move shows. */
static EaselImage *
numbered (int w, int h)
{
  EaselImage *img = easel_image_new (w, h, 0);

  CHECK (img != NULL);
  for (int i = 0; i < w * h; i++)
    img->px[i] = 0xFF000000u + (uint32_t) i + 1;
  return img;
}

static bool
same_pixels (const EaselImage *a, const EaselImage *b)
{
  return a->width == b->width && a->height == b->height
         && memcmp (a->px, b->px, (size_t) a->width * (size_t) a->height * sizeof (uint32_t)) == 0;
}

static EaselImage *
copy_of (const EaselImage *src)
{
  EaselImage *c = easel_image_new (src->width, src->height, 0);

  CHECK (c != NULL);
  memcpy (c->px, src->px, (size_t) src->width * (size_t) src->height * sizeof (uint32_t));
  return c;
}

/* Each op moves each pixel of a w x h picture to where the spec says (the spec's formulas written out here, not the orientation's
   own), in all four channels. */
static void
check_orient_size (int w, int h)
{
  EaselImage *src = numbered (w, h);

  for (int op = EASEL_OP_FLIP_H; op <= EASEL_OP_ROTATE_270; op++)
    {
      EaselOrient o = easel_op_orient ((EaselOp) op);
      int ow = easel_orient_swaps (o) ? h : w, oh = easel_orient_swaps (o) ? w : h;
      uint32_t *dst = malloc ((size_t) ow * (size_t) oh * sizeof *dst);

      CHECK (dst != NULL);
      easel_orient_pixels (src->px, w, h, o, dst);
      for (int y = 0; y < h; y++)
        for (int x = 0; x < w; x++)
          {
            int nx = x, ny = y;

            switch (op)
              {
              case EASEL_OP_FLIP_H: nx = w - 1 - x; break;
              case EASEL_OP_FLIP_V: ny = h - 1 - y; break;
              case EASEL_OP_ROTATE_90: nx = h - 1 - y; ny = x; break;
              case EASEL_OP_ROTATE_180: nx = w - 1 - x; ny = h - 1 - y; break;
              case EASEL_OP_ROTATE_270: nx = y; ny = w - 1 - x; break;
              }
            CHECK (dst[(size_t) ny * (size_t) ow + (size_t) nx] == src->px[(size_t) y * (size_t) w + (size_t) x]);
          }
      free (dst);
    }
  easel_image_free (src);
}

/* AS-2 (REQ-0018-04), the pixel rules for every size up to 5 x 4, and for sizes that cross the 64-pixel blocks a transposing copy
   works in (a partial block on each side). */
static void
as2_orient_pixels (void)
{
  for (int w = 1; w <= 5; w++)
    for (int h = 1; h <= 4; h++)
      check_orient_size (w, h);
  check_orient_size (130, 70);
  check_orient_size (67, 129);
  check_orient_size (64, 64);
  check_orient_size (65, 1);
}

/* The composition of orientations: applying a then b is applying compose (a, b), for all 64 pairs; an orientation then its inverse
   is the identity; a quarter turn is 90 and 270 (REQ-0018-04). */
static void
orient_composition (void)
{
  EaselImage *src = numbered (3, 2);

  for (EaselOrient a = 0; a < 8; a++)
    for (EaselOrient b = 0; b < 8; b++)
      {
        uint32_t t1[6], t2[6], t3[6];
        int w1 = easel_orient_swaps (a) ? 2 : 3, h1 = easel_orient_swaps (a) ? 3 : 2;

        easel_orient_pixels (src->px, 3, 2, a, t1);
        easel_orient_pixels (t1, w1, h1, b, t2);
        easel_orient_pixels (src->px, 3, 2, easel_orient_compose (a, b), t3);
        CHECK (memcmp (t2, t3, sizeof t2) == 0);
        CHECK (easel_orient_swaps (easel_orient_compose (a, b)) == (easel_orient_swaps (a) != easel_orient_swaps (b)));
      }
  for (EaselOrient a = 0; a < 8; a++)
    CHECK (easel_orient_compose (a, easel_orient_inverse (a)) == EASEL_ORIENT_IDENTITY);
  CHECK (easel_orient_swaps (easel_op_orient (EASEL_OP_ROTATE_90)) && easel_orient_swaps (easel_op_orient (EASEL_OP_ROTATE_270)));
  CHECK (!easel_orient_swaps (easel_op_orient (EASEL_OP_ROTATE_180)));
  CHECK (easel_orient_compose (easel_op_orient (EASEL_OP_ROTATE_90), easel_op_orient (EASEL_OP_ROTATE_90))
         == easel_op_orient (EASEL_OP_ROTATE_180));
  easel_image_free (src);
}

/* AS-2 (REQ-0018-04, REQ-0018-09): the 3 x 2 picture, white but red at (0, 0) and blue at (2, 1): each command puts them where the
   spec says, with the size, as one step; Undo gives the 3 x 2 picture back exactly, Redo the result again; four turns of 90 are the
   picture, four steps. */
static void
as2_picture_commands (void)
{
  static const struct { EaselOp op; int w, h, rx, ry, bx, by; } want[] = {
    { EASEL_OP_ROTATE_90, 2, 3, 1, 0, 0, 2 },
    { EASEL_OP_ROTATE_270, 2, 3, 0, 2, 1, 0 },
    { EASEL_OP_ROTATE_180, 3, 2, 2, 1, 0, 0 },
    { EASEL_OP_FLIP_H, 3, 2, 2, 0, 0, 1 },
    { EASEL_OP_FLIP_V, 3, 2, 0, 1, 2, 0 },
  };

  for (size_t k = 0; k < sizeof want / sizeof *want; k++)
    {
      EaselImage *img = easel_image_new (3, 2, 0xFFFFFFFFu), *start;
      EaselHistory *h = easel_history_new (512u << 20);
      EaselImage *after;

      CHECK (img != NULL && h != NULL);
      easel_image_set (img, 0, 0, 0xFFFF0000u);
      easel_image_set (img, 2, 1, 0xFF0000FFu);
      start = copy_of (img);
      CHECK (easel_image_transform (img, h, want[k].op, 0, BIG) == EASEL_TRANSFORM_DONE);
      CHECK (img->width == want[k].w && img->height == want[k].h);
      CHECK (easel_image_get (img, want[k].rx, want[k].ry) == 0xFFFF0000u);
      CHECK (easel_image_get (img, want[k].bx, want[k].by) == 0xFF0000FFu);
      after = copy_of (img);
      CHECK (easel_history_can_undo (h) && easel_history_is_modified (h));
      CHECK (easel_history_undo (h, img));
      CHECK (same_pixels (img, start));
      CHECK (!easel_history_can_undo (h));
      CHECK (easel_history_redo (h, img));
      CHECK (same_pixels (img, after));
      easel_image_free (after);
      easel_image_free (start);
      easel_history_free (h);
      easel_image_free (img);
    }
  /* four turns of 90: the picture, and four steps */
  {
    EaselImage *img = numbered (3, 2), *start = numbered (3, 2);
    EaselHistory *h = easel_history_new (512u << 20);

    for (int i = 0; i < 4; i++)
      CHECK (easel_image_transform (img, h, EASEL_OP_ROTATE_90, 0, BIG) == EASEL_TRANSFORM_DONE);
    CHECK (same_pixels (img, start));
    for (int i = 0; i < 4; i++)
      CHECK (easel_history_undo (h, img));
    CHECK (!easel_history_can_undo (h));
    CHECK (img->width == 3 && img->height == 2 && same_pixels (img, start));
    easel_history_free (h);
    easel_image_free (start);
    easel_image_free (img);
  }
}

/* AS-4 (REQ-0018-05; the spec writes #RRGGBBAA, the pixels are 0xAARRGGBB): #12345678, #00000000 and #FFFFFFFF become #EDCBA978,
   #FFFFFF00 and #000000FF; again, as they were; an all transparent picture is inverted as a step all the same. */
static void
as4_invert (void)
{
  EaselImage *img = easel_image_new (3, 1, 0), *clear = easel_image_new (4, 4, 0);
  EaselHistory *h = easel_history_new (512u << 20);

  CHECK (img != NULL && clear != NULL && h != NULL);
  img->px[0] = 0x78123456u;
  img->px[1] = 0x00000000u;
  img->px[2] = 0xFFFFFFFFu;
  CHECK (easel_image_transform (img, h, EASEL_OP_INVERT, 0, BIG) == EASEL_TRANSFORM_DONE);
  CHECK (img->px[0] == 0x78EDCBA9u && img->px[1] == 0x00FFFFFFu && img->px[2] == 0xFF000000u);
  CHECK (easel_image_transform (img, h, EASEL_OP_INVERT, 0, BIG) == EASEL_TRANSFORM_DONE);
  CHECK (img->px[0] == 0x78123456u && img->px[1] == 0u && img->px[2] == 0xFFFFFFFFu);
  CHECK (easel_history_undo (h, img) && img->px[0] == 0x78EDCBA9u);
  CHECK (easel_image_transform (clear, h, EASEL_OP_INVERT, 0, BIG) == EASEL_TRANSFORM_DONE);
  CHECK (clear->px[0] == 0x00FFFFFFu && clear->px[15] == 0x00FFFFFFu);
  easel_history_free (h);
  easel_image_free (clear);
  easel_image_free (img);
}

/* AS-5 (REQ-0018-06): Clear Image gives every pixel the background, exactly, a transparent one #00000000; a picture already all in
   the background adds no step. */
static void
as5_clear (void)
{
  EaselImage *img = numbered (5, 4);
  EaselHistory *h = easel_history_new (512u << 20);

  CHECK (easel_image_transform (img, h, EASEL_OP_CLEAR, 0xFF00FF00u, BIG) == EASEL_TRANSFORM_DONE);
  for (int i = 0; i < 20; i++)
    CHECK (img->px[i] == 0xFF00FF00u);
  CHECK (easel_image_transform (img, h, EASEL_OP_CLEAR, 0xFF00FF00u, BIG) == EASEL_TRANSFORM_NOTHING);
  CHECK (easel_history_undo (h, img) && img->px[0] == 0xFF000001u && img->px[19] == 0xFF000014u);
  CHECK (!easel_history_can_undo (h));
  CHECK (easel_image_transform (img, h, EASEL_OP_CLEAR, 0u, BIG) == EASEL_TRANSFORM_DONE);
  CHECK (img->px[7] == 0u);
  easel_history_free (h);
  easel_image_free (img);
  /* a picture nearly all in the background: the clear changes 2 pixels, a log of 2 (12 bytes each plus 64) */
  img = easel_image_new (10, 10, 0xFF00FF01u);
  h = easel_history_new (512u << 20);
  easel_image_set (img, 1, 1, 0xFF123456u);
  easel_image_set (img, 8, 9, 0xFF654321u);
  CHECK (easel_image_transform (img, h, EASEL_OP_CLEAR, 0xFF00FF01u, BIG) == EASEL_TRANSFORM_DONE);
  CHECK (easel_history_bytes (h) == 2u * 12u + 64u);
  CHECK (easel_image_get (img, 1, 1) == 0xFF00FF01u && easel_image_get (img, 8, 9) == 0xFF00FF01u);
  CHECK (easel_history_undo (h, img) && easel_image_get (img, 1, 1) == 0xFF123456u && easel_image_get (img, 8, 9) == 0xFF654321u);
  easel_history_free (h);
  easel_image_free (img);
}

/* REQ-0018-09: a command that changes no pixel and no size adds no step and no modified mark: a flip of a picture that is its own
   mirror, Rotate 180 of one that is its own turn, a quarter turn of a square one-color picture. A quarter turn of a non-square
   one-color picture changes the size, so it is a step. */
static void
nothing_changed_no_step (void)
{
  EaselImage *img = easel_image_new (4, 3, 0xFF112233u);
  EaselImage *sq = easel_image_new (3, 3, 0xFF112233u);
  EaselHistory *h = easel_history_new (512u << 20);

  easel_history_mark_saved (h);
  CHECK (easel_image_transform (img, h, EASEL_OP_FLIP_H, 0, BIG) == EASEL_TRANSFORM_NOTHING);
  CHECK (easel_image_transform (img, h, EASEL_OP_FLIP_V, 0, BIG) == EASEL_TRANSFORM_NOTHING);
  CHECK (easel_image_transform (img, h, EASEL_OP_ROTATE_180, 0, BIG) == EASEL_TRANSFORM_NOTHING);
  CHECK (easel_image_transform (sq, h, EASEL_OP_ROTATE_90, 0, BIG) == EASEL_TRANSFORM_NOTHING);
  CHECK (easel_image_transform (sq, h, EASEL_OP_ROTATE_270, 0, BIG) == EASEL_TRANSFORM_NOTHING);
  CHECK (!easel_history_can_undo (h) && !easel_history_is_modified (h));
  CHECK (easel_image_transform (img, h, EASEL_OP_ROTATE_90, 0, BIG) == EASEL_TRANSFORM_DONE);
  CHECK (img->width == 3 && img->height == 4 && easel_history_is_modified (h));
  easel_history_free (h);
  easel_image_free (sq);
  easel_image_free (img);
}

/* R5, the buffer step (REQ-0018-09, REQ-0002-12 to -14): a quarter turn is a step that keeps a whole buffer and counts 4 bytes a pixel
   plus 64 (square or not); Undo and Redo swap it with the picture's: the size, every pixel, the saved state and the modified mark
   follow; a step is dropped whole when the budget needs it. */
static void
buffer_step_swap (void)
{
  EaselImage *img = numbered (6, 4), *start = numbered (6, 4);
  EaselImage *sq = numbered (5, 5);
  EaselHistory *h = easel_history_new (512u << 20), *h2 = easel_history_new (512u << 20);
  EaselRect r;

  easel_history_mark_saved (h);
  CHECK (easel_image_transform (img, h, EASEL_OP_ROTATE_90, 0, BIG) == EASEL_TRANSFORM_DONE);
  CHECK (easel_history_bytes (h) == 4u * 6u * 4u + 64u);
  CHECK (easel_history_is_modified (h));
  CHECK (easel_image_take_dirty (img, &r) && r.x0 == 0 && r.y0 == 0 && r.x1 == 4 && r.y1 == 6);   /* the command marks the turned picture */
  CHECK (easel_history_undo (h, img));
  CHECK (same_pixels (img, start) && !easel_history_is_modified (h));
  CHECK (easel_image_take_dirty (img, &r) && r.x0 == 0 && r.y0 == 0 && r.x1 == 6 && r.y1 == 4);   /* the whole picture shows again */
  CHECK (easel_history_redo (h, img));
  CHECK (img->width == 4 && img->height == 6 && easel_history_is_modified (h));
  CHECK (easel_image_take_dirty (img, &r) && r.x0 == 0 && r.y0 == 0 && r.x1 == 4 && r.y1 == 6);
  CHECK (easel_history_bytes (h) == 4u * 6u * 4u + 64u);
  CHECK (!easel_history_refused (h, &(int) { 0 }, &(int) { 0 }));
  /* a square picture: the same cost, 4 bytes a pixel plus 64 */
  CHECK (easel_image_transform (sq, h2, EASEL_OP_ROTATE_90, 0, BIG) == EASEL_TRANSFORM_DONE);
  CHECK (easel_history_bytes (h2) == 4u * 25u + 64u);
  easel_history_free (h2);
  /* a step larger than the whole budget is the only one kept (REQ-0002-17); the next step drops it */
  h2 = easel_history_new (100);
  CHECK (easel_image_transform (sq, h2, EASEL_OP_ROTATE_90, 0, BIG) == EASEL_TRANSFORM_DONE);
  CHECK (easel_history_can_undo (h2) && easel_history_bytes (h2) > 100);
  CHECK (easel_image_transform (sq, h2, EASEL_OP_ROTATE_90, 0, BIG) == EASEL_TRANSFORM_DONE);
  CHECK (easel_history_undo (h2, sq) && !easel_history_can_undo (h2));
  easel_history_free (h2);
  easel_history_free (h);
  easel_image_free (sq);
  easel_image_free (start);
  easel_image_free (img);
}

/* REQ-0018-09: the cheaper step is kept: a flip that changes few pixels is a change log (12 bytes a changed pixel plus 64, within 16),
   one that changes most of the picture is a buffer step (4 bytes a pixel plus 64, within 16 a changed pixel). */
static void
cheaper_step_chosen (void)
{
  EaselImage *few = easel_image_new (100, 100, 0xFFFFFFFFu), *most = numbered (100, 100);
  EaselHistory *h = easel_history_new (512u << 20), *h2 = easel_history_new (512u << 20);
  size_t n_few = 0;

  /* a picture white but for two pixels in row 0: Flip Vertical moves them to the last row, 4 pixels change */
  easel_image_set (few, 3, 0, 0xFF000000u);
  easel_image_set (few, 90, 0, 0xFF000000u);
  CHECK (easel_image_transform (few, h, EASEL_OP_FLIP_V, 0, BIG) == EASEL_TRANSFORM_DONE);
  n_few = 4;
  CHECK (easel_history_bytes (h) == n_few * 12 + 64);
  CHECK (easel_image_get (few, 3, 99) == 0xFF000000u && easel_image_get (few, 3, 0) == 0xFFFFFFFFu);
  CHECK (easel_history_undo (h, few) && easel_image_get (few, 3, 0) == 0xFF000000u && easel_image_get (few, 3, 99) == 0xFFFFFFFFu);
  CHECK (easel_history_redo (h, few) && easel_image_get (few, 90, 99) == 0xFF000000u);
  /* every pixel different: Flip Horizontal changes all 10000 (the 100 columns are even in number: none stays) */
  CHECK (easel_image_transform (most, h2, EASEL_OP_FLIP_H, 0, BIG) == EASEL_TRANSFORM_DONE);
  CHECK (easel_history_bytes (h2) == 4u * 10000u + 64u);
  CHECK (easel_history_bytes (h2) <= 16u * 10000u + 64u);
  CHECK (easel_history_undo (h2, most) && most->px[0] == 0xFF000001u && most->px[99] == 0xFF000064u);
  CHECK (easel_history_redo (h2, most) && most->px[0] == 0xFF000064u);
  easel_history_free (h2);
  easel_history_free (h);
  easel_image_free (most);
  easel_image_free (few);
  /* a quarter turn is a buffer step whatever it changes: a square white picture with one black pixel changes 2 pixels, and counts
     4 bytes a pixel plus 64 (REQ-0018-09) */
  few = easel_image_new (100, 100, 0xFFFFFFFFu);
  h = easel_history_new (512u << 20);
  easel_image_set (few, 3, 0, 0xFF000000u);
  CHECK (easel_image_transform (few, h, EASEL_OP_ROTATE_90, 0, BIG) == EASEL_TRANSFORM_DONE);
  CHECK (easel_history_bytes (h) == 4u * 10000u + 64u);
  CHECK (easel_image_get (few, 99, 3) == 0xFF000000u && easel_image_get (few, 3, 0) == 0xFFFFFFFFu);
  easel_history_free (h);
  easel_image_free (few);
}

/* The count stops once the answer is decided (more than a third of the pixels changed: a buffer step), but not before it is exact for a
   log: a 6 x 6 picture with a different pixel at the start of each row changes 12 pixels, exactly a third, under Flip Horizontal: a log
   of all 12, and Undo gives the picture back. One pixel more changed (a 13th) is a buffer step. */
static void
log_at_the_limit (void)
{
  EaselImage *img = easel_image_new (6, 6, 0xFFFFFFFFu), *start;
  EaselHistory *h = easel_history_new (512u << 20);

  for (int y = 0; y < 6; y++)
    easel_image_set (img, 0, y, 0xFF000000u + (uint32_t) y);
  start = copy_of (img);
  CHECK (easel_image_transform (img, h, EASEL_OP_FLIP_H, 0, BIG) == EASEL_TRANSFORM_DONE);
  for (int y = 0; y < 6; y++)
    CHECK (easel_image_get (img, 5, y) == 0xFF000000u + (uint32_t) y && easel_image_get (img, 0, y) == 0xFFFFFFFFu);
  CHECK (easel_history_bytes (h) == 12u * 12u + 64u);
  CHECK (easel_history_undo (h, img) && same_pixels (img, start));
  easel_history_free (h);
  easel_image_free (start);
  easel_image_free (img);
}

/* AS-12 (REQ-0018-10), the core part: a command whose new pixels or step cannot be had (an allocation above max_bytes) is NO_MEMORY and
   changes nothing, neither the picture nor the history, for every op. */
static void
as12_transform_no_memory (void)
{
  for (int op = EASEL_OP_FLIP_H; op <= EASEL_OP_CLEAR; op++)
    {
      EaselImage *img = numbered (20, 10), *start = numbered (20, 10);
      EaselHistory *h = easel_history_new (512u << 20);

      CHECK (easel_image_transform (img, h, (EaselOp) op, 0xFF00FF00u, 3) == EASEL_TRANSFORM_NO_MEMORY);
      CHECK (same_pixels (img, start));
      CHECK (!easel_history_can_undo (h) && easel_history_bytes (h) == 0 && !easel_history_is_modified (h));
      CHECK (easel_image_transform (img, h, (EaselOp) op, 0xFF00FF00u, BIG) == EASEL_TRANSFORM_DONE);
      CHECK (easel_history_can_undo (h));
      easel_history_free (h);
      easel_image_free (start);
      easel_image_free (img);
    }
}

/* AS-12: the change log is an allocation too: a flip whose log (4 pixels, 48 bytes) is above the limit is NO_MEMORY, nothing changed. */
static void
as12_log_above_limit (void)
{
  EaselImage *img = easel_image_new (100, 100, 0xFFFFFFFFu), *start;
  EaselHistory *h = easel_history_new (512u << 20);

  easel_image_set (img, 3, 0, 0xFF000000u);
  easel_image_set (img, 90, 0, 0xFF000000u);
  start = copy_of (img);
  CHECK (easel_image_transform (img, h, EASEL_OP_FLIP_V, 0, 47) == EASEL_TRANSFORM_NO_MEMORY);
  CHECK (same_pixels (img, start) && !easel_history_can_undo (h) && easel_history_bytes (h) == 0);
  CHECK (easel_image_transform (img, h, EASEL_OP_FLIP_V, 0, 48) == EASEL_TRANSFORM_DONE);
  easel_history_free (h);
  easel_image_free (start);
  easel_image_free (img);
}

int
main (void)
{
  RUN (as2_orient_pixels);
  RUN (orient_composition);
  RUN (as2_picture_commands);
  RUN (as4_invert);
  RUN (as5_clear);
  RUN (nothing_changed_no_step);
  RUN (buffer_step_swap);
  RUN (cheaper_step_chosen);
  RUN (log_at_the_limit);
  RUN (as12_transform_no_memory);
  RUN (as12_log_above_limit);
  return 0;
}
