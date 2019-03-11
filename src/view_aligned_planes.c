#include "view_aligned_planes.h"

#include "gl_includes.h"
#include "error.h"
#include "extra_math.h"
#include "geometry.h"
#include "dynamic_string.h"
#include "linked_list.h"
#include "transformation.h"
#include "shader_generator.h"

#include <stdlib.h>
#include <math.h>
#include <float.h>

#include <stdio.h>


typedef struct Uniform
{
    DynamicString name;
    GLint location;
    int needs_update;
} Uniform;

typedef struct PlaneVertex
{
    GLuint vertex_idx;
    GLuint plane_idx;
    Vector3f position;
} PlaneVertex;

typedef struct PlaneVertices
{
    PlaneVertex vertices[6];
} PlaneVertices;

typedef struct PlaneFaces
{
    GLuint indices[12];
} PlaneFaces;

typedef struct PlaneStack
{
    DynamicString plane_vertex_idx_name;
    DynamicString plane_idx_name;
    PlaneVertices* plane_vertices;
    PlaneFaces* plane_faces;
    unsigned int n_planes;
    void* vertex_buffer;
    void* face_buffer;
    size_t vertex_buffer_size;
    size_t face_buffer_size;
    GLuint vertex_array_object_id;
    GLuint vertex_buffer_id;
    GLuint face_buffer_id;
} PlaneStack;

typedef struct PlaneSeparation
{
    GLfloat value;
    float current_multiplier;
    Uniform uniform;
} PlaneSeparation;

typedef struct BrickCorners
{
    Vector3f corners[8];
    Vector3f current_spatial_extent;
    Uniform uniform;
} BrickCorners;


static void initialize_uniform(Uniform* uniform, const char* name);
static void load_uniform(const ShaderProgram* shader_program, Uniform* uniform);
static void initialize_plane_stack(void);
static void initialize_vertex_array_object(void);
static void update_number_of_planes(unsigned int n_planes);
static void allocate_plane_buffers(unsigned int n_planes);
static void update_plane_buffer_data(void);
static void update_vertex_array_object(void);
static void draw_plane_faces(unsigned int n_planes);
static void cleanup_plane_stack(void);
static void destroy_vertex_array_object(void);
static void clear_plane_stack(void);
static void reset_plane_stack_attributes(void);

static void compute_vertex_positions(unsigned int n_required_planes, float back_plane_dist, unsigned int back_corner_idx, const Vector3f* brick_offset);


static PlaneStack plane_stack;

static PlaneSeparation plane_separation;
static BrickCorners brick_corners;

static Uniform corner_permutations_uniform;
static const GLuint corner_permutations[64] = {0, 1, 2, 3, 4, 5, 6, 7,  // Back corner 0
                                               1, 4, 5, 0, 3, 7, 2, 6,  // Back corner 1
                                               2, 5, 6, 0, 1, 7, 3, 4,  // Back corner 2
                                               3, 4, 0, 6, 7, 1, 2, 5,  // Back corner 3
                                               4, 7, 1, 3, 6, 5, 0, 2,  // Back corner 4
                                               5, 1, 7, 2, 0, 4, 6, 3,  // Back corner 5
                                               6, 3, 2, 7, 4, 0, 5, 1,  // Back corner 6
                                               7, 6, 5, 4, 3, 2, 1, 0}; // Back corner 7

//    2----------5        5----------7
//   /|         /|       /|         /|
//  / |        / |      / |        / |
// 6----------7  |     2----------6  |
// |  |       |  |     |  |       |  |
// |  0-------|--1     |  1-------|--4
// | /        | /      | /        | /
// |/         |/       |/         |/
// 3----------4        0----------3

//    2----------5        6----------7
//   /|         /|       /|         /|
//  / |        / |      / |        / |
// 6----------7  |     3----------4  |
// |  |       |  |     |  |       |  |
// |  0-------|--1     |  2-------|--5
// | /        | /      | /        | /
// |/         |/       |/         |/
// 3----------4        0----------1

//    2----------5        0----------1
//   /|         /|       /|         /|
//  / |        / |      / |        / |
// 6----------7  |     2----------5  |
// |  |       |  |     |  |       |  |
// |  0-------|--1     |  3-------|--4
// | /        | /      | /        | /
// |/         |/       |/         |/
// 3----------4        6----------7

