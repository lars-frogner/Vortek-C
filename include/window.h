#ifndef WINDOW_H
#define WINDOW_H

void initialize_window(void);

void mainloop(void);

void get_window_shape_in_pixels(int* width, int* height);
void get_window_shape_in_screen_coordinates(int* width, int* height);
float get_window_aspect_ratio(void);

void cleanup_window(void);

#endif
