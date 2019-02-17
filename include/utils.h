#ifndef UTILS_H
#define UTILS_H

#include "gl_includes.h"

#include <stddef.h>

void find_float_array_limits(const float* array, size_t length, float* min_value, float* max_value);

void scale_float_array(float* array, size_t length, float zero_value, float unity_value);

void abort_on_GL_error(const char* message);

GLuint load_shader_from_file(const char* filename, GLenum shader_type);
GLuint load_shader_from_string(char* source_string, GLenum shader_type);

#endif
