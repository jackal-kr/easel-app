/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Probe of feature 0017's risks R1 to R3 (the Plan of docs/specs/0017-text.md): how Pango and Cairo draw text into an A8 surface at
   picture scale, and whether the planned rasterizing holds. Not a test: it prints what it finds.
   R1  gray antialiasing on A8 gives each pixel its coverage; "at least half" is alpha >= 128; the same text gives the same pixels
       twice; the letters it gives at 8, 12 and 72 points look like letters (printed as text art at 12).
   R2  one pass per style run, the other runs at foreground alpha 1 (of 65535): the run's pixels are where they are in a pass of the
       whole text (positions kept), the hidden runs add no pixel at the threshold, and a run's underline and strikethrough come with it.
   R3  a color emoji drawn into A8 gives its shape as coverage (T001: a solid silhouette, so JOD chose its own colors);
   R3b the same emoji drawn into ARGB32: the pixels with alpha >= 128 and their colors (unpremultiplied), and whether HarfBuzz's color
       tables tell a color font (Noto Color Emoji) from a plain one (DejaVu Sans). */

#include <hb-ot.h>
#include <pango/pangocairo.h>
#include <stdio.h>
#include <string.h>

#define W 400
#define H 140

static PangoLayout *
make_layout (cairo_t *cr, const char *family, int pt, const char *text)
{
  PangoContext *ctx = pango_cairo_create_context (cr);
  cairo_font_options_t *fo = cairo_font_options_create ();
  PangoLayout *layout;
  PangoFontDescription *fd = pango_font_description_new ();

  cairo_font_options_set_antialias (fo, CAIRO_ANTIALIAS_GRAY);
  cairo_font_options_set_hint_style (fo, CAIRO_HINT_STYLE_NONE);
  cairo_font_options_set_hint_metrics (fo, CAIRO_HINT_METRICS_OFF);
  pango_cairo_context_set_font_options (ctx, fo);
  pango_cairo_context_set_resolution (ctx, 96);
  pango_context_set_round_glyph_positions (ctx, FALSE);
  layout = pango_layout_new (ctx);
  pango_font_description_set_family (fd, family);
  pango_font_description_set_size (fd, pt * PANGO_SCALE);
  pango_layout_set_font_description (layout, fd);
  pango_layout_set_text (layout, text, -1);
  pango_font_description_free (fd);
  cairo_font_options_destroy (fo);
  g_object_unref (ctx);
  return layout;
}

/* Draws the layout into a fresh A8 surface and returns the mask of pixels with alpha >= 128 (W x H). */
static unsigned char *
mask_of (PangoLayout *layout, cairo_surface_t *surface)
{
  cairo_t *cr = cairo_create (surface);
  unsigned char *m = g_malloc0 (W * H), *data;
  int stride;

  cairo_set_operator (cr, CAIRO_OPERATOR_CLEAR);
  cairo_paint (cr);
  cairo_set_operator (cr, CAIRO_OPERATOR_OVER);
  cairo_set_source_rgba (cr, 0, 0, 0, 1);
  cairo_move_to (cr, 2, 2);
  pango_cairo_update_layout (cr, layout);
  pango_cairo_show_layout (cr, layout);
  cairo_surface_flush (surface);
  data = cairo_image_surface_get_data (surface);
  stride = cairo_image_surface_get_stride (surface);
  for (int y = 0; y < H; y++)
    for (int x = 0; x < W; x++)
      m[y * W + x] = data[y * stride + x] >= 128;
  cairo_destroy (cr);
  return m;
}

static int
count (const unsigned char *m)
{
  int n = 0;

  for (int i = 0; i < W * H; i++)
    n += m[i];
  return n;
}

static void
art (const unsigned char *m, int x0, int y0, int w, int h)
{
  for (int y = y0; y < y0 + h; y++)
    {
      for (int x = x0; x < x0 + w; x++)
        putchar (m[y * W + x] ? '#' : '.');
      putchar ('\n');
    }
}

/* Sets foreground alpha `alpha` on [from, to). */
static void
alpha_range (PangoAttrList *attrs, int from, int to, guint16 alpha)
{
  PangoAttribute *a = pango_attr_foreground_alpha_new (alpha);

  a->start_index = from;
  a->end_index = to;
  pango_attr_list_change (attrs, a);
}

