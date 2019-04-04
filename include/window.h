#ifndef WINDOW_H
#define WINDOW_H

void initialize_window(void);

void initialize_mainloop(void);
int step_mainloop(void);
void mainloop(void);

void focus_window(void);

void get_window_shape_in_pixels(int* width, int* height);
void get_window_shape_in_screen_coordinates(int* width, int* height);
float get_window_aspect_ratio(void);

void cleanup_window(void);

#endif
