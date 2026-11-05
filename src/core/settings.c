/* SPDX-License-Identifier: GPL-3.0-or-later */

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include "catalog.h"
#include "ini.h"
#include "number.h"
#include "settings.h"

#define SECTION "new-picture"
#define SECTION_HISTORY "history"
#define SECTION_DRAWING "drawing"
#define SECTION_VIEW "view"
#define SECTION_INTERFACE "interface"
#define SECTION_SAVE "save"
#define SECTION_TEXT "text"

/* Digits only, 1..EASEL_SIZE_MAX, or auto. */
static bool
parse_size (EaselSpan v, int *out)
{
  int n = 0;

  if (easel_span_is_nocase (v, "auto"))
    {
      *out = EASEL_SIZE_AUTO;
      return true;
    }
  if (v.n == 0)
    return false;
  for (size_t i = 0; i < v.n; i++)
    {
      if (v.p[i] < '0' || v.p[i] > '9')
        return false;
      n = n * 10 + (v.p[i] - '0');
      if (n > EASEL_SIZE_MAX)
        return false;
    }
  if (n < 1)
    return false;
  *out = n;
  return true;
}

/* Digits only, EASEL_UNDO_MB_MIN..EASEL_UNDO_MB_MAX. */
static bool
parse_undo_mb (EaselSpan v, int *out)
{
  int n = 0;

  if (v.n == 0)
    return false;
  for (size_t i = 0; i < v.n; i++)
    {
      if (v.p[i] < '0' || v.p[i] > '9')
        return false;
      n = n * 10 + (v.p[i] - '0');
      if (n > EASEL_UNDO_MB_MAX)
        return false;
    }
  if (n < EASEL_UNDO_MB_MIN)
    return false;
  *out = n;
  return true;
}

const char easel_settings_default_text[] =
  "[" SECTION "]\n"
  "width=auto\n"
  "height=auto\n"
  "canvas-color=#FFFFFF\n"
  "\n"
  "[" SECTION_HISTORY "]\n"
  "memory=512\n"
  "\n"
  "[" SECTION_INTERFACE "]\n"
  "language=auto\n"
  "\n"
  "[" SECTION_VIEW "]\n"
  "grid=off\n"
  "grid-size=1\n"
  "grid-axonometric=off\n"
  "toolbox=on\n"
  "colorbox=on\n"
  "statusbar=on\n"
  "style=system\n"
  "tint=12\n"
  "frame-time=off\n"
  "\n"
  "[" SECTION_SAVE "]\n"
  "jpeg-quality=90\n"
  "\n"
  "[" SECTION_TEXT "]\n"
  "font=auto\n"
  "size=12\n"
  "bold=false\n"
  "italic=false\n"
  "underline=false\n"
  "strikethrough=false\n"
  "\n"
  "[" SECTION_DRAWING "]\n"
  "brush=circle-4\n"
  "airbrush=16\n"
  "eraser=8\n"
  "eraser-mode=opaque\n"
  "line-width=1\n"
  "fill-style=outline\n"
  "magnifier=2\n";

static const char *const brush_names[] = {
  "circle-7", "circle-4", "circle-1", "square-8", "square-5", "square-2",
  "slash-8", "slash-5", "slash-2", "backslash-8", "backslash-5", "backslash-2",
};
static const char *const style_names[] = { "system", "light", "dark" };
static const char *const fill_names[] = { "outline", "outline-and-fill", "fill" };
static const char *const airbrush_values[] = { "9", "16", "24" };
static const char *const eraser_values[] = { "4", "6", "8", "10" };

const char *
easel_settings_style_name (EaselStyle style)
{
  return style_names[style];
}

const char *
easel_settings_fill_name (EaselFillStyle fill)
{
  return fill_names[fill];
}

const char *
easel_settings_brush_name (int brush)
{
  return brush_names[brush];
}

/* The place of the value in names, or -1. */
static int
index_of (EaselSpan value, const char *const *names, int n)
{
  for (int i = 0; i < n; i++)
    if (easel_span_is (value, names[i]))
      return i;
  return -1;
}

