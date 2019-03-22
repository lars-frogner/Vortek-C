#ifndef RENDERER_H
#define RENDERER_H

void initialize_renderer(void);
void cleanup_renderer(void);

void update_renderer_window_size_in_pixels(int width, int height);

void renderer_update_callback(void);
void renderer_resize_callback(int width, int height);
void renderer_key_action_callback(void);

#endif
