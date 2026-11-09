/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Remove background (feature 0035): the pixel rules around the model (REQ-0035-02, -03, -05, -07): the model's input, the early
   answer, the answer applied (alpha 0, color kept, hard edges, only pixels looked at), the whole picture's step and a float's image. */

#include <malloc.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "check.h"
#include "history.h"
#include "remove-bg.h"
#include "selection.h"

#define BIG ((size_t) 1 << 40)
#define RED 0xFFFF0000u
#define BLUE 0xFF0000FFu

static bool
near (float a, float b)
{
  return fabsf (a - b) < 1e-6f;
}

/* The input: planes of red, green and blue from 0 to 1; scaled with pixel centers aligned; a place not looked at, or fully
   transparent, is gray 0.5. */
static void
input (void)
{
  uint32_t px[2] = { RED, BLUE };
  uint8_t mask[2] = { 1, 0 };
  float out[3 * 4 * 4];

  easel_rbg_input (px, NULL, 2, 1, 2, out);
  CHECK (near (out[0], 1) && near (out[1], 0));                          /* red plane, row 0 */
  CHECK (near (out[2 * 4 + 0], 0) && near (out[2 * 4 + 1], 1));          /* blue plane (side 2: 4 values a plane), row 0 */
  easel_rbg_input (px, NULL, 2, 1, 4, out);
  CHECK (near (out[0], 1) && near (out[1], 0.75f) && near (out[2], 0.25f) && near (out[3], 0));
  CHECK (near (out[12], 1) && near (out[15], 0));                        /* every row the same: one row of source */
  easel_rbg_input (px, mask, 2, 1, 4, out);
  CHECK (near (out[0], 1) && near (out[1], 0.875f) && near (out[2], 0.625f) && near (out[3], 0.5f));
  px[1] = 0x000000FFu;                                                   /* fully transparent: gray too */
  easel_rbg_input (px, NULL, 2, 1, 4, out);
  CHECK (near (out[3], 0.5f) && near (out[2 * 16 + 3], 0.5f));
}

/* REQ-0035-05's early answer: one color, or under 16 pixels wide or high; only the pixels looked at count. */
static void
trivial (void)
{
  uint32_t *px = malloc (20 * 20 * sizeof *px);
  uint8_t mask[20 * 20];

  CHECK (px != NULL);
  for (int i = 0; i < 400; i++)
    px[i] = RED, mask[i] = i % 20 < 10;
  CHECK (easel_rbg_trivial (px, NULL, 20, 20));
  px[399] = BLUE;
  CHECK (!easel_rbg_trivial (px, NULL, 20, 20));
  CHECK (easel_rbg_trivial (px, mask, 20, 20));                          /* the blue pixel is not looked at */
  px[1] = BLUE;                                                          /* two colors in every size tried below */
  CHECK (easel_rbg_trivial (px, NULL, 15, 20) && easel_rbg_trivial (px, NULL, 20, 15));
  CHECK (!easel_rbg_trivial (px, NULL, 16, 16));
  px[0] = RED & 0x00FFFFFFu;                                             /* alpha counts as a channel */
  CHECK (!easel_rbg_trivial (px, mask, 20, 20));
  free (px);
}

/* The answer applied: stretched by its own range, scaled back, below one half removed (alpha 0, color kept); a pixel not looked at,
   or already transparent, stays. */
static void
apply (void)
{
  static const float found[4] = { 0, 10, 10, 0 };
  uint32_t px[4] = { RED, BLUE, 0xFF00FF00u, 0x00123456u };
  uint8_t mask[4] = { 1, 1, 1, 1 };
  uint32_t big[16];

  CHECK (easel_rbg_apply (px, NULL, 2, 2, found, 2) == 1);              /* (1, 1) was already transparent */
  CHECK (px[0] == (RED & 0x00FFFFFFu) && px[1] == BLUE && px[2] == 0xFF00FF00u && px[3] == 0x00123456u);
  px[0] = RED;
  mask[0] = 0;
  CHECK (easel_rbg_apply (px, mask, 2, 2, found, 2) == 0 && px[0] == RED);
  /* Scaled back: 2 x 2 to 4 x 4 with centers aligned; (1, 1) is 0.375, removed, (1, 2) 0.625, kept. */
  for (int i = 0; i < 16; i++)
    big[i] = BLUE;
  CHECK (easel_rbg_apply (big, NULL, 4, 4, found, 2) > 0);
  CHECK ((big[1 * 4 + 1] >> 24) == 0 && (big[2 * 4 + 1] >> 24) == 0xFF && (big[1 * 4 + 2] >> 24) == 0xFF);
  CHECK ((big[0] >> 24) == 0 && (big[3] >> 24) == 0xFF && (big[15] >> 24) == 0);
  /* An answer of one value tells nothing apart: everything is subject, nothing removed. */
  {
    static const float flat[4] = { 3, 3, 3, 3 };

    CHECK (easel_rbg_apply (big, NULL, 4, 4, flat, 2) == 0);
  }
}

