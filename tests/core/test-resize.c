/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Resize scenarios of feature 0004 (REQ-0004-20, -22, -23, -24, -26). */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/resource.h>

#include "check.h"
#include "history.h"
#include "stroke.h"

/* An opaque, distinct color for each pixel position of a 4-wide pattern. */
static uint32_t
pattern (int x, int y)
{
  return 0xFF000000u | (uint32_t) (y << 8) | (uint32_t) x;
}

static EaselImage *
pattern_image (int w, int h)
{
  EaselImage *img = easel_image_new (w, h, 0xFFFFFFFFu);

  CHECK (img != NULL);
  for (int y = 0; y < h; y++)
    for (int x = 0; x < w; x++)
      img->px[(size_t) y * (size_t) w + (size_t) x] = pattern (x, y);
  return img;
}

typedef struct
{
  int w, h;
  uint32_t *px;
} Copy;

static Copy
copy_of (const EaselImage *img)
{
  Copy c = { img->width, img->height, malloc ((size_t) img->width * (size_t) img->height * 4) };

  CHECK (c.px != NULL);
  memcpy (c.px, img->px, (size_t) img->width * (size_t) img->height * 4);
  return c;
}

static int
same_as (const EaselImage *img, const Copy *c)
{
  return img->width == c->w && img->height == c->h
         && memcmp (img->px, c->px, (size_t) c->w * (size_t) c->h * 4) == 0;
}

/* REQ-0004-20: the pixels that stay are unchanged, new pixels take the fill, removed ones are gone. */
static void
as19_grow_and_shrink (void)
{
  EaselImage *img = pattern_image (4, 4);
  int fills = 0;

  CHECK (easel_image_resize (img, 6, 5, 0xFFAABBCCu) == EASEL_RESIZE_OK);
  CHECK (img->width == 6 && img->height == 5);
  for (int y = 0; y < 5; y++)
    for (int x = 0; x < 6; x++)
      {
        uint32_t v = easel_image_get (img, x, y);

        if (x < 4 && y < 4)
          CHECK (v == pattern (x, y));
        else
          {
            CHECK (v == 0xFFAABBCCu);
            fills++;
          }
      }
  CHECK (fills == 14);

  /* A transparent fill gives transparent pixels. */
  CHECK (easel_image_resize (img, 7, 5, 0x00000000u) == EASEL_RESIZE_OK);
  for (int y = 0; y < 5; y++)
    CHECK (easel_image_get (img, 6, y) == 0x00000000u);

  /* Shrinking keeps the top-left part. */
  CHECK (easel_image_resize (img, 2, 2, 0xFF000000u) == EASEL_RESIZE_OK);
  CHECK (img->width == 2 && img->height == 2);
  for (int y = 0; y < 2; y++)
    for (int x = 0; x < 2; x++)
      CHECK (easel_image_get (img, x, y) == pattern (x, y));

  /* Wider and shorter at once. */
  easel_image_free (img);
  img = pattern_image (4, 4);
  CHECK (easel_image_resize (img, 6, 2, 0xFF123456u) == EASEL_RESIZE_OK);
  CHECK (img->width == 6 && img->height == 2);
  CHECK (easel_image_get (img, 3, 1) == pattern (3, 1) && easel_image_get (img, 4, 0) == 0xFF123456u
         && easel_image_get (img, 5, 1) == 0xFF123456u);

  /* Sizes that are not allowed leave the picture alone. */
  {
    Copy before = copy_of (img);

    CHECK (easel_image_resize (img, 0, 5, 0) == EASEL_RESIZE_BAD_SIZE);
    CHECK (easel_image_resize (img, 5, 0, 0) == EASEL_RESIZE_BAD_SIZE);
    CHECK (easel_image_resize (img, -1, 3, 0) == EASEL_RESIZE_BAD_SIZE);
    CHECK (easel_image_resize (img, 10001, 5, 0) == EASEL_RESIZE_BAD_SIZE);
    CHECK (easel_image_resize (img, 5, 10001, 0) == EASEL_RESIZE_BAD_SIZE);
    CHECK (same_as (img, &before));
    free (before.px);
  }
  /* The limits themselves are allowed. */
  easel_image_free (img);
  img = easel_image_new (1, 1, 0xFF000000u);
  CHECK (easel_image_resize (img, 10000, 1, 0xFF010203u) == EASEL_RESIZE_OK && img->width == 10000);
  CHECK (easel_image_resize (img, 1, 1, 0) == EASEL_RESIZE_OK && img->width == 1 && easel_image_get (img, 0, 0) == 0xFF000000u);
  easel_image_free (img);
}

