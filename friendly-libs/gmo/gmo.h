/* GObject Class Mutation
 *
 * Copyright (C) 2023 Alexander Slobodeniuk <aleksandr.slobodeniuk@gmx.es>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#ifndef _GMO_H
#  define _GMO_H

#  include <glib-object.h>

typedef struct {
  GTypeInfo info;
  GType type;
  struct {
    GType (*iface_type) (void);
    GInterfaceInfo info;
  } ifaces[];
} GMOInfo;

GType gmo_register_mutant (const gchar * mutant_name,
                           const GType base_type, const gchar * mutogene_name);

GType gmo_register_mutogene (const gchar * mutogene_name, GMOInfo * minfo);

gpointer gmo_class_get_mutogene (gpointer class, const GType mutogene);
gpointer gmo_instance_get_mutogene (gpointer instance, const GType mutogene);
GQuark gmo_info_qrk (void);

#  define GMO_IFACE(head, body, iface, ...)                               \
  { .iface_type = head##_##iface##_get_type,                            \
        .info = { (GInterfaceInitFunc)head##_##body##_##iface##_init, __VA_ARGS__ } }

#  define GMO_DEFINE_MUTOGENE_WITH_IFACES(name, Name, ...)        \
  static void name##_class_init (GObjectClass *, gpointer);       \
  static void name##_init (GObject *, gpointer);                  \
  static Name *gmo_get_##Name (gpointer gmo);                     \
  static Name##Class *gmo_class_get_##Name (gpointer gmo);        \
  static gpointer name##_parent_class;                            \
  static void                                                     \
  name##_proxy_class_init (gpointer class, gpointer p)            \
  {                                                               \
    name##_parent_class = g_type_class_peek_parent (class);       \
    name##_class_init (class, gmo_class_get_##Name (class));      \
  }                                                               \
                                                                  \
  static void                                                     \
  name##_proxy_init (GTypeInstance * instance, gpointer p)              \
  {                                                                     \
    name##_init (G_OBJECT (instance), gmo_get_##Name (instance));       \
  }                                                                     \
                                                                        \
  static GMOInfo name##_info = {                                        \
    .info = {                                                           \
      .class_init = name##_proxy_class_init,                            \
      .instance_init = name##_proxy_init,                               \
      .class_size = sizeof (Name##Class),                               \
      .instance_size = sizeof (Name)                                    \
    },                                                                  \
    .ifaces = {                                                         \
      __VA_ARGS__                                                       \
    }                                                                   \
  };                                                                    \
                                                                        \
  GType name##_get_type (void) {                                        \
    static gsize g_define_type_id = 0;                                  \
    if (g_once_init_enter (&g_define_type_id)) {                        \
      g_define_type_id = gmo_register_mutogene (#Name, &name##_info);   \
      name##_info.type = g_define_type_id;                              \
      g_type_set_qdata (name##_info.type, gmo_info_qrk (), &name##_info); \
    }                                                                   \
    return g_define_type_id;                                            \
  }                                                                     \
                                                                        \
  static Name *gmo_get_##Name (gpointer gmo) {                          \
    return gmo_instance_get_mutogene (gmo, name##_info.type);           \
  }                                                                     \
  static Name##Class *gmo_class_get_##Name (gpointer gmo) {             \
    return gmo_class_get_mutogene (gmo, name##_info.type);              \
  }

#endif /* _GMO_H */
