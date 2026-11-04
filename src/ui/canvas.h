/* SPDX-License-Identifier: GPL-3.0-or-later */
#pragma once

#include <gtk/gtk.h>

#include "history.h"
#include "image.h"
#include "selection.h"
#include "rich-text.h"
#include "text-buffer.h"
#include "text-put.h"
#include "footprint.h"
#include "tool.h"
#include "view.h"

G_BEGIN_DECLS

#define EASEL_TYPE_CANVAS (easel_canvas_get_type ())
G_DECLARE_FINAL_TYPE (EaselCanvas, easel_canvas, EASEL, CANVAS, GtkWidget)

/* Shows img (not owned; it must outlive the canvas or be replaced first). NULL shows nothing. */
void easel_canvas_set_image (EaselCanvas *self, EaselImage *img);

/* Re-uploads the pixels the image marked dirty and redraws. Call after every edit. */
void easel_canvas_update (EaselCanvas *self);

int easel_canvas_get_zoom (EaselCanvas *self);

/* The picture is shown as tiles of EASEL_CANVAS_TILE x EASEL_CANVAS_TILE picture pixels (fewer at the right and bottom
   edges), each a texture that reads the picture's own memory. An update rebuilds only the tiles it touches: at 8000x8000
   the Vulkan renderer uploads a whole texture each frame, which one big texture made cost 56 ms. 500 divides every zoom
   level into whole device pixels (500 * 10 % = 50), so tile edges never fall between device pixels. */
#define EASEL_CANVAS_TILE 500

/* Test hooks (0004 T001): the texture of tile (tx, ty), not owned (NULL outside the picture); how many tile textures
   have been built since the picture was set; how many bytes of the picture the canvas has copied (always 0: the tiles
   read the picture's own memory). */
GdkTexture *easel_canvas_get_tile_texture (EaselCanvas *self, int tx, int ty);
gsize easel_canvas_get_tiles_rebuilt (EaselCanvas *self);
/* Test hook (0015): the pixels of the floating selection's image made so far, and its whole size (a lazily stretched image is made in parts
   while the button is held, whole at the release). */
void easel_canvas_get_stretch_made (EaselCanvas *self, gsize *made, gsize *total);
gsize easel_canvas_get_selection_copies (EaselCanvas *self);      /* test hook: the copies a smear or a stamp left on the floating selection */
/* Test hooks of the preview of a floating selection (0014 T023): the tiles laid whole (the picture, the hole and every copy), the copies
   laid one by one into a tile, the tiles where the float was laid over the trail, and the textures made of the trail (a texture is made
   anew whenever its tile changes: one renderer may keep what it uploaded), all counted so far; the bytes the trail holds now; and a drop
   of everything kept, so that the next frame lays it all afresh. */
