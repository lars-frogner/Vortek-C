#include "clip_planes.h"

#include "error.h"
#include "extra_math.h"
#include "colors.h"
#include "linked_list.h"
#include "trackball.h"
#include "transformation.h"
#include "indicators.h"
#include "shader_generator.h"
#include "view_aligned_planes.h"

#include <math.h>


#define MAX_CLIP_PLANES 6
#define CLIP_PLANE_ALPHA 0.8f


enum controller_state {NO_CONTROL, CONTROL};
enum plane_controllability {SHIFT_CONTROL, FULL_CONTROL};

typedef struct ClipPlane
{
    Vector3f normal;
    float origin_shift;
    unsigned int axis_aligned_box_back_corner;
    unsigned int axis_aligned_box_front_corner;
    Color color;
    enum clip_plane_state state;
    enum plane_controllability controllability;
    Uniform normal_uniform;
    Uniform origin_shift_uniform;
    const char* boundary_indicator_name;
} ClipPlane;

typedef struct ClipPlaneController
{
    float origin_shift_rate_modifier;
    Vector3f max_abs_origin_shifts;
    unsigned int controllable_idx;
    enum controller_state state;
    int is_dragging;
} ClipPlaneController;


static void generate_shader_code_for_clip_planes(void);
static void sync_clip_plane(unsigned int idx);

static void create_boundary_indicator(unsigned int idx);
static void create_normal_indicator(void);

static void update_boundary_indicator(unsigned int idx);
static void update_normal_indicator(void);

static void draw_boundary_indicator(unsigned int idx);
static void draw_normal_indicator(void);


// Corner positions of a (twice) unit axis aligned cube centered on the origin
static const Vector3f centered_corners[8] = {{{-1, -1, -1}},
                                             {{ 1, -1, -1}},
                                             {{-1,  1, -1}},
                                             {{-1, -1,  1}},
                                             {{ 1, -1,  1}},
                                             {{ 1,  1, -1}},
                                             {{-1,  1,  1}},
                                             {{ 1,  1,  1}}};

static ClipPlane clip_planes[MAX_CLIP_PLANES];

static Vector3f disabled_normal;
static float disabled_origin_shift;

static const char* normal_indicator_name = NULL;
static const int clip_plane_hex_colors[MAX_CLIP_PLANES] =
    {0xE8042E,
     0x19C659,
     0x2695EF,
     0x2AAAA2,
     0xC11F7D,
     0xE36414};

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
        clip_planes[idx].controllability = FULL_CONTROL;
        initialize_uniform(&clip_planes[idx].normal_uniform, "clip_plane_normals[%d]", idx);
        initialize_uniform(&clip_planes[idx].origin_shift_uniform, "clip_plane_origin_shifts[%d]", idx);
    }

    set_vector3f_elements(&disabled_normal, 0, 0, 0);
    disabled_origin_shift = -1.0f;

    controller.origin_shift_rate_modifier = 5e-3f;
    set_vector3f_elements(&controller.max_abs_origin_shifts, 1.0f, 1.0f, 1.0f);
    controller.controllable_idx = 0;
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

        create_boundary_indicator(idx);
    }

    create_normal_indicator();
}

void set_max_clip_plane_origin_shifts(float max_x, float max_y, float max_z)
{
    set_vector3f_elements(&controller.max_abs_origin_shifts, max_x, max_y, max_z);
}

void set_clip_plane_state(unsigned int idx, enum clip_plane_state state)
{
    check(idx < MAX_CLIP_PLANES);

    const enum clip_plane_state previous_state = clip_planes[idx].state;
    clip_planes[idx].state = state;

    if (controller.controllable_idx == idx && state == CLIP_PLANE_DISABLED)
    {
        controller.controllable_idx = 0;
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

        if (controller.controllable_idx == idx)
        {
            controller.controllable_idx = 0;
            disable_clip_plane_control();
        }
    }

    sync_clip_plane(idx);
}

