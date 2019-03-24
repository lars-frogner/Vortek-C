#include "indicators.h"

#include "gl_includes.h"
#include "error.h"
#include "geometry.h"
#include "dynamic_string.h"
#include "hash_map.h"
#include "transformation.h"

#include <stdlib.h>
#include <stdarg.h>


typedef struct IndicatorEdge
{
    GLuint indices[2];
} IndicatorEdge;

typedef struct EdgeSet
{
    DynamicString name;
    IndicatorVertices vertices;
    IndicatorEdge* edges;
    unsigned int n_edges;
    void* vertex_buffer;
    void* edge_buffer;
    size_t vertex_buffer_size;
    size_t edge_buffer_size;
    GLuint vertex_array_object_id;
    GLuint vertex_buffer_id;
    GLuint edge_buffer_id;
} EdgeSet;


static void generate_shader_code_for_indicators(void);

static EdgeSet* create_edge_set(const DynamicString* name, unsigned int n_vertices, unsigned int n_edges);
static EdgeSet* get_edge_set(const char* name);

static void initialize_vertex_array_object_for_edge_set(EdgeSet* edge_set);

static void allocate_edge_set_buffers(EdgeSet* edge_set, unsigned int n_vertices, unsigned int n_edges);

static void set_cube_vertex_positions(IndicatorVertices* vertices, unsigned int* running_vertex_idx, const Vector3f* lower_corner, const Vector3f* extent);
static void set_cube_edges(EdgeSet* edge_set, unsigned int* running_vertex_idx, unsigned int* running_edge_idx);
static void set_vertex_colors(IndicatorVertices* vertices, unsigned int start_idx, unsigned int n_vertices, const Vector3f* color);

static void update_vertex_array_object_for_edge_set(EdgeSet* edge_set);

static void destroy_edge_set(EdgeSet* edge_set);
static void destroy_vertex_array_object_for_edge_set(EdgeSet* edge_set);
static void clear_edge_set(EdgeSet* edge_set);
static void reset_edge_set_attributes(EdgeSet* edge_set);


static HashMap edge_sets;

static ShaderProgram* active_shader_program = NULL;


void set_active_shader_program_for_indicators(ShaderProgram* shader_program)
{
    active_shader_program = shader_program;
}

void initialize_indicators(void)
{
    edge_sets = create_map();
    generate_shader_code_for_indicators();
}

IndicatorVertices* access_edge_indicator_vertices(const char* name)
{
    EdgeSet* const edge_set = get_edge_set(name);
    return &edge_set->vertices;
}

const char* add_cube_edge_indicator(const Vector3f* lower_corner, const Vector3f* extent, const Vector3f* color, const char* name, ...)
{
    check(lower_corner);
    check(extent);
    check(color);

    va_list args;
    va_start(args, name);
    const DynamicString full_name = create_string_from_arg_list(name, args);
    va_end(args);

    EdgeSet* const edge_set = create_edge_set(&full_name, 8, 12);

    unsigned int start_vertex_idx = 0;
    set_cube_vertex_positions(&edge_set->vertices, &start_vertex_idx, lower_corner, extent);

    start_vertex_idx = 0;
    unsigned int edge_idx = 0;
    set_cube_edges(edge_set, &start_vertex_idx, &edge_idx);

    set_vertex_colors(&edge_set->vertices, 0, 8, color);

    update_vertex_array_object_for_edge_set(edge_set);

    return edge_set->name.chars;
}

void set_cube_edge_indicator_vertex_positions(const char* name, const Vector3f* lower_corner, const Vector3f* extent)
{
    EdgeSet* const edge_set = get_edge_set(name);

    unsigned int start_vertex_idx = 0;
    set_cube_vertex_positions(&edge_set->vertices, &start_vertex_idx, lower_corner, extent);

    update_vertex_array_object_for_edge_set(edge_set);
}

void set_edge_indicator_constant_color(const char* name, const Vector3f* color)
{
    EdgeSet* const edge_set = get_edge_set(name);
    set_vertex_colors(&edge_set->vertices, 0, 8, color);
    update_vertex_array_object_for_edge_set(edge_set);
}

void sync_edge_indicator(const char* name)
{
    EdgeSet* const edge_set = get_edge_set(name);
    update_vertex_array_object_for_edge_set(edge_set);
}

