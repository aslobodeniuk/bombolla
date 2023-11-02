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
#include <gmo/gmo.h>

/* HACK: Needed to use LBA_LOG */
static const gchar *global_lba_plugin_name = "LbaCore";

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
    /* FIXME: LBA_LOG_WARNING ?? */
    g_warning ("%s", request_failure_msg);
    return FALSE;
  } else {
    GObject *obj;

    /* If the type is mutogene, then we might be able to instantiate
     * the mutant. If it's not abstract. */
    if (G_TYPE_IS_MUTOGENE (obj_type))
      obj_type = gmo_register_mutant (NULL, G_TYPE_OBJECT, obj_type);

    obj = g_object_new (obj_type, NULL);

    if (obj) {
      g_object_set_data (obj, "bombolla-commands-ctx", ctx);
      g_hash_table_insert (ctx->objects, (gpointer) g_strdup (varname), obj);

      LBA_LOG ("%s %s created", typename, varname);
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
    LBA_LOG ("%s destroyed", varname);
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

  LBA_LOG ("calling %s ()", signal_name);

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
  GTypeQuery query = { 0 };

  g_type_query (plugin_type, &query);
  if (query.type)
    g_printf ("Dumping GType '%s'\n", query.type_name);

  if (G_TYPE_IS_MUTOGENE (plugin_type)) {
    g_printf ("Mutogene %s\n", g_type_name (plugin_type));
    lba_command_dump_type (gmo_register_mutant (NULL, G_TYPE_OBJECT, plugin_type));
    return;
  }

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
    GObject *obj;

    obj = g_hash_table_lookup (ctx->objects, tokens[1]);
    if (!obj) {
      g_warning ("Neither type nor an object '%s' haven't been found", tokens[1]);
      return FALSE;
    }

    t = G_OBJECT_TYPE (obj);
  }

  lba_command_dump_type (t);
  return TRUE;
}

static gboolean
lba_command_bind (BombollaContext * ctx, gchar ** tokens) {
  gchar *prop_name1 = NULL;
  gchar *prop_name2 = NULL;
  GObject *obj1 = NULL;
  GObject *obj2 = NULL;
  GParamSpec *pspec1;
  GParamSpec *pspec2;
  gboolean ret = FALSE;
  GBindingFlags flags = G_BINDING_SYNC_CREATE | G_BINDING_DEFAULT;
  GBinding *binding;
  gchar *binding_name = NULL;

  if (NULL == tokens[1]) {
    g_warning ("Need obj1.prop");
    return FALSE;
  }

  if (NULL == tokens[2]) {
    g_warning ("Need obj2.prop");
    return FALSE;
  }

  if (NULL != tokens[3]) {
    g_warning ("Too many arguments for 'bind' command");
    return FALSE;
  }

  if (!lba_core_parse_obj_fld (ctx, tokens[1], &obj1, &prop_name1)) {
    return FALSE;
  }

  if (!lba_core_parse_obj_fld (ctx, tokens[2], &obj2, &prop_name2)) {
    g_free (prop_name1);
    return FALSE;
  }

  /* Now we figure out the binding flags:
   * If one of the properties is read-only, we bind in one direction */
  pspec1 = g_object_class_find_property (G_OBJECT_GET_CLASS (obj1), prop_name1);

  if (!pspec1) {
    g_warning ("property [%s] not found", prop_name1);
    goto done;
  }

  pspec2 = g_object_class_find_property (G_OBJECT_GET_CLASS (obj2), prop_name2);

  if (!pspec2) {
    g_warning ("property [%s] not found", prop_name2);
    goto done;
  }

  binding_name = g_strdup_printf ("%s_%s", tokens[1], tokens[2]);

  if (((pspec1->flags & G_PARAM_READWRITE) == G_PARAM_READWRITE)
      && ((pspec2->flags & G_PARAM_READWRITE) == G_PARAM_READWRITE)) {
    /* easy case: both are rw. Do bidirectional binding */
    LBA_LOG ("Adding bidirectional binding [%s]<-->[%s]", tokens[1], tokens[2]);
    flags |= G_BINDING_BIDIRECTIONAL;
  } else if ((pspec1->flags & G_PARAM_READABLE)
             && (pspec2->flags & G_PARAM_WRITABLE)) {
    LBA_LOG ("Adding monodirectional binding [%s]--->[%s]", tokens[1], tokens[2]);
  } else if ((pspec2->flags & G_PARAM_READABLE)
             && (pspec1->flags & G_PARAM_WRITABLE)) {

    LBA_LOG ("Adding monodirectional binding [%s]<---[%s]", tokens[1], tokens[2]);
    binding = g_object_bind_property (obj2, prop_name2, obj1, prop_name1, flags);
    /* TODO: we will need it for "unbind" command */
    g_hash_table_insert (ctx->bindings, binding_name, binding);
    LBA_LOG ("Added binding %s", binding_name);
    binding_name = NULL;
    ret = TRUE;
    goto done;
  } else {
    g_warning ("Couldn't bind property [%s](%c%c) to [%s](%c%c)", tokens[1],
               (pspec1->flags & G_PARAM_READABLE) ? 'r' : '_',
               (pspec1->flags & G_PARAM_WRITABLE) ? 'w' : '_', tokens[2],
               (pspec2->flags & G_PARAM_READABLE) ? 'r' : '_',
               (pspec2->flags & G_PARAM_WRITABLE) ? 'w' : '_');
    goto done;
  }

  binding = g_object_bind_property (obj1, prop_name1, obj2, prop_name2, flags);
  /* TODO: we will need it for "unbind" command */
  g_hash_table_insert (ctx->bindings, binding_name, binding);
  LBA_LOG ("Added binding %s", binding_name);
  binding_name = NULL;
  ret = TRUE;
done:
  g_free (binding_name);
  g_free (prop_name1);
  g_free (prop_name2);
  return ret;
}

