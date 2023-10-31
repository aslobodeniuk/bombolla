/* la Bombolla GObject shell.
 * Copyright (C) 2023 Alexander Slobodeniuk
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

#include <gmo/gmo.h>
#include "bombolla/lba-plugin-system.h"
#include "bombolla/lba-log.h"
#include "bombolla/base/i3d.h"
#include "../base/icogl.h"

typedef struct _LbaCoglCube {
  CoglPrimitive *prim;
  CoglIndices *indices;
  CoglTexture *texture;

} LbaCoglCube;

typedef struct _LbaCoglCubeClass {
  int dummy;
} LbaCoglCubeClass;

GType lba_cogl_get_type (void);
GType lba_mutogene_3d_get_type (void);

static void
  lba_cogl_cube_icogl_init (LbaICogl * iface);

/* *INDENT-OFF* */ 
GMO_DEFINE_MUTOGENE (lba_cogl_cube, LbaCoglCube,
    GMO_IMPLEMENTS_IFACE (lba, cogl_cube, icogl),
    GMO_ADD_DEP (lba_cogl),
    GMO_ADD_DEP (lba_mutogene_3d));
/* *INDENT-ON* */ 

static void
lba_cogl_cube_paint (GObject * obj, CoglFramebuffer * fb, CoglPipeline * pipeline) {
  int framebuffer_width;
  int framebuffer_height;
  float rotation = 75.0;
  LbaCoglCube *self = gmo_get_LbaCoglCube (obj);

  framebuffer_width = cogl_framebuffer_get_width (fb);
  framebuffer_height = cogl_framebuffer_get_height (fb);

  cogl_framebuffer_push_matrix (fb);

  cogl_framebuffer_translate (fb, framebuffer_width / 2, framebuffer_height / 2, 0);

  cogl_framebuffer_scale (fb, 35, 35, 35);

  /* Rotate the cube separately around each axis.
   *
   * Note: Cogl matrix manipulation follows the same rules as for
   * OpenGL. We use column-major matrices and - if you consider the
   * transformations happening to the model - then they are combined
   * in reverse order which is why the rotation is done last, since
   * we want it to be a rotation around the origin, before it is
   * scaled and translated.
   */
  cogl_framebuffer_rotate (fb, rotation, 0, 0, 1);
  cogl_framebuffer_rotate (fb, rotation, 0, 1, 0);
  cogl_framebuffer_rotate (fb, rotation, 1, 0, 0);

  cogl_primitive_draw (self->prim, fb, pipeline);
  cogl_framebuffer_pop_matrix (fb);
}

