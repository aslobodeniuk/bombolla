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

#include "lba-remote-object-private.h"
#include "bombolla/lba-plugin-system.h"
#include "bombolla/lba-log.h"
#include "bombolla/base/lba-byte-stream.h"
#include <string.h>

typedef struct _lbaRemoteObject {
  GObject parent;

  lbaRemoteObjectClass *klass;

  /* Has to be set through lba_remote_object_set_client() after object creation. */
  lbaRemoteObjectClient *remote_client_object;

} lbaRemoteObject;

typedef struct _lbaRemoteObjectClass {
  GObjectClass parent;

  lbaRemoteObjectClassData *data;

} lbaRemoteObjectClass;

const guint remote_object_instance_size = sizeof (lbaRemoteObject);
const guint remote_object_class_size = sizeof (lbaRemoteObjectClass);

void
lba_remote_object_setup (lbaRemoteObject * self,
                         lbaRemoteObjectClient * client_object,
                         GType remote_object_gtype) {
  g_return_if_fail (client_object);
  g_return_if_fail (remote_object_gtype);

  self->remote_client_object = client_object;
  self->klass = (lbaRemoteObjectClass *) g_type_class_peek (remote_object_gtype);

  g_return_if_fail (self->klass);
}

guint
lba_remote_object_get_original_signal_id (lbaRemoteObject * self, guint id) {
  gint i;

  lbaRemoteObjectClass *klass = self->klass;

  g_assert (klass->data);
  g_assert (klass->data->signals_num);
  g_assert (id);

  for (i = 0; i < klass->data->signals_num; i++) {
    if (klass->data->signals[i].registered_id == id)
      return klass->data->signals[i].id;
  }

  /* can never happen */
  g_abort ();
}

void
lba_remote_object_action_report_marshal (GClosure * closure,
                                         GValue * return_value,
                                         guint n_param_values,
                                         const GValue * param_values,
                                         gpointer invocation_hint,
                                         gpointer marshal_data) {
  /* NOTE: Marshal! Marshal! Can you hear me? Is everything ok in this function?
   * Marshal: Copy that.. Not sure if G_CCLOSURE_SWAP_DATA() is possible here..
   * I would say no. */

  lbaRemoteObject *self = (lbaRemoteObject *) closure->data;
  GSignalInvocationHint *hint = (GSignalInvocationHint *) invocation_hint;
  gint client_side_signal_id;

  g_assert (self->remote_client_object);

  client_side_signal_id =
      lba_remote_object_get_original_signal_id (self, hint->signal_id);
  g_assert (client_side_signal_id);
  /* Also here remote-client object blocks the call until it
   * receives an answer from the server. It is actually possible and
   * absolutely normal, that on the server side original object will block
   * the call for whatever amount of time. */
  lba_remote_object_client_report_action (self->remote_client_object,
                                          client_side_signal_id, return_value,
                                          n_param_values, param_values);
}

static void
lba_remote_object_get_property (GObject * object,
                                guint property_id, GValue * value,
                                GParamSpec * pspec) {
  /* lbaRemoteObject *self = (lbaRemoteObject *) object; */

  /* Someone is asking for the property value. Just return it. */
  /* g_assert (self->properties_num > property_id); */

  /* LBA_LOG ("Return property %s.", self->properties[property_id].pspec->name); */

  /* g_value_copy (&self->properties[property_id].cur_value, value); */

  /* FIXME: call the client */
}

static void
lba_remote_object_set_property (GObject * object,
                                guint property_id, const GValue * value,
                                GParamSpec * pspec) {
  /* lbaRemoteObject *self = (lbaRemoteObject *) object; */

  /* Someone sets the property. Just update the cur_value, no need to do
   * anything else. */

  /* g_assert (self->properties_num > property_id); */

  /* LBA_LOG ("Update property %s.", self->properties[property_id].pspec->name); */

  /* g_value_unset (&self->properties[property_id].cur_value); */
  /* g_value_copy (value, &self->properties[property_id].cur_value); */

  /* FIXME: call the client */
}

static void
lba_remote_object_dispose (GObject * gobject) {
}

void
lba_remote_object_init (lbaRemoteObject * self) {
  /* Copy  */
}

void
lba_remote_object_class_init (lbaRemoteObjectClass * klass,
                              lbaRemoteObjectClassData * class_data) {

  GObjectClass *object_class = (GObjectClass *) klass;

  klass->data = class_data;

  object_class->set_property = lba_remote_object_set_property;
  object_class->get_property = lba_remote_object_get_property;
  object_class->dispose = lba_remote_object_dispose;

  /* Now install properties and signals */
  if (class_data->properties) {
    gint i;

    for (i = 0; i < class_data->properties_num; i++) {
      lbaRemoteObjectProperty *prop;

      prop = &class_data->properties[i];

      LBA_LOG ("Installing property %s of type %s", prop->pspec->name,
               g_type_name (prop->pspec->value_type));

      g_object_class_install_property (object_class, i, prop->pspec);

      /* TODO: copy the the class */
    }
  }

  if (class_data->signals) {
    gint i;

    for (i = 0; i < class_data->signals_num; i++) {
      GSignalCMarshaller marshal;
      lbaRemoteObjectSignal *sig;

      sig = &class_data->signals[i];

      /* There're 2 types of signals: action signals and event signals.
       * --------------------------------------------------------------
       * Action signals can be called from outside by the user on client side.
       * Event signals are going to be triggered by the LbaRemoteObjectClient when
       * it receives a message from the server. Once LbaRemoteObject receives such
       * signal from LbaRemoteObjectClient, it triggers an event for the user.
       *
       * These 2 signal types are 2 different cases:
       * - Action signal is started by the user and
       * then we pass all its data to the LbaRemoteObjectClient, so
       * LbaRemoteObjectClient will send this data to the server.
       * Scheme: user -> LbaRemoteObject -> LbaRemoteObjectClient -> bytestream -> LbaRemoteObjectServer -> original object
       *
       * - Event signal has to call a C callback for the
       * user of the remote object. LbaRemoteObjectClient receives the event message
       * from the bytesteam, and calls us, so we have all the data, and we emit the
       * signal.
       * */

      if (sig->flags & G_SIGNAL_ACTION) {
        marshal = lba_remote_object_action_report_marshal;
      } else {
        /* If not we trust default automatic libffi marshaller,
         * lets see.. */
        marshal = NULL;
      }

      /* FIXME: does it really work without class closure? */
      klass->data->signals[i].registered_id =
          g_signal_newv (sig->name, G_TYPE_FROM_CLASS (klass), sig->flags,
                         /* GClosure *class_closure */ NULL,
                         /* GSignalAccumulator accumulator */ NULL,
                         /* gpointer accu_data */ NULL,
                         marshal, sig->return_type, sig->n_params, sig->param_types);
    }
  }
}
