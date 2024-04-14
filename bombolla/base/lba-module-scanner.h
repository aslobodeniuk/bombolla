/* la Bombolla GObject shell.
 *
 * Copyright (C) 2023 Alexander Slobodeniuk <aleksandr.slobodeniuk@gmx.es>
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
#ifndef _LBA_MODULE_SCANNER

typedef struct _LbaModuleScannerClass {
  const char *plugin_path_env;
  const char *plugin_prefix;
  const char *plugin_suffix;
  gboolean scan_on_constructed;
  /* Virtual function */
  void (*have_file) (GObject *, const gchar *);
  /* Signal handler */
  void (*scan_in_thread) (GObject *, const gchar *);
  void (*scan) (GObject *, const gchar *);
} LbaModuleScannerClass;

GType lba_module_scanner_get_type (void);

#endif
