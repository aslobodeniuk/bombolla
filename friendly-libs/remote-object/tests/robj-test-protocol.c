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
  RObjMap recv_map;
  RObjMap send_map;
} Fixture;

static void
fixture_set_up (Fixture * fixture, gconstpointer user_data) {
  robj_protocol_init ();

  robj_map_init (&fixture->recv_map);
  fixture->ohash = robj_map_add_object (&fixture->recv_map, "foo");

  robj_map_init (&fixture->send_map);
  robj_map_add_object (&fixture->send_map, "foo");
}

static void
fixture_tear_down (Fixture * fixture, gconstpointer user_data) {
  robj_map_clear (&fixture->recv_map);
  robj_map_clear (&fixture->send_map);
}

static GBytes *
send (GValue * val, Fixture * fixture) {
  GBytes *msg;
  RObjPN *pn_send;
  RObjPN *pn_recv;

  pn_send = robj_map_new_pn (&fixture->send_map, fixture->ohash, "some-name", val);
  g_assert_nonnull (pn_send);
  pn_recv = robj_map_new_pn (&fixture->recv_map, fixture->ohash, "some-name", val);
  g_assert_nonnull (pn_recv);

  msg = robj_protocol_pn_to_message (pn_send);
  g_assert_nonnull (msg);
  g_value_unset (val);

  return msg;
}

static RObjPN *
recv (GBytes * msg, Fixture * fixture) {
  RObjPN *pn;

  pn = robj_protocol_message_to_pn (&fixture->send_map, msg);
  g_bytes_unref (msg);

  return pn;
}

static void
test_int_pn (Fixture * fixture, gconstpointer user_data) {
  GValue pval = G_VALUE_INIT;
  gint recvi;
  gint i = g_random_int ();
  RObjPN *rpn;

  g_value_init (&pval, G_TYPE_INT);
  g_value_set_int (&pval, i);

  rpn = recv (send (&pval, fixture), fixture);
  recvi = g_value_get_int (&rpn->pval);
  g_assert_cmpint (i, ==, recvi);
}

static void
test_uint_pn (Fixture * fixture, gconstpointer user_data) {
  GValue pval = G_VALUE_INIT;
  guint recvi;
  guint u = g_random_int ();
  RObjPN *rpn;

  g_value_init (&pval, G_TYPE_UINT);
  g_value_set_uint (&pval, u);

  rpn = recv (send (&pval, fixture), fixture);
  recvi = g_value_get_uint (&rpn->pval);
  g_assert_cmpint (u, ==, recvi);
}

static void
test_uint64_pn (Fixture * fixture, gconstpointer user_data) {
  GValue pval = G_VALUE_INIT;
  guint64 recvi;
  guint64 u64 = (((guint64) g_random_int ()) << 32) | (guint64) g_random_int ();
  RObjPN *rpn;

  g_value_init (&pval, G_TYPE_UINT64);
  g_value_set_uint64 (&pval, u64);

  rpn = recv (send (&pval, fixture), fixture);
  recvi = g_value_get_uint64 (&rpn->pval);
  g_assert (u64 == recvi);
}

static void
test_string_pn (Fixture * fixture, gconstpointer user_data) {
  GValue pval = G_VALUE_INIT;
  const gchar *recv_str;
  const gchar *str = (const gchar *)user_data;
  RObjPN *rpn;

  g_value_init (&pval, G_TYPE_STRING);
  g_value_set_string (&pval, str);

  rpn = recv (send (&pval, fixture), fixture);

  recv_str = g_value_get_string (&rpn->pval);
  g_assert_cmpstr (str, ==, recv_str);
}

int
main (int argc, char *argv[]) {
  g_test_init (&argc, &argv, NULL);

  g_test_add ("/robj/test-string-pn", Fixture, "xxxxxxxxxxxxxxxxxxxxxxxx"
              "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"
              "xxxxxxxxxxxx", fixture_set_up, test_string_pn, fixture_tear_down);

  g_test_add ("/robj/test-string-pn2", Fixture, "",
              fixture_set_up, test_string_pn, fixture_tear_down);

  g_test_add ("/robj/test-string-pn3", Fixture, NULL,
              fixture_set_up, test_string_pn, fixture_tear_down);

  g_test_add ("/robj/test-int-pn", Fixture, NULL,
              fixture_set_up, test_int_pn, fixture_tear_down);

  g_test_add ("/robj/test-uint-pn", Fixture, NULL,
              fixture_set_up, test_uint_pn, fixture_tear_down);

  g_test_add ("/robj/test-uint64-pn", Fixture, NULL,
              fixture_set_up, test_uint64_pn, fixture_tear_down);
  return g_test_run ();
}
