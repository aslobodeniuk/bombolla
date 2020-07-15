#include "bombolla/base/lba-basedrawable.h"
#include "bombolla/lba-plugin-system.h"
#include <glib/gstdio.h>
#include "GL/gl.h"


/* ======================= Instance */
typedef struct _GLCube
{
  BaseDrawable parent;
} GLCube;


/* ======================= Class */
typedef struct _GLCubeClass
{
  BaseDrawableClass parent;
} GLCubeClass;


static void
gl_cube_draw (BaseDrawable * base)
{
  g_print ("draw\n");
}

static void
gl_cube_init (GLCube * self)
{
  g_printf ("init\n");
}


/* =================== CLASS */

static void
gl_cube_class_init (GLCubeClass * klass)
{
  BaseDrawableClass *base_class = BASE_DRAWABLE_GET_CLASS (klass);

  base_class->draw = gl_cube_draw;
}


G_DEFINE_TYPE (GLCube, gl_cube, G_TYPE_BASE_DRAWABLE)
BOMBOLLA_PLUGIN_SYSTEM_PROVIDE_GTYPE (gl_cube);
