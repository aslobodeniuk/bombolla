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
#include <gjs/gjs.h>

typedef struct _LbaGjs LbaGjs;
struct _LbaGjs {
  GObject parent;

  GjsContext *js_context;

  gchar *plugins_path;

  GMutex lock;
  GCond cond;
  GSource *async_ctx;
  const gchar *module_to_scan;
};

typedef struct _LbaGjsClass {
  GObjectClass parent;
} LbaGjsClass;

G_DEFINE_TYPE (LbaGjs, lba_gjs, G_TYPE_OBJECT);

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
lba_gjs_sync_call_through_main_loop (LbaGjs * self, GSourceFunc cmd) {
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

static GSList *lba_gjs_modules_scan_path (const gchar * path,
                                          GSList * modules_files);

/* This function is recursive */
static GSList *
lba_gjs_modules_scan_path (const gchar * path, GSList * modules_files) {
  static const char *LBA_GJS_PLUGIN_PREFIX = "lba-";
  static const char *LBA_GJS_PLUGIN_SUFFIX = ".js";

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
        modules_files = lba_gjs_modules_scan_path (sub_path, modules_files);
        g_free (sub_path);
        continue;
      }

      /* If not a directory */
      if (g_str_has_prefix (file, LBA_GJS_PLUGIN_PREFIX) &&
          g_str_has_suffix (file, LBA_GJS_PLUGIN_SUFFIX)) {
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
lba_gjs_scan (LbaGjs * self) {
  GSList *modules_files = NULL,
      *l;

  g_return_if_fail (self->plugins_path != NULL);

  modules_files = lba_gjs_modules_scan_path (self->plugins_path, NULL);

  for (l = modules_files; l; l = l->next) {
    self->module_to_scan = (gchar *) l->data;
    lba_gjs_sync_call_through_main_loop (self, lba_gjs_async_eval_file);
  }
  self->module_to_scan = NULL;
  g_slist_free_full (modules_files, g_free);
}

static void
lba_gjs_init (LbaGjs * self) {
  static volatile gboolean once;

  g_mutex_init (&self->lock);
  g_cond_init (&self->cond);

  if (!once) {
    if (!self->plugins_path) {
      self->plugins_path = g_strdup (g_getenv ("LBA_PLUGINS_PATH"));

      if (!self->plugins_path) {
        LBA_LOG ("No plugin path is set. Will scan current directory.");
        self->plugins_path = g_get_current_dir ();
      }
    }

    lba_gjs_scan (self);
    once = 1;
  }
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
lba_gjs_dispose (GObject * gobject) {
  LbaGjs *self = (LbaGjs *) gobject;

  lba_gjs_sync_call_through_main_loop (self, lba_gjs_async_dispose);
  G_OBJECT_CLASS (lba_gjs_parent_class)->dispose (gobject);
}

static void
lba_gjs_finalize (GObject * gobject) {
  LbaGjs *self = (LbaGjs *) gobject;

  g_mutex_clear (&self->lock);
  g_cond_clear (&self->cond);
  G_OBJECT_CLASS (lba_gjs_parent_class)->finalize (gobject);
}

static void
lba_gjs_class_init (LbaGjsClass * klass) {
  GObjectClass *object_class = (GObjectClass *) klass;

  object_class->dispose = lba_gjs_dispose;
  object_class->dispose = lba_gjs_finalize;
}

/* Export plugin */
BOMBOLLA_PLUGIN_SYSTEM_PROVIDE_GTYPE (lba_gjs);
