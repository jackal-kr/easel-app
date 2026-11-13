/* SPDX-License-Identifier: GPL-3.0-or-later */
/* The text of a text box, scenarios of feature 0017: its characters, their style runs and the style at the caret. */

#include <stdint.h>
#include <string.h>

#include "check.h"
#include "text-buffer.h"

#define BLACK 0xFF000000u
#define RED 0xFFFF0000u

static EaselTextStyle
plain (void)
{
  return (EaselTextStyle) { "DejaVu Sans", 12, 0, BLACK };
}

static bool
text_is (const EaselTextBuffer *b, const char *want)
{
  size_t n;
  const char *t = easel_text_buffer_text (b, &n);

  return n == strlen (want) && memcmp (t, want, n) == 0;
}

/* Types `s` at the caret in the style at the caret, as the box does (REQ-0017-12). */
static void
type (EaselTextBuffer *b, const char *s)
{
  EaselTextStyle st;

  easel_text_buffer_style_at_caret (b, false, &st);
  CHECK (easel_text_buffer_insert (b, s, strlen (s), &st, EASEL_TEXT_EDIT_TYPING));
}

/* AS-11 (REQ-0017-12, REQ-0017-14), the core part: "abc def" in the starting style; "def" selected and set to 24 and bold: "def" is
   24 bold and "abc " is not, in two runs; "c d" selected: its size and bold are mixed, its family not; the caret after "abc " with
   nothing selected, italic set, then "x" typed: only "x" is italic. */
static void
as11_style_runs (void)
{
  EaselTextStyle start = plain (), v = { 0 }, st;
  EaselTextBuffer *b = easel_text_buffer_new (&start);
  const EaselTextRun *runs;
  size_t n;
  unsigned mixed;

  CHECK (b != NULL);
  type (b, "abc def");
  CHECK (text_is (b, "abc def"));
  runs = easel_text_buffer_runs (b, &n);
  CHECK (n == 1 && runs[0].start == 0 && runs[0].style.size_pt == 12);
  CHECK (strcmp (runs[0].style.family, "DejaVu Sans") == 0);
  /* "def" selected (bytes 4 to 7), size 24 and bold */
  easel_text_buffer_select (b, 4, 7);
  v.size_pt = 24;
  CHECK (easel_text_buffer_apply (b, EASEL_TEXT_SIZE, &v, false));
  v.flags = EASEL_TEXT_BOLD;
  CHECK (easel_text_buffer_apply (b, EASEL_TEXT_FLAG_BOLD, &v, false));
  runs = easel_text_buffer_runs (b, &n);
  CHECK (n == 2);
  CHECK (runs[0].start == 0 && runs[0].style.size_pt == 12 && runs[0].style.flags == 0);
  CHECK (runs[1].start == 4 && runs[1].style.size_pt == 24 && runs[1].style.flags == EASEL_TEXT_BOLD);
  CHECK (runs[0].style.family == runs[1].style.family);                     /* interned: one family, one pointer */
  /* "c d" (bytes 2 to 5): size and bold differ, the family and the color do not */
  easel_text_buffer_select (b, 2, 5);
  easel_text_buffer_selection_style (b, false, &st, &mixed);
  CHECK (mixed == (EASEL_TEXT_SIZE | EASEL_TEXT_FLAG_BOLD));
  /* the caret after "abc " with nothing selected, italic, then "x": only "x" is italic */
  easel_text_buffer_select (b, 4, 4);
  v.flags = EASEL_TEXT_ITALIC;
  CHECK (easel_text_buffer_apply (b, EASEL_TEXT_FLAG_ITALIC, &v, false));
  CHECK (text_is (b, "abc def"));                                           /* no text changed */
  type (b, "x");
  CHECK (text_is (b, "abc xdef"));
  runs = easel_text_buffer_runs (b, &n);
  CHECK (n == 3);
  CHECK (runs[1].start == 4 && (runs[1].style.flags & EASEL_TEXT_ITALIC) && runs[1].style.size_pt == 12);
  CHECK (runs[2].start == 5 && !(runs[2].style.flags & EASEL_TEXT_ITALIC) && runs[2].style.size_pt == 24);
  CHECK (!(runs[0].style.flags & EASEL_TEXT_ITALIC));
  easel_text_buffer_free (b);
}

/* REQ-0017-26: the style at the caret is the style of the character before it; at a line's start (the caller says) or with none
   before, of the character after it; in an empty buffer the starting style. A pending style is forgotten when the caret moves. */
