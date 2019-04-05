#ifndef GL_INCLUDES_H
#define GL_INCLUDES_H

#ifdef __APPLE__
    #include <OpenGL/gl3.h>
    #include <OpenGL/glu.h>
#else
    #ifdef _WIN32
        #include <windows.h>
    #endif
    #define GL_GLEXT_PROTOTYPES
    #include <GL/glcorearb.h>
    #include <GL/glu.h>
#endif

#endif
