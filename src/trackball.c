#include "trackball.h"

#include "error.h"
#include "extra_math.h"

#include <math.h>


static void compute_trackball_point(const Trackball* trackball, Vector3* point, double x, double y);
static double compute_trackball_pick_depth(const Trackball* trackball, double x, double y);
static void screen_coords_to_trackball_coords(double screen_coord_x, double screen_coord_y,
                                              int screen_width, int screen_height,
                                              double* x, double* y);


void initialize_trackball(Trackball* trackball)
{
    assert(trackball);
    trackball->radius = 1.0;
    set_vector3_elements(&trackball->previous_activation_point, 0, 0, 0);
    set_vector3f_elements(&trackball->current_rotation_axis, 0, 0, 0);
    trackball->current_rotation_angle = 0;
}

void activate_trackball(Trackball* trackball, double screen_coord_x, double screen_coord_y, int screen_width, int screen_height)
{
    assert(trackball);
    assert(screen_width > 0);
    assert(screen_height > 0);

    double x, y;
    screen_coords_to_trackball_coords(screen_coord_x, screen_coord_y, screen_width, screen_height, &x, &y);
    compute_trackball_point(trackball, &trackball->previous_activation_point, x, y);
}

void drag_trackball(Trackball* trackball, double screen_coord_x, double screen_coord_y, int screen_width, int screen_height)
{
    assert(trackball);
    assert(screen_width > 0);
    assert(screen_height > 0);

    double x, y;
    screen_coords_to_trackball_coords(screen_coord_x, screen_coord_y, screen_width, screen_height, &x, &y);
    Vector3 current_activation_point;
    compute_trackball_point(trackball, &current_activation_point, x, y);

    Vector3 rotation_axis = cross3(&trackball->previous_activation_point, &current_activation_point);
    normalize_vector3(&rotation_axis);
    copy_vector3_to_vector3f(&rotation_axis, &trackball->current_rotation_axis);

    trackball->current_rotation_angle = (float)acos(dot3(&trackball->previous_activation_point, &current_activation_point));

    copy_vector3(&current_activation_point, &trackball->previous_activation_point);
}

void scale_trackball(Trackball* trackball, double scale)
{
    assert(trackball);
    trackball->radius *= scale;
}

static void compute_trackball_point(const Trackball* trackball, Vector3* point, double x, double y)
{
    assert(trackball);
    assert(point);
    set_vector3_elements(point, x, y, compute_trackball_pick_depth(trackball, x, y));
    normalize_vector3(point);
}

static double compute_trackball_pick_depth(const Trackball* trackball, double x, double y)
{
    assert(trackball);

    const double squared_2D_radius = x*x + y*y;
    const double squared_trackball_radius = trackball->radius*trackball->radius;
    const double squared_2D_radius_limit = 0.5*squared_trackball_radius;
    double z;

    // Project to sphere if close to center or hyperbolic surface otherwise
    if (squared_2D_radius <= squared_2D_radius_limit)
        z = sqrt(squared_trackball_radius - squared_2D_radius);
    else
        z = squared_2D_radius_limit/sqrt(squared_2D_radius);

    return z;
}

static void screen_coords_to_trackball_coords(double screen_coord_x, double screen_coord_y,
                                              int screen_width, int screen_height,
                                              double* x, double* y)
{
    assert(screen_width > 0);
    assert(screen_height > 0);
    assert(x);
    assert(y);

    const double scale = 2.0/screen_height;
    *x =  scale*(screen_coord_x - 0.5*screen_width);
    *y = -scale*(screen_coord_y - 0.5*screen_height);
}