static void
style_at_caret_rule (void)
{
  EaselTextStyle start = plain (), v = { 0 }, st;
  EaselTextBuffer *b = easel_text_buffer_new (&start);

  CHECK (b != NULL);
  easel_text_buffer_style_at_caret (b, false, &st);
  CHECK (st.size_pt == 12 && st.color == BLACK);                           /* empty: the starting style */
  type (b, "ab");
  easel_text_buffer_select (b, 1, 2);                                      /* "b" red */
  v.color = RED;
  CHECK (easel_text_buffer_apply (b, EASEL_TEXT_COLOR, &v, false));
  easel_text_buffer_select (b, 2, 2);
  easel_text_buffer_style_at_caret (b, false, &st);
  CHECK (st.color == RED);                                                 /* before the caret: "b" */
  easel_text_buffer_select (b, 1, 1);
  easel_text_buffer_style_at_caret (b, false, &st);
  CHECK (st.color == BLACK);                                               /* before the caret: "a" */
  easel_text_buffer_style_at_caret (b, true, &st);
  CHECK (st.color == RED);                                                 /* at a line's start: after it, "b" */
  easel_text_buffer_select (b, 0, 0);
  easel_text_buffer_style_at_caret (b, false, &st);
  CHECK (st.color == BLACK);                                               /* none before: "a" after */
  /* a pending style lasts until the caret moves */
  v.size_pt = 30;
  CHECK (easel_text_buffer_apply (b, EASEL_TEXT_SIZE, &v, false));
  easel_text_buffer_style_at_caret (b, false, &st);
  CHECK (st.size_pt == 30);
  easel_text_buffer_select (b, 1, 1);
  easel_text_buffer_style_at_caret (b, false, &st);
  CHECK (st.size_pt == 12);
  /* the caret set where it is (a press in the box at the caret) is no move: the pending style stays (JOD's bug, 2026-11-11) */
  CHECK (easel_text_buffer_apply (b, EASEL_TEXT_SIZE, &v, false));
  easel_text_buffer_select (b, 1, 1);
  easel_text_buffer_style_at_caret (b, false, &st);
  CHECK (st.size_pt == 30);
  easel_text_buffer_free (b);
  /* and in an empty box, before its first letter */
  b = easel_text_buffer_new (&start);
  CHECK (b != NULL);
  v.size_pt = 15;
  CHECK (easel_text_buffer_apply (b, EASEL_TEXT_SIZE, &v, false));
  easel_text_buffer_select (b, 0, 0);
  type (b, "d");
  CHECK (easel_text_buffer_runs (b, &(size_t) { 0 })[0].style.size_pt == 15);
  easel_text_buffer_free (b);
}

/* REQ-0017-29 (JOD, 2026-11-13): typed text takes the style at the caret with the foreground's color, whatever color the text at the caret
   has: after a 20-point bold black "a" and before a 12-point red "b", with the foreground white: white, 20, bold; at a line's start, "b"'s
   12 points, white; a style set for the text typed next (Bold, a color) kept but its color the foreground's; a foreground with alpha 0 kept
   as given; an empty buffer: the starting style, the foreground's color. */
static void
as29_typed_style (void)
{
  EaselTextStyle start = plain (), v = { 0 }, st;
  EaselTextBuffer *b = easel_text_buffer_new (&start);
  const uint32_t white = 0xFFFFFFFFu, clear = 0x00000000u;

  CHECK (b != NULL);
  easel_text_buffer_typed_style (b, false, white, &st);
  CHECK (st.color == white && st.size_pt == 12 && st.flags == 0);             /* empty: the starting style, the foreground */
  type (b, "ab");
  easel_text_buffer_select (b, 0, 1);
  v.size_pt = 20;
  CHECK (easel_text_buffer_apply (b, EASEL_TEXT_SIZE, &v, false));
  v.flags = EASEL_TEXT_BOLD;
  CHECK (easel_text_buffer_apply (b, EASEL_TEXT_FLAG_BOLD, &v, false));
  easel_text_buffer_select (b, 1, 2);
  v.color = RED;
  CHECK (easel_text_buffer_apply (b, EASEL_TEXT_COLOR, &v, false));
  easel_text_buffer_select (b, 1, 1);
  easel_text_buffer_style_at_caret (b, false, &st);
  CHECK (st.color == BLACK && st.size_pt == 20);                              /* the caret's own: the black "a" */
  easel_text_buffer_typed_style (b, false, white, &st);
  CHECK (st.color == white && st.size_pt == 20 && (st.flags & EASEL_TEXT_BOLD) && strcmp (st.family, "DejaVu Sans") == 0);
  easel_text_buffer_typed_style (b, true, white, &st);
  CHECK (st.color == white && st.size_pt == 12 && !(st.flags & EASEL_TEXT_BOLD));   /* at a line's start: "b"'s, not red */
  easel_text_buffer_typed_style (b, false, clear, &st);
  CHECK (st.color == clear);                                                  /* alpha 0 as given (holes, as the Pencil) */
  /* a style for the text typed next: its Bold kept, its color not */
  easel_text_buffer_select (b, 2, 2);
  v.flags = EASEL_TEXT_BOLD;
  CHECK (easel_text_buffer_apply (b, EASEL_TEXT_FLAG_BOLD, &v, false));
  v.color = RED;
  CHECK (easel_text_buffer_apply (b, EASEL_TEXT_COLOR, &v, false));
  easel_text_buffer_typed_style (b, false, white, &st);
  CHECK (st.color == white && (st.flags & EASEL_TEXT_BOLD));
  easel_text_buffer_free (b);
}

/* Inserting replaces the selection and puts the caret after the text; deleting puts it at the start; runs stay minimal (no empty run,
   no two equal neighbours) and on UTF-8 boundaries. */
