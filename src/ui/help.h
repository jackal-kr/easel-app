/* SPDX-License-Identifier: GPL-3.0-or-later */
#pragma once

#include <adwaita.h>

G_BEGIN_DECLS

/* One row of Keyboard Shortcuts (REQ-0021-02): accel is one key or several, apart by spaces; action is the action that holds them, NULL for a key
   no action holds. */
typedef struct
{
  const char *group;
  const char *title;
  const char *accel;
  const char *action;
} EaselShortcut;

/* The rows, in the dialog's order. */
const EaselShortcut *easel_shortcuts_table (gsize *n);

AdwDialog *easel_shortcuts_dialog_new (void);
AdwDialog *easel_about_dialog_new (void);

/* Where the Help page is: EASEL_HELP_PAGE if set (the tests), else a build tree's own copy, else where it was installed. */
const char *easel_help_page_path (void);
/* Where a language's page is: its own, else the English one (REQ-0023-09). */
char *easel_help_page_for (const char *language);

/* The build's version, and the About dialog's acknowledgements (REQ-0021-05). */
const char *easel_build_version (void);
gsize easel_about_n_acknowledgements (void);
const char *easel_about_acknowledgement (gsize i);

G_END_DECLS
