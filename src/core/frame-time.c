/* SPDX-License-Identifier: GPL-3.0-or-later */

#include "frame-time.h"

#include <stdio.h>

#define PACE_US 250000

void
easel_frame_time_format (double ms, char *out, size_t size)
{
  snprintf (out, size, "%.1f ms", ms);
}

bool
easel_frame_time_due (int64_t now_us, int64_t shown_us)
{
  return now_us - shown_us >= PACE_US;
}

int
easel_frame_time_wait_ms (int64_t now_us, int64_t shown_us)
{
  int64_t left = PACE_US - (now_us - shown_us);

  return left <= 0 ? 0 : (int) ((left + 999) / 1000);
}
