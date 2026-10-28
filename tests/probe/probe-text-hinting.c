/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Probe for the Text tool's rendering choice (feature 0017, the repeated-letter look JOD saw at 800 %): with gray antialiasing, no
   hinting and no rounding (today's settings) every letter of a line sits at a fractional x, so the same letter gets a different 1-bit
   shape at the put-down's threshold (coverage >= half). The probe tries the settings that put every glyph at a whole pixel and says,
   for each:
   - identical: are the repeated letters of a line identical (the share of letters that occur twice or more in the sentence whose
     occurrences all have one shape, and the number of different shapes of the h and of the k in "hhhhhkkkkk"); the pass condition is 100 %;
   - zoom and screen: the layout and the pixels under a changed screen resolution of the font map (the context sets 96 dpi itself, and
     the rasterizing is once at picture scale, so no zoom enters it);
   - wrapping: the number of lines of the sentence in a 300 px wide layout, and its widths, at the changed screen resolution;
   - width: the width of the sentence against today's, per size;
   - emoji: whether the shape of a color emoji (as coverage in an A8 surface, thresholded) is the same shape as today's;
   - underline and strikethrough: whether the line is one straight run of the same rows over the whole text;
   - a font without hinting information (no fpgm table: Ubuntu Sans) and the ones with it.
   It also writes the contact sheets (PNG: thresholded pixels, the rows being the settings), into the directory of its first argument.
   Not a test: it prints what it finds. Usage: probe-text-hinting OUTDIR */

#include <hb-ot.h>
#include <math.h>
#include <pango/pangocairo.h>
#include <stdio.h>
#include <string.h>

#define MAXW 3600
#define MAXH 240

typedef struct
{
  const char *name;
  cairo_hint_metrics_t metrics;
  cairo_hint_style_t style;
  gboolean round;
  cairo_antialias_t aa;
} Setting;

static const Setting settings[] = {
  { "today: gray, metrics off, no rounding, hint none", CAIRO_HINT_METRICS_OFF, CAIRO_HINT_STYLE_NONE, FALSE, CAIRO_ANTIALIAS_GRAY },
  { "gray, metrics on, round, hint none", CAIRO_HINT_METRICS_ON, CAIRO_HINT_STYLE_NONE, TRUE, CAIRO_ANTIALIAS_GRAY },
  { "gray, metrics on, round, hint slight", CAIRO_HINT_METRICS_ON, CAIRO_HINT_STYLE_SLIGHT, TRUE, CAIRO_ANTIALIAS_GRAY },
  { "gray, metrics on, round, hint medium", CAIRO_HINT_METRICS_ON, CAIRO_HINT_STYLE_MEDIUM, TRUE, CAIRO_ANTIALIAS_GRAY },
  { "gray, metrics on, round, hint full", CAIRO_HINT_METRICS_ON, CAIRO_HINT_STYLE_FULL, TRUE, CAIRO_ANTIALIAS_GRAY },
  { "G: antialias none, metrics on, round, hint full", CAIRO_HINT_METRICS_ON, CAIRO_HINT_STYLE_FULL, TRUE, CAIRO_ANTIALIAS_NONE },
  { "H: antialias none, metrics on, round, hint slight", CAIRO_HINT_METRICS_ON, CAIRO_HINT_STYLE_SLIGHT, TRUE, CAIRO_ANTIALIAS_NONE },
  { "I: antialias none, metrics on, round, hint medium", CAIRO_HINT_METRICS_ON, CAIRO_HINT_STYLE_MEDIUM, TRUE, CAIRO_ANTIALIAS_NONE },
};
#define NS G_N_ELEMENTS (settings)

static const char *families[] = { "Ubuntu Sans", "DejaVu Sans", "DejaVu Serif" };
static const char *style_names[] = { "regular", "bold", "italic" };
static const int sizes[] = { 8, 10, 12, 14, 16, 24, 72 };
static const char *SENTENCE = "The quick brown fox jumps over the lazy dog.";
static const char *REPEAT = "hhhhhh gggggg kkkkkk";

/* The antialiasing of the drawing context itself (the lines of an underline and a strikethrough are rectangles it draws), set from the
   setting being tried. */
static cairo_antialias_t cr_aa = CAIRO_ANTIALIAS_DEFAULT;

static PangoContext *
make_context (const Setting *s)
{
  cr_aa = s->aa == CAIRO_ANTIALIAS_NONE ? CAIRO_ANTIALIAS_NONE : CAIRO_ANTIALIAS_DEFAULT;
  PangoContext *ctx = pango_font_map_create_context (pango_cairo_font_map_get_default ());
  cairo_font_options_t *fo = cairo_font_options_create ();

  cairo_font_options_set_antialias (fo, s->aa);
  cairo_font_options_set_hint_style (fo, s->style);
  cairo_font_options_set_hint_metrics (fo, s->metrics);
  pango_cairo_context_set_font_options (ctx, fo);
  pango_cairo_context_set_resolution (ctx, 96);
  pango_context_set_round_glyph_positions (ctx, s->round);
  cairo_font_options_destroy (fo);
  return ctx;
}

static PangoLayout *
make_layout (PangoContext *ctx, const char *family, int style, int pt, const char *text)
{
  PangoLayout *layout = pango_layout_new (ctx);
  PangoFontDescription *fd = pango_font_description_new ();

  pango_font_description_set_family (fd, family);
  pango_font_description_set_size (fd, pt * PANGO_SCALE);
  pango_font_description_set_weight (fd, style == 1 ? PANGO_WEIGHT_BOLD : PANGO_WEIGHT_NORMAL);
  pango_font_description_set_style (fd, style == 2 ? PANGO_STYLE_ITALIC : PANGO_STYLE_NORMAL);
  pango_layout_set_font_description (layout, fd);
  pango_layout_set_text (layout, text, -1);
  pango_font_description_free (fd);
  return layout;
}

/* Draws the layout into an A8 surface and returns the thresholded mask (alpha >= 128), width w, height h (the caller frees). */
static guchar *
mask_of (PangoLayout *layout, int w, int h)
{
  cairo_surface_t *surf = cairo_image_surface_create (CAIRO_FORMAT_A8, w, h);
  cairo_t *cr = cairo_create (surf);
  guchar *m = g_malloc0 ((gsize) w * h), *data;
  int stride;

  cairo_set_antialias (cr, cr_aa);
  cairo_set_source_rgba (cr, 0, 0, 0, 1);
  cairo_move_to (cr, 4, 4);
  pango_cairo_update_layout (cr, layout);
  pango_cairo_show_layout (cr, layout);
  cairo_surface_flush (surf);
  data = cairo_image_surface_get_data (surf);
  stride = cairo_image_surface_get_stride (surf);
  for (int y = 0; y < h; y++)
    for (int x = 0; x < w; x++)
      m[y * w + x] = data[y * stride + x] >= 128;
  cairo_destroy (cr);
  cairo_surface_destroy (surf);
  return m;
}

/* The ink of a mask cropped to its bounding box, as a string key; "" if empty. Also the box's left edge and width. */
static char *
crop_key (const guchar *m, int w, int h, int *left, int *bw)
{
  int x0 = w, x1 = -1, y0 = h, y1 = -1;
  GString *k = g_string_new (NULL);

  for (int y = 0; y < h; y++)
    for (int x = 0; x < w; x++)
      if (m[y * w + x])
        {
          x0 = MIN (x0, x), x1 = MAX (x1, x), y0 = MIN (y0, y), y1 = MAX (y1, y);
        }
  if (x1 >= 0)
    {
      g_string_append_printf (k, "%dx%d:", x1 - x0 + 1, y1 - y0 + 1);
      for (int y = y0; y <= y1; y++)
        for (int x = x0; x <= x1; x++)
          g_string_append_c (k, m[y * w + x] ? '#' : '.');
    }
  *left = x0;
  *bw = x1 - x0 + 1;
  return g_string_free (k, FALSE);
}

/* The mask of only the characters [from, to) (bytes) of the text, the rest being at alpha 1 of 65535 (invisible at the threshold), the
   layout kept (the way the Text tool's passes are made). */
static guchar *
only_mask (PangoLayout *layout, int from, int to, int w, int h)
{
  PangoAttrList *attrs = pango_attr_list_new ();
  PangoAttribute *a = pango_attr_foreground_alpha_new (1);
  PangoAttribute *b = pango_attr_foreground_alpha_new (65535);
  guchar *m;

  a->start_index = 0;
  a->end_index = G_MAXUINT;
  b->start_index = from;
  b->end_index = to;
  pango_attr_list_insert (attrs, a);
  pango_attr_list_insert (attrs, b);
  pango_layout_set_attributes (layout, attrs);
  m = mask_of (layout, w, h);
  pango_layout_set_attributes (layout, NULL);
  pango_attr_list_unref (attrs);
  return m;
}

static double
text_width (PangoLayout *layout)
{
  PangoRectangle r;

  pango_layout_get_extents (layout, NULL, &r);
  return (double) r.width / PANGO_SCALE;
}

/* The distinct shapes of each repeated letter: returns the number of letters (distinct, occurring twice or more) and how many of them have
   one shape for all their occurrences; when `want` is not 0 also the number of shapes of that letter. */
static void
identical_letters (PangoLayout *layout, const char *text, int *letters, int *same, char want, int *want_shapes)
{
  int w = MIN (MAXW, (int) text_width (layout) + 16), h = MIN (MAXH, 200);
  GHashTable *shapes = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, NULL);
  int count[256] = { 0 };
  GPtrArray *keys[256] = { 0 };

  *letters = 0;
  *same = 0;
  if (want_shapes != NULL)
    *want_shapes = 0;
  for (int i = 0; text[i] != '\0'; i++)
    {
      guchar c = (guchar) text[i];
      guchar *m;
      char *key;
      int left, bw;

      if (c == ' ' || c == '.')
        continue;
      m = only_mask (layout, i, i + 1, w, h);
      key = crop_key (m, w, h, &left, &bw);
      g_free (m);
      if (keys[c] == NULL)
        keys[c] = g_ptr_array_new_with_free_func (g_free);
      g_ptr_array_add (keys[c], key);
      count[c]++;
    }
  for (int c = 0; c < 256; c++)
    if (keys[c] != NULL)
      {
        guint distinct = 0;

        g_hash_table_remove_all (shapes);
        for (guint i = 0; i < keys[c]->len; i++)
          if (g_hash_table_add (shapes, g_strdup (g_ptr_array_index (keys[c], i))))
            distinct++;
        if (count[c] >= 2)
          {
            (*letters)++;
            if (distinct == 1)
              (*same)++;
          }
        if (want_shapes != NULL && c == (guchar) want)
          *want_shapes = (int) distinct;
        g_ptr_array_free (keys[c], TRUE);
      }
  g_hash_table_unref (shapes);
}

