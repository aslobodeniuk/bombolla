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

#include "bombolla/lba-plugin-system.h"
#include "bombolla/lba-log.h"
#include <glib-object.h>

typedef struct _LbaClock {
  GObject parent;

  /* TODO: mutex */
  guint64 tick_interval;
  guint source_id;
} LbaClock;

typedef struct _LbaClockClass {
  GObjectClass parent;
} LbaClockClass;

typedef enum {
  PROP_TICK_INTERVAL = 1,
  PROP_CURRENT_TIME,
  N_PROPERTIES
} LbaClockProperty;

G_DEFINE_TYPE (LbaClock, lba_clock, G_TYPE_OBJECT);

static gboolean
lba_clock_tick (gpointer ptr) {
  LbaClock *self = (LbaClock *) ptr;

  LBA_LOG ("%p, Timer notify", self);

  g_object_notify (G_OBJECT (self), "current-time");
  return G_SOURCE_CONTINUE;
}

static void
lba_clock_set_property (GObject * object,
                        guint property_id, const GValue * value,
                        GParamSpec * pspec) {
  LbaClock *self = (LbaClock *) object;

  switch ((LbaClockProperty) property_id) {
  case PROP_TICK_INTERVAL:{

      guint64 tick = g_value_get_uint (value);

      /* MUTEX_LOCK */
      if (self->tick_interval != tick) {
        /* Remove old timer */
        if (self->tick_interval != 0) {
          g_source_remove (self->source_id);
        }

        if (tick != 0) {
#if 0
          /* #include <sys/timerfd.h> */
          /* #include <glib-unix.h> */

          /* wtf this doesn't work ??
           * It's dispatched all the time!!
           * Need to read the code of g_socket. */
          int tfd = timerfd_create (CLOCK_MONOTONIC, 0 /* TFD_NONBLOCK */ );

          struct itimerspec t;

          t.it_interval.tv_sec = tick / 1000;
          t.it_interval.tv_nsec = tick * 1000000 - t.it_interval.tv_sec * 1000000000;
          t.it_value.tv_sec = 0;
          t.it_value.tv_nsec = 1;       /* First expiration is immediate */

          timerfd_settime (tfd, 0, &t, NULL);

          GSource *source = g_unix_fd_source_new (tfd, G_IO_IN);
#else
          /* What a shame - no cool unix stuff :/ */
          GSource *source = g_timeout_source_new (tick);
#endif

          g_source_set_callback (source, lba_clock_tick, self, NULL);

          /* So we will need to remove this source when the
           * tick-interval changes OR when we teardown */
          self->source_id = g_source_attach (source,
                                             /* lba-core main context */
                                             NULL);
        }
      }

      self->tick_interval = tick;
      /* MUTEX UNLOCK */
      break;
    }
  default:
    /* We don't have any other property... */
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}

static void
lba_clock_get_property (GObject * object,
                        guint property_id, GValue * value, GParamSpec * pspec) {
  LbaClock *self = (LbaClock *) object;

  switch ((LbaClockProperty) property_id) {
  case PROP_TICK_INTERVAL:
    g_value_set_uint (value, self->tick_interval);
    break;
  case PROP_CURRENT_TIME:{
      g_value_take_boxed (value, g_date_time_new_now_local ());
      break;
    }
  default:
    /* We don't have any other property... */
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}

static void
lba_clock_init (LbaClock * self) {
}

static void
lba_clock_dispose (GObject * gobject) {
  LbaClock *self = (LbaClock *) gobject;

  /* NOTE: is there a race condition?
   * Like if the source is executed right now?? */
  if (self->tick_interval != 0) {
    g_source_remove (self->source_id);
    self->tick_interval = 0;
  }

  G_OBJECT_CLASS (lba_clock_parent_class)->dispose (gobject);
}

static void
_datetime2str (const GValue * src_value, GValue * dest_value) {
  g_value_take_string (dest_value, g_date_time_format_iso8601 ((GDateTime *)
                                                               g_value_get_boxed
                                                               (src_value)));
}

static void
lba_clock_class_init (LbaClockClass * klass) {
  GObjectClass *gobj_class = G_OBJECT_CLASS (klass);

  gobj_class->dispose = lba_clock_dispose;
  gobj_class->set_property = lba_clock_set_property;
  gobj_class->get_property = lba_clock_get_property;

  g_object_class_install_property
      (gobj_class,
       PROP_TICK_INTERVAL,
       g_param_spec_uint ("tick-interval-ms",
                          "TickIntervalMS",
                          "Tick interval in milliseconds (0 = disabled)", 0,
                          G_MAXUINT, 0,
                          G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE |
                          G_PARAM_CONSTRUCT));

  /* TODO: return boxed GDateTime and register a convertion func to string */
  g_object_class_install_property (gobj_class, PROP_CURRENT_TIME,
                                   g_param_spec_boxed ("current-time",
                                                       "CurrentTime",
                                                       "Current time",
                                                       G_TYPE_DATE_TIME,
                                                       G_PARAM_STATIC_STRINGS |
                                                       G_PARAM_READABLE));

  static gboolean convertions;

  if (convertions == FALSE) {
    convertions = TRUE;
    g_value_register_transform_func (G_TYPE_DATE_TIME, G_TYPE_STRING, _datetime2str);
  }
}

/* Export plugin */
BOMBOLLA_PLUGIN_SYSTEM_PROVIDE_GTYPE (lba_clock);
