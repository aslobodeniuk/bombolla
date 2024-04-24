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

#include "lba-basedrawable.h"
#include "bombolla/lba-log.h"

static const gchar *global_lba_plugin_name = "BaseDrawable";

G_DEFINE_TYPE (BaseDrawable, base_drawable, G_TYPE_OBJECT);

typedef enum {
  PROP_DRAWING_SCENE = 1,
  PROP_ENABLED,
  PROP_TEXTURE
} BaseDrawableProperty;

static void
base_drawable_scene_on_draw_cb (GObject * scene, BaseDrawable * self) {
  BaseDrawableClass *klass = BASE_DRAWABLE_GET_CLASS (self);

  if (self->enabled) {
    if (self->texture) {
      g_signal_emit_by_name (self->texture, "set", self);
    }

    klass->draw (self);

    if (self->texture) {
      g_signal_emit_by_name (self->texture, "unset", self);
    }
  }
}

static void
base_drawable_set_property (GObject * object,
                            guint property_id, const GValue * value,
                            GParamSpec * pspec) {
  BaseDrawable *self = (BaseDrawable *) object;

  switch ((BaseDrawableProperty) property_id) {
  case PROP_TEXTURE:
    g_clear_pointer (&self->texture, g_object_unref);
    self->texture = g_value_dup_object (value);
    /* Update if new texture is set */
    if (self->enabled && self->scene && self->texture) {
//      g_signal_emit_by_name (self->texture, "set", NULL);
      g_signal_emit_by_name (self->scene, "request-redraw", NULL);
    }
    break;

  case PROP_DRAWING_SCENE:
    {
      BaseDrawableClass *klass = BASE_DRAWABLE_GET_CLASS (self);

      self->scene = g_value_get_object (value);

      if (klass->draw) {
        g_signal_connect (self->scene, "on-draw",
                          G_CALLBACK (base_drawable_scene_on_draw_cb), self);
      }

      LBA_LOG ("drawing scene set");
    }
    break;

  case PROP_ENABLED:
    {
      gboolean prev_enabled = self->enabled;

      self->enabled = g_value_get_boolean (value);
      LBA_LOG ("enabled = %s", self->enabled ? "TRUE" : "FALSE");

      /* We have registered this property as G_PARAM_EXPLICIT_NOTIFY,
       * to make it emit "notify" signal only when we call
       * g_object_notify () or g_object_notify_by_pspec().
       *
       * This way we emit "notify" signal only if self->enabled have
       * really changed. */
      if (prev_enabled != self->enabled) {
        g_object_notify_by_pspec (object, pspec);

        if (self->scene) {
          g_signal_emit_by_name (self->scene, "request-redraw", NULL);
        }
      }
    }
    break;

  default:
    /* We don't have any other property... */
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}

static void
base_drawable_get_property (GObject * object,
                            guint property_id, GValue * value, GParamSpec * pspec) {
}

static void
base_drawable_init (BaseDrawable * self) {
}

static void
base_drawable_dispose (GObject * gobject) {
  BaseDrawable *self = (BaseDrawable *) gobject;

  g_clear_pointer (&self->texture, g_object_unref);
  G_OBJECT_CLASS (base_drawable_parent_class)->dispose (gobject);
}

static void
base_drawable_class_init (BaseDrawableClass * klass) {
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->set_property = base_drawable_set_property;
  object_class->get_property = base_drawable_get_property;
  object_class->dispose = base_drawable_dispose;

  g_object_class_install_property (object_class, PROP_TEXTURE,
                                   g_param_spec_object ("texture",
                                                        "Texture",
                                                        "Texture that will be used for this object",
                                                        G_TYPE_OBJECT,
                                                        G_PARAM_STATIC_STRINGS |
                                                        G_PARAM_READWRITE));

  /* NOTE: notify:: signal works only if parameter is readable */
  g_object_class_install_property (object_class, PROP_DRAWING_SCENE,
                                   g_param_spec_object ("drawing-scene",
                                                        "Drawing Scene",
                                                        "Scene that triggers drawing of the object",
                                                        G_TYPE_OBJECT,
                                                        G_PARAM_STATIC_STRINGS |
                                                        G_PARAM_READWRITE));

  g_object_class_install_property (object_class, PROP_ENABLED,
                                   g_param_spec_boolean ("enabled",
                                                         "Enabled", "On/Off",
                                                         TRUE,
                                                         G_PARAM_STATIC_STRINGS |
                                                         G_PARAM_READWRITE |
                                                         G_PARAM_CONSTRUCT |
                                                         G_PARAM_EXPLICIT_NOTIFY));
}

BOMBOLLA_PLUGIN_SYSTEM_PROVIDE_GTYPE (base_drawable);