void set_controllable_clip_plane(unsigned int idx)
{
    check(idx < MAX_CLIP_PLANES);
    if (clip_planes[idx].state == CLIP_PLANE_ENABLED)
    {
        controller.controllable_idx = idx;
        update_normal_indicator();
    }
}

void enable_clip_plane_control(void)
{
    controller.state = CONTROL;
}

void disable_clip_plane_control(void)
{
    controller.state = NO_CONTROL;
}

void clip_plane_control_drag_start_callback(double screen_coord_x, double screen_coord_y)
{
    if (controller.state == CONTROL &&
        clip_planes[controller.controllable_idx].state != CLIP_PLANE_DISABLED &&
        clip_planes[controller.controllable_idx].controllability == FULL_CONTROL)
    {
        activate_trackball_in_world_space(screen_coord_x, screen_coord_y);
        controller.is_dragging = 1;
    }
}

void clip_plane_control_drag_callback(double screen_coord_x, double screen_coord_y)
{
    if (controller.state == CONTROL &&
        clip_planes[controller.controllable_idx].state != CLIP_PLANE_DISABLED &&
        clip_planes[controller.controllable_idx].controllability == FULL_CONTROL &&
        controller.is_dragging)
    {
        drag_trackball_in_world_space(screen_coord_x, screen_coord_y);

        rotate_vector3f_about_axis(&clip_planes[controller.controllable_idx].normal,
                                   get_current_trackball_rotation_axis(),
                                   get_current_trackball_rotation_angle());
        normalize_vector3f(&clip_planes[controller.controllable_idx].normal);

        sync_clip_plane(controller.controllable_idx);
    }
}

void clip_plane_control_drag_end_callback(void)
{
    if (controller.is_dragging)
        controller.is_dragging = 0;
}

void clip_plane_control_scroll_callback(double scroll_rate)
{
    if (controller.state == CONTROL &&
        clip_planes[controller.controllable_idx].state != CLIP_PLANE_DISABLED)
    {
        const float new_origin_shift = clip_planes[controller.controllable_idx].origin_shift + controller.origin_shift_rate_modifier*(float)scroll_rate;

        const Vector3f max_shift_vector = multiplied_vector3f(&controller.max_abs_origin_shifts,
                                                            centered_corners + clip_planes[controller.controllable_idx].axis_aligned_box_front_corner);
        const float max_abs_origin_shift = dot3f(&max_shift_vector, &clip_planes[controller.controllable_idx].normal);

        clip_planes[controller.controllable_idx].origin_shift = clamp(new_origin_shift, -max_abs_origin_shift, max_abs_origin_shift);

        sync_clip_plane(controller.controllable_idx);
    }
}

void clip_plane_control_flip_callback(void)
{
    if (controller.state == CONTROL &&
        clip_planes[controller.controllable_idx].state != CLIP_PLANE_DISABLED)
    {
        invert_vector3f(&clip_planes[controller.controllable_idx].normal);
        clip_planes[controller.controllable_idx].origin_shift = -clip_planes[controller.controllable_idx].origin_shift;
        sync_clip_plane(controller.controllable_idx);
    }
}

void clip_plane_control_set_normal_to_x_axis_callback(void)
{
    if (controller.state == CONTROL &&
        clip_planes[controller.controllable_idx].state != CLIP_PLANE_DISABLED)
    {
        set_vector3f_elements(&clip_planes[controller.controllable_idx].normal, -1.0f, 0.0f, 0.0f);
        sync_clip_plane(controller.controllable_idx);
    }
}

void clip_plane_control_set_normal_to_y_axis_callback(void)
{
    if (controller.state == CONTROL &&
        clip_planes[controller.controllable_idx].state != CLIP_PLANE_DISABLED)
    {
        set_vector3f_elements(&clip_planes[controller.controllable_idx].normal, 0.0f, 1.0f, 0.0f);
        sync_clip_plane(controller.controllable_idx);
    }
}

void clip_plane_control_set_normal_to_z_axis_callback(void)
{
    if (controller.state == CONTROL &&
        clip_planes[controller.controllable_idx].state != CLIP_PLANE_DISABLED)
    {
        set_vector3f_elements(&clip_planes[controller.controllable_idx].normal, 0.0f, 0.0f, -1.0f);
        sync_clip_plane(controller.controllable_idx);
    }
}

