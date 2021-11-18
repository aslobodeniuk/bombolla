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

#include "bombolla/lba-plugin-system.h"
#include "bombolla/lba-log.h"
#include "bombolla/base/lba-byte-stream.h"
#include <string.h>

enum {
  SIGNAL_START,
  LAST_SIGNAL
};

static guint lba_remote_object_server_signals[LAST_SIGNAL] = { 0 };

typedef enum {
  PROP_SOURCE = 1,
  PROP_OUTPUT_STREAM
} LbaRemoteObjectServerProperty;

typedef struct _LbaRemoteObjectServer {
  GObject parent;
  GObject *source;
  LbaByteStream *output_stream;
} LbaRemoteObjectServer;

typedef struct _LbaRemoteObjectServerClass {
  GObjectClass parent;

  /* Actions */
  void (*start) (LbaRemoteObjectServer *);

} LbaRemoteObjectServerClass;

G_DEFINE_TYPE (LbaRemoteObjectServer, lba_remote_object_server, G_TYPE_OBJECT);

static void
_str2bytes (const GValue * src_value, GValue * dest_value) {
  const gchar *s;
  GBytes *bytes;

  s = g_value_get_string (src_value);
  bytes = g_bytes_new (s, 1 + strlen (s));

  g_value_set_boxed (dest_value, bytes);
}

G_STATIC_ASSERT (sizeof (gdouble) == 8);
G_STATIC_ASSERT (sizeof (gfloat) == 4);
G_STATIC_ASSERT (sizeof (gint) == 4);
G_STATIC_ASSERT (sizeof (gboolean) == 4);

static void
_double2bytes (const GValue * src_value, GValue * dest_value) {
  gdouble d;
  GBytes *bytes;

  d = g_value_get_double (src_value);
  bytes = g_bytes_new (&d, sizeof (gdouble));

  g_value_set_boxed (dest_value, bytes);
}

static void
_float2bytes (const GValue * src_value, GValue * dest_value) {
  gfloat f;
  GBytes *bytes;

  f = g_value_get_float (src_value);
  bytes = g_bytes_new (&f, sizeof (gfloat));

  g_value_set_boxed (dest_value, bytes);
}

static void
_int2bytes (const GValue * src_value, GValue * dest_value) {
  gint i;
  GBytes *bytes;

  i = g_value_get_int (src_value);
  bytes = g_bytes_new (&i, sizeof (gint));

  g_value_set_boxed (dest_value, bytes);
}

static void
_uint2bytes (const GValue * src_value, GValue * dest_value) {
  guint u;
  GBytes *bytes;

  u = g_value_get_uint (src_value);
  bytes = g_bytes_new (&u, sizeof (guint));

  g_value_set_boxed (dest_value, bytes);
}

static void
_boolean2bytes (const GValue * src_value, GValue * dest_value) {
  gboolean b;
  GBytes *bytes;

  b = g_value_get_boolean (src_value);
  bytes = g_bytes_new (&b, sizeof (gboolean));

  g_value_set_boxed (dest_value, bytes);
}

static GBytes *
lba_remote_object_server_value2bytes (const GValue * src_val) {
  GBytes *ret = NULL;
  GValue dst_val = G_VALUE_INIT;

  if (G_VALUE_TYPE (src_val) == G_TYPE_BYTES) {
    ret = g_value_get_boxed (src_val);
    return ret ? g_bytes_ref (ret) : NULL;
  }

  if (!g_value_type_transformable (G_VALUE_TYPE (src_val), G_TYPE_BYTES)) {
    g_warning ("Type %s can't be transformed to bytes",
               g_type_name (G_VALUE_TYPE (src_val)));
    goto exit;
  }

  g_value_init (&dst_val, G_TYPE_BYTES);

  if (!g_value_transform (src_val, &dst_val)) {
    g_warning ("Couldn't transform");
    goto exit;
  }

  ret = g_value_get_boxed (&dst_val);

  if (ret) {
    g_bytes_ref (ret);
  }
exit:
  g_value_unset (&dst_val);
  return ret;
}

