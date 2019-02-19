#ifndef SHADERS_H
#define SHADERS_H

#include "gl_includes.h"

typedef struct ShaderProgram
{
    GLuint id;
    GLuint vertex_shader_id;
    GLuint fragment_shader_id;
} ShaderProgram;

GLuint load_shader_from_file(const char* filename, GLenum shader_type);
GLuint load_shader_from_string(char* source_string, GLenum shader_type);

void create_shader_program(ShaderProgram* shader_program);
void destroy_shader_program(ShaderProgram* shader_program);

#endif
