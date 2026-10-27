/* SPDX-License-Identifier: GPL-3.0-or-later */

#include "text-buffer.h"

#include <stdlib.h>
#include <string.h>

/* A piece of text with its runs (starts relative to the piece). */
typedef struct
{
  char *text;
  size_t len;
  EaselTextRun *runs;
  size_t n;
} Piece;

/* One edit (REQ-0017-08): at `pos`, `before` was replaced by `after`; for a geometry edit the box's rectangles. */
typedef struct
{
  EaselTextEditKind kind;
  size_t pos;
  Piece before, after;
  size_t anchor_before, caret_before;
  size_t anchor_after, caret_after;   /* right after the edit was made, for Redo (REQ-0017-08) */
  int old_rect[4], new_rect[4];
} Edit;

struct EaselTextBuffer
{
  char *text;                 /* not NUL-terminated */
  size_t len, cap;
  EaselTextRun *runs;
  size_t n_runs, runs_cap;
  char **families;            /* the interned family names, owned */
  size_t n_families, families_cap;
  EaselTextStyle start;       /* the style of an empty box */
  size_t anchor, caret;
  bool has_pending;
  EaselTextStyle pending;
  Edit *edits;                /* the first n_done can be undone, the rest up to n_edits redone */
  size_t n_done, n_edits, edits_cap;
  bool can_join;              /* the last edit's run may grow: no caret move, undo or other edit since */
};

static void
piece_free (Piece *p)
{
  free (p->text);
  free (p->runs);
  *p = (Piece) { 0 };
}

static void
drop_edits_from (EaselTextBuffer *b, size_t from)
{
  for (size_t i = from; i < b->n_edits; i++)
    {
      piece_free (&b->edits[i].before);
      piece_free (&b->edits[i].after);
    }
  b->n_edits = from;
}

static bool
same_style (const EaselTextStyle *a, const EaselTextStyle *b)
{
  return a->family == b->family && a->size_pt == b->size_pt && a->flags == b->flags && a->color == b->color;
}

/* Makes room for `n` elements of `size` bytes in *items (capacity *cap); false, unchanged, without memory. */
static bool
reserve (void **items, size_t *cap, size_t n, size_t size)
{
  size_t want = *cap ? *cap : 8;
  void *p;

  if (n <= *cap)
    return true;
  while (want < n)
    {
      if (want > SIZE_MAX / 2 / size)
        return false;
      want *= 2;
    }
  p = realloc (*items, want * size);
  if (p == NULL)
    return false;
  *items = p;
  *cap = want;
  return true;
}

const char *
easel_text_buffer_intern (EaselTextBuffer *b, const char *family)
{
  size_t n;
  char *copy;

  for (size_t i = 0; i < b->n_families; i++)
    if (strcmp (b->families[i], family) == 0)
      return b->families[i];
  if (!reserve ((void **) &b->families, &b->families_cap, b->n_families + 1, sizeof *b->families))
    return NULL;
  n = strlen (family);
  copy = malloc (n + 1);
  if (copy == NULL)
    return NULL;
  memcpy (copy, family, n + 1);
  b->families[b->n_families++] = copy;
  return copy;
}

EaselTextBuffer *
easel_text_buffer_new (const EaselTextStyle *start)
{
  EaselTextBuffer *b = calloc (1, sizeof *b);

  if (b == NULL)
    return NULL;
  b->start = *start;
  b->start.family = easel_text_buffer_intern (b, start->family);
  if (b->start.family == NULL)
    {
      easel_text_buffer_free (b);
      return NULL;
    }
  return b;
}

