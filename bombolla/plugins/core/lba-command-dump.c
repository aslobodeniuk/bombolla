/* la Bombolla GObject shell
 *
 * Copyright (c) 2025, Alexander Slobodeniuk <aleksandr.slobodeniuk@gmx.es>
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

#include <bombolla/lba-log.h>
#include <bmixin/bmixin.h>

static void
lba_command_dump_type (GType plugin_type) {
  GTypeQuery query = { 0 };

  g_type_query (plugin_type, &query);
  if (query.type)
    g_printf ("Dumping GType '%s'\n", query.type_name);

  if (BM_GTYPE_IS_BMIXIN (plugin_type)) {
    g_printf ("Mixin %s\n", g_type_name (plugin_type));
    lba_command_dump_type (bm_register_mixed_type
                           (NULL, G_TYPE_OBJECT, plugin_type, NULL));
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
    GList *tree = NULL;

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

        tree = g_list_prepend (tree, (gpointer) t);
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
      g_free (tab);
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

    {
      gint i;
      gchar offset[256] = { 0 };
      GList *l;

      g_printf ("\nInheiritance tree:\n");
      for (l = tree, i = 0; (l); l = l->next) {
        if (i < (256 - 2))
          i += 2;
        memset (offset, '-', i);
        offset[i] = 0;
        g_printf ("%s%s\n", offset, g_type_name ((GType) l->data));
      }
    }

    g_type_class_unref (klass);
    g_free (properties);
    g_list_free (tree);
  }
}

static void
lba_command_dump (GObject *core, const char *name) {
  GType t;

  g_return_if_fail (name != NULL);
  LBA_LOG ("Dumping [%s]", name);

  t = g_type_from_name (name);
  if (0 == t) {
    GObject *obj = NULL;

    g_signal_emit_by_name (core, "pick", name, &obj);
    if (G_UNLIKELY (!obj)) {
      g_warning ("Neither type nor an object '%s' haven't been found", name);
      // trigger signal "report-error" on the core ??

      return;
    }

    t = G_OBJECT_TYPE (obj);
  }

  lba_command_dump_type (t);

}

BOMBOLLA_PLUGIN_SYSTEM_PROVIDE_COMMAND (dump, LBA_COMMAND_SETUP (
                                                                  /* This is supposed to be a bit faster */
                                                                  .c_marshaller =
                                                                  g_cclosure_marshal_VOID__STRING),
                                        G_TYPE_STRING);
