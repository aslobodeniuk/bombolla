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

#include <bmixin/bmixin.h>

typedef struct {
  GObject *obj;
} Fixture;

static void
fixture_set_up (Fixture * fixture, gconstpointer user_data) {
  GType testwolf;
  GType wolf_get_type (void);

  g_message ("registering testwolf");
  testwolf = bm_register_mixed_type ("TestWolf", G_TYPE_OBJECT, wolf_get_type (), NULL);

  fixture->obj = g_object_new (testwolf, NULL);
}

static void
fixture_tear_down (Fixture * fixture, gconstpointer user_data) {
  g_clear_object (&fixture->obj);
}

static void
test_wolf_howl (Fixture * fixture, gconstpointer user_data) {
  g_signal_emit_by_name (fixture->obj, "howl");
}

static void
test_domestic_override (Fixture * fixture, gconstpointer user_data) {
  gboolean domestic;

  g_object_get (fixture->obj, "is-domestic", &domestic, NULL);

  g_assert_false (domestic);

  {
    GType wolf2;
    GObject *obj;
    GType wolf2_get_type (void);

    g_message ("registering Wolf2");
    wolf2 = bm_register_mixed_type ("TestWolf2", G_TYPE_OBJECT, wolf2_get_type (), NULL);

    obj = g_object_new (wolf2, NULL);

    g_object_get (obj, "is-domestic", &domestic, NULL);

    g_assert_true (domestic);
    g_object_unref (obj);
  }
}

int
main (int argc, char *argv[]) {
  g_test_init (&argc, &argv, NULL);

  g_test_add ("/bmixin/test-wolf-howl", Fixture, NULL,
              fixture_set_up, test_wolf_howl, fixture_tear_down);

  g_test_add ("/bmixin/test-domestic-override", Fixture, NULL,
              fixture_set_up, test_domestic_override, fixture_tear_down);
  return g_test_run ();
}
