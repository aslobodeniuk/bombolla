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

#ifndef _BOMBOLLA_LOG
#  define _BOMBOLLA_LOG
#  include <glib/gstdio.h>
#  include "bombolla/lba-plugin-system.h"

#  define LBA_LOG(form, ...) do {                                       \
    g_printf ("[LBA %p %s %s] " form "\n",                              \
        g_thread_self (), global_lba_plugin_name, __func__,  ##__VA_ARGS__); \
  } while (0)

#endif
