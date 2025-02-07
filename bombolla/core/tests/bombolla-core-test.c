/* la Bombolla GObject shell
 *
 * Copyright (c) 2024, Alexander Slobodeniuk <aleksandr.slobodeniuk@gmx.es>
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

#include <glib-object.h>

/* Declare this magic symbol explicitly */
GType lba_core_object_get_type (void);

typedef struct {
  GObject *obj;
} Fixture;

static void
fixture_set_up (Fixture *fixture, gconstpointer user_data) {
  fixture->obj = g_object_new (lba_core_object_get_type (), NULL);
}

static void
fixture_tear_down (Fixture *fixture, gconstpointer user_data) {
  g_clear_object (&fixture->obj);
}

static void
test_empty_string (Fixture *fixture, gconstpointer user_data) {
  /* execute empty string */
  g_signal_emit_by_name (fixture->obj, "execute", "\n");
}

static void
test_dump (Fixture *fixture, gconstpointer user_data) {
  /* execute empty string */
  g_signal_emit_by_name (fixture->obj, "execute", "(dump LbaCoreObject)");
}

static void
test_singleton (Fixture *fixture, gconstpointer user_data) {
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
  g_test_add ("/core/test-dump", Fixture, NULL,
              fixture_set_up, test_dump, fixture_tear_down);

  return g_test_run ();
}
