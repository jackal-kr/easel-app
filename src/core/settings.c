/* SPDX-License-Identifier: GPL-3.0-or-later */

#include <string.h>

#include "ini.h"
#include "number.h"
#include "settings.h"

#define SECTION "new-picture"
#define SECTION_HISTORY "history"
#define SECTION_VIEW "view"
#define SECTION_SAVE "save"

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
  "background=#FFFFFF\n"
  "\n"
  "[" SECTION_HISTORY "]\n"
  "memory=512\n"
  "\n"
  "[" SECTION_VIEW "]\n"
  "grid=off\n"
  "grid-size=1\n"
  "grid-axonometric=off\n"
  "toolbox=on\n"
  "colorbox=on\n"
  "statusbar=on\n"
  "\n"
  "[" SECTION_SAVE "]\n"
  "jpeg-quality=90\n";

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

/* An invalid value resets the key to its default, so a later valid repeat still wins. */
static void
apply (EaselSpan section, EaselSpan key, EaselSpan value, void *data)
{
  EaselSettings *s = data;

  if (easel_span_is (section, SECTION))
    {
      if (easel_span_is (key, "width") && !parse_size (value, &s->width))
        s->width = EASEL_SIZE_AUTO;
      else if (easel_span_is (key, "height") && !parse_size (value, &s->height))
        s->height = EASEL_SIZE_AUTO;
      else if (easel_span_is (key, "background") && !easel_ini_parse_color (value, &s->background))
        s->background = 0xFFFFFFFFu;
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
      else if (easel_span_is (key, "workspace-color"))
        s->has_workspace_color = parse_rgb (value, &s->workspace_color);
      else if (easel_span_is (key, "workspace-color-dark"))
        s->has_workspace_color_dark = parse_rgb (value, &s->workspace_color_dark);
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

  easel_settings_defaults (&parsed);
  /* An unparseable file gives every default (REQ-0001-05, REQ-0001-40). */
  if (!easel_ini_parse (text, len, apply, &parsed))
    {
      easel_settings_defaults (s);
      return;
    }
  *s = parsed;
}

void
easel_settings_resolve_size (const EaselSettings *s, int area_width, int area_height,
                             int *width, int *height)
{
  /* Largest last. Preferences will make this list a setting. */
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
