/* SPDX-License-Identifier: GPL-3.0-or-later */
/* A classical way to find the subject (0035, JOD's step 2): a prototype, not in the app. Integers only, so the same input gives the
   same pixels on every machine.

     cc -O2 -std=c11 -o classical-mask classical-mask.c
     ./classical-mask in.ppm out.pgm          (binary P6 in, binary P5 out: 255 subject, 0 background)

   1. The background's colors: the pixels of a border ring (1 % of the shorter side, at least 2 pixels), each put in a bin of 4 bits a
      channel; up to three bins with at least 5 % of the ring, each standing for the mean of its pixels.
   2. Edge barriers: a pixel whose color gradient (the largest channel's |x+1 - x-1| + |y+1 - y-1|) is at least GRAD is a wall.
   3. Region growing: from every ring pixel near a background color, 4-connected, into a neighbor that is no wall, is near a
      background color (each channel within FAR) and near the pixel it comes from (each channel within STEP).
   4. Clean-up: subject pieces smaller than SPECK of the picture are dropped (made background); background pieces the growing did not
      reach are subject already (holes filled), whatever their size.
   5. A hard mask: subject or background, nothing between. */

#define _POSIX_C_SOURCE 200809L
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/resource.h>
#include <time.h>

/* The four settings, chosen on the five wallpapers of models-sheet.png (-D to try others). */
#ifndef GRAD
#define GRAD 30
#endif
#ifndef FAR
#define FAR 60
#endif
#ifndef STEP
#define STEP 8
#endif
#ifndef SPECK_PERMILLE
#define SPECK_PERMILLE 2                  /* 0.2 % of the picture */
#endif

static int
read_ppm (const char *path, int *w, int *h, uint8_t **rgb)
{
  FILE *f = fopen (path, "rb");
  int maxv;

  if (f == NULL || fscanf (f, "P6 %d %d %d", w, h, &maxv) != 3 || maxv != 255 || fgetc (f) == EOF)
    return 0;
  *rgb = malloc ((size_t) *w * *h * 3);
  if (*rgb == NULL || fread (*rgb, 3, (size_t) *w * *h, f) != (size_t) *w * *h)
    return 0;
  fclose (f);
  return 1;
}

static int
near_color (const uint8_t *p, const int (*bg)[3], int nbg, int tol)
{
  for (int k = 0; k < nbg; k++)
    if (abs (p[0] - bg[k][0]) <= tol && abs (p[1] - bg[k][1]) <= tol && abs (p[2] - bg[k][2]) <= tol)
      return 1;
  return 0;
}

static int
near_pixel (const uint8_t *a, const uint8_t *b)
{
  return abs (a[0] - b[0]) <= STEP && abs (a[1] - b[1]) <= STEP && abs (a[2] - b[2]) <= STEP;
}

