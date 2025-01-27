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
#include <gst/gst.h>
#include <gio/gio.h>

typedef struct _LbaGst {
  GObject parent;
  gchar *pipeline_desc;
  GstElement *pipeline;

  /* Picture mixin */
  gint w;
  gint h;
  gchar *format;
  GBytes *data;
} LbaGst;

typedef struct _LbaGstClass {
  GObjectClass parent;
} LbaGstClass;

typedef enum {
  PROP_PIPELINE = 1,
  PROP_H,
  PROP_W,
  PROP_DATA,
  PROP_FORMAT,
  N_PROPERTIES
} LbaGstProperty;

G_DEFINE_TYPE (LbaGst, lba_gst, G_TYPE_OBJECT);

static GstFlowReturn
lba_gst_new_sample (GstElement * object, gpointer user_data) {
  LbaGst *self = (LbaGst *) user_data;
  GstSample *samp = NULL;
  GstBuffer *buffer;
  GstMapInfo map;

  g_signal_emit_by_name (object, "pull-sample", &samp);
  buffer = gst_sample_get_buffer (samp);

  gst_buffer_map (buffer, &map, GST_MAP_READ);
  /* This buffer will travel with the data through the LbaPicture mixin */
  GBytes *bytes = g_bytes_new_with_free_func (map.data,
                                              map.size,
                                              (GDestroyNotify) gst_buffer_unref,
                                              gst_buffer_ref (buffer));

  gst_buffer_unmap (buffer, &map);

  /* set w and h from caps */
  GstStructure *s = gst_caps_get_structure (gst_sample_get_caps (samp), 0);

  if (s && gst_structure_get_int (s, "width", &self->w)
      && gst_structure_get_int (s, "height", &self->h)) {
    /* TODO: format. For now we stick with RGBA */
    if (self->data)
      g_bytes_unref (self->data);
    self->data = bytes;

    LBA_LOG ("Notifying bytes of size %" G_GSIZE_FORMAT, map.size);
    g_object_notify (G_OBJECT (self), "data");
  }

  gst_sample_unref (samp);
  return GST_FLOW_OK;
}

static void
lba_gst_pipeline_update (LbaGst * self, const gchar * desc) {
  g_free (self->pipeline_desc);
  self->pipeline_desc = g_strdup (desc);

  if (self->pipeline) {
    gst_element_set_state (self->pipeline, GST_STATE_NULL);
    gst_object_unref (self->pipeline);
  }

  LBA_LOG ("Starting the pipeline '%s'", self->pipeline_desc);
  self->pipeline = gst_parse_launch (self->pipeline_desc, NULL);

  GstElement *appsink = gst_bin_get_by_name (GST_BIN (self->pipeline), "lba_out");

  g_signal_connect (appsink, "new-sample", G_CALLBACK (lba_gst_new_sample), self);
  gst_object_unref (appsink);

  gst_element_set_state (self->pipeline, GST_STATE_PLAYING);
}

static void
lba_gst_set_property (GObject * object,
                      guint property_id, const GValue * value, GParamSpec * pspec) {
  LbaGst *self = (LbaGst *) object;

  switch ((LbaGstProperty) property_id) {
  case PROP_PIPELINE:
    lba_gst_pipeline_update (self, g_value_get_string (value));
    break;
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
lba_gst_get_property (GObject * object,
                      guint property_id, GValue * value, GParamSpec * pspec) {
  LbaGst *self = (LbaGst *) object;

  switch ((LbaGstProperty) property_id) {
  case PROP_PIPELINE:
    g_value_set_string (value, self->pipeline_desc);
    break;
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
lba_gst_init (LbaGst * self) {
}

static void
lba_gst_dispose (GObject * gobject) {
  LbaGst *self = (LbaGst *) gobject;

  if (self->pipeline) {
    gst_element_set_state (self->pipeline, GST_STATE_NULL);
    g_clear_object (&self->pipeline);
  }

  g_clear_pointer (&self->format, g_free);
  g_clear_pointer (&self->pipeline_desc, g_free);
  g_clear_pointer (&self->data, g_bytes_unref);

  G_OBJECT_CLASS (lba_gst_parent_class)->dispose (gobject);
}

static void
lba_gst_class_init (LbaGstClass * klass) {
  GObjectClass *gobj_class = G_OBJECT_CLASS (klass);

  gobj_class->dispose = lba_gst_dispose;
  gobj_class->set_property = lba_gst_set_property;
  gobj_class->get_property = lba_gst_get_property;

  g_object_class_install_property (gobj_class, PROP_PIPELINE,
                                   g_param_spec_string ("pipeline",
                                                        "Pipeline",
                                                        "Pipeline",
                                                        "videotestsrc ! videoconvert ! appsink name=lba_out emit-signals=true",
                                                        G_PARAM_STATIC_STRINGS |
                                                        G_PARAM_READWRITE));

  g_object_class_install_property
      (gobj_class,
       PROP_W,
       g_param_spec_uint ("width",
                          "W", "Width",
                          2, 2048,
                          32,
                          G_PARAM_STATIC_STRINGS |
                          G_PARAM_READWRITE | G_PARAM_CONSTRUCT));

  g_object_class_install_property
      (gobj_class,
       PROP_H,
       g_param_spec_uint ("height",
                          "H", "Height",
                          2, 2048,
                          32,
                          G_PARAM_STATIC_STRINGS |
                          G_PARAM_READWRITE | G_PARAM_CONSTRUCT));

  g_object_class_install_property
      (gobj_class,
       PROP_DATA,
       g_param_spec_boxed ("data",
                           "Data", "Data",
                           G_TYPE_BYTES, G_PARAM_STATIC_STRINGS | G_PARAM_READABLE));

  g_object_class_install_property (gobj_class, PROP_FORMAT,
                                   g_param_spec_string ("format",
                                                        "Format",
                                                        "Format",
                                                        "rgba8888",
                                                        G_PARAM_STATIC_STRINGS |
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));

  static int gstinit = 0;

  if (gstinit == 0) {
    gstinit = 1;
    gst_init (NULL, NULL);
  }
}

/* Export plugin */
BOMBOLLA_PLUGIN_SYSTEM_PROVIDE_GTYPE (lba_gst);