EaselTextBuffer *
easel_text_buffer_copy (const EaselTextBuffer *b)
{
  EaselTextBuffer *c = easel_text_buffer_new (&b->start);

  if (c == NULL)
    return NULL;
  if (b->len > 0
      && (!reserve ((void **) &c->text, &c->cap, b->len, 1)
          || !reserve ((void **) &c->runs, &c->runs_cap, b->n_runs, sizeof *c->runs)))
    {
      easel_text_buffer_free (c);
      return NULL;
    }
  if (b->len > 0)
    memcpy (c->text, b->text, b->len);
  c->len = b->len;
  for (size_t i = 0; i < b->n_runs; i++)
    {
      c->runs[i] = b->runs[i];
      c->runs[i].style.family = easel_text_buffer_intern (c, b->runs[i].style.family);   /* the copy's own names */
      if (c->runs[i].style.family == NULL)
        {
          easel_text_buffer_free (c);
          return NULL;
        }
    }
  c->n_runs = b->n_runs;
  c->anchor = b->anchor;
  c->caret = b->caret;
  c->has_pending = b->has_pending;
  c->pending = b->pending;
  if (b->has_pending && (c->pending.family = easel_text_buffer_intern (c, b->pending.family)) == NULL)
    {
      easel_text_buffer_free (c);
      return NULL;
    }
  return c;
}

void
easel_text_buffer_free (EaselTextBuffer *b)
{
  if (b == NULL)
    return;
  for (size_t i = 0; i < b->n_families; i++)
    free (b->families[i]);
  free (b->families);
  drop_edits_from (b, 0);
  free (b->edits);
  free (b->runs);
  free (b->text);
  free (b);
}

const char *
easel_text_buffer_text (const EaselTextBuffer *b, size_t *length)
{
  *length = b->len;
  return b->text != NULL ? b->text : "";
}

const EaselTextRun *
easel_text_buffer_runs (const EaselTextBuffer *b, size_t *n)
{
  *n = b->n_runs;
  return b->runs;
}

void
easel_text_buffer_capacity (const EaselTextBuffer *b, size_t *text, size_t *runs)
{
  *text = b->cap;
  *runs = b->runs_cap;
}

size_t
easel_text_buffer_caret (const EaselTextBuffer *b)
{
  return b->caret;
}

void
easel_text_buffer_select (EaselTextBuffer *b, size_t anchor, size_t caret)
{
  b->anchor = anchor <= b->len ? anchor : b->len;
  b->caret = caret <= b->len ? caret : b->len;
  b->has_pending = false;                      /* the caret moved: a pending style is forgotten (REQ-0017-12) */
  b->can_join = false;                         /* and a run of typing or deleting ends (REQ-0017-08) */
}

bool
easel_text_buffer_selection (const EaselTextBuffer *b, size_t *from, size_t *to)
{
  *from = b->anchor < b->caret ? b->anchor : b->caret;
  *to = b->anchor < b->caret ? b->caret : b->anchor;
  return *from < *to;
}

/* The index of the run holding the byte at `pos` (pos < len). */
static size_t
run_at (const EaselTextBuffer *b, size_t pos)
{
  size_t i = 0;

  while (i + 1 < b->n_runs && b->runs[i + 1].start <= pos)
    i++;
  return i;
}

void
easel_text_buffer_style_at_caret (const EaselTextBuffer *b, bool at_line_start, EaselTextStyle *out)
{
  size_t pos = b->caret;

  if (b->has_pending)
    *out = b->pending;
  else if (b->len == 0)
    *out = b->start;
  else if ((at_line_start || pos == 0) && pos < b->len)
    *out = b->runs[run_at (b, pos)].style;               /* the character after the caret */
  else
    *out = b->runs[run_at (b, pos - 1)].style;           /* the character before it (its bytes all have its style) */
}

void
easel_text_buffer_selection_style (const EaselTextBuffer *b, bool at_line_start, EaselTextStyle *out, unsigned *mixed)
{
  size_t from, to;

  *mixed = 0;
  if (!easel_text_buffer_selection (b, &from, &to))
    {
      easel_text_buffer_style_at_caret (b, at_line_start, out);
      return;
    }
  *out = b->runs[run_at (b, from)].style;
  for (size_t i = run_at (b, from) + 1; i < b->n_runs && b->runs[i].start < to; i++)
    {
      const EaselTextStyle *s = &b->runs[i].style;

      if (s->family != out->family)
        *mixed |= EASEL_TEXT_FAMILY;
      if (s->size_pt != out->size_pt)
        *mixed |= EASEL_TEXT_SIZE;
      if ((s->flags ^ out->flags) & EASEL_TEXT_BOLD)
        *mixed |= EASEL_TEXT_FLAG_BOLD;
      if ((s->flags ^ out->flags) & EASEL_TEXT_ITALIC)
        *mixed |= EASEL_TEXT_FLAG_ITALIC;
      if ((s->flags ^ out->flags) & EASEL_TEXT_UNDERLINE)
        *mixed |= EASEL_TEXT_FLAG_UNDERLINE;
      if ((s->flags ^ out->flags) & EASEL_TEXT_STRIKE)
        *mixed |= EASEL_TEXT_FLAG_STRIKE;
      if (s->color != out->color)
        *mixed |= EASEL_TEXT_COLOR;
    }
}

