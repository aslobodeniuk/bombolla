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
#include "bombolla/core/bombolla-commands.h"
#include <glib/gstdio.h>
#include <gmodule.h>
#include <string.h>
#include <stdlib.h>

enum {
  SIGNAL_EXECUTE,
  LAST_SIGNAL
};

typedef enum {
  PROP_PLUGINS_PATH = 1
} LbaCoreProperty;

static guint lba_core_signals[LAST_SIGNAL] = { 0 };

typedef struct _LbaCore {
  GObject parent;

  gchar *plugins_path;
  BombollaContext *ctx;

  gboolean started;
  GThread *mainloop_thr;
  GMainContext *mainctx;
  GMainLoop *mainloop;
  GMutex lock;
  GCond cond;

  GMutex async_cmd_guard;
  GList *async_cmds;
} LbaCore;

typedef struct _LbaCoreClass {
  GObjectClass parent;

  void (*execute) (LbaCore *, const gchar *);

} LbaCoreClass;

G_DEFINE_TYPE (LbaCore, lba_core, G_TYPE_OBJECT);

/* HACK: Needed to use LBA_LOG */
static const gchar *global_lba_plugin_name = "LbaCore";

static gpointer
lba_core_mainloop (gpointer data) {
  LbaCore *self = (LbaCore *) data;

  LBA_LOG ("starting the mainloop");

  /* FIXME: for the beginning we use default context */
//  self->mainctx = g_main_context_new ();
//  g_main_context_acquire (self->mainctx);
  self->mainloop = g_main_loop_new (NULL, TRUE);

  g_mutex_lock (&self->lock);
  g_cond_broadcast (&self->cond);
  g_mutex_unlock (&self->lock);

  /* Proccessing events here until quit event arrives */
  g_main_loop_run (self->mainloop);
  self->started = FALSE;
  LBA_LOG ("mainloop stopped");
  return NULL;
}

/* Callback proccessed in the main loop.
 * Simply makes it quit. */
gboolean
lba_core_quit_msg (gpointer data) {
  LbaCore *self = (LbaCore *) data;

  g_main_loop_quit (self->mainloop);
  return G_SOURCE_REMOVE;
}

static void
lba_core_stop (LbaCore * self) {
  GSource *s;

  /* Send quit message to the main loop */
  s = g_idle_source_new ();
  g_source_set_priority (s, G_PRIORITY_HIGH);

  g_source_set_callback (s, lba_core_quit_msg, self, NULL);
  g_source_attach (s, NULL);

  /* Wait for main loop to quit */
  g_thread_join (self->mainloop_thr);
  self->mainloop_thr = NULL;

  g_source_destroy (s);
  g_source_unref (s);

  g_main_loop_unref (self->mainloop);
  self->mainloop = NULL;
  self->started = FALSE;
}

static void
lba_core_init (LbaCore * self) {
  g_mutex_init (&self->async_cmd_guard);
  g_mutex_init (&self->lock);
  g_cond_init (&self->cond);
}

void lba_core_sync_with_async_cmds (GObject * obj);

static void
lba_core_dispose (GObject * gobject) {
  LbaCore *self = (LbaCore *) gobject;

  /* Probably here objects may need to perform some preparations for
   * the destruction */

  if (self->async_cmds) {
    lba_core_sync_with_async_cmds (gobject);
  }

  if (self->started) {
    lba_core_stop (self);
  }

  if (self->ctx) {
    if (self->ctx->bindings) {
      // The bindings actually belong to the object, and are
      // automatically destroyed when the objects are destroyed
      g_hash_table_unref (self->ctx->bindings);
    }
    if (self->ctx->objects) {
      g_hash_table_remove_all (self->ctx->objects);
      g_hash_table_unref (self->ctx->objects);
    }
    g_free (self->ctx);
    self->ctx = NULL;
  }

  g_free (self->plugins_path);
  self->plugins_path = NULL;

  /* Always chain up */
  G_OBJECT_CLASS (lba_core_parent_class)->dispose (gobject);
}