static void
insert_delete_runs (void)
{
  EaselTextStyle start = plain (), red = plain ();
  EaselTextBuffer *b = easel_text_buffer_new (&start);
  const EaselTextRun *runs;
  size_t n, from, to;

  CHECK (b != NULL);
  red.color = RED;
  red.family = easel_text_buffer_intern (b, "DejaVu Sans");
  type (b, "h\xC3\xA9llo");                                                 /* "héllo": é is 2 bytes */
  CHECK (easel_text_buffer_caret (b) == 6);
  easel_text_buffer_select (b, 1, 3);                                      /* "é" */
  CHECK (easel_text_buffer_selection (b, &from, &to) && from == 1 && to == 3);
  CHECK (easel_text_buffer_insert (b, "E", 1, &red, EASEL_TEXT_EDIT_PASTE));
  CHECK (text_is (b, "hEllo") && easel_text_buffer_caret (b) == 2);
  CHECK (!easel_text_buffer_selection (b, &from, &to));
  runs = easel_text_buffer_runs (b, &n);
  CHECK (n == 3 && runs[1].start == 1 && runs[1].style.color == RED && runs[2].start == 2);
  CHECK (easel_text_buffer_delete (b, 1, 2, EASEL_TEXT_EDIT_DELETE));                              /* the red E gone: the two black runs join */
  CHECK (text_is (b, "hllo") && easel_text_buffer_caret (b) == 1);
  runs = easel_text_buffer_runs (b, &n);
  CHECK (n == 1 && runs[0].start == 0);
  CHECK (easel_text_buffer_delete (b, 0, 4, EASEL_TEXT_EDIT_DELETE));
  CHECK (text_is (b, ""));
  runs = easel_text_buffer_runs (b, &n);
  CHECK (n == 0);
  easel_text_buffer_free (b);
}

/* REQ-0017-12: a change of selected text in the middle of the text reaches exactly the selected characters: "b" of "abc" made bold
   leaves "a" and "c" as they were. */
static void
apply_to_middle_only (void)
{
  EaselTextStyle start = plain (), v = { 0 };
  EaselTextBuffer *b = easel_text_buffer_new (&start);
  const EaselTextRun *runs;
  size_t n;

  CHECK (b != NULL);
  type (b, "abc");
  easel_text_buffer_select (b, 1, 2);
  v.flags = EASEL_TEXT_BOLD;
  CHECK (easel_text_buffer_apply (b, EASEL_TEXT_FLAG_BOLD, &v, false));
  runs = easel_text_buffer_runs (b, &n);
  CHECK (n == 3);
  CHECK (runs[0].start == 0 && runs[0].style.flags == 0);
  CHECK (runs[1].start == 1 && runs[1].style.flags == EASEL_TEXT_BOLD);
  CHECK (runs[2].start == 2 && runs[2].style.flags == 0);
  easel_text_buffer_free (b);
}

/* AS-8 (REQ-0017-08), the core part: "abc" typed (one run), the caret moved left, "x" typed (another run), "b" made bold (a format
   edit), the box moved (a geometry edit): Undo takes back the move, the bold, the "x", the "abc", then has nothing left; Redo redoes
   them in turn. A bold set with nothing selected is no edit and is forgotten when the caret moves. A new edit drops the redo edits. */
static void
as8_edit_runs_and_undo (void)
{
  EaselTextStyle start = plain (), v = { 0 }, st;
  EaselTextBuffer *b = easel_text_buffer_new (&start);
  int old_rect[4] = { 10, 10, 60, 20 }, new_rect[4] = { 40, 30, 60, 20 }, rect[4];
  bool geometry;
  const EaselTextRun *runs;
  size_t n;

  CHECK (b != NULL);
  CHECK (!easel_text_buffer_can_undo (b) && !easel_text_buffer_can_redo (b));
  type (b, "a");
  type (b, "b");
  type (b, "c");
  easel_text_buffer_select (b, 2, 2);                                       /* Left: between "b" and "c" */
  type (b, "x");                                                           /* "abxc" */
  easel_text_buffer_select (b, 1, 2);                                       /* "b" */
  v.flags = EASEL_TEXT_BOLD;
  CHECK (easel_text_buffer_apply (b, EASEL_TEXT_FLAG_BOLD, &v, false));
  CHECK (easel_text_buffer_push_geometry (b, old_rect, new_rect));
  CHECK (text_is (b, "abxc"));
  /* Undo: the move, the bold, the "x", the "abc" */
  CHECK (easel_text_buffer_undo (b, rect, &geometry) && geometry && memcmp (rect, old_rect, sizeof rect) == 0);
  CHECK (easel_text_buffer_undo (b, rect, &geometry) && !geometry);
  runs = easel_text_buffer_runs (b, &n);
  CHECK (n == 1 && runs[0].style.flags == 0 && text_is (b, "abxc"));
  CHECK (easel_text_buffer_undo (b, rect, &geometry) && text_is (b, "abc"));
  CHECK (easel_text_buffer_undo (b, rect, &geometry) && text_is (b, ""));
  CHECK (!easel_text_buffer_can_undo (b) && !easel_text_buffer_undo (b, rect, &geometry));
  /* Redo in turn */
  CHECK (easel_text_buffer_can_redo (b));
  CHECK (easel_text_buffer_redo (b, rect, &geometry) && text_is (b, "abc"));
  CHECK (easel_text_buffer_redo (b, rect, &geometry) && text_is (b, "abxc"));
  CHECK (easel_text_buffer_redo (b, rect, &geometry) && !geometry);
  runs = easel_text_buffer_runs (b, &n);
  CHECK (n == 3 && runs[1].start == 1 && runs[1].style.flags == EASEL_TEXT_BOLD);
  CHECK (easel_text_buffer_redo (b, rect, &geometry) && geometry && memcmp (rect, new_rect, sizeof rect) == 0);
  CHECK (!easel_text_buffer_can_redo (b));
  /* A bold with nothing selected is no edit, and is forgotten when the caret moves */
  CHECK (easel_text_buffer_undo (b, rect, &geometry));                      /* the move undone: redo has one */
  easel_text_buffer_select (b, 4, 4);
  v.flags = EASEL_TEXT_BOLD;
  CHECK (easel_text_buffer_apply (b, EASEL_TEXT_FLAG_BOLD, &v, false));
  CHECK (easel_text_buffer_can_redo (b));                                   /* no edit: the redo edit is still there */
  easel_text_buffer_select (b, 0, 0);
  type (b, "y");
  easel_text_buffer_style_at_caret (b, false, &st);
  CHECK (!(st.flags & EASEL_TEXT_BOLD));
  CHECK (!easel_text_buffer_can_redo (b));                                  /* the typing dropped the redo edit */
  CHECK (easel_text_buffer_undo (b, rect, &geometry) && text_is (b, "abxc")); /* "y" alone, not joined with "x" */
  easel_text_buffer_free (b);
}

