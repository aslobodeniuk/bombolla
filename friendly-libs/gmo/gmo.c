/* GObject Class Mutation
 *
 * Copyright (C) 2023 Alexander Slobodeniuk <aslobodeniuk@fluendo.com>
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
 * Experimental approach that we call GObject Mutation,
 * or GMO (G Mutated Object). It bases on the concept of Mutogene.
 * Mutogene is a code that together with some parent can produce
 * a child class, but Mutogene is agnostic to what class is going
 * to be the parent.
 */

#include <gmo/gmo.h>

#define G_TYPE_IS_MUTOGENE(t) (G_TYPE_FUNDAMENTAL (t) == gmo_fundamental_get_type ())

static GTypeInfo empty_info;

GQuark
gmo_info_qrk (void) {
  return g_quark_from_static_string ("GMOInfo");
}

GType
gmo_fundamental_get_type (void) {
  static GType ret;
  GTypeFundamentalInfo finfo = { G_TYPE_FLAG_DERIVABLE };
  if (G_LIKELY (ret))
    return ret;

  return (ret = g_type_register_fundamental (g_type_fundamental_next (),
                                             "GMOMutogene", &empty_info, &finfo, 0));
}

GType
gmo_type_peek_mutant (const GType mutant_type, const GType mutogene) {
  GType t = mutant_type;

  g_assert (mutogene != 0);
  g_assert (t != 0);
  for (;;) {
    GMOInfo *info;

    info = g_type_get_qdata (t, gmo_info_qrk ());
    if (info == 0)
      return 0;

    if (info->type == mutogene)
      return t;

    t = g_type_parent (t);
  }

  return 0;
}

GType
gmo_register_mutant (const gchar * mutant_name, GType base_type, GType t) {
  GType ret;
  GTypeQuery base_info;
  GMOInfo *minfo;
  GTypeInfo mutant_info;
  gint i;
  GTypeFlags result_type_flags = 0;
  gchar *mutant_name_on_fly = NULL;

  g_return_val_if_fail (t, 0);
  g_return_val_if_fail (base_type, 0);

  if (G_UNLIKELY (!G_TYPE_IS_MUTOGENE (t))) {
    g_critical ("Type '%s' is not a mutogene", g_type_name (t));
    return 0;
  }

  minfo = g_type_get_qdata (t, gmo_info_qrk ());
  g_return_val_if_fail (minfo, 0);

  g_return_val_if_fail (0 == minfo->add[0].gtype, 0);
  for (i = 1; minfo->add[i].gtype != 0; i++) {
    switch (minfo->add[i].add_type) {
    case GMO_ADD_TYPE_IFACE:
      /* Will proccess after type regostration */
      continue;
    case GMO_ADD_TYPE_DEP:
      if (minfo->add[i].gtype == GMO_DEP_ANY_SEP_GTYPE) {
        g_error ("Variant dependencies not supported yet");
      } else {
        /*
         * If the dep is GObject..
         * If the dep is Interface..
         * If the dep is Mutogene..
         */
        GType dep_type = minfo->add[i].gtype ();

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

          g_critical ("Mutogene %s needs %s, but this dependency is not satisfied",
                      g_type_name (t), g_type_name (dep_type));
          return 0;
        }

        if (G_TYPE_IS_MUTOGENE (dep_type)) {
          /* Mutogene:
           * So, our dep is a mutogene. Here come 2 scenarios: either base_type already has it,
           * either not. If it's already included to the base class, then we're done, and there's
           * nothing to do.
           * If required mutoge is not contained in the dna yet, then we add it.
           */
          if (gmo_type_peek_mutant (base_type, dep_type)) {
            /* Mutogene found in the base class, nothing to do */
            continue;
          }

          /* Dependency mutogene is not found in the base class: then we add it
           * on top of the base. */
          base_type = gmo_register_mutant (NULL, base_type, dep_type);
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
           * A new mutogene that implements the interface might be added,
           * and then the type is complete.
           * So we don't fail the registration, but set "abstract" flag to the type
           * so the instance of it can't be created on it's own.. */
          result_type_flags |= G_TYPE_FLAG_ABSTRACT;
        }
      }
      break;
    default:
      g_critical ("Unexpected add type %d", minfo->add[i].add_type);
    }
  }

  g_type_query (base_type, &base_info);

  mutant_info = minfo->info;
  mutant_info.class_size += base_info.class_size;
  mutant_info.instance_size += base_info.instance_size;

  if (mutant_name == NULL) {
    mutant_name_on_fly = g_strdup_printf ("%s+%s", g_type_name (base_type),
                                          g_type_name (t));
    mutant_name = mutant_name_on_fly;
  }

  if (g_type_from_name (mutant_name)) {
    /* It might happen that such a mutant already have been created */
    ret = g_type_from_name (mutant_name);
  } else {
    ret =
        g_type_register_static (base_type, mutant_name, &mutant_info,
                                result_type_flags);

    g_type_set_qdata (ret, gmo_info_qrk (), minfo);

    for (i = 1; minfo->add[i].gtype != 0; i++) {
      if (minfo->add[i].add_type == GMO_ADD_TYPE_IFACE) {
        g_type_add_interface_static (ret, minfo->add[i].gtype (),
                                     &minfo->add[i].iface_info);
      }
    }
  }

  if (mutant_name_on_fly) {
    g_free (mutant_name_on_fly);
    mutant_name = mutant_name_on_fly = NULL;
  }

  return ret;
}

GType
gmo_register_mutogene (const gchar * mutogene_name, GMOInfo * minfo) {
  GType ret;

  ret =
      g_type_register_static (gmo_fundamental_get_type (), mutogene_name,
                              &empty_info, 0);

  g_return_val_if_fail (ret != 0, 0);

  g_type_set_qdata (ret, gmo_info_qrk (), minfo);

  return ret;
}

static GTypeQuery
gmo_class_get_parent_info (GType t) {
  GTypeQuery parent_info;
  GType parent = g_type_parent (t);

  g_assert (parent != 0);
  g_type_query (parent, &parent_info);
  return parent_info;
}

static GType
gmo_class_peek_mutant (gpointer class, const GType mutogene) {
  GType ret;
  GType big_mutant_type = G_TYPE_FROM_CLASS (class);

  ret = gmo_type_peek_mutant (big_mutant_type, mutogene);
  if (ret == 0)
    g_critical ("Can't find mutant in class: "
                "Type '%s' doesn't contain mutogene '%s'",
                g_type_name (big_mutant_type), g_type_name (mutogene));

  return ret;
}

gpointer
gmo_class_get_mutogene (gpointer class, const GType mutogene) {
  return class + gmo_class_get_parent_info (gmo_class_peek_mutant (class, mutogene)
      ).class_size;
}

gpointer
gmo_instance_get_mutogene (gpointer instance, const GType mutogene) {
  return instance +
      gmo_class_get_parent_info (gmo_type_peek_mutant
                                 (G_TYPE_FROM_INSTANCE (instance), mutogene)
      ).instance_size;
}
