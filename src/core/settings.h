/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Settings file text (REQ-0001-01..05, REQ-0001-40). Reading and writing the file is io's job. */
#pragma once

#include <stddef.h>
#include <stdint.h>

enum
{
  EASEL_SIZE_AUTO = 0,
  EASEL_SIZE_MAX = 10000,
  EASEL_UNDO_MB_MIN = 16,
  EASEL_UNDO_MB_MAX = 8192,
  EASEL_UNDO_MB_DEFAULT = 512,
};

typedef struct
{
  int width;              /* EASEL_SIZE_AUTO or 1..EASEL_SIZE_MAX */
  int height;
  uint32_t background;    /* 0xAARRGGBB, straight alpha */
  int undo_budget_mb;     /* EASEL_UNDO_MB_MIN..EASEL_UNDO_MB_MAX (REQ-0002-18) */
} EaselSettings;

/* What a newly created settings file holds (REQ-0001-04). */
extern const char easel_settings_default_text[];

void easel_settings_defaults (EaselSettings *s);

/* Fills *s from text of len bytes (which may contain NULs). Invalid values keep their defaults;
   an unparseable text gives all defaults. */
void easel_settings_parse (const char *text, size_t len, EaselSettings *s);

/* Picture size. If width or height is auto, the largest of 640x480, 800x600, 1024x768 that is smaller than
   the area (device pixels) in both directions, else 640x480 (REQ-0001-02). */
void easel_settings_resolve_size (const EaselSettings *s, int area_width, int area_height,
                                  int *width, int *height);
