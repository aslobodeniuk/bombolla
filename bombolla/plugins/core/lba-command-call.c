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
#include <bombolla/core/lba-list.h>

static void
lba_command_call (GObject *core, GObject *obj,
                  const char *signame, const LbaList *commands) {
  guint signal_id;
  GSignalQuery query;
  GValue return_value = G_VALUE_INIT;
  GArray *instance_and_params = NULL;

  {
    gchar *x = lba_list_to_string (commands);

    LBA_LOG ("Hello from call command ([%s] --> [%s])", signame, x);
    g_free (x);
  }

  signal_id = g_signal_lookup (signame, G_OBJECT_TYPE (obj));
  if (!signal_id) {
    g_warning ("No signal '%s'", signame);
    /* TODO: Core error */
    return;
  }

  g_signal_query (signal_id, &query);
  if (G_UNLIKELY (query.n_params != lba_list_length (commands))) {
    g_warning ("Signal '%s' has different amount of parameters (%d) "
               "then the list provides (%d)", signame, query.n_params,
               lba_list_length (commands));
    /* TODO: Core error. We report the error string. */
    return;
  }

  /* Fixme: use LbaList instead */
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

  for (guint p = 0; p < query.n_params; p++) {
    GValue param = G_VALUE_INIT;
    GType ptype = query.param_types[p];
    const GValue *v = lba_list_index (commands, p);

    // check if we can process this param
    if (!G_TYPE_IS_OBJECT (ptype) &&
        !g_value_type_transformable (G_VALUE_TYPE (v), ptype)) {
      g_warning ("[%s]: don't know how to set parameter %d of type %s",
                 signame, p, g_type_name (ptype));
      goto done;
    }
    // so now we need to get the param
    {
      g_value_init (&param, ptype);
      if (!g_value_transform (v, &param)) {
        g_warning ("%s(%d): could not transform [%s] --> [%s]",
                   signame, p, g_type_name (G_VALUE_TYPE (v)), g_type_name (ptype));
        g_value_unset (&param);
        /* TODO: Core error. We report the error string. */
        goto done;
      }
    }

    g_array_append_val (instance_and_params, param);
  }

  LBA_LOG ("calling %s()", signame);
  g_signal_emitv ((GValue *) instance_and_params->data, signal_id, 0, &return_value);
done:
  g_value_unset (&return_value);
  if (instance_and_params)
    g_array_unref (instance_and_params);
}

BOMBOLLA_PLUGIN_SYSTEM_PROVIDE_COMMAND (call, LBA_COMMAND_SETUP_DEFAULT, G_TYPE_OBJECT, /* obj */
                                        G_TYPE_STRING,  /* signal */
                                        lba_list_get_type ()    /* params */
    );
