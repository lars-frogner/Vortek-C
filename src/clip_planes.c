#include "clip_planes.h"

#include "error.h"
#include "extra_math.h"
#include "linked_list.h"
#include "trackball.h"
#include "shader_generator.h"
#include "view_aligned_planes.h"

#include <math.h>


#define MAX_CLIP_PLANES 6


enum controller_state {NO_CONTROL, CONTROL};
enum plane_controllability {SHIFT_CONTROL, FULL_CONTROL};

typedef struct ClipPlane
{
    Vector3f normal;
    float origin_shift;
    unsigned int axis_aligned_box_front_corner;
    enum clip_plane_state state;
    enum plane_controllability controllability;
    Uniform normal_uniform;
    Uniform origin_shift_uniform;
} ClipPlane;

typedef struct ClipPlaneController
{
    Trackball trackball;
    float origin_shift_rate_modifier;
    float max_abs_origin_shift;
    unsigned int controllable_clip_plane_idx;
    enum controller_state state;
} ClipPlaneController;


static void generate_shader_code_for_clip_planes(void);
static void sync_clip_plane(unsigned int idx);


static ClipPlane clip_planes[MAX_CLIP_PLANES];

static Vector3f disabled_normal;
static float disabled_origin_shift;

static ClipPlaneController controller;

static ShaderProgram* active_shader_program = NULL;


void set_active_shader_program_for_clip_planes(ShaderProgram* shader_program)
{
    active_shader_program = shader_program;
}

void initialize_clip_planes(void)
{
    for (unsigned int idx = 0; idx < MAX_CLIP_PLANES; idx++)
    {
        glEnable(GL_CLIP_DISTANCE0 + idx);
        clip_planes[idx].state = CLIP_PLANE_DISABLED;
        clip_planes[idx].controllability = (idx < 3) ? SHIFT_CONTROL : FULL_CONTROL;
        initialize_uniform(&clip_planes[idx].normal_uniform, "clip_plane_normals[%d]", idx);
        initialize_uniform(&clip_planes[idx].origin_shift_uniform, "clip_plane_origin_shifts[%d]", idx);
    }

    set_vector3f_elements(&disabled_normal, 0, 0, 0);
    disabled_origin_shift = -1.0f;

    initialize_trackball(&controller.trackball);
    controller.origin_shift_rate_modifier = 5e-3f;
    controller.max_abs_origin_shift = sqrtf(3.0f);
    controller.controllable_clip_plane_idx = 0;
    controller.state = NO_CONTROL;

    generate_shader_code_for_clip_planes();
}

void load_clip_planes(void)
{
    check(active_shader_program);

    for (unsigned int idx = 0; idx < MAX_CLIP_PLANES; idx++)
    {
        load_uniform(active_shader_program, &clip_planes[idx].normal_uniform);
        load_uniform(active_shader_program, &clip_planes[idx].origin_shift_uniform);
        reset_clip_plane(idx);
    }
}

void set_clip_plane_state(unsigned int idx, enum clip_plane_state state)
{
    check(idx < MAX_CLIP_PLANES);

    const enum clip_plane_state previous_state = clip_planes[idx].state;
    clip_planes[idx].state = state;

    if (controller.controllable_clip_plane_idx == idx && state == CLIP_PLANE_DISABLED)
    {
        controller.controllable_clip_plane_idx = 0;
        disable_clip_plane_control();
    }

    if (state != previous_state)
        sync_clip_plane(idx);
}

void toggle_clip_plane_enabled_state(unsigned int idx)
{
    check(idx < MAX_CLIP_PLANES);

    if (clip_planes[idx].state == CLIP_PLANE_DISABLED)
    {
        clip_planes[idx].state = CLIP_PLANE_ENABLED;
    }
    else
    {
        clip_planes[idx].state = CLIP_PLANE_DISABLED;

        if (controller.controllable_clip_plane_idx == idx)
        {
            controller.controllable_clip_plane_idx = 0;
            disable_clip_plane_control();
        }
    }

    sync_clip_plane(idx);
}

