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

typedef struct _LbaPythonScanner
{
  GObject parent;
  GModule *module;
} LbaPythonScanner;


typedef struct _LbaPythonScannerClass
{
  GObjectClass parent;
} LbaPythonScannerClass;


static void
lba_python_scanner_init (LbaPythonScanner * self)
{
  static volatile gboolean once;

  if (!once) {
    GModule *module;

    /* NOTE: this is a hack to load gi python module, without it we
     * get "undefined symbol: PyExc_NotImplementedError"
     * TODO: iterate over versions, also maybe try not to link at all */
    module = g_module_open ("libpython3.8.so", 0);
    g_module_make_resident (module);
    once = TRUE;
  }

  Py_Initialize ();

  PyRun_SimpleString ("print('Hello from Bombolla home python')");
  PyRun_SimpleString ("from gi.repository import GObject");
  // TODO: scan path for files
  {
    // As you can see, my friend, as you can see...
    char filename[] = "examples/python/plugin-in-python.py";
    FILE *fp;
    fp = g_fopen (filename, "r");

    if (fp) {
      /* TODO: we really should dump what we have there..
       * But how to get names of the types registered ?? */
      PyRun_SimpleFile (fp, filename);
      fclose (fp);

      // Now extract that string to the C code!!
      PyRun_SimpleString ("print ('loaded class: %s' % GObject.GType(lba_plugin).name)");
    }
  }
}

static void
lba_python_scanner_dispose (GObject * gobject)
{
  PyRun_SimpleString ("print('Bombolla home python goes to sleep')");
  Py_Finalize ();
}

static void
lba_python_scanner_class_init (LbaPythonScannerClass * klass)
{
  GObjectClass *object_class = (GObjectClass *) klass;

  object_class->dispose = lba_python_scanner_dispose;
}


G_DEFINE_TYPE (LbaPythonScanner, lba_python_scanner, G_TYPE_OBJECT)
/* Export plugin */
    BOMBOLLA_PLUGIN_SYSTEM_PROVIDE_GTYPE (lba_python_scanner);
