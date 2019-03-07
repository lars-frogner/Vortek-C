#ifndef FIELDS_H
#define FIELDS_H

#include <stddef.h>

enum field_type {NULL_FIELD, SCALAR_FIELD, VECTOR_FIELD};

typedef struct Field
{
    float* data;
    enum field_type type;
    size_t size_x;
    size_t size_y;
    size_t size_z;
    float extent_x;
    float extent_y;
    float extent_z;
    float min_value;
    float max_value;
    float lower_clip_value;
    float upper_clip_value;
} Field;

void initialize_fields(void);

Field* create_field_from_bifrost_file(const char* name, const char* data_filename, const char* header_filename);

Field* get_field(const char* name);

void destroy_field(const char* name);
void cleanup_fields(void);

void clip_field_values(const char* name, float lower_clip_value, float upper_clip_value);

float field_value_to_normalized_value(const char* name, float field_value);
float normalized_value_to_field_value(const char* name, float normalized_value);

#endif
