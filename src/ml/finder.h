/* SPDX-License-Identifier: GPL-3.0-or-later */
/* The subject finder of Remove background (feature 0035, ADR-0006): the model run on the machine through ONNX Runtime. Given the
   pixels looked at as three planes from 0 to 1 (easel_rbg_input), it returns the model's answer for each place of its square;
   the pixel rules around it are the core's (remove-bg.h). No display, no GLib: only the ui links it, from a worker thread. */
#pragma once

typedef struct EaselFinder EaselFinder;

typedef enum
{
  EASEL_FINDER_DONE,        /* out holds the answer */
  EASEL_FINDER_CANCELLED,   /* easel_finder_cancel ended the run; out is not to be used */
  EASEL_FINDER_FAILED       /* the model could not run (REQ-0035-06's "not available") */
} EaselFinderResult;

/* Loads the model at path, run with the given number of threads (fixed, so that the result does not depend on the machine's load,
   ADR-0006). NULL if it cannot be loaded, with *error (if error is not NULL) a message the caller frees with free (). */
EaselFinder *easel_finder_open (const char *path, int threads, char **error);

/* The side of the model's square: its input is 3 x side x side, its answer side x side. */
int easel_finder_side (const EaselFinder *f);

/* Runs the model on in (3 x side x side, red, green and blue planes from 0 to 1) into out (side x side). One run at a time. */
EaselFinderResult easel_finder_run (EaselFinder *f, const float *in, float *out);

/* Ends a run in progress as soon as the runtime can, or the next one at once (from any thread), until easel_finder_rearm. A caller
   rearms, then checks its own request to cancel, then runs: whatever the order of the two threads, a cancel is not lost. */
void easel_finder_cancel (EaselFinder *f);

/* Clears a cancel, so that the next run runs. */
void easel_finder_rearm (EaselFinder *f);

void easel_finder_close (EaselFinder *f);
