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
#include "bombolla/shell/commands/bombolla-commands.h"

static gboolean
lba_command_create (BombollaContext *ctx, gchar **tokens)
{
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
      if (!ctx->objects) {
        ctx->objects =
            g_hash_table_new_full (g_str_hash, g_str_equal, g_free, g_object_unref);
      }
      
      g_hash_table_insert (ctx->objects, (gpointer) g_strdup (varname), obj);

      /* FIXME: LBA_LOG */
      g_printf ("%s %s created\n", typename, varname);
    }
  }

  return TRUE;
}

static gboolean
lba_command_destroy (BombollaContext *ctx, gchar **tokens)
{
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
lba_command_call (BombollaContext *ctx, gchar **tokens)
{
  const gchar *objname, *signal_name;
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


const BombollaCommand commands[] = {
  { "create", lba_command_create },
  { "destroy", lba_command_destroy },
  { "call", lba_command_call },
  { "on", lba_command_on },
  { "set", lba_command_set },
  /* End of list */
  { NULL, NULL }
};
