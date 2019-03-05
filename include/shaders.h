#ifndef SHADERS_H
#define SHADERS_H

#include "gl_includes.h"
#include "shader_generator.h"

typedef struct ShaderProgram
{
    GLuint id;
    ShaderSource vertex_shader_source;
    ShaderSource fragment_shader_source;
    GLuint vertex_shader_id;
    GLuint fragment_shader_id;
} ShaderProgram;

GLuint load_shader_from_file(const char* filename, GLenum shader_type);
GLuint load_shader_from_string(const char* source_string, GLenum shader_type);

void initialize_shader_program(ShaderProgram* shader_program);
void compile_shader_program(ShaderProgram* shader_program);
void destroy_shader_program(ShaderProgram* shader_program);

#endif
