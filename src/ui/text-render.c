/* SPDX-License-Identifier: GPL-3.0-or-later */

#include "text-render.h"

#include <hb-ot.h>
#include <math.h>

PangoContext *
easel_text_render_context (void)
{
  return easel_text_render_context_for (FALSE);
}

PangoContext *
easel_text_render_context_for (gboolean smooth)
{
  PangoContext *ctx = pango_font_map_create_context (pango_cairo_font_map_get_default ());
  cairo_font_options_t *fo = cairo_font_options_create ();

  /* 1-bit glyphs on the grid (REQ-0017-28): no antialiasing, the outlines fitted to the pixels, the metrics and the glyph positions in
     whole pixels, so the same letter is the same bitmap wherever it falls in a line; the resolution is fixed, so the layout does not
     depend on the screen (the probe `probe-text-hinting`, its setting G). Smooth (0040) changes the antialiasing only, to gray (never
     subpixel: REQ-0040-04), so it lays text out alike (`text-smooth-layout`). The context's options decide the drawing: Pango's override
     the surface's. */
  cairo_font_options_set_antialias (fo, smooth ? CAIRO_ANTIALIAS_GRAY : CAIRO_ANTIALIAS_NONE);
  cairo_font_options_set_hint_style (fo, CAIRO_HINT_STYLE_FULL);
  cairo_font_options_set_hint_metrics (fo, CAIRO_HINT_METRICS_ON);
  pango_cairo_context_set_font_options (ctx, fo);
  pango_cairo_context_set_resolution (ctx, 96);
  pango_context_set_round_glyph_positions (ctx, TRUE);
  cairo_font_options_destroy (fo);
  return ctx;
}

PangoFontDescription *
easel_text_render_font (const EaselTextStyle *style)
{
  PangoFontDescription *fd = pango_font_description_new ();

  pango_font_description_set_family (fd, style->family);
  pango_font_description_set_size (fd, style->size_pt * PANGO_SCALE);
  pango_font_description_set_weight (fd, (style->flags & EASEL_TEXT_BOLD) ? PANGO_WEIGHT_BOLD : PANGO_WEIGHT_NORMAL);
  pango_font_description_set_style (fd, (style->flags & EASEL_TEXT_ITALIC) ? PANGO_STYLE_ITALIC : PANGO_STYLE_NORMAL);
  return fd;
}

void
easel_text_render_metrics (const EaselTextStyle *style, int *line_height, int *twenty_zeros)
{
  PangoContext *ctx = easel_text_render_context ();
  PangoLayout *layout = pango_layout_new (ctx);
  PangoFontDescription *fd = easel_text_render_font (style);
  PangoRectangle logical;

  pango_layout_set_font_description (layout, fd);
  pango_layout_set_text (layout, "0", -1);
  pango_layout_get_extents (layout, NULL, &logical);
  *line_height = (int) ceil ((double) logical.height / PANGO_SCALE);
  *twenty_zeros = (int) ceil (20.0 * logical.width / PANGO_SCALE);
  pango_font_description_free (fd);
  g_object_unref (layout);
  g_object_unref (ctx);
}

const char *
easel_text_render_find_family (const char *name)
{
  PangoFontFamily **families = NULL;
  int n = 0;
  const char *found = NULL;

  pango_font_map_list_families (pango_cairo_font_map_get_default (), &families, &n);
  for (int i = 0; i < n && found == NULL; i++)
    if (g_ascii_strcasecmp (pango_font_family_get_name (families[i]), name) == 0)
      found = pango_font_family_get_name (families[i]);       /* owned by the font map, which lives as long as the app */
  g_free (families);
  return found;
}

gboolean
easel_text_render_is_generic (const char *name)
{
  static const char *generic[] = { "sans", "sans-serif", "serif", "monospace", "system-ui", "cursive", "fantasy", "emoji", "math",
                                   "fangsong" };

  for (guint i = 0; name != NULL && i < G_N_ELEMENTS (generic); i++)
    if (g_ascii_strcasecmp (name, generic[i]) == 0)
      return TRUE;
  return FALSE;
}

