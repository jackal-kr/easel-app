/* SPDX-License-Identifier: GPL-3.0-or-later */
/* The floating selection of feature 0013. The picture is not touched while a selection floats: the float holds the lifted
   pixels, its place, the hole it left (REQ-0013-08) and the copies that Ctrl leaves (REQ-0013-09); one function lays the
   result over picture pixels, for the preview and for the put-down, which is one undo step (REQ-0013-11). */
#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "history.h"
#include "black-white.h"
#include "image.h"
#include "transform.h"

/* A copy left in the picture by a Ctrl drag of a float: where. */
typedef struct
{
  int x, y;
} EaselStamp;

/* An image a copy was made with, kept after the float's own image changed by a stretch (0015). */
typedef struct
{
  int w, h;
  uint32_t *px;
  uint8_t *mask;
} EaselKept;

/* The image a copy was made with: the float's current one, the one as lifted, or a kept one by its number. */
#define EASEL_IMAGE_CURRENT (-1)
#define EASEL_IMAGE_ORIGINAL (-2)

/* The pixels (w x h, 0xAARRGGBB straight alpha) as they are now at the place (x, y) of the picture, which may be partly or wholly
   outside it; the hole at (hx, hy), ow x oh (the size as lifted), in hole_color (none after a lift with Ctrl); the stamps, in their
   order. mask is NULL for a rectangle, else w x h bytes, 1 for a pixel of the shape (0014): only those pixels are lifted, leave the
   hole, are laid and put down. After a stretch (0015) px, mask, w and h are the stretched image, always made from the pixels as
   lifted, which orig_px and orig_mask then hold (NULL until the first stretch: px and mask are then the original); the hole keeps
   the original's size and shape; stamp_img says what image each copy was made with (NULL: all the current one) and kept holds the
   images of copies made before a later stretch. */
typedef struct
{
  int x, y, w, h;
  uint32_t *px;
  uint8_t *mask;
  bool hole;
  int hx, hy;
  uint32_t hole_color;
  EaselStamp *stamps;
  size_t n_stamps, cap;
  int ow, oh;
  uint32_t *orig_px;
  uint8_t *orig_mask;
  int *stamp_img;
  EaselKept *kept;
  size_t n_kept;
  uint8_t *fill;                   /* a lazily stretched image: per 256 x 256 block of it, 1 once made; NULL when whole */
  int fill_cols, fill_rows;
  size_t made_px, kept_px;         /* the pixels of the current image made so far, and those of them in the shape */
  bool skip_clear;                 /* a paste (0016): its fully transparent pixels are neither laid nor put down, nor its copies' */
  bool hole_fixed;                 /* after a skew (0019): the hole is hole_w x hole_h, in the shape hole_mask (NULL: all), since the lifted image has changed */
  int hole_w, hole_h;
  uint8_t *hole_mask;
  EaselOrient orient;              /* the flips and turns made to it (0018): the shown image is the lifted one scaled to the size before
                                      them, then oriented (REQ-0018-07); the identity until the first */
} EaselFloat;

typedef enum
{
  EASEL_SELECTION_DONE,       /* done; for a put-down, one undo step (with a history) */
  EASEL_SELECTION_NOTHING,    /* no pixel of the picture differs: no step */
  EASEL_SELECTION_NO_MEMORY   /* nothing changed */
} EaselSelectionResult;

/* Lifts the rectangle (x, y, w, h), inside the picture, into f (which must be empty): its pixels are copied, and unless copy
   it leaves a hole in hole_color. With a mask (w x h bytes, copied) only the pixels of the shape are part of it and leave the
   hole. NO_MEMORY, with f empty, if its w x h x 4 bytes (and the mask's w x h) exceed max_bytes or cannot be had. */
EaselSelectionResult easel_selection_lift (EaselFloat *f, const EaselImage *img, int x, int y, int w, int h,
                                           const uint8_t *mask, bool copy, uint32_t hole_color, size_t max_bytes);

