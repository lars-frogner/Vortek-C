#ifndef TEXTURES_H
#define TEXTURES_H

#include "fields.h"
#include "shaders.h"

void add_scalar_field_texture(const Field* field);
void load_textures(const ShaderProgram* shader_program);
void cleanup_textures(void);

#endif