/* Makes a run start at `pos` (0 < pos < len) if none does; room for it is reserved by the caller. */
static void
split_at (EaselTextBuffer *b, size_t pos)
{
  size_t i;

  if (pos == 0 || pos >= b->len || b->n_runs == 0)
    return;
  i = run_at (b, pos);
  if (b->runs[i].start == pos)
    return;
  memmove (&b->runs[i + 2], &b->runs[i + 1], (b->n_runs - i - 1) * sizeof *b->runs);
  b->runs[i + 1] = (EaselTextRun) { pos, b->runs[i].style };
  b->n_runs++;
}

/* Drops the runs of no characters and joins equal neighbours. */
static void
normalize (EaselTextBuffer *b)
{
  size_t out = 0;

  for (size_t i = 0; i < b->n_runs; i++)
    {
      size_t end = i + 1 < b->n_runs ? b->runs[i + 1].start : b->len;

      if (b->runs[i].start >= end)
        continue;
      if (out > 0 && same_style (&b->runs[out - 1].style, &b->runs[i].style))
        continue;
      b->runs[out++] = b->runs[i];
    }
  b->n_runs = out;
  if (b->n_runs > 0)
    b->runs[0].start = 0;
}

/* Removes the bytes from `from` to `to` and their runs (room for two splits reserved by the caller). */
static void
remove_range (EaselTextBuffer *b, size_t from, size_t to)
{
  size_t gone = to - from, out = 0;

  split_at (b, from);
  split_at (b, to);
  for (size_t i = 0; i < b->n_runs; i++)
    {
      if (b->runs[i].start >= from && b->runs[i].start < to)
        continue;
      if (b->runs[i].start >= to)
        b->runs[i].start -= gone;
      b->runs[out++] = b->runs[i];
    }
  b->n_runs = out;
  memmove (b->text + from, b->text + to, b->len - to);
  b->len -= gone;
  normalize (b);
}

/* The bytes from `pos` to `pos + len` and their runs, copied; false without memory. */
static bool
capture (const EaselTextBuffer *b, size_t pos, size_t len, Piece *out)
{
  size_t first, n = 0;

  *out = (Piece) { 0 };
  if (len == 0)
    return true;
  out->text = malloc (len);
  first = run_at (b, pos);
  while (first + n < b->n_runs && b->runs[first + n].start < pos + len)
    n++;
  out->runs = malloc (n * sizeof *out->runs);
  if (out->text == NULL || out->runs == NULL)
    {
      piece_free (out);
      return false;
    }
  memcpy (out->text, b->text + pos, len);
  out->len = len;
  for (size_t k = 0; k < n; k++)
    {
      out->runs[k] = b->runs[first + k];
      out->runs[k].start = k == 0 ? 0 : out->runs[k].start - pos;
    }
  out->n = n;
  return true;
}

/* Puts `p` in at `pos` (room for the text and for p->n + 1 runs reserved by the caller). */
static void
put_piece (EaselTextBuffer *b, size_t pos, const Piece *p)
{
  size_t i = 0;

  if (p->len == 0)
    return;
  split_at (b, pos);
  memmove (b->text + pos + p->len, b->text + pos, b->len - pos);
  memcpy (b->text + pos, p->text, p->len);
  b->len += p->len;
  while (i < b->n_runs && b->runs[i].start < pos)
    i++;
  for (size_t k = i; k < b->n_runs; k++)
    b->runs[k].start += p->len;
  memmove (&b->runs[i + p->n], &b->runs[i], (b->n_runs - i) * sizeof *b->runs);
  for (size_t k = 0; k < p->n; k++)
    {
      b->runs[i + k] = p->runs[k];
      b->runs[i + k].start += pos;
    }
  b->n_runs += p->n;
  normalize (b);
}

