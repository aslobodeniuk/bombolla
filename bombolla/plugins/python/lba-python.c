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
#include <gmodule.h>
#include <Python.h>
#include <dlfcn.h>

typedef struct _LbaPython {
  GObject parent;
  gchar *plugins_path;
  gboolean initialized;
  void (*py_initialize) ();
  int (*py_run_simple_string) (const char *command);
  int (*py_run_simple_file) (FILE * fp, const char *filename);
  void (*py_finalize) ();
} LbaPython;

typedef struct _LbaPythonClass {
  GObjectClass parent;
} LbaPythonClass;

static gboolean
lba_python_eval_file (LbaPython * self, const gchar * filename) {
  FILE *fp;

  LBA_LOG ("Evaluating [%s]", filename);

  fp = g_fopen (filename, "r");
  if (fp) {
    /* FIXME: catch exception */
    self->py_run_simple_file (fp, filename);
    fclose (fp);

    /* FIXME: access lba_plugin, to add a reference on LbaPython for this
     * object. */
    self->py_run_simple_string
        ("print ('loaded class: %s' % GObject.GType(lba_plugin).name)");
    return TRUE;
  }

  return FALSE;
}

static GSList *lba_python_modules_scan_path (const gchar * path,
                                             GSList * modules_files);

/* This function is recursive */
static GSList *
lba_python_modules_scan_path (const gchar * path, GSList * modules_files) {
  static const char *LBA_PY_PLUGIN_PREFIX = "lba-";
  static const char *LBA_PY_PLUGIN_SUFFIX = ".py";

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
        modules_files = lba_python_modules_scan_path (sub_path, modules_files);
        g_free (sub_path);
        continue;
      }

      /* If not a directory */
      if (g_str_has_prefix (file, LBA_PY_PLUGIN_PREFIX) &&
          g_str_has_suffix (file, LBA_PY_PLUGIN_SUFFIX)) {
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
lba_python_scan (LbaPython * self) {
  GSList *modules_files = NULL;
  GSList *l;

  g_return_if_fail (self->plugins_path != NULL);

  modules_files = lba_python_modules_scan_path (self->plugins_path, NULL);

  for (l = modules_files; l; l = l->next) {
    lba_python_eval_file (self, (gchar *) l->data);
  }
  g_slist_free_full (modules_files, g_free);
}

static gboolean
lba_python_fill_vtable_local (LbaPython * self) {
  self->py_initialize = (void (*)(void))dlsym (NULL, "Py_Initialize");
  if (!self->py_initialize) {
    g_critical ("Couldn't load Py_Initialize");
    return FALSE;
  }

  self->py_run_simple_string =
      (int (*)(const char *))dlsym (NULL, "PyRun_SimpleString");

  if (!self->py_run_simple_string) {
    g_critical ("Couldn't load PyRun_SimpleString");
    return FALSE;
  }

  self->py_run_simple_file =
      (int (*)(FILE *, const char *))dlsym (NULL, "PyRun_SimpleFile");

  if (!self->py_run_simple_file) {
    g_critical ("Couldn't load PyRun_SimpleFile");
    return FALSE;
  }

  self->py_finalize = (void (*)(void))dlsym (NULL, "Py_Finalize");
  if (!self->py_finalize) {
    g_critical ("Couldn't load Py_Finalize");
    return FALSE;
  }

  return TRUE;
}

static gboolean
lba_python_fill_vtable (LbaPython * self) {
  GModule *module = NULL;
  gint i;

  const gchar *pythons[] = {
    "libpython3.10.so",
    "libpython3.9.so",
    "libpython3.8.so",
    "libpython3.7.so",
    "libpython3.6.so",
    "libpython3.5.so",
    "libpython3.4.so",
    "libpython3.3.so",
    "libpython3.2.so",
    "libpython3.1.so",
    NULL
  };

  if (dlsym (NULL, "Py_Initialize")) {
    LBA_LOG ("Python already have been loaded."
             " Possibly we are even running from it.");
    /* FIXME: running python plugin from python still crashes */
    return lba_python_fill_vtable_local (self);
  }

  for (i = 0; pythons[i] && module == NULL; i++) {
    module = g_module_open (pythons[i], 0);
  }

  if (G_UNLIKELY (!module)) {
    g_critical ("Python not found");
    return FALSE;
  }

  LBA_LOG ("Loaded Python module: %s", pythons[i]);
  g_module_make_resident (module);

  if (!g_module_symbol (module, "Py_Initialize", (gpointer *) & self->py_initialize)) {
    g_critical ("Couldn't load Py_Initialize");
    return FALSE;
  }
  if (!g_module_symbol
      (module, "PyRun_SimpleString", (gpointer *) & self->py_run_simple_string)) {
    g_critical ("Couldn't load PyRun_SimpleString");
    return FALSE;
  }
  if (!g_module_symbol
      (module, "PyRun_SimpleFile", (gpointer *) & self->py_run_simple_file)) {
    g_critical ("Couldn't load PyRun_SimpleFile");
    return FALSE;
  }
  if (!g_module_symbol (module, "Py_Finalize", (gpointer *) & self->py_finalize)) {
    g_critical ("Couldn't load Py_Finalize");
    return FALSE;
  }

  self->py_initialize ();
  return TRUE;
}

static void
lba_python_init (LbaPython * self) {
  static volatile gboolean once;

  if (!self->plugins_path) {
    self->plugins_path = g_strdup (g_getenv ("LBA_PLUGINS_PATH"));

    if (!self->plugins_path) {
      LBA_LOG ("No plugin path is set. Will scan current directory.");
      self->plugins_path = g_get_current_dir ();
    }
  }

  if (!once) {
    once = TRUE;

    if (!lba_python_fill_vtable (self)) {
      return;
    }

    self->py_run_simple_string ("print('Hello from Bombolla home python')");
    self->py_run_simple_string ("from gi.repository import GObject");

    lba_python_scan (self);
    self->initialized = TRUE;
  }
}

static void
lba_python_dispose (GObject * gobject) {
  LbaPython *self = (LbaPython *) gobject;

  if (self->initialized) {
    self->py_run_simple_string ("print('Bombolla home python goes to sleep')");
  }
}

static void
lba_python_finalize (GObject * gobject) {
  LbaPython *self = (LbaPython *) gobject;

  if (self->initialized) {
    /* FIXME: if we don't remove all the classes defined in python
     * before LbaPython, the proccess crashes. */
    self->py_finalize ();
  }
}

static void
lba_python_class_init (LbaPythonClass * klass) {
  GObjectClass *object_class = (GObjectClass *) klass;

  object_class->dispose = lba_python_dispose;
  object_class->finalize = lba_python_finalize;
}

G_DEFINE_TYPE (LbaPython, lba_python, G_TYPE_OBJECT);
/* Export plugin */
BOMBOLLA_PLUGIN_SYSTEM_PROVIDE_GTYPE (lba_python);
