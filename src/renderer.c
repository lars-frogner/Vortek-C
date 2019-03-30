#include "renderer.h"

#include "gl_includes.h"
#include "error.h"
#include "geometry.h"
#include "colors.h"
#include "fields.h"
#include "trackball.h"
#include "transformation.h"
#include "indicators.h"
#include "texture.h"
#include "field_textures.h"
#include "transfer_functions.h"
#include "view_aligned_planes.h"
#include "clip_planes.h"
#include "shader_generator.h"
#include "shaders.h"
#include "window.h"


typedef struct SingleFieldRenderingState
{
    const char* texture_name;
    const char* TF_name;
} SingleFieldRenderingState;


static void initialize_rendering_settings(void);
static void pre_initialize_single_field_rendering(SingleFieldRenderingState* state);
static void post_initialize_single_field_rendering(SingleFieldRenderingState* state);


static ShaderProgram rendering_shader_program;
static ShaderProgram indicator_shader_program;

static SingleFieldRenderingState single_field_rendering_state;


void initialize_renderer(void)
{
    print_info_message("OpenGL Version: %s", glGetString(GL_VERSION));

    glGetError();

    initialize_shader_program(&rendering_shader_program);
    initialize_shader_program(&indicator_shader_program);

    add_active_shader_program_for_transformation(&rendering_shader_program);
    add_active_shader_program_for_transformation(&indicator_shader_program);
    set_active_shader_program_for_planes(&rendering_shader_program);
    set_active_shader_program_for_clip_planes(&rendering_shader_program);
    set_active_shader_program_for_textures(&rendering_shader_program);
    set_active_shader_program_for_field_textures(&rendering_shader_program);
    set_active_shader_program_for_transfer_functions(&rendering_shader_program);
    set_active_shader_program_for_indicators(&indicator_shader_program);

    initialize_rendering_settings();
    initialize_fields();
    initialize_trackball();
    initialize_transformation();
    initialize_planes();
    initialize_clip_planes();
    initialize_textures();
    initialize_field_textures();
    initialize_transfer_functions();
    initialize_indicators();

    pre_initialize_single_field_rendering(&single_field_rendering_state);

    compile_shader_program(&rendering_shader_program);
    compile_shader_program(&indicator_shader_program);

    load_transformation();
    load_planes();
    load_clip_planes();
    load_textures();
    load_transfer_functions();

    post_initialize_single_field_rendering(&single_field_rendering_state);

    int width, height;
    get_window_shape_in_pixels(&width, &height);
    glViewport(0, 0, width, height);

    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
}

void cleanup_renderer(void)
{
    cleanup_indicators();
    cleanup_transfer_functions();
    cleanup_field_textures();
    cleanup_textures();
    cleanup_clip_planes();
    cleanup_planes();
    cleanup_transformation();
    cleanup_fields();
    destroy_shader_program(&indicator_shader_program);
    destroy_shader_program(&rendering_shader_program);
}

void renderer_update_callback(void)
{
    glClear(GL_COLOR_BUFFER_BIT);

    draw_active_bricked_field();
    draw_clip_planes();
}

void renderer_resize_callback(int width, int height)
{
    update_camera_aspect_ratio((float)width/height);
    glViewport(0, 0, width, height);
}

static void initialize_rendering_settings(void)
{
    glDisable(GL_DEPTH_TEST);
    abort_on_GL_error("Could not disable depth testing");

    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);
    abort_on_GL_error("Could not set face culling options");

    //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    abort_on_GL_error("Could not set blending options");

    glPointSize(5.0f);
}

static void pre_initialize_single_field_rendering(SingleFieldRenderingState* state)
{
    check(state);

    Field* const field = create_field_from_bifrost_file("temperature_field",
                                                        "/Users/larsfrog/Code/output_visualization/no_ebeam/en024031_emer3.0sml_orig_631_tg.raw",
                                                        "/Users/larsfrog/Code/output_visualization/no_ebeam/en024031_emer3.0sml_orig_631_tg.dat");

    //Field field = read_bifrost_field("/Users/larsfrog/Code/output_visualization/ebeam/en024031_emer3.0sml_ebeam_631_qbeam_hires.raw",
    //                                 "/Users/larsfrog/Code/output_visualization/ebeam/en024031_emer3.0sml_ebeam_631_qbeam_hires.dat");

    set_max_clip_plane_origin_shifts(field->halfwidth, field->halfheight, field->halfdepth);

    set_min_sub_brick_size(6);

    state->texture_name = create_scalar_field_texture(field, 6, 2);
    state->TF_name = create_transfer_function();

    set_active_bricked_field(get_texture_bricked_field(state->texture_name));

    const Color field_boundary_color = create_standard_color(COLOR_WHITE, 0.15f);
    const Color brick_boundary_color = create_standard_color(COLOR_YELLOW, 0.15f);
    const Color sub_brick_boundary_color = create_standard_color(COLOR_CYAN, 0.15f);
    add_boundary_indicator_for_field(state->texture_name, &field_boundary_color);
    add_boundary_indicator_for_bricks(state->texture_name, &brick_boundary_color);
    add_boundary_indicator_for_sub_bricks(state->texture_name, &sub_brick_boundary_color);

    const size_t field_texture_variable_number = apply_scalar_field_texture_sampling_in_shader(&rendering_shader_program.fragment_shader_source, state->texture_name, "out_tex_coord");
    const size_t mapped_field_texture_variable_number = apply_transfer_function_in_shader(&rendering_shader_program.fragment_shader_source, state->TF_name, field_texture_variable_number);
    assign_variable_to_new_output_in_shader(&rendering_shader_program.fragment_shader_source, "vec4", mapped_field_texture_variable_number, "out_color");
}

static void post_initialize_single_field_rendering(SingleFieldRenderingState* state)
{
    check(state);

    //set_transfer_function_upper_limit(state->TF_name, field_value_to_texture_value(state->texture_name, 5000.0f));
    //set_transfer_function_upper_node(state->TF_name, TF_ALPHA, 0);

    set_transfer_function_lower_limit(state->TF_name, field_value_to_texture_value(state->texture_name, 30000.0f));
    set_transfer_function_lower_node(state->TF_name, TF_ALPHA, 0);

    set_logarithmic_transfer_function(state->TF_name, TF_RED,   0, 1);
    set_logarithmic_transfer_function(state->TF_name, TF_GREEN, 0, 1);
    set_logarithmic_transfer_function(state->TF_name, TF_BLUE,  0, 1);
    set_logarithmic_transfer_function(state->TF_name, TF_ALPHA, 0, 1);
    //set_piecewise_linear_transfer_function_node(state->TF_name, TF_ALPHA, TF_START_NODE, 0);

    update_visibility_ratios(state->TF_name, get_texture_bricked_field(state->texture_name));

    //print_transfer_function(state->TF_name, TF_ALPHA);

    set_view_distance(2.0f);

    update_camera_properties(60.0f, get_window_aspect_ratio(), 0.01f, 100.0f, PERSPECTIVE_PROJECTION);
    //update_camera_properties(2.0f, (float)window_shape.width/window_shape.height, 0.01f, 100.0f, ORTHOGRAPHIC_PROJECTION);

    set_plane_separation(0.5f);
}
