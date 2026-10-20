/* SPDX-License-Identifier: GPL-3.0-or-later */
/* The pixels an edit changes, in the order it changes them, with what restoring them takes: the log that becomes one undo
   step of the history (REQ-0002-01). The figures (0010, 0011) and the gradient (0012) draw through it. */
#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "history.h"
#include "image.h"

typedef struct
{
  EaselChange *log;
  size_t n, cap;
  size_t max_bytes;                 /* the most the log may use (a test hook; SIZE_MAX for none) */
} EaselChangeLog;

void easel_change_log_init (EaselChangeLog *l, size_t max_bytes);

/* Sets the pixel (x, y), inside the picture, to value through easel_image_set (so the screen update sees it), and logs it if
   it changes. False, with the pixel left as it was, if the log cannot grow (its memory, or max_bytes). */
bool easel_change_log_set (EaselChangeLog *l, EaselImage *img, int x, int y, uint32_t value);

/* Puts every logged pixel back, last first. */
void easel_change_log_restore (const EaselChangeLog *l, EaselImage *img);

/* The log as one step of the history. It grew by doubling, so it is first shrunk to its length: what the history holds is
   what its budget counts (REQ-0002-16). True if the history took it (the log is then the history's and l is empty); false
   if not, the log staying the caller's. */
bool easel_change_log_push (EaselChangeLog *l, EaselHistory *h);

void easel_change_log_free (EaselChangeLog *l);