void easel_canvas_get_trail_counts (EaselCanvas *self, gsize *tiles, gsize *copies, gsize *floats, gsize *textures);
gsize easel_canvas_get_trail_bytes (EaselCanvas *self);
gsize easel_canvas_get_preview_bytes_held (EaselCanvas *self);   /* test hook: the trail's tiles and the float's tiles over it, as held now */
void easel_canvas_drop_preview (EaselCanvas *self);
/* Benchmark hook (0014 T015): draw the selection's line and its tint, or not, to measure what each costs a frame. Both are drawn by default. */
void easel_canvas_set_overlay_probe (EaselCanvas *self, gboolean line, gboolean tint);
/* The strength of the selection's tint, in percent (REQ-0014-17, REQ-0020-03): 0 draws none. */
void easel_canvas_set_tint (EaselCanvas *self, int percent);
/* "painted" comes once after each frame the canvas draws (0021). The CPU time its last frame took to build, in milliseconds (REQ-0021-12). */
double easel_canvas_get_frame_ms (EaselCanvas *self);
/* Test hooks: a drawing made slower by this many milliseconds of work; how many frames it has drawn. */
void easel_canvas_set_draw_delay (EaselCanvas *self, int ms);
guint easel_canvas_get_frames_drawn (EaselCanvas *self);
/* The keyboard's marker around one picture pixel (REQ-0022-05). It is unset until placed, and after a reset. */
void easel_canvas_marker_show (EaselCanvas *self, gboolean shown);
gboolean easel_canvas_marker_shown (EaselCanvas *self);
void easel_canvas_marker_get (EaselCanvas *self, int *x, int *y);
void easel_canvas_marker_set (EaselCanvas *self, int x, int y);
gboolean easel_canvas_marker_is_set (EaselCanvas *self);
void easel_canvas_marker_reset (EaselCanvas *self);
/* A press of the keyboard at the marker (REQ-0022-06, -07): TRUE if something now holds. Shift and Ctrl count for that press. */
void easel_canvas_key_press (EaselCanvas *self, guint button, gboolean ctrl, gboolean shift);
void easel_canvas_key_release (EaselCanvas *self);
/* Moves the marker by (dx, dy) picture pixels, kept inside the picture; while a press is held that is a move of the mouse (REQ-0022-08). */
void easel_canvas_key_move (EaselCanvas *self, int dx, int dy);
gboolean easel_canvas_is_key_held (EaselCanvas *self);
/* What the mouse does during a keyboard press (REQ-0022-10): TRUE if the event ends here. */
gboolean easel_canvas_key_hold_event (EaselCanvas *self, GdkEventType type, guint button);
/* The menu a secondary press at the marker opens, at the caret with a box open (REQ-0022-09). */
void easel_canvas_key_menu (EaselCanvas *self);
/* An open box moved by (dx, dy), or its far corner, as a drag of one edit (REQ-0022-12). */
void easel_canvas_text_nudge (EaselCanvas *self, gboolean corner, int dx, int dy);
/* The color a put-down would write at the marker (REQ-0022-13). */
uint32_t easel_canvas_marker_color (EaselCanvas *self);
/* Where the marker's square lies, in the canvas's logical pixels. */
void easel_canvas_marker_bounds (EaselCanvas *self, double *x0, double *y0, double *x1, double *y1);
/* A texture of the w x h picture pixels at (x0, y0), which must lie in the picture, as the put-down would write them: the picture, what is pending and an open box's
   letters, with no overlay (REQ-0021-07); NULL with no memory for it. */
GdkTexture *easel_canvas_compose_view (EaselCanvas *self, int x0, int y0, int w, int h);
const EaselImage *easel_canvas_get_image (EaselCanvas *self);
/* The canvas's checkerboard over bounds (REQ-0001-11). */
void easel_canvas_append_checkerboard (GtkSnapshot *snapshot, const graphene_rect_t *bounds);
/* Test hooks of the free-form line (0014 T015): its white line drawn as one strip per straight run (the default) or one per grid step,
   and how many white strips the last frame drew for it. */
/* Test hook (0015): draw the eight handles of a selection or not (they are drawn by default); the cases whose subject is the line and
   the tint of the selection do not, so that the handles do not cover the pixels they check. The handles' geometry is unchanged. */
void easel_canvas_set_selection_handles_shown (EaselCanvas *self, gboolean shown);
void easel_canvas_set_line_merge (EaselCanvas *self, gboolean merge);
gsize easel_canvas_get_line_strips (EaselCanvas *self);
/* Test hooks of the tint of a free-form selection (0014 T015): below 100 % drawn one band per device row (the default) or one rectangle per
   picture row as before, and how many rectangles the last frame drew for it. */
void easel_canvas_set_tint_bands (EaselCanvas *self, gboolean bands);
gsize easel_canvas_get_tint_rects (EaselCanvas *self);
gsize easel_canvas_get_bytes_copied (EaselCanvas *self);

/* Device pixels per logical pixel of the canvas's surface. */
double easel_canvas_get_scale (EaselCanvas *self);

/* While blocked (the color chooser is open, REQ-0003-14), presses on the canvas do nothing. */
void easel_canvas_set_input_blocked (EaselCanvas *self, gboolean blocked);

