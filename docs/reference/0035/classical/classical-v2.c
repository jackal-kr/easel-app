/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Remove background, the classical method improved (0035, JOD's step after ADR-0007): a prototype, not in the app. libc and integers
   only, so the same input gives the same pixels on every machine. Four stages, each one adding to the one before:

     1  the first prototype's rules (border colors, walls, growing, specks, holes) with colors compared in Lab (an integer
        approximation of CIE Lab, D65) instead of RGB;
     2  the background as a smooth surface: per Lab channel a quadratic in x and y fitted by least squares (fixed point) to the border
        ring, refitted once without the samples far from the first fit; a pixel is compared with the surface where it lies;
     3  a marker-based watershed: background seeds in the border ring near the surface, subject seeds anywhere far from it (3 times the
        tolerance), the rest flooded in order of edge strength (the Lab gradient), a bucket queue first in first out, so that edges
        decide the boundary;
     4  the largest subject region kept, with any other at least a tenth of its size (two subjects side by side), and enclosed background
        pieces under 1 % of the picture filled.

     cc -O2 -std=c11 -o classical-v2 classical-v2.c
     ./classical-v2 in.ppm out.pgm STAGE TOLERANCE      (binary P6 in, P5 out: 255 subject, 0 background; TOLERANCE in delta-E)

   Not used: GrabCut or any iterated graph cut with color models (JOD, 2026-11-09: a patent is believed to cover it, US 7,660,463,
   its status unchecked). */

#define _POSIX_C_SOURCE 200809L
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/resource.h>
#include <time.h>

typedef __int128 i128;

/* sRGB to linear light, Q16 (generated once from the sRGB formula and kept as integers). */
static const int32_t LIN[256] = { 0, 20, 40, 60, 80, 99, 119, 139, 159, 179, 199, 219, 241, 264, 288, 313, 340, 367, 396, 427, 458, 491, 526, 562, 599, 637, 677, 718, 761, 805, 851, 898, 947, 997, 1048, 1101, 1156, 1212, 1270, 1330, 1391, 1453, 1517, 1583, 1651, 1720, 1790, 1863, 1937, 2013, 2090, 2170, 2250, 2333, 2418, 2504, 2592, 2681, 2773, 2866, 2961, 3058, 3157, 3258, 3360, 3464, 3570, 3678, 3788, 3900, 4014, 4129, 4247, 4366, 4488, 4611, 4736, 4864, 4993, 5124, 5257, 5392, 5530, 5669, 5810, 5953, 6099, 6246, 6395, 6547, 6700, 6856, 7014, 7174, 7335, 7500, 7666, 7834, 8004, 8177, 8352, 8528, 8708, 8889, 9072, 9258, 9445, 9635, 9828, 10022, 10219, 10417, 10619, 10822, 11028, 11235, 11446, 11658, 11873, 12090, 12309, 12530, 12754, 12980, 13209, 13440, 13673, 13909, 14146, 14387, 14629, 14874, 15122, 15371, 15623, 15878, 16135, 16394, 16656, 16920, 17187, 17456, 17727, 18001, 18277, 18556, 18837, 19121, 19407, 19696, 19987, 20281, 20577, 20876, 21177, 21481, 21787, 22096, 22407, 22721, 23038, 23357, 23678, 24002, 24329, 24658, 24990, 25325, 25662, 26001, 26344, 26688, 27036, 27386, 27739, 28094, 28452, 28813, 29176, 29542, 29911, 30282, 30656, 31033, 31412, 31794, 32179, 32567, 32957, 33350, 33745, 34143, 34544, 34948, 35355, 35764, 36176, 36591, 37008, 37429, 37852, 38278, 38706, 39138, 39572, 40009, 40449, 40891, 41337, 41785, 42236, 42690, 43147, 43606, 44069, 44534, 45002, 45473, 45947, 46423, 46903, 47385, 47871, 48359, 48850, 49344, 49841, 50341, 50844, 51349, 51858, 52369, 52884, 53401, 53921, 54445, 54971, 55500, 56032, 56567, 57105, 57646, 58190, 58737, 59287, 59840, 60396, 60955, 61517, 62082, 62650, 63221, 63795, 64372, 64952, 65535 };
/* XYZ from linear RGB, each row divided by the D65 white's (Xn, 1, Zn), Q16. */
static const int32_t M[3][3] = { { 28439, 24655, 12441 }, { 13938, 46868, 4730 }, { 1164, 7174, 57198 } };

