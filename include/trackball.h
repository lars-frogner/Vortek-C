#ifndef TRACKBALL_H
#define TRACKBALL_H

#include "geometry.h"

typedef struct Trackball
{
    double radius;
    Vector3 previous_activation_point;
    Vector3f current_rotation_axis;
    float current_rotation_angle;
} Trackball;

void initialize_trackball(Trackball* trackball);
void activate_trackball(Trackball* trackball, double screen_coord_x, double screen_coord_y, int screen_width, int screen_height);
void drag_trackball(Trackball* trackball, double screen_coord_x, double screen_coord_y, int screen_width, int screen_height);
void scale_trackball(Trackball* trackball, double scale);

#endif