//    2----------5        1----------5
//   /|         /|       /|         /|
//  / |        / |      / |        / |
// 6----------7  |     0----------2  |
// |  |       |  |     |  |       |  |
// |  0-------|--1     |  4-------|--7
// | /        | /      | /        | /
// |/         |/       |/         |/
// 3----------4        3----------6

//    2----------5        7----------4
//   /|         /|       /|         /|
//  / |        / |      / |        / |
// 6----------7  |     6----------3  |
// |  |       |  |     |  |       |  |
// |  0-------|--1     |  5-------|--1
// | /        | /      | /        | /
// |/         |/       |/         |/
// 3----------4        2----------0

//    2----------5        2----------0
//   /|         /|       /|         /|
//  / |        / |      / |        / |
// 6----------7  |     5----------1  |
// |  |       |  |     |  |       |  |
// |  0-------|--1     |  6-------|--3
// | /        | /      | /        | /
// |/         |/       |/         |/
// 3----------4        7----------4

//    2----------5        5----------2
//   /|         /|       /|         /|
//  / |        / |      / |        / |
// 6----------7  |     1----------0  |
// |  |       |  |     |  |       |  |
// |  0-------|--1     |  7-------|--6
// | /        | /      | /        | /
// |/         |/       |/         |/
// 3----------4        4----------3

static Uniform edge_starts_uniform;
static const GLuint edge_starts[24] = {0, 1, 4, 0,  // Hexagon corner 0
                                       1, 0, 1, 4,  // Hexagon corner 1
                                       0, 2, 5, 0,  // Hexagon corner 2
                                       2, 0, 2, 5,  // Hexagon corner 3
                                       0, 3, 6, 0,  // Hexagon corner 4
                                       3, 0, 3, 6}; // Hexagon corner 5

static Uniform edge_ends_uniform;
static const GLuint edge_ends[24]   = {1, 4, 7, 0,  // Hexagon corner 0
                                       5, 1, 4, 7,  // Hexagon corner 1
                                       2, 5, 7, 0,  // Hexagon corner 2
                                       6, 2, 5, 7,  // Hexagon corner 3
                                       3, 6, 7, 0,  // Hexagon corner 4
                                       4, 3, 6, 7}; // Hexagon corner 5
//                                     ^  ^  ^  ^
//               Intersection check #  1  2  3  4

static Uniform brick_offset_uniform;
static Uniform back_plane_dist_uniform;
static Uniform back_corner_idx_uniform;

static const BrickedField* active_bricked_field = NULL;


void initialize_planes(void)
{
    initialize_plane_stack();

    plane_separation.value = 0.0f;
    plane_separation.current_multiplier = 0.0f;
    initialize_uniform(&plane_separation.uniform, "plane_separation");

    set_vector3f_elements(&brick_corners.current_spatial_extent, 0.0f, 0.0f, 0.0f);
    initialize_uniform(&brick_corners.uniform, "brick_corners");

    initialize_uniform(&brick_offset_uniform, "brick_offset");
    initialize_uniform(&back_plane_dist_uniform, "back_plane_dist");
    initialize_uniform(&back_corner_idx_uniform, "back_corner_idx");
    initialize_uniform(&corner_permutations_uniform, "corner_permutations");
    initialize_uniform(&edge_starts_uniform, "edge_starts");
    initialize_uniform(&edge_ends_uniform, "edge_ends");
}

