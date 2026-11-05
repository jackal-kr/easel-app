/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Minimal libc-only test helpers for core tests: a failed CHECK prints its location and the test exits 1. */
#pragma once

#include <stdio.h>
#include <stdlib.h>

#define CHECK(cond) \
  do { \
    if (!(cond)) \
      { \
        fprintf (stderr, "%s:%d: CHECK failed: %s\n", __FILE__, __LINE__, #cond); \
        exit (1); \
      } \
  } while (0)

#define RUN(test) \
  do { \
    test (); \
    printf ("ok %s\n", #test); \
  } while (0)
