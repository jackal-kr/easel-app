/* SPDX-License-Identifier: GPL-3.0-or-later */
/* File names for Save: 0001's PNG rule (AS-25) and 0005's formats (AS-9). */

#include <stdlib.h>
#include <string.h>

#include "check.h"
#include "format.h"

static void
expect (const char *path, const char *want, EaselFormat format)
{
  char *fixed = NULL;
  EaselFormat f = EASEL_FORMAT_UNKNOWN;

  CHECK (easel_format_fix_name (path, &fixed, &f) == EASEL_FORMAT_OK);
  CHECK (fixed != NULL);
  if (strcmp (fixed, want) != 0 || f != format)
    {
      fprintf (stderr, "%s -> %s (%d), want %s (%d)\n", path, fixed, (int) f, want, (int) format);
      exit (1);
    }
  free (fixed);
}

static void
expect_unknown (const char *path)
{
  char *fixed = (char *) "untouched";
  EaselFormat f = EASEL_FORMAT_PNG;

  if (easel_format_fix_name (path, &fixed, &f) != EASEL_FORMAT_UNKNOWN_EXTENSION || fixed != NULL
      || f != EASEL_FORMAT_UNKNOWN)
    {
      fprintf (stderr, "%s should be an unknown extension\n", path);
      exit (1);
    }
}

/* 0001 AS-25: a name without an extension gets .png; the dot rules; .PNG stays as typed. */
static void
as25_save_name_rule (void)
{
  expect ("/home/u/Pictures/a.png", "/home/u/Pictures/a.png", EASEL_FORMAT_PNG);
  expect ("/home/u/Pictures/b", "/home/u/Pictures/b.png", EASEL_FORMAT_PNG);
  expect ("/home/u/Pictures/E.PNG", "/home/u/Pictures/E.PNG", EASEL_FORMAT_PNG);
  expect ("/home/u/Pictures/x.PnG", "/home/u/Pictures/x.PnG", EASEL_FORMAT_PNG);
  expect ("/home/u/Pictures/d.", "/home/u/Pictures/d.png", EASEL_FORMAT_PNG);
  expect ("/home/u/Pictures/.png", "/home/u/Pictures/.png.png", EASEL_FORMAT_PNG);
  expect ("/home/u/my.dir/b", "/home/u/my.dir/b.png", EASEL_FORMAT_PNG);
  expect ("b", "b.png", EASEL_FORMAT_PNG);
  expect_unknown ("/home/u/Pictures/archive.png.bak");
}

/* 0005 AS-9: the extension, in any case, decides the format; nothing else is saveable. */
static void
as9_format_by_name (void)
{
  expect ("/p/a.PNG", "/p/a.PNG", EASEL_FORMAT_PNG);
  expect ("/p/a.jpg", "/p/a.jpg", EASEL_FORMAT_JPEG);
  expect ("/p/a.jpeg", "/p/a.jpeg", EASEL_FORMAT_JPEG);
  expect ("/p/a.JpEg", "/p/a.JpEg", EASEL_FORMAT_JPEG);
  expect ("/p/a.bmp", "/p/a.bmp", EASEL_FORMAT_BMP);
  expect ("/p/a.tif", "/p/a.tif", EASEL_FORMAT_TIFF);
  expect ("/p/a.tiff", "/p/a.tiff", EASEL_FORMAT_TIFF);
  expect ("/p/a.WEBP", "/p/a.WEBP", EASEL_FORMAT_WEBP);
  expect ("/p/a", "/p/a.png", EASEL_FORMAT_PNG);
  expect ("/p/a.", "/p/a.png", EASEL_FORMAT_PNG);
  expect (".png", ".png.png", EASEL_FORMAT_PNG);
  expect ("/p/.hidden", "/p/.hidden.png", EASEL_FORMAT_PNG);          /* a leading dot is no extension */
  expect ("/p/my.dir/a.jpg", "/p/my.dir/a.jpg", EASEL_FORMAT_JPEG);

  expect_unknown ("/p/a.gif");                                         /* GIF can be opened, not saved */
  expect_unknown ("/p/a.xyz");
  expect_unknown ("/p/report 2024.05.10");                             /* the text after the last dot is the extension */
  expect_unknown ("/p/a.jpg.bak");
  expect_unknown ("/p/a.jpgx");
  expect_unknown ("/p/a.tifff");
  expect_unknown ("/p/a.web");
}

/* Which formats lose transparency (REQ-0005-11). */
static void
as10_flat_formats (void)
{
  CHECK (easel_format_is_flat (EASEL_FORMAT_JPEG));
  CHECK (easel_format_is_flat (EASEL_FORMAT_BMP));
  CHECK (!easel_format_is_flat (EASEL_FORMAT_PNG));
  CHECK (!easel_format_is_flat (EASEL_FORMAT_TIFF));
  CHECK (!easel_format_is_flat (EASEL_FORMAT_WEBP));
}

int
main (void)
{
  RUN (as25_save_name_rule);
  RUN (as9_format_by_name);
  RUN (as10_flat_formats);
  return 0;
}