void set_controllable_clip_plane(unsigned int idx)
{
    check(idx < MAX_CLIP_PLANES);
    if (clip_planes[idx].state == CLIP_PLANE_ENABLED)
        controller.controllable_clip_plane_idx = idx;
}

void enable_clip_plane_control(void)
{
    controller.state = CONTROL;
}

void disable_clip_plane_control(void)
{
    controller.state = NO_CONTROL;
}

void clip_plane_control_drag_start_callback(double screen_coord_x, double screen_coord_y, int screen_width, int screen_height)
{
    if (controller.state == CONTROL &&
        clip_planes[controller.controllable_clip_plane_idx].state != CLIP_PLANE_DISABLED &&
        clip_planes[controller.controllable_clip_plane_idx].controllability == FULL_CONTROL)
    {
        activate_trackball(&controller.trackball, screen_coord_x, screen_coord_y, screen_width, screen_height);
    }
}

void clip_plane_control_drag_callback(double screen_coord_x, double screen_coord_y, int screen_width, int screen_height)
{
    if (controller.state == CONTROL &&
        clip_planes[controller.controllable_clip_plane_idx].state != CLIP_PLANE_DISABLED &&
        clip_planes[controller.controllable_clip_plane_idx].controllability == FULL_CONTROL)
    {
        drag_trackball(&controller.trackball, screen_coord_x, screen_coord_y, screen_width, screen_height);

        rotate_vector3f_about_axis(&clip_planes[controller.controllable_clip_plane_idx].normal,
                                   &controller.trackball.current_rotation_axis,
                                   controller.trackball.current_rotation_angle);
        normalize_vector3f(&clip_planes[controller.controllable_clip_plane_idx].normal);

        sync_clip_plane(controller.controllable_clip_plane_idx);
    }
}

void clip_plane_control_scroll_callback(double scroll_rate)
{
    if (controller.state == CONTROL &&
        clip_planes[controller.controllable_clip_plane_idx].state != CLIP_PLANE_DISABLED)
    {
        const float new_origin_shift = clip_planes[controller.controllable_clip_plane_idx].origin_shift + controller.origin_shift_rate_modifier*(float)scroll_rate;
        clip_planes[controller.controllable_clip_plane_idx].origin_shift = clamp(new_origin_shift, -controller.max_abs_origin_shift, controller.max_abs_origin_shift);
        sync_clip_plane(controller.controllable_clip_plane_idx);
    }
}

void clip_plane_control_flip_callback(void)
{
    if (controller.state == CONTROL &&
        clip_planes[controller.controllable_clip_plane_idx].state != CLIP_PLANE_DISABLED)
    {
        invert_vector3f(&clip_planes[controller.controllable_clip_plane_idx].normal);
        clip_planes[controller.controllable_clip_plane_idx].origin_shift = -clip_planes[controller.controllable_clip_plane_idx].origin_shift;
        sync_clip_plane(controller.controllable_clip_plane_idx);
    }
}

void reset_clip_plane(unsigned int idx)
{
    check(idx < MAX_CLIP_PLANES);

    set_vector3f_elements(&clip_planes[idx].normal, 0, 0, 0);
    clip_planes[idx].normal.a[idx % 3] = (idx % 3 == 1) ? 1.0f : -1.0f;

    clip_planes[idx].origin_shift = 0;

    sync_clip_plane(idx);
}

int axis_aligned_box_in_clipped_region(const Vector3f* offset, const Vector3f* extent)
{
    assert(offset);
    assert(extent);

    const Vector3f* corners = get_unit_axis_aligned_box_corners();

    for (unsigned int idx = 0; idx < MAX_CLIP_PLANES; idx++)
    {
        if (clip_planes[idx].state != CLIP_PLANE_DISABLED)
        {
            const Vector3f unoffset_front_corner = multiply_vector3f(extent, corners + clip_planes[idx].axis_aligned_box_front_corner);
            const Vector3f front_corner = add_vector3f(offset, &unoffset_front_corner);

            if (dot3f(&front_corner, &clip_planes[idx].normal) < clip_planes[idx].origin_shift)
                return 1;
        }
    }

    return 0;
}

