/* la Bombolla GObject shell
 *
 * Copyright (c) 2025, Alexander Slobodeniuk <aleksandr.slobodeniuk@gmx.es>
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

#include "bombolla/lba-plugin-system.h"
#include "bombolla/lba-log.h"
#include <cairo.h>

typedef struct _LbaCairo {
  GObject parent;

  cairo_surface_t *surface;
  
  /* Picture mixin */
  gint w;
  gint h;
  gchar *format;
  GBytes *data;

  /* ?? should be also basedrawable?? */
} LbaCairo;

typedef struct _LbaCairoClass {
  GObjectClass parent;
} LbaCairoClass;

typedef enum {
  PROP_H = 1,
  PROP_W,
  PROP_DATA,
  PROP_FORMAT,
  N_PROPERTIES
} LbaCairoProperty;

G_DEFINE_TYPE (LbaCairo, lba_cairo, G_TYPE_OBJECT);

static void
lba_cairo_set_property (GObject * object,
                      guint property_id, const GValue * value, GParamSpec * pspec) {
  LbaCairo *self = (LbaCairo *) object;

  switch ((LbaCairoProperty) property_id) {
  case PROP_FORMAT:
    g_free (self->format);
    self->format = g_value_dup_string (value);
    break;
  case PROP_W:
    self->w = g_value_get_uint (value);
    break;
  case PROP_H:
    self->h = g_value_get_uint (value);
    break;
  default:
    /* We don't have any other property... */
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}

static void
lba_cairo_get_property (GObject * object,
                      guint property_id, GValue * value, GParamSpec * pspec) {
  LbaCairo *self = (LbaCairo *) object;

  switch ((LbaCairoProperty) property_id) {
  case PROP_W:
    g_value_set_uint (value, self->w);
    break;
  case PROP_H:
    g_value_set_uint (value, self->h);
    break;
  case PROP_DATA:
    g_value_set_boxed (value, self->data);
    break;
  case PROP_FORMAT:
    g_value_set_string (value, self->format);
    break;
  default:
    /* We don't have any other property... */
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}

static void
lba_cairo_init (LbaCairo * self) {

  /* set LbaPicture things */
  self->w = 512;
  self->h = 512;
  self->format = g_strdup ("rgb888");

  /* Cairo */
  self->surface =
      cairo_image_surface_create (CAIRO_FORMAT_ARGB32, self->h, self->w);

  {
    cairo_t *cr = cairo_create (self->surface);

    /* --------------------------------------- how to?? */
    cairo_select_font_face (cr, "serif", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
    cairo_set_font_size (cr, 32.0);
    cairo_set_source_rgb (cr, 0.0, 0.0, 1.0);
    cairo_move_to (cr, 10.0, 50.0);
    cairo_show_text (cr, "Hello, world");
    /* ---------------------------------------- */

    cairo_destroy (cr);

    /* Must do after modifying the surface */
    cairo_surface_flush (self->surface);
    cairo_surface_mark_dirty (self->surface);
  }

  /* LbaPicture */
  self->data = g_bytes_new_with_free_func (cairo_image_surface_get_data (self->surface),
      self->w * self->h *3,
      /* TODO need to block and unblock when the surface have been rendered
       * (in the OpenGL thread). So we don't modify the surface meanwhile. */
      NULL,
      NULL);
}

static void
lba_cairo_dispose (GObject * gobject) {
  LbaCairo *self = (LbaCairo *) gobject;

  g_clear_pointer (&self->surface, cairo_surface_destroy);

  /* LbaPicture */
  g_clear_pointer (&self->data, g_bytes_unref);
  g_clear_pointer (&self->format, g_free);

  G_OBJECT_CLASS (lba_cairo_parent_class)->dispose (gobject);
}

static void
lba_cairo_class_init (LbaCairoClass * klass) {
  GObjectClass *gobj_class = G_OBJECT_CLASS (klass);

  gobj_class->dispose = lba_cairo_dispose;
  gobj_class->set_property = lba_cairo_set_property;
  gobj_class->get_property = lba_cairo_get_property;

  /* LbaPicture mixin TODO: set if format/w/h are writable through
   * ->writable_format in the base_init.
   * Make BM_BASE_INIT_CODE() macro for that. */
  g_object_class_install_property
      (gobj_class,
       PROP_W,
       g_param_spec_uint ("width",
                          "W", "Width",
                          2, 2048,
                          32,
                          G_PARAM_STATIC_STRINGS |
                          G_PARAM_READWRITE));

  g_object_class_install_property
      (gobj_class,
       PROP_H,
       g_param_spec_uint ("height",
                          "H", "Height",
                          2, 2048,
                          32,
                          G_PARAM_STATIC_STRINGS |
                          G_PARAM_READWRITE));

  g_object_class_install_property
      (gobj_class,
       PROP_DATA,
       g_param_spec_boxed ("data",
                           "Data", "Data",
                           G_TYPE_BYTES, G_PARAM_STATIC_STRINGS | G_PARAM_READABLE));

  /* GType ENUM?? */
  g_object_class_install_property (gobj_class, PROP_FORMAT,
                                   g_param_spec_string ("format",
                                                        "Format",
                                                        "Format",
                                                        "rgba8888",
                                                        G_PARAM_STATIC_STRINGS |
                                                        G_PARAM_READWRITE));
}

/* Export plugin */
BOMBOLLA_PLUGIN_SYSTEM_PROVIDE_GTYPE (lba_cairo);
