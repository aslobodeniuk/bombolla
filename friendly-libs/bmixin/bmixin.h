/* BMixin - "Bombolla Mixin" - GObject-based mixins
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

#ifndef _BMIXIN_H
#  define _BMIXIN_H

#  include <glib-object.h>

typedef enum {
  BM_ADD_TYPE_IFACE,
  BM_ADD_TYPE_DEP,
} BMAddType;

typedef struct {
  BMAddType add_type;
    GType (*gtype) (void);
  GInterfaceInfo iface_info;
} BMParams;

typedef struct {
  GTypeInfo info;
  GType type;
  const BMParams *params;
  gushort num_params;
} BMInfo;

#  define G_TYPE_IS_BMIXIN(t) (G_TYPE_FUNDAMENTAL (t) == bm_fundamental_get_type ())

GType bm_fundamental_get_type (void);
GType bm_register_mixed_type (const gchar * mixed_type_name,
                              GType base_type, GType mixin);

GType bm_register_mixin (const gchar * mixin_name, BMInfo * minfo);

gpointer bm_class_get_mixin (gpointer class, GType mixin);
gpointer bm_instance_get_mixin (gpointer instance, GType mixin);

GType bm_type_peek_mixed_type (const GType mixed_type, const GType mixin);
GQuark bmixin_info_qrk (void);

#  define BM_ADD_DEP(name) { .add_type = BM_ADD_TYPE_DEP, .gtype = name##_get_type }
#  define BM_DEP_ANY_SEP_GTYPE ((GType (*) (void)) 0xabc)
#  define BM_DEP_ANY_SEP { .add_type = BM_ADD_TYPE_DEP, .gtype = BM_DEP_ANY_SEP_GTYPE }
#  define BM_DEP_ANY_OF(...) BM_DEP_ANY_SEP, __VA_ARGS__ , BM_DEP_ANY_SEP

#  define BM_IMPLEMENTS_IFACE(head, body, iface, ...)                   \
  { .add_type = BM_ADD_TYPE_IFACE, .gtype = head##_##iface##_get_type,  \
        .iface_info = { (GInterfaceInitFunc)head##_##body##_##iface##_init, __VA_ARGS__ } }

#  define BM_DEFINE_MIXIN(name, Name, ...)                        \
  static void name##_class_init (GObjectClass *, Name##Class*);   \
  static void name##_init (GObject *, Name *);                    \
  static Name *bm_get_##Name (gpointer gobject);                  \
  static Name##Class *bm_class_get_##Name (gpointer gobject);     \
  static void                                                     \
  name##_proxy_class_init (gpointer class, gpointer p)            \
  {                                                               \
    name##_class_init (class, bm_class_get_##Name (class));       \
  }                                                               \
                                                                  \
  static void                                                     \
  name##_proxy_init (GTypeInstance * instance, gpointer p)              \
  {                                                                     \
    name##_init (G_OBJECT (instance), bm_get_##Name (instance));        \
  }                                                                     \
                                                                        \
  static const BMParams name##_bmixin_params[] = {__VA_ARGS__};         \
                                                                        \
  static BMInfo name##_info = {                                         \
    .info = {                                                           \
      .class_init = name##_proxy_class_init,                            \
      .instance_init = name##_proxy_init,                               \
      .class_size = sizeof (Name##Class),                               \
      .instance_size = sizeof (Name)                                    \
    },                                                                  \
    .params = name##_bmixin_params,                                     \
    .num_params = G_N_ELEMENTS (name##_bmixin_params)                   \
  };                                                                    \
                                                                        \
  GType name##_get_type (void) {                                        \
    static gsize g_define_type_id = 0;                                  \
    if (g_once_init_enter (&g_define_type_id)) {                        \
      g_define_type_id = bm_register_mixin (#Name, &name##_info);       \
      name##_info.type = g_define_type_id;                              \
      g_type_set_qdata (name##_info.type, bmixin_info_qrk (), &name##_info); \
    }                                                                   \
    return g_define_type_id;                                            \
  }                                                                     \
                                                                        \
  static Name *bm_get_##Name (gpointer gobject) {                       \
    return bm_instance_get_mixin (gobject, name##_info.type);        \
  }                                                                     \
  static Name##Class *bm_class_get_##Name (gpointer gobject) {          \
    return bm_class_get_mixin (gobject, name##_info.type);           \
  }                                                                     \

/**
 * BM_CHAINUP:
 *
 * @gobject: instance of the GObject (not bmixin private data)
 * @name: identifyer of the mixin we are going to chainup from
 * @Parent: return value will be casted to the Parent+Class
 *
 * Returns: the parent class - previous class in the inheiritance
 * tree, relatively to the mixin, identified by @name.
 *
 * ```C
 *   BM_CHAINUP (gobject, lba_cogl, GObject)->dispose (gobject);
 * ```
 */
#  define BM_CHAINUP(gobject, name, Parent)                             \
  ((Parent##Class*)                                                     \
      g_type_class_peek (                                               \
        g_type_parent (                                                 \
          bm_type_peek_mixed_type (                                     \
            G_TYPE_FROM_INSTANCE(gobject),                              \
            name##_info.type)                                           \
          )))
/* TODO: way faster solution - cache the pointers in the BMixin base struct.
 * Pointers to cache: gobject, gobject_class, gobject_parent_class, gmo_info.
 */
#endif /* _BMIXIN_H */
