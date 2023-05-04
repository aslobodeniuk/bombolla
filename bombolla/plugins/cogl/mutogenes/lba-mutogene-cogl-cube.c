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

static void
  lba_cogl_cube_iface_init (LbaICogl * iface);

GMO_DEFINE_MUTOGENE_WITH_CODE (lba_cogl_cube, LbaCoglCube, {
                               .iface_type = lba_icogl_get_type,.info = {
                               (void (*)(void *, void *))lba_cogl_cube_iface_init
                               }});

static void
lba_cogl_cube_paint (GObject * obj, CoglFramebuffer * fb, CoglPipeline * pipeline) {
  double x,
    y,
    z;
  int framebuffer_width;
  int framebuffer_height;
  float rotation = 75.0;
  LbaCoglCube *self = gmo_get_LbaCoglCube (obj);
  LbaI3D *iface3d = G_TYPE_INSTANCE_GET_INTERFACE (obj,
                                                   LBA_I3D,
                                                   LbaI3D);

  iface3d->xyz (obj, &x, &y, &z);

  LBA_LOG ("draw (%f, %f, %f)", x, y, z);

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

  /* A cube modelled using 4 vertices for each face.
   *
   * We use an index buffer when drawing the cube later so the GPU will
   * actually read each face as 2 separate triangles.
   */
  static CoglVertexP3T2 vertices[] = {
    /* Front face */
    { /* pos = */ -1.0f, -1.0f, 1.0f, /* tex coords = */ 0.0f, 1.0f },
    { /* pos = */ 1.0f, -1.0f, 1.0f, /* tex coords = */ 1.0f, 1.0f },
    { /* pos = */ 1.0f, 1.0f, 1.0f, /* tex coords = */ 1.0f, 0.0f },
    { /* pos = */ -1.0f, 1.0f, 1.0f, /* tex coords = */ 0.0f, 0.0f },

    /* Back face */
    { /* pos = */ -1.0f, -1.0f, -1.0f, /* tex coords = */ 1.0f, 0.0f },
    { /* pos = */ -1.0f, 1.0f, -1.0f, /* tex coords = */ 1.0f, 1.0f },
    { /* pos = */ 1.0f, 1.0f, -1.0f, /* tex coords = */ 0.0f, 1.0f },
    { /* pos = */ 1.0f, -1.0f, -1.0f, /* tex coords = */ 0.0f, 0.0f },

    /* Top face */
    { /* pos = */ -1.0f, 1.0f, -1.0f, /* tex coords = */ 0.0f, 1.0f },
    { /* pos = */ -1.0f, 1.0f, 1.0f, /* tex coords = */ 0.0f, 0.0f },
    { /* pos = */ 1.0f, 1.0f, 1.0f, /* tex coords = */ 1.0f, 0.0f },
    { /* pos = */ 1.0f, 1.0f, -1.0f, /* tex coords = */ 1.0f, 1.0f },

    /* Bottom face */
    { /* pos = */ -1.0f, -1.0f, -1.0f, /* tex coords = */ 1.0f, 1.0f },
    { /* pos = */ 1.0f, -1.0f, -1.0f, /* tex coords = */ 0.0f, 1.0f },
    { /* pos = */ 1.0f, -1.0f, 1.0f, /* tex coords = */ 0.0f, 0.0f },
    { /* pos = */ -1.0f, -1.0f, 1.0f, /* tex coords = */ 1.0f, 0.0f },

    /* Right face */
    { /* pos = */ 1.0f, -1.0f, -1.0f, /* tex coords = */ 1.0f, 0.0f },
    { /* pos = */ 1.0f, 1.0f, -1.0f, /* tex coords = */ 1.0f, 1.0f },
    { /* pos = */ 1.0f, 1.0f, 1.0f, /* tex coords = */ 0.0f, 1.0f },
    { /* pos = */ 1.0f, -1.0f, 1.0f, /* tex coords = */ 0.0f, 0.0f },

    /* Left face */
    { /* pos = */ -1.0f, -1.0f, -1.0f, /* tex coords = */ 0.0f, 0.0f },
    { /* pos = */ -1.0f, -1.0f, 1.0f, /* tex coords = */ 1.0f, 0.0f },
    { /* pos = */ -1.0f, 1.0f, 1.0f, /* tex coords = */ 1.0f, 1.0f },
    { /* pos = */ -1.0f, 1.0f, -1.0f, /* tex coords = */ 0.0f, 1.0f }
  };

  LBA_LOG ("reopening");

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
lba_cogl_cube_iface_init (LbaICogl * iface) {
  iface->paint = lba_cogl_cube_paint;
  iface->reopen = lba_cogl_cube_reopen;
}

BOMBOLLA_PLUGIN_SYSTEM_PROVIDE_GTYPE (lba_cogl_cube);