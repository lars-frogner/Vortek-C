#include "fields.h"

#include "error.h"
#include "io.h"
#include "hash_map.h"

#include <stdlib.h>
#include <math.h>
#include <float.h>


static Field* create_field(const char* name,
                           enum field_type type, float* data,
                           size_t size_x, size_t size_y, size_t size_z,
                           float physical_extent_x, float physical_extent_y, float physical_extent_z);
static size_t get_field_array_length(const Field* field);
static void swap_x_and_z_axes(const float* input_array, size_t size_x, size_t size_y, size_t size_z, float* output_array);
static void find_float_array_limits(const float* array, size_t length, float* min_value, float* max_value);
static void scale_float_array(float* array, size_t length, float zero_value, float unity_value);
static void clear_field(Field* field);


static HashMap fields;


void initialize_fields(void)
{
    fields = create_map();
}

Field* create_field_from_bifrost_file(const char* name, const char* data_filename, const char* header_filename)
{
    check(name);
    check(data_filename);
    check(header_filename);

    char* header = read_text_file(header_filename);

    if (!header)
        print_severe_message("Could not read header file.");

    const char element_kind = find_char_entry_in_header(header, "element_kind", ":");
    const int element_size = find_int_entry_in_header(header, "element_size", ":");
    const char endianness = find_char_entry_in_header(header, "endianness", ":");
    const int dimensions = find_int_entry_in_header(header, "dimensions", ":");
    const char order = find_char_entry_in_header(header, "order", ":");
    const int signed_size_x = find_int_entry_in_header(header, "x_size", ":");
    const int signed_size_y = find_int_entry_in_header(header, "y_size", ":");
    const int signed_size_z = find_int_entry_in_header(header, "z_size", ":");
    const float dx = find_float_entry_in_header(header, "dx", ":");
    const float dy = find_float_entry_in_header(header, "dy", ":");
    const float dz = find_float_entry_in_header(header, "dz", ":");

    free(header);

    if (element_kind == 0 || element_size == 0 || dimensions == 0 ||
        signed_size_x == 0 || signed_size_y == 0 || signed_size_z == 0 ||
        dx == 0 || dy == 0 || dz == 0)
        print_severe_message("Could not determine all required header entries.");

    if (element_kind != 'f')
        print_severe_message("Field data must be floating-point.");

    if (element_size != 4)
        print_severe_message("Field data must have 4-byte precision.");

    if (is_little_endian())
    {
        if (endianness != 'l')
            print_severe_message("Field data must be little-endian.");
    }
    else
    {
        if (endianness != 'b')
            print_severe_message("Field data must be big-endian.");
    }

    if (dimensions != 3)
        print_severe_message("Field data must be 3D.");

    if (order != 'C')
        print_severe_message("Field data must laid out row-major order.");

    if (signed_size_x < 2 || signed_size_y < 2 || signed_size_z < 2)
        print_severe_message("Field dimensions cannot smaller than 2 along any axis.");

    const size_t size_x = (size_t)signed_size_x;
    const size_t size_y = (size_t)signed_size_y;
    const size_t size_z = (size_t)signed_size_z;

    const size_t length = size_x*size_y*size_z;

    float* data = (float*)read_binary_file(data_filename, length, sizeof(float));

    const float physical_extent_x = (size_x - 1)*dx;
    const float physical_extent_y = (size_y - 1)*dy;
    const float physical_extent_z = (size_z - 1)*dz;

    return create_field(name, SCALAR_FIELD, data,
                        size_x, size_y, size_z,
                        physical_extent_x, physical_extent_y, physical_extent_z);
}

Field* get_field(const char* name)
{
    check(name);

    MapItem item = get_map_item(&fields, name);
    assert(item.size == sizeof(Field));
    Field* const field = (Field*)item.data;
    check(field);

    return field;
}

void destroy_field(const char* name)
{
    Field* const field = get_field(name);
    clear_field(field);
    remove_map_item(&fields, name);
}

