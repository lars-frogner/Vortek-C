#ifndef UTILS_H
#define UTILS_H

#include "gl_includes.h"

extern const double PI;

int max(int a, int b);
int min(int a, int b);

unsigned int umax(unsigned int a, unsigned int b);
unsigned int umin(unsigned int a, unsigned int b);

unsigned int argfmax3(float x, float y, float z);

int sign(float x);
int signum(float x);

float cotangent(float angle);
float degrees_to_radians(float degrees);
float radians_to_regrees(float radians);

char* read_file(const char* filename);

void print_info(const char* message, ...);
void print_error(const char* message, ...);

void abort_on_GL_error(const char* message);

GLuint load_shader_from_file(const char* filename, GLenum shader_type);
GLuint load_shader_from_string(char* source_string, GLenum shader_type);

#endif
