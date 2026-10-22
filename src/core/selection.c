/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "selection.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "change-log.h"
#include "line.h"

/* An image of the float or of a copy, as the lay and the put-down read it. */
typedef struct
{
  int w, h;
  const uint32_t *px;
  const uint8_t *mask;
} ImageView;

static ImageView
current_view (const EaselFloat *f)
{
  return (ImageView) { f->w, f->h, f->px, f->mask };
}

/* The image as lifted: the float's own until it is stretched. */
static ImageView
original_view (const EaselFloat *f)
{
  if (f->orig_px == NULL)
    return current_view (f);
  return (ImageView) { f->ow, f->oh, f->orig_px, f->orig_mask };
}

/* The image the copy k was made with. */
static ImageView
stamp_view (const EaselFloat *f, size_t k)
{
  int id = f->stamp_img != NULL ? f->stamp_img[k] : EASEL_IMAGE_CURRENT;

  if (id == EASEL_IMAGE_CURRENT)
    return current_view (f);
  if (id == EASEL_IMAGE_ORIGINAL)
    return original_view (f);
  return (ImageView) { f->kept[id].w, f->kept[id].h, f->kept[id].px, f->kept[id].mask };
}

static bool
inside (int x, int y, int rx, int ry, int rw, int rh)
{
  return x >= rx && y >= ry && x - rx < rw && y - ry < rh;
}

EaselSelectionResult
easel_selection_lift (EaselFloat *f, const EaselImage *img, int x, int y, int w, int h, const uint8_t *mask, bool copy,
                      uint32_t hole_color, size_t max_bytes)
{
  size_t n = (size_t) w * (size_t) h;
  size_t bytes;

  *f = (EaselFloat) { 0 };
  if (w <= 0 || h <= 0 || x < 0 || y < 0 || x > img->width - w || y > img->height - h)
    return EASEL_SELECTION_NO_MEMORY;
  if (n > SIZE_MAX / (sizeof *f->px + 1))
    return EASEL_SELECTION_NO_MEMORY;
  bytes = n * sizeof *f->px + (mask != NULL ? n : 0);
  if (bytes > max_bytes)
    return EASEL_SELECTION_NO_MEMORY;
  f->px = malloc (n * sizeof *f->px);
  if (f->px == NULL)
    return EASEL_SELECTION_NO_MEMORY;
  if (mask != NULL)
    {
      f->mask = malloc (n);
      if (f->mask == NULL)
        {
          free (f->px);
          *f = (EaselFloat) { 0 };
          return EASEL_SELECTION_NO_MEMORY;
        }
      memcpy (f->mask, mask, n);
    }
  for (int j = 0; j < h; j++)
    memcpy (f->px + (size_t) j * w, img->px + (size_t) (y + j) * img->width + x, (size_t) w * sizeof *f->px);
  f->x = f->hx = x;
  f->y = f->hy = y;
  f->w = w;
  f->h = h;
  f->ow = w;
  f->oh = h;
  f->hole = !copy;
  f->hole_color = hole_color;
  return EASEL_SELECTION_DONE;
}

bool
easel_selection_stamp (EaselFloat *f, size_t max_bytes)
{
  easel_selection_complete (f);                /* a copy is made with a whole image */
  if (f->n_stamps == f->cap)
    {
      size_t cap = f->cap ? f->cap * 2 : 8;
      EaselStamp *p;
      int *img = NULL;

      if (cap > SIZE_MAX / (sizeof *p + sizeof *img) || cap * (sizeof *p + (f->stamp_img != NULL ? sizeof *img : 0)) > max_bytes)
        return false;
      p = realloc (f->stamps, cap * sizeof *p);
      if (p == NULL)
        return false;
      f->stamps = p;
      if (f->stamp_img != NULL)
        {
          img = realloc (f->stamp_img, cap * sizeof *img);
          if (img == NULL)
            return false;                          /* the copies' list grew, its images did not: the count is unchanged */
          f->stamp_img = img;
        }
      f->cap = cap;
    }
  if (f->stamp_img != NULL)
    f->stamp_img[f->n_stamps] = EASEL_IMAGE_CURRENT;
  f->stamps[f->n_stamps++] = (EaselStamp) { f->x, f->y };
  return true;
}

