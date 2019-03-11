#ifndef TRANSFORMATION_H
#define TRANSFORMATION_H

#include "geometry.h"
#include "shaders.h"

void initialize_transformation(void);

void generate_shader_code_for_transformation(ShaderProgram* shader_program);

void load_transformation(const ShaderProgram* shader_program);

void set_view_distance(float view_distance);
void apply_model_scaling(float scale);
void apply_model_translation(float dx, float dy, float dz);
void apply_view_rotation_about_axis(const Vector3f* axis, float angle);
void apply_origin_centered_view_rotation_about_axis(const Vector3f* axis, float angle);

void update_camera_properties(float vertical_field_of_view,
                              float aspect_ratio,
                              float near_plane_distance,
                              float far_plane_distance);
void update_camera_vertical_field_of_view(float vertical_field_of_view);
void update_camera_aspect_ratio(float aspect_ratio);
void update_camera_clip_plane_distances(float near_plane_distance, float far_plane_distance);

void sync_transformation(const ShaderProgram* shader_program);

const char* get_transformation_name(void);
const char* get_look_axis_name(void);

Matrix4f get_view_transform_matrix(void);
Matrix4f get_model_transform_matrix(void);
Matrix4f get_projection_transform_matrix(void);
Matrix4f get_modelview_transform_matrix(void);
Matrix4f get_model_view_projection_transform_matrix(void);
Vector3f get_look_axis(void);

void cleanup_transformation(void);

#endif
