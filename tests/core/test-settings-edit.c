/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Preferences (0020): the new keys of the settings file, and the edit of its text that leaves everything else as it is. */

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "check.h"
#include "settings.h"

/* Allocation failure: this test is linked with --wrap, so every allocation of the core comes here. */
static int fail_at, allocations;
void *__real_malloc (size_t n);
void *__wrap_malloc (size_t n);

void *
__wrap_malloc (size_t n)
{
  allocations++;
  return fail_at != 0 && allocations == fail_at ? NULL : __real_malloc (n);
}

static void
check_edit (const char *in, const char *section, const char *key, const char *value, const char *expected)
{
  char *out = NULL;
  size_t n = 0;

  CHECK (easel_settings_edit (in, strlen (in), section, key, value, &out, &n));
  if (n != strlen (expected) || memcmp (out, expected, n) != 0)
    fprintf (stderr, "got:\n%.*s\nexpected:\n%s\n", (int) n, out, expected);
  CHECK (n == strlen (expected) && memcmp (out, expected, n) == 0);
  free (out);
}

/* AS-11 (REQ-0020-09): only that key's line changes, in the form key=value; a repeated key: the last line, the one that counts. */
static void
as11_edit_changes_one_line (void)
{
  check_edit ("[text]\nsize=12\n", "text", "size", "14", "[text]\nsize=14\n");
  check_edit ("# mine\n\n[text]\n# about the size\nsize = 12   \nunknown=1\n\n[other]\nsize=9\n", "text", "size", "14",
              "# mine\n\n[text]\n# about the size\nsize=14\nunknown=1\n\n[other]\nsize=9\n");
  check_edit ("[text]\nsize=10\nbold=true\nsize=12\n", "text", "size", "14", "[text]\nsize=10\nbold=true\nsize=14\n");
  check_edit ("  [text]  \n\tsize\t=\t12\n", "text", "size", "14", "  [text]  \nsize=14\n");
  check_edit ("[text]\n#size=1\nsize=12\n", "text", "size", "14", "[text]\n#size=1\nsize=14\n");
  check_edit ("size=1\n[text]\nsize=12\n", "text", "size", "14", "size=1\n[text]\nsize=14\n");     /* a key above any header is no section's */
  check_edit ("[Text]\nsize=12\n", "text", "size", "14", "[Text]\nsize=12\n\n[text]\nsize=14\n");  /* names are case-sensitive */
  check_edit ("[text]\nsize=12", "text", "size", "14", "[text]\nsize=14");                         /* no line ending stays so */
  check_edit ("[text]\r\nsize=12\r\nbold=true\r\n", "text", "size", "14", "[text]\r\nsize=14\r\nbold=true\r\n");
  /* The value's line in a repeated section: the last, wherever it is. */
  check_edit ("[text]\nsize=1\n[other]\nx=1\n[text]\nsize=2\n", "text", "size", "14", "[text]\nsize=1\n[other]\nx=1\n[text]\nsize=14\n");
  check_edit ("[text]\nsize=1\n[text]\nbold=true\n", "text", "size", "14", "[text]\nsize=14\n[text]\nbold=true\n");
}

/* AS-11: a missing key goes directly after the last key line of the section's last occurrence, or after its header. */
static void
as11_edit_adds_a_key (void)
{
  check_edit ("[text]\nsize=12\n", "text", "bold", "true", "[text]\nsize=12\nbold=true\n");
  check_edit ("[text]\nsize=12\n# end of text\n\n[other]\nx=1\n", "text", "bold", "true", "[text]\nsize=12\nbold=true\n# end of text\n\n[other]\nx=1\n");
  check_edit ("[text]\n# nothing yet\n\n[other]\n", "text", "bold", "true", "[text]\nbold=true\n# nothing yet\n\n[other]\n");
  check_edit ("[text]\nsize=1\n[other]\nx=1\n[text]\n", "text", "bold", "true", "[text]\nsize=1\n[other]\nx=1\n[text]\nbold=true\n");
  check_edit ("[text]\nsize=12", "text", "bold", "true", "[text]\nsize=12\nbold=true\n");
  check_edit ("[text]", "text", "bold", "true", "[text]\nbold=true\n");
  check_edit ("[text]\r\nsize=12\r\n", "text", "bold", "true", "[text]\r\nsize=12\r\nbold=true\r\n");
}

/* AS-11: a missing section goes at the end, after a blank line, with none in an empty file. */
static void
as11_edit_adds_a_section (void)
{
  check_edit ("", "text", "size", "14", "[text]\nsize=14\n");
  check_edit ("[other]\nx=1\n", "text", "size", "14", "[other]\nx=1\n\n[text]\nsize=14\n");
  check_edit ("[other]\nx=1", "text", "size", "14", "[other]\nx=1\n\n[text]\nsize=14\n");
  check_edit ("[other]\nx=1\n\n", "text", "size", "14", "[other]\nx=1\n\n[text]\nsize=14\n");
  check_edit ("[other]\r\nx=1\r\n", "text", "size", "14", "[other]\r\nx=1\r\n\r\n[text]\r\nsize=14\r\n");
  check_edit ("# only a comment\n", "text", "size", "14", "# only a comment\n\n[text]\nsize=14\n");
}

/* AS-11: a removed key's lines all go, in every occurrence of its section; nothing else does. */
static void
as11_edit_removes_a_key (void)
{
  check_edit ("[view]\nworkspace-color=#111111\ntint=12\nworkspace-color=#222222\n", "view", "workspace-color", NULL, "[view]\ntint=12\n");
  check_edit ("[view]\nx=1\n[other]\nx=2\n[view]\nx=3\n# stay\n", "view", "x", NULL, "[view]\n[other]\nx=2\n[view]\n# stay\n");
  check_edit ("[view]\ntint=12\n", "view", "absent", NULL, "[view]\ntint=12\n");
  check_edit ("[other]\n", "view", "absent", NULL, "[other]\n");
  check_edit ("", "view", "absent", NULL, "");
}

