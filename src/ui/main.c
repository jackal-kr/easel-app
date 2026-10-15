/* SPDX-License-Identifier: GPL-3.0-or-later */

#include "application.h"

int
main (int argc, char **argv)
{
  g_autoptr (GtkApplication) app = easel_application_new ();

  return g_application_run (G_APPLICATION (app), argc, argv);
}
