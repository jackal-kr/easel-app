/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "config.h"
#include "help.h"

/* Every key of the app, with the action that holds it or NULL (REQ-0021-02); a key with two meanings has a row for each. */
static const EaselShortcut shortcuts[] = {
  { "File", "New", "<Control>n", "win.new" },
  { "File", "Open…", "<Control>o", "win.open" },
  { "File", "Save", "<Control>s", "win.save" },
  { "File", "Save As…", "<Control><Shift>s", "win.save-as" },
  { "File", "Print…", "<Control>p", "win.print" },
  { "File", "Preferences", "<Control>comma", "win.preferences" },
  { "File", "Keyboard Shortcuts", "<Control>question <Control>F1", "win.shortcuts" },
  { "File", "Help", "F1", "win.help" },
  { "File", "Close the window", "<Control>w", "win.close" },
  { "File", "Quit", "<Control>q", "app.quit" },

  { "Edit", "Undo", "<Control>z", "win.undo" },
  { "Edit", "Redo", "<Control>y <Control><Shift>z", "win.redo" },
  { "Edit", "Cut", "<Control>x", "win.cut" },
  { "Edit", "Copy", "<Control>c", "win.copy" },
  { "Edit", "Paste", "<Control>v", "win.paste" },
  { "Edit", "Select All", "<Control>a", "win.select-all" },
  { "Edit", "Clear the selection", "Delete", NULL },
  { "Edit", "Move the selection 1 pixel", "Left Right Up Down", NULL },
  { "Edit", "Move the selection 10 pixels", "<Shift>Left <Shift>Right <Shift>Up <Shift>Down", NULL },
  { "Edit", "Put down a selection or a waiting shape", "Return", NULL },
  { "Edit", "Put down a selection, drop a waiting shape, cancel a drag", "Escape", NULL },

  { "Image", "Rotate 90°", "<Control>r", "win.rotate-90" },
  { "Image", "Rotate 270°", "<Control>l", "win.rotate-270" },
  { "Image", "Stretch and Skew…", "<Control>t", "win.stretch-skew" },
  { "Image", "Invert Colors", "<Control>i", NULL },
  { "Image", "Attributes…", "<Control>e", "win.attributes" },
  { "Image", "Clear Image", "<Control><Shift>n", "win.clear-image" },

  { "View", "Zoom In", "<Control>equal <Control>plus <Control>KP_Add", "win.zoom-in" },
  { "View", "Zoom Out", "<Control>minus <Control>KP_Subtract", "win.zoom-out" },
  { "View", "Normal Size", "<Control>Page_Up", "win.zoom-normal" },
  { "View", "Large Size", "<Control>Page_Down", "win.zoom-large" },
  { "View", "Fit to Window", "<Control>0", "win.zoom-fit" },
  { "View", "Show Grid", "<Control>g", "win.grid" },
  { "View", "Close the thumbnail", "Escape", NULL },
  { "View", "Move the focus", "Tab <Shift>Tab", NULL },

  { "Tools", "The next larger size of the tool", "KP_Add", "win.preset-larger" },
  { "Tools", "The next smaller size of the tool", "KP_Subtract", "win.preset-smaller" },

  { "Colors", "Swap foreground and background", "<Control><Shift>x", "win.swap-colors" },
  { "Colors", "Edit the focused swatch", "F2", NULL },

  { "Text", "Bold", "<Control>b", NULL },
  { "Text", "Italic", "<Control>i", NULL },
  { "Text", "Underline", "<Control>u", NULL },
  { "Text", "Strikethrough", "<Control><Shift>5", NULL },
  { "Text", "Font size 1 point larger or smaller", "<Control>bracketright <Control>bracketleft", NULL },
  { "Text", "Open the font family list", "<Control><Shift>f", NULL },
  { "Text", "Move the highlight in the family list", "Up Down", NULL },
  { "Text", "Choose the highlighted family", "Return", NULL },
  { "Text", "Close the family list", "Escape", NULL },
  { "Text", "New line", "Return", NULL },
  { "Text", "Delete before or after the caret", "BackSpace Delete", NULL },
  { "Text", "Move the caret", "Left Right Up Down", NULL },
  { "Text", "Move the caret by a word", "<Control>Left <Control>Right", NULL },
  { "Text", "To the start or the end of the line", "Home End", NULL },
  { "Text", "Extend the text selection", "<Shift>Left <Shift>Right <Shift>Up <Shift>Down <Shift>Home <Shift>End", NULL },
  { "Text", "Cut the selected text", "<Control>x", "win.cut" },
  { "Text", "Copy the selected text", "<Control>c", "win.copy" },
  { "Text", "Paste into the box", "<Control>v", "win.paste" },
  { "Text", "Paste as Plain Text", "<Control><Shift>v", "win.paste-plain" },
  { "Text", "Select all the box's text", "<Control>a", "win.select-all" },
  { "Text", "Put the box down", "Escape", NULL },
};