/* Room to replace `old_len` bytes at some place with `p`: the text and the runs; false without memory. */
static bool
room_for (EaselTextBuffer *b, const Piece *p, size_t old_len)
{
  if (p->len > SIZE_MAX - (b->len - old_len) || p->n > SIZE_MAX - 4 - b->n_runs)
    return false;
  return reserve ((void **) &b->text, &b->cap, b->len - old_len + p->len, 1)
         && reserve ((void **) &b->runs, &b->runs_cap, b->n_runs + p->n + 4, sizeof *b->runs);
}

/* Room for the runs that the Undo and the Redo of an edit replacing `before_n` runs with `after_n` will need, taken when the edit is
   made, so that Undo and Redo never need memory (REQ-0017-08); the text needs none, as every length an Undo or Redo makes back has
   been held. False without memory. */
static bool
steps_room (EaselTextBuffer *b, size_t before_n, size_t after_n)
{
  if (before_n > SIZE_MAX / 4 || after_n > SIZE_MAX / 4 || b->n_runs > SIZE_MAX / 4)
    return false;
  return reserve ((void **) &b->runs, &b->runs_cap, b->n_runs + before_n + after_n + 8, sizeof *b->runs);
}

/* Replaces the `old_len` bytes at `pos` with `p` (room reserved). */
static void
replace (EaselTextBuffer *b, size_t pos, size_t old_len, const Piece *p)
{
  if (old_len > 0)
    remove_range (b, pos, pos + old_len);
  put_piece (b, pos, p);
}

/* Room for one more edit (the redo edits dropped first); false without memory. */
static bool
room_for_edit (EaselTextBuffer *b)
{
  return reserve ((void **) &b->edits, &b->edits_cap, b->n_done + 1, sizeof *b->edits);
}

/* The last edit, if a run of `kind` may still join it. */
static Edit *
joinable (EaselTextBuffer *b, EaselTextEditKind kind)
{
  if (!b->can_join || b->n_done == 0 || b->n_done != b->n_edits || b->edits[b->n_done - 1].kind != kind)
    return NULL;
  return &b->edits[b->n_done - 1];
}

/* `a` followed by `c`, as a new piece; false without memory. */
static bool
join_pieces (const Piece *a, const Piece *c, Piece *out)
{
  *out = (Piece) { 0 };
  if (a->len > SIZE_MAX - c->len)
    return false;
  out->text = malloc (a->len + c->len);
  out->runs = malloc ((a->n + c->n) * sizeof *out->runs);
  if (out->text == NULL || out->runs == NULL)
    {
      piece_free (out);
      return false;
    }
  memcpy (out->text, a->text, a->len);
  memcpy (out->text + a->len, c->text, c->len);
  memcpy (out->runs, a->runs, a->n * sizeof *out->runs);
  for (size_t k = 0; k < c->n; k++)
    {
      out->runs[a->n + k] = c->runs[k];
      out->runs[a->n + k].start += a->len;
    }
  out->len = a->len + c->len;
  out->n = a->n + c->n;
  return true;
}

/* Records a new edit (the redo edits dropped). The pieces are taken. */
/* The last edit records the caret and selection it left, which Redo gives back (REQ-0017-08). */
static void
set_after (EaselTextBuffer *b)
{
  b->edits[b->n_done - 1].anchor_after = b->anchor;
  b->edits[b->n_done - 1].caret_after = b->caret;
}

static void
push_edit (EaselTextBuffer *b, EaselTextEditKind kind, size_t pos, Piece *before, Piece *after, size_t anchor, size_t caret)
{
  drop_edits_from (b, b->n_done);
  b->edits[b->n_done] = (Edit) { kind, pos, *before, *after, anchor, caret, anchor, caret, { 0 }, { 0 } };
  b->n_done = b->n_edits = b->n_done + 1;
  *before = (Piece) { 0 };
  *after = (Piece) { 0 };
}

