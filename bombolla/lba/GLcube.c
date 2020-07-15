#include "bombolla/base/lba-basedrawable.h"
#include "bombolla/lba-plugin-system.h"
#include "bombolla/lba-log.h"
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
  LBA_LOG ("draw");
}

static void
gl_cube_init (GLCube * self)
{
  LBA_LOG ("init");
}


/* =================== CLASS */

static void
gl_cube_class_init (GLCubeClass * klass)
{
  BaseDrawableClass *base_class = BASE_DRAWABLE_CLASS (klass);

  LBA_LOG ("%p", base_class);
  base_class->draw = gl_cube_draw;
}


G_DEFINE_TYPE (GLCube, gl_cube, G_TYPE_BASE_DRAWABLE)
BOMBOLLA_PLUGIN_SYSTEM_PROVIDE_GTYPE (gl_cube);
