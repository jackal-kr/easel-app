/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "curve-bend.h"

#include <math.h>
#include <stdlib.h>

#define COORD_LIMIT 1000000
#define T EASEL_CURVE_STEPS            /* 4096 = 2^12, so T^3 = 2^36 = EASEL_CURVE_ONE */

typedef __int128 i128;

static int
clamp_coord (int v)
{
  return v < -COORD_LIMIT ? -COORD_LIMIT : v > COORD_LIMIT ? COORD_LIMIT : v;
}

/* REQ-0011-05's N: the length of the path start, C1, C2, end, rounded up, at least 1 (as the curve's own pixels count it). */
int
easel_curve_grid (const int pts[8], bool straight)
{
  double len = 0;
  long long n;
  int m = 64;

  if (straight)
    len = hypot ((double) clamp_coord (pts[6]) - clamp_coord (pts[0]), (double) clamp_coord (pts[7]) - clamp_coord (pts[1]));
  else
    for (int k = 0; k < 3; k++)
      {
        double dx = (double) (clamp_coord (pts[2 * k + 2]) - clamp_coord (pts[2 * k]));
        double dy = (double) (clamp_coord (pts[2 * k + 3]) - clamp_coord (pts[2 * k + 1]));

        len += sqrt (dx * dx + dy * dy);
      }
  n = (long long) ceil (len);
  while (m < n && m < T)
    m *= 2;
  return m;
}

/* The Bernstein weights at t / T, over T^3: (T - t)^3, 3 (T - t)^2 t, 3 (T - t) t^2, t^3; each at most 2^36. */
static void
weights (int t, int64_t w[4])
{
  int64_t u = T - t, v = t;

  w[0] = u * u * u;
  w[1] = 3 * u * u * v;
  w[2] = 3 * u * v * v;
  w[3] = v * v * v;
}

/* B (t / T) over T^3 = EASEL_CURVE_ONE: the coordinates are at most 10^6 < 2^20 and the weights sum to 2^36, so 2^56. */
void
easel_curve_point (const int pts[8], bool straight, int t, int64_t *x, int64_t *y)
{
  int p[8];
  int64_t w[4];

  for (int k = 0; k < 8; k++)
    p[k] = clamp_coord (pts[k]);
  if (straight)
    {
      int64_t step = EASEL_CURVE_ONE / T;                    /* 2^24 */

      *x = (int64_t) p[0] * EASEL_CURVE_ONE + (int64_t) t * (p[6] - p[0]) * step;
      *y = (int64_t) p[1] * EASEL_CURVE_ONE + (int64_t) t * (p[7] - p[1]) * step;
      return;
    }
  weights (t, w);
  *x = w[0] * p[0] + w[1] * p[2] + w[2] * p[4] + w[3] * p[6];
  *y = w[0] * p[1] + w[1] * p[3] + w[2] * p[5] + w[3] * p[7];
}

int
easel_curve_grab (const int pts[8], bool straight, int px, int py)
{
  int m = easel_curve_grid (pts, straight), best = 0;
  unsigned __int128 best_d = 0;
  int64_t cx = (int64_t) clamp_coord (px) * EASEL_CURVE_ONE, cy = (int64_t) clamp_coord (py) * EASEL_CURVE_ONE;

  for (int i = 0; i <= m; i++)
    {
      int64_t x, y;
      i128 dx, dy;
      unsigned __int128 d;

      easel_curve_point (pts, straight, i * (T / m), &x, &y);
      dx = (i128) x - cx;
      dy = (i128) y - cy;
      d = (unsigned __int128) (dx * dx) + (unsigned __int128) (dy * dy);       /* each below 2^116 */
      if (i == 0 || d < best_d)
        {
          best_d = d;
          best = i * (T / m);
        }
    }
  return best < T / 8 ? T / 8 : best > T - T / 8 ? T - T / 8 : best;
}

/* floor (num / den) for den > 0 */
static i128
floor_div (i128 num, i128 den)
{
  i128 q = num / den;

  return (num % den != 0 && num < 0) ? q - 1 : q;
}

/* The place: round (4096 A / (L ONE)), A the dot product of P - S and E - S (P over ONE), L the square of the line's length;
   A below 2^79 and L ONE below 2^80, within 128 bits. */
int
easel_curve_place (int sx, int sy, int ex, int ey, const EaselCurveBend *b)
{
  int64_t dx = (int64_t) clamp_coord (ex) - clamp_coord (sx), dy = (int64_t) clamp_coord (ey) - clamp_coord (sy);
  i128 length2 = (i128) dx * dx + (i128) dy * dy, a, t;

  if (length2 == 0)
    return T / 2;
  a = ((i128) b->x - (i128) clamp_coord (sx) * EASEL_CURVE_ONE) * dx + ((i128) b->y - (i128) clamp_coord (sy) * EASEL_CURVE_ONE) * dy;
  t = floor_div (2 * T * a + length2 * EASEL_CURVE_ONE, 2 * length2 * EASEL_CURVE_ONE);
  return t < T / 8 ? T / 8 : t > T - T / 8 ? T - T / 8 : (int) t;
}

void
easel_curve_places (int sx, int sy, int ex, int ey, const EaselCurveBend *b, int n, int t[2])
{
  t[0] = easel_curve_place (sx, sy, ex, ey, &b[0]);
  t[1] = n > 1 ? easel_curve_place (sx, sy, ex, ey, &b[1]) : t[0];
  if (n > 1 && t[1] - t[0] < T / 8)
    {
      int m = (t[0] + t[1]) / 2;

      m = m < 3 * T / 16 ? 3 * T / 16 : m > 13 * T / 16 ? 13 * T / 16 : m;
      t[0] = m - T / 16;
      t[1] = m + T / 16;
    }
}

