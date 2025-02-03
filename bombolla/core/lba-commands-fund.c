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

typedef struct _LbaCommands {
  BMixinInstance i;
} LbaCommands;

typedef struct _LbaCommandsClass {
  BMixinClass c;
} LbaCommandsClass;

static void
lba_commands_init (GObject *object, LbaCommands *self) {
}

static void
lba_commands_class_init (GObjectClass *object_class, LbaCommandsClass *klass) {
}

BM_DEFINE_MIXIN (lba_commands, LbaCommands,
                 /* BM_ADD_DEP (lba_singletone)?? */
    );

GType
lba_command_fundamental_get_type (void) {
  GTypeInfo dummy = { 0 };
  static GType ret;
  GTypeFundamentalInfo finfo = { G_TYPE_FLAG_DERIVABLE };
  if (G_LIKELY (ret))
    return ret;

  return (ret = g_type_register_fundamental
          (g_type_fundamental_next (), "LbaCommand", &dummy, &finfo, 0));
}
