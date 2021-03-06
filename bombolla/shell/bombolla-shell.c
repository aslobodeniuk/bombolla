/* la Bombolla GObject shell.
 * Copyright (C) 2020 Aleksandr Slobodeniuk
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

/* Compile:
 * gcc my-plugin-system-inspect.c -o my-plugin-system-inspect $(pkg-config --cflags --libs gobject-2.0) $(pkg-config --libs gmodule-2.0)
 */

#include "bombolla/lba-plugin-system.h"
#include <glib/gstdio.h>
#include <gmodule.h>
#include <string.h>
#include <stdlib.h>


#ifdef G_OS_WIN32
const gchar *B_PLUGIN_EXTENSION = ".dll";
const gchar *B_OS_FILE_SEPARATOR = "\\";
#else
const gchar *B_PLUGIN_EXTENSION = ".so";
const gchar *B_OS_FILE_SEPARATOR = "/";
#endif

GHashTable *objects = NULL;

void
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

void
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


void
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

void
_str2obj (const GValue * src_value, GValue * dest_value)
{
  GObject *obj = NULL;
  const gchar *s = g_value_get_string (src_value);

  if (s) {
    obj = g_hash_table_lookup (objects, (gpointer) s);
  }

  if (!obj) {
    g_warning ("couldn't convert string %s to GObject", s);
  }

  g_value_set_object (dest_value, obj);
}

void
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

static gboolean proccess_shell_string (const gchar * a);

/* Callback for "on" command. It's designed to use parameters
 * of the signal, but for now we just ignore them.
 * So, it executes the commands, stored for that "on" instance. */
static void
lba_on_cb (GClosure * closure,
    GValue * return_value,
    guint n_param_values,
    const GValue * param_values,
    gpointer invocation_hint, gpointer marshal_data)
{
  gpointer user_data;
  GSList *commands, *l;
  /* Execute stored commands */
  g_printf ("on something of %d parameters\n", n_param_values);

  /* Closure data is our pointer to the list */
  user_data = closure->data;

  /* take user_data param, and execute it as a list of strings */
  commands = *((GSList **) user_data);

  /* Now execute line by line everything */
  for (l = commands; l; l = l->next) {
    proccess_shell_string ((const gchar *) l->data);
  }
}


