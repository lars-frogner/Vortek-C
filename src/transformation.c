#include "transformation.h"

#include "gl_includes.h"
#include "error.h"
#include "dynamic_string.h"
#include "trackball.h"
#include "view_aligned_planes.h"
#include "window.h"

#include <math.h>


#define MAX_ACTIVE_SHADER_PROGRAMS 2


enum controller_state {NO_CONTROL, CONTROL};

typedef struct ProjectionTransformation
{
    float field_of_view;
    float aspect_ratio;
    float near_plane_distance;
    float far_plane_distance;
    enum projection_type type;
    Matrix4f matrix;
} ProjectionTransformation;

typedef struct Transformation
{
    Matrix4f model_matrix;
    Matrix4f view_matrix;
    ProjectionTransformation projection;
    Matrix4f modelview_matrix;
    Matrix4f MVP_matrix;
    Matrix4f inverse_view_matrix;
    Uniform uniforms[MAX_ACTIVE_SHADER_PROGRAMS];
} Transformation;

typedef struct Camera
{
    Vector3f look_axis;
    Vector3f position;
    Uniform look_axis_uniforms[MAX_ACTIVE_SHADER_PROGRAMS];
} Camera;

typedef struct CameraController
{
    float zoom_rate_modifier;
    float plane_separation_modifier;
    float current_plane_separation;
    enum controller_state state;
    int is_dragging;
} CameraController;

typedef struct ActiveShaderPrograms
{
    ShaderProgram* programs[MAX_ACTIVE_SHADER_PROGRAMS];
    unsigned int n_active_programs;
} ActiveShaderPrograms;


static void generate_shader_code_for_transformation(void);
static void update_projection_matrix(void);
static void sync_transformation(void);
static void sync_camera(void);


static Transformation transformation;
static Camera camera;
static CameraController camera_controller;

static ActiveShaderPrograms active_shader_programs = {{0}, 0};


void add_active_shader_program_for_transformation(ShaderProgram* shader_program)
{
    check(active_shader_programs.n_active_programs < MAX_ACTIVE_SHADER_PROGRAMS);
    active_shader_programs.programs[active_shader_programs.n_active_programs++] = shader_program;
}

void initialize_transformation(void)
{
    transformation.model_matrix = IDENTITY_MATRIX4F;
    transformation.view_matrix = IDENTITY_MATRIX4F;
    transformation.projection.matrix = IDENTITY_MATRIX4F;

    transformation.projection.field_of_view = 60.0f;
    transformation.projection.aspect_ratio = 1.0f;
    transformation.projection.near_plane_distance = 0.1f;;
    transformation.projection.far_plane_distance = 100.0f;
    transformation.projection.type = PERSPECTIVE_PROJECTION;

    for (unsigned int program_idx = 0; program_idx < active_shader_programs.n_active_programs; program_idx++)
    {
        initialize_uniform(transformation.uniforms + program_idx, "MVP_matrix");
        initialize_uniform(camera.look_axis_uniforms + program_idx, "look_axis");
    }

    camera_controller.zoom_rate_modifier = 1e-2f;
    camera_controller.plane_separation_modifier = 2.0f;
    camera_controller.state = CONTROL;
    camera_controller.is_dragging = 0;

    generate_shader_code_for_transformation();
}

void load_transformation(void)
{
    for (unsigned int program_idx = 0; program_idx < active_shader_programs.n_active_programs; program_idx++)
    {
        ShaderProgram* const active_shader_program = active_shader_programs.programs[program_idx];
        check(active_shader_program);

        load_uniform(active_shader_program, transformation.uniforms + program_idx);
        load_uniform(active_shader_program, camera.look_axis_uniforms + program_idx);
    }

    sync_transformation();
    sync_camera();
}

void set_view_distance(float view_distance)
{
    set_transform_translation(&transformation.view_matrix, 0, 0, -view_distance);
    sync_transformation();
    sync_camera();
}

void apply_model_scaling(float scale)
{
    assert(scale > 0);
    apply_scaling(&transformation.model_matrix, scale, scale, scale);
    sync_transformation();
}

void apply_model_translation(float dx, float dy, float dz)
{
    apply_translation(&transformation.model_matrix, dx, dy, dz);
    sync_transformation();
}