/* Where finished strokes are recorded as undo steps (not owned; NULL records nothing). */
void easel_canvas_set_history (EaselCanvas *self, EaselHistory *history);

void easel_canvas_set_tool (EaselCanvas *self, EaselTool tool);
void easel_canvas_set_colors (EaselCanvas *self, uint32_t foreground, uint32_t background);

/* While busy (a picture is loading, REQ-0005-02) the cursor over the canvas is the busy one. */
void easel_canvas_set_busy (EaselCanvas *self, gboolean busy);

/* Shows the picture from its start: zoom 100 %, scrolled to the top-left corner (REQ-0005-03, REQ-0005-07). */
void easel_canvas_reset_view (EaselCanvas *self);

/* What the Brush, the Airbrush and the Eraser paint with, as the options panel has it (REQ-0006-04..11): the footprint of
   the active tool, and whether the Eraser erases to transparency. */
void easel_canvas_set_footprint (EaselCanvas *self, EaselFootprint footprint);

/* How many surface pixels of a smooth scroll make one notch of the wheel. */
#define EASEL_CANVAS_PIXELS_PER_NOTCH 30.0

/* The Magnifier's selected level in percent (200, 400, ...), as the options panel has it (REQ-0007-02). */
void easel_canvas_set_magnifier_level (EaselCanvas *self, int pct);
/* The line width (1 to 5) and the fill style (0 outline, 1 outline and fill, 2 fill) of the shape tools (REQ-0010-02). */
void easel_canvas_set_shape_options (EaselCanvas *self, int width, int style);
void easel_canvas_set_shift (EaselCanvas *self, gboolean down);
/* A curve or polygon is pending (0011): shown over the picture, with a stage held or waiting for the next one. */
gboolean easel_canvas_is_pending (EaselCanvas *self);
/* The pending curve or polygon is drawn as shown (one undo step, none if it changes no pixel), or dropped with nothing drawn. */
gboolean easel_canvas_put_down (EaselCanvas *self);
/* The same for Save and Save As: a marked selection stays (REQ-0013-12). FALSE if a floating selection could not be put down
   for lack of memory (REQ-0013-19). */
gboolean easel_canvas_put_down_keeping_marked (EaselCanvas *self);
/* Enter: puts a waiting curve or polygon down (a polygon closed); does nothing while a stage's button is held. TRUE if one is
   pending, so the key is taken. */
gboolean easel_canvas_enter (EaselCanvas *self);
void easel_canvas_drop (EaselCanvas *self);
/* The wheel handler: dy in notches, the point in view coordinates; GDK_EVENT_PROPAGATE lets the view scroll as usual. */
gboolean easel_canvas_wheel (EaselCanvas *self, GdkModifierType state, double notches_dy, double view_x, double view_y);
/* The Magnifier's preview box in picture pixels (REQ-0007-04); FALSE when none is shown (not the Magnifier, not 100 %, a
   level that does not zoom in, or the pointer not over a picture pixel). */
gboolean easel_canvas_get_magnifier_box (EaselCanvas *self, int *x, int *y, int *w, int *h);

/* The window saw the pointer move, or a button go down, with no mouse button down: a release was lost (REQ-0007-18). Whatever
   press the canvas still holds (a stroke, a pick, a Fill or Magnifier press, a handle drag) ends as if it had been released. */
void easel_canvas_buttons_released (EaselCanvas *self);

/* Test hook: what a Fill may use for its work buffers before it fails with the memory dialog (default: no limit). */
void easel_canvas_set_fill_memory_limit (EaselCanvas *self, size_t max_bytes);
/* Test hook: the most a shape's or a gradient's undo log may use, to make a release or an apply fail (REQ-0010-20,
   REQ-0012-09; a gradient that fails emits "gradient-failed"). */
void easel_canvas_set_shape_memory_limit (EaselCanvas *self, size_t max_bytes);
/* Test hook: the most a frame's gradient preview may use for its blended pixels; a drag whose preview cannot get them is
   dropped at its release with "gradient-failed" (REQ-0012-09; default: no limit). */
