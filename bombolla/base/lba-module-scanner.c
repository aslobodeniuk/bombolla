/* la Bombolla GObject shell.
 *
 *   Copyright (C) 2023 Alexander Slobodeniuk <aleksandr.slobodeniuk@gmx.es>
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
#include "lba-module-scanner.h"
#include <bmixin/bmixin.h>

typedef enum {
  PROP_PLUGINS_PATH = 1
} LbaModuleScannerProperty;

typedef struct _LbaModuleScanner {
  BMixinInstance i;
  // TODO: this should be in the BMixin structure (that doesn't exist yet)
  GObject *gobject;
  LbaModuleScannerClass *klass;
  // ---------------------------------------------------------------------

  gchar *plugin_path;
  GArray *threads;
} LbaModuleScanner;

BM_DEFINE_MIXIN (lba_module_scanner, LbaModuleScanner);

enum {
  SIGNAL_SCAN,
  SIGNAL_SCAN_IN_THREAD,
  LAST_SIGNAL
};

static guint lba_module_scanner_signals[LAST_SIGNAL] = { 0 };

static GSList *lba_module_scanner_scan_path (LbaModuleScannerClass * klass,
                                             const gchar * path,
                                             GSList * modules_files);

/* This function is recursive */
static GSList *
lba_module_scanner_scan_path (LbaModuleScannerClass *klass, const gchar *path,
                              GSList *modules_files) {
  /* FIXME: BMixin should contain ptr to gobject and to klass */
  g_return_val_if_fail (klass->plugin_prefix != NULL, NULL);
  g_return_val_if_fail (klass->plugin_suffix != NULL, NULL);

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
        modules_files =
            lba_module_scanner_scan_path (klass, sub_path, modules_files);
        g_free (sub_path);
        continue;
      }

      /* If not a directory */
      if (g_str_has_prefix (file, klass->plugin_prefix) &&
          g_str_has_suffix (file, klass->plugin_suffix)) {
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
lba_module_scanner_scan (GObject *gobject, const gchar *file) {
  LbaModuleScanner *self;

  LBA_LOG ("Going to scan: [%s]", file);

  self = bm_get_LbaModuleScanner (gobject);
  g_return_if_fail (file != NULL);

  self->klass->have_file (self->gobject, file);
}

static void
lba_module_scanner_scan_start (LbaModuleScanner *self) {
  GSList *modules_files = NULL,
      *l;

  g_return_if_fail (self->plugin_path != NULL);
  g_return_if_fail (self->klass->have_file != NULL);

  modules_files =
      lba_module_scanner_scan_path (self->klass, self->plugin_path, NULL);
  for (l = modules_files; l; l = l->next) {
    self->klass->have_file (self->gobject, (gchar *) l->data);
  }
  g_slist_free_full (modules_files, g_free);
}

typedef struct {
  LbaModuleScanner *scanner;
  gchar *file;
  GThread *thread;
} LbaModuleScannerThreadScan;

static gpointer
lba_module_scanner_scan_custom_thread (gpointer data) {
  LbaModuleScannerThreadScan *ev = (LbaModuleScannerThreadScan *) data;
  LbaModuleScanner *self = ev->scanner;

  LBA_LOG ("Scanuating right here: %s", ev->file);
  self->klass->have_file (self->gobject, ev->file);

  return NULL;
}

static void
lba_module_scanner_scan_in_thread (GObject *gobject, const gchar *file) {
  LbaModuleScanner *self;

  LBA_LOG ("Going to scan in thread: [%s]", file);

  self = bm_get_LbaModuleScanner (gobject);
  g_return_if_fail (file != NULL);

  {
    // FIXME: really need mutex here.
    {
      LbaModuleScannerThreadScan ev = { 0 };
      g_array_append_val (self->threads, ev);
    }

    LbaModuleScannerThreadScan *pev;

    pev =
        &((LbaModuleScannerThreadScan *) (self->threads->data))[self->threads->len -
                                                                1];
    pev->file = g_strdup (file);
    pev->scanner = self;
    // FIXME: RCs here, and all around
    pev->thread = g_thread_new (NULL, lba_module_scanner_scan_custom_thread, pev);
  }

}

static void
lba_module_scanner_thread_scan_clear (gpointer data) {
  LbaModuleScannerThreadScan *ev = (LbaModuleScannerThreadScan *) data;

  g_thread_join (ev->thread);
  g_free (ev->file);
}

static void
lba_module_scanner_init (GObject *gobject, LbaModuleScanner *self) {
  self->threads = g_array_new (FALSE, FALSE, sizeof (LbaModuleScannerThreadScan));
  g_array_set_clear_func (self->threads, lba_module_scanner_thread_scan_clear);
  self->gobject = gobject;
}

static void
lba_module_scanner_constructed (GObject *gobject) {
  LbaModuleScanner *self = bm_get_LbaModuleScanner (gobject);

  self->klass = bm_class_get_LbaModuleScanner (G_OBJECT_GET_CLASS (gobject));

  /* need "scan" signal?? */
  if (!self->plugin_path) {
    if (self->klass->plugin_path_env)
      self->plugin_path = g_strdup (g_getenv (self->klass->plugin_path_env));

    if (!self->plugin_path && 0) {
      // FIXME: that's nasty
      LBA_LOG ("No plugin path is set. Will scan current directory.");
      self->plugin_path = g_get_current_dir ();
    }
  }

  if (self->klass->scan_on_constructed && self->plugin_path)
    lba_module_scanner_scan_start (self);
}

static void
lba_module_scanner_finalize (GObject *gobject) {
  LbaModuleScanner *self = bm_get_LbaModuleScanner (gobject);

  g_array_unref (self->threads);
  g_free (self->plugin_path);

  BM_CHAINUP (gobject, lba_module_scanner, GObject)->finalize (gobject);
}

static void
lba_module_scanner_set_property (GObject *object,
                                 guint property_id, const GValue *value,
                                 GParamSpec *pspec) {
  LbaModuleScanner *self = bm_get_LbaModuleScanner (object);

  switch ((LbaModuleScannerProperty) property_id) {
  case PROP_PLUGINS_PATH:
    g_free (self->plugin_path);
    self->plugin_path = g_value_dup_string (value);
    break;
  default:
    /* We don't have any other property... */
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}

static void
lba_module_scanner_get_property (GObject *object,
                                 guint property_id, GValue *value,
                                 GParamSpec *pspec) {
  LbaModuleScanner *self = bm_get_LbaModuleScanner (object);

  switch ((LbaModuleScannerProperty) property_id) {
  case PROP_PLUGINS_PATH:
    g_value_set_string (value, self->plugin_path);
    break;
  default:
    /* We don't have any other property... */
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}

static void
lba_module_scanner_class_init (GObjectClass *gobject_class,
                               LbaModuleScannerClass *self_class) {
  gobject_class->finalize = lba_module_scanner_finalize;
  gobject_class->constructed = lba_module_scanner_constructed;
  gobject_class->set_property = lba_module_scanner_set_property;
  gobject_class->get_property = lba_module_scanner_get_property;

  self_class->scan_on_constructed = TRUE;
  self_class->scan = lba_module_scanner_scan;
  self_class->scan_in_thread = lba_module_scanner_scan_in_thread;

  g_object_class_install_property (gobject_class, PROP_PLUGINS_PATH,
                                   g_param_spec_string ("plugins-path",
                                                        "Plugins path",
                                                        "Path to scan the plugins",
                                                        NULL,
                                                        G_PARAM_STATIC_STRINGS |
                                                        G_PARAM_READWRITE));

  lba_module_scanner_signals[SIGNAL_SCAN_IN_THREAD] =
      g_signal_new ("scan-in-thread", G_TYPE_FROM_CLASS (gobject_class),
                    G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                    /* FIXME: how to protect user from putting G_STRUCT_OFFSET ?? */
                    BM_CLASS_VFUNC_OFFSET (self_class, scan_in_thread),
                    NULL, NULL,
                    g_cclosure_marshal_VOID__STRING,
                    G_TYPE_NONE, 1, G_TYPE_STRING, G_TYPE_NONE);

  lba_module_scanner_signals[SIGNAL_SCAN] =
      g_signal_new ("scan", G_TYPE_FROM_CLASS (gobject_class),
                    G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                    /* FIXME: how to protect user from putting G_STRUCT_OFFSET ?? */
                    BM_CLASS_VFUNC_OFFSET (self_class, scan),
                    NULL, NULL,
                    g_cclosure_marshal_VOID__STRING,
                    G_TYPE_NONE, 1, G_TYPE_STRING, G_TYPE_NONE);

}

/* Export plugin */
BOMBOLLA_PLUGIN_SYSTEM_PROVIDE_GTYPE (lba_module_scanner);
