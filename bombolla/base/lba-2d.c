/* la Bombolla GObject shell
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