void
lba_remote_object_server_source_param_notify (GObject * gobject,
                                              GParamSpec * pspec,
                                              gpointer user_data) {
  const gchar *pspec_name;
  GValue src_val = G_VALUE_INIT;
  GBytes *bytes = NULL;
  const gchar *type_name;
  LbaRemoteObjectServer *self = (LbaRemoteObjectServer *) user_data;

  /* FIXME: lock mutex */

  pspec_name = g_param_spec_get_name (pspec);

  g_object_getv (gobject, 1, &pspec_name, &src_val);

  type_name = g_type_name (G_VALUE_TYPE (&src_val));

  LBA_LOG ("Notify [%s %s]", type_name, pspec_name);

  bytes = lba_remote_object_server_value2bytes (&src_val);

  if (G_UNLIKELY (!bytes)) {
    g_warning ("No bytes after transformation..");
    goto exit;
  }

  /* Finally, write our value to the output */
  {
    GBytes *message;

    /* The protocol message is:
       [pnot] - message magic of "parameter notify", 4 bytes
       [size] - size of the message (without size and magic), 4 bytes
       [param name] - zero-terminated string
       [bytes] - new parameter value
     */
    gchar notify_magic[4] = { 'p', 'n', 'a', 't' };
    guint msg_body_size,
      msg_size,
      type_name_len,
      msg_size_be;
    gpointer msg;
    gsize bytes_size;
    gconstpointer bytes_ptr;
    GError *err = NULL;

    bytes_ptr = g_bytes_get_data (bytes, &bytes_size);

    type_name_len = strlen (type_name) + 1;

    msg_body_size = type_name_len + bytes_size;
    msg_size = 4 + 4 + msg_body_size;

    msg_size_be = GUINT32_TO_BE (msg_size);

    msg = g_malloc (msg_size);

    memcpy (msg, notify_magic, 4);
    memcpy (msg + 4, &msg_size_be, 4);
    memcpy (msg + 8, type_name, type_name_len);
    memcpy (msg + 8 + type_name_len, bytes_ptr, bytes_size);

    message = g_bytes_new_take (msg, msg_size);

    /* Finally, write the bytes */
    LBA_LOG ("Writing %u bytes", msg_size);
    if (!lba_byte_stream_write_gbytes (self->output_stream, message, &err)) {
      g_critical ("Couldn't write param header: [%s]", err ? err->message : "");
      g_error_free (err);
      goto exit;
    }
  }

exit:
  g_value_unset (&src_val);
  if (bytes)
    g_bytes_unref (bytes);
}

