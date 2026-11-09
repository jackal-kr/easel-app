/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Black and White (feature 0034): the three conversions (REQ-0034-04 to -06) on pixels, with a shape and transparent pixels, against
   a reference written from the spec's words; the whole picture's one undo step and the floating selection as shown (REQ-0034-03). */

#include <malloc.h>
#include <stdlib.h>
#include <string.h>

#include "black-white.h"
#include "check.h"
#include "history.h"
#include "selection.h"

#define BIG ((size_t) 1 << 40)
#define W_ 0xFFFFFFFFu
#define B_ 0xFF000000u
#define GRAY(g, a) ((uint32_t) (a) << 24 | (uint32_t) (g) << 16 | (uint32_t) (g) << 8 | (uint32_t) (g))

static const int matrix[8][8] = { { 0, 32, 8, 40, 2, 34, 10, 42 }, { 48, 16, 56, 24, 50, 18, 58, 26 },
                                  { 12, 44, 4, 36, 14, 46, 6, 38 }, { 60, 28, 52, 20, 62, 30, 54, 22 },
                                  { 3, 35, 11, 43, 1, 33, 9, 41 },  { 51, 19, 59, 27, 49, 17, 57, 25 },
                                  { 15, 47, 7, 39, 13, 45, 5, 37 }, { 63, 31, 55, 23, 61, 29, 53, 21 } };

/* ---- The reference, from the spec's words: a full table of values, shares by C division (toward zero) ---- */

static long long
brightness (uint32_t p)
{
  return 299LL * (p >> 16 & 0xFF) + 587LL * (p >> 8 & 0xFF) + 114LL * (p & 0xFF);
}

static uint32_t
bw (uint32_t p, int white)
{
  return (p & 0xFF000000u) | (white ? 0x00FFFFFFu : 0);
}

static void
reference (uint32_t *px, const uint8_t *mask, int w, int h, EaselBw how)
{
  long long *v = malloc ((size_t) w * (size_t) h * sizeof *v);

  CHECK (v != NULL);
  for (int i = 0; i < w * h; i++)
    v[i] = brightness (px[i]);
  for (int y = 0; y < h; y++)
    for (int x = 0; x < w; x++)
      {
        int i = y * w + x, white;

        if (mask != NULL && !mask[i])
          continue;
        if (how == EASEL_BW_THRESHOLD)
          white = v[i] >= 128000;
        else if (how == EASEL_BW_ORDERED)
          white = 128 * v[i] >= 255000LL * (2 * matrix[y % 8][x % 8] + 1);
        else if ((px[i] >> 24) == 0)
          white = brightness (px[i]) >= 128000;        /* by its own Y alone, no share in or out */
        else
          {
            static const int sx[4] = { 1, -1, 0, 1 }, sy[4] = { 0, 1, 1, 1 }, k[4] = { 7, 3, 5, 1 };
            long long e;

            white = v[i] >= 128000;
            e = v[i] - (white ? 255000 : 0);
            for (int s = 0; s < 4; s++)
              {
                int nx = x + sx[s], ny = y + sy[s], j = ny * w + nx;

                if (nx < 0 || nx >= w || ny >= h || (mask != NULL && !mask[j]) || (px[j] >> 24) == 0)
                  continue;
                v[j] += k[s] * e / 16;
              }
          }
        px[i] = bw (px[i], white);
      }
  free (v);
}

/* ---- The scenarios ---- */

/* AS-2: Threshold on six colors; the alpha kept. */
static void
as2_threshold (void)
{
  uint32_t px[6] = { 0xFF7F7F7Fu, 0xFF808080u, 0xFFFF0000u, 0xFFFFFF00u, 0xFF00FF00u, 0x800000FFu };
  static const uint32_t want[6] = { B_, W_, B_, W_, W_, 0x80000000u };

  CHECK (easel_bw_convert (px, NULL, 6, 1, EASEL_BW_THRESHOLD));
  for (int i = 0; i < 6; i++)
    CHECK (px[i] == want[i]);
}

