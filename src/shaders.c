#include "shaders.h"

#include "error.h"
#include "io.h"

#include <stdlib.h>
#include <stdio.h>


GLuint load_shader_from_file(const char* filename, GLenum shader_type)
{
    assert(filename);

    GLuint shader_id = 0;

    char* source_string = read_text_file(filename);

    if (source_string)
    {
        shader_id = load_shader_from_string((const char*)source_string, shader_type);
        free(source_string);

        if (shader_id == 0)
            print_error_message("Could not load shader source \"%s\".", filename);
    }

    return shader_id;
}

GLuint load_shader_from_string(const char* source_string, GLenum shader_type)
{
    assert(source_string);

    GLuint shader_id = glCreateShader(shader_type);

    if (shader_id != 0)
    {
        glShaderSource(shader_id, 1, (const GLchar**)&source_string, NULL);
        glCompileShader(shader_id);

        GLint is_compiled = 0;
        glGetShaderiv(shader_id, GL_COMPILE_STATUS, &is_compiled);
        if(is_compiled == GL_FALSE)
        {
            GLint max_length = 0;
            glGetShaderiv(shader_id, GL_INFO_LOG_LENGTH, &max_length);

            GLchar* error_log = (GLchar*)malloc((size_t)max_length*sizeof(GLchar));

            glGetShaderInfoLog(shader_id, max_length, &max_length, error_log);

            print_error_message("Could not compile shader: %s", error_log);

            free(error_log);
            glDeleteShader(shader_id);

            shader_id = 0;
        }
    }
    else
    {
        print_error_message("Could not create shader.");
    }

    return shader_id;
}

void initialize_shader_program(ShaderProgram* shader_program)
{
    check(shader_program);

    shader_program->id = glCreateProgram();
    abort_on_GL_error("Could not create shader program");

    initialize_shader_source(&shader_program->vertex_shader_source);
    initialize_shader_source(&shader_program->fragment_shader_source);

    shader_program->vertex_shader_id = 0;
    shader_program->vertex_shader_id = 0;
}

void compile_shader_program(ShaderProgram* shader_program)
{
    check(shader_program);

    if (0)
    {
        const char* vertex_source_string = generate_shader_code(&shader_program->vertex_shader_source);
        const char* fragment_source_string = generate_shader_code(&shader_program->fragment_shader_source);

        printf("\n------------------------- Vertex shader -------------------------\n\n%s",
               vertex_source_string);
        printf("\n------------------------ Fragment shader ------------------------\n\n%s\n-----------------------------------------------------------------\n\n",
               fragment_source_string);

        shader_program->vertex_shader_id = load_shader_from_string(vertex_source_string, GL_VERTEX_SHADER);
        shader_program->fragment_shader_id = load_shader_from_string(fragment_source_string, GL_FRAGMENT_SHADER);
    }
    else
    {
        shader_program->vertex_shader_id = load_shader_from_file("/Users/larsfrog/Dropbox/Code/Rendering/Vortek/temp_shaders/shader.vert", GL_VERTEX_SHADER);
        shader_program->fragment_shader_id = load_shader_from_file("/Users/larsfrog/Dropbox/Code/Rendering/Vortek/temp_shaders/shader.frag", GL_FRAGMENT_SHADER);
    }

    if (shader_program->vertex_shader_id == 0 ||
        shader_program->fragment_shader_id == 0)
        print_severe_message("Could not load shaders.");

    glAttachShader(shader_program->id, shader_program->vertex_shader_id);
    glAttachShader(shader_program->id, shader_program->fragment_shader_id);

    glLinkProgram(shader_program->id);

    GLint is_linked = 0;
    glGetProgramiv(shader_program->id, GL_LINK_STATUS, &is_linked);
    if (is_linked == GL_FALSE)
    {
        GLint max_length = 0;
        glGetProgramiv(shader_program->id, GL_INFO_LOG_LENGTH, &max_length);

        GLchar* error_log = (GLchar*)malloc((size_t)max_length*sizeof(GLchar));

        glGetProgramInfoLog(shader_program->id, max_length, &max_length, error_log);

        print_error_message("Could not link shader program: %s", error_log);

        free(error_log);
        glDeleteProgram(shader_program->id);

        exit(EXIT_FAILURE);
    }

    glDeleteShader(shader_program->vertex_shader_id);
    glDeleteShader(shader_program->fragment_shader_id);
}

void destroy_shader_program(ShaderProgram* shader_program)
{
    check(shader_program);

    glDeleteProgram(shader_program->id);
    abort_on_GL_error("Could not destroy shader program");

    destroy_shader_source(&shader_program->vertex_shader_source);
    destroy_shader_source(&shader_program->fragment_shader_source);

    shader_program->id = 0;
    shader_program->vertex_shader_id = 0;
    shader_program->fragment_shader_id = 0;
}