void draw_edge_indicator(const char* name)
{
    EdgeSet* const edge_set = get_edge_set(name);

    assert(edge_set->vertex_array_object_id > 0);

    glUseProgram(active_shader_program->id);
    abort_on_GL_error("Could not use shader program");

    glBindVertexArray(edge_set->vertex_array_object_id);
    abort_on_GL_error("Could not bind VAO for drawing");

    glDrawElements(GL_LINES, (GLsizei)(2*edge_set->n_edges), GL_UNSIGNED_INT, (GLvoid*)0);
    abort_on_GL_error("Could not draw edge set");

    glBindVertexArray(0);

    glUseProgram(0);
}

void destroy_edge_indicator(const char* name)
{
    EdgeSet* const edge_set = get_edge_set(name);
    destroy_edge_set(edge_set);
}

void cleanup_indicators(void)
{
    for (reset_map_iterator(&edge_sets); valid_map_iterator(&edge_sets); advance_map_iterator(&edge_sets))
    {
        EdgeSet* const edge_set = get_edge_set(get_current_map_key(&edge_sets));
        destroy_edge_set(edge_set);
    }

    destroy_map(&edge_sets);
}

static void generate_shader_code_for_indicators(void)
{
    check(active_shader_program);

    add_vertex_input_in_shader(&active_shader_program->vertex_shader_source, "vec4", "in_position", 0);
    add_vertex_input_in_shader(&active_shader_program->vertex_shader_source, "vec4", "in_color", 1);

    assign_transformed_input_to_output_in_shader(&active_shader_program->vertex_shader_source,
                                                 get_transformation_name(), "in_position", "gl_Position");

    assign_input_to_new_output_in_shader(&active_shader_program->vertex_shader_source, "vec4", "in_color", "ex_color");

    add_input_in_shader(&active_shader_program->fragment_shader_source, "vec4", "ex_color");
    assign_input_to_new_output_in_shader(&active_shader_program->fragment_shader_source, "vec4", "ex_color", "out_color");
}

static EdgeSet* create_edge_set(const DynamicString* name, unsigned int n_vertices, unsigned int n_edges)
{
    check(name);

    if (map_has_key(&edge_sets, name->chars))
        print_severe_message("Cannot create edge set \"%s\" because an edge set with this name already exists.", name);

    MapItem item = insert_new_map_item(&edge_sets, name->chars, sizeof(EdgeSet));
    EdgeSet* const edge_set = (EdgeSet*)item.data;

    copy_string_attributes(name, &edge_set->name);
    reset_edge_set_attributes(edge_set);
    initialize_vertex_array_object_for_edge_set(edge_set);
    allocate_edge_set_buffers(edge_set, n_vertices, n_edges);

    return edge_set;
}

static EdgeSet* get_edge_set(const char* name)
{
    check(name);

    MapItem item = get_map_item(&edge_sets, name);

    if (!item.data)
        print_severe_message("Could not get edge set \"%s\" because it doesn't exist.", name);

    assert(item.size == sizeof(EdgeSet));
    EdgeSet* const edge_set = (EdgeSet*)item.data;
    check(edge_set);

    return edge_set;
}

static void initialize_vertex_array_object_for_edge_set(EdgeSet* edge_set)
{
    assert(edge_set);

    // Generate vertex array object for keeping track of vertex attributes
    glGenVertexArrays(1, &edge_set->vertex_array_object_id);
    abort_on_GL_error("Could not generate VAO");

    glBindVertexArray(edge_set->vertex_array_object_id);
    abort_on_GL_error("Could not bind VAO");

    // Generate buffer object for vertices
    glGenBuffers(1, &edge_set->vertex_buffer_id);
    abort_on_GL_error("Could not generate vertex buffer object");

    // Generate buffer object for face indices
    glGenBuffers(1, &edge_set->edge_buffer_id);
    abort_on_GL_error("Could not generate face buffer object");

    glBindVertexArray(0);
}

static void allocate_edge_set_buffers(EdgeSet* edge_set, unsigned int n_vertices, unsigned int n_edges)
{
    assert(edge_set);

    if (edge_set->vertex_buffer || edge_set->edge_buffer)
        clear_edge_set(edge_set);

    edge_set->vertices.n_vertices = n_vertices;
    edge_set->n_edges = n_edges;

    edge_set->vertex_buffer_size = 2*n_vertices*sizeof(Vector4f);
    edge_set->edge_buffer_size = n_edges*sizeof(IndicatorEdge);

    edge_set->vertex_buffer = malloc(edge_set->vertex_buffer_size);
    if (!edge_set->vertex_buffer)
        print_severe_message("Could not allocate memory for edge set vertices.");

    edge_set->edge_buffer = malloc(edge_set->edge_buffer_size);
    if (!edge_set->edge_buffer)
        print_severe_message("Could not allocate memory for edge set edges.");

    edge_set->vertices.positions = (Vector4f*)edge_set->vertex_buffer;
    edge_set->vertices.colors = edge_set->vertices.positions + n_vertices;

    edge_set->edges = (IndicatorEdge*)edge_set->edge_buffer;
}