static gboolean
has_fpgm (PangoContext *ctx, const char *family, int style)
{
  PangoFontDescription *fd = pango_font_description_new ();
  PangoFont *font;
  gboolean r = FALSE;

  pango_font_description_set_family (fd, family);
  pango_font_description_set_size (fd, 12 * PANGO_SCALE);
  pango_font_description_set_weight (fd, style == 1 ? PANGO_WEIGHT_BOLD : PANGO_WEIGHT_NORMAL);
  font = pango_context_load_font (ctx, fd);
  if (font != NULL)
    {
      hb_face_t *face = hb_font_get_face (pango_font_get_hb_font (font));
      hb_blob_t *blob = hb_face_reference_table (face, HB_TAG ('f', 'p', 'g', 'm'));

      r = hb_blob_get_length (blob) > 0;
      hb_blob_destroy (blob);
      g_object_unref (font);
    }
  pango_font_description_free (fd);
  return r;
}

/* The length of the longest unbroken run of set pixels in a row. */
static int
longest_run (const guchar *m, int w, int y)
{
  int best = 0, cur = 0;

  for (int x = 0; x < w; x++)
    {
      cur = m[y * w + x] ? cur + 1 : 0;
      best = MAX (best, cur);
    }
  return best;
}

/* The mask of a layout with its underline (or strikethrough) on. */
static guchar *
line_mask (PangoLayout *layout, gboolean strike, int w, int h)
{
  PangoAttrList *attrs = pango_attr_list_new ();
  guchar *m;

  pango_attr_list_insert (attrs, strike ? pango_attr_strikethrough_new (TRUE) : pango_attr_underline_new (PANGO_UNDERLINE_SINGLE));
  pango_layout_set_attributes (layout, attrs);
  m = mask_of (layout, w, h);
  pango_layout_set_attributes (layout, NULL);
  pango_attr_list_unref (attrs);
  return m;
}