bool
easel_selection_smear (EaselFloat *f, int x, int y, size_t max_bytes)
{
  EaselLine l;
  int lx, ly, x0 = f->x, y0 = f->y;
  size_t n = f->n_stamps;

  easel_line_init (&l, x0, y0, x, y);
  while (easel_line_next (&l, &lx, &ly))
    {
      if (lx == x && ly == y)
        break;                                  /* the place it moves to is not a copy */
      f->x = lx;
      f->y = ly;
      if (!easel_selection_stamp (f, max_bytes))
        {
          f->n_stamps = n;
          f->x = x0;
          f->y = y0;
          return false;
        }
    }
  f->x = x;
  f->y = y;
  return true;
}

/* Is (x, y) a pixel of the float's shape when it is at (px, py)? */
static bool
in_shape (const ImageView *v, int px, int py, int x, int y)
{
  return inside (x, y, px, py, v->w, v->h) && (v->mask == NULL || v->mask[(size_t) (y - py) * v->w + (x - px)] != 0);
}

/* The float's pixels at (px, py) written over the part of the window (x0, y0, w, h) that is inside the picture: the cost is the float's
   own area, not the window's. */
static void
lay_float_at (const ImageView *f, int px, int py, const EaselImage *img, int x0, int y0, int w, int h, uint32_t *dst,
              size_t dst_stride)
{
  int ax0 = px > x0 ? px : x0, ay0 = py > y0 ? py : y0;
  int ax1 = px + f->w < x0 + w ? px + f->w : x0 + w, ay1 = py + f->h < y0 + h ? py + f->h : y0 + h;

  ax0 = ax0 > 0 ? ax0 : 0;
  ay0 = ay0 > 0 ? ay0 : 0;
  ax1 = ax1 < img->width ? ax1 : img->width;
  ay1 = ay1 < img->height ? ay1 : img->height;
  for (int y = ay0; y < ay1; y++)
    for (int x = ax0; x < ax1; x++)
      {
        size_t at = (size_t) (y - py) * f->w + (size_t) (x - px);

        if (f->mask == NULL || f->mask[at] != 0)
          dst[(size_t) (y - y0) * dst_stride + (size_t) (x - x0)] = f->px[at];
      }
}

void
easel_selection_lay (const EaselFloat *f, bool with_float, const EaselImage *img, int x0, int y0, int w, int h,
                     uint32_t *dst, size_t dst_stride)
{
  easel_selection_lay_range (f, 0, f->n_stamps, with_float, img, x0, y0, w, h, dst, dst_stride, true);
}

void
easel_selection_lay_range (const EaselFloat *f, size_t first, size_t last, bool with_float, const EaselImage *img, int x0, int y0,
                           int w, int h, uint32_t *dst, size_t dst_stride, bool with_base)
{
  ImageView hole = original_view (f), cur = current_view (f), sv;

  for (int j = 0; with_base && j < h; j++)
    {
      int y = y0 + j;

      if (y < 0 || y >= img->height)
        continue;
      for (int i = 0; i < w; i++)
        {
          int x = x0 + i;
          uint32_t v;

          if (x < 0 || x >= img->width)
            continue;
          v = img->px[(size_t) y * img->width + x];
          if (f->hole && in_shape (&hole, f->hx, f->hy, x, y))
            v = f->hole_color;
          dst[(size_t) j * dst_stride + i] = v;
        }
    }
  for (size_t k = first; k < last; k++)
    {
      sv = stamp_view (f, k);
      lay_float_at (&sv, f->stamps[k].x, f->stamps[k].y, img, x0, y0, w, h, dst, dst_stride);
    }
  if (with_float)
    lay_float_at (&cur, f->x, f->y, img, x0, y0, w, h, dst, dst_stride);
}