void generate_shader_code_for_planes(ShaderProgram* shader_program)
{
    check(shader_program);

    const char* vertex_idx_name = plane_stack.plane_vertex_idx_name.chars;
    const char* plane_idx_name = plane_stack.plane_idx_name.chars;
    const char* plane_separation_name =  plane_separation.uniform.name.chars;
    const char* brick_corners_name =  brick_corners.uniform.name.chars;
    const char* brick_offset_name =  brick_offset_uniform.name.chars;
    const char* back_plane_dist_name =  back_plane_dist_uniform.name.chars;
    const char* back_corner_idx_name =  back_corner_idx_uniform.name.chars;
    const char* corner_permutations_name =  corner_permutations_uniform.name.chars;
    const char* edge_starts_name =  edge_starts_uniform.name.chars;
    const char* edge_ends_name =  edge_ends_uniform.name.chars;

    add_vertex_input_in_shader(&shader_program->vertex_shader_source, "uint", vertex_idx_name, 0);
    add_vertex_input_in_shader(&shader_program->vertex_shader_source, "uint", plane_idx_name, 1);

    add_uniform_in_shader(&shader_program->vertex_shader_source, "float", plane_separation_name);
    add_array_uniform_in_shader(&shader_program->vertex_shader_source, "vec3", brick_corners_name, 8);

    add_uniform_in_shader(&shader_program->vertex_shader_source, "vec3", brick_offset_name);
    add_uniform_in_shader(&shader_program->vertex_shader_source, "float", back_plane_dist_name);
    add_uniform_in_shader(&shader_program->vertex_shader_source, "uint", back_corner_idx_name);
    add_array_uniform_in_shader(&shader_program->vertex_shader_source, "uint", corner_permutations_name, 64);
    add_array_uniform_in_shader(&shader_program->vertex_shader_source, "uint", edge_starts_name, 24);
    add_array_uniform_in_shader(&shader_program->vertex_shader_source, "uint", edge_ends_name, 24);

    DynamicString position_code = create_string(
      "    float plane_dist = back_plane_dist + plane_idx*plane_separation;"
    "\n"
    "\n    vec4 position;"
    "\n"
    "\n    for (uint edge_idx = 0; edge_idx < 4; edge_idx++)"
    "\n    {"
    "\n        uint edge_start_idx = edge_starts[4*vertex_idx + edge_idx];"
    "\n        uint edge_end_idx   =   edge_ends[4*vertex_idx + edge_idx];"
    "\n"
    "\n        vec3 edge_start = brick_corners[corner_permutations[8*back_corner_idx + edge_start_idx]];"
    "\n        vec3 edge_end   = brick_corners[corner_permutations[8*back_corner_idx + edge_end_idx]];"
    "\n"
    "\n        vec3 edge_origin = edge_start + brick_offset;"
    "\n        vec3 edge_vector = edge_end - edge_start;"
    "\n"
    "\n        float denom = dot(edge_vector, look_axis);"
    "\n        float lambda = (denom != 0.0) ? (plane_dist - dot(edge_origin, look_axis))/denom : -1.0;"
    "\n"
    "\n        if (lambda >= 0.0 && lambda <= 1.0)"
    "\n        {"
    "\n            position.xyz = edge_origin + lambda*edge_vector;"
    "\n            position.w = 1.0;"
    "\n            break;"
    "\n        }"
    "\n    }"
    );

    LinkedList position_global_dependencies = create_list();
    append_string_to_list(&position_global_dependencies, vertex_idx_name);
    append_string_to_list(&position_global_dependencies, plane_idx_name);
    append_string_to_list(&position_global_dependencies, plane_separation_name);
    append_string_to_list(&position_global_dependencies, brick_corners_name);
    append_string_to_list(&position_global_dependencies, brick_offset_name);
    append_string_to_list(&position_global_dependencies, back_plane_dist_name);
    append_string_to_list(&position_global_dependencies, back_corner_idx_name);
    append_string_to_list(&position_global_dependencies, corner_permutations_name);
    append_string_to_list(&position_global_dependencies, edge_starts_name);
    append_string_to_list(&position_global_dependencies, edge_ends_name);
    append_string_to_list(&position_global_dependencies, get_look_axis_name());

    const size_t position_variable_number = add_snippet_in_shader(&shader_program->vertex_shader_source, "vec4", "position", position_code.chars, &position_global_dependencies, NULL);

    clear_list(&position_global_dependencies);
    clear_string(&position_code);

    assign_transformed_variable_to_output_in_shader(&shader_program->vertex_shader_source, get_transformation_name(), position_variable_number, "gl_Position");


    DynamicString tex_coord_code = create_string(
    "\n    vec3 tex_coord = 0.5*variable_%d.xyz + vec3(0.5, 0.5, 0.5);",
    position_variable_number);

    LinkedList tex_coord_variable_dependencies = create_list();
    append_size_t_to_list(&tex_coord_variable_dependencies, position_variable_number);

    const size_t tex_coord_variable_number = add_snippet_in_shader(&shader_program->vertex_shader_source, "vec3", "tex_coord", tex_coord_code.chars, NULL, &tex_coord_variable_dependencies);

    clear_list(&tex_coord_variable_dependencies);
    clear_string(&tex_coord_code);

    assign_variable_to_new_output_in_shader(&shader_program->vertex_shader_source, "vec3", tex_coord_variable_number, "out_tex_coord");
    add_input_in_shader(&shader_program->fragment_shader_source, "vec3", "out_tex_coord");
}

