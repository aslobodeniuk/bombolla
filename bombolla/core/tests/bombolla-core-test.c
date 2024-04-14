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

static void
fixture_set_up (Fixture * fixture, gconstpointer user_data) {
  fixture->obj = g_object_new (lba_core_object_get_type (), NULL);
}

static void
fixture_tear_down (Fixture * fixture, gconstpointer user_data) {
  g_clear_object (&fixture->obj);
}

static void
test_empty_string (Fixture * fixture, gconstpointer user_data) {
  /* execute empty string */
  g_signal_emit_by_name (fixture->obj, "execute", "\n");
}

static void
test_singleton (Fixture * fixture, gconstpointer user_data) {
  GObject *more_lba_cores[2];

  more_lba_cores[0] = g_object_new (lba_core_object_get_type (), NULL);
  more_lba_cores[1] = g_object_new (lba_core_object_get_type (), NULL);

  g_assert (more_lba_cores[0] == more_lba_cores[1]);
  g_object_unref (more_lba_cores[0]);
  g_object_unref (more_lba_cores[1]);

  fixture->obj = g_object_new (lba_core_object_get_type (), NULL);
}

int
main (int argc, char *argv[]) {
  g_test_init (&argc, &argv, NULL);

  g_test_add ("/core/test-empty-string", Fixture, NULL,
              fixture_set_up, test_empty_string, fixture_tear_down);
  g_test_add ("/core/singleton", Fixture, "some-user-data",
              fixture_set_up, test_singleton, fixture_tear_down);

  return g_test_run ();
}
