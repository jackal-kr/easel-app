/* SPDX-License-Identifier: GPL-3.0-or-later */
/* The one list of tools; the toolbox, the win.tool action and the canvas all use it. */
#pragma once

#include <glib.h>

G_BEGIN_DECLS

typedef enum
{
  EASEL_TOOL_PAN,
  EASEL_TOOL_PENCIL,
  EASEL_TOOL_BRUSH,
  EASEL_TOOL_AIRBRUSH,
  EASEL_TOOL_ERASER,
} EaselTool;

/* Reads a win.tool action target ("pan", "pencil", "brush", "airbrush", "eraser"). */
gboolean easel_tool_from_name (const char *name, EaselTool *tool);

G_END_DECLS