#ifndef SEED_FAR
#define SEED_FAR 4                     /* a subject seed is this many tolerances from the surface */
#endif
#ifndef ORDER
#define ORDER 3                        /* the surface's order: 2 (6 terms) or 3 (10 terms) */
#endif
#define NB (ORDER == 3 ? 10 : 6)
enum { WALL = 30, STEP = 4, BUCKETS = 1024 };

static int W, H;
static int16_t *Lab;                   /* L, a, b per pixel, each times 64 */
static int32_t *FT;                    /* f(t) of CIE Lab for t = 0 .. 65536 (Q16), integer cube roots */

static uint64_t
icbrt (uint64_t v)
{
  uint64_t r = 0;

  for (int s = 63; s >= 0; s -= 3)
    {
      uint64_t b = 3 * r * (2 * r + 1) + 1;

      r <<= 1;
      if ((v >> s) >= b)
        {
          v -= b << s;
          r++;
        }
    }
  return r;
}

static void
make_ft (void)
{
  FT = malloc (65537 * sizeof *FT);
  for (int t = 0; t <= 65536; t++)
    FT[t] = t > 580 ? (int32_t) icbrt ((uint64_t) t << 32) : (int32_t) ((int64_t) t * 841 / 108 + 4 * 65536 / 29);
}

static int32_t
f_of (int64_t t)
{
  return FT[t < 0 ? 0 : t > 65536 ? 65536 : t];
}

static void
to_lab (const uint8_t *p, int16_t *out)
{
  int64_t r = LIN[p[0]], g = LIN[p[1]], b = LIN[p[2]];
  int32_t fx = f_of ((M[0][0] * r + M[0][1] * g + M[0][2] * b) >> 16);
  int32_t fy = f_of ((M[1][0] * r + M[1][1] * g + M[1][2] * b) >> 16);
  int32_t fz = f_of ((M[2][0] * r + M[2][1] * g + M[2][2] * b) >> 16);

  out[0] = (int16_t) ((116 * (int64_t) fy - 16 * 65536) >> 10);
  out[1] = (int16_t) ((500 * (int64_t) (fx - fy)) >> 10);
  out[2] = (int16_t) ((200 * (int64_t) (fy - fz)) >> 10);
}

/* Squared distance of two Lab colors, in (delta-E times 64) squared. */
static int64_t
de2 (const int16_t *a, const int16_t *b)
{
  int64_t d0 = a[0] - b[0], d1 = a[1] - b[1], d2 = a[2] - b[2];

  return d0 * d0 + d1 * d1 + d2 * d2;
}

/* ---- the background surface: per channel c(x, y) = sum k_j phi_j, phi = 1, u, v, u^2, uv, v^2, u and v in -1 .. 1 (Q8) ---- */

static int64_t K[3][10];               /* coefficients, value * 2^24 per Q8 basis */

static void
basis (int x, int y, int64_t phi[10])
{
  int64_t u = W > 1 ? ((int64_t) x * 512 / (W - 1)) - 256 : 0, v = H > 1 ? ((int64_t) y * 512 / (H - 1)) - 256 : 0;

  phi[0] = 256, phi[1] = u, phi[2] = v, phi[3] = u * u >> 8, phi[4] = u * v >> 8, phi[5] = v * v >> 8;
  phi[6] = phi[3] * u >> 8, phi[7] = phi[3] * v >> 8, phi[8] = phi[5] * u >> 8, phi[9] = phi[5] * v >> 8;
}