static size_t
heap_in_use (void)
{
  struct mallinfo2 mi = mallinfo2 ();

  return mi.uordblks + mi.hblkhd;
}

/* The whole picture: one step and Undo; nothing removed is no step; the memory held is what the history counts; NO_MEMORY changes
   nothing. */
static void
picture_step (void)
{
  float *found = malloc (64 * 64 * sizeof *found);
  EaselImage *img = easel_image_new (300, 200, BLUE), *before = easel_image_new (300, 200, BLUE);
  EaselHistory *h = easel_history_new ((size_t) 512 << 20);
  size_t start, held, counted;

  CHECK (found != NULL && img != NULL && before != NULL && h != NULL);
  for (int i = 0; i < 64 * 64; i++)
    found[i] = (i % 64) < 32 ? 0.0f : 1.0f;                            /* the left half is background */
  CHECK (easel_image_remove_background (img, h, found, 64, 1000) == EASEL_TRANSFORM_NO_MEMORY);
  CHECK (memcmp (img->px, before->px, 300 * 200 * 4) == 0 && !easel_history_can_undo (h));
  start = heap_in_use ();
  CHECK (easel_image_remove_background (img, h, found, 64, BIG) == EASEL_TRANSFORM_DONE);
  held = heap_in_use () - start;
  counted = easel_history_bytes (h);
  CHECK (held <= counted + counted / 100 + 4096);
  CHECK ((img->px[100 * 300 + 10] >> 24) == 0 && (img->px[100 * 300 + 10] & 0x00FFFFFFu) == (BLUE & 0x00FFFFFFu));
  CHECK (img->px[100 * 300 + 290] == BLUE);
  CHECK (easel_image_remove_background (img, h, found, 64, BIG) == EASEL_TRANSFORM_NOTHING);   /* nothing more */
  CHECK (easel_history_undo (h, img));
  CHECK (memcmp (img->px, before->px, 300 * 200 * 4) == 0 && !easel_history_can_undo (h));
  easel_history_free (h);
  easel_image_free (before);
  easel_image_free (img);
  free (found);
}

/* A float: the answer is applied to what is shown (a 4 x 4 lifted, stretched to 8 x 8), which becomes its image; a shape keeps
   the pixels outside it; a pasted float's transparent pixels stay. */
static void
float_shown (void)
{
  EaselImage *img = easel_image_new (10, 10, BLUE);
  EaselFloat f = { 0 }, g = { 0 };
  float found[8 * 8];
  static const uint8_t L[9] = { 1, 0, 0, 1, 0, 0, 1, 1, 1 };
  float all_bg[9] = { 0, 0, 0, 0, 0, 0, 0, 0, 1 };

  CHECK (img != NULL);
  for (int i = 0; i < 64; i++)
    found[i] = (i % 8) < 4 ? 0.0f : 1.0f;
  CHECK (easel_selection_lift (&f, img, 1, 1, 4, 4, NULL, false, 0xFFFFFFFFu, SIZE_MAX) == EASEL_SELECTION_DONE);
  CHECK (easel_selection_stretch (&f, 1, 1, 8, 8, SIZE_MAX) == EASEL_SELECTION_DONE);
  CHECK (easel_selection_remove_background (&f, found, 8, SIZE_MAX) == EASEL_SELECTION_DONE);
  CHECK (f.w == 8 && f.h == 8 && f.orig_px == NULL && f.orient == EASEL_ORIENT_IDENTITY);
  for (int y = 0; y < 8; y++)
    for (int x = 0; x < 8; x++)
      CHECK ((f.px[y * 8 + x] >> 24) == (x < 4 ? 0 : 0xFF));
  CHECK (easel_selection_remove_background (&f, found, 8, SIZE_MAX) == EASEL_SELECTION_NOTHING);
  easel_float_free (&f);
  CHECK (easel_selection_lift (&g, img, 2, 2, 3, 3, L, false, 0xFFFFFFFFu, SIZE_MAX) == EASEL_SELECTION_DONE);
  CHECK (easel_selection_remove_background (&g, all_bg, 3, SIZE_MAX) == EASEL_SELECTION_DONE);
  for (int i = 0; i < 9; i++)
    if (L[i] && i != 8)
      CHECK ((g.px[i] >> 24) == 0);
  CHECK (g.mask != NULL && memcmp (g.mask, L, 9) == 0);
  easel_float_free (&g);
  easel_image_free (img);
}

int
main (void)
{
  RUN (input);
  RUN (trivial);
  RUN (apply);
  RUN (picture_step);
  RUN (float_shown);
  return 0;
}
