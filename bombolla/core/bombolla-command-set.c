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

#include "bombolla/lba-log.h"
#include "bombolla/core/bombolla-commands.h"


static void
_str2double (const GValue * src_value, GValue * dest_value)
{
  gdouble ret = 0;
  const gchar *s = g_value_get_string (src_value);

  if (s) {
    ret = atof (s);
  } else
    g_warning ("couldn't convert string %s to double", s);

  g_value_set_double (dest_value, ret);
}


static void
_str2bool (const GValue * src_value, GValue * dest_value)
{
  guint ret = 0;
  const gchar *s = g_value_get_string (src_value);

  if (s) {
    ret = !g_strcmp0 (s, "true");
  } else
    g_warning ("couldn't convert string %s to uint", s);

  g_value_set_boolean (dest_value, ret);
}


static void
_str2uint (const GValue * src_value, GValue * dest_value)
{
  guint ret = 0;
  const gchar *s = g_value_get_string (src_value);

  if (s) {
    ret = atoi (s);
  } else
    g_warning ("couldn't convert string %s to uint", s);

  g_value_set_uint (dest_value, ret);
}


static void
_str2gtype (const GValue * src_value, GValue * dest_value)
{
  GType t = G_TYPE_NONE;
  gpointer ptr;
  const gchar *s = g_value_get_string (src_value);

  if (s) {
    t = g_type_from_name (s);
  }

  if (!t || t == G_TYPE_NONE) {
    g_warning ("couldn't convert string %s to GType", s);
  }

  g_value_set_gtype (dest_value, t);
}


static gboolean
lba_command_set_str2obj (BombollaContext *ctx,
    const GValue * src_value, GValue * dest_value)
{
  GObject *obj = NULL;
  const gchar *s = g_value_get_string (src_value);

  if (s) {
    obj = g_hash_table_lookup (ctx->objects, (gpointer) s);
  }

  if (!obj) {
    g_warning ("couldn't convert string %s to GObject", s);
    return FALSE;
  }

  g_value_set_object (dest_value, obj);
  return TRUE;
}


gboolean
lba_command_set (BombollaContext *ctx, gchar **tokens)
{
  
  GValue inp = G_VALUE_INIT;
  GValue outp = G_VALUE_INIT;
  const gchar *prop_name, *prop_val;
  const gchar *objname;
  GObject *obj;
  GParamSpec *prop;
  char **tmp = NULL;
  gboolean ret = FALSE;
  static volatile gboolean once;
  
  tmp = g_strsplit (tokens[1], ".", 2);

  if (FALSE == (tmp && tmp[0] && tmp[1] && tokens[2])) {
    g_warning ("wrong syntax");
    goto done;
  }

  objname = tmp[0];
  prop_name = tmp[1];

  obj = g_hash_table_lookup (ctx->objects, objname);

  if (!obj) {
    g_warning ("object %s not found", objname);
    goto done;
  }
  


  if (!once) {
    /* Register basic transform functions for types */
    g_value_register_transform_func (G_TYPE_STRING, G_TYPE_BOOLEAN, _str2bool);
    g_value_register_transform_func (G_TYPE_STRING, G_TYPE_UINT, _str2uint);
    g_value_register_transform_func (G_TYPE_STRING, G_TYPE_DOUBLE, _str2double);
    g_value_register_transform_func (G_TYPE_STRING, G_TYPE_GTYPE, _str2gtype);

    once = 1;
  }
  
  /* FIXME */
  prop_val = tokens [2];

  /* Now we need to find out gtype of out */
  prop = g_object_class_find_property (G_OBJECT_GET_CLASS (obj), prop_name);

  if (!prop) {
    g_warning ("property %s not found", prop_name);
    goto done;
  }

  g_value_init (&inp, G_TYPE_STRING);
  g_value_set_string (&inp, prop_val);
  g_value_init (&outp, prop->value_type);

 /* Now set outp */
  if (prop->value_type == G_TYPE_OBJECT) {
    if (!lba_command_set_str2obj (ctx, &inp, &outp)) {
      g_warning ("object '%s' not found", prop_val);
      goto done;
    }

  } else if (!g_value_transform (&inp, &outp)) {
    g_warning ("unsupported parameter type");
    goto done;
  }

  g_printf ("setting %s.%s to %s\n", objname, prop_name, prop_val);
  g_object_set_property (obj, prop_name, &outp);

  ret = TRUE;
done:
  if (tmp) {
    g_strfreev (tmp);
  }

  return ret;
}