void load_planes(const ShaderProgram* shader_program)
{
    check(shader_program);

    load_uniform(shader_program, &plane_separation.uniform);
    load_uniform(shader_program, &brick_corners.uniform);
    load_uniform(shader_program, &brick_offset_uniform);
    load_uniform(shader_program, &back_plane_dist_uniform);
    load_uniform(shader_program, &back_corner_idx_uniform);
    load_uniform(shader_program, &corner_permutations_uniform);
    load_uniform(shader_program, &edge_starts_uniform);
    load_uniform(shader_program, &edge_ends_uniform);

    glUseProgram(shader_program->id);
    abort_on_GL_error("Could not use shader program for setting view aligned planes uniforms");

    glUniform1uiv(corner_permutations_uniform.location, 64, corner_permutations);
    abort_on_GL_error("Could not set corner permutations uniform");

    glUniform1uiv(edge_starts_uniform.location, 24, edge_starts);
    abort_on_GL_error("Could not set edge starts uniform");

    glUniform1uiv(edge_ends_uniform.location, 24, edge_ends);
    abort_on_GL_error("Could not set edge ends uniform");

    glUseProgram(0);
}

void set_active_bricked_field(const BrickedField* bricked_field)
{
    active_bricked_field = bricked_field;
}

void set_plane_separation(float spacing_multiplier)
{
    check(active_bricked_field);
    check(active_bricked_field->field);
    check(spacing_multiplier > 0);

    const unsigned int max_n_planes = (unsigned int)(2*active_bricked_field->brick_size/spacing_multiplier);

    if (max_n_planes < 2)
        print_severe_message("Cannot create fewer than two planes.");

    plane_separation.value = active_bricked_field->spatial_brick_diagonal/(max_n_planes - 1);
    plane_separation.current_multiplier = spacing_multiplier;
    plane_separation.uniform.needs_update = 1;

    if (max_n_planes > plane_stack.n_planes)
        update_number_of_planes(max_n_planes);
}

float get_plane_separation(void)
{
    return plane_separation.current_multiplier;
}

