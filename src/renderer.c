#include "renderer.h"

#include "gl_includes.h"
#include "error.h"
#include "fields.h"
#include "shaders.h"
#include "transformation.h"
#include "texture.h"
#include "field_textures.h"
#include "transfer_functions.h"
#include "view_aligned_planes.h"
#include "shader_generator.h"


typedef struct WindowShape
{
    int width;
    int height;
} WindowShape;

typedef struct SingleFieldRenderingState
{
    const char* texture_name;
    const char* TF_name;
} SingleFieldRenderingState;


static void initialize_rendering_settings(void);
static void pre_initialize_single_field_rendering(SingleFieldRenderingState* state);
static void post_initialize_single_field_rendering(SingleFieldRenderingState* state);


static WindowShape window_shape;

static ShaderProgram shader_program;

static SingleFieldRenderingState single_field_rendering_state;


void initialize_renderer(void)
{
    print_info_message("OpenGL Version: %s", glGetString(GL_VERSION));

    glGetError();

    initialize_shader_program(&shader_program);

    set_active_shader_program_for_transformation(&shader_program);
    set_active_shader_program_for_planes(&shader_program);
    set_active_shader_program_for_textures(&shader_program);
    set_active_shader_program_for_field_textures(&shader_program);
    set_active_shader_program_for_transfer_functions(&shader_program);

    initialize_rendering_settings();
    initialize_fields();
    initialize_transformation();
    initialize_planes();
    initialize_textures();
    initialize_field_textures();
    initialize_transfer_functions();

    pre_initialize_single_field_rendering(&single_field_rendering_state);

    compile_shader_program(&shader_program);

    load_transformation();
    load_planes();
    load_textures();
    load_transfer_functions();

    post_initialize_single_field_rendering(&single_field_rendering_state);

    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
}

void cleanup_renderer(void)
{
    cleanup_transfer_functions();
    cleanup_field_textures();
    cleanup_textures();
    cleanup_planes();
    cleanup_transformation();
    cleanup_fields();
    destroy_shader_program(&shader_program);
}

void update_renderer_window_size_in_pixels(int width, int height)
{
    assert(width > 0);
    assert(height > 0);

    window_shape.width = width;
    window_shape.height = height;

    glViewport(0, 0, width, height);
}

void renderer_update_callback(void)
{
    glClear(GL_COLOR_BUFFER_BIT);

    draw_active_bricked_field();
}

void renderer_resize_callback(int width, int height)
{
    update_renderer_window_size_in_pixels(width, height);
    update_camera_aspect_ratio((float)width/height);
}

void renderer_key_action_callback(void)
{}

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
}

static void pre_initialize_single_field_rendering(SingleFieldRenderingState* state)
{
    check(state);

    Field* const field = create_field_from_bifrost_file("temperature_field",
                                                        "/Users/larsfrog/Code/output_visualization/no_ebeam/en024031_emer3.0sml_orig_631_tg_hires.raw",
                                                        "/Users/larsfrog/Code/output_visualization/no_ebeam/en024031_emer3.0sml_orig_631_tg_hires.dat");

    //Field field = read_bifrost_field("/Users/larsfrog/Code/output_visualization/ebeam/en024031_emer3.0sml_ebeam_631_qbeam_hires.raw",
    //                                 "/Users/larsfrog/Code/output_visualization/ebeam/en024031_emer3.0sml_ebeam_631_qbeam_hires.dat");

    set_min_sub_brick_size(8);

    state->texture_name = create_scalar_field_texture(field, 6, 2);
    state->TF_name = create_transfer_function();

    const size_t field_texture_variable_number = apply_scalar_field_texture_sampling_in_shader(&shader_program.fragment_shader_source, state->texture_name, "out_tex_coord");
    const size_t mapped_field_texture_variable_number = apply_transfer_function_in_shader(&shader_program.fragment_shader_source, state->TF_name, field_texture_variable_number);
    assign_variable_to_new_output_in_shader(&shader_program.fragment_shader_source, "vec4", mapped_field_texture_variable_number, "out_color");
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

    update_camera_properties(60.0f, (float)window_shape.width/window_shape.height, 0.01f, 100.0f, PERSPECTIVE_PROJECTION);
    //update_camera_properties(2.0f, (float)window_shape.width/window_shape.height, 0.01f, 100.0f, ORTHOGRAPHIC_PROJECTION);

    set_active_bricked_field(get_texture_bricked_field(state->texture_name));
    set_plane_separation(0.5f);
}
