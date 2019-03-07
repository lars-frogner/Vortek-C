#ifndef TRANSFER_FUNCTIONS_H
#define TRANSFER_FUNCTIONS_H

#include "shaders.h"

enum transfer_function_component {TF_RED = 0, TF_GREEN = 1, TF_BLUE = 2, TF_ALPHA = 3};

void initialize_transfer_functions(void);

void print_transfer_function(const char* name, enum transfer_function_component component);

const char* create_transfer_function(ShaderProgram* shader_program);

void add_piecewise_linear_transfer_function_node(const char* name, enum transfer_function_component component,
                                                 float texture_coordinate, float value);

void remove_piecewise_linear_transfer_function_node(const char* name, enum transfer_function_component component,
                                                    float texture_coordinate);

void set_logarithmic_transfer_function(const char* name, enum transfer_function_component component,
                                       float start_texture_coordinate, float end_texture_coordinate,
                                       float start_value, float end_value);

void remove_transfer_function(const char* name);

void sync_transfer_functions(void);

void cleanup_transfer_functions(void);

#endif
