#ifndef IO_H
#define IO_H

#include <stddef.h>

typedef struct FloatField
{
    float* data;
    size_t size_x;
    size_t size_y;
    size_t size_z;
    float min_value;
    float max_value;

} FloatField;

char* read_text_file(const char* filename);

FloatField read_bifrost_field(const char* data_filename, const char* header_filename);

#endif
