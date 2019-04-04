#ifndef RENDERER_H
#define RENDERER_H

#include "fields.h"

void initialize_renderer(void);
void cleanup_renderer(void);

void update_renderer_window_size_in_pixels(int width, int height);

void renderer_update_callback(void);
void renderer_resize_callback(int width, int height);

int has_rendering_data(void);

const char* get_single_field_rendering_texture_name(void);
const char* get_single_field_rendering_TF_name(void);

void set_single_field_rendering_field(const char* field_name);

#endif