static void
surface_at (int x, int y, int16_t out[3])
{
  int64_t phi[10];

  basis (x, y, phi);
  for (int c = 0; c < 3; c++)
    {
      int64_t s = 0;

      for (int j = 0; j < NB; j++)
        s += K[c][j] * phi[j];
      s >>= 24;
      out[c] = (int16_t) (s < -32768 ? -32768 : s > 32767 ? 32767 : s);
    }
}

static int
band (void)
{
  int b = (W < H ? W : H) / 100;

  return b < 2 ? 2 : b;
}

static int
in_band (int x, int y)
{
  int b = band ();

  return x < b || y < b || x >= W - b || y >= H - b;
}

/* A least-squares fit over the band's pixels (a sample every 'stride' along it), leaving out those farther than limit2 from the last
   fit when 'refit'. Gaussian elimination in 128-bit integers, the pivot the largest in its column (ties: the first). */
static void
fit (int refit, int64_t limit2)
{
  int64_t A[10][10] = { { 0 } }, R[3][10] = { { 0 } }, n = 0;
  int64_t phi[10];
  size_t ring = 2 * ((size_t) W + H) * band ();
  int stride = (int) (ring / 8192) + 1, k = 0;

  for (int y = 0; y < H; y++)
    for (int x = 0; x < W; x++)
      {
        int16_t s[3];
        const int16_t *p = Lab + 3 * ((size_t) y * W + x);

        if (!in_band (x, y) || (k++ % stride) != 0)
          continue;
        if (refit)
          {
            surface_at (x, y, s);
            if (de2 (p, s) > limit2)
              continue;
          }
        basis (x, y, phi);
        for (int i = 0; i < NB; i++)
          {
            for (int j = 0; j < NB; j++)
              A[i][j] += phi[i] * phi[j];
            for (int c = 0; c < 3; c++)
              R[c][i] += phi[i] * p[c];
          }
        n++;
      }
  if (n < NB)
    return;
  for (int c = 0; c < 3; c++)
    {
      i128 m[10][11];

      for (int i = 0; i < NB; i++)
        {
          for (int j = 0; j < NB; j++)
            m[i][j] = A[i][j] / n;
          m[i][NB] = ((i128) R[c][i] << 24) / n;
        }
      for (int col = 0; col < NB; col++)
        {
          int piv = col;

          for (int i = col + 1; i < NB; i++)
            if ((m[i][col] < 0 ? -m[i][col] : m[i][col]) > (m[piv][col] < 0 ? -m[piv][col] : m[piv][col]))
              piv = i;
          if (piv != col)
            for (int j = 0; j <= NB; j++)
              {
                i128 t = m[col][j];

                m[col][j] = m[piv][j];
                m[piv][j] = t;
              }
          if (m[col][col] == 0)
            continue;
          for (int i = col + 1; i < NB; i++)
            {
              i128 a = m[i][col], d = m[col][col];

              for (int j = col; j <= NB; j++)
                m[i][j] = m[i][j] - m[col][j] * a / d;
            }
        }
      for (int i = NB - 1; i >= 0; i--)
        {
          i128 s = m[i][NB];

          for (int j = i + 1; j < NB; j++)
            s -= m[i][j] * (i128) K[c][j];
          K[c][i] = m[i][i] != 0 ? (int64_t) (s / m[i][i]) : 0;
        }
    }
}

/* ---- the edge strength: the Lab gradient, |right - left| + |below - above| summed over L, a, b, in delta-E, capped ---- */

static int
edge (size_t i, int x, int y)
{
  int64_t g = 0;

  if (x == 0 || y == 0 || x == W - 1 || y == H - 1)
    return 0;
  for (int c = 0; c < 3; c++)
    {
      int64_t dx = Lab[3 * (i + 1) + c] - Lab[3 * (i - 1) + c], dy = Lab[3 * (i + W) + c] - Lab[3 * (i - W) + c];

      g += (dx < 0 ? -dx : dx) + (dy < 0 ? -dy : dy);
    }
  g >>= 6;
  return g >= BUCKETS ? BUCKETS - 1 : (int) g;
}

