/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Languages (0023): the interface's language for this run, and its strings. */
#pragma once

#include <glib.h>

G_BEGIN_DECLS

/* Chooses the language by the settings file, or EASEL_TEST_LANGUAGE in a test, and reads its catalog. Once, before any window class is made: a change takes effect
   at the next start (REQ-0023-05). */
void easel_i18n_init (void);
/* The language of this run. */
const char *easel_i18n_language (void);
/* The language setting and the system's list make: REQ-0023-02, -03. */
const char *easel_i18n_choose (const char *setting);
/* Where this run reads its compiled translations from. */
char *easel_i18n_catalog_path (const char *language);
/* A template with its marked strings in this language: malloc'd, or NULL with no memory. */
char *easel_i18n_translate_template (const char *xml);

/* A string in the interface's language (REQ-0023-02). */
const char *_ (const char *msgid) G_GNUC_FORMAT (1);
/* The form of a message for n (REQ-0023-06). */
const char *_n (const char *msgid, const char *msgid_plural, unsigned long n) G_GNUC_FORMAT (1) G_GNUC_FORMAT (2);
/* Marks a string for translation where it is only stored; _ reads it later. */
#define N_(s) (s)

G_END_DECLS
