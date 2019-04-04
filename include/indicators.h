#ifndef INDICATORS_H
#define INDICATORS_H

#include "gl_includes.h"
#include "geometry.h"
#include "colors.h"
#include "dynamic_string.h"
#include "shaders.h"

typedef struct IndicatorVertices
{
    Vector4f* positions;
    Color* colors;
} IndicatorVertices;

typedef struct Indicator
{
    DynamicString name;
    IndicatorVertices vertices;
    void* vertex_buffer;
    unsigned int* index_buffer;
    size_t n_vertices;
    size_t n_indices;
    size_t position_buffer_size;
    size_t color_buffer_size;
    size_t vertex_buffer_size;
    size_t index_buffer_size;
    GLuint vertex_array_object_id;
    GLuint vertex_buffer_id;
    GLuint index_buffer_id;
} Indicator;

enum indicator_drawing_pass {INDICATOR_BACK_PASS, INDICATOR_FRONT_PASS};

void set_active_shader_program_for_indicators(ShaderProgram* shader_program);

void initialize_indicators(void);

Indicator* create_indicator(const DynamicString* name, size_t n_vertices, size_t n_indices);
Indicator* get_indicator(const char* name);

GLuint get_active_indicator_shader_program_id(void);

void set_vertex_colors_for_indicator(Indicator* indicator, size_t start_vertex_idx, size_t n_vertices, const Color* color);

void set_cube_vertex_positions_for_indicator(Indicator* indicator, size_t* running_vertex_idx, const Vector3f* lower_corner, const Vector3f* extent);
void set_cube_edges_for_indicator(Indicator* indicator, size_t start_vertex_idx, size_t* running_index_idx);

void load_buffer_data_for_indicator(Indicator* indicator);

void update_vertex_buffer_data_for_indicator(Indicator* indicator);
void update_position_buffer_data_for_indicator(Indicator* indicator);
void update_color_buffer_data_for_indicator(Indicator* indicator);

void destroy_indicator(const char* name);
void cleanup_indicators(void);

#endif
