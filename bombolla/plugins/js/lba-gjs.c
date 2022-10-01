/* la Bombolla GObject shell.
 * Copyright (C) 2021 Aleksandr Slobodeniuk
 *
 *   This file is part of bombolla.
 *
 *   Bombolla is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   Bombolla is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with bombolla.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "bombolla/lba-plugin-system.h"
#include "bombolla/lba-log.h"
#include <gjs/gjs.h>

typedef struct _LbaGjs {
  GObject parent;

  GjsContext *js_context;
} LbaGjs;

typedef struct _LbaGjsClass {
  GObjectClass parent;
} LbaGjsClass;

static void
lba_gjs_init (LbaGjs * self) {

  self->js_context = GJS_CONTEXT (g_object_new (GJS_TYPE_CONTEXT,
//        "search-path", "",
//        "exec-as-module", TRUE,
                                                NULL));

  if (!self->js_context) {
    g_critical ("Couldn't create context");
    return;
  }
// TODO: scan path for files
  {
    GError *error = NULL;
    gint exit_status;

    // As you can see, my friend, as you can see...
    char filename[] = "examples/js/plugin-in-js.js";

    if (!gjs_context_eval_file (self->js_context, filename, &exit_status, &error)) {
      g_critical ("GJS error [%d], [%s]", exit_status, error->message);
      g_clear_error (&error);
    }
  }
}

static void
lba_gjs_dispose (GObject * gobject) {
  LbaGjs *self = (LbaGjs *) gobject;

  if (self->js_context) {
    g_object_unref (self->js_context);
  }
}

static void
lba_gjs_class_init (LbaGjsClass * klass) {
  GObjectClass *object_class = (GObjectClass *) klass;

  object_class->dispose = lba_gjs_dispose;
}

G_DEFINE_TYPE (LbaGjs, lba_gjs, G_TYPE_OBJECT)
/* Export plugin */
    BOMBOLLA_PLUGIN_SYSTEM_PROVIDE_GTYPE (lba_gjs);