void easel_canvas_set_preview_memory_limit (EaselCanvas *self, size_t max_bytes);
/* The marker of a waiting gradient's start (end 0) or end (end 1), in canvas coordinates (REQ-0012-13): FALSE when no marker
   is shown. */
gboolean easel_canvas_get_marker_rect (EaselCanvas *self, int end, graphene_rect_t *rect);
/* The selection (REQ-0013-04, 0013): TRUE and its rectangle in picture pixels if one is marked or floating, and whether it
   floats. */
gboolean easel_canvas_get_selection (EaselCanvas *self, int *x, int *y, int *w, int *h, gboolean *floating);
/* The size the status bar shows (REQ-0013-06): the part inside the picture of a rectangle being dragged out, or the
   selection's; FALSE for none. "selection-changed" (void) is emitted when it, or the selection, may have changed. */
gboolean easel_canvas_get_selection_size (EaselCanvas *self, int *w, int *h);
/* The shape of a marked or floating free-form selection (0014): w x h bytes, 1 for a pixel of its shape, over the rectangle that
   easel_canvas_get_selection gives; NULL for none and for a rectangle. */
const uint8_t *easel_canvas_get_selection_mask (EaselCanvas *self, int *w, int *h);
/* The corners of the line of a free-form selection as drawn, closed (x, y pairs), and how far the selection has moved from them (feature
   0015, REQ-0015-06); NULL for a selection with no line. */
const int *easel_canvas_get_selection_path (EaselCanvas *self, size_t *n, int *dx, int *dy);
/* Test hook: whether the canvas now asks for a frame every frame for the moving dashes of a selection's line (REQ-0014-17 c). */
gboolean easel_canvas_get_ants_running (EaselCanvas *self);
/* Test hooks of the moving dashes (0014 T015): the redraws they asked for so far and their step now (the whole logical pixels they have
   advanced, 12 a second); a fixed time for them (0: the real clock) and whether they advance continuously instead of in steps. */
void easel_canvas_get_ants_counts (EaselCanvas *self, gsize *redraws, gint64 *step);
gsize easel_canvas_get_ants_calls (EaselCanvas *self);              /* test hook: the times the dashes' callback ran so far */
void easel_canvas_set_ants_clock (EaselCanvas *self, gint64 usec, gboolean continuous);
/* Test hook: hides the markers and the guide line of a waiting gradient (they still take presses), so that a rendered window
   is the picture and the preview alone. */
void easel_canvas_set_guides_hidden (EaselCanvas *self, gboolean hidden);
gboolean easel_canvas_get_guides_hidden (EaselCanvas *self);
void easel_canvas_set_eraser_transparent (EaselCanvas *self, gboolean transparent);
/* The selection's mode (REQ-0013-02): what the next lift and Clear Selection leave behind, transparent or the background
   color (REQ-0013-08, -14); a floating selection is not changed by it (REQ-0013-13). */
void easel_canvas_set_selection_transparent (EaselCanvas *self, gboolean transparent);
/* Test hooks: the most a selection's float and copies may use (a lift, a Ctrl copy), and the most a put-down's or a Clear's
   undo log may use, to make them fail (REQ-0013-19). "selection-failed" (int kind: 0 move, 1 clear) is emitted then. */
void easel_canvas_set_selection_lift_limit (EaselCanvas *self, size_t max_bytes);
void easel_canvas_set_selection_step_limit (EaselCanvas *self, size_t max_bytes);
/* Ctrl held or not, from the window's key controller: a move of the selection pressed with it moves a copy (REQ-0013-09). */
void easel_canvas_set_ctrl (EaselCanvas *self, gboolean down);
/* An arrow key's move of the selection by (dx, dy) picture pixels (REQ-0013-10): the first lifts a marked one. FALSE, and
   nothing done, without a selection or while a button is held. */
