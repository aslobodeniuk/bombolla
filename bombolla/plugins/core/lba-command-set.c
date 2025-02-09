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
lba_command_set (GObject *core, GObject *obj, const gchar *prop,
                 /* Fixme: better to get an abstract value here */
                 const gchar *strval) {
  GValue in = G_VALUE_INIT;
  GValue out = G_VALUE_INIT;
  GParamSpec *p;

  g_return_if_fail (obj != NULL);
  g_return_if_fail (prop != NULL);

  LBA_LOG ("Setting [%s] <-- [%s]", prop, strval);

  p = g_object_class_find_property (G_OBJECT_GET_CLASS (obj), prop);
  if (G_UNLIKELY (NULL == p)) {
    g_warning ("property %s not found", prop);
    /* TODO: emit core error */
    return;
  }

  g_value_init (&in, G_TYPE_STRING);
  g_value_set_string (&in, strval);
  g_value_init (&out, p->value_type);

  /* Now set outp */
  if (G_TYPE_IS_OBJECT (p->value_type)) {
    GObject *o = NULL;

    g_signal_emit_by_name (core, "pick", strval, &o);
    if (G_UNLIKELY (NULL == o)) {
      g_warning ("Object %s not found", strval);
      return;
    }

    g_value_take_object (&out, o);
  } else if (!g_value_transform (&in, &out)) {
    g_warning ("could not transform [%s]-->[%s]", strval,
               g_type_name (p->value_type));
    goto done;
  }

  g_object_set_property (obj, prop, &out);
done:
  g_value_unset (&in);
  g_value_unset (&out);
}

BOMBOLLA_PLUGIN_SYSTEM_PROVIDE_COMMAND (set, LBA_COMMAND_SETUP_DEFAULT,
                                        G_TYPE_OBJECT, G_TYPE_STRING, G_TYPE_STRING);
