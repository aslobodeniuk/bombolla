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
lba_command_dna (GObject *core, const gchar *mixed_type_name,
                 const gchar *base_name, const gchar *mixins) {
  gint t;
  GType base_type;
  gchar **tokens = NULL;

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

  if (NULL == tokens || tokens[0] == NULL) {
    g_warning ("No mixins listed");
    goto syntax;
  }

  for (t = 0; tokens[t]; t++) {
    /* In fact we have to register various intermediate mixed_type classes in order
     * to reach the requested one */
    const gchar *mixin_name = tokens[t];
    GType mixin_type = g_type_from_name (mixin_name);

    /* Final type */
    if (tokens[t + 1] == NULL) {
      base_type =
          bm_register_mixed_type (mixed_type_name, base_type, mixin_type, NULL);
    } else {
      /* Intermediate type */
      base_type = bm_register_mixed_type (NULL, base_type, mixin_type, NULL);
    }

    base_name = g_type_name (base_type);

    if (0 == base_type) {
      g_warning ("Error occured");
      goto freee;
    }

    g_message ("Have type %s", g_type_name (base_type));
  }

freee:
  g_strfreev (tokens);
  return;
syntax:
  g_error ("Syntax error. Expected: "
           "dna <mixed type name> <base type> <mixin 1> ... <mixin N>");
  goto freee;

}

BOMBOLLA_PLUGIN_SYSTEM_PROVIDE_COMMAND (dna, LBA_COMMAND_SETUP_DEFAULT,
                                        /* name */
                                        G_TYPE_STRING,
                                        /* base name */
                                        G_TYPE_STRING,
                                        /* dna */
                                        G_TYPE_STRING,);