/* The rectangle of the picture the result can differ in: the hole and the float, limited to the picture. FALSE if none. */
bool
easel_selection_bounds (const EaselFloat *f, bool with_float, const EaselImage *img, int *x0, int *y0, int *x1, int *y1)
{
  long long ax0 = img->width, ay0 = img->height, ax1 = -1, ay1 = -1;

  if (f->hole)
    {
      ImageView hv = original_view (f);

      ax0 = f->hx;
      ay0 = f->hy;
      ax1 = (long long) f->hx + hv.w - 1;
      ay1 = (long long) f->hy + hv.h - 1;
    }
  for (size_t k = 0; k <= f->n_stamps; k++)
    {
      /* each stamp, then the float itself */
      int px = k < f->n_stamps ? f->stamps[k].x : f->x, py = k < f->n_stamps ? f->stamps[k].y : f->y;
      ImageView v = k < f->n_stamps ? stamp_view (f, k) : current_view (f);

      if (k == f->n_stamps && !with_float)
        break;
      ax0 = px < ax0 ? px : ax0;
      ay0 = py < ay0 ? py : ay0;
      ax1 = (long long) px + v.w - 1 > ax1 ? (long long) px + v.w - 1 : ax1;
      ay1 = (long long) py + v.h - 1 > ay1 ? (long long) py + v.h - 1 : ay1;
    }
  ax0 = ax0 < 0 ? 0 : ax0;
  ay0 = ay0 < 0 ? 0 : ay0;
  ax1 = ax1 > img->width - 1 ? img->width - 1 : ax1;
  ay1 = ay1 > img->height - 1 ? img->height - 1 : ay1;
  if (ax0 > ax1 || ay0 > ay1)
    return false;
  *x0 = (int) ax0;
  *y0 = (int) ay0;
  *x1 = (int) ax1;
  *y1 = (int) ay1;
  return true;
}

EaselSelectionResult
easel_selection_put_down (const EaselFloat *f, bool with_float, EaselImage *img, EaselHistory *history, size_t max_bytes)
{
  EaselChangeLog l;
  int x0, y0, x1, y1;
  uint32_t *row;
  bool ok = true;

  if (!easel_selection_bounds (f, with_float, img, &x0, &y0, &x1, &y1))
    return EASEL_SELECTION_NOTHING;
  row = malloc ((size_t) (x1 - x0 + 1) * sizeof *row);
  if (row == NULL)
    return EASEL_SELECTION_NO_MEMORY;
  easel_change_log_init (&l, max_bytes);
  /* Each row laid as the preview lays it, then the pixels that differ set through the log (it logs only changes). */
  for (int y = y0; ok && y <= y1; y++)
    {
      easel_selection_lay (f, with_float, img, x0, y, x1 - x0 + 1, 1, row, (size_t) (x1 - x0 + 1));
      for (int x = x0; ok && x <= x1; x++)
        ok = easel_change_log_set (&l, img, x, y, row[x - x0]);
    }
  free (row);
  if (!ok)
    {
      easel_change_log_restore (&l, img);
      easel_change_log_free (&l);
      return EASEL_SELECTION_NO_MEMORY;
    }
  if (l.n == 0)
    {
      easel_change_log_free (&l);
      return EASEL_SELECTION_NOTHING;
    }
  if (history == NULL)
    {
      easel_change_log_free (&l);
      return EASEL_SELECTION_DONE;
    }
  if (!easel_change_log_push (&l, history))
    {
      easel_change_log_restore (&l, img);
      easel_change_log_free (&l);
      return EASEL_SELECTION_NO_MEMORY;
    }
  return EASEL_SELECTION_DONE;
}

