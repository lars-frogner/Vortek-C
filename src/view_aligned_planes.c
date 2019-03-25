/*
 * The slicing of view aligned planes through a box is implemented in a vertex
 * shader, based on Salama and Kolb (2005) "A Vertex Program for Efficient
 * Box-Plane Intersection". This allows for a large number of small boxes
 * to be sliced per frame (see bricks.c) since very little data has to be
 * sent to the GPU for each box.
 */

#include "view_aligned_planes.h"

#include "gl_includes.h"
#include "error.h"
#include "extra_math.h"
#include "geometry.h"
#include "dynamic_string.h"
#include "linked_list.h"
#include "transformation.h"
#include "indicators.h"
#include "shader_generator.h"
#include "clip_planes.h"

#include <stdlib.h>
#include <math.h>
#include <float.h>

#include <stdio.h>


typedef struct PlaneVertex
{
    GLuint vertex_idx;
    GLuint plane_idx;
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
    float original_value;
    Uniform uniform;
} PlaneSeparation;

typedef struct ActiveBrickedField
{
    const BrickedField* bricked_field;
    const Vector3f* current_look_axis;
    const Vector3f* current_camera_position;
    unsigned int current_back_corner_idx;
    unsigned int current_front_corner_idx;
} ActiveBrickedField;

typedef struct Configuration
{
    float plane_separation_multiplier;
    float lower_visibility_threshold;
    float upper_visibility_threshold;
    int draw_field_outline;
    int draw_brick_outline;
    int draw_sub_brick_outline;
} Configuration;


static void initialize_plane_stack(void);
static void initialize_vertex_array_object(void);

static void update_number_of_planes(unsigned int n_planes);
static void allocate_plane_buffers(unsigned int n_planes);
static void update_plane_buffer_data(void);
static void update_vertex_array_object(void);

static void generate_shader_code_for_planes(void);

static void draw_brick_tree_nodes(BrickTreeNode* node);
static void draw_brick(const Brick* brick);
static void draw_sub_brick_tree_nodes(SubBrickTreeNode* node);
static void draw_sub_brick(const SubBrickTreeNode* node);
static void draw_plane_faces(unsigned int n_planes);

static void sync_plane_separation(void);

static void cleanup_plane_stack(void);
static void destroy_vertex_array_object(void);
static void clear_plane_stack(void);
static void reset_plane_stack_attributes(void);


static PlaneStack plane_stack;

static PlaneSeparation plane_separation;

// Corner positions of a unit axis aligned cube
static Uniform corners_uniform;                   //    2----------5
static const Vector3f corners[8] = {{{0, 0, 0}},  //   /|         /|
                                    {{1, 0, 0}},  //  / |        / |
                                    {{0, 1, 0}},  // 6----------7  |
                                    {{0, 0, 1}},  // |  |       |  |
                                    {{1, 0, 1}},  // |  0-------|--1
                                    {{1, 1, 0}},  // | /        | /
                                    {{0, 1, 1}},  // |/         |/
                                    {{1, 1, 1}}}; // 3----------4

static const unsigned int back_corners[2][2][2] = {{{0, 3}, {2, 6}},
                                                   {{1, 4}, {5, 7}}};

static const unsigned int front_corners[2][2][2] = {{{7, 5}, {4, 1}},
                                                    {{6, 2}, {3, 0}}};

static const unsigned int opposite_corners[8] = {7, 6, 4, 5, 2, 3, 1, 0};

// Brick corner permutations corresponding to the 8 different rotational arrangements
static Uniform corner_permutations_uniform;
static const GLuint corner_permutations[64] = {0, 1, 2, 3, 4, 5, 6, 7,  // Back corner 0
                                               1, 4, 5, 0, 3, 7, 2, 6,  // Back corner 1
                                               2, 5, 6, 0, 1, 7, 3, 4,  // Back corner 2
                                               3, 4, 0, 6, 7, 1, 2, 5,  // Back corner 3
                                               4, 7, 1, 3, 6, 5, 0, 2,  // Back corner 4
                                               5, 1, 7, 2, 0, 4, 6, 3,  // Back corner 5
                                               6, 3, 2, 7, 4, 0, 5, 1,  // Back corner 6
                                               7, 6, 5, 4, 3, 2, 1, 0}; // Back corner 7

