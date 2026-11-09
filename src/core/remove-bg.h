/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Remove background (feature 0035): the pixel rules around the model that finds the subject (ADR-0006), which itself runs outside the
   core (src/ml). The input the model is given, the early answer of REQ-0035-05, the model's answer turned into removed pixels
   (REQ-0035-03, -07: alpha 0, the color kept, hard edges) and the result as one step or as a float's image (REQ-0035-02). */
#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "history.h"
#include "image.h"
#include "selection.h"
#include "transform.h"

/* The pixels looked at scaled to side x side (bilinear, pixel centers aligned) as three planes of red, green and blue from 0 to 1, in
   out (3 x side x side floats); a place not looked at (mask 0) or fully transparent is the neutral gray 0.5. mask is NULL or w x h
   bytes, 1 in the shape. */
void easel_rbg_input (const uint32_t *px, const uint8_t *mask, int w, int h, int side, float *out);

/* REQ-0035-05's early answer, without the model: true when the pixels looked at all have one color (all four channels), or when w
   or h is under 16. */
bool easel_rbg_trivial (const uint32_t *px, const uint8_t *mask, int w, int h);

/* The model's answer (side x side, any range) applied to the w x h pixels px in place: the answer stretched to 0 to 1 by its own
   lowest and highest values, scaled to w x h (bilinear, pixel centers aligned), and every pixel looked at (mask) whose value is below
   one half made fully transparent, its color kept; a pixel already fully transparent stays. Returns the pixels changed. */
size_t easel_rbg_apply (uint32_t *px, const uint8_t *mask, int w, int h, const float *found, int side);

/* The whole picture with the answer applied, as one undo step (REQ-0035-02, through easel_image_replace_pixels). NOTHING when no pixel
   changes; NO_MEMORY, with everything as it was, when an allocation above max_bytes or one that cannot be had is needed. */
EaselTransformResult easel_image_remove_background (EaselImage *img, EaselHistory *history, const float *found, int side,
                                                    size_t max_bytes);

/* The floating selection's shown image, with its shape, with the answer applied, becomes its image (as easel_selection_black_white
   does). NOTHING, the float untouched, when no pixel would change; NO_MEMORY as there. */
EaselSelectionResult easel_selection_remove_background (EaselFloat *f, const float *found, int side, size_t max_bytes);
