/* SPDX-License-Identifier: GPL-3.0-or-later */
/* REQ-0005-06, AS-20: a JPEG's EXIF blocks read and left out by the app (2026-11-10, at the beta gate). */

#include <string.h>
#include "check.h"
#include "jpeg-exif.h"

/* An APP1 EXIF segment with one directory holding the orientation tag, big-endian (MM) or little-endian (II). */
static size_t
exif_segment (uint8_t *p, int tag, int little)
{
  uint8_t tiff[26];
  size_t n = 0;

  memcpy (tiff, little ? "II\x2a\x00\x08\x00\x00\x00" : "MM\x00\x2a\x00\x00\x00\x08", 8);
  if (little)
    memcpy (tiff + 8, (uint8_t[]) { 1, 0, 0x12, 0x01, 3, 0, 1, 0, 0, 0, (uint8_t) tag, 0, 0, 0, 0, 0, 0, 0 }, 18);
  else
    memcpy (tiff + 8, (uint8_t[]) { 0, 1, 0x01, 0x12, 0, 3, 0, 0, 0, 1, 0, (uint8_t) tag, 0, 0, 0, 0, 0, 0 }, 18);
  p[n++] = 0xFF, p[n++] = 0xE1;
  p[n++] = 0, p[n++] = (uint8_t) (2 + 6 + sizeof tiff);
  memcpy (p + n, "Exif\0\0", 6), n += 6;
  memcpy (p + n, tiff, sizeof tiff), n += sizeof tiff;
  return n;
}

static const uint8_t APP0[] = { 0xFF, 0xE0, 0, 7, 'J', 'F', 'I', 'F', 0 };
static const uint8_t SCAN[] = { 0xFF, 0xDA, 0, 4, 1, 2, 0xAB, 0xFF, 0xE1, 0xCD, 0xFF, 0xD9 };   /* an FF E1 inside the scan stays */

static size_t
jpeg (uint8_t *p, int tag1, int tag2, int little)
{
  size_t n = 0;

  p[n++] = 0xFF, p[n++] = 0xD8;
  memcpy (p + n, APP0, sizeof APP0), n += sizeof APP0;
  if (tag1 >= 0)
    n += exif_segment (p + n, tag1, little);
  if (tag2 >= 0)
    n += exif_segment (p + n, tag2, little);
  memcpy (p + n, SCAN, sizeof SCAN), n += sizeof SCAN;
  return n;
}

/* The bytes less the EXIF segments: SOI, APP0, the scan, byte for byte. */
static void
check_plain (const uint8_t *o, size_t on)
{
  CHECK (on == 2 + sizeof APP0 + sizeof SCAN);
  CHECK (o[0] == 0xFF && o[1] == 0xD8);
  CHECK (memcmp (o + 2, APP0, sizeof APP0) == 0);
  CHECK (memcmp (o + 2 + sizeof APP0, SCAN, sizeof SCAN) == 0);
}

static void
reads_and_leaves_out (void)
{
  uint8_t p[256], *o = NULL;
  size_t n, on = 0;
  int orient = 0;

  for (int little = 0; little < 2; little++)
    for (int tag = 1; tag <= 8; tag++)
      {
        n = jpeg (p, tag, -1, little);
        CHECK (easel_jpeg_without_exif (p, n, &o, &on, &orient) == 1);
        CHECK (orient == tag);
        check_plain (o, on);
        free (o);
      }
  /* two blocks: the first decides */
  n = jpeg (p, 6, 3, 0);
  CHECK (easel_jpeg_without_exif (p, n, &o, &on, &orient) == 1 && orient == 6);
  check_plain (o, on);
  free (o);
  /* no block: the bytes as they are, orientation 1 */
  n = jpeg (p, -1, -1, 0);
  CHECK (easel_jpeg_without_exif (p, n, &o, &on, &orient) == 1 && orient == 1 && on == n && memcmp (o, p, n) == 0);
  free (o);
}

