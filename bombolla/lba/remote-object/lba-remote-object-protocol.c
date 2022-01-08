/* la Bombolla GObject shell.
 * Copyright (C) 2022 Aleksandr Slobodeniuk
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

#include "lba-remote-object-protocol.h"

const guchar *
lba_robj_protocol_parse_int (const guchar * msg, gint * ret) {
  *ret = GINT32_FROM_BE (*((gint *) msg));

  return msg + 4;
}

const guchar *
lba_robj_protocol_parse_uint (const guchar * msg, guint * ret) {
  *ret = GUINT32_FROM_BE (*((guint *) msg));

  return msg + 4;
}

const guchar *
lba_robj_protocol_parse_gvalue (const guchar * msg, GType value_type, GValue * ret) {
  guint value_size;
  GValue cur_value_bytes_value = G_VALUE_INIT;
  GBytes *cur_value_bytes;

  /* init */
  *ret = cur_value_bytes_value;

  msg = lba_robj_protocol_parse_uint (msg, &value_size);

  cur_value_bytes = g_bytes_new (msg, value_size);

  g_value_init (ret, value_type);
  g_value_init (&cur_value_bytes_value, G_TYPE_BYTES);

  g_value_set_boxed (&cur_value_bytes_value, cur_value_bytes);
  cur_value_bytes = NULL;       /* reference has been given away */

  if (!g_value_type_transformable (G_TYPE_BYTES, value_type)) {
    g_error ("Type %s can't be transformed from bytes", g_type_name (value_type));
  }

  if (!g_value_transform (&cur_value_bytes_value, ret)) {
    g_error ("Couldn't transform");
  }

  return msg + value_size;
}

const guchar *
lba_robj_protocol_parse_string (const guchar * msg, const gchar ** ret) {
  /* 
     [name] - 1 byte
     [property name] - zero-terminated string
   */
  guint string_size;

  msg = lba_robj_protocol_parse_uint (msg, &string_size);

  g_assert (msg[string_size] == 0);

  *ret = (const gchar *)msg;
  g_assert (*ret);

  return msg + string_size + 1;
}

const guchar *
lba_robj_protocol_parse_name (const guchar * msg, const gchar ** ret) {
  /* 
     [name] - 1 byte
     [property name] - zero-terminated string
   */
  guint8 name_size = msg[0];

  msg += 1;

  g_assert (msg[name_size] == 0);

  *ret = (const gchar *)msg;
  g_assert (*ret);

  return msg + name_size + 1;
}

const guchar *
lba_robj_protocol_parse_gtype (const guchar * msg, GType * ret) {
  const gchar *name;

  msg = lba_robj_protocol_parse_name (msg, &name);

  *ret = g_type_from_name (name);
  g_assert (*ret);

  return msg;
}

