#ifndef FS_GLU_H_
#define FS_GLU_H_

#ifdef MACOSX
#include <OpenGL/glu.h>
#elif defined(HAVE_GLES)
#include <GLES/glues.h>
#else
#include <GL/glu.h>
#endif

#endif // FS_GLU_H_