// Indices of the corners giving the starting points of the edges to test intersections against
static Uniform edge_starts_uniform;
static const GLuint edge_starts[24] = {0, 1, 4, 0,  // Hexagon corner 0
                                       1, 0, 1, 4,  // Hexagon corner 1
                                       0, 2, 5, 0,  // Hexagon corner 2
                                       2, 0, 2, 5,  // Hexagon corner 3
                                       0, 3, 6, 0,  // Hexagon corner 4
                                       3, 0, 3, 6}; // Hexagon corner 5

// Indices of the corners giving the ending points of the edges to test intersections against
static Uniform edge_ends_uniform;
static const GLuint edge_ends[24]   = {1, 4, 7, 0,  // Hexagon corner 0
                                       5, 1, 4, 7,  // Hexagon corner 1
                                       2, 5, 7, 0,  // Hexagon corner 2
                                       6, 2, 5, 7,  // Hexagon corner 3
                                       3, 6, 7, 0,  // Hexagon corner 4
                                       4, 3, 6, 7}; // Hexagon corner 5
//                                     ^  ^  ^  ^
//               Intersection check #  1  2  3  4

static Uniform orientation_permutations_uniform;
static const GLuint orientation_permutations[9] = {0, 1, 2,  // Cycle 0
                                                   1, 2, 0,  // Cycle 1
                                                   2, 0, 1}; // Cycle 2

static Uniform brick_offset_uniform;
static Uniform brick_extent_uniform;
static Uniform pad_fractions_uniform;
static Uniform subbrick_offset_uniform;
static Uniform subbrick_extent_uniform;

static Uniform back_plane_dist_uniform;
static Uniform back_corner_idx_uniform;

static Uniform orientation_uniform;

static Uniform sampling_correction_uniform;

static size_t position_variable_number;
static size_t tex_coord_variable_number;

static ActiveBrickedField active_bricked_field = {NULL, NULL, NULL, 0, 0};

static ShaderProgram* active_shader_program = NULL;

static Configuration configuration;


void set_active_shader_program_for_planes(ShaderProgram* shader_program)
{
    active_shader_program = shader_program;
}

void initialize_planes(void)
{
    initialize_plane_stack();

    configuration.lower_visibility_threshold = 0.0f;
    configuration.upper_visibility_threshold = 0.9f;
    configuration.draw_field_outline = 1;
    configuration.draw_brick_outline = 0;
    configuration.draw_sub_brick_outline = 0;

    plane_separation.value = 0.0f;
    plane_separation.original_value = 0.0f;
    configuration.plane_separation_multiplier = 0.0f;
    initialize_uniform(&plane_separation.uniform, "plane_separation");

    initialize_uniform(&corners_uniform, "corners");
    initialize_uniform(&corner_permutations_uniform, "corner_permutations");
    initialize_uniform(&edge_starts_uniform, "edge_starts");
    initialize_uniform(&edge_ends_uniform, "edge_ends");
    initialize_uniform(&orientation_permutations_uniform, "orientation_permutations");

    initialize_uniform(&brick_offset_uniform, "brick_offset");
    initialize_uniform(&brick_extent_uniform, "brick_extent");
    initialize_uniform(&pad_fractions_uniform, "pad_fractions");
    initialize_uniform(&subbrick_offset_uniform, "sub_brick_offset");
    initialize_uniform(&subbrick_extent_uniform, "sub_brick_extent");

    initialize_uniform(&back_plane_dist_uniform, "back_plane_dist");
    initialize_uniform(&back_corner_idx_uniform, "back_corner_idx");

    initialize_uniform(&orientation_uniform, "orientation");

    initialize_uniform(&sampling_correction_uniform, "sampling_correction");

    generate_shader_code_for_planes();
}

