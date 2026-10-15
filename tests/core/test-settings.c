/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Settings scenarios of feature 0001. */

#include <string.h>

#include "check.h"
#include "image.h"
#include "settings.h"

static EaselSettings
parse (const char *text)
{
  EaselSettings s;

  easel_settings_parse (text, strlen (text), &s);
  return s;
}

static void
check_defaults (const EaselSettings *s)
{
  CHECK (s->width == EASEL_SIZE_AUTO);
  CHECK (s->height == EASEL_SIZE_AUTO);
  CHECK (s->background == 0xFFFFFFFFu);
  CHECK (s->undo_budget_mb == 512);
}

static void
as2_auto_size_fits_area (void)
{
  EaselSettings s = parse ("[new-picture]\nwidth=auto\nheight=auto\n");
  int w, h;

  easel_settings_resolve_size (&s, 1390, 853, &w, &h);
  CHECK (w == 1024 && h == 768);
  easel_settings_resolve_size (&s, 1000, 700, &w, &h);
  CHECK (w == 800 && h == 600);
  easel_settings_resolve_size (&s, 600, 400, &w, &h);
  CHECK (w == 640 && h == 480);

  /* Strictly smaller: an area of exactly 1024x768 gets 800x600. */
  easel_settings_resolve_size (&s, 1024, 768, &w, &h);
  CHECK (w == 800 && h == 600);
  /* Both directions must fit: wide but short. */
  easel_settings_resolve_size (&s, 3000, 700, &w, &h);
  CHECK (w == 800 && h == 600);

  s = parse ("[new-picture]\nwidth=300\nheight=200\n");
  easel_settings_resolve_size (&s, 1000, 700, &w, &h);
  CHECK (w == 300 && h == 200);

  /* One auto makes both auto. */
  s = parse ("[new-picture]\nwidth=300\nheight=auto\n");
  easel_settings_resolve_size (&s, 1000, 700, &w, &h);
  CHECK (w == 800 && h == 600);
}

static void
as3_transparent_background (void)
{
  EaselSettings s = parse ("[new-picture]\nwidth=4\nheight=3\nbackground=transparent\n");
  EaselImage *img;

  CHECK (s.background == 0x00000000u);
  img = easel_image_new (s.width, s.height, s.background);
  CHECK (img != NULL);
  CHECK (img->width == 4 && img->height == 3);
  for (int y = 0; y < 3; y++)
    for (int x = 0; x < 4; x++)
      CHECK (easel_image_get (img, x, y) == 0x00000000u);
  easel_image_free (img);
}

static void
as5_invalid_value_falls_back (void)
{
  EaselSettings s = parse ("[new-picture]\nwidth=abc\nheight=200\nbackground=#00FF00\n");

  CHECK (s.width == EASEL_SIZE_AUTO);
  CHECK (s.height == 200);
  CHECK (s.background == 0xFF00FF00u);
  {
    int w, h;

    easel_settings_resolve_size (&s, 1000, 700, &w, &h);
    CHECK (w == 800 && h == 600);
  }

  s = parse ("[new-picture]\nbackground=#11223344\n");
  CHECK (s.background == 0x44112233u);

  s = parse ("[new-picture]\nbackground=#12345\n");
  CHECK (s.background == 0xFFFFFFFFu);
  s = parse ("[new-picture]\nbackground=#GG0000\n");
  CHECK (s.background == 0xFFFFFFFFu);
  s = parse ("[new-picture]\nbackground=red\n");
  CHECK (s.background == 0xFFFFFFFFu);
}

static void
as6_garbage_is_all_defaults (void)
{
  static const char garbage[] = "\x89PNG\r\n\x1a\n\0\0\0\rIHDR\xff\xfe";
  EaselSettings s;

  easel_settings_parse (garbage, sizeof garbage - 1, &s);
  check_defaults (&s);
}

static void
as4_default_text_parses_to_defaults (void)
{
  EaselSettings s = parse (easel_settings_default_text);

  check_defaults (&s);
}

static void
as7_size_limits (void)
{
  CHECK (parse ("[new-picture]\nwidth=0\n").width == EASEL_SIZE_AUTO);
  CHECK (parse ("[new-picture]\nwidth=-5\n").width == EASEL_SIZE_AUTO);
  CHECK (parse ("[new-picture]\nwidth=10001\n").width == EASEL_SIZE_AUTO);
  CHECK (parse ("[new-picture]\nwidth=99999999999999999999\n").width == EASEL_SIZE_AUTO);
  CHECK (parse ("[new-picture]\nwidth=1\n").width == 1);
  CHECK (parse ("[new-picture]\nwidth=10000\n").width == 10000);
}

