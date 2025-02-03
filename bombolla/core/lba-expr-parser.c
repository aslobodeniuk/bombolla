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

#include "lba-expr-parser.h"

static gboolean
lba_expr_parser_space (char c) {
  return c == ' ' || c == '\t' || c == '\n';
}

static inline gint
lba_expr_parser_skip_emptiness (gint i, const gchar *expr, guint total) {
  while (lba_expr_parser_space (expr[i]) && i < total)
    i++;

  /* This position is not space anymore */
  return i;
}

static inline gint
lba_expr_parser_end_stone (gint i, const gchar *expr, guint total) {
  while (!lba_expr_parser_space (expr[i])
         && G_LIKELY (i < total)
         && G_LIKELY (expr[i] != 0)
         && G_LIKELY (expr[i] != ')')
         && G_LIKELY (expr[i] != '(')) {
    i++;
  }

  /* Here we know the next char is not a stone */
  return i;
}

static inline gint
lba_expr_parser_end_string (gint i, const gchar *expr, guint total) {
  gboolean escape = FALSE;
  char quote = expr[i];

  while (++i < total) {
    if (G_UNLIKELY (escape)) {
      escape = FALSE;
      continue;
    }

    if (G_UNLIKELY (expr[i] == '\\')) {
      escape = TRUE;
      continue;
    }

    /* End of story */
    if (G_UNLIKELY (expr[i] == quote))
      break;
  }

  return i;
}

static inline gint
lba_expr_parser_skip_comments (gint i, const gchar *expr, guint total) {
  if (G_LIKELY (expr[i] != '#'))
    /* Not a comment */
    return i;

  i++;
  /* Comments: skip until next line */
  while (expr[i] != '\n' && i < total && expr[i] != 0)
    i++;

  return i;
}

/* OOOK, so the return value carries a GNode tree with all the ((expressions)),
 * and stones, but without spaces or commens.
 * TODO: "strings", 'strings', 123ints, 1.23floats
 * */
GNode *
lba_expr_parser_sniff (LbaExprNodeType type, const gchar *expr, guint total) {
  GNode *ret;
  gint i;
  gint depth = 0;
  gboolean capturing_list = FALSE;
  const gchar *expr_start,
   *expr_end;

  g_return_val_if_fail (expr != NULL, NULL);
  g_return_val_if_fail (total != 0, NULL);

  ret = lba_expr_node_new (type, expr, total);

  for (i = 0; i < total && expr[i] != 0; i++) {
    i = lba_expr_parser_skip_emptiness (i, expr, total);
    if (G_UNLIKELY (i == total))
      break;

    i = lba_expr_parser_skip_comments (i, expr, total);
    if (G_UNLIKELY (i == total))
      break;

    if (expr[i] == '(') {
      /* FIXME: lba_expr_parser_end_expression() ?? */
      if (0 == depth++)
        /* start from the inside of an expr */
        expr_start = &expr[i + 1];
      continue;
    }

    if (expr[i] == ')') {
      if (G_UNLIKELY (depth == 0)) {
        /* Abort here, since we haven't designed error handling.
         * Currently we are focused on complex design challenges. */
        g_critical ("Bad parentesis [%.*s]", total, expr);
        return NULL;
      }

      if (--depth == 0) {
        expr_end = &expr[i];

        if (capturing_list) {
          type = LBA_EXPR_NODE_IS_LIST;
          capturing_list = FALSE;
        } else {
          type = LBA_EXPR_NODE_IS_EXPR;
        }

        /* Parse expression and add as new node */
        g_node_append (ret, lba_expr_parser_sniff (type,
                                                   expr_start,
                                                   expr_end - expr_start));

        capturing_list = FALSE;
      }
      continue;
    }

    if (depth == 0 && (g_ascii_isalpha (expr[i]) || expr[i] == '_')) {
      expr_start = &expr[i];
      /* Jump until the next space */
      i = lba_expr_parser_end_stone (i++, expr, total);
      expr_end = &expr[i];
      g_node_append (ret, lba_expr_node_new (LBA_EXPR_NODE_IS_STONE,
                                             expr_start, expr_end - expr_start));
      continue;
    }

    if (depth == 0 && (expr[i] == '"' || expr[i] == '\'')) {
      /* Capture the string */
      expr_start = &expr[i + 1];
      i = lba_expr_parser_end_string (i, expr, total);
      expr_end = &expr[i];
      g_node_append (ret, lba_expr_node_new (LBA_EXPR_NODE_IS_STONE,
                                             expr_start, expr_end - expr_start));
      continue;
    }

    if (depth == 0 && expr[i] == '@') {
      capturing_list = TRUE;
      /* FIXME: change to [foo bar (baz)] lists, and implement
       * lba_expr_parser_end_list ()*/
    }
  }

  if (G_UNLIKELY (depth > 0)) {
    g_critical ("Bad parentesis [%.*s]", total, expr);
    return NULL;
  }
  return ret;
}

const gchar *
DEPRECATED_lba_expr_parser_find_next (const gchar *expr, guint total, guint *len) {
  gint i;
  guint depth = 0;
  const gchar *expr_start = expr;
  const gchar *expr_end = NULL;

  g_return_val_if_fail (expr != NULL, NULL);
  g_return_val_if_fail (len != NULL, NULL);
  g_return_val_if_fail (g_str_is_ascii (expr), NULL);
  for (i = 0; i < total && expr[i] != 0; i++) {
    char c = expr[i];

    if (lba_expr_parser_space (c))
      continue;
    if (c == '(') {
      if (G_UNLIKELY (0 == depth++))
        /* start from the inside of an expr */
        expr_start = &expr[i + 1];
      continue;
    }

    if (c == '#') {
      /* skip until next line */
      while (expr[++i] != '\n' && i < total && expr[i] != 0);
    }

    if (c == ')') {
      if (G_UNLIKELY (depth == 0)) {
        /* Abort here, since we haven't designed error handling.
         * Currently we are focused on complex design challenges. */
        g_error ("Bad parentesis [%.*s]", total, expr);
      }

      if (G_UNLIKELY (--depth == 0)) {
        expr_end = &expr[i];
        /* So we are done: our expression have been found */
        *len = expr_end - expr_start;
        return expr_start;
      }
      continue;
    }

    if (depth == 0 && g_ascii_isalpha (c)) {
      /* Jump until the next space */
      while (!lba_expr_parser_space (expr[++i])
             && i < total && expr[i] != 0);
      expr_end = &expr[i];
      *len = expr_end - expr_start;
      return expr_start;
    }

    if (c == '"' || c == '\'') {
      g_error ("String params not supported yet");
    }
  }

  if (G_UNLIKELY (depth > 0))
    g_error ("Bad parentesis [%.*s]", total, expr);
  /* VALID: no more expressions have been found */
  return NULL;
}
