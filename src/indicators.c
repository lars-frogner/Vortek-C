#include "indicators.h"

#include "error.h"
#include "hash_map.h"
#include "transformation.h"

#include <stdlib.h>
#include <stdarg.h>


static void generate_shader_code_for_indicators(void);

static void initialize_vertex_array_object_for_indicator(Indicator* indicator);

static void allocate_indicator_buffers(Indicator* indicator, size_t n_vertices, size_t n_indices);

static void destroy_vertex_array_object_for_indicator(Indicator* indicator);
static void clear_indicator(Indicator* indicator);
static void reset_indicator_attributes(Indicator* indicator);


static HashMap indicators;

static ShaderProgram* active_shader_program = NULL;

// Indices for vertices outlining the edges of each cube face
static const unsigned int cube_edge_vertex_indices[24] = {0, 3, 6, 2,
                                                          4, 1, 5, 7,
                                                          0, 1, 4, 3,
                                                          6, 7, 5, 2,
                                                          0, 2, 5, 1,
                                                          6, 3, 4, 7};


void set_active_shader_program_for_indicators(ShaderProgram* shader_program)
{
    active_shader_program = shader_program;
}

void initialize_indicators(void)
{
    indicators = create_map();
    generate_shader_code_for_indicators();
}

Indicator* create_indicator(const DynamicString* name, size_t n_vertices, size_t n_indices)
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

Indicator* get_indicator(const char* name)
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

GLuint get_active_indicator_shader_program_id(void)
{
    assert(active_shader_program);
    return active_shader_program->id;
}

void set_vertex_colors_for_indicator(Indicator* indicator, size_t start_vertex_idx, size_t n_vertices, const Color* color)
{
    assert(indicator);
    assert(start_vertex_idx + n_vertices <= indicator->n_vertices);
    assert(color);

    for (size_t vertex_idx = start_vertex_idx; vertex_idx < start_vertex_idx + n_vertices; vertex_idx++)
        indicator->vertices.colors[vertex_idx] = *color;
}

void set_cube_edges_for_indicator(Indicator* indicator, size_t start_vertex_idx, size_t* running_index_idx)
{
    assert(indicator);
    assert(start_vertex_idx + 8 <= indicator->n_vertices);
    assert(running_index_idx);
    assert(*running_index_idx + 24 <= indicator->n_indices);

    size_t start_index_idx = *running_index_idx;

    for(size_t index_idx = 0; index_idx < 24; index_idx++)
    {
        indicator->index_buffer[start_index_idx + index_idx] = (unsigned int)(start_vertex_idx + cube_edge_vertex_indices[index_idx]);
    }

    *running_index_idx = start_index_idx + 24;
}

void set_cube_vertex_positions_for_indicator(Indicator* indicator, size_t* running_vertex_idx, const Vector3f* lower_corner, const Vector3f* extent)
{
    assert(indicator);
    assert(running_vertex_idx);
    assert(*running_vertex_idx + 8 <= indicator->n_vertices);
    assert(lower_corner);
    assert(extent);

    size_t vertex_idx = *running_vertex_idx;

    set_vector4f_elements(indicator->vertices.positions + vertex_idx, lower_corner->a[0],                lower_corner->a[1],                lower_corner->a[2],                1.0f);
    vertex_idx++;
    set_vector4f_elements(indicator->vertices.positions + vertex_idx, lower_corner->a[0] + extent->a[0], lower_corner->a[1],                lower_corner->a[2],                1.0f);
    vertex_idx++;
    set_vector4f_elements(indicator->vertices.positions + vertex_idx, lower_corner->a[0],                lower_corner->a[1] + extent->a[1], lower_corner->a[2],                1.0f);
    vertex_idx++;
    set_vector4f_elements(indicator->vertices.positions + vertex_idx, lower_corner->a[0],                lower_corner->a[1],                lower_corner->a[2] + extent->a[2], 1.0f);
    vertex_idx++;
    set_vector4f_elements(indicator->vertices.positions + vertex_idx, lower_corner->a[0] + extent->a[0], lower_corner->a[1],                lower_corner->a[2] + extent->a[2], 1.0f);
    vertex_idx++;
    set_vector4f_elements(indicator->vertices.positions + vertex_idx, lower_corner->a[0] + extent->a[0], lower_corner->a[1] + extent->a[1], lower_corner->a[2],                1.0f);
    vertex_idx++;
    set_vector4f_elements(indicator->vertices.positions + vertex_idx, lower_corner->a[0],                lower_corner->a[1] + extent->a[1], lower_corner->a[2] + extent->a[2], 1.0f);
    vertex_idx++;
    set_vector4f_elements(indicator->vertices.positions + vertex_idx, lower_corner->a[0] + extent->a[0], lower_corner->a[1] + extent->a[1], lower_corner->a[2] + extent->a[2], 1.0f);
    vertex_idx++;

    *running_vertex_idx = vertex_idx;
}