static void
lba_core_finalize (GObject * gobject) {
  LbaCore *self = (LbaCore *) gobject;

  g_mutex_clear (&self->async_cmd_guard);
  g_mutex_clear (&self->lock);
  g_cond_clear (&self->cond);

  G_OBJECT_CLASS (lba_core_parent_class)->finalize (gobject);
}

static gboolean
lba_core_proccess_line (GObject * obj, const gchar * str) {
  gboolean ret = TRUE;
  char **tokens;
  const BombollaCommand *command;
  LbaCore *self = (LbaCore *) obj;

  tokens = g_strsplit (str, " ", 0);

  if (!tokens || !tokens[0]) {
    goto done;
  }

  if (!self->ctx || !self->ctx->capturing_on_command) {
    LBA_LOG ("processing '%s'\n", str);
  }

  if (!self->ctx) {
    self->ctx = g_new0 (BombollaContext, 1);
    self->ctx->self = (GObject *) self;
    self->ctx->proccess_command = lba_core_proccess_line;
    self->ctx->objects =
        g_hash_table_new_full (g_str_hash, g_str_equal, g_free, g_object_unref);

    // The bindings actually belong to the object, and are
    // automatically destroyed when the objects are destroyed.
    // The only point of storing them is the "unbind" command
    self->ctx->bindings =
        g_hash_table_new_full (g_str_hash, g_str_equal, g_free, NULL);
  }

  /* FIXME: that's hackish */
  if (self->ctx->capturing_on_command) {
    if (!lba_command_on_append (self->ctx->capturing_on_command, str)) {
      self->ctx->capturing_on_command = NULL;
    }
    goto done;
  }

  for (command = commands; command->name != NULL; command++) {
    if (0 == g_strcmp0 (command->name, tokens[0])) {
      if (!command->parse (self->ctx, tokens)) {
        /* Bad syntax. FIXME: should we optionally stop proccessing?? */
        goto done;
      }

      ret = TRUE;
      goto done;
    }
  }

  g_warning ("Unknown command [%s]", tokens[0]);
done:
  g_strfreev (tokens);
  return ret;
}

static void
lba_core_scan_gtype (LbaCore * self, const gchar * module_filename) {
  GModule *module = NULL;
  gpointer ptr;
  lBaPluginSystemGetGtypeFunc get_type_f;
  GType plugin_gtype;

  module = g_module_open (module_filename, G_MODULE_BIND_LOCAL);
  if (!module) {
    // too noisy
    //      g_warning ("Failed to load plugin '%s': %s", module_filename,
    //          g_module_error ());
    return;
  }

  if (!g_module_symbol (module, BOMBOLLA_PLUGIN_SYSTEM_ENTRY, &ptr)) {
    //      g_warning ("File '%s' is not a plugin from my system", module_filename);
    g_module_close (module);
    return;
  }

  /* If module is from plugin system - we don't want to unload it */
  g_module_make_resident (module);

  get_type_f = (lBaPluginSystemGetGtypeFunc) ptr;
  plugin_gtype = get_type_f ();

  /* This function does nothing important, only prints everything
   * it can about the GType it has. It could output something like a
   * dot graph actually, or so. */
  LBA_LOG ("Found plugin: type = [%s] file = [%s]", g_type_name (plugin_gtype),
           module_filename);
}

static GSList *lba_core_scan_path (const gchar * path, GSList * modules_files);