/* Whether the underline (or the strikethrough) of the text is one unbroken run: some row has every column from the text's left edge to
   its right edge set. */
static gboolean
lines_straight (PangoLayout *layout, gboolean strike)
{
  int w = MIN (MAXW, (int) text_width (layout) + 16), h = 200;
  guchar *mt = line_mask (layout, strike, w, h);
  int x1 = 4 + (int) text_width (layout);
  gboolean found = FALSE;

  for (int y = 0; y < h && !found; y++)
    {
      found = longest_run (mt, w, y) >= x1 - 4 - 3;
    }
  g_free (mt);
  return found;
}

/* A color emoji between a and b: its thresholded shape, cropped (key), and where its left edge is. */
static char *
emoji_key (PangoContext *ctx, int pt, int *bw)
{
  PangoLayout *layout = make_layout (ctx, "Ubuntu Sans", 0, pt, "a\xf0\x9f\x98\x80" "b");
  int w = 400, h = 200, left;
  guchar *m = only_mask (layout, 1, 5, w, h);
  char *k = crop_key (m, w, h, &left, bw);

  g_free (m);
  g_object_unref (layout);
  return k;
}

/* TRUE if every glyph of the layout has a whole-pixel advance, no offset, and every run starts on a whole pixel. */
static gboolean
whole_pixel_glyphs (PangoLayout *layout)
{
  PangoLayoutIter *it = pango_layout_get_iter (layout);
  gboolean ok = TRUE;

  do
    {
      PangoLayoutRun *run = pango_layout_iter_get_run_readonly (it);
      PangoRectangle ink, logical;

      if (run == NULL)
        continue;
      pango_layout_iter_get_run_extents (it, &ink, &logical);
      ok &= logical.x % PANGO_SCALE == 0;
      for (int g = 0; g < run->glyphs->num_glyphs; g++)
        {
          PangoGlyphGeometry *geo = &run->glyphs->glyphs[g].geometry;

          ok &= geo->width % PANGO_SCALE == 0 && geo->x_offset == 0 && geo->y_offset == 0;
        }
    }
  while (pango_layout_iter_next_run (it));
  pango_layout_iter_free (it);
  return ok;
}