/* REQ-0017-08: Backspace pressed three times in a row is one edit; Delete pressed twice at the same place is one edit; a Backspace
   after moving the caret starts a new one. Typing over a selection is one edit with it. */
static void
delete_runs (void)
{
  EaselTextStyle start = plain ();
  EaselTextBuffer *b = easel_text_buffer_new (&start);
  int rect[4];
  bool geometry;

  CHECK (b != NULL);
  type (b, "abcdefgh");
  easel_text_buffer_select (b, 8, 8);
  for (size_t c = 8; c > 5; c--)                                           /* three Backspaces: "abcde" */
    CHECK (easel_text_buffer_delete (b, c - 1, c, EASEL_TEXT_EDIT_BACKSPACE));
  CHECK (text_is (b, "abcde"));
  easel_text_buffer_select (b, 1, 1);
  CHECK (easel_text_buffer_delete (b, 1, 2, EASEL_TEXT_EDIT_DELETE));       /* two Deletes at 1: "ade" */
  CHECK (easel_text_buffer_delete (b, 1, 2, EASEL_TEXT_EDIT_DELETE));
  CHECK (text_is (b, "ade"));
  easel_text_buffer_select (b, 3, 3);
  CHECK (easel_text_buffer_delete (b, 2, 3, EASEL_TEXT_EDIT_BACKSPACE));    /* a new run after the caret moved: "ad" */
  CHECK (easel_text_buffer_undo (b, rect, &geometry) && text_is (b, "ade"));
  CHECK (easel_text_buffer_undo (b, rect, &geometry) && text_is (b, "abcde"));
  CHECK (easel_text_buffer_undo (b, rect, &geometry) && text_is (b, "abcdefgh"));
  /* typing over a selection: one edit with the removal */
  easel_text_buffer_select (b, 2, 5);
  type (b, "Z");
  CHECK (text_is (b, "abZfgh"));
  CHECK (easel_text_buffer_undo (b, rect, &geometry) && text_is (b, "abcdefgh"));
  easel_text_buffer_free (b);
}

/* REQ-0017-08: a run of typing ends when the caret moves, also when it comes back to where the run ends: "ab", Left, Right, "c" are
   two edits. Two Undos then a new edit leave nothing to redo. */
static void
runs_end_and_redo_dropped (void)
{
  EaselTextStyle start = plain ();
  EaselTextBuffer *b = easel_text_buffer_new (&start);
  int rect[4];
  bool geometry;

  CHECK (b != NULL);
  type (b, "ab");
  easel_text_buffer_select (b, 1, 1);
  easel_text_buffer_select (b, 2, 2);
  type (b, "c");
  CHECK (easel_text_buffer_undo (b, rect, &geometry) && text_is (b, "ab"));
  CHECK (easel_text_buffer_undo (b, rect, &geometry) && text_is (b, ""));
  type (b, "z");
  CHECK (!easel_text_buffer_can_redo (b));
  CHECK (!easel_text_buffer_redo (b, rect, &geometry) && text_is (b, "z"));
  easel_text_buffer_free (b);
}

/* REQ-0017-08, AS-8: after Undo the caret and selection are as before the edit, after Redo as right after it: "abc", Left, "x": Undo
   leaves the caret where the "x" was, Redo after it; "b" set bold: Undo and Redo both leave "b" selected; "abcd" with "c" removed by
   Delete: Redo leaves the caret where "c" was; a move leaves the caret where it is. */
