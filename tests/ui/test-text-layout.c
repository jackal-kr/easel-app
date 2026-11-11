/* SPDX-License-Identifier: GPL-3.0-or-later */
/* 0040 (JOD's note b, the Plan's risks 1 and 2): Sharp and Smooth lay text out alike, glyph by glyph, line by line, on the 40 font and
   size cases of docs/reference/proto-0040/same_layout.py; and a Smooth context draws partial coverage where Sharp draws none. Pango and
   fontconfig only, no display. A font not installed is skipped; none installed at all skips the test (77). */

#include <string.h>

#include "text-render.h"

/* "... Ünïcödé — ffi fl; WAVE Tò Ty." in UTF-8, each escape ended so the next letter is not read into it */
static const char *const TEXT = "The quick brown fox jumps over the lazy dog, 0123456789. \xc3\x9c" "n\xc3\xaf" "c\xc3\xb6" "d\xc3\xa9"
                                " \xe2\x80\x94 ffi fl; WAVE T\xc3\xb2 Ty.";
static int ran;

static EaselTextBuffer *
buffer (const char *family, int size, uint8_t flags)
{
  EaselTextStyle st = { family, size, flags, 0xFF000000u };
  EaselTextBuffer *b = easel_text_buffer_new (&st);

  g_assert_nonnull (b);
  g_assert_true (easel_text_buffer_insert (b, TEXT, strlen (TEXT), &st, EASEL_TEXT_EDIT_TYPING));
  return b;
}

static void
same_layout (void)
{
  static const struct
  {
    const char *family;
    uint8_t flags;
  } fonts[] = { { "DejaVu Sans", 0 }, { "DejaVu Serif", 0 }, { "Ubuntu Sans", 0 }, { "DejaVu Sans", EASEL_TEXT_BOLD },
                { "DejaVu Serif", EASEL_TEXT_ITALIC } };
  static const int sizes[] = { 6, 8, 11, 12, 14, 24, 37, 72 };

  for (guint f = 0; f < G_N_ELEMENTS (fonts); f++)
    {
      if (easel_text_render_find_family (fonts[f].family) == NULL)
        {
          g_test_message ("%s is not installed: its cases skipped", fonts[f].family);
          continue;
        }
      for (guint s = 0; s < G_N_ELEMENTS (sizes); s++)
        {
          EaselTextBuffer *b = buffer (fonts[f].family, sizes[s], fonts[f].flags);
          PangoLayout *sharp = easel_text_render_layout_for (b, 180, FALSE), *smooth = easel_text_render_layout_for (b, 180, TRUE);
          int ws, hs, wm, hm;

          g_assert_cmpint (pango_layout_get_line_count (sharp), ==, pango_layout_get_line_count (smooth));
          pango_layout_get_pixel_size (sharp, &ws, &hs);
          pango_layout_get_pixel_size (smooth, &wm, &hm);
          g_assert_cmpint (ws, ==, wm);
          g_assert_cmpint (hs, ==, hm);
          for (int i = 0; i <= (int) strlen (TEXT); i++)
            {
              PangoRectangle a, m;

              pango_layout_index_to_pos (sharp, i, &a);
              pango_layout_index_to_pos (smooth, i, &m);
              if (a.x != m.x || a.y != m.y || a.width != m.width)
                g_test_message ("%s %d: byte %d at %d,%d (%d) with Sharp, %d,%d (%d) with Smooth", fonts[f].family, sizes[s], i, a.x,
                                a.y, a.width, m.x, m.y, m.width);
              g_assert_cmpint (a.x, ==, m.x);
              g_assert_cmpint (a.y, ==, m.y);
              g_assert_cmpint (a.width, ==, m.width);
            }
          g_object_unref (sharp);
          g_object_unref (smooth);
          easel_text_buffer_free (b);
          ran++;
        }
    }
}

/* The coverage a layout's text draws in an 8-bit surface: how many pixels are partial (neither 0 nor 255). */
static int
partial_pixels (PangoLayout *layout)
{
  cairo_surface_t *a8 = cairo_image_surface_create (CAIRO_FORMAT_A8, 200, 120);
  cairo_t *cr = cairo_create (a8);
  const unsigned char *d;
  int stride, n = 0;

  pango_cairo_show_layout (cr, layout);
  cairo_destroy (cr);
  cairo_surface_flush (a8);
  d = cairo_image_surface_get_data (a8);
  stride = cairo_image_surface_get_stride (a8);
  for (int y = 0; y < 120; y++)
    for (int x = 0; x < 200; x++)
      n += d[y * stride + x] > 0 && d[y * stride + x] < 255;
  cairo_surface_destroy (a8);
  return n;
}

/* Risk 1: the context's options decide the drawing (Pango's context options override the surface's): Sharp draws no partial pixel,
   Smooth draws many. */
