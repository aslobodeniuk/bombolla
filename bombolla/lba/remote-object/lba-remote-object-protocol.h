/* la Bombolla GObject shell.
 * Copyright (C) 2022 Aleksandr Slobodeniuk
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

#ifndef __LBA_REMOTE_OBJECT_PROTOCOL_H__
#  define __LBA_REMOTE_OBJECT_PROTOCOL_H__

#  include <glib-object.h>

const guchar *lba_robj_protocol_parse_pspec (const guchar * msg, GParamSpec **);

const guchar *lba_robj_protocol_parse_int (const guchar * msg, gint * ret);

const guchar *lba_robj_protocol_parse_uint (const guchar * msg, guint * ret);

const guchar *lba_robj_protocol_parse_gvalue (const guchar * msg, GType value_type,
                                              GValue * ret);

const guchar *lba_robj_protocol_parse_string (const guchar * msg,
                                              const gchar ** ret);

const guchar *lba_robj_protocol_parse_name (const guchar * msg, const gchar ** ret);

const guchar *lba_robj_protocol_parse_gtype (const guchar * msg, GType * ret);

#endif
