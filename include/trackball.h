#ifndef TRACKBALL_H
#define TRACKBALL_H

#include "geometry.h"

void initialize_trackball(void);

void activate_trackball_in_eye_space(double screen_coord_x, double screen_coord_y);
void activate_trackball_in_world_space(double screen_coord_x, double screen_coord_y);

void drag_trackball_in_eye_space(double screen_coord_x, double screen_coord_y);
void drag_trackball_in_world_space(double screen_coord_x, double screen_coord_y);

void scale_trackball(double scale);

const Vector3f* get_current_trackball_rotation_axis(void);
float get_current_trackball_rotation_angle(void);

#endif
