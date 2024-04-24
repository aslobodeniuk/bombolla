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
#include "bombolla/base/i2d.h"

typedef struct _LbaMixin2D {
  BMixinInstance i;
  double x,
    y;
  // TODO: lock
} LbaMixin2D;

typedef struct _LbaMixin2DClass {
  BMixinClass c;
} LbaMixin2DClass;

static void
  lba_mixin_i2d_init (LbaI2D * iface);

BM_DEFINE_MIXIN (lba_mixin_2d, LbaMixin2D, BM_ADD_IFACE (lba, mixin, i2d));

typedef enum {
  PROP_X = 1,
  PROP_Y,
} LbaMixin2DProperty;

static void
lba_mixin_2d_xy (GObject * object, gdouble * x, gdouble * y) {
  LbaMixin2D *self = bm_get_LbaMixin2D (object);

  if (x)
    *x = self->x;
  if (y)
    *y = self->y;
}

static void
lba_mixin_i2d_init (LbaI2D * iface) {
  iface->xy = lba_mixin_2d_xy;
}

static void
lba_mixin_2d_set_property (GObject * object,
                           guint property_id, const GValue * value,
                           GParamSpec * pspec) {
  LbaMixin2D *self = bm_get_LbaMixin2D (object);

  switch ((LbaMixin2DProperty) property_id) {
  case PROP_X:
    self->x = g_value_get_double (value);
    break;

  case PROP_Y:
    self->y = g_value_get_double (value);
    break;

  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}

static void
lba_mixin_2d_get_property (GObject * object,
                           guint property_id, GValue * value, GParamSpec * pspec) {
  LbaMixin2D *self = bm_get_LbaMixin2D (object);

  switch ((LbaMixin2DProperty) property_id) {
  case PROP_X:
    g_value_set_double (value, self->x);
    break;

  case PROP_Y:
    g_value_set_double (value, self->y);
    break;

  default:
    /* We don't have any other property... */
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
lba_mixin_2d_init (GObject * object, LbaMixin2D * self) {
}

static void
lba_mixin_2d_class_init (GObjectClass * object_class, LbaMixin2DClass * mixin_class) {

  object_class->set_property = lba_mixin_2d_set_property;
  object_class->get_property = lba_mixin_2d_get_property;

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
}

BOMBOLLA_PLUGIN_SYSTEM_PROVIDE_GTYPE (lba_mixin_2d);
