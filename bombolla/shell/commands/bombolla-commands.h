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

typedef struct {
  GHashTable *objects;
  gpointer capturing_on_command;
  GObject *self;
  gboolean (*proccess_command) (GObject *obj, const gchar * str);

} BombollaContext;

typedef struct {
  const gchar *name;
  gboolean (*parse) (BombollaContext *ctx, gchar **tokens);
} BombollaCommand;

extern const BombollaCommand commands[];

/* bombolla-command-on.c */
gboolean lba_command_on_append (gpointer ctx_ptr, const gchar *command);
gboolean lba_command_on (BombollaContext *ctx, gchar **tokens);

/* bombolla-command-set.c */
gboolean lba_command_set (BombollaContext *ctx, gchar **tokens);
