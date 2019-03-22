#include "transformation.h"

#include "gl_includes.h"
#include "error.h"
#include "dynamic_string.h"


typedef struct ProjectionTransformation
{
    float field_of_view;
    float aspect_ratio;
    float near_plane_distance;
    float far_plane_distance;
    enum projection_type type;
    Matrix4f matrix;
    int needs_update;
} ProjectionTransformation;

typedef struct Transformation
{
    Matrix4f model_matrix;
    Matrix4f view_matrix;
    ProjectionTransformation projection;
    Matrix4f modelview_matrix;
    Matrix4f viewprojection_matrix;
    Matrix4f MVP_matrix;
    Uniform uniform;
} Transformation;

typedef struct Camera
{
    Vector3f look_axis;
    Vector3f position;
    Uniform look_axis_uniform;
    int needs_update;
} Camera;


static void generate_shader_code_for_transformation(void);
static void update_projection_matrix(void);
static void sync_transformation(void);


static Transformation transformation;
static Camera camera;

static ShaderProgram* active_shader_program = NULL;


void set_active_shader_program_for_transformation(ShaderProgram* shader_program)
{
    active_shader_program = shader_program;
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
    transformation.projection.needs_update = 0;
    transformation.projection.type = PERSPECTIVE_PROJECTION;

    initialize_uniform(&transformation.uniform, "MVP_matrix");
    initialize_uniform(&camera.look_axis_uniform, "look_axis");

    generate_shader_code_for_transformation();
}

void load_transformation(void)
{
    check(active_shader_program);

    load_uniform(active_shader_program, &transformation.uniform);
    load_uniform(active_shader_program, &camera.look_axis_uniform);

    transformation.uniform.needs_update = 1;
    camera.needs_update = 1;

    sync_transformation();
}

void set_view_distance(float view_distance)
{
    set_transform_translation(&transformation.view_matrix, 0, 0, -view_distance);
    transformation.uniform.needs_update = 1;
    camera.needs_update = 1;
    sync_transformation();
}

void apply_model_scaling(float scale)
{
    assert(scale > 0);
    apply_scaling(&transformation.model_matrix, scale, scale, scale);
    transformation.uniform.needs_update = 1;
    sync_transformation();
}

void apply_model_translation(float dx, float dy, float dz)
{
    apply_translation(&transformation.model_matrix, dx, dy, dz);
    transformation.uniform.needs_update = 1;
    sync_transformation();
}

void apply_view_rotation_about_axis(const Vector3f* axis, float angle)
{
    assert(axis);
    apply_rotation_about_axis(&transformation.view_matrix, axis, angle);
    transformation.uniform.needs_update = 1;
    camera.needs_update = 1;
    sync_transformation();
}

void apply_origin_centered_view_rotation_about_axis(const Vector3f* axis, float angle)
{
    assert(axis);

    Vector3f view_translation;
    get_matrix4f_fourth_column_vector3f(&transformation.view_matrix, &view_translation);

    set_transform_translation(&transformation.view_matrix, 0, 0, 0);
    apply_rotation_about_axis(&transformation.view_matrix, axis, angle);
    set_transform_translation(&transformation.view_matrix, view_translation.a[0], view_translation.a[1], view_translation.a[2]);

    transformation.uniform.needs_update = 1;
    camera.needs_update = 1;
    sync_transformation();
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
    destroy_uniform(&transformation.uniform);
    destroy_uniform(&camera.look_axis_uniform);
}

static void generate_shader_code_for_transformation(void)
{
    check(active_shader_program);
    add_uniform_in_shader(&active_shader_program->vertex_shader_source, "mat4", transformation.uniform.name.chars);
    add_uniform_in_shader(&active_shader_program->vertex_shader_source, "vec3", camera.look_axis_uniform.name.chars);
}

const char* get_transformation_name(void)
{
    return transformation.uniform.name.chars;
}

const char* get_camera_look_axis_name(void)
{
    return camera.look_axis_uniform.name.chars;
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

const Matrix4f* get_viewprojection_transform_matrix(void)
{
    return &transformation.viewprojection_matrix;
}

const Matrix4f* get_model_view_projection_transform_matrix(void)
{
    return &transformation.MVP_matrix;
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

    transformation.uniform.needs_update = 1;
    sync_transformation();
}

static void sync_transformation(void)
{
    check(active_shader_program);

    glUseProgram(active_shader_program->id);
    abort_on_GL_error("Could not use shader program for updating transformation uniforms");

    if (transformation.uniform.needs_update)
    {
        transformation.modelview_matrix = multiply_matrix4f(&transformation.view_matrix, &transformation.model_matrix);
        transformation.MVP_matrix = multiply_matrix4f(&transformation.projection.matrix, &transformation.modelview_matrix);
        transformation.viewprojection_matrix = multiply_matrix4f(&transformation.projection.matrix, &transformation.view_matrix);

        glUniformMatrix4fv(transformation.uniform.location, 1, GL_TRUE, transformation.MVP_matrix.a);
        abort_on_GL_error("Could not update transform matrix uniform");

        transformation.uniform.needs_update = 0;
    }

    if (camera.needs_update)
    {
        const Matrix4f* view_matrix = &transformation.view_matrix;

        Matrix4f inverse_view_matrix = *view_matrix;
        invert_matrix4f(&inverse_view_matrix);
        get_matrix4f_third_column_vector3f(&inverse_view_matrix, &camera.look_axis);
        normalize_vector3f(&camera.look_axis);
        get_matrix4f_fourth_column_vector3f(&inverse_view_matrix, &camera.position);

        glUniform3f(camera.look_axis_uniform.location, camera.look_axis.a[0], camera.look_axis.a[1], camera.look_axis.a[2]);
        abort_on_GL_error("Could not update look axis uniform");

        camera.needs_update = 0;
    }

    glUseProgram(0);
}