gboolean easel_canvas_move_selection (EaselCanvas *self, int dx, int dy);
/* Shift with an arrow: as a Shift drag, no hole on a lift and a copy left behind (REQ-0014-11). */
gboolean easel_canvas_smear_selection (EaselCanvas *self, int dx, int dy);
/* Esc and Enter (REQ-0013-11, -12): puts a floating selection down and removes the selection; FALSE, nothing done, without
   one or while a button is held. */
gboolean easel_canvas_finish_selection (EaselCanvas *self);
/* The copy of the selection (REQ-0016-03): a new picture of its rectangle, its shape's pixels as they are, the rest #00000000; NULL with
   no selection or when its memory cannot be had (the selection's limit). */
EaselImage *easel_canvas_copy_selection (EaselCanvas *self);
/* The picture pixel at the top-left point of the part of the picture in view; (0, 0) while the picture's top-left corner is in view. */
void easel_canvas_top_left_in_view (EaselCanvas *self, int *x, int *y);
/* A paste (REQ-0016-05, -06): pic (taken) floats at (x, y), a rectangle with no hole whose fully transparent pixels leave the picture
   as it is; a floating selection is put down first and a marked one removed. FALSE, with nothing changed, if its memory cannot be had. */
gboolean easel_canvas_paste (EaselCanvas *self, EaselImage *pic, int x, int y);
/* A paste that grows the picture to width x height first (Resize Canvas to Picture, REQ-0016-07): the new area in fill, pic (taken)
   floating at (0, 0); the growth is one step with the put-down, and Undo while it floats takes it back; "picture-replaced" is emitted
   when the picture's size changes. FALSE, with nothing changed, if its memory cannot be had. */
gboolean easel_canvas_paste_grown (EaselCanvas *self, EaselImage *pic, int width, int height, uint32_t fill);
/* Whether the memory for a paste of pic_w x pic_h can be had, and for the grown picture grown_w x grown_h (0 for none): each checked on its
   own against the selection's limit and the system (REQ-0016-11), so that a refused paste is known before anything is changed. */
gboolean easel_canvas_can_paste (EaselCanvas *self, int pic_w, int pic_h, int grown_w, int grown_h);
/* The limit of the selection's memory (a test hook sets it). */
size_t easel_canvas_get_selection_limit (EaselCanvas *self);
/* Clear Selection (REQ-0013-14): FALSE, nothing done, without a selection or while a button is held. */
gboolean easel_canvas_clear_selection (EaselCanvas *self);
/* Select All (REQ-0013-15): a floating selection put down, then the whole picture marked. */
void easel_canvas_select_all (EaselCanvas *self);
/* Removes a marked selection (after an Undo or Redo, REQ-0013-12); a floating one is left alone. */
void easel_canvas_deselect (EaselCanvas *self);

/* Test hook: how many sprays the current or the last Airbrush stroke has made. */
int easel_canvas_get_spray_count (EaselCanvas *self);

/* A pencil stroke is in progress. Signal "stroking-changed" (gboolean stroking) comes when one starts, and when it ends or
   is cancelled. */
gboolean easel_canvas_is_stroking (EaselCanvas *self);
/* A flip, a turn or Invert Colors of the selection (REQ-0018-03, REQ-0018-07): a marked selection is lifted first, the float is made afresh
   (easel_selection_transform), its rectangle keeps its place (a quarter turn swaps its size), and the change joins the one step of its
   put-down. FALSE, nothing done, without a selection. If the lift cannot get its memory the dialog of REQ-0013-19 comes and the selection
   stays marked; if the new image cannot, the dialog of the command ("selection-failed" kind 4 flip, 5 rotate, 6 invert) and the selection
   is as it was (marked again if this command lifted it). */
gboolean easel_canvas_transform_selection (EaselCanvas *self, EaselOp op);
/* Stretch and Skew on the selection (REQ-0019-12): to W x H at its place, then the amounts, and it floats. FALSE, nothing done, without a
   selection. No change at all leaves it as it is, marked or floating (REQ-0019-13). If the lift cannot get its memory the dialog of
   REQ-0013-19 comes; if the rest cannot, "selection-failed" kind 7, with the selection as it was (marked again if this lifted it). */
