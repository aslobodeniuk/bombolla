/* la Bombolla GObject shell.
 * Copyright (C) 2023 Aleksandr Slobodeniuk
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
#include <gmo/gmo.h>

/* Declare this magic symbols explicitly */
GType lba_core_get_type2 (void);
GType lba_async_string_input_get_type (void);

typedef struct {
  GObject *core;
  GObject *inp;
  gboolean executed;
} Fixture;

static void
fixture_set_up (Fixture * fixture, gconstpointer user_data) {
  GType async_type;

  /* Needed to have GMainContext running */
  fixture->core = g_object_new (lba_core_get_type2 (), NULL);

  /* Doesn't warn on running multiple times */
  async_type = gmo_register_mutant (NULL, G_TYPE_OBJECT,
                                    lba_async_string_input_get_type ());
  fixture->inp = g_object_new (async_type, NULL);
}

static void
fixture_tear_down (Fixture * fixture, gconstpointer user_data) {
  g_clear_object (&fixture->core);
  g_clear_object (&fixture->inp);
}

static void
test_have_string (GObject * obj, gchar * string, Fixture * fixture) {
  g_assert (obj == fixture->inp);
  g_assert_cmpstr (string, ==, "test");
  fixture->executed = TRUE;
}

static void
test_async_stream_input (Fixture * fixture, gconstpointer user_data) {
  /* execute empty string, to start  */
  g_signal_emit_by_name (fixture->core, "execute", "\n");

  fixture->executed = FALSE;
  g_signal_connect (fixture->inp, "have-string", G_CALLBACK (test_have_string),
                    fixture);
  g_signal_emit_by_name (fixture->inp, "input-string", "test");

  g_assert (fixture->executed);
}

int
main (int argc, char *argv[]) {
  g_test_init (&argc, &argv, NULL);

  g_test_add ("/js/test-async-string-input", Fixture, NULL,
              fixture_set_up, test_async_stream_input, fixture_tear_down);
  return g_test_run ();
}
