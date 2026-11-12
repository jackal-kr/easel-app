/* SPDX-License-Identifier: GPL-3.0-or-later */
/* libFuzzer harness: the settings file's text (a file the user may edit by hand): read, checked, one key edited. */
#include <stdint.h>
#include <stdlib.h>
#include "settings.h"

int
LLVMFuzzerTestOneInput (const uint8_t *data, size_t size)
{
  EaselSettings s;
  char *out = NULL;
  size_t out_len = 0;

  easel_settings_parse ((const char *) data, size, &s);
  (void) easel_settings_parseable ((const char *) data, size);
  (void) easel_settings_toolbox_width ((const char *) data, size);
  if (easel_settings_edit ((const char *) data, size, "new-picture", "width", "640", &out, &out_len))
    free (out);
  if (easel_settings_edit ((const char *) data, size, "interface", "language", NULL, &out, &out_len))
    free (out);
  return 0;
}