void load_planes(void)
{
    check(active_shader_program);

    load_uniform(active_shader_program, &plane_separation.uniform);

    load_uniform(active_shader_program, &corners_uniform);
    load_uniform(active_shader_program, &corner_permutations_uniform);
    load_uniform(active_shader_program, &edge_starts_uniform);
    load_uniform(active_shader_program, &edge_ends_uniform);
    load_uniform(active_shader_program, &orientation_permutations_uniform);

    load_uniform(active_shader_program, &brick_offset_uniform);
    load_uniform(active_shader_program, &brick_extent_uniform);
    load_uniform(active_shader_program, &pad_fractions_uniform);
    load_uniform(active_shader_program, &subbrick_offset_uniform);
    load_uniform(active_shader_program, &subbrick_extent_uniform);

    load_uniform(active_shader_program, &back_plane_dist_uniform);
    load_uniform(active_shader_program, &back_corner_idx_uniform);

    load_uniform(active_shader_program, &orientation_uniform);

    load_uniform(active_shader_program, &sampling_correction_uniform);

    glUseProgram(active_shader_program->id);
    abort_on_GL_error("Could not use shader program for setting view aligned planes uniforms");

    glUniform3fv(corners_uniform.location, 8, (GLfloat*)corners);
    abort_on_GL_error("Could not set corners uniform");

    glUniform1uiv(corner_permutations_uniform.location, 64, corner_permutations);
    abort_on_GL_error("Could not set corner permutations uniform");

    glUniform1uiv(edge_starts_uniform.location, 24, edge_starts);
    abort_on_GL_error("Could not set edge starts uniform");

    glUniform1uiv(edge_ends_uniform.location, 24, edge_ends);
    abort_on_GL_error("Could not set edge ends uniform");

    glUniform1uiv(orientation_permutations_uniform.location, 9, orientation_permutations);
    abort_on_GL_error("Could not set orientation permutations uniform");

    glUseProgram(0);
}

void set_active_bricked_field(const BrickedField* bricked_field)
{
    active_bricked_field.bricked_field = bricked_field;
}

void set_lower_visibility_threshold(float threshold)
{
    check(threshold >= 0.0f && threshold <= configuration.upper_visibility_threshold);
    configuration.lower_visibility_threshold = threshold;
}

void set_upper_visibility_threshold(float threshold)
{
    check(threshold >= configuration.lower_visibility_threshold && threshold <= 1.0f);
    configuration.upper_visibility_threshold = threshold;
}

void toggle_field_outline_drawing(void)
{
    configuration.draw_field_outline = !configuration.draw_field_outline;
}

void toggle_brick_outline_drawing(void)
{
    configuration.draw_brick_outline = !configuration.draw_brick_outline;
}

void toggle_sub_brick_outline_drawing(void)
{
    configuration.draw_sub_brick_outline = !configuration.draw_sub_brick_outline;
}

void set_plane_separation(float spacing_multiplier)
{
    const BrickedField* const bricked_field = active_bricked_field.bricked_field;

    check(bricked_field);
    check(bricked_field->field);
    check(spacing_multiplier > 0);

    const float voxel_width = bricked_field->field->voxel_width;
    const float voxel_height = bricked_field->field->voxel_height;
    const float voxel_depth = bricked_field->field->voxel_depth;

    const float min_voxel_extent = fminf(voxel_width, fminf(voxel_height, voxel_depth));
    const float max_voxel_extent = sqrtf(voxel_width*voxel_width + voxel_height*voxel_height + voxel_depth*voxel_depth);

    plane_separation.value = min_voxel_extent*spacing_multiplier;

    const unsigned int max_n_planes = (unsigned int)(bricked_field->brick_size*max_voxel_extent/plane_separation.value) + 1;

    if (max_n_planes < 2)
        print_severe_message("Cannot create fewer than two planes.");

    configuration.plane_separation_multiplier = spacing_multiplier;

    if (plane_separation.original_value == 0.0f)
        plane_separation.original_value = plane_separation.value;

    if (max_n_planes > plane_stack.n_planes)
        update_number_of_planes(max_n_planes);

    sync_plane_separation();
}

float get_plane_separation(void)
{
    return configuration.plane_separation_multiplier;
}

size_t get_vertex_position_variable_number(void)
{
    return position_variable_number;
}

const Vector3f* get_unit_axis_aligned_box_corners(void)
{
    return corners;
}

unsigned int get_axis_aligned_box_back_corner_for_plane(const Vector3f* plane_normal)
{
    return back_corners[plane_normal->a[0] < 0][plane_normal->a[1] < 0][plane_normal->a[2] < 0];
}

