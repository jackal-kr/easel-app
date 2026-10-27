/* SPDX-License-Identifier: GPL-3.0-or-later */
/* The text of an open text box (feature 0017): its UTF-8 characters, the style of each (as runs), the caret and the selection.
   Offsets are byte offsets on character boundaries. Pure C, no allocation fails silently: a function that allocates returns false
   with the buffer unchanged when the memory cannot be had. */
#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

enum
{
  EASEL_TEXT_BOLD = 1 << 0,
  EASEL_TEXT_ITALIC = 1 << 1,
  EASEL_TEXT_UNDERLINE = 1 << 2,
  EASEL_TEXT_STRIKE = 1 << 3,
};

/* A character's style (REQ-0017-12). `family` is interned by the buffer (easel_text_buffer_intern), so two styles have the same
   family exactly when the pointers are equal. */
typedef struct
{
  const char *family;
  int size_pt;
  uint8_t flags;
  uint32_t color;
} EaselTextStyle;

/* The fields of a style, for a change of one of them and for the fields that differ within a selection. */
typedef enum
{
  EASEL_TEXT_FAMILY = 1 << 0,
  EASEL_TEXT_SIZE = 1 << 1,
  EASEL_TEXT_FLAG_BOLD = 1 << 2,
  EASEL_TEXT_FLAG_ITALIC = 1 << 3,
  EASEL_TEXT_FLAG_UNDERLINE = 1 << 4,
  EASEL_TEXT_FLAG_STRIKE = 1 << 5,
  EASEL_TEXT_COLOR = 1 << 6,
} EaselTextField;

/* A run: the characters from `start` to the next run's start (or the end of the text) have `style`. Adjacent runs never have equal
   styles, and there is no run of no characters. */
typedef struct
{
  size_t start;
  EaselTextStyle style;
} EaselTextRun;

/* The kinds of edit of a box (REQ-0017-08): a run of typed characters, a run of Backspace or of Delete, a paste, a cut, a format
   change of selected text, a finished move or resize of the box (its rectangle, which the buffer keeps for Undo but does not use). */
typedef enum
{
  EASEL_TEXT_EDIT_TYPING,
  EASEL_TEXT_EDIT_BACKSPACE,
  EASEL_TEXT_EDIT_DELETE,
  EASEL_TEXT_EDIT_PASTE,
  EASEL_TEXT_EDIT_CUT,
  EASEL_TEXT_EDIT_FORMAT,
  EASEL_TEXT_EDIT_GEOMETRY,
} EaselTextEditKind;

typedef struct EaselTextBuffer EaselTextBuffer;

/* An empty buffer whose starting style (the style of an empty box, REQ-0017-26) is `start` (its family interned). NULL without memory. */
EaselTextBuffer *easel_text_buffer_new (const EaselTextStyle *start);
void easel_text_buffer_free (EaselTextBuffer *b);

/* A copy of the text, its runs, the caret, the selection and a pending style, with no edits to undo: the box tries an edit on it first
   and makes it only if the result can be shown and fits the picture (REQ-0017-07). NULL without memory. */
EaselTextBuffer *easel_text_buffer_copy (const EaselTextBuffer *b);

/* The buffer's own copy of `family`, the same pointer for the same name; NULL without memory. */
const char *easel_text_buffer_intern (EaselTextBuffer *b, const char *family);

const char *easel_text_buffer_text (const EaselTextBuffer *b, size_t *length);
const EaselTextRun *easel_text_buffer_runs (const EaselTextBuffer *b, size_t *n);
/* The memory held for the text (bytes) and for its runs (entries), not only their lengths (tests, REQ-0017-08). */
void easel_text_buffer_capacity (const EaselTextBuffer *b, size_t *text, size_t *runs);

/* The caret and the selection: the selection is from the anchor to the caret (either order), empty when they are equal. Setting
   them forgets a pending style (REQ-0017-12: a change with nothing selected applies to the next typed text until the caret moves). */
