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

/* HACK: Needed to use LBA_LOG */
static const gchar *global_lba_plugin_name = "LbaCore";

static void
_str2float (const GValue * src_value, GValue * dest_value) {
  gfloat ret = 0;
  const gchar *s = g_value_get_string (src_value);

  if (s) {
    ret = atof (s);
  } else
    g_warning ("couldn't convert string %s to float", s);

  g_value_set_float (dest_value, ret);
}

static void
_str2double (const GValue * src_value, GValue * dest_value) {
  gdouble ret = 0;
  const gchar *s = g_value_get_string (src_value);

  if (s) {
    ret = atof (s);
  } else
    g_warning ("couldn't convert string %s to double", s);

  g_value_set_double (dest_value, ret);
}

static void
_str2bool (const GValue * src_value, GValue * dest_value) {
  guint ret = 0;
  const gchar *s = g_value_get_string (src_value);

  if (s) {
    ret = !g_strcmp0 (s, "true");
  } else
    g_warning ("couldn't convert string %s to uint", s);

  g_value_set_boolean (dest_value, ret);
}

static void
_str2int (const GValue * src_value, GValue * dest_value) {
  guint ret = 0;
  const gchar *s = g_value_get_string (src_value);

  if (s) {
    ret = atoi (s);
  } else
    g_warning ("couldn't convert string %s to uint", s);

  g_value_set_int (dest_value, ret);
}

static void
_str2uint (const GValue * src_value, GValue * dest_value) {
  guint ret = 0;
  const gchar *s = g_value_get_string (src_value);

  if (s) {
    ret = atoi (s);
  } else
    g_warning ("couldn't convert string %s to uint", s);

  g_value_set_uint (dest_value, ret);
}

static void
_str2gtype (const GValue * src_value, GValue * dest_value) {
  GType t = G_TYPE_NONE;
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
lba_command_set_str2obj (BombollaContext * ctx,
                         const GValue * src_value, GValue * dest_value) {
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
lba_core_parse_obj_fld (BombollaContext * ctx, const gchar * str, GObject ** obj,
                        gchar ** fld) {
  gchar **tmp;
  gboolean ret = FALSE;

  tmp = g_strsplit (str, ".", 2);

  if (!tmp || !tmp[0] || !tmp[1]) {
    g_warning ("Couldn't parse [%s]", str);
    goto error;
  }
  *obj = g_hash_table_lookup (ctx->objects, tmp[0]);
  if (!*obj) {
    g_warning ("Object [%s] not found", tmp[0]);
    goto error;
  }

  *fld = g_strdup (tmp[1]);
  ret = TRUE;
error:
  if (tmp) {
    g_strfreev (tmp);
  }
  return ret;
}

void
lba_core_init_convertion_functions (void) {
  static volatile gboolean once;

  if (!once) {
    /* Register basic transform functions for types */
    g_value_register_transform_func (G_TYPE_STRING, G_TYPE_BOOLEAN, _str2bool);
    g_value_register_transform_func (G_TYPE_STRING, G_TYPE_INT, _str2int);
    g_value_register_transform_func (G_TYPE_STRING, G_TYPE_UINT, _str2uint);
    g_value_register_transform_func (G_TYPE_STRING, G_TYPE_DOUBLE, _str2double);
    g_value_register_transform_func (G_TYPE_STRING, G_TYPE_FLOAT, _str2float);
    g_value_register_transform_func (G_TYPE_STRING, G_TYPE_GTYPE, _str2gtype);

    once = 1;
  }
}

gboolean
lba_command_set (BombollaContext * ctx, gchar ** tokens) {

  GValue inp = G_VALUE_INIT;
  GValue outp = G_VALUE_INIT;
  gchar *prop_name = NULL;
  gchar *prop_val = NULL;
  GObject *obj = NULL;
  GParamSpec *prop;
  gboolean ret = FALSE;
  gint i;

  if (FALSE == (tokens[1] && tokens[2])) {
    g_warning ("Wrong syntax for command 'set'");
    goto done;
  }

  if (!lba_core_parse_obj_fld (ctx, tokens[1], &obj, &prop_name)) {
    goto done;
  }

  prop_val = g_strdup (tokens[2]);

  for (i = 3; tokens[i]; i++) {
    /* FIXME: we recover original string from tokens, it would be better to
     * just have an original string here an take it. This code has a bit
     * of problems, for example, tabs won't be recovered. */
    gchar *tmp_prop_val;

    tmp_prop_val = g_strdup_printf ("%s %s", prop_val, tokens[i]);
    g_free (prop_val);
    prop_val = tmp_prop_val;
  }

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

  LBA_LOG ("setting %s to [%s]", tokens[1], prop_val);
  g_object_set_property (obj, prop_name, &outp);

  ret = TRUE;
done:
  g_free (prop_val);
  g_free (prop_name);
  g_value_unset (&inp);
  g_value_unset (&outp);

  return ret;
}
