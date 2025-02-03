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
lba_command_create (GObject *core, const char *type_name, const char *var_name) {
  GType obj_type;

  LBA_LOG ("Hello from create command ([%s] --> [%s])", type_name, var_name);

  obj_type = g_type_from_name (type_name);

  if (G_UNLIKELY (obj_type == 0)) {
    g_warning ("Type %s not found", type_name);
    // trigger signal "report-error" on the core ??
    return;
  }

  if (BM_GTYPE_IS_BMIXIN (obj_type)) {
    obj_type = bm_register_mixed_type (NULL, G_TYPE_OBJECT, obj_type, NULL);

    LBA_LOG ("Type '%s' is a mixin, will use '%s'", type_name,
             g_type_name (obj_type));
  }

  GObject *obj = g_object_new (obj_type,
                               /* TODO: properties??
                                * could take a list as a parameter */
                               NULL);

  g_signal_emit_by_name (core, "add", obj, var_name);
  g_object_unref (obj);
}

BOMBOLLA_PLUGIN_SYSTEM_PROVIDE_COMMAND (create, LBA_COMMAND_SETUP_DEFAULT,
                                        G_TYPE_STRING, G_TYPE_STRING);
