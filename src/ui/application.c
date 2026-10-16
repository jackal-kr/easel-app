/* SPDX-License-Identifier: GPL-3.0-or-later */

#include <adwaita.h>

#include "config.h"
#include "application.h"
#include "window.h"

/* GApplication is unique per app id, so a second launch only activates the running instance (REQ-0001-07). */
static void
on_activate (GApplication *app)
{
  GtkWindow *window = gtk_application_get_active_window (GTK_APPLICATION (app));

  if (window == NULL)
    window = GTK_WINDOW (easel_window_new (GTK_APPLICATION (app)));
  gtk_window_present (window);
}

/* The first file on the command line is the start-up picture; the others are ignored. If a window exists already, it is
   only presented, as a second launch always was (REQ-0001-07, REQ-0005-15). */
static void
on_open (GApplication *app, GFile **files, int n_files, const char *hint)
{
  GtkWindow *window = gtk_application_get_active_window (GTK_APPLICATION (app));

  if (window == NULL && n_files > 0)
    window = GTK_WINDOW (easel_window_new_with_file (GTK_APPLICATION (app), files[0]));
  if (window == NULL)
    {
      g_application_activate (app);
      return;
    }
  gtk_window_present (window);
}

/* Quit asks each window to close, so unsaved changes are prompted for (REQ-0001-34, REQ-0001-35). */
static void
quit_activated (GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
  for (GList *l = gtk_application_get_windows (GTK_APPLICATION (user_data)); l != NULL; l = l->next)
    gtk_window_close (l->data);
}

static const GActionEntry app_actions[] = {
  { .name = "quit", .activate = quit_activated },
};

GtkApplication *
easel_application_new (void)
{
  AdwApplication *app = adw_application_new (APP_ID, G_APPLICATION_HANDLES_OPEN);

  g_signal_connect (app, "activate", G_CALLBACK (on_activate), NULL);
  g_signal_connect (app, "open", G_CALLBACK (on_open), NULL);
  gtk_application_set_accels_for_action (GTK_APPLICATION (app), "win.new", (const char *[]) { "<Control>n", NULL });
  gtk_application_set_accels_for_action (GTK_APPLICATION (app), "win.open", (const char *[]) { "<Control>o", NULL });
  gtk_application_set_accels_for_action (GTK_APPLICATION (app), "win.save", (const char *[]) { "<Control>s", NULL });
  gtk_application_set_accels_for_action (GTK_APPLICATION (app), "win.save-as",
                                         (const char *[]) { "<Control><Shift>s", NULL });
  gtk_application_set_accels_for_action (GTK_APPLICATION (app), "win.undo", (const char *[]) { "<Control>z", NULL });
  gtk_application_set_accels_for_action (GTK_APPLICATION (app), "win.redo",
                                         (const char *[]) { "<Control>y", "<Control><Shift>z", NULL });
  gtk_application_set_accels_for_action (GTK_APPLICATION (app), "win.swap-colors",
                                         (const char *[]) { "<Control><Shift>x", NULL });
  gtk_application_set_accels_for_action (GTK_APPLICATION (app), "win.zoom-in",
                                         (const char *[]) { "<Control>equal", "<Control>plus", "<Control>KP_Add", NULL });
  gtk_application_set_accels_for_action (GTK_APPLICATION (app), "win.zoom-out",
                                         (const char *[]) { "<Control>minus", "<Control>KP_Subtract", NULL });
  gtk_application_set_accels_for_action (GTK_APPLICATION (app), "win.preset-larger", (const char *[]) { "KP_Add", NULL });
  gtk_application_set_accels_for_action (GTK_APPLICATION (app), "win.preset-smaller", (const char *[]) { "KP_Subtract", NULL });
  gtk_application_set_accels_for_action (GTK_APPLICATION (app), "win.zoom-normal", (const char *[]) { "<Control>Page_Up", NULL });
  gtk_application_set_accels_for_action (GTK_APPLICATION (app), "win.zoom-large", (const char *[]) { "<Control>Page_Down", NULL });
  gtk_application_set_accels_for_action (GTK_APPLICATION (app), "win.zoom-fit", (const char *[]) { "<Control>0", NULL });
  /* The grid and Attributes actions come with their own tasks; their shortcuts are registered with the menus. */
  gtk_application_set_accels_for_action (GTK_APPLICATION (app), "win.grid", (const char *[]) { "<Control>g", NULL });
  gtk_application_set_accels_for_action (GTK_APPLICATION (app), "win.attributes", (const char *[]) { "<Control>e", NULL });
  gtk_application_set_accels_for_action (GTK_APPLICATION (app), "win.close", (const char *[]) { "<Control>w", NULL });
  gtk_application_set_accels_for_action (GTK_APPLICATION (app), "app.quit", (const char *[]) { "<Control>q", NULL });
  g_action_map_add_action_entries (G_ACTION_MAP (app), app_actions, G_N_ELEMENTS (app_actions), app);
  return GTK_APPLICATION (app);
}
