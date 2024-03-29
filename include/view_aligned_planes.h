#ifndef VIEW_ALIGNED_PLANES_H
#define VIEW_ALIGNED_PLANES_H

#include "bricks.h"
#include "shaders.h"

void set_active_shader_program_for_planes(ShaderProgram* shader_program);

void initialize_planes(void);

void load_planes(void);

void set_active_bricked_field(const BrickedField* bricked_field);

void set_lower_visibility_threshold(float threshold);
void set_upper_visibility_threshold(float threshold);

void toggle_field_outline_drawing(void);
void toggle_brick_outline_drawing(void);
void toggle_sub_brick_outline_drawing(void);

void set_plane_separation(float spacing_multiplier);
float get_plane_separation(void);

size_t get_vertex_position_variable_number(void);

const Vector3f* get_unit_axis_aligned_box_corners(void);
const Vector3f* get_centered_unit_axis_aligned_box_corners(void);
unsigned int get_axis_aligned_box_back_corner_for_plane(const Vector3f* plane_normal);
unsigned int get_axis_aligned_box_front_corner_for_plane(const Vector3f* plane_normal);

void draw_active_bricked_field(void);

void compute_plane_bounding_box_intersection_vertex(const Vector3f* plane_normal,
                                                    float origin_shift,
                                                    unsigned int back_corner_idx,
                                                    unsigned int vertex_idx,
                                                    Vector4f* intersection_position);

void cleanup_planes(void);

#endif