const char *
easel_text_render_resolve_family (const char *name)
{
  PangoContext *ctx;
  PangoFontDescription *fd;
  PangoFont *font;
  const char *found = NULL;

  if (!easel_text_render_is_generic (name))
    return name;
  /* fontconfig's choice for the name: the font it loads, by the family that font describes */
  ctx = easel_text_render_context ();
  fd = pango_font_description_new ();
  pango_font_description_set_family (fd, name);
  pango_font_description_set_size (fd, 12 * PANGO_SCALE);
  font = pango_context_load_font (ctx, fd);
  if (font != NULL)
    {
      PangoFontDescription *got = pango_font_describe (font);

      found = easel_text_render_find_family (pango_font_description_get_family (got));
      pango_font_description_free (got);
      g_object_unref (font);
    }
  pango_font_description_free (fd);
  g_object_unref (ctx);
  return found != NULL && !easel_text_render_is_generic (found) ? found : name;
}

/* The attributes of the runs from byte `from` to `to` of the text, all of them at full alpha when `only` is FALSE, else those outside
   [only_from, only_to) at alpha 1 (of 65535), which leaves them out of a pass while the layout stays the same (R2). */
static PangoAttrList *
run_attributes (const EaselTextBuffer *b, gboolean only, size_t only_from, size_t only_to)
{
  PangoAttrList *attrs = pango_attr_list_new ();
  const EaselTextRun *runs;
  size_t n, len;

  easel_text_buffer_text (b, &len);
  runs = easel_text_buffer_runs (b, &n);
  for (size_t i = 0; i < n; i++)
    {
      const EaselTextStyle *s = &runs[i].style;
      guint start = (guint) runs[i].start, end = (guint) (i + 1 < n ? runs[i + 1].start : len);
      PangoFontDescription *fd = easel_text_render_font (s);
      PangoAttribute *a[5];
      guint16 alpha = !only || (start >= only_from && end <= only_to) ? 65535 : 1;

      a[0] = pango_attr_font_desc_new (fd);
      a[1] = pango_attr_underline_new ((s->flags & EASEL_TEXT_UNDERLINE) ? PANGO_UNDERLINE_SINGLE : PANGO_UNDERLINE_NONE);
      a[2] = pango_attr_strikethrough_new ((s->flags & EASEL_TEXT_STRIKE) != 0);
      a[3] = pango_attr_foreground_new ((guint16) (((s->color >> 16) & 255) * 257), (guint16) (((s->color >> 8) & 255) * 257),
                                        (guint16) ((s->color & 255) * 257));
      a[4] = pango_attr_foreground_alpha_new (alpha);
      for (int k = 0; k < 5; k++)
        {
          a[k]->start_index = start;
          a[k]->end_index = end;
          pango_attr_list_insert (attrs, a[k]);
        }
      pango_font_description_free (fd);
    }
  return attrs;
}

PangoLayout *
easel_text_render_layout (const EaselTextBuffer *b, int width)
{
  return easel_text_render_layout_for (b, width, FALSE);
}

PangoLayout *
easel_text_render_layout_for (const EaselTextBuffer *b, int width, gboolean smooth)
{
  PangoContext *ctx = easel_text_render_context_for (smooth);
  PangoLayout *layout = pango_layout_new (ctx);
  EaselTextStyle st;
  PangoFontDescription *fd;
  PangoAttrList *attrs;
  size_t len;
  const char *text = easel_text_buffer_text (b, &len);

  /* The font of an empty text, and of a line with no character: the style at the caret (REQ-0017-26). */
  easel_text_buffer_style_at_caret (b, FALSE, &st);
  fd = easel_text_render_font (&st);
  pango_layout_set_font_description (layout, fd);
  pango_font_description_free (fd);
  pango_layout_set_text (layout, text, (int) len);
  attrs = run_attributes (b, FALSE, 0, 0);
  pango_layout_set_attributes (layout, attrs);
  pango_attr_list_unref (attrs);
  pango_layout_set_width (layout, MAX (width, 1) * PANGO_SCALE);
  pango_layout_set_wrap (layout, PANGO_WRAP_WORD_CHAR);
  g_object_unref (ctx);
  return layout;
}