/* AS-3: Ordered dither of gray 128: white exactly where the matrix is below 32; 16 x 16 repeats it; black and white stay. */
static void
as3_ordered (void)
{
  uint32_t px[16 * 16];
  int whites = 0;

  for (int i = 0; i < 16 * 16; i++)
    px[i] = GRAY (128, 255);
  CHECK (easel_bw_convert (px, NULL, 16, 16, EASEL_BW_ORDERED));
  for (int y = 0; y < 16; y++)
    for (int x = 0; x < 16; x++)
      {
        CHECK (px[y * 16 + x] == (matrix[y % 8][x % 8] < 32 ? W_ : B_));
        whites += px[y * 16 + x] == W_;
      }
  CHECK (whites == 4 * 32);
  CHECK (px[0] == W_ && px[1] == B_);                      /* row 0: W B W B ... */
  for (int i = 0; i < 64; i++)
    px[i] = i < 32 ? B_ : W_;
  CHECK (easel_bw_convert (px, NULL, 8, 8, EASEL_BW_ORDERED));
  for (int i = 0; i < 64; i++)
    CHECK (px[i] == (i < 32 ? B_ : W_));
}

/* AS-4: Error diffusion of gray 128 (4 x 2) and gray 64 (4 x 4). */
static void
as4_diffusion (void)
{
  static const char *const want128 = "WBWBBWBW", *const want64 = "BBBBBWBWBBBBBWBB";
  uint32_t px[16];

  for (int i = 0; i < 8; i++)
    px[i] = GRAY (128, 255);
  CHECK (easel_bw_convert (px, NULL, 4, 2, EASEL_BW_DIFFUSION));
  for (int i = 0; i < 8; i++)
    CHECK (px[i] == (want128[i] == 'W' ? W_ : B_));
  for (int i = 0; i < 16; i++)
    px[i] = GRAY (64, 255);
  CHECK (easel_bw_convert (px, NULL, 4, 4, EASEL_BW_DIFFUSION));
  for (int i = 0; i < 16; i++)
    CHECK (px[i] == (want64[i] == 'W' ? W_ : B_));
  /* A share is rounded toward zero (REQ-0034-06): gray 128 is white with e = -127000, 7e / 16 = -55562.06 gives -55562, and the
     next pixel's Y of 183562 (29, 249, 252) then reaches exactly 128000: white; rounded down it would be 127999, black. */
  px[0] = GRAY (128, 255);
  px[1] = 0xFF1DF9FCu;
  CHECK (easel_bw_convert (px, NULL, 2, 1, EASEL_BW_DIFFUSION));
  CHECK (px[0] == W_ && px[1] == W_);
}

/* AS-6: a free-form L in a 3 x 3 box of gray 128: W, B, W, B, W, the rest unchanged; a transparent pixel takes and passes no
   error: W, W, W, B, where all opaque gives W, B, W, B. */
static void
as6_shape_and_transparent (void)
{
  static const uint8_t mask[9] = { 1, 0, 0, 1, 0, 0, 1, 1, 1 };
  static const int order[5] = { 0, 3, 6, 7, 8 };
  static const char want[5] = { 'W', 'B', 'W', 'B', 'W' };
  uint32_t px[9], row[4];

  for (int i = 0; i < 9; i++)
    px[i] = GRAY (128, 255);
  CHECK (easel_bw_convert (px, mask, 3, 3, EASEL_BW_DIFFUSION));
  for (int k = 0; k < 5; k++)
    CHECK (px[order[k]] == (want[k] == 'W' ? W_ : B_));
  CHECK (px[1] == GRAY (128, 255) && px[2] == GRAY (128, 255) && px[4] == GRAY (128, 255) && px[5] == GRAY (128, 255));
  row[0] = GRAY (128, 255), row[1] = GRAY (128, 0), row[2] = GRAY (128, 255), row[3] = GRAY (128, 255);
  CHECK (easel_bw_convert (row, NULL, 4, 1, EASEL_BW_DIFFUSION));
  CHECK (row[0] == W_ && row[1] == 0x00FFFFFFu && row[2] == W_ && row[3] == B_);
  for (int i = 0; i < 4; i++)
    row[i] = GRAY (128, 255);
  CHECK (easel_bw_convert (row, NULL, 4, 1, EASEL_BW_DIFFUSION));
  CHECK (row[0] == W_ && row[1] == B_ && row[2] == W_ && row[3] == B_);
}

