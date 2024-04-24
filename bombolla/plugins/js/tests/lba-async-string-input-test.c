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
#include <bmixin/bmixin.h>

/* Declare this magic symbols explicitly */
GType lba_core_object_get_type (void);
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
  fixture->core = g_object_new (lba_core_object_get_type (), NULL);

  /* Doesn't warn on running multiple times */
  async_type = bm_register_mixed_type (NULL, G_TYPE_OBJECT,
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