gboolean easel_canvas_stretch_skew_selection (EaselCanvas *self, int W, int H, int amount_h, int amount_v);
/* Whether a Pan drag is held (REQ-0018-01: the Image commands wait for it, as for a stroke). */
gboolean easel_canvas_is_panning (EaselCanvas *self);

/* Cancels a stroke in progress, restoring the picture, and ignores the rest of that mouse press (REQ-0002-09). */
void easel_canvas_cancel_stroke (EaselCanvas *self);

/* A mouse button of a cancelled stroke is still held: history changes wait until it is released. */
gboolean easel_canvas_is_waiting_for_release (EaselCanvas *self);

/* The resize handles (REQ-0004-16..19). They show while a tool other than Pan is active and a picture exists, just
   outside its right edge, bottom edge and bottom-right corner (5 x 5 logical pixels, touching the picture), and the
   canvas reserves room for them. Rectangles are in the canvas's own logical coordinates. */
gboolean easel_canvas_get_handle_rect (EaselCanvas *self, EaselHandle handle, graphene_rect_t *rect);
/* A handle of the selection (0015), by the core's enum, as a 5 x 5 rectangle in canvas logical pixels; FALSE if there is no selection. */
gboolean easel_canvas_get_selection_handle_rect (EaselCanvas *self, EaselSelHandle handle, graphene_rect_t *rect);
/* The handle of the selection under a canvas point (an EaselSelHandle), or -1: the handle's own square first, then the 9 x 9 areas less the
   selection's rectangle, a corner before an edge handle (REQ-0015-02). */
int easel_canvas_selection_handle_at (EaselCanvas *self, double x, double y);

/* The handle whose 9 x 9 hit area holds the point (the corner wins an overlap), or EASEL_HANDLE_NONE; always NONE while
   the handles are hidden. */
EaselHandle easel_canvas_handle_at (EaselCanvas *self, double x, double y);

/* A handle drag is in progress; its outline is the size the picture would get (FALSE and untouched if none). */
gboolean easel_canvas_is_resizing (EaselCanvas *self);
gboolean easel_canvas_get_resize_outline (EaselCanvas *self, int *width, int *height);

/* Cancels a handle drag, changing nothing, and ignores the rest of that mouse press (REQ-0004-19). On release the canvas
   emits "resize-requested" (width, height) if the size differs from the one at the press. */
void easel_canvas_cancel_resize (EaselCanvas *self);

/* The window lost focus, so the pointer buttons can no longer be trusted: ends a stroke in progress (kept), forgets
   a cancelled stroke's button and a pan, and cancels a handle drag (REQ-0001-25, REQ-0002-09, REQ-0004-19). */
void easel_canvas_reset_pointer (EaselCanvas *self);

/* Ends a stroke in progress as if its button were released, keeping what was drawn (REQ-0001-25, REQ-0001-33). */
void easel_canvas_end_stroke (EaselCanvas *self);

/* Input entry points, in canvas coordinates (logical pixels from the picture's top-left). The event
   controller feeds them; tests call them directly. Emit "stroke-finished" (guint changed_pixels), also for a Fill (REQ-0007-05) and for a shape dragged in one go (0010);
   a curve or polygon put down emits "pending-changed" instead (0011);
   a Fill that cannot get its memory emits "fill-failed" and changes nothing (REQ-0007-07), and so does a shape release ("shape-failed", REQ-0010-20). A Pick Color press is a stroke
   as far as easel_canvas_is_stroking, Undo and the other button are concerned: "color-picked" (guint button, guint color)
   comes at the press and when the pointer moves to another pixel, "pick-ended" (gboolean cancelled) when it ends. */
void easel_canvas_press (EaselCanvas *self, guint button, double x, double y);
/* The same with the event's time in milliseconds, which a double-click is told by (0011); the tests give their own. */
void easel_canvas_press_timed (EaselCanvas *self, guint button, double x, double y, guint32 time_ms);
void easel_canvas_motion (EaselCanvas *self, double x, double y);
void easel_canvas_release (EaselCanvas *self, guint button);

