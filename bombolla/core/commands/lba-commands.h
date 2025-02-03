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

#ifndef _BOMBOLLA_COMMANDS
#  define _BOMBOLLA_COMMANDS

typedef struct {
  GHashTable *objects;
  GHashTable *bindings;

  gpointer self;
  void (*proccess_command) (gpointer self, const gchar * expr, guint len);

} BombollaContext;

typedef struct {
  const gchar *name;
    gboolean (*parse) (BombollaContext * ctx, const gchar * expr, guint len);
} BombollaCommand;

extern const BombollaCommand commands[];

/* bombolla-command-on.c */
gboolean lba_command_on (BombollaContext * ctx, const gchar * expr, guint len);

/* bombolla-command-set.c */
gboolean lba_command_set (BombollaContext * ctx, const gchar * expr, guint len);
gboolean
lba_core_parse_obj_fld (BombollaContext * ctx, const gchar * str, GObject ** obj,
                        gchar ** fld);
void lba_core_init_convertion_functions (void);

void lba_core_shedule_async_script (GObject * obj, gchar * command);
void lba_core_sync_with_async_cmds (gpointer core);

gboolean
lba_command_set_str2obj (BombollaContext * ctx,
                         const GValue * src_value, GValue * dest_value);

gchar **FIXME_adapt_to_old (const gchar * expr, guint len);

const gchar *lba_expr_parser_find_next (const gchar * expr, guint total,
                                        guint * len);
#endif