/* The color emoji drawn into ARGB32 (as the Text tool's colored path would): the number of pixels with alpha >= 128 whose color is not
   a gray, and how many different colors they have. */
static void
emoji_colors (PangoContext *ctx, int pt, int *colored, int *distinct)
{
  PangoLayout *layout = make_layout (ctx, "Ubuntu Sans", 0, pt, "a\xf0\x9f\x98\x80" "b");
  cairo_surface_t *surf = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, 400, 200);
  cairo_t *cr = cairo_create (surf);
  GHashTable *seen = g_hash_table_new (g_direct_hash, g_direct_equal);
  guchar *data;
  int stride;

  cairo_set_source_rgba (cr, 0, 0, 0, 1);
  cairo_move_to (cr, 4, 4);
  pango_cairo_update_layout (cr, layout);
  pango_cairo_show_layout (cr, layout);
  cairo_surface_flush (surf);
  data = cairo_image_surface_get_data (surf);
  stride = cairo_image_surface_get_stride (surf);
  *colored = 0;
  for (int y = 0; y < 200; y++)
    for (int x = 0; x < 400; x++)
      {
        guint32 px = *(guint32 *) (data + y * stride + x * 4);
        int a = px >> 24, r = (px >> 16) & 255, g = (px >> 8) & 255, b = px & 255;

        if (a >= 128 && (abs (r - g) > 8 || abs (g - b) > 8))
          {
            (*colored)++;
            g_hash_table_add (seen, GUINT_TO_POINTER (px));
          }
      }
  *distinct = (int) g_hash_table_size (seen);
  g_hash_table_unref (seen);
  cairo_destroy (cr);
  cairo_surface_destroy (surf);
  g_object_unref (layout);
}

