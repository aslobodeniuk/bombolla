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
#include <libsoup/soup.h>
#include <string.h>

enum
{
  SIGNAL_START,
  SIGNAL_STOP,
  SIGNAL_MESSAGE,
  LAST_SIGNAL
};

static guint http_server_signals[LAST_SIGNAL] = { 0 };

typedef enum
{
  PROP_PORT = 1,
} HTTPServerProperty;


typedef struct _HTTPServer
{
  GObject parent;

  int port;
  gboolean started;

  GThread *mainloop_thr;
  GMainContext *mainctx;
  GMainLoop *mainloop;

  SoupServer *soup;
} HTTPServer;


typedef struct _HTTPServerClass
{
  GObjectClass parent;

  void (*start) (HTTPServer *);
  void (*stop) (HTTPServer *);

} HTTPServerClass;

GEnumClass *http_server_message_response_class;

static void
http_server_soup_cb (SoupServer * server, SoupMessage * msg,
    const char *path, GHashTable * query,
    SoupClientContext * context, gpointer data)
{
  HTTPServer *self = (HTTPServer *) data;
  int response;
  const gchar *resp_str;

  LBA_LOG ("%s %s HTTP/1.%d\n", msg->method, path,
      soup_message_get_http_version (msg));

  /* Now finally upstream the message text to the client */
  g_signal_emit_by_name (self, "message", path, &response);

  {
    /* Now use our own enum !!! */
    GEnumValue *enum_value;
    enum_value =
        g_enum_get_value (http_server_message_response_class, response);

    LBA_LOG ("callback returned %d (%s)", response,
        enum_value ? enum_value->value_name : "unknown");
  }

  resp_str =
      response ==
      888 ? "Happily handled" : "No parser for this unknown request";

  /* Set ok-ish response */
  soup_message_set_response (msg, "text/html",
      SOUP_MEMORY_STATIC, resp_str, strlen (resp_str) + 1);
  soup_message_set_status (msg, SOUP_STATUS_OK);
}


static gpointer
http_server_mainloop (gpointer data)
{
  HTTPServer *self = (HTTPServer *) data;
  GError *error = NULL;

  LBA_LOG ("starting on port %d", self->port);

  self->mainctx = g_main_context_new ();
  self->mainloop = g_main_loop_new ( /*self->mainctx */ NULL, TRUE);

  self->soup = soup_server_new (        //"async-context", self->mainctx,
      "server-header", "Bombolla http server", NULL);

  /* Open some port */
  soup_server_listen_all (self->soup, self->port, 0, &error);

  soup_server_add_handler (self->soup, NULL, http_server_soup_cb, self, NULL);
  if (error) {
    LBA_LOG ("Just couldn't");
    /* FIXME: some cleanup, so we could start again */
    return NULL;
  }

  /* Proccessing events here until quit event will arrive */
  g_main_loop_run (self->mainloop);
  self->started = FALSE;
  LBA_LOG ("stopped");
  return NULL;
}


static void
http_server_start (HTTPServer * self)
{
  /* OMG, no thread safety at all. Needs a mutex */
  if (self->started)
    return;

  if (!self->mainloop_thr)
    self->mainloop_thr =
        g_thread_new ("HttpServerMainLoop", http_server_mainloop, self);

  self->started = TRUE;
}

/* Callback proccessed in the main loop.
 * Simply makes it quit. */
gboolean
http_server_quit_msg (gpointer data)
{
  HTTPServer *self = (HTTPServer *) data;

  g_main_loop_quit (self->mainloop);
  return G_SOURCE_REMOVE;
}


static void
http_server_stop (HTTPServer * self)
{
  GSource *s;

  /* Send quit message to the main loop */
  s = g_idle_source_new ();
  /* Drop all the other messages: high priority stuff */
  g_source_set_priority (s, G_PRIORITY_HIGH);

  g_source_attach (s, NULL);
  g_source_set_callback (s, http_server_quit_msg, self, NULL);

  /* Wait for main loop to quit */
  g_thread_join (self->mainloop_thr);
  g_source_destroy (s);
}


static void
http_server_init (HTTPServer * self)
{
}

