/* la Bombolla GObject shell.
 * Copyright (C) 2025 Aleksandr Slobodeniuk
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

/* This is a variable to cache plugin's type's name for logging */
G_GNUC_UNUSED static const gchar *global_lba_plugin_name;

#  define BOMBOLLA_PLUGIN_SYSTEM_PROVIDE_GTYPE(name)                    \
  GType BOMBOLLA_PLUGIN_SYSTEM_ENTRY_ (void)                            \
  {                                                                     \
    GType ret = name##_get_type ();                                     \
    if (!ret)                                                           \
      return 0;                                                         \
    global_lba_plugin_name = g_type_name (ret);                         \
    return ret;                                                         \
  }

GType lba_command_fundamental_get_type (void);
GType lba_core_object_get_type (void);

/* If we register a type for a command and derive it from LbaCommand,
   what's the point?
   1. we'll be able to attach our things to it. But what things??
     g_type_register_static (base_type, mixed_type_name, &mixed_type_info,
       result_type_flags);
*/
#  define BOMBOLLA_PLUGIN_SYSTEM_PROVIDE_COMMAND(name, overs, ...) \
  GType BOMBOLLA_PLUGIN_SYSTEM_ENTRY_ (void)                        \
  {                                                                 \
    GType params[] = { __VA_ARGS__ };                               \
    struct                                                          \
    {                                                               \
      GSignalFlags flags;                                           \
      GType rtype;                                                  \
      GSignalAccumulator ac;                                        \
      gpointer ac_data;                                             \
      GSignalCMarshaller c_marshaller;                              \
    } ovs = overs;                                                  \
                                                                    \
    /* FIXME: should take core lock */                                 \
    g_signal_new_class_handler (#name,                                 \
        lba_core_object_get_type (),                                   \
        (ovs.flags ? ovs.flags :                                    \
            (G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION)),                 \
        (GCallback) lba_command_##name,                             \
        ovs.ac,                                                     \
        ovs.ac_data,                                                \
        ovs.c_marshaller,                                           \
        ovs.rtype ? ovs.rtype : G_TYPE_NONE,                        \
        G_N_ELEMENTS (params),                                      \
        __VA_ARGS__                                                 \
      );                                                            \
    /* ----------------------------- */                             \
                                                                    \
    global_lba_plugin_name = "LbaCommand_" #name;                   \
    return lba_command_fundamental_get_type ();                     \
  }                                                                 \

#endif

/* These two are only added to avoid gnu indent screwing the format.
 * When we pass '{}' to a macro, formatting nightmare takes place
 * (and we are not the ones to blame formatting tool).
 * When a proper parameter will be implemented in the gnu indent this
 * macros can be dropped. */
#define LBA_COMMAND_SETUP(...) {__VA_ARGS__}
#define LBA_COMMAND_SETUP_DEFAULT {}