static const char *const acknowledgements[] = { "Microsoft Paint in Windows XP", "JS Paint by Isaiah Odhner" };

const EaselShortcut *
easel_shortcuts_table (gsize *n)
{
  *n = G_N_ELEMENTS (shortcuts);
  return shortcuts;
}

AdwDialog *
easel_shortcuts_dialog_new (void)
{
  AdwShortcutsDialog *dialog = ADW_SHORTCUTS_DIALOG (adw_shortcuts_dialog_new ());
  AdwShortcutsSection *section = NULL;
  const char *group = NULL;

  for (gsize i = 0; i < G_N_ELEMENTS (shortcuts); i++)
    {
      if (g_strcmp0 (group, shortcuts[i].group) != 0)
        {
          group = shortcuts[i].group;
          section = adw_shortcuts_section_new (group);
          adw_shortcuts_dialog_add (dialog, section);
        }
      adw_shortcuts_section_add (section, adw_shortcuts_item_new (shortcuts[i].title, shortcuts[i].accel));
    }
  return ADW_DIALOG (dialog);
}

const char *
easel_help_page_path (void)
{
  static char *in_build;
  const char *set = g_getenv ("EASEL_HELP_PAGE");

  if (set != NULL)
    return set;
  /* An uninstalled build: its own copy, beside the program's folder, however the program was started. */
  if (in_build == NULL)
    {
      g_autofree char *program = g_file_read_link ("/proc/self/exe", NULL);
      g_autofree char *folder = program != NULL ? g_path_get_dirname (program) : g_strdup (".");

      in_build = g_build_filename (folder, "..", "data", "help-index.html", NULL);
    }
  return g_file_test (in_build, G_FILE_TEST_IS_REGULAR) ? in_build : HELP_PAGE;
}

const char *
easel_build_version (void)
{
  return VERSION;
}

gsize
easel_about_n_acknowledgements (void)
{
  return G_N_ELEMENTS (acknowledgements);
}

const char *
easel_about_acknowledgement (gsize i)
{
  return acknowledgements[i];
}

AdwDialog *
easel_about_dialog_new (void)
{
  AdwAboutDialog *dialog = ADW_ABOUT_DIALOG (adw_about_dialog_new ());
  const char *people[G_N_ELEMENTS (acknowledgements) + 1] = { NULL };

  for (gsize i = 0; i < G_N_ELEMENTS (acknowledgements); i++)
    people[i] = acknowledgements[i];
  adw_about_dialog_set_application_name (dialog, "Easel");
  adw_about_dialog_set_application_icon (dialog, APP_ID);
  adw_about_dialog_set_version (dialog, VERSION);
  adw_about_dialog_set_license_type (dialog, GTK_LICENSE_GPL_3_0);
  adw_about_dialog_add_acknowledgement_section (dialog, NULL, people);
  return ADW_DIALOG (dialog);
}
