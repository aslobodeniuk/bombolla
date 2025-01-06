/* Remote GObject
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

#include "../robj-protocol.h"

typedef struct {
  guint32 ohash;

} Fixture;

static void
fixture_set_up (Fixture * fixture, gconstpointer user_data) {
  robj_protocol_init ();
  fixture->ohash = robj_brain_add_object ("foo");
}

static void
fixture_tear_down (Fixture * fixture, gconstpointer user_data) {
  robj_brain_forget_object (fixture->ohash);
}

static void
test_int_pn (Fixture * fixture, gconstpointer user_data) {

  GValue pval = G_VALUE_INIT;
  RObjPN *pn;
  GBytes *msg;
  gint recv_val;

  g_value_init (&pval, G_TYPE_INT);
  g_value_set_int (&pval, 123);
  pn = robj_brain_learn_pn (fixture->ohash, "some-name", &pval);
  g_assert_nonnull (pn);

  msg = robj_protocol_pn_to_message (pn);

  g_assert_nonnull (msg);

  /* FIXME: this overwrites the PN, need 2 different brains
   * to handle that properly */
  pn = robj_protocol_message_to_pn (msg);

  recv_val = g_value_get_int (&pn->pval);
  g_assert_cmpint (123, ==, recv_val);

  g_value_unset (&pval);
  g_bytes_unref (msg);
}

int
main (int argc, char *argv[]) {
  g_test_init (&argc, &argv, NULL);

  g_test_add ("/robj/test-int-pn", Fixture, NULL,
              fixture_set_up, test_int_pn, fixture_tear_down);

  return g_test_run ();
}