EaselSelectionResult
easel_selection_clear (EaselImage *img, EaselHistory *history, int x, int y, int w, int h, const uint8_t *mask, uint32_t color,
                       size_t max_bytes)
{
  EaselChangeLog l;
  bool ok = true;

  if (w <= 0 || h <= 0 || x < 0 || y < 0 || x > img->width - w || y > img->height - h)
    return EASEL_SELECTION_NOTHING;
  easel_change_log_init (&l, max_bytes);
  for (int j = y; ok && j < y + h; j++)
    for (int i = x; ok && i < x + w; i++)
      if (mask == NULL || mask[(size_t) (j - y) * w + (i - x)] != 0)
        ok = easel_change_log_set (&l, img, i, j, color);
  if (!ok)
    {
      easel_change_log_restore (&l, img);
      easel_change_log_free (&l);
      return EASEL_SELECTION_NO_MEMORY;
    }
  if (l.n == 0)
    {
      easel_change_log_free (&l);
      return EASEL_SELECTION_NOTHING;
    }
  if (history != NULL && !easel_change_log_push (&l, history))
    {
      easel_change_log_restore (&l, img);
      easel_change_log_free (&l);
      return EASEL_SELECTION_NO_MEMORY;
    }
  easel_change_log_free (&l);
  return EASEL_SELECTION_DONE;
}

void
easel_float_free (EaselFloat *f)
{
  free (f->px);
  free (f->mask);
  free (f->orig_px);
  free (f->orig_mask);
  for (size_t i = 0; i < f->n_kept; i++)
    {
      free (f->kept[i].px);
      free (f->kept[i].mask);
    }
  free (f->kept);
  free (f->stamp_img);
  free (f->stamps);
  free (f->fill);
  *f = (EaselFloat) { 0 };
}

#define FILL_BLOCK 256
#define MIN(a, b) ((a) < (b) ? (a) : (b))

/* The new pixels of the rectangle [x0, x1) x [y0, y1) of a W x H image made from the image o by the nearest pixel (REQ-0015-05): the
   same function whole or in parts. With a mask the shape's pixels of the rectangle are counted: the number returned. */
static size_t
scale_region (const ImageView *o, int W, int H, uint32_t *op, uint8_t *om, int x0, int y0, int x1, int y1)
{
  size_t kept = 0;

  for (int j = y0; j < y1; j++)
    {
      int sj = (int) (((2LL * j + 1) * o->h) / (2LL * H));

      for (int i = x0; i < x1; i++)
        {
          int si = (int) (((2LL * i + 1) * o->w) / (2LL * W));
          size_t from = (size_t) sj * (size_t) o->w + (size_t) si;

          op[(size_t) j * (size_t) W + (size_t) i] = o->px[from];
          if (om != NULL)
            {
              om[(size_t) j * (size_t) W + (size_t) i] = o->mask[from] != 0;
              kept += o->mask[from] != 0;
            }
        }
    }
  return kept;
}

/* A shape that the nearest pixel lost keeps one pixel (REQ-0015-06): the new pixel nearest the center of the box among those whose
   footprint (the lifted pixels it covers) holds a shape pixel, with the color of the first such shape pixel in reading order. */
static void
keep_one_pixel (const ImageView *o, int W, int H, uint32_t *op, uint8_t *om)
{
  double best = 1e300;
  int bi = -1, bj = -1;

  for (int j = 0; j < H; j++)
    {
      int y0 = (int) (((long long) j * o->h) / H), y1 = (int) ((((long long) j + 1) * o->h + H - 1) / H) - 1;

      for (int i = 0; i < W; i++)
        {
          int x0 = (int) (((long long) i * o->w) / W), x1 = (int) ((((long long) i + 1) * o->w + W - 1) / W) - 1;
          double dx = i + 0.5 - W / 2.0, dy = j + 0.5 - H / 2.0, d = dx * dx + dy * dy;
          bool holds = false;

          if (x1 < x0)
            x1 = x0;
          if (y1 < y0)
            y1 = y0;
          for (int y = y0; y <= y1 && !holds; y++)
            for (int x = x0; x <= x1 && !holds; x++)
              holds = o->mask[(size_t) y * (size_t) o->w + (size_t) x] != 0;
          if (holds && d < best)
            {
              best = d;
              bi = i;
              bj = j;
            }
        }
    }
  if (bi >= 0)
    {
      int y0 = (int) (((long long) bj * o->h) / H), y1 = (int) ((((long long) bj + 1) * o->h + H - 1) / H) - 1;
      int x0 = (int) (((long long) bi * o->w) / W), x1 = (int) ((((long long) bi + 1) * o->w + W - 1) / W) - 1;
      bool done = false;

      if (x1 < x0)
        x1 = x0;
      if (y1 < y0)
        y1 = y0;
      for (int y = y0; y <= y1 && !done; y++)
        for (int x = x0; x <= x1 && !done; x++)
          if (o->mask[(size_t) y * (size_t) o->w + (size_t) x] != 0)
            {
              om[(size_t) bj * (size_t) W + (size_t) bi] = 1;
              op[(size_t) bj * (size_t) W + (size_t) bi] = o->px[(size_t) y * (size_t) o->w + (size_t) x];
              done = true;
            }
    }
}