static gboolean
lba_remote_object_server_send_source_header (LbaRemoteObjectServer * self) {
  GError *err = NULL;
  GType t,
    ti;
  GByteArray *msg;
  GBytes *msg_bytes;
  guchar magic[4] = { 'd', 'u', 'm', 'p' };
  guint8 class_name_size;
  guint8 num_signals = 0;

  t = G_OBJECT_TYPE (self->source);

  LBA_LOG ("Dumping gtype %s", g_type_name (t));

  msg = g_byte_array_new ();
  /* Class name actually doesn't matter, but we will still dump it,
   * just for information.

   NOTE: "notify" signal is not included into the dump,
   because when the parameter changes, RemoteObjectClient emits
   it anyway.

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
   [current value size] - 4 bytes
   [current value] - bytes

   FIXME: min/max ? flags? default value?
   */

  /* [dump] */
  g_byte_array_append (msg, magic, 4);

  /* [class name size] - 1 byte */
  class_name_size = strlen (g_type_name (t)) + 1;
  g_byte_array_append (msg, &class_name_size, 1);

  /* [class name] - zero-terminated string */
  g_byte_array_append (msg, (guchar *) g_type_name (t), class_name_size);

  /* Count number of signals */
  for (ti = t; ti; ti = g_type_parent (ti)) {
    guint *signals,
      s,
      n_signals;
    guint has_notify = 0;

    signals = g_signal_list_ids (ti, &n_signals);

    for (s = 0; s < n_signals; s++) {
      GSignalQuery query;

      g_signal_query (signals[s], &query);
      if (0 == g_strcmp0 (query.signal_name, "notify")) {
        has_notify = 1;
      }
    }

    num_signals += n_signals - has_notify;

    g_free (signals);
  }

  LBA_LOG ("Number of signals in dump: %u", num_signals);

  /* [number of signals] - 1 byte */
  g_byte_array_append (msg, &num_signals, 1);

  /* for each signal.. */
  for (ti = t; ti; ti = g_type_parent (ti)) {
    guint *signals,
      s,
      n_signals;

    signals = g_signal_list_ids (ti, &n_signals);

    for (s = 0; s < n_signals; s++) {
      GSignalQuery query;
      guint8 signal_name_size,
        gtype_size,
        params_num;
      guint32 signal_id,
        signal_flags,
        p;

      g_signal_query (signals[s], &query);
      if (0 == g_strcmp0 (query.signal_name, "notify")) {
        LBA_LOG ("skipping 'notify' signal");
        g_assert (ti == G_TYPE_OBJECT);
        continue;
      }

      LBA_LOG ("Dumping signal %s (%u params, id=%d)", query.signal_name,
               query.n_params, query.signal_id);

      /* [signal name size] - 1 byte */
      signal_name_size = strlen (query.signal_name) + 1;
      g_byte_array_append (msg, &signal_name_size, 1);

      /* [signal name] - string */
      g_byte_array_append (msg, (guchar *) query.signal_name, signal_name_size);

      /* [signal id] - 4 bytes */
      signal_id = GUINT32_TO_BE (query.signal_id);
      g_byte_array_append (msg, (guchar *) & signal_id, 4);

      /* [signal flags] - 4 bytes */
      signal_flags = GUINT32_TO_BE (query.signal_flags);
      g_byte_array_append (msg, (guchar *) & signal_flags, 4);

      /* [gtype name size] - 1 byte */
      gtype_size = strlen (g_type_name (query.return_type)) + 1;
      g_byte_array_append (msg, &gtype_size, 1);

      /* [gtype name] - name of return type */
      g_byte_array_append (msg, (guchar *) g_type_name (query.return_type),
                           gtype_size);

      /* [number of parameters] - 1 byte */
      params_num = query.n_params;
      g_byte_array_append (msg, &params_num, 1);

      /* for each parameter: */
      for (p = 0; p < query.n_params; p++) {
        /* [gtype name size] - 1 byte */
        gtype_size = strlen (g_type_name (query.param_types[p])) + 1;
        g_byte_array_append (msg, &gtype_size, 1);

        /* [gtype name] - string */
        g_byte_array_append (msg, (guchar *) g_type_name (query.param_types[p]),
                             gtype_size);
      }
    }

    g_free (signals);
  }

  {
    GParamSpec **properties;
    guint n_properties,
      p;
    GObjectClass *klass;
    guint8 props_num;

    klass = g_type_class_peek (t);

    properties = g_object_class_list_properties (klass, &n_properties);

    LBA_LOG ("Properties in dump %u", n_properties);

    /* [number of properties] - 1 byte */
    props_num = n_properties;
    g_byte_array_append (msg, &props_num, 1);

    /* for each property: */
    for (p = 0; p < n_properties; p++) {
      guint8 type_name_size,
        prop_name_size;
      GValue value = G_VALUE_INIT;
      GBytes *value_bytes;
      guint32 value_bytes_size_be;

      LBA_LOG ("Dumping property [%s %s]", g_type_name (properties[p]->value_type),
               properties[p]->name);

      /* [property name size] - 1 byte */
      prop_name_size = strlen (properties[p]->name) + 1;
      g_byte_array_append (msg, &prop_name_size, 1);

      /* [property name] - zero-terminated string */
      g_byte_array_append (msg, (guchar *) properties[p]->name, prop_name_size);

      /* [gtype name size] - 1 byte */
      type_name_size = strlen (g_type_name (properties[p]->value_type)) + 1;
      g_byte_array_append (msg, &type_name_size, 1);

      /* [gtype name] - zero-terminated string */
      g_byte_array_append (msg, (guchar *) g_type_name (properties[p]->value_type),
                           type_name_size);

      /* We also should dump current value */
      g_object_get_property (self->source, properties[p]->name, &value);
      value_bytes = lba_remote_object_server_value2bytes (&value);
      g_assert (value_bytes);   /* FIXME: we definitelly should just skip untransformable properties and signals in future */

      /* [current value size] */
      value_bytes_size_be = GUINT32_TO_BE (g_bytes_get_size (value_bytes));
      g_byte_array_append (msg, (guchar *) & value_bytes_size_be, 4);

      /* [current value] */
      g_byte_array_append (msg, g_bytes_get_data (value_bytes, NULL),
                           g_bytes_get_size (value_bytes));

      g_bytes_unref (value_bytes);
      g_value_unset (&value);
    }

    g_free (properties);
  }

  msg_bytes = g_byte_array_free_to_bytes (msg);
  g_assert (msg_bytes);

  /* Now we finally write the dump */
  LBA_LOG ("Writing %" G_GSIZE_FORMAT " bytes", g_bytes_get_size (msg_bytes));
  if (!lba_byte_stream_write_gbytes (self->output_stream, msg_bytes, &err)) {
    g_critical ("Couldn't write object dump: [%s]", err ? err->message : "");
    g_error_free (err);
  }

  return TRUE;
}

