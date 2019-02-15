#ifndef RENDERER_H
#define RENDERER_H

#include "transform.h"

void initialize_renderer();
void cleanup_renderer();

void update_renderer_window_size_in_pixels(int width, int height);

void renderer_update_callback();
void renderer_resize_callback(int width, int height);
void renderer_key_action_callback();

void apply_model_rotation_about_axis(const Vector3f* axis, float angle);
void apply_model_scaling(float scale);

#endif