static void
as18_budget_setting (void)
{
  CHECK (parse ("[history]\nmemory=64\n").undo_budget_mb == 64);
  CHECK (parse ("[history]\nmemory=16\n").undo_budget_mb == 16);
  CHECK (parse ("[history]\nmemory=8192\n").undo_budget_mb == 8192);

  /* Missing, invalid or out of range: 512, silently. */
  CHECK (parse ("").undo_budget_mb == 512);
  CHECK (parse ("[history]\n").undo_budget_mb == 512);
  CHECK (parse ("[history]\nmemory=abc\n").undo_budget_mb == 512);
  CHECK (parse ("[history]\nmemory=8\n").undo_budget_mb == 512);
  CHECK (parse ("[history]\nmemory=15\n").undo_budget_mb == 512);
  CHECK (parse ("[history]\nmemory=8193\n").undo_budget_mb == 512);
  CHECK (parse ("[history]\nmemory=99999\n").undo_budget_mb == 512);
  CHECK (parse ("[history]\nmemory=99999999999999999999\n").undo_budget_mb == 512);
  CHECK (parse ("[history]\nmemory=-64\n").undo_budget_mb == 512);
  CHECK (parse ("[history]\nmemory=\n").undo_budget_mb == 512);
  CHECK (parse ("[history]\nmemory=auto\n").undo_budget_mb == 512);

  /* Same syntax as the other settings (REQ-0001-40): spaces, repeats (last wins), other sections ignored. */
  CHECK (parse ("[history]\n memory = 0128 \n").undo_budget_mb == 128);
  CHECK (parse ("[history]\nmemory=64\nmemory=256\n").undo_budget_mb == 256);
  CHECK (parse ("[history]\nmemory=64\nmemory=bad\n").undo_budget_mb == 512);
  CHECK (parse ("[new-picture]\nmemory=64\n").undo_budget_mb == 512);
  CHECK (parse ("memory=64\n").undo_budget_mb == 512);

  /* The sections do not leak into each other. */
  {
    EaselSettings s = parse ("[history]\nmemory=64\nwidth=300\n[new-picture]\nheight=200\nmemory=32\n");

    CHECK (s.undo_budget_mb == 64 && s.width == EASEL_SIZE_AUTO && s.height == 200);
  }
  /* A file that cannot be parsed gives every default, the budget included. */
  CHECK (parse ("[history]\nmemory=64\nhello\n").undo_budget_mb == 512);
}

static void
as36_syntax_rules (void)
{
  EaselSettings s = parse ("# note\n"
                           "\n"
                           "[new-picture]\n"
                           "width = 05 \n"
                           "Width=9\n"
                           "background=TRANSPARENT\n"
                           "height=7\n"
                           "height=8\n"
                           "[other]\n"
                           "width=3\n");

  CHECK (s.width == 5);
  CHECK (s.height == 8);
  CHECK (s.background == 0x00000000u);

  CHECK (parse ("[new-picture]\nwidth=+5\n").width == EASEL_SIZE_AUTO);
  CHECK (parse ("[new-picture]\nwidth=5.0\n").width == EASEL_SIZE_AUTO);
  CHECK (parse ("[new-picture]\nwidth=AUTO\n").width == EASEL_SIZE_AUTO);
  CHECK (parse ("[new-picture]\nbackground=#ff00ff\n").background == 0xFFFF00FFu);
  CHECK (parse ("[new-picture]\r\nwidth=12\r\n").width == 12);
  CHECK (parse ("[new-picture]\nheight=20").height == 20);

  /* A key outside any section is not in [new-picture]. */
  CHECK (parse ("width=12\n").width == EASEL_SIZE_AUTO);

  s = parse ("[new-picture]\nwidth=300\nhello\n");
  check_defaults (&s);
  s = parse ("[new-picture\nwidth=300\n");
  check_defaults (&s);
}

int
main (void)
{
  RUN (as2_auto_size_fits_area);
  RUN (as3_transparent_background);
  RUN (as4_default_text_parses_to_defaults);
  RUN (as5_invalid_value_falls_back);
  RUN (as18_budget_setting);
  RUN (as6_garbage_is_all_defaults);
  RUN (as7_size_limits);
  RUN (as36_syntax_rules);
  return 0;
}
