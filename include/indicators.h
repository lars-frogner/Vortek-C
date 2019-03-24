#ifndef INDICATORS_H
#define INDICATORS_H

#include "geometry.h"
#include "shaders.h"

typedef struct IndicatorVertices
{
    Vector4f* positions;
    Vector4f* colors;
    unsigned int n_vertices;
} IndicatorVertices;

void set_active_shader_program_for_indicators(ShaderProgram* shader_program);

void initialize_indicators(void);

IndicatorVertices* access_edge_indicator_vertices(const char* name);

const char* add_cube_edge_indicator(const Vector3f* lower_corner, const Vector3f* extent, const Vector3f* color, const char* name, ...);
void set_cube_edge_indicator_vertex_positions(const char* name, const Vector3f* lower_corner, const Vector3f* extent);

void set_edge_indicator_constant_color(const char* name, const Vector3f* color);

void sync_edge_indicator(const char* name);

void draw_edge_indicator(const char* name);

void destroy_edge_indicator(const char* name);
void cleanup_indicators(void);

#endif