/* This function is recursive */
static GSList *
lba_core_scan_path (const gchar * path, GSList * modules_files) {
  static const char *LBA_PLUGIN_PREFIX = "liblba-";
  static const char *LBA_PLUGIN_SUFFIX = G_MODULE_SUFFIX;

  if (!g_file_test (path, G_FILE_TEST_EXISTS)) {
    g_warning ("Path [%s] doesn't exist", path);
    return modules_files;
  }

  if (g_file_test (path, G_FILE_TEST_IS_DIR)) {
    GError *err;
    const gchar *file;
    GDir *dir;

    LBA_LOG ("Scan directory [%s]", path);
    dir = g_dir_open (path, 0, &err);
    if (!dir) {
      g_warning ("Couldn't open [%s]: [%s]", path,
                 err ? err->message : "no message");
      if (err)
        g_error_free (err);
      return modules_files;
    }

    while ((file = g_dir_read_name (dir))) {
      gchar *sub_path = g_build_filename (path, file, NULL);

      /* Step into the directories */
      if (g_file_test (sub_path, G_FILE_TEST_IS_DIR)) {
        modules_files = lba_core_scan_path (sub_path, modules_files);
        g_free (sub_path);
        continue;
      }

      /* If not a directory */
      if (g_str_has_prefix (file, LBA_PLUGIN_PREFIX) &&
          g_str_has_suffix (file, LBA_PLUGIN_SUFFIX)) {
        modules_files = g_slist_append (modules_files, sub_path);
        sub_path = NULL;
      } else {
        g_free (sub_path);
      }
    }

    g_dir_close (dir);
  } else {
    LBA_LOG ("Scan single file [%s]", path);
    modules_files = g_slist_append (modules_files, g_strdup (path));
  }

  return modules_files;
}

static void
lba_core_scan (LbaCore * self) {
  GSList *modules_files = NULL,
      *l;

  g_return_if_fail (self->plugins_path != NULL);

  modules_files = lba_core_scan_path (self->plugins_path, NULL);

  for (l = modules_files; l; l = l->next) {
    lba_core_scan_gtype (self, (const gchar *)l->data);
  }

  g_slist_free_full (modules_files, g_free);
}

/* TODO: return FALSE if execution fails */
static void
lba_core_execute (LbaCore * self, const gchar * commands) {
  if (!self->started) {
    /* Start main loop and scan the plugins */
    static volatile gboolean once;

    if (!once) {
      /* Scan for plugins */
      if (!self->plugins_path) {
        self->plugins_path = g_strdup (g_getenv ("LBA_PLUGINS_PATH"));

        if (!self->plugins_path) {
          gchar *cur_dir = g_get_current_dir ();

          LBA_LOG ("No plugin path is set.");
          self->plugins_path = g_build_filename (cur_dir, "build", NULL);
          g_free (cur_dir);
        }
      }

      LBA_LOG ("Scan %s", self->plugins_path);
      lba_core_scan (self);

      once = 1;
    }

    /* Start the main loop */
    g_mutex_lock (&self->lock);
    /* FIXME: custom MainContext would be nice */
    self->mainloop_thr = g_thread_new ("LbaCoreMainLoop", lba_core_mainloop, self);
    g_cond_wait (&self->cond, &self->lock);
    /* Done */
    self->started = TRUE;
    g_mutex_unlock (&self->lock);
  }

  /* Proccessing commands */
  if (commands) {
    char **lines;
    int i;

    LBA_LOG ("Going to exec: [%s]", commands);

    lines = g_strsplit (commands, "\n", 0);

    for (i = 0; lines[i]; i++) {
      if (!lba_core_proccess_line ((GObject *) self, lines[i]))
        break;
    }

    g_strfreev (lines);
  }
}

typedef struct _LbaCoreAsyncCmd {
  gchar *command;
  LbaCore *core;
  GSource *source;

  GMutex lock;
  GCond cond;
  gboolean done;
} LbaCoreAsyncCmd;

static void
lba_core_async_cmd_free (gpointer data) {
  LbaCoreAsyncCmd *ctx = (LbaCoreAsyncCmd *) data;

  g_free (ctx->command);
  g_source_unref (ctx->source);
  g_mutex_clear (&ctx->lock);
  g_cond_clear (&ctx->cond);
  g_free (ctx);
}

static void
lba_core_async_cmd_done (gpointer data) {
  LbaCoreAsyncCmd *ctx = (LbaCoreAsyncCmd *) data;

  g_mutex_lock (&ctx->lock);
  ctx->done = TRUE;
  g_cond_broadcast (&ctx->cond);
  g_mutex_unlock (&ctx->lock);
}

