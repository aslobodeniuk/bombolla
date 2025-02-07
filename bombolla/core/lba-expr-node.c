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

#include "lba-boxed.h"

LBA_DEFINE_BOXED (LbaExprNode, lba_expr_node);

static void
lba_expr_node_free (gpointer p) {
  LbaExprNode *en = (LbaExprNode *) p;

  g_value_unset (&en->value);
  g_free (en->str);
  g_free (en);
}

GNode *
lba_expr_node_new (LbaExprNodeType type, const gchar *expr, guint len) {
  LbaExprNode *ret = g_new0 (LbaExprNode, 1);

  lba_boxed_init (&ret->bxd, lba_expr_node_get_type (), lba_expr_node_free);
  ret->str = g_strndup (expr, len);
  ret->type = type;
  ret->node = g_node_new (ret);
  return ret->node;
}

static gboolean
lba_expr_node_destroy_each (GNode *node, gpointer data) {
  g_clear_pointer (&node->data, lba_boxed_unref);
  return FALSE;
}

void
lba_expr_node_destroy (GNode *tree) {
  g_node_traverse (tree,
                   G_LEVEL_ORDER, G_TRAVERSE_ALL, -1, lba_expr_node_destroy_each,
                   NULL);
  g_node_destroy (tree);
}
