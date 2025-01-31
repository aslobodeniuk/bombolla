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
#include <gjs/gjs.h>
#include "bombolla/base/lba-module-scanner.h"
#include <bmixin/bmixin.h>

typedef struct _LbaGjs LbaGjs;

/* TODO: split into mixins: PluginSystem + Async + Gjs */
struct _LbaGjs {
  BMixinInstance i;

  GjsContext *js_context;

  GMutex lock;
  GCond cond;
  GSource *async_ctx;
  const gchar *module_to_scan;
};

typedef struct _LbaGjsClass {
  BMixinClass c;
} LbaGjsClass;

BM_DEFINE_MIXIN (lba_gjs, LbaGjs, BM_ADD_DEP (lba_module_scanner));

static void
lba_gjs_async_cmd_free (gpointer data) {
  LbaGjs *self = (LbaGjs *) data;

  g_mutex_lock (&self->lock);
  g_source_unref (self->async_ctx);
  self->async_ctx = NULL;
  g_cond_broadcast (&self->cond);
  g_mutex_unlock (&self->lock);
}

static void
lba_gjs_sync_call_through_main_loop (LbaGjs *self, GSourceFunc cmd) {
  g_warn_if_fail (self->async_ctx == NULL);

  self->async_ctx = g_idle_source_new ();
  g_source_set_priority (self->async_ctx, G_PRIORITY_HIGH);
  g_source_set_callback (self->async_ctx, cmd, self, lba_gjs_async_cmd_free);

  /* Attach the source and wait for it to finish */
  g_mutex_lock (&self->lock);
  g_source_attach (self->async_ctx, NULL);
  g_cond_wait (&self->cond, &self->lock);
  g_mutex_unlock (&self->lock);
}

static gboolean
lba_gjs_async_eval_file (gpointer data) {
  LbaGjs *self = (LbaGjs *) data;
  GError *error = NULL;
  gint exit_status;

  g_return_val_if_fail (self->module_to_scan, FALSE);

  /* NOTE: for some reason gjs doesn't crash only if we use it through the main loop.
   * Same with the GTypes we register in the js we run. Properties and signals can
   * only be accessed from the main loop. Otherwise gjs crashes */

  LBA_LOG ("Evaluating file [%s]", self->module_to_scan);

  if (!self->js_context) {
    self->js_context = GJS_CONTEXT (g_object_new (GJS_TYPE_CONTEXT, NULL));

    if (!self->js_context) {
      g_critical ("Couldn't create context");
      return G_SOURCE_REMOVE;
    }
  }

  if (G_UNLIKELY (!gjs_context_eval_file (self->js_context, self->module_to_scan,
                                          &exit_status, &error))) {
    g_critical ("GJS error [%d], [%s]", exit_status, error->message);
    g_clear_error (&error);
  }

  /* IDEA: maybe we should create a dynamic class here that wraps all the classes
   * registered by js ?? Like right after the evaluation of this file we just
   * search for some class that would correlate with a part of the file name,
   * add same signals and properties there, and proxy to the js instance through
   * the main loop. */

  return G_SOURCE_REMOVE;
}

static void
lba_gjs_load_module (GObject *gobj, const gchar *module_filename) {
  LbaGjs *self = bm_get_LbaGjs (gobj);

  g_return_if_fail (module_filename);

  self->module_to_scan = module_filename;
  lba_gjs_sync_call_through_main_loop (self, lba_gjs_async_eval_file);
  self->module_to_scan = NULL;
}

static void
lba_gjs_init (GObject *object, LbaGjs *self) {
  g_mutex_init (&self->lock);
  g_cond_init (&self->cond);
}

static gboolean
lba_gjs_async_dispose (gpointer data) {
  LbaGjs *self = (LbaGjs *) data;

  if (self->js_context) {
    g_object_unref (self->js_context);
    self->js_context = NULL;
  }

  return G_SOURCE_REMOVE;
}

static void
lba_gjs_dispose (GObject *gobject) {
  LbaGjs *self = bm_get_LbaGjs (gobject);

  lba_gjs_sync_call_through_main_loop (self, lba_gjs_async_dispose);

  BM_CHAINUP (self, GObject)->dispose (gobject);
}

static void
lba_gjs_finalize (GObject *gobject) {
  LbaGjs *self = bm_get_LbaGjs (gobject);

  g_mutex_clear (&self->lock);
  g_cond_clear (&self->cond);

  BM_CHAINUP (self, GObject)->finalize (gobject);
}

static void
lba_gjs_class_init (GObjectClass *object_class, LbaGjsClass *klass) {
  LbaModuleScannerClass *lms_class;

  object_class->dispose = lba_gjs_dispose;
  object_class->finalize = lba_gjs_finalize;

  lms_class = BM_CLASS_LOOKUP_MIXIN (klass, LbaModuleScanner);
  lms_class->plugin_path_env = "LBA_JS_PLUGINS_PATH";
  lms_class->plugin_prefix = "lba-";
  lms_class->plugin_suffix = ".js";
  lms_class->have_file = lba_gjs_load_module;
}

/* Export plugin */
BOMBOLLA_PLUGIN_SYSTEM_PROVIDE_GTYPE (lba_gjs);