bool
easel_text_buffer_delete (EaselTextBuffer *b, size_t from, size_t to, EaselTextEditKind kind)
{
  Piece gone, joined = { 0 }, none = { 0 };
  Edit *last;

  if (to > b->len)
    to = b->len;
  if (from >= to)
    return true;
  if (!reserve ((void **) &b->runs, &b->runs_cap, b->n_runs + 2, sizeof *b->runs) || !room_for_edit (b) || !capture (b, from, to - from, &gone))
    return false;
  last = joinable (b, kind);
  /* A Backspace right before the previous one's place, or a Delete at the same place, joins its run (REQ-0017-08). */
  if (last != NULL && !((kind == EASEL_TEXT_EDIT_BACKSPACE && last->pos == to) || (kind == EASEL_TEXT_EDIT_DELETE && last->pos == from)))
    last = NULL;
  if (!steps_room (b, last != NULL ? last->before.n + gone.n : gone.n, 0))
    {
      piece_free (&gone);
      return false;
    }
  if (last != NULL)
    {
      bool ok = kind == EASEL_TEXT_EDIT_BACKSPACE ? join_pieces (&gone, &last->before, &joined) : join_pieces (&last->before, &gone, &joined);

      piece_free (&gone);
      if (!ok)
        return false;
      piece_free (&last->before);
      last->before = joined;
      last->pos = from;
    }
  else
    {
      push_edit (b, kind, from, &gone, &none, b->anchor, b->caret);
    }
  remove_range (b, from, to);
  b->anchor = b->caret = from;
  set_after (b);
  b->has_pending = false;
  b->can_join = kind == EASEL_TEXT_EDIT_BACKSPACE || kind == EASEL_TEXT_EDIT_DELETE;
  return true;
}

bool
easel_text_buffer_insert (EaselTextBuffer *b, const char *utf8, size_t length, const EaselTextStyle *style,
                          EaselTextEditKind kind)
{
  size_t from, to;
  EaselTextRun run = { 0, *style };
  Piece added = { (char *) utf8, length, &run, length > 0 ? 1 : 0 }, gone = { 0 }, copy = { 0 }, joined = { 0 };
  Edit *last;

  easel_text_buffer_selection (b, &from, &to);
  /* Everything that can fail first: the buffer's room, the edit's room and its copies. */
  if (!room_for (b, &added, to - from) || !room_for_edit (b) || !capture (b, from, to - from, &gone))
    return false;
  if (!join_pieces (&added, &(Piece) { 0 }, &copy))
    {
      piece_free (&gone);
      return false;
    }
  last = from == to ? joinable (b, EASEL_TEXT_EDIT_TYPING) : NULL;
  if (kind != EASEL_TEXT_EDIT_TYPING || (last != NULL && last->pos + last->after.len != from))
    last = NULL;
  if (!steps_room (b, last != NULL ? last->before.n : gone.n, last != NULL ? last->after.n + 1 : 1))
    {
      piece_free (&gone);
      piece_free (&copy);
      return false;
    }
  /* Typed characters right after the previous typed ones join their run (REQ-0017-08). */
  if (last != NULL)
    {
      if (!join_pieces (&last->after, &copy, &joined))
        {
          piece_free (&copy);
          return false;
        }
      piece_free (&copy);
      piece_free (&last->after);
      last->after = joined;
    }
  else
    push_edit (b, kind, from, &gone, &copy, b->anchor, b->caret);
  replace (b, from, to - from, &added);
  b->anchor = b->caret = from + length;
  set_after (b);
  b->has_pending = false;
  b->can_join = kind == EASEL_TEXT_EDIT_TYPING;
  return true;
}

