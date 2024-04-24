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
#include "bombolla/lba-log.h"
#include "bombolla/base/lba-module-scanner.h"
#include "commands/lba-commands.h"
#include <bmixin/bmixin.h>
#include <glib/gstdio.h>
#include <gmodule.h>
#include <string.h>
#include <stdlib.h>

enum {
  SIGNAL_EXECUTE,
  LAST_SIGNAL
};

static guint lba_core_signals[LAST_SIGNAL] = { 0 };

typedef struct _LbaCore {
  BMixinInstance i;
  BombollaContext *ctx;
  /* FIXME: to BMixin */
  GObject *papi;
  /* ---------------- */

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
  BMixinClass c;
  void (*execute) (GObject *, const gchar *);
} LbaCoreClass;

BM_DEFINE_MIXIN (lba_core, LbaCore, BM_ADD_DEP (lba_module_scanner));

/* HACK: Needed to use LBA_LOG */
static const gchar *global_lba_plugin_name = "LbaCore";

static gpointer
lba_core_mainloop (gpointer data) {
  LbaCore *self = (LbaCore *) data;

  LBA_LOG ("starting the mainloop");

  /* FIXME: for the beginning we use default context */
//  self->mainctx = g_main_context_new ();
//  g_main_context_acquire (self->mainctx);

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
lba_core_init (GObject * object, LbaCore * self) {
  self->papi = object;

  g_mutex_init (&self->async_cmd_guard);
  g_mutex_init (&self->lock);
  g_cond_init (&self->cond);

  if (!self->started) {
    /* Start the main loop */
    g_mutex_lock (&self->lock);
    /* FIXME: custom MainContext would be nice */
    self->mainloop = g_main_loop_new (NULL, TRUE);
    self->mainloop_thr = g_thread_new ("LbaCoreMainLoop", lba_core_mainloop, self);
    while (!g_main_loop_is_running (self->mainloop))
      g_usleep (1);             // FIXME: wait properly
    /* Done */
    self->started = TRUE;
    g_mutex_unlock (&self->lock);
  }
}

void lba_core_sync_with_async_cmds (gpointer core);

static void
lba_core_dispose (GObject * gobject) {
  LbaCore *self = bm_get_LbaCore (gobject);

  if (self->async_cmds) {
    lba_core_sync_with_async_cmds (self);
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

  if (self->started) {
    lba_core_stop (self);
  }

  BM_CHAINUP (gobject, lba_core, GObject)->dispose (gobject);
}

static void
lba_core_finalize (GObject * gobject) {
  LbaCore *self = bm_get_LbaCore (gobject);

  g_mutex_clear (&self->async_cmd_guard);
  g_mutex_clear (&self->lock);
  g_cond_clear (&self->cond);

  BM_CHAINUP (gobject, lba_core, GObject)->finalize (gobject);
}

static gboolean
lba_core_proccess_line (gpointer obj, const gchar * str) {
  LbaCore *self = (LbaCore *) obj;
  gboolean ret = TRUE;
  char **tokens;
  const BombollaCommand *command;

  tokens = g_strsplit (str, " ", 0);

  if (!tokens || !tokens[0]) {
    goto done;
  }

  if (!self->ctx || !self->ctx->capturing_on_command) {
    LBA_LOG ("processing '%s'", str);
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
lba_core_load_module (GObject * gobj, const gchar * module_filename) {
  GModule *module = NULL;
  gpointer ptr;
  lBaPluginSystemGetGtypeFunc get_type_f;
  GType plugin_gtype;

  g_return_if_fail (module_filename);

  module = g_module_open (module_filename, G_MODULE_BIND_LOCAL);
  if (!module) {
    LBA_LOG ("Failed to load plugin '%s': %s", module_filename, g_module_error ());
    return;
  }

  if (!g_module_symbol (module, BOMBOLLA_PLUGIN_SYSTEM_ENTRY, &ptr)) {
    LBA_LOG ("File '%s' is not a bombolla plugin", module_filename);
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

/* TODO: return FALSE if execution fails */
static void
lba_core_execute (GObject * gobject, const gchar * commands) {
  LbaCore *self = bm_get_LbaCore (gobject);

  /* Proccessing commands */
  if (commands) {
    char **lines;
    int i;

    LBA_LOG ("Going to exec: [%s]", commands);

    lines = g_strsplit (commands, "\n", 0);

    for (i = 0; lines[i]; i++) {
      if (!lba_core_proccess_line (self, lines[i]))
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

  lba_core_execute (ctx->core->papi, ctx->command);

  return G_SOURCE_REMOVE;
}

void
lba_core_sync_with_async_cmds (gpointer core) {
  LbaCore *self = (LbaCore *) core;
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

G_LOCK_DEFINE_STATIC (singleton_lock);
static GObject *singleton_object;

static void
lba_core_reset_singleton (gpointer data, GObject * where_the_object_was) {
  G_LOCK (singleton_lock);
  singleton_object = NULL;
  G_UNLOCK (singleton_lock);
}

static GObject *
lba_core_constructor (GType type, guint n_cp, GObjectConstructParam * cp) {
  GObjectClass *chain_up =
      (GObjectClass *) g_type_class_peek_parent (g_type_class_peek (type));

  if (G_UNLIKELY (n_cp != 0 && singleton_object != NULL))
    g_warning ("Will ignore the construct properties, this object is a singleton!");

  G_LOCK (singleton_lock);
  if (singleton_object == NULL) {
    singleton_object = chain_up->constructor (type, n_cp, cp);
    g_object_weak_ref (singleton_object, lba_core_reset_singleton, NULL);
  } else
    g_object_ref (singleton_object);
  G_UNLOCK (singleton_lock);

  return singleton_object;
}

static void
lba_core_class_init (GObjectClass * object_class, LbaCoreClass * klass) {
  LbaModuleScannerClass *lms_class;

  object_class->dispose = lba_core_dispose;
  object_class->finalize = lba_core_finalize;
  object_class->constructor = lba_core_constructor;

  klass->execute = lba_core_execute;

  /* TODO: change to bool_string, need some syntax checking */
  lba_core_signals[SIGNAL_EXECUTE] =
      g_signal_new ("execute", G_TYPE_FROM_CLASS (object_class),
                    G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                    BM_CLASS_VFUNC_OFFSET (klass, execute),
                    NULL, NULL,
                    g_cclosure_marshal_VOID__STRING,
                    G_TYPE_NONE, 1, G_TYPE_STRING, G_TYPE_NONE);

  lba_core_init_convertion_functions ();

  lms_class =
      (LbaModuleScannerClass *) bm_class_get_mixin (object_class,
                                                    lba_module_scanner_get_type ());

  lms_class->plugin_path_env = "LBA_PLUGINS_PATH";
  lms_class->plugin_prefix = "liblba-";
  lms_class->plugin_suffix = G_MODULE_SUFFIX;
  lms_class->have_file = lba_core_load_module;
}

/* GObject entry point */
GType
lba_core_object_get_type (void) {
  static GType ret;

  return ret ? ret : (ret =
                      bm_register_mixed_type ("LbaCoreObject", G_TYPE_OBJECT,
                                              lba_core_get_type ()));
}