static void
lba_on_marshal (GClosure * closure,
    GValue * return_value,
    guint n_param_values,
    const GValue * param_values,
    gpointer invocation_hint, gpointer marshal_data)
{
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
on_commands_list_free (gpointer data, GClosure * closure)
{
  GSList *commands;

  if (!data)
    return;

  commands = *((GSList **) data);

  g_slist_free_full (commands, g_free);
  g_free (data);
}

gpointer *on_commands_list;

static gboolean
proccess_shell_string (const gchar * a)
{
  gboolean ret = TRUE;
  char **tmp = NULL;
  char **tokens;

  tokens = g_strsplit (a, " ", 0);

  if (!tokens || !tokens[0]) {
    goto done;
  }

  g_printf ("processing '%s'\n", a);

  if (!g_strcmp0 (tokens[0], "q")) {
    /* Quit */
    ret = FALSE;
    goto done;
  }

  /* On ... commands */
  if (on_commands_list) {
    if (!g_strcmp0 (tokens[0], "on")) {
      g_warning ("forbidden to call on from on");
      goto done;
    }
    /* TODO: syntax check */
    if (!g_strcmp0 (tokens[0], "end")) {
      /* We're done with this ptr. Now it's the closure who takes care of it */
      on_commands_list = NULL;
      goto done;
    }

    /* appending command */
    *on_commands_list = g_slist_append (*on_commands_list, g_strdup (a));
    goto done;
  }

  if (tokens[1]) {
    /* Create */
    if (!g_strcmp0 (tokens[0], "create")) {
      const gchar *typename = tokens[1];
      const gchar *varname = tokens[2];
      const gchar *request_failure_msg = NULL;
      GType obj_type = g_type_from_name (typename);

      if (!varname) {
        request_failure_msg = "need varname";
      } else if (g_hash_table_lookup (objects, varname)) {
        request_failure_msg = "variable already exists";
      } else if (!obj_type) {
        request_failure_msg = "type not found";
      }

      if (request_failure_msg) {
        g_warning (request_failure_msg);
      } else {
        GObject *obj = g_object_new (obj_type, NULL);

        if (obj) {
          g_hash_table_insert (objects, (gpointer) g_strdup (varname), obj);
          g_printf ("%s %s created\n", typename, varname);
        }
      }

      goto done;
    }

    /* Destroy */
    if (!g_strcmp0 (tokens[0], "destroy")) {
      const gchar *varname = tokens[1];
      GObject *obj = g_hash_table_lookup (objects, varname);

      if (obj) {
        g_hash_table_remove (objects, varname);
        g_printf ("%s destroyed\n", varname);
      } else
        g_warning ("object %s not found", varname);

      goto done;
    }

    /* Call */
    if (!g_strcmp0 (tokens[0], "call")) {
      const gchar *objname, *signal_name;
      GObject *obj;

      tmp = g_strsplit (tokens[1], ".", 2);

      if (FALSE == (tmp && tmp[0] && tmp[1])) {
        g_warning ("wrong syntax");
        goto done;
      }

      objname = tmp[0];
      signal_name = tmp[1];

      obj = g_hash_table_lookup (objects, objname);

      if (!obj) {
        g_warning ("object %s not found", objname);
        goto done;
      }

      g_printf ("calling %s ()\n", signal_name);
      g_signal_emit_by_name (obj, signal_name, NULL);
      goto done;
    }


    /* On */
    if (!g_strcmp0 (tokens[0], "on")) {
      const gchar *objname, *signal_name;
      GObject *obj;

      tmp = g_strsplit (tokens[1], ".", 2);

      if (FALSE == (tmp && tmp[0] && tmp[1])) {
        g_warning ("wrong syntax");
        goto done;
      }

      objname = tmp[0];
      signal_name = tmp[1];

      obj = g_hash_table_lookup (objects, objname);

      if (!obj) {
        g_warning ("object %s not found", objname);
        goto done;
      }

      {
        GClosure *closure;

        g_printf ("on %s.%s () ...\n", objname, signal_name);

        /* allocate pointer to a pointer where we'll store commands */
        on_commands_list = g_new0 (gpointer, 1);

        /* User data are the lines we will execute. */
        closure =
            g_cclosure_new (G_CALLBACK (lba_on_cb), on_commands_list,
            on_commands_list_free);
        /* Set our custom passthrough marshaller */
        g_closure_set_marshal (closure, lba_on_marshal);
        /* memory management: alive while object is alive,
         * invalidated if object is freed */
        g_object_watch_closure (obj, closure);

        /* Connect our super closure. */
        g_signal_connect_closure (obj, signal_name, closure,
            /* execute after default handler = TRUE */
            TRUE);

        /* unref closure ?? */
      }

      /* All the next wishes until "end" must be stored */

      /* Now read all the wishes */


      goto done;
    }


    /* Set */
    if (!g_strcmp0 (tokens[0], "set")) {
      GValue inp = G_VALUE_INIT;
      GValue outp = G_VALUE_INIT;
      const gchar *prop_name, *prop_val;
      const gchar *objname;
      GObject *obj;
      GParamSpec *prop;

      tmp = g_strsplit (tokens[1], ".", 2);

      if (FALSE == (tmp && tmp[0] && tmp[1] && tokens[2])) {
        g_warning ("wrong syntax");
        goto done;
      }

      objname = tmp[0];
      prop_name = tmp[1];

      obj = g_hash_table_lookup (objects, objname);

      if (!obj) {
        g_warning ("object %s not found", objname);
        goto done;
      }

      /* Value is all the string after the second space */
      prop_val =
          g_strstr_len (g_strstr_len (a, sizeof (a), " ") + 1, -1, " ") + 1;

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
      if (!g_value_transform (&inp, &outp)) {
        g_warning ("unsupported parameter type");
        goto done;
      }

      g_printf ("setting %s.%s to %s\n", objname, prop_name, prop_val);
      g_object_set_property (obj, prop_name, &outp);
      goto done;
    }
  }

  g_warning ("unknown command");
done:
  if (tmp)
    g_strfreev (tmp);
  g_strfreev (tokens);
  return ret;
}


int
main (int argc, char **argv)
{
  GModule *module = NULL;
  gpointer ptr;
  lBaPluginSystemGetGtypeFunc get_type_f;
  int ret = 1;
  GSList *modules_files = NULL, *l;
  GDir *dir = NULL;
  gchar *script_contents = NULL;

  g_value_register_transform_func (G_TYPE_STRING, G_TYPE_BOOLEAN, _str2bool);
  g_value_register_transform_func (G_TYPE_STRING, G_TYPE_UINT, _str2uint);
  g_value_register_transform_func (G_TYPE_STRING, G_TYPE_DOUBLE, _str2double);
  g_value_register_transform_func (G_TYPE_STRING, G_TYPE_OBJECT, _str2obj);
  g_value_register_transform_func (G_TYPE_STRING, G_TYPE_GTYPE, _str2gtype);

  /* Parse arguments */
  {
    GOptionContext *ctx;
    GError *err = NULL;
    char *path = NULL;
    char *script = FALSE;
    GOptionEntry options[] = {
      {"script", 'i', 0, G_OPTION_ARG_STRING, &script,
          "Proccess input file before entering shell", NULL},
      {"path", 'p', 0, G_OPTION_ARG_STRING, &path,
          "Path to the plugins directory", NULL},
      {NULL}
    };

    ctx = g_option_context_new ("-i <script file> | -p <plugins path or file>");
    g_option_context_add_main_entries (ctx, options, NULL);
    if (!g_option_context_parse (ctx, &argc, &argv, &err)) {
      g_print ("Error initializing: %s\n", err->message);
      exit (1);
    }
    g_option_context_free (ctx);

    if (path) {
      if (!g_file_test (path, G_FILE_TEST_EXISTS)) {
        g_warning ("Path doesn't exist\n");
        return 1;
      }

      if (g_file_test (path, G_FILE_TEST_IS_DIR)) {
        GError *err;
        const gchar *file;
        dir = g_dir_open (path, 0, &err);
        if (!dir) {
          g_warning ("%s", err->message);
          g_error_free (err);
        }

        while ((file = g_dir_read_name (dir))) {
          if (g_str_has_suffix (file, B_PLUGIN_EXTENSION)) {
            modules_files = g_slist_append (modules_files,
                g_strjoin (B_OS_FILE_SEPARATOR, path, file, NULL));
          }
        }
      } else {
        modules_files = g_slist_append (modules_files, g_strdup (path));
      }
      g_free (path);
    }

    if (script) {
      gsize length = 0;
      if (!g_file_test (script, G_FILE_TEST_EXISTS)) {
        g_warning ("Script file doesn't exist\n");
        return 1;
      }

      if (g_file_get_contents (script, &script_contents, &length, NULL)
          && length) {
        script_contents[length - 1] = 0;
      }

      g_free (script);
    }
  }


  for (l = modules_files; l; l = l->next) {
    const gchar *module_filename = (const gchar *) l->data;

    module = g_module_open (module_filename, G_MODULE_BIND_LOCAL);
    if (!module) {
      // too noisy
//      g_warning ("Failed to load plugin '%s': %s", module_filename,
//          g_module_error ());
      continue;
    }

    if (!g_module_symbol (module, BOMBOLLA_PLUGIN_SYSTEM_ENTRY, &ptr)) {
//      g_warning ("File '%s' is not a plugin from my system", module_filename);
      g_module_close (module);
      module = NULL;
      continue;
    }

    /* If module is from plugin system - we don't want to unload it */
    g_module_make_resident (module);

    get_type_f = (lBaPluginSystemGetGtypeFunc) ptr;

    {
      GTypeQuery query;
      GType plugin_type = get_type_f ();

      g_printf ("======\nPlugin file %s:\n", module_filename);
      g_type_query (plugin_type, &query);
      g_printf ("GType name = '%s'\n", query.type_name);

      if (G_TYPE_IS_OBJECT (plugin_type)) {
        GObject *obj;
        GParamSpec **properties;
        guint n_properties, p;
        guint *signals, s, n_signals;

        g_printf ("GType is a GObject.\n");

        obj = g_object_new (plugin_type, NULL);

        properties =
            g_object_class_list_properties (G_OBJECT_GET_CLASS (obj),
            &n_properties);

        for (p = 0; p < n_properties; p++) {
          GParamSpec *prop = properties[p];
          gchar *def_val;

          def_val =
              g_strdup_value_contents (g_param_spec_get_default_value (prop));

          g_printf ("- Property: (%s) %s = %s\n",
              G_PARAM_SPEC_TYPE_NAME (prop),
              g_param_spec_get_name (prop), def_val);

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
                g_printf ("%s%s", p ? ", " : "",
                    g_type_name (query.param_types[p]));
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

        g_object_unref (obj);
        g_free (properties);
      }
    }
  }

  g_printf ("scanning finished, entering shell...\n"
      "commands:\n------------\n"
      "create <type name> <var name>\n"
      "destroy <var name>\n"
      "set <var name>.<property> <value>\n"
      "call <var name>.<signal> (params are not supported yet)\n"
      "on <var name>.<signal>\n" "q (quit)\n------------\n");

  /* HT for variables "var name"/obj */
  objects =
      g_hash_table_new_full (g_str_hash, g_str_equal, g_free, g_object_unref);

  /* Proccessing script */
  if (script_contents) {
    char **lines;
    int i;

    lines = g_strsplit (script_contents, "\n", 0);

    for (i = 0; lines[i]; i++) {
      if (!proccess_shell_string (lines[i]))
        break;
    }

    g_strfreev (lines);
    g_free (script_contents);
  }

  /* Shell */
  for (;;) {
    char a[512];

    /* Stdin input. */
    g_printf ("\n\t# ");
    fgets (a, sizeof (a), stdin);
    a[strlen (a) - 1] = 0;

    if (!proccess_shell_string (a))
      break;

  }

  ret = 0;
  if (module)
    g_module_close (module);
  if (dir)
    g_dir_close (dir);
  g_slist_free_full (modules_files, g_free);
  return ret;
}