void clip_plane_control_set_normal_to_look_axis_callback(void)
{
    if (controller.state == CONTROL &&
        clip_planes[controller.controllable_idx].state != CLIP_PLANE_DISABLED)
    {
        const Vector3f* const look_axis = get_camera_look_axis();
        clip_planes[controller.controllable_idx].normal = *look_axis;
        invert_vector3f(&clip_planes[controller.controllable_idx].normal);
        sync_clip_plane(controller.controllable_idx);
    }
}

void clip_plane_control_reset_origin_shift_callback(void)
{
    if (controller.state == CONTROL &&
        clip_planes[controller.controllable_idx].state != CLIP_PLANE_DISABLED)
    {
        clip_planes[controller.controllable_idx].origin_shift = 0.0f;
        sync_clip_plane(controller.controllable_idx);
    }
}

void draw_clip_planes(void)
{
    glUseProgram(get_active_indicator_shader_program_id());
    abort_on_GL_error("Could not use shader program for drawing indicator");

    for (unsigned int idx = 0; idx < MAX_CLIP_PLANES; idx++)
    {
        if (clip_planes[idx].state == CLIP_PLANE_ENABLED)
            draw_boundary_indicator(idx);
    }

    if (controller.state == CONTROL && clip_planes[controller.controllable_idx].state == CLIP_PLANE_ENABLED)
        draw_normal_indicator();

    glUseProgram(0);
}

void reset_clip_plane(unsigned int idx)
{
    check(idx < MAX_CLIP_PLANES);

    set_vector3f_elements(&clip_planes[idx].normal, 0.0f, 0.0f, 0.0f);
    clip_planes[idx].normal.a[idx % 3] = (idx % 3 == 1) ? 1.0f : -1.0f;

    clip_planes[idx].origin_shift = 0;

    clip_planes[idx].color = create_hex_color(clip_plane_hex_colors[idx], CLIP_PLANE_ALPHA);

    sync_clip_plane(idx);
}

