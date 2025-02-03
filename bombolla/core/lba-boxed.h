/* la Bombolla GObject shell
 *
 * Copyright (c) 2025, Alexander Slobodeniuk <aleksandr.slobodeniuk@gmx.es>
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

#ifndef _LBA_BOXED
#  define _LBA_BOXED
#  include <glib-object.h>

typedef struct {
  GType self_type;

  gint refcount;
  GDestroyNotify free;
} LbaBoxed;

void lba_boxed_init (LbaBoxed * bxd, GType self_type, GDestroyNotify free);

void lba_boxed_unref (gpointer b);

gpointer lba_boxed_ref (gpointer b);

#  define LBA_DEFINE_BOXED(Type, type) G_DEFINE_BOXED_TYPE (Type, type, lba_boxed_ref, lba_boxed_unref)

typedef enum {
  LBA_EXPR_NODE_IS_EXPR,
  LBA_EXPR_NODE_IS_STONE,
  LBA_EXPR_NODE_IS_LIST,
  LBA_EXPR_NODE_IS_STRING,
} LbaExprNodeType;

typedef struct {
  LbaBoxed bxd;

  GNode *node;
  LbaExprNodeType type;
  GValue value;
  gchar *str;
  gint actionrefs;
} LbaExprNode;

GNode *lba_expr_node_new (LbaExprNodeType type, const gchar * expr, guint len);

void lba_expr_node_destroy (GNode * tree);
#endif
