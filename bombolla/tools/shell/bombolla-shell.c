/* la Bombolla GObject shell
 *
 * Copyright (c) 2024, Alexander Slobodeniuk <aleksandr.slobodeniuk@gmx.es>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * - Redistributions of source code must retain the above copyright notice, this
 *   list of conditions and the following disclaimer.
 *
 * - Redistributions in binary form must reproduce the above copyright notice, this
 *   list of conditions and the following disclaimer in the documentation and/or
 *   other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "bombolla/lba-plugin-system.h"
#include <glib/gstdio.h>
#include <gmodule.h>
#include <string.h>
#include <stdlib.h>
#include <bmixin/bmixin.h>

/* Declare this magic symbol explicitly */
GType lba_core_object_get_type (void);

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
      g_error_free (err);
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
  core = g_object_new (lba_core_object_get_type (), NULL);

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
