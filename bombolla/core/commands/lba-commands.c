/* la Bombolla GObject shell
 *
 * Copyright (c) 2024, Alexander Slobodeniuk <aleksandr.slobodeniuk@gmx.es>
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

#include "bombolla/lba-log.h"
#include "lba-commands.h"
#include <bmixin/bmixin.h>

/* HACK: Needed to use LBA_LOG */
static const gchar *global_lba_plugin_name = "LbaCore";

gchar **
FIXME_adapt_to_old (const gchar *expr, guint len) {
  gchar **tokens;
  gchar *e = g_strndup (expr, len);

  tokens = g_strsplit (e, " ", 0);
  g_free (e);

  return tokens;
}

static gboolean
lba_command_destroy (BombollaContext *ctx, const gchar *expr, guint len) {
  gchar **tokens = FIXME_adapt_to_old (expr, len);
  const gchar *varname = tokens[1];
  GObject *obj;

  obj = g_hash_table_lookup (ctx->objects, varname);

  if (obj) {
    g_hash_table_remove (ctx->objects, varname);
    LBA_LOG ("%s destroyed", varname);
    g_strfreev (tokens);
    return TRUE;
  }

  g_error ("object %s not found", varname);
  return FALSE;

}

static gboolean
lba_command_call (BombollaContext *ctx, const gchar *expr, guint len) {
  const gchar *objname,
   *signame;
  GObject *obj;
  char **tmp = NULL;
  gboolean ret = FALSE;
  GValue return_value = G_VALUE_INIT;
  GArray *instance_and_params = NULL;

  gchar **tokens = FIXME_adapt_to_old (expr, len);

  if (FALSE == (tokens[0] && tokens[1])) {
    g_warning ("wrong syntax");
    goto done;
  }

  tmp = g_strsplit (tokens[1], ".", 2);

  if (FALSE == (tmp && tmp[0] && tmp[1])) {
    g_warning ("wrong syntax");
    goto done;
  }

  objname = tmp[0];
  signame = tmp[1];

  obj = g_hash_table_lookup (ctx->objects, objname);

  if (!obj) {
    g_warning ("object %s not found", objname);
    goto done;
  }

  {
    int p;
    guint signal_id;
    GSignalQuery query;

    signal_id = g_signal_lookup (signame, G_OBJECT_TYPE (obj));
    if (!signal_id) {
      g_warning ("%s doesn't have signal '%s'", objname, signame);
      goto done;
    }
    g_signal_query (signal_id, &query);

    instance_and_params = g_array_sized_new (FALSE,
                                             FALSE,
                                             sizeof (GValue), query.n_params + 1);
    g_array_set_clear_func (instance_and_params, (GDestroyNotify) g_value_unset);

    {
      GValue instance = G_VALUE_INIT;

      g_value_init (&instance, G_OBJECT_TYPE (obj));
      g_value_set_object (&instance, obj);
      g_array_append_val (instance_and_params, instance);
    }

    for (p = 0; p < query.n_params; p++) {
      GValue param = G_VALUE_INIT;
      GType ptype = query.param_types[p];

      // check if we can process this param
      if (!G_TYPE_IS_OBJECT (ptype) &&
          !g_value_type_transformable (G_TYPE_STRING, ptype)) {
        g_warning ("[%s.%s]: don't know how to set parameter %d of type %s",
                   objname, signame, p, g_type_name (ptype));
        goto done;
      }
      // so now we need to get the param
      {
        // FIXME: handle spaces
        const gchar *strparam = tokens[2 + p];

        // -------------------
        GValue strparamv = G_VALUE_INIT;

        if (!strparam) {
          g_warning ("signal %s parameter %d is not set", signame, p);
          goto done;
        }

        LBA_LOG ("%s.%s(%d): setting [%s]-->[%s]", objname, signame, p, strparam,
                 g_type_name (ptype));

        g_value_init (&param, ptype);
        g_value_init (&strparamv, G_TYPE_STRING);
        g_value_set_string (&strparamv, strparam);

        if (G_TYPE_IS_OBJECT (ptype)) {
          if (!lba_command_set_str2obj (ctx, &strparamv, &param)) {
            g_warning ("object '%s' not found", strparam);
            g_value_unset (&strparamv);
            g_value_unset (&param);
            goto done;
          }
        } else if (!g_value_transform (&strparamv, &param)) {
          g_warning ("%s.%s(%d): could not transform [%s]-->[%s]",
                     objname, signame, p, strparam, g_type_name (ptype));
          g_value_unset (&strparamv);
          g_value_unset (&param);
          goto done;
        }
      }

      g_array_append_val (instance_and_params, param);
    }

    LBA_LOG ("calling %s.%s ()", objname, signame);
    g_signal_emitv ((GValue *) instance_and_params->data,
                    signal_id, 0, &return_value);
  }

  ret = TRUE;
done:
  g_value_unset (&return_value);
  if (instance_and_params)
    g_array_unref (instance_and_params);
  if (tmp)
    g_strfreev (tmp);
  g_strfreev (tokens);

  return ret;
}

