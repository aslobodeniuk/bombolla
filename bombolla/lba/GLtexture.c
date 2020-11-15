/* la Bombolla GObject shell.
 * Copyright (C) 2020 Aleksandr Slobodeniuk
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

#include "bombolla/base/lba-basegl2d.h"
#include "bombolla/lba-plugin-system.h"
#include "bombolla/lba-log.h"

typedef struct _GLTexture
{
  Basegl2d parent;

  lba_GLuint tex;
  gboolean tex_uploaded;
} GLTexture;


typedef struct _GLTextureClass
{
  Basegl2dClass parent;
} GLTextureClass;


static void
gl_texture_upload (GLTexture * self)
{
  Basegl2d *base = (Basegl2d *) self;
  BaseOpenGLInterface *i = base->i;
  gint j = 0;

  if (!i) {
    LBA_LOG ("Too early, set interface first");
    return;
  }
  //create test checker image
  unsigned char texDat[64][4] = { 0 };
  /* Alpha */
  for (int i = 0; i < 64; i++) {
    texDat[i][3] = 255;
  }
  /* Red */
  for (int i = 0; i < 64; i++) {
    j += 3;
    texDat[i][0] = j;
  }

  /* leak here, if called for the second time */
  i->glGenTextures (1, &self->tex);

  i->glBindTexture (i->LBA_GL_TEXTURE_2D, self->tex);

  i->glTexParameteri (i->LBA_GL_TEXTURE_2D, i->LBA_GL_TEXTURE_MIN_FILTER,
      i->LBA_GL_NEAREST);
  i->glTexParameteri (i->LBA_GL_TEXTURE_2D, i->LBA_GL_TEXTURE_MAG_FILTER,
      i->LBA_GL_NEAREST);

  i->glTexImage2D (i->LBA_GL_TEXTURE_2D, 0, i->LBA_GL_RGBA, 8, 8, 0,
      i->LBA_GL_RGBA, i->LBA_GL_UNSIGNED_BYTE, texDat);

  i->glBindTexture (i->LBA_GL_TEXTURE_2D, 0);

  self->tex_uploaded = TRUE;
  LBA_LOG ("Texture %d uploaded", self->tex);

  /* FIXME */
  //match projection to window resolution (could be in reshape callback)
  i->glMatrixMode (i->LBA_GL_PROJECTION);
  i->glOrtho (0, 800, 0, 600, -1, 1);
  i->glMatrixMode (i->LBA_GL_MODELVIEW);
}


static void
gl_texture_draw (Basegl2d * base, BaseOpenGLInterface * i)
{
  GLTexture *self = (GLTexture *) base;
  Base2d *s2d = (Base2d *) base;
  guint x, y, width, height;

  if (!self->tex_uploaded) {
    gl_texture_upload (self);
  }

  x = s2d->x;
  y = s2d->y;
  width = s2d->width;
  height = s2d->height;

  LBA_LOG ("draw (%d, %d) : (%d, %d)", x, y, width, height);

  /* Reset transformations */
  i->glMatrixMode(i->LBA_GL_MODELVIEW);
  i->glLoadIdentity();
  //match projection to window resolution (could be in reshape callback)
  i->glMatrixMode (i->LBA_GL_PROJECTION);
  i->glOrtho (0, 800, 0, 600, -1, 1);   // needs scene size
  i->glMatrixMode (i->LBA_GL_MODELVIEW);

  //clear and draw quad with texture (could be in display callback)
  i->glBindTexture (i->LBA_GL_TEXTURE_2D, self->tex);
  i->glEnable (i->LBA_GL_TEXTURE_2D);
  i->glBegin (i->LBA_GL_QUADS);

  i->glTexCoord2i (0, 0);
  i->glVertex2i (x, y);

  i->glTexCoord2i (0, 1);
  i->glVertex2i (x, y + height);

  i->glTexCoord2i (1, 1);
  i->glVertex2i (x + width, y + height);

  i->glTexCoord2i (1, 0);
  i->glVertex2i (x + width, y);

  i->glEnd ();
  i->glDisable (i->LBA_GL_TEXTURE_2D);
  i->glBindTexture (i->LBA_GL_TEXTURE_2D, 0);
}


static void
gl_texture_init (GLTexture * self)
{

}


static void
gl_texture_class_init (GLTextureClass * klass)
{
  Basegl2dClass *base_gl2d_class = (Basegl2dClass *) klass;

  base_gl2d_class->draw = gl_texture_draw;
}


G_DEFINE_TYPE (GLTexture, gl_texture, G_TYPE_BASEGL2D)

/* Export plugin */
    BOMBOLLA_PLUGIN_SYSTEM_PROVIDE_GTYPE (gl_texture);