/* The three conversions against the reference: random sizes, colors, alphas (some 0) and shapes; and twice changes nothing. */
static void
matches_the_reference (void)
{
  unsigned int seed = 34;

  for (int n = 0; n < 3000; n++)
    {
      int w, h;
      uint32_t *px, *want, *again;
      uint8_t *mask = NULL;

      seed = seed * 1103515245u + 12345u;
      w = 1 + (int) ((seed >> 8) % 23);
      seed = seed * 1103515245u + 12345u;
      h = 1 + (int) ((seed >> 8) % 19);
      px = malloc ((size_t) w * h * sizeof *px);
      want = malloc ((size_t) w * h * sizeof *want);
      again = malloc ((size_t) w * h * sizeof *again);
      CHECK (px != NULL && want != NULL && again != NULL);
      if (n % 3 == 1)
        {
          mask = malloc ((size_t) w * h);
          CHECK (mask != NULL);
        }
      for (int i = 0; i < w * h; i++)
        {
          seed = seed * 1103515245u + 12345u;
          px[i] = seed ^ seed << 11;
          if (n % 4 == 0)
            px[i] |= 0xFF000000u;
          if (seed % 7 == 0)
            px[i] &= 0x00FFFFFFu;                         /* fully transparent */
          if (mask != NULL)
            mask[i] = (seed >> 3) % 3 != 0;
        }
      for (int how = 0; how < 3; how++)
        {
          memcpy (want, px, (size_t) w * h * sizeof *px);
          memcpy (again, px, (size_t) w * h * sizeof *px);
          reference (want, mask, w, h, (EaselBw) how);
          CHECK (easel_bw_convert (again, mask, w, h, (EaselBw) how));
          CHECK (memcmp (want, again, (size_t) w * h * sizeof *px) == 0);
          CHECK (easel_bw_convert (again, mask, w, h, (EaselBw) how));            /* black and white stay */
          CHECK (memcmp (want, again, (size_t) w * h * sizeof *px) == 0);
        }
      free (mask);
      free (again);
      free (want);
      free (px);
    }
}

/* ---- The whole picture's step ---- */

static size_t
heap_in_use (void)
{
  struct mallinfo2 mi = mallinfo2 ();

  return mi.uordblks + mi.hblkhd;
}

/* AS-5's whole picture: one step, Undo gives it back exactly; few changes are a log, many a buffer; the memory held is what the
   history counts; a picture already black and white adds no step; NO_MEMORY under a limit leaves everything as it was. */
static void
picture_step (void)
{
  for (int kind = 0; kind < 2; kind++)
    {
      EaselImage *img = easel_image_new (300, 200, kind == 0 ? W_ : GRAY (100, 255)), *before;
      EaselHistory *h = easel_history_new ((size_t) 512 << 20);
      size_t held, counted, start;

      CHECK (img != NULL && h != NULL);
      for (int i = 0; i < 300 * 200; i += 7)
        img->px[i] = GRAY ((i * 13) % 256, (i % 3) ? 255 : 128);
      before = easel_image_new (300, 200, 0);
      CHECK (before != NULL);
      memcpy (before->px, img->px, (size_t) 300 * 200 * sizeof (uint32_t));
      start = heap_in_use ();
      CHECK (easel_image_black_white (img, h, EASEL_BW_DIFFUSION, BIG) == EASEL_TRANSFORM_DONE);
      held = heap_in_use () - start;
      counted = easel_history_bytes (h);
      CHECK (held <= counted + counted / 100 + 4096);
      if (kind == 0)
        CHECK (counted < (size_t) 300 * 200 * 4);                 /* few pixels changed: a log */
      else
        CHECK (counted == (size_t) 300 * 200 * 4 + 64);           /* most changed: the old buffer */
      for (int i = 0; i < 300 * 200; i++)
        CHECK ((img->px[i] & 0x00FFFFFFu) == 0 || (img->px[i] & 0x00FFFFFFu) == 0x00FFFFFFu);
      CHECK (easel_history_undo (h, img));
      CHECK (memcmp (img->px, before->px, (size_t) 300 * 200 * sizeof (uint32_t)) == 0);
      easel_history_free (h);
      easel_image_free (before);
      easel_image_free (img);
    }
}

