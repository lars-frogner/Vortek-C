#ifndef INDICATORS_H
#define INDICATORS_H

#include "geometry.h"
#include "bricks.h"
#include "shaders.h"

enum indicator_drawing_pass {INDICATOR_BACK_PASS, INDICATOR_FRONT_PASS};

void set_active_shader_program_for_indicators(ShaderProgram* shader_program);

void initialize_indicators(void);

void add_boundary_indicator_for_field(const char* field_texture_name, const Vector4f* color);
void add_boundary_indicator_for_bricks(const char* field_texture_name, const Vector4f* color);
void add_boundary_indicator_for_sub_bricks(const char* field_texture_name, const Vector4f* color);
const char* add_boundary_indicator_for_clip_plane(unsigned int clip_plane_idx,
                                                  const Vector3f* normal,
                                                  float origin_shift,
                                                  unsigned int back_corner_idx,
                                                  const Vector4f* color);

void update_clip_plane_boundary_indicator(const char* indicator_name,
                                          const Vector3f* normal,
                                          float origin_shift,
                                          unsigned int back_corner_idx);

void draw_field_boundary_indicator(const BrickedField* bricked_field, unsigned int reference_corner_idx, enum indicator_drawing_pass pass);
void draw_brick_boundary_indicator(const BrickedField* bricked_field);
void draw_sub_brick_boundary_indicator(const BrickedField* bricked_field);
void draw_clip_plane_boundary_indicator(const char* indicator_name);

void destroy_edge_indicator(const char* name);
void cleanup_indicators(void);

#endif