/* REQ-0020-10: what makes a file unparseable (REQ-0001-05), and an allocation failing leaves nothing. */
static void
as12_edit_refused (void)
{
  char *out = (char *) 0x1;
  size_t n = 7;

  CHECK (easel_settings_parseable ("[a]\nb=1\n", 8));
  CHECK (easel_settings_parseable ("", 0));
  CHECK (!easel_settings_parseable ("a line with no equals sign\n", 27));
  CHECK (!easel_settings_parseable ("a=b\0c\n", 6));
  CHECK (!easel_settings_edit ("no equals sign\n", 15, "text", "size", "14", &out, &n));
  CHECK (out == (char *) 0x1 && n == 7);
  allocations = 0;
  fail_at = 1;
  CHECK (!easel_settings_edit ("[text]\nsize=1\n", 14, "text", "size", "14", &out, &n));
  fail_at = 0;
  CHECK (out == (char *) 0x1 && n == 7);
}

/* AS-13 (REQ-0020-11): the new keys, what is invalid, and a created file. */
static void
as13_new_keys_and_defaults (void)
{
  EaselSettings s;
  char *text;

  easel_settings_defaults (&s);
  CHECK (s.style == EASEL_STYLE_SYSTEM && s.tint == 12);
  CHECK (s.brush == 1 && s.airbrush == 1 && s.eraser == 2 && !s.eraser_transparent);
  CHECK (s.line_width == 1 && s.fill_style == EASEL_FILL_OUTLINE && s.magnifier == 2);
  text = "[view]\nstyle=DARK\ntint=40\n[drawing]\nbrush=backslash-2\nairbrush=24\neraser=10\neraser-mode=transparent\nline-width=5\nfill-style=fill\nmagnifier=16\n";
  easel_settings_parse (text, strlen (text), &s);
  CHECK (s.style == EASEL_STYLE_DARK && s.tint == 40);
  CHECK (s.brush == 11 && s.airbrush == 2 && s.eraser == 3 && s.eraser_transparent);
  CHECK (s.line_width == 5 && s.fill_style == EASEL_FILL_ONLY && s.magnifier == 16);
  text = "[view]\nstyle=Light\ntint=0\n[drawing]\nbrush=circle-7\nairbrush=9\neraser=4\neraser-mode=opaque\nline-width=1\nfill-style=outline-and-fill\nmagnifier=1\n";
  easel_settings_parse (text, strlen (text), &s);
  CHECK (s.style == EASEL_STYLE_LIGHT && s.tint == 0);
  CHECK (s.brush == 0 && s.airbrush == 0 && s.eraser == 0 && !s.eraser_transparent);
  CHECK (s.fill_style == EASEL_FILL_OUTLINE_AND_FILL && s.magnifier == 1);
  text = "[view]\nstyle=sepia\ntint=51\n[drawing]\nbrush=circle-3\nairbrush=10\neraser=5\neraser-mode=x\nline-width=0\nfill-style=none\nmagnifier=3\n";
  easel_settings_parse (text, strlen (text), &s);
  CHECK (s.style == EASEL_STYLE_SYSTEM && s.tint == 12);
  CHECK (s.brush == 1 && s.airbrush == 1 && s.eraser == 2 && !s.eraser_transparent);
  CHECK (s.line_width == 1 && s.fill_style == EASEL_FILL_OUTLINE && s.magnifier == 2);
  text = "[view]\ntint=abc\n[drawing]\nline-width=6\n";
  easel_settings_parse (text, strlen (text), &s);
  CHECK (s.tint == 12 && s.line_width == 1);
  /* A created file holds them, with their defaults, and reads back the same. */
  CHECK (strstr (easel_settings_default_text, "style=system\n") != NULL);
  CHECK (strstr (easel_settings_default_text, "tint=12\n") != NULL);
  CHECK (strstr (easel_settings_default_text, "[drawing]\nbrush=circle-4\nairbrush=16\neraser=8\neraser-mode=opaque\nline-width=1\nfill-style=outline\nmagnifier=2\n") != NULL);
  easel_settings_parse (easel_settings_default_text, strlen (easel_settings_default_text), &s);
  CHECK (s.brush == 1 && s.airbrush == 1 && s.eraser == 2 && s.magnifier == 2);
}

/* The names the dialog writes are the ones the parser reads, for every preset. */
static void
as7_new_keys (void)
{
  for (int i = 0; i < 12; i++)
    {
      char text[64];
      EaselSettings s;

      snprintf (text, sizeof text, "[drawing]\nbrush=%s\n", easel_settings_brush_name (i));
      easel_settings_parse (text, strlen (text), &s);
      CHECK (s.brush == i);
    }
  CHECK (strcmp (easel_settings_brush_name (1), "circle-4") == 0);
  CHECK (strcmp (easel_settings_brush_name (6), "slash-8") == 0);
  CHECK (strcmp (easel_settings_style_name (EASEL_STYLE_DARK), "dark") == 0);
  CHECK (strcmp (easel_settings_fill_name (EASEL_FILL_OUTLINE_AND_FILL), "outline-and-fill") == 0);
}

int
main (void)
{
  RUN (as11_edit_changes_one_line);
  RUN (as11_edit_adds_a_key);
  RUN (as11_edit_adds_a_section);
  RUN (as11_edit_removes_a_key);
  RUN (as12_edit_refused);
  RUN (as13_new_keys_and_defaults);
  RUN (as7_new_keys);
  return 0;
}
