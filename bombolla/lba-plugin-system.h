/* la Bombolla GObject shell.
 * Copyright (C) 2020 Aleksandr Slobodeniuk
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

#ifndef _BOMBOLLA_PLUGIN_SYSTEM
#  define _BOMBOLLA_PLUGIN_SYSTEM
#  include <glib-object.h>

typedef GType (*lBaPluginSystemGetGtypeFunc) (void);

#  define BOMBOLLA_PLUGIN_SYSTEM_ENTRY_   bombolla_plugin_system_get_gtype
#  define BOMBOLLA_PLUGIN_SYSTEM_ENTRY   G_STRINGIFY (BOMBOLLA_PLUGIN_SYSTEM_ENTRY_)

#  define BOMBOLLA_PLUGIN_SYSTEM_PROVIDE_GTYPE(name)                      \
  GType BOMBOLLA_PLUGIN_SYSTEM_ENTRY_ (void)                            \
  {                                                                     \
    return name##_get_type ();                                          \
  }

#endif
