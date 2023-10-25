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

static GTypeInfo empty_info;

GQuark
gmo_info_qrk (void) {
  return g_quark_from_static_string ("GMOInfo");
}

static GType
gmo_base_get_type (void) {
  static GType ret;
  GTypeFundamentalInfo finfo = { G_TYPE_FLAG_DERIVABLE };
  if (G_LIKELY (ret))
    return ret;

  return (ret = g_type_register_fundamental (g_type_fundamental_next (),
                                             "GMOBase", &empty_info, &finfo, 0));
}

GType
gmo_register_mutant (const gchar * mutant_name,
                     const GType base_type, const gchar * mutogene_name) {
  GType t;
  GType ret;
  GTypeQuery base_info;
  GMOInfo *minfo;
  GTypeInfo mutant_info;
  gint i;

  g_return_val_if_fail (base_type, 0);

  t = g_type_from_name (mutogene_name);

  if (0 == t) {
    g_warning ("Mutogene '%s' not found", mutogene_name);
    return 0;
  }

  if (gmo_base_get_type () != g_type_parent (t)) {
    g_warning ("Type '%s' found but it's not a mutogene", mutogene_name);
    return 0;
  }

  minfo = g_type_get_qdata (t, gmo_info_qrk ());
  g_return_val_if_fail (minfo, 0);

  g_type_query (base_type, &base_info);

  mutant_info = minfo->info;
  mutant_info.class_size += base_info.class_size;
  mutant_info.instance_size += base_info.instance_size;

  ret = g_type_register_static (base_type, mutant_name, &mutant_info, 0);
  g_type_set_qdata (ret, gmo_info_qrk (), minfo);

  g_assert (0 == minfo->add[0].gtype);
  for (i = 1; minfo->add[i].gtype != 0; i++) {
    switch (minfo->add[i].add_type) {
    case GMO_ADD_TYPE_IFACE:
      g_type_add_interface_static (ret, minfo->add[i].gtype (),
                                   &minfo->add[i].iface_info);
      break;
    case GMO_ADD_TYPE_FRIEND:
      g_critical
          ("TODO: Friend means this Mutogene brings this one too, automatically."
           " Friend will be added to the inheritance tree before the actual mutogene.");
      break;
    case GMO_ADD_TYPE_REQ:
      g_critical ("TODO: Requirement means that the base type MUST be of this type."
                  " Can also require one of multiple types in the list.");
      break;
    default:
      g_critical ("Unexpected add type %d", minfo->add[i].add_type);
    }
  }
  return ret;
}

GType
gmo_register_mutogene (const gchar * mutogene_name, GMOInfo * minfo) {
  GType ret;

  ret = g_type_register_static (gmo_base_get_type (), mutogene_name, &empty_info, 0);

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

GType
gmo_type_peek_mutant (const GType mutant_type, const GType mutogene) {
  GType t = mutant_type;

  g_assert (mutogene != 0);
  g_assert (t != 0);
  for (;;) {
    GMOInfo *info;

    info = g_type_get_qdata (t, gmo_info_qrk ());
    g_assert (info != 0);

    if (info->type == mutogene)
      return t;

    t = g_type_parent (t);
  }

  g_error ("Mutogene '%s' offset not found in type '%s'",
           g_type_name (mutogene), g_type_name (mutant_type));
  return 0;
}

static GType
gmo_class_peek_mutant (gpointer class, const GType mutogene) {
  return gmo_type_peek_mutant (G_TYPE_FROM_CLASS (class), mutogene);
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