void
easel_settings_defaults (EaselSettings *s)
{
  s->width = EASEL_SIZE_AUTO;
  s->height = EASEL_SIZE_AUTO;
  s->background = 0xFFFFFFFFu;
  s->undo_budget_mb = EASEL_UNDO_MB_DEFAULT;
  s->grid = false;
  s->grid_size = EASEL_GRID_SIZE_DEFAULT;
  s->grid_axonometric = false;
  s->show_toolbox = true;
  s->show_colorbox = true;
  s->show_statusbar = true;
  s->has_workspace_color = false;
  s->workspace_color = 0;
  s->has_workspace_color_dark = false;
  s->workspace_color_dark = 0;
  s->jpeg_quality = EASEL_JPEG_QUALITY_DEFAULT;
  s->text_font[0] = '\0';
  s->style = EASEL_STYLE_SYSTEM;
  s->tint = EASEL_TINT_DEFAULT;
  strcpy (s->language, "auto");
  s->frame_time = false;
  s->brush = 1;
  s->airbrush = 1;
  s->eraser = 2;
  s->eraser_transparent = false;
  s->line_width = 1;
  s->fill_style = EASEL_FILL_OUTLINE;
  s->magnifier = 2;
  s->text_size = EASEL_TEXT_SIZE_DEFAULT;
  s->text_bold = s->text_italic = s->text_underline = s->text_strike = false;
}

/* true or false, in any case (REQ-0017-24). */
static bool
parse_true_false (EaselSpan v, bool *out)
{
  if (easel_span_is_nocase (v, "true"))
    *out = true;
  else if (easel_span_is_nocase (v, "false"))
    *out = false;
  else
    return false;
  return true;
}

/* on or off, in any case. */
static bool
parse_on_off (EaselSpan v, bool *out)
{
  if (easel_span_is_nocase (v, "on"))
    *out = true;
  else if (easel_span_is_nocase (v, "off"))
    *out = false;
  else
    return false;
  return true;
}

/* `#rrggbb` exactly: a `#` and six hexadecimal digits, either case (REQ-0009-04). */
static bool
parse_rgb (EaselSpan v, uint32_t *out)
{
  uint32_t c = 0;

  if (v.n != 7 || v.p[0] != '#')
    return false;
  for (size_t i = 1; i < 7; i++)
    {
      char ch = v.p[i];
      int d = ch >= '0' && ch <= '9'   ? ch - '0'
              : ch >= 'a' && ch <= 'f' ? ch - 'a' + 10
              : ch >= 'A' && ch <= 'F' ? ch - 'A' + 10
                                       : -1;

      if (d < 0)
        return false;
      c = c << 4 | (uint32_t) d;
    }
  *out = c;
  return true;
}

/* What the parser keeps while it reads: the settings, and the picture's color under its two names, resolved at the end
   (REQ-0001-03: a valid `canvas-color` wins, else a valid `background`, else white). */
typedef struct
{
  EaselSettings *s;
  bool has_canvas_color, has_background;
  uint32_t canvas_color, background;
} Parse;

