/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Putting a text box down, scenarios of feature 0017 (REQ-0017-16). */

#include <malloc.h>
#include <stdint.h>
#include <stdlib.h>

#include "check.h"
#include "text-put.h"

#define WHITE 0xFFFFFFFFu
#define BLACK 0xFF000000u
#define RED 0xFFFF0000u
#define YELLOW 0xFFFFFF00u
#define HALF_RED 0x80FF0000u

/* AS-16 (REQ-0017-16), the core part, on a 10 x 6 white picture with a box (2, 1, 5, 3):
   - Transparent: the ink's masked pixels are written, alpha included (a half-transparent red stays half-transparent), the others keep
     their color; one step, and Undo gives the white picture back;
   - Opaque, yellow background: the whole box yellow first, the ink's masked pixels over it;
   - ink reaching past the picture's bottom-right: only the pixels inside change;
   - an ink with no masked pixel in Transparent mode, or one that changes no pixel: no step. */
static void
as16_put_down_ink (void)
{
  EaselImage *img = easel_image_new (10, 6, WHITE);
  EaselHistory *h = easel_history_new (1 << 20);
  /* a 3 x 2 ink at (3, 2): masked are (3, 2) black, (4, 2) half red, (5, 3) red; (4, 3) has a color but no mask */
  uint32_t color[6] = { BLACK, HALF_RED, BLACK, BLACK, RED, RED };
  uint8_t mask[6] = { 1, 1, 0, 0, 0, 1 };
  EaselTextInk ink = { 3, 2, 3, 2, color, mask };
  uint8_t none[6] = { 0 };
  EaselTextInk blank = { 3, 2, 3, 2, color, none };

  CHECK (img != NULL && h != NULL);
  CHECK (easel_text_put_down (img, h, 2, 1, 5, 3, &ink, false, YELLOW, SIZE_MAX) == EASEL_SELECTION_DONE);
  CHECK (easel_image_get (img, 3, 2) == BLACK && easel_image_get (img, 4, 2) == HALF_RED && easel_image_get (img, 5, 3) == RED);
  CHECK (easel_image_get (img, 5, 2) == WHITE && easel_image_get (img, 3, 3) == WHITE && easel_image_get (img, 4, 3) == WHITE);
  CHECK (easel_image_get (img, 2, 1) == WHITE);                                 /* Transparent: no fill */
  CHECK (easel_history_can_undo (h));
  CHECK (easel_history_undo (h, img));
  for (int y = 0; y < 6; y++)
    for (int x = 0; x < 10; x++)
      CHECK (easel_image_get (img, x, y) == WHITE);
  /* Opaque: the box (2..6, 1..3) yellow, the masked pixels over it, nothing outside the box but the ink */
  CHECK (easel_text_put_down (img, h, 2, 1, 5, 3, &ink, true, YELLOW, SIZE_MAX) == EASEL_SELECTION_DONE);
  CHECK (easel_image_get (img, 2, 1) == YELLOW && easel_image_get (img, 6, 3) == YELLOW && easel_image_get (img, 4, 3) == YELLOW);
  CHECK (easel_image_get (img, 3, 2) == BLACK && easel_image_get (img, 5, 3) == RED);
  CHECK (easel_image_get (img, 1, 1) == WHITE && easel_image_get (img, 7, 1) == WHITE && easel_image_get (img, 2, 4) == WHITE);
  CHECK (easel_history_undo (h, img));
  /* ink past the picture's edges: only inside changes, no crash */
  {
    EaselTextInk edge = { 8, 5, 3, 2, color, mask };

    CHECK (easel_text_put_down (img, h, 8, 5, 3, 2, &edge, false, YELLOW, SIZE_MAX) == EASEL_SELECTION_DONE);
    CHECK (easel_image_get (img, 8, 5) == BLACK && easel_image_get (img, 9, 5) == HALF_RED);
    CHECK (easel_history_undo (h, img));
  }
  /* Opaque with ink reaching past the box's right edge (an italic overhang): the fill stays in the box; the unmasked ink pixel at
     (4, 2), right of the box (2, 1, 2, 2), keeps its own color */
  CHECK (easel_text_put_down (img, h, 2, 1, 2, 2, &ink, true, YELLOW, SIZE_MAX) == EASEL_SELECTION_DONE);
  CHECK (easel_image_get (img, 3, 1) == YELLOW && easel_image_get (img, 4, 1) == WHITE && easel_image_get (img, 5, 2) == WHITE);
  CHECK (easel_image_get (img, 4, 2) == HALF_RED);                               /* masked: written, outside the box too */
  CHECK (easel_history_undo (h, img));
  /* no masked pixel, Transparent: nothing, no step */
  CHECK (easel_text_put_down (img, h, 2, 1, 5, 3, &blank, false, YELLOW, SIZE_MAX) == EASEL_SELECTION_NOTHING);
  CHECK (easel_history_can_redo (h));                                            /* no step was added: the redo step is there */
  /* Opaque with a background equal to the picture and no ink: no pixel differs, no step */
  CHECK (easel_text_put_down (img, h, 2, 1, 5, 3, &blank, true, WHITE, SIZE_MAX) == EASEL_SELECTION_NOTHING);
  CHECK (easel_history_can_redo (h));
  /* a step added discards the redo steps (REQ-0002-05) */
  CHECK (easel_text_put_down (img, h, 2, 1, 5, 3, &ink, false, YELLOW, SIZE_MAX) == EASEL_SELECTION_DONE);
  CHECK (!easel_history_can_redo (h));
  easel_history_free (h);
  easel_image_free (img);
}