unsigned int get_axis_aligned_box_front_corner_for_plane(const Vector3f* plane_normal)
{
    return front_corners[plane_normal->a[0] < 0][plane_normal->a[1] < 0][plane_normal->a[2] < 0];
}

void draw_active_bricked_field()
{
    const BrickedField* const bricked_field = active_bricked_field.bricked_field;

    check(active_shader_program);
    check(bricked_field);
    check(plane_stack.n_planes > 0);

    active_bricked_field.current_look_axis = get_camera_look_axis();
    active_bricked_field.current_camera_position = get_camera_position();

    active_bricked_field.current_back_corner_idx = get_axis_aligned_box_back_corner_for_plane(active_bricked_field.current_look_axis);
    active_bricked_field.current_front_corner_idx = opposite_corners[active_bricked_field.current_back_corner_idx];

    if (configuration.draw_field_outline)
        draw_field_boundary_indicator(bricked_field, active_bricked_field.current_back_corner_idx, INDICATOR_BACK_PASS);

    BrickTreeNode* node = bricked_field->tree;

    glUseProgram(active_shader_program->id);
    abort_on_GL_error("Could not use shader program");

    glUniform1ui(back_corner_idx_uniform.location, (GLuint)active_bricked_field.current_back_corner_idx);

    glBindVertexArray(plane_stack.vertex_array_object_id);
    abort_on_GL_error("Could not bind VAO for drawing");

    glActiveTexture(GL_TEXTURE0);
    abort_on_GL_error("Could not set active texture unit");

    draw_brick_tree_nodes(node);

    glBindVertexArray(0);

    glUseProgram(0);

    if (configuration.draw_sub_brick_outline)
        draw_sub_brick_boundary_indicator(bricked_field);

    if (configuration.draw_brick_outline)
        draw_brick_boundary_indicator(bricked_field);

    if (configuration.draw_field_outline)
        draw_field_boundary_indicator(bricked_field, active_bricked_field.current_front_corner_idx, INDICATOR_FRONT_PASS);

    active_bricked_field.current_look_axis = NULL;
    active_bricked_field.current_camera_position = NULL;
}

void cleanup_planes(void)
{
    cleanup_plane_stack();

    destroy_uniform(&plane_separation.uniform);

    destroy_uniform(&corners_uniform);
    destroy_uniform(&corner_permutations_uniform);
    destroy_uniform(&edge_starts_uniform);
    destroy_uniform(&edge_ends_uniform);
    destroy_uniform(&orientation_permutations_uniform);

    destroy_uniform(&brick_offset_uniform);
    destroy_uniform(&brick_extent_uniform);
    destroy_uniform(&pad_fractions_uniform);
    destroy_uniform(&subbrick_offset_uniform);
    destroy_uniform(&subbrick_extent_uniform);

    destroy_uniform(&back_plane_dist_uniform);
    destroy_uniform(&back_corner_idx_uniform);

    destroy_uniform(&orientation_uniform);

    destroy_uniform(&sampling_correction_uniform);

    active_bricked_field.bricked_field = NULL;
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

    unsigned int i, j, offset;
    for (i = 0; i < plane_stack.n_planes; i++)
    {
        PlaneVertex* const plane_vertices = plane_stack.plane_vertices[i].vertices;
        GLuint* const plane_face_indices = plane_stack.plane_faces[i].indices;

        for (j = 0; j < 6; j++)
        {
            plane_vertices[j].vertex_idx = j;
            plane_vertices[j].plane_idx = i;
        }

        offset = 6*i;

        plane_face_indices[ 0] = offset + 0;
        plane_face_indices[ 1] = offset + 2;
        plane_face_indices[ 2] = offset + 4;

        plane_face_indices[ 3] = offset + 0;
        plane_face_indices[ 4] = offset + 1;
        plane_face_indices[ 5] = offset + 2;

        plane_face_indices[ 6] = offset + 2;
        plane_face_indices[ 7] = offset + 3;
        plane_face_indices[ 8] = offset + 4;

        plane_face_indices[ 9] = offset + 4;
        plane_face_indices[10] = offset + 5;
        plane_face_indices[11] = offset + 0;
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

    // Store buffer of all face indices on device
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, plane_stack.face_buffer_id);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, (GLsizeiptr)plane_stack.face_buffer_size, (GLvoid*)plane_stack.face_buffer, GL_STATIC_DRAW);
    abort_on_GL_error("Could not bind IBO to VAO");

    glBindVertexArray(0);
}

