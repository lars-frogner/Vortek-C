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

Field read_bifrost_field(const char* data_filename, const char* header_filename);

Field create_field(enum field_type type, float* data,
                   size_t size_x, size_t size_y, size_t size_z,
                   float extent_x, float extent_y, float extent_z);

void reset_field(Field* field);

void clip_field_values(Field* field, float lower_clip_value, float upper_clip_value);

float field_value_to_normalized_value(const Field* field, float field_value);
float normalized_value_to_field_value(const Field* field, float normalized_value);

#endif
