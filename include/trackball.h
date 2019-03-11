#ifndef TRACKBALL_H
#define TRACKBALL_H

void trackball_leftclick_callback(double screen_coord_x, double screen_coord_y, int screen_width, int screen_height);
void trackball_mouse_drag_callback(double screen_coord_x, double screen_coord_y, int screen_width, int screen_height);
void trackball_leftrelease_callback(void);
void trackball_zoom_callback(double zoom_rate);

#endif
