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
static void pre_initialize_single_field_rendering(void);
static void post_initialize_single_field_rendering(void);


static ShaderProgram rendering_shader_program;
static ShaderProgram indicator_shader_program;

static SingleFieldRenderingState single_field_rendering_state;

static int rendering_required;
static int has_data;


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
    initialize_bricks();
    initialize_clip_planes();
    initialize_textures();
    initialize_field_textures();
    initialize_transfer_functions();
    initialize_indicators();

    pre_initialize_single_field_rendering();

    compile_shader_program(&rendering_shader_program);
    compile_shader_program(&indicator_shader_program);

    load_transformation();
    load_planes();
    load_clip_planes();
    load_textures();
    load_transfer_functions();

    post_initialize_single_field_rendering();

    has_data = 0;

    int width, height;
    get_window_shape_in_pixels(&width, &height);
    glViewport(0, 0, width, height);

    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
}

void cleanup_renderer(void)
{
    cleanup_transfer_functions();
    cleanup_field_textures();
    cleanup_textures();
    cleanup_clip_planes();
    cleanup_planes();
    cleanup_transformation();
    cleanup_fields();
    cleanup_indicators();
    destroy_shader_program(&indicator_shader_program);
    destroy_shader_program(&rendering_shader_program);
}

int perform_rendering(void)
{
    const int was_rendered = rendering_required;

    if (rendering_required)
    {
        glClear(GL_COLOR_BUFFER_BIT);

        draw_active_bricked_field();
        draw_clip_planes();

        rendering_required = 0;
    }

    return was_rendered;
}

void renderer_resize_callback(int width, int height)
{
    update_camera_aspect_ratio((float)width/(float)height);
    glViewport(0, 0, width, height);
}

void require_rendering(void)
{
    rendering_required = 1;
}

int has_rendering_data(void)
{
    return has_data;
}

const char* get_single_field_rendering_texture_name(void)
{
    return single_field_rendering_state.texture_name;
}

const char* get_single_field_rendering_TF_name(void)
{
    return single_field_rendering_state.TF_name;
}

void set_single_field_rendering_field(const char* field_name)
{
    check(single_field_rendering_state.texture_name);

    Field* const existing_field = get_field_texture_field(single_field_rendering_state.texture_name);
    if (existing_field)
        destroy_field(existing_field->name.chars);

    Field* const field = get_field(field_name);

    set_field_texture_field(single_field_rendering_state.texture_name, field);
    set_max_clip_plane_origin_shifts(field->halfwidth, field->halfheight, field->halfdepth);
    set_active_bricked_field(get_field_texture_bricked_field(single_field_rendering_state.texture_name));
    set_plane_separation(0.5f);

    has_data = 1;
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

static void pre_initialize_single_field_rendering(void)
{
    single_field_rendering_state.texture_name = create_scalar_field_texture();
    single_field_rendering_state.TF_name = create_transfer_function();

    const size_t field_texture_variable_number = apply_scalar_field_texture_sampling_in_shader(&rendering_shader_program.fragment_shader_source,
                                                                                               single_field_rendering_state.texture_name,
                                                                                               "out_tex_coord");

    const size_t mapped_field_texture_variable_number = apply_transfer_function_in_shader(&rendering_shader_program.fragment_shader_source,
                                                                                          single_field_rendering_state.TF_name,
                                                                                          field_texture_variable_number);

    assign_variable_to_new_output_in_shader(&rendering_shader_program.fragment_shader_source,
                                            "vec4",
                                            mapped_field_texture_variable_number,
                                            "out_color");
}

static void post_initialize_single_field_rendering(void)
{
    set_view_distance(2.0f);
    update_camera_aspect_ratio(get_window_aspect_ratio());
}
