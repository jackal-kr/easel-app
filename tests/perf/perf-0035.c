/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Measurements of feature 0035 (REQ-0035-09, AS-7): Remove background's analysis with the first preview (the dialog's opening), a
   preview for another tolerance, and the result at full size, on a 1920 x 1080 and an 8000 x 8000 picture, in the core, three runs;
   the memory the analysis holds besides. The picture: a disk and a square on a gradient with noise, so that the surface, the edges and
   the boundary all have work. Run in the real session with the load noted by the caller. It prints numbers and does not fail. */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "remove-bg.h"

#define RUNS 3

static double
now_ms (void)
{
  struct timespec t;

  clock_gettime (CLOCK_MONOTONIC, &t);
  return t.tv_sec * 1000.0 + t.tv_nsec / 1e6;
}

static void
pattern (uint32_t *px, int w, int h)
{
  long r2 = (long) (h / 3) * (h / 3);

  for (int y = 0; y < h; y++)
    for (int x = 0; x < w; x++)
      {
        size_t i = (size_t) y * w + x;
        long dx = x - w / 3, dy = y - h / 2;
        uint32_t g = 180 + (uint32_t) (60L * y / h) + ((uint32_t) (i * 2654435761u) >> 29);

        if (dx * dx + dy * dy <= r2)
          px[i] = 0xFFC02020u;
        else if (x > w * 3 / 5 && x < w * 4 / 5 && y > h / 4 && y < h * 3 / 4)
          px[i] = 0xFF2040A0u;
        else
          px[i] = 0xFF000000u | g << 16 | g << 8 | (g - 20);
      }
}