/* REQ-0004-22: undo restores size and every pixel exactly, redo gives the same result, including the fill. */
static void
as21_undo_redo_resize_exact (void)
{
  EaselImage *img = pattern_image (4, 4);
  EaselHistory *h = easel_history_new ((size_t) 512 << 20);
  Copy start, shrunk, grown, wide;

  /* Transparent pixels, some with a color, must come back exactly. */
  img->px[1 * 4 + 3] = 0x00000000u;
  img->px[3 * 4 + 0] = 0x00FF0000u;
  img->px[2 * 4 + 2] = 0x80123456u;
  img->px[3 * 4 + 3] = 0x01020304u;
  start = copy_of (img);

  /* Not a resize: the same size adds no step. */
  CHECK (easel_resize_edit (img, h, 4, 4, 0xFFFFFFFFu) == EASEL_RESIZE_UNCHANGED);
  CHECK (!easel_history_can_undo (h) && same_as (img, &start));

  CHECK (easel_resize_edit (img, h, 2, 2, 0xFFFFFFFFu) == EASEL_RESIZE_OK);
  shrunk = copy_of (img);
  CHECK (img->width == 2 && img->height == 2);
  for (int round = 0; round < 3; round++)
    {
      CHECK (easel_history_undo (h, img));
      CHECK (same_as (img, &start));
      CHECK (easel_history_redo (h, img));
      CHECK (same_as (img, &shrunk));
    }
  CHECK (easel_history_undo (h, img) && same_as (img, &start));

  /* Growing: undo cuts the added area, redo adds it again with the same fill, whatever the caller thinks of it now. */
  CHECK (easel_resize_edit (img, h, 6, 5, 0xFF112233u) == EASEL_RESIZE_OK);        /* discards the redo step */
  CHECK (!easel_history_can_redo (h));
  grown = copy_of (img);
  CHECK (easel_image_get (img, 5, 4) == 0xFF112233u);
  CHECK (easel_history_undo (h, img) && same_as (img, &start));
  CHECK (easel_history_redo (h, img) && same_as (img, &grown));
  CHECK (easel_history_undo (h, img) && same_as (img, &start));

  /* Wider and shorter at once: the removed rows come back, the added columns go. */
  CHECK (easel_resize_edit (img, h, 6, 2, 0xFF445566u) == EASEL_RESIZE_OK);
  wide = copy_of (img);
  CHECK (img->width == 6 && img->height == 2 && easel_image_get (img, 5, 1) == 0xFF445566u);
  CHECK (easel_history_undo (h, img) && same_as (img, &start));
  CHECK (easel_history_redo (h, img) && same_as (img, &wide));
  CHECK (easel_history_undo (h, img) && same_as (img, &start));

  free (start.px);
  free (shrunk.px);
  free (grown.px);
  free (wide.px);
  easel_history_free (h);
  easel_image_free (img);
}

static void
draw_line (EaselImage *img, EaselHistory *h, int x0, int y0, int x1, int y1, uint32_t color)
{
  EaselStroke *s = easel_stroke_begin (img, x0, y0, color);

  CHECK (s != NULL);
  easel_stroke_to (s, x1, y1);
  easel_stroke_end (s, h);
}

