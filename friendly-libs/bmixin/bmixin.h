/* BMixin - "Bombolla Mixin" - GObject-based mixins
 *
 * Copyright (c) 2024, Alexander Slobodeniuk <aleksandr.slobodeniuk@gmx.es>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * - Redistributions of source code must retain the above copyright notice, this
 *   list of conditions and the following disclaimer.
 *
 * - Redistributions in binary form must reproduce the above copyright notice, this
 *   list of conditions and the following disclaimer in the documentation and/or
 *   other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
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

/**
 * BMAddType: (skip)
 *
 * Used internally by BM_DEFINE_MIXIN()
 */
typedef enum {
  BM_ADD_TYPE_IFACE,
  BM_ADD_TYPE_DEP,
} BMAddType;

/**
 * BMParams: (skip)
 *
 * Used internally by BM_DEFINE_MIXIN()
 */
typedef struct {
  BMAddType add_type;
  /* wtf is wrong with the indent here?? */
    GType (*gtype) (void);
  GInterfaceInfo iface_info;
} BMParams;

/**
 * BMInfo: (skip)
 *
 * Used internally by BM_DEFINE_MIXIN()
 */
typedef struct {
  GTypeInfo info;
  GType type;
  const BMParams *params;
  gushort num_params;
} BMInfo;

/**
 * BMixin:
 * @type_instance: a #GTypeInstance
 * @root_object: a pointer to the root object instance
 *
 * A structure to put as a first member of your mixin structure.
 * It provides various things:
 * - cached pointer of the root GObject
 * - GTypeInstance, that allows to treat it as a normal GType instance. In particular
 * this allows type checks and discovery on the mixin pointer. So it would be safe
 * to call G_IS_OBJECT on the mixin instance pointer.
 */
typedef struct {
  GTypeInstance type_instance;
  GObject *root_object;
} BMixinInstance;

typedef struct {
  GTypeClass type_class;
  GObjectClass *root_object_class;
  GObjectClass *chainup_class;
} BMixinClass;

/**
 * BMIXIN:
 * @instance:
 *
 * Macro used to cast the mixin instance to BMixin.
 * Also performs a sanity check.
 */
#  define BMIXIN(mx) (G_TYPE_CHECK_INSTANCE_CAST ((mx), BM_TYPE_MIXIN, BMixinInstance))

/**
 * BM_GET_GOBJECT:
 *
 * Points from a mixin instance to the mixed object instance
 */
#  define BM_GET_GOBJECT(mx) (BMIXIN (mx)->root_object)

/**
 * BM_GET_CLASS:
 *
 * Get the class of the mixin
 */
#  define BM_GET_CLASS(mx) (G_TYPE_INSTANCE_GET_CLASS ((mx), BM_TYPE_MIXIN, BMixinClass))

/**
 * BM_CLASS_VFUNC_OFFSET:
 *
 * Use this macro when attaching a signal to #BMixin
 */
/* FIXME: how to protect user from putting G_STRUCT_OFFSET ?? */
/* TODO: make a static function with type checks */
#  define BM_CLASS_VFUNC_OFFSET(mclass,member)                          \
  (((gpointer)&mclass->member - (gpointer)mclass) + ((gpointer)mclass - (gpointer)((BMixinClass*)mclass)->root_object_class))

/**
 * BM_TYPE_MIXIN:
 *
 * Fundamental GType of BMixin
 */
#  define BM_TYPE_MIXIN bm_fundamental_get_type ()

/**
 * BM_GTYPE_IS_BMIXIN:
 * @type: A #GType value.
 *
 * Checks if @type is a BMixin type.
 *
 * Returns: %TRUE if @type is a mixin
 */