/* An invalid value resets the key to its default, so a later valid repeat still wins. */
static void
apply (EaselSpan section, EaselSpan key, EaselSpan value, void *data)
{
  Parse *p = data;
  EaselSettings *s = p->s;

  if (easel_span_is (section, SECTION))
    {
      if (easel_span_is (key, "width") && !parse_size (value, &s->width))
        s->width = EASEL_SIZE_AUTO;
      else if (easel_span_is (key, "height") && !parse_size (value, &s->height))
        s->height = EASEL_SIZE_AUTO;
      else if (easel_span_is (key, "canvas-color"))
        p->has_canvas_color = easel_ini_parse_color (value, &p->canvas_color);
      else if (easel_span_is (key, "background"))   /* the name before the rename (REQ-0001-03) */
        p->has_background = easel_ini_parse_color (value, &p->background);
    }
  else if (easel_span_is (section, SECTION_HISTORY) && easel_span_is (key, "memory")
           && !parse_undo_mb (value, &s->undo_budget_mb))
    s->undo_budget_mb = EASEL_UNDO_MB_DEFAULT;
  else if (easel_span_is (section, SECTION_SAVE) && easel_span_is (key, "jpeg-quality"))
    {
      char text[16] = "";

      if (value.n < sizeof text)
        memcpy (text, value.p, value.n);
      else
        text[0] = 'x';   /* too long to be a number in range */
      if (!easel_parse_field (text, EASEL_JPEG_QUALITY_MIN, EASEL_JPEG_QUALITY_MAX, &s->jpeg_quality))
        s->jpeg_quality = EASEL_JPEG_QUALITY_DEFAULT;
    }
  else if (easel_span_is (section, SECTION_TEXT))
    {
      char text[16] = "";

      if (easel_span_is (key, "font"))
        {
          /* `auto`, empty or too long: the desktop's document font; a family is matched with the installed ones by the ui. */
          if (value.n == 0 || value.n >= sizeof s->text_font || easel_span_is_nocase (value, "auto"))
            s->text_font[0] = '\0';
          else
            {
              memcpy (s->text_font, value.p, value.n);
              s->text_font[value.n] = '\0';
            }
        }
      else if (easel_span_is (key, "size"))
        {
          if (value.n < sizeof text)
            memcpy (text, value.p, value.n);
          else
            text[0] = 'x';
          if (!easel_parse_field (text, EASEL_TEXT_SIZE_MIN, EASEL_TEXT_SIZE_MAX, &s->text_size))
            s->text_size = EASEL_TEXT_SIZE_DEFAULT;
        }
      else if (easel_span_is (key, "bold") && !parse_true_false (value, &s->text_bold))
        s->text_bold = false;
      else if (easel_span_is (key, "italic") && !parse_true_false (value, &s->text_italic))
        s->text_italic = false;
      else if (easel_span_is (key, "underline") && !parse_true_false (value, &s->text_underline))
        s->text_underline = false;
      else if (easel_span_is (key, "strikethrough") && !parse_true_false (value, &s->text_strike))
        s->text_strike = false;
    }
  else if (easel_span_is (section, SECTION_DRAWING))
    {
      int i;

      if (easel_span_is (key, "brush"))
        s->brush = (i = index_of (value, brush_names, 12)) >= 0 ? i : 1;
      else if (easel_span_is (key, "airbrush"))
        s->airbrush = (i = index_of (value, airbrush_values, 3)) >= 0 ? i : 1;
      else if (easel_span_is (key, "eraser"))
        s->eraser = (i = index_of (value, eraser_values, 4)) >= 0 ? i : 2;
      else if (easel_span_is (key, "eraser-mode"))
        s->eraser_transparent = easel_span_is (value, "transparent");
      else if (easel_span_is (key, "fill-style"))
        s->fill_style = (i = index_of (value, fill_names, 3)) >= 0 ? (EaselFillStyle) i : EASEL_FILL_OUTLINE;
      else if (easel_span_is (key, "line-width") || easel_span_is (key, "magnifier"))
        {
          static const char *const widths[] = { "1", "2", "3", "4", "5" };
          static const char *const levels[] = { "1", "2", "4", "6", "8", "16" };

          if (easel_span_is (key, "line-width"))
            s->line_width = (i = index_of (value, widths, 5)) >= 0 ? i + 1 : 1;
          else
            s->magnifier = (i = index_of (value, levels, 6)) >= 0 ? (value.n == 2 ? 16 : value.p[0] - '0') : 2;
        }
    }
  else if (easel_span_is (section, SECTION_INTERFACE) && easel_span_is (key, "language"))
    {
      /* A code or auto, in any case; anything else, a region's code too, is auto (REQ-0023-04). */
      strcpy (s->language, "auto");
      if (value.n == 2)
        {
          char code[3] = { (char) tolower ((unsigned char) value.p[0]), (char) tolower ((unsigned char) value.p[1]), '\0' };

          if (easel_language_known (code))
            strcpy (s->language, code);
        }
    }
  else if (easel_span_is (section, SECTION_VIEW))
    {
      /* easel_parse_field wants a NUL-terminated text; a number in range is short, so copy it. */
      char text[16] = "";

      if (easel_span_is (key, "grid") && !parse_on_off (value, &s->grid))
        s->grid = false;
      else if (easel_span_is (key, "grid-axonometric") && !parse_on_off (value, &s->grid_axonometric))
        s->grid_axonometric = false;
      else if (easel_span_is (key, "toolbox") && !parse_on_off (value, &s->show_toolbox))
        s->show_toolbox = true;
      else if (easel_span_is (key, "colorbox") && !parse_on_off (value, &s->show_colorbox))
        s->show_colorbox = true;
      else if (easel_span_is (key, "statusbar") && !parse_on_off (value, &s->show_statusbar))
        s->show_statusbar = true;
      else if (easel_span_is (key, "frame-time") && !parse_on_off (value, &s->frame_time))
        s->frame_time = false;
      else if (easel_span_is (key, "workspace-color"))
        s->has_workspace_color = parse_rgb (value, &s->workspace_color);
      else if (easel_span_is (key, "workspace-color-dark"))
        s->has_workspace_color_dark = parse_rgb (value, &s->workspace_color_dark);
      else if (easel_span_is (key, "style"))
        {
          s->style = EASEL_STYLE_SYSTEM;
          for (int i = 0; i < 3; i++)
            if (easel_span_is_nocase (value, style_names[i]))
              s->style = (EaselStyle) i;
        }
      else if (easel_span_is (key, "tint"))
        {
          if (value.n < sizeof text)
            memcpy (text, value.p, value.n);
          else
            text[0] = 'x';   /* too long to be a number in range */
          if (!easel_parse_field (text, EASEL_TINT_MIN, EASEL_TINT_MAX, &s->tint))
            s->tint = EASEL_TINT_DEFAULT;
        }
      else if (easel_span_is (key, "grid-size"))
        {
          if (value.n < sizeof text)
            memcpy (text, value.p, value.n);
          else
            text[0] = 'x';   /* too long to be a number in range */
          if (!easel_parse_field (text, EASEL_GRID_SIZE_MIN, EASEL_GRID_SIZE_MAX, &s->grid_size))
            s->grid_size = EASEL_GRID_SIZE_DEFAULT;
        }
    }
}

