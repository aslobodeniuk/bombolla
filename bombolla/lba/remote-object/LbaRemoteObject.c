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

#include "bombolla/lba-plugin-system.h"
#include "bombolla/lba-log.h"
#include <string.h>

typedef enum
{
  PROP_SOURCE = 1,
} LbaRemoteObjectProperty;


typedef struct _LbaRemoteObject
{
  GObject parent;
  GObject *source;
} LbaRemoteObject;


typedef struct _LbaRemoteObjectClass
{
  GObjectClass parent;
} LbaRemoteObjectClass;


static void
lba_remote_object_set_property (GObject * object,
    guint property_id, const GValue * value, GParamSpec * pspec)
{
  LbaRemoteObject *self = (LbaRemoteObject *) object;

  switch ((LbaRemoteObjectProperty) property_id) {
    case PROP_SOURCE:
      if (self->source) {
        g_critical ("Changing the source on fly is not supported yet");
      }

      self->source = g_value_get_object (value);

      if (self->source) {
        g_object_ref (self->source);
        
        // Here we must:
        // 1) Dump object's class: client remote object
        // decides to connect, the first thing we do is
        // send the class, so client could instantiate an
        // object with the same properties and signals.
        
        // 2) connect to all the event signals. If source object
        // emits an event signal, we send it to the client.
        // Also connect to all the notify:: signals for the same
        // reason.

        // 3) start listening on the data input: this input is
        // can trigger setting the properties or calling action
        // signals

        // ------------------------------------------------

        // So, we basically need:
        // - one data output point to write
        // - one data input point to read
        // For the beginning it can be just unix pipes
        // Perfect chance to play with GIO.

        // ------------------------------------------------
        // Now let's define a protocol to dump the class.
        // [KlassDump0000000] - magic number, 16 bytes
        // [Signal0000000000] - magic says there's a signal, 16 bytes
        // [flags] - 4 bytes. (reserved)
        // [Number of parameters] - 2 bytes. Last bit says if it has return value or not.
        // (optionally) [name of GType of the return value: size / string]
        // [name of GType of the parameter: size / string]

        write (output, "KlassDump0000000", 16);
        for (;;) {
          write (output, "Signal0000000000", 16);
          write (output, &num_params, 2);
          for (;;) {
            write (output, &string_size, 2);
            write (output, string, string_size);
          }
        }

        // How do we write?
        // We have some objects of type LbaStream: output-stream, input-stream
        // LbaStream has actions "write", "read", "seek", and has event "have-data"
        // So, we call something like
        // g_signal_emit_by_name (self->output_stream, "write", data, size, &ret);
        
      }

      break;
    default:
      /* We don't have any other property... */
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}


static void
lba_remote_object_get_property (GObject * object,
    guint property_id, GValue * value, GParamSpec * pspec)
{
  LbaRemoteObject *self = (LbaRemoteObject *) object;

  switch ((LbaRemoteObjectProperty) property_id) {
    case PROP_SOURCE:
      // Probably other side (client) could get the "ghost" object through
      // getting this property (if the connection is established).

      // If so, what we need:
      // 1) First receive class dump, and build a new class.
      // 2) Create an object of that class
      // 3) listen on the input, and emit event signals, set properties,
      // so the object also emits notify:: .
      // 4) On action signals dump the data and send it to the output
      // 5) Same when someone sets object's properties.
      g_value_set_object (value, self->source);
      break;
    default:
      /* We don't have any other property... */
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}



static void
lba_remote_object_init (LbaRemoteObject * self)
{
}

static void
lba_remote_object_dispose (GObject * gobject)
{
}

static void
lba_remote_object_class_init (LbaRemoteObjectClass * klass)
{
  GObjectClass *object_class = (GObjectClass *) klass;

  object_class->set_property = lba_remote_object_set_property;
  object_class->get_property = lba_remote_object_get_property;
  object_class->dispose = lba_remote_object_dispose;

  g_object_class_install_property (object_class,
      PROP_SOURCE,
      g_param_spec_object ("source",
          "Source", "Source object we bind in remote",
          G_TYPE_OBJECT, G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE));
}


G_DEFINE_TYPE (LbaRemoteObject, lba_remote_object, G_TYPE_OBJECT)

/* Export plugin */
    BOMBOLLA_PLUGIN_SYSTEM_PROVIDE_GTYPE (lba_remote_object);
