#include "renderer.h"

#include "gl_includes.h"
#include "error.h"
#include "fields.h"
#include "shaders.h"
#include "transformation.h"
#include "texture.h"
#include "field_textures.h"
#include "transfer_functions.h"
#include "axis_aligned_planes.h"


typedef struct WindowShape
{
    int width;
    int height;
} WindowShape;


static void initialize_rendering_settings(void);


static WindowShape window_shape;

static ShaderProgram shader_program;



void initialize_renderer(void)
{
    print_info_message("OpenGL Version: %s", glGetString(GL_VERSION));

    glGetError();

    Field field = read_bifrost_field("/Users/larsfrog/Code/output_visualization/no_ebeam/en024031_emer3.0sml_orig_631_tg_lowres.raw",
                                     "/Users/larsfrog/Code/output_visualization/no_ebeam/en024031_emer3.0sml_orig_631_tg_lowres.dat");

    const unsigned int texture_id = add_scalar_field_texture(&field);

    const unsigned int TF_id = add_transfer_function();
    //add_piecewise_linear_transfer_function_node(TF_id, TF_ALPHA, 0, 0);
    //add_piecewise_linear_transfer_function_node(TF_id, TF_ALPHA, 1, 1);
    const float low = 0.0f;
    const float high = 0.2f;
    set_logarithmic_transfer_function(TF_id, TF_RED, low, high, 0, 1);
    set_logarithmic_transfer_function(TF_id, TF_GREEN, low, high, 0, 1);
    set_logarithmic_transfer_function(TF_id, TF_BLUE, low, high, 0, 1);
    set_logarithmic_transfer_function(TF_id, TF_ALPHA, low, high, 0, 1);

    create_shader_program(&shader_program);

    create_planes(field.size_x, field.size_y, field.size_z, field.extent_x, field.extent_y, field.extent_z, 0.5f);

    create_transform_matrices(2.0f, 60.0f, (float)window_shape.width/window_shape.height, 0.01f, 100.0f);

    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

    initialize_rendering_settings();
    load_planes();
    load_textures(&shader_program);
    load_transform_matrices(&shader_program);

    print_transfer_function(TF_id, TF_ALPHA);
}

void sync_renderer(void)
{
    sync_transform_matrices(&shader_program);
    sync_planes();
}

void cleanup_renderer(void)
{
    cleanup_planes();
    cleanup_field_textures();
    cleanup_transfer_functions();
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

    glClear(GL_COLOR_BUFFER_BIT);

    glUseProgram(shader_program.id);
    abort_on_GL_error("Could not use shader program");

    draw_planes();

    glUseProgram(0);
}

void renderer_resize_callback(int width, int height)
{
    update_renderer_window_size_in_pixels(width, height);
    update_camera_aspect_ratio((float)width/height);
    sync_transform_matrices(&shader_program);
}

void renderer_key_action_callback(void) {}

static void initialize_rendering_settings(void)
{
    glDisable(GL_DEPTH_TEST);
    abort_on_GL_error("Could not disable depth testing");

    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);
    abort_on_GL_error("Could not set face culling options");

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    abort_on_GL_error("Could not set blending options");
}