int
main (int argc, char **argv)
{
  FILE *f;
  int maxv, stage, tol;
  uint8_t *rgb, *lab_flag, *out;
  size_t n;
  struct timespec t0, t1;
  struct rusage ru;

  if (argc != 5 || (f = fopen (argv[1], "rb")) == NULL || fscanf (f, "P6 %d %d %d", &W, &H, &maxv) != 3 || fgetc (f) == EOF)
    {
      fprintf (stderr, "usage: classical-v2 in.ppm out.pgm STAGE TOLERANCE\n");
      return 2;
    }
  stage = atoi (argv[3]);
  tol = atoi (argv[4]);
  n = (size_t) W * H;
  rgb = malloc (n * 3);
  if (rgb == NULL || fread (rgb, 3, n, f) != n)
    return 3;
  fclose (f);
  clock_gettime (CLOCK_MONOTONIC, &t0);
  make_ft ();
  Lab = malloc (n * 3 * sizeof *Lab);
  out = malloc (n);
  lab_flag = NULL;
  if (Lab == NULL || out == NULL)
    return 3;
  for (size_t i = 0; i < n; i++)
    to_lab (rgb + 3 * i, Lab + 3 * i);
  free (rgb);
  {
    int64_t t2 = (int64_t) tol * 64 * tol * 64, step2 = (int64_t) STEP * 64 * STEP * 64;
    int16_t flat[3][3];
    int nflat = 0;

    if (stage >= 2)
      {
        fit (0, 0);
        fit (1, 4 * t2);
      }
    else
      {
        /* stage 1: up to three flat colors from the band, binned in Lab (16 levels a channel), as the first prototype in RGB */
        static uint32_t cnt[4096];
        static int64_t sum[4096][3];
        uint64_t tot = 0;

        for (int y = 0; y < H; y++)
          for (int x = 0; x < W; x++)
            if (in_band (x, y))
              {
                const int16_t *p = Lab + 3 * ((size_t) y * W + x);
                int b = ((p[0] * 16 / 6401) & 15) << 8 | (((p[1] + 8192) * 16 / 16385) & 15) << 4 | (((p[2] + 8192) * 16 / 16385) & 15);

                cnt[b]++;
                sum[b][0] += p[0], sum[b][1] += p[1], sum[b][2] += p[2];
                tot++;
              }
        while (nflat < 3)
          {
            int best = -1;

            for (int b = 0; b < 4096; b++)
              if (cnt[b] > 0 && (uint64_t) cnt[b] * 20 >= tot && (best < 0 || cnt[b] > cnt[best]))
                best = b;
            if (best < 0)
              break;
            for (int c = 0; c < 3; c++)
              flat[nflat][c] = (int16_t) (sum[best][c] / (int64_t) cnt[best]);
            nflat++;
            cnt[best] = 0;
          }
      }

    if (stage <= 2)
      {
        /* growing from the band (the first prototype's rules), in Lab: near the background within tol, from pixel to pixel within STEP,
           no wall (edge strength WALL or more) */
        uint32_t *queue = malloc (n * sizeof *queue);
        size_t head = 0, tail = 0;

        memset (out, 255, n);
        for (int y = 0; y < H; y++)
          for (int x = 0; x < W; x++)
            {
              size_t i = (size_t) y * W + x;
              int16_t s[3];
              int nearbg = 0;

              if (!in_band (x, y) || edge (i, x, y) >= WALL)
                continue;
              if (stage == 1)
                for (int k = 0; k < nflat && !nearbg; k++)
                  nearbg = de2 (Lab + 3 * i, flat[k]) <= t2;
              else
                {
                  surface_at (x, y, s);
                  nearbg = de2 (Lab + 3 * i, s) <= t2;
                }
              if (nearbg)
                {
                  out[i] = 0;
                  queue[tail++] = (uint32_t) i;
                }
            }
        while (head < tail)
          {
            size_t i = queue[head++];
            int x = (int) (i % W), y = (int) (i / W);
            size_t nb[4];
            int k = 0;

            if (x > 0) nb[k++] = i - 1;
            if (x < W - 1) nb[k++] = i + 1;
            if (y > 0) nb[k++] = i - W;
            if (y < H - 1) nb[k++] = i + W;
            for (int j = 0; j < k; j++)
              {
                size_t m = nb[j];
                int mx = (int) (m % W), my = (int) (m / W), nearbg = 0;
                int16_t s[3];

                if (out[m] == 0 || edge (m, mx, my) >= WALL || de2 (Lab + 3 * m, Lab + 3 * i) > step2)
                  continue;
                if (stage == 1)
                  for (int q = 0; q < nflat && !nearbg; q++)
                    nearbg = de2 (Lab + 3 * m, flat[q]) <= t2;
                else
                  {
                    surface_at (mx, my, s);
                    nearbg = de2 (Lab + 3 * m, s) <= t2;
                  }
                if (nearbg)
                  {
                    out[m] = 0;
                    queue[tail++] = (uint32_t) m;
                  }
              }
          }
        free (queue);
      }
    else
      {
        /* stage 3: the watershed. label: 0 none, 1 background, 2 subject. A pixel is labeled when it is queued, with the label of the
           pixel that queued it; the queue is a bucket per edge strength, first in first out, never below the level being drained. */
        int32_t *next = malloc (n * sizeof *next), headb[BUCKETS], tailb[BUCKETS];
        int level = 0;

        for (int b = 0; b < BUCKETS; b++)
          headb[b] = tailb[b] = -1;
        memset (out, 0, n);
#define PUSH(idx, pr) do { int pr_ = (pr) < level ? level : (pr); next[idx] = -1; \
          if (tailb[pr_] < 0) headb[pr_] = (int32_t) (idx); else next[tailb[pr_]] = (int32_t) (idx); tailb[pr_] = (int32_t) (idx); } while (0)
        for (int y = 0; y < H; y++)
          for (int x = 0; x < W; x++)
            {
              size_t i = (size_t) y * W + x;
              int16_t s[3];
              int64_t d;

              surface_at (x, y, s);
              d = de2 (Lab + 3 * i, s);
              if (in_band (x, y) && d <= t2)
                out[i] = 1;
              else if (d >= (int64_t) SEED_FAR * SEED_FAR * t2)
                out[i] = 2;
              if (out[i] != 0)
                PUSH (i, 0);
            }
        for (level = 0; level < BUCKETS; level++)
          while (headb[level] >= 0)
            {
              size_t i = (size_t) headb[level];
              int x = (int) (i % W), y = (int) (i / W);
              size_t nb[4];
              int k = 0;

              headb[level] = next[i];
              if (headb[level] < 0)
                tailb[level] = -1;
              if (x > 0) nb[k++] = i - 1;
              if (x < W - 1) nb[k++] = i + 1;
              if (y > 0) nb[k++] = i - W;
              if (y < H - 1) nb[k++] = i + W;
              for (int j = 0; j < k; j++)
                if (out[nb[j]] == 0)
                  {
                    out[nb[j]] = out[i];
                    PUSH (nb[j], edge (nb[j], (int) (nb[j] % W), (int) (nb[j] / W)));
                  }
            }
        for (size_t i = 0; i < n; i++)
          out[i] = out[i] == 2 ? 255 : 0;          /* a pixel no seed reached (none at all) is background */

        if (stage >= 4)
          {
            /* the largest subject region, and others at least a tenth of it; then enclosed background under 1 % of the picture */
            int32_t *comp = (int32_t *) next;          /* reused: the component number of each pixel, -1 none */
            uint32_t *queue = malloc (n * sizeof *queue);
            size_t *sizes = NULL, ncomp = 0, largest = 0;

            for (int pass = 0; pass < 2; pass++)
              {
                uint8_t want = pass == 0 ? 255 : 0;

                for (size_t i = 0; i < n; i++)
                  comp[i] = -1;
                ncomp = 0;
                free (sizes);
                sizes = NULL;
                for (size_t s0 = 0; s0 < n; s0++)
                  {
                    size_t head = 0, tail = 0;
                    int touches = 0;

                    if (out[s0] != want || comp[s0] >= 0)
                      continue;
                    sizes = realloc (sizes, (ncomp + 1) * sizeof *sizes);
                    comp[s0] = (int32_t) ncomp;
                    queue[tail++] = (uint32_t) s0;
                    while (head < tail)
                      {
                        size_t i = queue[head++];
                        int x = (int) (i % W), y = (int) (i / W);
                        size_t nb[4];
                        int k = 0;

                        touches |= x == 0 || y == 0 || x == W - 1 || y == H - 1;
                        if (x > 0) nb[k++] = i - 1;
                        if (x < W - 1) nb[k++] = i + 1;
                        if (y > 0) nb[k++] = i - W;
                        if (y < H - 1) nb[k++] = i + W;
                        for (int j = 0; j < k; j++)
                          if (out[nb[j]] == want && comp[nb[j]] < 0)
                            {
                              comp[nb[j]] = (int32_t) ncomp;
                              queue[tail++] = (uint32_t) nb[j];
                            }
                      }
                    sizes[ncomp] = pass == 0 ? tail : (touches ? 0 : tail);     /* background touching the edge is no hole */
                    ncomp++;
                  }
                if (pass == 0)
                  {
                    for (size_t c = 0; c < ncomp; c++)
                      if (sizes[c] > largest)
                        largest = sizes[c];
                    for (size_t i = 0; i < n; i++)
                      if (out[i] == 255 && sizes[comp[i]] * 10 < largest)
                        out[i] = 0;
                  }
                else
                  for (size_t i = 0; i < n; i++)
                    if (out[i] == 0 && sizes[comp[i]] > 0 && sizes[comp[i]] * 100 < n)
                      out[i] = 255;
              }
            free (sizes);
            free (queue);
          }
        free (next);
      }
    if (stage <= 2)
      {
        /* the first prototype's clean-up: subject pieces under 0.2 % dropped (holes are filled by the growing's nature) */
        uint8_t *seen = calloc (n, 1);
        uint32_t *queue = malloc (n * sizeof *queue);

        for (size_t s0 = 0; s0 < n; s0++)
          {
            size_t head = 0, tail = 0;

            if (out[s0] == 0 || seen[s0])
              continue;
            seen[s0] = 1;
            queue[tail++] = (uint32_t) s0;
            while (head < tail)
              {
                size_t i = queue[head++];
                int x = (int) (i % W), y = (int) (i / W);
                size_t nb[4];
                int k = 0;

                if (x > 0) nb[k++] = i - 1;
                if (x < W - 1) nb[k++] = i + 1;
                if (y > 0) nb[k++] = i - W;
                if (y < H - 1) nb[k++] = i + W;
                for (int j = 0; j < k; j++)
                  if (out[nb[j]] != 0 && !seen[nb[j]])
                    {
                      seen[nb[j]] = 1;
                      queue[tail++] = (uint32_t) nb[j];
                    }
              }
            if (tail * 500 < n)
              for (size_t q = 0; q < tail; q++)
                out[queue[q]] = 0;
          }
        free (seen);
        free (queue);
      }
  }
  clock_gettime (CLOCK_MONOTONIC, &t1);
  f = fopen (argv[2], "wb");
  if (f == NULL)
    return 4;
  fprintf (f, "P5 %d %d 255\n", W, H);
  fwrite (out, 1, n, f);
  fclose (f);
  getrusage (RUSAGE_SELF, &ru);
  fprintf (stderr, "%d x %d stage %d tolerance %d: %.0f ms, peak %ld MB\n", W, H, stage, tol,
           (t1.tv_sec - t0.tv_sec) * 1000.0 + (t1.tv_nsec - t0.tv_nsec) / 1e6, ru.ru_maxrss / 1024);
  (void) lab_flag;
  return 0;
}
