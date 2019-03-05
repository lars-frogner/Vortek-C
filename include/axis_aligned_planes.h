#ifndef AXIS_ALIGNED_PLANES_H
#define AXIS_ALIGNED_PLANES_H

#include <stddef.h>

#include "shaders.h"

void create_planes(size_t size_x, size_t size_y, size_t size_z,
                   float extent_x, float extent_y, float extent_z,
                   float spacing_multiplier);
void generate_shader_code_for_planes(ShaderProgram* shader_program);
void sync_planes(void);
void draw_planes(void);
void cleanup_planes(void);

#endif