void apply_view_rotation_about_axis(const Vector3f* axis, float angle)
{
    assert(axis);
    apply_rotation_about_axis(&transformation.view_matrix, axis, angle);
    sync_transformation();
    sync_camera();
}

void apply_origin_centered_view_rotation_about_axis(const Vector3f* axis, float angle)
{
    assert(axis);

    Vector3f view_translation;
    get_matrix4f_fourth_column_vector3f(&transformation.view_matrix, &view_translation);

    set_transform_translation(&transformation.view_matrix, 0, 0, 0);
    apply_rotation_about_axis(&transformation.view_matrix, axis, angle);
    set_transform_translation(&transformation.view_matrix, view_translation.a[0], view_translation.a[1], view_translation.a[2]);

    sync_transformation();
    sync_camera();
}

void update_camera_properties(float field_of_view,
                              float aspect_ratio,
                              float near_plane_distance,
                              float far_plane_distance,
                              enum projection_type type)
{
    transformation.projection.field_of_view = field_of_view;
    transformation.projection.aspect_ratio = aspect_ratio;
    transformation.projection.near_plane_distance = near_plane_distance;
    transformation.projection.far_plane_distance = far_plane_distance;
    transformation.projection.type = type;
    update_projection_matrix();
}

void update_camera_field_of_view(float field_of_view)
{
    transformation.projection.field_of_view = field_of_view;
    update_projection_matrix();
}

void update_camera_aspect_ratio(float aspect_ratio)
{
    transformation.projection.aspect_ratio = aspect_ratio;
    update_projection_matrix();
}

void update_camera_clip_plane_distances(float near_plane_distance, float far_plane_distance)
{
    transformation.projection.near_plane_distance = near_plane_distance;
    transformation.projection.far_plane_distance = far_plane_distance;
    update_projection_matrix();
}

void update_camera_projection_type(enum projection_type type)
{
    transformation.projection.type = type;
    update_projection_matrix();
}

void cleanup_transformation(void)
{
    for (unsigned int program_idx = 0; program_idx < active_shader_programs.n_active_programs; program_idx++)
    {
        destroy_uniform(transformation.uniforms + program_idx);
        destroy_uniform(camera.look_axis_uniforms + program_idx);
        active_shader_programs.programs[program_idx] = NULL;
    }

    active_shader_programs.n_active_programs = 0;
}

const char* get_transformation_name(void)
{
    return transformation.uniforms[0].name.chars;
}

const char* get_camera_look_axis_name(void)
{
    return camera.look_axis_uniforms[0].name.chars;
}

const Matrix4f* get_view_transform_matrix(void)
{
    return &transformation.view_matrix;
}

const Matrix4f* get_model_transform_matrix(void)
{
    return &transformation.model_matrix;
}

const Matrix4f* get_projection_transform_matrix(void)
{
    return &transformation.projection.matrix;
}

const Matrix4f* get_modelview_transform_matrix(void)
{
    return &transformation.modelview_matrix;
}

const Matrix4f* get_model_view_projection_transform_matrix(void)
{
    return &transformation.MVP_matrix;
}

const Matrix4f* get_inverse_view_transform_matrix(void)
{
    return &transformation.inverse_view_matrix;
}

const Vector3f* get_camera_look_axis(void)
{
    return &camera.look_axis;
}

const Vector3f* get_camera_position(void)
{
    return &camera.position;
}

float get_model_scale(unsigned int axis)
{
    assert(axis < 3);
    return transformation.model_matrix.a[5*axis];
}

float get_component_of_vector_from_model_point_to_camera(const Vector3f* point, unsigned int component)
{
    assert(point);
    assert(component < 3);
    return (transformation.projection.type == PERSPECTIVE_PROJECTION) ?
        camera.position.a[component] - point->a[component]*get_model_scale(component) : camera.look_axis.a[component];
}

void enable_camera_control(void)
{
    camera_controller.state = CONTROL;
}

void disable_camera_control(void)
{
    camera_controller.state = NO_CONTROL;
}

void camera_control_drag_start_callback(double screen_coord_x, double screen_coord_y)
{
    if (camera_controller.state == CONTROL)
    {
        activate_trackball_in_eye_space(screen_coord_x, screen_coord_y);

        camera_controller.current_plane_separation = get_plane_separation();
        set_plane_separation(camera_controller.current_plane_separation*camera_controller.plane_separation_modifier);

        camera_controller.is_dragging = 1;
    }
}

