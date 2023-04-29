/* la Bombolla GObject shell.
 *
 * Copyright (C) 2023 Alexander Slobodeniuk <aleksandr.slobodeniuk@gmx.es>
 *
 *   This file is part of bombolla.
 *
 *   Bombolla is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   Bombolla is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with bombolla.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <gmo/gmo.h>
#include "bombolla/lba-log.h"

typedef struct _LbaMutogene3D {
  double x,
    y,
    z;
} LbaMutogene3D;

typedef struct _LbaMutogene3DClass {
  int dummy;
} LbaMutogene3DClass;

typedef enum {
  PROP_X = 1,
  PROP_Y,
  PROP_Z
} LbaMutogene3DProperty;

static void
lba_mutogene_3d_set_property (GObject * object,
                              guint property_id, const GValue * value,
                              GParamSpec * pspec) {

  LbaMutogene3D *self = gmo_instance_get_mutogene (object);

  switch ((LbaMutogene3DProperty) property_id) {
  case PROP_X:
    self->x = g_value_get_double (value);
    break;

  case PROP_Y:
    self->y = g_value_get_double (value);
    break;

  case PROP_Z:
    self->z = g_value_get_double (value);
    break;

  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}

static void
lba_mutogene_3d_get_property (GObject * object,
                              guint property_id, GValue * value,
                              GParamSpec * pspec) {
  LbaMutogene3D *self = gmo_instance_get_mutogene (object);

  switch ((LbaMutogene3DProperty) property_id) {
  case PROP_X:
    g_value_set_double (value, self->x);
    break;

  case PROP_Y:
    g_value_set_double (value, self->y);
    break;

  case PROP_Z:
    g_value_set_double (value, self->z);
    break;

  default:
    /* We don't have any other property... */
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
lba_mutogene_3d_init (gpointer object, gpointer mutogene) {
}

static void
lba_mutogene_3d_class_init (GObjectClass * object_class, gpointer gmo_class) {

  object_class->set_property = lba_mutogene_3d_set_property;
  object_class->get_property = lba_mutogene_3d_get_property;

  g_object_class_install_property
      (object_class,
       PROP_X,
       g_param_spec_double ("x",
                            "X", "X coordinate",
                            -G_MAXDOUBLE, G_MAXDOUBLE,
                            0.5,
                            G_PARAM_STATIC_STRINGS |
                            G_PARAM_WRITABLE | G_PARAM_CONSTRUCT));

  g_object_class_install_property
      (object_class,
       PROP_Y,
       g_param_spec_double ("y",
                            "Y", "Y coordinate",
                            -G_MAXDOUBLE, G_MAXDOUBLE,
                            0.5,
                            G_PARAM_STATIC_STRINGS |
                            G_PARAM_WRITABLE | G_PARAM_CONSTRUCT));

  g_object_class_install_property
      (object_class,
       PROP_Z,
       g_param_spec_double ("z",
                            "Z", "Z coordinate",
                            -G_MAXDOUBLE, G_MAXDOUBLE,
                            0.5,
                            G_PARAM_STATIC_STRINGS |
                            G_PARAM_WRITABLE | G_PARAM_CONSTRUCT));
}

GMO_DEFINE_MUTOGENE (lba_mutogene_3d, LbaMutogene3D);
BOMBOLLA_PLUGIN_SYSTEM_PROVIDE_GTYPE (lba_mutogene_3d);
