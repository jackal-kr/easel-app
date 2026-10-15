/* SPDX-License-Identifier: GPL-3.0-or-later */
/* The bar below the canvas: the fore/background indicator and the 28 palette swatches (REQ-0003-01..05). */
#pragma once

#include <gtk/gtk.h>

#include "palette.h"

G_BEGIN_DECLS

#define EASEL_TYPE_SWATCH (easel_swatch_get_type ())
G_DECLARE_FINAL_TYPE (EaselSwatch, easel_swatch, EASEL, SWATCH, GtkWidget)

#define EASEL_TYPE_COLOR_INDICATOR (easel_color_indicator_get_type ())
G_DECLARE_FINAL_TYPE (EaselColorIndicator, easel_color_indicator, EASEL, COLOR_INDICATOR, GtkWidget)

#define EASEL_TYPE_PALETTE_BAR (easel_palette_bar_get_type ())
G_DECLARE_FINAL_TYPE (EaselPaletteBar, easel_palette_bar, EASEL, PALETTE_BAR, GtkBox)

uint32_t easel_swatch_get_color (EaselSwatch *self);

void easel_palette_bar_set_palette (EaselPaletteBar *self, const EaselPalette *palette);
void easel_palette_bar_set_colors (EaselPaletteBar *self, uint32_t foreground, uint32_t background);
uint32_t easel_palette_bar_get_foreground (EaselPaletteBar *self);
uint32_t easel_palette_bar_get_background (EaselPaletteBar *self);

/* n is the swatch number, 1..28 (REQ-0003-01). */
EaselSwatch *easel_palette_bar_get_swatch (EaselPaletteBar *self, int n);
EaselColorIndicator *easel_palette_bar_get_indicator (EaselPaletteBar *self);
GtkButton *easel_palette_bar_get_swap_button (EaselPaletteBar *self);

/* Where the indicator draws its front (foreground) or back (background) square, in its own coordinates
   (REQ-0003-18). */
void easel_color_indicator_get_square (EaselColorIndicator *self, gboolean front, graphene_rect_t *out);

/* A press on swatch n (1..28): the swatch's click gesture feeds it; tests call it directly. The primary and secondary
   buttons focus the swatch and emit "swatch-pressed" (int n, guint button, int n_press); other buttons do nothing
   (REQ-0003-06). */
void easel_palette_bar_press (EaselPaletteBar *self, int n, guint button, int n_press);

/* A click on the indicator's front (foreground) or back (background) square: emits "indicator-pressed"
   (gboolean front) (REQ-0003-13). Each swatch's F2 key, action "swatch.edit", emits "swatch-edit" (int n)
   (REQ-0003-11). */
void easel_palette_bar_press_indicator (EaselPaletteBar *self, gboolean front);

void easel_palette_bar_set_swatch (EaselPaletteBar *self, int n, uint32_t color);

G_END_DECLS
