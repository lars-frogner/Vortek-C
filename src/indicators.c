#include "indicators.h"

#include "gl_includes.h"
#include "error.h"
#include "geometry.h"
#include "dynamic_string.h"
#include "hash_map.h"
#include "fields.h"
#include "transformation.h"
#include "field_textures.h"

#include <stdlib.h>
#include <stdarg.h>


typedef struct IndicatorVertices
{
    Vector4f* positions;
    Vector4f* colors;
    size_t n_vertices;
} IndicatorVertices;

typedef struct Indicator
{
    DynamicString name;
    IndicatorVertices vertices;
    void* vertex_buffer;
    size_t vertex_buffer_size;
    unsigned int* index_buffer;
    size_t n_indices;
    size_t index_buffer_size;
    GLuint vertex_array_object_id;
    GLuint vertex_buffer_id;
    GLuint index_buffer_id;
} Indicator;


static void generate_shader_code_for_indicators(void);

static const char* add_cube_edge_indicator(const Vector3f* lower_corner, const Vector3f* extent, const Vector4f* color, const char* name, ...);

static Indicator* create_indicator(const DynamicString* name, size_t n_vertices, size_t n_indices);
static Indicator* get_indicator(const char* name);

static void sync_indicator(Indicator* indicator);

static void initialize_vertex_array_object_for_indicator(Indicator* indicator);

static void allocate_indicator_buffers(Indicator* indicator, size_t n_vertices, size_t n_indices);

static void set_cube_vertex_positions(IndicatorVertices* vertices, size_t* running_vertex_idx, const Vector3f* lower_corner, const Vector3f* extent);
static void set_cube_edges(Indicator* indicator, size_t start_vertex_idx, size_t* running_index_idx);
static void set_vertex_colors(IndicatorVertices* vertices, size_t start_vertex_idx, size_t n_vertices, const Vector4f* color);

static void set_sub_brick_cube_data(Indicator* indicator, SubBrickTreeNode* node, size_t* running_vertex_idx, size_t* running_index_idx);

static void update_vertex_array_object_for_indicator(Indicator* indicator);

static void draw_brick_edges(const BrickTreeNode* node);
static void draw_sub_brick_edges(const SubBrickTreeNode* node);

static void destroy_indicator(Indicator* indicator);
static void destroy_vertex_array_object_for_indicator(Indicator* indicator);
static void clear_indicator(Indicator* indicator);
static void reset_indicator_attributes(Indicator* indicator);


// Indices for vertices outlining the edges of each cube face
static const unsigned int cube_edge_vertex_indices[24] = {0, 3, 6, 2,
                                                          4, 1, 5, 7,
                                                          0, 1, 4, 3,
                                                          6, 7, 5, 2,
                                                          0, 2, 5, 1,
                                                          6, 3, 4, 7};

// Sets of faces adjacent to each cube corner                      //    2----------5
static const unsigned int adjacent_cube_faces[8][3] = {{0, 2, 4},  //   /|         /|
                                                       {1, 2, 4},  //  / |       3/ |
                                                       {0, 3, 4},  // 6----------7 1|
                                                       {0, 2, 5},  // |  | 4   5 |  |
                                                       {1, 2, 5},  // |0 0-------|--1
                                                       {1, 3, 4},  // | /2       | /
                                                       {0, 3, 5},  // |/         |/
                                                       {1, 3, 5}}; // 3----------4

// Sign of the normal direction of each cube face
static const int cube_face_normal_signs[6] = {-1, 1, -1, 1, -1, 1};

static HashMap indicators;

static ShaderProgram* active_shader_program = NULL;


void set_active_shader_program_for_indicators(ShaderProgram* shader_program)
{
    active_shader_program = shader_program;
}

void initialize_indicators(void)
{
    indicators = create_map();
    generate_shader_code_for_indicators();
}

void add_boundary_indicator_for_field(const char* field_texture_name, const Vector4f* color)
{
    check(color);

    BrickedField* const bricked_field = get_texture_bricked_field(field_texture_name);
    Field* const field = bricked_field->field;

    Vector3f lower_corner = {{-field->halfwidth, -field->halfheight, -field->halfdepth}};
    Vector3f extent = {{2*field->halfwidth, 2*field->halfheight, 2*field->halfdepth}};

    bricked_field->field_boundary_indicator_name = add_cube_edge_indicator(&lower_corner, &extent, color, "%s_boundaries", field_texture_name);
}

