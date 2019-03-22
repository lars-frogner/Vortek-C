#ifndef VIEW_ALIGNED_PLANES_H
#define VIEW_ALIGNED_PLANES_H

#include "bricks.h"
#include "shaders.h"

void set_active_shader_program_for_planes(ShaderProgram* shader_program);

void initialize_planes(void);

void load_planes(void);

void set_active_bricked_field(const BrickedField* bricked_field);

void set_visibility_threshold(float threshold);

void set_plane_separation(float spacing_multiplier);
float get_plane_separation(void);

void draw_active_bricked_field(void);

void cleanup_planes(void);

#endif