/* A float made from a pasted picture (REQ-0016-05, -06): f (which must be empty) holds a copy of pic's pixels at (x, y), a rectangle
   with no hole, and its fully transparent pixels (alpha 0) leave the picture as it is wherever the float or its copies are laid or put
   down, also after a stretch. NO_MEMORY, with f empty, if its 4 bytes a pixel exceed max_bytes or cannot be had. */
EaselSelectionResult easel_selection_paste (EaselFloat *f, const EaselImage *pic, int x, int y, size_t max_bytes);

/* The copy of a selection (REQ-0016-03): a new w x h picture of its bounding rectangle, the pixels of its shape as they are and every
   other pixel #00000000. Without f, the marked selection (x, y, w, h) of img with its mask (NULL: a rectangle), which must be inside
   the picture; with f, the floating selection's own pixels and shape as they float (x, y, w, h being its own; its image whole). NULL if
   its 4 bytes a pixel exceed max_bytes or cannot be had. */
EaselImage *easel_selection_copy_image (const EaselImage *img, const EaselFloat *f, int x, int y, int w, int h, const uint8_t *mask,
                                        size_t max_bytes);

/* The size of a w x h picture fitted into W x H with its ratio kept (REQ-0016-07): the side that limits takes W or H, the other is
   rounded to the nearest pixel, halves up, at least 1. */
void easel_fit_size (int w, int h, int W, int H, int *nw, int *nh);

/* As easel_selection_put_down for a paste that grew the picture from old_w x old_h (the new area in fill): one step with the growth,
   added even when no pixel changes (REQ-0016-07); Undo crops back. */
EaselSelectionResult easel_selection_put_down_grown (const EaselFloat *f, bool with_float, EaselImage *img, EaselHistory *history,
                                                     int old_w, int old_h, uint32_t fill, size_t max_bytes);

/* Leaves a copy of the float where it is now, as it would be put down there (REQ-0013-09); it belongs to the float's one
   step. FALSE, with nothing added, if the stamps would need more than max_bytes or cannot grow. */
bool easel_selection_stamp (EaselFloat *f, size_t max_bytes);

/* The rectangle of the picture, corners included, in which the lay can differ from the picture: the hole, the copies and, with
   with_float, the float, clipped to the picture. FALSE if there is none. */
bool easel_selection_bounds (const EaselFloat *f, bool with_float, const EaselImage *img, int *x0, int *y0, int *x1, int *y1);

/* The float stretched to W x H at (x, y) (REQ-0015-05): its image is made afresh from the pixels as lifted, by easel_selection_scale,
   and copies left so far keep the image they were made with. NO_MEMORY, with the float, its copies and its place as they were, if the
   new image (4 bytes a pixel and a byte for a mask) exceeds max_bytes or the memory for it, or for the record of the copies' images,
   cannot be had. */
EaselSelectionResult easel_selection_stretch (EaselFloat *f, int x, int y, int W, int H, size_t max_bytes);

/* The same stretch with the pixels not made yet (the speed of a drag over a big selection): the memory of the whole image is had now
   and refused as the full stretch's is, but only the 256 x 256 blocks asked for by easel_selection_fill are made, with the pixels the
   full stretch makes, the rest being zero until easel_selection_complete makes them (a shape the nearest pixel lost keeps its one pixel
   then). What lays, puts down or copies the float reads an image that is whole or filled where it reads: the caller fills or completes
   first (easel_selection_stamp completes by itself). The next stretch replaces the image; a copy never uses an unfinished image. */
EaselSelectionResult easel_selection_stretch_lazy (EaselFloat *f, int x, int y, int W, int H, size_t max_bytes);

/* The float skewed by the two amounts of easel_ss_size (REQ-0019-12), at its place: the shown image, with its shape, becomes the new image as
   lifted, the orientation starts again, and what no row or column reaches is outside the shape. The hole and the copies made before keep
   theirs. NOTHING with no amount; NO_MEMORY, with the float as it was, past max_bytes (each allocation) or if one cannot be had. */
EaselSelectionResult easel_selection_skew (EaselFloat *f, int amount_h, int amount_v, size_t max_bytes);

