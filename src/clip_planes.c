#include "clip_planes.h"

#include "error.h"
#include "linked_list.h"
#include "shader_generator.h"
#include "view_aligned_planes.h"


#define MAX_CLIP_PLANES 3


typedef struct ClipPlanes
{
    unsigned int active_count;
    Vector3f origins[MAX_CLIP_PLANES];
    Vector3f normals[MAX_CLIP_PLANES];
    float origin_distances[MAX_CLIP_PLANES];
    unsigned int axis_aligned_box_front_corners[MAX_CLIP_PLANES];
    Uniform active_count_uniform;
    Uniform origin_distances_uniform;
    Uniform normals_uniform;
} ClipPlanes;


static void generate_shader_code_for_clip_planes(void);
static void sync_clip_planes(void);


static ClipPlanes clip_planes;

static ShaderProgram* active_shader_program = NULL;


void set_active_shader_program_for_clip_planes(ShaderProgram* shader_program)
{
    active_shader_program = shader_program;
}

void initialize_clip_planes(void)
{
    for (unsigned int plane_idx = 0; plane_idx < MAX_CLIP_PLANES; plane_idx++)
        glEnable(GL_CLIP_DISTANCE0 + plane_idx);

    clip_planes.active_count = 0;

    initialize_uniform(&clip_planes.active_count_uniform, "active_clip_plane_count");
    initialize_uniform(&clip_planes.normals_uniform, "clip_plane_normals");
    initialize_uniform(&clip_planes.origin_distances_uniform, "clip_plane_origin_distances");

    generate_shader_code_for_clip_planes();
}

void load_clip_planes(void)
{
    check(active_shader_program);

    load_uniform(active_shader_program, &clip_planes.active_count_uniform);
    load_uniform(active_shader_program, &clip_planes.normals_uniform);
    load_uniform(active_shader_program, &clip_planes.origin_distances_uniform);

    reset_clip_planes();

    clip_planes.active_count_uniform.needs_update = 1;
    sync_clip_planes();
}

void reset_clip_planes(void)
{
    unsigned int plane_idx;

    for (plane_idx = 0; plane_idx < MAX_CLIP_PLANES; plane_idx++)
        set_vector3f_elements(clip_planes.origins + plane_idx, 0, 0, 0);

    set_vector3f_elements(clip_planes.normals + 0, -1.0f, 0, 0);
    set_vector3f_elements(clip_planes.normals + 1, 0, -1.0f, 0);
    set_vector3f_elements(clip_planes.normals + 2, 0, 0, -1.0f);

    for (plane_idx = 3; plane_idx < MAX_CLIP_PLANES; plane_idx++)
        set_vector3f_elements(clip_planes.normals + plane_idx, 0, 0, 0);

    for (plane_idx = 0; plane_idx < MAX_CLIP_PLANES; plane_idx++)
        clip_planes.origin_distances[plane_idx] = dot3f(clip_planes.origins + plane_idx, clip_planes.normals + plane_idx);

    glUseProgram(active_shader_program->id);
    abort_on_GL_error("Could not use shader program for updating clip plane uniforms");

    glUniform3fv(clip_planes.normals_uniform.location, MAX_CLIP_PLANES, (const GLfloat*)clip_planes.normals);
    abort_on_GL_error("Could not update clip plane normals uniform");

    glUniform1fv(clip_planes.origin_distances_uniform.location, MAX_CLIP_PLANES, (const GLfloat*)clip_planes.origin_distances);
    abort_on_GL_error("Could not update clip plane origin distances uniform");

    clip_planes.normals_uniform.needs_update = 0;
    clip_planes.origin_distances_uniform.needs_update = 0;

    glUseProgram(0);
}

int axis_aligned_box_in_clipped_region(const Vector3f* offset, const Vector3f* extent)
{
    const Vector3f* corners = get_unit_axis_aligned_box_corners();

    unsigned int plane_idx;

    for (plane_idx = 0; plane_idx < clip_planes.active_count; plane_idx++)
    {
        const Vector3f unoffset_front_corner = multiply_vector3f(extent, corners + clip_planes.axis_aligned_box_front_corners[plane_idx]);
        const Vector3f front_corner = add_vector3f(offset, &unoffset_front_corner);

        if (dot3f(&front_corner, clip_planes.normals + plane_idx) < clip_planes.origin_distances[plane_idx])
            return 1;
    }

    return 0;
}

