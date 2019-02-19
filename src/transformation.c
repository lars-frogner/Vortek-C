#include "transformation.h"

#include "gl_includes.h"
#include "error.h"


typedef struct ProjectionTransformation
{
    float vertical_field_of_view;
    float aspect_ratio;
    float near_plane_distance;
    float far_plane_distance;
    Matrix4f matrix;
    int needs_update;
} ProjectionTransformation;

typedef struct Transformation
{
    Matrix4f model_matrix;
    Matrix4f view_matrix;
    ProjectionTransformation projection;
    Matrix4f model_view_projection_matrix;
    GLint uniform_location;
    const char* name;
    int needs_update;
} Transformation;


static Transformation transformation;


static void update_perspective_projection_matrix(void);


void create_transform_matrices(float view_distance,
                               float vertical_field_of_view,
                               float aspect_ratio,
                               float near_plane_distance,
                               float far_plane_distance)
{
    transformation.name = "model_view_projection_matrix";

    transformation.model_matrix = IDENTITY_MATRIX4F;

    transformation.view_matrix = IDENTITY_MATRIX4F;
    apply_translation(&transformation.view_matrix, 0, 0, -view_distance);

    update_camera_vertical_field_of_view(vertical_field_of_view);
    update_camera_aspect_ratio(aspect_ratio);
    update_camera_clip_plane_distances(near_plane_distance, far_plane_distance);
    update_perspective_projection_matrix();
}

void load_transform_matrices(const ShaderProgram* shader_program)
{
    check(shader_program);

    transformation.uniform_location = glGetUniformLocation(shader_program->id, transformation.name);
    abort_on_GL_error("Could not get transform matrix uniform location");

    transformation.needs_update = 1;
    sync_transform_matrices(shader_program);
}

void apply_model_rotation_about_axis(const Vector3f* axis, float angle)
{
    assert(axis);
    apply_rotation_about_axis(&transformation.model_matrix, axis, angle);
    transformation.needs_update = 1;
}

void apply_model_scaling(float scale)
{
    assert(scale > 0);
    apply_scaling(&transformation.model_matrix, scale, scale, scale);
    transformation.needs_update = 1;
}

void update_camera_vertical_field_of_view(float vertical_field_of_view)
{
    transformation.projection.vertical_field_of_view = vertical_field_of_view;
    transformation.projection.needs_update = 1;
}

void update_camera_aspect_ratio(float aspect_ratio)
{
    transformation.projection.aspect_ratio = aspect_ratio;
    transformation.projection.needs_update = 1;
}

void update_camera_clip_plane_distances(float near_plane_distance, float far_plane_distance)
{
    transformation.projection.near_plane_distance = near_plane_distance;
    transformation.projection.far_plane_distance = far_plane_distance;
    transformation.projection.needs_update = 1;
}

void sync_transform_matrices(const ShaderProgram* shader_program)
{
    assert(shader_program);

    update_perspective_projection_matrix();

    if (transformation.needs_update)
    {
        const Matrix4f modelview_matrix = matmul4f(&transformation.view_matrix, &transformation.model_matrix);
        transformation.model_view_projection_matrix = matmul4f(&transformation.projection.matrix, &modelview_matrix);

        glUseProgram(shader_program->id);
        abort_on_GL_error("Could not use shader program for updating transform matrix uniform");
        glUniformMatrix4fv(transformation.uniform_location, 1, GL_TRUE, transformation.model_view_projection_matrix.a);
        glUseProgram(0);

        transformation.needs_update = 0;
    }
}

Matrix4f get_view_transform_matrix(void)
{
    return transformation.view_matrix;
}

Matrix4f get_model_transform_matrix(void)
{
    return transformation.model_matrix;
}

Matrix4f get_projection_transform_matrix(void)
{
    return transformation.projection.matrix;
}

static void update_perspective_projection_matrix(void)
{
    if (transformation.projection.needs_update)
    {
        transformation.projection.matrix = create_perspective_transform(transformation.projection.vertical_field_of_view,
                                                                        transformation.projection.aspect_ratio,
                                                                        transformation.projection.near_plane_distance,
                                                                        transformation.projection.far_plane_distance);
        transformation.projection.needs_update = 0;
        transformation.needs_update = 1;
    }
}