static void
as8_caret_after_undo_redo (void)
{
  EaselTextStyle start = plain (), v = { 0 };
  EaselTextBuffer *b = easel_text_buffer_new (&start);
  int r0[4] = { 1, 2, 3, 4 }, r1[4] = { 5, 6, 3, 4 }, rect[4];
  bool geometry;
  size_t from, to;

  CHECK (b != NULL);
  type (b, "abc");
  easel_text_buffer_select (b, 2, 2);
  type (b, "x");
  CHECK (easel_text_buffer_undo (b, rect, &geometry) && text_is (b, "abc"));
  CHECK (!easel_text_buffer_selection (b, &from, &to) && easel_text_buffer_caret (b) == 2);
  CHECK (easel_text_buffer_redo (b, rect, &geometry) && text_is (b, "abxc"));
  CHECK (!easel_text_buffer_selection (b, &from, &to) && easel_text_buffer_caret (b) == 3);
  /* a Bold */
  easel_text_buffer_select (b, 1, 2);
  v.flags = EASEL_TEXT_BOLD;
  CHECK (easel_text_buffer_apply (b, EASEL_TEXT_FLAG_BOLD, &v, false));
  easel_text_buffer_select (b, 4, 4);
  CHECK (easel_text_buffer_undo (b, rect, &geometry));
  CHECK (easel_text_buffer_selection (b, &from, &to) && from == 1 && to == 2);
  easel_text_buffer_select (b, 4, 4);
  CHECK (easel_text_buffer_redo (b, rect, &geometry));
  CHECK (easel_text_buffer_selection (b, &from, &to) && from == 1 && to == 2);   /* still selected, as right after the Bold */
  /* a move */
  easel_text_buffer_select (b, 3, 3);
  CHECK (easel_text_buffer_push_geometry (b, r0, r1));
  easel_text_buffer_select (b, 4, 4);
  CHECK (easel_text_buffer_undo (b, rect, &geometry) && geometry && easel_text_buffer_caret (b) == 4);
  easel_text_buffer_select (b, 1, 1);
  CHECK (easel_text_buffer_redo (b, rect, &geometry) && geometry && easel_text_buffer_caret (b) == 1);
  easel_text_buffer_free (b);
  /* a delete */
  b = easel_text_buffer_new (&start);
  CHECK (b != NULL);
  type (b, "abcd");
  easel_text_buffer_select (b, 2, 2);
  CHECK (easel_text_buffer_delete (b, 2, 3, EASEL_TEXT_EDIT_DELETE) && text_is (b, "abd"));
  CHECK (easel_text_buffer_undo (b, rect, &geometry) && text_is (b, "abcd"));
  easel_text_buffer_select (b, 4, 4);
  CHECK (easel_text_buffer_redo (b, rect, &geometry) && text_is (b, "abd"));
  CHECK (!easel_text_buffer_selection (b, &from, &to) && easel_text_buffer_caret (b) == 2);
  easel_text_buffer_free (b);
}

/* "n" runs of `len` characters each, the runs in two colors by turns; one edit for each run in the history. */
static EaselTextBuffer *
striped (size_t n, size_t len)
{
  EaselTextStyle start = plain ();
  EaselTextBuffer *b = easel_text_buffer_new (&start);

  CHECK (b != NULL);
  for (size_t i = 0; i < n; i++)
    {
      EaselTextStyle st = plain ();

      st.color = i % 2 ? RED : BLACK;
      CHECK (easel_text_buffer_insert (b, "aaaa", len, &st, EASEL_TEXT_EDIT_PASTE));
    }
  return b;
}

/* The same as a copy, which holds no more memory than its text and runs need, and no edits. */
static EaselTextBuffer *
striped_tight (size_t n, size_t len)
{
  EaselTextBuffer *b = striped (n, len), *c = easel_text_buffer_copy (b);

  CHECK (c != NULL);
  easel_text_buffer_free (b);
  return c;
}

/* Undo then Redo of the last edit, which must not need more memory than is held (REQ-0017-08: the edit took it when it was made). */
static void
steps_need_no_memory (EaselTextBuffer *b)
{
  size_t t0, r0, t, r;
  int rect[4];
  bool geometry;

  easel_text_buffer_capacity (b, &t0, &r0);
  CHECK (easel_text_buffer_undo (b, rect, &geometry));
  easel_text_buffer_capacity (b, &t, &r);
  CHECK (t == t0 && r == r0);
  CHECK (easel_text_buffer_redo (b, rect, &geometry));
  easel_text_buffer_capacity (b, &t, &r);
  CHECK (t == t0 && r == r0);
}

/* REQ-0017-08: each kind of edit holds, once made, the memory its Undo and Redo need (the memory held, compared, not the length). Runs
   of two characters, 2 to 70 of them, in a copy (which holds only what it needs), so that the held room is at its edge for some; each
   edit starts and ends inside a run, where its Undo and Redo need the most: a paste over a selection, typing in other styles by turns
   (joined), a Delete, Backspaces joined in a run, a format change of selected text. */