void load_buffer_data_for_indicator(Indicator* indicator)
{
    assert(indicator);
    assert(indicator->vertex_buffer);
    assert(indicator->index_buffer);
    assert(indicator->vertex_array_object_id > 0);
    assert(indicator->vertex_buffer_id > 0);
    assert(indicator->index_buffer_id > 0);

    glBindVertexArray(indicator->vertex_array_object_id);
    abort_on_GL_error("Could not bind VAO for indicator");

    // Store buffer of all vertices on device
    glBindBuffer(GL_ARRAY_BUFFER, indicator->vertex_buffer_id);
    glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)indicator->vertex_buffer_size, (GLvoid*)indicator->vertex_buffer, GL_STATIC_DRAW);
    abort_on_GL_error("Could not load vertex buffer data for indicator");

    // Specify vertex attribute pointer for vertex positions
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, (GLvoid*)0);
    glEnableVertexAttribArray(0);
    abort_on_GL_error("Could not set position vertex attribute pointer for indicator");

    // Specify vertex attribute pointer for vertex colors
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 0, (GLvoid*)(sizeof(Vector4f)*indicator->n_vertices));
    glEnableVertexAttribArray(1);
    abort_on_GL_error("Could not set color vertex attribute pointer for indicator");

    // Store buffer of all edge indices on device
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indicator->index_buffer_id);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, (GLsizeiptr)indicator->index_buffer_size, (GLvoid*)indicator->index_buffer, GL_STATIC_DRAW);
    abort_on_GL_error("Could not load index buffer data for indicator");

    glBindVertexArray(0);
}

void update_vertex_buffer_data_for_indicator(Indicator* indicator)
{
    assert(indicator);
    assert(indicator->vertex_buffer);
    assert(indicator->vertex_array_object_id > 0);
    assert(indicator->vertex_buffer_id > 0);

    glBindVertexArray(indicator->vertex_array_object_id);
    abort_on_GL_error("Could not bind VAO for indicator");

    glBindBuffer(GL_ARRAY_BUFFER, indicator->vertex_buffer_id);
    glBufferSubData(GL_ARRAY_BUFFER, 0, (GLsizeiptr)indicator->vertex_buffer_size, (GLvoid*)indicator->vertex_buffer);
    abort_on_GL_error("Could not update vertex buffer data for indicator");

    glBindVertexArray(0);
}

void update_position_buffer_data_for_indicator(Indicator* indicator)
{
    assert(indicator);
    assert(indicator->vertex_buffer);
    assert(indicator->vertex_array_object_id > 0);
    assert(indicator->vertex_buffer_id > 0);

    glBindVertexArray(indicator->vertex_array_object_id);
    abort_on_GL_error("Could not bind VAO for indicator");

    glBindBuffer(GL_ARRAY_BUFFER, indicator->vertex_buffer_id);
    glBufferSubData(GL_ARRAY_BUFFER, 0, (GLsizeiptr)indicator->position_buffer_size, (GLvoid*)indicator->vertices.positions);
    abort_on_GL_error("Could not update position buffer data for indicator");

    glBindVertexArray(0);
}