static void set_cube_vertex_positions(IndicatorVertices* vertices, unsigned int* running_vertex_idx, const Vector3f* lower_corner, const Vector3f* extent)
{
    assert(vertices);
    assert(running_vertex_idx);
    assert(*running_vertex_idx + 8 <= vertices->n_vertices);
    assert(lower_corner);
    assert(extent);

    //    2----------5
    //   /|         /|
    //  / |        / |
    // 6----------7  |
    // |  |       |  |
    // |  0-------|--1
    // | /        | /
    // |/         |/
    // 3----------4

    unsigned int vertex_idx = *running_vertex_idx;

    set_vector4f_elements(vertices->positions + vertex_idx, lower_corner->a[0],                lower_corner->a[1],                lower_corner->a[2],                1.0f);
    vertex_idx++;
    set_vector4f_elements(vertices->positions + vertex_idx, lower_corner->a[0] + extent->a[0], lower_corner->a[1],                lower_corner->a[2],                1.0f);
    vertex_idx++;
    set_vector4f_elements(vertices->positions + vertex_idx, lower_corner->a[0],                lower_corner->a[1] + extent->a[1], lower_corner->a[2],                1.0f);
    vertex_idx++;
    set_vector4f_elements(vertices->positions + vertex_idx, lower_corner->a[0],                lower_corner->a[1],                lower_corner->a[2] + extent->a[2], 1.0f);
    vertex_idx++;
    set_vector4f_elements(vertices->positions + vertex_idx, lower_corner->a[0] + extent->a[0], lower_corner->a[1],                lower_corner->a[2] + extent->a[2], 1.0f);
    vertex_idx++;
    set_vector4f_elements(vertices->positions + vertex_idx, lower_corner->a[0] + extent->a[0], lower_corner->a[1] + extent->a[1], lower_corner->a[2],                1.0f);
    vertex_idx++;
    set_vector4f_elements(vertices->positions + vertex_idx, lower_corner->a[0],                lower_corner->a[1] + extent->a[1], lower_corner->a[2] + extent->a[2], 1.0f);
    vertex_idx++;
    set_vector4f_elements(vertices->positions + vertex_idx, lower_corner->a[0] + extent->a[0], lower_corner->a[1] + extent->a[1], lower_corner->a[2] + extent->a[2], 1.0f);
    vertex_idx++;

    *running_vertex_idx = vertex_idx;
}

static void set_cube_edges(EdgeSet* edge_set, unsigned int* running_vertex_idx, unsigned int* running_edge_idx)
{
    assert(edge_set);
    assert(running_vertex_idx);
    assert(*running_vertex_idx + 8 <= edge_set->vertices.n_vertices);
    assert(running_edge_idx);
    assert(*running_edge_idx + 12 <= edge_set->n_edges);

    unsigned int start_vertex_idx = *running_vertex_idx;
    unsigned int edge_idx = *running_edge_idx;

    edge_set->edges[edge_idx].indices[0] = start_vertex_idx + 0;
    edge_set->edges[edge_idx].indices[1] = start_vertex_idx + 1;
    edge_idx++;
    edge_set->edges[edge_idx].indices[0] = start_vertex_idx + 1;
    edge_set->edges[edge_idx].indices[1] = start_vertex_idx + 5;
    edge_idx++;
    edge_set->edges[edge_idx].indices[0] = start_vertex_idx + 5;
    edge_set->edges[edge_idx].indices[1] = start_vertex_idx + 2;
    edge_idx++;
    edge_set->edges[edge_idx].indices[0] = start_vertex_idx + 2;
    edge_set->edges[edge_idx].indices[1] = start_vertex_idx + 0;
    edge_idx++;
    edge_set->edges[edge_idx].indices[0] = start_vertex_idx + 0;
    edge_set->edges[edge_idx].indices[1] = start_vertex_idx + 3;
    edge_idx++;
    edge_set->edges[edge_idx].indices[0] = start_vertex_idx + 3;
    edge_set->edges[edge_idx].indices[1] = start_vertex_idx + 6;
    edge_idx++;
    edge_set->edges[edge_idx].indices[0] = start_vertex_idx + 6;
    edge_set->edges[edge_idx].indices[1] = start_vertex_idx + 2;
    edge_idx++;
    edge_set->edges[edge_idx].indices[0] = start_vertex_idx + 1;
    edge_set->edges[edge_idx].indices[1] = start_vertex_idx + 4;
    edge_idx++;
    edge_set->edges[edge_idx].indices[0] = start_vertex_idx + 4;
    edge_set->edges[edge_idx].indices[1] = start_vertex_idx + 7;
    edge_idx++;
    edge_set->edges[edge_idx].indices[0] = start_vertex_idx + 7;
    edge_set->edges[edge_idx].indices[1] = start_vertex_idx + 5;
    edge_idx++;
    edge_set->edges[edge_idx].indices[0] = start_vertex_idx + 3;
    edge_set->edges[edge_idx].indices[1] = start_vertex_idx + 4;
    edge_idx++;
    edge_set->edges[edge_idx].indices[0] = start_vertex_idx + 6;
    edge_set->edges[edge_idx].indices[1] = start_vertex_idx + 7;
    edge_idx++;

    *running_vertex_idx = start_vertex_idx + 8;
    *running_edge_idx = edge_idx;
}

