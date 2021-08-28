/* la Bombolla GObject shell.
 * Copyright (C) 2021 Aleksandr Slobodeniuk
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
#include "bombolla/lba-log.h"
#include "bombolla/shell/commands/bombolla-commands.h"
#include <glib/gstdio.h>
#include <gmodule.h>
#include <string.h>
#include <stdlib.h>

enum
{
  SIGNAL_EXECUTE,
  LAST_SIGNAL
};

static guint lba_core_signals[LAST_SIGNAL] = { 0 };

typedef struct _LbaCore
{
  GObject parent;

  gchar *plugins_path;
  BombollaContext *ctx;
  
  gboolean started;
  GThread *mainloop_thr;
  GMainContext *mainctx;
  GMainLoop *mainloop;
} LbaCore;


typedef struct _LbaCoreClass
{
  GObjectClass parent;

  void (*execute) (LbaCore *, const gchar *);

} LbaCoreClass;


static gpointer
lba_core_mainloop (gpointer data)
{
  LbaCore *self = (LbaCore *) data;

  LBA_LOG ("starting the mainloop");

  /* FIXME: for the beginning we use default context */
//  self->mainctx = g_main_context_new ();
//  g_main_context_acquire (self->mainctx);
  self->mainloop = g_main_loop_new (NULL, TRUE);

  /* Proccessing events here until quit event arrives */
  g_main_loop_run (self->mainloop);
  self->started = FALSE;
  LBA_LOG ("mainloop stopped");
  return NULL;
}


/* Callback proccessed in the main loop.
 * Simply makes it quit. */
gboolean
lba_core_quit_msg (gpointer data)
{
  LbaCore *self = (LbaCore *) data;

  g_main_loop_quit (self->mainloop);
  return G_SOURCE_REMOVE;
}


static void
lba_core_stop (LbaCore * self)
{
  GSource *s;

  /* Send quit message to the main loop */
  s = g_idle_source_new ();
  g_source_set_priority (s, G_PRIORITY_HIGH);

  g_source_attach (s, NULL);
  g_source_set_callback (s, lba_core_quit_msg, self, NULL);

  /* Wait for main loop to quit */
  g_thread_join (self->mainloop_thr);
  g_source_destroy (s);
}


static void
lba_core_init (LbaCore * self)
{
}


static void
lba_core_dispose (GObject * gobject)
{
  LbaCore *self = (LbaCore *) gobject;

  /* Probably here objects may need to perform some preparations for
   * the destruction */

  if (self->started) {
    lba_core_stop (self);
  }

  /* And now destroy all the objects, and free all the data.*/
  g_free (self->plugins_path);
}



static gboolean
lba_core_proccess_command (GObject *obj, const gchar * str)
{
  gboolean ret = TRUE;
  char **tokens;
  const BombollaCommand *command;
  LbaCore *self = (LbaCore *)obj;

  tokens = g_strsplit (str, " ", 0);

  if (!tokens || !tokens[0]) {
    goto done;
  }

  LBA_LOG ("processing '%s'\n", str);

  if (!self->ctx) {
    self->ctx = g_new0 (BombollaContext, 1);
    /* ref ?? */
    self->ctx->self = (GObject *)self;
    self->ctx->proccess_command = NULL; // <---------- FIXME
  }

  for (command = commands; command->name != NULL; command ++)
  {
    /* FIXME: that's hackish */
    if (self->ctx->capturing_on_command) {
      if (!lba_command_on_append (self->ctx->capturing_on_command, str)) {
        self->ctx->capturing_on_command = NULL;
      }
    }
    
    if (0 == g_strcmp0 (command->name, tokens[0])) {
      if (!command->parse (self->ctx, tokens)) {
        /* Bad syntax. FIXME: should we stop proccessing?? */
        goto done;
      }
    }
  }
 
  g_warning ("unknown command");
  ret = TRUE;
done:
  g_strfreev (tokens);
  return ret;
}


static void
lba_core_dump_type (GType plugin_type)
{
  GTypeQuery query;

  g_type_query (plugin_type, &query);
  LBA_LOG ("GType name = '%s'\n", query.type_name);

  if (G_TYPE_IS_OBJECT (plugin_type)) {
    GObject *obj;
    GParamSpec **properties;
    guint n_properties, p;
    guint *signals, s, n_signals;

    LBA_LOG ("GType is a GObject.\n");

    obj = g_object_new (plugin_type, NULL);

    properties =
        g_object_class_list_properties (G_OBJECT_GET_CLASS (obj),
            &n_properties);

    for (p = 0; p < n_properties; p++) {
      GParamSpec *prop = properties[p];
      gchar *def_val;

      def_val =
          g_strdup_value_contents (g_param_spec_get_default_value (prop));

      LBA_LOG ("- Property: (%s) %s = %s\n",
          G_PARAM_SPEC_TYPE_NAME (prop),
          g_param_spec_get_name (prop), def_val);

      LBA_LOG ("\tnick = '%s', %s\n\n",
          g_param_spec_get_nick (prop), g_param_spec_get_blurb (prop));
      g_free (def_val);
    }


    /* Iterate signals */
    {
      GType t;
      const gchar *_tab = "  ";
      gchar *tab = g_strdup (_tab);
      LBA_LOG ("--- Signals:\n");
      for (t = plugin_type; t; t = g_type_parent (t)) {
        gchar *tmptab;

        signals = g_signal_list_ids (t, &n_signals);

        for (s = 0; s < n_signals; s++) {
          GSignalQuery query;
          GTypeQuery t_query;

          g_signal_query (signals[s], &query);
          g_type_query (t, &t_query);

          if (t_query.type == 0) {
            g_warning ("Error quering type");
            break;
          }

          LBA_LOG ("%s%s:: %s (* %s) ", tab, t_query.type_name,
              g_type_name (query.return_type), query.signal_name);

          LBA_LOG ("(");
          for (p = 0; p < query.n_params; p++) {
            LBA_LOG ("%s%s", p ? ", " : "",
                g_type_name (query.param_types[p]));
          }
          LBA_LOG (");\n");
        }

        tmptab = tab;
        tab = g_strdup_printf ("%s%s", _tab, tab);
        g_free (tmptab);
        g_free (signals);
      }
    }

    /* Iterate interfaces */
    {
      guint i;
      guint n_interfaces;
      GType *in = g_type_interfaces (plugin_type,
          &n_interfaces);

      for (i = 0; i < n_interfaces; i++) {

        LBA_LOG ("Provides interface %s\n", g_type_name (in[i]));
      }
      g_free (in);
    }

    g_object_unref (obj);
    g_free (properties);
  }
}


