/* la Bombolla GObject shell.
 * Copyright (C) 2021 Aleksandr Slobodeniuk
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

#include "lba-remote-object-private.h"
#include "lba-remote-object-protocol.h"
#include "bombolla/lba-plugin-system.h"
#include "bombolla/lba-log.h"
#include "bombolla/base/lba-byte-stream.h"
#include <string.h>

typedef enum {
  PROP_INPUT_STREAM,
  PROP_OBJECT
} lbaRemoteObjectClientProperty;

typedef struct _lbaRemoteObjectClient {
  GObject parent;

  LbaByteStream *input_stream;
  lbaRemoteObject *object;

  lbaRemoteObjectClassData *object_class_data;
  GType obj_type;
} lbaRemoteObjectClient;

typedef struct _lbaRemoteObjectClientClass {
  GObjectClass parent;
} lbaRemoteObjectClientClass;

G_DEFINE_TYPE (lbaRemoteObjectClient, lba_remote_object_client, G_TYPE_OBJECT);

static void
lba_remote_object_client_init (lbaRemoteObjectClient * self) {
}

#define MAGIC(c0, c1, c2, c3) \
  ( (guint32)(c0) | ((guint32) (c1)) << 8  | ((guint32) (c2)) << 16 | ((guint32) (c3)) << 24 )

static void
lba_remote_object_client_prepare_object (lbaRemoteObjectClient * self,
                                         const guchar * msg, guint msg_size) {
  guint8 number_of_signals = 0,
      number_of_properties = 0;
  gchar *class_name;
  const gchar *class_name_tmp;
  lbaRemoteObjectProperty *properties = NULL;
  lbaRemoteObjectSignal *signals = NULL;

  /*
     Protocol is:
     [dump] - magic, 4 bytes
     [class name size] - 1 byte
     [class name] - zero-terminated string
     [number of signals] - 1 byte

     for each signal:
     [signal name size] - 1 byte
     [signal name] - zero-terminated string
     [signal id] - 4 bytes
     [signal flags] - 4 bytes
     [gtype name size] - 1 byte
     [gtype name] - name of return type

     [number of parameters] - 1 byte
     for each parameter:
     [gtype name size] - 1 byte
     [gtype name] - zero-terminated string

     [number of properties] - 1 byte
     for each property:
     [property name size] - 1 byte
     [property name] - zero-terminated string
     [gtype name size] - 1 byte
     [gtype name] - zero-terminated string
     [flags] - 4 bytes
     [param spec]
     [current value size] - 4 bytes
     [current value] - bytes
   */

  msg = lba_robj_protocol_parse_name (msg, &class_name_tmp);
  class_name = g_strdup_printf ("lbaRemoteObject_%s", class_name_tmp);

  number_of_signals = msg[0];
  msg += 1;

  if (number_of_signals) {
    gint i;

    signals = g_new0 (lbaRemoteObjectSignal, number_of_signals);

    /* for each signal: */
    for (i = 0; i < number_of_signals; i++) {
      guint8 p;
      const gchar *name;

      msg = lba_robj_protocol_parse_name (msg, &name);
      msg = lba_robj_protocol_parse_uint (msg, &signals[i].id);
      msg = lba_robj_protocol_parse_uint (msg, &signals[i].flags);

      signals[i].name = g_strdup (name);

      msg = lba_robj_protocol_parse_gtype (msg, &signals[i].return_type);
      g_assert (signals[i].return_type);

      /* [number of parameters] - 1 byte */
      signals[i].n_params = msg[0];
      msg += 1;

      signals[i].param_types = g_new (GType, signals[i].n_params);

      /* for each parameter: */
      for (p = 0; p < signals[i].n_params; p++) {
        /* parameter type */
        msg = lba_robj_protocol_parse_gtype (msg, &signals[i].param_types[p]);
      }
    }

    /* [number of properties] - 1 byte */
    number_of_properties = msg[0];
    msg += 1;

    properties = g_new (lbaRemoteObjectProperty, number_of_properties);

    /* for each property: */
    for (i = 0; i < number_of_properties; i++) {
      GValue cur_value;

      msg = lba_robj_protocol_parse_pspec (msg, &properties[i].pspec);
      msg =
          lba_robj_protocol_parse_gvalue (msg, properties[i].pspec->value_type,
                                          &cur_value);
      /* TODO: store cur_value and set it to the object instance */
    }
  }

  /* Now we should register a new class with captured signals and properties.
   * FIXME: if class already exists, we need different behaviour */

  self->object_class_data = g_new (lbaRemoteObjectClassData, 1);
  self->object_class_data->signals = signals;
  self->object_class_data->signals_num = number_of_signals;
  self->object_class_data->properties = properties;
  self->object_class_data->properties_num = number_of_properties;

  LBA_LOG ("Creating %s", class_name);

  {
    GTypeInfo t_info = {
      .class_size = remote_object_class_size,
      .class_init = (GClassInitFunc) lba_remote_object_class_init,
      .instance_size = remote_object_instance_size,
      .instance_init = (GInstanceInitFunc) lba_remote_object_init,
      .class_data = self->object_class_data
    };

    self->obj_type = g_type_register_static (G_TYPE_OBJECT, class_name, &t_info, 0);
    g_assert (self->obj_type);
  }

  /* Now create an instance of this object and set it up */
  self->object = g_object_new (self->obj_type, NULL);
  g_assert (self->object);

  lba_remote_object_setup (self->object, self, self->obj_type);
}

