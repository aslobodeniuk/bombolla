/* la Bombolla GObject shell.
 * Copyright (C) 2020 Aleksandr Slobodeniuk
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

enum {
  SIGNAL_FOO,
  SIGNAL_BAR,
  LAST_SIGNAL
};

static guint foobar_parser_signals[LAST_SIGNAL] = { 0 };

typedef enum {
  PROP_SOURCE = 1,
} FooBarParserProperty;

typedef struct _FooBarParser {
  GObject parent;
  GObject *source;
  gulong source_signal_id;
} FooBarParser;

typedef struct _FooBarParserClass {
  GObjectClass parent;
} FooBarParserClass;

static gint
foobar_parser_on_message_cb (GObject * source, const gchar * msg,
                             FooBarParser * self) {
  gint ret = 1234;

  if (strstr (msg, "foo")) {
    LBA_LOG ("foo");
    g_signal_emit (self, foobar_parser_signals[SIGNAL_FOO], 0);
    ret = 333;
  } else if (strstr (msg, "bar")) {
    LBA_LOG ("bar");
    g_signal_emit (self, foobar_parser_signals[SIGNAL_BAR], 0);
    ret = 444;
  }

  return ret;
}

static void
foobar_parser_source_free (FooBarParser * self) {
  if (self->source) {
    /* If source already have been set - free the previous one. */
    g_signal_handler_disconnect (self->source, self->source_signal_id);
    g_object_unref (self->source);
    self->source = NULL;
  }
}

static void
foobar_parser_set_property (GObject * object,
                            guint property_id, const GValue * value,
                            GParamSpec * pspec) {
  FooBarParser *self = (FooBarParser *) object;

  switch ((FooBarParserProperty) property_id) {
  case PROP_SOURCE:
    foobar_parser_source_free (self);

    self->source = g_value_get_object (value);

    if (self->source) {
      g_object_ref (self->source);
      /* TODO: check it has signal, and check signal signature */
      self->source_signal_id = g_signal_connect (self->source, "message",
                                                 G_CALLBACK
                                                 (foobar_parser_on_message_cb),
                                                 self);
    }

    break;
  default:
    /* We don't have any other property... */
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}

static void
foobar_parser_get_property (GObject * object,
                            guint property_id, GValue * value, GParamSpec * pspec) {
  FooBarParser *self = (FooBarParser *) object;

  switch ((FooBarParserProperty) property_id) {
  case PROP_SOURCE:
    g_value_set_object (value, self->source);
    break;
  default:
    /* We don't have any other property... */
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}

static void
foobar_parser_init (FooBarParser * self) {
}

static void
foobar_parser_dispose (GObject * gobject) {
  FooBarParser *self = (FooBarParser *) gobject;

  foobar_parser_source_free (self);
}

static void
foobar_parser_class_init (FooBarParserClass * klass) {
  GObjectClass *object_class = (GObjectClass *) klass;

  object_class->set_property = foobar_parser_set_property;
  object_class->get_property = foobar_parser_get_property;
  object_class->dispose = foobar_parser_dispose;

  foobar_parser_signals[SIGNAL_FOO] =
      g_signal_new ("foo", G_TYPE_FROM_CLASS (klass),
                    G_SIGNAL_RUN_LAST,
                    0, NULL, NULL,
                    g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0, G_TYPE_NONE);

  foobar_parser_signals[SIGNAL_BAR] =
      g_signal_new ("bar", G_TYPE_FROM_CLASS (klass),
                    G_SIGNAL_RUN_LAST,
                    0, NULL, NULL,
                    g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0, G_TYPE_NONE);

  g_object_class_install_property (object_class,
                                   PROP_SOURCE,
                                   g_param_spec_object ("source",
                                                        "Source",
                                                        "Source of messages we'll parse",
                                                        G_TYPE_OBJECT,
                                                        G_PARAM_STATIC_STRINGS |
                                                        G_PARAM_READWRITE));
}

G_DEFINE_TYPE (FooBarParser, foobar_parser, G_TYPE_OBJECT)
/* Export plugin */
    BOMBOLLA_PLUGIN_SYSTEM_PROVIDE_GTYPE (foobar_parser);
