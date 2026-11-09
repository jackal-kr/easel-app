/* SPDX-License-Identifier: GPL-3.0-or-later */
/* The subject finder over ONNX Runtime (feature 0035, T004), with the fake models of tests/data/finder: opening, the side, a run
   to the expected answer, failures as values, and a run ended from another thread. */

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "check.h"
#include "finder.h"

static char path[4096];

static const char *
data (const char *name)
{
  snprintf (path, sizeof path, "%s/finder/%s", getenv ("EASEL_TEST_DATA"), name);
  return path;
}

static double
now (void)
{
  struct timespec t;

  clock_gettime (CLOCK_MONOTONIC, &t);
  return t.tv_sec + t.tv_nsec / 1e9;
}

/* The fake answers with the red plane, as the model's normalization leaves it (x - 0.5): red 1 is 0.5, red 0 is -0.5. */
static void
fake_answer (void)
{
  char *error = NULL;
  EaselFinder *f = easel_finder_open (data ("fake-finder.onnx"), 2, &error);
  float *in, *out;
  int side;

  CHECK (f != NULL && error == NULL);
  side = easel_finder_side (f);
  CHECK (side == 64);
  in = calloc ((size_t) 3 * side * side, sizeof *in);
  out = calloc ((size_t) side * side, sizeof *out);
  CHECK (in != NULL && out != NULL);
  for (int y = 0; y < side; y++)
    for (int x = 0; x < side; x++)
      in[y * side + x] = x < side / 2 ? 1.0f : 0.0f;                    /* red on the left half */
  CHECK (easel_finder_run (f, in, out) == EASEL_FINDER_DONE);
  CHECK (out[0] == 0.5f && out[side - 1] == -0.5f && out[(side - 1) * side + 10] == 0.5f);
  memset (out, 0, (size_t) side * side * sizeof *out);
  CHECK (easel_finder_run (f, in, out) == EASEL_FINDER_DONE && out[0] == 0.5f);   /* twice, the same */
  free (out);
  free (in);
  easel_finder_close (f);
}

/* A missing and a broken model: NULL and a message, nothing else. */
static void
failures (void)
{
  char *error = NULL;
  char broken[] = "/tmp/easel-broken-finder-XXXXXX";
  int fd = mkstemp (broken);

  CHECK (easel_finder_open ("/nonexistent/easel-model.onnx", 1, &error) == NULL && error != NULL && error[0] != '\0');
  free (error);
  error = NULL;
  CHECK (fd >= 0 && write (fd, "not a model", 11) == 11);
  close (fd);
  CHECK (easel_finder_open (broken, 1, &error) == NULL && error != NULL);
  free (error);
  CHECK (easel_finder_open (broken, 1, NULL) == NULL);
  unlink (broken);
}

typedef struct
{
  EaselFinder *f;
  float *in, *out;
  EaselFinderResult r;
  double ended;
} Run;

static void *
run_thread (void *data)
{
  Run *run = data;

  run->r = easel_finder_run (run->f, run->in, run->out);
  run->ended = now ();
  return NULL;
}

/* A run of the slow fake ended from another thread: CANCELLED, within 0.5 s of the cancel; a run after it is cancelled too until
   the finder is rearmed. */
static void
cancel (void)
{
  EaselFinder *f = easel_finder_open (data ("slow-finder.onnx"), 2, NULL);
  Run run = { 0 };
  pthread_t t;
  double asked, full_start, full;
  int side;

  CHECK (f != NULL);
  side = easel_finder_side (f);
  run.f = f;
  run.in = calloc ((size_t) 3 * side * side, sizeof *run.in);
  run.out = calloc ((size_t) side * side, sizeof *run.out);
  CHECK (run.in != NULL && run.out != NULL);
  full_start = now ();
  CHECK (easel_finder_run (f, run.in, run.out) == EASEL_FINDER_DONE);
  full = now () - full_start;
  printf ("  the slow fake runs %.2f s\n", full);
  CHECK (full > 0.2);                                                   /* long enough to be cancelled in the middle */
  CHECK (pthread_create (&t, NULL, run_thread, &run) == 0);
  usleep ((useconds_t) (full * 0.3 * 1e6));
  asked = now ();
  easel_finder_cancel (f);
  pthread_join (t, NULL);
  printf ("  cancelled after %.3f s\n", run.ended - asked);
  CHECK (run.r == EASEL_FINDER_CANCELLED);
  CHECK (run.ended - asked < 0.5);
  CHECK (easel_finder_run (f, run.in, run.out) == EASEL_FINDER_CANCELLED);  /* still cancelled: a cancel is not lost */
  easel_finder_rearm (f);
  CHECK (easel_finder_run (f, run.in, run.out) == EASEL_FINDER_DONE);      /* rearmed, the next run runs */
  free (run.out);
  free (run.in);
  easel_finder_close (f);
}

int
main (void)
{
  RUN (fake_answer);
  RUN (failures);
  RUN (cancel);
  return 0;
}