static void set_vertex_colors(IndicatorVertices* vertices, unsigned int start_idx, unsigned int n_vertices, const Vector3f* color)
{
    assert(vertices);
    assert(start_idx + n_vertices <= vertices->n_vertices);
    assert(color);

    for (unsigned int vertex_idx = start_idx; vertex_idx < start_idx + n_vertices; vertex_idx++)
        copy_vector3f_to_vector4f(color, vertices->colors + vertex_idx);
}

static void update_vertex_array_object_for_edge_set(EdgeSet* edge_set)
{
    assert(edge_set);
    assert(edge_set->vertex_buffer);
    assert(edge_set->edge_buffer);
    assert(edge_set->vertex_array_object_id > 0);
    assert(edge_set->vertex_buffer_id > 0);
    assert(edge_set->edge_buffer_id > 0);

    glBindVertexArray(edge_set->vertex_array_object_id);
    abort_on_GL_error("Could not bind VAO");

    // Store buffer of all vertices on device
    glBindBuffer(GL_ARRAY_BUFFER, edge_set->vertex_buffer_id);
    glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)edge_set->vertex_buffer_size, (GLvoid*)edge_set->vertex_buffer, GL_STATIC_DRAW);
    abort_on_GL_error("Could not bind VBO to VAO");

    // Specify vertex attribute pointer for vertex positions
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, (GLvoid*)0);
    glEnableVertexAttribArray(0);
    abort_on_GL_error("Could not set VAO vertex index attributes");

    // Specify vertex attribute pointer for vertex colors
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 0, (GLvoid*)(sizeof(Vector4f)*edge_set->vertices.n_vertices));
    glEnableVertexAttribArray(1);
    abort_on_GL_error("Could not set VAO plane index attributes");

    // Store buffer of all edge indices on device
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, edge_set->edge_buffer_id);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, (GLsizeiptr)edge_set->edge_buffer_size, (GLvoid*)edge_set->edge_buffer, GL_STATIC_DRAW);
    abort_on_GL_error("Could not bind IBO to VAO");

    glBindVertexArray(0);
}

static void destroy_edge_set(EdgeSet* edge_set)
{
    destroy_vertex_array_object_for_edge_set(edge_set);
    clear_edge_set(edge_set);
    clear_string(&edge_set->name);
}

static void destroy_vertex_array_object_for_edge_set(EdgeSet* edge_set)
{
    assert(edge_set);

    if (edge_set->edge_buffer_id != 0)
        glDeleteBuffers(1, &edge_set->edge_buffer_id);

    if (edge_set->vertex_buffer_id != 0)
        glDeleteBuffers(1, &edge_set->vertex_buffer_id);

    if (edge_set->vertex_array_object_id != 0)
        glDeleteVertexArrays(1, &edge_set->vertex_array_object_id);

    abort_on_GL_error("Could not destroy buffer objects");
}

static void clear_edge_set(EdgeSet* edge_set)
{
    assert(edge_set);

    if (edge_set->vertex_buffer)
        free(edge_set->vertex_buffer);

    if (edge_set->edge_buffer)
        free(edge_set->edge_buffer);

    reset_edge_set_attributes(edge_set);
}

static void reset_edge_set_attributes(EdgeSet* edge_set)
{
    assert(edge_set);

    edge_set->vertices.positions = NULL;
    edge_set->vertices.colors = NULL;
    edge_set->vertices.n_vertices = 0;

    edge_set->edges = NULL;
    edge_set->n_edges = 0;

    edge_set->vertex_buffer = NULL;
    edge_set->edge_buffer = NULL;

    edge_set->vertex_buffer_size = 0;
    edge_set->edge_buffer_size = 0;
}
