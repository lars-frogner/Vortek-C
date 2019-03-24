#ifndef SHADER_GENERATOR_H
#define SHADER_GENERATOR_H

#include "dynamic_string.h"
#include "linked_list.h"
#include "hash_map.h"

#include <stddef.h>

typedef struct ShaderSource
{
    DynamicString code;
    HashMap global_variable_expressions;
    LinkedList variables;
    LinkedList deleted_variables;
    LinkedList output_variables;
    size_t next_undeleted_unused_variable_number;
} ShaderSource;

void initialize_shader_source(ShaderSource* source);

void add_input_in_shader(ShaderSource* source, const char* type, const char* name);
void add_vertex_input_in_shader(ShaderSource* source, const char* type, const char* name, unsigned int layout_location);
void add_uniform_in_shader(ShaderSource* source, const char* type, const char* name);
void add_array_uniform_in_shader(ShaderSource* source, const char* type, const char* name, size_t length);

void add_clip_distance_output_in_shader(ShaderSource* source, unsigned int max_clip_distances);

size_t transform_input_in_shader(ShaderSource* source, const char* matrix_name, const char* input_name);

void add_field_texture_in_shader(ShaderSource* source, const char* texture_name);
size_t apply_scalar_field_texture_sampling_in_shader(ShaderSource* source, const char* texture_name, const char* texture_coordinates_name);

void add_transfer_function_in_shader(ShaderSource* source, const char* transfer_function_name);
size_t apply_transfer_function_in_shader(ShaderSource* source, const char* transfer_function_name, size_t input_variable_number);

void add_output_snippet_in_shader(ShaderSource* source, const char* snippet,
                                  LinkedList* global_dependencies, LinkedList* variable_dependencies);
size_t add_variable_snippet_in_shader(ShaderSource* source, const char* output_type, const char* output_name,
                                      const char* snippet, LinkedList* global_dependencies, LinkedList* variable_dependencies);

void assign_variable_to_output_in_shader(ShaderSource* source, size_t variable_number, const char* output_name);
void assign_transformed_input_to_output_in_shader(ShaderSource* source, const char* matrix_name, const char* input_name, const char* output_name);
void assign_transformed_variable_to_output_in_shader(ShaderSource* source, const char* matrix_name, size_t variable_number, const char* output_name);
void assign_variable_to_new_output_in_shader(ShaderSource* source, const char* type, size_t variable_number, const char* output_name);
void assign_input_to_new_output_in_shader(ShaderSource* source, const char* type, const char* input_name, const char* output_name);

const char* generate_shader_code(ShaderSource* source);

void remove_variable_in_shader(ShaderSource* source, size_t variable_number);
void destroy_shader_source(ShaderSource* source);

#endif
