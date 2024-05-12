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
#include <bmixin/bmixin.h>
#include <gmodule.h>
#include <Python.h>
#include <dlfcn.h>

typedef struct _LbaPython {
  GObject parent;
  gboolean initialized;
  void (*py_initialize) ();
  int (*py_run_simple_string) (const char *command);
  int (*py_run_simple_file) (FILE * fp, const char *filename);
  void (*py_finalize) ();
} LbaPython;

typedef struct _LbaPythonClass {
  GObjectClass parent;
} LbaPythonClass;

BM_DEFINE_MIXIN (lba_python, LbaPython, BM_ADD_DEP (lba_module_scanner));

static void
lba_python_eval_file (GObject * gobject, const gchar * filename) {
  LbaPython *self = bm_get_LbaPython (gobject);
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
  }

  return;
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

  self->py_initialize ();
  return TRUE;
}

static gboolean
lba_python_fill_vtable (LbaPython * self) {
  GModule *module = NULL;
  gint i;

  const gchar *pythons[] = {
    /* FIXME: need better way */
    "libpython3.12.so",
    "libpython3.11.so",
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
lba_python_constructed (GObject * gobject) {
  LbaPython *self = bm_get_LbaPython (gobject);

  self->py_run_simple_string ("print('Hello from Bombolla home python')");
  self->py_run_simple_string ("from gi.repository import GObject");
  self->initialized = TRUE;

  BM_CHAINUP (self, GObject)->constructed (gobject);
}

static void
lba_python_init (GObject * object, LbaPython * self) {
  if (!lba_python_fill_vtable (self)) {
    g_warn_if_reached ();
    return;
  }
}

static void
lba_python_dispose (GObject * gobject) {
  LbaPython *self = bm_get_LbaPython (gobject);

  if (self->initialized) {
    self->py_run_simple_string ("print('Bombolla home python goes to sleep')");
  }

  BM_CHAINUP (self, GObject)->dispose (gobject);
}

static void
lba_python_finalize (GObject * gobject) {
  LbaPython *self = bm_get_LbaPython (gobject);

  if (self->initialized) {
    /* FIXME: if we don't remove all the classes defined in python
     * before LbaPython, the proccess crashes. */
    self->py_finalize ();
  }

  BM_CHAINUP (self, GObject)->finalize (gobject);
}

static void
lba_python_class_init (GObjectClass * object_class, LbaPythonClass * klass) {
  LbaModuleScannerClass *lms_class;

  object_class->constructed = lba_python_constructed;
  object_class->dispose = lba_python_dispose;
  object_class->finalize = lba_python_finalize;

  lms_class = BM_CLASS_LOOKUP_MIXIN (klass, LbaModuleScanner);
  lms_class->plugin_path_env = "LBA_PYTHON_PLUGINS_PATH";
  lms_class->plugin_prefix = "lba-";
  lms_class->plugin_suffix = ".py";
  lms_class->have_file = lba_python_eval_file;
}

/* Export plugin */
BOMBOLLA_PLUGIN_SYSTEM_PROVIDE_GTYPE (lba_python);
