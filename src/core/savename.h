/* SPDX-License-Identifier: GPL-3.0-or-later */
/* File names for Save: only PNG in feature 0001 (REQ-0001-28). */
#pragma once

typedef enum
{
  EASEL_SAVENAME_OK,
  EASEL_SAVENAME_NOT_PNG,     /* the name has another extension */
  EASEL_SAVENAME_NO_MEMORY,
} EaselSavenameResult;

/* On OK, *fixed is path with ".png" added when it has no extension (free with free()); otherwise *fixed is NULL.
   The extension is what follows the last dot of the file name, unless that dot starts the name. */
EaselSavenameResult easel_savename_fix (const char *path, char **fixed);
