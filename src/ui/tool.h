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
  EASEL_TOOL_MAGNIFIER,
  EASEL_TOOL_PICK,
  EASEL_TOOL_FILL,
  EASEL_TOOL_LINE,
  EASEL_TOOL_RECTANGLE,
  EASEL_TOOL_ROUNDED_RECTANGLE,
  EASEL_TOOL_ELLIPSE,
  EASEL_TOOL_CURVE,
  EASEL_TOOL_POLYGON,
} EaselTool;

/* Reads a win.tool action target ("pan", "pencil", "brush", "airbrush", "eraser", "magnifier", "pick", "fill", "line", "rectangle",
   "rounded-rectangle", "ellipse", "curve", "polygon"). */
gboolean easel_tool_from_name (const char *name, EaselTool *tool);

/* TRUE for Line, Rectangle, Rounded Rectangle and Ellipse: the tools that draw a shape by one drag (0010). */
gboolean easel_tool_draws_shape (EaselTool tool);

/* TRUE for Curve and Polygon: the tools that draw by several clicks and keep a pending shape between them (0011). */
gboolean easel_tool_draws_pending (EaselTool tool);

/* The win.tool action target of a tool: "pan", "pencil", ... */
const char *easel_tool_name (EaselTool tool);

G_END_DECLS
