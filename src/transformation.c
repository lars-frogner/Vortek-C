#include "transformation.h"

#include "gl_includes.h"
#include "error.h"
#include "dynamic_string.h"


typedef struct Uniform
{
    GLint location;
    DynamicString name;
    int needs_update;
} Uniform;

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
    Uniform uniform;
} Transformation;

typedef struct LookAxis
{
    Vector3f axis;
    Uniform uniform;
} LookAxis;


static void update_perspective_projection_matrix(void);


static Transformation transformation;
static LookAxis look_axis;


void initialize_transformation(void)
{
    transformation.model_matrix = IDENTITY_MATRIX4F;
    transformation.view_matrix = IDENTITY_MATRIX4F;
    transformation.projection.matrix = IDENTITY_MATRIX4F;

    transformation.projection.vertical_field_of_view = 60.0f;
    transformation.projection.aspect_ratio = 1.0f;
    transformation.projection.near_plane_distance = 0.1f;;
    transformation.projection.far_plane_distance = 100.0f;
    transformation.projection.needs_update = 0;

    transformation.uniform.name = create_string("MVP_matrix");
    transformation.uniform.location = -1;
    transformation.uniform.needs_update = 0;

    look_axis.uniform.name = create_string("look_axis");
    look_axis.uniform.location = -1;
    look_axis.uniform.needs_update = 1;
}

void generate_shader_code_for_transformation(ShaderProgram* shader_program)
{
    check(shader_program);
    add_uniform_in_shader(&shader_program->vertex_shader_source, "mat4", transformation.uniform.name.chars);
    add_uniform_in_shader(&shader_program->vertex_shader_source, "vec3", look_axis.uniform.name.chars);
}

void load_transformation(const ShaderProgram* shader_program)
{
    check(shader_program);

    transformation.uniform.location = glGetUniformLocation(shader_program->id, transformation.uniform.name.chars);
    abort_on_GL_error("Could not get transform matrix uniform location");

    if (transformation.uniform.location == -1)
        print_warning_message("Uniform \"%s\" not used in shader program.", transformation.uniform.name.chars);

    look_axis.uniform.location = glGetUniformLocation(shader_program->id, look_axis.uniform.name.chars);
    abort_on_GL_error("Could not get look axis uniform location");

    if (look_axis.uniform.location == -1)
        print_warning_message("Uniform \"%s\" not used in shader program.", look_axis.uniform.name.chars);

    sync_transformation(shader_program);
}

void set_view_distance(float view_distance)
{
    set_transform_translation(&transformation.view_matrix, 0, 0, -view_distance);
    transformation.uniform.needs_update = 1;
}

void apply_model_scaling(float scale)
{
    assert(scale > 0);
    apply_scaling(&transformation.model_matrix, scale, scale, scale);
    transformation.uniform.needs_update = 1;
}

void apply_model_translation(float dx, float dy, float dz)
{
    apply_translation(&transformation.model_matrix, dx, dy, dz);
    transformation.uniform.needs_update = 1;
}

void apply_view_rotation_about_axis(const Vector3f* axis, float angle)
{
    assert(axis);
    apply_rotation_about_axis(&transformation.view_matrix, axis, angle);
    transformation.uniform.needs_update = 1;
    look_axis.uniform.needs_update = 1;
}

void apply_origin_centered_view_rotation_about_axis(const Vector3f* axis, float angle)
{
    assert(axis);

    Vector3f view_translation;
    get_matrix4f_w_basis_vector(&transformation.view_matrix, &view_translation);

    set_transform_translation(&transformation.view_matrix, 0, 0, 0);
    apply_rotation_about_axis(&transformation.view_matrix, axis, angle);
    set_transform_translation(&transformation.view_matrix, view_translation.a[0], view_translation.a[1], view_translation.a[2]);

    transformation.uniform.needs_update = 1;
    look_axis.uniform.needs_update = 1;
}

void update_camera_properties(float vertical_field_of_view,
                              float aspect_ratio,
                              float near_plane_distance,
                              float far_plane_distance)
{
    transformation.projection.vertical_field_of_view = vertical_field_of_view;
    transformation.projection.aspect_ratio = aspect_ratio;
    transformation.projection.near_plane_distance = near_plane_distance;
    transformation.projection.far_plane_distance = far_plane_distance;
    update_perspective_projection_matrix();
}

void update_camera_vertical_field_of_view(float vertical_field_of_view)
{
    transformation.projection.vertical_field_of_view = vertical_field_of_view;
    update_perspective_projection_matrix();
}

void update_camera_aspect_ratio(float aspect_ratio)
{
    transformation.projection.aspect_ratio = aspect_ratio;
    update_perspective_projection_matrix();
}

void update_camera_clip_plane_distances(float near_plane_distance, float far_plane_distance)
{
    transformation.projection.near_plane_distance = near_plane_distance;
    transformation.projection.far_plane_distance = far_plane_distance;
    update_perspective_projection_matrix();
}

void sync_transformation(const ShaderProgram* shader_program)
{
    assert(shader_program);

    if (transformation.uniform.needs_update && transformation.uniform.location != -1)
    {
        const Matrix4f model_view_projection_matrix = get_model_view_projection_transform_matrix();

        glUseProgram(shader_program->id);
        abort_on_GL_error("Could not use shader program for updating transform matrix uniform");
        glUniformMatrix4fv(transformation.uniform.location, 1, GL_TRUE, model_view_projection_matrix.a);
        abort_on_GL_error("Could not update transform matrix uniform");
        glUseProgram(0);

        transformation.uniform.needs_update = 0;
    }

    if (look_axis.uniform.needs_update && look_axis.uniform.location != -1)
    {
        Matrix4f inverse_view_matrix = transformation.view_matrix;
        //invert_matrix4f(&inverse_view_matrix);
        invert_matrix4f_3x3_submatrix(&inverse_view_matrix);
        get_matrix4f_z_basis_vector(&inverse_view_matrix, &look_axis.axis);
        normalize_vector3f(&look_axis.axis);

        glUseProgram(shader_program->id);
        abort_on_GL_error("Could not use shader program for updating look axis uniform");
        glUniform3f(look_axis.uniform.location, look_axis.axis.a[0], look_axis.axis.a[1], look_axis.axis.a[2]);
        abort_on_GL_error("Could not update look axis uniform");
        glUseProgram(0);

        look_axis.uniform.needs_update = 0;
    }
}

void cleanup_transformation(void)
{
    clear_string(&transformation.uniform.name);
    clear_string(&look_axis.uniform.name);
}

const char* get_transformation_name(void)
{
    return transformation.uniform.name.chars;
}

const char* get_look_axis_name(void)
{
    return look_axis.uniform.name.chars;
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

Matrix4f get_modelview_transform_matrix(void)
{
    return matmul4f(&transformation.view_matrix, &transformation.model_matrix);
}

Matrix4f get_model_view_projection_transform_matrix(void)
{
    const Matrix4f modelview_matrix = get_modelview_transform_matrix();
    return matmul4f(&transformation.projection.matrix, &modelview_matrix);
}

Vector3f get_look_axis(void)
{
    return look_axis.axis;
}

static void update_perspective_projection_matrix(void)
{
    transformation.projection.matrix = create_perspective_transform(transformation.projection.vertical_field_of_view,
                                                                    transformation.projection.aspect_ratio,
                                                                    transformation.projection.near_plane_distance,
                                                                    transformation.projection.far_plane_distance);
    transformation.uniform.needs_update = 1;
}
