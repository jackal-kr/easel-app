/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Settings file text (REQ-0001-01..05, REQ-0001-40). Reading and writing the file is io's job. */
#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

enum
{
  EASEL_SIZE_AUTO = 0,
  EASEL_SIZE_MAX = 10000,
  EASEL_UNDO_MB_MIN = 16,
  EASEL_UNDO_MB_MAX = 8192,
  EASEL_UNDO_MB_DEFAULT = 512,
  EASEL_GRID_SIZE_MIN = 1,
  EASEL_GRID_SIZE_MAX = 1000,
  EASEL_GRID_SIZE_DEFAULT = 1,
  EASEL_JPEG_QUALITY_MIN = 1,
  EASEL_JPEG_QUALITY_MAX = 100,
  EASEL_JPEG_QUALITY_DEFAULT = 90,
  EASEL_TEXT_SIZE_MIN = 1,
  EASEL_TEXT_SIZE_MAX = 500,
  EASEL_TEXT_SIZE_DEFAULT = 12,
  EASEL_TINT_MIN = 0,
  EASEL_TINT_MAX = 50,
  EASEL_TINT_DEFAULT = 12,
  EASEL_TEXT_FONT_MAX = 128,      /* the longest family name taken, with its NUL */
};

typedef enum
{
  EASEL_STYLE_SYSTEM,
  EASEL_STYLE_LIGHT,
  EASEL_STYLE_DARK
} EaselStyle;

typedef enum
{
  EASEL_FILL_OUTLINE,
  EASEL_FILL_OUTLINE_AND_FILL,
  EASEL_FILL_ONLY
} EaselFillStyle;

typedef struct
{
  int width;              /* EASEL_SIZE_AUTO or 1..EASEL_SIZE_MAX */
  int height;
  uint32_t background;    /* a new picture's color, `canvas-color` (or the older `background`), 0xAARRGGBB, straight alpha */
  int undo_budget_mb;     /* EASEL_UNDO_MB_MIN..EASEL_UNDO_MB_MAX (REQ-0002-18) */
  bool grid;              /* the grid starts on (REQ-0004-15) */
  int grid_size;          /* EASEL_GRID_SIZE_MIN..EASEL_GRID_SIZE_MAX picture pixels */
  bool grid_axonometric;
  bool show_toolbox;      /* the panels start shown (REQ-0004-27) */
  bool show_colorbox;
  bool show_statusbar;
  bool has_workspace_color;   /* a valid `[view] workspace-color` was given (REQ-0009-04) */
  uint32_t workspace_color;   /* 0xRRGGBB, when has_workspace_color */
  bool has_workspace_color_dark;   /* a valid `[view] workspace-color-dark` was given (REQ-0009-04, REQ-0009-06) */
  uint32_t workspace_color_dark;   /* 0xRRGGBB, when has_workspace_color_dark */
  int jpeg_quality;       /* EASEL_JPEG_QUALITY_MIN..EASEL_JPEG_QUALITY_MAX (REQ-0005-14) */
  char text_font[EASEL_TEXT_FONT_MAX];   /* `[text] font`, trimmed; empty for `auto`, the desktop's document font (REQ-0017-24) */
  int text_size;          /* EASEL_TEXT_SIZE_MIN..EASEL_TEXT_SIZE_MAX points */
  bool text_bold, text_italic, text_underline, text_strike;
  bool frame_time;        /* `[view] frame-time`: the status bar shows it (REQ-0021-12) */
  EaselStyle style;       /* `[view] style` (REQ-0020-03) */
  int tint;               /* `[view] tint`, the selection's tint in percent, EASEL_TINT_MIN..EASEL_TINT_MAX */
  int brush;              /* `[drawing]` (REQ-0020-05): what the tools start with, by their place in the options panel */
  int airbrush;
  int eraser;
  bool eraser_transparent;
  int line_width;         /* 1..5 pixels */
  EaselFillStyle fill_style;
  int magnifier;          /* 1, 2, 4, 6, 8 or 16 */
} EaselSettings;

/* What a newly created settings file holds (REQ-0001-04). */
extern const char easel_settings_default_text[];

void easel_settings_defaults (EaselSettings *s);

/* The names the file holds for a setting (REQ-0020-03, REQ-0020-05). */
const char *easel_settings_style_name (EaselStyle style);
const char *easel_settings_fill_name (EaselFillStyle fill);
const char *easel_settings_brush_name (int brush);

/* Whether the text is one the settings file can be (REQ-0001-05, REQ-0001-40). */
bool easel_settings_parseable (const char *text, size_t len);

/* The text with one key changed and all else byte for byte as it was (REQ-0020-09): the key's last line of the section becomes key=value, with
   the line ending it had; a key that is missing goes directly after the last key line of the section's last occurrence, or after its header;
   a section that is missing goes at the end, after a blank line, with none in an empty file. A NULL value removes every line of the key in
   the section. A new line takes the file's line ending. In *out, for free (), with its length in *out_len. FALSE, with those untouched, for
   an unparseable text or if the memory cannot be had. */
bool easel_settings_edit (const char *text, size_t len, const char *section, const char *key, const char *value, char **out, size_t *out_len);

/* Fills *s from text of len bytes (which may contain NULs). Invalid values keep their defaults;
   an unparseable text gives all defaults. */
void easel_settings_parse (const char *text, size_t len, EaselSettings *s);

/* Picture size. If width or height is auto, the largest of 640x480, 800x600, 1024x768 that is smaller than
   the area (device pixels) in both directions, else 640x480 (REQ-0001-02). */
void easel_settings_resolve_size (const EaselSettings *s, int area_width, int area_height,
                                  int *width, int *height);
