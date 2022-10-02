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

static gboolean
lba_command_create (BombollaContext * ctx, gchar ** tokens) {
  const gchar *typename = tokens[1];
  const gchar *varname = tokens[2];
  const gchar *request_failure_msg = NULL;
  GType obj_type;

  obj_type = g_type_from_name (typename);

  if (!varname) {
    request_failure_msg = "need varname";
  } else if (g_hash_table_lookup (ctx->objects, varname)) {
    request_failure_msg = "variable already exists";
  } else if (!obj_type) {
    request_failure_msg = "type not found";
  }

  if (request_failure_msg) {
    /* FIXME: LBA_LOG */
    g_warning ("%s", request_failure_msg);
    return FALSE;
  } else {
    GObject *obj = g_object_new (obj_type, NULL);

    if (obj) {
      g_hash_table_insert (ctx->objects, (gpointer) g_strdup (varname), obj);

      /* FIXME: LBA_LOG */
      g_printf ("%s %s created\n", typename, varname);
    }
  }

  return TRUE;
}

static gboolean
lba_command_destroy (BombollaContext * ctx, gchar ** tokens) {
  const gchar *varname = tokens[1];
  GObject *obj;

  obj = g_hash_table_lookup (ctx->objects, varname);

  if (obj) {
    g_hash_table_remove (ctx->objects, varname);
    g_printf ("%s destroyed\n", varname);
    return TRUE;
  }

  g_warning ("object %s not found", varname);
  return FALSE;

}

static gboolean
lba_command_call (BombollaContext * ctx, gchar ** tokens) {
  const gchar *objname,
   *signal_name;
  GObject *obj;
  char **tmp = NULL;
  gboolean ret = FALSE;

  tmp = g_strsplit (tokens[1], ".", 2);

  if (FALSE == (tmp && tmp[0] && tmp[1])) {
    g_warning ("wrong syntax");
    goto done;
  }

  objname = tmp[0];
  signal_name = tmp[1];

  obj = g_hash_table_lookup (ctx->objects, objname);

  if (!obj) {
    g_warning ("object %s not found", objname);
    goto done;
  }

  g_printf ("calling %s ()\n", signal_name);

  g_signal_emit_by_name (obj, signal_name, NULL);

  ret = TRUE;
done:
  if (tmp) {
    g_strfreev (tmp);
  }

  return ret;
}

static void
lba_command_dump_type (GType plugin_type) {
  GTypeQuery query;

  g_type_query (plugin_type, &query);
  g_printf ("GType name = '%s'\n", query.type_name);

  if (G_TYPE_IS_OBJECT (plugin_type)) {
    GParamSpec **properties;
    guint n_properties,
      p;
    guint *signals,
      s,
      n_signals;
    GObjectClass *klass;

    g_printf ("GType is a GObject.");
    klass = (GObjectClass *) g_type_class_ref (plugin_type);

    properties = g_object_class_list_properties (klass, &n_properties);

    for (p = 0; p < n_properties; p++) {
      GParamSpec *prop = properties[p];
      gchar *def_val;

      def_val = g_strdup_value_contents (g_param_spec_get_default_value (prop));

      g_printf ("- Property: (%s) %s = %s\n",
                G_PARAM_SPEC_TYPE_NAME (prop), g_param_spec_get_name (prop),
                def_val);

      g_printf ("\tnick = '%s', %s\n\n",
                g_param_spec_get_nick (prop), g_param_spec_get_blurb (prop));
      g_free (def_val);
    }

    /* Iterate signals */
    {
      GType t;
      const gchar *_tab = "  ";
      gchar *tab = g_strdup (_tab);

      g_printf ("--- Signals:\n");
      for (t = plugin_type; t; t = g_type_parent (t)) {
        gchar *tmptab;

        signals = g_signal_list_ids (t, &n_signals);

        for (s = 0; s < n_signals; s++) {
          GSignalQuery query;
          GTypeQuery t_query;

          g_signal_query (signals[s], &query);
          g_type_query (t, &t_query);

          if (t_query.type == 0) {
            g_warning ("Error quering type");
            break;
          }

          g_printf ("%s%s:: %s (* %s) ", tab, t_query.type_name,
                    g_type_name (query.return_type), query.signal_name);

          g_printf ("(");
          for (p = 0; p < query.n_params; p++) {
            g_printf ("%s%s", p ? ", " : "", g_type_name (query.param_types[p]));
          }
          g_printf (");\n");
        }

        tmptab = tab;
        tab = g_strdup_printf ("%s%s", _tab, tab);
        g_free (tmptab);
        g_free (signals);
      }
    }

    /* Iterate interfaces */
    {
      guint i;
      guint n_interfaces;
      GType *in = g_type_interfaces (plugin_type,
                                     &n_interfaces);

      for (i = 0; i < n_interfaces; i++) {

        g_printf ("Provides interface %s\n", g_type_name (in[i]));
      }
      g_free (in);
    }

    g_type_class_unref (klass);
    g_free (properties);
  }
}

static gboolean
lba_command_dump (BombollaContext * ctx, gchar ** tokens) {
  GType t;

  if (NULL == tokens[1]) {
    g_warning ("No type specified");
    return FALSE;
  }

  t = g_type_from_name (tokens[1]);

  if (0 == t) {
    g_warning ("Type %s not found", tokens[1]);
    return FALSE;
  }

  lba_command_dump_type (t);
  return TRUE;
}

static gboolean
lba_command_bind (BombollaContext * ctx, gchar ** tokens) {
  gchar **tmp1 = NULL;
  gchar **tmp2 = NULL;
  const gchar *prop_name1;
  const gchar *prop_name2;
  const gchar *objname;
  GObject *obj1;
  GObject *obj2;

  if (NULL == tokens[1]) {
    g_warning ("Need obj1.prop");
    return FALSE;
  }

  if (NULL == tokens[2]) {
    g_warning ("Need obj2.prop");
    return FALSE;
  }

  /* todo: to func "parse obj.prop" */
  tmp1 = g_strsplit (tokens[1], ".", 2);

  objname = tmp1[0];
  prop_name1 = tmp1[1];
  obj1 = g_hash_table_lookup (ctx->objects, objname);

  tmp2 = g_strsplit (tokens[2], ".", 2);

  objname = tmp2[0];
  prop_name2 = tmp2[1];
  obj2 = g_hash_table_lookup (ctx->objects, objname);

  if (!obj1 || !obj2 || !prop_name1 || !prop_name2) {
    g_warning ("fail!");
    return FALSE;
  }

  /* at this moment obj1 -> obj2 */
  g_object_unref (g_object_bind_property (obj1,
                                          prop_name1,
                                          obj2, prop_name2, G_BINDING_SYNC_CREATE));

  if (tmp1) {
    g_strfreev (tmp1);
  }

  if (tmp2) {
    g_strfreev (tmp2);
  }

  return TRUE;
}

const BombollaCommand commands[] = {
  { "create", lba_command_create },
  { "destroy", lba_command_destroy },
  { "call", lba_command_call },
  { "on", lba_command_on },
  { "set", lba_command_set },
  { "dump", lba_command_dump },
  { "bind", lba_command_bind },
  /* End of list */
  { NULL, NULL }
};