static gboolean
lba_core_async_cmd (gpointer data) {
  LbaCoreAsyncCmd *ctx = (LbaCoreAsyncCmd *) data;

  lba_core_execute (ctx->core, ctx->command);

  return G_SOURCE_REMOVE;
}

void
lba_core_sync_with_async_cmds (GObject * obj) {
  LbaCore *self = (LbaCore *) obj;
  GList *it;

  /* To sync we do:
   * 1. copy a snap of the list of the async commands.
   * 2. iterate on this snap waiting for each. */

  g_mutex_lock (&self->async_cmd_guard);
  for (it = self->async_cmds; it != NULL; it = it->next) {
    LbaCoreAsyncCmd *ctx = (LbaCoreAsyncCmd *) it->data;

    g_mutex_lock (&ctx->lock);
    while (!ctx->done) {
      g_cond_wait (&ctx->cond, &ctx->lock);
    }
    g_mutex_unlock (&ctx->lock);
  }
  g_list_free_full (self->async_cmds, lba_core_async_cmd_free);
  self->async_cmds = NULL;
  g_mutex_unlock (&self->async_cmd_guard);
}

void
lba_core_shedule_async_script (GObject * obj, gchar * command) {
  LbaCore *self = (LbaCore *) obj;
  LbaCoreAsyncCmd *ctx = g_new0 (LbaCoreAsyncCmd, 1);

  LBA_LOG ("Shedulling command [%s] for async execution", command);

  ctx->core = self;
  ctx->command = command;
  ctx->source = g_idle_source_new ();
  g_mutex_init (&ctx->lock);
  g_cond_init (&ctx->cond);

  g_source_set_priority (ctx->source, G_PRIORITY_DEFAULT);

  g_source_set_callback (ctx->source, lba_core_async_cmd, ctx,
                         lba_core_async_cmd_done);

  self->async_cmds = g_list_append (self->async_cmds, ctx);
  g_source_attach (ctx->source, NULL);
}

static void
lba_core_set_property (GObject * object,
                       guint property_id, const GValue * value, GParamSpec * pspec) {
  LbaCore *self = (LbaCore *) object;

  switch ((LbaCoreProperty) property_id) {
  case PROP_PLUGINS_PATH:
    g_free (self->plugins_path);
    self->plugins_path = g_value_dup_string (value);
    break;
  default:
    /* We don't have any other property... */
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}

static void
lba_core_get_property (GObject * object,
                       guint property_id, GValue * value, GParamSpec * pspec) {
  LbaCore *self = (LbaCore *) object;

  switch ((LbaCoreProperty) property_id) {
  case PROP_PLUGINS_PATH:
    g_value_set_string (value, self->plugins_path);
    break;
  default:
    /* We don't have any other property... */
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}

static void
lba_core_class_init (LbaCoreClass * klass) {
  GObjectClass *object_class = (GObjectClass *) klass;

  klass->execute = lba_core_execute;
  object_class->dispose = lba_core_dispose;
  object_class->finalize = lba_core_finalize;
  object_class->set_property = lba_core_set_property;
  object_class->get_property = lba_core_get_property;

  /* TODO: change to bool_string, need some syntax checking */
  lba_core_signals[SIGNAL_EXECUTE] =
      g_signal_new ("execute", G_TYPE_FROM_CLASS (klass),
                    G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                    G_STRUCT_OFFSET (LbaCoreClass, execute), NULL, NULL,
                    g_cclosure_marshal_VOID__STRING,
                    G_TYPE_NONE, 1, G_TYPE_STRING, G_TYPE_NONE);

  g_object_class_install_property (object_class, PROP_PLUGINS_PATH,
                                   g_param_spec_string ("plugins-path",
                                                        "Plugins path",
                                                        "Path to scan the plugins",
                                                        NULL,
                                                        G_PARAM_STATIC_STRINGS |
                                                        G_PARAM_READWRITE));

  lba_core_init_convertion_functions ();
}