/* REQ-0004-22: steps before and after a resize stay undoable through it. */
static void
as22_steps_through_a_resize (void)
{
  EaselImage *img = easel_image_new (10, 10, 0xFFFFFFFFu);
  EaselHistory *h = easel_history_new ((size_t) 512 << 20);
  Copy s0 = copy_of (img), s1, s2, s3, s4, s5;

  draw_line (img, h, 0, 0, 9, 0, 0xFF000000u);          /* stroke A on 10 x 10 */
  s1 = copy_of (img);
  CHECK (easel_resize_edit (img, h, 5, 5, 0xFF777777u) == EASEL_RESIZE_OK);
  s2 = copy_of (img);
  draw_line (img, h, 2, 2, 4, 2, 0xFFFF0000u);          /* stroke B on 5 x 5 */
  s3 = copy_of (img);
  CHECK (easel_resize_edit (img, h, 12, 8, 0xFF0000FFu) == EASEL_RESIZE_OK);
  s4 = copy_of (img);
  draw_line (img, h, 0, 7, 11, 7, 0xFF00FF00u);         /* stroke C on 12 x 8: indexes use width 12 */
  s5 = copy_of (img);

  CHECK (easel_history_undo (h, img) && same_as (img, &s4));
  CHECK (easel_history_undo (h, img) && same_as (img, &s3));
  CHECK (easel_history_undo (h, img) && same_as (img, &s2));
  CHECK (easel_history_undo (h, img) && same_as (img, &s1));
  CHECK (easel_history_undo (h, img) && same_as (img, &s0));
  CHECK (!easel_history_can_undo (h));
  CHECK (easel_history_redo (h, img) && same_as (img, &s1));
  CHECK (easel_history_redo (h, img) && same_as (img, &s2));
  CHECK (easel_history_redo (h, img) && same_as (img, &s3));
  CHECK (easel_history_redo (h, img) && same_as (img, &s4));
  CHECK (easel_history_redo (h, img) && same_as (img, &s5));
  CHECK (!easel_history_can_redo (h));

  /* Modified follows the history across the resizes: saved after stroke B, undo to the resize, redo. */
  easel_history_mark_saved (h);
  CHECK (!easel_history_is_modified (h));
  CHECK (easel_history_undo (h, img) && easel_history_undo (h, img));
  CHECK (easel_history_is_modified (h));
  CHECK (easel_history_redo (h, img) && easel_history_redo (h, img));
  CHECK (!easel_history_is_modified (h));

  free (s0.px); free (s1.px); free (s2.px); free (s3.px); free (s4.px); free (s5.px);
  easel_history_free (h);
  easel_image_free (img);
}

/* REQ-0004-23: 4 bytes per removed pixel plus 64; adding pixels needs no storage. */
static void
as24_resize_step_budget (void)
{
  EaselImage *img = easel_image_new (100, 100, 0xFF445566u);
  EaselHistory *h = easel_history_new (10000);

  CHECK (easel_resize_edit (img, h, 90, 100, 0xFFFFFFFFu) == EASEL_RESIZE_OK);        /* removes 10 x 100 */
  CHECK (easel_history_bytes (h) == 4 * 1000 + 64);
  CHECK (easel_resize_edit (img, h, 80, 100, 0xFFFFFFFFu) == EASEL_RESIZE_OK);
  CHECK (easel_history_bytes (h) == 2 * (4 * 1000 + 64));
  /* The third does not fit in 10,000 bytes with the others: the oldest goes first. */
  CHECK (easel_resize_edit (img, h, 70, 100, 0xFFFFFFFFu) == EASEL_RESIZE_OK);
  CHECK (easel_history_bytes (h) == 2 * (4 * 1000 + 64));
  CHECK (easel_history_undo (h, img) && img->width == 80);
  CHECK (easel_history_undo (h, img) && img->width == 90);
  CHECK (!easel_history_can_undo (h));

  /* Growing removes nothing: 64 bytes, however much is added. */
  easel_history_free (h);
  h = easel_history_new ((size_t) 1 << 20);
  CHECK (easel_resize_edit (img, h, 5000, 5000, 0xFF000000u) == EASEL_RESIZE_OK);
  CHECK (easel_history_bytes (h) == 64);
  CHECK (easel_history_undo (h, img) && img->width == 90 && img->height == 100);
  easel_history_free (h);
  easel_image_free (img);
}

