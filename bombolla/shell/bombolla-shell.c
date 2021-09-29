/* la Bombolla GObject shell.
 * Copyright (C) 2020 Aleksandr Slobodeniuk
 *
 *   This file is part of bombolla.
 *
 *   Bombolla is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   Bombolla is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with bombolla.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "bombolla/lba-plugin-system.h"
#include <glib/gstdio.h>
#include <gmodule.h>
#include <string.h>
#include <stdlib.h>

/* Declare this magic symbol explicitly */
GType lba_core_get_type (void);

int
main (int argc, char **argv) {
  gchar *script_contents = NULL;
  GObject *core;

  /* Parse arguments */
  {
    GOptionContext *ctx;
    GError *err = NULL;
    gchar *path = NULL;
    gchar *script = NULL;

    GOptionEntry options[] = {
      { "script", 'i', 0, G_OPTION_ARG_STRING, &script,
       "Proccess input file before entering shell", NULL },
      { "path", 'p', 0, G_OPTION_ARG_STRING, &path,
       "Path to the plugins directory", NULL },
      { NULL }
    };

    ctx = g_option_context_new ("-i <script file> | -p <plugins path or file>");
    g_option_context_add_main_entries (ctx, options, NULL);
    if (!g_option_context_parse (ctx, &argc, &argv, &err)) {
      g_print ("Error initializing: %s\n", err->message);
      exit (1);
    }
    g_option_context_free (ctx);

    if (path) {
      g_setenv ("LBA_PLUGINS_PATH", path, TRUE);
      g_free (path);
      path = NULL;
    }

    if (script) {
      gsize length = 0;

      g_printf ("Opening script %s\n", script);

      if (!g_file_test (script, G_FILE_TEST_EXISTS)) {
        g_warning ("Script file doesn't exist\n");
        return 1;
      }

      if (g_file_get_contents (script, &script_contents, &length, NULL)
          && length) {
        script_contents[length - 1] = 0;
      }

      g_free (script);
    }
  }

  /* Create core object, and execute the script */
  core = g_object_new (lba_core_get_type (), NULL);

  if (!core) {
    g_critical ("fatal: can't create core");
    exit (1);
  }

  if (script_contents) {
    g_signal_emit_by_name (core, "execute", script_contents);
    g_free (script_contents);
    script_contents = NULL;
  }

  /* Now just wait on command line */

  /* FIXME: to property */
  g_printf ("scanning finished, entering shell...\n"
            "commands:\n------------\n"
            "create <type name> <var name>\n"
            "destroy <var name>\n"
            "set <var name>.<property> <value>\n"
            "call <var name>.<signal> (params are not supported yet)\n"
            "on <var name>.<signal>\n" "q (quit)\n------------\n");

  for (;;) {
    char a[512];

    /* Stdin input. */
    g_printf ("\n\t# ");
    if (NULL == fgets (a, sizeof (a), stdin)) {
      break;
    }
    a[strlen (a) - 1] = 0;

    if (a[0] == 'q') {
      break;
    }

    g_signal_emit_by_name (core, "execute", a);
  }

  g_object_unref (core);
  return 0;
}