typedef struct _LbaRemoteObjectServerSignalCtx {
  LbaRemoteObjectServer *self;
  GSignalQuery signal_query;
} LbaRemoteObjectServerSignalCtx;

static void
lba_remote_object_server_signal_cb (GClosure * closure,
                                    GValue * return_value,
                                    guint n_param_values,
                                    const GValue * param_values,
                                    gpointer invocation_hint,
                                    gpointer marshal_data) {
  LbaRemoteObjectServerSignalCtx *signal_ctx;
  LbaRemoteObjectServer *self;

  /* Execute stored commands */
  LBA_LOG ("signal with %d parameters\n", n_param_values);

  /* Closure data is a "user data" */
  signal_ctx = closure->data;
  g_assert (signal_ctx);
  self = signal_ctx->self;
  g_assert (self);
  g_assert (n_param_values >= 1);

  LBA_LOG ("Signal %s", signal_ctx->signal_query.signal_name);

  /* FIXME: we need to receive the return value, so byte stream has to be a
   * "connection". Maybe we need an LbaConnection object, that would have various
   * bytestreams inside.. */

  {
    /* The protocol message is:
       [sign] - message magic of "signal", 4 bytes
       [number of params] - number of parameters, 1 byte
       [signal id] - 4 bytes

       .. then for each param. GTypes are already known from the dump sent on start.

       [param value size] - 4 bytes
       [param value] - bytes

       FIXME: we actually don't need to write size for int types etc
     */
    gint p;
    GByteArray *msg = NULL;
    GBytes *msg_bytes = NULL;
    guchar sign_magic[4] = { 's', 'i', 'g', 'n' };
    guint8 msg_number_of_params = n_param_values;
    GError *err = NULL;
    guint32 sig_id;

    msg = g_byte_array_new ();

    /* [sign] */
    g_byte_array_append (msg, sign_magic, 4);
    /* [number of params] */
    g_byte_array_append (msg, &msg_number_of_params, 1);

    /* [signal id] */
    sig_id = GUINT32_TO_BE (signal_ctx->signal_query.signal_id);
    g_byte_array_append (msg, (guchar *) & sig_id, 2);

    /* We skip the first parameter because it is an object of the source */
    for (p = 1; p < n_param_values; p++) {
      GBytes *bytes;
      guint32 bytes_size,
        bytes_size_be;

      bytes = lba_remote_object_server_value2bytes (&param_values[p]);
      g_assert (bytes);

      bytes_size = g_bytes_get_size (bytes);
      bytes_size_be = GUINT32_TO_BE (bytes_size);

      /* [param value size] */
      g_byte_array_append (msg, (guint8 *) & bytes_size_be, 4);
      /* [param value] */
      g_byte_array_append (msg, g_bytes_get_data (bytes, NULL), bytes_size);
    }

    msg_bytes = g_byte_array_free_to_bytes (msg);

    /* Finally, write the bytes */
    LBA_LOG ("Writing %" G_GSIZE_FORMAT " bytes", g_bytes_get_size (msg_bytes));
    if (!lba_byte_stream_write_gbytes (self->output_stream, msg_bytes, &err)) {
      g_critical ("Couldn't write signal message: [%s]", err ? err->message : "");
      g_error_free (err);
    }
  }
}

static void
lba_remote_object_passthrough_marshal (GClosure * closure,
                                       GValue * return_value,
                                       guint n_param_values,
                                       const GValue * param_values,
                                       gpointer invocation_hint,
                                       gpointer marshal_data) {
  /* this is our "passthrough" marshaller. We could also just omit
   * the closure's callback and do what we want here, given that it's passthrough, but
   * that is a hack */

  GClosureMarshal callback;
  GCClosure *cc = (GCClosure *) closure;

  callback = (GClosureMarshal) (marshal_data ? marshal_data : cc->callback);

  callback (closure,
            return_value,
            n_param_values, param_values, invocation_hint, marshal_data);
}