void add_boundary_indicator_for_bricks(const char* field_texture_name, const Vector4f* color)
{
    check(color);

    BrickedField* const bricked_field = get_texture_bricked_field(field_texture_name);

    const DynamicString name = create_string("%s_brick_boundaries", field_texture_name);
    Indicator* const indicator = create_indicator(&name, 8*bricked_field->n_bricks, 24*bricked_field->n_bricks);

    size_t vertex_idx = 0;
    size_t index_idx = 0;

    for (size_t brick_idx = 0; brick_idx < bricked_field->n_bricks; brick_idx++)
    {
        const Brick* const brick = bricked_field->bricks + brick_idx;
        set_cube_edges(indicator, vertex_idx, &index_idx);
        set_cube_vertex_positions(&indicator->vertices, &vertex_idx, &brick->spatial_offset, &brick->spatial_extent);
    }

    set_vertex_colors(&indicator->vertices, 0, indicator->vertices.n_vertices, color);

    sync_indicator(indicator);

    bricked_field->brick_boundaries_indicator_name = indicator->name.chars;
}

void add_boundary_indicator_for_sub_bricks(const char* field_texture_name, const Vector4f* color)
{
    check(color);

    BrickedField* const bricked_field = get_texture_bricked_field(field_texture_name);

    size_t brick_idx;
    size_t n_sub_bricks = 0;

    for (brick_idx = 0; brick_idx < bricked_field->n_bricks; brick_idx++)
    {
        const Brick* const brick = bricked_field->bricks + brick_idx;
        n_sub_bricks += 1 + brick->tree->n_children;
    }

    const DynamicString name = create_string("%s_sub_brick_boundaries", field_texture_name);
    Indicator* const indicator = create_indicator(&name, 8*n_sub_bricks, 24*n_sub_bricks);

    size_t vertex_idx = 0;
    size_t index_idx = 0;

    for (brick_idx = 0; brick_idx < bricked_field->n_bricks; brick_idx++)
    {
        Brick* const brick = bricked_field->bricks + brick_idx;
        set_sub_brick_cube_data(indicator, brick->tree, &vertex_idx, &index_idx);
    }

    set_vertex_colors(&indicator->vertices, 0, indicator->vertices.n_vertices, color);

    sync_indicator(indicator);

    bricked_field->sub_brick_boundaries_indicator_name = indicator->name.chars;
}

void draw_field_boundary_indicator(const BrickedField* bricked_field, unsigned int reference_corner_idx, enum indicator_drawing_pass pass)
{
    assert(bricked_field);
    assert(reference_corner_idx < 8);

    if (!bricked_field->field_boundary_indicator_name)
        return;

    Indicator* const indicator = get_indicator(bricked_field->field_boundary_indicator_name);

    const Vector3f reference_corner = extract_vector3f_from_vector4f(indicator->vertices.positions + reference_corner_idx);

    assert(active_shader_program);
    glUseProgram(active_shader_program->id);
    abort_on_GL_error("Could not use shader program");

    assert(indicator->vertex_array_object_id > 0);
    glBindVertexArray(indicator->vertex_array_object_id);
    abort_on_GL_error("Could not bind VAO for drawing");

    unsigned int face_is_visible[6] = {0};

    for (unsigned int dim = 0; dim < 3; dim++)
    {
        const unsigned int adjacent_face_idx = adjacent_cube_faces[reference_corner_idx][dim];
        face_is_visible[adjacent_face_idx] = cube_face_normal_signs[adjacent_face_idx]*get_component_of_vector_from_model_point_to_camera(&reference_corner, dim) >= 0;
    }

    for (unsigned int face_idx = 0; face_idx < 6; face_idx++)
    {
        if ((pass == INDICATOR_FRONT_PASS && face_is_visible[face_idx]) || (pass == INDICATOR_BACK_PASS && !face_is_visible[face_idx]))
        {
            glDrawElements(GL_LINE_LOOP, 4, GL_UNSIGNED_INT, (GLvoid*)(4*face_idx*sizeof(unsigned int)));
            abort_on_GL_error("Could not draw indicator");
        }
    }

    glBindVertexArray(0);

    glUseProgram(0);
}

