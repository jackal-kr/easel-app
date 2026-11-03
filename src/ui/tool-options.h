/* SPDX-License-Identifier: GPL-3.0-or-later */
/* The options panel below the toolbox (REQ-0006-02, REQ-0006-03): the presets of the Brush, the Airbrush and the Eraser. */
#pragma once

#include <gtk/gtk.h>

#include "footprint.h"
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

/* The Magnifier's selected level in percent: 100, 200, 400, 600, 800 or 1600 (REQ-0007-02); kept for the session. */

/* While a stroke is in progress, choosing a preset is ignored (REQ-0006-03). */
void easel_tool_options_set_busy (EaselToolOptions *self, gboolean busy);

/* Signal "preset-changed" (EaselToolOptions *, gpointer data): the selected footprint or the Eraser's mode changed. */

G_END_DECLS
