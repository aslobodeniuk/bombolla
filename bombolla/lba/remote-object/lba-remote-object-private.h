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

#ifndef __LBA_REMOTE_OBJECT_H__
#  define __LBA_REMOTE_OBJECT_H__

#  include <glib-object.h>

typedef struct _lbaRemoteObjectSignal {
  gchar *name;
  guint id;
  guint flags;
  GType return_type;
  guint n_params;
  GType *param_types;

  guint registered_id;
} lbaRemoteObjectSignal;

typedef struct _lbaRemoteObjectProperty {
  /* gchar *name; */
  /* GType type; */
  GValue cur_value;

  GParamSpec *pspec;
} lbaRemoteObjectProperty;

typedef struct _lbaRemoteObjectClassData {
  /* FIXME: refcount? */
  lbaRemoteObjectSignal *signals;
  guint signals_num;
  lbaRemoteObjectProperty *properties;
  guint properties_num;
} lbaRemoteObjectClassData;

/* Opaque types */
typedef struct _lbaRemoteObjectClient lbaRemoteObjectClient;
typedef struct _lbaRemoteObjectClass lbaRemoteObjectClass;
typedef struct _lbaRemoteObject lbaRemoteObject;

extern const guint remote_object_instance_size;
extern const guint remote_object_class_size;

void lba_remote_object_client_report_action (lbaRemoteObjectClient *,
                                             guint signal_id, GValue * return_value,
                                             guint n_param_values,
                                             const GValue * param_values);

void


lba_remote_object_class_init (lbaRemoteObjectClass * klass,
                              lbaRemoteObjectClassData * class_data);

void
  lba_remote_object_init (lbaRemoteObject * self);

/* ----------------------------------------------------------------- */

void lba_remote_object_setup (lbaRemoteObject * self,
                              lbaRemoteObjectClient * client_object,
                              GType remote_object_gtype);

#endif
