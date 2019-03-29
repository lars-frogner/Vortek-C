#include "trackball.h"

#include "error.h"
#include "extra_math.h"
#include "transformation.h"
#include "window.h"

#include <math.h>


typedef struct Trackball
{
    double radius;
    Vector3 previous_activation_point;
    Vector3 previous_world_space_activation_point;
    Vector3f rotation_axis;
    float rotation_angle;
} Trackball;


static void compute_trackball_point(const Trackball* trackball, double x, double y, Vector3* point);
static double compute_trackball_pick_depth(const Trackball* trackball, double x, double y);
static void screen_coords_to_trackball_coords(double screen_coord_x, double screen_coord_y,
                                              double* x, double* y);


static Trackball trackball;


void initialize_trackball(void)
{
    trackball.radius = 1.0;
    set_vector3_elements(&trackball.previous_activation_point, 0, 0, 0);
    set_vector3f_elements(&trackball.rotation_axis, 0, 0, 1.0f);
    trackball.rotation_angle = 0;
}

void activate_trackball_in_eye_space(double screen_coord_x, double screen_coord_y)
{
    double x, y;
    screen_coords_to_trackball_coords(screen_coord_x, screen_coord_y, &x, &y);

    compute_trackball_point(&trackball, x, y, &trackball.previous_activation_point);
}

void activate_trackball_in_world_space(double screen_coord_x, double screen_coord_y)
{
    double x, y;
    screen_coords_to_trackball_coords(screen_coord_x, screen_coord_y, &x, &y);

    compute_trackball_point(&trackball, x, y, &trackball.previous_activation_point);
    trackball.previous_world_space_activation_point = multiply_matrix4f_sub_3x3_vector3(get_inverse_view_transform_matrix(), &trackball.previous_activation_point);
}

void drag_trackball_in_eye_space(double screen_coord_x, double screen_coord_y)
{
    double x, y;
    screen_coords_to_trackball_coords(screen_coord_x, screen_coord_y, &x, &y);

    Vector3 current_activation_point;
    compute_trackball_point(&trackball, x, y, &current_activation_point);

    Vector3 rotation_axis = cross3(&trackball.previous_activation_point, &current_activation_point);
    normalize_vector3(&rotation_axis);
    copy_vector3_to_vector3f(&rotation_axis, &trackball.rotation_axis);

    trackball.rotation_angle = (float)acos(dot3(&trackball.previous_activation_point, &current_activation_point));

    trackball.previous_activation_point = current_activation_point;
}

void drag_trackball_in_world_space(double screen_coord_x, double screen_coord_y)
{
    double x, y;
    screen_coords_to_trackball_coords(screen_coord_x, screen_coord_y, &x, &y);

    Vector3 current_activation_point, current_world_space_activation_point;
    compute_trackball_point(&trackball, x, y, &current_activation_point);
    current_world_space_activation_point = multiply_matrix4f_sub_3x3_vector3(get_inverse_view_transform_matrix(), &current_activation_point);

    Vector3 rotation_axis = cross3(&trackball.previous_world_space_activation_point, &current_world_space_activation_point);
    normalize_vector3(&rotation_axis);
    copy_vector3_to_vector3f(&rotation_axis, &trackball.rotation_axis);

    trackball.rotation_angle = (float)acos(dot3(&trackball.previous_activation_point, &current_activation_point));

    trackball.previous_activation_point = current_activation_point;
    trackball.previous_world_space_activation_point = current_world_space_activation_point;
}

void scale_trackball(double scale)
{
    trackball.radius *= scale;
}

const Vector3f* get_current_trackball_rotation_axis(void)
{
    return &trackball.rotation_axis;
}

float get_current_trackball_rotation_angle(void)
{
    return trackball.rotation_angle;
}

static void compute_trackball_point(const Trackball* trackball, double x, double y, Vector3* point)
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
                                              double* x, double* y)
{
    assert(x);
    assert(y);

    int screen_width, screen_height;
    get_window_shape_in_screen_coordinates(&screen_width, &screen_height);

    const double scale = 2.0/screen_height;
    *x =  scale*(screen_coord_x - 0.5*screen_width);
    *y = -scale*(screen_coord_y - 0.5*screen_height);
}
