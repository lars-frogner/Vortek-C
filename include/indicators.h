#ifndef INDICATORS_H
#define INDICATORS_H

#include "geometry.h"
#include "shaders.h"
#include "bricks.h"

enum indicator_drawing_pass {INDICATOR_BACK_PASS, INDICATOR_FRONT_PASS};

void set_active_shader_program_for_indicators(ShaderProgram* shader_program);

void initialize_indicators(void);

void add_boundary_indicator_for_field(const char* field_texture_name, const Vector4f* color);
void add_boundary_indicator_for_bricks(const char* field_texture_name, const Vector4f* color);
void add_boundary_indicator_for_sub_bricks(const char* field_texture_name, const Vector4f* color);

void draw_field_boundary_indicator(const BrickedField* bricked_field, unsigned int reference_corner_idx, enum indicator_drawing_pass pass);
void draw_brick_boundary_indicator(const BrickedField* bricked_field);
void draw_sub_brick_boundary_indicator(const BrickedField* bricked_field);

void destroy_edge_indicator(const char* name);
void cleanup_indicators(void);

#endif