int
easel_text_render_height (PangoLayout *layout)
{
  PangoRectangle logical;

  pango_layout_get_extents (layout, NULL, &logical);
  return (int) ceil ((double) logical.height / PANGO_SCALE);
}

/* Whether the run from byte `from` to `to` uses a font with color glyphs (R3b: read from the font each item of it was shaped with). */
static gboolean
run_has_color_glyphs (PangoLayout *layout, size_t from, size_t to)
{
  PangoLayoutIter *it = pango_layout_get_iter (layout);
  gboolean color = FALSE;

  do
    {
      PangoLayoutRun *run = pango_layout_iter_get_run_readonly (it);

      if (run != NULL && (size_t) run->item->offset < to && (size_t) (run->item->offset + run->item->length) > from)
        {
          hb_face_t *face = hb_font_get_face (pango_font_get_hb_font (run->item->analysis.font));

          color = hb_ot_color_has_png (face) || hb_ot_color_has_layers (face) || hb_ot_color_has_svg (face);
        }
    }
  while (!color && pango_layout_iter_next_run (it));
  pango_layout_iter_free (it);
  return color;
}

void
easel_text_render_ink_move (EaselTextInk *ink, int dx, int dy)
{
  ink->x += dx;
  ink->y += dy;
  for (size_t k = 0; k < ink->n_layers; k++)
    {
      ((EaselTextLayer *) ink->layers)[k].x += dx;
      ((EaselTextLayer *) ink->layers)[k].y += dy;
    }
}

void
easel_text_render_ink_free (EaselTextInk *ink)
{
  for (size_t k = 0; k < ink->n_layers; k++)
    {
      g_free ((uint8_t *) ink->layers[k].cover);
      g_free ((uint32_t *) ink->layers[k].colors);
    }
  g_free ((EaselTextLayer *) ink->layers);
  g_free ((uint32_t *) ink->color);
  g_free ((uint8_t *) ink->mask);
  *ink = (EaselTextInk) { 0 };
}

/* The straight, opaque color of a color glyph's pixel p (premultiplied ARGB) where it is not the run's own color c, as Sharp decides
   it (REQ-0017-06); c where it is or where nothing is drawn. */
static uint32_t
glyph_color (uint32_t p, uint32_t c)
{
  unsigned a = p >> 24, r, g, bl;

  if (a == 0)
    return c;
  r = ((p >> 16) & 255) * 255 / a;
  g = ((p >> 8) & 255) * 255 / a;
  bl = (p & 255) * 255 / a;
  if (ABS ((int) r - (int) ((c >> 16) & 255)) > 2 || ABS ((int) g - (int) ((c >> 8) & 255)) > 2 || ABS ((int) bl - (int) (c & 255)) > 2)
    return 0xFF000000u | r << 16 | g << 8 | bl;
  return c;
}

/* Smooth's ink (REQ-0040-04, REQ-0040-05): each style run drawn alone through a layout of the same text on the gray context (the same
   places as Sharp's, `text-smooth-layout`), its coverage kept over the rectangle where it is not 0, in the order of the runs. */