static void
edits_hold_their_steps (void)
{
  for (size_t n = 2; n <= 70; n++)
    {
      size_t len = 2 * n, from = 1, to = len - 1;
      EaselTextStyle st = plain (), v = { 0 };
      EaselTextBuffer *b = striped_tight (n, 2);

      easel_text_buffer_select (b, from, to);                                     /* a paste over most of it */
      st.color = 0xFF00FF00u;                                                     /* a style of neither neighbour */
      CHECK (easel_text_buffer_insert (b, "xy", 2, &st, EASEL_TEXT_EDIT_PASTE));
      steps_need_no_memory (b);
      easel_text_buffer_free (b);

      b = striped_tight (n, 2);
      easel_text_buffer_select (b, 1, 1);                                         /* typing inside the first run, joined */
      for (size_t k = 0; k < n; k++)
        {
          st.color = k % 2 ? 0xFF00FF00u : 0xFF0000FFu;
          CHECK (easel_text_buffer_insert (b, "x", 1, &st, EASEL_TEXT_EDIT_TYPING));
        }
      steps_need_no_memory (b);
      easel_text_buffer_free (b);

      b = striped_tight (n, 2);
      easel_text_buffer_select (b, 0, 0);                                         /* a Delete of most of it */
      CHECK (easel_text_buffer_delete (b, from, to, EASEL_TEXT_EDIT_DELETE));
      steps_need_no_memory (b);
      easel_text_buffer_free (b);

      b = striped_tight (n, 2);
      easel_text_buffer_select (b, to, to);                                       /* Backspaces in a run, back to inside the first run */
      for (size_t k = to; k > from; k--)
        CHECK (easel_text_buffer_delete (b, k - 1, k, EASEL_TEXT_EDIT_BACKSPACE));
      steps_need_no_memory (b);
      easel_text_buffer_free (b);

      b = striped_tight (n, 2);
      easel_text_buffer_select (b, from, to);                                     /* a paste of two runs over most of it */
      {
        EaselTextStyle green = plain (), blue = plain ();
        EaselTextRun two[2];

        green.color = 0xFF00FF00u;
        blue.color = 0xFF0000FFu;
        green.family = easel_text_buffer_intern (b, green.family);
        blue.family = easel_text_buffer_intern (b, blue.family);
        two[0] = (EaselTextRun) { 0, green };
        two[1] = (EaselTextRun) { 1, blue };
        CHECK (easel_text_buffer_insert_runs (b, "xy", 2, two, 2, EASEL_TEXT_EDIT_PASTE));
      }
      steps_need_no_memory (b);
      easel_text_buffer_free (b);

      b = striped_tight (n, 2);
      easel_text_buffer_select (b, from, to);                                     /* a format change of most of it */
      v.color = 0xFF0000FFu;
      CHECK (easel_text_buffer_apply (b, EASEL_TEXT_COLOR, &v, false));
      steps_need_no_memory (b);
      easel_text_buffer_free (b);

      b = striped_tight (n, 2);
      easel_text_buffer_select (b, from, to);                                     /* a size step of most of it (REQ-0017-12) */
      {
        bool changed;

        CHECK (easel_text_buffer_step_size (b, 1, 1, 500, false, &changed) && changed);
      }
      steps_need_no_memory (b);
      easel_text_buffer_free (b);
    }
}

/* The sizes of the runs of b, in order, are the n given. */
static bool
sizes_are (const EaselTextBuffer *b, size_t n, const int *want)
{
  size_t got;
  const EaselTextRun *runs = easel_text_buffer_runs (b, &got);

  if (got != n)
    return false;
  for (size_t i = 0; i < n; i++)
    if (runs[i].style.size_pt != want[i])
      return false;
  return true;
}

/* AS-26, the core part (REQ-0017-12): "abc" with "ab" at 12 and "c" at 20, all selected: a step of +1 makes them 13 and 21 as one
   edit, Undo gives 12 and 20 back; two steps of -1 make 11 and 19, one edit each; "a" at 500 and "b" at 499: +1 makes "b" 500 and
   leaves "a", and once more changes nothing and is no edit; "a" at 1: -1 the same; nothing selected at 12: +1 then "x" typed, "x" is
   13; nothing selected at 500: +1 changes nothing. */
