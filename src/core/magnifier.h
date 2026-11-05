/* SPDX-License-Identifier: GPL-3.0-or-later */
/* The Magnifier's rules (REQ-0007-03, REQ-0007-04). Zoom levels are percent; pixels are picture pixels. */
#pragma once

#include "view.h"

/* The zoom levels the Magnifier offers: 1x, 2x, 4x, 6x, 8x and 16x; 2x is the default (REQ-0007-02). They are the zoom presets
   (REQ-0008-02): one list, so the Magnifier's options and the View > Zoom submenu cannot differ. */
enum { EASEL_MAGNIFIER_N_LEVELS = EASEL_VIEW_N_PRESETS, EASEL_MAGNIFIER_DEFAULT = 1 };
#define easel_magnifier_levels easel_view_presets

/* The zoom a press gives when the picture is at `zoom` and the selected level is level_pct: the level, or 100 if the zoom
   already is that level. */
int easel_magnifier_zoom (int zoom, int level_pct);

/* The preview box (REQ-0007-04) for the pointer over picture pixel (px, py) at 100 %: the canvas area (area_w x area_h
   device pixels, no scrollbars) divided by the level, rounded down and at least 1, with its top-left corner at the pointer
   minus half its size (rounded down), moved as far as needed to lie inside the pic_w x pic_h picture; a picture smaller than
   the box gives the whole picture in that direction. */
void easel_magnifier_box (int area_w, int area_h, int level_pct, int pic_w, int pic_h, int px, int py, int *x, int *y,
                          int *w, int *h);