void
easel_settings_parse (const char *text, size_t len, EaselSettings *s)
{
  EaselSettings parsed;
  Parse p = { &parsed, false, false, 0, 0 };

  easel_settings_defaults (&parsed);
  /* An unparseable file gives every default (REQ-0001-05, REQ-0001-40). */
  if (!easel_ini_parse (text, len, apply, &p))
    {
      easel_settings_defaults (s);
      return;
    }
  if (p.has_canvas_color)
    parsed.background = p.canvas_color;
  else if (p.has_background)
    parsed.background = p.background;
  *s = parsed;
}

void
easel_settings_resolve_size (const EaselSettings *s, int area_width, int area_height,
                             int *width, int *height)
{
  /* Largest last. A constant (JOD, 2026-11-03: Preferences has no row for it). */
  static const int standard[][2] = { { 640, 480 }, { 800, 600 }, { 1024, 768 } };

  if (s->width != EASEL_SIZE_AUTO && s->height != EASEL_SIZE_AUTO)
    {
      *width = s->width;
      *height = s->height;
      return;
    }

  *width = standard[0][0];
  *height = standard[0][1];
  for (size_t i = 0; i < sizeof standard / sizeof standard[0]; i++)
    if (standard[i][0] < area_width && standard[i][1] < area_height)
      {
        *width = standard[i][0];
        *height = standard[i][1];
      }
}

static void
no_key (EaselSpan section, EaselSpan key, EaselSpan value, void *data)
{
}

bool
easel_settings_parseable (const char *text, size_t len)
{
  return easel_ini_parse (text, len, no_key, NULL);
}

/* One line of the text, its piece without the line ending, and where the next one starts. */
typedef struct
{
  size_t start, end, next;
} Line;

static bool
next_line (const char *text, size_t len, size_t pos, Line *line)
{
  const char *nl;

  if (pos >= len)
    return false;
  nl = memchr (text + pos, '\n', len - pos);
  line->start = pos;
  line->next = nl != NULL ? (size_t) (nl - text) + 1 : len;
  line->end = nl != NULL ? (size_t) (nl - text) : len;
  if (nl != NULL && line->end > pos && text[line->end - 1] == '\r')
    line->end--;
  return true;
}