/* One Undo restores the picture exactly; a second conversion of the result adds no step; under a limit nothing changes. */
static void
picture_undo_nothing_memory (void)
{
  EaselImage *img = easel_image_new (64, 48, 0), *before = easel_image_new (64, 48, 0);
  EaselHistory *h = easel_history_new ((size_t) 64 << 20);

  CHECK (img != NULL && before != NULL && h != NULL);
  for (int i = 0; i < 64 * 48; i++)
    img->px[i] = 0xFF000000u | (uint32_t) (i * 2654435761u >> 8);
  memcpy (before->px, img->px, (size_t) 64 * 48 * sizeof (uint32_t));
  CHECK (easel_image_black_white (img, h, EASEL_BW_ORDERED, 1000) == EASEL_TRANSFORM_NO_MEMORY);
  CHECK (memcmp (img->px, before->px, (size_t) 64 * 48 * 4) == 0 && !easel_history_can_undo (h));
  CHECK (easel_image_black_white (img, h, EASEL_BW_ORDERED, BIG) == EASEL_TRANSFORM_DONE);
  CHECK (memcmp (img->px, before->px, (size_t) 64 * 48 * 4) != 0 && easel_history_can_undo (h));
  CHECK (easel_image_black_white (img, h, EASEL_BW_ORDERED, BIG) == EASEL_TRANSFORM_NOTHING);
  CHECK (easel_history_undo (h, img));
  CHECK (memcmp (img->px, before->px, (size_t) 64 * 48 * 4) == 0 && !easel_history_can_undo (h));
  easel_history_free (h);
  easel_image_free (before);
  easel_image_free (img);
}

/* ---- The floating selection: what is shown is converted and becomes its image (REQ-0034-03, AS-6, AS-9) ---- */

/* A picture whose pixel (x, y) is a different gray-ish color, asymmetric, so a wrong orientation shows. */
static EaselImage *
ramp (int w, int h)
{
  EaselImage *img = easel_image_new (w, h, 0);

  CHECK (img != NULL);
  for (int y = 0; y < h; y++)
    for (int x = 0; x < w; x++)
      img->px[y * w + x] = 0xFF000000u | (uint32_t) ((x * 37 + y * 11) % 256) * 0x010101u;
  return img;
}

/* The float's shown image and mask, copied, then converted as REQ-0034-03 says: the expected image. */
static uint32_t *
expected_of (EaselFloat *f, EaselBw how, uint8_t **mask_out)
{
  uint32_t *px;
  uint8_t *mask = NULL;

  easel_selection_complete (f);
  px = malloc ((size_t) f->w * f->h * sizeof *px);
  CHECK (px != NULL);
  memcpy (px, f->px, (size_t) f->w * f->h * sizeof *px);
  if (f->mask != NULL)
    {
      mask = malloc ((size_t) f->w * f->h);
      CHECK (mask != NULL);
      memcpy (mask, f->mask, (size_t) f->w * f->h);
    }
  CHECK (easel_bw_convert (px, mask, f->w, f->h, how));
  *mask_out = mask;
  return px;
}

static void
check_float_is (const EaselFloat *f, const uint32_t *want, const uint8_t *mask, int w, int h)
{
  CHECK (f->w == w && f->h == h && f->ow == w && f->oh == h && f->orient == EASEL_ORIENT_IDENTITY && f->orig_px == NULL);
  for (int i = 0; i < w * h; i++)
    if (mask == NULL || mask[i])
      CHECK (f->px[i] == want[i]);
}

/* AS-9: a 4 x 4 float of gray 128 stretched to 8 x 8: its 8 x 8 pixels are an 8 x 8 picture's dither (one-pixel dots), and they are its
   image now (a later stretch starts from them). */
static void
float_stretched (void)
{
  EaselImage *img = easel_image_new (4, 4, GRAY (128, 255));
  EaselFloat f = { 0 };

  CHECK (img != NULL);
  CHECK (easel_selection_lift (&f, img, 0, 0, 4, 4, NULL, false, W_, SIZE_MAX) == EASEL_SELECTION_DONE);
  CHECK (easel_selection_stretch (&f, 0, 0, 8, 8, SIZE_MAX) == EASEL_SELECTION_DONE);
  CHECK (easel_selection_black_white (&f, EASEL_BW_ORDERED, SIZE_MAX) == EASEL_SELECTION_DONE);
  CHECK (f.w == 8 && f.h == 8 && f.ow == 8 && f.oh == 8 && f.orig_px == NULL);
  for (int y = 0; y < 8; y++)
    for (int x = 0; x < 8; x++)
      CHECK (f.px[y * 8 + x] == (matrix[y][x] < 32 ? W_ : B_));
  CHECK (f.px[0] == W_ && f.px[1] == B_);
  CHECK (easel_selection_stretch (&f, 0, 0, 16, 16, SIZE_MAX) == EASEL_SELECTION_DONE);    /* from the converted pixels */
  easel_selection_complete (&f);
  CHECK (f.px[0] == W_ && f.px[1] == W_ && f.px[2] == B_);
  easel_float_free (&f);
  easel_image_free (img);
}

