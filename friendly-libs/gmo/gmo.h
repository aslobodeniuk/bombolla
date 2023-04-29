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

#include <glib-object.h>

typedef struct {
  GTypeInfo info;
} GMOInfo;

GType gmo_register_mutant (const gchar * mutant_name,
                           const GType base_type, const gchar * mutogene_name);

GType gmo_register_mutogene (const gchar * mutogene_name, GMOInfo * minfo);

gpointer gmo_class_get_mutogene (gpointer class);

gpointer gmo_instance_get_mutogene (gpointer instance);

#define GMO_DEFINE_MUTOGENE(name, Name)                           \
  static void                                                     \
  name##_proxy_class_init (gpointer class, gpointer p)            \
  {                                                               \
    name##_class_init (class, gmo_class_get_mutogene (class));    \
  }                                                               \
                                                                  \
  static void                                                     \
  name##_proxy_init (GTypeInstance * instance, gpointer p)        \
  {                                                               \
  name##_init (instance, gmo_class_get_mutogene (instance));      \
  }                                                               \
                                                                  \
   static GMOInfo name##_info = {                                 \
     .info = {                                                    \
       .class_init = name##_proxy_class_init,                     \
       .instance_init = name##_proxy_init,                        \
       .class_size = sizeof (Name##Class),                        \
       .instance_size = sizeof (Name)                             \
     }                                                            \
   };                                                             \
                                                                  \
   GType name##_get_type (void) {                                 \
     static GType ret;                                            \
     if (G_LIKELY (ret))                                          \
       return ret;                                                \
     return (ret = gmo_register_mutogene (#name, &name##_info));  \
   }
