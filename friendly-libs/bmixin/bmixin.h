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

/**
 * BMixin:
 *
 * The base mixin type.
 *
 * `BMixin` (Bombolla Mixin) is a fundamental type providing ability to register
 * mixins for `GObject`-based types.
 * Mixin is a code that together with some parent can produce
 * a child class. Mixin is agnostic to what class is going
 * to be the parent.
 * Bombolla Mixin also has "dependencies". One point of view
 * on this - is a solution to resolve the problem of multiple
 * inheiritance. Another view on this - is that practically the code
 * of most of real mixins is going to depend on some other mixins or
 * interfaces, just because of the fact of using their API.
 * Mixed type - is a result of applying one or multiple mixins on
 * top of a base type, that has to be based on GObject.
 * When the creation of the mixed type starts, the dependency tree
 * is built, considering the dependencies and the dependencies of
 * the dependencies of each mixin in the tree.
 * When the dependency tree is resolved, there is only one instance
 * of each mixin applied to the inheiritance tree of the result object,
 * that is linear. The mixins are applied one after another in some order.
 */

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

/**
 * G_TYPE_IS_BMIXIN:
 * @type: A #GType value.
 *
 * Checks if @type is a BMixin type.
 *
 * Returns: %TRUE if @type is a mixin
 */
#  define G_TYPE_IS_BMIXIN(type) (G_TYPE_FUNDAMENTAL (type) == bm_fundamental_get_type ())

/**
 * bm_fundamental_get_type:
 *
 * The fundamental type for BMixin.
 *
 * Returns: a #GType that represents the fundamental type for BMixin
 */
GType bm_fundamental_get_type (void);

/**
 * bm_register_mixed_type:
 * @mixed_type_name: (nullable): name of the mixed type. If `NULL` is provided,
 *   a name will be generated automatically as `BaseType+Mixin1+...+MixinN`.
 * @base_type: a #GObject-based type, to apply the @mixin on. All the dependencies
 *   of the @mixin will be applied in between. Since any mixed type is #GObject-based,
 *   mixed type is suitable here.
 * @mixin: a BMixin type to apply on top of the #GObject-based @base_type.
 *
 * This function registers and returns a new #GType for a mixed type, based on the
 * @base_type with all the dependency tree of the @mixin applied to it. If @mixin has
 * dependencies, they are going to be resolved (as well as the dependencies of
 * dependencies): this means that some intermediate types, based on @base_type are
 * going to be registered. The end result will be a valid #GObject-based type with
 * a plane inheiritance tree, that will look like:
 * ```
 * GObject
 *  ...
 *  -- BaseType
 *     -- BaseType+Dep1
 *        ...
 *        -- BaseType+Dep1+..+DepN
 *           -- BaseType+Dep1+..+DepN+Mixin             
 * ```
 * It might happen that the dependency tree of the @mixin won't be resolved if some
 * of the dependencie is not satisfied. For example if the @mixin depends on a certain
 * interface, and none of the @mixin's dependencies, nor @base_type provide an
 * implementation for it. In this case this call will fail and return #G_TYPE_INVALID.
 *
 * Returns: a #GType of newly registered mixed type or #G_TYPE_INVALID if failed
 */
GType bm_register_mixed_type (const gchar * mixed_type_name,
                              GType base_type, GType mixin);

/**
 * bm_register_mixin:
 * @mixin_name: name of the mixin
 * @minfo: a completed #BMInfo structure. Note that this pointer must remain valid during
 *   all the lifetime of the proccess. The most usual option for it is a `static` storage.
 *   Also note that the `type` field of #BMInfo is going to be completed by this function,
 *   so there's no need to fill it.
 *
 * In a usual case you won't need this function because using BM_DEFINE_MIXIN() is much
 * simpler, and it covers most of the cases. However to get some extended control on
 * registering your mixin this function can be used. One of the real life cases -
 * is a need to set the `base_init()` callback in #GType-info of the result mixed type.
 * An example of using this function could be:
 * ```c
 * static const BMParams hello_mixin_params[] = {0}; // can add deps here
 * static BMInfo hello_info = {
 *   .info = {
 *     .class_init = hello_class_init,
 *     .instance_init = hello_init,
 *     .class_size = sizeof (HelloClass),
 *     .instance_size = sizeof (Hello),
 *     .base_init = hello_base_init
 *   },
 *   .params = hello_mixin_params,
 *   .num_params = G_N_ELEMENTS (hello_params)
 * };
 *
 * static Hello *
 * bm_get_Hello (gpointer gobject) {
 *   return bm_instance_get_mixin (gobject, hello_info.type);
 * }
 *
 * static HelloClass *
 * bm_class_get_Hello (gpointer class) {
 *   return bm_class_get_mixin (class, hello_info.type);
 * }
 *
 * GType hello_get_type (void) {
 *   static GType stype_id = 0;
 *   if (g_once_init_enter_pointer (&stype_id)) {
 *     GType type_id = bm_register_mixin ("HelloMixin", &hello_info);
 *     g_once_init_leave_pointer (&stype_id, type_id);
 *   }
 *   return stype_id;
 * }
 * ```
 *
 * Returns: a #GType of newly registered mixin
 */
GType bm_register_mixin (const gchar * mixin_name, BMInfo * minfo);

/**
 * bm_class_get_mixin:
 * @class:
 * @mixin:
 *
 * Returns: a pointer to the mixin class structure
 */
gpointer bm_class_get_mixin (gpointer class, GType mixin);

/**
 * bm_instance_get_mixin:
 * @instance:
 * @mixin:
 *
 * Returns: a pointer to the mixin structure
 */
gpointer bm_instance_get_mixin (gpointer instance, GType mixin);

/**
 * bm_type_peek_mixed_type:
 * @instance:
 * @mixin:
 *
 * Returns: a #GType of the mixed type that has @mixin on top of it.
 */
GType bm_type_peek_mixed_type (GType mixed_type, GType mixin);

/**
 * bmixin_info_qrk: (skip)
 *
 * Returns: internal #GQuark
 */
GQuark bmixin_info_qrk (void);

/**
 * BM_ADD_DEP:
 * @name: name of the dependency to add. Can be a mixin, #GObject-based type, or an interface
 */
#  define BM_ADD_DEP(name) { .add_type = BM_ADD_TYPE_DEP, .gtype = name##_get_type }

/**
 * BM_IMPLEMENTS_IFACE:
 * 
 * link current mixin to the `iface_init` function.
 * Example:
 * ```c
 * static void
 *  lba_cogl_cube_icogl_init (LbaICogl * iface);
 *
 *  BM_DEFINE_MIXIN (lba_cogl_cube, LbaCoglCube,
 *      BM_IMPLEMENTS_IFACE (lba, cogl_cube, icogl),
 *      BM_ADD_DEP (lba_cogl),
 *      BM_ADD_DEP (lba_mixin_3d));
 *
 * ...
 *
 * static void
 * lba_cogl_cube_icogl_init (LbaICogl * iface) {
 *   iface->paint = lba_cogl_cube_paint;
 *   iface->reopen = lba_cogl_cube_reopen;
 * }
 * ```
 */
#  define BM_IMPLEMENTS_IFACE(head, body, iface, ...)                   \
  { .add_type = BM_ADD_TYPE_IFACE, .gtype = head##_##iface##_get_type,  \
        .iface_info = { (GInterfaceInitFunc)head##_##body##_##iface##_init, __VA_ARGS__ } }

/**
 * BM_DEFINE_MIXIN:
 * TODO, epic
 */
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