void camera_control_drag_callback(double screen_coord_x, double screen_coord_y)
{
    if (camera_controller.state == CONTROL && camera_controller.is_dragging)
    {
        drag_trackball_in_eye_space(screen_coord_x, screen_coord_y);

        apply_origin_centered_view_rotation_about_axis(get_current_trackball_rotation_axis(),
                                                       get_current_trackball_rotation_angle());
    }
}

void camera_control_drag_end_callback(void)
{
    if (camera_controller.is_dragging)
    {
        set_plane_separation(camera_controller.current_plane_separation);
        camera_controller.is_dragging = 0;
    }
}

void camera_control_scroll_callback(double scroll_rate)
{
    if (camera_controller.state == CONTROL)
    {
        const double scale = exp(camera_controller.zoom_rate_modifier*scroll_rate);
        scale_trackball(scale);
        apply_model_scaling((float)scale);
    }
}

static void generate_shader_code_for_transformation(void)
{
    for (unsigned int program_idx = 0; program_idx < active_shader_programs.n_active_programs; program_idx++)
    {
        ShaderProgram* const active_shader_program = active_shader_programs.programs[program_idx];
        check(active_shader_program);

        add_uniform_in_shader(&active_shader_program->vertex_shader_source, "mat4", transformation.uniforms[program_idx].name.chars);
        add_uniform_in_shader(&active_shader_program->vertex_shader_source, "vec3", camera.look_axis_uniforms[program_idx].name.chars);
    }
}

static void update_projection_matrix(void)
{
    if (transformation.projection.type == PERSPECTIVE_PROJECTION)
    {
        transformation.projection.matrix = create_perspective_transform(transformation.projection.field_of_view,
                                                                        transformation.projection.aspect_ratio,
                                                                        transformation.projection.near_plane_distance,
                                                                        transformation.projection.far_plane_distance);
    }
    else
    {
        transformation.projection.matrix = create_orthographic_transform(transformation.projection.field_of_view,
                                                                         transformation.projection.aspect_ratio,
                                                                         transformation.projection.near_plane_distance,
                                                                         transformation.projection.far_plane_distance);
    }

    sync_transformation();
}

static void sync_transformation(void)
{
    transformation.modelview_matrix = multiply_matrix4f(&transformation.view_matrix, &transformation.model_matrix);
    transformation.MVP_matrix = multiply_matrix4f(&transformation.projection.matrix, &transformation.modelview_matrix);

    transformation.inverse_view_matrix = transformation.view_matrix;
    invert_matrix4f(&transformation.inverse_view_matrix);

    for (unsigned int program_idx = 0; program_idx < active_shader_programs.n_active_programs; program_idx++)
    {
        if (transformation.uniforms[program_idx].location == -1)
            continue;

        ShaderProgram* const active_shader_program = active_shader_programs.programs[program_idx];
        check(active_shader_program);

        glUseProgram(active_shader_program->id);
        abort_on_GL_error("Could not use shader program for updating transformation uniforms");

        glUniformMatrix4fv(transformation.uniforms[program_idx].location, 1, GL_TRUE, transformation.MVP_matrix.a);
        abort_on_GL_error("Could not update transform matrix uniform");
    }

    glUseProgram(0);
}

static void sync_camera(void)
{
    get_matrix4f_third_column_vector3f(&transformation.inverse_view_matrix, &camera.look_axis);
    get_matrix4f_fourth_column_vector3f(&transformation.inverse_view_matrix, &camera.position);

    normalize_vector3f(&camera.look_axis);

    for (unsigned int program_idx = 0; program_idx < active_shader_programs.n_active_programs; program_idx++)
    {
        if (camera.look_axis_uniforms[program_idx].location == -1)
            continue;

        ShaderProgram* const active_shader_program = active_shader_programs.programs[program_idx];
        check(active_shader_program);

        glUseProgram(active_shader_program->id);
        abort_on_GL_error("Could not use shader program for updating transformation uniforms");

        glUniform3f(camera.look_axis_uniforms[program_idx].location, camera.look_axis.a[0], camera.look_axis.a[1], camera.look_axis.a[2]);
        abort_on_GL_error("Could not update look axis uniform");
    }

    glUseProgram(0);
}
