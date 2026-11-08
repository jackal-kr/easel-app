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

/* AS-39, AS-4 (REQ-0001-03 as renamed): `canvas-color` is the picture's color; the older `background` is still read; a
   valid `canvas-color` wins over `background` in either order, and an invalid one leaves `background` to count. */
static void
as39_canvas_color_and_background (void)
{
  CHECK (parse ("[new-picture]\ncanvas-color=#0000FF\n").background == 0xFF0000FFu);
  CHECK (parse ("[new-picture]\nbackground=#00FF00\n").background == 0xFF00FF00u);
  CHECK (parse ("[new-picture]\ncanvas-color=#0000FF\nbackground=#00FF00\n").background == 0xFF0000FFu);
  CHECK (parse ("[new-picture]\nbackground=#00FF00\ncanvas-color=#0000FF\n").background == 0xFF0000FFu);
  CHECK (parse ("[new-picture]\ncanvas-color=red\nbackground=#00FF00\n").background == 0xFF00FF00u);
  CHECK (parse ("[new-picture]\nbackground=#00FF00\ncanvas-color=red\n").background == 0xFF00FF00u);
  CHECK (parse ("[new-picture]\ncanvas-color=transparent\n").background == 0x00000000u);
  CHECK (parse ("[new-picture]\ncanvas-color=red\n").background == 0xFFFFFFFFu);            /* neither valid: white */
  CHECK (parse ("").background == 0xFFFFFFFFu);
  /* A new file names the new key and not the old one. */
  CHECK (strstr (easel_settings_default_text, "canvas-color=#FFFFFF\n") != NULL);
  CHECK (strstr (easel_settings_default_text, "background=") == NULL);
}