void draw_brick(size_t brick_idx)
{
    check(plane_stack.n_planes > 0);
    check(active_bricked_field);
    check(brick_idx < active_bricked_field->n_bricks);

    const Brick* const brick = active_bricked_field->bricks + brick_idx;

    // Update offset to first brick corner
    glUniform3f(brick_offset_uniform.location,
                brick->spatial_offset.a[0],
                brick->spatial_offset.a[1],
                brick->spatial_offset.a[2]);

    // Update brick corner positions relative to offset if the brick extent has changed
    if (!equal_vector3f(&brick->spatial_extent, &brick_corners.current_spatial_extent))
    {
        const float dx = brick->spatial_extent.a[0];
        const float dy = brick->spatial_extent.a[1];
        const float dz = brick->spatial_extent.a[2];
                                                                      //    2----------5
        set_vector3f_elements(brick_corners.corners + 0,  0,  0,  0); //   /|         /|
        set_vector3f_elements(brick_corners.corners + 1, dx,  0,  0); //  / |        / |
        set_vector3f_elements(brick_corners.corners + 2,  0, dy,  0); // 6----------7  |
        set_vector3f_elements(brick_corners.corners + 3,  0,  0, dz); // |  |       |  |
        set_vector3f_elements(brick_corners.corners + 4, dx,  0, dz); // |  0-------|--1
        set_vector3f_elements(brick_corners.corners + 5, dx, dy,  0); // | /        | /
        set_vector3f_elements(brick_corners.corners + 6,  0, dy, dz); // |/         |/
        set_vector3f_elements(brick_corners.corners + 7, dx, dy, dz); // 3----------4

        glUniform3fv(brick_corners.uniform.location, 8, (GLfloat*)brick_corners.corners);

        // Update current spatial extent
        copy_vector3f(&brick->spatial_extent, &brick_corners.current_spatial_extent);
    }

    /*
    Project brick corners onto the look axis and find the one
    giving the smallest (most negative/least positive) value.
    This value gives the initial (signed) distance for the plane
    stack. The corresponding corner is the back corner. The difference
    between the largest (least negative/most positive) value and
    the smallest value gives the projected depth of the brick, which
    is needed to determine the number of planes to render.

    Camera views along the negative look axis.
     5   1   7   4   2   0   6   3
    -|---|---|---|---|---|---|---|--look axis-->    <-- (> [Camera]
     ^
    Back corner
    */

    const Vector3f look_axis = get_look_axis();

    unsigned int corner_idx;
    unsigned int back_corner_idx;
    float dist;
    float min_dist = FLT_MAX;
    float max_dist = -FLT_MAX;

    for (corner_idx = 0; corner_idx < 8; corner_idx++)
    {
        dist = dot3f(brick_corners.corners + corner_idx, &look_axis);

        if (dist > max_dist)
            max_dist = dist;

        if (dist < min_dist)
        {
            min_dist = dist;
            back_corner_idx = corner_idx;
        }
    }

    assert(max_dist > min_dist);

    // Offset start distance by half a plane spacing so that the first plane gets a non-zero area
    min_dist += 0.5f*plane_separation.value;

    const float back_plane_dist = min_dist + dot3f(&brick->spatial_offset, &look_axis);

    glUniform1f(back_plane_dist_uniform.location, (GLfloat)back_plane_dist);
    glUniform1ui(back_corner_idx_uniform.location, (GLuint)back_corner_idx);

    // Determine number of planes needed to traverse the brick from back to front along the view axis.
    // Also make sure it doesn't exceed the total number of available planes due to round-off errors.
    const unsigned int n_required_planes = uimin((unsigned int)((max_dist - min_dist)/plane_separation.value) + 1, plane_stack.n_planes);

    //compute_vertex_positions(n_required_planes, back_plane_dist, back_corner_idx, &brick->spatial_offset);
    draw_plane_faces(n_required_planes);
}