bool
easel_text_buffer_insert_runs (EaselTextBuffer *b, const char *utf8, size_t length, const EaselTextRun *runs, size_t n,
                               EaselTextEditKind kind)
{
  size_t from, to;
  Piece added = { (char *) utf8, length, (EaselTextRun *) runs, length > 0 ? n : 0 }, gone = { 0 }, copy = { 0 };

  easel_text_buffer_selection (b, &from, &to);
  /* Everything that can fail first: the buffer's room, the edit's room, its copies and its Undo's and Redo's room (REQ-0017-08). */
  if (!room_for (b, &added, to - from) || !room_for_edit (b) || !capture (b, from, to - from, &gone))
    return false;
  if (!join_pieces (&added, &(Piece) { 0 }, &copy) || !steps_room (b, gone.n, added.n))
    {
      piece_free (&gone);
      piece_free (&copy);
      return false;
    }
  push_edit (b, kind, from, &gone, &copy, b->anchor, b->caret);
  replace (b, from, to - from, &added);
  b->anchor = b->caret = from + length;
  set_after (b);
  b->has_pending = false;
  b->can_join = false;
  return true;
}

bool
easel_text_buffer_push_geometry (EaselTextBuffer *b, const int old_rect[4], const int new_rect[4])
{
  Piece none = { 0 }, none2 = { 0 };

  if (!room_for_edit (b))
    return false;
  push_edit (b, EASEL_TEXT_EDIT_GEOMETRY, b->caret, &none, &none2, b->anchor, b->caret);
  memcpy (b->edits[b->n_done - 1].old_rect, old_rect, sizeof b->edits[0].old_rect);
  memcpy (b->edits[b->n_done - 1].new_rect, new_rect, sizeof b->edits[0].new_rect);
  b->can_join = false;
  return true;
}

bool
easel_text_buffer_can_undo (const EaselTextBuffer *b)
{
  return b->n_done > 0;
}

bool
easel_text_buffer_can_redo (const EaselTextBuffer *b)
{
  return b->n_done < b->n_edits;
}

bool
easel_text_buffer_undo (EaselTextBuffer *b, int rect[4], bool *geometry)
{
  Edit *e;

  if (b->n_done == 0)
    return false;
  e = &b->edits[b->n_done - 1];
  *geometry = e->kind == EASEL_TEXT_EDIT_GEOMETRY;
  if (*geometry)
    memcpy (rect, e->old_rect, sizeof e->old_rect);
  else
    {
      if (!room_for (b, &e->before, e->after.len))
        return false;
      replace (b, e->pos, e->after.len, &e->before);
      b->anchor = e->anchor_before;
      b->caret = e->caret_before;
    }
  b->n_done--;
  b->has_pending = false;
  b->can_join = false;
  return true;
}

bool
easel_text_buffer_redo (EaselTextBuffer *b, int rect[4], bool *geometry)
{
  Edit *e;

  if (b->n_done == b->n_edits)
    return false;
  e = &b->edits[b->n_done];
  *geometry = e->kind == EASEL_TEXT_EDIT_GEOMETRY;
  if (*geometry)
    memcpy (rect, e->new_rect, sizeof e->new_rect);
  else
    {
      if (!room_for (b, &e->after, e->before.len))
        return false;
      replace (b, e->pos, e->before.len, &e->after);
      b->anchor = e->anchor_after;
      b->caret = e->caret_after;
    }
  b->n_done++;
  b->has_pending = false;
  b->can_join = false;
  return true;
}

static void
set_field (EaselTextStyle *s, EaselTextField field, const EaselTextStyle *value)
{
  switch (field)
    {
    case EASEL_TEXT_FAMILY:
      s->family = value->family;
      break;
    case EASEL_TEXT_SIZE:
      s->size_pt = value->size_pt;
      break;
    case EASEL_TEXT_FLAG_BOLD:
      s->flags = (uint8_t) ((s->flags & ~EASEL_TEXT_BOLD) | (value->flags & EASEL_TEXT_BOLD));
      break;
    case EASEL_TEXT_FLAG_ITALIC:
      s->flags = (uint8_t) ((s->flags & ~EASEL_TEXT_ITALIC) | (value->flags & EASEL_TEXT_ITALIC));
      break;
    case EASEL_TEXT_FLAG_UNDERLINE:
      s->flags = (uint8_t) ((s->flags & ~EASEL_TEXT_UNDERLINE) | (value->flags & EASEL_TEXT_UNDERLINE));
      break;
    case EASEL_TEXT_FLAG_STRIKE:
      s->flags = (uint8_t) ((s->flags & ~EASEL_TEXT_STRIKE) | (value->flags & EASEL_TEXT_STRIKE));
      break;
    case EASEL_TEXT_COLOR:
      s->color = value->color;
      break;
    }
}

