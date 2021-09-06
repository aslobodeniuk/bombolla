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
GType lba_core_get_type (void);

typedef struct {
  GObject *obj;
} Fixture;

/* Magic symbol from examples/vala/liblba-vala-example.la */
GType vala_test_get_type ();

static void
fixture_set_up (Fixture * fixture, gconstpointer user_data) {
  /* Register type manually. */
  vala_test_get_type ();

  fixture->obj = g_object_new (lba_core_get_type (), NULL);
}

static void
fixture_tear_down (Fixture * fixture, gconstpointer user_data) {
  g_clear_object (&fixture->obj);
}

static void
plugin_in_vala (Fixture * fixture, gconstpointer user_data) {
  g_signal_emit_by_name (fixture->obj, "execute",
                         "create ValaTest t\n" "call t.say_hello");
}

int
main (int argc, char *argv[]) {
  g_test_init (&argc, &argv, NULL);

  g_test_add ("/languages/vala/plugin", Fixture, "some-user-data",
              fixture_set_up, plugin_in_vala, fixture_tear_down);

  return g_test_run ();
}