static gboolean
smooth_ink (PangoLayout *sharp, const EaselTextBuffer *b, int x, int y, int ax, int ay, int aw, int ah, size_t max_bytes,
            EaselTextInk *ink)
{
  PangoLayout *layout = easel_text_render_layout_for (b, pango_layout_get_width (sharp) / PANGO_SCALE, TRUE);
  const EaselTextRun *runs;
  size_t n, len, used = 0;
  EaselTextLayer *layers;
  cairo_surface_t *a8 = cairo_image_surface_create (CAIRO_FORMAT_A8, aw, ah);
  gboolean ok = cairo_surface_status (a8) == CAIRO_STATUS_SUCCESS;

  easel_text_buffer_text (b, &len);
  runs = easel_text_buffer_runs (b, &n);
  layers = ok ? g_try_new0 (EaselTextLayer, MAX (n, 1)) : NULL;
  *ink = (EaselTextInk) { x + ax, y + ay, aw, ah, NULL, NULL, layers, 0 };
  ok = layers != NULL;
  for (size_t i = 0; ok && i < n; i++)
    {
      size_t from = runs[i].start, to = i + 1 < n ? runs[i + 1].start : len;
      PangoAttrList *attrs = run_attributes (b, TRUE, from, to);
      gboolean colored = run_has_color_glyphs (layout, from, to);
      cairo_surface_t *argb = colored ? cairo_image_surface_create (CAIRO_FORMAT_ARGB32, aw, ah) : NULL;
      cairo_t *cr = cairo_create (a8);
      const unsigned char *d;
      int stride, x0 = aw, y0 = ah, x1 = -1, y1 = -1;

      /* the run alone (the others at alpha 1 of 65535, which draws them at 0 coverage), as Sharp's pass */
      pango_layout_set_attributes (layout, attrs);
      cairo_set_operator (cr, CAIRO_OPERATOR_CLEAR);
      cairo_paint (cr);
      cairo_set_operator (cr, CAIRO_OPERATOR_OVER);
      cairo_move_to (cr, -ax, -ay);
      pango_cairo_show_layout (cr, layout);
      cairo_destroy (cr);
      cairo_surface_flush (a8);
      if (argb != NULL)
        {
          cr = cairo_create (argb);
          cairo_move_to (cr, -ax, -ay);
          pango_cairo_show_layout (cr, layout);
          cairo_destroy (cr);
          cairo_surface_flush (argb);
        }
      pango_attr_list_unref (attrs);
      d = cairo_image_surface_get_data (a8);
      stride = cairo_image_surface_get_stride (a8);
      for (int j = 0; j < ah; j++)
        for (int k = 0; k < aw; k++)
          if (d[(size_t) j * stride + k] > 0)
            {
              x0 = MIN (x0, k), x1 = MAX (x1, k);
              y0 = MIN (y0, j), y1 = MAX (y1, j);
            }
      if (x1 >= 0)
        {
          EaselTextLayer *l = &layers[ink->n_layers];
          int w = x1 - x0 + 1, h = y1 - y0 + 1;
          size_t a = (size_t) w * (size_t) h, bytes = a * (argb != NULL ? 5 : 1);
          uint8_t *cover = used + bytes <= max_bytes ? g_try_malloc (a) : NULL;
          uint32_t *colors = cover != NULL && argb != NULL ? g_try_new (uint32_t, a) : NULL;

          ok = cover != NULL && (argb == NULL || colors != NULL);
          if (ok)
            {
              used += bytes;
              for (int j = 0; j < h; j++)
                for (int k = 0; k < w; k++)
                  {
                    cover[(size_t) j * w + k] = d[(size_t) (y0 + j) * stride + x0 + k];
                    if (colors != NULL)
                      colors[(size_t) j * w + k] = glyph_color (((const uint32_t *) (cairo_image_surface_get_data (argb)
                                                                                     + (size_t) (y0 + j) * cairo_image_surface_get_stride (argb)))[x0 + k],
                                                                runs[i].style.color);
                  }
              *l = (EaselTextLayer) { x + ax + x0, y + ay + y0, w, h, runs[i].style.color, cover, colors };
              ink->n_layers++;
            }
          else
            {
              g_free (cover);
              g_free (colors);
            }
        }
      g_clear_pointer (&argb, cairo_surface_destroy);
    }
  cairo_surface_destroy (a8);
  g_object_unref (layout);
  if (!ok)
    easel_text_render_ink_free (ink);
  return ok;
}

