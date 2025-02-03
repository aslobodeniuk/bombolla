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

#include <glib-object.h>

/* Declare this magic symbol explicitly */
GType lba_core_object_get_type (void);

typedef struct {
  GObject *obj;
} Fixture;

/* Magic symbol from examples/vala/liblba-vala-example.la */
GType vala_test_get_type ();

static void
fixture_set_up (Fixture *fixture, gconstpointer user_data) {
  /* Register type manually. */
  vala_test_get_type ();

  fixture->obj = g_object_new (lba_core_object_get_type (), NULL, NULL);
}

static void
fixture_tear_down (Fixture *fixture, gconstpointer user_data) {
  g_clear_object (&fixture->obj);
}

static void
on (Fixture *fixture, gconstpointer user_data) {
  const gchar commands[] = {
    /* *INDENT-OFF* */
    "(create ValaTest t1)"
    "(create ValaTest t2)"
    "(set t1.str-rw 0.1234)"
    "(bind t1.str-rw t2.float-rw)"
    "(on t1 notify::str-rw '(dump ValaTest)' )"
    "(set t2.float-rw 0.3456)"
    "(destroy t1)"
    "(destroy t2)"
    /* *INDENT-ON* */
  };

  g_signal_emit_by_name (fixture->obj, "execute", commands);
}

static void
bindings (Fixture *fixture, gconstpointer user_data) {
  const gchar commands[] = {
    /* *INDENT-OFF* */
    "(create ValaTest t1)"
    "(create ValaTest t2)"
    "(bind t1.str-rw t2.str-rw)"
    "(bind t1.str-rw t2.str-w)"
    "(bind t2.str-rw t1.str-r)"
    "(bind t2.str-w t1.str-r)"
    "(set t2.str-rw oneeeeee)"
    "(set t1.str-rw twoooooo)"
    "(set t2.str-rw threeeee)"
    "(bind t1.float-rw t2.str-rw)"
    "(set t2.str-rw 1.234)"
    "(set t1.float-rw 0.999)"
    "(destroy t1)"
    "(destroy t2)"
    /* *INDENT-ON* */
  };

  g_signal_emit_by_name (fixture->obj, "execute", commands);

  /* TODO: now we need a signal in the core to be able to get the objects
   * or their values */
}

static void
async (Fixture *fixture, gconstpointer user_data) {
  const gchar commands[] = {
    /* *INDENT-OFF* */
    "(create ValaTest t1)"
    "(async set t1.str-rw oneeeeee)"
    "(sync)"
    "(destroy t1)"
    "(async create ValaTest t1)"
    "(async set t1.str-rw oneeeeee)"
    "(async destroy t1)"
    /* *INDENT-ON* */
  };

  g_signal_emit_by_name (fixture->obj, "execute", commands);

  /* TODO: now we need a signal in the core to be able to get the objects
   * or their values */
}

static void
plugin_in_vala (Fixture *fixture, gconstpointer user_data) {
  const gchar commands[] = {
    /* *INDENT-OFF* */
    "(create ValaTest t1)"
    "(call t1.say_hello)"
    "(destroy t1)"
    /* *INDENT-ON* */
  };

  g_signal_emit_by_name (fixture->obj, "execute", commands);
}

int
main (int argc, char *argv[]) {
  g_test_init (&argc, &argv, NULL);

  g_test_add ("/core/commands/on", Fixture, "some-user-data",
              fixture_set_up, on, fixture_tear_down);

  g_test_add ("/languages/vala/plugin", Fixture, "some-user-data",
              fixture_set_up, plugin_in_vala, fixture_tear_down);

  g_test_add ("/core/commands/bind", Fixture, "some-user-data",
              fixture_set_up, bindings, fixture_tear_down);

  g_test_add ("/core/commands/async", Fixture, "some-user-data",
              fixture_set_up, async, fixture_tear_down);

  return g_test_run ();
}
