/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Finding, reading and creating the settings file (REQ-0001-01, REQ-0001-04, REQ-0001-05). */
#pragma once

#include <glib.h>

#include "settings.h"

G_BEGIN_DECLS

/* ~/.config/easel/settings.ini (honours XDG_CONFIG_HOME). Free with g_free. */
char *easel_settings_file_path (void);

/* Fills *s from the file at path. A missing file is created with the defaults; a file that cannot be read,
   or a file that cannot be created, gives the defaults. Never reports to the user (REQ-0001-05). */
void easel_settings_file_load (const char *path, EaselSettings *s);

G_END_DECLS
