#ifndef UTILS_H
#define UTILS_H

#include "gl_includes.h"

void abort_on_GL_error(const char* message);

GLuint load_shader_from_file(const char* filename, GLenum shader_type);
GLuint load_shader_from_string(char* source_string, GLenum shader_type);

#endif
