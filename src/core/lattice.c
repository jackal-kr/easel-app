/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "lattice.h"

#include <math.h>

void
easel_lattice_init (EaselLattice *w, int x0, int y0, int x1, int y1)
{
  easel_line_init (&w->line, x0, y0, x1, y1);
  w->started = false;
  w->has_pending = false;
}

bool
easel_lattice_next (EaselLattice *w, int *x, int *y)
{
  int lx, ly;

  if (w->has_pending)
    {
      *x = w->pend_x;
      *y = w->pend_y;
      w->has_pending = false;
      w->px = *x;
      w->py = *y;
      return true;
    }
  if (!easel_line_next (&w->line, &lx, &ly))
    return false;
  if (w->started && lx != w->px && ly != w->py)
    {
      /* A diagonal step of the Pencil's line: along x first, then along y. */
      w->has_pending = true;
      w->pend_x = lx;
      w->pend_y = ly;
      *x = lx;
      *y = w->py;
      w->px = *x;
      return true;
    }
  w->started = true;
  *x = w->px = lx;
  *y = w->py = ly;
  return true;
}

/* The state of the walk along the dashes: where in the pattern, and the piece being made. */
typedef struct
{
  double dash, arc, phase;
  EaselDash *out;
  size_t max, n;
  bool open;                       /* a piece is being made, in cur */
  EaselDash cur;
} DashWalk;

static void
flush (DashWalk *d)
{
  if (!d->open)
    return;
  if (d->out != NULL && d->n < d->max)
    d->out[d->n] = d->cur;
  d->n++;
  d->open = false;
}

/* One step from (ax, ay) to (bx, by), a unit long: the black parts of the pattern that fall in it. */
static void
dash_step (DashWalk *d, int ax, int ay, int bx, int by)
{
  double ux = (bx > ax) - (bx < ax), uy = (by > ay) - (by < ay);

  for (long long k = (long long) floor ((d->arc - d->phase) / (2 * d->dash)) - 1; k * 2 * d->dash + d->phase < d->arc + 1; k++)
    {
      double from = k * 2 * d->dash + d->phase, to = from + d->dash;
      double x0, y0, x1, y1;

      from = from < d->arc ? d->arc : from;
      to = to > d->arc + 1 ? d->arc + 1 : to;
      if (from >= to)
        continue;
      x0 = ax + ux * (from - d->arc);
      y0 = ay + uy * (from - d->arc);
      x1 = ax + ux * (to - d->arc);
      y1 = ay + uy * (to - d->arc);
      /* A piece that goes on from the one before it, straight, is the same piece. */
      if (d->open && d->cur.x1 == x0 && d->cur.y1 == y0
          && (d->cur.x1 - d->cur.x0) * uy == (d->cur.y1 - d->cur.y0) * ux && (d->cur.x1 - d->cur.x0) * ux + (d->cur.y1 - d->cur.y0) * uy >= 0)
        {
          d->cur.x1 = x1;
          d->cur.y1 = y1;
        }
      else
        {
          flush (d);
          d->cur = (EaselDash) { x0, y0, x1, y1 };
          d->open = true;
        }
    }
  d->arc += 1;
}

size_t
easel_lattice_dashes (const int *xy, size_t n, bool closed, double dash, double phase, EaselDash *out, size_t max)
{
  DashWalk d = { dash, 0, phase, out, max, 0, false, { 0, 0, 0, 0 } };
  size_t segments = closed && n >= 2 ? n : (n >= 1 ? n - 1 : 0);

  if (!(dash > 0))
    return 0;
  for (size_t i = 0; i < segments; i++)
    {
      size_t j = (i + 1) % n;
      EaselLattice w;
      int x, y, px = 0, py = 0;
      bool first = true;

      easel_lattice_init (&w, xy[2 * i], xy[2 * i + 1], xy[2 * j], xy[2 * j + 1]);
      while (easel_lattice_next (&w, &x, &y))
        {
          if (!first)
            dash_step (&d, px, py, x, y);
          first = false;
          px = x;
          py = y;
        }
    }
  flush (&d);
  return d.n;
}

size_t
easel_lattice_runs (const int *xy, size_t n, bool closed, EaselDash *out, size_t max)
{
  size_t segments = closed && n >= 2 ? n : (n >= 1 ? n - 1 : 0), count = 0;
  int rx = 0, ry = 0, ex = 0, ey = 0, sx = 0, sy = 0;
  bool open_run = false;

  for (size_t i = 0; i < segments; i++)
    {
      size_t j = (i + 1) % n;
      EaselLattice w;
      int x, y, px = 0, py = 0;
      bool first = true;

      easel_lattice_init (&w, xy[2 * i], xy[2 * i + 1], xy[2 * j], xy[2 * j + 1]);
      while (easel_lattice_next (&w, &x, &y))
        {
          if (!first)
            {
              int dx = x - px, dy = y - py;

              if (open_run && dx == sx && dy == sy && px == ex && py == ey)
                {
                  ex = x;                                  /* the same way on: the run grows */
                  ey = y;
                }
              else
                {
                  if (open_run && count < max && out != NULL)
                    out[count] = (EaselDash) { rx, ry, ex, ey };
                  count += open_run;
                  rx = px;
                  ry = py;
                  ex = x;
                  ey = y;
                  sx = dx;
                  sy = dy;
                  open_run = true;
                }
            }
          first = false;
          px = x;
          py = y;
        }
    }
  if (open_run && count < max && out != NULL)
    out[count] = (EaselDash) { rx, ry, ex, ey };
  return count + open_run;
}