void
easel_curve_order (int sx, int sy, int ex, int ey, EaselCurveBend *b, int n)
{
  if (n == 2 && easel_curve_place (sx, sy, ex, ey, &b[1]) < easel_curve_place (sx, sy, ex, ey, &b[0]))
    {
      EaselCurveBend k = b[0];

      b[0] = b[1];
      b[1] = k;
    }
}

EaselCurveGrab
easel_curve_bend_rule (int sx, int sy, int ex, int ey, const EaselCurveBend *b, int n, int t, int *which)
{
  int p[2];

  if (n <= 0)
    {
      *which = 0;
      return EASEL_CURVE_NEW;
    }
  easel_curve_places (sx, sy, ex, ey, b, n, p);
  if (n == 1)
    {
      *which = abs (t - p[0]) < T / 8 ? 0 : 1;
      return *which == 0 ? EASEL_CURVE_MOVE : EASEL_CURVE_NEW;
    }
  *which = abs (t - p[0]) <= abs (t - p[1]) ? 0 : 1;
  return abs (t - p[*which]) < T / 8 ? EASEL_CURVE_MOVE : EASEL_CURVE_REPLACE;
}

/* round (num / den), a half toward larger, then limited to the coordinate range. */
static int
round_limited (i128 num, i128 den)
{
  i128 q, a, b;

  if (den < 0)
    {
      num = -num;
      den = -den;
    }
  a = 2 * num + den;
  b = 2 * den;
  q = a / b;
  if (a % b != 0 && a < 0)
    q--;                                         /* floor, not truncation */
  return q < -COORD_LIMIT ? -COORD_LIMIT : q > COORD_LIMIT ? COORD_LIMIT : (int) q;
}

/* One bend P, the peak (REQ-0033-05): with d = E - S, n = (-dy, dx), D = L ONE, A = (P - S) . d and G = (P - S) . n (over ONE), f =
   A / D and g = G / D; C1 = S + u d + (4g / 3) n, C2 = S + v d + (4g / 3) n, over the common denominator 3D: U = 0 and V = 8A - D
   when 2A <= D, else U = 8A - 4D and V = 3D, and K = 4G. D is below 2^80, A and G below 2^79, every numerator below 2^106. S = E:
   C = (4P - S) / 3. Two bends: B (t1) = P1 and B (t2) = P2 at their places, w0 S + w1 C1 + w2 C2 + w3 E = P with the weights over
   T^3 = 2^36 = ONE, by Cramer's rule: the right sides below 2^64, the weights below 2^37, the determinant below 2^75 and its
   numerators below 2^102. */
void
easel_curve_controls (int sx, int sy, int ex, int ey, const EaselCurveBend *b, int n, int out[4])
{
  int s[2] = { clamp_coord (sx), clamp_coord (sy) }, e[2] = { clamp_coord (ex), clamp_coord (ey) };
  int64_t w[4], v[4];

  if (n < 2)
    {
      int64_t dx = (int64_t) e[0] - s[0], dy = (int64_t) e[1] - s[1];
      i128 d = ((i128) dx * dx + (i128) dy * dy) * EASEL_CURVE_ONE, px, py, a, g, u, vv, k;

      if (d == 0)
        {
          for (int c = 0; c < 2; c++)
            out[c] = out[c + 2] = round_limited (4 * (i128) (c == 0 ? b[0].x : b[0].y) - (i128) s[c] * EASEL_CURVE_ONE,
                                                 3 * (i128) EASEL_CURVE_ONE);
          return;
        }
      px = (i128) b[0].x - (i128) s[0] * EASEL_CURVE_ONE;
      py = (i128) b[0].y - (i128) s[1] * EASEL_CURVE_ONE;
      a = px * dx + py * dy;
      g = -px * dy + py * dx;
      u = 2 * a <= d ? 0 : 8 * a - 4 * d;
      vv = 2 * a <= d ? 8 * a - d : 3 * d;
      k = 4 * g;
      out[0] = round_limited ((i128) s[0] * 3 * d + u * dx - k * dy, 3 * d);
      out[1] = round_limited ((i128) s[1] * 3 * d + u * dy + k * dx, 3 * d);
      out[2] = round_limited ((i128) s[0] * 3 * d + vv * dx - k * dy, 3 * d);
      out[3] = round_limited ((i128) s[1] * 3 * d + vv * dy + k * dx, 3 * d);
      return;
    }
  {
    int t[2];
    i128 det;

    easel_curve_places (sx, sy, ex, ey, b, 2, t);
    weights (t[0], w);
    weights (t[1], v);
    det = (i128) w[1] * v[2] - (i128) w[2] * v[1];
    for (int c = 0; c < 2; c++)
      {
        i128 r1 = (i128) (c == 0 ? b[0].x : b[0].y) - (i128) w[0] * s[c] - (i128) w[3] * e[c];
        i128 r2 = (i128) (c == 0 ? b[1].x : b[1].y) - (i128) v[0] * s[c] - (i128) v[3] * e[c];

        out[c] = round_limited (r1 * v[2] - (i128) w[2] * r2, det);
        out[c + 2] = round_limited ((i128) w[1] * r2 - r1 * v[1], det);
      }
  }
}
