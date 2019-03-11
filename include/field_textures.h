#ifndef FIELD_TEXTURES_H
#define FIELD_TEXTURES_H

#include "fields.h"
#include "bricks.h"
#include "shaders.h"

void initialize_field_textures(void);

const char* create_scalar_field_texture(const Field* field, ShaderProgram* shader_program);

const BrickedField* get_bricked_field_texture(const char* name);

void destroy_field_texture(const char* name);
void cleanup_field_textures(void);

#endif