static void
put (char **at, const char *text, size_t n)
{
  memcpy (*at, text, n);
  *at += n;
}

static void
put_str (char **at, const char *text)
{
  put (at, text, strlen (text));
}

/* What a line says, if it is the header of a section or the line of a key. */
static bool
line_section (EaselSpan line, EaselSpan *section)
{
  if (line.n < 2 || line.p[0] != '[' || line.p[line.n - 1] != ']')
    return false;
  *section = (EaselSpan) { line.p + 1, line.n - 2 };
  return true;
}

static bool
line_key (EaselSpan line, EaselSpan *key)
{
  const char *eq;

  if (line.n == 0 || line.p[0] == '#' || (eq = memchr (line.p, '=', line.n)) == NULL)
    return false;
  *key = easel_span_trim ((EaselSpan) { line.p, (size_t) (eq - line.p) });
  return true;
}

bool
easel_settings_edit (const char *text, size_t len, const char *section, const char *key, const char *value, char **out, size_t *out_len)
{
  const char *eol = "\n";
  const char *first_nl = memchr (text, '\n', len);
  bool in_section = false, has_section = false, has_key = false;
  size_t after = 0, key_start = 0, key_end = 0, room;
  size_t pos = 0;
  Line l;
  char *buffer, *at;

  if (!easel_settings_parseable (text, len))
    return false;
  if (first_nl != NULL && first_nl > text && first_nl[-1] == '\r')
    eol = "\r\n";
  room = len + strlen (section) + strlen (key) + (value != NULL ? strlen (value) : 0) + 16;
  buffer = malloc (room);
  if (buffer == NULL)
    return false;
  at = buffer;
  /* Where the key's last line is, and where a new one would go. */
  while (next_line (text, len, pos, &l))
    {
      EaselSpan line = easel_span_trim ((EaselSpan) { text + l.start, l.end - l.start });
      EaselSpan name;

      if (line_section (line, &name))
        {
          in_section = easel_span_is (name, section);
          if (in_section)
            {
              has_section = true;
              after = l.next;
            }
        }
      else if (in_section && line_key (line, &name))
        {
          after = l.next;
          if (easel_span_is (name, key))
            {
              has_key = true;
              key_start = l.start;
              key_end = l.end;
            }
        }
      pos = l.next;
    }
  if (value == NULL)
    {
      /* Every line of the key goes. */
      in_section = false;
      pos = 0;
      while (next_line (text, len, pos, &l))
        {
          EaselSpan line = easel_span_trim ((EaselSpan) { text + l.start, l.end - l.start });
          EaselSpan name;
          bool gone = false;

          if (line_section (line, &name))
            in_section = easel_span_is (name, section);
          else if (in_section && line_key (line, &name))
            gone = easel_span_is (name, key);
          if (!gone)
            put (&at, text + l.start, l.next - l.start);
          pos = l.next;
        }
    }
  else if (has_key)
    {
      put (&at, text, key_start);
      put_str (&at, key);
      put_str (&at, "=");
      put_str (&at, value);
      put (&at, text + key_end, len - key_end);
    }
  else if (has_section)
    {
      put (&at, text, after);
      if (after > 0 && text[after - 1] != '\n')
        put_str (&at, eol);
      put_str (&at, key);
      put_str (&at, "=");
      put_str (&at, value);
      put_str (&at, eol);
      put (&at, text + after, len - after);
    }
  else
    {
      put (&at, text, len);
      if (len > 0 && text[len - 1] != '\n')
        put_str (&at, eol);
      /* A blank line before it, if the file does not end in one already. */
      if (len > 0 && !(at - buffer >= 2 * (long) strlen (eol)
                       && memcmp (at - 2 * strlen (eol), eol, strlen (eol)) == 0 && memcmp (at - strlen (eol), eol, strlen (eol)) == 0))
        put_str (&at, eol);
      put_str (&at, "[");
      put_str (&at, section);
      put_str (&at, "]");
      put_str (&at, eol);
      put_str (&at, key);
      put_str (&at, "=");
      put_str (&at, value);
      put_str (&at, eol);
    }
  *out = buffer;
  *out_len = (size_t) (at - buffer);
  return true;
}
