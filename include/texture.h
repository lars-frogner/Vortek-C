#ifndef TEXTURE_H
#define TEXTURE_H

#include "gl_includes.h"
#include "shaders.h"

#define MAX_TEXTURE_NAME_LENGTH 15

typedef struct Texture
{
    GLuint unit;
    GLuint id;
    char name[MAX_TEXTURE_NAME_LENGTH];
} Texture;

void initialize_textures(void);
Texture* create_texture(void);
void load_textures(const ShaderProgram* shader_program);
void destroy_texture(Texture* texture);
void cleanup_textures(void);

#endif
