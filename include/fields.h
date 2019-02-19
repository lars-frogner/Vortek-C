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
    float dx;
    float dy;
    float dz;
    float min_value;
    float max_value;

} Field;

Field read_bifrost_field(const char* data_filename, const char* header_filename);

Field create_field(enum field_type type, float* data,
                   size_t size_x, size_t size_y, size_t size_z,
                   float dx, float dy, float dz);

void reset_field(Field* field);

#endif
