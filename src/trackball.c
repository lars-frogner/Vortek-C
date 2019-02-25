#include "trackball.h"

#include "error.h"
#include "extra_math.h"
#include "geometry.h"
#include "transformation.h"
#include "renderer.h"

#include <math.h>


static Vector3 compute_trackball_point(double x, double y);
static double compute_trackball_pick_depth(double x, double y);
static void screen_coords_to_trackball_coords(double screen_coord_x, double screen_coord_y,
                                              int screen_width, int screen_height,
                                              double* x, double* y);


static const double zoom_rate_modifier = 1e-2;

static double trackball_radius = 1.0;

static Vector3 previous_trackball_point = {{0}};


void trackball_leftclick_callback(double screen_coord_x, double screen_coord_y, int screen_width, int screen_height)
{
    assert(screen_width > 0);
    assert(screen_height > 0);

    double x, y;
    screen_coords_to_trackball_coords(screen_coord_x, screen_coord_y, screen_width, screen_height, &x, &y);
    previous_trackball_point = compute_trackball_point(x, y);
}

void trackball_mouse_drag_callback(double screen_coord_x, double screen_coord_y, int screen_width, int screen_height)
{
    assert(screen_width > 0);
    assert(screen_height > 0);

    double x, y;
    screen_coords_to_trackball_coords(screen_coord_x, screen_coord_y, screen_width, screen_height, &x, &y);
    const Vector3 current_trackball_point = compute_trackball_point(x, y);

    Vector3 rotation_axis = cross3(&previous_trackball_point, &current_trackball_point);
    normalize_vector3(&rotation_axis);
    const Vector3f rotation_axisf = vector3_to_vector3f(&rotation_axis);

    const float rotation_angle = (float)acos(dot3(&previous_trackball_point, &current_trackball_point));

    apply_model_rotation_about_axis(&rotation_axisf, rotation_angle);
    sync_renderer();

    previous_trackball_point = current_trackball_point;
}

void trackball_zoom_callback(double zoom_rate)
{
    const float scale = (float)exp(zoom_rate_modifier*zoom_rate);
    trackball_radius *= scale;
    apply_model_scaling(scale);
    sync_renderer();
}

static Vector3 compute_trackball_point(double x, double y)
{
    Vector3 point = {{x, y, compute_trackball_pick_depth(x, y)}};
    normalize_vector3(&point);
    return point;
}

static double compute_trackball_pick_depth(double x, double y)
{
    const double squared_2D_radius = x*x + y*y;
    const double squared_trackball_radius = trackball_radius*trackball_radius;
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