/* A size stepped by `delta`, clamped to min..max, so a size at the limit in the step's direction keeps it (REQ-0017-12,
   REQ-0017-25: Page Up from 497 gives 500, a step up from 500 gives 500). Every size of a box is within min..max (the card, the keys,
   the settings and the paste reader clamp what they give). */
static int
stepped (int size, int delta, int min, int max)
{
  return size + delta < min ? min : size + delta > max ? max : size + delta;
}

/* Changes the style of the selected text (from < to) as one FORMAT edit: `field` set to `value`, or, when `delta` is not 0, each part's
   size stepped by it within min..max. */
static bool
format_selection (EaselTextBuffer *b, size_t from, size_t to, EaselTextField field, const EaselTextStyle *value, int delta, int min,
                  int max)
{
  {
    Piece before, after;

    if (!reserve ((void **) &b->runs, &b->runs_cap, b->n_runs + 2, sizeof *b->runs) || !room_for_edit (b)
        || !capture (b, from, to - from, &before))
      return false;
    if (!steps_room (b, before.n, before.n))                /* the runs of the range after it are no more than before */
      {
        piece_free (&before);
        return false;
      }
    split_at (b, from);
    split_at (b, to);
    for (size_t i = 0; i < b->n_runs; i++)
      if (b->runs[i].start >= from && b->runs[i].start < to)
        {
          if (delta != 0)
            b->runs[i].style.size_pt = stepped (b->runs[i].style.size_pt, delta, min, max);
          else
            set_field (&b->runs[i].style, field, value);
        }
    normalize (b);
    if (!capture (b, from, to - from, &after))
      {
        /* Without the memory to record it, the change is taken back: the runs as they were (REQ-0017-08: no edit half done). */
        remove_range (b, from, to);
        put_piece (b, from, &before);
        piece_free (&before);
        return false;
      }
    push_edit (b, EASEL_TEXT_EDIT_FORMAT, from, &before, &after, b->anchor, b->caret);
  }
  b->can_join = false;
  return true;
}

bool
easel_text_buffer_apply (EaselTextBuffer *b, EaselTextField field, const EaselTextStyle *value, bool at_line_start)
{
  size_t from, to;

  if (!easel_text_buffer_selection (b, &from, &to))
    {
      EaselTextStyle s;

      easel_text_buffer_style_at_caret (b, at_line_start, &s);
      set_field (&s, field, value);
      b->pending = s;
      b->has_pending = true;
      return true;
    }
  return format_selection (b, from, to, field, value, 0, 0, 0);
}

bool
easel_text_buffer_step_size (EaselTextBuffer *b, int delta, int min, int max, bool at_line_start, bool *changed)
{
  size_t from, to;
  EaselTextStyle s;

  *changed = false;
  if (!easel_text_buffer_selection (b, &from, &to))
    {
      easel_text_buffer_style_at_caret (b, at_line_start, &s);
      if (stepped (s.size_pt, delta, min, max) == s.size_pt)
        return true;
      s.size_pt += delta;
      b->pending = s;
      b->has_pending = true;
      *changed = true;
      return true;
    }
  /* a step that changes no part's size is no edit */
  for (size_t i = run_at (b, from); i < b->n_runs && b->runs[i].start < to; i++)
    *changed |= stepped (b->runs[i].style.size_pt, delta, min, max) != b->runs[i].style.size_pt;
  if (!*changed)
    return true;
  if (!format_selection (b, from, to, EASEL_TEXT_SIZE, NULL, delta, min, max))
    {
      *changed = false;
      return false;
    }
  return true;
}
