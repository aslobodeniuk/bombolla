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

#include <bmixin/bmixin.h>
#include "bombolla/lba-plugin-system.h"
#include "bombolla/lba-log.h"
#include "bombolla/base/i3d.h"

typedef struct _LbaMixin3D {
  double x,
    y,
    z;
  // TODO: lock
} LbaMixin3D;

typedef struct _LbaMixin3DClass {
  int dummy;
} LbaMixin3DClass;

static void
  lba_mixin_i3d_init (LbaI3D * iface);

BM_DEFINE_MIXIN (lba_mixin_3d, LbaMixin3D, BM_IMPLEMENTS_IFACE (lba, mixin, i3d));

typedef enum {
  PROP_X = 1,
  PROP_Y,
  PROP_Z
} LbaMixin3DProperty;

static void
lba_mixin_3d_xyz (GObject * object, gdouble * x, gdouble * y, gdouble * z) {
  LbaMixin3D *self = bm_get_LbaMixin3D (object);

  if (x)
    *x = self->x;
  if (y)
    *y = self->y;
  if (z)
    *z = self->z;
}

static void
lba_mixin_i3d_init (LbaI3D * iface) {
  iface->xyz = lba_mixin_3d_xyz;
}

static void
lba_mixin_3d_set_property (GObject * object,
                           guint property_id, const GValue * value,
                           GParamSpec * pspec) {
  LbaMixin3D *self = bm_get_LbaMixin3D (object);

  switch ((LbaMixin3DProperty) property_id) {
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
lba_mixin_3d_get_property (GObject * object,
                           guint property_id, GValue * value, GParamSpec * pspec) {
  LbaMixin3D *self = bm_get_LbaMixin3D (object);

  switch ((LbaMixin3DProperty) property_id) {
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
lba_mixin_3d_init (GObject * object, LbaMixin3D * self) {
}

static void
lba_mixin_3d_class_init (GObjectClass * object_class, LbaMixin3DClass * mixin_class) {

  object_class->set_property = lba_mixin_3d_set_property;
  object_class->get_property = lba_mixin_3d_get_property;

  g_object_class_install_property
      (object_class,
       PROP_X,
       g_param_spec_double ("x",
                            "X", "X coordinate",
                            -G_MAXDOUBLE, G_MAXDOUBLE,
                            0.5,
                            G_PARAM_STATIC_STRINGS |
                            G_PARAM_READWRITE | G_PARAM_CONSTRUCT));

  g_object_class_install_property
      (object_class,
       PROP_Y,
       g_param_spec_double ("y",
                            "Y", "Y coordinate",
                            -G_MAXDOUBLE, G_MAXDOUBLE,
                            0.5,
                            G_PARAM_STATIC_STRINGS |
                            G_PARAM_READWRITE | G_PARAM_CONSTRUCT));

  g_object_class_install_property
      (object_class,
       PROP_Z,
       g_param_spec_double ("z",
                            "Z", "Z coordinate",
                            -G_MAXDOUBLE, G_MAXDOUBLE,
                            0.5,
                            G_PARAM_STATIC_STRINGS |
                            G_PARAM_READWRITE | G_PARAM_CONSTRUCT));
}

BOMBOLLA_PLUGIN_SYSTEM_PROVIDE_GTYPE (lba_mixin_3d);