int axis_aligned_box_in_clipped_region(const Vector3f* offset, const Vector3f* extent)
{
    assert(offset);
    assert(extent);

    const Vector3f* corners = get_unit_axis_aligned_box_corners();
    Vector3f front_corner;

    for (unsigned int idx = 0; idx < MAX_CLIP_PLANES; idx++)
    {
        if (clip_planes[idx].state != CLIP_PLANE_DISABLED)
        {
            front_corner = multiplied_vector3f(extent, corners + clip_planes[idx].axis_aligned_box_front_corner);
            add_vector3f(offset, &front_corner);

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
        destroy_indicator(clip_planes[idx].boundary_indicator_name);
    }

    destroy_indicator(normal_indicator_name);

    active_shader_program = NULL;
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
        clip_planes[idx].axis_aligned_box_back_corner = get_axis_aligned_box_back_corner_for_plane(&clip_planes[idx].normal);
        clip_planes[idx].axis_aligned_box_front_corner = get_axis_aligned_box_front_corner_for_plane(&clip_planes[idx].normal);

        glUniform3fv(clip_planes[idx].normal_uniform.location, 1, (const GLfloat*)(&clip_planes[idx].normal));
        abort_on_GL_error("Could not update clip plane normal uniform");

        glUniform1fv(clip_planes[idx].origin_shift_uniform.location, 1, (const GLfloat*)(&clip_planes[idx].origin_shift));
        abort_on_GL_error("Could not update clip plane origin distance uniform");

        update_boundary_indicator(idx);

        if (idx == controller.controllable_idx)
            update_normal_indicator();
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

static void create_boundary_indicator(unsigned int idx)
{
    assert(idx < MAX_CLIP_PLANES);

    const DynamicString name = create_string("clip_plane_%d_boundaries", idx);
    Indicator* const indicator = create_indicator(&name, 6, 6);

    for (unsigned int vertex_idx = 0; vertex_idx < 6; vertex_idx++)
        set_vector4f_elements(indicator->vertices.positions + vertex_idx, 0.0f, 0.0f, 0.0f, 1.0f);

    for (unsigned int index_idx = 0; index_idx < 6; index_idx++)
        indicator->index_buffer[index_idx] = index_idx;

    set_vertex_colors_for_indicator(indicator, 0, indicator->n_vertices, &clip_planes[idx].color);

    load_buffer_data_for_indicator(indicator);

    clip_planes[idx].boundary_indicator_name = indicator->name.chars;
}

static void create_normal_indicator(void)
{
    const DynamicString name = create_string("clip_plane_normal");
    Indicator* const indicator = create_indicator(&name, 2, 2);

    set_vector4f_elements(indicator->vertices.positions + 0, 0.0f, 0.0f, 0.0f, 1.0f);
    set_vector4f_elements(indicator->vertices.positions + 1, 0.0f, 0.0f, 0.0f, 1.0f);

    indicator->index_buffer[0] = 0;
    indicator->index_buffer[1] = 1;

    set_vertex_colors_for_indicator(indicator, 0, indicator->n_vertices, get_full_standard_color(COLOR_BLACK));

    load_buffer_data_for_indicator(indicator);

    normal_indicator_name = indicator->name.chars;
}

static void update_boundary_indicator(unsigned int idx)
{
    Indicator* const indicator = get_indicator(clip_planes[idx].boundary_indicator_name);

    for (unsigned int vertex_idx = 0; vertex_idx < 6; vertex_idx++)
        compute_plane_bounding_box_intersection_vertex(&clip_planes[idx].normal,
                                                       clip_planes[idx].origin_shift,
                                                       clip_planes[idx].axis_aligned_box_back_corner,
                                                       vertex_idx,
                                                       indicator->vertices.positions + vertex_idx);

    update_position_buffer_data_for_indicator(indicator);
}

static void update_normal_indicator(void)
{
    Indicator* const indicator = get_indicator(normal_indicator_name);

    Vector3f* const normal = &clip_planes[controller.controllable_idx].normal;
    const float origin_shift = clip_planes[controller.controllable_idx].origin_shift;
    Color* const color = &clip_planes[controller.controllable_idx].color;

    set_vector4f_elements(indicator->vertices.positions + 0, 0.0f, 0.0f, 0.0f, 1.0f);
    set_vector4f_elements(indicator->vertices.positions + 1, origin_shift*normal->a[0], origin_shift*normal->a[1], origin_shift*normal->a[2], 1.0f);

    set_vertex_colors_for_indicator(indicator, 0, 2, color);

    update_vertex_buffer_data_for_indicator(indicator);
}

static void draw_boundary_indicator(unsigned int idx)
{
    Indicator* const indicator = get_indicator(clip_planes[idx].boundary_indicator_name);

    assert(indicator->vertex_array_object_id > 0);
    glBindVertexArray(indicator->vertex_array_object_id);
    abort_on_GL_error("Could not bind VAO for drawing indicator");

    glDrawElements(GL_LINE_LOOP, (GLsizei)indicator->n_indices, GL_UNSIGNED_INT, (GLvoid*)0);
    abort_on_GL_error("Could not draw indicator");

    glBindVertexArray(0);
}

static void draw_normal_indicator(void)
{
    Indicator* const indicator = get_indicator(normal_indicator_name);

    assert(indicator->vertex_array_object_id > 0);
    glBindVertexArray(indicator->vertex_array_object_id);
    abort_on_GL_error("Could not bind VAO for drawing indicator");

    glDrawElements(GL_LINES, (GLsizei)indicator->n_indices, GL_UNSIGNED_INT, (GLvoid*)0);
    abort_on_GL_error("Could not draw indicator");

    glDrawElements(GL_POINTS, (GLsizei)indicator->n_indices, GL_UNSIGNED_INT, (GLvoid*)0);
    abort_on_GL_error("Could not draw indicator");

    glBindVertexArray(0);
}
