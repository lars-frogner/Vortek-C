#include "utils.h"

#include "io.h"

#include <stdlib.h>


void abort_on_GL_error(const char* message)
{
    const GLenum error_value = glGetError();

    if (error_value != GL_NO_ERROR)
    {
        print_error("%s: %s", message, gluErrorString(error_value));
        exit(EXIT_FAILURE);
    }
}

GLuint load_shader_from_file(const char* filename, GLenum shader_type)
{
    GLuint shader_id = 0;

    char* source_string = read_text_file(filename);

    if (source_string)
    {
        shader_id = load_shader_from_string(source_string, shader_type);
        free(source_string);
    }

    return shader_id;
}

GLuint load_shader_from_string(char* source_string, GLenum shader_type)
{
    GLuint shader_id = glCreateShader(shader_type);

    if (shader_id != 0)
    {
        glShaderSource(shader_id, 1, (const GLchar**)&source_string, NULL);
        glCompileShader(shader_id);
        abort_on_GL_error("Could not compile shader");
    }
    else
    {
        print_error("Could not create shader");
    }

    return shader_id;
}
