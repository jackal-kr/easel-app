/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Text laid out at picture scale (feature 0017, the Plan): 1 pixel is 1 picture pixel, 96 dpi, glyphs drawn 1-bit on the grid with
   whole-pixel metrics and positions (REQ-0017-28), so the lines wrap the same at every zoom and on every screen and a letter is the same
   bitmap wherever it falls. */
#pragma once

#include <pango/pangocairo.h>

#include "text-buffer.h"
#include "text-put.h"

G_BEGIN_DECLS

/* A Pango context for text at picture scale; the caller unrefs it. */
PangoContext *easel_text_render_context (void);

/* The same with Smooth (0040): gray antialiasing in place of 1-bit glyphs, everything else as Sharp's, so the two lay text out alike
   (REQ-0040-02, the Plan). easel_text_render_context is easel_text_render_context_for (FALSE). */
PangoContext *easel_text_render_context_for (gboolean smooth);

/* The font of a style: its family, its size in points, bold and italic. The caller frees it. */
PangoFontDescription *easel_text_render_font (const EaselTextStyle *style);

/* One line of the style: its height in picture pixels, and twenty widths of the digit 0, rounded up (REQ-0017-03). */
void easel_text_render_metrics (const EaselTextStyle *style, int *line_height, int *twenty_zeros);

/* The layout of a box's text at `width` picture pixels (REQ-0017-06): each run's family, size, bold, italic, underline, strikethrough and
   color as attributes; lines wrap between words, inside a word longer than the width. An empty text has one line of the style at
   the caret. The caller unrefs it. */
PangoLayout *easel_text_render_layout (const EaselTextBuffer *b, int width);

/* The same layout made on the context of easel_text_render_context_for (smooth); easel_text_render_layout is this with FALSE. */
PangoLayout *easel_text_render_layout_for (const EaselTextBuffer *b, int width, gboolean smooth);

/* The height of the layout's text in picture pixels, rounded up. */
int easel_text_render_height (PangoLayout *layout);

/* The text as pixels (REQ-0017-16, the Plan): `ink` covers the layout's ink and its logical box from (x, y) of the picture; a pixel is
   in the mask where a character covers at least half of it, with its run's color (a color glyph's own color, opaque), the later run
   winning. FALSE without memory or past `max_bytes` (5 bytes a pixel), with nothing made. With `smooth` (0040), the layers of
   REQ-0040-04 instead, one per style run with its coverage (1 byte a pixel of its rectangle, 4 more with color glyphs). The caller frees
   it with easel_text_render_ink_free. */
gboolean easel_text_render_ink (PangoLayout *layout, const EaselTextBuffer *b, int x, int y, size_t max_bytes, gboolean smooth,
                                EaselTextInk *ink);

/* Moves an ink by (dx, dy), its layers too (a box moved keeps its lines, REQ-0017-10). */
void easel_text_render_ink_move (EaselTextInk *ink, int dx, int dy);

/* Frees what easel_text_render_ink made (Sharp's color and mask, Smooth's layers) and empties the ink. */
void easel_text_render_ink_free (EaselTextInk *ink);

/* The installed family named `name` without regard to case, as Pango names it; NULL if none is installed. */
const char *easel_text_render_find_family (const char *name);

/* Whether `name` is a generic family name (Sans, Serif, Monospace and fontconfig's other aliases), case ignored (REQ-0017-14). */
gboolean easel_text_render_is_generic (const char *name);

/* The family to use for `name` (REQ-0017-24): a generic name becomes the installed family fontconfig gives it (Sans: Noto Sans on the dev
   machine); any other name, or one that does not resolve, is returned as it is. Owned by the font map, or `name` itself. */
const char *easel_text_render_resolve_family (const char *name);

G_END_DECLS
