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

/**
 * Design concept:
 * 
 * Experimental approach that we call "Bombolla Mixin",
 * or BMixin (aka bmixin).
 * 
 */

#include <bmixin/bmixin.h>

static GTypeInfo empty_info;

GQuark
bmixin_info_qrk (void) {
  static GQuark qrk;

  return G_LIKELY (qrk) ? qrk : (qrk = g_quark_from_static_string ("BMInfo"));
}

GType
bm_fundamental_get_type (void) {
  static GType ret;
  GTypeFundamentalInfo finfo = { G_TYPE_FLAG_DERIVABLE };
  if (G_LIKELY (ret))
    return ret;

  return (ret = g_type_register_fundamental (g_type_fundamental_next (),
                                             "BMixin", &empty_info, &finfo, 0));
}

GType
bm_type_peek_mixed_type (GType mixed_type, GType mixin) {
  g_return_val_if_fail (mixin, G_TYPE_INVALID);
  g_return_val_if_fail (mixed_type, G_TYPE_INVALID);
  for (;;) {
    BMInfo *info;

    info = g_type_get_qdata (mixed_type, bmixin_info_qrk ());
    if (info == 0)
      return 0;

    if (info->type == mixin)
      return mixed_type;

    mixed_type = g_type_parent (mixed_type);
  }

  return 0;
}

GType
bm_register_mixed_type (const gchar * mixed_type_name, GType base_type, GType mixin) {
  GType ret;
  GTypeQuery base_info;
  BMInfo *minfo;
  GTypeInfo mixed_type_info;
  gint i;
  GTypeFlags result_type_flags = 0;
  gchar *mixed_type_name_on_fly = NULL;

  g_return_val_if_fail (mixin, G_TYPE_INVALID);
  g_return_val_if_fail (base_type, G_TYPE_INVALID);
  g_return_val_if_fail (G_TYPE_IS_BMIXIN (mixin), G_TYPE_INVALID);

  minfo = g_type_get_qdata (mixin, bmixin_info_qrk ());
  g_return_val_if_fail (minfo, 0);

  for (i = 0; i < minfo->num_params; i++) {
    const BMParams *param = &minfo->params[i];

    switch (param->add_type) {
    case BM_ADD_TYPE_IFACE:
      /* Will proccess after type registration */
      continue;
    case BM_ADD_TYPE_DEP:
      {
        /*
         * If the dep is GObject..
         * If the dep is Interface..
         * If the dep is BMixin..
         */
        GType dep_type = param->gtype ();

        if (G_TYPE_IS_OBJECT (dep_type)) {
          /* GObject:
           * If the dep is GObject, then we check if the base_type
           * IS this type too. If it is, then the check have passed.
           * If the base_type is not that type, then we have a problem,
           * Houston.. But we are cool, and we check if "dep type" is actually
           * something of the base_type. Because if so, then we just replace
           * base_type by the dep type.
           * 
           * Setting various GObject types as a dependency doesn't make sence, but this
           * case will work too, as a side effect...
           */

          if (g_type_is_a (dep_type, base_type)) {
            base_type = dep_type;
            continue;
          }

          if (g_type_is_a (base_type, dep_type)) {
            /* Base type is a dep_type, and more, everything is ok!
             * Done with this check, let's jump to the next dep. */
            continue;
          }

          g_critical ("Mixin %s needs %s, but this dependency is not satisfied",
                      g_type_name (mixin), g_type_name (dep_type));
          return 0;
        }

        if (G_TYPE_IS_BMIXIN (dep_type)) {
          /* Mixin:
           * So, our dep is a mixin. Here come 2 scenarios: either base_type already has it,
           * either not. If it's already included to the base class, then we're done, and there's
           * nothing to do.
           * If required mutoge is not contained in the dna yet, then we add it.
           */
          if (bm_type_peek_mixed_type (base_type, dep_type)) {
            /* Mixin found in the base class, nothing to do */
            continue;
          }

          /* Dependency mixin is not found in the base class: then we add it
           * on top of the base. */
          base_type = bm_register_mixed_type (NULL, base_type, dep_type);
          if (base_type == 0)
            return 0;
          /* Done, now the base_type is updated and contains the dependency. */
          continue;
        }

        if (G_TYPE_IS_INTERFACE (dep_type)) {
          /* Interface:
           * If the dependency it an interface there isn't much we can do.
           * We could fail if the base class doesn't implement it, but
           * what if the type we build is only an intermediate one...
           * A new mixin that implements the interface might be added,
           * and then the type is complete.
           * So we don't fail the registration, but set "abstract" flag to the type
           * so the instance of it can't be created on it's own.. */
          result_type_flags |= G_TYPE_FLAG_ABSTRACT;
        }
      }
      break;
    default:
      g_critical ("Unexpected add type %d", param->add_type);
    }
  }

  g_type_query (base_type, &base_info);

  mixed_type_info = minfo->info;
  mixed_type_info.class_size += base_info.class_size;
  mixed_type_info.instance_size += base_info.instance_size;

  if (mixed_type_name == NULL) {
    mixed_type_name_on_fly = g_strdup_printf ("%s+%s", g_type_name (base_type),
                                              g_type_name (mixin));
    mixed_type_name = mixed_type_name_on_fly;
  }

  if (g_type_from_name (mixed_type_name)) {
    /* It might happen that such a mixed_type already have been created */
    ret = g_type_from_name (mixed_type_name);
  } else {
    ret =
        g_type_register_static (base_type, mixed_type_name, &mixed_type_info,
                                result_type_flags);

    g_type_set_qdata (ret, bmixin_info_qrk (), minfo);

    for (i = 0; i < minfo->num_params; i++) {
      if (minfo->params[i].add_type == BM_ADD_TYPE_IFACE) {
        g_type_add_interface_static (ret, minfo->params[i].gtype (),
                                     &minfo->params[i].iface_info);
      }
    }
  }

  if (mixed_type_name_on_fly) {
    g_free (mixed_type_name_on_fly);
    mixed_type_name = mixed_type_name_on_fly = NULL;
  }

  return ret;
}

GType
bm_register_mixin (const gchar * mixin_name, BMInfo * minfo) {
  GType ret;

  ret =
      g_type_register_static (bm_fundamental_get_type (), mixin_name,
                              &empty_info, 0);

  g_return_val_if_fail (ret != 0, 0);

  g_type_set_qdata (ret, bmixin_info_qrk (), minfo);

  return ret;
}

static GTypeQuery
bm_class_get_parent_info (GType t) {
  GTypeQuery parent_info;
  GType parent = g_type_parent (t);

  g_return_val_if_fail (parent != 0, (GTypeQuery) {
                        0});
  g_type_query (parent, &parent_info);
  return parent_info;
}

static GType
bm_class_peek_mixed_type (gpointer class, const GType mixin) {
  GType ret;
  GType big_mixed_type_type = G_TYPE_FROM_CLASS (class);

  ret = bm_type_peek_mixed_type (big_mixed_type_type, mixin);
  if (ret == 0)
    g_critical ("Type '%s' doesn't contain mixin '%s'",
                g_type_name (big_mixed_type_type), g_type_name (mixin));

  return ret;
}

gpointer
bm_class_get_mixin (gpointer class, const GType mixin) {
  return class + bm_class_get_parent_info (bm_class_peek_mixed_type (class, mixin)
      ).class_size;
}

gpointer
bm_instance_get_mixin (gpointer instance, const GType mixin) {
  return instance +
      bm_class_get_parent_info (bm_type_peek_mixed_type
                                (G_TYPE_FROM_INSTANCE (instance), mixin)
      ).instance_size;
}