static void
damaged_blocks (void)
{
  uint8_t p[256], *o = NULL;
  size_t n, on = 0;
  int orient = 0;

  /* a tag outside 1 to 8 */
  n = jpeg (p, 9, -1, 0);
  CHECK (easel_jpeg_without_exif (p, n, &o, &on, &orient) == 1 && orient == 1);
  check_plain (o, on);
  free (o);
  /* the directory's offset beyond the block, and a count of entries beyond it: still left out, orientation 1 */
  for (int k = 0; k < 2; k++)
    {
      n = jpeg (p, 6, -1, 0);
      if (k == 0)
        p[2 + sizeof APP0 + 10 + 7] = 0xF0;          /* IFD0 offset 0xF0 */
      else
        p[2 + sizeof APP0 + 10 + 9] = 0xFF;          /* 255 entries */
      CHECK (easel_jpeg_without_exif (p, n, &o, &on, &orient) == 1 && orient == 1);
      check_plain (o, on);
      free (o);
    }
  /* a segment length beyond the end: its marker, length and header left out, the rest kept */
  n = jpeg (p, 6, -1, 0);
  p[2 + sizeof APP0 + 2] = 0x7F;
  CHECK (easel_jpeg_without_exif (p, n, &o, &on, &orient) == 1 && orient == 1 && on == n - 10);
  CHECK (memcmp (o, p, 2 + sizeof APP0) == 0 && memcmp (o + 2 + sizeof APP0, p + 2 + sizeof APP0 + 10, n - 2 - sizeof APP0 - 10) == 0);
  free (o);
  /* not a JPEG, or too short */
  CHECK (easel_jpeg_without_exif ((const uint8_t *) "\x89PNG", 4, &o, &on, &orient) == 0);
  CHECK (easel_jpeg_without_exif (p, 1, &o, &on, &orient) == 0);
  /* every cut of a JPEG with a block: no read beyond it (the sanitizers' build sees one) */
  n = jpeg (p, 6, 3, 1);
  for (size_t cut = 0; cut <= n; cut++)
    {
      uint8_t *q = malloc (cut ? cut : 1);

      memcpy (q, p, cut);
      if (easel_jpeg_without_exif (q, cut, &o, &on, &orient) == 1)
        {
          CHECK (on <= cut && orient >= 1 && orient <= 8);
          free (o);
        }
      free (q);
    }
}

/* Fill bytes (FF FF ...) before a marker, and junk between two segments: the decoder skips both and finds the next marker, so the
   reader does too, and leaves out an EXIF block after them (the fuzzing's second round, 2026-11-11). */
static void
fill_and_junk (void)
{
  uint8_t p[256], q[300], *o = NULL;
  size_t n, m, on = 0;
  int orient = 0;

  for (int kind = 0; kind < 2; kind++)
    {
      n = jpeg (p, 6, -1, 0);
      m = 0;
      memcpy (q, p, 2 + sizeof APP0), m = 2 + sizeof APP0;               /* SOI, APP0 */
      if (kind == 0)
        q[m++] = 0xFF, q[m++] = 0xFF;                                        /* fill bytes before the EXIF segment's FF E1 */
      else
        q[m++] = 0xFA, q[m++] = 0x12;                                        /* junk */
      memcpy (q + m, p + 2 + sizeof APP0, n - 2 - sizeof APP0), m += n - 2 - sizeof APP0;
      CHECK (easel_jpeg_without_exif (q, m, &o, &on, &orient) == 1);
      CHECK (orient == 6);
      for (size_t k = 0; k + 4 <= on; k++)
        CHECK (memcmp (o + k, "Exif", 4) != 0);                              /* no EXIF block left */
      CHECK (memcmp (o + on - sizeof SCAN, SCAN, sizeof SCAN) == 0);
      free (o);
    }
}

/* An EXIF segment after the scan's start (inside or after the coded data, or after the end marker): the system's decoder still finds it
   (the fuzzing's third round, 2026-11-11), so it is left out wherever it is; FF E1 cannot occur in coded data, where every FF is
   followed by 00 or a restart marker. A length beyond the end loses only its marker, length and header. */
static void
after_the_scan (void)
{
  uint8_t p[256], q[400], *o = NULL;
  size_t n, m, on = 0;
  int orient = 0;

  for (int where = 0; where < 2; where++)
    {
      n = jpeg (p, -1, -1, 0);                                               /* SOI, APP0, the scan with its EOI */
      m = 0;
      if (where == 0)
        {
          memcpy (q, p, n - 2), m = n - 2;                                   /* before the EOI, as if inside the coded data */
          m += exif_segment (q + m, 3, 0);
          q[m++] = 0xFF, q[m++] = 0xD9;
        }
      else
        {
          memcpy (q, p, n), m = n;                                           /* after the EOI */
          m += exif_segment (q + m, 3, 1);
        }
      CHECK (easel_jpeg_without_exif (q, m, &o, &on, &orient) == 1);
      CHECK (orient == 3);
      for (size_t k = 0; k + 4 <= on; k++)
        CHECK (memcmp (o + k, "Exif", 4) != 0);
      CHECK (on == n);                                                       /* the rest byte for byte */
      CHECK (memcmp (o, p, n) == 0);
      free (o);
    }
  /* an EXIF segment whose length runs beyond the end, after the scan: its marker, length and header left out, no "Exif" left */
  n = jpeg (p, -1, -1, 0);
  memcpy (q, p, n), m = n;
  m += exif_segment (q + m, 3, 0);
  q[n + 2] = 0x7F;
  CHECK (easel_jpeg_without_exif (q, m, &o, &on, &orient) == 1 && orient == 1 && on == m - 10 && memcmp (o, p, n) == 0);
  for (size_t k = 0; k + 4 <= on; k++)
    CHECK (memcmp (o + k, "Exif", 4) != 0);
  free (o);
}

int
main (void)
{
  RUN (reads_and_leaves_out);
  RUN (damaged_blocks);
  RUN (fill_and_junk);
  RUN (after_the_scan);
  return 0;
}
