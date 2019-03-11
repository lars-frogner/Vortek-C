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


static void initialize_rendering_settings(void);
static void initialize_single_field_rendering(void);


static WindowShape window_shape;

static ShaderProgram shader_program;


void initialize_renderer(void)
{
    print_info_message("OpenGL Version: %s", glGetString(GL_VERSION));

    glGetError();

    initialize_rendering_settings();
    initialize_fields();
    initialize_transformation();
    initialize_planes();
    initialize_textures();
    initialize_field_textures();
    initialize_transfer_functions();
    initialize_shader_program(&shader_program);

    initialize_single_field_rendering();

    generate_shader_code_for_transformation(&shader_program);
    generate_shader_code_for_planes(&shader_program);

    compile_shader_program(&shader_program);

    load_textures(&shader_program);
    load_transformation(&shader_program);
    load_planes(&shader_program);

    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
}

void sync_renderer(void)
{
    sync_transformation(&shader_program);
    sync_planes(&shader_program);
}

void cleanup_renderer(void)
{
    cleanup_transformation();
    cleanup_planes();
    cleanup_fields();
    cleanup_transfer_functions();
    cleanup_field_textures();
    cleanup_textures();
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
    sync_transfer_functions();
    sync_planes(&shader_program);

    glClear(GL_COLOR_BUFFER_BIT);

    glUseProgram(shader_program.id);
    abort_on_GL_error("Could not use shader program");

    //draw_planes();
    draw_brick(0);

    glUseProgram(0);
}

void renderer_resize_callback(int width, int height)
{
    update_renderer_window_size_in_pixels(width, height);
    update_camera_aspect_ratio((float)width/height);
    sync_transformation(&shader_program);
}

void renderer_key_action_callback(void)
{}

static void initialize_rendering_settings(void)
{
    glDisable(GL_DEPTH_TEST);
    abort_on_GL_error("Could not disable depth testing");

    //glEnable(GL_CULL_FACE);
    //glCullFace(GL_BACK);
    //glFrontFace(GL_CCW);
    //abort_on_GL_error("Could not set face culling options");

    //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    abort_on_GL_error("Could not set blending options");
}

static void initialize_single_field_rendering(void)
{
    Field* const field = create_field_from_bifrost_file("temperature_field",
                                                        "/Users/larsfrog/Code/output_visualization/no_ebeam/en024031_emer3.0sml_orig_631_tg.raw",
                                                        "/Users/larsfrog/Code/output_visualization/no_ebeam/en024031_emer3.0sml_orig_631_tg.dat");
    field->extent_z = field->extent_x;
    //Field field = read_bifrost_field("/Users/larsfrog/Code/output_visualization/ebeam/en024031_emer3.0sml_ebeam_631_qbeam_hires.raw",
    //                                 "/Users/larsfrog/Code/output_visualization/ebeam/en024031_emer3.0sml_ebeam_631_qbeam_hires.dat");

    //clip_field_values(&field, 0.0f, field.max_value*0.04f);

    const char* texture_name = create_scalar_field_texture(field, &shader_program);
    const char* TF_name = create_transfer_function(&shader_program);

    /*const float low = 0.0f;//field_value_to_normalized_value(&field, 0);
    const float high = 0.2f;

    add_piecewise_linear_transfer_function_node(TF_name, TF_RED, low, 0);
    add_piecewise_linear_transfer_function_node(TF_name, TF_GREEN, low, 0);
    add_piecewise_linear_transfer_function_node(TF_name, TF_BLUE, low, 0);

    add_piecewise_linear_transfer_function_node(TF_name, TF_ALPHA, 0, 0);
    add_piecewise_linear_transfer_function_node(TF_name, TF_ALPHA, low, 0);
    add_piecewise_linear_transfer_function_node(TF_name, TF_ALPHA, high, 1);*/

    set_logarithmic_transfer_function(TF_name, TF_RED,   0, 1, 0, 1);
    set_logarithmic_transfer_function(TF_name, TF_GREEN, 0, 1, 0, 1);
    set_logarithmic_transfer_function(TF_name, TF_BLUE,  0, 1, 0, 1);
    set_logarithmic_transfer_function(TF_name, TF_ALPHA, 0, 1, 0, 1);

    const size_t field_texture_variable_number = apply_scalar_field_texture_sampling_in_shader(&shader_program.fragment_shader_source, texture_name, "out_tex_coord");
    const size_t mapped_field_texture_variable_number = apply_transfer_function_in_shader(&shader_program.fragment_shader_source, TF_name, field_texture_variable_number);
    assign_variable_to_new_output_in_shader(&shader_program.fragment_shader_source, "vec4", mapped_field_texture_variable_number, "out_color");

    set_view_distance(2.0f);

    update_camera_properties(60.0f, (float)window_shape.width/window_shape.height, 0.01f, 100.0f);

    set_active_bricked_field(get_bricked_field_texture(texture_name));
    set_plane_separation(1.0f);
}