static void generate_shader_code_for_planes(void)
{
    check(active_shader_program);

    const char* vertex_idx_name = plane_stack.plane_vertex_idx_name.chars;
    const char* plane_idx_name = plane_stack.plane_idx_name.chars;

    const char* plane_separation_name = plane_separation.uniform.name.chars;

    const char* corners_name = corners_uniform.name.chars;
    const char* corner_permutations_name = corner_permutations_uniform.name.chars;
    const char* edge_starts_name = edge_starts_uniform.name.chars;
    const char* edge_ends_name = edge_ends_uniform.name.chars;
    const char* orientation_permutations_name = orientation_permutations_uniform.name.chars;

    const char* brick_offset_name = brick_offset_uniform.name.chars;
    const char* brick_extent_name = brick_extent_uniform.name.chars;
    const char* pad_fractions_name = pad_fractions_uniform.name.chars;
    const char* sub_brick_offset_name = subbrick_offset_uniform.name.chars;
    const char* sub_brick_extent_name = subbrick_extent_uniform.name.chars;

    const char* back_plane_dist_name = back_plane_dist_uniform.name.chars;
    const char* back_corner_idx_name = back_corner_idx_uniform.name.chars;

    const char* orientation_name = orientation_uniform.name.chars;

    const char* sampling_correction_name = sampling_correction_uniform.name.chars;

    const char* look_axis_name = get_camera_look_axis_name();

    add_vertex_input_in_shader(&active_shader_program->vertex_shader_source, "uint", vertex_idx_name, 0);
    add_vertex_input_in_shader(&active_shader_program->vertex_shader_source, "uint", plane_idx_name, 1);

    add_uniform_in_shader(&active_shader_program->vertex_shader_source, "float", plane_separation_name);

    add_array_uniform_in_shader(&active_shader_program->vertex_shader_source, "vec3", corners_name, 8);
    add_array_uniform_in_shader(&active_shader_program->vertex_shader_source, "uint", corner_permutations_name, 64);
    add_array_uniform_in_shader(&active_shader_program->vertex_shader_source, "uint", edge_starts_name, 24);
    add_array_uniform_in_shader(&active_shader_program->vertex_shader_source, "uint", edge_ends_name, 24);
    add_array_uniform_in_shader(&active_shader_program->vertex_shader_source, "uint", orientation_permutations_name, 9);

    add_uniform_in_shader(&active_shader_program->vertex_shader_source, "vec3", brick_offset_name);
    add_uniform_in_shader(&active_shader_program->vertex_shader_source, "vec3", brick_extent_name);
    add_uniform_in_shader(&active_shader_program->vertex_shader_source, "vec3", pad_fractions_name);
    add_uniform_in_shader(&active_shader_program->vertex_shader_source, "vec3", sub_brick_offset_name);
    add_uniform_in_shader(&active_shader_program->vertex_shader_source, "vec3", sub_brick_extent_name);

    add_uniform_in_shader(&active_shader_program->vertex_shader_source, "float", back_plane_dist_name);
    add_uniform_in_shader(&active_shader_program->vertex_shader_source, "uint", back_corner_idx_name);

    add_uniform_in_shader(&active_shader_program->vertex_shader_source, "uint", orientation_name);

    DynamicString position_code = create_string(
      "    float plane_dist = %s + %s*%s;"
    "\n"
    "\n    vec4 position;"
    "\n"
    "\n    for (uint edge_idx = 0; edge_idx < 4; edge_idx++)"
    "\n    {"
    "\n        uint edge_start_idx = %s[4*vertex_idx + edge_idx];"
    "\n        uint edge_end_idx   =   %s[4*vertex_idx + edge_idx];"
    "\n"
    "\n        vec3 edge_start = %s*%s[%s[8*%s + edge_start_idx]];"
    "\n        vec3 edge_end   = %s*%s[%s[8*%s + edge_end_idx]];"
    "\n"
    "\n        vec3 edge_origin = edge_start + %s;"
    "\n        vec3 edge_vector = edge_end - edge_start;"
    "\n"
    "\n        float denom = dot(edge_vector, %s);"
    "\n        float lambda = (denom != 0.0) ? (plane_dist - dot(edge_origin, %s))/denom : -1.0;"
    "\n"
    "\n        if (lambda >= 0.0 && lambda <= 1.0)"
    "\n        {"
    "\n            position.xyz = edge_origin + lambda*edge_vector;"
    "\n            position.w = 1.0;"
    "\n            break;"
    "\n        }"
    "\n    }",
    back_plane_dist_name, plane_idx_name, plane_separation_name,
    edge_starts_name, edge_ends_name,
    sub_brick_extent_name, corners_name, corner_permutations_name, back_corner_idx_name,
    sub_brick_extent_name, corners_name, corner_permutations_name, back_corner_idx_name,
    sub_brick_offset_name,
    look_axis_name,
    look_axis_name);

    LinkedList global_dependencies = create_list();
    append_string_to_list(&global_dependencies, vertex_idx_name);
    append_string_to_list(&global_dependencies, plane_idx_name);
    append_string_to_list(&global_dependencies, plane_separation_name);
    append_string_to_list(&global_dependencies, corners_name);
    append_string_to_list(&global_dependencies, corner_permutations_name);
    append_string_to_list(&global_dependencies, edge_starts_name);
    append_string_to_list(&global_dependencies, edge_ends_name);
    append_string_to_list(&global_dependencies, sub_brick_offset_name);
    append_string_to_list(&global_dependencies, sub_brick_extent_name);
    append_string_to_list(&global_dependencies, back_plane_dist_name);
    append_string_to_list(&global_dependencies, back_corner_idx_name);
    append_string_to_list(&global_dependencies, look_axis_name);

    position_variable_number = add_variable_snippet_in_shader(&active_shader_program->vertex_shader_source,
                                                              "vec4", "position", position_code.chars,
                                                              &global_dependencies, NULL);

    clear_list(&global_dependencies);
    clear_string(&position_code);

    assign_transformed_variable_to_output_in_shader(&active_shader_program->vertex_shader_source,
                                                    get_transformation_name(), position_variable_number, "gl_Position");


    DynamicString tex_coord_code = create_string(
    "\n    vec3 tex_coord;"
    "\n    vec3 position_within_brick = (variable_%d.xyz - %s)/%s;"
    "\n    vec3 scale = vec3(1.0) - 2.0*%s;"
    "\n    for (uint component = 0; component < 3; component++)"
    "\n    {"
    "\n        uint permuted_component = %s[3*%s + component];"
    "\n        tex_coord[component] = scale[permuted_component]*position_within_brick[permuted_component] + %s[permuted_component];"
    "\n    }",
    position_variable_number, brick_offset_name, brick_extent_name,
    pad_fractions_name,
    orientation_permutations_name, orientation_name,
    pad_fractions_name);

    global_dependencies = create_list();
    append_string_to_list(&global_dependencies, brick_offset_name);
    append_string_to_list(&global_dependencies, brick_extent_name);
    append_string_to_list(&global_dependencies, pad_fractions_name);
    append_string_to_list(&global_dependencies, orientation_permutations_name);
    append_string_to_list(&global_dependencies, orientation_name);

    LinkedList tex_coord_variable_dependencies = create_list();
    append_size_t_to_list(&tex_coord_variable_dependencies, position_variable_number);

    tex_coord_variable_number = add_variable_snippet_in_shader(&active_shader_program->vertex_shader_source,
                                                               "vec3", "tex_coord", tex_coord_code.chars,
                                                               &global_dependencies, &tex_coord_variable_dependencies);

    clear_list(&global_dependencies);
    clear_list(&tex_coord_variable_dependencies);
    clear_string(&tex_coord_code);

    assign_variable_to_new_output_in_shader(&active_shader_program->vertex_shader_source,
                                            "vec3", tex_coord_variable_number, "out_tex_coord");
    add_input_in_shader(&active_shader_program->fragment_shader_source, "vec3", "out_tex_coord");

    add_uniform_in_shader(&active_shader_program->fragment_shader_source, "float", sampling_correction_name);
}

