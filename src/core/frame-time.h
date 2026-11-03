/* SPDX-License-Identifier: GPL-3.0-or-later */
/* What the status bar's Frame time part shows, and how often (REQ-0021-12). */
#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* The time as "6.2 ms": one decimal, the point as the locale writes it. */
void easel_frame_time_format (double ms, char *out, size_t size);
/* Whether a new value may show at now_us, the last one having shown at shown_us: at most once per 250 ms. */
bool easel_frame_time_due (int64_t now_us, int64_t shown_us);
/* How many milliseconds until it may, rounded up; 0 if it may now. */
int easel_frame_time_wait_ms (int64_t now_us, int64_t shown_us);