/* The rows (relative to the baseline, 0 being the first row below it) of the first underline row and of the strikethrough row of
   "hhhh" in the layout: the rows with every column set. -1000 if none. */
static void
line_rows (PangoLayout *layout, int *under, int *strike)
{
  int w = MIN (MAXW, (int) text_width (layout) + 16), h = 200;
  int baseline = 4 + (int) floor ((double) pango_layout_get_baseline (layout) / PANGO_SCALE + 0.5);
  int x1 = 4 + (int) text_width (layout);

  for (int k = 0; k < 2; k++)
    {
      guchar *m = line_mask (layout, k == 1, w, h);
      int found = -1000;
      for (int y = 0; y < h && found == -1000; y++)
        {
          if (longest_run (m, w, y) >= x1 - 4 - 3)
            found = y - baseline;
        }
      if (k == 0)
        *under = found;
      else
        *strike = found;
      g_free (m);
    }
}

/* The contact sheet of one family and style: for each size, a block of the strings, a row per setting; the pixels are the thresholded
   mask. Small sizes are magnified by `zoom` (nearest). */
static void
sheet (const char *outdir, int fam, int style, gboolean large)
{
  const int *szs = large ? sizes + 5 : sizes;
  int n = large ? 2 : 5;
  int zoom = large ? 1 : 3, pad = 8;
  int total_h = pad;
  int widths[2] = { 0, 0 };
  cairo_surface_t *surf;
  cairo_t *cr;
  int y = pad, maxw = 0;
  char *path;

  /* sizes first: the sheet's height and width */
  for (int i = 0; i < n; i++)
    for (guint s = 0; s < NS; s++)
      {
        PangoContext *ctx = make_context (&settings[s]);

        for (int t = 0; t < 2; t++)
          {
            PangoLayout *l = make_layout (ctx, families[fam], style, szs[i], t == 0 ? REPEAT : SENTENCE);
            PangoRectangle r;

            pango_layout_get_extents (l, NULL, &r);
            total_h += (r.height / PANGO_SCALE + 8 + 3) * zoom;
            widths[t] = MAX (widths[t], r.width / PANGO_SCALE + 8);
            g_object_unref (l);
          }
        g_object_unref (ctx);
        total_h += pad * zoom;
      }
  maxw = widths[1] * zoom + 2 * pad;
  surf = cairo_image_surface_create (CAIRO_FORMAT_RGB24, MIN (maxw, 16000), MIN (total_h, 16000));
  cr = cairo_create (surf);
  cairo_set_source_rgb (cr, 1, 1, 1);
  cairo_paint (cr);
  for (int i = 0; i < n; i++)
    for (guint s = 0; s < NS; s++)
      {
        PangoContext *ctx = make_context (&settings[s]);

        for (int t = 0; t < 2; t++)
          {
            PangoLayout *l = make_layout (ctx, families[fam], style, szs[i], t == 0 ? REPEAT : SENTENCE);
            PangoRectangle r;
            int w, h;
            guchar *m;

            pango_layout_get_extents (l, NULL, &r);
            w = r.width / PANGO_SCALE + 8;
            h = r.height / PANGO_SCALE + 8;
            m = mask_of (l, w, h);
            for (int py = 0; py < h; py++)
              for (int px = 0; px < w; px++)
                {
                  if (m[py * w + px])
                    cairo_set_source_rgb (cr, 0, 0, 0);
                  else
                    cairo_set_source_rgb (cr, (s % 2) ? 0.96 : 1.0, (s % 2) ? 0.96 : 1.0, (s % 2) ? 0.98 : 1.0);
                  cairo_rectangle (cr, pad + px * zoom, y + py * zoom, zoom, zoom);
                  cairo_fill (cr);
                }
            y += (h + 3) * zoom;
            g_free (m);
            g_object_unref (l);
          }
        y += pad * zoom;
        g_object_unref (ctx);
      }
  path = g_strdup_printf ("%s/sheet-%s-%s-%s.png", outdir, families[fam], style_names[style], large ? "large" : "small");
  for (char *p = path; *p; p++)
    if (*p == ' ')
      *p = '_';
  cairo_surface_write_to_png (surf, path);
  g_print ("wrote %s (%d settings per size, rows in the order of the table)\n", path, (int) NS);
  g_free (path);
  cairo_destroy (cr);
  cairo_surface_destroy (surf);
}

