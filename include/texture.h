#ifndef TEXTURE_H
#define TEXTURE_H

#include "gl_includes.h"
#include "dynamic_string.h"
#include "linked_list.h"
#include "shaders.h"

typedef struct Texture
{
    GLuint unit;
    LinkedList ids;
    DynamicString name;
} Texture;

void initialize_textures(void);
Texture* create_texture(void);
void load_textures(const ShaderProgram* shader_program);
void destroy_texture(Texture* texture);
void cleanup_textures(void);

#endif
