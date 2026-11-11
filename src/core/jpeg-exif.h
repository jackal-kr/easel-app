/* SPDX-License-Identifier: GPL-3.0-or-later */
/* A JPEG's EXIF blocks, read and left out by the app itself (REQ-0005-06, AS-20): the system's decoder aborted the whole app on some
   damaged blocks, so it never sees one. */
#pragma once

#include <stddef.h>
#include <stdint.h>

/* For the len bytes of data: 0 when they are not a JPEG (they do not start with FF D8), nothing allocated; 1 when they are, with *out
   (for free ()) the same bytes less every EXIF segment (FF E1, a length and "Exif") wherever it stands, *out_len their length, and
   *orientation the orientation tag (1 to 8) of the first whole one's first directory, or 1 when there is none or it cannot be read;
   -1 when the memory for *out cannot be had. A segment whose length runs beyond the end loses its marker, length and header only. */
int easel_jpeg_without_exif (const uint8_t *data, size_t len, uint8_t **out, size_t *out_len, int *orientation);
