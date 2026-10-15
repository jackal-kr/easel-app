/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Saving the picture as PNG (REQ-0001-30, REQ-0001-31). */
#pragma once

#include <glib.h>

#include "image.h"

G_BEGIN_DECLS

/* Writes img to path as PNG, keeping transparency. The file is replaced atomically: on failure an existing
   file keeps its old content, and error holds the system's reason. */
gboolean easel_png_save (const EaselImage *img, const char *path, GError **error);

G_END_DECLS