void draw_brick_boundary_indicator(const BrickedField* bricked_field)
{
    assert(bricked_field);

    if (!bricked_field->brick_boundaries_indicator_name)
        return;

    Indicator* const indicator = get_indicator(bricked_field->brick_boundaries_indicator_name);

    assert(active_shader_program);
    glUseProgram(active_shader_program->id);
    abort_on_GL_error("Could not use shader program");

    assert(indicator->vertex_array_object_id > 0);
    glBindVertexArray(indicator->vertex_array_object_id);
    abort_on_GL_error("Could not bind VAO for drawing");

    glDrawElements(GL_LINES, (GLsizei)indicator->n_indices, GL_UNSIGNED_INT, (GLvoid*)0);
    abort_on_GL_error("Could not draw indicator");

    glBindVertexArray(0);

    glUseProgram(0);
}

void draw_sub_brick_boundary_indicator(const BrickedField* bricked_field)
{
    assert(bricked_field);

    if (!bricked_field->sub_brick_boundaries_indicator_name)
        return;

    Indicator* const indicator = get_indicator(bricked_field->sub_brick_boundaries_indicator_name);

    assert(active_shader_program);
    glUseProgram(active_shader_program->id);
    abort_on_GL_error("Could not use shader program");

    assert(indicator->vertex_array_object_id > 0);
    glBindVertexArray(indicator->vertex_array_object_id);
    abort_on_GL_error("Could not bind VAO for drawing");

    draw_brick_edges(bricked_field->tree);

    glBindVertexArray(0);

    glUseProgram(0);
}

void destroy_edge_indicator(const char* name)
{
    Indicator* const indicator = get_indicator(name);
    destroy_indicator(indicator);
}

