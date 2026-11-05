/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Feature 0018, the memory peak of each command on the whole picture (the Plan's table, T001): one command, then Undo and Redo, on a
   size x size picture, in a process of its own (the resident-set high-water mark is the process's, so one command per run).
   Usage: probe-transform-memory COMMAND [SIZE]   COMMAND: flip-h flip-v rot90 rot180 rot270 invert clear (SIZE defaults to 8000).
   With "sel-" before the command (sel-flip-h, sel-rot90, sel-invert, ...) the command is made on a floating selection of the whole
   picture instead (T004), twice in a row; "sel-stretched-" first makes the float one pixel narrower than it was lifted, so that its
   lifted image and its shown image differ. The peak is read from the resident size before the command (the counter is reset). */

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/resource.h>
#include <time.h>
#include <unistd.h>

#include "history.h"
#include "selection.h"
#include "transform.h"

static bool
g_str_has_prefix_ (const char *s, const char *prefix)
{
  return strncmp (s, prefix, strlen (prefix)) == 0;
}

static double
now_ms (void)
{
  struct timespec t;

  clock_gettime (CLOCK_MONOTONIC, &t);
  return (double) t.tv_sec * 1000.0 + (double) t.tv_nsec / 1e6;
}

/* The resident set now, in MiB. */
static double
rss_mib (void)
{
  long pages = 0, resident = 0;
  FILE *f = fopen ("/proc/self/statm", "r");

  if (f != NULL)
    {
      if (fscanf (f, "%ld %ld", &pages, &resident) != 2)
        resident = 0;
      fclose (f);
    }
  return (double) resident * (double) sysconf (_SC_PAGESIZE) / 1048576.0;
}

/* The process's peak resident set so far, in MiB. */
static double
peak_mib (void)
{
  struct rusage u;

  getrusage (RUSAGE_SELF, &u);
  return (double) u.ru_maxrss / 1024.0;
}

/* The peak since the counter was reset (VmHWM, in MiB). */
static void
reset_peak (void)
{
  FILE *f = fopen ("/proc/self/clear_refs", "w");

  if (f != NULL)
    {
      fputs ("5", f);
      fclose (f);
    }
}

/* A command on a floating selection of the whole picture, twice (the second finds the float already made), core only. */
static int
selection_run (const char *name, EaselOp op, int size, bool stretched)
{
  EaselImage *img = easel_image_new (size, size, 0);
  EaselFloat f = { 0 };
  double before, t0, t1, t2;

  if (img == NULL)
    return 1;
  for (size_t i = 0; i < (size_t) size * (size_t) size; i++)
    img->px[i] = 0xFF000000u | (uint32_t) (i * 2654435761u);
  if (easel_selection_lift (&f, img, 0, 0, size, size, NULL, false, 0xFFFFFFFFu, (size_t) 4 << 30) != EASEL_SELECTION_DONE)
    return 1;
  if (stretched && easel_selection_stretch (&f, 0, 0, size - 1, size, (size_t) 4 << 30) != EASEL_SELECTION_DONE)
    return 1;
  reset_peak ();
  before = rss_mib ();
  printf ("%s%s %d x %d: picture and float resident %.0f MiB\n", stretched ? "sel-stretched-" : "sel-", name, size, size, before);
  t0 = now_ms ();
  if (easel_selection_transform (&f, op, (size_t) 4 << 30) != EASEL_SELECTION_DONE)
    return 1;
  t1 = now_ms ();
  printf ("  first: %.0f ms, peak %.0f MiB (%.0f MiB above)\n", t1 - t0, peak_mib (), peak_mib () - before);
  reset_peak ();
  before = rss_mib ();
  t1 = now_ms ();
  if (easel_selection_transform (&f, op, (size_t) 4 << 30) != EASEL_SELECTION_DONE)
    return 1;
  t2 = now_ms ();
  printf ("  second: %.0f ms, peak %.0f MiB (%.0f MiB above the %.0f resident)\n", t2 - t1, peak_mib (), peak_mib () - before, before);
  (void) t0;
  easel_float_free (&f);
  easel_image_free (img);
  return 0;
}

int
main (int argc, char **argv)
{
  static const struct { const char *name; EaselOp op; } ops[] = {
    { "flip-h", EASEL_OP_FLIP_H }, { "flip-v", EASEL_OP_FLIP_V }, { "rot90", EASEL_OP_ROTATE_90 },
    { "rot180", EASEL_OP_ROTATE_180 }, { "rot270", EASEL_OP_ROTATE_270 }, { "invert", EASEL_OP_INVERT }, { "clear", EASEL_OP_CLEAR },
  };
  int size = argc > 2 ? atoi (argv[2]) : 8000, op = -1;
  bool selection = argc > 1 && g_str_has_prefix_ (argv[1], "sel-");
  bool stretched = argc > 1 && g_str_has_prefix_ (argv[1], "sel-stretched-");
  const char *cmd = argc > 1 ? argv[1] + (stretched ? 14 : selection ? 4 : 0) : "";
  EaselImage *img;
  EaselHistory *h;
  double before, t0, t1, t2, t3;
  EaselTransformResult r;

  for (size_t i = 0; argc > 1 && i < sizeof ops / sizeof *ops; i++)
    if (strcmp (cmd, ops[i].name) == 0)
      op = (int) i;
  if (op < 0 || size < 1 || size > EASEL_IMAGE_MAX_SIZE)
    {
      fprintf (stderr, "usage: %s flip-h|flip-v|rot90|rot180|rot270|invert|clear [SIZE]\n", argv[0]);
      return 2;
    }
  if (selection)
    return selection_run (cmd, ops[op].op, size, stretched);
  img = easel_image_new (size, size, 0);
  h = easel_history_new ((size_t) 4 << 30);
  if (img == NULL || h == NULL)
    return 1;
  for (size_t i = 0; i < (size_t) size * (size_t) size; i++)
    img->px[i] = 0xFF000000u | (uint32_t) (i * 2654435761u);      /* every pixel different: the worst case of a change count */
  before = rss_mib ();
  printf ("%s %d x %d: picture resident %.0f MiB, process peak so far %.0f MiB\n", argv[1], size, size, before, peak_mib ());
  t0 = now_ms ();
  r = easel_image_transform (img, h, ops[op].op, 0xFF00FF00u, (size_t) 4 << 30);
  t1 = now_ms ();
  printf ("  command: result %d, %.0f ms, step %.0f MiB, peak %.0f MiB (%.0f MiB above the picture)\n", (int) r, t1 - t0,
          (double) easel_history_bytes (h) / 1048576.0, peak_mib (), peak_mib () - before);
  easel_history_undo (h, img);
  t2 = now_ms ();
  easel_history_redo (h, img);
  t3 = now_ms ();
  printf ("  undo %.0f ms, redo %.0f ms, peak after both %.0f MiB\n", t2 - t1, t3 - t2, peak_mib ());
  easel_history_free (h);
  easel_image_free (img);
  return 0;
}