int
main (void)
{
  cairo_surface_t *surface = cairo_image_surface_create (CAIRO_FORMAT_A8, W, H);
  cairo_t *cr = cairo_create (surface);

  /* R1 */
  for (int k = 0; k < 3; k++)
    {
      int pt = (int[]) { 8, 12, 72 }[k];
      PangoLayout *l = make_layout (cr, "DejaVu Sans", pt, pt == 72 ? "Hg" : "Hamburg 0 Ag");
      unsigned char *a = mask_of (l, surface), *b = mask_of (l, surface);
      int partial = 0;
      unsigned char *data = cairo_image_surface_get_data (surface);
      int stride = cairo_image_surface_get_stride (surface);

      for (int y = 0; y < H; y++)
        for (int x = 0; x < W; x++)
          partial += data[y * stride + x] > 0 && data[y * stride + x] < 255;
      printf ("R1 DejaVu Sans %d pt: %d pixels at >= 128, same twice: %s, partial-coverage pixels in the A8: %d\n", pt, count (a),
              memcmp (a, b, W * H) == 0 ? "yes" : "NO", partial);
      if (pt == 12)
        art (a, 0, 0, 100, 20);
      g_free (a);
      g_free (b);
      g_object_unref (l);
    }

  /* R2: "abc" plain, "XYZ" underlined and struck through. */
  {
    const char *text = "abcXYZ";
    PangoLayout *l = make_layout (cr, "DejaVu Sans", 12, text);
    PangoAttrList *attrs = pango_attr_list_new ();
    PangoAttribute *u = pango_attr_underline_new (PANGO_UNDERLINE_SINGLE), *s = pango_attr_strikethrough_new (TRUE);
    unsigned char *whole, *pass1, *pass2;
    int extra1 = 0, missing1 = 0, overlap = 0, union_diff = 0;

    u->start_index = 3; u->end_index = 6;
    s->start_index = 3; s->end_index = 6;
    pango_attr_list_insert (attrs, u);
    pango_attr_list_insert (attrs, s);
    pango_layout_set_attributes (l, attrs);
    whole = mask_of (l, surface);
    alpha_range (attrs, 3, 6, 1);                       /* pass of run 1: run 2 hidden */
    pango_layout_set_attributes (l, attrs);
    pass1 = mask_of (l, surface);
    alpha_range (attrs, 3, 6, 65535);
    alpha_range (attrs, 0, 3, 1);                       /* pass of run 2: run 1 hidden */
    pango_layout_set_attributes (l, attrs);
    pass2 = mask_of (l, surface);
    for (int i = 0; i < W * H; i++)
      {
        extra1 += pass1[i] && !whole[i];
        overlap += pass1[i] && pass2[i];
        union_diff += (pass1[i] || pass2[i]) != whole[i];
      }
    for (int y = 0; y < H; y++)
      for (int x = 0; x < 34; x++)
        missing1 += whole[y * W + x] && !pass1[y * W + x];
    printf ("R2 whole %d px; run 1 pass %d px (not in whole: %d; in whole left of x 34 but not in pass: %d); run 2 pass %d px;"
            " both passes on one pixel: %d; union differs from whole on %d px\n", count (whole), count (pass1), extra1, missing1,
            count (pass2), overlap, union_diff);
    printf ("R2 run 2 pass (underline and strikethrough should show under and through XYZ):\n");
    art (pass2, 20, 0, 60, 20);
    g_free (whole); g_free (pass1); g_free (pass2);
    pango_attr_list_unref (attrs);
    g_object_unref (l);
  }

  /* R3 */
  {
    PangoLayout *l = make_layout (cr, "Noto Color Emoji", 24, "\xF0\x9F\x98\x80");      /* U+1F600 */
    unsigned char *m = mask_of (l, surface);

    printf ("R3 Noto Color Emoji 24 pt U+1F600: %d pixels at >= 128\n", count (m));
    art (m, 0, 0, 40, 36);
    g_free (m);
    g_object_unref (l);
  }
  /* R3b */
  {
    cairo_surface_t *argb = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, 40, 40);
    cairo_t *c2 = cairo_create (argb);
    PangoLayout *l = make_layout (c2, "Noto Color Emoji", 24, "\xF0\x9F\x98\x80");
    unsigned char *d;
    int stride, kept = 0, colors = 0;
    guint32 seen[64];
    const char *fams[] = { "Noto Color Emoji", "DejaVu Sans" };

    cairo_move_to (c2, 2, 2);
    pango_cairo_show_layout (c2, l);
    cairo_surface_flush (argb);
    d = cairo_image_surface_get_data (argb);
    stride = cairo_image_surface_get_stride (argb);
    printf ("R3b ARGB, pixels with alpha >= 128 as the first letter of their color (y yellow, b brown/dark, o other):\n");
    for (int y = 0; y < 36; y++)
      {
        for (int x = 0; x < 40; x++)
          {
            guint32 px = *(guint32 *) (d + y * stride + x * 4);
            int a = px >> 24, r, g, b;
            char ch = '.';

            if (a >= 128)
              {
                r = ((px >> 16) & 255) * 255 / a;
                g = ((px >> 8) & 255) * 255 / a;
                b = (px & 255) * 255 / a;
                kept++;
                ch = (r > 200 && g > 150 && b < 120) ? 'y' : (r < 140 && g < 100) ? 'b' : 'o';
                if (colors < 64)
                  {
                    guint32 c = (guint32) (r << 16 | g << 8 | b);
                    int k;

                    for (k = 0; k < colors && seen[k] != c; k++)
                      ;
                    if (k == colors)
                      seen[colors++] = c;
                  }
              }
            putchar (ch);
          }
        putchar ('\n');
      }
    printf ("R3b %d pixels kept, at least %d colors\n", kept, colors);
    for (int f = 0; f < 2; f++)
      {
        PangoLayout *fl = make_layout (c2, fams[f], 24, f == 0 ? "\xF0\x9F\x98\x80" : "A");     /* a glyph the font has */
        PangoLayoutIter *it = pango_layout_get_iter (fl);
        PangoLayoutRun *run = pango_layout_iter_get_run_readonly (it);
        hb_font_t *hb = pango_font_get_hb_font (run->item->analysis.font);
        hb_face_t *face = hb_font_get_face (hb);

        printf ("R3b %s: color tables png %d, layers %d, svg %d (a color font: %s)\n", fams[f], hb_ot_color_has_png (face),
                hb_ot_color_has_layers (face), hb_ot_color_has_svg (face),
                hb_ot_color_has_png (face) || hb_ot_color_has_layers (face) || hb_ot_color_has_svg (face) ? "yes" : "no");
        pango_layout_iter_free (it);
        g_object_unref (fl);
      }
    g_object_unref (l);
    cairo_destroy (c2);
    cairo_surface_destroy (argb);
  }
  cairo_destroy (cr);
  cairo_surface_destroy (surface);
  return 0;
}
