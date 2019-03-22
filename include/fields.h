#ifndef FIELDS_H
#define FIELDS_H

#include <stddef.h>

enum field_type {NULL_FIELD = 0, SCALAR_FIELD = 1, VECTOR_FIELD = 2};

typedef struct Field
{
    float* data;
    enum field_type type;
    size_t size_x;
    size_t size_y;
    size_t size_z;
    float halfwidth;
    float halfheight;
    float halfdepth;
    float voxel_width;
    float voxel_height;
    float voxel_depth;
    float physical_extent_scale;
    float min_value;
    float max_value;
} Field;

void initialize_fields(void);

Field* create_field_from_bifrost_file(const char* name, const char* data_filename, const char* header_filename);

Field* get_field(const char* name);

void destroy_field(const char* name);
void cleanup_fields(void);

#endif
