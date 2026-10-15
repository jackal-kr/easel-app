/* SPDX-License-Identifier: GPL-3.0-or-later */
/* EaselImage bounds and size checks (core rule: bounds-clip every access, check size arithmetic). */

#include <string.h>

#include "check.h"
#include "image.h"

/* REQ-0001-01, REQ-0001-02: a picture of an impossible size is refused, never half-made. */
static void
image_rejects_bad_size (void)
{
  CHECK (easel_image_new (0, 5, 0) == NULL);
  CHECK (easel_image_new (5, -1, 0) == NULL);
  CHECK (easel_image_new (2147483647, 2147483647, 0) == NULL);
}

/* REQ-0001-22: reads outside the picture are clipped. */
static void
image_get_outside_is_transparent (void)
{
  EaselImage *img = easel_image_new (3, 2, 0xFF123456u);

  CHECK (img != NULL);
  CHECK (easel_image_get (img, 2, 1) == 0xFF123456u);
  CHECK (easel_image_get (img, 3, 0) == 0);
  CHECK (easel_image_get (img, 0, 2) == 0);
  CHECK (easel_image_get (img, -1, 0) == 0);
  CHECK (easel_image_get (img, 0, -1) == 0);
  easel_image_free (img);
  easel_image_free (NULL);
}

/* REQ-0001-19, REQ-0001-20: every pixel write marks the area the screen must update. */
static void
image_set_marks_dirty (void)
{
  EaselImage *img = easel_image_new (10, 10, 0);
  EaselRect r;

  CHECK (!easel_image_take_dirty (img, &r));
  easel_image_set (img, 2, 3, 0xFF000000u);
  easel_image_set (img, 7, 1, 0xFF000000u);
  easel_image_set (img, 10, 1, 0xFF000000u);
  easel_image_set (img, -1, 1, 0xFF000000u);
  CHECK (easel_image_get (img, 2, 3) == 0xFF000000u);
  CHECK (easel_image_take_dirty (img, &r));
  CHECK (r.x0 == 2 && r.y0 == 1 && r.x1 == 8 && r.y1 == 4);
  CHECK (!easel_image_take_dirty (img, &r));
  easel_image_free (img);
}

/* 0005 AS-10: the one flattening formula, round ((c * a + 255 * (255 - a)) / 255), at the values that matter. */
static void
as10_flatten_formula (void)
{
  static const int alphas[] = { 0, 1, 127, 128, 254, 255 };
  static const struct { int a, c, want; } hand[] = {
    { 0, 77, 255 }, { 255, 77, 77 }, { 127, 100, 178 }, { 128, 100, 177 }, { 1, 0, 254 }, { 254, 10, 11 },
    { 0, 0, 255 }, { 255, 255, 255 }, { 255, 0, 0 }, { 128, 255, 255 }, { 128, 0, 127 },
  };
  uint32_t px[1];
  uint8_t rgb[3];

  for (size_t i = 0; i < sizeof hand / sizeof hand[0]; i++)
    {
      px[0] = ((uint32_t) hand[i].a << 24) | ((uint32_t) hand[i].c << 16) | ((uint32_t) hand[i].c << 8) | (uint32_t) hand[i].c;
      easel_flatten_on_white (px, rgb, 1);
      CHECK (rgb[0] == hand[i].want && rgb[1] == hand[i].want && rgb[2] == hand[i].want);
    }
  /* Every channel and alpha against the formula in floating point, and the channels stay apart. */
  for (size_t k = 0; k < sizeof alphas / sizeof alphas[0]; k++)
    for (int c = 0; c < 256; c++)
      {
        int a = alphas[k], want = (int) ((c * a + 255.0 * (255 - a)) / 255.0 + 0.5);

        px[0] = ((uint32_t) a << 24) | ((uint32_t) c << 16) | (uint32_t) ((255 - c) << 8) | (uint32_t) (c / 2);
        easel_flatten_on_white (px, rgb, 1);
        CHECK (rgb[0] == want);
        CHECK (rgb[1] == (int) (((255 - c) * a + 255.0 * (255 - a)) / 255.0 + 0.5));
        CHECK (rgb[2] == (int) (((c / 2) * a + 255.0 * (255 - a)) / 255.0 + 0.5));
      }
}