/* REQ-0004-23, REQ-0002-17: a resize step bigger than the whole budget stays alone; older steps are dropped. */
static void
as29_oversized_resize_kept_alone (void)
{
  enum { BUDGET = 16 << 20 };
  EaselImage *img = easel_image_new (4000, 2000, 0xFF336699u);
  EaselHistory *h = easel_history_new (BUDGET);
  Copy before;

  img->px[0] = 0xFF010203u;
  img->px[(size_t) 1999 * 4000 + 3999] = 0xFFABCDEFu;
  img->px[(size_t) 1000 * 4000 + 10] = 0x00000000u;
  draw_line (img, h, 10, 10, 50, 10, 0xFF000000u);
  draw_line (img, h, 10, 20, 50, 20, 0xFF000000u);
  draw_line (img, h, 10, 30, 50, 30, 0xFF000000u);
  CHECK (easel_history_bytes (h) < BUDGET);
  before = copy_of (img);

  CHECK (easel_resize_edit (img, h, 1, 1, 0xFFFFFFFFu) == EASEL_RESIZE_OK);          /* removes 7,999,999 pixels */
  CHECK (4 * (size_t) 7999999 + 64 > BUDGET);
  CHECK (easel_history_bytes (h) == 4 * (size_t) 7999999 + 64);                      /* over the budget, but alone */
  CHECK (img->width == 1 && img->height == 1);

  /* The three strokes are gone from the history; the resize can be undone and redone exactly. */
  CHECK (easel_history_undo (h, img));
  CHECK (same_as (img, &before));
  CHECK (!easel_history_can_undo (h));
  CHECK (easel_history_redo (h, img) && img->width == 1 && img->height == 1);
  CHECK (easel_history_undo (h, img) && same_as (img, &before));

  /* The next small step drops the oversized one, as for any step. */
  CHECK (easel_history_redo (h, img));
  draw_line (img, h, 0, 0, 0, 0, 0xFF123456u);
  CHECK (easel_history_bytes (h) == 12 + 64);
  CHECK (easel_history_undo (h, img) && !easel_history_can_undo (h));

  free (before.px);
  easel_history_free (h);
  easel_image_free (img);
}

static long
vm_size_kb (void)
{
  FILE *f = fopen ("/proc/self/status", "r");
  char line[256];
  long kb = -1;

  while (f != NULL && fgets (line, sizeof line, f) != NULL)
    if (sscanf (line, "VmSize: %ld kB", &kb) == 1)
      break;
  if (f != NULL)
    fclose (f);
  return kb;
}

/* REQ-0004-26: a resize that cannot be allocated changes neither the picture nor the history. */
static void
as27_resize_refused (void)
{
  EaselImage *img = pattern_image (50, 40);
  EaselHistory *h = easel_history_new ((size_t) 512 << 20);
  Copy before = copy_of (img);
  struct rlimit old, tight;
  long vm = vm_size_kb ();

  /* Sizes that are not allowed. */
  CHECK (easel_resize_edit (img, h, 0, 5, 0) == EASEL_RESIZE_BAD_SIZE);
  CHECK (easel_resize_edit (img, h, 10001, 5, 0) == EASEL_RESIZE_BAD_SIZE);
  CHECK (easel_resize_edit (img, h, 5, -3, 0) == EASEL_RESIZE_BAD_SIZE);
  CHECK (same_as (img, &before) && !easel_history_can_undo (h) && easel_history_bytes (h) == 0);

  /* Memory that is not there: the address space is limited to a little more than the process uses now. */
  CHECK (vm > 0 && getrlimit (RLIMIT_AS, &old) == 0);
  tight = old;
  tight.rlim_cur = (rlim_t) vm * 1024 + (rlim_t) 64 * 1024 * 1024;
  if (old.rlim_max != RLIM_INFINITY && tight.rlim_cur > old.rlim_max)
    tight.rlim_cur = old.rlim_max;
  CHECK (setrlimit (RLIMIT_AS, &tight) == 0);
  {
    EaselResizeResult grow = easel_resize_edit (img, h, 10000, 10000, 0xFF000000u);           /* 400 MB */
    EaselResizeResult shrink_after = easel_resize_edit (img, h, 10, 10, 0xFF000000u);        /* small: fits */

    CHECK (setrlimit (RLIMIT_AS, &old) == 0);
    CHECK (grow == EASEL_RESIZE_NO_MEMORY);
    CHECK (shrink_after == EASEL_RESIZE_OK);
  }
  /* The refused resize left nothing behind: the one step is the later, small one. */
  CHECK (easel_history_undo (h, img) && same_as (img, &before));
  CHECK (!easel_history_can_undo (h));

  free (before.px);
  easel_history_free (h);
  easel_image_free (img);
}

int
main (void)
{
  RUN (as19_grow_and_shrink);
  RUN (as21_undo_redo_resize_exact);
  RUN (as22_steps_through_a_resize);
  RUN (as24_resize_step_budget);
  RUN (as29_oversized_resize_kept_alone);
  RUN (as27_resize_refused);
  return 0;
}
