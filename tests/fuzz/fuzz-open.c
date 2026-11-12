/* SPDX-License-Identifier: GPL-3.0-or-later */
/* libFuzzer harness: a picture opened from bytes (the clipboard's and the file's path, the EXIF guard included), then encoded as PNG. */
#include <stdint.h>
#include <gio/gio.h>
#include "picture-file.h"
#include "image.h"

int
LLVMFuzzerTestOneInput (const uint8_t *data, size_t size)
{
  g_autoptr (GBytes) b = g_bytes_new (data, size);
  EaselImage *img = NULL;
  int w = 0, h = 0;

  if (easel_picture_open_bytes (b, &img, &w, &h) == EASEL_OPEN_OK && img != NULL)
    {
      g_autoptr (GBytes) png = easel_picture_encode_fast_png (img, NULL);

      easel_image_free (img);
    }
  return 0;
}