void cleanup_clip_planes(void)
{
    for (unsigned int idx = 0; idx < MAX_CLIP_PLANES; idx++)
    {
        destroy_uniform(&clip_planes[idx].normal_uniform);
        destroy_uniform(&clip_planes[idx].origin_shift_uniform);
    }
}

static void generate_shader_code_for_clip_planes(void)
{
    check(active_shader_program);

    const char* clip_plane_normals_name = "clip_plane_normals";
    const char* clip_plane_origin_shifts_name = "clip_plane_origin_shifts";

    const size_t position_variable_number = get_vertex_position_variable_number();

    add_clip_distance_output_in_shader(&active_shader_program->vertex_shader_source, MAX_CLIP_PLANES);

    add_array_uniform_in_shader(&active_shader_program->vertex_shader_source, "vec3", clip_plane_normals_name, MAX_CLIP_PLANES);
    add_array_uniform_in_shader(&active_shader_program->vertex_shader_source, "float", clip_plane_origin_shifts_name, MAX_CLIP_PLANES);

    DynamicString clip_plane_code = create_string(
    "\n    uint clip_plane_idx;"
    "\n    for (clip_plane_idx = 0; clip_plane_idx < %d; clip_plane_idx++)"
    "\n    {"
    "\n        gl_ClipDistance[clip_plane_idx] = dot(variable_%d.xyz, %s[clip_plane_idx]) - %s[clip_plane_idx];"
    "\n    }",
    MAX_CLIP_PLANES,
    position_variable_number, clip_plane_normals_name, clip_plane_origin_shifts_name);

    LinkedList global_dependencies = create_list();
    append_string_to_list(&global_dependencies, "gl_PerVertex");
    append_string_to_list(&global_dependencies, clip_plane_normals_name);
    append_string_to_list(&global_dependencies, clip_plane_origin_shifts_name);

    LinkedList variable_dependencies = create_list();
    append_size_t_to_list(&variable_dependencies, position_variable_number);

    add_output_snippet_in_shader(&active_shader_program->vertex_shader_source, clip_plane_code.chars, &global_dependencies, &variable_dependencies);

    clear_list(&variable_dependencies);
    clear_list(&global_dependencies);
    clear_string(&clip_plane_code);
}

static void sync_clip_plane(unsigned int idx)
{
    check(active_shader_program);
    assert(idx < MAX_CLIP_PLANES);

    glUseProgram(active_shader_program->id);
    abort_on_GL_error("Could not use shader program for updating clip plane uniforms");

    if (clip_planes[idx].state == CLIP_PLANE_ENABLED)
    {
        clip_planes[idx].axis_aligned_box_front_corner = get_axis_aligned_box_front_corner_for_plane(&clip_planes[idx].normal);

        glUniform3fv(clip_planes[idx].normal_uniform.location, 1, (const GLfloat*)(&clip_planes[idx].normal));
        abort_on_GL_error("Could not update clip plane normal uniform");

        glUniform1fv(clip_planes[idx].origin_shift_uniform.location, 1, (const GLfloat*)(&clip_planes[idx].origin_shift));
        abort_on_GL_error("Could not update clip plane origin distance uniform");
    }
    else
    {
        glUniform3fv(clip_planes[idx].normal_uniform.location, 1, (const GLfloat*)(&disabled_normal));
        abort_on_GL_error("Could not update clip plane normal uniform");

        glUniform1fv(clip_planes[idx].origin_shift_uniform.location, 1, (const GLfloat*)(&disabled_origin_shift));
        abort_on_GL_error("Could not update clip plane origin distance uniform");
    }

    glUseProgram(0);
}
