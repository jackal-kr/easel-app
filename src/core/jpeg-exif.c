/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "jpeg-exif.h"

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

/* A 16- or 32-bit number of the TIFF block at offset at, in its byte order; the caller has checked that it lies inside. */
static unsigned
get16 (const uint8_t *t, size_t at, bool little)
{
  return little ? (unsigned) t[at] | (unsigned) t[at + 1] << 8 : (unsigned) t[at] << 8 | t[at + 1];
}

static uint32_t
get32 (const uint8_t *t, size_t at, bool little)
{
  return little ? (uint32_t) t[at] | (uint32_t) t[at + 1] << 8 | (uint32_t) t[at + 2] << 16 | (uint32_t) t[at + 3] << 24
                : (uint32_t) t[at] << 24 | (uint32_t) t[at + 1] << 16 | (uint32_t) t[at + 2] << 8 | t[at + 3];
}

/* The orientation tag (0x0112, a SHORT) of the first directory of the TIFF block t of n bytes: 1 to 8, or 1 when it cannot be read. */
static int
orientation_of (const uint8_t *t, size_t n)
{
  bool little;
  uint32_t ifd;
  unsigned count;

  if (n < 8 || !((t[0] == 'I' && t[1] == 'I') || (t[0] == 'M' && t[1] == 'M')))
    return 1;
  little = t[0] == 'I';
  if (get16 (t, 2, little) != 42)
    return 1;
  ifd = get32 (t, 4, little);
  if (ifd > n || n - ifd < 2)
    return 1;
  count = get16 (t, ifd, little);
  if ((size_t) count * 12 > n - ifd - 2)
    return 1;                                         /* a directory that runs beyond the block is damaged: none of it is read */
  for (unsigned k = 0; k < count; k++)
    {
      size_t e = (size_t) ifd + 2 + (size_t) k * 12;
      unsigned v;

      if (get16 (t, e, little) != 0x0112)
        continue;
      if (get16 (t, e + 2, little) != 3 || get32 (t, e + 4, little) != 1)
        return 1;                                     /* not one SHORT */
      v = get16 (t, e + 8, little);
      return v >= 1 && v <= 8 ? (int) v : 1;
    }
  return 1;
}

int
easel_jpeg_without_exif (const uint8_t *data, size_t len, uint8_t **out, size_t *out_len, int *orientation)
{
  uint8_t *o;
  size_t at = 0, n = 0;
  bool found = false;

  if (len < 2 || data[0] != 0xFF || data[1] != 0xD8)
    return 0;
  o = malloc (len);
  if (o == NULL)
    return -1;
  *orientation = 1;
  /* Every EXIF segment, wherever it is: the system's decoder found blocks behind fill bytes, behind junk and after the scan's start
     (the fuzzing's rounds of 2026-11-10 and -11). FF E1 cannot occur in coded data, where every FF is followed by 00 or a restart
     marker, so FF E1, a length and "Exif" are an EXIF segment wherever they stand. */
  while (at < len)
    {
      if (len - at >= 8 && data[at] == 0xFF && data[at + 1] == 0xE1 && memcmp (data + at + 4, "Exif", 4) == 0)
        {
          size_t seg = (size_t) data[at + 2] << 8 | data[at + 3];
          bool whole = seg >= 8 && seg <= len - at - 2;

          if (!found && whole && memcmp (data + at + 4, "Exif\0\0", 6) == 0)
            *orientation = orientation_of (data + at + 10, seg - 8);
          found = true;
          /* left out; with a length beyond the end, only its marker, length and header, so that no EXIF segment is left and the
             rest is passed over by the decoder as junk */
          at += whole ? seg + 2 : (len - at < 10 ? len - at : 10);
          continue;
        }
      o[n++] = data[at++];
    }
  *out = o;
  *out_len = n;
  return 1;
}
