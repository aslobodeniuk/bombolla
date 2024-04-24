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

#ifndef _BOMBOLLA_PLUGIN_SYSTEM
#  define _BOMBOLLA_PLUGIN_SYSTEM
#  include <glib-object.h>

typedef GType (*lBaPluginSystemGetGtypeFunc) (void);

#  define BOMBOLLA_PLUGIN_SYSTEM_ENTRY_   bombolla_plugin_system_get_gtype
#  define BOMBOLLA_PLUGIN_SYSTEM_ENTRY   G_STRINGIFY (BOMBOLLA_PLUGIN_SYSTEM_ENTRY_)

/* This is a variable to cache plugin's type's name for logging */
G_GNUC_UNUSED static const gchar *global_lba_plugin_name;

#  define BOMBOLLA_PLUGIN_SYSTEM_PROVIDE_GTYPE(name)                    \
  GType BOMBOLLA_PLUGIN_SYSTEM_ENTRY_ (void)                            \
  {                                                                     \
    GType ret = name##_get_type ();                                     \
    if (!ret)                                                           \
      return 0;                                                         \
    global_lba_plugin_name = g_type_name (ret);                         \
    return ret;                                                         \
  }

#endif
