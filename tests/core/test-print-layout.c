/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Print (0026): the scale, the sheets, their place and the average. */

#include <math.h>
#include <stdint.h>
#include <string.h>

#include "check.h"
#include "print-layout.h"

#define MM_PER_PIXEL (25.4 / 96)

/* A4 with 19 mm margins. */
#define PORTRAIT 172.0, 259.0
#define LANDSCAPE 259.0, 172.0

static bool
near (double a, double b)
{
  return fabs (a - b) < 0.05;
}

/* AS-5 (REQ-0026-06). */
static void
as5_scale (void)
{
  EaselPrintLayout l = easel_print_layout (600, 400, PORTRAIT, true, 100);

  CHECK (near (l.percent, 100));
  l = easel_print_layout (800, 600, PORTRAIT, true, 100);
  CHECK (near (l.percent, 81.3));
  CHECK (l.sheets_across == 1 && l.sheets_down == 1);
  l = easel_print_layout (3000, 2000, PORTRAIT, true, 100);
  CHECK (near (l.percent, 21.7));
  CHECK (l.sheets_across == 1 && l.sheets_down == 1);
  /* Never enlarged. */
  l = easel_print_layout (1, 1, PORTRAIT, true, 100);
  CHECK (near (l.percent, 100));
  /* No lower limit. */
  l = easel_print_layout (10000, 10000, PORTRAIT, true, 100);
  CHECK (near (l.percent, 6.5));
  CHECK (l.sheets_across == 1 && l.sheets_down == 1);
  /* With fit off, the percent given: double the size at 200 %. */
  l = easel_print_layout (600, 400, 400.0, 400.0, false, 200);
  CHECK (near (l.percent, 200));
  CHECK (near (600 * MM_PER_PIXEL * l.percent / 100, 317.5));
  l = easel_print_layout (800, 600, LANDSCAPE, true, 100);
  CHECK (near (l.percent, 100));
  l = easel_print_layout (3000, 2000, LANDSCAPE, true, 100);
  CHECK (near (l.percent, 32.5));
  /* Fit on one page: whole pixels never spill onto a further sheet. */
  for (int w = 1; w < 4000; w += 37)
    {
      l = easel_print_layout (w, 1 + w / 2, PORTRAIT, true, 100);
      CHECK (l.sheets_across == 1 && l.sheets_down == 1);
      CHECK (l.percent <= 100);
    }
}

/* AS-6 (REQ-0026-07): whole pixels, in order, each on exactly one sheet. */
static void
as6_sheets (void)
{
  EaselPrintLayout l = easel_print_layout (3000, 2000, PORTRAIT, false, 100);
  int x, y, w, h;
  long covered = 0;

  CHECK (l.pixels_across == 650);
  CHECK (l.pixels_down == 978);
  CHECK (l.sheets_across == 5 && l.sheets_down == 3);
  CHECK (easel_print_sheets (&l) == 15);
  for (int i = 0; i < 15; i++)
    {
      easel_print_sheet (&l, 3000, 2000, i, &x, &y, &w, &h);
      CHECK (x == (i % 5) * 650);
      CHECK (y == (i / 5) * 978);
      CHECK (w > 0 && h > 0 && x + w <= 3000 && y + h <= 2000);
      covered += (long) w * h;
    }
  CHECK (covered == 3000L * 2000);
  easel_print_sheet (&l, 3000, 2000, 4, &x, &y, &w, &h);
  CHECK (w == 3000 - 4 * 650);
  /* 650 is one sheet across, 651 two. */
  l = easel_print_layout (650, 10, PORTRAIT, false, 100);
  CHECK (l.sheets_across == 1);
  l = easel_print_layout (651, 10, PORTRAIT, false, 100);
  CHECK (l.sheets_across == 2);
}

/* AS-7 (REQ-0026-08). */
static void
as7_place (void)
{
  EaselPrintLayout l = easel_print_layout (100, 100, PORTRAIT, false, 100);
  double side = 100 * MM_PER_PIXEL, x, y;

  easel_print_place (&l, 100, 100, 172, 259, false, false, &x, &y);
  CHECK (near (x, 0) && near (y, 0));
  easel_print_place (&l, 100, 100, 172, 259, true, false, &x, &y);
  CHECK (near (x, (172 - side) / 2) && near (y, 0));
  easel_print_place (&l, 100, 100, 172, 259, false, true, &x, &y);
  CHECK (near (x, 0) && near (y, (259 - side) / 2));
  /* Two sheets across: from the margin, along that axis only. */
  l = easel_print_layout (651, 100, PORTRAIT, false, 100);
  easel_print_place (&l, 651, 100, 172, 259, true, true, &x, &y);
  CHECK (near (x, 0));
  CHECK (near (y, (259 - side) / 2));
}

/* AS-3 and AS-4 (REQ-0026-04, -05): the orientation that follows the picture, and the room. */
static void
as4_room_and_orientation (void)
{
  CHECK (easel_print_wide_is_landscape (800, 600));
  CHECK (!easel_print_wide_is_landscape (600, 800));
  CHECK (!easel_print_wide_is_landscape (600, 600));
  /* Adjust to: at least one pixel; 1 mm at 400 % (a pixel is 1.06 mm) is too little. */
  CHECK (!easel_print_area_ok (1.0, 100.0, false, 400));
  CHECK (easel_print_area_ok (1.1, 100.0, false, 400));
  /* Fit to: any area that is not empty. */
  CHECK (easel_print_area_ok (0.5, 0.5, true, 100));
  CHECK (!easel_print_area_ok (0.0, 100.0, true, 100));
  CHECK (!easel_print_area_ok (100.0, -1.0, true, 100));
}

/* AS-8 (REQ-0026-09): each dot the average of the pixels it covers, by area. */
static void
as8_average (void)
{
  const uint32_t black = 0xFF000000u, white = 0xFFFFFFFFu;
  uint32_t src[16], dst[4];

  for (int i = 0; i < 16; i++)
    src[i] = ((i % 4) + (i / 4)) % 2 ? white : black;
  /* Two pixels to a dot, a checker: every dot is the middle gray. */
  easel_print_average (src, 4, 4, 4, 2.0, dst, 2, 2);
  for (int i = 0; i < 4; i++)
    CHECK (dst[i] == 0xFF808080u);
  /* One color stays exactly that color. */
  for (int i = 0; i < 16; i++)
    src[i] = 0xFF123456u;
  easel_print_average (src, 4, 4, 4, 2.0, dst, 2, 2);
  CHECK (dst[0] == 0xFF123456u && dst[3] == 0xFF123456u);
  /* A left half red, a right half blue, at 4 pixels a dot: an even mix, each channel on its own. */
  for (int i = 0; i < 16; i++)
    src[i] = (i % 4) < 2 ? 0xFFFF0000u : 0xFF0000FFu;
  easel_print_average (src, 4, 4, 4, 4.0, dst, 1, 1);
  CHECK (dst[0] == 0xFF800080u);
  /* 1.5 pixels to a dot: the first dot is 1 of pixel 0 and 1/2 of pixel 1, by area. */
  {
    uint32_t row[3] = { 0xFF000000u, 0xFF969696u, 0xFF000000u }, out[2];

    easel_print_average (row, 3, 1, 3, 1.5, out, 2, 1);
    CHECK (out[0] == 0xFF323232u);
    CHECK (out[1] == 0xFF323232u);
  }
}

int
main (void)
{
  RUN (as5_scale);
  RUN (as6_sheets);
  RUN (as7_place);
  RUN (as4_room_and_orientation);
  RUN (as8_average);
  return 0;
}