/* Zoom commands from the menus and keys, with any tool (REQ-0004-04..07). Both emit "zoom-changed" (int percent) when the
   zoom changes. set_zoom keeps the center of the canvas area still as far as the edges allow; fit_to_window picks the
   largest level at which the whole picture fits the canvas area and shows its top-left corner. */
void easel_canvas_set_zoom (EaselCanvas *self, int pct);
void easel_canvas_fit_to_window (EaselCanvas *self);

/* The pixel grid (REQ-0004-12..14): on or off, the cell size in picture pixels, and whether it is axonometric. It is
   drawn over the picture, never saved, and only while one cell is at least 4 device pixels wide on screen. */
void easel_canvas_set_grid (EaselCanvas *self, gboolean on, int cell, gboolean axonometric);
gboolean easel_canvas_get_grid_on (EaselCanvas *self);
int easel_canvas_get_grid_cell (EaselCanvas *self);
gboolean easel_canvas_get_grid_axonometric (EaselCanvas *self);
/* On, and big enough to be drawn at the current zoom. */
gboolean easel_canvas_grid_is_drawn (EaselCanvas *self);

/* The pointer over the canvas area (REQ-0004-09): "pointer-moved" (int x, int y) with the picture pixel under it,
   negative or beyond the size when it is outside the picture, and "pointer-left" when it leaves the canvas area. The
   event controller feeds these; tests call them. (x, y) are in canvas coordinates, logical pixels from the picture's
   top-left. */
void easel_canvas_pointer_at (EaselCanvas *self, double x, double y);

/* The open text box (feature 0017): its left, top, width and height in picture pixels; FALSE with none. */
gboolean easel_canvas_text_box (EaselCanvas *self, int *x, int *y, int *w, int *h);
/* The starting style of a new box (REQ-0017-12, REQ-0017-24): its family, size in points and style flags (EASEL_TEXT_BOLD, ...); its color
   is the foreground color when the box is made. */
void easel_canvas_set_text_start_style (EaselCanvas *self, const char *family, int size_pt, guint8 flags);
/* The open box's text, NULL with none (tests; the edits of the window, T011). */
EaselTextBuffer *easel_canvas_text_buffer (EaselCanvas *self);
/* The box laid out and drawn again after its text or caret changed. */
void easel_canvas_text_edited (EaselCanvas *self);
/* A style field of the selected text set to `value`'s (one edit), or with nothing selected of the text typed next at the caret
   (REQ-0017-12); tried as an edit is (REQ-0017-07). FALSE with no box, or when refused. What the font card, Ctrl+B/I/U and a color
   pick call. */
gboolean easel_canvas_text_apply (EaselCanvas *self, EaselTextField field, const EaselTextStyle *value);
/* Each part of the selected text (or the text typed next) 1 point larger or smaller by `delta`, within 1 to 500, as one edit; nothing
   when no size changes (REQ-0017-12, Ctrl+] and Ctrl+[). FALSE with no box, or when refused. */
gboolean easel_canvas_text_step_size (EaselCanvas *self, int delta);
/* A style flag (EASEL_TEXT_BOLD, ...) toggled: on unless all the selected text (or the caret's style) has it (Ctrl+B/I/U, Ctrl+Shift+5). */
void easel_canvas_text_toggle (EaselCanvas *self, guint8 flag);
/* The selected text removed as one edit of `kind` (Cut, REQ-0017-19); FALSE with no box or nothing selected, or when refused. */
gboolean easel_canvas_text_delete_selection (EaselCanvas *self, EaselTextEditKind kind);
/* `len` bytes of pasted text put in at the caret over the selection as one edit (REQ-0017-20): with the n parts of `rich` (NULL: the
   style at the caret for all), line breaks made new lines and a tab a space; tried as any edit is (REQ-0017-07). */
