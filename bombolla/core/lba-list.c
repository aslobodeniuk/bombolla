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

#include "lba-list.h"

struct _LbaList {
  LbaBoxed bxd;

  GArray *values;
};

LBA_DEFINE_BOXED (LbaList, lba_list);

static void
lba_list_free (gpointer p) {
  LbaList *l = (LbaList *) p;

  g_array_unref (l->values);
  g_free (l);
}

gint
lba_list_length (const LbaList *l) {
  return l->values->len;
}

const GValue *
lba_list_index (const LbaList *l, gint i) {
  return &g_array_index (l->values, GValue, i);
}

gchar *
lba_list_to_string (const LbaList *l) {
  gint i;
  GString *str = g_string_new ("{");

  for (i = 0; i < lba_list_length (l); i++) {
    GValue out = G_VALUE_INIT;
    const GValue *in = lba_list_index (l, i);

    if (i != 0)
      str = g_string_append (str, ", ");

    g_value_init (&out, G_TYPE_STRING);
    str = g_string_append (str,
                           g_value_transform (in, &out) ?
                           g_value_get_string (&out) : "(ERROR!!!)");

    g_value_unset (&out);
  }

  str = g_string_append (str, "}");

  return g_string_free_and_steal (str);
}

LbaList *
lba_list_new () {
  LbaList *ret = g_new0 (LbaList, 1);

  lba_boxed_init (&ret->bxd, lba_list_get_type (), lba_list_free);

  ret->values = g_array_sized_new (FALSE, TRUE, sizeof (GValue), 4);
  g_array_set_clear_func (ret->values, (GDestroyNotify) g_value_unset);
  return ret;
}

void
lba_list_add (LbaList *l, GValue *v) {
  GValue cp = G_VALUE_INIT;

  /* We should make this func take ownership?? */
  g_value_copy (v, &cp);
  g_array_append_val (l->values, cp);
}
