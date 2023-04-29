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

static GQuark
gmo_info_qrk (void) {
  return g_quark_from_string ("GMOInfo");
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
  GTypeQuery base_info;
  const GMOInfo *minfo;
  GTypeInfo mutant_info;

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

  return g_type_register_static (base_type, mutant_name, &mutant_info, 0);
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
gmo_class_get_parent_info (gpointer class) {
  GTypeQuery parent_info;
  GType parent = g_type_parent (G_TYPE_FROM_CLASS (class));

  g_assert (parent != 0);
  g_type_query (parent, &parent_info);
  return parent_info;
}

gpointer
gmo_class_get_mutogene (gpointer class) {
  return class + gmo_class_get_parent_info (class).class_size;
}

gpointer
gmo_instance_get_mutogene (gpointer instance) {
  return instance +
      gmo_class_get_parent_info
      (((GTypeInstance *) instance)->g_class).instance_size;
}