static void compute_vertex_positions(unsigned int n_required_planes, float back_plane_dist, unsigned int back_corner_idx, const Vector3f* brick_offset)
{
    const Vector3f look_axis = get_look_axis();

    printf("Back corner: %d at %g, look axis: (%g, %g, %g)\n", back_corner_idx, back_plane_dist, look_axis.a[0], look_axis.a[1], look_axis.a[2]);

    unsigned int plane_idx, vertex_idx, edge_idx;

    for (plane_idx = 0; plane_idx < n_required_planes; plane_idx++)
    {
        float plane_dist = back_plane_dist + plane_idx*plane_separation.value;

        for (vertex_idx = 0; vertex_idx < 6; vertex_idx++)
        {
            for (edge_idx = 0; edge_idx < 4; edge_idx++)
            {
                unsigned int edge_start_idx = edge_starts[4*vertex_idx + edge_idx];
                unsigned int edge_end_idx   =   edge_ends[4*vertex_idx + edge_idx];

                Vector3f edge_start = brick_corners.corners[corner_permutations[8*back_corner_idx + edge_start_idx]];
                Vector3f edge_end   = brick_corners.corners[corner_permutations[8*back_corner_idx + edge_end_idx]];

                Vector3f edge_origin = add_vector3f(&edge_start, brick_offset);
                Vector3f edge_vector = subtract_vector3f(&edge_end, &edge_start);

                printf("Plane %d: Vertex %d: Edge (%g, %g, %g) -> (%g, %g, %g)\n", plane_idx, vertex_idx, edge_origin.a[0], edge_origin.a[1], edge_origin.a[2], edge_origin.a[0] + edge_vector.a[0], edge_origin.a[1] + edge_vector.a[1], edge_origin.a[2] + edge_vector.a[2]);

                float denom = dot3f(&edge_vector, &look_axis);
                float lambda = (denom != 0.0) ? (plane_dist - dot3f(&edge_origin, &look_axis))/denom : -1.0;

                if (lambda >= 0.0 && lambda <= 1.0)
                {
                    printf("Intersection: lambda = %g\n", lambda);
                    plane_stack.plane_vertices[plane_idx].vertices[vertex_idx].position.a[0] = edge_origin.a[0] + lambda*edge_vector.a[0];
                    plane_stack.plane_vertices[plane_idx].vertices[vertex_idx].position.a[1] = edge_origin.a[1] + lambda*edge_vector.a[1];
                    plane_stack.plane_vertices[plane_idx].vertices[vertex_idx].position.a[2] = edge_origin.a[2] + lambda*edge_vector.a[2];
                    break;
                }
            }
        }
        printf("Plane %d\n", plane_idx);
        for (vertex_idx = 0; vertex_idx < 6; vertex_idx++)
            printf("P%d = (%g, %g, %g)\n", vertex_idx, plane_stack.plane_vertices[plane_idx].vertices[vertex_idx].position.a[0], plane_stack.plane_vertices[plane_idx].vertices[vertex_idx].position.a[1], plane_stack.plane_vertices[plane_idx].vertices[vertex_idx].position.a[2]);

        /*Vector3f triangle_edge_1 = subtract_vector3f(&plane_stack.plane_vertices[plane_idx].vertices[2].position, &plane_stack.plane_vertices[plane_idx].vertices[0].position);
        Vector3f triangle_edge_2 = subtract_vector3f(&plane_stack.plane_vertices[plane_idx].vertices[4].position, &plane_stack.plane_vertices[plane_idx].vertices[2].position);
        Vector3f triangle_normal = cross3f(&triangle_edge_1, &triangle_edge_2);
        Vector4f triangle_normal_h = extend_vector3f_to_vector4f(&triangle_normal, 1.0f);
        Matrix4f modelview = get_modelview_transform_matrix();
        Vector4f transformed_triangle_normal_h = matvecmul4f(&modelview, &triangle_normal_h);
        Vector3f transformed_triangle_normal = extract_vector3f_from_vector4f(&transformed_triangle_normal_h);
        normalize_vector3f(&triangle_normal);
        normalize_vector3f(&transformed_triangle_normal);
        print_vector3f(&look_axis);
        print_vector3f(&triangle_normal);
        print_vector3f(&transformed_triangle_normal);*/
    }

    glBindVertexArray(plane_stack.vertex_array_object_id);
    abort_on_GL_error("Could not bind VAO");

    // Store buffer of all vertices on device
    glBindBuffer(GL_ARRAY_BUFFER, plane_stack.vertex_buffer_id);
    glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)plane_stack.vertex_buffer_size, (GLvoid*)plane_stack.vertex_buffer, GL_STATIC_DRAW);
    abort_on_GL_error("Could not bind VBO to VAO");

    glBindVertexArray(0);
}

void sync_planes(const ShaderProgram* shader_program)
{
    assert(shader_program);

    if (plane_separation.uniform.needs_update && plane_separation.uniform.location != -1)
    {
        glUseProgram(shader_program->id);
        abort_on_GL_error("Could not use shader program for updating plane separation uniform");
        glUniform1f(plane_separation.uniform.location, plane_separation.value);
        abort_on_GL_error("Could not update plane separation uniform");
        glUseProgram(0);

        plane_separation.uniform.needs_update = 0;
    }
}

void cleanup_planes(void)
{
    cleanup_plane_stack();

    clear_string(&plane_separation.uniform.name);
    clear_string(&brick_corners.uniform.name);
    clear_string(&brick_offset_uniform.name);
    clear_string(&back_plane_dist_uniform.name);
    clear_string(&back_corner_idx_uniform.name);
    clear_string(&corner_permutations_uniform.name);
    clear_string(&edge_starts_uniform.name);
    clear_string(&edge_ends_uniform.name);

    active_bricked_field = NULL;
}