/* Black and White of the float (REQ-0034-03): the image as shown (made whole), with its shape, converted (easel_bw_convert), becomes
   the image as lifted at the shown size, the orientation starting again, as a skew makes its image; the hole and the copies made before
   keep theirs. NO_MEMORY, with the float as it was, past max_bytes (each allocation) or if one cannot be had. */
EaselSelectionResult easel_selection_black_white (EaselFloat *f, EaselBw how, size_t max_bytes);

/* The float's image, made whole, replaced by px (its shown size, copied), its shape kept, as Black and White does (Remove background,
   REQ-0035-02): px becomes the image as lifted, the orientation starting again; the hole and the copies made before keep theirs.
   NO_MEMORY, the float as it was, past max_bytes or if an allocation cannot be had. */
EaselSelectionResult easel_selection_replace_image (EaselFloat *f, const uint32_t *px, size_t max_bytes);

/* The same two stretches with an orientation given (REQ-0018-11): the float's orientation becomes `orient`, the shown image being the
   lifted pixels scaled to W x H before it and then oriented; the plain ones keep the float's own. */
EaselSelectionResult easel_selection_stretch_oriented (EaselFloat *f, int x, int y, int W, int H, EaselOrient orient, size_t max_bytes);
EaselSelectionResult easel_selection_stretch_lazy_oriented (EaselFloat *f, int x, int y, int W, int H, EaselOrient orient,
                                                            size_t max_bytes);

/* Makes the blocks of a lazily stretched image that meet the rectangle [x0, x1) x [y0, y1) of its pixels (clipped), if not made. */
void easel_selection_fill (EaselFloat *f, int x0, int y0, int x1, int y1);

/* Makes the whole image of a lazily stretched float; nothing to do for a float whose image is whole. */
void easel_selection_complete (EaselFloat *f);

/* A flip, a turn or Invert Colors of the float (REQ-0018-07), as the commands of the Image menu: the shown image is made afresh, the
   lifted pixels staying as lifted, so that the shown image is exactly the command of what was shown; the place is kept (a quarter turn
   swaps the width and the height), the hole and the copies left before are not touched. Invert inverts the lifted and the shown pixels,
   not the shape; the copies keep the pixels they have. NO_MEMORY, with the float as it was, if the new image (4 bytes a pixel and a
   byte for a mask, and what the copies' record needs) exceeds max_bytes or cannot be had. NOTHING for Clear Image (no selection's). */
EaselSelectionResult easel_selection_transform (EaselFloat *f, EaselOp op, size_t max_bytes);

/* The eight handles of a selection, by the edges they stand for. */
typedef enum
{
  EASEL_SEL_HANDLE_TL, EASEL_SEL_HANDLE_T, EASEL_SEL_HANDLE_TR,
  EASEL_SEL_HANDLE_L,                       EASEL_SEL_HANDLE_R,
  EASEL_SEL_HANDLE_BL, EASEL_SEL_HANDLE_B, EASEL_SEL_HANDLE_BR
} EaselSelHandle;

/* The rectangle (x, y, w, h) of a selection dragged by a handle through (dx, dy) picture pixels (REQ-0015-03, -07): the edges the
   handle stands for move, the opposite ones stay; each side is 1 to 10000, a side dragged past the opposite edge being 1 pixel
   beside it. With keep_ratio and a corner handle the ratio of the size at the press is kept: after the limit the side whose size
   differs more from its size at the press, as a factor, leads (the width on a tie), the other is its size times the factor, halves
   up, at least 1, and if that passes 10000 the leading side is reduced so that both fit; an edge handle ignores keep_ratio. */
void easel_stretch_rect (EaselSelHandle handle, int x, int y, int w, int h, int dx, int dy, bool keep_ratio, int *nx, int *ny, int *nw,
                         int *nh);

