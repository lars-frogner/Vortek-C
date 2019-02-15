#include "utils.h"

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <math.h>


const double PI = 3.14159265358979323846;


int max(int a, int b)
{
    return (a >= b) ? a : b;
}

int min(int a, int b)
{
    return (a <= b) ? a : b;
}

unsigned int umax(unsigned int a, unsigned int b)
{
    return (a >= b) ? a : b;
}

unsigned int umin(unsigned int a, unsigned int b)
{
    return (a >= b) ? a : b;
}

unsigned int argfmax3(float x, float y, float z)
{
    // Returns the index of the largest number. When multiple numbers are equal
    // and largest the first index of these numbers is returned.
    return (z > y) ? ((z > x) ? 2 : 0) : ((y > x) ? 1 : 0);
}

int sign(float x)
{
    return (x >= 0) ? 1 : -1;
}

int signum(float x)
{
    return (x > 0) ? 1 : ((x < 0) ? -1 : 0);
}

float cotangent(float angle)
{
  return (float)(1.0/tan(angle));
}

float degrees_to_radians(float degrees)
{
  return degrees*(float)(PI/180);
}

float radians_to_regrees(float radians)
{
  return radians*(float)(180/PI);
}

char* read_file(const char* filename)
{
    FILE* file;
    long file_size = -1;
    char* content = NULL;

    file = fopen(filename, "rb");

    if (!file)
    {
        print_error("Could not open file %s", filename);
        return NULL;
    }

    if (fseek(file, 0, SEEK_END))
    {
        print_error("Could not seek in file %s", filename);
        fclose(file);
        return NULL;
    }

    file_size = ftell(file);

    if (file_size == -1)
    {
        print_error("Could not determine size of file %s", filename);
        fclose(file);
        return NULL;
    }

    rewind(file);

    content = (char*)malloc(file_size + 1);

    if (!content)
    {
        print_error("Could not allocate %i bytes", file_size);
        fclose(file);
        return NULL;
    }

    size_t read_size = fread(content, sizeof(char), file_size, file);

    fclose(file);

    if ((long)read_size != file_size)
    {
        print_error("Could not read file %s", filename);
        free(content);
        return NULL;
    }

    content[file_size] = '\0';

    return content;
}

void print_info(const char* message, ...)
{
    va_list args;
    printf("Info: ");
    va_start(args, message);
    vprintf(message, args);
    va_end(args);
    printf("\n");
}

void print_error(const char* message, ...)
{
    va_list args;
    fprintf(stderr, "Error: ");
    va_start(args, message);
    vfprintf(stderr, message, args);
    va_end(args);
    fprintf(stderr, "\n");
}

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

    char* source_string = read_file(filename);

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