const guchar *
lba_robj_protocol_parse_pspec (const guchar * msg, GParamSpec ** ret) {
  /* [property name size] - 1 byte
     [property name] - zero-terminated string
     [gtype name size] - 1 byte
     [gtype name] - zero-terminated string
     [flags] - 4 bytes
   */

  const gchar *name;
  GType gtype;
  GParamFlags flags;

  /* name */
  msg = lba_robj_protocol_parse_name (msg, &name);

  /* gtype */
  msg = lba_robj_protocol_parse_gtype (msg, &gtype);

  /* flags */
  msg = lba_robj_protocol_parse_int (msg, &flags);

  /* Try basic types */
  switch (gtype) {
  case G_TYPE_CHAR:
    /* ... if char
       [minimum] - 1 byte
       [maximum] - 1 byte
       [default] - 1 byte
     */

    *ret = g_param_spec_char (name,
                              name,
                              name,
                              ((gchar *) msg)[0],
                              ((gchar *) msg)[1], ((gchar *) msg)[2], flags);

    return msg + 3;

  case G_TYPE_UCHAR:
    /* ... if uchar
       [minimum] - 1 byte
       [maximum] - 1 byte
       [default] - 1 byte
     */

    *ret = g_param_spec_uchar (name, name, name, msg[0], msg[1], msg[2], flags);

    return msg + 3;

  case G_TYPE_BOOLEAN:
    /* ... if boolean 
       [default] - 1 byte
     */
    *ret = g_param_spec_boolean (name,
                                 name, name, msg[0] == 0 ? FALSE : TRUE, flags);

    return msg + 1;

  case G_TYPE_INT:
    /* 
       [minimum] - 4 byte
       [maximum] - 4 bytes
       [default] - 4 bytes
     */
    {
      gint *msg_int = (gint *) msg;

      *ret = g_param_spec_int (name,
                               name,
                               name, msg_int[0], msg_int[1], msg_int[2], flags);

      return msg + 12;
    }

  case G_TYPE_UINT:
    /* 
       [minimum] - 4 byte
       [maximum] - 4 bytes
       [default] - 4 bytes
     */
    {
      guint *msg_uint = (guint *) msg;

      *ret = g_param_spec_uint (name,
                                name,
                                name, msg_uint[0], msg_uint[1], msg_uint[2], flags);

      return msg + 12;
    }

  case G_TYPE_LONG:
    /* 
       [minimum] - 4 byte
       [maximum] - 4 bytes
       [default] - 4 bytes
     */
    {
      gint *msg_int = (gint *) msg;

      *ret = g_param_spec_long (name,
                                name,
                                name, msg_int[0], msg_int[1], msg_int[2], flags);

      return msg + 12;
    }

  case G_TYPE_ULONG:
    /* 
       [minimum] - 4 byte
       [maximum] - 4 bytes
       [default] - 4 bytes
     */
    {
      guint *msg_uint = (guint *) msg;

      *ret = g_param_spec_ulong (name,
                                 name,
                                 name, msg_uint[0], msg_uint[1], msg_uint[2], flags);

      return msg + 12;
    }

  case G_TYPE_INT64:
    /* 
       [minimum] - 8 bytes
       [maximum] - 8 bytes
       [default] - 8 bytes
     */
    {
      gint64 *msg_int64 = (gint64 *) msg;

      *ret = g_param_spec_int64 (name,
                                 name,
                                 name,
                                 msg_int64[0], msg_int64[1], msg_int64[2], flags);

      return msg + 24;
    }

  case G_TYPE_UINT64:
    /* 
       [minimum] - 8 bytes
       [maximum] - 8 bytes
       [default] - 8 bytes
     */
    {
      guint64 *msg_uint64 = (guint64 *) msg;

      *ret = g_param_spec_uint64 (name,
                                  name,
                                  name,
                                  msg_uint64[0],
                                  msg_uint64[1], msg_uint64[2], flags);

      return msg + 24;
    }

  case G_TYPE_ENUM:
    /* 
       [type name size] - 1 byte
       [type name]
       [default] - 4 bytes
     */
    {
      GType enum_gtype;
      gint def;

      msg = lba_robj_protocol_parse_gtype (msg, &enum_gtype);
      msg = lba_robj_protocol_parse_int (msg, &def);

      *ret = g_param_spec_enum (name, name, name, enum_gtype, def, flags);

      return msg;
    }

  case G_TYPE_FLAGS:
    /* 
       [type name size] - 1 byte
       [type name]
       [default] - 4 bytes
     */
    {
      GType flags_type;
      guint def;

      msg = lba_robj_protocol_parse_gtype (msg, &flags_type);
      msg = lba_robj_protocol_parse_uint (msg, &def);

      *ret = g_param_spec_flags (name, name, name, flags_type, def, flags);

      return msg;
    }

  case G_TYPE_FLOAT:
    /* 
       [minimum] - 4 bytes
       [maximum] - 4 bytes
       [default] - 4 bytes
     */
    {
      gfloat *msg_float = (gfloat *) msg;

      *ret = g_param_spec_float (name,
                                 name,
                                 name,
                                 msg_float[0], msg_float[1], msg_float[2], flags);

      return msg + 12;
    }

  case G_TYPE_DOUBLE:
    /* 
       [minimum] - 8 bytes
       [maximum] - 8 bytes
       [default] - 8 bytes
     */
    {
      gdouble *msg_double = (gdouble *) msg;

      *ret = g_param_spec_float (name,
                                 name,
                                 name,
                                 msg_double[0], msg_double[1], msg_double[2], flags);

      return msg + 24;
    }

  case G_TYPE_STRING:
    /* [string size] - 4 bytes
       [string] - zero terminated string
     */
    {
      const gchar *def;

      msg = lba_robj_protocol_parse_string (msg, &def);

      *ret = g_param_spec_string (name, name, name, def, flags);

      return msg;
    }
  }

  g_error ("type not implemented");
  return NULL;
}