static void draw_brick_tree_nodes(BrickTreeNode* node)
{
    assert(node);

    // If the brick is invisible, stop traversal of this branch
    if (node->visibility_ratio <= configuration.lower_visibility_threshold)
    {
        node->visibility = REGION_INVISIBLE;
        return;
    }

    if (node->brick)
    {
        draw_brick(node->brick);
        node->visibility = REGION_VISIBLE;
    }
    else
    {
        assert(node->lower_child);
        assert(node->upper_child);

        // Bricks that are completely clipped away do not have to be drawn
        const int lower_is_clipped = axis_aligned_box_in_clipped_region(&node->lower_child->spatial_offset, &node->lower_child->spatial_extent);
        const int upper_is_clipped = axis_aligned_box_in_clipped_region(&node->upper_child->spatial_offset, &node->upper_child->spatial_extent);

        // In order to determine whether the upper or lower child should be drawn first,
        // we can compute the vector going from the a point on the plane separating the
        // children to the camera. If the dot product between this vector and the normal
        // of the separating plane is positive, the upper child is closer to the camera
        // than the lower child. Since the normal is just the unit x- y- or z-vector,
        // this amounts to comparing the corresponding component of the camera position
        // and (e.g.) the upper child offset.

        if (get_component_of_vector_from_model_point_to_camera(&node->upper_child->spatial_offset, node->split_axis) >= 0)
        {
            if (!lower_is_clipped)
                draw_brick_tree_nodes(node->lower_child);
            else
                node->lower_child->visibility = REGION_CLIPPED;

            if (!upper_is_clipped)
                draw_brick_tree_nodes(node->upper_child);
            else
                node->upper_child->visibility = REGION_CLIPPED;
        }
        else
        {
            if (!upper_is_clipped)
                draw_brick_tree_nodes(node->upper_child);
            else
                node->upper_child->visibility = REGION_CLIPPED;

            if (!lower_is_clipped)
                draw_brick_tree_nodes(node->lower_child);
            else
                node->lower_child->visibility = REGION_CLIPPED;
        }

        node->visibility = UNDETERMINED_REGION_VISIBILITY;
    }
}