static void
lba_cogl_cube_reopen (GObject * base, CoglFramebuffer * fb,
                      CoglPipeline * pipeline, CoglContext * ctx) {
  LbaCoglCube *self = gmo_get_LbaCoglCube (base);
  LbaI3D *iface3d = G_TYPE_INSTANCE_GET_INTERFACE (base,
                                                   LBA_I3D,
                                                   LbaI3D);
  double x,
    y,
    z;

  iface3d->xyz (base, &x, &y, &z);
  LBA_LOG ("reopen (%f, %f, %f)", x, y, z);

  /* A cube modelled using 4 vertices for each face.
   *
   * We use an index buffer when drawing the cube later so the GPU will
   * actually read each face as 2 separate triangles.
   */
  CoglVertexP3T2 vertices[] = {
    /* Front face */
    { /* pos = */ (x - 1.0f), (y - 1.0f), (z + 1.0f), /* tex coords = */ 0.0f,
     1.0f },
    { /* pos = */ (x + 1.0f), (y - 1.0f), (z + 1.0f), /* tex coords = */ 1.0f,
     1.0f },
    { /* pos = */ (x + 1.0f), (y + 1.0f), (z + 1.0f), /* tex coords = */ 1.0f,
     0.0f },
    { /* pos = */ (x - 1.0f), (y + 1.0f), (z + 1.0f), /* tex coords = */ 0.0f,
     0.0f },

    /* Back face */
    { /* pos = */ (x - 1.0f), (y - 1.0f), (z - 1.0f), /* tex coords = */ 1.0f,
     0.0f },
    { /* pos = */ (x - 1.0f), (y + 1.0f), (z - 1.0f), /* tex coords = */ 1.0f,
     1.0f },
    { /* pos = */ (x + 1.0f), (y + 1.0f), (z - 1.0f), /* tex coords = */ 0.0f,
     1.0f },
    { /* pos = */ (x + 1.0f), (y - 1.0f), (z - 1.0f), /* tex coords = */ 0.0f,
     0.0f },

    /* Top face */
    { /* pos = */ (x - 1.0f), (y + 1.0f), (z - 1.0f), /* tex coords = */ 0.0f,
     1.0f },
    { /* pos = */ (x - 1.0f), (y + 1.0f), (z + 1.0f), /* tex coords = */ 0.0f,
     0.0f },
    { /* pos = */ (x + 1.0f), (y + 1.0f), (z + 1.0f), /* tex coords = */ 1.0f,
     0.0f },
    { /* pos = */ (x + 1.0f), (y + 1.0f), (z - 1.0f), /* tex coords = */ 1.0f,
     1.0f },

    /* Bottom face */
    { /* pos = */ (x - 1.0f), (y - 1.0f), (z - 1.0f), /* tex coords = */ 1.0f,
     1.0f },
    { /* pos = */ (x + 1.0f), (y - 1.0f), (z - 1.0f), /* tex coords = */ 0.0f,
     1.0f },
    { /* pos = */ (x + 1.0f), (y - 1.0f), (z + 1.0f), /* tex coords = */ 0.0f,
     0.0f },
    { /* pos = */ (x - 1.0f), (y - 1.0f), (z + 1.0f), /* tex coords = */ 1.0f,
     0.0f },

    /* Right face */
    { /* pos = */ (x + 1.0f), (y - 1.0f), (z - 1.0f), /* tex coords = */ 1.0f,
     0.0f },
    { /* pos = */ (x + 1.0f), (y + 1.0f), (z - 1.0f), /* tex coords = */ 1.0f,
     1.0f },
    { /* pos = */ (x + 1.0f), (y + 1.0f), (z + 1.0f), /* tex coords = */ 0.0f,
     1.0f },
    { /* pos = */ (x + 1.0f), (y - 1.0f), (z + 1.0f), /* tex coords = */ 0.0f,
     0.0f },

    /* Left face */
    { /* pos = */ (x - 1.0f), (y - 1.0f), (z - 1.0f), /* tex coords = */ 0.0f,
     0.0f },
    { /* pos = */ (x - 1.0f), (y - 1.0f), (z + 1.0f), /* tex coords = */ 1.0f,
     0.0f },
    { /* pos = */ (x - 1.0f), (y + 1.0f), (z + 1.0f), /* tex coords = */ 1.0f,
     1.0f },
    { /* pos = */ (x - 1.0f), (y + 1.0f), (z - 1.0f), /* tex coords = */ 0.0f, 1.0f }
  };

  /* rectangle indices allow the GPU to interpret a list of quads (the
   * faces of our cube) as a list of triangles.
   *
   * Since this is a very common thing to do
   * cogl_get_rectangle_indices() is a convenience function for
   * accessing internal index buffers that can be shared.
   */
  self->indices = cogl_get_rectangle_indices (ctx, 6 /* n_rectangles */ );
  self->prim = cogl_primitive_new_p3t2 (ctx, COGL_VERTICES_MODE_TRIANGLES,
                                        G_N_ELEMENTS (vertices), vertices);

  /* Each face will have 6 indices so we have 6 * 6 indices in total... */
  cogl_primitive_set_indices (self->prim, self->indices, 6 * 6);
}

static void
lba_cogl_cube_init (GObject * object, gpointer mutogene) {
}

static void
lba_cogl_cube_class_init (GObjectClass * object_class, gpointer gmo_class) {
}

static void
lba_cogl_cube_icogl_init (LbaICogl * iface) {
  iface->paint = lba_cogl_cube_paint;
  iface->reopen = lba_cogl_cube_reopen;
}

BOMBOLLA_PLUGIN_SYSTEM_PROVIDE_GTYPE (lba_cogl_cube);
