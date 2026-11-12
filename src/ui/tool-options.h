/* SPDX-License-Identifier: GPL-3.0-or-later */
/* The options panel below the toolbox (REQ-0006-02, REQ-0006-03): the presets of the Brush, the Airbrush and the Eraser. */
#pragma once

#include <gtk/gtk.h>

#include "gradient.h"

#include "footprint.h"
#include "shapes.h"
#include "tool.h"

G_BEGIN_DECLS

#define EASEL_TYPE_TOOL_OPTIONS (easel_tool_options_get_type ())
G_DECLARE_FINAL_TYPE (EaselToolOptions, easel_tool_options, EASEL, TOOL_OPTIONS, GtkBox)

EaselToolOptions *easel_tool_options_new (void);

/* Shows the presets of the tool, or nothing (and hides the panel with its line) for Pan and Pencil. */
void easel_tool_options_show_tool (EaselToolOptions *self, EaselTool tool);

/* The selected footprint of a tool (Brush, Airbrush or Eraser); each tool keeps its own for the session. */
EaselFootprint easel_tool_options_get_footprint (EaselToolOptions *self, EaselTool tool);
/* What the tools start with, by their place in the panel (REQ-0020-05). */
void easel_tool_options_set_start (EaselToolOptions *self, int brush, int airbrush, int eraser, gboolean transparent);

/* The Eraser's mode: TRUE for Transparent. */
gboolean easel_tool_options_get_transparent (EaselToolOptions *self);

/* The selection's Opaque / Transparent mode: TRUE for Transparent; its own, kept for the session (REQ-0013-02). */
gboolean easel_tool_options_get_select_transparent (EaselToolOptions *self);

/* The Text tool's Opaque / Transparent mode: TRUE for Transparent; its own, Transparent at each start, kept for the session
   (REQ-0017-02). */
gboolean easel_tool_options_get_text_transparent (EaselToolOptions *self);

/* Numpad + or - (direction 1 or -1): the next larger or smaller preset of the tool; the ends stay. Returns TRUE if the
   selection changed. */
gboolean easel_tool_options_step (EaselToolOptions *self, EaselTool tool, int direction);
/* The names of a tool's size presets (Brush, Airbrush, Eraser, or the line widths), NULL-terminated, or NULL for a tool
   without sizes; and the selected one's index (line_width for the tools with widths) (REQ-0030-06, -08). */
GStrv easel_tool_options_size_names (EaselTool tool);
int easel_tool_options_size_index (EaselToolOptions *self, EaselTool tool, int line_width);

/* The Magnifier's selected level in percent: 100, 200, 400, 600, 800 or 1600 (REQ-0007-02); kept for the session. */

/* Gradient's chosen type, Linear at each start and kept for the session (REQ-0032-01, -03). */
EaselGradientType easel_tool_options_get_gradient_type (EaselToolOptions *self);
/* Sharp or Smooth (0039): the Brush's cells show each footprint as a click paints it then (REQ-0039-11). */
void easel_tool_options_set_smooth (EaselToolOptions *self, gboolean smooth);
/* While a stroke is in progress, choosing a preset is ignored (REQ-0006-03). */
void easel_tool_options_set_busy (EaselToolOptions *self, gboolean busy);

/* The chosen shape shown on the Shape button: its outline and its accessible name, "Shape:" and the shape's name. */
void easel_tool_options_set_shape (EaselToolOptions *self, EaselShapeKind shape);
/* The Shape button's popover of the 19 shapes: while it is open the window's keys and the canvas's presses do nothing. */
GtkPopover *easel_tool_options_get_shape_popover (EaselToolOptions *self);
/* A shape's icon (REQ-0043-16): Rectangle, Rounded Rectangle and Ellipse their tool icons, the others their own; and its name, translated. */
char *easel_shape_icon_name (EaselShapeKind shape);
const char *easel_shape_label (EaselShapeKind shape);

/* Signal "preset-changed" (EaselToolOptions *, gpointer data): the selected footprint or the Eraser's mode changed. */

G_END_DECLS