static void draw_brick(const Brick* brick)
{
    assert(brick);

    // Update brick layout orientation
    glUniform1ui(orientation_uniform.location, (GLuint)brick->orientation);

    // Update offset to first brick corner
    glUniform3f(brick_offset_uniform.location,
                brick->spatial_offset.a[0],
                brick->spatial_offset.a[1],
                brick->spatial_offset.a[2]);

    // Update brick extent
    glUniform3f(brick_extent_uniform.location,
                brick->spatial_extent.a[0],
                brick->spatial_extent.a[1],
                brick->spatial_extent.a[2]);

    // Update brick pad fractions
    glUniform3f(pad_fractions_uniform.location,
                brick->pad_fractions.a[0],
                brick->pad_fractions.a[1],
                brick->pad_fractions.a[2]);

    glBindTexture(GL_TEXTURE_3D, brick->texture_id);
    abort_on_GL_error("Could not bind 3D texture");

    draw_sub_brick_tree_nodes(brick->tree);
}

static void draw_sub_brick_tree_nodes(SubBrickTreeNode* node)
{
    assert(node);

    if (node->visibility_ratio <= configuration.lower_visibility_threshold)
    {
        // If the sub brick is invisible, stop traversal of this branch
        node->visibility = REGION_INVISIBLE;
        return;
    }

    // If the sub brick is not sufficiently visible and it has children, traverse these recursively
    if (node->visibility_ratio < configuration.upper_visibility_threshold && node->lower_child)
    {
        assert(node->upper_child);

        // Sub bricks that are completely clipped away do not have to be drawn
        const int lower_is_clipped = axis_aligned_box_in_clipped_region(&node->lower_child->spatial_offset, &node->lower_child->spatial_extent);
        const int upper_is_clipped = axis_aligned_box_in_clipped_region(&node->upper_child->spatial_offset, &node->upper_child->spatial_extent);

        // Make sure to draw the children in the correct order (back to front)
        if (get_component_of_vector_from_model_point_to_camera(&node->upper_child->spatial_offset, node->split_axis) >= 0)
        {
            if (!lower_is_clipped)
                draw_sub_brick_tree_nodes(node->lower_child);
            else
                node->lower_child->visibility = REGION_CLIPPED;

            if (!upper_is_clipped)
                draw_sub_brick_tree_nodes(node->upper_child);
            else
                node->upper_child->visibility = REGION_CLIPPED;
        }
        else
        {
            if (!upper_is_clipped)
                draw_sub_brick_tree_nodes(node->upper_child);
            else
                node->upper_child->visibility = REGION_CLIPPED;

            if (!lower_is_clipped)
                draw_sub_brick_tree_nodes(node->lower_child);
            else
                node->lower_child->visibility = REGION_CLIPPED;
        }

        node->visibility = UNDETERMINED_REGION_VISIBILITY;
    }
    else
    {
        // If the sub brick is sufficiently visible or is a leaf node, draw it
        draw_sub_brick(node);
        node->visibility = REGION_VISIBLE;
    }
}

