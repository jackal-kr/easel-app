/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Languages (0023): a compiled translation read directly, with each language's plural rule, so that a chosen language needs no system locale
   (REQ-0023-05). */
#pragma once

#include <stddef.h>

typedef struct _EaselCatalog EaselCatalog;

/* The compiled file of len bytes, kept by the caller while the catalog lives. NULL for what is no such file, or with no memory. */
EaselCatalog *easel_catalog_new (const void *mo, size_t len);
void easel_catalog_free (EaselCatalog *cat);

/* The translation of msgid, or msgid itself when it lacks one (REQ-0023-02); a NULL catalog is English. */
const char *easel_catalog_get (const EaselCatalog *cat, const char *msgid);
/* The form of the message for n in lang (REQ-0023-06), or the source by English's rule. */
const char *easel_catalog_get_plural (const EaselCatalog *cat, const char *msgid, const char *msgid_plural, unsigned long n, const char *lang);

/* Which plural form lang takes for n, by REQ-0023-06; a language with no rule of its own takes English's. */
int easel_plural_form (const char *lang, unsigned long n);

/* The text of a template with each element marked translatable="yes" in its translation, escaped, and the mark gone. malloc'd; NULL with no memory. */
char *easel_catalog_translate_ui (const EaselCatalog *cat, const char *xml);

/* The twelve languages, by code (REQ-0023-01), and each one's own name (REQ-0023-04). */
int easel_language_count (void);
const char *easel_language_code (int i);
const char *easel_language_name (int i);
/* Whether code is one of them, exactly. */
int easel_language_known (const char *code);
/* The language to show: the setting unless it is "auto", else the first of names, the system's preferred list ending in NULL, that is one of the twelve by its
   language part, else English (REQ-0023-02, -03). */
const char *easel_language_resolve (const char *setting, const char *const *names);
