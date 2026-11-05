/* SPDX-License-Identifier: GPL-3.0-or-later */
/* The thumbnail (REQ-0021-07): a floating card over the view, showing the picture at 100 % from the pixel at the view's top left. It is a panel: it never writes
   a picture pixel, and asks the canvas for what its put-down would write. */
#pragma once

#include <gtk/gtk.h>

#include "canvas.h"

G_BEGIN_DECLS

#define EASEL_TYPE_THUMBNAIL_CARD (easel_thumbnail_card_get_type ())
G_DECLARE_FINAL_TYPE (EaselThumbnailCard, easel_thumbnail_card, EASEL, THUMBNAIL_CARD, GtkWidget)

/* The canvas it shows and the scrolled window that shows the canvas; it follows both. */
void easel_thumbnail_card_set_view (EaselThumbnailCard *self, EaselCanvas *canvas, GtkScrolledWindow *scroller);
GtkWidget *easel_thumbnail_card_get_area (EaselThumbnailCard *self);

G_END_DECLS
