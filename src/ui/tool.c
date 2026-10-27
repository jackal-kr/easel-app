/* SPDX-License-Identifier: GPL-3.0-or-later */

#include "tool.h"

static const char *const names[] = {
  [EASEL_TOOL_PAN] = "pan",
  [EASEL_TOOL_PENCIL] = "pencil",
  [EASEL_TOOL_BRUSH] = "brush",
  [EASEL_TOOL_AIRBRUSH] = "airbrush",
  [EASEL_TOOL_ERASER] = "eraser",
  [EASEL_TOOL_MAGNIFIER] = "magnifier",
  [EASEL_TOOL_PICK] = "pick",
  [EASEL_TOOL_FILL] = "fill",
  [EASEL_TOOL_LINE] = "line",
  [EASEL_TOOL_RECTANGLE] = "rectangle",
  [EASEL_TOOL_ROUNDED_RECTANGLE] = "rounded-rectangle",
  [EASEL_TOOL_ELLIPSE] = "ellipse",
  [EASEL_TOOL_CURVE] = "curve",
  [EASEL_TOOL_POLYGON] = "polygon",
  [EASEL_TOOL_GRADIENT] = "gradient",
  [EASEL_TOOL_SELECT] = "select",
  [EASEL_TOOL_FREE_SELECT] = "free-select",
  [EASEL_TOOL_TEXT] = "text",
};

gboolean
easel_tool_draws_shape (EaselTool tool)
{
  return tool == EASEL_TOOL_LINE || tool == EASEL_TOOL_RECTANGLE || tool == EASEL_TOOL_ROUNDED_RECTANGLE
         || tool == EASEL_TOOL_ELLIPSE;
}

gboolean
easel_tool_draws_pending (EaselTool tool)
{
  return tool == EASEL_TOOL_CURVE || tool == EASEL_TOOL_POLYGON || tool == EASEL_TOOL_GRADIENT;
}

gboolean
easel_tool_selects (EaselTool tool)
{
  return tool == EASEL_TOOL_SELECT || tool == EASEL_TOOL_FREE_SELECT;
}

gboolean
easel_tool_uses_line_width (EaselTool tool)
{
  return easel_tool_draws_shape (tool) || tool == EASEL_TOOL_CURVE || tool == EASEL_TOOL_POLYGON;
}

const char *
easel_tool_name (EaselTool tool)
{
  return names[tool];
}

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