void cleanup_indicators(void)
{
    for (reset_map_iterator(&indicators); valid_map_iterator(&indicators); advance_map_iterator(&indicators))
    {
        Indicator* const indicator = get_indicator(get_current_map_key(&indicators));
        destroy_indicator(indicator);
    }

    destroy_map(&indicators);
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

static const char* add_cube_edge_indicator(const Vector3f* lower_corner, const Vector3f* extent, const Vector4f* color, const char* name, ...)
{
    check(lower_corner);
    check(extent);
    check(color);

    va_list args;
    va_start(args, name);
    const DynamicString full_name = create_string_from_arg_list(name, args);
    va_end(args);

    Indicator* const indicator = create_indicator(&full_name, 8, 24);

    size_t vertex_idx = 0;
    set_cube_vertex_positions(&indicator->vertices, &vertex_idx, lower_corner, extent);

    size_t index_idx = 0;
    set_cube_edges(indicator, 0, &index_idx);

    set_vertex_colors(&indicator->vertices, 0, 8, color);

    sync_indicator(indicator);

    return indicator->name.chars;
}

static Indicator* create_indicator(const DynamicString* name, size_t n_vertices, size_t n_indices)
{
    check(name);

    if (map_has_key(&indicators, name->chars))
        print_severe_message("Cannot create indicator \"%s\" because an indicator with this name already exists.", name);

    MapItem item = insert_new_map_item(&indicators, name->chars, sizeof(Indicator));
    Indicator* const indicator = (Indicator*)item.data;

    copy_string_attributes(name, &indicator->name);
    reset_indicator_attributes(indicator);
    initialize_vertex_array_object_for_indicator(indicator);
    allocate_indicator_buffers(indicator, n_vertices, n_indices);

    return indicator;
}

static Indicator* get_indicator(const char* name)
{
    check(name);

    MapItem item = get_map_item(&indicators, name);

    if (!item.data)
        print_severe_message("Could not get indicator \"%s\" because it doesn't exist.", name);

    assert(item.size == sizeof(Indicator));
    Indicator* const indicator = (Indicator*)item.data;
    check(indicator);

    return indicator;
}

static void sync_indicator(Indicator* indicator)
{
    update_vertex_array_object_for_indicator(indicator);
}

static void initialize_vertex_array_object_for_indicator(Indicator* indicator)
{
    assert(indicator);

    // Generate vertex array object for keeping track of vertex attributes
    glGenVertexArrays(1, &indicator->vertex_array_object_id);
    abort_on_GL_error("Could not generate VAO");

    glBindVertexArray(indicator->vertex_array_object_id);
    abort_on_GL_error("Could not bind VAO");

    // Generate buffer object for vertices
    glGenBuffers(1, &indicator->vertex_buffer_id);
    abort_on_GL_error("Could not generate vertex buffer object");

    // Generate buffer object for indices
    glGenBuffers(1, &indicator->index_buffer_id);
    abort_on_GL_error("Could not generate index buffer object");

    glBindVertexArray(0);
}

static void allocate_indicator_buffers(Indicator* indicator, size_t n_vertices, size_t n_indices)
{
    assert(indicator);

    if (indicator->vertex_buffer || indicator->index_buffer)
        clear_indicator(indicator);

    indicator->vertices.n_vertices = n_vertices;
    indicator->n_indices = n_indices;

    indicator->vertex_buffer_size = 2*n_vertices*sizeof(Vector4f);
    indicator->index_buffer_size = n_indices*sizeof(unsigned int);

    indicator->vertex_buffer = malloc(indicator->vertex_buffer_size);
    if (!indicator->vertex_buffer)
        print_severe_message("Could not allocate memory for indicator vertices.");

    indicator->index_buffer = (unsigned int*)malloc(indicator->index_buffer_size);
    if (!indicator->index_buffer)
        print_severe_message("Could not allocate memory for indicator indices.");

    indicator->vertices.positions = (Vector4f*)indicator->vertex_buffer;
    indicator->vertices.colors = indicator->vertices.positions + n_vertices;
}

static void set_cube_vertex_positions(IndicatorVertices* vertices, size_t* running_vertex_idx, const Vector3f* lower_corner, const Vector3f* extent)
{
    assert(vertices);
    assert(running_vertex_idx);
    assert(*running_vertex_idx + 8 <= vertices->n_vertices);
    assert(lower_corner);
    assert(extent);

    size_t vertex_idx = *running_vertex_idx;

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

static void set_cube_edges(Indicator* indicator, size_t start_vertex_idx, size_t* running_index_idx)
{
    assert(indicator);
    assert(start_vertex_idx + 8 <= indicator->vertices.n_vertices);
    assert(running_index_idx);
    assert(*running_index_idx + 24 <= indicator->n_indices);

    size_t start_index_idx = *running_index_idx;

    for(size_t index_idx = 0; index_idx < 24; index_idx++)
    {
        indicator->index_buffer[start_index_idx + index_idx] = (unsigned int)(start_vertex_idx + cube_edge_vertex_indices[index_idx]);
    }

    *running_index_idx = start_index_idx + 24;
}

static void set_vertex_colors(IndicatorVertices* vertices, size_t start_vertex_idx, size_t n_vertices, const Vector4f* color)
{
    assert(vertices);
    assert(start_vertex_idx + n_vertices <= vertices->n_vertices);
    assert(color);

    for (size_t vertex_idx = start_vertex_idx; vertex_idx < start_vertex_idx + n_vertices; vertex_idx++)
        copy_vector4f(color, vertices->colors + vertex_idx);
}

static void set_sub_brick_cube_data(Indicator* indicator, SubBrickTreeNode* node, size_t* running_vertex_idx, size_t* running_index_idx)
{
    assert(indicator);
    assert(node);
    assert(running_vertex_idx);
    assert(running_index_idx);

    if (node->lower_child)
        set_sub_brick_cube_data(indicator, node->lower_child, running_vertex_idx, running_index_idx);

    if (node->upper_child)
        set_sub_brick_cube_data(indicator, node->upper_child, running_vertex_idx, running_index_idx);

    node->indicator_idx = *running_index_idx;
    set_cube_edges(indicator, *running_vertex_idx, running_index_idx);
    set_cube_vertex_positions(&indicator->vertices, running_vertex_idx, &node->spatial_offset, &node->spatial_extent);
}

static void update_vertex_array_object_for_indicator(Indicator* indicator)
{
    assert(indicator);
    assert(indicator->vertex_buffer);
    assert(indicator->index_buffer);
    assert(indicator->vertex_array_object_id > 0);
    assert(indicator->vertex_buffer_id > 0);
    assert(indicator->index_buffer_id > 0);

    glBindVertexArray(indicator->vertex_array_object_id);
    abort_on_GL_error("Could not bind VAO");

    // Store buffer of all vertices on device
    glBindBuffer(GL_ARRAY_BUFFER, indicator->vertex_buffer_id);
    glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)indicator->vertex_buffer_size, (GLvoid*)indicator->vertex_buffer, GL_STATIC_DRAW);
    abort_on_GL_error("Could not bind VBO to VAO");

    // Specify vertex attribute pointer for vertex positions
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, (GLvoid*)0);
    glEnableVertexAttribArray(0);
    abort_on_GL_error("Could not set VAO vertex index attributes");

    // Specify vertex attribute pointer for vertex colors
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 0, (GLvoid*)(sizeof(Vector4f)*indicator->vertices.n_vertices));
    glEnableVertexAttribArray(1);
    abort_on_GL_error("Could not set VAO plane index attributes");

    // Store buffer of all edge indices on device
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indicator->index_buffer_id);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, (GLsizeiptr)indicator->index_buffer_size, (GLvoid*)indicator->index_buffer, GL_STATIC_DRAW);
    abort_on_GL_error("Could not bind IBO to VAO");

    glBindVertexArray(0);
}