/* REQ-0017-18 (the core part): without the step's memory, NO_MEMORY and every pixel as it was. */
static void
put_down_without_memory (void)
{
  EaselImage *img = easel_image_new (10, 6, WHITE);
  EaselHistory *h = easel_history_new (1 << 20);
  uint32_t color[6] = { BLACK, BLACK, BLACK, BLACK, BLACK, BLACK };
  uint8_t mask[6] = { 1, 1, 1, 1, 1, 1 };
  EaselTextInk ink = { 3, 2, 3, 2, color, mask };

  CHECK (img != NULL && h != NULL);
  CHECK (easel_text_put_down (img, h, 2, 1, 5, 3, &ink, true, YELLOW, 8) == EASEL_SELECTION_NO_MEMORY);
  for (int y = 0; y < 6; y++)
    for (int x = 0; x < 10; x++)
      CHECK (easel_image_get (img, x, y) == WHITE);
  /* a limit that lets some pixels change before it refuses: the log first takes room for 1024 changes (12288 bytes), so an Opaque box
     of 50 x 30 (1500 to change) is refused at its 1025th pixel; the 1024 already set are put back too */
  {
    EaselImage *big = easel_image_new (60, 40, WHITE);
    EaselTextInk none = { 0, 0, 0, 0, NULL, NULL };

    CHECK (big != NULL);
    CHECK (easel_text_put_down (big, h, 0, 0, 50, 30, &none, true, YELLOW, 12288) == EASEL_SELECTION_NO_MEMORY);
    for (int y = 0; y < 40; y++)
      for (int x = 0; x < 60; x++)
        CHECK (easel_image_get (big, x, y) == WHITE);
    easel_image_free (big);
  }
  CHECK (!easel_history_can_undo (h));
  easel_history_free (h);
  easel_image_free (img);
}

static size_t
heap_in_use (void)
{
  struct mallinfo2 mi = mallinfo2 ();

  return mi.uordblks + mi.hblkhd;
}

/* REQ-0002-16, REQ-0017-16: the step of a put-down holds the memory the budget counts. An Opaque box of 513 x 512 changes just more
   than a power of two of pixels, so the log's spare half must not stay. A step larger than the whole budget is kept as the only step
   (REQ-0002-17). */
static void
as16_step_holds_what_it_counts (void)
{
  EaselImage *img = easel_image_new (600, 600, WHITE);
  EaselHistory *h = easel_history_new ((size_t) 512 << 20), *small = easel_history_new (1024);
  EaselTextInk none = { 0, 0, 0, 0, NULL, NULL };
  size_t before, held, counted;

  CHECK (img != NULL && h != NULL && small != NULL);
  before = heap_in_use ();
  CHECK (easel_text_put_down (img, h, 0, 0, 513, 512, &none, true, YELLOW, SIZE_MAX) == EASEL_SELECTION_DONE);
  held = heap_in_use () - before;
  counted = easel_history_bytes (h);
  printf ("  held %zu bytes, counted %zu\n", held, counted);
  CHECK (held <= counted + counted / 100 + 4096);
  CHECK (easel_text_put_down (img, small, 0, 0, 100, 100, &none, true, RED, SIZE_MAX) == EASEL_SELECTION_DONE);
  CHECK (easel_history_can_undo (small));                                        /* larger than 1024 bytes: kept alone */
  easel_history_free (small);
  easel_history_free (h);
  easel_image_free (img);
}

/* AS-5 (REQ-0025-01, REQ-0017-16): a foreground of any alpha is written as it is, over white, in Transparent and in Opaque mode; never blended. */
static void
as5_text_writes_exact_colors (void)
{
  static const uint32_t colors[] = { 0x00000000u, HALF_RED, 0xFF336699u };

  for (size_t c = 0; c < sizeof colors / sizeof colors[0]; c++)
    for (int opaque = 0; opaque < 2; opaque++)
      {
        EaselImage *img = easel_image_new (10, 6, WHITE);
        uint32_t color[6] = { colors[c], colors[c], colors[c], colors[c], colors[c], colors[c] };
        uint8_t mask[6] = { 1, 1, 0, 0, 0, 1 };
        EaselTextInk ink = { 3, 2, 3, 2, color, mask };

        CHECK (img != NULL);
        CHECK (easel_text_put_down (img, NULL, 2, 1, 5, 3, &ink, opaque, YELLOW, SIZE_MAX) == EASEL_SELECTION_DONE);
        CHECK (easel_image_get (img, 3, 2) == colors[c]);
        CHECK (easel_image_get (img, 4, 2) == colors[c]);
        CHECK (easel_image_get (img, 5, 3) == colors[c]);
        /* What has no ink: the background in Opaque mode, inside the box; otherwise what was there. */
        CHECK (easel_image_get (img, 5, 2) == (opaque ? YELLOW : WHITE));
        CHECK (easel_image_get (img, 0, 0) == WHITE);
        easel_image_free (img);
      }
}

int
main (void)
{
  RUN (as16_put_down_ink);
  RUN (as5_text_writes_exact_colors);
  RUN (put_down_without_memory);
  RUN (as16_step_holds_what_it_counts);
  return 0;
}
