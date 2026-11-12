/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "config.h"
#include "help.h"
#include "i18n.h"

/* Every key of the app, with the action that holds it or NULL (REQ-0021-02); a key with two meanings has a row for each. */
static const EaselShortcut shortcuts[] = {
  { N_ ("File"), N_ ("New"), "<Control>n", "win.new" },
  { N_ ("File"), N_ ("Open…"), "<Control>o", "win.open" },
  { N_ ("File"), N_ ("Save"), "<Control>s", "win.save" },
  { N_ ("File"), N_ ("Save As…"), "<Control><Shift>s", "win.save-as" },
  { N_ ("File"), N_ ("Print…"), "<Control>p", "win.print" },
  { N_ ("File"), N_ ("Preferences"), "<Control>comma", "win.preferences" },
  { N_ ("File"), N_ ("Keyboard Shortcuts"), "<Control>question <Control>F1", "win.shortcuts" },
  { N_ ("File"), N_ ("Help"), "F1", "win.help" },
  { N_ ("File"), N_ ("Close the window"), "<Control>w", "win.close" },
  { N_ ("File"), N_ ("Quit"), "<Control>q", "app.quit" },

  { N_ ("Edit"), N_ ("Undo"), "<Control>z", "win.undo" },
  { N_ ("Edit"), N_ ("Redo"), "<Control>y <Control><Shift>z", "win.redo" },
  { N_ ("Edit"), N_ ("Cut"), "<Control>x", "win.cut" },
  { N_ ("Edit"), N_ ("Copy"), "<Control>c", "win.copy" },
  { N_ ("Edit"), N_ ("Paste"), "<Control>v", "win.paste" },
  { N_ ("Edit"), N_ ("Select All"), "<Control>a", "win.select-all" },
  { N_ ("Edit"), N_ ("Invert Selection"), "<Control><Shift>j", "win.invert-selection" },
  { N_ ("Edit"), N_ ("Clear the selection"), "Delete", NULL },
  { N_ ("Edit"), N_ ("Move the selection 1 pixel"), "Left Right Up Down", NULL },
  { N_ ("Edit"), N_ ("Smear the selection 1 pixel"), "<Shift>Left <Shift>Right <Shift>Up <Shift>Down", NULL },
  { N_ ("Edit"), N_ ("Move a copy of the selection 10 pixels"), "<Control><Shift>Left <Control><Shift>Right <Control><Shift>Up <Control><Shift>Down", NULL },
  { N_ ("Edit"), N_ ("Put down a selection or a waiting shape"), "Return", NULL },
  { N_ ("Edit"), N_ ("Put down a selection, drop a waiting shape, cancel a drag"), "Escape", NULL },

  { N_ ("Image"), N_ ("Rotate 90°"), "<Control>r", "win.rotate-90" },
  { N_ ("Image"), N_ ("Rotate 270°"), "<Control>l", "win.rotate-270" },
  { N_ ("Image"), N_ ("Stretch and Skew…"), "<Control>t", "win.stretch-skew" },
  { N_ ("Image"), N_ ("Crop to Selection"), "<Control><Shift>k", "win.crop-to-selection" },
  { N_ ("Image"), N_ ("Invert Colors"), "<Control>i", NULL },
  { N_ ("Image"), N_ ("Attributes…"), "<Control>e", "win.attributes" },
  { N_ ("Image"), N_ ("Clear Image"), "<Control><Shift>n", "win.clear-image" },

  { N_ ("View"), N_ ("Zoom In"), "<Control>equal <Control>plus <Control>KP_Add", "win.zoom-in" },
  { N_ ("View"), N_ ("Zoom Out"), "<Control>minus <Control>KP_Subtract", "win.zoom-out" },
  { N_ ("View"), N_ ("Normal Size"), "<Control>Page_Up", "win.zoom-normal" },
  { N_ ("View"), N_ ("Large Size"), "<Control>Page_Down", "win.zoom-large" },
  { N_ ("View"), N_ ("Fit to Window"), "<Control>0", "win.zoom-fit" },
  { N_ ("View"), N_ ("Show Grid"), "<Control>g", "win.grid" },
  { N_ ("View"), N_ ("Close the thumbnail"), "Escape", NULL },
  { N_ ("View"), N_ ("Move the focus"), "Tab <Shift>Tab", NULL },
  { N_ ("View"), N_ ("Move to the next or the previous panel"), "F6 <Shift>F6", NULL },

  { N_ ("Tools"), N_ ("The next larger size of the tool"), "KP_Add", "win.preset-larger" },
  { N_ ("Tools"), N_ ("The next smaller size of the tool"), "KP_Subtract", "win.preset-smaller" },
  { N_ ("Tools"), N_ ("Move the focus in a panel"), "Left Right Up Down", NULL },
  { N_ ("Tools"), N_ ("Choose the focused control"), "space Return", NULL },
  { N_ ("Tools"), N_ ("Press or release at the marker"), "space", NULL },
  { N_ ("Tools"), N_ ("Press with the secondary button at the marker"), "<Shift>space", NULL },
  { N_ ("Tools"), N_ ("Press for a copy of a selection"), "<Control>space", NULL },
  { N_ ("Tools"), N_ ("Press for a smear of a selection"), "<Control><Shift>space", NULL },
  { N_ ("Tools"), N_ ("Move the marker 1 pixel"), "Left Right Up Down", NULL },
  { N_ ("Tools"), N_ ("Move the marker 10 pixels"), "<Shift>Left <Shift>Right <Shift>Up <Shift>Down", NULL },
  { N_ ("Tools"), N_ ("Move the marker alone, with a selection"), "<Alt>Left <Alt>Right <Alt>Up <Alt>Down", NULL },
  { N_ ("Tools"), N_ ("Open the menu at the marker"), "Menu <Shift>F10", NULL },
  { N_ ("Tools"), N_ ("Say the marker's place and color"), "F7", NULL },

  { N_ ("Colors"), N_ ("Swap foreground and background"), "<Control><Shift>x", "win.swap-colors" },
  { N_ ("Colors"), N_ ("Edit the focused swatch"), "F2", NULL },
  { N_ ("Colors"), N_ ("Move among the swatches"), "Left Right Up Down", NULL },
  { N_ ("Colors"), N_ ("Make the focused swatch the foreground color"), "space Return", NULL },
  { N_ ("Colors"), N_ ("Make the focused swatch the background color"), "<Shift>space <Shift>Return", NULL },

  { N_ ("Text"), N_ ("Bold"), "<Control>b", NULL },
  { N_ ("Text"), N_ ("Italic"), "<Control>i", NULL },
  { N_ ("Text"), N_ ("Underline"), "<Control>u", NULL },
  { N_ ("Text"), N_ ("Strikethrough"), "<Control><Shift>5", NULL },
  { N_ ("Text"), N_ ("Font size 1 point larger or smaller"), "<Control>bracketright <Control>bracketleft", NULL },
  { N_ ("Text"), N_ ("Open the font family list"), "<Control><Shift>f", NULL },
  { N_ ("Text"), N_ ("Move the highlight in the family list"), "Up Down", NULL },
  { N_ ("Text"), N_ ("Choose the highlighted family"), "Return", NULL },
  { N_ ("Text"), N_ ("Close the family list"), "Escape", NULL },
  { N_ ("Text"), N_ ("New line"), "Return", NULL },
  { N_ ("Text"), N_ ("Delete before or after the caret"), "BackSpace Delete", NULL },
  { N_ ("Text"), N_ ("Move the caret"), "Left Right Up Down", NULL },
  { N_ ("Text"), N_ ("Move the caret by a word"), "<Control>Left <Control>Right", NULL },
  { N_ ("Text"), N_ ("To the start or the end of the line"), "Home End", NULL },
  { N_ ("Text"), N_ ("Extend the text selection"), "<Shift>Left <Shift>Right <Shift>Up <Shift>Down <Shift>Home <Shift>End", NULL },
  { N_ ("Text"), N_ ("Cut the selected text"), "<Control>x", "win.cut" },
  { N_ ("Text"), N_ ("Copy the selected text"), "<Control>c", "win.copy" },
  { N_ ("Text"), N_ ("Paste into the box"), "<Control>v", "win.paste" },
  { N_ ("Text"), N_ ("Paste as Plain Text"), "<Control><Shift>v", "win.paste-plain" },
  { N_ ("Text"), N_ ("Select all the box's text"), "<Control>a", "win.select-all" },
  { N_ ("Text"), N_ ("Put the box down"), "Escape", NULL },
  { N_ ("Text"), N_ ("Move the box 1 pixel"), "<Alt>Left <Alt>Right <Alt>Up <Alt>Down", NULL },
  { N_ ("Text"), N_ ("Move the box's far corner 1 pixel"), "<Alt><Shift>Left <Alt><Shift>Right <Alt><Shift>Up <Alt><Shift>Down", NULL },
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
          section = adw_shortcuts_section_new (_ (group));
          adw_shortcuts_dialog_add (dialog, section);
        }
      adw_shortcuts_section_add (section, adw_shortcuts_item_new (_ (shortcuts[i].title), shortcuts[i].accel));
    }
  return ADW_DIALOG (dialog);
}