/* 0005 AS-18: the one pass that converts decoded bytes and applies the orientation (EXIF values 1 to 8). */
static void
as18_orientation_pass (void)
{
  /* A 3 x 2 picture, stored as   a b c   with a = 1 .. f = 6 in the blue channel. */
  static const uint8_t rgba[2][3][4] = {
    { { 0, 0, 1, 255 }, { 0, 0, 2, 255 }, { 0, 0, 3, 255 } },
    { { 0, 0, 4, 255 }, { 0, 0, 5, 255 }, { 0, 0, 6, 255 } },
  };
  /* The picture each orientation must give, row by row, as the numbers 1 .. 6 (EXIF 1 to 8). */
  static const struct { int w, h; int v[6]; } want[8] = {
    { 3, 2, { 1, 2, 3, 4, 5, 6 } },     /* 1 as stored */
    { 3, 2, { 3, 2, 1, 6, 5, 4 } },     /* 2 mirrored left to right */
    { 3, 2, { 6, 5, 4, 3, 2, 1 } },     /* 3 turned 180 degrees */
    { 3, 2, { 4, 5, 6, 1, 2, 3 } },     /* 4 mirrored top to bottom */
    { 2, 3, { 1, 4, 2, 5, 3, 6 } },     /* 5 transposed */
    { 2, 3, { 4, 1, 5, 2, 6, 3 } },     /* 6 turned 90 degrees clockwise */
    { 2, 3, { 6, 3, 5, 2, 4, 1 } },     /* 7 transversed */
    { 2, 3, { 3, 6, 2, 5, 1, 4 } },     /* 8 turned 90 degrees counter-clockwise */
  };
  uint8_t rgb[2][3][3];
  uint8_t padded[2][3 * 4 + 5];
  EaselImage *img;

  for (int o = 1; o <= 8; o++)
    {
      img = easel_image_from_bytes (&rgba[0][0][0], 3, 2, 12, 4, o);
      CHECK (img != NULL && img->width == want[o - 1].w && img->height == want[o - 1].h);
      for (int i = 0; i < 6; i++)
        CHECK (img->px[i] == (0xFF000000u | (uint32_t) want[o - 1].v[i]));
      easel_image_free (img);
    }
  /* A tag outside 1 to 8 is "as stored". */
  img = easel_image_from_bytes (&rgba[0][0][0], 3, 2, 12, 4, 0);
  CHECK (img != NULL && img->width == 3 && img->px[5] == 0xFF000006u);
  easel_image_free (img);
  img = easel_image_from_bytes (&rgba[0][0][0], 3, 2, 12, 4, 9);
  CHECK (img != NULL && img->width == 3 && img->px[5] == 0xFF000006u);
  easel_image_free (img);

  /* Three channels: opaque. Rows are stride bytes apart, and the padding is not read as pixels. */
  for (int y = 0; y < 2; y++)
    for (int x = 0; x < 3; x++)
      {
        rgb[y][x][0] = (uint8_t) (10 * y);
        rgb[y][x][1] = (uint8_t) (20 + x);
        rgb[y][x][2] = (uint8_t) (30 + y);
      }
  for (int y = 0; y < 2; y++)
    {
      memset (padded[y], 0xEE, sizeof padded[y]);
      memcpy (padded[y], rgb[y], 9);
    }
  img = easel_image_from_bytes (&padded[0][0], 3, 2, (int) sizeof padded[0], 3, 1);
  CHECK (img != NULL && img->width == 3 && img->height == 2);
  CHECK (img->px[0] == 0xFF00141Eu);
  CHECK (img->px[4] == 0xFF0A151Fu);
  easel_image_free (img);

  /* A fully transparent pixel is #00000000 whatever its color; a nearly transparent one is kept. */
  {
    static const uint8_t t[2][4] = { { 200, 100, 50, 0 }, { 200, 100, 50, 1 } };

    img = easel_image_from_bytes (&t[0][0], 2, 1, 8, 4, 1);
    CHECK (img != NULL && img->px[0] == 0x00000000u && img->px[1] == 0x01C86432u);
    easel_image_free (img);
  }
  /* No memory, and sizes that cannot be allocated: NULL. */
  CHECK (easel_image_from_bytes (&rgba[0][0][0], 0, 2, 12, 4, 1) == NULL);
}

int
main (void)
{
  RUN (image_rejects_bad_size);
  RUN (image_get_outside_is_transparent);
  RUN (image_set_marks_dirty);
  RUN (as10_flatten_formula);
  RUN (as18_orientation_pass);
  return 0;
}