static void
lba_remote_object_server_free_signal_ctx (gpointer data, GClosure * closure) {
  g_free (data);
}

/* FIXME: should return gboolean ?? */
static void
lba_remote_object_server_start (LbaRemoteObjectServer * self) {
  GError *err = NULL;
  GType t;
  static volatile gboolean once;

  LBA_LOG ("Starting server");

  if (!self->output_stream) {
    g_warning ("No output stream");
    return;
  }

  if (!self->source) {
    g_warning ("No source object");
    return;
  }

  if (!once) {
    /* Register basic transform functions for types */
    g_value_register_transform_func (G_TYPE_STRING, G_TYPE_BYTES, _str2bytes);
    g_value_register_transform_func (G_TYPE_DOUBLE, G_TYPE_BYTES, _double2bytes);
    g_value_register_transform_func (G_TYPE_FLOAT, G_TYPE_BYTES, _float2bytes);
    g_value_register_transform_func (G_TYPE_INT, G_TYPE_BYTES, _int2bytes);
    g_value_register_transform_func (G_TYPE_UINT, G_TYPE_BYTES, _uint2bytes);
    g_value_register_transform_func (G_TYPE_BOOLEAN, G_TYPE_BYTES, _boolean2bytes);

    once = 1;
  }

  /* Open the output stream */
  if (!lba_byte_stream_open (self->output_stream, &err)) {
    g_warning ("Couldn't open output stream: [%s]", err ? err->message : "");
    g_error_free (err);
    return;
  }

  /* FIXME: lock mutex */

  /* Now we introspect the source object and send it's description.
   * Note: for connection-type streams this should be sent each time new connection
   * is opened. */
  if (!lba_remote_object_server_send_source_header (self)) {
    return;
  }

  t = G_OBJECT_TYPE (self->source);

  /* Now we connect to all the signals and notifications, so each time
   * source notifies, we will send notification to the bytestream*/
  {
    GParamSpec **properties;
    guint n_properties,
      p;
    guint *signals,
      s,
      n_signals;
    GObjectClass *klass;

    klass = g_type_class_peek (t);

    properties = g_object_class_list_properties (klass, &n_properties);

    /* Connect to notify::property  */
    for (p = 0; p < n_properties; p++) {
      GParamSpec *prop;
      gchar *prop_notify_str;

      prop = properties[p];
      prop_notify_str = g_strdup_printf ("notify::%s", g_param_spec_get_name (prop));

      LBA_LOG ("Connecting to %s", prop_notify_str);

      g_signal_connect_after (self->source, prop_notify_str,
                              G_CALLBACK
                              (lba_remote_object_server_source_param_notify), self);

      g_free (prop_notify_str);
    }

    g_free (properties);

    /* Iterate signals. */
    {
      for (; t; t = g_type_parent (t)) {
        guint notify_signal_id = 0;

        if (t == G_TYPE_OBJECT) {
          notify_signal_id = g_signal_lookup ("notify", t);
        }

        signals = g_signal_list_ids (t, &n_signals);

        for (s = 0; s < n_signals; s++) {
          GSignalQuery query;
          GClosure *closure;
          LbaRemoteObjectServerSignalCtx *signal_ctx;

          /* Avoid connecting to "notify" signal, we iterate properties for this */
          if (notify_signal_id == signals[s])
            continue;

          g_signal_query (signals[s], &query);

          if (query.signal_flags & G_SIGNAL_ACTION) {
            LBA_LOG ("Skipping action signal %s", query.signal_name);
          }

          LBA_LOG ("Connecting to %s", query.signal_name);

          /* We need to pass both self and signal info to the callback */
          signal_ctx = g_new0 (LbaRemoteObjectServerSignalCtx, 1);
          signal_ctx->signal_query = query;
          signal_ctx->self = self;

          closure =
              g_cclosure_new (G_CALLBACK (lba_remote_object_server_signal_cb),
                              signal_ctx, lba_remote_object_server_free_signal_ctx);

          /* Set our custom passthrough marshaller */
          g_closure_set_marshal (closure, lba_remote_object_passthrough_marshal);

          /* Won't closure be unreffed once we disconnect the signal??  */
          g_object_watch_closure (G_OBJECT (self), closure);

          /* Connect our super closure. */
          g_signal_connect_closure_by_id (self->source, signals[s], 0, closure,
                                          /* Execute after default handler = FALSE.
                                           * We suppose that default handler can emit more signals,
                                           * so if we execute after, it may reorder a little bit the emissions
                                           */
                                          FALSE);

          /* unref closure ?? */
        }

        g_free (signals);
      }
    }
  }
}

