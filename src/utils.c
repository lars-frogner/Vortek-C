#include "utils.h"

#include "error.h"
#include "io.h"

#include <stdlib.h>
#include <float.h>


void find_float_array_limits(const float* array, size_t length, float* min_value, float* max_value)
{
    assert(array);
    assert(length > 0);
    assert(min_value);
    assert(max_value);

    size_t i;
    float value;
    float min = FLT_MAX;
    float max = -FLT_MAX;

    for (i = 0; i < length; i++)
    {
        value = array[i];
        if (value < min)
            min = value;
        if (value > max)
            max = value;
    }

    *min_value = min;
    *max_value = max;
}

void scale_float_array(float* array, size_t length, float zero_value, float unity_value)
{
    assert(array);
    assert(length > 0);

    if (zero_value >= unity_value)
    {
        print_warning_message("Can only scale array with unity value larger than zero value.");
        return;
    }

    size_t i;
    const float scale = 1.0f/(unity_value - zero_value);

    for (i = 0; i < length; i++)
        array[i] = (array[i] - zero_value)*scale;
}

void abort_on_GL_error(const char* message)
{
    assert(message);

    const GLenum error_value = glGetError();

    if (error_value != GL_NO_ERROR)
        print_severe_message("%s: %s", message, gluErrorString(error_value));
}

GLuint load_shader_from_file(const char* filename, GLenum shader_type)
{
    assert(filename);

    GLuint shader_id = 0;

    char* source_string = read_text_file(filename);

    if (source_string)
    {
        shader_id = load_shader_from_string(source_string, shader_type);
        free(source_string);

        if (shader_id == 0)
            print_error_message("Could not load shader source \"%s\".", filename);
    }

    return shader_id;
}

GLuint load_shader_from_string(char* source_string, GLenum shader_type)
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