/* The memory of a W x H image (4 bytes a pixel, a byte more with a mask) against the limit, and its buffers, zeroed or not. */
static EaselSelectionResult
image_alloc (bool with_mask, int W, int H, size_t max_bytes, bool zero, uint32_t **op, uint8_t **om)
{
  size_t n, bytes;

  *op = NULL;
  *om = NULL;
  if (W < 1 || H < 1 || (size_t) W > SIZE_MAX / (size_t) H)
    return EASEL_SELECTION_NO_MEMORY;
  n = (size_t) W * (size_t) H;
  if (n > (SIZE_MAX - n) / sizeof **op)
    return EASEL_SELECTION_NO_MEMORY;
  bytes = n * sizeof **op + (with_mask ? n : 0);
  if (bytes > max_bytes)
    return EASEL_SELECTION_NO_MEMORY;
  *op = zero ? calloc (n, sizeof **op) : malloc (n * sizeof **op);
  if (*op == NULL)
    return EASEL_SELECTION_NO_MEMORY;
  if (with_mask)
    {
      *om = zero ? calloc (n, 1) : malloc (n);
      if (*om == NULL)
        {
          free (*op);
          *op = NULL;
          return EASEL_SELECTION_NO_MEMORY;
        }
    }
  return EASEL_SELECTION_DONE;
}

EaselSelectionResult
easel_selection_scale (const uint32_t *px, const uint8_t *mask, int w, int h, int W, int H, uint32_t **out_px, uint8_t **out_mask,
                       size_t max_bytes)
{
  ImageView o = { w, h, px, mask };
  uint32_t *op;
  uint8_t *om;
  EaselSelectionResult r;

  *out_px = NULL;
  if (out_mask != NULL)
    *out_mask = NULL;
  if (w < 1 || h < 1)
    return EASEL_SELECTION_NO_MEMORY;
  r = image_alloc (mask != NULL, W, H, max_bytes, false, &op, &om);
  if (r != EASEL_SELECTION_DONE)
    return r;
  if (scale_region (&o, W, H, op, om, 0, 0, W, H) == 0 && om != NULL)
    keep_one_pixel (&o, W, H, op, om);
  *out_px = op;
  if (out_mask != NULL)
    *out_mask = om;
  else
    free (om);
  return EASEL_SELECTION_DONE;
}

#define STRETCH_MAX 10000

/* One side: the edge at `lo` or `hi` (hi = lo + size) dragged by d, the other edge fixed: its new start and size. */
static void
drag_side (bool moves_low, bool moves_high, int lo, int size, int d, int *nlo, int *nsize)
{
  long long a = lo, b = (long long) lo + size;

  if (moves_low)
    {
      a += d;
      if (b - a < 1)
        a = b - 1;                                /* past the opposite edge: one pixel beside it */
      if (b - a > STRETCH_MAX)
        a = b - STRETCH_MAX;
    }
  else if (moves_high)
    {
      b += d;
      if (b - a < 1)
        b = a + 1;
      if (b - a > STRETCH_MAX)
        b = a + STRETCH_MAX;
    }
  *nlo = (int) a;
  *nsize = (int) (b - a);
}