int
main (int argc, char **argv)
{
  int w, h, ring, nbg = 0, bg[3][3];
  uint8_t *rgb, *wall, *mask;
  uint32_t *queue;
  size_t n, head = 0, tail = 0, ring_n = 0;
  static uint32_t count[4096];
  static uint64_t sum[4096][3];
  struct timespec t0, t1;
  struct rusage ru;

  if (argc != 3 || !read_ppm (argv[1], &w, &h, &rgb))
    {
      fprintf (stderr, "usage: classical-mask in.ppm out.pgm\n");
      return 2;
    }
  clock_gettime (CLOCK_MONOTONIC, &t0);
  n = (size_t) w * h;
  ring = (w < h ? w : h) / 100;
  if (ring < 2)
    ring = 2;

  /* 1. The ring's colors. */
  for (int y = 0; y < h; y++)
    for (int x = 0; x < w; x++)
      if (x < ring || y < ring || x >= w - ring || y >= h - ring)
        {
          const uint8_t *p = rgb + ((size_t) y * w + x) * 3;
          int bin = (p[0] >> 4) << 8 | (p[1] >> 4) << 4 | p[2] >> 4;

          count[bin]++;
          sum[bin][0] += p[0], sum[bin][1] += p[1], sum[bin][2] += p[2];
          ring_n++;
        }
  for (int k = 0; k < 3; k++)
    {
      int best = -1;

      for (int b = 0; b < 4096; b++)
        if (count[b] * 20 >= ring_n && (best < 0 || count[b] > count[best]))
          best = b;
      if (best < 0)
        break;
      for (int c = 0; c < 3; c++)
        bg[nbg][c] = (int) (sum[best][c] / count[best]);
      nbg++;
      count[best] = 0;
    }

  /* 2. The walls. */
  wall = calloc (n, 1);
  mask = malloc (n);
  queue = malloc (n * sizeof *queue);
  if (wall == NULL || mask == NULL || queue == NULL)
    return 3;
  for (int y = 1; y < h - 1; y++)
    for (int x = 1; x < w - 1; x++)
      {
        const uint8_t *p = rgb + ((size_t) y * w + x) * 3;
        int g = 0;

        for (int c = 0; c < 3; c++)
          {
            int d = abs (p[3 + c] - p[c - 3]) + abs (p[(size_t) w * 3 + c] - p[c - (ptrdiff_t) w * 3]);

            if (d > g)
              g = d;
          }
        wall[(size_t) y * w + x] = g >= GRAD;
      }

  /* 3. Growing from the ring: mask 0 background, 255 not reached (subject). */
  memset (mask, 255, n);
  for (int y = 0; y < h; y++)
    for (int x = 0; x < w; x++)
      if (x < ring || y < ring || x >= w - ring || y >= h - ring)
        {
          size_t i = (size_t) y * w + x;

          if (!wall[i] && near_color (rgb + i * 3, (const int (*)[3]) bg, nbg, FAR) && mask[i] != 0)
            {
              mask[i] = 0;
              queue[tail++] = (uint32_t) i;
            }
        }
  while (head < tail)
    {
      size_t i = queue[head++];
      int x = (int) (i % w), y = (int) (i / w);
      size_t nb[4];
      int k = 0;

      if (x > 0) nb[k++] = i - 1;
      if (x < w - 1) nb[k++] = i + 1;
      if (y > 0) nb[k++] = i - w;
      if (y < h - 1) nb[k++] = i + w;
      for (int j = 0; j < k; j++)
        {
          size_t m = nb[j];

          if (mask[m] != 0 && !wall[m] && near_color (rgb + m * 3, (const int (*)[3]) bg, nbg, FAR)
              && near_pixel (rgb + m * 3, rgb + i * 3))
            {
              mask[m] = 0;
              queue[tail++] = (uint32_t) m;
            }
        }
    }

  /* 4. Specks: subject pieces under SPECK of the picture become background (wall reused as "seen"). */
  memset (wall, 0, n);
  for (size_t s = 0; s < n; s++)
    {
      size_t piece;

      if (mask[s] == 0 || wall[s])
        continue;
      head = tail = 0;
      queue[tail++] = (uint32_t) s;
      wall[s] = 1;
      while (head < tail)
        {
          size_t i = queue[head++];
          int x = (int) (i % w), y = (int) (i / w);
          size_t nb[4];
          int k = 0;

          if (x > 0) nb[k++] = i - 1;
          if (x < w - 1) nb[k++] = i + 1;
          if (y > 0) nb[k++] = i - w;
          if (y < h - 1) nb[k++] = i + w;
          for (int j = 0; j < k; j++)
            if (mask[nb[j]] != 0 && !wall[nb[j]])
              {
                wall[nb[j]] = 1;
                queue[tail++] = (uint32_t) nb[j];
              }
        }
      piece = tail;
      if (piece * 1000 < n * SPECK_PERMILLE)
        for (size_t q = 0; q < piece; q++)
          mask[queue[q]] = 0;
    }
  clock_gettime (CLOCK_MONOTONIC, &t1);

  {
    FILE *f = fopen (argv[2], "wb");

    if (f == NULL)
      return 4;
    fprintf (f, "P5 %d %d 255\n", w, h);
    fwrite (mask, 1, n, f);
    fclose (f);
  }
  getrusage (RUSAGE_SELF, &ru);
  fprintf (stderr, "%d x %d: %d background colors, %.0f ms, peak %ld MB\n", w, h, nbg,
           (t1.tv_sec - t0.tv_sec) * 1000.0 + (t1.tv_nsec - t0.tv_nsec) / 1e6, ru.ru_maxrss / 1024);
  return 0;
}