void update_color_buffer_data_for_indicator(Indicator* indicator)
{
    assert(indicator);
    assert(indicator->vertex_buffer);
    assert(indicator->vertex_array_object_id > 0);
    assert(indicator->vertex_buffer_id > 0);

    glBindVertexArray(indicator->vertex_array_object_id);
    abort_on_GL_error("Could not bind VAO for indicator");

    glBindBuffer(GL_ARRAY_BUFFER, indicator->vertex_buffer_id);
    glBufferSubData(GL_ARRAY_BUFFER, (GLintptr)indicator->position_buffer_size, (GLsizeiptr)indicator->color_buffer_size, (GLvoid*)indicator->vertices.colors);
    abort_on_GL_error("Could not update color buffer data for indicator");

    glBindVertexArray(0);
}

void destroy_indicator(const char* name)
{
    Indicator* const indicator = get_indicator(name);

    DynamicString texture_name_copy = create_duplicate_string(&indicator->name);

    destroy_vertex_array_object_for_indicator(indicator);
    clear_indicator(indicator);
    clear_string(&indicator->name);

    remove_map_item(&indicators, texture_name_copy.chars);

    clear_string(&texture_name_copy);
}

void cleanup_indicators(void)
{
    for (reset_map_iterator(&indicators); valid_map_iterator(&indicators); advance_map_iterator(&indicators))
    {
        Indicator* const indicator = get_indicator(get_current_map_key(&indicators));
        destroy_vertex_array_object_for_indicator(indicator);
        clear_indicator(indicator);
        clear_string(&indicator->name);
    }

    destroy_map(&indicators);

    active_shader_program = NULL;
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

static void initialize_vertex_array_object_for_indicator(Indicator* indicator)
{
    assert(indicator);

    // Generate vertex array object for keeping track of vertex attributes
    glGenVertexArrays(1, &indicator->vertex_array_object_id);
    abort_on_GL_error("Could not generate VAO for indicator");

    glBindVertexArray(indicator->vertex_array_object_id);
    abort_on_GL_error("Could not bind VAO for indicator");

    // Generate buffer object for vertices
    glGenBuffers(1, &indicator->vertex_buffer_id);
    abort_on_GL_error("Could not generate vertex buffer object for indicator");

    // Generate buffer object for indices
    glGenBuffers(1, &indicator->index_buffer_id);
    abort_on_GL_error("Could not generate index buffer object for indicator");

    glBindVertexArray(0);
}

static void allocate_indicator_buffers(Indicator* indicator, size_t n_vertices, size_t n_indices)
{
    assert(indicator);

    if (indicator->vertex_buffer || indicator->index_buffer)
        print_severe_message("Cannot allocate indicator buffers because they are already allocated.");

    indicator->n_vertices = n_vertices;
    indicator->n_indices = n_indices;

    indicator->position_buffer_size = n_vertices*sizeof(Vector4f);
    indicator->color_buffer_size = n_vertices*sizeof(Color);
    indicator->vertex_buffer_size = indicator->position_buffer_size + indicator->color_buffer_size;
    indicator->index_buffer_size = n_indices*sizeof(unsigned int);

    indicator->vertex_buffer = malloc(indicator->vertex_buffer_size);
    if (!indicator->vertex_buffer)
        print_severe_message("Could not allocate memory for indicator vertices.");

    indicator->index_buffer = (unsigned int*)malloc(indicator->index_buffer_size);
    if (!indicator->index_buffer)
        print_severe_message("Could not allocate memory for indicator indices.");

    indicator->vertices.positions = (Vector4f*)indicator->vertex_buffer;
    indicator->vertices.colors = (Color*)(indicator->vertices.positions + n_vertices);
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

    abort_on_GL_error("Could not destroy buffer objects for indicator");
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

    indicator->vertex_buffer = NULL;
    indicator->n_vertices = 0;
    indicator->position_buffer_size = 0;
    indicator->color_buffer_size = 0;
    indicator->vertex_buffer_size = 0;

    indicator->index_buffer = NULL;
    indicator->n_indices = 0;
    indicator->index_buffer_size = 0;
}
