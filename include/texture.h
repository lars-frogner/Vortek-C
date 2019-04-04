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

void set_active_shader_program_for_textures(ShaderProgram* shader_program);

void initialize_textures(void);

Texture* create_texture(void);

void load_textures(void);

void delete_texture_data(Texture* texture);
void destroy_texture(Texture* texture);
void cleanup_textures(void);

#endif
