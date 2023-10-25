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

typedef enum {
  GMO_ADD_TYPE_IFACE,
  GMO_ADD_TYPE_FRIEND,
  GMO_ADD_TYPE_REQ
} GMOAddType;

typedef struct {
  GTypeInfo info;
  GType type;
  struct {
    GMOAddType add_type;
      GType (*gtype) (void);
    GInterfaceInfo iface_info;
  } add[];
} GMOInfo;

GType gmo_register_mutant (const gchar * mutant_name,
                           const GType base_type, const gchar * mutogene_name);

GType gmo_register_mutogene (const gchar * mutogene_name, GMOInfo * minfo);

gpointer gmo_class_get_mutogene (gpointer class, const GType mutogene);
gpointer gmo_instance_get_mutogene (gpointer instance, const GType mutogene);
GType gmo_type_peek_mutant (const GType mutant_type, const GType mutogene);
GQuark gmo_info_qrk (void);

#  define GMO_ADD_FRIEND(name) { .add_type = GMO_ADD_TYPE_FRIEND, .gtype = name##_get_type }
#  define GMO_ADD_REQ(name) { .add_type = GMO_ADD_TYPE_REQ, .gtype = name##_get_type }

#  define GMO_REQ_VARIANTS_MARKER {.add_type = GMO_ADD_TYPE_REQ, .gtype = (GType (*) (void;) 0xabc }
#  define GMO_REQ_VARIANTS(...) GMO_REQ_VARIANTS_MARKER, __VA_ARGS__ , GMO_REQ_VARIANTS_MARKER

#  define GMO_ADD_IFACE(head, body, iface, ...)                         \
  { .add_type = GMO_ADD_TYPE_IFACE, .gtype = head##_##iface##_get_type, \
        .iface_info = { (GInterfaceInitFunc)head##_##body##_##iface##_init, __VA_ARGS__ } }

#  define GMO_DEFINE_MUTOGENE(name, Name, ...)                    \
  static void name##_class_init (GObjectClass *, gpointer);       \
  static void name##_init (GObject *, gpointer);                  \
  static Name *gmo_get_##Name (gpointer gmo);                     \
  static Name##Class *gmo_class_get_##Name (gpointer gmo);        \
  static void                                                     \
  name##_proxy_class_init (gpointer class, gpointer p)            \
  {                                                               \
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
    .add = {{0}, __VA_ARGS__, {0}}                                      \
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
  }                                                                     \

/**
 * GMO_CHAINUP:
 *
 * @gmo: instance of the GObject (not mutogene private data)
 * @name: identifyer of the mutogene we are going to chainup from
 * @Parent: return value will be casted to the Parent+Class
 *
 * Returns: the parent class - previous class in the inheiritance
 * tree, relatively to the mutogene, identified by @name.
 *
 * ```C
 *   GMO_CHAINUP (gobject, lba_cogl, GObject)->dispose (gobject);
 * ```
 */
#  define GMO_CHAINUP(gmo, name, Parent)                                \
  ((Parent##Class*)                                                     \
      g_type_class_peek (                                               \
        g_type_parent (                                                 \
          gmo_type_peek_mutant (                                        \
            G_TYPE_FROM_INSTANCE(gmo),                                  \
            name##_info.type)                                           \
          )))

#endif /* _GMO_H */