/* A turned float, a free-form one and a pasted one with transparent pixels: each converted as shown, with its shape; Error diffusion's
   rows and columns are the shown image's. */
static void
float_turned_shaped_pasted (void)
{
  EaselImage *img = ramp (7, 5);
  static const uint8_t L[9] = { 1, 0, 0, 1, 0, 0, 1, 1, 1 };

  for (int how = 0; how < 3; how++)
    {
      EaselFloat f = { 0 }, g = { 0 }, p = { 0 };
      uint32_t *want;
      uint8_t *mask;
      EaselImage *pic = ramp (5, 3);

      CHECK (easel_selection_lift (&f, img, 1, 1, 5, 3, NULL, false, W_, SIZE_MAX) == EASEL_SELECTION_DONE);
      CHECK (easel_selection_transform (&f, EASEL_OP_ROTATE_90, SIZE_MAX) == EASEL_SELECTION_DONE);
      want = expected_of (&f, (EaselBw) how, &mask);
      CHECK (f.w == 3 && f.h == 5);
      CHECK (easel_selection_black_white (&f, (EaselBw) how, SIZE_MAX) == EASEL_SELECTION_DONE);
      check_float_is (&f, want, mask, 3, 5);
      free (want);
      free (mask);
      easel_float_free (&f);

      CHECK (easel_selection_lift (&g, img, 2, 1, 3, 3, L, false, W_, SIZE_MAX) == EASEL_SELECTION_DONE);
      want = expected_of (&g, (EaselBw) how, &mask);
      CHECK (mask != NULL);
      CHECK (easel_selection_black_white (&g, (EaselBw) how, SIZE_MAX) == EASEL_SELECTION_DONE);
      check_float_is (&g, want, mask, 3, 3);
      CHECK (memcmp (g.mask, L, 9) == 0);                                  /* the shape kept */
      free (want);
      free (mask);
      easel_float_free (&g);

      pic->px[1] &= 0x00FFFFFFu;                                          /* a transparent pixel, not put down */
      pic->px[7] &= 0x00FFFFFFu;
      CHECK (easel_selection_paste (&p, pic, 0, 0, SIZE_MAX) == EASEL_SELECTION_DONE);
      want = expected_of (&p, (EaselBw) how, &mask);
      CHECK (easel_selection_black_white (&p, (EaselBw) how, SIZE_MAX) == EASEL_SELECTION_DONE);
      check_float_is (&p, want, mask, 5, 3);
      CHECK ((p.px[1] >> 24) == 0 && (p.px[7] >> 24) == 0 && p.skip_clear);
      free (want);
      free (mask);
      easel_float_free (&p);
      easel_image_free (pic);
    }
  easel_image_free (img);
}

/* A copy left before the conversion keeps its image; without the memory the float is as it was. */
static void
float_copies_and_memory (void)
{
  EaselImage *img = ramp (6, 4);
  EaselFloat f = { 0 };
  uint32_t before[16];

  CHECK (easel_selection_lift (&f, img, 1, 0, 4, 4, NULL, false, W_, SIZE_MAX) == EASEL_SELECTION_DONE);
  memcpy (before, f.px, sizeof before);
  CHECK (easel_selection_black_white (&f, EASEL_BW_THRESHOLD, 10) == EASEL_SELECTION_NO_MEMORY);
  CHECK (f.w == 4 && memcmp (f.px, before, sizeof before) == 0);
  CHECK (easel_selection_stamp (&f, SIZE_MAX));
  CHECK (easel_selection_black_white (&f, EASEL_BW_THRESHOLD, SIZE_MAX) == EASEL_SELECTION_DONE);
  CHECK (f.n_stamps == 1 && f.stamp_img != NULL && f.stamp_img[0] >= 0 && (size_t) f.stamp_img[0] < f.n_kept);
  CHECK (memcmp (f.kept[f.stamp_img[0]].px, before, sizeof before) == 0);      /* the copy keeps the colors */
  CHECK (memcmp (f.px, before, sizeof before) != 0);
  easel_float_free (&f);
  easel_image_free (img);
}

int
main (void)
{
  RUN (as2_threshold);
  RUN (as3_ordered);
  RUN (as4_diffusion);
  RUN (as6_shape_and_transparent);
  RUN (matches_the_reference);
  RUN (picture_step);
  RUN (picture_undo_nothing_memory);
  RUN (float_stretched);
  RUN (float_turned_shaped_pasted);
  RUN (float_copies_and_memory);
  return 0;
}
