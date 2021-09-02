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

static void
fixture_set_up (Fixture * fixture, gconstpointer user_data) {
  fixture->obj = g_object_new (lba_core_get_type (), NULL);
}

static void
fixture_tear_down (Fixture * fixture, gconstpointer user_data) {
  g_clear_object (&fixture->obj);
}

static void
test1 (Fixture * fixture, gconstpointer user_data) {
}

static void
test2 (Fixture * fixture, gconstpointer user_data) {
}

int
main (int argc, char *argv[]) {
  g_test_init (&argc, &argv, NULL);

  // Define the tests.
  g_test_add ("/core/test1", Fixture, "some-user-data",
              fixture_set_up, test1, fixture_tear_down);
  g_test_add ("/core/test2", Fixture, "some-user-data",
              fixture_set_up, test2, fixture_tear_down);

  return g_test_run ();
}