size_t easel_text_buffer_caret (const EaselTextBuffer *b);
void easel_text_buffer_select (EaselTextBuffer *b, size_t anchor, size_t caret);
bool easel_text_buffer_selection (const EaselTextBuffer *b, size_t *from, size_t *to);

/* The style at the caret (REQ-0017-26): a pending style if there is one; else the style of the character before the caret, or of the
   character after it when `at_line_start` (the caller knows where its lines start) or when there is none before; in an empty buffer the
   starting style. */
void easel_text_buffer_style_at_caret (const EaselTextBuffer *b, bool at_line_start, EaselTextStyle *out);

/* The style of the selected text, and in `mixed` the fields that differ within it (REQ-0017-14: shown empty or off). With nothing
   selected, the style at the caret and no mixed field. */
void easel_text_buffer_selection_style (const EaselTextBuffer *b, bool at_line_start, EaselTextStyle *out, unsigned *mixed);

/* Inserts `utf8` (length bytes, valid UTF-8) at the caret in `style`, replacing the selected text; the caret after it. One edit of
   `kind` (TYPING, PASTE): typed characters right after the previous typed ones, with the caret not moved in between, join its run. */
bool easel_text_buffer_insert (EaselTextBuffer *b, const char *utf8, size_t length, const EaselTextStyle *style,
                               EaselTextEditKind kind);

/* Inserts `length` bytes of `utf8` at the caret, replacing the selected text, with the n runs given (starts relative to the text, the
   first at 0; families interned in b), as one edit of `kind` (a paste, REQ-0017-20); the caret after it. False, the buffer unchanged,
   without memory. */
bool easel_text_buffer_insert_runs (EaselTextBuffer *b, const char *utf8, size_t length, const EaselTextRun *runs, size_t n,
                                    EaselTextEditKind kind);

/* Removes the characters from `from` to `to`; the caret at `from`. One edit of `kind` (BACKSPACE, DELETE, CUT, or TYPING when typing
   replaces a selection): a Backspace right before the previous one's place, or a Delete at the same place, joins its run. */
bool easel_text_buffer_delete (EaselTextBuffer *b, size_t from, size_t to, EaselTextEditKind kind);

/* A finished move or resize of the box, from `old_rect` to `new_rect` (x, y, width, set height), as one edit. */
bool easel_text_buffer_push_geometry (EaselTextBuffer *b, const int old_rect[4], const int new_rect[4]);

/* Undo and Redo of the box's edits (REQ-0017-08). Each returns false when there is none; for a geometry edit `rect` gets the box's
   rectangle to take (the old one on Undo, the new one on Redo) and `*geometry` is true. A new edit drops the edits that could be
   redone. */
bool easel_text_buffer_can_undo (const EaselTextBuffer *b);
bool easel_text_buffer_can_redo (const EaselTextBuffer *b);
bool easel_text_buffer_undo (EaselTextBuffer *b, int rect[4], bool *geometry);
bool easel_text_buffer_redo (EaselTextBuffer *b, int rect[4], bool *geometry);

/* Sets one field (`field`, one of EaselTextField) to the value in `value` for the selected text, as one FORMAT edit; with nothing
   selected, makes the pending style the style at the caret with that field changed, which is no edit (REQ-0017-08). */
bool easel_text_buffer_apply (EaselTextBuffer *b, EaselTextField field, const EaselTextStyle *value, bool at_line_start);

/* Steps the size of each part of the selected text by `delta` points from its own size, as one FORMAT edit, clamped to min..max; a
   part already at the limit in the step's direction keeps its size. With nothing selected, the pending style is the style at the caret with its size stepped (no edit). A step
   that changes no size does nothing, `*changed` false (REQ-0017-12). False, the buffer unchanged, without memory. */
bool easel_text_buffer_step_size (EaselTextBuffer *b, int delta, int min, int max, bool at_line_start, bool *changed);