static void
lba_remote_object_client_have_data (LbaByteStream * stream, GBytes * bytes,
                                    lbaRemoteObjectClient * self) {
  guint32 magic;
  const guchar *msg;
  gsize msg_size;

  msg = g_bytes_get_data (bytes, &msg_size);

  g_assert (msg_size > 5);

  LBA_LOG ("Receiving %" G_GSIZE_FORMAT " bytes of data, message [%c%c%c%c]",
           msg_size, msg[0], msg[1], msg[2], msg[3]);

  msg = lba_robj_protocol_parse_uint (msg, &magic);

  switch (magic) {
  case MAGIC ('d', 'u', 'm', 'p'):
    lba_remote_object_client_prepare_object (self, msg, msg_size);
    return;
  case MAGIC ('p', 'n', 'o', 't'):
    LBA_LOG ("TODO");
    return;
  }
}

static void
lba_remote_object_client_set_property (GObject * object,
                                       guint property_id, const GValue * value,
                                       GParamSpec * pspec) {
  lbaRemoteObjectClient *self = (lbaRemoteObjectClient *) object;

  switch ((lbaRemoteObjectClientProperty) property_id) {
  case PROP_INPUT_STREAM:

    {
      GObject *obj;

      obj = g_value_get_object (value);

      if (!LBA_IS_BYTE_STREAM (obj)) {
        /* Fixme: write it's own param_spec ?? */
        g_warning ("Object is not an LbaByteStream");
        break;
      }

      /* Changing the input on fly is not supported yet */
      g_assert (!self->input_stream);

      self->input_stream = (LbaByteStream *) obj;

      if (self->input_stream) {
        g_object_ref (self->input_stream);
        g_signal_connect (self->input_stream, "on-have-data",
                          G_CALLBACK (lba_remote_object_client_have_data), self);
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
lba_remote_object_client_get_property (GObject * object,
                                       guint property_id, GValue * value,
                                       GParamSpec * pspec) {
  lbaRemoteObjectClient *self = (lbaRemoteObjectClient *) object;

  switch ((lbaRemoteObjectClientProperty) property_id) {

  case PROP_INPUT_STREAM:
    g_value_set_object (value, self->input_stream);
    break;
  case PROP_OBJECT:
    g_value_set_object (value, self->object);
    break;
  default:
    /* We don't have any other property... */
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}

static void
lba_remote_object_client_dispose (GObject * gobject) {
  lbaRemoteObjectClient *self = (lbaRemoteObjectClient *) gobject;

  if (self->object) {
    g_object_unref (self->object);
  }

  if (self->input_stream) {
    g_object_unref (self->input_stream);
  }

  G_OBJECT_CLASS (lba_remote_object_client_parent_class)->dispose (gobject);
}

static void
lba_remote_object_client_class_init (lbaRemoteObjectClientClass * klass) {
  GObjectClass *object_class = (GObjectClass *) klass;

  object_class->set_property = lba_remote_object_client_set_property;
  object_class->get_property = lba_remote_object_client_get_property;
  object_class->dispose = lba_remote_object_client_dispose;

  g_object_class_install_property (object_class,
                                   PROP_OBJECT,
                                   g_param_spec_object ("object",
                                                        "Object",
                                                        "Object that represents the one server monitors",
                                                        G_TYPE_OBJECT,
                                                        G_PARAM_STATIC_STRINGS |
                                                        G_PARAM_READABLE));

  g_object_class_install_property (object_class,
                                   PROP_INPUT_STREAM,
                                   g_param_spec_object ("input-stream",
                                                        "Input Stream",
                                                        "Stream for data input",
                                                        G_TYPE_OBJECT,
                                                        G_PARAM_STATIC_STRINGS |
                                                        G_PARAM_READWRITE));

}

/* Export plugin */
BOMBOLLA_PLUGIN_SYSTEM_PROVIDE_GTYPE (lba_remote_object_client);
