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
  gchar *plugin_path;
} LbaModuleScanner;

BM_DEFINE_MIXIN (lba_module_scanner, LbaModuleScanner);

static GSList *lba_module_scanner_scan_path (LbaModuleScannerClass * klass,
                                             const gchar * path,
                                             GSList * modules_files);

/* This function is recursive */
static GSList *
lba_module_scanner_scan_path (LbaModuleScannerClass * klass, const gchar * path,
                              GSList * modules_files) {
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
lba_module_scanner_scan (LbaModuleScanner * self, GObject * gobject,
                         LbaModuleScannerClass * klass) {
  /* FIXME: BMixin should contain ptr to gobject and to klass */
  GSList *modules_files = NULL,
      *l;

  g_return_if_fail (self->plugin_path != NULL);
  g_return_if_fail (klass->have_file != NULL);

  modules_files = lba_module_scanner_scan_path (klass, self->plugin_path, NULL);
  for (l = modules_files; l; l = l->next) {
    klass->have_file (gobject, (gchar *) l->data);
  }
  g_slist_free_full (modules_files, g_free);
}

static void
lba_module_scanner_init (GObject * gobject, LbaModuleScanner * self) {
}

static void
lba_module_scanner_constructed (GObject * gobject) {
  /* FIXME: bmixin_get_LbaModuleScannerClass??
   * Ptr on Class can be cached in BMixin structure */
  LbaModuleScannerClass *klass =
      bm_class_get_LbaModuleScanner (G_OBJECT_GET_CLASS (gobject));
  LbaModuleScanner *self = bm_get_LbaModuleScanner (gobject);

  /* need "scan" signal?? */
  if (!self->plugin_path) {
    if (klass->plugin_path_env)
      self->plugin_path = g_strdup (g_getenv (klass->plugin_path_env));

    if (!self->plugin_path) {
      LBA_LOG ("No plugin path is set. Will scan current directory.");
      self->plugin_path = g_get_current_dir ();
    }
  }

  if (klass->scan_on_constructed)
    lba_module_scanner_scan (self, gobject, klass);
}

static void
lba_module_scanner_finalize (GObject * gobject) {
  LbaModuleScanner *self = bm_get_LbaModuleScanner (gobject);

  g_free (self->plugin_path);

  BM_CHAINUP (gobject, lba_module_scanner, GObject)->finalize (gobject);
}

static void
lba_module_scanner_set_property (GObject * object,
                                 guint property_id, const GValue * value,
                                 GParamSpec * pspec) {
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
lba_module_scanner_get_property (GObject * object,
                                 guint property_id, GValue * value,
                                 GParamSpec * pspec) {
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
lba_module_scanner_class_init (GObjectClass * gobject_class,
                               LbaModuleScannerClass * self_class) {
  gobject_class->finalize = lba_module_scanner_finalize;
  gobject_class->constructed = lba_module_scanner_constructed;
  gobject_class->set_property = lba_module_scanner_set_property;
  gobject_class->get_property = lba_module_scanner_get_property;

  self_class->scan_on_constructed = TRUE;

  g_object_class_install_property (gobject_class, PROP_PLUGINS_PATH,
                                   g_param_spec_string ("plugins-path",
                                                        "Plugins path",
                                                        "Path to scan the plugins",
                                                        NULL,
                                                        G_PARAM_STATIC_STRINGS |
                                                        G_PARAM_READWRITE));
}

/* Export plugin */
BOMBOLLA_PLUGIN_SYSTEM_PROVIDE_GTYPE (lba_module_scanner);