#  define BM_GTYPE_IS_BMIXIN(type) (G_TYPE_FUNDAMENTAL (type) == BM_TYPE_MIXIN)

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
 * |[
 * GObject
 *  ...
 *  -- BaseType
 *     -- BaseType+Dep1
 *        ...
 *        -- BaseType+Dep1+..+DepN
 *           -- BaseType+Dep1+..+DepN+Mixin
 * ]|
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
 * bm_register_mixin: (skip)
 *
 * Used internally by BM_DEFINE_MIXIN()
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
 * BM_ADD_IFACE:
 * 
 * link current mixin to the `iface_init` function.
 * This macro is used together with BM_DEFINE_MIXIN()
 *
 * Example:
 * |[<!-- language="C" -->
 *  static void
 *  lba_cogl_cube_icogl_init (LbaICogl * iface);
 *
 *  BM_DEFINE_MIXIN (lba_cogl_cube, LbaCoglCube,
 *      BM_ADD_IFACE (lba, cogl_cube, icogl),
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
 * ]|
 */
#  define BM_ADD_IFACE(head, body, iface, ...)                   \
  { .add_type = BM_ADD_TYPE_IFACE, .gtype = head##_##iface##_get_type,  \
        .iface_info = { (GInterfaceInitFunc)head##_##body##_##iface##_init, __VA_ARGS__ } }

/**
 * BM_DEFINE_MIXIN:
 *
 * Define a new mixin.
 * This macro will define a get_type function for the mixin type,
 * similar to G_DEFINE_TYPE for GObject.
 *
 * Example:
 * |[<!-- language="C" -->
 *  BM_DEFINE_MIXIN (lba_cogl_cube, LbaCoglCube,
 *      BM_ADD_IFACE (lba, cogl_cube, icogl),
 *      BM_ADD_DEP (lba_cogl),
 *      BM_ADD_DEP (lba_mixin_3d));
 * ]|
 * 
 */
#  define BM_DEFINE_MIXIN(name, Name, ...)                        \
  static void name##_class_init (GObjectClass *, Name##Class*);   \
  static void name##_init (GObject *, Name *);                    \
  static Name *bm_get_##Name (gpointer);                                \
  static Name##Class *bm_class_get_##Name (gpointer);                   \
                                                                        \
  static void name##_proxy_class_init (gpointer, gpointer);             \
  static void name##_proxy_init (GTypeInstance *, gpointer);            \
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
static void                                                       \
 name##_proxy_class_init (gpointer class, gpointer p)             \
  {                                                               \
    BMixinClass *bmc = (BMixinClass *)bm_class_get_##Name (class);  \
    bmc->type_class.g_type = name##_info.type;                    \
    bmc->root_object_class = (GObjectClass*) class;               \
    bmc->chainup_class = (GObjectClass *)g_type_class_peek (            \
        g_type_parent (                                                 \
        bm_type_peek_mixed_type (G_TYPE_FROM_CLASS (class),             \
            name##_info.type)));                                        \
    G_STATIC_ASSERT (sizeof (GTypeClass) == sizeof (GType));      \
    name##_class_init (class, (Name##Class*)bmc);                 \
  }                                                               \
                                                                  \
  static void                                                     \
  name##_proxy_init (GTypeInstance * instance, gpointer gclass)         \
  {                                                                     \
    BMixinInstance *bmi = (BMixinInstance *)bm_get_##Name (instance);   \
    bmi->type_instance.g_class = bm_class_get_mixin (gclass, name##_info.type); \
    bmi->root_object = (GObject*) instance;                             \
    G_STATIC_ASSERT (sizeof (GTypeInstance) == sizeof (gpointer));      \
                                                                        \
    name##_init (G_OBJECT (instance), (Name*)bmi);                      \
  }                                                                     \
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
    return bm_class_get_mixin (gobject, name##_info.type);              \
  }

/**
 * BM_CHAINUP:
 *
 * @gobject: instance of the GObject (not bmixin private data)
 * @Parent: return value will be casted to the Parent+Class
 *
 * Returns: the parent class - previous class in the inheiritance
 * tree, relatively to the mixin, identified by @name.
 *
 * |[<!-- language="C" -->
 *   BM_CHAINUP (mixin, GObject)->dispose (gobject);
 * ]|
 */
#  define BM_CHAINUP(mixin, Parent) ((Parent##Class*)(BM_GET_CLASS (mixin)->chainup_class))

#endif /* _BMIXIN_H */
