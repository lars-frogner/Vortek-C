#ifndef TRANSFORMATION_H
#define TRANSFORMATION_H

#include "geometry.h"
#include "shaders.h"

enum projection_type {PERSPECTIVE_PROJECTION, ORTHOGRAPHIC_PROJECTION};

void add_active_shader_program_for_transformation(ShaderProgram* shader_program);

void initialize_transformation(void);

void load_transformation(void);

void set_view_distance(float view_distance);
void apply_model_scaling(float scale);
void apply_model_translation(float dx, float dy, float dz);
void apply_view_rotation_about_axis(const Vector3f* axis, float angle);
void apply_origin_centered_view_rotation_about_axis(const Vector3f* axis, float angle);

void update_camera_properties(float vertical_field_of_view,
                              float aspect_ratio,
                              float near_plane_distance,
                              float far_plane_distance,
                              enum projection_type type);
void update_camera_field_of_view(float field_of_view);
void update_camera_aspect_ratio(float aspect_ratio);
void update_camera_clip_plane_distances(float near_plane_distance, float far_plane_distance);
void update_camera_projection_type(enum projection_type type);

const char* get_transformation_name(void);
const char* get_camera_look_axis_name(void);

const Matrix4f* get_view_transform_matrix(void);
const Matrix4f* get_model_transform_matrix(void);
const Matrix4f* get_projection_transform_matrix(void);
const Matrix4f* get_modelview_transform_matrix(void);
const Matrix4f* get_model_view_projection_transform_matrix(void);
const Matrix4f* get_inverse_view_transform_matrix(void);

const Vector3f* get_camera_look_axis(void);
const Vector3f* get_camera_position(void);

float get_model_scale(unsigned int axis);
float get_component_of_vector_from_model_point_to_camera(const Vector3f* point, unsigned int component);

void enable_camera_control(void);
void disable_camera_control(void);

void camera_control_drag_start_callback(double screen_coord_x, double screen_coord_y);
void camera_control_drag_callback(double screen_coord_x, double screen_coord_y);
void camera_control_drag_end_callback(void);
void camera_control_scroll_callback(double scroll_rate);

void cleanup_transformation(void);

#endif
