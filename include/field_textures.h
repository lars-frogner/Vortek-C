#ifndef FIELD_TEXTURES_H
#define FIELD_TEXTURES_H

#include "fields.h"
#include "bricks.h"
#include "shaders.h"

void initialize_field_textures(void);

void set_active_shader_program_for_field_textures(ShaderProgram* shader_program);

const char* create_scalar_field_texture(Field* field, unsigned int brick_size_exponent, unsigned int kernel_size);

BrickedField* get_texture_bricked_field(const char* name);
Field* get_texture_field(const char* name);

float field_value_to_texture_value(const char* name, float field_value);
float texture_value_to_field_value(const char* name, float texture_value);

void destroy_field_texture(const char* name);
void cleanup_field_textures(void);

#endif
