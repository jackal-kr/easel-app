/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Save-name scenarios of feature 0001. */

#include <stdlib.h>
#include <string.h>

#include "check.h"
#include "savename.h"

static void
expect (const char *path, const char *want)
{
  char *fixed = NULL;

  CHECK (easel_savename_fix (path, &fixed) == EASEL_SAVENAME_OK);
  CHECK (fixed != NULL);
  if (strcmp (fixed, want) != 0)
    {
      fprintf (stderr, "%s -> %s, want %s\n", path, fixed, want);
      exit (1);
    }
  free (fixed);
}

static void
as25_save_name_rule (void)
{
  char *fixed = (char *) "untouched";

  expect ("/home/u/Pictures/a.png", "/home/u/Pictures/a.png");
  expect ("/home/u/Pictures/b", "/home/u/Pictures/b.png");
  expect ("/home/u/Pictures/E.PNG", "/home/u/Pictures/E.PNG");
  expect ("/home/u/Pictures/x.PnG", "/home/u/Pictures/x.PnG");
  expect ("/home/u/Pictures/d.", "/home/u/Pictures/d.png");
  expect ("/home/u/Pictures/.png", "/home/u/Pictures/.png.png");
  expect ("/home/u/my.dir/b", "/home/u/my.dir/b.png");
  expect ("b", "b.png");

  CHECK (easel_savename_fix ("/home/u/Pictures/c.jpg", &fixed) == EASEL_SAVENAME_NOT_PNG);
  CHECK (fixed == NULL);
  CHECK (easel_savename_fix ("/home/u/Pictures/archive.png.bak", &fixed) == EASEL_SAVENAME_NOT_PNG);
}

int
main (void)
{
  RUN (as25_save_name_rule);
  return 0;
}