gboolean easel_canvas_text_paste (EaselCanvas *self, const char *text, size_t len, const EaselRichRun *rich, size_t n_rich);
/* The style of the selected text, or at the caret, and in `mixed` the fields that differ within the selection (REQ-0017-14). */
void easel_canvas_text_selection_style (EaselCanvas *self, EaselTextStyle *out, unsigned *mixed);
/* Select All with a box open: all its text selected (REQ-0017-17); FALSE with no box. */
gboolean easel_canvas_text_select_all (EaselCanvas *self);
/* Undo or Redo of the open box's edits (REQ-0017-08): one edit taken back or done again, a move or resize taking the box to its place;
   Undo with no edit left closes the box with nothing put down. FALSE with no box open. */
gboolean easel_canvas_text_step (EaselCanvas *self, gboolean undo);
/* Whether the open box has an edit to redo; FALSE with no box. */
gboolean easel_canvas_text_can_redo (EaselCanvas *self);
/* The box's pixels (REQ-0017-16); FALSE with no box. */
gboolean easel_canvas_text_ink (EaselCanvas *self, EaselTextInk *ink);
/* The style at the caret, a line's start being the text's or a line started by Enter (REQ-0017-26). */
void easel_canvas_text_style_at_caret (EaselCanvas *self, EaselTextStyle *out);
/* The Text tool's mode: TRUE for Opaque (REQ-0017-02). */
void easel_canvas_set_text_opaque (EaselCanvas *self, gboolean opaque);
/* A key for the open box, as the window's key handler gives it (REQ-0017-06): TRUE if the box took it. */
gboolean easel_canvas_text_key (EaselCanvas *self, guint keyval, GdkModifierType state);
/* Text an input method made (its "commit"), or a typed character: inserted at the caret, as one edit of a run of typing. */
void easel_canvas_text_commit (EaselCanvas *self, const char *utf8);
/* A key event given to the input method first: TRUE if it took it. */
gboolean easel_canvas_text_filter (EaselCanvas *self, GdkEvent *event);
/* The canvas point of the character at byte `index` of the box's text (tests). */
void easel_canvas_text_point (EaselCanvas *self, size_t index, double *x, double *y);
/* The most the box's pixels may use (a test hook, REQ-0017-07), and the error bells sounded (a test hook). */
void easel_canvas_set_text_show_limit (EaselCanvas *self, size_t max_bytes);
/* The most a view built on the drawn edges may ask for at once, as if the memory ended there (a test hook; the edge case of 0025). */
void easel_canvas_set_view_memory_limit (EaselCanvas *self, size_t max_bytes);
/* Whether the box's caret is shown now (a test hook). */
gboolean easel_canvas_text_caret_on (EaselCanvas *self);
/* The most the put-down's step may use (a test hook, REQ-0017-18). */
void easel_canvas_set_text_step_limit (EaselCanvas *self, size_t max_bytes);
guint easel_canvas_get_error_bells (EaselCanvas *self);
void easel_canvas_pointer_left (EaselCanvas *self);

/* The touchpad or touch pinch (REQ-0004-08). The gesture feeds these; tests call them. scale is the gesture's scale since
   it began; (view_x, view_y) is its center relative to the canvas area. Pan only. */
void easel_canvas_pinch_begin (EaselCanvas *self);
void easel_canvas_pinch (EaselCanvas *self, double scale, double view_x, double view_y);
void easel_canvas_pinch_end (EaselCanvas *self);

/* In Pan mode, moves notches zoom levels (positive = in), keeping the picture point at view (x, y) still
   (REQ-0001-16). view_x, view_y are relative to the visible area. */
void easel_canvas_zoom_at (EaselCanvas *self, int notches, double view_x, double view_y);

/* Takes pointer input from area (the canvas's scrolled area), so strokes and pans can start outside the
   picture (REQ-0001-14, REQ-0001-22). */
void easel_canvas_listen_on (EaselCanvas *self, GtkWidget *area);

/* The checkerboard drawn behind transparency, on screen only (REQ-0001-11, REQ-0003-05). */
void easel_snapshot_append_checkerboard (GtkSnapshot *snapshot, const graphene_rect_t *bounds, float cell);

G_END_DECLS