static void draw_brick_edges(const BrickTreeNode* node)
{
    assert(node);

    if (node->visibility == REGION_INVISIBLE || node->visibility == REGION_CLIPPED)
        return;

    if (node->brick)
    {
        draw_sub_brick_edges(node->brick->tree);
    }
    else
    {
        if (node->lower_child)
            draw_brick_edges(node->lower_child);

        if (node->upper_child)
            draw_brick_edges(node->upper_child);
    }
}

static void draw_sub_brick_edges(const SubBrickTreeNode* node)
{
    assert(node);

    if (node->visibility == REGION_INVISIBLE || node->visibility == REGION_CLIPPED)
        return;

    if (node->visibility == REGION_VISIBLE)
    {
        glDrawElements(GL_LINE_LOOP, 24, GL_UNSIGNED_INT, (GLvoid*)(node->indicator_idx*sizeof(unsigned int)));
        abort_on_GL_error("Could not draw indicator");
    }
    else
    {
        if (node->lower_child)
            draw_sub_brick_edges(node->lower_child);

        if (node->upper_child)
            draw_sub_brick_edges(node->upper_child);
    }
}

static void destroy_indicator(Indicator* indicator)
{
    destroy_vertex_array_object_for_indicator(indicator);
    clear_indicator(indicator);
    clear_string(&indicator->name);
}

static void destroy_vertex_array_object_for_indicator(Indicator* indicator)
{
    assert(indicator);

    if (indicator->index_buffer_id != 0)
        glDeleteBuffers(1, &indicator->index_buffer_id);

    if (indicator->vertex_buffer_id != 0)
        glDeleteBuffers(1, &indicator->vertex_buffer_id);

    if (indicator->vertex_array_object_id != 0)
        glDeleteVertexArrays(1, &indicator->vertex_array_object_id);

    abort_on_GL_error("Could not destroy buffer objects");
}

static void clear_indicator(Indicator* indicator)
{
    assert(indicator);

    if (indicator->vertex_buffer)
        free(indicator->vertex_buffer);

    if (indicator->index_buffer)
        free(indicator->index_buffer);

    reset_indicator_attributes(indicator);
}

static void reset_indicator_attributes(Indicator* indicator)
{
    assert(indicator);

    indicator->vertices.positions = NULL;
    indicator->vertices.colors = NULL;
    indicator->vertices.n_vertices = 0;

    indicator->vertex_buffer = NULL;
    indicator->vertex_buffer_size = 0;

    indicator->index_buffer = NULL;
    indicator->n_indices = 0;
    indicator->index_buffer_size = 0;
}
