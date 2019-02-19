#ifndef TRANSFORMATION_H
#define TRANSFORMATION_H

#include "geometry.h"
#include "shaders.h"

void create_transform_matrices(float view_distance,
                               float vertical_field_of_view,
                               float aspect_ratio,
                               float near_plane_distance,
                               float far_plane_distance);

void load_transform_matrices(const ShaderProgram* shader_program);

void apply_model_rotation_about_axis(const Vector3f* axis, float angle);
void apply_model_scaling(float scale);
void update_camera_vertical_field_of_view(float vertical_field_of_view);
void update_camera_aspect_ratio(float aspect_ratio);
void update_camera_clip_plane_distances(float near_plane_distance, float far_plane_distance);

void sync_transform_matrices(const ShaderProgram* shader_program);

Matrix4f get_view_transform_matrix(void);
Matrix4f get_model_transform_matrix(void);
Matrix4f get_projection_transform_matrix(void);

#endif