static void
smooth_context_draws_partial (void)
{
  EaselTextBuffer *b = buffer ("DejaVu Sans", 24, 0);
  PangoLayout *sharp = easel_text_render_layout_for (b, 180, FALSE), *smooth = easel_text_render_layout_for (b, 180, TRUE);

  if (easel_text_render_find_family ("DejaVu Sans") == NULL)
    {
      g_test_skip ("DejaVu Sans is not installed");
      return;
    }
  g_assert_cmpint (partial_pixels (sharp), ==, 0);
  g_assert_cmpint (partial_pixels (smooth), >, 100);
  g_object_unref (sharp);
  g_object_unref (smooth);
  easel_text_buffer_free (b);
}

/* The ink laid over white, as the put-down lays it (easel_text_ink_value), into a w x h picture from (0, 0). */
static uint32_t *
laid_over_white (const EaselTextInk *ink, int w, int h)
{
  uint32_t *px = g_new (uint32_t, (gsize) w * h);

  for (int y = 0; y < h; y++)
    for (int x = 0; x < w; x++)
      px[(gsize) y * w + x] = easel_text_ink_value (ink, x, y, 0xFFFFFFFFu, 0, 0, w, h, FALSE, 0);
  return px;
}

/* 0040 AS-3, JOD's note a (REQ-0040-04, REQ-0040-08): black text on white with Smooth: every changed pixel is black or a gray with
   R = G = B (no colored, subpixel fringe), and there are grays; in "l   l   l" every "l" has the same pixels (the spaces keep a soft edge
   out of the next letter's place). Invariants only, no stored pixels (JOD's note c). */
static void
smooth_ink_gray_and_same_letters (void)
{
  static const char *const texts[2] = { "Hello", "l   l   l" };

  if (easel_text_render_find_family ("DejaVu Sans") == NULL)
    {
      g_test_skip ("DejaVu Sans is not installed");
      return;
    }
  for (int t = 0; t < 2; t++)
    {
      EaselTextStyle st = { "DejaVu Sans", 24, 0, 0xFF000000u };
      EaselTextBuffer *b = easel_text_buffer_new (&st);
      PangoLayout *layout;
      EaselTextInk ink;
      uint32_t *px;
      int w = 300, h = 60, gray = 0;

      g_assert_true (easel_text_buffer_insert (b, texts[t], strlen (texts[t]), &st, EASEL_TEXT_EDIT_TYPING));
      layout = easel_text_render_layout (b, w);
      g_assert_true (easel_text_render_ink (layout, b, 0, 0, SIZE_MAX, TRUE, &ink));
      g_assert_null (ink.mask);
      g_assert_cmpuint (ink.n_layers, ==, 1);
      px = laid_over_white (&ink, w, h);
      for (int i = 0; i < w * h; i++)
        {
          uint32_t p = px[i];
          unsigned r = p >> 16 & 255, g = p >> 8 & 255, bl = p & 255;

          if (p == 0xFFFFFFFFu)
            continue;
          if (r != g || g != bl || p >> 24 != 255)
            g_test_message ("\"%s\": pixel %d is %08X, not a gray", texts[t], i, p);
          g_assert_cmpuint (r, ==, g);
          g_assert_cmpuint (g, ==, bl);
          g_assert_cmphex (p >> 24, ==, 255);
          gray += p != 0xFF000000u;
        }
      g_assert_cmpint (gray, >, 20);
      if (t == 1)
        {
          /* each "l": from one pixel before its place to one after its advance, every row */
          PangoRectangle first, other;

          pango_layout_index_to_pos (layout, 0, &first);
          for (int k = 4; k <= 8; k += 4)
            {
              pango_layout_index_to_pos (layout, k, &other);
              for (int y = 0; y < h; y++)
                for (int dx = -1; dx <= first.width / PANGO_SCALE; dx++)
                  g_assert_cmphex (px[(gsize) y * w + first.x / PANGO_SCALE + dx + 1], ==, px[(gsize) y * w + other.x / PANGO_SCALE + dx + 1]);
            }
        }
      g_free (px);
      easel_text_render_ink_free (&ink);
      g_object_unref (layout);
      easel_text_buffer_free (b);
    }
}

int
main (int argc, char **argv)
{
  g_test_init (&argc, &argv, NULL);
  g_test_add_func ("/0040/same_layout", same_layout);
  g_test_add_func ("/0040/smooth_context_draws_partial", smooth_context_draws_partial);
  g_test_add_func ("/0040/smooth_ink_gray_and_same_letters", smooth_ink_gray_and_same_letters);
  {
    int r = g_test_run ();

    if (r == 0 && ran == 0)
      return 77;                                              /* no font of the cases installed */
    return r;
  }
}