/* The look at a high zoom (JOD's 800 % check): "hhhhhkkkkk" at 12 points, a row per setting, the three styles side by side, one file per
   family, magnified 6 times. */
static void
zoom_sheet (const char *outdir, int fam)
{
  const int zoom = 6, pt = 12;
  int cw = 0, rh = 0;
  cairo_surface_t *surf;
  cairo_t *cr;
  char *path;

  for (int st = 0; st < 3; st++)
    {
      PangoContext *ctx = make_context (&settings[0]);
      PangoLayout *l = make_layout (ctx, families[fam], st, pt, REPEAT);
      PangoRectangle r;

      pango_layout_get_extents (l, NULL, &r);
      cw = MAX (cw, r.width / PANGO_SCALE + 8);
      rh = MAX (rh, r.height / PANGO_SCALE + 8);
      g_object_unref (l);
      g_object_unref (ctx);
    }
  surf = cairo_image_surface_create (CAIRO_FORMAT_RGB24, (cw * 3 + 2) * zoom, (int) NS * (rh + 2) * zoom);
  cr = cairo_create (surf);
  cairo_set_source_rgb (cr, 0.8, 0.2, 0.2);
  cairo_paint (cr);
  for (guint s = 0; s < NS; s++)
    for (int st = 0; st < 3; st++)
      {
        PangoContext *ctx = make_context (&settings[s]);
        PangoLayout *l = make_layout (ctx, families[fam], st, pt, REPEAT);
        guchar *m = mask_of (l, cw, rh);

        for (int py = 0; py < rh; py++)
          for (int px = 0; px < cw; px++)
            {
              cairo_set_source_rgb (cr, m[py * cw + px] ? 0 : 1, m[py * cw + px] ? 0 : 1, m[py * cw + px] ? 0 : 1);
              cairo_rectangle (cr, (st * (cw + 1) + px) * zoom, (int) (s * (rh + 2) + py) * zoom, zoom, zoom);
              cairo_fill (cr);
            }
        g_free (m);
        g_object_unref (l);
        g_object_unref (ctx);
      }
  path = g_strdup_printf ("%s/zoom6-%s.png", outdir, families[fam]);
  for (char *q = path; *q; q++)
    if (*q == ' ')
      *q = '_';
  cairo_surface_write_to_png (surf, path);
  g_print ("wrote %s (rows: today, gray none, gray slight, gray medium, gray full, G, H, I; columns: regular, bold, italic)\n", path);
  g_free (path);
  cairo_destroy (cr);
  cairo_surface_destroy (surf);
}

