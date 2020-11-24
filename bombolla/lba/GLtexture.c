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

  lba_GLuint shader_program;
} GLTexture;


typedef struct _GLTextureClass
{
  Basegl2dClass parent;
} GLTextureClass;


static gboolean
gl_texture_compile_shader (BaseOpenGLInterface * i, const gchar * src,
    lba_GLenum shader_type, lba_GLuint * res)
{
  *res = i->glCreateShader (shader_type);

  i->glShaderSource (*res, 1, &src, NULL);
  i->glCompileShader (*res);
  {
    lba_GLint success;
    lba_GLchar log[512];
    i->glGetShaderiv (*res, i->LBA_GL_COMPILE_STATUS, &success);
    if (!success) {
      i->glGetShaderInfoLog (*res, 512, NULL, log);
      LBA_LOG ("Shader compilation failed: %s", log);
      return FALSE;
    }
  }

  return TRUE;
}


static void
gl_texture_build_program (GLTexture * self)
{
  lba_GLuint shader_program;
  lba_GLuint vertex_shader;
  lba_GLuint fragment_shader;

  const char *vertex_shader_source =
      "#version 320 es\n"
      "in vec2 pos;"
      "out vec2 texCoord;"
      "void main() {"
      "texCoord = pos*0.5f + 0.5f;"
      "gl_Position = vec4(pos.x, pos.y, 0.0, 1.0);" "}";

  const char *fragment_shader_source =
      "#version 320 es\n"
      "precision mediump float;"
      "uniform sampler2D srcTex;"
      "in mediump vec2 texCoord;"
      "out mediump vec4 color;"
      "void main() {"
      "float c = texture(srcTex, texCoord).x;"
      "color = vec4(c, 1.0, 1.0, 1.0);" "}";

  Basegl2d *base = (Basegl2d *) self;
  BaseOpenGLInterface *i = base->i;

  if (!gl_texture_compile_shader (i, vertex_shader_source,
          i->LBA_GL_VERTEX_SHADER, &vertex_shader)) {
    LBA_LOG ("FAIL");
    return;
  }

  if (!gl_texture_compile_shader (i, fragment_shader_source,
          i->LBA_GL_FRAGMENT_SHADER, &fragment_shader)) {
    LBA_LOG ("FAIL");           // leaks
    return;
  }

  shader_program = i->glCreateProgram ();

  i->glAttachShader (shader_program, vertex_shader);
  i->glAttachShader (shader_program, fragment_shader);
  i->glLinkProgram (shader_program);

  {
    lba_GLint success;
    lba_GLchar log[512];

    i->glGetProgramiv (shader_program, i->LBA_GL_LINK_STATUS, &success);
    if (!success) {
      i->glGetProgramInfoLog (shader_program, 512, NULL, log);
      LBA_LOG ("Program linkage failed: %s", log);
      return;
    }
  }

  i->glDeleteShader (vertex_shader);
  i->glDeleteShader (fragment_shader);

  self->shader_program = shader_program;

#if 0
  {
    // We create a single float channel 512^2 texture
    lba_GLuint texHandle;
    i->glGenTextures (1, &texHandle);

    i->glActiveTexture (i->GL_TEXTURE0);
    i->glBindTexture (i->GL_TEXTURE_2D, texHandle);
    i->glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D (GL_TEXTURE_2D, 0, GL_R32F, 512, 512, 0, GL_RED, GL_FLOAT,
        NULL);

    // Because we're also using this tex as an image (in order to write to it),
    // we bind it to an image unit as well
    glBindImageTexture (0, texHandle, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_R32F);
    checkErrors ("Gen texture");
    i->glUniform1i (i->glGetUniformLocation (shader_program, "srcTex"), 0);
  }
#endif
  
  {
    lba_GLuint vertArray;
    i->glGenVertexArrays (1, &vertArray);
    i->glBindVertexArray (vertArray);
  }

  {
    float data[] = {
      -1.0f, -1.0f,
      -1.0f, 1.0f,
      1.0f, -1.0f,
      1.0f, 1.0f
    };

    lba_GLint posPtr;
    lba_GLuint posBuf;

    i->glGenBuffers (1, &posBuf);
    i->glBindBuffer (i->LBA_GL_ARRAY_BUFFER, posBuf);
    i->glBufferData (i->LBA_GL_ARRAY_BUFFER, sizeof (float) * 8, data,
        i->LBA_GL_STREAM_DRAW);

    posPtr = i->glGetAttribLocation (shader_program, "pos");
    i->glVertexAttribPointer (posPtr, 2, i->LBA_GL_FLOAT, i->LBA_GL_FALSE, 0,
        0);
    i->glEnableVertexAttribArray (posPtr);
  }
}


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

  i->glGenerateMipmap (i->LBA_GL_TEXTURE_2D);

  i->glBindTexture (i->LBA_GL_TEXTURE_2D, 0);

  self->tex_uploaded = TRUE;
  LBA_LOG ("Texture %d uploaded", self->tex);


  gl_texture_build_program (self);
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
  i->glMatrixMode (i->LBA_GL_MODELVIEW);
  i->glLoadIdentity ();

  i->glUseProgram (self->shader_program);

  i->glDrawArrays (i->LBA_GL_TRIANGLE_STRIP, 0, 4);
  // glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);  
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
