/* SPDX-License-Identifier: GPL-3.0-or-later */
/* EaselImage bounds and size checks (core rule: bounds-clip every access, check size arithmetic). */

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

int
main (void)
{
  RUN (image_rejects_bad_size);
  RUN (image_get_outside_is_transparent);
  RUN (image_set_marks_dirty);
  return 0;
}