gchar *
assemble_line (gchar ** tokens) {
  gchar *ret;
  gint i;

  ret = g_strdup (tokens[0]);

  for (i = 1; tokens[i]; i++) {
    /* FIXME: we recover original string from tokens, it would be better to
     * just have an original string here an take it. This code has a bit
     * of problems, for example, tabs won't be recovered. */
    gchar *tmp;

    tmp = g_strdup_printf ("%s %s", ret, tokens[i]);
    g_free (ret);
    ret = tmp;
  }

  return ret;
}

static gboolean
lba_command_log (BombollaContext * ctx, gchar ** tokens) {
  if (NULL == tokens[1] || NULL != tokens[2]) {
    g_warning ("invalid syntax for 'log' command");
  }

  g_printf ("Warning: using g_setenv, that is not thread safe");
  g_setenv ("G_MESSAGES_DEBUG", tokens[1], TRUE);
  return TRUE;
}

static gboolean
lba_command_sync (BombollaContext * ctx, gchar ** tokens) {
  if (NULL != tokens[1]) {
    g_warning ("invalid syntax for 'sync' command");
  }

  lba_core_sync_with_async_cmds (ctx->self);
  return TRUE;
}

static gboolean
lba_command_async (BombollaContext * ctx, gchar ** tokens) {
  lba_core_shedule_async_script (ctx->self, assemble_line (tokens + 1));
  return TRUE;
}

static gboolean
lba_comment (BombollaContext * ctx, gchar ** tokens) {
  return TRUE;
}

static gboolean
lba_command_dna (BombollaContext * ctx, gchar ** tokens) {
  gint t;
  GType base_type;
  const gchar *mutant_name = tokens[1];
  const gchar *base_name = tokens[2];

  if (NULL == mutant_name || NULL == base_name) {
    goto syntax;
  }

  base_type = g_type_from_name (base_name);

  if (0 == base_type) {
    g_warning ("Type '%s' not found", base_name);
    goto syntax;
  }

  if (0 != g_type_from_name (mutant_name)) {
    g_warning ("Type '%s' already exists", mutant_name);
  }

  if (NULL == tokens[3]) {
    g_warning ("No mutogenes listed");
    goto syntax;
  }

  for (t = 3; tokens[t]; t++) {
    /* In fact we have to register various intermediate mutant classes in order
     * to reach the requested one */
    const gchar *mutogene_name = tokens[t];
    GType mutogene_type = g_type_from_name (mutogene_name);

    /* Final type */
    if (tokens[t + 1] == NULL) {
      base_type = gmo_register_mutant (mutant_name, base_type, mutogene_type);
      base_name = g_type_name (base_type);
    } else {
      /* Intermediate type */
      base_type = gmo_register_mutant (NULL, base_type, mutogene_type);
      base_name = g_type_name (base_type);
    }

    if (0 == base_type) {
      g_warning ("Error occured");
      return FALSE;
    }

    g_message ("Have type %s", g_type_name (base_type));
  }

  return TRUE;

syntax:
  g_warning ("Syntax error. Expected: "
             "dna <mutant name> <base type> <mutogene 1> ... <mutogene N>");
  return FALSE;
}

const BombollaCommand commands[] = {
  { "create", lba_command_create },
  { "destroy", lba_command_destroy },
  { "call", lba_command_call },
  { "on", lba_command_on },
  { "set", lba_command_set },
  { "dump", lba_command_dump },
  { "bind", lba_command_bind },
  { "async", lba_command_async },
  { "sync", lba_command_sync },
  { "dna", lba_command_dna },
  { "log", lba_command_log },
  { "#", lba_comment },
  /* End of list */
  { NULL, NULL }
};