gboolean
easel_text_render_ink (PangoLayout *layout, const EaselTextBuffer *b, int x, int y, size_t max_bytes, gboolean smooth,
                       EaselTextInk *ink)
{
  PangoRectangle inked, logical;
  const EaselTextRun *runs;
  size_t n, len, area;
  int ax, ay, aw, ah;
  uint32_t *color;
  uint8_t *mask;
  cairo_surface_t *a8;
  PangoAttrList *saved = pango_attr_list_ref (pango_layout_get_attributes (layout));

  *ink = (EaselTextInk) { 0 };
  easel_text_buffer_text (b, &len);
  runs = easel_text_buffer_runs (b, &n);
  pango_layout_get_pixel_extents (layout, &inked, &logical);
  /* The area: the logical box and the ink, which may reach past it (an italic overhang) */
  ax = MIN (logical.x, inked.x);
  ay = MIN (logical.y, inked.y);
  aw = MAX (logical.x + logical.width, inked.x + inked.width) - ax;
  ah = MAX (logical.y + logical.height, inked.y + inked.height) - ay;
  aw = MAX (aw, 1);
  ah = MAX (ah, 1);
  area = (size_t) aw * (size_t) ah;
  if (area > SIZE_MAX / 5 || area * 5 > max_bytes)
    {
      pango_attr_list_unref (saved);
      return FALSE;
    }
  if (smooth)
    {
      pango_attr_list_unref (saved);
      return smooth_ink (layout, b, x, y, ax, ay, aw, ah, max_bytes, ink);
    }
  color = g_try_malloc (area * sizeof *color);
  mask = g_try_malloc0 (area);
  a8 = color != NULL && mask != NULL ? cairo_image_surface_create (CAIRO_FORMAT_A8, aw, ah) : NULL;
  if (a8 == NULL || cairo_surface_status (a8) != CAIRO_STATUS_SUCCESS)
    {
      g_free (color);
      g_free (mask);
      g_clear_pointer (&a8, cairo_surface_destroy);
      pango_attr_list_unref (saved);
      return FALSE;
    }
  for (size_t i = 0; i < n; i++)
    {
      size_t from = runs[i].start, to = i + 1 < n ? runs[i + 1].start : len;
      PangoAttrList *attrs = run_attributes (b, TRUE, from, to);
      gboolean colored = run_has_color_glyphs (layout, from, to);
      cairo_surface_t *argb = colored ? cairo_image_surface_create (CAIRO_FORMAT_ARGB32, aw, ah) : NULL;
      cairo_t *cr = cairo_create (a8);
      const unsigned char *data;
      int stride;

      /* One pass: the run alone, its other runs at alpha 1; its coverage in A8 (R1, R2) */
      pango_layout_set_attributes (layout, attrs);
      cairo_set_operator (cr, CAIRO_OPERATOR_CLEAR);
      cairo_paint (cr);
      cairo_set_operator (cr, CAIRO_OPERATOR_OVER);
      cairo_move_to (cr, -ax, -ay);
      pango_cairo_show_layout (cr, layout);
      cairo_destroy (cr);
      cairo_surface_flush (a8);
      if (argb != NULL)
        {
          cr = cairo_create (argb);
          cairo_move_to (cr, -ax, -ay);
          pango_cairo_show_layout (cr, layout);
          cairo_destroy (cr);
          cairo_surface_flush (argb);
        }
      data = cairo_image_surface_get_data (a8);
      stride = cairo_image_surface_get_stride (a8);
      for (int j = 0; j < ah; j++)
        for (int k = 0; k < aw; k++)
          if (data[(size_t) j * stride + k] >= 128)
            {
              size_t at = (size_t) j * aw + k;
              uint32_t c = runs[i].style.color;

              if (argb != NULL)
                {
                  /* A color glyph's own color, opaque (REQ-0017-06): where the drawn color is not the run's letters' */
                  uint32_t p = ((const uint32_t *) (cairo_image_surface_get_data (argb) + (size_t) j * cairo_image_surface_get_stride (argb)))[k];
                  unsigned a = p >> 24;

                  if (a > 0)
                    {
                      unsigned r = ((p >> 16) & 255) * 255 / a, g = ((p >> 8) & 255) * 255 / a, bl = (p & 255) * 255 / a;
                      int dr = (int) r - (int) ((c >> 16) & 255), dg = (int) g - (int) ((c >> 8) & 255), db = (int) bl - (int) (c & 255);

                      if (ABS (dr) > 2 || ABS (dg) > 2 || ABS (db) > 2)
                        c = 0xFF000000u | r << 16 | g << 8 | bl;
                    }
                }
              color[at] = c;
              mask[at] = 1;
            }
      g_clear_pointer (&argb, cairo_surface_destroy);
      pango_attr_list_unref (attrs);
    }
  pango_layout_set_attributes (layout, saved);
  pango_attr_list_unref (saved);
  cairo_surface_destroy (a8);
  *ink = (EaselTextInk) { x + ax, y + ay, aw, ah, color, mask };
  return TRUE;
}
