/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Feature 0025, T001: can the canvas draw a fractional zoom by its own edges, and what does a frame cost? The ways of drawing the tiles:
   "renderer" (each tile scaled with NEAREST, as the canvas does today), "shift" and "shift+clip" (the same, moved by under 0.01 device
   pixel to break ties one way, the second clipped to whole device pixels), "edges" (the view built at device size by the edge rule,
   the tiles read each frame) and "indexed" (the same from the picture's pixels, through a source index per device column).
   Usage: probe-edge-drawing edges    the edges read back from the drawn pixels on three rows or columns; run under each renderer
          probe-edge-drawing frames   frame and CPU time in a 1920 x 1080 view; real session, no other Easel window, pointer parked */

#include <gtk/gtk.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

enum { TILE = 500, CROSS = 500 };
#define SHIFT (1.0 / 256)

typedef struct {
  int w, h, cols, rows;
  uint32_t *px;
  GdkTexture **tiles;
} Picture;

/* The rule REQ-0025-02 asks for, in the form the Plan gives it. */
static int
edge_of (int k, int p)
{
  return (int) floor ((double) k * p / 100.0 + 0.5);
}

static int
cell_at (int d, int p)
{
  int64_t a = ((int64_t) 2 * d + 1) * 50;

  return (int) ((a > 0 ? (a + p - 1) / p : -(-a / p)) - 1);
}

/* Every pixel a color no neighbour has: x in the low 12 bits, y in the high 12. */
static uint32_t
color_of (int x, int y)
{
  uint32_t v = (uint32_t) x | ((uint32_t) y << 12);
  guchar c[4] = { v & 255, (v >> 8) & 255, (v >> 16) & 255, 255 };
  uint32_t out;

  memcpy (&out, c, 4);
  return out;
}

static void
build_tile (Picture *pic, int tx, int ty)
{
  int id = ty * pic->cols + tx;
  int tw = MIN (TILE, pic->w - tx * TILE), th = MIN (TILE, pic->h - ty * TILE);
  gsize stride = (gsize) pic->w * 4, start = (gsize) ty * TILE * stride + (gsize) tx * TILE * 4;
  g_autoptr (GBytes) b = g_bytes_new_static ((guchar *) pic->px + start, (gsize) (th - 1) * stride + (gsize) tw * 4);

  g_clear_object (&pic->tiles[id]);
  pic->tiles[id] = gdk_memory_texture_new (tw, th, GDK_MEMORY_R8G8B8A8, b, stride);
}

static Picture *
picture_new (int w, int h)
{
  Picture *pic = g_new0 (Picture, 1);

  pic->w = w;
  pic->h = h;
  pic->cols = (w + TILE - 1) / TILE;
  pic->rows = (h + TILE - 1) / TILE;
  pic->tiles = g_new0 (GdkTexture *, pic->cols * pic->rows);
  pic->px = g_new (uint32_t, (gsize) w * h);
  for (int y = 0; y < h; y++)
    for (int x = 0; x < w; x++)
      pic->px[(gsize) y * w + x] = color_of (x, y);
  for (int ty = 0; ty < pic->rows; ty++)
    for (int tx = 0; tx < pic->cols; tx++)
      build_tile (pic, tx, ty);
  return pic;
}

static void
picture_free (Picture *pic)
{
  for (int i = 0; i < pic->cols * pic->rows; i++)
    g_object_unref (pic->tiles[i]);
  g_free (pic->tiles);
  g_free (pic->px);
  g_free (pic);
}

/* shift: 0 none, 1 moved by SHIFT, 2 moved and clipped to the tile's whole device pixels. */
static void
append_by_renderer (GtkSnapshot *s, Picture *pic, int zoom, int shift)
{
  double l = zoom / 100.0, d = shift ? SHIFT : 0;

  for (int ty = 0; ty < pic->rows; ty++)
    for (int tx = 0; tx < pic->cols; tx++)
      {
        GdkTexture *t = pic->tiles[ty * pic->cols + tx];
        double x = tx * TILE * l, y = ty * TILE * l, w = gdk_texture_get_width (t) * l, h = gdk_texture_get_height (t) * l;

        if (shift == 2)
          gtk_snapshot_push_clip (s, &GRAPHENE_RECT_INIT ((float) x, (float) y, (float) ceil (w), (float) ceil (h)));
        gtk_snapshot_append_scaled_texture (s, t, GSK_SCALING_FILTER_NEAREST,
                                            &GRAPHENE_RECT_INIT ((float) (x + d), (float) (y + d), (float) w, (float) h));
        if (shift == 2)
          gtk_snapshot_pop (s);
      }
}

/* The view built at device size, per pixel, the tiles in view read once a frame. */
static GdkTexture *
view_by_edges (Picture *pic, int p, int vx, int vy, int vw, int vh)
{
  guchar *dst = g_malloc0 ((gsize) vw * vh * 4);
  guchar **src = g_new0 (guchar *, pic->cols * pic->rows);
  GBytes **held = g_new0 (GBytes *, pic->cols * pic->rows);
  g_autoptr (GdkTextureDownloader) dl = NULL;
  g_autoptr (GBytes) bytes = NULL;
  gsize stride[1];
  gsize strides[1024];
  int prev_k = -1;

  g_assert (pic->cols * pic->rows <= 1024);
  for (int r = 0; r < vh; r++)
    {
      int ky = cell_at (vy + r, p);
      guchar *row = dst + (gsize) r * vw * 4;

      if (ky < 0 || ky >= pic->h)
        continue;
      if (ky == prev_k)
        {
          memcpy (row, row - (gsize) vw * 4, (gsize) vw * 4);
          continue;
        }
      prev_k = ky;
      for (int i = 0; i < vw; i++)
        {
          int kx = cell_at (vx + i, p);
          int id;

          if (kx < 0 || kx >= pic->w)
            continue;
          id = (ky / TILE) * pic->cols + kx / TILE;
          if (src[id] == NULL)
            {
              if (dl == NULL)
                {
                  dl = gdk_texture_downloader_new (pic->tiles[id]);
                  gdk_texture_downloader_set_format (dl, GDK_MEMORY_R8G8B8A8);
                }
              else
                gdk_texture_downloader_set_texture (dl, pic->tiles[id]);
              held[id] = gdk_texture_downloader_download_bytes (dl, stride);
              strides[id] = stride[0];
              src[id] = (guchar *) g_bytes_get_data (held[id], NULL);
            }
          memcpy (row + (gsize) i * 4, src[id] + (gsize) (ky % TILE) * strides[id] + (gsize) (kx % TILE) * 4, 4);
        }
    }
  bytes = g_bytes_new_take (dst, (gsize) vw * vh * 4);
  for (int i = 0; i < pic->cols * pic->rows; i++)
    g_clear_pointer (&held[i], g_bytes_unref);
  g_free (held);
  g_free (src);
  return gdk_memory_texture_new (vw, vh, GDK_MEMORY_R8G8B8A8, bytes, (gsize) vw * 4);
}

/* The same from the picture's own pixels: one source index per device column, one source row per device row. */
static GdkTexture *
view_indexed (Picture *pic, int p, int vx, int vy, int vw, int vh)
{
  uint32_t *dst = g_malloc0 ((gsize) vw * vh * 4);
  int *xk = g_new (int, vw);
  int i0 = vw, i1 = 0, prev_k = -1;
  g_autoptr (GBytes) bytes = NULL;

  for (int i = 0; i < vw; i++)
    {
      xk[i] = cell_at (vx + i, p);
      if (xk[i] >= 0 && xk[i] < pic->w)
        {
          i0 = MIN (i0, i);
          i1 = MAX (i1, i + 1);
        }
    }
  for (int r = 0; r < vh; r++)
    {
      int ky = cell_at (vy + r, p);
      uint32_t *row = dst + (gsize) r * vw;

      if (ky < 0 || ky >= pic->h)
        continue;
      if (ky == prev_k)
        memcpy (row + i0, row - vw + i0, (gsize) (i1 - i0) * 4);
      else
        {
          const uint32_t *src = pic->px + (gsize) ky * pic->w;

          for (int i = i0; i < i1; i++)
            row[i] = src[xk[i]];
        }
      prev_k = ky;
    }
  g_free (xk);
  bytes = g_bytes_new_take (dst, (gsize) vw * vh * 4);
  return gdk_memory_texture_new (vw, vh, GDK_MEMORY_R8G8B8A8, bytes, (gsize) vw * 4);
}

/* ---- edges ---- */

enum { WAY_RENDERER, WAY_SHIFT, WAY_SHIFT_CLIP, WAY_EDGES, WAY_INDEXED, N_WAYS, WAY_UPLOAD = N_WAYS, WAY_REUSED };
/* WAY_UPLOAD and WAY_REUSED are frames only (T014): the view's pixels made before the run, a new texture of them each frame (the upload and the
   render, no build), or the same two textures again (the render alone). */
static const char *const way_names[] = { "renderer", "shift", "shift+clip", "edges", "indexed", "upload", "reused" };

typedef struct { int skipped, blended, jagged; } Read;

/* e[k] = where the cell k starts, on the picture's scale, -1 if not seen, read on the lines at 3, 40 % and 80 % of the cell rows drawn:
   an edge that differs between them is counted as jagged, a pixel that is no picture pixel's color as blended. */
static void
read_edges (GtkWindow *win, Picture *pic, int zoom, int way, int scroll, gboolean vertical, int *e, int n, Read *rd)
{
  GtkSnapshot *s = gtk_snapshot_new ();
  int along = edge_of (vertical ? pic->h : pic->w, zoom) + 1, cross = edge_of (CROSS, zoom);
  int vw = vertical ? cross : along, vh = vertical ? along : cross;
  int lines[] = { 3, cross * 2 / 5, cross * 4 / 5 };
  GskRenderNode *node;
  g_autoptr (GdkTexture) out = NULL;
  g_autoptr (GdkTexture) view = NULL;
  guchar *px;
  int ow, oh;
  g_autofree int *other = g_new (int, n);

  gtk_snapshot_translate (s, &GRAPHENE_POINT_INIT (vertical ? 0 : (float) -scroll, vertical ? (float) -scroll : 0));
  if (way == WAY_EDGES || way == WAY_INDEXED)
    {
      int vx = vertical ? 0 : scroll, vy = vertical ? scroll : 0;

      view = way == WAY_EDGES ? view_by_edges (pic, zoom, vx, vy, vw, vh) : view_indexed (pic, zoom, vx, vy, vw, vh);
      gtk_snapshot_append_scaled_texture (s, view, GSK_SCALING_FILTER_NEAREST, &GRAPHENE_RECT_INIT ((float) vx, (float) vy, (float) vw, (float) vh));
    }
  else
    append_by_renderer (s, pic, zoom, way == WAY_SHIFT ? 1 : way == WAY_SHIFT_CLIP ? 2 : 0);
  node = gtk_snapshot_free_to_node (s);
  out = gsk_renderer_render_texture (gtk_native_get_renderer (GTK_NATIVE (win)), node, &GRAPHENE_RECT_INIT (0, 0, vw, vh));
  ow = gdk_texture_get_width (out);
  oh = gdk_texture_get_height (out);
  px = g_malloc ((gsize) ow * oh * 4);
  gdk_texture_download (out, px, (gsize) ow * 4);
  memset (rd, 0, sizeof *rd);
  for (int li = 0; li < 3; li++)
    {
      int *t = li == 0 ? e : other, prev = -1, len = vertical ? oh : ow, limit = edge_of (n, zoom) - scroll;
      int expect = cell_at (lines[li], zoom);

      for (int k = 0; k < n; k++)
        t[k] = -1;
      for (int d = 0; d < len; d++)
        {
          guchar *c = vertical ? px + (gsize) d * ow * 4 + (gsize) lines[li] * 4 : px + (gsize) lines[li] * ow * 4 + (gsize) d * 4;   /* BGRA */
          uint32_t v = (uint32_t) c[2] | ((uint32_t) c[1] << 8) | ((uint32_t) c[0] << 16);
          int k = vertical ? (int) (v >> 12) : (int) (v & 0xfff), o = vertical ? (int) (v & 0xfff) : (int) (v >> 12);

          if (d >= limit)
            break;
          if (c[3] != 255 || k >= n || abs (o - expect) > 1)
            {
              rd->blended++;
              continue;
            }
          if (k != prev)
            {
              if (prev >= 0 && k != prev + 1)
                rd->skipped++;
              if (d > 0)
                t[k] = d + scroll;
              prev = k;
            }
        }
      if (li > 0)
        for (int k = 0; k < n; k++)
          if (t[k] >= 0 && e[k] >= 0 && t[k] != e[k])
            rd->jagged++;
    }
  g_free (px);
  gsk_render_node_unref (node);
}

static gboolean
run_edges (gpointer data)
{
  GtkWindow *win = data;
  static const int zooms[] = { 113, 129, 133, 150, 171, 200 };
  static const int scrolls[] = { 0, 1, 7, 13, 101 };
  static const int sizes[] = { 290, 900, 1200, 4000 };
  gboolean passes[N_WAYS];

  printf ("renderer %s\n", G_OBJECT_TYPE_NAME (gtk_native_get_renderer (GTK_NATIVE (win))));
  for (int way = 0; way < N_WAYS; way++)
    {
      passes[way] = TRUE;
      for (guint z = 0; z < G_N_ELEMENTS (zooms); z++)
        {
          int zoom = zooms[z], fl = zoom / 100, ce = (zoom + 99) / 100;
          int width_bad = 0, bound_bad = 0, moved = 0, size_bad = 0, seen = 0;
          Read total = { 0 };
          g_autofree int *big = g_new (int, 4100);

          for (int axis = 0; axis < 2; axis++)
            for (guint si = G_N_ELEMENTS (sizes); si-- > 0;)
              {
                int size = sizes[si];
                g_autofree int *ref = g_new (int, size), *e = g_new (int, size);
                Picture *pic = axis ? picture_new (CROSS, size) : picture_new (size, CROSS);

                for (guint sc = 0; sc < G_N_ELEMENTS (scrolls); sc++)
                  {
                    Read rd;

                    read_edges (win, pic, zoom, way, scrolls[sc], axis, e, size, &rd);
                    total.skipped += rd.skipped;
                    total.blended += rd.blended;
                    total.jagged += rd.jagged;
                    if (sc == 0)
                      memcpy (ref, e, (gsize) size * sizeof (int));
                    for (int k = 1; k < size; k++)
                      {
                        if (e[k] < 0)
                          continue;
                        seen++;
                        if (e[k - 1] >= 0 && e[k] - e[k - 1] != fl && e[k] - e[k - 1] != ce)
                          width_bad++;
                        if (fabs (e[k] - k * zoom / 100.0) > 0.5)
                          bound_bad++;
                        if (ref[k] >= 0 && e[k] != ref[k])
                          moved++;
                      }
                  }
                if (si == G_N_ELEMENTS (sizes) - 1)
                  memcpy (big, ref, (gsize) size * sizeof (int));
                else
                  for (int k = 1; k < size; k++)
                    if (ref[k] >= 0 && big[k] >= 0 && ref[k] != big[k])
                      size_bad++;
                picture_free (pic);
              }
          printf ("  %-10s %3d %%: read %6d | width %d | half-pixel %d | skipped %d | blended %d | jagged %4d | moved by scroll %4d | differs with size %4d\n",
                  way_names[way], zoom, seen, width_bad, bound_bad, total.skipped, total.blended, total.jagged, moved, size_bad);
          if (width_bad || bound_bad || total.skipped || total.blended || total.jagged || moved || size_bad)
            passes[way] = FALSE;
        }
    }
  for (int way = 0; way < N_WAYS; way++)
    printf ("way %-10s: %s\n", way_names[way], passes[way] ? "every rule holds" : "a rule fails");
  fflush (stdout);
  exit (0);
  return G_SOURCE_REMOVE;
}

/* ---- frames ---- */

enum { ACT_SCROLL, ACT_PENCIL, ACT_IDLE };
static const char *const act_names[] = { "scroll", "pencil", "idle" };
typedef struct { int way, zoom, act; } Config;
static const Config configs[] = {
  { WAY_RENDERER, 800, ACT_SCROLL },
  { WAY_RENDERER, 129, ACT_SCROLL }, { WAY_EDGES, 129, ACT_SCROLL }, { WAY_INDEXED, 129, ACT_SCROLL },
  { WAY_RENDERER, 129, ACT_PENCIL }, { WAY_INDEXED, 129, ACT_PENCIL }, { WAY_RENDERER, 129, ACT_IDLE }, { WAY_INDEXED, 129, ACT_IDLE },
  { WAY_RENDERER, 150, ACT_SCROLL }, { WAY_EDGES, 150, ACT_SCROLL }, { WAY_INDEXED, 150, ACT_SCROLL },
  { WAY_RENDERER, 150, ACT_PENCIL }, { WAY_INDEXED, 150, ACT_PENCIL }, { WAY_RENDERER, 150, ACT_IDLE }, { WAY_INDEXED, 150, ACT_IDLE },
  { WAY_UPLOAD, 129, ACT_SCROLL }, { WAY_REUSED, 129, ACT_SCROLL }, { WAY_UPLOAD, 150, ACT_SCROLL }, { WAY_REUSED, 150, ACT_SCROLL },
};
enum { ROUNDS = 3, FRAMES = 180, SKIP = 10, VIEW_W = 1920, VIEW_H = 1080 };

#define PROBE_TYPE_VIEW (probe_view_get_type ())
G_DECLARE_FINAL_TYPE (ProbeView, probe_view, PROBE, VIEW, GtkWidget)
struct _ProbeView { GtkWidget parent; Picture *pic; int zoom, way, vx, vy; gboolean changed; GdkTexture *last; double build_ms; GdkTexture *made[2]; GBytes *bytes[2]; int made_zoom; gsize stride; };
G_DEFINE_TYPE (ProbeView, probe_view, GTK_TYPE_WIDGET)

static void
probe_view_snapshot (GtkWidget *widget, GtkSnapshot *s)
{
  ProbeView *v = (ProbeView *) widget;
  gint64 t0 = g_get_monotonic_time ();

  gtk_snapshot_translate (s, &GRAPHENE_POINT_INIT ((float) -v->vx, (float) -v->vy));
  if (v->way == WAY_RENDERER)
    append_by_renderer (s, v->pic, v->zoom, 0);
  else if (v->way == WAY_UPLOAD || v->way == WAY_REUSED)
    {
      int vw = MIN (VIEW_W, gtk_widget_get_width (widget)), vh = MIN (VIEW_H, gtk_widget_get_height (widget)), at = v->vx & 1;
      g_autoptr (GdkTexture) fresh = NULL;

      if (v->made_zoom != v->zoom)
        for (int i = 0; i < 2; i++)
          {
            g_autoptr (GdkTextureDownloader) dl = NULL;

            g_clear_object (&v->made[i]);
            g_clear_pointer (&v->bytes[i], g_bytes_unref);
            v->made[i] = view_indexed (v->pic, v->zoom, 2000 + (i ^ (2000 & 1)), v->vy, vw, vh);
            dl = gdk_texture_downloader_new (v->made[i]);
            gdk_texture_downloader_set_format (dl, GDK_MEMORY_R8G8B8A8);
            v->bytes[i] = gdk_texture_downloader_download_bytes (dl, &v->stride);
          }
      v->made_zoom = v->zoom;
      t0 = g_get_monotonic_time ();
      if (v->way == WAY_UPLOAD)
        fresh = gdk_memory_texture_new (vw, vh, GDK_MEMORY_R8G8B8A8, v->bytes[at], v->stride);
      gtk_snapshot_append_scaled_texture (s, fresh != NULL ? fresh : v->made[at], GSK_SCALING_FILTER_NEAREST,
                                          &GRAPHENE_RECT_INIT ((float) v->vx, (float) v->vy, (float) vw, (float) vh));
    }
  else
    {
      int vw = MIN (VIEW_W, gtk_widget_get_width (widget)), vh = MIN (VIEW_H, gtk_widget_get_height (widget));

      /* A repaint with nothing changed reuses the last texture. */
      if (v->changed || v->last == NULL)
        {
          g_clear_object (&v->last);
          v->last = v->way == WAY_EDGES ? view_by_edges (v->pic, v->zoom, v->vx, v->vy, vw, vh) : view_indexed (v->pic, v->zoom, v->vx, v->vy, vw, vh);
          v->changed = FALSE;
        }
      gtk_snapshot_append_scaled_texture (s, v->last, GSK_SCALING_FILTER_NEAREST, &GRAPHENE_RECT_INIT ((float) v->vx, (float) v->vy, (float) vw, (float) vh));
    }
  v->build_ms = (g_get_monotonic_time () - t0) / 1000.0;
}

static void probe_view_class_init (ProbeViewClass *k) { GTK_WIDGET_CLASS (k)->snapshot = probe_view_snapshot; }
static void probe_view_init (ProbeView *v) {}

static double
cpu_ms (void)
{
  struct timespec t;

  clock_gettime (CLOCK_PROCESS_CPUTIME_ID, &t);
  return t.tv_sec * 1000.0 + t.tv_nsec / 1e6;
}

static int
cmp (const void *a, const void *b)
{
  double d = *(const double *) a - *(const double *) b;

  return d < 0 ? -1 : d > 0;
}

static void
report (const char *what, double *v, int n)
{
  qsort (v, n, sizeof v[0], cmp);
  printf (" %s %.2f / %.2f", what, v[n / 2], v[n - 1]);
  if (strcmp (what, "frame ms") == 0)
    {
      int over_17 = 0, over_20 = 0, over_33 = 0;

      for (int i = 0; i < n; i++)
        {
          over_17 += v[i] > 16.7;
          over_20 += v[i] > 20;
          over_33 += v[i] > 33;
        }
      printf (" (over 16.7 ms %.1f %%, 20 ms %.1f %%, 33 ms %.1f %%)", 100.0 * over_17 / n, 100.0 * over_20 / n, 100.0 * over_33 / n);
    }
}

static gboolean
frames_tick (GtkWidget *widget, GdkFrameClock *clock, gpointer data)
{
  ProbeView *v = (ProbeView *) widget;
  static int round, cfg, frame;
  static gint64 last;
  static double last_cpu, ft[FRAMES], cpu[FRAMES], build[FRAMES];
  gint64 now = g_get_monotonic_time ();
  double c = cpu_ms ();
  const Config *k = &configs[cfg];

  if (frame == 0)
    {
      v->zoom = k->zoom;
      v->way = k->way;
      v->changed = TRUE;
    }
  else if (frame > SKIP)
    {
      ft[frame - SKIP - 1] = (now - last) / 1000.0;
      cpu[frame - SKIP - 1] = c - last_cpu;
      build[frame - SKIP - 1] = v->build_ms;
    }
  last = now;
  last_cpu = c;
  if (k->act == ACT_SCROLL)
    {
      v->vx = 2000 + frame % 2;       /* a new view every frame, by one pixel, back and forth */
      v->changed = TRUE;
    }
  else if (k->act == ACT_PENCIL)
    {
      int x = v->vx * 100 / k->zoom + 20 + frame, y = v->vy * 100 / k->zoom + 20;

      v->pic->px[(gsize) y * v->pic->w + x] = 0xff000000u;
      if (k->way == WAY_RENDERER)
        build_tile (v->pic, x / TILE, y / TILE);
      v->changed = TRUE;
    }
  gtk_widget_queue_draw (widget);
  if (++frame > FRAMES + SKIP)
    {
      printf ("%s round %d %-8s %4d %% %-6s window %d x %d:", G_OBJECT_TYPE_NAME (gtk_native_get_renderer (gtk_widget_get_native (widget))),
              round + 1, way_names[k->way], k->zoom, act_names[k->act], gtk_widget_get_width (widget), gtk_widget_get_height (widget));
      report ("frame ms", ft, FRAMES);
      report ("| build ms", build, FRAMES);
      report ("| process CPU ms", cpu, FRAMES);
      printf ("\n");
      fflush (stdout);
      frame = 0;
      if (++cfg == G_N_ELEMENTS (configs))
        {
          cfg = 0;
          if (++round == ROUNDS)
            exit (0);
        }
    }
  return G_SOURCE_CONTINUE;
}

/* The slow case the budget names: the view's build alone at 3840 x 2160; the screen shows no such view, so no frame is timed. */
static void
report_4k (Picture *pic)
{
  enum { N = 21 };
  static const int zooms[] = { 129, 150 };

  for (guint z = 0; z < G_N_ELEMENTS (zooms); z++)
    {
      double t[N];

      for (int i = 0; i < N; i++)
        {
          double c0 = cpu_ms ();
          GdkTexture *view = view_indexed (pic, zooms[z], 500 + i % 2, 100, 3840, 2160);

          t[i] = cpu_ms () - c0;
          g_object_unref (view);
        }
      printf ("build alone, indexed, 3840 x 2160, %d %%:", zooms[z]);
      report ("CPU ms", t, N);
      printf ("\n");
    }
}

int
main (int argc, char **argv)
{
  gboolean frames = argc > 1 && strcmp (argv[1], "frames") == 0;
  GtkWindow *win;

  gtk_init ();
  win = GTK_WINDOW (gtk_window_new ());
  if (frames)
    {
      ProbeView *v = g_object_new (PROBE_TYPE_VIEW, NULL);
      double l[3];

      v->pic = picture_new (4000, 4000);
      v->vy = 500;
      v->zoom = 100;
      if (getloadavg (l, 3) == 3)
        printf ("load %.2f %.2f %.2f\n", l[0], l[1], l[2]);
      report_4k (v->pic);
      fflush (stdout);
      gtk_window_set_default_size (win, VIEW_W, VIEW_H);
      gtk_window_maximize (win);
      gtk_window_set_child (win, GTK_WIDGET (v));
      gtk_widget_add_tick_callback (GTK_WIDGET (v), frames_tick, NULL, NULL);
      gtk_window_present (win);
    }
  else
    {
      gtk_window_present (win);
      g_timeout_add (500, run_edges, win);
    }
  while (TRUE)
    g_main_context_iteration (NULL, TRUE);
  return 0;
}