static void
lba_core_scan (LbaCore * self, const gchar *path)
{
  GModule *module = NULL;
  gpointer ptr;
  lBaPluginSystemGetGtypeFunc get_type_f;
  GSList *modules_files = NULL, *l;
  GDir *dir = NULL;

  if (path) {
    if (!g_file_test (path, G_FILE_TEST_EXISTS)) {
      g_warning ("Path doesn't exist\n");
      return;
    }

    if (g_file_test (path, G_FILE_TEST_IS_DIR)) {
      GError *err;
      const gchar *file;
      dir = g_dir_open (path, 0, &err);
      if (!dir) {
        g_warning ("%s", err->message);
        g_error_free (err);
      }

      while ((file = g_dir_read_name (dir))) {
        if (g_str_has_suffix (file, G_MODULE_SUFFIX)) {
          modules_files = g_slist_append (modules_files,
              g_build_filename (path, file, NULL));
        }
      }
    } else {
      modules_files = g_slist_append (modules_files, g_strdup (path));
    }
  }

  for (l = modules_files; l; l = l->next) {
    const gchar *module_filename = (const gchar *) l->data;

    module = g_module_open (module_filename, G_MODULE_BIND_LOCAL);
    if (!module) {
      // too noisy
      //      g_warning ("Failed to load plugin '%s': %s", module_filename,
      //          g_module_error ());
      continue;
    }

    if (!g_module_symbol (module, BOMBOLLA_PLUGIN_SYSTEM_ENTRY, &ptr)) {
      //      g_warning ("File '%s' is not a plugin from my system", module_filename);
      g_module_close (module);
      module = NULL;
      continue;
    }

    /* If module is from plugin system - we don't want to unload it */
    g_module_make_resident (module);
    module = NULL;

    get_type_f = (lBaPluginSystemGetGtypeFunc) ptr;

    /* This function does nothing important, only prints everything
     * it can about the GType it has. It could output something like a
     * dot graph actually, or so. */
    LBA_LOG ("======\nPlugin file %s:\n", module_filename);
    lba_core_dump_type (get_type_f ());
  }

  if (module)
    g_module_close (module);
  if (dir)
    g_dir_close (dir);
  g_slist_free_full (modules_files, g_free);
}


/* TODO: return FALSE if execution fails */
static void
lba_core_execute (LbaCore * self, const gchar *commands)
{
  if (!self->started) {
    /* Start main loop and scan the plugins */
    static volatile gboolean once;

    if (!once) {
      /* Scan for plugins */
      if (!self->plugins_path) {
        self->plugins_path = g_strdup (g_getenv ("LBA_PLUGINS_PATH"));

        if (self->plugins_path) {
          gchar *cur_dir = g_get_current_dir ();

          /* Is it really a proper default path ?? */
          self->plugins_path = g_build_filename (cur_dir, "build", "lib", NULL);
          g_free (cur_dir);
        }
        return;
      }

      LBA_LOG ("Scan %s", self->plugins_path);
      lba_core_scan (self, self->plugins_path);

      once = 1;
    }

    /* Start the main loop */
    /* FIXME: custom MainContext would be nice */
    self->mainloop_thr =
        g_thread_new ("LbaCoreMainLoop", lba_core_mainloop, self);

    /* Done */
    self->started = TRUE;
  }

  /* Proccessing commands */
  if (commands) {
    char **lines;
    int i;

    LBA_LOG ("Going to exec: [%s]", commands);
      
    lines = g_strsplit (commands, "\n", 0);

    for (i = 0; lines[i]; i++) {
      if (!lba_core_proccess_command ((GObject *)self, lines[i]))
        break;
    }
      
    g_strfreev (lines);
  }
}


static void
lba_core_class_init (LbaCoreClass * klass)
{
  GObjectClass *object_class = (GObjectClass *) klass;

  klass->execute = lba_core_execute;
  object_class->dispose = lba_core_dispose;

  /* TODO: change to bool_string, need some syntax checking */
  lba_core_signals[SIGNAL_EXECUTE] =
      g_signal_new ("execute", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
      G_STRUCT_OFFSET (LbaCoreClass, execute), NULL, NULL,
      g_cclosure_marshal_VOID__STRING,
          G_TYPE_NONE, 1, G_TYPE_STRING, G_TYPE_NONE);
}


G_DEFINE_TYPE (LbaCore, lba_core, G_TYPE_OBJECT)
/* Just for logging */
  BOMBOLLA_PLUGIN_SYSTEM_PROVIDE_GTYPE (lba_core);