void
easel_stretch_rect (EaselSelHandle handle, int x, int y, int w, int h, int dx, int dy, bool keep_ratio, int *nx, int *ny, int *nw,
                    int *nh)
{
  bool left = handle == EASEL_SEL_HANDLE_TL || handle == EASEL_SEL_HANDLE_L || handle == EASEL_SEL_HANDLE_BL;
  bool right = handle == EASEL_SEL_HANDLE_TR || handle == EASEL_SEL_HANDLE_R || handle == EASEL_SEL_HANDLE_BR;
  bool top = handle == EASEL_SEL_HANDLE_TL || handle == EASEL_SEL_HANDLE_T || handle == EASEL_SEL_HANDLE_TR;
  bool bottom = handle == EASEL_SEL_HANDLE_BL || handle == EASEL_SEL_HANDLE_B || handle == EASEL_SEL_HANDLE_BR;
  bool corner = (left || right) && (top || bottom);

  drag_side (left, right, x, w, dx, nx, nw);
  drag_side (top, bottom, y, h, dy, ny, nh);
  if (keep_ratio && corner)
    {
      double fw = (double) *nw / w, fh = (double) *nh / h;
      bool width_leads = fabs (fw - 1) >= fabs (fh - 1);
      double f = width_leads ? fw : fh;
      int lead = width_leads ? *nw : *nh, old_other = width_leads ? h : w;
      long long other = (long long) floor (old_other * f + 0.5);

      if (other < 1)
        other = 1;
      if (other > STRETCH_MAX)
        {
          other = STRETCH_MAX;
          lead = (int) floor ((double) (width_leads ? w : h) * STRETCH_MAX / old_other + 0.5);
          if (lead < 1)
            lead = 1;
        }
      if (width_leads)
        {
          *nw = lead;
          *nh = (int) other;
        }
      else
        {
          *nh = lead;
          *nw = (int) other;
        }
      /* The edges the handle does not stand for stay: the new origin follows from the new size. */
      if (left)
        *nx = x + w - *nw;
      else
        *nx = x;
      if (top)
        *ny = y + h - *nh;
      else
        *ny = y;
    }
}

static EaselSelectionResult
stretch_to (EaselFloat *f, int x, int y, int W, int H, size_t max_bytes, bool lazy)
{
  ImageView orig = original_view (f);
  uint32_t *npx = NULL;
  uint8_t *nmask = NULL, *nfill = NULL;
  int *img = NULL;
  EaselKept *kept = NULL;
  bool uses_current = false, move_to_kept = false;
  EaselSelectionResult r;

  /* The new image first, from the pixels as lifted: nothing is changed until everything is had. */
  if (lazy)
    {
      r = image_alloc (orig.mask != NULL, W, H, max_bytes, true, &npx, &nmask);
      if (r == EASEL_SELECTION_DONE)
        {
          nfill = calloc ((size_t) ((W + FILL_BLOCK - 1) / FILL_BLOCK) * (size_t) ((H + FILL_BLOCK - 1) / FILL_BLOCK), 1);
          if (nfill == NULL)
            {
              free (npx);
              free (nmask);
              r = EASEL_SELECTION_NO_MEMORY;
            }
        }
    }
  else
    r = easel_selection_scale (orig.px, orig.mask, orig.w, orig.h, W, H, &npx, orig.mask != NULL ? &nmask : NULL, max_bytes);
  if (r != EASEL_SELECTION_DONE)
    return r;
  for (size_t k = 0; k < f->n_stamps && !uses_current; k++)
    uses_current = f->stamp_img == NULL || f->stamp_img[k] == EASEL_IMAGE_CURRENT;
  if (uses_current)
    {
      /* Copies made with the current image keep it: the original when it is the current one, else it is kept (whole). */
      if (f->stamp_img == NULL)
        {
          /* The copies' list now holds an image for each place: counted as the list is when it grows (easel_selection_stamp). */
          if (f->cap > SIZE_MAX / (sizeof *f->stamps + sizeof *img) || f->cap * (sizeof *f->stamps + sizeof *img) > max_bytes)
            goto no_memory;
          img = malloc (f->cap * sizeof *img);
          if (img == NULL)
            goto no_memory;
        }
      if (f->orig_px != NULL)
        {
          move_to_kept = true;
          kept = realloc (f->kept, (f->n_kept + 1) * sizeof *kept);
          if (kept == NULL)
            goto no_memory;
        }
    }
  /* Everything is had: from here nothing fails. */
  if (uses_current)
    {
      int id;

      easel_selection_complete (f);
      if (img != NULL)
        {
          f->stamp_img = img;
          for (size_t k = 0; k < f->n_stamps; k++)
            f->stamp_img[k] = EASEL_IMAGE_CURRENT;
        }
      if (move_to_kept)
        {
          f->kept = kept;
          f->kept[f->n_kept] = (EaselKept) { f->w, f->h, f->px, f->mask };
          id = (int) f->n_kept++;
        }
      else
        id = EASEL_IMAGE_ORIGINAL;
      for (size_t k = 0; k < f->n_stamps; k++)
        if (f->stamp_img[k] == EASEL_IMAGE_CURRENT)
          f->stamp_img[k] = id;
    }
  if (f->orig_px == NULL)
    {
      f->orig_px = f->px;                           /* the first stretch: the pixels as lifted are kept */
      f->orig_mask = f->mask;
    }
  else if (!move_to_kept)
    {
      free (f->px);                                 /* the image replaced, used by no copy */
      free (f->mask);
    }
  free (f->fill);
  f->fill = nfill;
  f->fill_cols = (W + FILL_BLOCK - 1) / FILL_BLOCK;
  f->fill_rows = (H + FILL_BLOCK - 1) / FILL_BLOCK;
  f->made_px = lazy ? 0 : (size_t) W * (size_t) H;
  f->kept_px = 0;
  f->px = npx;
  f->mask = nmask;
  f->w = W;
  f->h = H;
  f->x = x;
  f->y = y;
  return EASEL_SELECTION_DONE;
no_memory:
  free (npx);
  free (nmask);
  free (nfill);
  free (img);
  return EASEL_SELECTION_NO_MEMORY;
}