static void
measure (int w, int h, double open_budget, double result_budget)
{
  uint32_t *px = malloc ((size_t) w * h * sizeof *px), *dst = malloc ((size_t) w * h * sizeof *dst);
  double open[RUNS], again[RUNS], result[RUNS], press1[RUNS], press1000[RUNS], stroke_set[RUNS], stroke_tol[RUNS], stroke_ok[RUNS], stroke_one[RUNS];
  size_t bytes = 0;
  int automatic = 0;

  pattern (px, w, h);
  for (int i = 0; i < RUNS; i++)
    {
      double t0 = now_ms (), t1, t2;
      EaselRbg *r = easel_rbg_new (px, NULL, w, h, (size_t) -1);

      automatic = easel_rbg_auto_tolerance (r);
      easel_rbg_label (r, automatic, NULL);
      easel_rbg_preview (r, px, NULL, dst, NULL);
      t1 = now_ms ();
      easel_rbg_label (r, automatic + 5, NULL);
      easel_rbg_preview (r, px, NULL, dst, NULL);
      t2 = now_ms ();
      easel_rbg_result (r, px, NULL, dst, NULL, NULL);
      open[i] = t1 - t0;
      again[i] = t2 - t1;
      result[i] = now_ms () - t2 + again[i];   /* at OK: the labels for the number shown, then the result */
      {
        /* 0037 (REQ-0037-08): a preview with one press, and with 1 000 presses spread over the picture */
        EaselRbgPress *pr = malloc (1000 * sizeof *pr);
        double t3, t4;

        for (int k = 0; k < 1000; k++)
          pr[k] = (EaselRbgPress) { (int) ((uint32_t) (k * 2654435761u) % (uint32_t) w), (int) ((uint32_t) (k * 40503u) % (uint32_t) h) };
        t3 = now_ms ();
        easel_rbg_preview (r, px, NULL, dst, NULL);
        easel_rbg_flips (r, px, NULL, dst, pr, 1, NULL);
        t4 = now_ms ();
        press1[i] = t4 - t3;
        easel_rbg_label (r, automatic + 1, NULL);
        easel_rbg_preview (r, px, NULL, dst, NULL);
        easel_rbg_flips (r, px, NULL, dst, pr, 1000, NULL);
        press1000[i] = now_ms () - t4;
        free (pr);
      }
      {
        /* 0038 (REQ-0038-04, -08): 100 strokes of 1 000 places each spread over the picture, width 16; the strokes set and a preview
           (a stroke's end), then a Tolerance change with them, then the result at OK */
        int32_t *xy = malloc (100 * 1000 * 2 * sizeof *xy);
        EaselRbgStroke *st = malloc (100 * sizeof *st);
        double t5, t6, t7;

        for (int k = 0; k < 100; k++)
          {
            for (int j = 0; j < 1000; j++)
              {
                xy[(k * 1000 + j) * 2] = (int32_t) ((uint32_t) (k * 2654435761u + j * 3u) % (uint32_t) w);
                xy[(k * 1000 + j) * 2 + 1] = (int32_t) ((uint32_t) (k * 40503u) % (uint32_t) h);
              }
            st[k] = (EaselRbgStroke) { k % 2 == 0, 16, 1000, xy + k * 2000 };
          }
        t5 = now_ms ();
        easel_rbg_set_strokes (r, st, 1);
        easel_rbg_label (r, automatic, NULL);
        easel_rbg_preview (r, px, NULL, dst, NULL);
        stroke_one[i] = now_ms () - t5;
        t5 = now_ms ();
        easel_rbg_set_strokes (r, st, 100);
        easel_rbg_label (r, automatic, NULL);
        easel_rbg_preview (r, px, NULL, dst, NULL);
        t6 = now_ms ();
        easel_rbg_label (r, automatic + 2, NULL);
        easel_rbg_preview (r, px, NULL, dst, NULL);
        t7 = now_ms ();
        easel_rbg_result (r, px, NULL, dst, NULL, NULL);
        stroke_set[i] = t6 - t5;
        stroke_tol[i] = t7 - t6;
        stroke_ok[i] = now_ms () - t7;
        free (xy);
        free (st);
      }
      bytes = easel_rbg_bytes (r);
      easel_rbg_free (r);
    }
  printf ("%d x %d (automatic %d): opening with its preview %.0f %.0f %.0f ms, a new preview %.0f %.0f %.0f ms (%s < %.0f); "
          "the result %.0f %.0f %.0f ms (%s < %.0f); the analysis holds %zu MB\n",
          w, h, automatic, open[0], open[1], open[2], again[0], again[1], again[2],
          (open[0] < open_budget && again[0] < open_budget && again[1] < open_budget && again[2] < open_budget) ? "within" : "OVER",
          open_budget, result[0], result[1], result[2],
          (result[0] < result_budget && result[1] < result_budget && result[2] < result_budget) ? "within" : "OVER", result_budget,
          bytes >> 20);
  printf ("%d x %d: a preview with a press %.0f %.0f %.0f ms, after a Tolerance change with 1 000 presses %.0f %.0f %.0f ms (%s < 300)\n",
          w, h, press1[0], press1[1], press1[2], press1000[0], press1000[1], press1000[2],
          (press1[0] < 300 && press1[1] < 300 && press1[2] < 300 && press1000[0] < 300 && press1000[1] < 300 && press1000[2] < 300)
            ? "within" : "OVER");
  printf ("%d x %d: one stroke of 1 000 places set with its preview %.0f %.0f %.0f ms (%s < 300)\n", w, h, stroke_one[0], stroke_one[1],
          stroke_one[2], (stroke_one[0] < 300 && stroke_one[1] < 300 && stroke_one[2] < 300) ? "within" : "OVER");
  printf ("%d x %d: 100 strokes of 1 000 places set with their preview %.0f %.0f %.0f ms, a Tolerance change with them %.0f %.0f %.0f ms "
          "(%s < 300); the result with them %.0f %.0f %.0f ms\n", w, h, stroke_set[0], stroke_set[1], stroke_set[2], stroke_tol[0],
          stroke_tol[1], stroke_tol[2],
          (stroke_set[0] < 300 && stroke_set[1] < 300 && stroke_set[2] < 300 && stroke_tol[0] < 300 && stroke_tol[1] < 300 && stroke_tol[2] < 300)
            ? "within" : "OVER", stroke_ok[0], stroke_ok[1], stroke_ok[2]);
  free (px);
  free (dst);
}

int
main (void)
{
  measure (1920, 1080, 300, 1000);
  measure (8000, 8000, 300, 2000);
  return 0;
}