static gboolean
lba_command_bind (BombollaContext *ctx, const gchar *expr, guint len) {
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
  gchar **tokens = FIXME_adapt_to_old (expr, len);

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
  g_strfreev (tokens);
  g_free (binding_name);
  g_free (prop_name1);
  g_free (prop_name2);
  return ret;
}

gchar *
assemble_line (gchar **tokens) {
  gchar *ret;
  gint i;
  gchar *tmp;

  ret = g_strdup (tokens[0]);

  for (i = 1; tokens[i]; i++) {
    /* FIXME: we recover original string from tokens, it would be better to
     * just have an original string here an take it. This code has a bit
     * of problems, for example, tabs won't be recovered. */

    tmp = g_strdup_printf ("%s %s", ret, tokens[i]);
    g_free (ret);
    ret = tmp;
  }

  /* Wrap into (), oh what a terrible workaround */
  ret = g_strdup_printf ("(%s)", ret);
  g_free (tmp);
  g_message ("assembled %s", ret);
  return ret;
}

static gboolean
lba_command_log (BombollaContext *ctx, const gchar *expr, guint len) {
  gchar **tokens = FIXME_adapt_to_old (expr, len);

  g_message ("Will log %s", tokens[1]);
//  g_log_writer_default_set_debug_domains (&tokens[1]);
  g_strfreev (tokens);
  return TRUE;
}

static gboolean
lba_command_sync (BombollaContext *ctx, const gchar *expr, guint len) {
  gchar **tokens = FIXME_adapt_to_old (expr, len);

  if (NULL != tokens[1]) {
    g_warning ("invalid syntax for 'sync' command");
  }

  lba_core_sync_with_async_cmds (ctx->self);

  g_strfreev (tokens);
  return TRUE;
}

static gboolean
lba_command_async (BombollaContext *ctx, const gchar *expr, guint len) {
  gchar **tokens = FIXME_adapt_to_old (expr, len);

  lba_core_shedule_async_script (ctx->self, assemble_line (tokens + 1));
  g_strfreev (tokens);
  return TRUE;
}

static gboolean
lba_command_dna (BombollaContext *ctx, const gchar *expr, guint len) {
  gint t;
  GType base_type;

  gchar **tokens = FIXME_adapt_to_old (expr, len);

  const gchar *mixed_type_name = tokens[1];
  const gchar *base_name = tokens[2];

  if (NULL == mixed_type_name || NULL == base_name) {
    goto syntax;
  }

  base_type = g_type_from_name (base_name);

  if (0 == base_type) {
    g_warning ("Type '%s' not found", base_name);
    goto syntax;
  }

  if (0 != g_type_from_name (mixed_type_name)) {
    g_warning ("Type '%s' already exists", mixed_type_name);
  }

  if (NULL == tokens[3]) {
    g_warning ("No mixins listed");
    goto syntax;
  }

  for (t = 3; tokens[t]; t++) {
    /* In fact we have to register various intermediate mixed_type classes in order
     * to reach the requested one */
    const gchar *mixin_name = tokens[t];
    GType mixin_type = g_type_from_name (mixin_name);

    /* Final type */
    if (tokens[t + 1] == NULL) {
      base_type =
          bm_register_mixed_type (mixed_type_name, base_type, mixin_type, NULL);
      base_name = g_type_name (base_type);
    } else {
      /* Intermediate type */
      base_type = bm_register_mixed_type (NULL, base_type, mixin_type, NULL);
      base_name = g_type_name (base_type);
    }

    if (0 == base_type) {
      g_warning ("Error occured");
      return FALSE;
    }

    g_message ("Have type %s", g_type_name (base_type));
  }

  g_strfreev (tokens);
  return TRUE;

syntax:
  g_error ("Syntax error. Expected: "
           "dna <mixed type name> <base type> <mixin 1> ... <mixin N>");
  g_strfreev (tokens);
  return FALSE;
}

const BombollaCommand commands[] = {
  { "destroy", lba_command_destroy },
  { "call", lba_command_call },
  { "set", lba_command_set },
  { "bind", lba_command_bind },
  { "async", lba_command_async },
  { "sync", lba_command_sync },
  { "dna", lba_command_dna },
  { "log", lba_command_log },
  /* End of list */
  { NULL, NULL }
};
