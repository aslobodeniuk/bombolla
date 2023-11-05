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

#ifndef _BOMBOLLA_COMMANDS
#  define _BOMBOLLA_COMMANDS

typedef struct {
  GHashTable *objects;
  GHashTable *bindings;
  gpointer capturing_on_command;

  gpointer self;
    gboolean (*proccess_command) (gpointer self, const gchar * str);

} BombollaContext;

typedef struct {
  const gchar *name;
    gboolean (*parse) (BombollaContext * ctx, gchar ** tokens);
} BombollaCommand;

extern const BombollaCommand commands[];

/* bombolla-command-on.c */
gboolean lba_command_on_append (gpointer ctx_ptr, const gchar * command);
gboolean lba_command_on (BombollaContext * ctx, gchar ** tokens);

/* bombolla-command-set.c */
gboolean lba_command_set (BombollaContext * ctx, gchar ** tokens);
gboolean
lba_core_parse_obj_fld (BombollaContext * ctx, const gchar * str, GObject ** obj,
                        gchar ** fld);
void lba_core_init_convertion_functions (void);

void lba_core_shedule_async_script (GObject * obj, gchar * command);
void lba_core_sync_with_async_cmds (gpointer core);

#endif