static void
http_server_set_property (GObject * object,
    guint property_id, const GValue * value, GParamSpec * pspec)
{
  HTTPServer *self = (HTTPServer *) object;

  switch ((HTTPServerProperty) property_id) {
    case PROP_PORT:
      self->port = g_value_get_int (value);
      break;
    default:
      /* We don't have any other property... */
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

static void
http_server_get_property (GObject * object,
    guint property_id, GValue * value, GParamSpec * pspec)
{
  HTTPServer *self = (HTTPServer *) object;

  switch ((HTTPServerProperty) property_id) {
    case PROP_PORT:
      g_value_set_int (value, self->port);
      break;
    default:
      /* We don't have any other property... */
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

GType http_server_get_type (void);

static void
http_server_dispose (GObject * gobject)
{
  HTTPServer *self = (HTTPServer *) gobject;
  HTTPServerClass *klass = G_TYPE_INSTANCE_GET_CLASS ((self),
      http_server_get_type (), HTTPServerClass);

  if (self->started)
    klass->stop (self);
}


/* This is a signal accumulator: it is executed after every handler returns some
 * value, and is needed to make decisions:
 * 1) is other signal handlers should be called
 * 2) what to return from signal emission */
static gboolean
http_server_message_signal_accum
    (GSignalInvocationHint * ihint,
    GValue * return_accu, const GValue * handler_return, gpointer data)
{
  /* if handler returns 1234, it means "not handled". Otherwise handled.
   * so we'll return 888. */
  int ret;

  ret = g_value_get_enum (handler_return);
  if (ret == 1234) {
    /* Not handled, continue emission.
     * We must set the return value here, otherwise it will be 0 */
    g_value_set_enum (return_accu, 1234);
    return TRUE;
  }

  /* handled, set 888 return and stop emission */
  g_value_set_enum (return_accu, 888);
  return FALSE;
}


/* Not used, only for presentation, how marshaller can look like */
#if 0
/* Marshaller for signal of type "int (*myfunc) (GObject * obj, gchar * param, gpointer data)" */
void
lba_cclosure_marshal_ENUM__STRING (GClosure * closure,
    GValue * return_value,
    guint n_param_values,
    const GValue * param_values,
    gpointer invocation_hint G_GNUC_UNUSED, gpointer marshal_data)
{
  typedef gboolean (*GMarshalFunc_ENUM__STRING) (gpointer data1,
      gpointer arg_1, gpointer data2);

  GMarshalFunc_ENUM__STRING callback;
  GCClosure *cc = (GCClosure *) closure;
  gpointer data1, data2;
  gboolean v_return;

  g_return_if_fail (return_value != NULL);
  g_return_if_fail (n_param_values == 2);

  /* ---------------------------- Common marshaller's stuff */
  if (G_CCLOSURE_SWAP_DATA (closure)) {
    data1 = closure->data;
    data2 = g_value_peek_pointer (param_values + 0);
  } else {
    data1 = g_value_peek_pointer (param_values + 0);
    data2 = closure->data;
  }

  callback =
      (GMarshalFunc_ENUM__STRING) (marshal_data ? marshal_data : cc->callback);
  /* ----------------------------------------------------- */

  /* Execute callback */
  v_return = callback (data1, g_value_peek_pointer (param_values + 1), data2);

  /* Set return GValue */
  g_value_set_int (return_value, v_return);
}
#endif

static GType http_server_message_response;

static void
http_server_class_init (HTTPServerClass * klass)
{
  GObjectClass *object_class = (GObjectClass *) klass;

  klass->start = http_server_start;
  klass->stop = http_server_stop;

  object_class->set_property = http_server_set_property;
  object_class->get_property = http_server_get_property;
  object_class->dispose = http_server_dispose;

  http_server_signals[SIGNAL_MESSAGE] = g_signal_new ("message",
      G_TYPE_FROM_CLASS (klass), G_SIGNAL_RUN_LAST, 0,       // no default handler is set
      http_server_message_signal_accum, // <-- accumulator function for return values
      NULL,                     // user data for accumulator
      NULL,                     // marshaller, could be lba_cclosure_marshal_ENUM__STRING
      /* Returns enum, has one string parameter */
      http_server_message_response,               /* return type is our own enum */
      1, G_TYPE_STRING, G_TYPE_NONE);

  http_server_signals[SIGNAL_START] =
      g_signal_new ("start", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
      G_STRUCT_OFFSET (HTTPServerClass, start), NULL, NULL,
      NULL, G_TYPE_NONE, 0, G_TYPE_NONE);

  http_server_signals[SIGNAL_STOP] =
      g_signal_new ("stop", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
      G_STRUCT_OFFSET (HTTPServerClass, stop), NULL, NULL,
      NULL, G_TYPE_NONE, 0, G_TYPE_NONE);

  g_object_class_install_property (object_class,
      PROP_PORT,
      g_param_spec_int ("port",
          "PORT", "Port",
          G_MININT, G_MAXINT, 8000,
          G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE | G_PARAM_CONSTRUCT));

  /* If class was created, cache emum class too */
  http_server_message_response_class =
      g_type_class_ref (http_server_message_response);
}

/* Now our custom boilerplate: we also want to register our enums */
GType
http_server_get_type (void)
{
  /* FIXME: better use g_once_init_enter */
  static GType http_server_type = 0;

  if (!http_server_type) {
    static const GTypeInfo http_server_info = {
      sizeof (HTTPServerClass),
      NULL,
      NULL,
      (GClassInitFunc) http_server_class_init,
      NULL,
      NULL,
      sizeof (HTTPServer),
      0,
      (GInstanceInitFunc) http_server_init,
    };

    /* register enum: why we wanted to write a custom boilerplate */
    {
      /* This variable MUST be static following the documentation */
      static const GEnumValue enm[] = {
        {1234, "NOT_HANDLED_YET", "not_handled_yet"},
        {888, "HAPPILY_HANDLED", "happily_handled"},
        {0, NULL, NULL}
      };

      http_server_message_response =
          g_enum_register_static ("HTTPServerMessageResponse", enm);
    }

    http_server_type = g_type_register_static (G_TYPE_OBJECT,
        "HTTPServer", &http_server_info, 0);
  }

  return http_server_type;
}


/* Export plugin */
BOMBOLLA_PLUGIN_SYSTEM_PROVIDE_GTYPE (http_server);