/* The same drag, a side dragged past the opposite edge mirroring instead (REQ-0018-11): the size of a side is the distance from the
   opposite edge to the dragged one, 1 to 10000, and *mirror_x / *mirror_y say that the dragged edge is past the opposite one (on the
   side it was not on); with the dragged edge on the opposite edge the side is 1 pixel, not mirrored, on the side it was. With keep_ratio
   a corner keeps the ratio as easel_stretch_rect does, the side that does not lead running from its opposite edge to the side the
   pointer is on. */
void easel_stretch_rect_mirror (EaselSelHandle handle, int x, int y, int w, int h, int dx, int dy, bool keep_ratio, int *nx, int *ny,
                                int *nw, int *nh, bool *mirror_x, bool *mirror_y);

/* The pixels (and shape) of a w x h selection scaled to W x H by the nearest pixel (REQ-0015-05, -06): the new pixel (i, j) is the
   lifted pixel (floor ((i + 1/2) w / W), floor ((j + 1/2) h / H)); with a mask it is in the shape when that pixel is. If that leaves
   no pixel in the shape, the one new pixel nearest the center of the new box among those whose footprint (the lifted pixels it
   covers) holds a shape pixel stays, with the color of the first such shape pixel in reading order. out_px (and out_mask, if mask is
   given) are new buffers of W x H items; mask NULL means a rectangle and out_mask may be NULL. NO_MEMORY, with both outputs NULL, if
   the bytes (4 per pixel and 1 per mask byte) exceed max_bytes or cannot be had or overflow. */
EaselSelectionResult easel_selection_scale (const uint32_t *px, const uint8_t *mask, int w, int h, int W, int H, uint32_t **out_px,
                                            uint8_t **out_mask, size_t max_bytes);

/* The smear (REQ-0014-11): moves the float from its place to (x, y), leaving a copy at every place on the 8-connected line between
   them (as the Pencil draws it), the place it leaves included and the place it moves to excluded, so that a drag leaves a trail
   without gaps however far each pointer event jumped. FALSE, with the float where it was and the copies of this call taken back
   (those it had stay), if the copies would need more than max_bytes or cannot grow. */
bool easel_selection_smear (EaselFloat *f, int x, int y, size_t max_bytes);

/* Lays the result over the rectangle of w x h picture pixels at (x0, y0), written to dst (stride: dst_stride pixels): the
   picture, the hole, each stamp, then, if with_float, the float at its place; a float or stamp pixel replaces what is under
   it in all four channels, whatever its color (REQ-0013-13). Only places of picture pixels are written. */
void easel_selection_lay (const EaselFloat *f, bool with_float, const EaselImage *img, int x0, int y0, int w, int h,
                          uint32_t *dst, size_t dst_stride);

/* The same in parts, for a preview kept between frames: with with_base, the picture and the hole; then the copies first to last - 1 in
   their order; then, with with_float, the float; each over what dst holds. Laying the base, then the copies in any split of 0 to n,
   then the float, gives what easel_selection_lay gives. */
void easel_selection_lay_range (const EaselFloat *f, size_t first, size_t last, bool with_float, const EaselImage *img, int x0, int y0,
                                int w, int h, uint32_t *dst, size_t dst_stride, bool with_base);

/* Writes the result into the picture as one undo step (REQ-0013-11): DONE, NOTHING if no pixel differs, or NO_MEMORY with
   the picture as before if the step's log would exceed max_bytes or cannot be had. The float is kept (the caller frees it). */
EaselSelectionResult easel_selection_put_down (const EaselFloat *f, bool with_float, EaselImage *img,
                                               EaselHistory *history, size_t max_bytes);

/* Clear Selection on a marked selection (REQ-0013-14): the rectangle (x, y, w, h), inside the picture, or with a mask only the
   pixels of the shape (REQ-0014-09), takes color as one undo step; NOTHING if no pixel changes, NO_MEMORY with the picture as
   before. A float is cleared by easel_selection_put_down without the float. */
EaselSelectionResult easel_selection_clear (EaselImage *img, EaselHistory *history, int x, int y, int w, int h,
                                            const uint8_t *mask, uint32_t color, size_t max_bytes);

/* Frees the float's memory and empties it. */
void easel_float_free (EaselFloat *f);