char *
easel_help_page_for (const char *language)
{
  g_autofree char *program = g_file_read_link ("/proc/self/exe", NULL);
  g_autofree char *folder = program != NULL ? g_path_get_dirname (program) : g_strdup (".");
  g_autofree char *base = g_path_get_dirname (HELP_PAGE);
  g_autofree char *build_page = g_strdup_printf ("help-%s.html", language);
  g_autofree char *in_build = g_build_filename (folder, "..", "data", build_page, NULL);
  g_autofree char *installed = g_build_filename (base, language, "index.html", NULL);
  g_autofree char *english = g_build_filename (folder, "..", "data", "help-index.html", NULL);

  /* An uninstalled build: its own copy, beside the program's folder, however it was started. A language whose page is missing opens the English one (REQ-0023-09). */
  if (g_file_test (in_build, G_FILE_TEST_IS_REGULAR))
    return g_steal_pointer (&in_build);
  if (strcmp (language, "en") != 0 && g_file_test (installed, G_FILE_TEST_IS_REGULAR))
    return g_steal_pointer (&installed);
  return g_strdup (g_file_test (english, G_FILE_TEST_IS_REGULAR) ? english : HELP_PAGE);
}

const char *
easel_help_page_path (void)
{
  static char *path;
  const char *set = g_getenv ("EASEL_HELP_PAGE");

  if (set != NULL)
    return set;
  g_free (path);
  path = easel_help_page_for (easel_i18n_language ());
  return path;
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
  adw_about_dialog_set_developer_name (dialog, "JOD");                                   /* REQ-0021-05 (JOD, 2026-11-11) */
  adw_about_dialog_set_website (dialog, "https://github.com/jackal-kr/easel-app");     /* the metainfo's homepage */
  adw_about_dialog_set_license_type (dialog, GTK_LICENSE_GPL_3_0);
  adw_about_dialog_add_acknowledgement_section (dialog, NULL, people);
  return ADW_DIALOG (dialog);
}
