#ifndef TEXTURE_H
#define TEXTURE_H

#include "gl_includes.h"
#include "shaders.h"

#define MAX_TEXTURES 2

typedef struct Texture
{
    GLuint unit;
    GLuint id;
    unsigned int handle;
} Texture;

Texture* create_texture(const char* name, ...);
void load_textures(const ShaderProgram* shader_program);
void destroy_texture(unsigned int handle);

#endif
