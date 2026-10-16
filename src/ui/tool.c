/* SPDX-License-Identifier: GPL-3.0-or-later */

#include "tool.h"

static const char *const names[] = {
  [EASEL_TOOL_PAN] = "pan",
  [EASEL_TOOL_PENCIL] = "pencil",
  [EASEL_TOOL_BRUSH] = "brush",
  [EASEL_TOOL_AIRBRUSH] = "airbrush",
  [EASEL_TOOL_ERASER] = "eraser",
};

gboolean
easel_tool_from_name (const char *name, EaselTool *tool)
{
  for (guint i = 0; i < G_N_ELEMENTS (names); i++)
    if (g_strcmp0 (names[i], name) == 0)
      {
        *tool = (EaselTool) i;
        return TRUE;
      }
  return FALSE;
}