static void
as5_invalid_value_falls_back (void)
{
  EaselSettings s = parse ("[new-picture]\nwidth=abc\nheight=200\ncanvas-color=#00FF00\n");

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

/* 0009 AS-3 (REQ-0009-04): `[view] workspace-color` is exactly `#rrggbb`; anything else, or nothing, means no color set. */
static void
as3_workspace_color_setting (void)
{
  EaselSettings s;

  s = parse ("");
  CHECK (!s.has_workspace_color);
  s = parse ("[view]\nworkspace-color=#3465A4\n");
  CHECK (s.has_workspace_color && s.workspace_color == 0x3465A4u);
  s = parse ("[view]\nworkspace-color=#3465a4\n");
  CHECK (s.has_workspace_color && s.workspace_color == 0x3465A4u);
  s = parse ("[view]\nworkspace-color = #E7E7E7 \n");                  /* spaces around, as every value */
  CHECK (s.has_workspace_color && s.workspace_color == 0xE7E7E7u);
  CHECK (!parse ("[view]\nworkspace-color=3465A4\n").has_workspace_color);
  CHECK (!parse ("[view]\nworkspace-color=#346\n").has_workspace_color);
  CHECK (!parse ("[view]\nworkspace-color=#3465A4FF\n").has_workspace_color);
  CHECK (!parse ("[view]\nworkspace-color=blue\n").has_workspace_color);
  CHECK (!parse ("[view]\nworkspace-color=\n").has_workspace_color);
  CHECK (!parse ("[view]\nworkspace-color=#3465G4\n").has_workspace_color);
  CHECK (!parse ("[view]\nworkspace-color=transparent\n").has_workspace_color);
  /* The last repeat wins; an invalid repeat resets to none. */
  CHECK (parse ("[view]\nworkspace-color=#000000\nworkspace-color=#FFFFFF\n").workspace_color == 0xFFFFFFu);
  CHECK (!parse ("[view]\nworkspace-color=#000000\nworkspace-color=red\n").has_workspace_color);
  /* Only in [view]. */
  CHECK (!parse ("[new-picture]\nworkspace-color=#3465A4\n").has_workspace_color);
  /* The dark key, the same syntax, set on its own (REQ-0009-04 as amended before M2). */
  s = parse ("[view]\nworkspace-color-dark=#102030\n");
  CHECK (s.has_workspace_color_dark && s.workspace_color_dark == 0x102030u && !s.has_workspace_color);
  s = parse ("[view]\nworkspace-color=#3465A4\nworkspace-color-dark=#123\n");
  CHECK (!s.has_workspace_color_dark && s.has_workspace_color);
  CHECK (!parse ("").has_workspace_color_dark);
  CHECK (!parse ("[view]\nworkspace-color-dark=\n").has_workspace_color_dark);
}

static void
as14_view_settings (void)
{
  EaselSettings s;

  /* Defaults: nothing said, an empty section, and the text a new file gets. */
  s = parse ("");
  CHECK (!s.grid && s.grid_size == 1 && !s.grid_axonometric && s.show_toolbox && s.show_colorbox && s.show_statusbar);
  s = parse ("[view]\n");
  CHECK (!s.grid && s.grid_size == 1 && !s.grid_axonometric && s.show_toolbox && s.show_colorbox && s.show_statusbar);
  s = parse (easel_settings_default_text);
  CHECK (!s.grid && s.grid_size == 1 && !s.grid_axonometric && s.show_toolbox && s.show_colorbox && s.show_statusbar);

  /* Valid values, case-insensitive, spaces trimmed. */
  s = parse ("[view]\ngrid=on\ngrid-size=8\ngrid-axonometric=ON\ntoolbox=off\ncolorbox = Off \nstatusbar=OFF\n");
  CHECK (s.grid && s.grid_size == 8 && s.grid_axonometric && !s.show_toolbox && !s.show_colorbox && !s.show_statusbar);
  CHECK (parse ("[view]\ngrid-size=1000\n").grid_size == 1000);
  CHECK (parse ("[view]\ngrid-size=0016\n").grid_size == 16);

  /* Invalid or out of range: the default, not clamped. */
  CHECK (!parse ("[view]\ngrid=maybe\n").grid);
  CHECK (!parse ("[view]\ngrid=1\n").grid);
  CHECK (!parse ("[view]\ngrid=\n").grid);
  CHECK (parse ("[view]\ngrid-size=0\n").grid_size == 1);
  CHECK (parse ("[view]\ngrid-size=1001\n").grid_size == 1);
  CHECK (parse ("[view]\ngrid-size=-4\n").grid_size == 1);
  CHECK (parse ("[view]\ngrid-size=abc\n").grid_size == 1);
  CHECK (parse ("[view]\ngrid-size=\n").grid_size == 1);
  CHECK (parse ("[view]\ngrid-size=99999999999999999999\n").grid_size == 1);
  CHECK (parse ("[view]\ntoolbox=maybe\n").show_toolbox);
  CHECK (parse ("[view]\ncolorbox=0\n").show_colorbox);
  CHECK (parse ("[view]\nstatusbar=\n").show_statusbar);

  /* Same syntax as the other settings: the last repeat wins, an invalid repeat resets, other sections are ignored,
     the sections do not leak into each other, and an unparseable file gives every default. */
  CHECK (parse ("[view]\ngrid-size=8\ngrid-size=16\n").grid_size == 16);
  CHECK (parse ("[view]\ngrid-size=8\ngrid-size=bad\n").grid_size == 1);
  CHECK (!parse ("[history]\ngrid=on\n").grid);
  CHECK (!parse ("grid=on\n").grid);
  s = parse ("[view]\ngrid=on\nwidth=300\n[new-picture]\nheight=200\ngrid-size=9\n");
  CHECK (s.grid && s.width == EASEL_SIZE_AUTO && s.height == 200 && s.grid_size == 1);
  s = parse ("[view]\ngrid=on\ntoolbox=off\nhello\n");
  CHECK (!s.grid && s.show_toolbox);
}

/* 0030 AS-6 (REQ-0030-12): [view] toolbox-width is a whole number of logical pixels, past a limit the limit (0 to 240);
   a missing key, text that is no number, or an empty value give 0, the default width. */
static void
as6_toolbox_width_key (void)
{
  CHECK (parse ("").toolbox_width == 0);
  CHECK (parse (easel_settings_default_text).toolbox_width == 0);
  CHECK (parse ("[view]\ntoolbox-width=180\n").toolbox_width == 180);
  CHECK (parse ("[view]\ntoolbox-width = 120 \n").toolbox_width == 120);
  CHECK (parse ("[view]\ntoolbox-width=240\n").toolbox_width == 240);
  CHECK (parse ("[view]\ntoolbox-width=999\n").toolbox_width == 240);
  CHECK (parse ("[view]\ntoolbox-width=99999999999999999999\n").toolbox_width == 240);
  CHECK (parse ("[view]\ntoolbox-width=-5\n").toolbox_width == 0);
  CHECK (parse ("[view]\ntoolbox-width=0\n").toolbox_width == 0);
  CHECK (parse ("[view]\ntoolbox-width=wide\n").toolbox_width == 0);
  CHECK (parse ("[view]\ntoolbox-width=12px\n").toolbox_width == 0);
  CHECK (parse ("[view]\ntoolbox-width=1.5\n").toolbox_width == 0);
  CHECK (parse ("[view]\ntoolbox-width=\n").toolbox_width == 0);
  CHECK (parse ("[view]\ntoolbox-width=180\ntoolbox-width=bad\n").toolbox_width == 0);    /* the last repeat wins */
  CHECK (parse ("[history]\ntoolbox-width=180\n").toolbox_width == 0);                    /* another section */
}

/* 0005 AS-16: [save] jpeg-quality is a whole number from 1 to 100, else 90, silently. */
static void
as16_jpeg_quality_setting (void)
{
  CHECK (parse ("").jpeg_quality == 90);
  CHECK (parse ("[save]\n").jpeg_quality == 90);
  CHECK (parse (easel_settings_default_text).jpeg_quality == 90);
  CHECK (parse ("[save]\njpeg-quality=30\n").jpeg_quality == 30);
  CHECK (parse ("[save]\njpeg-quality=1\n").jpeg_quality == 1);
  CHECK (parse ("[save]\njpeg-quality=100\n").jpeg_quality == 100);
  CHECK (parse ("[save]\njpeg-quality = 075 \n").jpeg_quality == 75);
  CHECK (parse ("[save]\njpeg-quality=0\n").jpeg_quality == 90);
  CHECK (parse ("[save]\njpeg-quality=101\n").jpeg_quality == 90);
  CHECK (parse ("[save]\njpeg-quality=-5\n").jpeg_quality == 90);
  CHECK (parse ("[save]\njpeg-quality=abc\n").jpeg_quality == 90);
  CHECK (parse ("[save]\njpeg-quality=5.0\n").jpeg_quality == 90);
  CHECK (parse ("[save]\njpeg-quality=\n").jpeg_quality == 90);
  CHECK (parse ("[save]\njpeg-quality=99999999999999999999\n").jpeg_quality == 90);
  /* The same syntax as the other keys: the last repeat wins, an invalid repeat resets, other sections do not count. */
  CHECK (parse ("[save]\njpeg-quality=30\njpeg-quality=40\n").jpeg_quality == 40);
  CHECK (parse ("[save]\njpeg-quality=30\njpeg-quality=bad\n").jpeg_quality == 90);
  CHECK (parse ("[view]\njpeg-quality=30\n").jpeg_quality == 90);
  CHECK (parse ("jpeg-quality=30\n").jpeg_quality == 90);
  CHECK (parse ("[save]\njpeg-quality=30\nhello\n").jpeg_quality == 90);   /* an unparseable file: every default */
  CHECK (parse ("[save]\njpeg-quality=30\n[new-picture]\nwidth=300\n").width == 300);
}

/* AS-23 (REQ-0017-24): `[text]`: `font` a family name (spaces round it trimmed; matching it with the installed families, without
   regard to case, is the ui's) or `auto` (the desktop's document font: an empty name here); `size` 1 to 500; four booleans `true` or
   `false` in any case; a missing, empty or bad value takes the default: auto, 12, false. The same syntax as the other keys. */
static void
as23_text_settings (void)
{
  EaselSettings s = parse ("");

  CHECK (s.text_font[0] == '\0' && s.text_size == 12 && !s.text_bold && !s.text_italic && !s.text_underline && !s.text_strike);
  s = parse (easel_settings_default_text);
  CHECK (s.text_font[0] == '\0' && s.text_size == 12 && !s.text_bold);
  s = parse ("[text]\nfont = dejavu serif \nsize=18\nbold=TRUE\nitalic=True\nunderline=false\nstrikethrough=tRuE\n");
  CHECK (strcmp (s.text_font, "dejavu serif") == 0 && s.text_size == 18);
  CHECK (s.text_bold && s.text_italic && !s.text_underline && s.text_strike);
  CHECK (parse ("[text]\nfont=auto\n").text_font[0] == '\0');
  CHECK (parse ("[text]\nfont=AUTO\n").text_font[0] == '\0');
  CHECK (parse ("[text]\nfont=\n").text_font[0] == '\0');
  CHECK (parse ("[text]\nsize=1\n").text_size == 1);
  CHECK (parse ("[text]\nsize=500\n").text_size == 500);
  CHECK (parse ("[text]\nsize=0\n").text_size == 12);
  CHECK (parse ("[text]\nsize=501\n").text_size == 12);
  CHECK (parse ("[text]\nsize=abc\n").text_size == 12);
  CHECK (parse ("[text]\nsize=\n").text_size == 12);
  CHECK (!parse ("[text]\nitalic=yes\n").text_italic);
  CHECK (!parse ("[text]\nbold=1\n").text_bold);
  CHECK (parse ("[text]\nbold=true\nbold=bad\n").text_bold == false);                 /* an invalid repeat resets */
  CHECK (parse ("[text]\nsize=20\nsize=30\n").text_size == 30);                         /* the last repeat wins */
  CHECK (parse ("[text]\nsize=20\nsize=bad\n").text_size == 12);                        /* an invalid repeat resets */
  CHECK (parse ("[view]\nsize=30\n").text_size == 12);                                   /* another section does not count */
  {
    char longname[300];

    memset (longname, 'a', sizeof longname);
    memcpy (longname, "[text]\nfont=", 12);
    longname[sizeof longname - 2] = '\n';
    longname[sizeof longname - 1] = '\0';
    CHECK (parse (longname).text_font[0] == '\0');                                         /* too long to be a family: auto */
  }
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
  RUN (as39_canvas_color_and_background);
  RUN (as4_default_text_parses_to_defaults);
  RUN (as5_invalid_value_falls_back);
  RUN (as18_budget_setting);
  RUN (as14_view_settings);
  RUN (as3_workspace_color_setting);
  RUN (as16_jpeg_quality_setting);
  RUN (as23_text_settings);
  RUN (as6_garbage_is_all_defaults);
  RUN (as7_size_limits);
  RUN (as36_syntax_rules);
  RUN (as6_toolbox_width_key);
  return 0;
}
