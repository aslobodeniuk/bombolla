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

#include "bombolla/lba-log.h"

/* HACK: Needed to use LBA_LOG */
static const gchar *global_lba_plugin_name = "LbaCore";

static void
_str2float (const GValue *src_value, GValue *dest_value) {
  gfloat ret = 0;
  const gchar *s = g_value_get_string (src_value);

  if (s) {
    ret = atof (s);
  } else
    g_warning ("couldn't convert string %s to float", s);

  g_value_set_float (dest_value, ret);
}

static void
_str2double (const GValue *src_value, GValue *dest_value) {
  gdouble ret = 0;
  const gchar *s = g_value_get_string (src_value);

  if (s) {
    ret = atof (s);
  } else
    g_warning ("couldn't convert string %s to double", s);

  g_value_set_double (dest_value, ret);
}

static void
_str2bool (const GValue *src_value, GValue *dest_value) {
  guint ret = 0;
  const gchar *s = g_value_get_string (src_value);

  if (s) {
    ret = !g_strcmp0 (s, "true");
  } else
    g_warning ("couldn't convert string %s to uint", s);

  g_value_set_boolean (dest_value, ret);
}

static void
_str2int (const GValue *src_value, GValue *dest_value) {
  guint ret = 0;
  const gchar *s = g_value_get_string (src_value);

  if (s) {
    ret = atoi (s);
  } else
    g_warning ("couldn't convert string %s to uint", s);

  g_value_set_int (dest_value, ret);
}

static void
_str2uint (const GValue *src_value, GValue *dest_value) {
  guint ret = 0;
  const gchar *s = g_value_get_string (src_value);

  if (s) {
    ret = atoi (s);
  } else
    g_warning ("couldn't convert string %s to uint", s);

  g_value_set_uint (dest_value, ret);
}

static void
_str2gtype (const GValue *src_value, GValue *dest_value) {
  GType t = G_TYPE_NONE;
  const gchar *s = g_value_get_string (src_value);

  if (s) {
    t = g_type_from_name (s);
  }

  if (!t || t == G_TYPE_NONE) {
    g_warning ("couldn't convert string %s to GType", s);
  }

  g_value_set_gtype (dest_value, t);
}

void
lba_core_init_convertion_functions (void) {
  static volatile gboolean once;

  if (!once) {
    /* Register basic transform functions for types */
    g_value_register_transform_func (G_TYPE_STRING, G_TYPE_BOOLEAN, _str2bool);
    g_value_register_transform_func (G_TYPE_STRING, G_TYPE_INT, _str2int);
    g_value_register_transform_func (G_TYPE_STRING, G_TYPE_UINT, _str2uint);
    g_value_register_transform_func (G_TYPE_STRING, G_TYPE_DOUBLE, _str2double);
    g_value_register_transform_func (G_TYPE_STRING, G_TYPE_FLOAT, _str2float);
    g_value_register_transform_func (G_TYPE_STRING, G_TYPE_GTYPE, _str2gtype);

    once = 1;
  }
}
