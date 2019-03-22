#ifndef TRANSFER_FUNCTIONS_H
#define TRANSFER_FUNCTIONS_H

#include "shaders.h"
#include "bricks.h"

#define TF_START_NODE 1
#define TF_END_NODE 254

void set_active_shader_program_for_transfer_functions(ShaderProgram* shader_program);

enum transfer_function_component {TF_RED = 0, TF_GREEN = 1, TF_BLUE = 2, TF_ALPHA = 3};

void initialize_transfer_functions(void);

const char* create_transfer_function(void);

void load_transfer_functions(void);
void sync_transfer_functions(void);

void print_transfer_function(const char* name, enum transfer_function_component component);

void set_piecewise_linear_transfer_function_node(const char* name, enum transfer_function_component component,
                                                 unsigned int node, float value);
void remove_piecewise_linear_transfer_function_node(const char* name, enum transfer_function_component component,
                                                    unsigned int node);

void set_logarithmic_transfer_function(const char* name, enum transfer_function_component component,
                                       float start_value, float end_value);

void set_transfer_function_lower_limit(const char* name, float lower_value);
void set_transfer_function_upper_limit(const char* name, float upper_value);

void set_transfer_function_lower_node(const char* name, enum transfer_function_component component, float value);
void set_transfer_function_upper_node(const char* name, enum transfer_function_component component, float value);

void update_visibility_ratios(const char* transfer_function_name, BrickedField* bricked_field);

unsigned int texture_coordinate_to_nearest_transfer_function_node(float texture_coordinate);
float transfer_function_node_to_texture_coordinate(unsigned int node);

void remove_transfer_function(const char* name);
void cleanup_transfer_functions(void);

#endif
