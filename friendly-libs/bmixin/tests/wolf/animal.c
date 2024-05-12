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

#include "animal.h"

typedef struct {
  BMixinInstance i;
} Animal;

BM_DEFINE_MIXIN (animal, Animal);

static void
animal_init (GObject * object, Animal * self) {
  g_message ("animal_init");
}

typedef enum {
  PROP_IS_DOMESTIC = 1
} AnimalProperty;

static void
animal_get_property (GObject * object,
                     guint property_id, GValue * value, GParamSpec * pspec) {
  Animal *self = bm_get_Animal (object);

  switch ((AnimalProperty) property_id) {
  case PROP_IS_DOMESTIC:
    g_value_set_boolean (value,
                         G_TYPE_INSTANCE_GET_CLASS (self, animal_get_type (),
                                                    AnimalClass)->is_domestic);
    break;
  default:
    /* We don't have any other property... */
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}

static void
animal_class_init (GObjectClass * gobject_class, AnimalClass * klass) {
  g_message ("animal_class_init");

  gobject_class->get_property = animal_get_property;
  klass->is_domestic = TRUE;

  g_object_class_install_property (gobject_class, PROP_IS_DOMESTIC,
                                   g_param_spec_boolean ("is-domestic",
                                                         "Is domestic",
                                                         "If this animal is domestic",
                                                         TRUE,
                                                         G_PARAM_STATIC_STRINGS |
                                                         G_PARAM_READABLE));
}
