/* SPDX-License-Identifier: GPL-3.0-or-later */
/* The palette file the app owns (REQ-0003-15, REQ-0003-16, REQ-0003-17). */
#pragma once

#include <glib.h>

#include "palette.h"

G_BEGIN_DECLS

/* ~/.config/easel/palette.ini (honours XDG_CONFIG_HOME). Free with g_free. */
char *easel_palette_file_path (void);

/* Fills *p from the file. A missing, unreadable or unparseable file gives the defaults; the file is never created. */
void easel_palette_file_load (const char *path, EaselPalette *p);

/* Writes the whole palette, replacing the file in one step. Returns FALSE if it could not; nothing is reported to the
   user (REQ-0003-16). */
gboolean easel_palette_file_save (const char *path, const EaselPalette *p);

G_END_DECLS