static void initialize_uniform(Uniform* uniform, const char* name)
{
    check(uniform);
    check(name);
    uniform->name = create_string(name);
    uniform->location = -1;
    uniform->needs_update = 0;
}

static void load_uniform(const ShaderProgram* shader_program, Uniform* uniform)
{
    check(shader_program);
    check(uniform);

    uniform->location = glGetUniformLocation(shader_program->id, uniform->name.chars);
    abort_on_GL_error("Could not get location of view aligned planes uniform");

    if (plane_separation.uniform.location == -1)
        print_warning_message("Uniform \"%s\" not used in shader program.", uniform->name.chars);
}

static void initialize_plane_stack(void)
{
    reset_plane_stack_attributes();
    initialize_vertex_array_object();

    plane_stack.plane_vertex_idx_name = create_string("vertex_idx");
    plane_stack.plane_idx_name = create_string("plane_idx");
}

static void initialize_vertex_array_object(void)
{
    // Generate vertex array object for keeping track of vertex attributes
    glGenVertexArrays(1, &plane_stack.vertex_array_object_id);
    abort_on_GL_error("Could not generate VAO");

    glBindVertexArray(plane_stack.vertex_array_object_id);
    abort_on_GL_error("Could not bind VAO");

    // Generate buffer object for vertices
    glGenBuffers(1, &plane_stack.vertex_buffer_id);
    abort_on_GL_error("Could not generate vertex buffer object");

    // Generate buffer object for face indices
    glGenBuffers(1, &plane_stack.face_buffer_id);
    abort_on_GL_error("Could not generate face buffer object");

    glBindVertexArray(0);
}

static void update_number_of_planes(unsigned int n_planes)
{
    allocate_plane_buffers(n_planes);
    update_plane_buffer_data();
    update_vertex_array_object();
}

static void allocate_plane_buffers(unsigned int n_planes)
{
    if (plane_stack.vertex_buffer || plane_stack.face_buffer)
        clear_plane_stack();

    plane_stack.n_planes = n_planes;

    plane_stack.vertex_buffer_size = plane_stack.n_planes*sizeof(PlaneVertices);
    plane_stack.face_buffer_size = plane_stack.n_planes*sizeof(PlaneFaces);

    plane_stack.vertex_buffer = malloc(plane_stack.vertex_buffer_size);
    if (!plane_stack.vertex_buffer)
        print_severe_message("Could not allocate memory for plane vertices.");

    plane_stack.face_buffer = malloc(plane_stack.face_buffer_size);
    if (!plane_stack.face_buffer)
        print_severe_message("Could not allocate memory for plane faces.");

    plane_stack.plane_vertices = (PlaneVertices*)plane_stack.vertex_buffer;
    plane_stack.plane_faces = (PlaneFaces*)plane_stack.face_buffer;
}

static void update_plane_buffer_data(void)
{
    check(plane_stack.vertex_buffer);
    check(plane_stack.face_buffer);

    unsigned int i, j;
    for (i = 0; i < plane_stack.n_planes; i++)
    {
        PlaneVertex* const plane_vertices = plane_stack.plane_vertices[i].vertices;
        GLuint* const plane_face_indices = plane_stack.plane_faces[i].indices;

        for (j = 0; j < 6; j++)
        {
            plane_vertices[j].vertex_idx = j;
            plane_vertices[j].plane_idx = i;
            set_vector3f_elements(&plane_vertices[j].position, 0, 0, 0);
        }

        plane_face_indices[0] = 6*i + 0;
        plane_face_indices[1] = 6*i + 2;
        plane_face_indices[2] = 6*i + 4;

        plane_face_indices[3] = 6*i + 0;
        plane_face_indices[4] = 6*i + 1;
        plane_face_indices[5] = 6*i + 2;

        plane_face_indices[6] = 6*i + 2;
        plane_face_indices[7] = 6*i + 3;
        plane_face_indices[8] = 6*i + 4;

        plane_face_indices[9] = 6*i + 4;
        plane_face_indices[10] = 6*i + 5;
        plane_face_indices[11] = 6*i + 0;
    }
}