static void
as26_step_size (void)
{
  EaselTextStyle start = plain (), v = { 0 }, st;
  EaselTextBuffer *b = easel_text_buffer_new (&start);
  int rect[4];
  bool geometry, changed;

  CHECK (b != NULL);
  type (b, "abc");
  easel_text_buffer_select (b, 2, 3);
  v.size_pt = 20;
  CHECK (easel_text_buffer_apply (b, EASEL_TEXT_SIZE, &v, false));
  easel_text_buffer_select (b, 0, 3);
  CHECK (easel_text_buffer_step_size (b, 1, 1, 500, false, &changed) && changed);
  CHECK (sizes_are (b, 2, (int[]) { 13, 21 }));
  CHECK (easel_text_buffer_undo (b, rect, &geometry) && sizes_are (b, 2, (int[]) { 12, 20 }));
  CHECK (easel_text_buffer_undo (b, rect, &geometry) && sizes_are (b, 1, (int[]) { 12 }));   /* the step was one edit */
  CHECK (easel_text_buffer_redo (b, rect, &geometry) && easel_text_buffer_redo (b, rect, &geometry));
  CHECK (sizes_are (b, 2, (int[]) { 13, 21 }));
  CHECK (easel_text_buffer_step_size (b, -1, 1, 500, false, &changed) && changed);
  CHECK (easel_text_buffer_step_size (b, -1, 1, 500, false, &changed) && changed);
  CHECK (sizes_are (b, 2, (int[]) { 11, 19 }));
  CHECK (easel_text_buffer_undo (b, rect, &geometry) && sizes_are (b, 2, (int[]) { 12, 20 }));
  /* the limits: "a" 500 and "b" 499 */
  easel_text_buffer_select (b, 0, 1);
  v.size_pt = 500;
  CHECK (easel_text_buffer_apply (b, EASEL_TEXT_SIZE, &v, false));
  easel_text_buffer_select (b, 1, 2);
  v.size_pt = 499;
  CHECK (easel_text_buffer_apply (b, EASEL_TEXT_SIZE, &v, false));
  easel_text_buffer_select (b, 0, 2);
  CHECK (easel_text_buffer_step_size (b, 1, 1, 500, false, &changed) && changed);
  CHECK (sizes_are (b, 2, (int[]) { 500, 20 }));
  CHECK (easel_text_buffer_step_size (b, 1, 1, 500, false, &changed) && !changed);
  CHECK (sizes_are (b, 2, (int[]) { 500, 20 }));
  CHECK (easel_text_buffer_undo (b, rect, &geometry) && sizes_are (b, 3, (int[]) { 500, 499, 20 }));   /* no edit was added */
  easel_text_buffer_select (b, 0, 1);
  v.size_pt = 1;
  CHECK (easel_text_buffer_apply (b, EASEL_TEXT_SIZE, &v, false));
  CHECK (easel_text_buffer_step_size (b, -1, 1, 500, false, &changed) && !changed);
  CHECK (sizes_are (b, 3, (int[]) { 1, 499, 20 }));
  CHECK (easel_text_buffer_undo (b, rect, &geometry) && sizes_are (b, 3, (int[]) { 500, 499, 20 }));
  /* a step of 10 (Page Up and Page Down, REQ-0017-25) is clamped: "a" at 497 and "b" at 500 both go to 500, and again nothing
     changes; "a" at 4 and "b" at 1 both go to 1 */
  easel_text_buffer_select (b, 0, 1);
  v.size_pt = 497;
  CHECK (easel_text_buffer_apply (b, EASEL_TEXT_SIZE, &v, false));
  easel_text_buffer_select (b, 1, 2);
  v.size_pt = 500;
  CHECK (easel_text_buffer_apply (b, EASEL_TEXT_SIZE, &v, false));
  easel_text_buffer_select (b, 0, 2);
  CHECK (easel_text_buffer_step_size (b, 10, 1, 500, false, &changed) && changed);
  CHECK (sizes_are (b, 2, (int[]) { 500, 20 }));
  CHECK (easel_text_buffer_step_size (b, 10, 1, 500, false, &changed) && !changed);
  easel_text_buffer_select (b, 0, 1);
  v.size_pt = 4;
  CHECK (easel_text_buffer_apply (b, EASEL_TEXT_SIZE, &v, false));
  easel_text_buffer_select (b, 1, 2);
  v.size_pt = 1;
  CHECK (easel_text_buffer_apply (b, EASEL_TEXT_SIZE, &v, false));
  easel_text_buffer_select (b, 0, 2);
  CHECK (easel_text_buffer_step_size (b, -10, 1, 500, false, &changed) && changed);
  CHECK (sizes_are (b, 2, (int[]) { 1, 20 }));
  easel_text_buffer_free (b);
  /* nothing selected: the text typed next */
  b = easel_text_buffer_new (&start);
  CHECK (b != NULL);
  type (b, "ab");
  CHECK (easel_text_buffer_step_size (b, 1, 1, 500, false, &changed) && changed);
  CHECK (!easel_text_buffer_can_redo (b) && text_is (b, "ab"));
  type (b, "x");
  CHECK (sizes_are (b, 2, (int[]) { 12, 13 }));
  v.size_pt = 500;
  CHECK (easel_text_buffer_apply (b, EASEL_TEXT_SIZE, &v, false));
  CHECK (easel_text_buffer_step_size (b, 1, 1, 500, false, &changed) && !changed);
  easel_text_buffer_style_at_caret (b, false, &st);
  CHECK (st.size_pt == 500);
  easel_text_buffer_free (b);
}

/* AS-24, the core part (REQ-0017-08, REQ-0017-07): "ab" in two styles, "b" selected, one edit to redo; an edit that cannot have
   its memory is refused and leaves the text, the styles, the caret, the selection and the edits to undo and redo exactly as before. */
static void
refused_edit_changes_nothing (void)
{
  EaselTextBuffer *b = striped (3, 1);
  EaselTextStyle st = plain ();
  const EaselTextRun *runs;
  size_t n, from, to;
  int rect[4];
  bool geometry;

  CHECK (easel_text_buffer_undo (b, rect, &geometry) && text_is (b, "aa"));   /* the third "a" to redo */
  easel_text_buffer_select (b, 1, 2);
  CHECK (!easel_text_buffer_insert (b, "x", SIZE_MAX / 2, &st, EASEL_TEXT_EDIT_PASTE));   /* more than memory can hold */
  CHECK (text_is (b, "aa"));
  runs = easel_text_buffer_runs (b, &n);
  CHECK (n == 2 && runs[0].style.color == BLACK && runs[1].start == 1 && runs[1].style.color == RED);
  CHECK (easel_text_buffer_selection (b, &from, &to) && from == 1 && to == 2 && easel_text_buffer_caret (b) == 2);
  CHECK (easel_text_buffer_can_redo (b));
  CHECK (easel_text_buffer_redo (b, rect, &geometry) && text_is (b, "aaa"));
  CHECK (easel_text_buffer_undo (b, rect, &geometry) && easel_text_buffer_undo (b, rect, &geometry) && text_is (b, "a"));
  easel_text_buffer_free (b);
}