static void draw_sub_brick(const SubBrickTreeNode* node)
{
    assert(node);
    assert(active_bricked_field.current_look_axis);

    // Update offset to first sub brick corner
    glUniform3f(subbrick_offset_uniform.location,
                node->spatial_offset.a[0],
                node->spatial_offset.a[1],
                node->spatial_offset.a[2]);

    // Update sub brick extent
    glUniform3f(subbrick_extent_uniform.location,
                node->spatial_extent.a[0],
                node->spatial_extent.a[1],
                node->spatial_extent.a[2]);

    /*
    Project sub brick corners onto the look axis and find the one
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

    const float plane_dist_offset = dot3f(&node->spatial_offset, active_bricked_field.current_look_axis);

    const Vector3f scaled_back_corner = multiply_vector3f(corners + active_bricked_field.current_back_corner_idx, &node->spatial_extent);
    float back_plane_dist = dot3f(&scaled_back_corner, active_bricked_field.current_look_axis) + plane_dist_offset;

    const Vector3f scaled_front_corner = multiply_vector3f(corners + active_bricked_field.current_front_corner_idx, &node->spatial_extent);
    float front_plane_dist = dot3f(&scaled_front_corner, active_bricked_field.current_look_axis) + plane_dist_offset;

    // Offset start distance by half a plane spacing so that the first plane gets a non-zero area
    back_plane_dist += 0.5f*plane_separation.value;

    glUniform1f(back_plane_dist_uniform.location, (GLfloat)back_plane_dist);

    // Determine number of planes needed to traverse the brick from back to front along the view axis.
    // Also make sure it doesn't exceed the total number of available planes due to round-off errors.
    const unsigned int n_required_planes = uimin((unsigned int)((front_plane_dist - back_plane_dist)/plane_separation.value) + 1,
                                                 plane_stack.n_planes);

    draw_plane_faces(n_required_planes);
}

static void draw_plane_faces(unsigned int n_planes)
{
    assert(n_planes <= plane_stack.n_planes);
    assert(plane_stack.vertex_array_object_id > 0);

    glDrawElements(GL_TRIANGLES, (GLsizei)(12*n_planes), GL_UNSIGNED_INT, (GLvoid*)0);
    abort_on_GL_error("Could not draw planes");
}

static void sync_plane_separation(void)
{
    check(active_shader_program);

    glUseProgram(active_shader_program->id);
    abort_on_GL_error("Could not use shader program for updating plane uniforms");

    glUniform1f(plane_separation.uniform.location, plane_separation.value);
    abort_on_GL_error("Could not update plane separation uniform");

    glUniform1f(sampling_correction_uniform.location, plane_separation.value/plane_separation.original_value);
    abort_on_GL_error("Could not update sampling correction uniform");

    glUseProgram(0);
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
    plane_stack.vertex_buffer = NULL;
    plane_stack.face_buffer = NULL;

    plane_stack.vertex_buffer_size = 0;
    plane_stack.face_buffer_size = 0;

    plane_stack.plane_vertices = NULL;
    plane_stack.plane_faces = NULL;

    plane_stack.n_planes = 0;
}