EaselSelectionResult
easel_selection_stretch (EaselFloat *f, int x, int y, int W, int H, size_t max_bytes)
{
  return stretch_to (f, x, y, W, H, max_bytes, false);
}

EaselSelectionResult
easel_selection_stretch_lazy (EaselFloat *f, int x, int y, int W, int H, size_t max_bytes)
{
  return stretch_to (f, x, y, W, H, max_bytes, true);
}

void
easel_selection_fill (EaselFloat *f, int x0, int y0, int x1, int y1)
{
  ImageView orig;

  if (f->fill == NULL)
    return;
  x0 = x0 < 0 ? 0 : x0;
  y0 = y0 < 0 ? 0 : y0;
  x1 = x1 > f->w ? f->w : x1;
  y1 = y1 > f->h ? f->h : y1;
  if (x0 >= x1 || y0 >= y1)
    return;
  orig = original_view (f);
  for (int by = y0 / FILL_BLOCK; by <= (y1 - 1) / FILL_BLOCK; by++)
    for (int bx = x0 / FILL_BLOCK; bx <= (x1 - 1) / FILL_BLOCK; bx++)
      {
        int ax = bx * FILL_BLOCK, ay = by * FILL_BLOCK, bx1 = MIN (ax + FILL_BLOCK, f->w), by1 = MIN (ay + FILL_BLOCK, f->h);

        if (f->fill[by * f->fill_cols + bx])
          continue;
        f->kept_px += scale_region (&orig, f->w, f->h, f->px, f->mask, ax, ay, bx1, by1);
        f->made_px += (size_t) (bx1 - ax) * (size_t) (by1 - ay);
        f->fill[by * f->fill_cols + bx] = 1;
      }
}

void
easel_selection_complete (EaselFloat *f)
{
  ImageView orig;

  if (f->fill == NULL)
    return;
  easel_selection_fill (f, 0, 0, f->w, f->h);
  orig = original_view (f);
  if (f->mask != NULL && f->kept_px == 0)
    keep_one_pixel (&orig, f->w, f->h, f->px, f->mask);
  free (f->fill);
  f->fill = NULL;
}