static void
lba_remote_object_server_set_property (GObject * object,
                                       guint property_id, const GValue * value,
                                       GParamSpec * pspec) {
  LbaRemoteObjectServer *self = (LbaRemoteObjectServer *) object;

  switch ((LbaRemoteObjectServerProperty) property_id) {
  case PROP_OUTPUT_STREAM:

    {
      GObject *obj;

      obj = g_value_get_object (value);

      if (!LBA_IS_BYTE_STREAM (obj)) {
        /* Fixme: write it's own param_spec ?? */
        g_warning ("Object is not an LbaByteStream");
        break;
      }

      /* Changing the output on fly is not supported yet */
      g_assert (!self->output_stream);

      self->output_stream = (LbaByteStream *) obj;

      if (self->output_stream) {
        g_object_ref (self->output_stream);
      }
    }
    break;

  case PROP_SOURCE:
    /* Changing the source on fly is not supported yet */
    g_assert (!self->source);

    self->source = g_value_get_object (value);

    if (self->source) {
      g_object_ref (self->source);
    }

    break;
  default:
    /* We don't have any other property... */
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}

static void
lba_remote_object_server_get_property (GObject * object,
                                       guint property_id, GValue * value,
                                       GParamSpec * pspec) {
  LbaRemoteObjectServer *self = (LbaRemoteObjectServer *) object;

  switch ((LbaRemoteObjectServerProperty) property_id) {

  case PROP_OUTPUT_STREAM:
    g_value_set_object (value, self->output_stream);
    break;
  case PROP_SOURCE:
    g_value_set_object (value, self->source);
    break;
  default:
    /* We don't have any other property... */
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}

static void
lba_remote_object_server_init (LbaRemoteObjectServer * self) {
}

static void
lba_remote_object_server_dispose (GObject * gobject) {
  LbaRemoteObjectServer *self = (LbaRemoteObjectServer *) gobject;

  /* FIXME: lock mutex */

  if (self->source) {
    g_signal_handlers_disconnect_by_data (self->source, self);
    g_object_unref (self->source);
  }

  if (self->output_stream) {
    /* FIXME should we stop here?? */
    lba_byte_stream_close (self->output_stream);
    g_object_unref (self->output_stream);
  }

  G_OBJECT_CLASS (lba_remote_object_server_parent_class)->dispose (gobject);
}

static void
lba_remote_object_server_class_init (LbaRemoteObjectServerClass * klass) {
  GObjectClass *object_class = (GObjectClass *) klass;

  klass->start = lba_remote_object_server_start;

  object_class->set_property = lba_remote_object_server_set_property;
  object_class->get_property = lba_remote_object_server_get_property;
  object_class->dispose = lba_remote_object_server_dispose;

  g_object_class_install_property (object_class,
                                   PROP_SOURCE,
                                   g_param_spec_object ("source",
                                                        "Source",
                                                        "Source object we bind in remote",
                                                        G_TYPE_OBJECT,
                                                        G_PARAM_STATIC_STRINGS |
                                                        G_PARAM_READWRITE));

  g_object_class_install_property (object_class,
                                   PROP_OUTPUT_STREAM,
                                   g_param_spec_object ("output-stream",
                                                        "Output Stream",
                                                        "Stream for data output",
                                                        G_TYPE_OBJECT,
                                                        G_PARAM_STATIC_STRINGS |
                                                        G_PARAM_READWRITE));

  lba_remote_object_server_signals[SIGNAL_START] =
      g_signal_new ("start", G_TYPE_FROM_CLASS (klass),
                    G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                    G_STRUCT_OFFSET (LbaRemoteObjectServerClass, start), NULL, NULL,
                    g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);
}

/* Export plugin */
BOMBOLLA_PLUGIN_SYSTEM_PROVIDE_GTYPE (lba_remote_object_server);
