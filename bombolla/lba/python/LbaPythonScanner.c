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

#include <Python.h>

typedef struct _LbaPythonScanner
{
  GObject parent;
} LbaPythonScanner;


typedef struct _LbaPythonScannerClass
{
  GObjectClass parent;
} LbaPythonScannerClass;


static void
lba_python_scanner_init (LbaPythonScanner * self)
{
  Py_Initialize();

  PyRun_SimpleString("print('Hello from Bombolla home python')");
  // Scan path for files
  {
    char filename[] = "pyemb7.py";
    FILE* fp;
    fp = g_fopen (filename, "r");

    if (fp) {
      PyRun_SimpleFile(fp, filename);
      fclose (fp);
    }
  }
}

static void
lba_python_scanner_dispose (GObject * gobject)
{
  PyRun_SimpleString("print('Bombolla home python goes to sleep')");
  Py_Finalize();
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
