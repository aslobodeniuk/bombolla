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
#include "bombolla/base/lba-picture.h"

typedef struct _LbaGst {
  BMixinInstance i;

  gchar *pipeline_desc;
  GstElement *pipeline;
  GstElement *appsrc;
} LbaGst;

typedef struct _LbaGstClass {
  BMixinClass c;
} LbaGstClass;

typedef enum {
  PROP_PIPELINE = 1,
  N_PROPERTIES
} LbaGstProperty;

static void
lba_custom_picture_base_init (gpointer class_ptr)
{
  LbaPictureClass *pklass =
      bm_class_get_mixin (class_ptr, lba_picture_get_type ());

  /* TODO: make function lba_picture_class_set_writable_properties() */
  pklass->writable_properties = TRUE;
}

BM_DEFINE_MIXIN (lba_gst, LbaGst, BM_ADD_DEP (lba_picture),
    BM_ADD_BASE_INIT (lba_custom_picture_base_init));

static const struct
{
  const gchar *lba;
  const gchar *gst;
} bunch[] = {
  { "argb8888", "ARGB" },
  { "rgba8888", "RGBA" }
};

static const char*
lba_gst_format_to_gst (const char *fmt)
{
  int i;
  
  for (i = 0; i < G_N_ELEMENTS (bunch); i++)
    if (g_strcmp0 (bunch[i].lba, fmt)) {
      return bunch[i].gst;
    }

  return NULL;
}

static const char*
lba_gst_format_to_lba (const char *fmt)
{
  int i;
  
  for (i = 0; i < G_N_ELEMENTS (bunch); i++)
    if (g_strcmp0 (bunch[i].gst, fmt)) {
      return bunch[i].lba;
    }

  return NULL;
}

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

  GstStructure *s = gst_caps_get_structure (gst_sample_get_caps (samp), 0);
  gint w, h;
  if (s && gst_structure_get_int (s, "width", &w)
      && gst_structure_get_int (s, "height", &h)) {

    const gchar *fmt = lba_gst_format_to_lba (gst_structure_get_string (s, "format"));

    LBA_LOG ("Notifying bytes of size %" G_GSIZE_FORMAT, map.size);

    lba_picture_set (BM_GET_GOBJECT (self), fmt, w, h, bytes);
  }

  gst_sample_unref (samp);
  return GST_FLOW_OK;
}

static void
lba_gst_data_update_cb (GObject * pic,
    GParamSpec * pspec, LbaGst * self) {
  gchar format[16];
  guint w, h;
  GBytes * data;
  
  g_return_if_fail (self->appsrc != NULL);

  /* g_clear_pointer (&self->data, g_bytes_unref); */
  data = (GBytes *) lba_picture_get (pic, format, &w, &h);

  /* So now we push this data to the appsrc */
  {
    GstSample *sample;
    GstBuffer *buf = gst_buffer_new_wrapped_bytes (data);
    GstCaps *caps = gst_caps_new_simple ("video/x-raw",
        "width", G_TYPE_INT, w,
          "height", G_TYPE_INT, h,
          "format", G_TYPE_STRING, lba_gst_format_to_gst (format),
          NULL);
      GstSegment segment;

      gst_segment_init (&segment, GST_FORMAT_TIME);
      sample = gst_sample_new (buf, caps, &segment, NULL);

      /* TODO push only buffer if the format is kept the same */
      /* TODO handle ret value */
      GstFlowReturn ret;
      g_signal_emit_by_name (self->appsrc, "push-sample", sample, &ret);

      gst_sample_unref (sample);
      gst_buffer_unref (buf);
      gst_caps_unref (caps);
    }
}

static void
lba_gst_pipeline_update (LbaGst * self, const gchar * desc) {
  GstElement *appsink;
  
  g_free (self->pipeline_desc);
  self->pipeline_desc = g_strdup (desc);

  if (self->pipeline) {
    gst_element_set_state (self->pipeline, GST_STATE_NULL);
    gst_object_unref (self->pipeline);
  }

  LBA_LOG ("Starting the pipeline '%s'", self->pipeline_desc);
  self->pipeline = gst_parse_launch (self->pipeline_desc, NULL);

  g_clear_pointer (&self->appsrc, gst_object_unref);
  self->appsrc = gst_bin_get_by_name (GST_BIN (self->pipeline), "lba_in");
  if (self->appsrc) {
    /* NOTE: this is a hack, we must have input pictures and output pictures */
    g_signal_connect (BM_GET_GOBJECT (self), "notify::data",
        G_CALLBACK (lba_gst_data_update_cb), self);
  }

  appsink = gst_bin_get_by_name (GST_BIN (self->pipeline), "lba_out");
  if (appsink) {
    g_signal_connect (appsink, "new-sample", G_CALLBACK (lba_gst_new_sample), self);

    g_warn_if_fail (self->appsrc == NULL || appsink == NULL);
    gst_object_unref (appsink);
  }

  gst_element_set_state (self->pipeline, GST_STATE_PLAYING);
}

static void
lba_gst_set_property (GObject * object,
                      guint property_id, const GValue * value, GParamSpec * pspec) {
  LbaGst *self = bm_get_LbaGst (object);

  switch ((LbaGstProperty) property_id) {
  case PROP_PIPELINE:
    lba_gst_pipeline_update (self, g_value_get_string (value));
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
  LbaGst *self = bm_get_LbaGst (object);

  switch ((LbaGstProperty) property_id) {
  case PROP_PIPELINE:
    g_value_set_string (value, self->pipeline_desc);
    break;
  default:
    /* We don't have any other property... */
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}

static void
lba_gst_init (GObject * gobj, LbaGst * self) {
  /* TODO: input-picture and output-picture?? */
}

static void
lba_gst_dispose (GObject * gobject) {
  LbaGst *self = bm_get_LbaGst (gobject);

  if (self->pipeline) {
    gst_element_set_state (self->pipeline, GST_STATE_NULL);
    g_clear_object (&self->pipeline);
  }

  g_clear_object (&self->appsrc);
  g_clear_pointer (&self->pipeline_desc, g_free);

  BM_CHAINUP (self, GObject)->finalize (gobject);
}

static void
lba_gst_class_init (GObjectClass * gobj_class, LbaGstClass * mixin_class) {
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

  static int gstinit = 0;

  if (gstinit == 0) {
    gstinit = 1;
    gst_init (NULL, NULL);
  }
}

/* Export plugin */
BOMBOLLA_PLUGIN_SYSTEM_PROVIDE_GTYPE (lba_gst);
