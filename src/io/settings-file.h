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

/* One key of the file: its section, its name, and its new text, or NULL to remove it. */
typedef struct
{
  const char *section, *key, *value;
} EaselSettingChange;

/* The changes made to the file as it is on disk now, all in one step, or none (REQ-0020-09, REQ-0020-10): every other byte stays. A missing file
   or folder is made with the defaults first; a link stays and its target is written; the file's mode stays. FALSE, with the file exactly as it
   was, if it cannot be read or written, may not be written, is a folder or is unparseable. */
gboolean easel_settings_file_change (const char *path, const EaselSettingChange *changes, size_t n);

G_END_DECLS
