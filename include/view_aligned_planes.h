#ifndef VIEW_ALIGNED_PLANES_H
#define VIEW_ALIGNED_PLANES_H

#include "bricks.h"
#include "shaders.h"

void initialize_planes(void);

void generate_shader_code_for_planes(ShaderProgram* shader_program);

void load_planes(const ShaderProgram* shader_program);

void set_active_bricked_field(const BrickedField* bricked_field);

void set_plane_separation(float spacing_multiplier);
float get_plane_separation(void);

void draw_brick(size_t brick_idx);

void sync_planes(const ShaderProgram* shader_program);

void cleanup_planes(void);

#endif