static void update_vertex_array_object(void)
{
    check(plane_stack.vertex_buffer);
    check(plane_stack.face_buffer);
    assert(plane_stack.vertex_array_object_id > 0);
    assert(plane_stack.vertex_buffer_id > 0);
    assert(plane_stack.face_buffer_id > 0);

    glBindVertexArray(plane_stack.vertex_array_object_id);
    abort_on_GL_error("Could not bind VAO");

    // Store buffer of all vertices on device
    glBindBuffer(GL_ARRAY_BUFFER, plane_stack.vertex_buffer_id);
    glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)plane_stack.vertex_buffer_size, (GLvoid*)plane_stack.vertex_buffer, GL_STATIC_DRAW);
    abort_on_GL_error("Could not bind VBO to VAO");

    // Specify vertex attribute pointer for vertex indices
    glVertexAttribIPointer(0, 1, GL_UNSIGNED_INT, sizeof(PlaneVertex), (GLvoid*)0);
    glEnableVertexAttribArray(0);
    abort_on_GL_error("Could not set VAO vertex index attributes");

    // Specify vertex attribute pointer for plane indices
    glVertexAttribIPointer(1, 1, GL_UNSIGNED_INT, sizeof(PlaneVertex), (GLvoid*)sizeof(GLuint));
    glEnableVertexAttribArray(1);
    abort_on_GL_error("Could not set VAO plane index attributes");

    // Specify vertex attribute pointer for vertex positions
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(PlaneVertex), (GLvoid*)(2*sizeof(GLuint)));
    glEnableVertexAttribArray(2);
    abort_on_GL_error("Could not set VAO vertex position attributes");

    // Store buffer of all face indices on device
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, plane_stack.face_buffer_id);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, (GLsizeiptr)plane_stack.face_buffer_size, (GLvoid*)plane_stack.face_buffer, GL_STATIC_DRAW);
    abort_on_GL_error("Could not bind IBO to VAO");

    glBindVertexArray(0);
}

static void draw_plane_faces(unsigned int n_planes)
{
    assert(n_planes <= plane_stack.n_planes);
    assert(plane_stack.vertex_array_object_id > 0);

    glBindVertexArray(plane_stack.vertex_array_object_id);
    abort_on_GL_error("Could not bind VAO for drawing");

    glDrawElements(GL_TRIANGLES, (GLsizei)(12*n_planes), GL_UNSIGNED_INT, (GLvoid*)0);
    //glDrawElements(GL_TRIANGLES, (GLsizei)12, GL_UNSIGNED_INT, (GLvoid*)(n_planes*sizeof(PlaneFaces)));
    abort_on_GL_error("Could not draw planes");

    glBindVertexArray(0);
}

static void cleanup_plane_stack(void)
{
    destroy_vertex_array_object();
    clear_plane_stack();

    clear_string(&plane_stack.plane_vertex_idx_name);
    clear_string(&plane_stack.plane_idx_name);
}

static void destroy_vertex_array_object(void)
{
    if (plane_stack.face_buffer_id != 0)
        glDeleteBuffers(1, &plane_stack.face_buffer_id);

    if (plane_stack.vertex_buffer_id != 0)
        glDeleteBuffers(1, &plane_stack.vertex_buffer_id);

    if (plane_stack.vertex_array_object_id != 0)
        glDeleteVertexArrays(1, &plane_stack.vertex_array_object_id);

    abort_on_GL_error("Could not destroy buffer objects");
}

static void clear_plane_stack(void)
{
    if (plane_stack.vertex_buffer)
        free(plane_stack.vertex_buffer);

    if (plane_stack.face_buffer)
        free(plane_stack.face_buffer);

    reset_plane_stack_attributes();
}

static void reset_plane_stack_attributes(void)
{
    plane_stack.vertex_array_object_id = 0;
    plane_stack.vertex_buffer_id = 0;
    plane_stack.face_buffer_id = 0;

    plane_stack.vertex_buffer = NULL;
    plane_stack.face_buffer = NULL;

    plane_stack.vertex_buffer_size = 0;
    plane_stack.face_buffer_size = 0;

    plane_stack.plane_vertices = NULL;
    plane_stack.plane_faces = NULL;

    plane_stack.n_planes = 0;
}