/* REQ-0017-20, the core part: "x" then "bc" over a selected "b" of "abc": a paste of "XY" in two runs (X bold, Y red) replaces the
   selection as one edit, the caret after it, its runs kept; Undo gives "abc" back, Redo the paste. */
static void
paste_runs_one_edit (void)
{
  EaselTextStyle start = plain (), bold = plain (), red = plain ();
  EaselTextBuffer *b = easel_text_buffer_new (&start);
  EaselTextRun runs[2];
  const EaselTextRun *got;
  size_t n;
  int rect[4];
  bool geometry;

  CHECK (b != NULL);
  type (b, "abc");
  bold.flags = EASEL_TEXT_BOLD;
  bold.family = easel_text_buffer_intern (b, bold.family);
  red.color = RED;
  red.family = easel_text_buffer_intern (b, red.family);
  runs[0] = (EaselTextRun) { 0, bold };
  runs[1] = (EaselTextRun) { 1, red };
  easel_text_buffer_select (b, 1, 2);
  CHECK (easel_text_buffer_insert_runs (b, "XY", 2, runs, 2, EASEL_TEXT_EDIT_PASTE));
  CHECK (text_is (b, "aXYc"));
  CHECK (easel_text_buffer_caret (b) == 3);
  got = easel_text_buffer_runs (b, &n);
  CHECK (n == 4 && got[1].start == 1 && got[1].style.flags == EASEL_TEXT_BOLD && got[2].start == 2 && got[2].style.color == RED
         && got[3].start == 3 && got[3].style.flags == 0 && got[3].style.color == BLACK);
  CHECK (easel_text_buffer_undo (b, rect, &geometry) && text_is (b, "abc"));
  CHECK (easel_text_buffer_redo (b, rect, &geometry) && text_is (b, "aXYc"));
  easel_text_buffer_free (b);
}

/* REQ-0017-07 (the box tries an edit on a copy): a copy has the same text, runs, caret, selection and pending style, no edits to undo,
   and changing it leaves the original as it was. */
static void
copy_is_independent (void)
{
  EaselTextStyle start = plain (), v = { 0 }, st;
  EaselTextBuffer *b = easel_text_buffer_new (&start), *c;
  const EaselTextRun *rb, *rc;
  size_t nb, nc, from, to;

  CHECK (b != NULL);
  type (b, "abc def");
  easel_text_buffer_select (b, 4, 7);
  v.color = RED;
  CHECK (easel_text_buffer_apply (b, EASEL_TEXT_COLOR, &v, false));
  easel_text_buffer_select (b, 1, 3);
  c = easel_text_buffer_copy (b);
  CHECK (c != NULL);
  CHECK (text_is (c, "abc def"));
  rb = easel_text_buffer_runs (b, &nb);
  rc = easel_text_buffer_runs (c, &nc);
  CHECK (nb == nc && nc == 2 && rc[1].start == 4 && rc[1].style.color == RED && strcmp (rc[1].style.family, rb[1].style.family) == 0);
  CHECK (easel_text_buffer_selection (c, &from, &to) && from == 1 && to == 3 && easel_text_buffer_caret (c) == 3);
  CHECK (!easel_text_buffer_can_undo (c));
  CHECK (easel_text_buffer_can_undo (b));
  /* a pending style is copied */
  easel_text_buffer_select (b, 2, 2);
  v.size_pt = 40;
  CHECK (easel_text_buffer_apply (b, EASEL_TEXT_SIZE, &v, false));
  easel_text_buffer_free (c);
  c = easel_text_buffer_copy (b);
  CHECK (c != NULL);
  easel_text_buffer_style_at_caret (c, false, &st);
  CHECK (st.size_pt == 40);
  /* the copy changed, the original not */
  type (c, "XYZ");
  CHECK (text_is (c, "abXYZc def"));
  CHECK (text_is (b, "abc def"));
  easel_text_buffer_free (c);
  easel_text_buffer_free (b);
}

int
main (void)
{
  RUN (as11_style_runs);
  RUN (style_at_caret_rule);
  RUN (insert_delete_runs);
  RUN (apply_to_middle_only);
  RUN (as8_edit_runs_and_undo);
  RUN (delete_runs);
  RUN (runs_end_and_redo_dropped);
  RUN (as8_caret_after_undo_redo);
  RUN (edits_hold_their_steps);
  RUN (refused_edit_changes_nothing);
  RUN (paste_runs_one_edit);
  RUN (copy_is_independent);
  RUN (as26_step_size);
  RUN (as29_typed_style);
  return 0;
}