void cleanup_clip_planes(void)
{
    destroy_uniform(&clip_planes.active_count_uniform);
    destroy_uniform(&clip_planes.normals_uniform);
    destroy_uniform(&clip_planes.origin_distances_uniform);
}

static void generate_shader_code_for_clip_planes(void)
{
    check(active_shader_program);

    const char* active_clip_plane_count_name = clip_planes.active_count_uniform.name.chars;
    const char* clip_plane_normals_name = clip_planes.normals_uniform.name.chars;
    const char* clip_plane_origin_distances_name = clip_planes.origin_distances_uniform.name.chars;

    const size_t position_variable_number = get_vertex_position_variable_number();

    add_clip_distance_output_in_shader(&active_shader_program->vertex_shader_source, MAX_CLIP_PLANES);

    add_uniform_in_shader(&active_shader_program->vertex_shader_source, "uint", active_clip_plane_count_name);

    add_array_uniform_in_shader(&active_shader_program->vertex_shader_source, "vec3", clip_plane_normals_name, MAX_CLIP_PLANES);
    add_array_uniform_in_shader(&active_shader_program->vertex_shader_source, "float", clip_plane_origin_distances_name, MAX_CLIP_PLANES);

    DynamicString clip_plane_code = create_string(
    "\n    uint clip_plane_idx;"
    "\n    for (clip_plane_idx = 0; clip_plane_idx < %s; clip_plane_idx++)"
    "\n    {"
    "\n        gl_ClipDistance[clip_plane_idx] = dot(variable_%d.xyz, %s[clip_plane_idx]) - %s[clip_plane_idx];"
    "\n    }"
    "\n    for (clip_plane_idx = %s; clip_plane_idx < %d; clip_plane_idx++)"
    "\n    {"
    "\n        gl_ClipDistance[clip_plane_idx] = 1.0;"
    "\n    }",
    active_clip_plane_count_name,
    position_variable_number, clip_plane_normals_name, clip_plane_origin_distances_name,
    active_clip_plane_count_name, MAX_CLIP_PLANES);

    LinkedList global_dependencies = create_list();
    append_string_to_list(&global_dependencies, "gl_PerVertex");
    append_string_to_list(&global_dependencies, active_clip_plane_count_name);
    append_string_to_list(&global_dependencies, clip_plane_normals_name);
    append_string_to_list(&global_dependencies, clip_plane_origin_distances_name);

    LinkedList variable_dependencies = create_list();
    append_size_t_to_list(&variable_dependencies, position_variable_number);

    add_output_snippet_in_shader(&active_shader_program->vertex_shader_source, clip_plane_code.chars, &global_dependencies, &variable_dependencies);

    clear_list(&variable_dependencies);
    clear_list(&global_dependencies);
    clear_string(&clip_plane_code);
}

static void sync_clip_planes(void)
{
    check(active_shader_program);

    glUseProgram(active_shader_program->id);
    abort_on_GL_error("Could not use shader program for updating clip plane uniforms");

    if (clip_planes.active_count_uniform.needs_update)
    {
        glUniform1ui(clip_planes.active_count_uniform.location, clip_planes.active_count);
        abort_on_GL_error("Could not update number of active clip planes uniform");

        clip_planes.active_count_uniform.needs_update = 0;
    }

    if (clip_planes.normals_uniform.needs_update)
    {
        for (unsigned int plane_idx = 0; plane_idx < clip_planes.active_count; plane_idx++)
            clip_planes.axis_aligned_box_front_corners[plane_idx] = get_axis_aligned_box_front_corner_for_plane(clip_planes.normals + plane_idx);

        glUniform3fv(clip_planes.normals_uniform.location, (GLsizei)clip_planes.active_count, (const GLfloat*)clip_planes.normals);
        abort_on_GL_error("Could not update clip plane normals uniform");

        clip_planes.normals_uniform.needs_update = 0;
    }

    if (clip_planes.origin_distances_uniform.needs_update)
    {
        for (unsigned int plane_idx = 0; plane_idx < clip_planes.active_count; plane_idx++)
            clip_planes.origin_distances[plane_idx] = dot3f(clip_planes.origins + plane_idx, clip_planes.normals + plane_idx);

        glUniform1fv(clip_planes.origin_distances_uniform.location, (GLsizei)clip_planes.active_count, (const GLfloat*)clip_planes.origin_distances);
        abort_on_GL_error("Could not update clip plane origins uniform");

        clip_planes.origin_distances_uniform.needs_update = 0;
    }

    glUseProgram(0);
}