int
main (int argc, char **argv)
{
  const char *outdir = argc > 1 ? argv[1] : ".";
  PangoFontMap *map = pango_cairo_font_map_get_default ();
  char *base_emoji[7] = { 0 };

  g_print ("== per setting; sizes %s; families Ubuntu Sans, DejaVu Sans, DejaVu Serif; styles regular, bold, italic ==\n", "8 10 12 14 16 24 72");
  for (int f = 0; f < 3; f++)
    {
      PangoContext *c = make_context (&settings[0]);

      g_print ("font %s: hinting tables (fpgm) %s\n", families[f], has_fpgm (c, families[f], 0) ? "present" : "ABSENT");
      g_object_unref (c);
    }
  for (guint s = 0; s < NS; s++)
    {
      int ident_letters = 0, ident_same = 0, combos = 0, hs_ok = 0, gs_ok = 0, ks_ok = 0, h_max = 0, g_max = 0, k_max = 0, straight_u = 0, straight_s = 0,
          full_same = 0, full_total = 0, style_same = 0, style_total = 0, whole_ok = 0, whole_total = 0, line_combos = 0, wrap_same = 0, wrap_total = 0, pix_same = 0, pix_total = 0, emoji_same = 0, emoji_total = 0;
      int fl[3] = { 0 }, fs[3] = { 0 }, em_col = 0, em_dist = 0;
      double wd_sum[7] = { 0 }, wd_max[7] = { 0 };
      int wd_n[7] = { 0 };

      for (int f = 0; f < 3; f++)
        for (int st = 0; st < 3; st++)
          for (int i = 0; i < 7; i++)
            {
              PangoContext *ctx = make_context (&settings[s]);
              PangoLayout *l = make_layout (ctx, families[f], st, sizes[i], SENTENCE);
              PangoLayout *r = make_layout (ctx, families[f], st, sizes[i], REPEAT);
              int letters, same, hs, gs, ks;
              double width = text_width (l);

              if (sizes[i] <= 24)
                {
                  identical_letters (l, SENTENCE, &letters, &same, 0, NULL);
                  ident_letters += letters;
                  ident_same += same;
                  fl[f] += letters;
                  fs[f] += same;
                }
              identical_letters (r, REPEAT, &letters, &same, 'h', &hs);
              identical_letters (r, REPEAT, &letters, &same, 'g', &gs);
              identical_letters (r, REPEAT, &letters, &same, 'k', &ks);
              combos++;
              whole_total++;
              whole_ok += whole_pixel_glyphs (l) && whole_pixel_glyphs (r);
              hs_ok += hs == 1;
              gs_ok += gs == 1;
              ks_ok += ks == 1;
              h_max = MAX (h_max, hs);
              g_max = MAX (g_max, gs);
              k_max = MAX (k_max, ks);
              {
                static double today[3][3][7];

                if (s == 0)
                  today[f][st][i] = width;
                else if (today[f][st][i] > 0)
                  {
                    double d = 100.0 * (width - today[f][st][i]) / today[f][st][i];

                    wd_sum[i] += d;
                    wd_n[i]++;
                    if (fabs (d) > fabs (wd_max[i]))
                      wd_max[i] = d;
                  }
              }
              if (sizes[i] <= 24)
                {
                  gboolean u = lines_straight (r, FALSE), k = lines_straight (r, TRUE);

                  line_combos++;
                  straight_u += u;
                  straight_s += k;
                }
              /* screen independence: the font map's resolution changed (our context sets 96 dpi itself) */
              {
                int w = MIN (MAXW, (int) width + 16), h = 200;
                guchar *m1 = mask_of (l, w, h);
                PangoLayout *l2;
                guchar *m2;
                PangoContext *ctx2;
                int lines1, lines2;
                double w1, w2;

                pango_layout_set_width (l, 300 * PANGO_SCALE);
                lines1 = pango_layout_get_line_count (l);
                w1 = text_width (l);
                pango_layout_set_width (l, -1);
                pango_cairo_font_map_set_resolution (PANGO_CAIRO_FONT_MAP (map), 192);
                ctx2 = make_context (&settings[s]);
                l2 = make_layout (ctx2, families[f], st, sizes[i], SENTENCE);
                m2 = mask_of (l2, w, h);
                pango_layout_set_width (l2, 300 * PANGO_SCALE);
                lines2 = pango_layout_get_line_count (l2);
                w2 = text_width (l2);
                pango_cairo_font_map_set_resolution (PANGO_CAIRO_FONT_MAP (map), 96);
                {
                  static guchar *ref[3][3][7];
                  static int refw[3][3][7];

                  static guchar *ref4[3][3][7];

                  if (s == 4)
                    ref4[f][st][i] = g_memdup2 (m1, (gsize) w * h);
                  else if (s > 4 && ref4[f][st][i] != NULL)
                    {
                      full_total++;
                      full_same += memcmp (ref4[f][st][i], m1, (gsize) w * h) == 0;
                    }
                  if (s == 1)
                    {
                      ref[f][st][i] = g_memdup2 (m1, (gsize) w * h);
                      refw[f][st][i] = w;
                    }
                  else if (s > 1 && ref[f][st][i] != NULL)
                    {
                      style_total++;
                      style_same += refw[f][st][i] == w && memcmp (ref[f][st][i], m1, (gsize) w * h) == 0;
                    }
                }
                pix_total++;
                pix_same += memcmp (m1, m2, (gsize) w * h) == 0;
                wrap_total++;
                wrap_same += lines1 == lines2 && w1 == w2;
                g_free (m1);
                g_free (m2);
                g_object_unref (l2);
                g_object_unref (ctx2);
              }
              /* the emoji: the same shape as today's (cropped) */
              if (f == 0 && st == 0)
                {
                  int bw;
                  char *k = emoji_key (ctx, sizes[i], &bw);

                  if (s == 0)
                    base_emoji[i] = k;
                  else
                    {
                      emoji_total++;
                      emoji_same += strcmp (k, base_emoji[i]) == 0;
                      g_free (k);
                    }
                }
              g_object_unref (l);
              g_object_unref (r);
              g_object_unref (ctx);
            }
      g_print ("\n[%u] %s\n", s, settings[s].name);
      g_print ("  identical letters (sentence, sizes 8-24, 3 families x 3 styles): %d of %d letters have one shape for all occurrences (%.0f %%)\n",
               ident_same, ident_letters, ident_letters ? 100.0 * ident_same / ident_letters : 0.0);
      g_print ("  by font: Ubuntu Sans (no fpgm) %d of %d, DejaVu Sans %d of %d, DejaVu Serif %d of %d\n", fs[0], fl[0], fs[1], fl[1], fs[2], fl[2]);
      g_print ("  \"hhhhhh gggggg kkkkkk\": h has one shape in %d of %d combinations (worst %d shapes), g in %d (worst %d), k in %d (worst %d)\n", hs_ok, combos, h_max,
               gs_ok, g_max, ks_ok, k_max);
      g_print ("  screen resolution of the font map 96 -> 192: the pixels of the sentence equal in %d of %d, the wrap (lines at 300 px, width) equal in %d of %d\n",
               pix_same, pix_total, wrap_same, wrap_total);
      if (s > 0)
        {
          g_print ("  width of the sentence against today's, mean / largest by size (%%):");
          for (int i = 0; i < 7; i++)
            g_print (" %dpt %+.1f/%+.1f", sizes[i], wd_n[i] ? wd_sum[i] / wd_n[i] : 0.0, wd_max[i]);
          g_print ("\n");
          g_print ("  emoji (Ubuntu Sans regular, the a<emoji>b of sizes 8-72): the shape equal to today's in %d of %d\n", emoji_same, emoji_total);
        }
      if (s > 1)
        g_print ("  the pixels of the sentence equal to those of the gray 'hint none' row: %d of %d\n", style_same, style_total);
      {
        PangoContext *c2 = make_context (&settings[s]);
        PangoLayout *l12 = make_layout (c2, "DejaVu Sans", 0, 12, REPEAT);
        int under, strike;

        emoji_colors (c2, 24, &em_col, &em_dist);
        line_rows (l12, &under, &strike);
        if (s > 4)
        g_print ("  the pixels of the sentence equal to those of the gray 'hint full' row (same grid-fitting, a threshold in place of 1-bit): %d of %d\n", full_same, full_total);
      g_print ("  glyph geometry (advances, offsets and run starts all whole pixels): %d of %d layouts\n", whole_ok, whole_total);
        g_print ("  colour emoji at 24 pt drawn into ARGB32: %d coloured pixels, %d different colours\n", em_col, em_dist);
        g_print ("  DejaVu Sans 12 pt: the first full row of the underline is %d rows from the baseline, of the strikethrough %d\n", under,
                 strike);
        g_object_unref (l12);
        g_object_unref (c2);
      }
      g_print ("  underline one straight run in %d of %d, strikethrough in %d of %d (sizes 8-24)\n", straight_u, line_combos, straight_s,
               line_combos);
    }
  g_print ("\n== contact sheets ==\n");
  for (int f = 0; f < 3; f++)
    zoom_sheet (outdir, f);
  for (int f = 0; f < 3; f++)
    for (int st = 0; st < 3; st++)
      {
        sheet (outdir, f, st, FALSE);
        sheet (outdir, f, st, TRUE);
      }
  return 0;
}