void cleanup_fields(void)
{
    for (reset_map_iterator(&fields); valid_map_iterator(&fields); advance_map_iterator(&fields))
    {
        Field* const field = get_field(get_current_map_key(&fields));
        clear_field(field);
    }

    destroy_map(&fields);
}

static Field* create_field(const char* name,
                           enum field_type type, float* data,
                           size_t size_x, size_t size_y, size_t size_z,
                           float physical_extent_x, float physical_extent_y, float physical_extent_z)
{
    check(name);
    check(data);

    MapItem item = insert_new_map_item(&fields, name, sizeof(Field));
    Field* const field = (Field*)item.data;

    field->data = data;
    field->type = type;
    field->size_x = size_x;
    field->size_y = size_y;
    field->size_z = size_z;

    const float max_physical_extent = fmaxf(physical_extent_x, fmaxf(physical_extent_y, physical_extent_z));
    const float spatial_normalization = 1.0f/max_physical_extent;

    field->halfwidth = spatial_normalization*physical_extent_x;
    field->halfheight = spatial_normalization*physical_extent_y;
    field->halfdepth = spatial_normalization*physical_extent_z;

    field->voxel_width = 2*field->halfwidth/size_x;
    field->voxel_height = 2*field->halfheight/size_y;
    field->voxel_depth = 2*field->halfdepth/size_z;

    field->physical_extent_scale = 0.5f*max_physical_extent;

    const size_t length = get_field_array_length(field);

    find_float_array_limits(data, length, &field->min_value, &field->max_value);

    scale_float_array(data, length, field->min_value, field->max_value);

    return field;
}

static size_t get_field_array_length(const Field* field)
{
    return field->size_x*field->size_y*field->size_z*((field->type == VECTOR_FIELD) ? 3 : 1);
}

static void swap_x_and_z_axes(const float* input_array, size_t size_x, size_t size_y, size_t size_z, float* output_array)
{
    assert(input_array);
    assert(output_array);
    assert(output_array != input_array);

    size_t i, j, k;
    size_t input_idx, output_idx;

    for (i = 0; i < size_x; i++)
        for (j = 0; j < size_y; j++)
            for (k = 0; k < size_z; k++)
            {
                input_idx = (i*size_y + j)*size_z + k;
                output_idx = (k*size_y + j)*size_x + i;

                output_array[output_idx] = input_array[input_idx];
            }
}

static void find_float_array_limits(const float* array, size_t length, float* min_value, float* max_value)
{
    assert(array);
    assert(length > 0);
    assert(min_value);
    assert(max_value);

    size_t i;
    float value;
    float min = FLT_MAX;
    float max = -FLT_MAX;

    for (i = 0; i < length; i++)
    {
        value = array[i];
        if (value < min)
            min = value;
        if (value > max)
            max = value;
    }

    *min_value = min;
    *max_value = max;
}

static void scale_float_array(float* array, size_t length, float zero_value, float unity_value)
{
    assert(array);
    assert(length > 0);

    if (zero_value >= unity_value)
    {
        print_warning_message("Can only scale array with unity value larger than zero value.");
        return;
    }

    size_t i;
    const float scale = 1.0f/(unity_value - zero_value);

    for (i = 0; i < length; i++)
        array[i] = (array[i] - zero_value)*scale;
}

static void clear_field(Field* field)
{
    check(field);

    if (field->data)
        free(field->data);

    field->data = NULL;
    field->type = NULL_FIELD;
    field->size_x = 0;
    field->size_y = 0;
    field->size_z = 0;
    field->halfwidth = 0;
    field->halfheight = 0;
    field->halfdepth = 0;
    field->voxel_width = 0;
    field->voxel_height = 0;
    field->voxel_depth = 0;
    field->physical_extent_scale = 0;
    field->min_value = 0;
    field->max_value = 0;
}
